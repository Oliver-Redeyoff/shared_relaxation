make p

for i in {1..11}
do
   ./relaxation 1024 $(($i*4)) 3
done