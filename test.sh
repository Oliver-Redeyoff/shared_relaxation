make

for i in {1..3}
do
   ./relaxation $((10*$i)) 1 3
done