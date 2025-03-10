#! /bin/bash

set -e

STANDARD="(ADD|FIX|RM|DOCS|REFACT|TEST|MERGE|UPDATE)"

LAST_COMMITS=$(git log --format="%s" -n 10)

FAILED=false

while IFS= read -r COMMIT; do
  if [[ ! $COMMIT =~ $STANDARD ]]; then
    echo "::error title=This commit does not respect the standard::$COMMIT"
    FAILED=true
  fi
done <<< "$LAST_COMMITS"

if [ "$FAILED" = true ]; then
  exit 1
fi
echo "::notice title=Standard check::is ok"