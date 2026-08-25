#!/bin/sh

# Copyright (C) 2026 Free Software Foundation, Inc.
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Wrapper around xmllint to make it exit with non-zero if there is any output.

tmp=""

cleanup()
{
    if [ "$tmp" != "" ]; then
	rm -f "$tmp"
    fi
}

# Schedule cleanup.
trap cleanup EXIT

tmp=$(mktemp)

check_basic=""
check_includes=""
for f in "$@"; do
    case $f in
        gdb/testsuite/gdb.xml/tdesc-bogus.xml)
	    # Malformed.
	    continue
	    ;;
	gdb/testsuite/gdb.xml/bad-include.xml \
	    | gdb/testsuite/gdb.xml/core-only.xml \
	    | gdb/testsuite/gdb.xml/extra-regs.xml \
	    | gdb/testsuite/gdb.xml/loop.xml)
            # Don't check includes for:
            # - gdb.xml/bad-include.xml (XInclude error: nonexistent.xml)
            # - gdb.xml/{core-only,extra-regs}.xml (XInclude error: core-regs.xml)
            # - gdb.xml/loop.xml (XInclude error: loop.xml)
	    check_basic="$check_basic $f"
	    ;;
	*)
	    check_includes="$check_includes $f"
	    ;;
    esac
done

st1=0
if [ "$check_basic" != "" ]; then
    xmllint --noout \
	    $check_basic \
	    > "$tmp" \
	    2>&1; st1=$?
fi

st2=0
if [ "$check_includes" != "" ]; then
    # Provide the --path parts to let xmllint find the .dtd files.
    xmllint --noout --xinclude --path gdb/syscalls --path gdb/features \
	    $check_includes \
	    >> "$tmp" \
	    2>&1; st2=$?
fi

if [ $st1 -eq 0 ] && [ $st2 -eq 0 ]; then
    st=0
else
    st=1
fi

if [ -s "$tmp" ]; then
    cat "$tmp"
    if [ $st -eq 0 ]; then
	st=1
    fi
fi

exit $st
