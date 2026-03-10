#!/bin/bash

clang -o corder corder.c -lm -lsndfile $(pkg-config --cflags --libs libpipewire-0.3 libspa-0.2)
