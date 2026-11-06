#!/bin/sh
PATH=/usr/bin:/bin
export PATH
out=$(mktemp) || exit 1
cc -o "$out" "$1" || { rm -f "$out"; exit 1; }
"$out"
status=$?
rm -f "$out"
exit $status
