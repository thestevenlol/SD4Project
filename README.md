# C Fuzzer - 2024 Software Development

## C00274246 - Jack Foley

## What is this project?

This project is a fuzzing tool written in the C Programming language. It uses the standard C library on UNIX, glib.

## How to use

Please note: You MUST run this on a UNIX system. It will not work if you do not have a UNIX system (WSL works too!).

1. Ensure lcov is installed

```
sudo apt install lcov
```

2. Ensure flex is installed

```
sudo apt install flex
```

Finally, run the software by running the following commands.

```
./main problems/Problem10.c  
```