import os
import sys
import pandas as pd
import math

def get_cumple_results(fn):
    time = 0
    if not os.path.isfile(fn):
        return 0
    with open(fn) as f:
        lines = f.readlines()
        line = lines[-1].strip()
        if "Total HPWL" in line or "Total runtime" in line:
            time = float(line.split()[-2])
    return time

def get_vivado_PnR_time(fn):
    print("=================", fn, "=================")
    place_time = 0
    route_time = 0
    if not os.path.isfile(fn):
        return 0, 0
    with open(fn) as f:
        for line in f.readlines():
            if "place_design: Time (s): " in line:
                line_col = line.strip().split()
                time_str = line_col[9]
                time_col = time_str.split(":")
                hour = int(time_col[0])
                minute = int(time_col[1])
                second = int(time_col[2])
                place_time = hour * 3600 + minute * 60 + second
            elif "route_design: Time (s): " in line:
                line_col = line.strip().split()
                time_str = line_col[9]
                time_col = time_str.split(":")
                hour = int(time_col[0])
                minute = int(time_col[1])
                second = int(time_col[2])
                route_time = hour * 3600 + minute * 60 + second
    return place_time, route_time

def get_vivado_scores(fn):
    print("=================", fn, "=================")
    init_scores = []
    detailed_score = 0
    if not os.path.isfile(fn):
        return 0, 0
    with open(fn) as f:
        start = False
        start2 = False
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

    if len(init_scores) == 0:
        print("No congested area !!!")
        init_scores = [0 for i in range(8)]
    else:
        print("Init scores:", init_scores)

    init_score = 1
    for i in range(len(init_scores)):
        coef = 1.0
        init_score += coef * max(0, init_scores[i] - 3) ** 2
    print("Init_Score:", init_score, "Detailed_Score", detailed_score)
    return init_score, detailed_score


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
        macro_place_time = get_cumple_results(cumple_log_fn)

        vivado_log_fn = os.path.join(design_path, "vivado.log")
        LUTFF_place_time, route_time = get_vivado_PnR_time(vivado_log_fn)
        init_score, detailed_score = get_vivado_scores(vivado_log_fn)

        total_place_time = macro_place_time + LUTFF_place_time
        PnR_time = total_place_time + route_time
        
        congest_score = init_score + detailed_score
        res.append(["Design_{}".format(design[1:]), init_score, detailed_score, congest_score, total_place_time, route_time, PnR_time])
    
    df = pd.DataFrame(res, columns=["Design", "Init_Score", "Detailed_Score", "Congest_Score", "Place_time", "Route_time", "PnR_time"])
    
    print(df)
    df.to_csv(os.path.join(run_name, "stat.csv"), index=None)
