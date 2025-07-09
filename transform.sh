#!/bin/sh

files=$(find gdb/testsuite -type f -name "*.exp*")

tmp=$(mktemp)
for f in $files; do
    ./transform.py "$f" > "$tmp"
    cp "$tmp" "$f"
done
rm -f "$tmp"
