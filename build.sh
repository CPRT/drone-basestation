#!/bin/bash

gcc -lpthread joyclient.c -o joyclient
gcc serialserver.c -o serialserver
