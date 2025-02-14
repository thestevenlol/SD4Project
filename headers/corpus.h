#ifndef CORPUS_H
#define CORPUS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*

CORPUS CREATION:
1. Create a new directory named "corpus" in the root of the project.
2. Create a new file named "corpus.txt" in the "corpus" directory.
3. Cleanup, close file.

SELECTING RANDOM VALUE FROM CORPUS:
1. Open "corpus.txt" in read mode.
2. Read the number of lines in the file.
3. Generate a random number between 1 and the number of lines.
4. Read the line at the generated random number.
5. Close file.

ADDING NEW VALUE TO CORPUS:
1. Open "corpus.txt" in append mode.
2. Write the new value to the file.
3. Close file.


REMOVING VALUE FROM CORPUS:
1. Open "corpus.txt" in read mode and create a temporary file.
2. Copy all lines except the one to be removed to the temporary file.
3. Rename the temporary file to "corpus.txt".
4. Close both files.

VALIDATING CORPUS CONTENT:
1. Open "corpus.txt" in read mode.
2. Check if the file is empty or contains invalid data.
3. Close file.

*/

int createCorpus(char* path);
int selectRandomValueFromCorpus(char* path);
int addValueToCorpus(char* path, int value);
int removeValueFromCorpus(char* path, int value);
int validateCorpusContent(char* path);

#endif