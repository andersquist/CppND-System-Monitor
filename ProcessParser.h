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

// TODO: Define all of the above functions below:
vector<string> ProcessParser::split(string line) {
    istringstream buffer(line);
    istream_iterator<string> beg(buffer), end;
    return vector<string>(beg, end);
}

string ProcessParser::getKey(const std::string& key, std::ifstream& stream, const int index) {
    string line;
    while (getline(stream, line)) {
        if (line.find(key) == 0)
            return split(line)[index];
    }
    return "n/a";
}


string ProcessParser::getCmd(string pid) {
    std::ifstream cmd_stream;
    string cmdline;
    Util::getStream(Path::basePath() + pid + Path::cmdPath(), cmd_stream);
    if ( getline(cmd_stream, cmdline) )
        return cmdline;
    return "n/a";
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
    return std::to_string(stof(getKey("VmSize", stream)) / 1024);
}

int ProcessParser::getNumberOfCores() {
    std::ifstream stream;
    Util::getStream(Path::basePath() + Path::cpuInfo(), stream);
    return stoi(getKey("cpu cores", stream, 3));
}