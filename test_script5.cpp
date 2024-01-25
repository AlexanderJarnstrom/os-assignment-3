#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "test_script.h"
#include "fs.h"

#define PRINTDIV std::cout <<  "================================================================================" << std::endl
#define PRINTDIV2 std::cout << "----------------------------------------" << std::endl

std::string commands_str[] = {
    "format", "create", "cat", "ls",
    "cp", "mv", "rm", "append",
    "mkdir", "cd", "pwd",
    "chmod",
    "help", "quit"
};

Shell::Shell()
{
    std::cout << "Creating and starting shell...\n";
}

Shell::~Shell()
{
    std::cout << "Exiting shell...\n";
}

void
Shell::run()
{
    std::string cmd, arg1, arg2;
    int ret_val = 0;
    int fd[2];
    int fw;
    std::string input1 = "hej heja hejare\n";
    std::string input2 = "hej heja hejare hejast\n";

    PRINTDIV;
    std::cout << "\\ / \\ / \\ / \\ / \\ / \\ / \\     new test session     / \\ / \\ / \\ / \\ / \\ / \\ / \\ /" << std::endl;
    PRINTDIV;
    std::cout << "Starting test sequence..." << std::endl;
    PRINTDIV;
    std::cout << "Task 5 ..." << std::endl;
    PRINTDIV2;

    std::cout << "Testing access rights..." << std::endl;
    std::cout << "Starting with empty disk..." << std::endl;
    std::cout << "Use \"/\" as test dir..." << std::endl;
    filesystem.format();
    fw = open("input1.txt", O_RDONLY);
    dup2(fw,0);
    arg1 = "f1";
    filesystem.create(arg1);
    close(fw);
    fw = open("input2.txt", O_RDONLY);
    dup2(fw,0);
    arg1 = "f2";
    filesystem.create(arg1);
    close(fw);

    std::cout << "checking start files..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "name\t type\t accessrights\t size" << std::endl;
    std::cout << "f1\t file\t rw-\t 16" << std::endl;
    std::cout << "f2\t file\t rw-\t 23" << std::endl;
    std::cout << "Actual output:" << std::endl;
    filesystem.pwd();
    filesystem.ls();
    std::cout << "-----" << std::endl;

    arg1 = "2";
    arg2 = "f1";
    std::cout << "chmod(2,f1)..." << std::endl;
    ret_val = filesystem.chmod(arg1, arg2);
    if (ret_val)
        std::cout << "Error: chmod(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;
    std::cout << "chmod(4,f2)..." << std::endl;
    arg1 = "4";
    arg2 = "f2";
    ret_val = filesystem.chmod(arg1, arg2);
    if (ret_val)
        std::cout << "Error: chmod(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "name\t type\t accessrights\t size" << std::endl;
    std::cout << "f1\t file\t -w-\t 16" << std::endl;
    std::cout << "f2\t file\t r-\t 23" << std::endl;
    std::cout << "Actual output:" << std::endl;
    filesystem.ls();

    std::cout << "cat(f1)..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f1";
    filesystem.cat(arg1);
    std::cout << "-----" << std::endl;
    std::cout << "append(f1,f2)..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f1";
    arg2 = "f2";
    filesystem.append(arg1, arg2);
    std::cout << "-----" << std::endl;
    std::cout << "append(f2,f1)..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t type\t accessrights\t size" << std::endl;
    std::cout << "f1\t file\t -w-\t 39" << std::endl;
    std::cout << "f2\t file\t r-\t 23" << std::endl;
    std::cout << "Actual output:" << std::endl;
    filesystem.append(arg2, arg1);
    filesystem.ls();
    PRINTDIV2;

    std::cout << "... Task 5 done" << std::endl;
    PRINTDIV;
}
