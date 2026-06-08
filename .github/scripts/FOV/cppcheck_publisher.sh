#! /bin/bash

cppcheck_output_file="check.txt"
ERROR=0

cat "$cppcheck_output_file" | while IFS= read -r line; do
  if [[ $line =~ ^(.+):([0-9]+):([0-9]+):\ ([a-z]+):\ (.+)$ ]]; then
    filepath="${BASH_REMATCH[1]}"
    line="${BASH_REMATCH[2]}"
    col="${BASH_REMATCH[3]}"
    level="${BASH_REMATCH[4]}"
    message="${BASH_REMATCH[5]}"

    if [[ "$level" == "style" ]]; then
      level="warning"
    elif [[ "$level" == "error" ]]; then
      level="error"
    else
      level="notice"
    fi

    echo "::${level} file=${filepath},line=${line},col=${col}::${message}"
  fi
done