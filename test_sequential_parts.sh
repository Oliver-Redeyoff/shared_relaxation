make s

for i in {1..12}
do
   ./relaxation $((1000*$i)) 3
done