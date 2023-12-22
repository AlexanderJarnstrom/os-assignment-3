/******************************************************************************
 *             File : test_script1.cpp
 *        Author(s) : Håkan Grahn
 *          Created : 2020-12-01
 *    Last Modified : 2022-11-24
 * Last Modified by : HGR
 *          Version : v1.3
 * 
 * Test program to check Task 1 in Lab Assignment 3 in the 
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
    std::cout << "Task 1 ..." << std::endl;
    PRINTDIV2;

    std::cout << "Testing format()..." << std::endl;
    ret_val = filesystem.format();
    if (ret_val) {
        std::cout << "Error: format failed, error code " << ret_val << std::endl;
    }
    // check that the disk is empty
    std::cout << "Executing ls" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();
    if (ret_val) {
        std::cout << "Error: ls failed, error code " << ret_val << std::endl;
    }
    std::cout << "Executing pwd" << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "/" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.pwd();
    if (ret_val) {
        std::cout << "Error: pwd failed, error code " << ret_val << std::endl;
    }
    std::cout << "... done format()" << std::endl;
    PRINTDIV2;

    std::cout << "Testing create(f1)..." << std::endl;
    arg1 = "f1";
    fw = open("input1.txt", O_RDONLY);
    dup2(fw,0);
    ret_val = filesystem.create(arg1);
    if (ret_val) {
        std::cout << "Error: create " << arg1;
        std::cout << " failed, error code " << ret_val << std::endl;
    }
    // check that the file is there
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "f1\t 16" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();
    if (ret_val) {
        std::cout << "Error: ls failed, error code " << ret_val << std::endl;
    }
    close(fw);

    std::cout << "Testing create() with large file..." << std::endl;
    arg1 = "f4129";
    fw = open("input3.txt", O_RDONLY);
    dup2(fw,0);
    ret_val = filesystem.create(arg1);
    if (ret_val) {
        std::cout << "Error: create " << arg1;
        std::cout << " failed, error code " << ret_val << std::endl;
    }
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "f1\t 16" << std::endl;
    std::cout << "f4129\t 4129" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();
    close(fw);

    std::cout << "... done create()" << std::endl;
    PRINTDIV2;

    std::cout << "Testing cat(f1)..." << std::endl;
    arg1 = "f1";
    std::cout << "Expected output:" << std::endl;
    std::cout << input1;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.cat(arg1);
    if (ret_val)
    {
        std::cout << "Error: cat " << arg1;
        std::cout << " failed, error code " << ret_val << std::endl;
    }
    std::cout << "--------\nTry to cat a non-existing file..." << std::endl;
    arg1 = "f2";
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.cat(arg1);
    if (ret_val)
    {
        std::cout << "Error: cat " << arg1;
        std::cout << " failed, error code " << ret_val << std::endl;
    }
    std::cout << "... done cat()" << std::endl;
    PRINTDIV2;

    std::cout << "Checking long file names..." << std::endl;
    std::cout << "Formatting disk ..." << std::endl;
    ret_val = filesystem.format();
    std::cout << "55 char names should work..." << std::endl;
    arg1 = "AbcdefghijAbcdefghijAbcdefghijAbcdefghijAbcdefghijAbcde";
    fw = open("input1.txt", O_RDONLY);
    dup2(fw, 0);
    ret_val = filesystem.create(arg1);
    if (ret_val)
    {
        std::cout << "Error: create " << arg1;
        std::cout << " failed, error code " << ret_val << std::endl;
    }
    close(fw);
    // check that the file is there
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "AbcdefghijAbcdefghijAbcdefghijAbcdefghijAbcdefghijAbcde\t 16" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();

    std::cout << "56 char names should give an error..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << " ... some error message" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "AbcdefghijAbcdefghijAbcdefghijAbcdefghijAbcdefghijAbcde\t 16" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "AbcdefghijAbcdefghijAbcdefghijAbcdefghijAbcdefghijAbcdeX";
    fw = open("input1.txt", O_RDONLY);
    dup2(fw, 0);
    ret_val = filesystem.create(arg1);
    if (ret_val)
    {
        std::cout << "Error: create " << arg1;
        std::cout << " failed, error code " << ret_val << std::endl;
    }
    close(fw);
    ret_val = filesystem.ls();

    PRINTDIV2;

    std::cout << "Testing the number of files that fit in a directory ..." << std::endl;
    std::cout << "Formatting disk ..." << std::endl;
    ret_val = filesystem.format();

    // check how many dir entries that fit in a block
    int no_dir_entries = BLOCK_SIZE / sizeof(dir_entry);
    std::cout << "BLOCK_SIZE = " << BLOCK_SIZE << ", sizeof(dir_entry) = " << sizeof(dir_entry) << ", " << no_dir_entries << " dir_entries per disk block." << std::endl;
    std::cout << "Creating " << no_dir_entries << " files in the root directory..." << std::endl;
    for (int i = 0; i < no_dir_entries; ++i) {
        arg1 = "f" + std::to_string(i);
        fw = open("input1.txt", O_RDONLY);
        dup2(fw, 0);
        ret_val = filesystem.create(arg1);
        if (ret_val) {
            std::cout << "Error: create " << arg1;
            std::cout << " failed, error code " << ret_val << std::endl;
        }
        close(fw);
    }
    std::cout << "Expected output:" << std::endl;
    std::cout << "name\t size" << std::endl;
    std::cout << "f0\t 16" << std::endl;
    std::cout << "f1\t 16" << std::endl;
    std::cout << "f2\t 16" << std::endl;
    std::cout << "...\t ..." << std::endl;
    std::cout << "f62\t 16" << std::endl;
    std::cout << "f63\t 16" << std::endl;
    std::cout << "Actual output:" << std::endl;
    ret_val = filesystem.ls();

    std::cout << "--------\nAdding one more file should give an error..." << std::endl;
    std::cout << "Expected output:" << std::endl;
    std::cout << "... some kind of error message" << std::endl;
    std::cout << "Actual output:" << std::endl;
    arg1 = "fx";
    fw = open("input1.txt", O_RDONLY);
    dup2(fw, 0);
    ret_val = filesystem.create(arg1);
    if (ret_val)
    {
        std::cout << "Error: create " << arg1;
        std::cout << " failed, error code " << ret_val << std::endl;
    }
    close(fw);

    PRINTDIV2;

    std::cout << "... Task 1 done" << std::endl;
    PRINTDIV;

}
