make s

for i in {1..10}
do
   ./relaxation $(($i*500)) 3
done