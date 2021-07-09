# Copyright (C) 2021 Free Software Foundation, Inc.

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

# Test that we can access memory while all the threads of the inferior
# are running, and even if:
#
# - the leader thread exits
# - the selected thread exits
#
# This test constantly spawns short lived threads to make sure that on
# systems with debug APIs that require passing down a specific thread
# to work with (e.g., GNU/Linux ptrace and /proc filesystem), GDB
# copes with accessing memory just while the thread it is accessing
# memory through exits.
#
# The test spawns two processes and alternates memory accesses between
# them to force flushing per-process caches.  At the time of writing,
# the Linux backend accesses inferior memory via /proc/<pid>/mem, and
# keeps one such file open, as a cache.  Alternating inferiors forces
# opening such file for a different process, which fails if GDB tries
# to open the file for a thread that exited.  The test does ensures
# those reopen/fail code paths are exercised.

standard_testfile access-mem-running-thread-exit.c

if {[build_executable "failed to prepare" $testfile $srcfile {debug pthreads}] == -1} {
    return -1
}

clean_restart

gdb_test_no_output "set non-stop $non_stop"

gdb_load $binfile

if ![runto_main] {
    fail "cannot run to main"
    return -1
}

# If debugging with target remote, check whether the all-stop variant
# of the RSP is being used.  If so, we can't run the background tests.
if {!$non_stop
    && [target_info exists gdb_protocol]
    && ([target_info gdb_protocol] == "remote"
	|| [target_info gdb_protocol] == "extended-remote")} {

    gdb_test_multiple "maint show target-non-stop" "" {
	-wrap -re "(is|currently) on.*" {
	}
	-wrap -re "(is|currently) off.*" {
	    unsupported "can't issue commands while target is running"
	    return 0
	}
    }
}

delete_breakpoints

# Start the second inferior.
with_test_prefix "second inferior" {
    gdb_test "add-inferior -no-connection" "New inferior 2.*"
    gdb_test "inferior 2" "Switching to inferior 2 .*"

    gdb_load $binfile

    if ![runto_main] {
	fail "cannot run to main"
	return -1
    }
}

delete_breakpoints

# These put too much noise in the logs.
#gdb_test_no_output "set print thread-events off"

# Continue all threads of both processes.
gdb_test_no_output "set schedule-multiple on"
if {$non_stop == "off"} {
    set cmd "continue &"
} else {
    set cmd "continue -a &"
}
gdb_test_multiple $cmd "continuing" {
    -re "Continuing\.\r\n$gdb_prompt " {
	pass $gdb_test_name
    }
}

# Like gdb_test, but:
# - don't issue a pass on success.
# - on failure, clear the ok variable in the calling context, and
#   break it.
proc my_gdb_test {cmd pattern message} {
    global gdb_prompt

    upvar inf inf
    upvar iter iter
    set res2 0
    set res [gdb_test_multiple $cmd "access mem ($message, inf=$inf, iter=$iter)" {
	-re "$pattern.*\r\n$gdb_prompt " {
	}
	-re "\r\n$gdb_prompt " {
	    fail $gdb_test_name
	    set res2 -1
	}
    }]

    if { $res != 0 || $res2 != 0 } {
	uplevel 1 {set ok 0}
	return -code break
    }
}

gdb_test_multiple "set debug lin-lwp-mem 1" "" {
	-re "\r\n$gdb_prompt " {
	}
}

# Hammer away for 30 minutes, alternating between inferiors.
set second 1000
set minute [expr 60 * $second]
set duration [expr 30 * $minute]

# Setup watchdog.
set ::done 0
after $duration { set ::done 1 }

set inf 1
set ok 1
set iter 0
while {!$::done && $ok} {
    incr iter
    verbose -log "xxxxx: iteration $iter"
    gdb_test_multiple "info threads" "" {
	-re "\r\n$gdb_prompt " {
	}
    }

    if {$inf == 1} {
	set inf 2
    } else {
	set inf 1
    }

    my_gdb_test "inferior $inf" ".*" "inferior $inf"

    my_gdb_test "print global_var = 555" " = 555" \
	"write to global_var"
    my_gdb_test "print global_var" " = 555" \
	"print global_var after writing"
    my_gdb_test "print global_var = 333" " = 333" \
	"write to global_var again"
    my_gdb_test "print global_var" " = 333" \
	"print global_var after writing again"
}

if {$ok} {
    pass "access mem"
}

gdb_test_multiple "set debug lin-lwp-mem 0" "" {
	-re "\r\n$gdb_prompt " {
	}
}

gdb_test_multiple "kill inferiors 1" "" {
	-re "\r\n$gdb_prompt " {
	}
}

gdb_test_multiple "kill inferiors 2" "" {
	-re "\r\n$gdb_prompt " {
	}
}
