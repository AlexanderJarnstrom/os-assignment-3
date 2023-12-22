/******************************************************************************
 *             File : test_script2.cpp
 *        Author(s) : Håkan Grahn
 *          Created : 2020-12-01
 *    Last Modified : 2022-11-24
 * Last Modified by : HGR
 *          Version : v1.3
 *
 * Test program to check Task 2 in Lab Assignment 3 in the
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

    std::cout << "Task 2 ..." << std::endl;
    PRINTDIV2;

    std::cout << "Formatting and creating a test file (f1)..." << std::endl;
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
    PRINTDIV2;

    std::cout << "Testing cp(f1,f2)..." << std::endl;
    arg1 = "f1";
    arg2 = "f2";
    ret_val = filesystem.cp(arg1,arg2);
    if (ret_val) 
        std::cout << "Error: cp(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;
    // check that there exists a copy with the same size
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "f1\t 16" << std::endl;
    std::cout << "f2\t 16" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();
    std::cout << "Checking file contents" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << input1;
    std::cout << input1;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f1";
    ret_val = filesystem.cat(arg1);
    arg1 = "f2";
    ret_val = filesystem.cat(arg1);
    std::cout << "... done cp(f1,f2)" << std::endl;

    std::cout << "--------\nTry to cp to an existing file... cp(f1,f2)" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f1";
    arg2 = "f2";
    ret_val = filesystem.cp(arg1, arg2);
    if (ret_val) 
        std::cout << "Error: cp(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;

    std::cout << "--------\nTry to cp from a non-existing file... cp(f4,f2)" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f4";
    arg2 = "f2";
    ret_val = filesystem.cp(arg1, arg2);
    if (ret_val) 
        std::cout << "Error: cp(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;
    PRINTDIV2;

    std::cout << "Testing mv(f2,f3)..." << std::endl;
    arg1 = "f2";
    arg2 = "f3";
    ret_val = filesystem.mv(arg1,arg2);
    if (ret_val) 
        std::cout << "Error: mv(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;
    // check that there exists a copy with the same size
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "f1\t 16" << std::endl;
    std::cout << "f3\t 16" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();
    std::cout << "Checking file contents of f3" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << input1;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f3";
    ret_val = filesystem.cat(arg1);
    std::cout << "... done mv(f2,f3)" << std::endl;

    std::cout << "--------\nTry to mv to an existing file... mv(f1,f3)" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f1";
    arg2 = "f3";
    ret_val = filesystem.mv(arg1, arg2);
    if (ret_val)
        std::cout << "Error: mv(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;

    std::cout << "--------\nTry to mv from a non-existing file... mv(f4,f3)" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f4";
    arg2 = "f3";
    ret_val = filesystem.mv(arg1, arg2);
    if (ret_val)
        std::cout << "Error: mv(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;

    PRINTDIV2;

    std::cout << "Testing rm(f1)..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "f3\t 16" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f1";
    ret_val = filesystem.rm(arg1);
    if (ret_val) 
        std::cout << "Error: rm(f1) " << arg1 << " failed, error code " << ret_val << std::endl;
    ret_val = filesystem.ls();

    std::cout << "--------\nTry to rm to a non-existing file... rm(f1)" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f1";
    ret_val = filesystem.rm(arg1);
    if (ret_val)
        std::cout << "Error: rm(f1) " << arg1 << " failed, error code " << ret_val << std::endl;

    std::cout << "... done rm(f1)" << std::endl;
    PRINTDIV2;

    std::cout << "Testing append(f1,f3)..." << std::endl;
    std::cout << "execute create(f1)..." << std::endl;
    fw = open("input2.txt", O_RDONLY);
    dup2(fw,0);
    arg1 = "f1";
    ret_val = filesystem.create(arg1);
    std::cout << "checking files..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "f1\t 23" << std::endl;
    std::cout << "f3\t 16" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();
    arg1 = "f1";
    arg2 = "f3";
    ret_val = filesystem.append(arg1,arg2);
    if (ret_val)
        std::cout << "Error: append(" << arg1 << "," << arg2 << ") failed, error code " << ret_val << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "f1\t 23" << std::endl;
    std::cout << "f3\t 39" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();
    std::cout << "Checking file contents of f3" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << input1 << input2;
    std::cout << "Actual output:" << std::endl;
    arg1 = "f3";
    ret_val = filesystem.cat(arg1);
    close(fw);
    std::cout << "... done append(f1,f3)" << std::endl;
    PRINTDIV2;

    std::cout << "... Task 2 done" << std::endl;
    PRINTDIV;

}

