#!/bin/bash
make clean
make
#./lisod
./lisod 8080 1699 ../lisod.log ../lisod.lock www/ cgi/wsgi_wrapper.py grader.key grader.crt
