#!/bin/bash

<<<<<<< HEAD
designs=designs=(2 5 7 10 12 15 17 20 22 25 27 30 32 35 37 40 42 45 47 50 52 55 57 60 62 65 67 70 72 75 77 80 82 85 87 90 92 95 97 100 102 105 107 110 112 115 117 120 122 125 127 130 132 135 137 140 142 145 147 150 152 155 160 162 165 167 170 172 175 180)
=======
designs=(2 5 7 10 12 15 17 20 22 25 27 30 32 35 37 40 42 45 47 50 52 55 57 60 62 65 67 70 72 75 77 80 82 85 87 90 92 95 97 100 102 105 107 110 112 115 117 120 122 125 127 130 132 135 137 140 142 145 147 150 152 155 160 162 165 167 170 172 175 180)
>>>>>>> 2dccbc227c8885d909952c0bfa468a0bedfa0b7f
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
