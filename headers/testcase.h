#ifndef TESTCASE_H
#define TESTCASE_H

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int createMetadataFile(const char* filename);
int createTestSuiteFolder(const char* foldername);
char* get_current_time(void);
char* get_full_path(const char* filename);
int close_file(FILE *file);

#endif