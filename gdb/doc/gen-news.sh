#!/bin/sh

set -e

f=gdb/NEWS

gen_news ()
{
    cat <<EOF
# THIS FILE IS GENERATED -*- buffer-read-only: t -*-
# vi:set ro:
# The contents of this generated file are maintained at gdb/doc/NEWS-*.
#
		What has changed in GDB?
	     (Organized release by release)
EOF

    lst=$(cat gdb/doc/NEWS.lst)

    first=true
    for version in $lst; do
	echo

	if $first; then
	    echo "*** Changes since GDB $((version - 1))"
	    first=false
	else
	    echo "*** Changes in GDB $version"
	fi
	echo
	cat "gdb/doc/NEWS-$version"
    done
}

gen_news > "$f"
