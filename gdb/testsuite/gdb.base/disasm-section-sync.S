/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2026 Free Software Foundation, Inc.

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

	.section        .text_1,"ax",@progbits
	.globl  foo_1
	.type   foo_1, @function
foo_1:
	endbr64
	sub    $0x8,%rsp
	mov    0x2fd1(%rip),%rax
	test   %rax,%rax
	je     401016
	call   *%rax
	add    $0x8,%rsp
	ret

	# Get instruction decoding out of sync.
	.byte 0

	.section        .text_2,"ax",@progbits
	.globl  foo_2
	.type   foo_2, @function
foo_2:
	push   0x2fca(%rip)
	jmp    *0x2fcc(%rip)
	nopl   0x0(%rax)

	.globl  foo_3
	.type   foo_3, @function
foo_3:
	nop

	.section        .note.GNU-stack,"",@progbits
