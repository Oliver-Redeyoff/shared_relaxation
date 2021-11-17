make s

for i in {8..10}
do
   ./relaxation $((1000*$i)) 3
done