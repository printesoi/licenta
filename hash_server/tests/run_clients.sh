#!/bin/bash

for i in `seq 1 100`; do
    ../build/client inputs/test_0004.txt &
done
