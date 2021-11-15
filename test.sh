make

for i in {1..20}
do
   ./relaxation $(($i*20)) 4 3
done