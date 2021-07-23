/* Native-dependent code for AArch64.

   Copyright (C) 2011-2023 Free Software Foundation, Inc.

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

#include "defs.h"
#include "gdbarch.h"
#include "inferior.h"
#include "cli/cli-cmds.h"
#include "aarch64-nat.h"
#include "arch/aarch64-insn.h"

#include <unordered_map>

/* Hash table storing per-process data.  We don't bind this to a
   per-inferior registry because of targets like x86 GNU/Linux that
   need to keep track of processes that aren't bound to any inferior
   (e.g., fork children, checkpoints).  */

static std::unordered_map<pid_t, aarch64_debug_reg_state>
aarch64_debug_process_state;

/* See aarch64-nat.h.  */

struct aarch64_debug_reg_state *
aarch64_lookup_debug_reg_state (pid_t pid)
{
  auto it = aarch64_debug_process_state.find (pid);
  if (it != aarch64_debug_process_state.end ())
    return &it->second;

  return nullptr;
}

/* See aarch64-nat.h.  */

struct aarch64_debug_reg_state *
aarch64_get_debug_reg_state (pid_t pid)
{
  return &aarch64_debug_process_state[pid];
}

/* See aarch64-nat.h.  */

void
aarch64_remove_debug_reg_state (pid_t pid)
{
  aarch64_debug_process_state.erase (pid);
}

/* Returns the number of hardware watchpoints of type TYPE that we can
   set.  Value is positive if we can set CNT watchpoints, zero if
   setting watchpoints of type TYPE is not supported, and negative if
   CNT is more than the maximum number of watchpoints of type TYPE
   that we can support.  TYPE is one of bp_hardware_watchpoint,
   bp_read_watchpoint, bp_write_watchpoint, or bp_hardware_breakpoint.
   CNT is the number of such watchpoints used so far (including this
   one).  OTHERTYPE is non-zero if other types of watchpoints are
   currently enabled.  */

int
aarch64_can_use_hw_breakpoint (enum bptype type, int cnt, int othertype)
{
  if (type == bp_hardware_watchpoint || type == bp_read_watchpoint
      || type == bp_access_watchpoint || type == bp_watchpoint)
    {
      if (aarch64_num_wp_regs == 0)
	return 0;
    }
  else if (type == bp_hardware_breakpoint)
    {
      if (aarch64_num_bp_regs == 0)
	return 0;
    }
  else
    gdb_assert_not_reached ("unexpected breakpoint type");

  /* We always return 1 here because we don't have enough information
     about possible overlap of addresses that they want to watch.  As an
     extreme example, consider the case where all the watchpoints watch
     the same address and the same region length: then we can handle a
     virtually unlimited number of watchpoints, due to debug register
     sharing implemented via reference counts.  */
  return 1;
}

/* Insert a hardware-assisted breakpoint at BP_TGT->reqstd_address.
   Return 0 on success, -1 on failure.  */

int
aarch64_insert_hw_breakpoint (struct gdbarch *gdbarch,
			      struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address = bp_tgt->reqstd_address;
  int len;
  const enum target_hw_bp_type type = hw_execute;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

  gdbarch_breakpoint_from_pc (gdbarch, &addr, &len);

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"insert_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
		(unsigned long) addr, len);

  ret = aarch64_handle_breakpoint (type, addr, len, 1 /* is_insert */,
				   inferior_ptid, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "insert_hw_breakpoint", addr, len, type);
    }

  return ret;
}

/* Remove a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */

int
aarch64_remove_hw_breakpoint (struct gdbarch *gdbarch,
			      struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address;
  int len = 4;
  const enum target_hw_bp_type type = hw_execute;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

  gdbarch_breakpoint_from_pc (gdbarch, &addr, &len);

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"remove_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
		(unsigned long) addr, len);

  ret = aarch64_handle_breakpoint (type, addr, len, 0 /* is_insert */,
				   inferior_ptid, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "remove_hw_watchpoint", addr, len, type);
    }

  return ret;
}

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

int
aarch64_insert_watchpoint (CORE_ADDR addr, int len, enum target_hw_bp_type type,
			   struct expression *cond)
{
  int ret;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"insert_watchpoint on entry (addr=0x%08lx, len=%d)\n",
		(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = aarch64_handle_watchpoint (type, addr, len, 1 /* is_insert */,
				   inferior_ptid, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "insert_watchpoint", addr, len, type);
    }

  return ret;
}

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */

int
aarch64_remove_watchpoint (CORE_ADDR addr, int len, enum target_hw_bp_type type,
			   struct expression *cond)
{
  int ret;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"remove_watchpoint on entry (addr=0x%08lx, len=%d)\n",
		(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = aarch64_handle_watchpoint (type, addr, len, 0 /* is_insert */,
				   inferior_ptid, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "remove_watchpoint", addr, len, type);
    }

  return ret;
}

/* Macros to distinguish between various types of load/store instructions.  */

/* Regular and Advanced SIMD load/store instructions.  */
#define GENERAL_LOAD_STORE_P(insn) (bit (insn, 25) == 0 && bit (insn, 27) == 1)

/* SVE load/store instruction.  */
#define SVE_LOAD_STORE_P(insn) (bits (insn, 25, 28) == 0x2		      \
				&& (bits (insn, 29, 31) == 0x4		      \
				    || bits (insn, 29, 31) == 0x5	      \
				    || bits (insn, 29, 31) == 0x6	      \
				    || (bits (insn, 29, 31) == 0x7	      \
					&& ((bit (insn, 15) == 0x0	      \
					     && (bit (insn, 13) == 0x0	      \
						 || bit (insn, 13) == 0x1))   \
					     || (bit (insn, 15) == 0x1	      \
						 && bit (insn, 13) == 0x0)    \
					     || bits (insn, 13, 15) == 0x6    \
					     || bits (insn, 13, 15) == 0x7))))

/* Any load/store instruction (regular, Advanced SIMD or SVE).  */
#define LOAD_STORE_P(insn) (GENERAL_LOAD_STORE_P (insn)			      \
			    || SVE_LOAD_STORE_P (insn))

/* Load/Store pair (regular or vector).  */
#define LOAD_STORE_PAIR_P(insn) (bit (insn, 28) == 0 && bit (insn, 29) == 1)

#define COMPARE_SWAP_PAIR_P(insn) (bits (insn, 30, 31) == 0		      \
				   && bits (insn, 28, 29) == 0		      \
				   && bit (insn, 26) == 0		      \
				   && bits (insn, 23, 24) == 0		      \
				   && bit (insn, 11) == 1)
#define LOAD_STORE_EXCLUSIVE_PAIR_P(insn) (bit (insn, 31) == 1		      \
					   && bits (insn, 28, 29) == 0	      \
					   && bit (insn, 26) == 0	      \
					   && bits (insn, 23, 24) == 0	      \
					   && bit (insn, 11) == 1)

#define LOAD_STORE_LITERAL_P(insn) (bit (insn, 28) == 1			      \
				    && bit (insn, 29) == 0		      \
				    && bit (insn, 24) == 0)

/* Vector Load/Store multiple structures.  */
#define LOAD_STORE_MS(insn) (bits (insn, 28, 29) == 0x0			      \
			     && bit (insn, 31) == 0x0			      \
			     && bit (insn, 26) == 0x1			      \
			     && ((bits (insn, 23, 24) == 0x0		      \
				  && bits (insn, 16, 21) == 0x0)	      \
				 || (bits (insn, 23, 24) == 0x1		      \
				     && bit (insn, 21) == 0x0)))

/* Vector Load/Store single structure.  */
#define LOAD_STORE_SS(insn) (bits (insn, 28, 29) == 0x0			      \
			     && bit (insn, 31) == 0x0			      \
			     && bit (insn, 26) == 0x1			      \
			     && ((bits (insn, 23, 24) == 0x2		      \
				  && bits (insn, 16, 20) == 0x0)	      \
				 || (bits (insn, 23, 24) == 0x3)))

/* Assuming INSN is a load/store instruction, return the size of the
   memory access.  The patterns are documented in the ARM Architecture
   Reference Manual - Index by encoding.  */

static unsigned int
get_load_store_access_size (CORE_ADDR insn)
{
  if (SVE_LOAD_STORE_P (insn))
    {
      /* SVE load/store instructions.  */

      /* This is not always correct for SVE instructions, but it is a reasonable
	 default for now.  Calculating the exact memory access size for SVE
	 load/store instructions requires parsing instructions and evaluating
	 the vector length.  */
      return 16;
    }

  /* Non-SVE instructions.  */

  bool vector = (bit (insn, 26) == 1);
  bool ldst_pair = LOAD_STORE_PAIR_P (insn);

  /* Is this an Advanced SIMD load/store instruction?  */
  if (vector)
    {
      unsigned int size = bits (insn, 30, 31);

      if (LOAD_STORE_LITERAL_P (insn))
	{
	  /* Advanced SIMD load/store literal */
	  /* Sizes 4, 8 and 16 bytes.  */
	  return 4 << size;
	}
      else if (LOAD_STORE_MS (insn))
	{
	  size = 8 << bit (insn, 30);

	  unsigned char opcode = bits (insn, 12, 15);

	  if (opcode == 0x0 || opcode == 0x2)
	    size *= 4;
	  else if (opcode == 0x4 || opcode == 0x6)
	    size *= 3;
	  else if (opcode == 0x8 || opcode == 0xa)
	    size *= 2;

	  return size;
	}
      else if (LOAD_STORE_SS (insn))
	{
	  size = 8 << bit (insn, 30);
	  return size;
	}
      /* Advanced SIMD load/store instructions.  */
      else if (ldst_pair)
	{
	  /* Advanced SIMD load/store pair.  */
	  /* Sizes 8, 16 and 32 bytes.  */
	  return (8 << size);
	}
      else
	{
	  /* Regular Advanced SIMD load/store instructions.  */
	  size = size | (bit (insn, 23) << 2);
	  return 1 << size;
	}
    }

  /* This must be a regular GPR load/store instruction.  */

  unsigned int size = bits (insn, 30, 31);
  bool cs_pair = COMPARE_SWAP_PAIR_P (insn);
  bool ldstx_pair = LOAD_STORE_EXCLUSIVE_PAIR_P (insn);

  if (ldst_pair)
    {
      /* Load/Store pair.  */
      if (size == 0x1)
	{
	  if (bit (insn, 22) == 0)
	    /* STGP - 16 bytes.  */
	    return 16;
	  else
	    /* LDPSW - 8 bytes.  */
	    return 8;
	}
      /* Other Load/Store pair.  */
      return (size == 0)? 8 : 16;
    }
  else if (cs_pair || ldstx_pair)
    {
      /* Compare Swap Pair or Load Store Exclusive Pair.  */
      /* Sizes 8 and 16 bytes.  */
      size = bit (insn, 30);
      return (8 << size);
    }
  else if (LOAD_STORE_LITERAL_P (insn))
    {
      /* Load/Store literal.  */
      /* Sizes between 4 and 8 bytes.  */
      if (size == 0x2)
	return 4;

      return 4 << size;
    }
  else
    {
      /* General load/store instructions, with memory access sizes between
	 1 and 8 bytes.  */
      return (1 << size);
    }
}

/* Return true if the regions [mem_addr, mem_addr + mem_len) and
   [watch_addr, watch_addr + watch_len) overlap.  False otherwise.  */

static bool
hw_watch_regions_overlap (CORE_ADDR mem_addr, size_t mem_len,
			  CORE_ADDR watch_addr, size_t watch_len)
{
  /* Quick check for non-overlapping case.  */
  if (watch_addr >= (mem_addr + mem_len)
      || mem_addr >= (watch_addr + watch_len))
    return false;

  CORE_ADDR start = std::max (mem_addr, watch_addr);
  CORE_ADDR end = std::min (mem_addr + mem_len, watch_addr + watch_len);

  return ((end - start) > 0);
}

/* Check if a hardware watchpoint has triggered.  If a trigger is detected,
   return true and update ADDR_P with the stopped data address.
   Otherwise return false.

   STATE is the debug register's state, INSN is the instruction the inferior
   stopped at and ADDR_TRAP is the reported stopped data address.  */

bool
aarch64_stopped_data_address (const struct aarch64_debug_reg_state *state,
			      CORE_ADDR insn, CORE_ADDR addr_trap,
			      CORE_ADDR *addr_p)
{
  /* There are 6 variations of watchpoint range and memory access
     range positioning:

     - W is the byte in the watchpoint range only.

     - B is the byte in the memory access range ony.

     - O is the byte in the overlapping region of the watchpoint range and
       the memory access range.

     1 - Non-overlapping, no triggers.

     [WWWWWWWW]...[BBBBBBBB]

     2 - Non-overlapping, no triggers.

     [BBBBBBBB]...[WWWWWWWW]

     3 - Overlapping partially, triggers.

     [BBBBOOOOWWWW]

     4 - Overlapping partially, triggers.

     [WWWWOOOOBBBB]

     5 - Memory access contained in watchpoint range, triggers.

     [WWWWOOOOOOOOWWWW]

     6 - Memory access containing watchpoint range, triggers.

     [BBBBOOOOOOOOBBBB]
  */

  /* If there are no load/store instructions at PC, this must be a hardware
     breakpoint hit.  Return early.

     If a hardware breakpoint is placed at a PC containing a load/store
     instruction, we will go through the memory access size check anyway, but
     will eventually figure out there are no watchpoints matching such an
     address.

     There is one corner case though, which is having a hardware breakpoint and
     a hardware watchpoint at PC, when PC contains a load/store
     instruction.  That is an ambiguous case that is hard to differentiate.

     There's not much we can do about that unless the kernel sends us enough
     information to tell them apart.  */
  if (!LOAD_STORE_P (insn))
    return false;

  /* Get the memory access size for the instruction at PC.  */
  unsigned int memory_access_size = get_load_store_access_size (insn);

  for (int i = aarch64_num_wp_regs - 1; i >= 0; --i)
    {
      const unsigned int offset
	= aarch64_watchpoint_offset (state->dr_ctrl_wp[i]);
      const unsigned int len = aarch64_watchpoint_length (state->dr_ctrl_wp[i]);
      const CORE_ADDR addr_watch = state->dr_addr_wp[i] + offset;
      const CORE_ADDR addr_orig = state->dr_addr_orig_wp[i];

      if ((state->dr_ref_count_wp[i]
	  && DR_CONTROL_ENABLED (state->dr_ctrl_wp[i]))
	  && hw_watch_regions_overlap (addr_trap, memory_access_size,
				       addr_watch, len))
	{
	  /* ADDR_TRAP reports the first address of the memory range
	     accessed by the CPU, regardless of what was the memory
	     range watched.  Thus, a large CPU access that straddles
	     the ADDR_WATCH..ADDR_WATCH+LEN range may result in an
	     ADDR_TRAP that is lower than the
	     ADDR_WATCH..ADDR_WATCH+LEN range.  E.g.:

	     addr: |   4   |   5   |   6   |   7   |   8   |
				   |---- range watched ----|
		   |----------- range accessed ------------|

	     In this case, ADDR_TRAP will be 4.

	     To match a watchpoint known to GDB core, we must never
	     report *ADDR_P outside of any ADDR_WATCH..ADDR_WATCH+LEN
	     range.  ADDR_WATCH <= ADDR_TRAP < ADDR_ORIG is a false
	     positive on kernels older than 4.10.  See PR
	     external/20207.  */
	  *addr_p = addr_orig;
	  return true;
	}
    }

  /* No hardware watchpoint hits detected.  */
  return false;
}

/* Define AArch64 maintenance commands.  */

static void
add_show_debug_regs_command (void)
{
  /* A maintenance command to enable printing the internal DRi mirror
     variables.  */
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &show_debug_regs, _("\
Set whether to show variables that mirror the AArch64 debug registers."), _("\
Show whether to show variables that mirror the AArch64 debug registers."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, the debug registers values are shown when GDB inserts\n\
or removes a hardware breakpoint or watchpoint, and when the inferior\n\
triggers a breakpoint or watchpoint."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}

void
aarch64_initialize_hw_point ()
{
  add_show_debug_regs_command ();
}
