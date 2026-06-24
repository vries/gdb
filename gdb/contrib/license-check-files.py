#! /usr/bin/env python3

import hashlib
import json
import os
import re
import subprocess
import sys

db_tmp_file = ".git/gdb-licenses.tmp.json"
db_file = ".git/gdb-licenses.json"

COMMON_LICENSES = ["GPL-2.0-or-later", "GPL-3.0-or-later"]

# We ignore ChangeLogs because they're unmaintained.  We ignore .gitignore and
# .gitattributes because scancode does so.
re_ignore = re.compile(r"ChangeLog|\.(gitignore|gitattributes)$")

table_gdb_re = {
    re.compile(r"gdb/features/.*\.(xml(\.in)?|dtd)$"): [
        "FSFAP-no-warranty-disclaimer",
        "FSFAP",
    ],
    re.compile(r"gdb/features/.*\.c$"): ["None"],
    re.compile(r"gdb/regformats/.*\.dat$"): ["None"],
    re.compile(r"gdb/syscalls/.*\.(xml(\.in)?|dtd)$"): [
        "FSFAP-no-warranty-disclaimer",
        "FSFAP",
    ],
    re.compile(r"gdb/syscalls/.*\.sh$"): [
        "GPL-3.0-or-later AND FSFAP-no-warranty-disclaimer"
    ],
    re.compile(r"gdb/stubs/.*\.c$"): ["LicenseRef-scancode-public-domain-disclaimer"],
}

table_gdb = {
    "gdb/aclocal.m4": "FSFULLRWD AND FSFULLR",
    "gdb/acx_configure_dir.m4": "FSFULLR",
    "gdb/ax_cxx_compile_stdcxx.m4": "FSFAP",
    "gdb/configure": "FSFUL AND GPL-3.0-only",
    "gdb/COPYING": "GPL-3.0-only",
    "gdb/copying.c": "GPL-3.0-only",
    "gdb/exc_request.defs": "CMU-Mach",
    "gdb/jit-reader.in": "GPL-3.0-or-later AND LicenseRef-scancode-unknown-license-reference",
    "gdb/NEWS": "BSD-3-Clause AND GFDL-1.1-or-later AND LGPL-2.0-or-later AND (LicenseRef-scancode-other-copyleft AND LGPL-2.0-or-later AND GPL-1.0-or-later)",
    "gdb/README": "GPL-1.0-or-later",
    "gdb/top.c": "GPL-3.0-or-later AND (GPL-3.0-or-later AND LicenseRef-scancode-warranty-disclaimer) AND GPL-1.0-or-later",
    "gdb/transform.m4": "FSFULLR",
    "gdb/contrib/license-check-new-files.py": "GPL-3.0-or-later AND (GPL-2.0-or-later AND GPL-3.0-or-later)",
    "gdb/doc/annotate.texinfo": "GFDL-1.3-or-later AND GFDL-1.1-or-later AND GPL-1.0-or-later",
    "gdb/doc/fdl.texi": "GFDL-1.3-only AND GFDL-1.3-invariants-only",
    "gdb/doc/gdb.texinfo": "(GFDL-1.3-or-later AND GFDL-1.1-or-later) AND (GPL-1.0-or-later AND GFDL-1.1-or-later) AND GPL-1.0-or-later AND (GFDL-1.2-or-later AND LicenseRef-scancode-unknown-license-reference) AND LicenseRef-scancode-other-copyleft AND LicenseRef-scancode-warranty-disclaimer AND LicenseRef-scancode-unknown-license-reference",
    "gdb/doc/gpl.texi": "GPL-1.0-or-later AND LicenseRef-scancode-other-copyleft AND (LicenseRef-scancode-other-copyleft AND GPL-3.0-only)",
    "gdb/doc/guile.texi": "GFDL-1.1-only AND GFDL-1.3-or-later AND GFDL-1.1-or-later",
    "gdb/doc/python.texi": "GFDL-1.1-only AND GFDL-1.3-or-later AND GFDL-1.1-or-later",
    "gdb/doc/refcard.tex": "GPL-3.0-or-later AND (GPL-1.0-or-later AND LicenseRef-scancode-warranty-disclaimer) AND GPL-1.0-or-later",
    "gdb/doc/stack_frame.svg": "GFDL-1.1-or-later",
    "gdb/nat/glibc_thread_db.h": "LGPL-2.1-or-later",
    "gdb/nat/x86-gcc-cpuid.h": "GPL-3.0-or-later WITH GCC-exception-3.1",
    "gdb/contrib/license-check-files.py": "(JSON AND GPL-2.0-or-later AND GPL-3.0-or-later) AND (FSFAP AND LicenseRef-scancode-warranty-disclaimer) AND (FSFAP AND LicenseRef-scancode-warranty-disclaimer AND GPL-3.0-or-later) AND GPL-1.0-or-later AND (FSFULLR AND FSFUL AND FSFAP AND LicenseRef-scancode-warranty-disclaimer AND GPL-3.0-or-later AND GPL-1.0-or-later AND GPL-3.0-only) AND JSON",
    "gdbsupport/aclocal.m4": "FSFULLRWD AND FSFULLR AND FSFULLRSD",
    "gdbsupport/configure": "FSFUL",
    "gdbsupport/gdb_proc_service.h": "GPL-3.0-or-later AND LGPL-2.1-or-later",
    "gdbsupport/libiberty.m4": "GPL-3.0-only AND (GPL-3.0-or-later AND LGPL-3.0-or-later)",
    "gdbsupport/Makefile.in": "FSFULLRWD AND GPL-3.0-or-later",
    "gdbsupport/unordered_dense/stl.h": "MIT",
    "gdbsupport/unordered_dense/unordered_dense.h": "MIT",
    "gdbserver/aclocal.m4": "FSFULLRWD AND FSFULLR",
    "gdbserver/configure": "FSFUL",
    "gdbserver/gdbreplay.cc": "GPL-3.0-or-later AND GPL-1.0-or-later",
    "gdbserver/server.cc": "GPL-3.0-or-later AND GPL-1.0-or-later",
}

table_gdb_none = [
    "gdb/acinclude.m4",
    "gdb/config.in",
    "gdb/configure.host",
    "gdb/configure.tgt",
    "gdb/CONTRIBUTE",
    "gdb/copying.awk",
    "gdb/gdb-gdb.gdb.in",
    "gdb/gdb.gdb",
    "gdb/MAINTAINERS",
    "gdb/msg.defs",
    "gdb/msg_reply.defs",
    "gdb/notify.defs",
    "gdb/PROBLEMS",
    "gdb/process_reply.defs",
    "gdb/SECURITY.txt",
    "gdb/setup.cfg",
    "gdb/silent-rules.mk",
    "gdb/version.in",
    "gdb/config/djgpp/config.sed",
    "gdb/config/djgpp/fnchange.lst",
    "gdb/config/djgpp/README",
    "gdb/config/i386/x86-gnu.mn",
    "gdb/doc/a4rc.sed",
    "gdb/doc/agentexpr.texi",
    "gdb/doc/all-cfg.texi",
    "gdb/doc/lpsrc.sed",
    "gdb/doc/psrc.sed",
    "gdb/doc/stack_frame.eps",
    "gdb/doc/stack_frame.pdf",
    "gdb/doc/stack_frame.png",
    "gdb/doc/stack_frame.txt",
    "gdb/features/gdbserver-regs.xsl",
    "gdb/features/m68k-core.xml",
    "gdb/features/number-regs.xsl",
    "gdb/features/sort-regs.xsl",
    "gdb/guile/README",
    "gdb/po/gdbtext",
    "gdb/python/python-config.py",
    "gdbsupport/acinclude.m4",
    "gdbsupport/config.in",
    "gdbsupport/README",
    "gdbserver/acinclude.m4",
    "gdbserver/config.in",
    "gdbserver/configure.srv",
    "gdbserver/README",
]

table_testsuite = {
    "gdb/testsuite/aclocal.m4": "FSFULLR",
    "gdb/testsuite/configure": "FSFUL",
    "gdb/testsuite/gdb.arch/aarch64-dbreg-contents.c": "LicenseRef-scancode-other-permissive",
    "gdb/testsuite/gdb.arch/arc-tdesc-cpu.xml": "FSFAP-no-warranty-disclaimer",
    "gdb/testsuite/gdb.base/default.exp": "GPL-3.0-or-later AND (GPL-1.0-or-later AND LicenseRef-scancode-other-copyleft) AND GPL-3.0-only",
    "gdb/testsuite/gdb.threads/killed.exp": "GPL-3.0-or-later AND GPL-1.0-or-later",
    "gdb/testsuite/gdb.tui/source-search.exp": "GPL-3.0-or-later AND GPL-1.0-or-later",
    "gdb/testsuite/gdb.xml/maint_print_struct.xml": "FSFAP-no-warranty-disclaimer",
    "gdb/testsuite/lib/ton.tcl": "TCL",
}

table_testsuite_none = [
    "gdb/testsuite/README",
    "gdb/testsuite/TODO",
    "gdb/testsuite/boards/README",
    "gdb/testsuite/config/arm-ice.exp",
    "gdb/testsuite/config/bfin.exp",
    "gdb/testsuite/config/cygmon.exp",
    "gdb/testsuite/config/h8300.exp",
    "gdb/testsuite/config/i386-bozo.exp",
    "gdb/testsuite/gdb.arch/aarch64-gcs-tdesc-without-linux.xml",
    "gdb/testsuite/gdb.arch/altivec-abi.c",
    "gdb/testsuite/gdb.arch/altivec-regs.c",
    "gdb/testsuite/gdb.arch/amd64-stap-three-arg-disp.S",
    "gdb/testsuite/gdb.arch/cordic.ko.bz2",
    "gdb/testsuite/gdb.arch/cordic.ko.debug.bz2",
    "gdb/testsuite/gdb.arch/core-file-pid0.x86-64.core.bz2",
    "gdb/testsuite/gdb.arch/e500-abi.c",
    "gdb/testsuite/gdb.arch/e500-regs.c",
    "gdb/testsuite/gdb.arch/i386-biarch-core.core.bz2",
    "gdb/testsuite/gdb.arch/mips-octeon-bbit.c",
    "gdb/testsuite/gdb.arch/pa-nullify.s",
    "gdb/testsuite/gdb.arch/pa64-nullify.s",
    "gdb/testsuite/gdb.arch/riscv-tdesc-fcsr-32.xml",
    "gdb/testsuite/gdb.arch/riscv-tdesc-fcsr-64.xml",
    "gdb/testsuite/gdb.arch/riscv-tdesc-loading-01.xml",
    "gdb/testsuite/gdb.arch/riscv-tdesc-loading-02.xml",
    "gdb/testsuite/gdb.arch/riscv-tdesc-loading-03.xml",
    "gdb/testsuite/gdb.arch/riscv-tdesc-loading-04.xml",
    "gdb/testsuite/gdb.arch/riscv-tdesc-loading-05.xml",
    "gdb/testsuite/gdb.arch/riscv-tdesc-loading-06.xml",
    "gdb/testsuite/gdb.arch/riscv-tdesc-regs-32.xml",
    "gdb/testsuite/gdb.arch/riscv-tdesc-regs-64.xml",
    "gdb/testsuite/gdb.arch/vsx-regs.c",
    "gdb/testsuite/gdb.asm/aarch64.inc",
    "gdb/testsuite/gdb.asm/alpha.inc",
    "gdb/testsuite/gdb.asm/arm.inc",
    "gdb/testsuite/gdb.asm/asmsrc1.s",
    "gdb/testsuite/gdb.asm/asmsrc2.s",
    "gdb/testsuite/gdb.asm/bfin.inc",
    "gdb/testsuite/gdb.asm/common.inc",
    "gdb/testsuite/gdb.asm/empty.inc",
    "gdb/testsuite/gdb.asm/frv.inc",
    "gdb/testsuite/gdb.asm/h8300.inc",
    "gdb/testsuite/gdb.asm/i386.inc",
    "gdb/testsuite/gdb.asm/ia64.inc",
    "gdb/testsuite/gdb.asm/iq2000.inc",
    "gdb/testsuite/gdb.asm/m32c.inc",
    "gdb/testsuite/gdb.asm/m32r-linux.inc",
    "gdb/testsuite/gdb.asm/m32r.inc",
    "gdb/testsuite/gdb.asm/m68hc11.inc",
    "gdb/testsuite/gdb.asm/m68k.inc",
    "gdb/testsuite/gdb.asm/mips.inc",
    "gdb/testsuite/gdb.asm/netbsd.inc",
    "gdb/testsuite/gdb.asm/openbsd.inc",
    "gdb/testsuite/gdb.asm/pa.inc",
    "gdb/testsuite/gdb.asm/pa64.inc",
    "gdb/testsuite/gdb.asm/powerpc.inc",
    "gdb/testsuite/gdb.asm/powerpc64.inc",
    "gdb/testsuite/gdb.asm/powerpc64le.inc",
    "gdb/testsuite/gdb.asm/s390.inc",
    "gdb/testsuite/gdb.asm/s390x.inc",
    "gdb/testsuite/gdb.asm/sh.inc",
    "gdb/testsuite/gdb.asm/sparc.inc",
    "gdb/testsuite/gdb.asm/sparc64.inc",
    "gdb/testsuite/gdb.asm/v850.inc",
    "gdb/testsuite/gdb.asm/x86_64.inc",
    "gdb/testsuite/gdb.asm/xstormy16.inc",
    "gdb/testsuite/gdb.base/advance.c",
    "gdb/testsuite/gdb.base/all-types.c",
    "gdb/testsuite/gdb.base/annota1.c",
    "gdb/testsuite/gdb.base/annota3.c",
    "gdb/testsuite/gdb.base/anon.c",
    "gdb/testsuite/gdb.base/args.c",
    "gdb/testsuite/gdb.base/async.c",
    "gdb/testsuite/gdb.base/attach.c",
    "gdb/testsuite/gdb.base/attach2.c",
    "gdb/testsuite/gdb.base/attach3.c",
    "gdb/testsuite/gdb.base/average.c",
    "gdb/testsuite/gdb.base/bar.c",
    "gdb/testsuite/gdb.base/batch-exit-status.bad-commands",
    "gdb/testsuite/gdb.base/batch-exit-status.good-commands",
    "gdb/testsuite/gdb.base/baz.c",
    "gdb/testsuite/gdb.base/bitfields.c",
    "gdb/testsuite/gdb.base/bitfields2.c",
    "gdb/testsuite/gdb.base/call-ar-st.c",
    "gdb/testsuite/gdb.base/call-rt-st.c",
    "gdb/testsuite/gdb.base/call-strs.c",
    "gdb/testsuite/gdb.base/catch-syscall.c",
    "gdb/testsuite/gdb.base/chng-syms.c",
    "gdb/testsuite/gdb.base/consecutive.c",
    "gdb/testsuite/gdb.base/constvars.c",
    "gdb/testsuite/gdb.base/debug-expr.c",
    "gdb/testsuite/gdb.base/display.c",
    "gdb/testsuite/gdb.base/dump.c",
    "gdb/testsuite/gdb.base/ending-run.c",
    "gdb/testsuite/gdb.base/execd-prog.c",
    "gdb/testsuite/gdb.base/exprs.c",
    "gdb/testsuite/gdb.base/fileio.c",
    "gdb/testsuite/gdb.base/fixsectshr.c",
    "gdb/testsuite/gdb.base/foll-fork.c",
    "gdb/testsuite/gdb.base/foo.c",
    "gdb/testsuite/gdb.base/funcargs.c",
    "gdb/testsuite/gdb.base/gdb_history",
    "gdb/testsuite/gdb.base/gdbvars.c",
    "gdb/testsuite/gdb.base/grbx.c",
    "gdb/testsuite/gdb.base/huge.c",
    "gdb/testsuite/gdb.base/info-macros.c",
    "gdb/testsuite/gdb.base/int-type.c",
    "gdb/testsuite/gdb.base/interrupt.c",
    "gdb/testsuite/gdb.base/jit-elf-dlmain.c",
    "gdb/testsuite/gdb.base/jump.c",
    "gdb/testsuite/gdb.base/label.c",
    "gdb/testsuite/gdb.base/langs0.c",
    "gdb/testsuite/gdb.base/langs1.c",
    "gdb/testsuite/gdb.base/langs2.c",
    "gdb/testsuite/gdb.base/langs2.cxx",
    "gdb/testsuite/gdb.base/lineinc.c",
    "gdb/testsuite/gdb.base/lineinc1.h",
    "gdb/testsuite/gdb.base/lineinc2.h",
    "gdb/testsuite/gdb.base/lineinc3.h",
    "gdb/testsuite/gdb.base/list0.c",
    "gdb/testsuite/gdb.base/list0.h",
    "gdb/testsuite/gdb.base/list1.c",
    "gdb/testsuite/gdb.base/m32r.ld",
    "gdb/testsuite/gdb.base/macscp1.c",
    "gdb/testsuite/gdb.base/macscp2.h",
    "gdb/testsuite/gdb.base/macscp3.h",
    "gdb/testsuite/gdb.base/macscp4.h",
    "gdb/testsuite/gdb.base/mips_pro.c",
    "gdb/testsuite/gdb.base/miscexprs.c",
    "gdb/testsuite/gdb.base/nodebug.c",
    "gdb/testsuite/gdb.base/opaque0.c",
    "gdb/testsuite/gdb.base/opaque1.c",
    "gdb/testsuite/gdb.base/overlays.c",
    "gdb/testsuite/gdb.base/ovlymgr.c",
    "gdb/testsuite/gdb.base/ovlymgr.h",
    "gdb/testsuite/gdb.base/pc-fp.c",
    "gdb/testsuite/gdb.base/pi.txt",
    "gdb/testsuite/gdb.base/pointers.c",
    "gdb/testsuite/gdb.base/pr10179-a.c",
    "gdb/testsuite/gdb.base/pr10179-b.c",
    "gdb/testsuite/gdb.base/printcmds.c",
    "gdb/testsuite/gdb.base/psymtab1.c",
    "gdb/testsuite/gdb.base/psymtab2.c",
    "gdb/testsuite/gdb.base/ptype.c",
    "gdb/testsuite/gdb.base/ptype1.c",
    "gdb/testsuite/gdb.base/recurse.c",
    "gdb/testsuite/gdb.base/remote.c",
    "gdb/testsuite/gdb.base/reread1.c",
    "gdb/testsuite/gdb.base/reread2.c",
    "gdb/testsuite/gdb.base/return2.c",
    "gdb/testsuite/gdb.base/run.c",
    "gdb/testsuite/gdb.base/scope0.c",
    "gdb/testsuite/gdb.base/scope1.c",
    "gdb/testsuite/gdb.base/setshow.c",
    "gdb/testsuite/gdb.base/setvar.c",
    "gdb/testsuite/gdb.base/shmain.c",
    "gdb/testsuite/gdb.base/shr1.c",
    "gdb/testsuite/gdb.base/shr2.c",
    "gdb/testsuite/gdb.base/shreloc.c",
    "gdb/testsuite/gdb.base/shreloc1.c",
    "gdb/testsuite/gdb.base/shreloc2.c",
    "gdb/testsuite/gdb.base/sigall.c",
    "gdb/testsuite/gdb.base/signals.c",
    "gdb/testsuite/gdb.base/sizeof.c",
    "gdb/testsuite/gdb.base/skip-solib-lib.c",
    "gdb/testsuite/gdb.base/skip-solib-main.c",
    "gdb/testsuite/gdb.base/so-impl-ld.c",
    "gdb/testsuite/gdb.base/solib1.c",
    "gdb/testsuite/gdb.base/ss.h",
    "gdb/testsuite/gdb.base/step-test.c",
    "gdb/testsuite/gdb.base/store.c",
    "gdb/testsuite/gdb.base/structs2.c",
    "gdb/testsuite/gdb.base/sum.c",
    "gdb/testsuite/gdb.base/twice.c",
    "gdb/testsuite/gdb.base/unwind-on-each-insn-amd64-2.s",
    "gdb/testsuite/gdb.base/unwind-on-each-insn-amd64.s",
    "gdb/testsuite/gdb.base/unwind-on-each-insn-i386.s",
    "gdb/testsuite/gdb.base/varargs.c",
    "gdb/testsuite/gdb.base/watchpoint.c",
    "gdb/testsuite/gdb.base/comp-dir/subdir/dummy.txt",
    "gdb/testsuite/gdb.base/gdbinit-history/unlimited/.gdbinit",
    "gdb/testsuite/gdb.base/gdbinit-history/zero/.gdbinit",
    "gdb/testsuite/gdb.cp/ambiguous.cc",
    "gdb/testsuite/gdb.cp/annota2.cc",
    "gdb/testsuite/gdb.cp/annota3.cc",
    "gdb/testsuite/gdb.cp/anon-union.cc",
    "gdb/testsuite/gdb.cp/casts.cc",
    "gdb/testsuite/gdb.cp/converts.cc",
    "gdb/testsuite/gdb.cp/cpcompletion.cc",
    "gdb/testsuite/gdb.cp/cplusfuncs.cc",
    "gdb/testsuite/gdb.cp/destrprint.cc",
    "gdb/testsuite/gdb.cp/fpointer.cc",
    "gdb/testsuite/gdb.cp/gdb1355.cc",
    "gdb/testsuite/gdb.cp/hang.H",
    "gdb/testsuite/gdb.cp/hang1.cc",
    "gdb/testsuite/gdb.cp/hang2.cc",
    "gdb/testsuite/gdb.cp/hang3.cc",
    "gdb/testsuite/gdb.cp/koenig.cc",
    "gdb/testsuite/gdb.cp/local-static.cc",
    "gdb/testsuite/gdb.cp/local.cc",
    "gdb/testsuite/gdb.cp/m-data.cc",
    "gdb/testsuite/gdb.cp/m-static.cc",
    "gdb/testsuite/gdb.cp/m-static.h",
    "gdb/testsuite/gdb.cp/m-static1.cc",
    "gdb/testsuite/gdb.cp/mb-ctor.cc",
    "gdb/testsuite/gdb.cp/mb-templates.cc",
    "gdb/testsuite/gdb.cp/method.cc",
    "gdb/testsuite/gdb.cp/method2.cc",
    "gdb/testsuite/gdb.cp/namespace-enum-main.cc",
    "gdb/testsuite/gdb.cp/namespace-enum.cc",
    "gdb/testsuite/gdb.cp/namespace-nested-import.cc",
    "gdb/testsuite/gdb.cp/namespace.cc",
    "gdb/testsuite/gdb.cp/nsdecl.cc",
    "gdb/testsuite/gdb.cp/nsimport.cc",
    "gdb/testsuite/gdb.cp/nsnested.cc",
    "gdb/testsuite/gdb.cp/nsnoimports.cc",
    "gdb/testsuite/gdb.cp/nsrecurs.cc",
    "gdb/testsuite/gdb.cp/nsstress.cc",
    "gdb/testsuite/gdb.cp/nsusing.cc",
    "gdb/testsuite/gdb.cp/operator.cc",
    "gdb/testsuite/gdb.cp/oranking.cc",
    "gdb/testsuite/gdb.cp/overload.cc",
    "gdb/testsuite/gdb.cp/ovldbreak.cc",
    "gdb/testsuite/gdb.cp/pr-1023.cc",
    "gdb/testsuite/gdb.cp/pr-1210.cc",
    "gdb/testsuite/gdb.cp/pr10687.cc",
    "gdb/testsuite/gdb.cp/pr10728-x.cc",
    "gdb/testsuite/gdb.cp/pr10728-x.h",
    "gdb/testsuite/gdb.cp/pr10728-y.cc",
    "gdb/testsuite/gdb.cp/pr12028.cc",
    "gdb/testsuite/gdb.cp/pr9067.cc",
    "gdb/testsuite/gdb.cp/pr9167.cc",
    "gdb/testsuite/gdb.cp/shadow.cc",
    "gdb/testsuite/gdb.cp/templates.cc",
    "gdb/testsuite/gdb.cp/virtbase.cc",
    "gdb/testsuite/gdb.ctf/cross-tu-cyclic-1.c",
    "gdb/testsuite/gdb.ctf/cross-tu-cyclic-2.c",
    "gdb/testsuite/gdb.ctf/cross-tu-cyclic-3.c",
    "gdb/testsuite/gdb.ctf/cross-tu-cyclic-4.c",
    "gdb/testsuite/gdb.disasm/am33.s",
    "gdb/testsuite/gdb.disasm/h8300s.s",
    "gdb/testsuite/gdb.disasm/hppa.s",
    "gdb/testsuite/gdb.disasm/mn10200.s",
    "gdb/testsuite/gdb.disasm/mn10300.s",
    "gdb/testsuite/gdb.disasm/sh3.s",
    "gdb/testsuite/gdb.disasm/t01_mov.s",
    "gdb/testsuite/gdb.disasm/t02_mova.s",
    "gdb/testsuite/gdb.disasm/t03_add.s",
    "gdb/testsuite/gdb.disasm/t04_sub.s",
    "gdb/testsuite/gdb.disasm/t05_cmp.s",
    "gdb/testsuite/gdb.disasm/t06_ari2.s",
    "gdb/testsuite/gdb.disasm/t07_ari3.s",
    "gdb/testsuite/gdb.disasm/t08_or.s",
    "gdb/testsuite/gdb.disasm/t09_xor.s",
    "gdb/testsuite/gdb.disasm/t10_and.s",
    "gdb/testsuite/gdb.disasm/t11_logs.s",
    "gdb/testsuite/gdb.disasm/t12_bit.s",
    "gdb/testsuite/gdb.disasm/t13_otr.s",
    "gdb/testsuite/gdb.dwarf2/clztest.c",
    "gdb/testsuite/gdb.dwarf2/dw4-sig-types-b.cc",
    "gdb/testsuite/gdb.dwarf2/dw4-sig-types.cc",
    "gdb/testsuite/gdb.dwarf2/dw4-sig-types.h",
    "gdb/testsuite/gdb.dwarf2/file1.txt",
    "gdb/testsuite/gdb.dwarf2/implptr.S",
    "gdb/testsuite/gdb.dwarf2/imported-unit-c.exp",
    "gdb/testsuite/gdb.dwarf2/pr10770.c",
    "gdb/testsuite/gdb.dwarf2/trace-crash.S",
    "gdb/testsuite/gdb.dwarf2/typeddwarf.c",
    "gdb/testsuite/gdb.fortran/charset.f90",
    "gdb/testsuite/gdb.go/chan.go",
    "gdb/testsuite/gdb.go/global-local-var-shadow.go",
    "gdb/testsuite/gdb.go/handcall.go",
    "gdb/testsuite/gdb.go/hello.go",
    "gdb/testsuite/gdb.go/integers.go",
    "gdb/testsuite/gdb.go/methods.go",
    "gdb/testsuite/gdb.go/package1.go",
    "gdb/testsuite/gdb.go/package2.go",
    "gdb/testsuite/gdb.go/strings.go",
    "gdb/testsuite/gdb.go/types.go",
    "gdb/testsuite/gdb.go/unsafe.go",
    "gdb/testsuite/gdb.guile/scm-frame.c",
    "gdb/testsuite/gdb.linespec/body.h",
    "gdb/testsuite/gdb.linespec/lspec.cc",
    "gdb/testsuite/gdb.linespec/lspec.h",
    "gdb/testsuite/gdb.linespec/base/one/thefile.cc",
    "gdb/testsuite/gdb.linespec/base/two/thefile.cc",
    "gdb/testsuite/gdb.mi/gdb701.c",
    "gdb/testsuite/gdb.mi/gdb792.cc",
    "gdb/testsuite/gdb.mi/mi-console.c",
    "gdb/testsuite/gdb.mi/mi-inheritance-syntax-error.cc",
    "gdb/testsuite/gdb.mi/mi-read-memory.c",
    "gdb/testsuite/gdb.mi/mi-syn-frame.c",
    "gdb/testsuite/gdb.mi/run-with-two-mi-uis.c",
    "gdb/testsuite/gdb.mi/testcmds",
    "gdb/testsuite/gdb.mi/until.c",
    "gdb/testsuite/gdb.multi/bkpt-multi-exec.c",
    "gdb/testsuite/gdb.multi/crashme.c",
    "gdb/testsuite/gdb.objc/basicclass.m",
    "gdb/testsuite/gdb.objc/nondebug.m",
    "gdb/testsuite/gdb.objc/objcdecode.m",
    "gdb/testsuite/gdb.pascal/hello.pas",
    "gdb/testsuite/gdb.perf/README",
    "gdb/testsuite/gdb.python/py-frame.c",
    "gdb/testsuite/gdb.python/py-inferior.c",
    "gdb/testsuite/gdb.python/py-infthread.c",
    "gdb/testsuite/gdb.reverse/i387-env-reverse.c",
    "gdb/testsuite/gdb.reverse/i387-stack-reverse.c",
    "gdb/testsuite/gdb.server/attach-flag.c",
    "gdb/testsuite/gdb.server/transfer.txt",
    "gdb/testsuite/gdb.threads/linux-dp.c",
    "gdb/testsuite/gdb.threads/print-threads.c",
    "gdb/testsuite/gdb.threads/sigthread.c",
    "gdb/testsuite/gdb.threads/tls-main.c",
    "gdb/testsuite/gdb.threads/tls-nodebug.c",
    "gdb/testsuite/gdb.threads/tls-shared.c",
    "gdb/testsuite/gdb.threads/tls.c",
    "gdb/testsuite/gdb.tui/tui-mode-switch.exp",
    "gdb/testsuite/gdb.xml/bad-include.xml",
    "gdb/testsuite/gdb.xml/core-only.xml",
    "gdb/testsuite/gdb.xml/extra-regs.xml",
    "gdb/testsuite/gdb.xml/inc-2.xml",
    "gdb/testsuite/gdb.xml/inc-body.xml",
    "gdb/testsuite/gdb.xml/includes.xml",
    "gdb/testsuite/gdb.xml/loop.xml",
    "gdb/testsuite/gdb.xml/maint-xml-dump-01.xml",
    "gdb/testsuite/gdb.xml/maint-xml-dump-02.xml",
    "gdb/testsuite/gdb.xml/maint-xml-dump-03.xml",
    "gdb/testsuite/gdb.xml/single-reg.xml",
    "gdb/testsuite/gdb.xml/tdesc-bogus.xml",
    "gdb/testsuite/gdb.xml/tdesc-unknown.xml",
    "gdb/testsuite/gdb.xml/trivial.xml",
    "gdb/testsuite/lib/opencl_kernel.cl",
]


def handle_file(f):
    entry = db[f]
    license = entry[1]
    if license is None:
        license = "None"

    if license in COMMON_LICENSES:
        return True

    if f in table_gdb_none or f in table_testsuite_none:
        return True

    if f in table_gdb and table_gdb[f] == license:
        return True

    if f in table_testsuite and table_testsuite[f] == license:
        return True

    matched = False
    for entry in table_gdb_re:
        m = entry.search(f)
        if m and license in table_gdb_re[entry]:
            matched = True
            break
    if matched:
        return True

    print(f + ": " + license)
    return False


def load_db(filename):
    try:
        with open(filename, "r") as fh:
            res = json.load(fh)
            print("Loaded " + filename)
    except OSError:
        print("Failed to load " + filename)
        res = None
    return res


def write_db(db, filename):
    print("Writing " + filename)
    with open(filename, "w") as fh:
        json.dump(db, fh, indent=2)


def gen_db_tmp(filename, dir):
    print("Generating " + filename)
    cmd = [
        "scancode",
        "-q",
        "--license",
        "--json-pp",
        filename,
        dir,
    ]
    res = subprocess.run(cmd)
    if res.returncode != 0:
        sys.exit(res.returncode)


def get_md5sum(filename):
    with open(filename, "rb") as fh:
        data = fh.read()
        hash = hashlib.md5(data)
        return hash.hexdigest()


def gen_db(db, db_tmp):
    for entry in db_tmp["files"]:
        type = entry["type"]
        if type == "directory":
            continue

        f = entry["path"]
        m = re_ignore.search(f)
        if m:
            continue
        md5sum = get_md5sum(f)

        license = entry["detected_license_expression_spdx"]

        db.update({f: (md5sum, license)})


def find_files():
    res = []
    for dir in ["gdb", "gdbsupport", "gdbserver"]:
        for root, dirs, files in os.walk(dir):
            for f in files:
                m = re_ignore.search(f)
                if m:
                    continue
                res.append(os.path.join(root, f))
    return res


def get_md5sums(files):
    res = {}

    for f in files:
        md5sum = get_md5sum(f)
        res.update({f: md5sum})

    return res


def main():

    files = find_files()
    # md5sums = get_md5sums(files)

    global db
    db = load_db(db_file)
    if db is None:
        db = {}
        for dir in ["gdb", "gdbsupport", "gdbserver"]:
            fn = db_tmp_file + "." + dir
            development = True
            if development:
                db_tmp = load_db(fn)
            if db_tmp is None:
                gen_db_tmp(fn, dir)
            db_tmp = load_db(fn)
            gen_db(db, db_tmp)
        write_db(db, db_file)

    for f in files:
        if f not in db:
            print("SKIPPED: " + f)

    exit_code = 0

    for entry in db:
        if not handle_file(entry):
            exit_code = 1

    sys.exit(exit_code)


main()
