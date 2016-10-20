#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <unistd.h>
#include <bits/stdc++.h>
using namespace std;

#define NUM_THREADS 32

struct param{
    int thread_id;
    char* msg;
    bool inCriticalState;
};
//int[] A;
//int* p= A;
struct param thread_data_array[NUM_THREADS];


void* PrintHello(void* thread_arg){
    struct param *mydata ;

    mydata = (struct param*)thread_arg;
    int tid = (long)mydata->thread_id;
    char* msg = mydata->msg;
    std::string str(msg);
    bool crtcl_flag = mydata->inCriticalState;
    cout<<" Thread"<<tid<<" sent ->> "<<str<<" has flag: "<<(bool)crtcl_flag<<endl;
    sleep(2);
    pthread_exit(NULL);


}
int main() {
    cout<<"Hello";
    int task_id[NUM_THREADS];
    //Initialize threads
    pthread_t threads[NUM_THREADS];
    int rc;

    for (int t=0;t<NUM_THREADS;t++){
        task_id[t] = t;
        //cout<<"Creating thread: "<<t<<endl;
        thread_data_array[t].thread_id = t;
        thread_data_array[t].msg = "Hello ";
        thread_data_array[t].inCriticalState = false;
        //rc = 1|0
        rc = pthread_create(&threads[t], NULL, PrintHello,
        (void *) &thread_data_array[t]);

        if (rc) {
          printf("ERROR; return code from pthread_create() is %d\n", rc);
          exit(-1);
          }
    }
    cout<<"main(): created "<<NUM_THREADS<< "threads. \n ";
    pthread_exit(NULL);

    return 0;
}
