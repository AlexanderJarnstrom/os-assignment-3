/******************************************************************************
 *             File : test_script4.cpp
 *        Author(s) : Håkan Grahn
 *          Created : 2020-12-01
 *    Last Modified : 2022-11-25
 * Last Modified by : HGR
 *          Version : v1.3
 *
 * Test program to check Task 4 in Lab Assignment 3 in the
 * operating system courses DV1628/DV1629
 *
 * Copyright 2020-2022 by Håkan Grahn
 * Blekinge Institute of Technology.
 * All Rights Reserved
 *****************************************************************************/

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

    /*
    std::cout << "Task 4 ..." << std::endl;
    PRINTDIV2;

    std::cout << "Formatting and creating some test files (f1,f2) and directories (d1,d2)..." << std::endl;
    ret_val = filesystem.format();
    if (ret_val)
        std::cout << "Error: format failed, error code " << ret_val << std::endl;
    arg1 = "f1";
    fw = open("input1.txt", O_RDONLY);
    dup2(fw, 0);
    ret_val = filesystem.create(arg1);
    if (ret_val)
        std::cout << "Error: create " << arg1 << " failed, error code " << ret_val << std::endl;
    close(fw);
    arg1 = "f2";
    fw = open("input2.txt", O_RDONLY);
    dup2(fw, 0);
    ret_val = filesystem.create(arg1);
    if (ret_val)
        std::cout << "Error: create " << arg1 << " failed, error code " << ret_val << std::endl;
    close(fw);
    arg1 = "d1";
    ret_val = filesystem.mkdir(arg1);
    if (ret_val)
        std::cout << "Error: mkdir(d1) " << arg1 << " failed, error code " << ret_val << std::endl;
    arg1 = "d2";
    ret_val = filesystem.mkdir(arg1);
    if (ret_val)
        std::cout << "Error: mkdir(d1) " << arg1 << " failed, error code " << ret_val << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "d1\t dir\t -" << std::endl;
    std::cout << "d2\t dir\t -" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "f2\t file\t 23" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    ret_val = filesystem.ls();
    PRINTDIV2;
    */

    std::cout << "Task 4 ..." << std::endl;
    PRINTDIV2;

    std::cout << "Testing mkdir and cd with absolut and relative paths..." << std::endl;
    std::cout << "Starting with empty disk..." << std::endl;
    filesystem.format();
    std::cout << "mkdir(/d1), mkdir(d1/d2), mkdir(d1/d2/../d3)..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "d1\t dir\t -" << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << "/d1" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "d2\t dir\t -" << std::endl;
    std::cout << "d3\t dir\t -" << std::endl;
    std::cout << "Actual output:" << std::endl;
    filesystem.pwd();
    filesystem.mkdir("/d1");
    filesystem.mkdir("d1/d2");
    filesystem.mkdir("d1/d2/../d3");
    filesystem.ls();
    std::cout << "-----" << std::endl;
    filesystem.cd("d1");
    filesystem.pwd();
    filesystem.ls();
    PRINTDIV2;

    std::cout << "Testing cp/mv with absolut and relative paths..." << std::endl;
    std::cout << "cd(d2), create(f1) and create(f2)..." << std::endl;
    filesystem.cd("d2");
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
    std::cout << "checking files..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/d1/d2" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "f2\t file\t 23" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    ret_val = filesystem.ls();
    std::cout << "-----" << std::endl;
    std::cout << "cp(f1,f3)..." << std::endl;
    filesystem.cp("f1","f3");
    std::cout << "cp(f2,f4)..." << std::endl;
    filesystem.cp("f2","f4");
    std::cout << "cp(f1,..)..." << std::endl;
    filesystem.cp("f1","..");
    std::cout << "cp(f2,/d1)..." << std::endl;
    filesystem.cp("f2","/d1");
    std::cout << "mv(f3,..)..." << std::endl;
    filesystem.mv("f3","..");
    std::cout << "mv(f4,/d1)..." << std::endl;
    filesystem.mv("f4","/d1");
    std::cout << "Expected output:" << std::endl;
    std::cout << "/d1/d2" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "f2\t file\t 23" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    ret_val = filesystem.ls();
    std::cout << "-----" << std::endl;
    filesystem.cd("/d1");
    std::cout << "Expected output:" << std::endl;
    std::cout << "/d1" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "d2\t dir\t -" << std::endl;
    std::cout << "d3\t dir\t -" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "f2\t file\t 23" << std::endl;
    std::cout << "f3\t file\t 16" << std::endl;
    std::cout << "f4\t file\t 23" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    ret_val = filesystem.ls();
    PRINTDIV2;

    std::cout << "... Task 4 done" << std::endl;
    PRINTDIV;
}
