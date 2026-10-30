
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <glob.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include "command.h"

#include <fstream>
#include <thread>
#include "tokenizer.h"
#include <cstring>
#include <cstdlib>

void expandWildcards(Command *currentCommand)
{
    for (int i = 0; i < currentCommand->_numberOfSimpleCommands; i++) {

        SimpleCommand *cmd = currentCommand->_simpleCommands[i];

        std::vector<std::string> args;

        // copy the existing arguments into a vector
        for (int j = 0; j < cmd->_numberOfArguments; j++) {
            args.push_back(cmd->_arguments[j]);
        }

        std::vector<std::string> expanded;

        // Expand each argument
        for (const std::string& arg : args) {
            if (arg.find_first_of("*?") == std::string::npos) {
                expanded.push_back(arg);
                continue;
            }

            glob_t results{};

            int ret = glob(arg.c_str(), 0, nullptr, &results); //finds all the files that have wildcards

            if (ret == 0) { //returns zero if successful
                for (size_t j = 0; j < results.gl_pathc; j++) {
                    expanded.push_back(results.gl_pathv[j]);
                }
            } else {
                // n matches keep the original argument
                expanded.push_back(arg);
            }

            globfree(&results);
        }

        // replace the old argument list
        for (int j = 0; j < cmd->_numberOfArguments; j++) {
            free(cmd->_arguments[j]);
        }

        free(cmd->_arguments);


        cmd->_numberOfArguments = 0;
        cmd->_numberOfAvailableArguments = 5;  //same reference
        cmd->_arguments = (char**)malloc(
            cmd->_numberOfAvailableArguments * sizeof(char*)
        );


        for (const std::string& arg : expanded) {
            cmd->insertArgument(strdup(arg.c_str()));
        }
    }
}

void parse(std::vector<Token> &tokens)
{
    Command currentCommand;
    SimpleCommand * currentSimpleCommand = new SimpleCommand();

    int redirectFlag = 0;
    int inputFlag = 0;
    int errorFlag = 0;
    int redirectErrorFlag = 0;


    for(auto &token : tokens){
        char * tokenValue = strdup(token.value.c_str()); //converts it to a c string and duplicates it for it to get freed
        switch(token.type){
            case TOKEN_COMMAND:
                 currentSimpleCommand->insertArgument(tokenValue);  //inserts command in argument[0]
                 break;

            case TOKEN_ARGUMENT:
                 if (redirectFlag) {
                     currentCommand._outFile = tokenValue;
                     redirectFlag = 0;
                     break;
                 }
                 if (inputFlag) {
                     currentCommand._inputFile = tokenValue;
                     inputFlag = 0;
                     break;
                 }
                if (errorFlag) {
                    currentCommand._errFile = tokenValue;
                    errorFlag = 0;
                    break;
                }
                if (redirectErrorFlag) {
                    currentCommand._errFile = tokenValue;
                    currentCommand._outFile = tokenValue;
                    currentCommand._out_error = 1;
                    redirectErrorFlag = 0;
                    break;
                }
                 currentSimpleCommand->insertArgument(tokenValue); //inserts the arguments in argument[1], argument[2]..
                 break;
            case TOKEN_REDIRECT: // >
                 redirectFlag = 1;
                 break;
            case TOKEN_APPEND: // >>
                 redirectFlag = 1;
                 currentCommand._append = 1;
                 break;
            case TOKEN_INPUT: // <
                 inputFlag = 1;
                 break;
            case TOKEN_ERROR:
                 errorFlag = 1;
                 break;
            case TOKEN_REDIRECT_AND_ERROR:
                 redirectErrorFlag = 1;
                 currentCommand._append = 1;
                 break;
            case TOKEN_PIPE: // |
                 currentCommand.insertSimpleCommand(currentSimpleCommand); //creates a new simple command
                 currentSimpleCommand = new SimpleCommand();
                 break;
            case TOKEN_BACKGROUND: // &
                 currentCommand._background = 1;
                 break;
            case TOKEN_EOF:
                if (redirectFlag || inputFlag ||
                    errorFlag || redirectErrorFlag) {
                    fprintf(stderr,
                            "Syntax error: missing redirection filename\n");

                    currentCommand.clear();
                    return;
                }


                currentCommand.insertSimpleCommand(currentSimpleCommand);

                break;
            default:
                break;
        }
    }

    expandWildcards(&currentCommand);
    currentCommand.execute();
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{   _numberOfAvailableSimpleCommands = 1;
    _numberOfSimpleCommands = 0;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _append = 0;
    _out_error = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile && _errFile != _outFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _append = 0;
    _out_error = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

void closePipes(int pipes[][2], int n) {
    for (int i = 0; i < n; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}


void logTermination(pid_t pid, int status) {
    std::ofstream logFile("log.txt", std::ios::app);

    logFile << "Child " << pid << " terminated";

    if (WIFEXITED(status)) {
        logFile << " with exit code " << WEXITSTATUS(status);
    }

    logFile << '\n';
}


void Command::execute() {


    if (_numberOfSimpleCommands == 0) {
        clear();
        return;
    }

    for (int i = 0; i < _numberOfSimpleCommands; i++) {
        if (_simpleCommands[i]->_numberOfArguments == 0) {
            fprintf(stderr, "Syntax error\n");
            clear();
            return;
        }
    }


    if (_numberOfSimpleCommands == 1 && //checks if the command is cd before forking
    strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {

        char* dir = _simpleCommands[0]->_arguments[1];

        if (dir == nullptr) {
            dir = getenv("HOME");
        }

        if (dir == nullptr || chdir(dir) == -1) {
            perror("cd");
        }

        clear();
        return;
    }

    int fdOut = 1;
    int fdIn = 0;
    int fdErr = 2;





    std::vector<pid_t> childPIDs = {};

    int nPipes = _numberOfSimpleCommands - 1;
    int pipesFd[nPipes][2]; //pipeFd[0] read, pipeFd[1] write for each pipe

    for (int i = 0; i < nPipes; i++) {
        pipe(pipesFd[i]); //prepare pipes by sending a system call to return the fds of the pipes
    }

    for (int i =0; i < _numberOfSimpleCommands; i++) {


        pid_t pID = fork();

        if (pID == 0) { //inside child process

            if (i < _numberOfSimpleCommands - 1) //write-end of the pipe
                dup2(pipesFd[i][1], 1);

            if (i > 0)  //read-end of the pipe
                dup2(pipesFd[i-1][0], 0);


            closePipes(pipesFd,nPipes);

           if (i == 0 && _inputFile) {
                fdIn = open(_inputFile, O_RDONLY);

                if (fdIn == -1) {
                  perror("Error opening input file");
                    _exit(1);
                }

                dup2(fdIn, 0);
                close(fdIn);   //replaces the file descriptor of the child process from STD_IN to the file
            }

                if (i == _numberOfSimpleCommands - 1) {

                    // combined stdout and stderr: >>&
                    if (_out_error) {
                        int fd = open(_outFile,
                                      O_WRONLY | O_CREAT | O_APPEND, 0644);

                        if (fd == -1) {
                            perror("open");
                            _exit(1);
                        }

                        dup2(fd, STDOUT_FILENO);
                        dup2(fd, STDERR_FILENO);
                        close(fd);
                    }
                    else {
                        // normal output: > or >>
                        if (_outFile) {
                            int flags = O_WRONLY | O_CREAT;

                            if (_append)
                                flags |= O_APPEND;
                            else
                                flags |= O_TRUNC;

                            int fd = open(_outFile, flags, 0644);

                            if (fd == -1) {
                                perror("open output");
                                _exit(1);
                            }

                            dup2(fd, STDOUT_FILENO);
                            close(fd);
                        }

                        // error output: 2>
                        if (_errFile) {
                            int fd = open(_errFile,
                                          O_WRONLY | O_CREAT | O_TRUNC, 0644);

                            if (fd == -1) {
                                perror("open error");
                                _exit(1);
                            }

                            dup2(fd, STDERR_FILENO);
                            close(fd);
                        }
                }
            }

            signal(SIGINT, SIG_DFL); //don't ignore ctrl c in child processes since it copied it from the parent
            execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
            perror("execvp error");
            _exit(1);
        }
        childPIDs.push_back(pID);
    }
    closePipes(pipesFd, nPipes);


    if (!_background) {
        for (pid_t pid : childPIDs) {
            int status;
            if (waitpid(pid, &status, 0) != -1) {
                logTermination(pid, status);
            }
        }
    } else {
        std::thread([pids = childPIDs]() { //creates a thread that reaps the children if theres no background
            for (pid_t pid : pids) {
                int status;
                if (waitpid(pid, &status, 0) != -1) {
                    logTermination(pid, status);
                }
            }
        }).detach(); //detaches it from the main thread
    }

    clear();
}



void Command::prompt()
{
    printf("myshell>");
    fflush(stdout);
    std::string input;
    signal(SIGINT, SIG_IGN); //ignores control C which generates the interrupt to the process

    while (true)
    {
        std::getline(std::cin, input);

        if (input.find_first_not_of(" \t") == std::string::npos) {
            printf("myshell>");
            fflush(stdout);
            continue;
        }

        if (input == "exit") {
            printf("Good bye!!\n");
            return;
        }

        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
        printf("myshell>");
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    Command::_currentCommand.prompt();
    return 0;
}