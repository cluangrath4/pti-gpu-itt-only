//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_TOOLS_UNITRACE_CHROME_LOGGER_H_
#define PTI_TOOLS_UNITRACE_CHROME_LOGGER_H_

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <thread>
#include <tuple>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <cstring>

#include "trace_options.h"
#include "unitimer.h"
#include "unikernel.h"
#include "unievent.h"
#include "unimemory.h"
#include <atomic>

#include "common_header.gen"

static inline std::string GetHostName(void) {
  char hname[256];
#ifdef _WIN32
  DWORD size = sizeof(hname);
  GetComputerNameA(hname, &size);
#else  /* _WIN32 */
  gethostname(hname, sizeof(hname));
#endif /* _WIN32 */
  hname[255] = 0;
  return hname;
}

#ifdef _WIN32
#define strdup _strdup
#else /* _WIN32 */
#define strdup strdup
#endif /* _WIN32 */

static std::string EncodeURI(const std::string &input) {
  std::ostringstream encoded;
  encoded.fill('0');
  encoded << std::hex;

  for (auto c : input) {
    // accepted characters
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded << c;
      continue;
    }

    // percent-encoded otherwise
    encoded << std::uppercase;
    encoded << '%' << std::setw(2) << int(c);
    encoded << std::nouppercase;
  }

  return encoded.str();
}

static Logger* logger_ = nullptr;
std::recursive_mutex logger_lock_; //lock to synchronize file write

#if BUILD_WITH_ITT
static std::string convertDataToString(IttArgs* args) {
  std::string strData = "";
  void* dataPtr = args->isIndirectData ? args->data[0] : args->data;
  if (args->count) {
    switch (args->type) {
      case __itt_metadata_u64: {
        const uint64_t* uint64Ptr = reinterpret_cast<const uint64_t*>(dataPtr);
        for (int i=0; i < args->count; i++) {
          if (i) {
            strData += ",";
          }
          strData += std::to_string(*(uint64Ptr + i));
        }
        break;
      }
      case __itt_metadata_s64: {
        const int64_t* int64Ptr = reinterpret_cast<const int64_t*>(dataPtr);
        for (int i=0; i < args->count; i++) {
          if (i) {
            strData += ",";
          }
          strData += std::to_string(*(int64Ptr + i));
        }
        break;
      }
      case __itt_metadata_u32: {
        const uint32_t* uint32Ptr = reinterpret_cast<const uint32_t*>(dataPtr);
        for (int i=0; i < args->count; i++) {
          if (i) {
            strData += ",";
          }
          strData += std::to_string(*(uint32Ptr + i));
        }
        break;
      }
      case __itt_metadata_s32: {
        const int32_t* int32Ptr = reinterpret_cast<const int32_t*>(dataPtr);
        for (int i=0; i < args->count; i++) {
          if (i) {
            strData += ",";
          }
          strData += std::to_string(*(int32Ptr + i));
        }
        break;
      }
      case __itt_metadata_u16: {
        const uint16_t* uint16Ptr = reinterpret_cast<const uint16_t*>(dataPtr);
        for (int i=0; i < args->count; i++) {
          if (i) {
            strData += ",";
          }
          strData += std::to_string(*(uint16Ptr + i));
        }
        break;
      }
      case __itt_metadata_s16: {
        const int16_t* uint16Ptr = reinterpret_cast<const int16_t*>(dataPtr);
        for (int i=0; i < args->count; i++) {
          if (i) {
            strData += ",";
          }
          strData += std::to_string(*(uint16Ptr + i));
        }
        break;
      }
      case __itt_metadata_float: {
        const float* floatPtr = reinterpret_cast<const float*>(dataPtr);
        for (int i=0; i < args->count; i++) {
          if (i) {
            strData += ",";
          }
          strData += std::to_string(*(floatPtr + i));
        }
        break;
      }
      case __itt_metadata_double: {
        const double* doublePtr = reinterpret_cast<const double*>(dataPtr);
        for (int i=0; i < args->count; i++) {
          if (i) {
            strData += ",";
          }
          strData += std::to_string(*(doublePtr + i));
        }
        break;
      }
      default: {  // default is string
        const char* stringPtr = reinterpret_cast<const char*>(dataPtr);
        strData += "\"";
        strData += std::string(stringPtr, args->count);
        strData += "\"";
        break;
      }
    }
  }
  return strData;
}
#else /* BUILD_WITH_ITT */
static std::string convertDataToString(IttArgs* args) {
  return "";
}
#endif /* BUILD_WITH_ITT */

class TraceBuffer;
std::set<TraceBuffer *> *trace_buffers_ = nullptr;

#define BUFFER_SLICE_SIZE_DEFAULT	(0x1 << 20)

class TraceBuffer {
  public:
    TraceBuffer() : flush_immediately_(false) {
      std::string szstr = utils::GetEnv("UNITRACE_ChromeEventBufferSize");
      if (szstr.empty() || (szstr == "-1")) {
        buffer_capacity_ = -1;
        slice_capacity_ = BUFFER_SLICE_SIZE_DEFAULT;
      }
      else {
        buffer_capacity_ = std::stoi(szstr);
        if (buffer_capacity_ == 0) {
          buffer_capacity_ = 1;	// at least one event slot
          flush_immediately_ = true;
        }
        slice_capacity_ = buffer_capacity_;
      }

      HostEventRecord *her = (HostEventRecord *)(malloc(sizeof(HostEventRecord) * slice_capacity_));
      UniMemory::ExitIfOutOfMemory((void *)(her));

      host_event_buffer_.push_back(her);
      tid_= utils::GetTid();
      pid_= utils::GetPid();

      current_host_event_buffer_slice_ = 0;
      next_host_event_index_ = 0;
      host_event_buffer_flushed_ = false;
      finalized_.store(false, std::memory_order_release);
      if ((utils::GetEnv("UNITRACE_MetricQuery") == "1") || (utils::GetEnv("UNITRACE_KernelMetrics") == "1")) {
        metrics_enabled_ = true;
      }
      else {
        metrics_enabled_ = false;
      }

      std::lock_guard<std::recursive_mutex> lock(logger_lock_);	// use this lock to protect trace_buffers_

      if (trace_buffers_ == NULL) {
        trace_buffers_ = new std::set<TraceBuffer *>;
        UniMemory::ExitIfOutOfMemory((void *)(trace_buffers_));
      }
      trace_buffers_->insert(this);
    }

    ~TraceBuffer() {
      std::lock_guard<std::recursive_mutex> lock(logger_lock_);
      if (!finalized_.exchange(true)) {
        // finalize if not finalized
        if (!host_event_buffer_flushed_) {
          for (int i = 0; i < current_host_event_buffer_slice_; i++) {
            for (int j = 0; j < slice_capacity_; j++) {
              FlushHostEvent(host_event_buffer_[i][j]);
            }
          }
          for (int j = 0; j < next_host_event_index_; j++) {
            FlushHostEvent(host_event_buffer_[current_host_event_buffer_slice_][j]);
          }
          host_event_buffer_flushed_ = true;
        }

        for (auto& slice : host_event_buffer_) {
          free(slice);
        }
        host_event_buffer_.clear();
        trace_buffers_->erase(this);
      }
    }

    TraceBuffer(const TraceBuffer& that) = delete;
    TraceBuffer& operator=(const TraceBuffer& that) = delete;

    HostEventRecord *GetHostEvent(void) {
      if (next_host_event_index_ ==  slice_capacity_) {
        if (buffer_capacity_ == -1) {
          HostEventRecord *her = (HostEventRecord *)(malloc(sizeof(HostEventRecord) * slice_capacity_));
          UniMemory::ExitIfOutOfMemory((void *)(her));
          host_event_buffer_.push_back(her);
          current_host_event_buffer_slice_++;
          next_host_event_index_ = 0;
        }
        else {
          FlushHostBuffer();
        }
      }
      return &(host_event_buffer_[current_host_event_buffer_slice_][next_host_event_index_]);
    }

    void BufferHostEvent(void) {
      if (flush_immediately_) {
        std::lock_guard<std::recursive_mutex> lock(logger_lock_);
        FlushHostEvent(host_event_buffer_[current_host_event_buffer_slice_][next_host_event_index_]);
        // in case that flush_immediately_ is true, only one slice and one even slot, so set the flushed flag to true
        host_event_buffer_flushed_ = true;
      }
      else {
        next_host_event_index_++;
        host_event_buffer_flushed_ = false;
      }
    }

    uint32_t GetTid() { return tid_; }
    uint32_t GetPid() { return pid_; }

    std::string StringifyHostEvent(HostEventRecord& rec) {
      std::string str = ",\n{";  // header

      if (rec.type_ == EVENT_COMPLETE) {
        str += "\"ph\": \"X\"";
      } else if (rec.type_ == EVENT_DURATION_START) {
        str += "\"ph\": \"B\"";
      } else if (rec.type_ == EVENT_DURATION_END) {
        str += "\"ph\": \"E\"";
      } else if (rec.type_ == EVENT_FLOW_SOURCE) {
        str += "\"ph\": \"s\"";
      } else if (rec.type_ == EVENT_FLOW_SINK) {
        str += "\"ph\": \"t\"";
      } else if (rec.type_ == EVENT_MARK) {
        str += "\"ph\": \"R\"";
      } else {
        // should never get here
      }

      str += ", \"tid\": " + std::to_string(tid_);
      str += ", \"pid\": " + std::to_string(pid_);

      if (rec.type_ == EVENT_FLOW_SOURCE) {
        str += ", \"name\": \"dep\"";
        str += ", \"cat\": \"Flow_H2D_" + std::to_string(rec.id_) + "\"";
      } else if (rec.type_ == EVENT_FLOW_SINK) {
        str += ", \"name\": \"dep\"";
        str += ", \"cat\": \"Flow_D2H_" + std::to_string(rec.id_) + "\"";
      } else {
        if (rec.name_ != nullptr) {
          if (rec.name_[0] == '\"') {
            // name is already quoted
            str += ", \"name\": " + std::string(rec.name_);
          } else {
            str += ", \"name\": \"" + std::string(rec.name_) + "\"";
          }
        } else {
          // if ((rec.api_id_ != XptiTracingId) && (rec.api_id_ != IttTracingId)) {
          //   //str += ", \"name\": \"" + get_symbol(rec.api_id_) + "\"";
          // }
        }
        str += ", \"cat\": \"cpu_op\"";
      }

      // free rec.name_. It is not needed any more
      if (rec.name_ != nullptr) {
        free(rec.name_);
        rec.name_ = nullptr;
      }

      // It is always present
      str += ", \"ts\": " + std::to_string(UniTimer::GetEpochTimeInUs(rec.start_time_));

      if (rec.type_ == EVENT_COMPLETE) {
        str += ", \"dur\": " + std::to_string(UniTimer::GetTimeInUs(rec.end_time_ - rec.start_time_));
      }

      std::string str_args = "";  // build arguments
      if (rec.api_type_ == API_TYPE_ITT) {
        str_args = str_args + "\"" + rec.itt_args_.key + "\":[";
        str_args += convertDataToString(&rec.itt_args_);
        str_args += "]";
        if (rec.itt_args_.isIndirectData) {
          free(rec.itt_args_.data[0]);
        }
        IttArgs* args = rec.itt_args_.next;
        while (args != nullptr) {
          str_args += ",";
          str_args = str_args + "\"" + args->key + "\":[";
          str_args += convertDataToString(args);
          str_args += "]";
          IttArgs* toFree = args;
          args = args->next;
          free(toFree);
        }
        // reset count to 0 and type to API_TYPE_NONE
        rec.itt_args_.count = 0;
        rec.api_type_ = API_TYPE_NONE;
      }

      if (!str_args.empty()) {
        str += ", \"args\": {" + str_args + "}";
      } else {
        str += ", \"id\": " + std::to_string(rec.id_);
      }

      // end
      str += "}";  // footer

      return str;
    }

    void FlushHostEvent(HostEventRecord& rec) {
      logger_->Log(StringifyHostEvent(rec));
    }

    void FlushHostBuffer() {
      std::lock_guard<std::recursive_mutex> lock(logger_lock_);
      if (host_event_buffer_flushed_) {
        return;
      }

      for (int i = 0; i < current_host_event_buffer_slice_; i++) {
        for (int j = 0; j < slice_capacity_; j++) {
          FlushHostEvent(host_event_buffer_[i][j]);
        }
      }
      for (int j = 0; j < next_host_event_index_; j++) {
        FlushHostEvent(host_event_buffer_[current_host_event_buffer_slice_][j]);
      }
      current_host_event_buffer_slice_ = 0;
      next_host_event_index_ = 0;
      host_event_buffer_flushed_ = true;
    }

    void Finalize() {
      std::lock_guard<std::recursive_mutex> lock(logger_lock_);
      if (!finalized_.exchange(true)) {
        if (!host_event_buffer_flushed_) {
          for (int i = 0; i < current_host_event_buffer_slice_; i++) {
            for (int j = 0; j < slice_capacity_; j++) {
              FlushHostEvent(host_event_buffer_[i][j]);
            }
          }
          for (int j = 0; j < next_host_event_index_; j++) {
            FlushHostEvent(host_event_buffer_[current_host_event_buffer_slice_][j]);
          }
          host_event_buffer_flushed_ = true;
        }
      }
    }

    bool IsFinalized() {
      return finalized_.load(std::memory_order_acquire);
    }

  private:
    int32_t buffer_capacity_;
    int32_t slice_capacity_;	// each buffer can have multiple slices
    int32_t current_host_event_buffer_slice_;	// host slice in use
    int32_t next_host_event_index_;	// next free host event in in-use slice
    uint32_t tid_;
    uint32_t pid_;
    std::vector<HostEventRecord *> host_event_buffer_;
    bool flush_immediately_;
    bool host_event_buffer_flushed_;
    std::atomic<bool> finalized_;
    bool metrics_enabled_;
};

thread_local TraceBuffer thread_local_buffer_;

class ChromeLogger {
  private:
    TraceOptions options_;
    bool filtering_on_ = true;
    bool filter_in_ = false;  // --filter-in suggests only include/collect these kernel names in output (filter-out is reverse of this and excludes/drops these)
    std::set<std::string> filter_strings_set_;
    std::string process_name_;
    std::string chrome_trace_file_name_;
    std::iostream::pos_type data_start_pos_;
    uint64_t process_start_time_;

    ChromeLogger(const TraceOptions& options, const char* filename) : options_(options) {
      process_start_time_ = UniTimer::GetEpochTimeInUs(UniTimer::GetHostTimestamp());
      process_name_ = filename;
      chrome_trace_file_name_ = TraceOptions::GetChromeTraceFileName(filename);
      if (this->CheckOption(TRACE_OUTPUT_DIR_PATH)) {
          std::string dir = utils::GetEnv("UNITRACE_TraceOutputDir");
          chrome_trace_file_name_ = (dir + '/' + chrome_trace_file_name_);
      }

      if (this->CheckOption(TRACE_KERNEL_NAME_FILTER)) {
        if (this->CheckOption(TRACE_K_NAME_FILTER_IN)) {
          filter_in_ = true;
        }
        std::string tmpString;
        tmpString = utils::GetEnv("UNITRACE_TraceKernelString");
        filter_strings_set_.insert(std::move(tmpString));
      } else if (this->CheckOption(TRACE_K_NAME_FILTER_FILE)) {
        if (this->CheckOption(TRACE_K_NAME_FILTER_IN)) {
          filter_in_ = true;
        }
        std::string kernel_file =  utils::GetEnv("UNITRACE_TraceKernelFilePath");
        std::ifstream kfile(kernel_file, std::ios::in);
        PTI_ASSERT(kfile.fail() != 1 && kfile.eof() != 1);
        while (!kfile.eof()) {
          std::string tmpString;
          kfile >> tmpString;
          filter_strings_set_.insert(std::move(tmpString));
        }
      } else {
        filtering_on_ = false;
        filter_strings_set_.insert("ALL");
      }
      logger_ = new Logger(chrome_trace_file_name_.c_str(), true, true);
      UniMemory::ExitIfOutOfMemory((void *)(logger_));

      logger_->Log("{ \"traceEvents\":[\n");

      std::string str("{\"ph\": \"M\", \"name\": \"process_name\", \"pid\": ");

      str += std::to_string(utils::GetPid()) + ", \"ts\": " + std::to_string(process_start_time_) + ", \"args\": {\"name\": \"";

      std::string host = GetHostName();
      std::string rank_str = utils::GetEnv("PMI_RANK"); // Simplified from original
      if (rank_str.empty()) {
          rank_str = utils::GetEnv("PMIX_RANK");
      }

      if (rank_str.empty()) {
        str += "HOST<" + host + ">\"}}";
      }
      else {
        str += "RANK " + rank_str + " HOST<" + host + ">\"}}";
      }

      logger_->Log(str);
      logger_->Flush();
      data_start_pos_ = logger_->GetLogFilePosition();
    }

  public:
    ChromeLogger(const ChromeLogger& that) = delete;
    ChromeLogger& operator=(const ChromeLogger& that) = delete;

    ~ChromeLogger() {
      if (logger_ != nullptr) {
        logger_lock_.lock();
        if (trace_buffers_) {
          for (auto it = trace_buffers_->begin(); it != trace_buffers_->end();) {
            (*it)->Finalize();
            it = trace_buffers_->erase(it);
          }
        }

        logger_lock_.unlock();

        if (logger_->GetLogFilePosition() == data_start_pos_) {
          // no data has been logged
          // remove the log file, but close it first
          delete logger_;
          if (std::remove(chrome_trace_file_name_.c_str()) == 0) {
            std::cerr << "[INFO] No event of interest is logged for process " << utils::GetPid() << " (" << process_name_ << ")" << std::endl;
          } else {
            std::cerr << "[INFO] No event of interest is logged for process " << utils::GetPid() << " (" << process_name_ << ") in file " << chrome_trace_file_name_ << std::endl;
          }
        } else {
          std::string str = "\n]\n}\n";
          logger_->Log(str);
          delete logger_;
          std::cerr << "[INFO] Timeline is stored in " << chrome_trace_file_name_ << std::endl;
        }
      }
    }

    static ChromeLogger* Create(const TraceOptions& options, const char* filename) {
      ChromeLogger *chrome_logger  = new ChromeLogger(options, filename);
      UniMemory::ExitIfOutOfMemory((void *)(chrome_logger));
      return chrome_logger;
    }

    bool CheckOption(uint32_t option) {
      return options_.CheckFlag(option);
    }

    static void XptiLoggingCallback(EVENT_TYPE etype, const char *name, uint64_t start_ts, uint64_t end_ts) {
      if (!thread_local_buffer_.IsFinalized()) {
        HostEventRecord *rec = thread_local_buffer_.GetHostEvent();
        rec->type_ = etype;

        if (name != nullptr) {
          rec->name_ = strdup(name);
        } else {
          rec->name_ = nullptr;
        }

        rec->api_type_ = API_TYPE_NONE;
        rec->api_id_ = XptiTracingId;
        rec->start_time_ = start_ts;
        if (etype == EVENT_COMPLETE) {
          rec->end_time_ = end_ts;
        }
        rec->id_ = 0;
        thread_local_buffer_.BufferHostEvent();
      }
    }

    static void IttLoggingCallback(const char *name, uint64_t start_ts, uint64_t end_ts, IttArgs* metadata_args) {
      if (!thread_local_buffer_.IsFinalized()) {
        HostEventRecord *rec = thread_local_buffer_.GetHostEvent();

        rec->type_ = EVENT_COMPLETE;

        if (name != nullptr) {
          rec->name_ = strdup(name);
        } else {
          rec->name_ = nullptr;
        }

        rec->api_id_ = IttTracingId;
        rec->start_time_ = start_ts;
        rec->end_time_ = end_ts;
        rec->id_ = 0;
        if ((metadata_args != nullptr) && (metadata_args->count != 0)) {
          rec->api_type_ = API_TYPE_ITT;
          rec->itt_args_ = *metadata_args;
        }
	else {
          // no arguments so set api_type_ to API_TYPE_NONE
          rec->api_type_ = API_TYPE_NONE;
          rec->itt_args_.count = 0;
	}

        thread_local_buffer_.BufferHostEvent();
      }
    }

    static void ChromeCallLoggingCallback(std::vector<uint64_t> *kids, FLOW_DIR flow_dir, API_TRACING_ID api_id,
      uint64_t started, uint64_t ended) {
      if (thread_local_buffer_.IsFinalized()) {
        return;
      }

      HostEventRecord *rec = thread_local_buffer_.GetHostEvent();

      rec->type_ = EVENT_COMPLETE;
      rec->api_type_ = API_TYPE_NONE;
      rec->api_id_ = api_id;
      rec->start_time_ = started;
      rec->end_time_ = ended;
      rec->id_ = 0;
      rec->name_ = nullptr;
      thread_local_buffer_.BufferHostEvent();

      // Device-side flow correlation removed
    }
};

#endif // PTI_TOOLS_UNITRACE_CHROME_LOGGER_H_
