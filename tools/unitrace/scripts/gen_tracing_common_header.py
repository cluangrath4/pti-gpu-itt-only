#==============================================================
# Copyright (C) Intel Corporation
#
# SPDX-License-Identifier: MIT
# =============================================================

import os
import sys

def gen_enums(out_file):
  # header
  out_file.write("typedef enum {\n")
  # common enums
  out_file.write("  UnknownTracingId,\n")
  out_file.write("  DummyTracingId,\n")
  #xpti api id
  out_file.write("  XptiTracingId,\n")
  #itt api id
  out_file.write("  IttTracingId,\n")
  #footer
  out_file.write("} API_TRACING_ID;\n")

def main():
  if len(sys.argv) < 2:
    print("Usage: python gen_tracing_common_header.py <output_file_path>")
    return

  dst_file_path = sys.argv[1]

  with open(dst_file_path, "wt") as dst_file:
    dst_file.write("#ifndef PTI_TOOLS_COMMON_H_\n")
    dst_file.write("#define PTI_TOOLS_COMMON_H_\n\n")
    gen_enums(dst_file)
    dst_file.write("\n#endif //PTI_TOOLS_COMMON_H_\n")

if __name__ == "__main__":
  main()
