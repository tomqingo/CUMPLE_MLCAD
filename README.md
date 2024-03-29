# CUMPLE_MLCAD

# Intro

The location of the macros (BRAMs and DSPs) plays an important role in the overall routability and timing performance. The Cumple tool is enhanced macro placer based on the SimPL framework when two important design constraints in the industry are considered, relative placement constraint (RPC) and regional constraint (RC). Both RPC and RC are important to achieve the timing closure and would increase the difficulties with sparse locations and increased localized density. The Cumple tool is able to decrease congestion levels under these two constraints after intergrating the Vivado ML 2021.1. Although initially our macro placer ranks 3rd place in MLCAD 2023 FPGA Macro Placement Contest, the enhanced version is competitive with the open-sourced DreamplaceFPGA-MP. In particular, our macro placement performs better on those cases with both RPC and RC.

# Benchmarks

The overall evaluation is based on the MLCAD 2023 contest benchmark. The contest website is:

<https://github.com/TILOS-AI-Institute/MLCAD-2023-FPGA-Macro-Placement-Contest>

The link of the contest benchmark is:

<https://www.kaggle.com/datasets/ismailbustany/updated-mlcad-2023-contest-benchmark>


You could download the benchmark from this link. And then hard link the benchmark under the repo with the
following commands:
~~~
cd CUMPLE_MLCAD
mkdir benchmarks
ln -s <benchmark> benchmarks/mlcad2023_v2
~~~

# Vivado Environment
If you want to check the routability of the macro placement solution, you are required to install the Vivado ML 2021.1, the link of downloading the Vivado is:

<https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/archive.html>

Meanwhile, you could obtain the license from the AMD University Program:

<https://www.amd.com/en/corporate/university-program/donation-program.html>

# Input file format

design.nodes: nodes in the netlist <br>
design.nets: nets in the netlist <br>
design.lib: library containing all the cells <br>
design.scl: distribution of the macros on the FPGA board <br>
design.cascade_shape: cascaded macros type <br>
design.cascade_shape_instances: cascade macro instances <br>
design.regions: regions constraining all the macros <br>
design.pl : fixed IBUF/OBUF ports on the FPGA <br>
design.dcp : the dcp/checkpoint file in Vivado <br>

# Output file format
macroplacement.pl: the location of the macros <br>
place_io.tcl: the tcl scripts placing the IBUF/OBUF ports <br>
place_macro.tcl : the tcl scripts placing the macros <br>
flow.tcl: the tcl scripts of the overall flow of place and route in Vivado <br>
run.log: the log file of the macro placement <br>
vivado.log: the report of overall place and route in the vivado <br>

# API
~~~
binary name: Cumple 
options:
--benchmark_path: the path storing the benchmark folders (default: ../benchmarks/mlcad2023_v2/)
--flow: the flow of the place and route (cumple: simply macro placement, all: overall place and route)
--log_dir: the directory storing all the output files
~~~


# How to run
**Step 1:** Go to the project root and build by
~~~
$ cd CUMPLE_MLCAD
$ ./scripts/build.py -o release
~~~
**Step 2:** Run the designs by
~~~
$ cd run
$ python3 run.py <case> --benchmark_path <bencmark_path> --flow <flow> --log_dir <path_log>
~~~
For example, if we want to run Design_2 with the overall place and route flow, then
~~~
$ python3 run.py d2 --benchmark_path ../benchmarks/mlcad2023_v2/ --flow all --log_dir case_2
~~~
If we want to run all the testcases in the MLCAD2023 benchmark in parallel, then:
~~~
bash run_all.sh <parallel_num> <start_id> <end_id> <log_dir>
~~~
For example, if we want to run all the cases with 5 cases in parallel, then
~~~
bash run_all.sh 5 0 140 version_1
~~~
**Step 3:** Calculate the congestion scores with statistics.py scripts
~~~
python3 statistics.py <log_dir>
~~~
For example, if we want to obtain the congestion scores in version_1, then
~~~
python3 statistics.py version_1
~~~

### Dependencies

* g++ (version >= 5.4.0) or other working c++ compliers
* CMake (version >= 3.5.1)
* Boost (version >= 1.58)
* Python (version 3, optional, for using scripts, also pyjson5 is required)

### Acknowledge

In the end, we are sincerely grateful to Tingyuan Liang for his great work of AMFPlacers, an open-sourced Mixed Size FPGA placers (<https://github.com/zslwyuan/AMF-Placer>), and we have learned a lot from it.