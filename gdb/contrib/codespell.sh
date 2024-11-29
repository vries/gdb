#!/bin/bash

# Script to call codespell using pre-commit.

# Copyright (C) 2024 Free Software Foundation, Inc.
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

usage ()
{
    local prog
    prog=$(basename "$0")

    echo "Usage: $prog <codespell files>"
    echo "       $prog <codespell args> -- <codespell files>"
    echo "       $prog <local args> -- <codespell args> -- <codespell files>"
    echo "where <local args> are:"
    echo "  --always-pass: use exit status 0 if spell check fails"
    echo "  --staged     : Only check staged changes"
}

parse_local_args ()
{
    while [ $# -gt 0 ]; do
	case " $1 " in
	    " --always-pass ")
		always_pass=true
		;;
	    " --staged ")
		staged=true
		;;
	    *)
		echo "Don't know how to handle local arg: $1"
		exit 1
		;;
	esac

	shift
    done
}

parse_args ()
{
    if [ $# -eq 0 ]; then
	usage
	exit 1
    fi

    local args1
    declare -a args1
    local args2
    declare -a args2
    local args3
    declare -a args3

    args1=()
    args2=()
    args3=()

    dash_dash=0
    while [ $# -gt 0 ]; do
	case " $1 " in
	    " -- ")
		dash_dash=$((dash_dash + 1))
		if [ $dash_dash -eq 3 ]; then
		    usage
		    exit 1
		fi
		;;
	    *)
		if [ $dash_dash -eq 0 ]; then
		    args1=("${args1[@]}" "$1")
		elif [ $dash_dash -eq 1 ]; then
		    args2=("${args2[@]}" "$1")
		else
		    args3=("${args3[@]}" "$1")
		fi
		;;
	esac

	shift
    done

    if [ $dash_dash -eq 2 ]; then
	parse_local_args "${args1[@]}"
	args=("${args2[@]}")
	files=("${args3[@]}")
    elif [ $dash_dash -eq 1 ]; then
	args=("${args1[@]}")
	files=("${args2[@]}")
    else
	files=("${args1[@]}")
    fi
}

python_list ()
{
    echo -n "[ "

    local first
    first=true
    for arg in "$@"; do
	if $first; then
	    first=false
	else
	    echo -n ", "
	fi
	echo -n "\"$arg\""
    done

    echo -n " ]"
}

gen_cfg ()
{
    cat > "$1" <<EOF
repos:
- repo: https://github.com/codespell-project/codespell
  rev: v2.3.0
  hooks:
  - id: codespell
    args: $(python_list "${args[@]}")
EOF
}

gen_awk_prog ()
{
    cat > "$1" <<EOF
BEGIN{
  do_print=0
}
/^$/{
  # Skip empty lines.
  next
}
//{
  if (do_print) print
}
/- exit code: [0-9][0-9]*/{
  do_print=1
}
EOF
}

main ()
{
    set -o pipefail

    # Local options.
    always_pass=false
    staged=false

    declare -a files
    declare -a args
    parse_args "$@"

    local cfg
    cfg=$(mktemp)
    local awk_prog
    awk_prog=$(mktemp)
    local output
    output=$(mktemp)

    declare -a tmp_files
    tmp_files=("$cfg" "$awk_prog" "$output")

    trap 'rm -f "${tmp_files[@]}"' EXIT

    gen_cfg "$cfg"
    gen_awk_prog "$awk_prog"

    declare -A org_files
    declare -a staged_parts
    if $staged; then
	local f
	for f in "${files[@]}"; do
	    local tmp
	    tmp=$(mktemp)
	    tmp_files=("${tmp_files[@]}" "$tmp")
	    staged_parts=("${staged_parts[@]}" "$tmp")
	    org_files[$tmp]="$f"

	    git diff --staged --minimal "$f" \
		| tail -n +5 \
		| grep -E "^\+" \
		| sed 's/^\+//' \
		      > "$tmp"
	done
	files=("${staged_parts[@]}")
    fi

    local status
    if pre-commit \
	   run \
	   -c "$cfg" \
	   codespell \
	   --files "${files[@]}" \
	   > "$output" \
	   2>&1; then
	status=0
    else
	status=$?
    fi

    if [ $status -eq 0 ]; then
	exit 0
    fi

    local exit_code
    exit_code=$(grep "\- exit code: " "$output" | sed 's/.*: //')

    if [ "$exit_code" = "" ]; then
	echo "error: Could not find exit code"
	cat "$output"
	exit 1
    fi

    if [ "$exit_code" -eq 65 ]; then
	# Spell check failed.  Filter out pre-commit output.
	if $staged; then
	    # Map staged_parts files back to original files.
	    for f in "${files[@]}"; do
		sed -i "s%^.*$f:[0-9][0-9]*: %${org_files[$f]}: %" "$output"
	    done
	fi
	awk -f "$awk_prog" "$output"

	if $always_pass; then
	    exit 0
	fi
    else
	# Something else went wrong, show full output.
	cat "$output"
    fi

    exit "$exit_code"
}

main "$@"
