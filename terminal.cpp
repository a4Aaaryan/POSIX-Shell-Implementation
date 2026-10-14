#include <unistd.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "ls.h"
#include "search_.h"
#include "others.h"
#include "pinfo.h"
#include "history_.h"

using namespace std;

static void sigintHandler(int sig) {
    (void)sig;
    cout << "\n";
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}

struct BgProcess {
    pid_t pid;
    string cmd;
};
static vector<BgProcess> bgProcesses;

void checkBackgroundProcesses() {
    int status;
    for (auto it = bgProcesses.begin(); it != bgProcesses.end(); ) {
        pid_t res = waitpid(it->pid, &status, WNOHANG);
        if (res > 0) {
            if (WIFEXITED(status)) {
                cout << "\n" << it->cmd << " with pid " << it->pid << " exited normally\n";
            } else if (WIFSIGNALED(status)) {
                cout << "\n" << it->cmd << " with pid " << it->pid << " terminated by signal " << WTERMSIG(status) << "\n";
            } else {
                cout << "\n" << it->cmd << " with pid " << it->pid << " exited\n";
            }
            it = bgProcesses.erase(it);
        } else {
            ++it;
        }
    }
}

bool argsForRedirec(vector<char*>& args, string &inputFile, string &outputFile, string &redirOp){
    inputFile = "";
    outputFile = "";
    redirOp = "";

    vector<char*> argsWitoutOp;

    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == nullptr)
            break;
        
        if (strcmp(args[i], "<") == 0)
        {
            if (i + 1 >= args.size() || args[i + 1] == nullptr) {
                cerr << "syntax error near unexpected token 'newline'\n";
                return false;
            }
            inputFile = args[i + 1];
            i++;
            continue;
        }
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0)
        {
            if (i + 1 >= args.size() || args[i + 1] == nullptr) {
                cerr << "syntax error near unexpected token 'newline'\n";
                return false;
            }
            outputFile = args[i + 1];
            redirOp = args[i];
            i++;
            continue;
        }
        
        argsWitoutOp.push_back(args[i]);
    }
    argsWitoutOp.push_back(nullptr);
    args = argsWitoutOp;
    return true;
}

int ioRedirec(const string &inputFile, const string &outputFile, const string &redirType) {
    if (!inputFile.empty()) {
        int fd = open(inputFile.c_str(), O_RDONLY);
        if (fd < 0) {
            perror("Input file doesn't exist");
            return -1;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (!outputFile.empty()) {
        int fd;
        if (redirType == ">>")
            fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        else 
            fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd < 0) {
            perror("Output file couldn't be created");
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    return 0;
}

string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n");
    return s.substr(start, end - start + 1);
}

void executeCommand(vector<char*> &args, const char* shellRootDir) {
    if (args.empty() || args[0] == nullptr) exit(0);

    if (strcmp(args[0], "pwd") == 0) {
        char cur[2048];
        if (getcwd(cur, sizeof(cur))) {
            printf("%s\n", cur);
        } else {
            perror("pwd");
        }
    } else if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; args[i] != nullptr; i++) {
            cout << args[i];
            if (args[i + 1] != nullptr) cout << " ";
        }
        cout << "\n";
    } else if (strcmp(args[0], "ls") == 0) {
        call_ls(args, shellRootDir);
    } else if (strcmp(args[0], "pinfo") == 0) {
        call_pinfo(args, shellRootDir);
    } else if (strcmp(args[0], "search") == 0) {
        call_search(args);
    } else if (strcmp(args[0], "history") == 0) {
        printHistory(args);
    } else {
        callOtherJobs(args);
    }
    exit(0);
}

int main(){
    char shellRootDir[1024];
    if (!getcwd(shellRootDir, sizeof(shellRootDir))) {
        strcpy(shellRootDir, ".");
    }

    char prevDir[1024];
    strcpy(prevDir, shellRootDir);

    string historyFilePath = string(shellRootDir) + "/.my_shell_history";

    using_history();               
    stifle_history(20);
    read_history(historyFilePath.c_str()); 

    signal(SIGINT, sigintHandler);
    signal(SIGTSTP, SIG_IGN);

    char hostName[300];
    gethostname(hostName, sizeof(hostName));

    char* userName = getlogin();
    if (userName == nullptr) userName = getenv("USER");
    if (userName == nullptr) userName = (char*)"User";

    while (true)
    {
        checkBackgroundProcesses();

        char cwd[2048];
        if (!getcwd(cwd, sizeof(cwd))) {
            strcpy(cwd, "");
        }

        string homeDir = shellRootDir;
        string displayPath = cwd;
        if (displayPath == homeDir) {
            displayPath = "~";
        } else if (displayPath.rfind(homeDir + "/", 0) == 0) {
            displayPath = "~" + displayPath.substr(homeDir.length());
        }

        string prompt = string(userName) + "@" + hostName + ":" + displayPath + "> ";
        char* cmdLine_ = readline(prompt.c_str()); 
        if (!cmdLine_) break;          
        string cmdLine = cmdLine_;
        free(cmdLine_);                 

        string trimmedCmd = trim(cmdLine);
        if (!trimmedCmd.empty()) {
            add_history(cmdLine.c_str());       
            write_history(historyFilePath.c_str()); 
        } else {
            continue;
        }

        vector<string> cmds;
        char* raw = &cmdLine[0];
        char* token = strtok(raw, ";");

        while (token != nullptr) {
            string t = trim(token);
            if (!t.empty()) cmds.push_back(t);
            token = strtok(nullptr, ";");
        }

        for (auto &cmd: cmds)
        {
            vector<string> pipelineCmds;
            char* rawPipe = &cmd[0];
            char* pipeToken = strtok(rawPipe, "|");

            while (pipeToken != nullptr) {
                string t = trim(pipeToken);
                if (!t.empty()) pipelineCmds.push_back(t);
                pipeToken = strtok(nullptr, "|");
            }
            
            int n = pipelineCmds.size();
            if (n == 0)
                continue;
            
            if (n == 1)
            {
                vector<char*> args;
                string trimmed = trim(pipelineCmds[0]);
                char* raw_ = &trimmed[0];
                char* arg = strtok(raw_, " \t\n");

                while (arg != nullptr) {
                    args.push_back(arg);
                    arg = strtok(nullptr, " \t\n");
                }

                if (args.empty() || args[0] == nullptr)
                    continue;

                if (strcmp(args[0], "exit") == 0) 
                    return 0;

                if (strcmp(args[0], "cd") == 0)
                {
                    if (args.size() > 2 && args[2] != nullptr)
                    {
                        cerr << "cd: too many arguments\n";
                        continue;
                    }
                    
                    char currentDir[1024];
                    if (!getcwd(currentDir, sizeof(currentDir))) {
                        strcpy(currentDir, "");
                    }

                    if (args.size() == 1 || args[1] == nullptr || strcmp(args[1], "~") == 0)
                    {
                        if (chdir(shellRootDir) < 0) {
                            perror("chdir");
                        } else {
                            strcpy(prevDir, currentDir);
                        }
                    }
                    else if (strcmp(args[1], ".") == 0)
                    {
                        strcpy(prevDir, currentDir);
                    }
                    else if (strcmp(args[1], "..") == 0)
                    {
                        if (chdir("..") < 0) {
                            perror("chdir");
                        } else {
                            strcpy(prevDir, currentDir);
                        }
                    }
                    else if (strcmp(args[1], "-") == 0)
                    {
                        if (chdir(prevDir) < 0) {
                            perror("chdir");
                        } else {
                            cout << prevDir << endl;
                            strcpy(prevDir, currentDir);
                        }
                    }
                    else
                    {
                        string target = args[1];
                        if (target == "~") {
                            target = shellRootDir;
                        } else if (target.rfind("~/", 0) == 0) {
                            target = string(shellRootDir) + target.substr(1);
                        }

                        if (chdir(target.c_str()) < 0) {
                            perror("chdir");
                        } else {
                            strcpy(prevDir, currentDir);
                        }
                    }
                    continue;
                }

                bool bg = false;
                size_t numArgs = 0;
                while (numArgs < args.size() && args[numArgs] != nullptr) numArgs++;

                if (numArgs > 0 && strcmp(args[numArgs - 1], "&") == 0) {
                    bg = true;
                    args[numArgs - 1] = nullptr;
                }

                string inputFile, outputFile, redirOp;
                if (!argsForRedirec(args, inputFile, outputFile, redirOp)) {
                    continue;
                }

                if (args.empty() || args[0] == nullptr)
                    continue;

                string cmdName = args[0];

                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    continue;
                }

                if (pid == 0) {
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);

                    if (bg && inputFile.empty()) {
                        int devNull = open("/dev/null", O_RDONLY);
                        if (devNull >= 0) {
                            dup2(devNull, STDIN_FILENO);
                            close(devNull);
                        }
                    }

                    if (ioRedirec(inputFile, outputFile, redirOp) < 0) {
                        exit(1);
                    }

                    executeCommand(args, shellRootDir);
                } else {
                    if (bg) {
                        cout << "[" << pid << "]\n";
                        bgProcesses.push_back({pid, cmdName});
                    } else {
                        waitpid(pid, nullptr, 0);
                    }
                }
            }
            else
            {
                vector<int> pipefds(2 * (n - 1));
                bool pipeFailed = false;
                for (int i = 0; i < n - 1; i++) {
                    if (pipe(&pipefds[2 * i]) < 0) {
                        perror("pipe");
                        pipeFailed = true;
                        break;
                    }
                }
                if (pipeFailed) {
                    for (int fd : pipefds) {
                        if (fd > 0) close(fd);
                    }
                    continue;
                }

                vector<pid_t> pids;
                bool pipelineError = false;

                for (int i = 0; i < n; i++) {
                    vector<char*> args;
                    string trimmed = trim(pipelineCmds[i]);
                    char* raw_ = &trimmed[0];
                    char* arg = strtok(raw_, " \t\n");

                    while (arg != nullptr) {
                        args.push_back(arg);
                        arg = strtok(nullptr, " \t\n");
                    }

                    if (args.empty() || args[0] == nullptr) {
                        cerr << "syntax error near unexpected token '|'\n";
                        pipelineError = true;
                        break;
                    }

                    if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "exit") == 0) {
                        cerr << "Error: '" << args[0] << "' cannot be used in a pipeline\n";
                        pipelineError = true;
                        break;
                    }

                    string inputFile, outputFile, redirOp;
                    if (!argsForRedirec(args, inputFile, outputFile, redirOp)) {
                        pipelineError = true;
                        break;
                    }

                    if (args.empty() || args[0] == nullptr) {
                        pipelineError = true;
                        break;
                    }

                    pid_t pid = fork();
                    if (pid < 0) {
                        perror("fork");
                        pipelineError = true;
                        break;
                    }

                    if (pid == 0) {
                        signal(SIGINT, SIG_DFL);
                        signal(SIGTSTP, SIG_DFL);

                        if (i > 0) {
                            dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
                        }
                        if (i < n - 1) {
                            dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
                        }

                        for (int fd : pipefds) {
                            close(fd);
                        }

                        if (ioRedirec(inputFile, outputFile, redirOp) < 0) {
                            exit(1);
                        }

                        executeCommand(args, shellRootDir);
                    } else {
                        pids.push_back(pid);
                    }
                }

                for (int fd : pipefds) {
                    close(fd);
                }

                for (pid_t p : pids) {
                    waitpid(p, nullptr, 0);
                }

                if (pipelineError) {
                    // Closed all pipes above
                }
            }
        }      
    }

    return 0;
}