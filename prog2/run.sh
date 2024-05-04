mpicc -Wall -O3 main.c bitonicSort.c -o a.o -lm
n=
while getopts "s:n:f:h" opt; do
    case $opt in
        n) n=$OPTARG;;
    esac
done
shift 2
mpiexec -n $n ./a.o $@