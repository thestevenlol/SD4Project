#ifndef TESTCASE_H
#define TESTCASE_H

int createTestSuiteFolder(const char* path);
int createMetadataFile(const char* filePath, const char* filename);
int createTestSuiteAndMetadata(const char* fullPath, const char* filename);
int createTestInputFile(const int* inputs, size_t num_inputs, const char* test_suite);
char* getFullPath(const char* filePath);

#endif