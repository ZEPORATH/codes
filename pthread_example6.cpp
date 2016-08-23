#include <bits/stdc++.h>
#include <pthread.h>

using namespace std;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void*
runner(void* args)
{
    pthread_mutex_lock(&mutex1);
    char* s;
    s = (char* )args;
    for(int i=0;i<10;i++)
    {
        cout<<"Thread: "<<s<<" i:"<<i<<endl;
    }
    pthread_mutex_unlock(&mutex1);
    pthread_exit(NULL);
}

int main()
{

    pthread_t tid1;
    pthread_t tid2;

    char msg1[]="tid1";
    char msg2[]="tid2";

    pthread_create(&tid1,NULL,runner,(void*)msg1);
    pthread_create(&tid2,NULL,runner,(void*)msg2);

    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);

    return 0;
}
