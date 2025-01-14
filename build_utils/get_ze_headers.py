import sys
import os

import build_utils

url = "https://github.com/oneapi-src/level-zero.git"
commit = "a4afcb39ee265e595d3f0aa57b25b5e845fb494c"

def main():
  if len(sys.argv) < 3:
    print("Usage: python get_ze_headers.py <include_path> <build_path>")
    return

  dst_path = sys.argv[1]
  if (not os.path.exists(dst_path)):
    os.mkdir(dst_path)
  dst_path = os.path.join(dst_path, "level_zero")
  if (not os.path.exists(dst_path)):
    os.mkdir(dst_path)

  clone_path = sys.argv[2]
  clone_path = os.path.join(clone_path, "level-zero")
  build_utils.clone(url, commit, clone_path)

  src_path = os.path.join(clone_path, "include")
  build_utils.copy(src_path, dst_path,\
    ["ze_api.h", "zes_api.h", "zet_api.h"])

  if (not os.path.exists(os.path.join(dst_path, "layers"))):
    os.mkdir(os.path.join(dst_path, "layers"))
  src_path = os.path.join(clone_path, "include", "layers")
  build_utils.copy(src_path, os.path.join(dst_path, "layers"),\
    ["zel_tracing_api.h", "zel_tracing_register_cb.h"])

if __name__ == "__main__":
  main()
