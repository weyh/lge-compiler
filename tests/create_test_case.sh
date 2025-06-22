#!/bin/bash

EXE="$1"
RUNTIME="$2"

FILE="$3"

$EXE tests/examples/$FILE.lge | lli -load="$RUNTIME" > tests/snapshots/"$FILE"_stdout.txt 2> tests/snapshots/"$FILE"_stderr.txt

echo "Ret: $?"
