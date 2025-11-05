#!/bin/sh

# Copyright (C) 2025 Free Software Foundation, Inc.
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

set -e

scriptdir=$(cd "$(dirname "$0")" || exit 1; pwd -P)

tmp=""
patch=""
tmpdir=""

cleanup()
{
    for f in "$tmp" "$patch" "$tmpdir"; do
	if [ "$f" = "" ]; then
	    continue
	fi
	if [ -d "$f" ]; then
	    rm -Rf "$tmp"
	else
	    rm -f "$tmp"
	fi
    done
}

# Schedule cleanup.
trap cleanup EXIT

# Get temporary file and directory.
tmp=$(mktemp)
patch=$(mktemp)
tmpdir=$(mktemp -d)

# Generate initial patch.
git diff --staged -- "$@" \
    > "$tmp"

for f in $(git diff --staged --name-only -- "$@"); do

    # Create dir.
    dir=$(dirname "$f")
    mkdir -p "$tmpdir/$dir"

    # Create post file.
    cp "$f" "$tmpdir/$f.post"

    # Create pre file.
    cp "$f" "$tmpdir/$f"
done

(
    cd "$tmpdir"
    patch -p1 -R \
	  < "$tmp" \
	  > /dev/null
)

for f in $(git diff --staged --name-only -- "$@"); do
    if [ -f "$tmpdir/$f" ]; then
	mv "$tmpdir/$f" "$tmpdir/$f.pre"
    else
	touch "$tmpdir/$f.pre"
    fi

    # Apply filter to pre and post files.
    for p in pre post; do
	"$scriptdir/filter-c.py" \
	    "$tmpdir/$f.$p" \
	    > "$tmpdir/$f.$p.filtered"
    done

    # Generate filtered patch.
    (
	cd "$tmpdir"
	diff -U3 \
	     "$f.pre.filtered" \
	     "$f.post.filtered" \
	     >> "$patch" \
	    || true
    )
done

# Verify patch.  Ignore exit status.
"$scriptdir"/../../contrib/check_GNU_style.py "$patch" \
    || true
