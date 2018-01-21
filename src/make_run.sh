#!/bin/bash
make clean
make
#./lisod
./lisod 8080 1699 ../lisod.log ../lisod.lock www/ cgi/flaskr.py grader.key grader.crt
