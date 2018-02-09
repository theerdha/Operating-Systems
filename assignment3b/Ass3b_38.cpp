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
vector<int> status;
int curr_thread;
int change = 1;

bool Allended(vector<int> v)
{
	for(int i = 0; i < v.size(); i++)
	{
		if(v[i] != 2)return false;
	}
	
	return true;
}

void * worker(void * index)
{
	cout << "in worker" << endl;
	//int *x = index;
	vector <int> RandNos(5);
	for(int i = 0 ; i < RandNos.size(); i++)
	{
		RandNos[i] = rand() % 10000;
	}
	sort(RandNos.begin(),RandNos.end());

	sleep(rand() % 10 + 1);
	status[*((int *)index)] = 2;
	cout << "Sorted 1000 numbers in thread : " <<endl;
	//<< *(int *)index << endl;
	pthread_exit(0);
}

void * reporter(void * param)
{
	/*while(!Allended(status))
	{
		for(int i = 0; i < status.size(); i++)
		{
			if(status[i] == 1) cout << "thread " << i <<" is currently running" << endl;
		}
	}*/
	while(!Allended(status) && change == 1)
	{
		cout << "thread " << curr_thread <<" is currently running" << endl;
		change = 0;
	} 
	if(Allended(status))cout << "All threads finished" << endl;
	pthread_exit(0);
}

void signal_handler(int sig_no)
{
	if(sig_no == SIGUSR1) 
	{
		pause(); 
		signal(SIGUSR1,signal_handler);
	}
	else
	{
		signal(SIGUSR2,signal_handler);
	}
}

void * scheduler(void * ids)
{
	curr_thread = 0;

	/* 
	schedules in a round robin fashion
	current thread is sent a signal to wake up and its status is put 1
	rest of them are put 0 if they are not finished
	*/
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

	pthread_exit(0);
}


int main()
{
	int N; cin >> N;
	srand(time(NULL));
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	for(int j = 0; j < N; j++) status.push_back(0);
	pthread_t rep;
	pthread_t sch;
	pthread_t mythreads[N];
	//cout << "here" << endl;
	signal(SIGUSR1,signal_handler);
	signal(SIGUSR2,signal_handler);

	for(int j = 0; j < N; j++)
	{
		void * pointer = &j;
		pthread_create(&mythreads[j],&attr,worker,pointer);
		//cout << "here1" << endl;
	}
	pthread_create(&rep,&attr,reporter,NULL);
	pthread_create(&sch,&attr,scheduler,(void *)&mythreads);

	for(int j = 0; j < N; j++)
	{
		pthread_join(mythreads[j],NULL);
	}
	pthread_join(rep,NULL);
	pthread_join(sch,NULL);

	return 0;
}
