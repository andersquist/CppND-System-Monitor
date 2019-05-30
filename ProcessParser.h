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
    static string getKey(const std::string& key, std::ifstream& stream, const int index = 1);
    static string getLine(std::ifstream& stream, const std::string& default_value = "n/a");
    static vector<string> getStatValues(string pid);
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
    static std::string PrintCpuStats(std::vector<std::string> values1, std::vector<std::string>values2);
    static bool isPidExisting(string pid);
};

vector<string> ProcessParser::split(string line) {
    istringstream buffer(line);
    istream_iterator<string> beg(buffer), end;
    return vector<string>(beg, end);
}

string ProcessParser::getKey(const std::string& key, std::ifstream& stream, const int index) {
    string line;
    while (getline(stream, line)) {
        if (line.find(key) == 0)
            return ProcessParser::split(line)[index];
    }
    return "n/a";
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
    return std::to_string(stof(ProcessParser::getKey("VmSize", stream)) / 1024);
}

int ProcessParser::getNumberOfCores() {
    std::ifstream stream;
    Util::getStream(Path::basePath() + Path::cpuInfo(), stream);
    return stoi(ProcessParser::getKey("cpu cores", stream, 3));
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