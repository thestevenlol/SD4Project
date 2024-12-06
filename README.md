# C Fuzzer - 2024 Software Development

## C00274246 - Jack Foley

## What is this project?

This project is a fuzzing tool written in the C Programming language. It uses the standard C library on UNIX, glib.

## How to use

Please note: You MUST run this on a UNIX system. It will not work if you do not have a UNIX system.

1. Ensure you have testcov installed.

```
see https://gitlab.com/sosy-lab/software/test-suite-validator
Install gcc-multilib (Linux fedora):
sudo apt install gcc glibc-devel.i686 libgcc.i686
sudo apt install --fix-missing python3-pip
git clone https://gitlab.com/sosy-lab/software/test-suite-validator.git TestCov
cd TestCov
pip install --break-system-packages . 
pip3 install --break-system-packages --user matplotlib

Check that testcov is installed by running: testcov
```

2. Ensure lcov is installed

```
sudo apt install lcov
```

3. Ensure flex is installed

```
sudo apt install flex
```

Finally, run the software by running the following commands.

```
./main problems/Problem10.c
./zip-test-suite.sh Problem10.c    
testcov --no-isolation --test-suite test-suites/Problem10.c-test-suite.zip problems/Problem10.c  
```

This will create the test inputs for the problem file and then produce a coverage report on it.