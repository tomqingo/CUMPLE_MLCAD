#!/bin/bash

designs=(2 5 7 10 12 15 17 20 22 25 27 30 32 35 37 40 42 45 47 50 52 55 57 60 62 65 67 70 72 75 77 80 82 85 87 90 92 95 97 100 102 105 107 110 112 115 117 120 122 125 127 130 132 135 137 140 142 145 147 150 152 155 160 162 165 167 170 172 175 180)
# designs=(2 181)
DREAMPlaceFPGA_dir=$1
DREAMPlaceFPGA_benchmark=$2
DREAMPlaceFPGA_rundir=$3
start_id=$4
end_id=$5
gpu_flag=$6


if [ -f ${DREAMPlaceFPGA_dir}/dreamplacefpga/Placer.py ]; then
    echo "DREAMPlaceFPGA_dir = ${DREAMPlaceFPGA_dir}"
else
    echo "DREAMPlaceFPGA_dir = ${DREAMPlaceFPGA_dir} is not a valid directory"
    exit 0
fi

if [[ -z $6 ]] ; then
    echo "option not given"
    echo "CPU mode: run_mlcad_design.sh ${DREAMPlaceFPGA_dir} 0"
    gpu_flag=0
else
    if [[ $6 -eq 1 ]] ; then
        echo "GPU mode: run_mlcad_design.sh ${DREAMPlaceFPGA_dir} ${gpu_flag}"
    else
        echo "CPU mode: run_mlcad_design.sh ${DREAMPlaceFPGA_dir} ${gpu_flag}"
    fi
fi

#fixing bookshelf files typos
for design in ${designs[@]:$start_id:$end_id}
do

echo Design_$design

lib_file=${DREAMPlaceFPGA_benchmark}/Design_$design/design.lib
if [ -f $lib_file ]; then
    sed -i 's/CELL END/END CELL/' $lib_file
fi


instance_file=${DREAMPlaceFPGA_benchmark}/Design_$design/design.cascade_shape_instances
if [ -f $instance_file ]; then
    sed -i 's/cascade/CASCADE/' $instance_file
    sed -i 's/BRAM_CASCADE /BRAM_CASCADE_2 /' $instance_file
fi


#'DSP_ALU_INST' not present in design.nodes
region_file=${DREAMPlaceFPGA_benchmark}/Design_$design/design.regions
if [ -f $region_file ]; then
    sed -i 's+/DSP_ALU_INST++' $region_file
fi


#create design.aux file
if [ -e ${DREAMPlaceFPGA_benchmark}/Design_$design/design.aux ]; then
    rm ${DREAMPlaceFPGA_benchmark}/Design_$design/design.aux
fi

touch ${DREAMPlaceFPGA_benchmark}/Design_$design/design.aux
echo -n "design : design.nodes design.nets design.pl design.scl design.lib design.regions design.cascade_shape" >> ${DREAMPlaceFPGA_benchmark}/Design_$design/design.aux

if [ -e ${DREAMPlaceFPGA_benchmark}/Design_$design/design.cascade_shape_instances ]; then
    echo -n " design.cascade_shape_instances" >> ${DREAMPlaceFPGA_benchmark}/Design_$design/design.aux
fi


if [ -e ${DREAMPlaceFPGA_benchmark}/Design_$design/design.macros ]; then
    echo -n " design.macros" >> ${DREAMPlaceFPGA_benchmark}/Design_$design/design.aux
fi

if [ -e ${DREAMPlaceFPGA_benchmark}/Design_$design/design.wts ]; then
    echo -n " design.wts" >> ${DREAMPlaceFPGA_benchmark}/Design_$design/design.aux
fi

echo "" >> ${DREAMPlaceFPGA_benchmark}/Design_$design/design.aux


#create design.json file

mkdir -p ${DREAMPlaceFPGA_rundir}/Design_$design
if [ -f "${DREAMPlaceFPGA_rundir}/Design_$design/design.json" ]; then
    rm ${DREAMPlaceFPGA_rundir}/Design_$design/design.json
fi

cat > ${DREAMPlaceFPGA_benchmark}/Design_$design/design.json <<EOF
{
    "aux_input" : "${DREAMPlaceFPGA_benchmark}/Design_$design/design.aux",
    "gpu" : 0,
    "num_bins_x" : 512,
    "num_bins_y" : 512,
    "global_place_stages" : [
    {"num_bins_x" : 512, "num_bins_y" : 512, "iteration" : 1800, "learning_rate" : 0.01, "wirelength" : "weighted_average", "optimizer" : "nesterov"}
    ],
    "target_density" : 1.0,
    "density_weight" : 8e-5,
    "random_seed" : 1000,
    "scale_factor" : 1.0,
    "global_place_flag" : 1,
    "dtype" : "float32",
    "num_threads" : 40,
    "deterministic_flag" : 1,
    "run_dir" : "${DREAMPlaceFPGA_rundir}/Design_$design"
}
EOF

#run the design
python3 ${DREAMPlaceFPGA_dir}/dreamplacefpga/Placer.py ${DREAMPlaceFPGA_benchmark}/Design_$design/design.json > ${DREAMPlaceFPGA_rundir}/Design_$design/log.txt

done

