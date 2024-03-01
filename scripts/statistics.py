import os
import sys
import pandas as pd
import math

def get_cumple_results(fn):
    time = 0
    wl = 0
    if not os.path.isfile(fn):
        return 0, 0
    with open(fn) as f:
        lines = f.readlines()
        line = lines[-1].strip()
        if "Total HPWL" in line:
            time = float(line.split()[-2])
            wl =  float(line.split()[3])
    return time, wl

def get_vivado_scores(fn):
    print("=================", fn, "=================")
    init_scores = []
    detailed_score = 0
    if not os.path.isfile(fn):
        return 0, 0
    with open(fn) as f:
        start = False
        start2 = False
        date_start = 0
        date_end = 0
        time_start = 0
        time_end = 0
        dirs = ["NORTH", "SOUTH", "EAST", "WEST"]
        max_level = 0
        for line in f.readlines():
            if "Initial Estimated Congestion" in line:
                start = True
            if "Congestion Report" in line:
                start = False
            if start:
                for dir in dirs:
                    if dir in line:
                        print(line, end="")
                        _, _, gc, _, lc, _, sc, _= line.split()
                        gc = int(gc[0:gc.find('x')])
                        lc = int(lc[0:lc.find('x')])
                        sc = int(sc[0:sc.find('x')])
                        gc_ = int(math.log2(gc))
                        lc_ = int(math.log2(lc))
                        sc_ = int(math.log2(sc))
                        assert gc == math.pow(2, gc_) and lc == math.pow(2, lc_) and sc == math.pow(2, sc_)
                        init_scores += [gc_, sc_]

            if start2 and "Rip-up And Reroute" in line:
                start2 = False
            elif "Rip-up And Reroute" in line:
                start2 = True
            if start2:
                if "Number of Nodes with overlaps" in line:
                    detailed_score += 1
            
            if "Start of session at" in line:
                time_col = line.split()
                date_start = int(time_col[7])
                hour_min_col = time_col[8].split(':')
                time_start = int(hour_min_col[0]) + int(hour_min_col[1])*1.0/60 + (int(hour_min_col[2])*1.0)/3600

            if "Exiting Vivado" in line:
                time_col = line.split()
                date_end = int(time_col[8])
                hour_min_col = time_col[9].split(':')
                time_end = int(hour_min_col[0]) + int(hour_min_col[1])*1.0/60 + (int(hour_min_col[2])*1.0)/3600
            
        vivado_time = (time_end - time_start) + (date_end - date_start)*24

            

    if len(init_scores) == 0:
        print("No congested area !!!")
        init_scores = [0 for i in range(8)]
    else:
        print("Init scores:", init_scores)

    # 1.2 x sumi(max(0, short_congestion_leveli-3)2) + sumi(max(0, global_congestion_leveli-3)2), where i=north, south, east, and south.
    init_score = 1
    for i in range(len(init_scores)):
        coef = 1.0
        # if i % 2 == 1:
        #     coef = 1.2
        init_score += coef * max(0, init_scores[i] - 3) ** 2
    print("Init_Score:", init_score, "Detailed_Score", detailed_score)
    return init_score, detailed_score, vivado_time


if __name__ == "__main__":
    run_name = sys.argv[1] # like: 0817_all_test
    res = []
    designs = []
    for design in os.listdir(run_name):
        design_path = os.path.join(run_name, design)
        if not os.path.isdir(design_path):
            continue
        designs.append(design)
    designs.sort(key=lambda d : int(d[1:]))

    for design in designs:
        design_path = os.path.join(run_name, design)
        cumple_log_fn = os.path.join(design_path, "run.log")
        time, wl = get_cumple_results(cumple_log_fn)

        vivado_log_fn = os.path.join(design_path, "vivado.log")
        init_score, detailed_score, vivado_time = get_vivado_scores(vivado_log_fn)
        final_score = init_score * detailed_score * (1 + max(1.22*time - 600, 0) / 60) * 1.22*vivado_time
        res.append(["Design_{}".format(design[1:]), wl, time, vivado_time, init_score, detailed_score, final_score])
    df = pd.DataFrame(res, columns=["Design", "HPWL", "Time", "Vivado_Time", "Init_Score", "Detailed_Score", "Final_Score"])
    print(df)
    df.to_csv(os.path.join(run_name, "stat.csv"), index=None)
