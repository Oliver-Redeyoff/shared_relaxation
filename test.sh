make

for i in {5..15}
do
   ./relaxation $(($i*10)) 2 3
done