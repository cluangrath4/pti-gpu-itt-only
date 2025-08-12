# Unified Tracing and Profiling Tool

## Introduction

This a performance tool for Intel(R) oneAPI applications. It traces and profiles host/device activities, interactions and hardware utilizations for
Intel(R) GPU applications.

## Supported Platforms

- Linux
- Intel(R) oneAPI Base Toolkits
- Intel(R) GPUs including Intel(R) Data Center GPU Max Series

## Requirements

- CMake 3.22 or above (CMake versions prior to 3.22 are not fully tested or validated)
- C++ compiler with C++17 support
- Intel(R) oneAPI Base Toolkits
- Python 3.9 or later
- Matplotlib 3.8 or later (https://matplotlib.org/)
- Pandas 2.2.1 or later (https://pandas.pydata.org/)
- Intel(R) MPI (optional)

## Build and Install

```sh
set up Intel(R) oneAPI environment
set up Intel(R) MPI Environment (optional)
```

### Linux

```sh
cd <....>/tools/unitrace
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
or
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
or
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_MPI=<0|1> ..
make
or
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_MPI=<0|1> -DCMAKE_INSTALL_PREFIX=<installpath> ..
make install
or
make install
```

**BUILD_WITH_ITT=<1/0>** to enable/disable oneCCL/oneDNN profiling support (enabled by default),\

Example:

```
cmake -DBUILD_WITH_ITT=1 ..
```

## Test

After unitrace is built, run ctest from the build folder:

```sh
ctest -V
```
or run test_unitrace.py from the test folder

```sh
cd test
python test_unitrace.py
```

By default, command **python test_unitrace.py** builds and runs all the tests. If the tests are already built and rebuilding the tests is not needed, you can use **--run** to skip buidling the tests:

```sh
cd test
python test_unitrace.py --run
```

The default testing scenarios are defined in file **test/scenarios.txt**. You can edit this file to change the scenarios. You can also create your own testing scenarios in a .txt file, for example, **myscenarios.txt**:

```sh
    -c
    -h
    --chrome-call-logging 
    --chrome-kernel-logging
```

and test the scenarios:

```sh
cd test
python test_unitrace.py --config myscenarios.txt
```

To create and add a new test, for example, **mytest**, you need to add the following statement in the **CMakeLists.txt** file of the new test:

```sh
add_test(NAME mytest COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/run_test.py ${CMAKE_SOURCE_DIR} mytest <args>)
```

The arguments of the test can be passed in **args**.


If the new test is in a new folder, for example, **mytestdir**, you also need to add the new folder to the top **CMakeLists.txt** file:

```sh
add_subdirectory(mytestdir)
```

By default, only ITT tracing/profiling is enabled.

### View Traces and Profiles

If one or more of the **--chrome-** options are used, a .json file, for example, **myapp.json**, will be generated. To view the trace, please open **https://ui.perfetto.dev/** in either Google Chrome or Microsoft Edge browser and load the .json file. **Do NOT use chrome://tracing/**! 

### Location of Output

By default, all output profile data are written to files in the current working directory. You can use the **--output-dir-path** option to specify a different location:

```
unitrace --chrome-kernel-logging --output-dir-path /tmp/unitrace-result myapp
```

The output profile data are written to files in **/tmp/unitrace-result**.

This option is especially useful when the application is distributed workload.

### Hardware Performance Metrics

Hardware performance metric counter can be profiled at the same time while host/device activities are profiled in the same run or they can be done in separate runs.

Please note that device timing is also enabled if hardware performance metric counter profiling is enabled. The device timing information guides you to the hot kernels so you know which kernel's performance counters are of most interest.

#### Sample Stalls at Instruction Level

##### List Contents of the Metric Data File

The first step is to inspect the contents of the metric data file using **-l** or **--list** option. 

   ```sh
   python analyzeperfmetrics.py -l perfmetrics.12345.csv
   ```

This shows the device, metrics and kernels profiled:

```sh
Device 0
    Metric
        GpuTime[ns]
        GpuCoreClocks[cycles]
        AvgGpuCoreFrequencyMHz[MHz]
        GpuSliceClocksCount[events]
        AvgGpuSliceFrequencyMHz[MHz]
        L3_BYTE_READ[bytes]
        L3_BYTE_WRITE[bytes]
        GPU_MEMORY_BYTE_READ[bytes]
        GPU_MEMORY_BYTE_WRITE[bytes]
        XVE_ACTIVE[%]
        XVE_STALL[%]
        XVE_BUSY[events]
        XVE_THREADS_OCCUPANCY_ALL[%]
        XVE_COMPUTE_THREAD_COUNT[threads]
        XVE_ATOMIC_ACCESS_COUNT[messages]
        XVE_BARRIER_MESSAGE_COUNT[messages]
        XVE_INST_EXECUTED_ALU0_ALL[events]
        XVE_INST_EXECUTED_ALU1_ALL[events]
        XVE_INST_EXECUTED_XMX_ALL[events]
        XVE_INST_EXECUTED_SEND_ALL[events]
        XVE_INST_EXECUTED_CONTROL_ALL[events]
        XVE_PIPE_ALU0_AND_ALU1_ACTIVE[%]
        XVE_PIPE_ALU0_AND_XMX_ACTIVE[%]
        XVE_INST_EXECUTED_ALU0_ALL_UTILIZATION[%]
        XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION[%]
        XVE_INST_EXECUTED_SEND_ALL_UTILIZATION[%]
        XVE_INST_EXECUTED_CONTROL_ALL_UTILIZATION[%]
        XVE_INST_EXECUTED_XMX_ALL_UTILIZATION[%]
        QueryBeginTime[ns]
        CoreFrequencyMHz[MHz]
        XveSliceFrequencyMHz[MHz]
        ReportReason
        ContextIdValid
        ContextId
        SourceId
        StreamMarker
    Kernel, Number of Instances
        "main::{lambda(auto:1)#1}[SIMD32 {8192; 1; 1} {128; 1; 1}]", 1
        "main::{lambda(auto:1)#2}[SIMD32 {8192; 1; 1} {128; 1; 1}]", 1
        "main::{lambda(auto:1)#3}[SIMD32 {8192; 1; 1} {128; 1; 1}]", 1
        "main::{lambda(auto:1)#4}[SIMD32 {4096; 1; 1} {256; 1; 1}]", 5
        "main::{lambda(auto:1)#5}[SIMD32 {4096; 1; 1} {256; 1; 1}]", 5
        "main::{lambda(auto:1)#6}[SIMD32 {4096; 1; 1} {256; 1; 1}]", 5
        "main::{lambda(auto:1)#7}[SIMD32 {2048; 1; 1} {512; 1; 1}]", 5
```

The **Device** is the device on which the metrics are sampled. In this example output, the decice is 0. If multiple devices are used and sampled, multiple sections of **Device** will be present.

The **Metric** section shows the metrics collected on the device and the **Kernel, Number of Instances** shows the kernels and number of instances for each kernel are profiled. An instance is one kernel execution sampled on the device. For example, The kernel "main::{lambda(auto:1)#4}[SIMD32 {4096; 1; 1} {256; 1; 1}]" having 5 instances means the 5 exeuctions of the kernel are sampled. Please note that the number of instances of a kernel here may be less than the total number of exeuctions or submissions of the kernel in the application, especially when the kernel is short and/or sampling interval is large. 

The number of instances is not applicable to stall sampling metric data:

```sh
Device 0
    Metric
        Active[Events]
        ControlStall[Events]
        PipeStall[Events]
        SendStall[Events]
        DistStall[Events]
        SbidStall[Events]
        SyncStall[Events]
        InstrFetchStall[Events]
        OtherStall[Events]
    Kernel
        "main::{lambda(auto:1)#1}"
        "main::{lambda(auto:1)#2}"
        "main::{lambda(auto:1)#3}"
        "main::{lambda(auto:1)#5}"
        "main::{lambda(auto:1)#6}"
        "main::{lambda(auto:1)#7}"
        "main::{lambda(auto:1)#4}"
```

You can also use the **-o** option to redirect the output to a text file for later reference:

   ```sh
   python analyzeperfmetrics.py -l -o contents.txt perfmetrics.12345.csv
   ```

##### Analyze Kernel Performance Metrics

Once you have the knowledge of the device, the metrics and the kernels in the metric data file, you can run the same script to analyze specific performance metrics of a specific instance of a specific kernel, all instances of a specific kernel or all instances of all kernels executed on a specific device. The performance chart will be stored in a PDF file. 

   ```sh
   python analyzeperfmetrics.py -d 0 -k "main::{lambda(auto:1)#4}[SIMD32 {4096; 1; 1} {256; 1; 1}]" -i 2 -m "XVE_STALL[%],XVE_INST_EXECUTED_ALU0_ALL_UTILIZATION[%],XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION[%],XVE_INST_EXECUTED_SEND_ALL_UTILIZATION[%],XVE_INST_EXECUTED_CONTROL_ALL_UTILIZATION[%],XVE_INST_EXECUTED_XMX_ALL_UTILIZATION[%]" -y "Utilization and Stall (%)" -t "Utilization and Stall" -o perfchart.pdf perfmetrics.12345.csv
   ```

This command plots a chart of XVE stall and function unit utilizations for the **second** instance of kernel **"main::{lambda(auto:1)#4}[SIMD32 {4096; 1; 1} {256; 1; 1}]"** profiled on device **0** and stores the chart in file **perfchart.pdf**.

![Analyze Kernel Performance Metrics!](/tools/unitrace/doc/images/perfchart.png)

If instance is 0, all 5 instances of the kernel **"main::{lambda(auto:1)#4}[SIMD32 {4096; 1; 1} {256; 1; 1}]"** are analyzed.

   ```sh
   python analyzeperfmetrics.py -d 0 -k "main::{lambda(auto:1)#4}[SIMD32 {4096; 1; 1} {256; 1; 1}]" -i 0 -m "XVE_STALL[%],XVE_INST_EXECUTED_ALU0_ALL_UTILIZATION[%],XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION[%],XVE_INST_EXECUTED_SEND_ALL_UTILIZATION[%],XVE_INST_EXECUTED_CONTROL_ALL_UTILIZATION[%],XVE_INST_EXECUTED_XMX_ALL_UTILIZATION[%]" -y "Utilization and Stall (%)" -t "Utilization and Stall" -o perfchart.pdf perfmetrics.12345.csv

   ```

If **-k** option is not present and instance is 0, all instances of all kernels are analyzed.

   ```sh
   python analyzeperfmetrics.py -d 0 -i 0 -m "XVE_STALL[%],XVE_INST_EXECUTED_ALU0_ALL_UTILIZATION[%],XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION[%],XVE_INST_EXECUTED_SEND_ALL_UTILIZATION[%],XVE_INST_EXECUTED_CONTROL_ALL_UTILIZATION[%],XVE_INST_EXECUTED_XMX_ALL_UTILIZATION[%]" -y "Utilization and Stall (%)" -t "Utilization and Stall" -o perfchart.pdf perfmetrics.12345.csv

   ```

The **-m** option can be repeated multiple times to analyze multiple sets of metrics at the same time, for example:

   ```sh
   python analyzeperfmetrics.py -d 0 -k "main::{lambda(auto:1)#4}[SIMD32 {4096; 1; 1} {256; 1; 1}]" -i 2 -m "XVE_STALL[%],XVE_INST_EXECUTED_ALU0_ALL_UTILIZATION[%],XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION[%],XVE_INST_EXECUTED_SEND_ALL_UTILIZATION[%],XVE_INST_EXECUTED_CONTROL_ALL_UTILIZATION[%],XVE_INST_EXECUTED_XMX_ALL_UTILIZATION[%]" -y "Utilization and Stall (%)" -m "L3_BYTE_READ[bytes],L3_BYTE_WRITE[bytes]" -y "L3 Cache Read/Write (bytes)" -o perfchart.pdf perfmetrics.12345.csv
   ```

![Analyze Multiple Performance Metric Sets!](/tools/unitrace/doc/images/perfchart-multi-sets.png)

Instead of typing the command options in every run, you can store the options in a text configuration file and use the **-f** or **--config** option to read the options from the file. For example, the command options above can be stored in a **myconfig.txt**:

   ```sh
   python analyzeperfmetrics.py -f myconfig.txt -o perfchart.pdf perfmetrics.12345.csv
   ```

Please note that the input file cannot be present in the command option configuration file.

You can also use the **-b** option together with one or more **-m** options to get throughput data.

![Analyze Multiple Performance Metric Sets and Throughputs!](/tools/unitrace/doc/images/throughput.png)

If the input metric data file has stall sampling events collected using **--stall-sampling** option, the chart generated shows stall events and instruction addresses.

![Analyze Stall Metrics!](/tools/unitrace/doc/images/stallchart.png)

From this chart, we can easily see that the most stalls are **SbidStalls** at instruction **0x000001B8**. To reduce or eliminate the stalls, we need to analyze the stalls at instruction level to find out the cause of the stalls.

###### Use Pre-configured Options #####

A pre-configured option file **metrics/config/ComputeBasic.txt** for **ComputeBasic** (the default metric group) on PVC is provided. You can use it as it is:

   ```sh
   python analyzeperfmetrics.py -f config/pvc/ComputeBasic.txt -o perfchart.pdf perfmetrics.12345.csv
   ```

Or you can customize it to create your own recipes. 

You may also want to create configurations for other metric groups and/or devices.

##### Analyze Stalls at Instruction Level

To pinpoint stalls to exact instructions, you need kernel shader dump. If you want to pinpoint to the source lines and source files, you also need to use the compiler option **-gline-tables-only**, for example:

    ```sh
    icpx -fsycl -gline-tables-only -O2 -o mytest mytest.cpp
    ````

To get a kernel shader dump, run the application with environment variables **IGC_ShaderDumpEnable** and **IGC_DumpToCustomDir** set:

    ```sh
    IGC_ShaderDumpEnable=1 IGC_DumpToCustomDir=dump mytest
    ````

After you run unitrace using **--stall-sampling**, you can run the same script to analyze stalls:

    ```sh
     python analyzeperfmetrics.py -k "main::{lambda(auto:1)#3}" -s ./dump -o stallchart.pdf ./stallmetrics.56789.csv
    ```

In addition to the stall statistics chart, a stall analysis report is also generated:

```sh
Kernel: main::{lambda(auto:1)#3}
Assembly with instruction addresses: ./dump.3/OCL_asme5a3f8d8dcdadd7e_simd32_entry_0003.asm.ip
***********************************************************************************************
Sbid Stalls:

Instruction
  /* [000001B8] */         sync.nop                             null                             {Compacted,$5.dst}     // $15
  Line 40:  c[index] = a[index] + b[index];
  File: /nfs/pdx/home/zma2/Box/PVC/grf.cpp
is stalled potentially by
  instruction
    /* [00000198] */         load.ugm.d32.a64 (32|M0)  r18:2         [r14:4]            {I@1,$5} // ex_desc:0x0; desc:0x8200580 // $14
    Line 40:  c[index] = a[index] + b[index];
    File: /nfs/pdx/home/zma2/Box/PVC/grf.cpp

Instruction
  /* [00000118] */ (W)     mul (1|M0)               acc0.0<1>:d   r5.0<0;1,0>:d     r0.2<0;1,0>:uw   {Compacted,A@1,$3.dst} //  ALU pipe: int; $4
  Line 96:  return __spirv_BuiltInGlobalInvocationId.x;
  File: /opt/hpc_software/compilers/intel/nightly/20240527/compiler/latest/bin/compiler/../../include/sycl/CL/__spirv/spirv_vars.hpp
is stalled potentially by
  instruction
    /* [00000100] */ (W)     load.ugm.d32x8t.a32.ca.ca (1|M0)  r5:1  bti[255][r126:1]   {I@1,$3} // ex_desc:0xFF000000; desc:0x6218C500 //
    Line 1652:  }
    File: /opt/hpc_software/compilers/intel/nightly/20240527/compiler/latest/bin/compiler/../../include/sycl/handler.hpp
```

The report shows not only the instruction and the location (address, source line and source file if available) of each stall, but also the instruction and the location if available that causes the stall.

To eliminate or reduce the stalls, you need to fix the cause.

The stall analysis report can also be stored in a text file if you prefer:


    ```sh
     python analyzeperfmetrics.py -k "main::{lambda(auto:1)#3}" -s ./dump -o stallchart.pdf -r stallreport.txt ./stallmetrics.56789.csv
    ```

##### View Kernel Performance Metrics in a Browser

If you use **-k** or **-q** or **--stall-sampling** option together with **--chrome-kernel-logging** or **--chrome-device-logging** option, you can use **analyzeperfmetrics.py** to view metrics in a browser.  
    
    ```sh
     $ unitrace -k --chrome-kernel-logging -o perf.csv ./testapp
     ... ...

     [INFO] Log is stored in perf.1092793.csv
     [INFO] Timeline is stored in testapp.1092793.json
     [INFO] Device metrics are stored in perf.metrics.1092768.csv
    ```

The performance metrics are stored in file **perf.metrics.1092768.csv** and the event trace is stored in the .json file. 

Run **analyzeperfmetrics.py** in a shell window with **-q** option, for example:

    ```sh
     $ python analyzeperfmetrics.py -q -m "XVE_STALL[%],XVE_INST_EXECUTED_ALU0_ALL_UTILIZATION[%],XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION[%],XVE_INST_EXECUTED_SEND_ALL_UTILIZATION[%],
     XVE_INST_EXECUTED_CONTROL_ALL_UTILIZATION[%],XVE_INST_EXECUTED_XMX_ALL_UTILIZATION[%]" -y "Stall and Utilizations" -t "Stall and Utilizations" ./perf.metrics.1092768.csv
    ```

The **-q** option starts a http server.

If you have a .json file that contains https links generated using earlier versions of the tool, please use **-p** instead of **-q**. In this case, if no certificate and private key are provided, a self-signed certificate and private key will be generated and used.
    
Now load the event trace .json file into https://ui.perfetto.dev:

![Performance Metrics Through Event Trace!](/tools/unitrace/doc/images/perfmetricstrace.png)
    
Once you click the link next to **metrics:** in the **"Arguments"**, another browser window is opened:

![Performance Metrics Browswe Window!](/tools/unitrace/doc/images/perfmetricsbrowser.png)

The metrics shown in the browser are the metrics passed to the **-m** option when you start **analyzeperfmetrics.py**. If you stop and restart **analyzeperfmetrics.py** with a different set of metrics passed to **-m** option, for example:

    ```sh
     $ python analyzeperfmetrics.py -p -m "L3_BYTE_READ[bytes],L3_BYTE_WRITE[bytes]" -y "Bytes" -t "L3 Traffic" ./grfbasic.metrics.1092768.csv
    ```

Refreshing the same link will show the new metrics:

![Performance Metrics Browswe Window #2!](/tools/unitrace/doc/images/perfmetricsbrowser2.png)

In case of stall sampling, for example:

    ```sh
     $ unitrace --stall-sampling --chrome-kernel-logging -o perfstall.csv ./testapp
    ```

The **-m** option is not required for **analyzeperfmetrics.py**:

    ```sh
    python analyzeperfmetrics.py -s ./dump.1 -p ./perfstall.metrics.564289.csv -t "XVE Stall Statistics and Report"
    ```

Rereshing the same link will show stall statistics by type and instruction address:

![Stall Statistics!](/tools/unitrace/doc/images/stallstatistics.png)

followed by source stall analysis report:

![Stall Report!](/tools/unitrace/doc/images/stallreport.png)

## Activate and Deactivate Tracing and Profiling at Runtime

By default, the application is traced/profiled from the start to the end. In certain cases, however, it is more efficient and desirable to
dynamically activate and deactivate tracing at runtime. You can do so by using **--conditional-collection** option together with setting and
unsetting environment variable **"PTI_ENABLE_COLLECTION"** in the application:

```cpp
// activate tracing
setenv("PTI_ENABLE_COLLECTION", "1", 1);
// tracing is now activated

......

// deactivate tracing
setenv("PTI_ENABLE_COLLECTION", "0", 1);
// tracing is now deactivated
```

The following command traces events only in the code section between **setenv("PTI_ENABLE_COLLECTION", "1", 1)** and
**unsetenv("PTI_ENABLE_COLLECTION")**.

```sh
unitrace --chrome-call-logging --chrome-kernel-logging --conditional-collection <application> [args]
```

You can also use __itt_pause() and __itt_resume() APIs to pause and resume collection:

```cpp
__itt_pause();
// tracing is now deactivated

......

__itt_resume();
// tracing is now activated
```

If both environment variable **PTI_ENABLE_COLLECTION** and **__itt_pause()/__itt_resume()** are present, **__itt_pause()/__itt_resume()** takes precedence.

By default, collection is disabled when the application is started. To change the default, you can start the application with  **PTI_ENABLE_COLLECTION** set to 1, for example:

```sh
PTI_ENABLE_COLLECTION=1 unitrace --chrome-call-logging --chrome-kernel-logging --conditional-collection <application> [args]
```

If **--conditional-collection** option is not specified, however, PTI_ENABLE_COLLECTION settings or __itt_pause()/__itt_resume() calls have **no** effect and the application is traced/profiled from the start to the end.

## Profile MPI Workloads

### Run Profiling

To profile an MPI workload, you need to launch the tool on each rank, for example:

```sh
mpiexec <options> unitrace ...
```

You can also do selective rank profiling by launching the tool only on the ranks of interest.

The result .json file has the rank id embedded as **<application>.<pid>.<rank>.json**.

### Extended Support for Intel® MPI

With Intel® MPI version 2021.15 and above, extra information can be profiled that may be critical to performance optimization, for example, the idle time caused by communication or application imbalance:

![MPI Application Imbalance!](/tools/unitrace/doc/images/mpi-imbalance.png)

and the device-initiated communications that are executued on host:

![MPI Device-initiated Communications!](/tools/unitrace/doc/images/mpi-device-initiated.png)

The argument `mpi_counter` in the device-initiated communication event is a non-negative integer. It represents operation sequence number for identifying particular call triggered by a GPU kernel.

![mpi_counter Argument!](/tools/unitrace/doc/images/mpi-counter-parameter.png)

### Merge and View Traces from Multiple MPI Ranks

You can view the result files rank by rank. But often you may want to view the traces from multiple ranks at the same time.
To view traces from multiple MPI ranks, you can use **scripts/tracemerge/mergetrace.py** script to merge them first and then load the merged trace into
manually load the merged trace into https://ui.perfetto.dev/ or use **uniview.py**.

```sh
python mergetrace.py -o <output-trace-file> <input-trace-file-1> <input-trace-file-2> <input-trace-file-3> ...
```
![Multiple MPI Ranks Host-Device Timelines!](/tools/unitrace/doc/images/multipl-ranks-timelines.png)

## Profile PyTorch

To profile PyTorch, you need to enclose the code to be profiled with

```sh
with torch.autograd.profiler.emit_itt():
    ......
```

For example:

```sh
with torch.autograd.profiler.emit_itt(record_shapes=False):
    for batch_idx, (data, target) in enumerate(train_loader):
        optimizer.zero_grad()
        data = data.to("xpu")
        target = target.to("xpu")
        with torch.xpu.amp.autocast(enabled=True, dtype=torch.bfloat16):
            output = model(data)
            loss = criterion(output, target)
        loss.backward()
        optimizer.step()
```

You also need to use one or more options from **--chrome-mpi-logging**,  **--chrome-ccl-logging** and **--chrome-dnn-logging** to enable PyTorch prifling, for example:

```sh
unitrace --chrome-ccl-logging python ./rn50.py
```
or
```sh
unitrace --chrome-dnn-logging python ./rn50.py
```
or
```sh
unitrace --chrome-mpi-logging --chrome-ccl-logging --chrome-dnn-logging python ./rn50.py
```

Other options can be used together with one or more of these options, for example:

```sh
unitrace --chrome-kernel-logging --chrome-mpi-logging --chrome-ccl-logging --chrome-dnn-logging python ./rn50.py
```

![PyTorch Profiling!](/tools/unitrace/doc/images/pytorch.png)

You can use **PTI_ENABLE_COLLECTION** environment variable to selectively enable/disable profiling.

```sh
with torch.autograd.profiler.emit_itt(record_shapes=False):
    os.environ["PTI_ENABLE_COLLECTION"] = "1"
    for batch_idx, (data, target) in enumerate(train_loader):
        optimizer.zero_grad()
        data = data.to("xpu")
        target = target.to("xpu")
        with torch.xpu.amp.autocast(enabled=True, dtype=torch.bfloat16):
            output = model(data)
            loss = criterion(output, target)
        loss.backward()
        optimizer.step()
        if (batch_idx == 2):
            os.environ["PTI_ENABLE_COLLECTION"] = "0"
```
Alternatively, you can use itt-python to do selective profiling as well. The itt-python can be installed from conda-forge

```sh
conda install -c conda-forge --override-channels itt-python
```

```sh
import itt
......

with torch.autograd.profiler.emit_itt(record_shapes=False):
    itt.resume()
    for batch_idx, (data, target) in enumerate(train_loader):
        optimizer.zero_grad()
        data = data.to("xpu")
        target = target.to("xpu")
        with torch.xpu.amp.autocast(enabled=True, dtype=torch.bfloat16):
            output = model(data)
            loss = criterion(output, target)
        loss.backward()
        optimizer.step()
        if (batch_idx == 2):
            itt.pause()
```

```sh
unitrace --chrome-kernel-logging --chrome-dnn-logging --conditional-collection python ./rn50.py
```

## Categorize GPU Kernels

In case of a large application, for example, LLaMA, there may be a lot of small kernels with long 
kernel names in the profiled data. To analyze the data at a high level, you may find the kernel categorizing script are helpful.

The summarizing and categorizing script analyzes unitrace reports and aggregates kernels by categories. The summary is stored in `JSON` format (see example below) for further analysis or as input to other tools.

```json
{
  "allreduce_time": 660801949838.875,
  "allreduce_calls": 6937803.0,
  "matmul_time": 163155211000.0,
  "matmul_calls": 11520000.0,
  "attn_time": 53528349820.0,
  "attn_calls": 3276800.0,
  "norm_time": 26989135820.0,
  "norm_calls": 3379201.0,
  "mem_op_time": 4660369802.857142,
  "mem_op_calls": 873879.0,
}
```

Before runing the script, all summary outputs from all processes should be packed into a single `tarball`. 

```sh
unitrace -h -d -r mpirun -np 4 python ...
tar cfz unitrace_4_mpi_100iter.tgz output.*
```

The summary script takes the `tarball` and a schema file as inputs and aggregates kernels from all processes before categorizing them.

```sh
python summary.py --input unitrace_4_mpi_100iter.tgz --schema schemas/LLaMA.json --output summary_4_mpi_100iter.json
```

The schema defines classes of kernels and how the kerenls should be categorized in INI format:

```ini
[matmul if equals to]
gemm_kernel

[matmul if starts with]
xpu::xetla::hgemm_caller
xpu::xetla::HgemmQKVKernel

[allreduce if ends with]
ALLREDUCE_SMALL
ALLREDUCE_MEDIUM
ALLREDUCE_LARGE
```

Each section starts with a category name (matmul, allreduce etc.) and a condition followed by a list of kernel names.
There are 3 kinds of conditions: equals to, starts with and ends with.

Before it can be used with `summary.py`, a schema needs to be converted to JSON format:

```sh
python categorize.py --input LLaMA.ini --output LLaMA.json
```
 
## GPU Roofline

It is often desirable to estimate the peak performance of a GPU kernel on a specific device using the roofline model.  This can be done by running the **roofline.py** tool. 

```sh
$ python roofline.py -h
options:
  -h, --help            show this help message and exit
  --compute COMPUTE     compute metrics collected by unitrace
  --memory MEMORY       memory metrics collected by unitrace
  --app APP             application to profile
  --device DEVICE       device configuration file
  --output OUTPUT       output file in HTML format
  --unitrace UNITRACE   path to unitrace executable if not in PATH

```
You can get the roofline in 2 ways:

1. run unitrace and roofline model at the same time 

```sh

python roofline.py --app <application> --device <device-config> --output <output-file>

```

If the path of **unitrace** is not set in your PATH environment, you need to explicitly specify it using **--unitrace** option:

```sh

python roofline.py --app <application> --device <device-config> --output <output-file> --unitrace <path-to-unitrace>

```

2. run unitrace to profile the application, then run roofline model

This is useful if you want to profile your workload once and later run the roofline model multiple times, or if you profile your workload on one machine, but need to run the roofline model on a different machine.

```sh

python roofline.py  --compute <compute-profile> --memory <memory-profile> --device <device-config> --output <output-file>

```
The `<compute-profile>` is created by running:

```sh

unitrace --g ComputeBasic -q --chrome-kernel-logging -o <compute-profile> <application>

```
The `<memory-profile>` is created by running: 

```sh

unitrace --g VectorEngine138 -q --chrome-kernel-logging -o <memory-profile> <application>

```
The `<device-config>` is required in both usage cases. This file is device specific and can be found in the **device_configs** folder.

The `<output-file>` is in HTML format. It can be loaded and viewd in a browser:

![GPU Roofline and Kernel Summary!](/tools/unitrace/doc/images/roofline.png)

## Recommended Usage

Only use to trace for ITT
boom
