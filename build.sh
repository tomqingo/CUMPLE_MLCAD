mkdir -p build 
cd build
shopt -s extglob
# rm !(_deps) -rf
cmake ../src
make -j8
# load MKL for parallelism
# execute the line below before you run the placer in the terminal
# source /opt/intel/compilers_and_libraries_2020.4.304/linux/mkl/bin/mklvars.sh intel64
cd ..
mkdir -p run
mkdir -p run/placerDumpData
cp build/AMFPlacer run/
cp build/partitionHyperGraph run/