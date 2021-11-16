make p

for i in {10..12}
do
   ./relaxation $(($i*1000)) 44 3
done