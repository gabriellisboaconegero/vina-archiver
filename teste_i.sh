#! /usr/bin/bash

set -x

rm -f teste/archive.vpp
if ! ./vina++ -i teste/archive.vpp teste/teste1.in teste/teste2.in teste/Lorem.txt teste/progc teste/bib teste/paper1; then
    rm -f teste/archive.vpp
else
    xxd -g 8 -c 50 teste/archive.vpp > vizu.xxd
fi
