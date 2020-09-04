#!/usr/bin/env bash

FORMAT_OPTS="-i -style=file"
TIDY_OPTS="-p . --fix --fix-errors"
COMPILER_OPTS="-fno-builtin -std=gnu90 -Iinclude -Isrc -D_LANGUAGE_C -DNON_MATCHING"

shopt -s globstar

if (( $# > 0 )); then
    echo "Formatting file(s) $*"
    echo "Running clang-format..."
    clang-format ${FORMAT_OPTS} "$@"
    echo "Running clang-tidy..."
    clang-tidy ${TIDY_OPTS} "$@" -- ${COMPILER_OPTS} &> /dev/null
    echo "Done formatting file(s) $*"
    exit
fi

files=$(find . -not -path "./tools/*" -type f \( -iname \*.c -o -iname \*.h \))
echo "Formatting C files. This will take a bit"
echo "Running clang-format..."
for file in $files
do
    echo 'Formatting:' $file
    clang-format ${FORMAT_OPTS} $file
    clang-tidy ${TIDY_OPTS} $file -- ${COMPILER_OPTS} &> /dev/null
    echo 'Tidying:' $file
    $(clang-format -i $file)
done
echo "Done formatting all files."