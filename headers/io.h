#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

FILE* openFile(const char* filename);
int closeFile(FILE *file);
int writeStringToFile(FILE* file, const char* string);
int createFolder(const char* basePath, const char* folderName);
char* getFullPath(const char* filename);
char* getCurrentTime(void);
char* getHash(const char* filepath);

#endif