# Copyright (C) 2025 Free Software Foundation, Inc.

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

from importlib import import_module


def verify_import_all(modulename, globals):
    module = import_module(modulename, package=__package__)
    missing_symbols = [
        symbol
        for symbol in module.__dict__
        if not symbol.startswith("_") and symbol not in globals
    ]
    if len(missing_symbols) != 0:
        print("Missing import:")
        print("from %s import (" % modulename)
        for symbol in missing_symbols:
            print("    %s," % symbol)
        print(")")
    assert len(missing_symbols) == 0
