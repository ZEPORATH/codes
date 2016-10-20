#include <bits/stdc++.h>
#include <pthread.h>
using namespace std;
#define MAX 10000000
#define QUEUESIZE 10
#define LOOP 20

//declare Producer and Consumer

void* producer(void* args);
void* consumer(void* args);

//Take the mutexes

pthread_mutex_t the_mutex;
pthread_cond_t condc, condp;
int buffer = 0;




int main(){
    pthread_t prod, consm;

    //Initialize the pthread mutex and
    //condition variable


    pthread_mutex_init(&the_mutex,NULL);
    pthread_cond_init(&condc,NULL);
    pthread_cond_init(&condp,NULL);

    //create threads
    int rc1 = pthread_create(&prod,NULL,producer,NULL);
    if(rc1){
        cout<<"Error creating Producer thread"<<endl;
        exit(-1);

    }
    int rc2 = pthread_create(&consm, NULL,consumer,NULL);
    if(rc1){
        cout<<"Error creating the consumer thread"<<endl;
        exit(-1);
    }

    //Wait for threads to finish
    //Otherwise main might run to end
    //and kill entire process when it exist

    pthread_join(&consm,NULL);
    pthread_join(&prod, NULL);

    //Cleanup -- would happen automatically at end of program

    pthread_mutex_destroy(&the_mutex);
    pthread_cond_destroy(&condc);
    pthread_cond_destroy(&condp);
    return 0;
}

void* producer(void* ){
    int i =0;
    for (i =0; i<=MAX; i++){
        pthread_mutex_lock(&the_mutex);             //Protect the buffer
        while(buffer!=0)
            pthread_cond_wait(&condp,&the_mutex);   //If there is something in the buffer then wait
            buffer = i;
            pthread_cond_signal(&condc);            //Wake up the consumer
            pthread_mutex_unlock(&the_mutex);       //release the buffer
        }
        pthread_exit(0);
    }

void* consumer(void*){
    for(int i = 0; i<MAX; i++){
        pthread_mutex_lock(&the_mutex);
        while(buffer == 0)
            pthread_cond_wait(&condc, &the_mutex);
        buffer =0
        pthread_cond_signal(&condp);
        pthread_mutex_unlock(&the_mutex);

    }
    pthread_exit(0);
}

