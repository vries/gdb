#!/bin/bash

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

# Include file for spellcheck.sh.  Not taken into account for invalidation of
# cached files.

ignore_word_in_file ()
{
    local word
    word="$1"

    local file
    file="$2"

    case " $word " in
	" onot ")
	    case $file in
		gdb/ada-lang.c)
		    return 1
		;;
	    esac
	    ;;
    esac

    return 0
}
