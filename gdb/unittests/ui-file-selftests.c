/* Self tests for ui_file

   Copyright (C) 2022-2026 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "gdbsupport/scoped_restore.h"
#include "gdbsupport/selftest.h"
#include "ui-file.h"

namespace selftests {
namespace file {

static void
check_one (const char *str, int quoter, const char *result)
{
  string_file out;
  out.putstr (str, quoter);
  SELF_CHECK (out.string () == result);
}

static void
run_tests ()
{
  check_one ("basic stuff: \\", '\\',
	     "basic stuff: \\\\");
  check_one ("more basic stuff: \\Q", 'Q',
	     "more basic stuff: \\\\\\Q");
  check_one ("more basic stuff: \\Q", '\0',
	     "more basic stuff: \\Q");

  check_one ("weird stuff: \x1f\x90\n\b\t\f\r\033\007", '\\',
	     "weird stuff: \\037\\220\\n\\b\\t\\f\\r\\e\\a");

  scoped_restore save_7 = make_scoped_restore (&sevenbit_strings, true);
  check_one ("more weird stuff: \xa5", '\\',
	     "more weird stuff: \\245");

  {
    /* There's a bug that has the effect "*redirectable_stderr () == nullptr".
       In that case, gdb_stderr is not nullptr, but the underlying pointer is
       a nullptr.  Check that "gdb_stderr->fd ()" doesn't dereference the
       underlying nullptr.
       This allows us to check for a usable gdb_stderr using
       "gdb_stderr != nullptr && gdb_stderr->fd () == -1", as we do in
       sig_write.  */
    scoped_restore restore_stderr
      = make_scoped_restore (redirectable_stderr (), nullptr);
    SELF_CHECK (gdb_stderr != nullptr);
    SELF_CHECK (gdb_stderr->fd () == -1);
  }
}

} /* namespace file*/
} /* namespace selftests */

INIT_GDB_FILE (ui_file_selftest)
{
  selftests::register_test ("ui-file",
			    selftests::file::run_tests);
}
