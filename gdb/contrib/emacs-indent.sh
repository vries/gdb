#!/bin/bash

mode="$1"
shift

if [ "$mode" = "" ]; then
    echo "Missing mode argument"
    exit 1
fi

if [ $# -eq 0 ]; then
    echo "No files"
    exit 1
fi

if ! emacs --version > /dev/null; then
    echo "Please install emacs"
    exit 1
fi

files=()
for f in "$@"; do
    case "$mode" in
	tcl-mode)
	    case "$f" in
		# Imported.
		gdb/testsuite/lib/ton.tcl)
		    continue
		    ;;
	    esac
	    ;;
    esac

    files=("${files[@]}" "$f")
done

if [ ${#files[@]} -eq 0 ]; then
    exit
fi

tmp=""

cleanup()
{
    if [ "$tmp" != "" ]; then
	rm -f "$tmp"
    fi
}

# Schedule cleanup.
trap cleanup EXIT

# Get temporary file.
tmp=$(mktemp) || exit 1

if [ "$mode" = "tcl-mode" ]; then
    # Kludge: Hide backslashes at end of comment from emacs tcl-mode, by
    # appending '#'.
    sed \
	-i \
	's%^\([ \t]*#.*\)\\$%\1\\#%' \
	"${files[@]}" \
	|| exit 1
fi

script="
(dolist
 (f command-line-args-left)
 (with-current-buffer
  (find-file-noselect f)
  ($mode)
  (indent-region (point-min) (point-max))
  (save-buffer)
  (kill-buffer)))"

if ! emacs \
     -batch \
     --eval="$script" \
     "${files[@]}" \
     > "$tmp" \
     2>&1; then
    # Output is verbose, only show on error.
    cat "$tmp"
    exit 1
fi
