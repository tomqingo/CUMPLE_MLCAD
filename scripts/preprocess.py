import re
import argparse
import os
import math

def get_option():
    parser = argparse.ArgumentParser("Extract Macro Location")
    parser.add_argument("--input_dir", type=str, default="/data/ssd/qluo/benchmark/mlcad2023_v2/Design_2/place_results/", help="the folder for the initial placement results")
    parser.add_argument("--output_dir", type=str, default="/data/ssd/qluo/benchmark/mlcad2023_v2/Design_2/solution/", help="the folder for the output placement results")   
    args = parser.parse_args()
    return args

def output_tcl(sol, solution_pl_path):
    out_str = ""
    out_str += "set fo [open \"./placementError\" \"w\"]\n"
    out_str += "place_design -unplace\n"
    out_str += "set errorNum 0\n"
    if True:
        for id in range(len(sol)):
            macro_name = sol[id][0]
            macro_site = sol[id][1]

            if "DSP" in macro_name:
                macro_site += "/DSP_ALU"
            
            out_str += "if { [catch {place_cell { "
            out_str += (macro_name + " ")
            out_str += (macro_site + "\n")
            out_str += "} }]} {\n"
            out_str += "incr errorNum\n"
            out_str += "puts $fo \" "
            out_str += (macro_name + " ")
            out_str += (macro_site + "\n")
            out_str += "\"\n"
            out_str += "}\n"

        out_str += "$errorNum"

    f_out = open(solution_pl_path, "w")
    f_out.write(out_str)
    f_out.close()

# get macro solution
def get_solution_from_pl(solution_pl_path):
    bram_list = [11, 17, 35, 60, 72, 101, 105, 130, 142, 171, 189, 195]
    dsp_list = [2, 20, 26, 30, 38, 44, 53, 63, 75, 98, 108, 114, 123, 133, 145, 168, 174, 186, 202]

    bram_dict = {}
    dsp_dict = {}
    for bram_X in range(len(bram_list)):
        col_id = bram_list[bram_X]
        bram_dict[col_id] = bram_X
    
    for dsp_X in range(len(dsp_list)):
        col_id = dsp_list[dsp_X]
        dsp_dict[col_id] = dsp_X
    
    sols = []
    with open(solution_pl_path) as f_org:
        all_lines = f_org.read().splitlines()
        for id in range(len(all_lines)):
            cur_line = all_lines[id]
            cur_line_col = cur_line.strip().split()
            macro_name = cur_line_col[0]
            macro_X = cur_line_col[1]
            macro_Y = cur_line_col[2]
            macro_bel = cur_line_col[3]
            if "DSP" in macro_name:
                col_id = dsp_dict[int(macro_X)]
                suffix = "DSP48E2_"
                row_id = math.ceil(int(macro_Y)*1.0/2.5)
            else:
                col_id = bram_dict[int(macro_X)]
                suffix = "RAMB36_"
                row_id = int(int(macro_Y)/5)
            
            macro_site = suffix + "X" + str(col_id) + "Y" + str(row_id)
            sols.append([macro_name, macro_site])
    
    return sols

def get_solution_from_tcl(input_tcl_path):
	pass

if __name__=="__main__":
    args = get_option()
    if not os.path.exists(args.output_dir):
        os.mkdirs(args.output_dir)
    
    solution_pl_path = os.path.join(args.input_dir, "solution_final.pl") # input
    # input_tcl_path = os.path.join(args.output_dir, "merge.vivado.tcl") # input
    output_tcl_path = os.path.join(args.output_dir, "dumpmacrosite.tcl") # output

    sol = get_solution_from_pl(solution_pl_path)
    output_tcl(sol, output_tcl_path)