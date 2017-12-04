#!/bin/bash

# Please use shellcheck when modify this script

MAX_LINE_LENGTH=80

pushd "$(dirname $0)" > /dev/null
ROOT=$(git rev-parse --show-toplevel)
popd > /dev/null

pushd "${ROOT}" > /dev/null

# checkpatch
./tools/checkpatch.pl --max-line-length="${MAX_LINE_LENGTH}" --color=never --emacs --file --no-tree --fix-inplace src/*.[ch]

popd > /dev/null
