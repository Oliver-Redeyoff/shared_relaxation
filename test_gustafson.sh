make p

for i in {1..10}
do
   ./relaxation $(($i*100)) 44 3
done