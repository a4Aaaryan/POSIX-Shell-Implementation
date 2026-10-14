#include "ls.h"
#include <iostream>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <dirent.h>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <unistd.h>

using namespace std;

static void printPermissions(mode_t mode) {
    if (S_ISDIR(mode)) cout << "d";
    else if (S_ISLNK(mode)) cout << "l";
    else if (S_ISCHR(mode)) cout << "c";
    else if (S_ISBLK(mode)) cout << "b";
    else if (S_ISFIFO(mode)) cout << "p";
    else if (S_ISSOCK(mode)) cout << "s";
    else cout << "-";

    cout << ((mode & S_IRUSR) ? "r" : "-");
    cout << ((mode & S_IWUSR) ? "w" : "-");
    cout << ((mode & S_IXUSR) ? "x" : "-");

    cout << ((mode & S_IRGRP) ? "r" : "-");
    cout << ((mode & S_IWGRP) ? "w" : "-");
    cout << ((mode & S_IXGRP) ? "x" : "-");

    cout << ((mode & S_IROTH) ? "r" : "-");
    cout << ((mode & S_IWOTH) ? "w" : "-");
    cout << ((mode & S_IXOTH) ? "x" : "-");
}

static void printLongEntry(const string &displayName, const string &fullPath, const struct stat &st) {
    printPermissions(st.st_mode);
    cout << " " << setw(3) << st.st_nlink;

    passwd* pw = getpwuid(st.st_uid);
    group* gr = getgrgid(st.st_gid);

    cout << " " << (pw ? pw->pw_name : to_string(st.st_uid));
    cout << " " << (gr ? gr->gr_name : to_string(st.st_gid));
    cout << " " << setw(8) << st.st_size;

    char timeRec[100];
    strftime(timeRec, sizeof(timeRec), "%b %d %H:%M", localtime(&st.st_mtime));
    cout << " " << timeRec;

    cout << " " << displayName;

    if (S_ISLNK(st.st_mode)) {
        char linkTarget[1024];
        ssize_t len = readlink(fullPath.c_str(), linkTarget, sizeof(linkTarget) - 1);
        if (len != -1) {
            linkTarget[len] = '\0';
            cout << " -> " << linkTarget;
        }
    }
    cout << "\n";
}

static void listTarget(const string &rawTarget, bool a, bool l, const char* shellRootDir) {
    string target = rawTarget;
    if (target == "~") {
        target = shellRootDir;
    } else if (target.rfind("~/", 0) == 0) {
        target = string(shellRootDir) + target.substr(1);
    }

    struct stat st;
    if (lstat(target.c_str(), &st) == -1) {
        perror(("ls: cannot access '" + rawTarget + "'").c_str());
        return;
    }

    if (!S_ISDIR(st.st_mode)) {
        if (l) {
            printLongEntry(rawTarget, target, st);
        } else {
            cout << rawTarget << "\n";
        }
        return;
    }

    DIR* dir = opendir(target.c_str());
    if (!dir) {
        perror(("ls: cannot open directory '" + rawTarget + "'").c_str());
        return;
    }

    vector<string> items;
    dirent* item;
    while ((item = readdir(dir)) != nullptr) {
        string name = item->d_name;
        if (!a && name[0] == '.') continue;
        items.push_back(name);
    }
    closedir(dir);

    sort(items.begin(), items.end());

    if (l) {
        long long total = 0;
        for (const auto& name : items) {
            string fullPath = target + "/" + name;
            struct stat itemSt;
            if (lstat(fullPath.c_str(), &itemSt) == 0) {
                total += itemSt.st_blocks;
            }
        }
        cout << "total " << (total / 2) << "\n";

        for (const auto& name : items) {
            string fullPath = target + "/" + name;
            struct stat itemSt;
            if (lstat(fullPath.c_str(), &itemSt) == 0) {
                printLongEntry(name, fullPath, itemSt);
            }
        }
    } else {
        for (const auto& name : items) {
            cout << name << "  ";
        }
        if (!items.empty()) cout << "\n";
    }
}

void call_ls(vector<char*> &args, const char* shellRootDir){
    bool a = false; 
    bool l = false;  
    vector<string> targets;

    for (size_t i = 1; i < args.size() && (args[i] != nullptr); i++) {
        string arg = args[i];
        if (arg[0] == '-' && arg.length() > 1) {
            for (size_t j = 1; j < arg.length(); j++) {
                if (arg[j] == 'a') a = true;
                else if (arg[j] == 'l') l = true;
                else {
                    cerr << "ls: invalid option -- '" << arg[j] << "'\n";
                    return;
                }
            }
        } else {
            targets.push_back(arg);
        }
    }

    if (targets.empty()) {
        targets.push_back(".");
    }

    sort(targets.begin(), targets.end());

    for (size_t i = 0; i < targets.size(); i++) {
        if (targets.size() > 1) {
            cout << targets[i] << ":\n";
        }
        listTarget(targets[i], a, l, shellRootDir);
        if (targets.size() > 1 && i + 1 < targets.size()) {
            cout << "\n";
        }
    }
}