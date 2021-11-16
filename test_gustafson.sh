make p

for i in {1..10}
do
   ./relaxation $(($i*1000)) 44 3
done