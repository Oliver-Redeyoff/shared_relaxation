make s

for i in {9..12}
do
   ./relaxation $((2**$i)) 3
done