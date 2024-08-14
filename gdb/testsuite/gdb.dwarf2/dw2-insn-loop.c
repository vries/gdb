/*
   Copyright 2024 Free Software Foundation, Inc.

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

static int loop = 5;

static int
foo (void)
{ /* foo start.  */
  asm ("foo_label: .globl foo_label");
  return --loop; /* foo return.  */
}

int
main (void)
{ /* main start.  */
  asm ("main_label: .globl main_label");

  while (foo ()) /* main while loop.  */
    ;

  asm ("main_label_2: .globl main_label_2");
  return 0; /* main return.  */
}
