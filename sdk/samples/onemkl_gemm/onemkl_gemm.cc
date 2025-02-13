//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <exception>
#include <iostream>
#include <oneapi/mkl.hpp>
#include <string>
#include <sycl/sycl.hpp>
#include <vector>

#include "samples_utils.h"

void StartTracing() {
  PTI_THROW(ptiViewEnable(PTI_VIEW_DEVICE_GPU_KERNEL));
  PTI_THROW(ptiViewEnable(PTI_VIEW_DEVICE_GPU_MEM_FILL));
  PTI_THROW(ptiViewEnable(PTI_VIEW_DEVICE_GPU_MEM_COPY));
  PTI_THROW(ptiViewEnable(PTI_VIEW_RUNTIME_API));
}

void StopTracing() {
  PTI_THROW(ptiViewDisable(PTI_VIEW_DEVICE_GPU_KERNEL));
  PTI_THROW(ptiViewDisable(PTI_VIEW_DEVICE_GPU_MEM_FILL));
  PTI_THROW(ptiViewDisable(PTI_VIEW_DEVICE_GPU_MEM_COPY));
  PTI_THROW(ptiViewDisable(PTI_VIEW_RUNTIME_API));
}

template <typename T>
void oneMKLGemm(void) {
  // Create a sycl device of type GPU
  sycl::device my_device;
  try {
    my_device = sycl::device(sycl::gpu_selector_v);
  } catch (...) {
    std::cerr << "Warning: GPU not found !" << std::endl;
    throw;
  }

  if constexpr (!std::is_same_v<T, std::complex<float>> && sizeof(T) > sizeof(float)) {
    if (!my_device.has(sycl::aspect::fp64)) {
      std::cerr << "Warning: GPU device does not support fp64. Skipping operation" << std::endl;
      return;
    }
  }

  // Create asynchronous exceptions handler to be attached to queue.
  // Not required; can provide helpful information in case the system isn’t correctly configured.
  auto my_exception_handler = [](sycl::exception_list exceptions) {
    for (std::exception_ptr const &e : exceptions) {
      try {
        std::rethrow_exception(e);
      } catch (sycl::exception const &e) {
        std::cerr << "Caught asynchronous SYCL exception:\n" << e.what() << std::endl;
      } catch (std::exception const &e) {
        std::cerr << "Caught asynchronous STL exception:\n" << e.what() << std::endl;
      }
    }
  };

  // Create sycl queue
#if __INTEL_LLVM_COMPILER >= 20240000
  sycl::property_list prop{sycl::property::queue::in_order()};
#else
  sycl::property_list prop{};
#endif
  sycl::queue main_queue(my_device, my_exception_handler, prop);
  // Create sycl context
  auto main_context = main_queue.get_context();
  int m = 16;
  int n = 16;
  int k = 16;

  int lda = 16;
  int ldb = 16;
  int ldc = 16;
  int sizea, sizeb, sizec = ldc * n;

  T alpha = 1.0f;
  T beta = 2.0f;

  constexpr auto trans_a = oneapi::mkl::transpose::trans;
  constexpr auto trans_b = oneapi::mkl::transpose::nontrans;

  sizea = lda * m;  // sizea = (trans_a == oneapi::mkl::transpose::nontrans) ? lda * k : lda * m;
  sizeb = ldb * n;  // sizeb = (trans_b == oneapi::mkl::transpose::nontrans) ? ldb * n : ldb * k;

  // Allocate host memory
  std::vector<T> hostA(sizea);
  std::vector<T> hostB(sizeb);
  std::vector<T> hostC(sizec);

  // Allocate device memory
  auto devA = static_cast<T *>(malloc_device(sizea * sizeof(T), main_queue));
  auto devB = static_cast<T *>(malloc_device(sizeb * sizeof(T), main_queue));
  auto devC = static_cast<T *>(malloc_device(sizec * sizeof(T), main_queue));
  if (!devA || !devB || !devC) {
    throw std::runtime_error("Failed to allocate USM memory.");
  }

  main_queue.memset(devA, 0, sizea * sizeof(T)).wait();

  // Initialize host matrices
  for (int i = 0; i < m * k; i++) {
    hostA[i] = i;
  }

  for (int i = 0; i < k * n; i++) {
    hostB[i] = i;
  }

  for (int i = 0; i < m * n; i++) {
    hostC[i] = 0;
  }

  // Transfer the host data to device
  main_queue.memcpy(devA, hostA.data(), sizea * sizeof(T));
  main_queue.wait();
  main_queue.memcpy(devB, hostB.data(), sizeb * sizeof(T));
  main_queue.wait();
  main_queue.memcpy(devC, hostC.data(), sizec * sizeof(T));
  main_queue.wait();

  sycl::event gemm_done;

  // Execute oneMKL GEMM
  try {
    gemm_done = oneapi::mkl::blas::gemm(main_queue, trans_a, trans_b, m, n, k, alpha, devA, lda,
                                        devB, ldb, beta, devC, ldc);
  } catch (sycl::exception const &e) {
    std::cout << "\t\tCaught synchronous SYCL exception during GEMM:\n" << e.what() << std::endl;
  }
  gemm_done.wait();

  // Read the device produced results to host memory
  main_queue.memcpy(hostC.data(), devC, sizec * sizeof(T));
  main_queue.wait();

#ifdef VERBOSE
  for (int i = 0; i < m * n; i++) {
    std::cout << hostC[i] << " ";
  }
  std::cout << "\n";
#endif
  free(devA, main_context);
  free(devB, main_context);
  free(devC, main_context);
}

int main() {
  int exit_code = EXIT_SUCCESS;

  try {
    // Setup PTI call backs
    PTI_THROW(ptiViewSetCallbacks(
        [](auto **buf, auto *buf_size) {
          *buf_size = sizeof(pti_view_record_kernel) * 100;
          void *ptr = ::operator new(*buf_size);
          ptr = std::align(8, sizeof(unsigned char), ptr, *buf_size);
          *buf = reinterpret_cast<unsigned char *>(ptr);
          if (!*buf) {
            std::abort();
          }
          return;
        },
        [](auto *buf, auto buf_size, auto valid_buf_size) {
          if (!buf_size || !valid_buf_size || !buf_size) {
            std::cerr << "Received empty buffer" << '\n';
            if (valid_buf_size) {
              ::operator delete(buf);
            }
            return;
          }
          pti_view_record_base *ptr = nullptr;
          while (true) {
            auto buf_status = ptiViewGetNextRecord(buf, valid_buf_size, &ptr);
            if (buf_status == pti_result::PTI_STATUS_END_OF_BUFFER) {
              std::cout << "Reached End of buffer" << '\n';
              break;
            }
            if (buf_status != pti_result::PTI_SUCCESS) {
              std::cerr << "Found Error Parsing Records from PTI" << '\n';
              break;
            }
            switch (ptr->_view_kind) {
              case pti_view_kind::PTI_VIEW_INVALID: {
                std::cout << "Found Invalid Record" << '\n';
                break;
              }
              case pti_view_kind::PTI_VIEW_RUNTIME_API: {
                std::cout << "---------------------------------------------------"
                             "-----------------------------"
                          << '\n';
                std::cout << "Found Sycl Runtime Record" << '\n';
                samples_utils::DumpRecord(reinterpret_cast<pti_view_record_api *>(ptr));
                break;
              }
              case pti_view_kind::PTI_VIEW_DEVICE_GPU_MEM_FILL: {
                std::cout << "---------------------------------------------------"
                             "-----------------------------"
                          << '\n';
                std::cout << "Found Memory Record" << '\n';
                samples_utils::DumpRecord(reinterpret_cast<pti_view_record_memory_fill *>(ptr));
                std::cout << "---------------------------------------------------"
                             "-----------------------------"
                          << '\n';
                break;
              }
              case pti_view_kind::PTI_VIEW_DEVICE_GPU_MEM_COPY: {
                std::cout << "---------------------------------------------------"
                             "-----------------------------"
                          << '\n';
                std::cout << "Found Memory Record" << '\n';
                samples_utils::DumpRecord(reinterpret_cast<pti_view_record_memory_copy *>(ptr));
                std::cout << "---------------------------------------------------"
                             "-----------------------------"
                          << '\n';
                break;
              }
              case pti_view_kind::PTI_VIEW_DEVICE_GPU_KERNEL: {
                pti_view_record_kernel *rec = reinterpret_cast<pti_view_record_kernel *>(ptr);
                std::cout << "---------------------------------------------------"
                             "-----------------------------"
                          << '\n';
                std::cout << "Found Kernel Record" << '\n';
                samples_utils::DumpRecord(rec);

                std::cout << "---------------------------------------------------"
                             "-----------------------------"
                          << '\n';
                if (samples_utils::isMonotonic({rec->_sycl_task_begin_timestamp,
                                                rec->_sycl_enqk_begin_timestamp,
                                                rec->_append_timestamp, rec->_submit_timestamp,
                                                rec->_start_timestamp, rec->_end_timestamp})) {
                  std::cout << "------------>     All Monotonic" << std::endl;
                } else {
                  std::cout << "------------>     Something wrong: NOT All monotonic" << std::endl;
                }
                if (rec->_sycl_task_begin_timestamp == 0)
                  std::cout << "------------>     Something wrong: Sycl Task Begin Time is 0"
                            << std::endl;
                if (rec->_sycl_enqk_begin_timestamp == 0)
                  std::cout << "------------>     Something wrong: Sycl Enq Launch Kernel Time is 0"
                            << std::endl;

                break;
              }
              default: {
                std::cerr << "This shouldn't happen" << '\n';
                break;
              }
            }
          }
          ::operator delete(buf);
        }));

    // Initate PTI Tracing
    StartTracing();
    // Invoke SGEMM
    oneMKLGemm<float>();
    // Stop PTI tracing
    StopTracing();

    // Initate PTI Tracing
    StartTracing();
    // Invoke DGEMM
    oneMKLGemm<double>();
    // Stop PTI tracing
    StopTracing();

    // Initate PTI Tracing
    StartTracing();
    // Invoke CGEMM
    oneMKLGemm<std::complex<float>>();
    // Stop PTI tracing
    StopTracing();

    // Initate PTI Tracing
    StartTracing();
    // Invoke ZGEMM
    oneMKLGemm<std::complex<double>>();

    StopTracing();

    PTI_THROW(ptiFlushAllViews());

  } catch (const sycl::exception &e) {
    std::cerr << "Error: Exception while executing SYCL " << e.what() << '\n';
    std::cerr << "\tError code: " << e.code().value() << "\n\tCategory: " << e.category().name()
              << "\n\tMessage: " << e.code().message() << '\n';
    exit_code = EXIT_FAILURE;
  } catch (const std::exception &e) {
    std::cerr << "Error: Exception caught " << e.what() << '\n';
    exit_code = EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Error: Unknown exception caught." << '\n';
    exit_code = EXIT_FAILURE;
  }
  // Flush all the PTI views
  return exit_code;
}
