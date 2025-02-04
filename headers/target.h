#ifndef TARGET_H
#define TARGET_H

int compileTargetFile(const char* path, const char* fileName);
int executeTargetInt(int input);
void cleanupTarget(void);

#endif