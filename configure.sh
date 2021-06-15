#!/bin/bash
FLAGS="NOERR|NOSHOWWAKE|"

excl="${1-NOSHM}"

excl="$FLAGS|$excl"

noshm=`[[ "$excl" =~ NOSHM ]] && echo 1 || echo 0`

pre=pre
src=src

[ ! -e src ] && mkdir src
rm $src/*

for f in `ls $pre`; do
    if [ "$noshm" -eq 1 ]; then
        if [[ "$f" =~ shm ]]; then
        continue; fi
    fi
    echo $f >&2
    awk -P "/\/\/ifndef ($excl)$/ {token=\$NF;flag=1};\
    {if (!flag) print};\
    /\/\/endif ($excl)/ {if (\$NF==token) flag=0}" $pre/$f > $src/$f
done