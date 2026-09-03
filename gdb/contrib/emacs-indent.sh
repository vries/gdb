#!/bin/bash

mode="$1"
shift

if [ "$mode" = "" ]; then
    echo "Missing mode argument"
    exit 1
fi

kludge=false
case "$mode" in
    tcl-mode)
	kludge=true
	;;
    *)
	echo "Unhandled mode: $mode"
	exit 1
	;;
esac

if [ $# -eq 0 ]; then
    echo "No files"
    exit 1
fi

if ! emacs --version > /dev/null 2>&1; then
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
    exit 0
fi

tmp=""
tmp_files=()

cleanup()
{
    if [ "$tmp" != "" ]; then
	rm -f "$tmp"
    fi

    if [ ${#tmp_files[@]} -ne 0 ]; then
	rm -f "${tmp_files[@]}"
    fi
}

# Schedule cleanup.
trap cleanup EXIT

# Get temporary file.
tmp=$(mktemp) || exit 1

# If a comment contains some code example, and that example is indented using
# tabs, then re-indenting that comment may break the formatting of the code
# example.  Fix this by expanding those tabs into spaces.
mapfile -t tab_in_comment_files < <(grep -E -l $'^[ \t]*#.*\t' "${files[@]}")
for f in "${tab_in_comment_files[@]}"; do
awk '
/^[ \t]*#/ {
    result=""
    in_comment=0
    pos=0
    for (i = 1; i <= length($0); i++) {
	c=substr($0, i, 1)
	if (in_comment == 0 && c == "#") {
	    in_comment = 1
	}
	if (in_comment == 0) {
	    result = result c
	    if (c == "\011") {
		nr = 8 - (pos % 8)
		for (s = 0; s < nr; ++s) {
		    pos = pos + 1
		}
	    } else {
		pos=pos+1
	    }
	} else {
	    if (c == "\011") {
		nr = 8 - (pos % 8)
		for (s = 0; s < nr; ++s) {
		    result = result " "
		    pos = pos + 1
		}
	    } else {
		result = result c
		pos = pos + 1
	    }
	}
    }
    print result
    next
}
// {
   print
}
' "$f" > "$tmp"
    mv "$tmp" "$f"
done

if $kludge; then
    # Stop emacs from changing this:
    # ...
    # Try this command:
    #     foo \
    #         arg1 \
    #         arg2
    # ...
    # into:
    # ...
    #    # Try this command:
    #    #     foo \
    #        #         arg1 \
    #        #         arg2
    # ...
    # by adding the string <KLUDGE> after such lines before running emacs, and
    # removing it afterwards.

    # First, find out if any files need the kludge.
    mapfile -t kludge_files < <(grep -E -l $'^[ \t]*#.*\\\\$' "${files[@]}")

    if [ ${#kludge_files[@]} -ne 0 ]; then

	# Find out which files don't need the kludge.
	nokludge_files=()
	for f in "${files[@]}"; do
	    found=false
	    for k in "${kludge_files[@]}"; do
		if [ "$f" == "$k" ]; then
		    found=true
		    break
		fi
	    done
	    if ! $found; then
		nokludge_files=("${nokludge_files[@]}" "$f")
	    fi
	done

	# Use temporary files to make cleanup on exit easier.
	for f in "${kludge_files[@]}"; do
	    tmp_file=$(mktemp) || exit 1
	    cp "$f" "$tmp_file"
	    tmp_files=("${tmp_files[@]}" "$tmp_file")
	done
	files=("${nokludge_files[@]}" "${tmp_files[@]}")

	# Apply the kludge.
	sed \
	    -i \
	    's%^\([ \t]*#.*\)\\$%\1\\<KLUDGE>%' \
	    "${tmp_files[@]}" \
	    || exit 1

    fi
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
    # Output is verbose; only show on error.
    cat "$tmp"
    exit 1
fi

if $kludge; then
    if [ ${#kludge_files[@]} -ne 0 ]; then
	# Revert the kludge.
	sed \
	    -i \
	    's%^\([ \t]*#.*\)\\<KLUDGE>$%\1\\%' \
	    "${tmp_files[@]}" \
	    || exit 1

	# Update the files needing the kludge.
	for ((i=0; i < ${#kludge_files[@]}; i++)); do
	    cp "${tmp_files[$i]}" "${kludge_files[$i]}"
	done
    fi
fi
