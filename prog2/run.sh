mpicc -Wall -O3 $1 -o a.o -lm
shift 1
mpiexec $@ ./a.o