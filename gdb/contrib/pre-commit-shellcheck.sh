#!/bin/bash

set -o pipefail

always_pass=false
if [ "$1" = --always-pass ]; then
    always_pass=true
    shift
fi

awk_prog=$(mktemp)
trap 'rm -f "$awk_prog"' EXIT

cat > "$awk_prog" <<EOF
BEGIN{
  do_print=0
}
//{
  if (do_print) print
}
/- exit code: 1/{
  do_print=1
}
EOF

status=0
if ! pre-commit \
     run \
     -c gdb/contrib/pre-commit-shellcheck.yaml \
     shellcheck \
     --files "$@" \
	| awk -f "$awk_prog"; then
    status=1
fi

if $always_pass; then
    exit 0
fi

exit $status
