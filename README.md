# Unix Shell Implementation

A custom Unix shell implemented in C++ using POSIX system calls. The project implements command parsing and execution, process management, inter-process communication, I/O redirection, and background execution.

## Overview

This project implements a command-line shell that accepts user commands, parses them into executable components, and launches them as processes.

It uses low-level Unix interfaces to manage processes and file descriptors, providing support for pipelines, input/output redirection, wildcard expansion, and asynchronous command execution.

## Features

- **Command execution:** Executes external programs using `fork()` and `execvp()`.
- **Pipelines:** Connects multiple commands using Unix pipes (`|`).
- **Input redirection:** Redirects standard input using `<`.
- **Output redirection:** Supports output truncation (`>`) and appending (`>>`).
- **Error redirection:** Redirects standard error using `2>`.
- **Combined output and error redirection:** Supports appending both standard output and standard error using `>>&`.
- **Background execution:** Executes commands asynchronously using `&`.
- **Wildcard expansion:** Supports `*` and `?` patterns through the POSIX `glob()` interface.
- **Built-in `cd` command:** Changes the shell's working directory, defaulting to `$HOME` when no directory is specified.
- **Child process management:** Uses `waitpid()` to collect terminated child processes.
- **Termination logging:** Records child process termination information and exit codes in `log.txt`.

## Technical Implementation

The project uses several fundamental Unix system interfaces:

| System call / Interface | Purpose |
|---|---|
| `fork()` | Creates child processes for command execution. |
| `execvp()` | Replaces child processes with the requested programs. |
| `waitpid()` | Waits for child process termination and collects their status. |
| `pipe()` | Creates communication channels between processes. |
| `dup2()` | Redirects standard input, output, and error file descriptors. |
| `open()` | Opens files for input and output redirection. |
| `close()` | Closes file descriptors after use. |
| `chdir()` | Changes the shell's working directory. |
| `glob()` | Expands wildcard patterns into matching filenames. |

### Command Processing

The shell processes commands through the following stages:

1. **Tokenization:** Converts the input command into tokens.
2. **Parsing:** Constructs command objects and identifies arguments, pipes, redirections, and background execution.
3. **Wildcard expansion:** Expands filename patterns using `glob()`.
4. **Execution:** Creates child processes, configures file descriptors, and executes commands.
5. **Process management:** Waits for foreground processes or reaps background processes asynchronously.

### Pipeline Execution

For commands connected through pipes, the shell creates the required pipes before launching the child processes.

Each process receives the appropriate input and output file descriptors through `dup2()`, allowing the output of one command to become the input of the next.

For example:

```bash
ls -l | grep ".cpp" | wc -l
```

### I/O Redirection

The shell uses file descriptors to implement input, output, and error redirection.

Examples:

```bash
cat < input.txt
ls > output.txt
echo "Hello" >> output.txt
ls nonexistent 2> errors.txt
command1 >>& combined.log
```

## Project Structure

```text
unix-shell/
├── command.cc
├── command.h
├── tokenizer.cc
├── tokenizer.h
├── Makefile
├── README.md
└── .gitignore
```

## Building and Running

Clone the repository:

```bash
git clone https://github.com/mohandamr1/unix-shell.git
cd unix-shell
```

Build the project:

```bash
make
```

Run the shell:

```bash
./myshell
```

## Example Usage

```bash
myshell> pwd
myshell> ls -la
myshell> ls | grep ".cpp"
myshell> cat < input.txt
myshell> echo "Hello World" > output.txt
myshell> sleep 5 &
myshell> cd ..
myshell> exit
```

## Technologies

- C++
- Unix / POSIX system calls
- Process management
- Inter-process communication (IPC)
- File descriptor manipulation
- Multithreading (`std::thread`)
- Make

---

This project was developed as a hands-on exercise in Unix systems programming and operating-system concepts.
