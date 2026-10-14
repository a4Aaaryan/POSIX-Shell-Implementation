#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include "others.h"

using namespace std;

void callOtherJobs(vector<char*> args) {
    if (args.empty() || args[0] == nullptr) return;

    execvp(args[0], args.data());
    cerr << args[0] << ": command not found\n";
    exit(1);
}

