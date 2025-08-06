#==============================================================
# Copyright (C) Intel Corporation
#
# SPDX-License-Identifier: MIT
# =============================================================

import os
import sys

def main():
  if len(sys.argv) < 2:
    print("Usage: python gen_tracing_callbacks.py <output_file_path>")
    return

  dst_file_path = sys.argv[1]

  with open(dst_file_path, "wt") as dst_file:
    dst_file.write("//\n")
    dst_file.write("// GENERATED FILE - DO NOT EDIT\n")
    dst_file.write("// L0/OCL functionality removed.\n")
    dst_file.write("//\n\n")

if __name__ == "__main__":
  main()
