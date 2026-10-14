#include <iostream>
#include <vector>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "search_.h"

using namespace std;

bool dfs(string target, const string &currentDir){

    DIR* dir = opendir(currentDir.c_str());
    if (!dir) {
        return false;
    }

    dirent* item;

    while ((item = readdir(dir)) != nullptr) {
        string name = item->d_name;

        if (name == "." || name == "..") 
            continue; 

        if (name == target){
            closedir(dir);
            return true;
        }

        string nextPath = currentDir + "/" + name;
        struct stat st;
        if (lstat(nextPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            if (dfs(target, nextPath)) {
                closedir(dir);
                return true;
            }
        }
    }

    closedir(dir);
    return false;
}

void call_search(vector<char*> &args){
    if (args.size() < 2 || args[1] == nullptr)
    {
        cerr << "No target for search found.\n";
        return;
    }

    char cwd[2048];
    if (!getcwd(cwd, sizeof(cwd))) {
        perror("getcwd");
        return;
    }
    string target = args[1];

    bool res = dfs(target, cwd);
    cout << (res ? "True\n" : "False\n");
}