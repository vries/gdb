/* Variant of std::optional that works around GCC Wmaybe-uninitialized false
   positives.

   Copyright (C) 2026 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_GDB_OPTIONAL_H
#define GDBSUPPORT_GDB_OPTIONAL_H

#include <optional>
#include <utility>
#include "diagnostics.h"

namespace gdb {

template<class T>
class optional: public std::optional<T>
{
public:
  using std::optional<T>::optional;

  /* Work around Wmaybe-uninitialized false positives.  Wrap the destructor
     in order to wrap the call to std::~optional.  */
  //#if defined (__GNUC__) && !defined (__clang__)
  DIAGNOSTIC_PUSH
  DIAGNOSTIC_IGNORE ("-Wmaybe-uninitialized")
  //#endif
  ~optional ()
  {
  }
  //#if defined (__GNUC__) && !defined (__clang__)
  DIAGNOSTIC_POP
  //#endif
};

} /* namespace gdb */

#endif /* GDBSUPPORT_GDB_OPTIONAL_H */
