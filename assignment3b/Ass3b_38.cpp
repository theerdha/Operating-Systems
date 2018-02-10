#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <signal.h>
#include <algorithm>
#include <unistd.h>

using namespace std;

// waiting 0
//currently running 1
//finished 2
//
vector<int> status;
int curr_thread = -1;
int thread_count = 0;
pthread_mutex_t lock;

int Allended()
{
    bool waiting = false;
    for(int i = 0; i < thread_count; i++)
    {
        if(status[i] == 1)
            return i;
        if(status[i] == 0)
            waiting = true;
    }

    if(waiting == true)
        return -1;

    return -2;
}

void * worker(void * index)
{
    int index_int = *((int*) index);
    cout << "Worker " << index_int << " active\n";
    vector <int> RandNos(5);
    for(int i = 0 ; i < RandNos.size(); i++)
    {
        RandNos[i] = rand() % 10000;
    }
    sort(RandNos.begin(),RandNos.end());

    sleep(rand() % 10 + 1);
    pthread_mutex_lock(&lock);
    status[index_int] = 2;
    pthread_mutex_unlock(&lock);
    cout << "Sorted 1000 numbers in thread : " << index_int<< endl;
    //<< *(int *)index << endl;
    pthread_exit(0);
}

void * reporter(void * param)
{
    printf("Reporter Active\n");
    /*while(!Allended(status))
      {
      for(int i = 0; i < status.size(); i++)
      {
      if(status[i] == 1) cout << "thread " << i <<" is currently running" << endl;
      }
      }
      while(!Allended(status))
      {
      if(change){
      cout << "thread " << curr_thread <<" is currently running" << endl;
      change = 0;
      }
      } */
    int i;
    while((i = Allended()) != -2){
        pthread_mutex_lock(&lock);
        if(curr_thread != i){
            if(curr_thread != -1 && i != -1)
                cout << "Context switched from thread " << curr_thread << " to thread " << i << endl; 
            else if(i != -1)
                cout << "Running thread " << i << endl;
            curr_thread = i;
        }
        pthread_mutex_unlock(&lock);
    }
    cout << "All threads finished" << endl;
    pthread_exit(0);

}

void signal_handler(int sig_no)
{
    if(sig_no == SIGUSR1) 
    {
        pause(); 
        //signal(SIGUSR1,signal_handler);
    }
    else
    {
        //signal(SIGUSR2,signal_handler);
    }
}

void * scheduler(void * ids)
{

    /* 
       schedules in a round robin fashion
       current thread is sent a signal to wake up and its status is put 1
       rest of them are put 0 if they are not finished
     *
     while(!Allended(status))
     {
     status[curr_thread] = 1;
     pthread_kill( *( (pthread_t *)(ids+(curr_thread * sizeof(pthread_t))) ) , SIGUSR2);
     for(int i = 0; i < status.size();i++)
     {
     if(i != curr_thread && status[i] != 2) 
     {
     status[i] = 0;
     pthread_kill( *( (pthread_t *)(ids+(i * sizeof(pthread_t))) ) , SIGUSR1);
     }
     }

     for(int i = 0; i < status.size();i++)
     {
     curr_thread = (curr_thread + 1) % status.size();
     change = 1;
     if(status[curr_thread] != 2) break;
     }
     sleep(1);
     }
     */
    int i;
    pthread_t * threads = (pthread_t*)ids;
    printf("Scheduler Active\n");
    while((i = Allended()) != -2){
        pthread_mutex_lock(&lock);
        if(curr_thread >= 0 && status[curr_thread] != 2){ 
            status[curr_thread] = 0;
            pthread_kill(threads[curr_thread],SIGUSR1);
        }
        int j = (curr_thread+1)%thread_count;
        while(j != curr_thread){
            if(status[j]==0)
                break;
            j = (j+1)%thread_count;
        }
        status[j] = 1;
        pthread_kill(threads[j],SIGUSR2);
        pthread_mutex_unlock(&lock);
        sleep(1);
    }

    pthread_exit(0);
}


int main()
{
    int N,n; cin >> N;
    thread_count = N;
    srand(time(NULL));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (pthread_mutex_init(&lock, NULL) != 0){
        printf("\n mutex init has failed\n");
    }
    for(int j = 0; j < N; j++) status.push_back(0);
    pthread_t rep;
    pthread_t sch;
    pthread_t mythreads[N];
    //cout << "here" << endl;
    signal(SIGUSR1,signal_handler);
    signal(SIGUSR2,signal_handler);

    for(int j = 0; j < N; j++)
    {
        int* index = new int;
        *index = j;
        n = pthread_create(&mythreads[j],&attr,worker,(void *)index);
        if(n<0)
            cerr << "Error creating thread\n";
        //cout << "here1" << endl;
        pthread_kill(mythreads[j],SIGUSR1);
    }
    if(pthread_create(&rep,&attr,reporter,NULL) !=  0)
        cerr << "Error creating thread\n";
    if(pthread_create(&sch,&attr,scheduler,(void *)&mythreads) !=  0)
        cerr << "Error creating thread\n";

    for(int j = 0; j < N; j++)
    {
        pthread_join(mythreads[j],NULL);
    }
    pthread_join(rep,NULL);
    pthread_join(sch,NULL);
    
    pthread_mutex_destroy(&lock);;
    return 0;
}
