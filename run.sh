#!/bin/bash
if make; then

    while getopts "c:" arg; do
        case $arg in
            c) xyz=( ${@:$OPTIND-1:3} ) ;;
        esac
    done

    if [ -z "$xyz" -o ! ${#xyz[@]} -eq 3 ]; then
        echo Please provide 3 sizes in the \"-c\" option, not ${#xyz[@]}
        exit 1
    fi

    size=$(( ${xyz[0]} + ${xyz[1]} + ${xyz[2]} ))

    cmd="mpirun -v --oversubscribe -np $size out/proj $@"
    echo $cmd
    $cmd
fi