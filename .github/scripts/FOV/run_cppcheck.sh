#!/bin/sh

cppcheck -j10 --enable=all --quiet --check-level=exhaustive --suppress=missingIncludeSystem --suppress=missingInclude \
--suppress=unusedFunction --suppress=preprocessorErrorDirective --suppress=unmatchedSuppression --project=build/compile_commands.json \
--suppress=useStlAlgorithm --suppress=checkersReport > check.txt 2>&1
