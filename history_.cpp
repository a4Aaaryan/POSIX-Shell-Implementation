#include <iostream>
#include <readline/history.h>
#include "history_.h"
#include <cstdlib> 

using namespace std;

void printHistory(const vector<char*> &args) {
    int num = -1; // -1 indicates print all history

    if (args.size() > 1 && args[1] != nullptr) { 
        num = atoi(args[1]); 
        if (num < 0) return;
        if (num == 0) return;
    }

    HIST_ENTRY **hist = history_list();
    if (!hist) return;

    int count = 0;
    while (hist[count]) count++;

    int start = 0;
    if (num > 0 && num < count) {
        start = count - num;
    }

    for (int i = start; i < count; i++) {
        if (hist[i] && hist[i]->line) {
            cout << hist[i]->line << endl;
        }
    }
}
