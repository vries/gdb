/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-core.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arm_arm_core (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.core");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_flags (feature, "arm_cpsr", 4);
  tdesc_add_bitfield (type_with_fields, "M", 0, 4);
  tdesc_add_flag (type_with_fields, 5, "T");
  tdesc_add_flag (type_with_fields, 6, "F");
  tdesc_add_flag (type_with_fields, 7, "I");
  tdesc_add_flag (type_with_fields, 8, "A");
  tdesc_add_flag (type_with_fields, 9, "E");
  tdesc_add_bitfield (type_with_fields, "IT72", 10, 15);
  tdesc_add_bitfield (type_with_fields, "GE", 16, 19);
  tdesc_add_flag (type_with_fields, 24, "J");
  tdesc_add_bitfield (type_with_fields, "IT10", 25, 26);
  tdesc_add_flag (type_with_fields, 27, "Q");
  tdesc_add_flag (type_with_fields, 28, "V");
  tdesc_add_flag (type_with_fields, 29, "C");
  tdesc_add_flag (type_with_fields, 30, "Z");
  tdesc_add_flag (type_with_fields, 31, "N");

  tdesc_create_reg (feature, "r0", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r1", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r2", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r3", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r4", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r5", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r6", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r7", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r8", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r9", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r10", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r11", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r12", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "sp", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "lr", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "pc", regnum++, 1, NULL, 32, "code_ptr");
  regnum = 25;
  tdesc_create_reg (feature, "cpsr", regnum++, 1, NULL, 32, "arm_cpsr");
  return regnum;
}
