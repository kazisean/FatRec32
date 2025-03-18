#!/bin/bash

rm -rf fatrec32.zip
zip fatrec32.zip Makefile *.c
./test-cases.sh