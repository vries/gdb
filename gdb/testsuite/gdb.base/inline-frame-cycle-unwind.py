# Copyright (C) 2021-2026 Free Software Foundation, Inc.

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

import re

import gdb
from gdb.unwinder import Unwinder

# Set this to the stack level the backtrace should be corrupted at.
# This will only work for frame 1, 3, or 5 in the test this unwinder
# was written for.
stop_at_level = None

# List of FrameId instances, one for each stack frame.
frame_ids = []


def print_frame_ids():
    for i in range(len(frame_ids)):
        fid = frame_ids[i]
        print("frame-id for frame #%s: {stack=0x%x,code=0x%x}" % (i, fid.sp, fid.pc))


class FrameId(object):
    def __init__(self, sp, pc):
        self._sp = sp
        self._pc = pc

    @property
    def sp(self):
        return self._sp

    @property
    def pc(self):
        return self._pc


class TestUnwinder(Unwinder):
    def __init__(self):
        Unwinder.__init__(self, "stop at level")

    def __call__(self, pending_frame):
        if stop_at_level is None or pending_frame.level() != stop_at_level:
            return None

        if len(frame_ids) < stop_at_level:
            raise gdb.GdbError("not enough parsed frame-ids")

        if stop_at_level not in [1, 3, 5]:
            raise gdb.GdbError("invalid stop_at_level")

        # We're unwinding frame #n (n == pending_frame.level()), and are
        # expected to return:
        # - a frame ID for frame #n, and
        # - the register values for frame #(n+1), as saved in frame #n
        #
        # Instead, we return the frame ID for frame #n, and the register
        # values for frame #n.
        #
        # This will cause the backtrace stop at level #n, meaning:
        # - the entry for frame #n will be printed.
        # - the entry for frame #(n+1) will not be printed, instead
        #   "Backtrace stopped: <reason>" will be printed.
        unwinder = pending_frame.create_unwind_info(frame_ids[stop_at_level])

        for reg in pending_frame.architecture().registers("general"):
            val = pending_frame.read_register(reg)
            # Having unavailable registers leads to a fall back to the standard
            # unwinders.  Don't add unavailable registers to avoid this.
            if str(val) == "<unavailable>":
                continue
            unwinder.add_saved_register(reg, val)
        return unwinder


gdb.unwinder.register_unwinder(None, TestUnwinder(), True)

# When loaded, it is expected that the stack looks like:
#
#   main -> normal_func -> inline_func -> normal_func -> inline_func -> normal_func -> inline_func
#
# Iterate through frames 0 to 6, parse their frame-id and store it
# into the global FRAME_IDS list.
for i in range(7):
    # Get the frame-id in a verbose text form.
    output = gdb.execute("maint print frame-id %d" % i, to_string=True)

    # Parse the frame-id in OUTPUT, find the stack and code addresses.
    match = re.search(r"stack=(0x[0-9a-fA-F]+).*?code=(0x[0-9a-fA-F]+)", output)
    if not match:
        raise gdb.GdbError("Could not parse frame-id for frame #%d" % i)

    # Create the FrameId object.
    sp_addr = int(match.group(1), 16)
    pc_addr = int(match.group(2), 16)
    frame_ids.append(FrameId(sp_addr, pc_addr))
