#!/bin/bash

# Example invocation:
# ./src/gdb/contrib/replace-on-error.sh \
#    "no matching function for call to .regcache::cooked_write" \
#    "cooked_write" \
#    "deprecated_cooked_write" \
#    build-log.txt

error="$1"
pat="$2"
repl="$3"
build_log="$4"

while read -r -a fields; do
    file=${fields[0]}
    line=${fields[1]}

    found=false
    while true; do
	md5sum=$(md5sum "$file")
	sed -i \
	    "${line}s/$pat/$repl/" \
	    "$file"
	md5sum_2=$(md5sum "$file")

	if [ "$md5sum" != "$md5sum_2" ]; then
	    found=true
	    break
	fi

	line=$((line - 1))

	if [ $line -eq 0 ]; then
	    break
	fi
    done

    if ! $found; then
	echo "TODO: $file $line"
    fi
done < <(grep "$error" "$build_log" \
	     | awk '{print $1}' \
	     | awk -F : '{print $1, $2}')
