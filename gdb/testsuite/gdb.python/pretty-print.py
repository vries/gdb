# Copyright (C) 2022-2026 Free Software Foundation, Inc.

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


import gdb


class MytypePrinter:
    """pretty print my type"""

    def __init__(self, val):
        self.val = val

    def children(self):
        yield "p", self.val["p"].dereference()

    def to_string(self):
        return "pp(" + str(self.val["p"]) + ")"


def s1s2_lookup_function(val):
    typ = val.type
    if typ.code == gdb.TYPE_CODE_REF:
        typ = typ.target()
    if typ.code == gdb.TYPE_CODE_PTR:
        typ = typ.target()
    if str(typ) == "struct s1" or str(typ) == "struct s2":
        return MytypePrinter(val)
    return None


gdb.pretty_printers.append(s1s2_lookup_function)
