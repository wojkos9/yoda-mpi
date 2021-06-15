#!/bin/bash
excl="${1-NOSHM}"

noshm=`[[ "$excl" =~ NOSHM ]] && echo 1 || echo 0`

pre=pre
src=src

rm $src/*

for f in `ls $pre`; do
    if [ "$noshm" -eq 1 ]; then
        if [[ "$f" =~ shm ]]; then
        continue; fi
    fi
    awk -P "/\/\/ifndef ($excl)$/ {token=\$NF;flag=1};\
    {if (!flag) print};\
    /\/\/endif ($excl)/ {if (\$NF==token) flag=0}" $pre/$f > $src/$f
done