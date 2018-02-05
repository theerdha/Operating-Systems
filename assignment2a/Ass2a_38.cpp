
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h> 
#include <vector>
#include <ctime>

#define key 38

using namespace std;

int prime_numbers[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 
	73, 79, 83, 89, 97,101, 103, 107, 109, 113, 127, 131, 137, 139, 149,151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199};

void producers(int NP, vector<int> pids,int* parentseg)
{
	int X = fork();
	if(X == 0)
	{
		for(int i = 1; i <= NP; i++)
		{
			if(fork() == 0)
			{
				srand (time(NULL) + i);
				int w = rand() % 5;
				pids.push_back((int)getpid());
				int r = rand() % 45;
				r = prime_numbers[r];
				
				int success = 0;	
				sleep(w);
				while(!success){
					for(int j = 0; j < 5; j++){
						if(parentseg[j] == -1) {
							parentseg[j] = r;
							success = 1;
							break;
						}
					}
				}
				const time_t ctt = time(0);
				cout << "Producer" << i  << ": " << r << " ,time : " << asctime(localtime(&ctt)) << endl; 	
			}

			else
			{
				wait(NULL);
			}
		}
	}
	else return;
}


void consumers(int NC, vector<int> pids,int* parentseg)
{
	int X = fork();
	if(X == 0)
	{
		for(int i = 1; i <= NC; i++)
		{
			if(fork() == 0)
			{
				srand (time(NULL) + i + 10);
				int w = rand() % 5;
				pids.push_back((int)getpid());
				sleep(w);
				int success = 0;
				int value;
				while(!success){
					for(int j = 0; j < 5; j++){
						if(parentseg[j] != -1) {
							value = parentseg[j];
							parentseg[j] = -1;
							success = 1;
							break;
						}
						
					}
				}
				const time_t ctt = time(0);
				cout << "Consumer" << i << ": " << value << " ,time : " << asctime(localtime(&ctt)) << endl;
			}

			else
			{
				wait(NULL);
			}
		}
	}

	else return;
}

int main()
{	
	int NP,NC,status;
	cout << "enter number of producers : "; cin >> NP;
	cout << "enter number of customers : "; cin >> NC;
	int shmid = shmget(key, 5 * sizeof(int), IPC_CREAT|0700);
	int* parentseg = (int *) shmat(shmid,NULL,0);
	parentseg[0] = -1;parentseg[1] = -1;parentseg[2] = -1;parentseg[3] = -1;parentseg[4] = -1;
	vector<int> pids;

	producers(NP,pids,parentseg);
	consumers(NC,pids,parentseg);
	sleep(20);
	kill(0,SIGTERM);
	return 0;
}
	
