//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_TOOLS_UNITRACE_UNIFIED_TRACER_H_
#define PTI_TOOLS_UNITRACE_UNIFIED_TRACER_H_

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "trace_options.h"
#include "logger.h"
#include "utils.h"
#include "itt_collector.h"
#include "chromelogger.h"
#include "unimemory.h"

static std::string GetChromeTraceFileName(void) {
  std::ifstream comm("/proc/self/comm");
  if (comm) {
    std::string name;
    std::getline(comm, name);
    comm.close();
    if (!name.empty()) {
      return std::move(name);
    }
  }
  // should never get here
  return "unitrace";
}

class UniTracer {
 public:
  static UniTracer* Create(const TraceOptions& options) {

    UniTracer* tracer = new UniTracer(options);
    UniMemory::ExitIfOutOfMemory((void *)tracer);

    if (tracer->CheckOption(TRACE_CHROME_ITT_LOGGING) || tracer->CheckOption(TRACE_CCL_SUMMARY_REPORT)) {
        itt_collector = IttCollector::Create(ChromeLogger::IttLoggingCallback);
        if (itt_collector) {
            if (tracer->CheckOption(TRACE_CCL_SUMMARY_REPORT)) {
                itt_collector->EnableCclSummary();
            }
            if (tracer->CheckOption(TRACE_CHROME_ITT_LOGGING)) {
                itt_collector->EnableChromeLogging();
            }
        }
    }
    else {
        itt_collector = IttCollector::Create(nullptr);
    }

    return tracer;
  }

  ~UniTracer() {
    total_execution_time_ = utils::GetSystemTime() - start_time_;

    Report();

    if (itt_collector != nullptr){
      // Print CCL summary before deleting the object
      // If CCL summary is not enbled summary string will be empty
      std::string summary = itt_collector->CclSummaryReport();
      if (summary.size() > 0){
        logger_.Log(summary);
      }
      delete itt_collector;
    }

    if (CheckOption(TRACE_LOG_TO_FILE)) {
      std::cerr << "[INFO] Log is stored in " <<
        options_.GetLogFileName() << std::endl;
    }

    if (chrome_logger_ != nullptr) {
      delete chrome_logger_;
    }
  }

  bool CheckOption(uint32_t option) {
    return options_.CheckFlag(option);
  }

  UniTracer(const UniTracer& that) = delete;
  UniTracer& operator=(const UniTracer& that) = delete;

 private:
  UniTracer(const TraceOptions& options)
      : options_(options),
        logger_(options.GetLogFileName(),
        CheckOption(TRACE_CONDITIONAL_COLLECTION)) {

    start_time_ = utils::GetSystemTime();
    if (CheckOption(TRACE_CHROME_CALL_LOGGING) || CheckOption(TRACE_CHROME_KERNEL_LOGGING) || CheckOption(TRACE_CHROME_DEVICE_LOGGING) || CheckOption(TRACE_CHROME_SYCL_LOGGING) || CheckOption(TRACE_CHROME_ITT_LOGGING)) {
      chrome_logger_ = ChromeLogger::Create(options, GetChromeTraceFileName().c_str());
    }

  }

  void Report() {
    logger_.Log("\n");
  }

 private:
  TraceOptions options_;

  Logger logger_;
  uint64_t start_time_;
  uint64_t total_execution_time_ = 0;

  ChromeLogger* chrome_logger_ = nullptr;
};

#endif // PTI_TOOLS_UNITRACE_UNIFIED_TRACER_H_
