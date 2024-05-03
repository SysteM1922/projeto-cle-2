mpicc -Wall -O3 $1 -o a.o -lm
shift 1
n=
while getopts "n:f:h" opt; do
    case $opt in
        n) n=$OPTARG;;
    esac
done
shift 2
mpiexec -n $n ./a.o $@