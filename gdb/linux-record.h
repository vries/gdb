/* Process record and replay target code for GNU/Linux.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#ifndef GDB_LINUX_RECORD_H
#define GDB_LINUX_RECORD_H

struct linux_record_tdep
{
  /* The size of the type that will be used in a system call.  */
  int size_pointer;
  int size__old_kernel_stat;
  int size_tms;
  int size_loff_t;
  int size_flock;
  int size_oldold_utsname;
  int size_ustat;
  int size_old_sigaction;
  int size_old_sigset_t;
  int size_rlimit;
  int size_rusage;
  int size_timeval;
  int size_timezone;
  int size_old_gid_t;
  int size_old_uid_t;
  int size_fd_set;
  int size_old_dirent;
  int size_statfs;
  int size_statfs64;
  int size_sockaddr;
  int size_int;
  int size_long;
  int size_ulong;
  int size_msghdr;
  int size_itimerval;
  int size_stat;
  int size_old_utsname;
  int size_sysinfo;
  int size_msqid_ds;
  int size_shmid_ds;
  int size_new_utsname;
  int size_timex;
  int size_mem_dqinfo;
  int size_if_dqblk;
  int size_fs_quota_stat;
  int size_timespec;
  int size_pollfd;
  int size_NFS_FHSIZE;
  int size_knfsd_fh;
  int size_TASK_COMM_LEN;
  int size_sigaction;
  int size_sigset_t;
  int size_siginfo_t;
  int size_cap_user_data_t;
  int size_stack_t;
  int size_off_t;
  int size_stat64;
  int size_gid_t;
  int size_uid_t;
  int size_PAGE_SIZE;
  int size_flock64;
  int size_user_desc;
  int size_io_event;
  int size_iocb;
  int size_epoll_event;
  int size_itimerspec;
  int size_mq_attr;
  int size_termios;
  int size_termios2;
  int size_pid_t;
  int size_winsize;
  int size_serial_struct;
  int size_serial_icounter_struct;
  int size_hayes_esp_config;
  int size_size_t;
  int size_iovec;
  int size_time_t;

  /* The values of the second argument of system call "sys_ioctl".  */
  ULONGEST ioctl_TCGETS;
  ULONGEST ioctl_TCSETS;
  ULONGEST ioctl_TCSETSW;
  ULONGEST ioctl_TCSETSF;
  ULONGEST ioctl_TCGETA;
  ULONGEST ioctl_TCSETA;
  ULONGEST ioctl_TCSETAW;
  ULONGEST ioctl_TCSETAF;
  ULONGEST ioctl_TCSBRK;
  ULONGEST ioctl_TCXONC;
  ULONGEST ioctl_TCFLSH;
  ULONGEST ioctl_TIOCEXCL;
  ULONGEST ioctl_TIOCNXCL;
  ULONGEST ioctl_TIOCSCTTY;
  ULONGEST ioctl_TIOCGPGRP;
  ULONGEST ioctl_TIOCSPGRP;
  ULONGEST ioctl_TIOCOUTQ;
  ULONGEST ioctl_TIOCSTI;
  ULONGEST ioctl_TIOCGWINSZ;
  ULONGEST ioctl_TIOCSWINSZ;
  ULONGEST ioctl_TIOCMGET;
  ULONGEST ioctl_TIOCMBIS;
  ULONGEST ioctl_TIOCMBIC;
  ULONGEST ioctl_TIOCMSET;
  ULONGEST ioctl_TIOCGSOFTCAR;
  ULONGEST ioctl_TIOCSSOFTCAR;
  ULONGEST ioctl_FIONREAD;
  ULONGEST ioctl_TIOCINQ;
  ULONGEST ioctl_TIOCLINUX;
  ULONGEST ioctl_TIOCCONS;
  ULONGEST ioctl_TIOCGSERIAL;
  ULONGEST ioctl_TIOCSSERIAL;
  ULONGEST ioctl_TIOCPKT;
  ULONGEST ioctl_FIONBIO;
  ULONGEST ioctl_TIOCNOTTY;
  ULONGEST ioctl_TIOCSETD;
  ULONGEST ioctl_TIOCGETD;
  ULONGEST ioctl_TCSBRKP;
  ULONGEST ioctl_TIOCTTYGSTRUCT;
  ULONGEST ioctl_TIOCSBRK;
  ULONGEST ioctl_TIOCCBRK;
  ULONGEST ioctl_TIOCGSID;
  ULONGEST ioctl_TCGETS2;
  ULONGEST ioctl_TCSETS2;
  ULONGEST ioctl_TCSETSW2;
  ULONGEST ioctl_TCSETSF2;
  ULONGEST ioctl_TIOCGPTN;
  ULONGEST ioctl_TIOCSPTLCK;
  ULONGEST ioctl_FIONCLEX;
  ULONGEST ioctl_FIOCLEX;
  ULONGEST ioctl_FIOASYNC;
  ULONGEST ioctl_TIOCSERCONFIG;
  ULONGEST ioctl_TIOCSERGWILD;
  ULONGEST ioctl_TIOCSERSWILD;
  ULONGEST ioctl_TIOCGLCKTRMIOS;
  ULONGEST ioctl_TIOCSLCKTRMIOS;
  ULONGEST ioctl_TIOCSERGSTRUCT;
  ULONGEST ioctl_TIOCSERGETLSR;
  ULONGEST ioctl_TIOCSERGETMULTI;
  ULONGEST ioctl_TIOCSERSETMULTI;
  ULONGEST ioctl_TIOCMIWAIT;
  ULONGEST ioctl_TIOCGICOUNT;
  ULONGEST ioctl_TIOCGHAYESESP;
  ULONGEST ioctl_TIOCSHAYESESP;
  ULONGEST ioctl_FIOQSIZE;

  /* The values of the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  */
  int fcntl_F_GETLK;
  int fcntl_F_GETLK64;
  int fcntl_F_SETLK64;
  int fcntl_F_SETLKW64;

  /* The number of the registers that are used as the arguments of
     a system call.  */
  int arg1;
  int arg2;
  int arg3;
  int arg4;
  int arg5;
  int arg6;
  int arg7;
};

/* Enum that defines the gdb-canonical set of Linux syscall identifiers.
   Different architectures will have different sets of syscall ids, and
   each must provide a mapping from their set to this one.  */

enum gdb_syscall {
  /* An unknown GDB syscall, not a real syscall.  */
  gdb_sys_no_syscall = -1,

#define GDB_SYSCALL(NAME, NUMBER)		\
  gdb_sys_ ## NAME = NUMBER,
#define GDB_UNSUPPORTED_SYSCALL(NAME)

#include "gdb/gdb-syscalls.def"

#undef GDB_SYSCALL
#undef GDB_UNSUPPORTED_SYSCALL
};

/* Record a linux syscall.  */

extern int record_linux_system_call (enum gdb_syscall num, 
				     struct regcache *regcache,
				     struct linux_record_tdep *tdep);

#endif /* GDB_LINUX_RECORD_H */
