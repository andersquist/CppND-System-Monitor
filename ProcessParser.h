#include <algorithm>
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>
#include <iterator>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "constants.h"


using namespace std;

class ProcessParser{
private:
    static vector<string> split(string line);
    static string getKey(const std::string& key, std::ifstream& stream, const int index = 1, const std::string& default_value = "n/a");
    static string getLine(std::ifstream& stream, const std::string& default_value = "n/a");
    static vector<string> getStatValues(string pid);
    static float getSysActiveCpuTime(std::vector<string>& values);
    static float getSysIdleCpuTime(std::vector<string>& values);
    std::ifstream stream;

public:
    static string getCmd(string pid);
    static vector<string> getPidList();
    static std::string getVmSize(string pid);
    static int getNumberOfCores();
    static std::string getCpuPercent(string pid);
    static long int getSysUpTime();
    static std::string getProcUpTime(string pid);
    static string getProcUser(string pid);
    static vector<string> getSysCpuPercent(string coreNumber = "");
    static float getSysRamPercent();
    static string getSysKernelVersion();
    static int getTotalThreads();
    static int getTotalNumberOfProcesses();
    static int getNumberOfRunningProcesses();
    static string getOSName();
    static std::string PrintCpuStats(std::vector<std::string>& values1, std::vector<std::string>& values2);
    static bool isPidExisting(string pid);
};

vector<string> ProcessParser::split(string line) {
    istringstream buffer(line);
    istream_iterator<string> beg(buffer), end;
    return vector<string>(beg, end);
}

string ProcessParser::getKey(const std::string& key,
        std::ifstream& stream, const int index, const std::string& default_value) {
    string line;
    while (getline(stream, line)) {
        if (line.find(key) == 0) {
            if (index < 0) {
                return line;
            } else {
                return ProcessParser::split(line)[index];
            }
        }
    }
    return default_value;
}

string ProcessParser::getLine(std::ifstream &stream, const std::string& default_value) {
    string line;
    if (getline(stream, line))
        return line;
    return default_value;
}

string ProcessParser::getCmd(string pid) {
    std::ifstream cmd_stream;
    Util::getStream(Path::basePath() + pid + Path::cmdPath(), cmd_stream);
    return ProcessParser::getLine(cmd_stream);
}

vector<string> ProcessParser::getStatValues(string pid) {
    std::ifstream stream;
    Util::getStream(Path::basePath() + pid + Path::statPath(), stream);
    auto line = ProcessParser::getLine(stream);
    return ProcessParser::split(line);
}

float ProcessParser::getSysActiveCpuTime(std::vector<string>& values) {
    return (stof(values[S_USER]) +
            stof(values[S_NICE]) +
            stof(values[S_SYSTEM]) +
            stof(values[S_IRQ]) +
            stof(values[S_SOFTIRQ]) +
            stof(values[S_STEAL]) +
            stof(values[S_GUEST]) +
            stof(values[S_GUEST_NICE]));
}

float ProcessParser::getSysIdleCpuTime(std::vector<string>& values) {
    return (stof(values[S_IDLE]) + stof(values[S_IOWAIT]));
}

vector<string> ProcessParser::getPidList() {
    DIR *dir;
    vector<string> pid_list;

    if (!(dir = opendir(Path::basePath().c_str())))
        throw std::runtime_error(std::strerror(errno));

    while (dirent *dirp = readdir(dir)) {
        if (dirp->d_type != DT_DIR) {
            continue;
        }

        if (all_of(dirp->d_name, dirp->d_name + std::strlen(dirp->d_name),
                [](char c) { return std::isdigit(c); } )) {
            pid_list.emplace_back(dirp->d_name);
        }
    }

    if (closedir(dir))
        throw std::runtime_error(std::strerror(errno));

    return pid_list;
}

string ProcessParser::getVmSize(string pid) {
    std::ifstream stream;
    Util::getStream(Path::basePath() + pid + Path::statusPath(), stream);
    return std::to_string(stof(ProcessParser::getKey("VmSize", stream, 1, "0.0")) / 1024);
}

int ProcessParser::getNumberOfCores() {
    // On my virtual ubuntu machine parsing cpu info reported the incorrect
    // number of cores
    return sysconf(_SC_NPROCESSORS_ONLN);
}

std::string ProcessParser::getCpuPercent(string pid) {
    auto values = ProcessParser::getStatValues(pid);
    float utime = stof(ProcessParser::getProcUpTime(pid));
    float stime = stof(values[14]);
    float cutime = stof(values[15]);
    float cstime = stof(values[16]);
    float starttime = stof(values[21]);
    float uptime = ProcessParser::getSysUpTime();
    float freq = sysconf(_SC_CLK_TCK);
    float total_time = utime + stime + cutime + cstime;
    float seconds = uptime - (starttime / freq);
    float result = 100.0 * ((total_time/freq)/seconds);
    return std::to_string(result);
}

std::string ProcessParser::getProcUpTime(string pid) {
    auto values = ProcessParser::getStatValues(pid);
    return std::to_string(stof(values[13]) / sysconf(_SC_CLK_TCK) );
}

long int ProcessParser::getSysUpTime() {
    std::ifstream uptime_stream;
    Util::getStream(Path::basePath() + Path::upTimePath(), uptime_stream);
    auto values = ProcessParser::split(ProcessParser::getLine(uptime_stream));
    return stoi(values[0]);
}

std::string ProcessParser::getProcUser(string pid) {
    std::ifstream stream;
    Util::getStream(Path::basePath() + pid + Path::statusPath(), stream);
    auto user_id = ProcessParser::getKey("Uid", stream);

    std::ifstream pass_stream;
    std::string line, find_str = "x:" + user_id + ":";
    Util::getStream("/etc/passwd", pass_stream);

    while (getline(pass_stream, line)) {
        auto pos = line.find(find_str);
        if (pos != std::string::npos) {
            return line.substr(0, pos - 1);
        }
    }

    return user_id;
}

std::vector<std::string> ProcessParser::getSysCpuPercent(string coreNumber) {
    std::string key = "cpu" + coreNumber;
    std::ifstream stream;
    Util::getStream(Path::basePath() + Path::statPath(), stream);
    auto values = ProcessParser::split( ProcessParser::getKey(key, stream, -1) );
    return values;
}

std::string ProcessParser::PrintCpuStats(std::vector<std::string>& values1, std::vector<std::string>& values2) {
    float active_time = ProcessParser::getSysActiveCpuTime(values2) - ProcessParser::getSysActiveCpuTime(values1);
    float idle_time = ProcessParser::getSysIdleCpuTime(values2) - ProcessParser::getSysIdleCpuTime(values1);
    float total_time = active_time + idle_time;
    float result = 100.0 * (active_time / total_time);
    return to_string(result);
}

float ProcessParser::getSysRamPercent() {
    std::ifstream stream;
    Util::getStream(Path::basePath() + Path::memInfoPath(), stream);
    float mem_available = stof(ProcessParser::getKey("MemAvailable:", stream));
    stream.clear();
    stream.seekg(0, std::ios::beg);
    float mem_free = stof(ProcessParser::getKey("MemFree:", stream));
    stream.clear();
    stream.seekg(0, std::ios::beg);
    float mem_buffers = stof(ProcessParser::getKey("Buffers:", stream));

    return 100.0 * (1.0 - (mem_free /(mem_available - mem_buffers)));
}

std::string ProcessParser::getSysKernelVersion() {
    std::ifstream stream;
    Util::getStream(Path::basePath() + Path::versionPath(), stream);
    return ProcessParser::split(ProcessParser::getLine(stream))[2];
}

std::string ProcessParser::getOSName() {
    std::ifstream stream;
    std::string line;
    std::string key = "PRETTY_NAME=";
    Util::getStream("/etc/os-release", stream);
    while(getline(stream, line))
    {
        if (line.find(key) == 0)
        {
            auto ret_value = line.substr(key.size());
            ret_value.erase(std::remove(ret_value.begin(), ret_value.end(), '"'), ret_value.end());
            return ret_value;

        }
    }
    return "Unknown";
}

int ProcessParser::getTotalThreads() {
    auto threads = 0;
    auto pid_list = ProcessParser::getPidList();

    for (const auto pid : pid_list)
    {
        std::ifstream stream;
        Util::getStream(Path::basePath() + pid + Path::statusPath(), stream);
        threads += stoi(ProcessParser::getKey("Threads:", stream));
    }
    return threads;
}

int ProcessParser::getTotalNumberOfProcesses() {
    std::ifstream stream;
    Util::getStream(Path::basePath() + Path::statPath(), stream);
    return stoi(ProcessParser::getKey("processes", stream));
}

int ProcessParser::getNumberOfRunningProcesses() {
    std::ifstream stream;
    Util::getStream(Path::basePath() + Path::statPath(), stream);
    return stoi(ProcessParser::getKey("procs_running", stream));
}

bool ProcessParser::isPidExisting(string pid) {
    auto pid_list = ProcessParser::getPidList();

    auto pid_result = std::find(pid_list.begin(), pid_list.end(), pid);

    return pid_result != pid_list.end();
}