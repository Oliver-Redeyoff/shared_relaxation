make s

for i in {1..10}
do
   ./relaxation $((100*$i)) 3
done