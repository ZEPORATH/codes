//Pthread example 2
// author  = S.Shekhar
//Date : 08/08/16

#include <pthread.h>
#include <bits/stdc++.h>


//int pthread_create(pthread_t *thread, const pthread_attr_t *aattr, void *(*start_routine)(void*),void *arg);
//int pthread_join(pthread_t thread, void** value_ptr);
//void pthread_exit(void *value_ptr);


using namespace std;

void *worker_thread(void *arg)
{
    cout<<"this is worker thread() \n";
    pthread_exit(NULL);

}

int main()
{

    pthread_t myThread;
    int ret;

    cout<<"In main: creating thread \n";
    ret = pthread_create(&myThread,NULL, &worker_thread,NULL);
    if (ret !=0){
            cout<<"Error creating threads\n";
            exit(EXIT_FAILURE);
    }
    pthread_exit(NULL);
}
