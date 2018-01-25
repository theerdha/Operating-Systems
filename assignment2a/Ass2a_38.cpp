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

int prime_numbers[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97,101, 103, 107, 109, 113, 127, 131, 137, 139, 149,151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199};



int main()
{
	clock_t begin = clock();	
	int NP,NC,status;
	cout << "enter number of producers : "; cin >> NP;
	cout << "enter number of customers : "; cin >> NC;

	int r,w,success,element,value;

	int shmid = shmget(key, 5 * sizeof(int), IPC_CREAT|0700);
	int* parentseg = (int *) shmat(shmid,NULL,0);
	parentseg[0] = -1;parentseg[1] = -1;parentseg[2] = -1;parentseg[3] = -1;parentseg[4] = -1;

	vector<int> pids;

	for(int i = 1; i <= NP + NC; i++){
		if(fork() == 0){
			srand (time(NULL) + i);
			//int* myseg = (int *)shmat(shmid,NULL,0);
			w = rand() % 5;

			if(i <= NP){
				pids.push_back((int)getpid());
				r = rand() % 45;
				r = prime_numbers[r];
				
				 success = 0; element = 0;	
				 usleep(w * 1000);
				 while(!success){
					for(int j = 0; j < 5; j++){
						if(parentseg[j] == -1) {
							parentseg[j] = r;
							success = 1;
							break;
						}
					}
				}
				cout << "Producer" << i  << ": " << r << " ,time : " << endl; 			
				
			}

			else{
				pids.push_back((int)getpid());
				usleep(w * 1000);
				success = 0;
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
				cout << "Consumer" << i - NP << ": " << value << " ,time : " << endl;
			}
			

		}

		else{
			clock_t end = clock();
			//parent(pids,begin);
			double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

			if(elapsed_secs >= 30){
				for(int k = 0; k < pids.size(); k++){
					kill(pids[k],SIGQUIT);
				}
				shmdt(parentseg);
				shmctl(shmid, IPC_RMID, 0);
				_exit(0);
			}
			//wait(&status);
			break;
			
		}

	}	

	 for(int i = 0; i < NP + NC; i++) // loop will run n times (n=5)
     wait(NULL);

}
