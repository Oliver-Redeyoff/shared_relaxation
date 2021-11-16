make

for i in {1..11}
do
   ./relaxation 150 $((4*$i)) 3
done