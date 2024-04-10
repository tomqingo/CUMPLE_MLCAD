#!/bin/bash

designs=(2)
cnt=0
thread=$1
start_id=$2
end_id=$3
log_dir=$4

for design in ${designs[@]:$start_id:$end_id}
do
	cnt=$(( cnt + 1))
	echo $cnt/${#designs[@]} "Design_"$design
	
	python run_vivado.py d$design -l $log_dir &
	if [ $cnt == $thread ]
	then
		wait
		cnt=0
	fi
done
