p: relaxation_technique.c
	gcc -o relaxation relaxation_technique.c -lm -lpthread

s: relaxation_technique_sequential.c
	gcc -o relaxation relaxation_technique_sequential.c -lm -lpthread