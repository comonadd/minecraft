#!/bin/bash

./scripts/build-debug.sh
out=massif.out
valgrind --tool=massif --massif-out-file=$out ./Debug/minecraft
massif-visualizer $out
