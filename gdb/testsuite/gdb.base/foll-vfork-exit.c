/* This testcase is part of GDB, the GNU debugger.

   Copyright 1997-2021 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <unistd.h>

int
main ()
{
  int pid;

  /* A statement before vfork to make sure a breakpoint on main isn't
     set on vfork below.  */
  pid = 1;
  pid = vfork (); /* VFORK */
  if (pid == 0)
    {
      const char *s = "I'm the child!";
#if DEBUG
      printf ("%s\n", s);
#else
    const char *volatile v = s;
#endif
      _exit (0);
    }
  else
    {
      const char *s = "I'm the proud parent of child";
#if DEBUG
      printf ("%s #%d!\n", s, pid);
#else
    const char *volatile v = s;
#endif
    }

  return 0;
}
