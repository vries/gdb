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
check_valid=""
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
	gdb/features/aarch64-core.xml \
	    | gdb/features/aarch64-fpmr.xml \
	    | gdb/features/aarch64-fpu.xml \
	    | gdb/features/alpha-core.xml \
	    | gdb/features/alpha.xml \
	    | gdb/features/arc/v1-aux.xml \
	    | gdb/features/arc/v1-core.xml \
	    | gdb/features/arc/v2-aux.xml \
	    | gdb/features/arc/v2-core.xml \
	    | gdb/features/cskyv2-linux.xml \
	    | gdb/features/i386/32bit-linux.xml \
	    | gdb/features/i386/64bit-avx512.xml \
	    | gdb/features/or1k-core.xml \
	    | gdb/features/or1k-linux.xml \
	    | gdb/features/or1k.xml \
	    | gdb/features/rs6000/powerpc-403gc.xml \
	    | gdb/features/rs6000/powerpc-403.xml \
	    | gdb/features/rs6000/powerpc-405.xml \
	    | gdb/features/rs6000/powerpc-505.xml \
	    | gdb/features/rs6000/powerpc-601.xml \
	    | gdb/features/rs6000/powerpc-602.xml \
	    | gdb/features/rs6000/powerpc-603.xml \
	    | gdb/features/rs6000/powerpc-604.xml \
	    | gdb/features/rs6000/powerpc-750.xml \
	    | gdb/features/rs6000/powerpc-860.xml \
	    | gdb/features/rx.xml \
	    | gdb/features/s390-gs-linux64.xml \
	    | gdb/features/s390-te-linux64.xml \
	    | gdb/features/s390-tevx-linux64.xml \
	    | gdb/features/s390-vx-linux64.xml \
	    | gdb/features/s390x-gs-linux64.xml \
	    | gdb/features/s390x-te-linux64.xml \
	    | gdb/features/s390x-tevx-linux64.xml \
	    | gdb/features/s390x-vx-linux64.xml \
	    | gdb/syscalls/freebsd.xml \
	    | gdb/syscalls/netbsd.xml \
	    | gdb/testsuite/gdb.xml/inc-2.xml \
	    | gdb/testsuite/gdb.xml/inc-body.xml \
	    | gdb/testsuite/gdb.xml/includes.xml \
	    | gdb/testsuite/gdb.xml/maint_print_struct.xml \
	    | gdb/testsuite/gdb.xml/maint-xml-dump-02.xml \
	    | gdb/testsuite/gdb.xml/maint-xml-dump-03.xml \
	    | gdb/testsuite/gdb.xml/single-reg.xml \
	    | gdb/testsuite/gdb.xml/tdesc-unknown.xml \
	    | gdb/testsuite/gdb.xml/trivial.xml \
	    | gdb/syscalls/apply-defaults.xsl \
	    | gdb/features/print-osabi.xsl \
	    | gdb/features/number-regs.xsl \
	    | gdb/features/gdbserver-regs.xsl \
	    | gdb/features/sort-regs.xsl \
	    | gdb/doc/stack_frame.svg)
	    # Files that are not clean with --valid.
	    check_includes="$check_includes $f"
	    ;;
	*)
	    check_valid="$check_valid $f"
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

st3=0
if [ "$check_valid" != "" ]; then
    # Provide the --path parts to let xmllint find the .dtd files.
    xmllint --noout --xinclude --path gdb/syscalls --path gdb/features \
	    --valid \
	    $check_valid \
	    >> "$tmp" \
	    2>&1; st3=$?
fi

if [ $st1 -eq 0 ] && [ $st2 -eq 0 ] && [ $st3 -eq 0 ]; then
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
