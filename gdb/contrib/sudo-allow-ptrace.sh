#!/bin/sh

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

# This script intends to facilitate using gdb to attach to processes
# on a kernel.yama.ptrace_scope=1 system, without changing the setting and
# without becoming root.
#
# Example usage (running the gdb testsuite):
#   $ cd build/gdb/testsuite
#   $ sudo-allow-ptrace.sh make check
#
# Example usage (using gdb to attach to process):
#   $ sudo-allow-ptrace.sh gdb -p <pid>
#
# The script is based on this [1] recipe.
#
# [1] https://wiki.archlinux.org/title/Capabilities.

set -e

case " $1 " in
    " --stage2 ")
	stage=2
	shift
	;;

    " --stage3 ")
	stage=3
	shift
	;;

    *)
	stage=1
	;;
esac

if [ $stage = 1 ]; then
    # STAGE 1, as user $USER.

    # shellcheck disable=SC3045
    ulimit_core_hard=$(ulimit -Hc)
    # shellcheck disable=SC3045
    ulimit_core_soft=$(ulimit -Sc)

    exec \
	sudo -E \
	"$0" \
	--stage2 \
	"$USER" \
	"$HOME" \
	"$ulimit_core_hard" \
	"$ulimit_core_soft" \
	"$@"
elif [ $stage = 2 ]; then
    # STAGE 2, as user root.

    export user="$1"
    shift

    export home="$1"
    shift

    ulimit_core_hard="$1"
    shift

    ulimit_core_soft="$1"
    shift

    # shellcheck disable=SC3045
    ulimit -Hc "$ulimit_core_hard"
    # shellcheck disable=SC3045
    ulimit -Sc "$ulimit_core_soft"

    exec \
	capsh \
	--caps="cap_setpcap,cap_setuid,cap_setgid+ep cap_sys_ptrace+eip" \
	--keep=1 \
	--user="$user" \
	--addamb="cap_sys_ptrace" \
	--shell="$0" \
	-- \
	--stage3 \
	"$user" \
	"$home" \
	"$@"
elif [ $stage = 3 ]; then
    # STAGE 3, as user root with "assumed identity" $USER.

    export USER="$1"
    export LOGNAME="$1"
    shift

    export HOME="$1"
    shift

    exec "$@"
fi
