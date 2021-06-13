# Yoda MPI

X, Y, Z process types.

## Running:
`$ ./run.sh -c cx cy cz [-e energy] [-v verbosity level] [-s (use shared memory for debugging)] [-t time_mul (more = longer sleeps)]`

## Example:
`./run.sh -c 3 3 3 -v 30 -t 0.1 -s 2>&1 | tee log.txt`

### Inspect shared memory:
`watch -n0.1 xxd /dev/shm/yoda_shm`