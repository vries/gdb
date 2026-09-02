#!/bin/bash

f2=()
f4=()

for f in "$@"; do
    # The gdb/config/djgpp/* pattern matches the explicitly mentioned
    # djcheck.sh and djconfig.sh.  Allow this.
    # shellcheck disable=SC2221,SC2222
    case "$f" in
	*/configure)
	    # Generated.
	    continue
	    ;;
	gdb/config/djgpp/*)
	    # For now, these scripts prefer `...` over $(...). See
	    # gdb/config/djgpp/.shellcheckrc.
	    # Shfmt automatically rewrites to $(...), so skip these.
	    continue
	    ;;
	gdb/config/djgpp/djcheck.sh)
	    # Mixed 2/4 indentation.
	    continue
	    ;;
	gdb/config/djgpp/djconfig.sh | \
	    gdb/contrib/expect-read1.sh | \
	    gdb/features/feature_to_c.sh | \
	    gdb/gdb_buildall.sh)
	    f2=("${f2[@]}" "$f")
	    ;;
	*)
	    f4=("${f4[@]}" "$f")
	    ;;
    esac
done

with_indent()
{
    indent="$1"
    shift

    if [ $# -eq 0 ]; then
	return
    fi

    shfmt \
	--language-dialect=auto \
	--indent="$indent" \
	--func-next-line \
	--space-redirects \
	--case-indent \
	--binary-next-line \
	--write \
	"$@"
}

with_indent 2 "${f2[@]}"
with_indent 4 "${f4[@]}"

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

for f in "${f2[@]}" "${f4[@]}"; do
    unexpand \
	--first-only \
	--tabs=8 \
	"$f" \
	> "$tmp"

    # Use cat to preserve permissions on $f.
    cat "$tmp" > "$f"
done
