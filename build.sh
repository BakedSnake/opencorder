#!/bin/bash

clang -o pv pv.c -lm -lsndfile $(pkg-config --cflags --libs libpipewire-0.3 libspa-0.2)
