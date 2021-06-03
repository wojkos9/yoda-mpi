#!/bin/bash
if make; then
    cmd="mpirun --oversubscribe -np ${1-4} out/proj ${@:2:$#-1}"
    echo $cmd
    $cmd
fi