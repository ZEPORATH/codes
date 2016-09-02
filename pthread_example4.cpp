
//Pthread example 2
// author  = S.Shekhar
//Date : 08/08/16

#include <pthread.h>
#include <bits/stdc++.h>
#include <stdlib.h>


//int pthread_create(pthread_t *thread, const pthread_attr_t *aattr, void *(*start_routine)(void*),void *arg);
//int pthread_join(pthread_t thread, void** value_ptr);
//void pthread_exit(void *value_ptr);


using namespace std;

void*
do_loop(void* data)
{

    int me = *((int*)data);

    for(int i=0;i<10;i++){
        for(int j=0;j<5000000;j++);
        cout<<me<<" =GOT "<<i<<endl;
    }
    pthread_exit(NULL);
}

int main()
{

    int thr_id;
    pthread_t p_thread;
    int a = 1;
    int b = 2;
    thr_id = pthread_create(&p_thread,NULL,do_loop,(void*)&a);
    do_loop((void*)&b);
    /*NOT REACHED*/
    return 0;
}
