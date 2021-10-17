## Fork POC

The purpose is to reach a load average as high as possible (visible with `uptime` command)

### Principle

Use fork(2) and RT possibilities

1st fork is the evil child that will keep the specified CPU busy and set with a RT priority high enough to prevent all following forks to have a chance to run on the same CPU

Next forks use the same CPU but have a RT priority smaller than the evil child's one

### Test

- compile with `make`

- run with sudo, specify the id of CPU to use, and the max number of forks to create:

`$ sudo ./a.out 2 5000`

- monitor with htop to get the values of current load_average, number of tasks/threads, and each CPU occupation

- compile with `$ make WITH_THREADS=1` to enable the creation of RT threads within each fork
