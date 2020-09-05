#!/bin/bash

cf_version=$(clang-format --version | cut -d " " -f3)
min_version="10.0.0"
if [ $(echo -e "${min_version}\n${cf_version}" | sort -V | head -n1) != "$min_version" ]; then
    echo "Must use clang-format version $min_version or greater"
    exit 1
fi

files=$(find . -not -path "./tools/*" -type f \( -iname \*.c -o -iname \*.h \))
for file in $files
do
    echo 'Formatting:' $file
    $(clang-format -i $file)
done