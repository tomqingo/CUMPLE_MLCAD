#!/usr/bin/env python3

import argparse
import os
import datetime
from run_base import *
import pyjson5 
import json

# argparse
parser = argparse.ArgumentParser()
parser.add_argument('benchmarks', choices=all_benchmarks.get_choices(), nargs='+', metavar='BENCHMARK',
                    help='Choices are ' + ', '.join(all_benchmarks.get_choices()))
parser.add_argument('-p', '--benchmark_path', default='../benchmarks/mlcad2023_v2/')
parser.add_argument('-l', '--log_dir')

args = parser.parse_args()

# seleted benchmarks
bms = all_benchmarks.get_selected(args.benchmarks)
bm_path = args.benchmark_path

# run
now = datetime.datetime.now()
log_dir = args.log_dir
if not log_dir:
    log_dir = 'run{:02d}{:02d}'.format(now.month, now.day)

run('mkdir -p {}'.format(log_dir))
print('The following benchmarks will be ran: ', bms)

# read basic configures
with open("../configures/mlcad.json", 'r') as f:
    conf = pyjson5.load(f)

def run_vivado(design_name, bm_log_dir):
    dcp_path = os.path.join(bm_path, design_name, "design.dcp")
    macro_pl_path = os.path.join(bm_log_dir, "place_macro.tcl")
    io_path = os.path.join(bm_log_dir, "place_io.tcl")
    tcl_our = '''open_checkpoint {}
set_param place.timingDriven false
source {} 
place_design -verbose'''.format(dcp_path, macro_pl_path)

    tcl_our += '''
get_cells -hierarchical -filter {STATUS == UNPLACED}
route_design -no_timing_driven -verbose
report_route_status
# write_checkpoint design.dcp
exit '''

    disable_checkpoint = "#"
    tcl_new = '''set_param -name place.hardVerbose -value 735361
set dcp_path {0}
set io_tcl_path {1}
{3}set placed_checkpoint_path "merge.placed.dcp"
{3}set routed_checkpoint_path "merge.routed.dcp"
'''.format(dcp_path, io_path, macro_pl_path, disable_checkpoint)

    tcl_new +='''puts "Opening Unrouted Checkpoint..."

open_checkpoint $dcp_path

set_param place.timingDriven false

puts "Placing IOs"
source $io_tcl_path

puts "Placing Design..."
place_design -verbose

{0}puts "Writing Placed Checkpoints..."
{0}write_checkpoint -force $placed_checkpoint_path

puts "Routing Design..."
route_design -no_timing_driven -verbose

{0}puts "Writing Routed Checkpoints..."
{0}write_checkpoint -force $routed_checkpoint_path

puts "Report Congestiong Level..."
report_design_analysis -congestion -min_congestion_level 3

puts "Reporting Routing Status..."
report_route_status
exit
'''.format(disable_checkpoint)

    flow_fn= os.path.join(bm_log_dir, "flow.tcl")
    final_tcl = tcl_new
    with open(flow_fn, 'w') as f:
        f.write(final_tcl)
    cmd = 'vivado -mode tcl -so {} -log {}/vivado.log -jou {}/vivado.jou'.format(flow_fn, bm_log_dir, bm_log_dir)
    run(cmd)

def run_convert(bm_dir, bm_log_dir):
    cmd = './io_map {}/design.pl {}/place_io.tcl '.format(bm_dir, bm_log_dir)
    run(cmd)
    #cmd = 'diff {0}/amd_io_site.log {0}/merge.vivado.tcl.debug | tee {0}/diff.log'.format(bm_log_dir)
    #run(cmd)
    #assert os.stat("{}/diff.log".format(bm_log_dir)).st_size == 0

for bm in bms:
    bm_log_dir = '{}/{}'.format(log_dir, bm.abbr_name)
    run('mkdir -p {}'.format(bm_log_dir))
    conf['designPath'] = bm_path + bm.full_name
    conf['logPath'] = bm_log_dir

    # store json
    # with open(os.path.join(bm_log_dir, "mlcad.json"), 'w') as f:
    #     obj = json.dumps(conf, indent=4)
    #     f.write(obj)
    run_convert(conf['designPath'], bm_log_dir)

    cmd = "rm {}/vivado*".format(bm_log_dir)
    run(cmd)
    run_vivado(bm.full_name, bm_log_dir)
