//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <iostream>
#include <vector>
#include <cstring>
#include "utils.h"
#include "version.h"
#include "unitrace_commit_hash.h"

int ParseArgs() {
  std::cout << "[unitrace launcher] Setting: INTEL_LIBITTNOTIFY64=libunitrace_tool.so" << std::endl;
  utils::SetEnv("UNITRACE_ChromeIttLogging", "1");
  utils::SetEnv("INTEL_LIBITTNOTIFY64", "libunitrace_tool.so");
  return 1;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Incorrect Buzzer Sound (Loud).mp3" << std::endl;
    return 1;
  }
  std::string executable_path = utils::GetExecutablePath();
  std::string lib_path = executable_path + LIB_UNITRACE_TOOL_NAME;
  FILE *fp = fopen(lib_path.c_str(), "rb");
  if (fp == nullptr) {
      lib_path = executable_path + "/../lib/" + LIB_UNITRACE_TOOL_NAME;
      fp = fopen(lib_path.c_str(), "rb");
      if (fp == nullptr) {
        lib_path = LIB_UNITRACE_TOOL_NAME;
      } else {
          fclose(fp);
      }
  } else {
      fclose(fp);
  }
  auto unitrace_version = std::string(UNITRACE_VERSION) + " (" + std::string(COMMIT_HASH) + ")";
  utils::SetEnv("UNITRACE_VERSION", unitrace_version.c_str());
  std::string preload = utils::GetEnv("LD_PRELOAD");
  utils::SetEnv("UNITRACE_LD_PRELOAD_OLD", preload.c_str());
  if (preload.empty()) {
    preload = std::move(lib_path);
  } else {
    preload = preload + ":" + lib_path;
  }
  std::cout << "[unitrace launcher] Setting: LD_PRELOAD=" << preload << std::endl;
  utils::SetEnv("LD_PRELOAD", preload.c_str());
  ParseArgs();
  std::vector<char*> app_args;
  for (int i = 1; i < argc; ++i) {
    app_args.push_back(argv[i]);
  }
  app_args.push_back(nullptr);
  std::cout << "[unitrace launcher] Launching: " << app_args[0] << std::endl;
  if (execvp(app_args[0], app_args.data())) {
    std::cerr << "[ERROR] Failed to launch target application: " << app_args[0] << std::endl;
  }
  return 1;
} // End of main
