#!/bin/bash

mode="$1"
shift

kludge=false
case "$mode" in
    "")
	echo "Missing mode argument"
	exit 1
	;;
    sh-mode)
	kludge=true
	;;
    *)
	echo "Unknown mode: $mode"
	exit 1
	;;
esac

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
	sh-mode)
	    case "$f" in
		# Generated.
		*/configure)
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

if $kludge; then
    # Kludge: Hide backslashes at end of comment from emacs, by
    # appending '#'.
    # We could do this temporarily before using emacs, and undoing it
    # afterwards, but users can run into the same problem when editing so we
    # make the change permanent.
    # This is a kludge, because we use sed which has no idea about shell
    # syntax, and consequently also triggers inside here documents.
    sed \
	-i \
	's%^\([ \t]*#.*\)\\$%\1\\#%' \
	"${files[@]}" \
	|| exit 1
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
