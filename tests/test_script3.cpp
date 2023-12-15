/******************************************************************************
 *             File : test_script3.cpp
 *        Author(s) : Håkan Grahn
 *          Created : 2020-12-01
 *    Last Modified : 2022-11-25
 * Last Modified by : HGR
 *          Version : v1.3
 *
 * Test program to check Task 3 in Lab Assignment 3 in the
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
    std::cout << "Task 3 ..." << std::endl;
    PRINTDIV2;

    std::cout << "Formatting and creating some test files (f1,f2)..." << std::endl;
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
    PRINTDIV2;

    std::cout << "Testing mkdir(d1)..." << std::endl;
    arg1 = "d1";
    ret_val = filesystem.mkdir(arg1);
    if (ret_val) 
        std::cout << "Error: mkdir(d1) " << arg1 << " failed, error code " << ret_val << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "d1\t dir\t -" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "f2\t file\t 23" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();
    std::cout << "--------\nTry to mkdir(f1)" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f1";
    ret_val = filesystem.mkdir(arg1);
    if (ret_val)
        std::cout << "Error: mkdir " << arg1 << " failed, error code " << ret_val << std::endl;
    std::cout << "... done mkdir(d1)" << std::endl;
    PRINTDIV2;

    std::cout << "Testing pwd and cd(d1)..." << std::endl;
    std::cout << "checking pwd, starting in root directory..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed in root directory, error code " << ret_val << std::endl;

    std::cout << "Executing cd(d1)..." << std::endl;
    arg1 = "d1";
    ret_val = filesystem.cd(arg1);
    if (ret_val) 
        std::cout << "Error: cd " << arg1 << " failed, error code " << ret_val << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/d1" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;

    std::cout << "go back to root..." << std::endl;
    arg1 = "..";
    ret_val = filesystem.cd(arg1);
    if (ret_val)
        std::cout << "Error: cd " << arg1 << " failed, error code " << ret_val << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;

    std::cout << "--------\nCheck that we can't do cd to a file, cd(f1)" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f1";
    ret_val = filesystem.cd(arg1);
    if (ret_val)
        std::cout << "Error: cd " << arg1 << " failed, error code " << ret_val << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;

    std::cout << "--------\nCheck that we can't do cat on a directory, cat(d1)" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "d1";
    ret_val = filesystem.cat(arg1);
    if (ret_val)
        std::cout << "Error: cat " << arg1 << " failed, error code " << ret_val << std::endl;
    PRINTDIV2;

    std::cout << "Checking that cp works with a directory as destination" << std::endl;
    std::cout << "Testing cp(f1,d1)..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "d1\t dir\t -" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "f2\t file\t 23" << std::endl;
    std::cout << "/d1" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;
    ret_val = filesystem.ls();
    if (ret_val)
        std::cout << "Error: ls failed, error code " << ret_val << std::endl;
    arg1 = "f1";
    arg2 = "d1";
    ret_val = filesystem.cp(arg1, arg2);
    if (ret_val)
        std::cout << "Error: cp(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;
    arg1 = "d1";
    ret_val = filesystem.cd(arg1);
    if (ret_val)
        std::cout << "Error: cd " << arg1 << " failed, error code " << ret_val << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;
    ret_val = filesystem.ls();
    if (ret_val)
        std::cout << "Error: ls failed, error code " << ret_val << std::endl;
    arg1 = "..";
    ret_val = filesystem.cd(arg1);
    if (ret_val)
        std::cout << "Error: cd " << arg1 << " failed, error code " << ret_val << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;
    PRINTDIV2;

    std::cout << "Checking that mv works with a directory as destination" << std::endl;
    std::cout << "Testing mv(f2,d1)..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "d1\t dir\t -" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "f2\t file\t 23" << std::endl;
    std::cout << "/d1" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "f2\t file\t 23" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "name\t type\t size" << std::endl;
    std::cout << "d1\t dir\t -" << std::endl;
    std::cout << "f1\t file\t 16" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;
    ret_val = filesystem.ls();
    if (ret_val)
        std::cout << "Error: ls failed, error code " << ret_val << std::endl;
    arg1 = "f2";
    arg2 = "d1";
    ret_val = filesystem.mv(arg1, arg2);
    if (ret_val)
        std::cout << "Error: mv(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;
    arg1 = "d1";
    ret_val = filesystem.cd(arg1);
    if (ret_val)
        std::cout << "Error: cd " << arg1 << " failed, error code " << ret_val << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;
    ret_val = filesystem.ls();
    if (ret_val)
        std::cout << "Error: ls failed, error code " << ret_val << std::endl;
    arg1 = "..";
    ret_val = filesystem.cd(arg1);
    if (ret_val)
        std::cout << "Error: cd " << arg1 << " failed, error code " << ret_val << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val)
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;
    ret_val = filesystem.ls();
    if (ret_val)
        std::cout << "Error: ls failed, error code " << ret_val << std::endl;
    PRINTDIV2;

    std::cout << "... Task 3 done" << std::endl;
    PRINTDIV;
}

