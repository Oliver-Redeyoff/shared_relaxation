make s

for i in {8..12}
do
   ./relaxation $((2**$i)) 3
done