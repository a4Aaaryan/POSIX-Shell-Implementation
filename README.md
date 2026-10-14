# Custom Linux Shell Assignment

## Overview

This project implements a custom Linux shell in C++ with several features, including:

- Command execution (both built-in and external programs)
- Command history with `up` and `down` arrow navigation
- Input/output redirection (`<`, `>`, `>>`)
- Pipelines using `|`
- Built-in commands: `cd`, `pwd`, `echo`, `exit`, `history`
- Support for multiple commands separated by `;`
- Background process handling
- Custom `pinfo` and `search` commands
- History limited to **20 commands** as per assignment requirement

---

## Files

- **terminal.cpp**: Main shell program
- **history_.cpp / history_.h**: Command history management and printing
- **ls.h / search_.h / others.h / pinfo.h**: Header files for built-in commands
- **Makefile (optional)**: Compile all files

---

## Features

### 1. Command History

- Uses GNU Readline library for input
- Stores history in `.my_shell_history`
- Up to 20 commands stored (`stifle_history(20)`)
- Built-in `history [num]` prints the last `num` commands (all if `num` not provided)

### 2. Input/Output Redirection

- `<` : redirect input from a file
- `>` : redirect output to a file (truncate)
- `>>` : redirect output to a file (append)

### 3. Pipelines

- Supports multiple commands separated by `|`
- Each command in the pipeline executed in a child process
- Output of one command is fed to the next via pipes

### 4. Built-in Commands

- `cd [dir]` : Change directory
- `pwd` : Print current working directory
- `echo [args]` : Print arguments
- `exit` : Exit the shell
- `history [num]` : Print last `num` commands
- `pinfo [pid]` : Display process info
- `search [options]` : Custom search command

### 5. Multiple Commands

- Commands separated by `;` are executed sequentially

---

## Compilation

Make sure **readline library** is installed.

```bash
sudo apt-get install libreadline-dev
