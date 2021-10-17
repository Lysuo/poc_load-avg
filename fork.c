#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef WITH_INNER_THREADS
#include <pthread.h>
#define PRIO_THREAD 30
#define NB_THREADS 3 /* per fork */
#endif

#define PRIO_EVIL_CHILD 80
#define PRIO_SHEEP_CHILD 40

#ifdef WITH_INNER_THREADS
void * thread_function(void * arg)
{
   (void) arg;
   __asm__("nop");
   return NULL;
}

/*
 * create NB_THREADS
 *    with FIFO RT sched policy
 *    with priority PRIO_THREAD
 *    set to run on same cpu as evil child
 */
void create_rt_thread(int cpu, int prio)
{
   pthread_attr_t attr;
   pthread_t thr[NB_THREADS];
   pthread_attr_init (& attr);

   /* use same cpu as tasks for the threads */
   cpu_set_t cpuset;
   CPU_ZERO(&(cpuset));
   CPU_SET(cpu, &(cpuset));
   pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);

   /* set RT sched policy to FIFO */
   if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
      fprintf(stderr, "setschedpolicy: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }
   if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0) {
      fprintf(stderr, "setinheritsched: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   /* set RT prio for threads */
   struct sched_param param;
   param.sched_priority = prio;
   if (pthread_attr_setschedparam(&attr, &param) != 0) {
      fprintf(stderr, "setschedparam: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   /* create NB_THREADS threads */
   int i;
   for (i = 0; i < NB_THREADS; i++) {
      if (pthread_create(& (thr[i]), &attr,
                  thread_function, NULL) != 0) {
         fprintf(stderr, "pthread_create: %s\n", strerror(errno));
         exit(EXIT_FAILURE);
      }
   }

   return;
}
#endif

void set_cpu_rt (int cpu, int prio)
{
   struct sched_param param;
   cpu_set_t cpuset;

   CPU_ZERO(& cpuset);
   CPU_SET(cpu, &cpuset);
   if (sched_setaffinity(0, sizeof(cpuset), & cpuset) !=0) {
      perror("sched_setaffinity");
      exit(EXIT_FAILURE);
   }

   param.sched_priority = prio;
   if (sched_setscheduler(0, SCHED_FIFO, & param) != 0) {
      perror("sched_setscheduler");
      exit(EXIT_FAILURE);
   }
}

int main (int argc, char *argv[])
{
   int nforks, nproc, cpu;
   pid_t pid;

   if (argc < 3) {
      fprintf(stderr, "usage: %s cpu_id n_forks\n", argv[0]);
      exit(EXIT_FAILURE);
   }

   nproc = sysconf(_SC_NPROCESSORS_ONLN);
   printf("%d CPU available \n", nproc);
   if (atoi(argv[1]) < 0 || atoi(argv[1]) >= nproc) {
      fprintf(stderr, "cpu_id must be in range [%d;%d[ \n", 0, nproc);
      exit(EXIT_FAILURE);
   }

   cpu = atoi(argv[1]);
   printf("using CPU #%d\n", cpu);

   nforks = atoi(argv[2]);
   printf("will fork %d times\n", nforks);

   /*
    * trying to set max user processes param with setrlimit: no impact
    *
   const struct rlimit rl = {
         .rlim_cur = RLIM_INFINITY,
         .rlim_max = RLIM_INFINITY
      };
   setrlimit(RLIMIT_NPROC, &rl);
    */

   /*
    * 1st evil child with high RT priority
    * that will keep the chosen cpu busy and prevent
    * next forks to run on the same cpu
    */
   if (0 == fork()) {
      pid = getpid();
      printf("evil child pid= %d\n", (int)pid);
      set_cpu_rt(cpu, PRIO_EVIL_CHILD);
      for (;;) { __asm__("nop"); }
   }
   else {
      pid = getpid();
      printf("father pid= %d\n", (int)pid);
   }

   /*
    * create N forks that will never have a
    * chance to run because of the EVIL child
    */
   int i;
   for (i=0;i<nforks;i++)
   {
      if (0 == fork()) {
         pid = getpid();
#ifdef WITH_INNER_THREADS
         /*
          * creating threads into each fork
          * doesn't have any impact on the limit reached
          */
         create_rt_thread(cpu, PRIO_THREAD);
#endif
         set_cpu_rt(cpu, PRIO_SHEEP_CHILD);
         __asm__("nop");
      }

      if (errno != 0) {
         sleep(1);
         fprintf(stderr, "error %d: %s\n", errno, strerror(errno));
      }
      usleep(250);
   }

   wait(NULL);
   return 0;
}
