path=../../benchmarks/mlcad2023_v2/
for folder in `ls $path`
do
echo "Replace file for $folder"
cp -r design.cascade_shape_instances $path$folder
cp -r design.lib $path$folder
cp -r design.scl $path$folder
done