#include <iostream>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

#include "pinfo.h"

using namespace std;

void call_pinfo(vector<char*> args, const char* shellRootDir){
    pid_t pid;
    if (args.size() > 1 && args[1] != nullptr)
         pid = atoi(args[1]);
    else pid = getpid();

    string infoPath = "/proc/" + to_string(pid) + "/stat";
    string exePath = "/proc/" + to_string(pid) + "/exe";

    int fd = open(infoPath.c_str(), O_RDONLY);
    if (fd == -1) {
        cerr << "Process: " << pid << " Not found (" << strerror(errno) << ")\n";
        return;
    }

    char buffer[4096];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytesRead <= 0) {
        cerr << "Failed to read stat file\n";
        return;
    }

    buffer[bytesRead] = '\0';
    
    char* closeParen = strrchr(buffer, ')');
    if (!closeParen) {
        cerr << "Failed to parse stat file\n";
        return;
    }

    string processState;
    unsigned long virtualMemory = 0;
    pid_t pgrp = -1;
    pid_t tpgid = -1;

    char* token = strtok(closeParen + 1, " ");
    int fieldNumber = 3;

    while (token != nullptr) {
        if (fieldNumber == 3) 
            processState = token;
        else if (fieldNumber == 5)
            pgrp = atoi(token);
        else if (fieldNumber == 8)
            tpgid = atoi(token);
        else if (fieldNumber == 23) 
            virtualMemory = strtoul(token, nullptr, 10);

        fieldNumber++;
        token = strtok(nullptr, " ");
    }

    if (pgrp > 0 && tpgid > 0 && pgrp == tpgid) {
        processState += "+";
    }

    char exeRealPath[2048];
    ssize_t len = readlink(exePath.c_str(), exeRealPath, sizeof(exeRealPath)-1);
    string executablePath;

    if (len != -1) {
        exeRealPath[len] = '\0';
        executablePath = exeRealPath;

        string homeDir = shellRootDir;
        if (executablePath == homeDir) {
            executablePath = "~";
        } else if (executablePath.rfind(homeDir + "/", 0) == 0) {
            executablePath = "~" + executablePath.substr(homeDir.length());
        }
    } else {
        executablePath = "Executable path not found";
    }

    cout << "pid -- " << pid << "\n";
    cout << "Process Status -- " << processState << "\n";
    cout << "memory -- " << virtualMemory << " {Virtual Memory}\n";
    cout << "Executable Path -- " << executablePath << "\n";
}