#include <math.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NTHREADS        32

void *Hello(void *threadid)
{
   int i;
   double result=0.0;
   long my_id = *(long*)threadid;
   sleep(3);

   for (i=0; i<10000; i++) {
     result = result + sin(i) * tan(i);
   }

   printf("%ld: Hello World!\n", my_id);
   pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
   pthread_t threads[NTHREADS];
   int rc;
   long t;
   long a[NTHREADS];

   for(t=0;t<NTHREADS;t++){
      a[t] = t;
      rc = pthread_create(&threads[t], NULL, Hello, &a[t]);
      if (rc){
         printf("ERROR: return code from pthread_create() is %d\n", rc);
         printf("Code %d= %s\n",rc,strerror(rc));
         exit(-1);
      }
   }

   printf("main(): Created %ld threads.\n", t);
   pthread_exit(NULL);
}
