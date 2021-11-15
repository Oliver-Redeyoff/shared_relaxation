make

for i in {1..20}
do
   ./relaxation $(($i*10)) 4 3
done