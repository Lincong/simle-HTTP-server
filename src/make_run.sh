#!/bin/bash
make clean
make
#./lisod
./lisod 7788 1699 ../lisod.log ../lisod.lock www cgi/cgi_script.py grader.key grader.crt
