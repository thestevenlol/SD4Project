#ifndef TESTCASE_H
#define TESTCASE_H

int createTestSuiteFolder(const char* path);
int createMetadataFile(const char* filePath, const char* filename);
char* getFullPath(const char* filePath);

#endif