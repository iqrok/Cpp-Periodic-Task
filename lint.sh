#/bin/sh

DIR=$( cd -P -- "$(dirname -- "$(command -v -- "$0")")" && pwd -P )
FILE_LIST="$(find "${DIR}" \
	| grep -v "${DIR}/build" \
	| grep -E ".*(\.ino|\.cpp|\.c|\.h|\.hpp|\.hh)$"
)"

clang-format -i --verbose ${FILE_LIST}
