make p

for i in {8..10}
do
   ./relaxation $(($i*1000)) 44 3
done