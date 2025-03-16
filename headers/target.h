#ifndef TARGET_H
#define TARGET_H

int compileTargetFile(const char* sourcePath, const char* fileName);
int executeTargetInt(int input, char* filename);  // Keep original signature to maintain compatibility
void cleanupTarget();

#endif