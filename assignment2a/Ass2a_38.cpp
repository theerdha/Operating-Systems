#include <iostream>
#include <stdio.h>
#include <time.h>
#include <ctime>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
using namespace std;

void read_from_memory(int*,int*,int*,int,int);
void write_in_memory(int*,int*,int*,int,int);

int main(int argn,char** argc){
    int shmid[3];
    int np = atoi(argc[1]);
    int nc = atoi(argc[2]);
    int bufferSize = 5;
    int *base,*head,*shm;
    int x;
    key_t key[3];
    for(int i = 0; i < 3; i++){
        if(key[i] = ftok("./",i+1) < 0)
           printf("ERROR in generating key\n");
    }

    if(shmget(key[0],bufferSize*sizeof(int),IPC_CREAT|IPC_EXCL|0600) < 0)
        printf("ERROR occured in shared memory allocation.\n");
    
    if(shmget(key[1],sizeof(int),IPC_CREAT|IPC_EXCL|0600) < 0)
        printf("ERROR occured in shared memory allocation.\n");
    
    if(shmget(key[2],sizeof(int),IPC_CREAT|IPC_EXCL|0600) < 0)
        printf("ERROR occured in shared memory allocation.\n");
    
    base = (int*) shmat(shmid[1],NULL,0);
    head = (int*) shmat(shmid[2],NULL,0);
    shm = (int*) shmat(shmid[0],NULL,0);        
    
    *base = 0;
    *head = 0;
   
    if(shmdt(base) < 0 or shmdt(head) < 0 or shmdt(shm) < 0)
        printf("ERROR in deallocating address space to shared memory.\n");
    
    for(int i = 0; i < np+nc; i++){
        x = fork();
        if(x == 0){
            srand(time(NULL));
            if(shmid[0] = shmget(key[0],bufferSize*sizeof(int),0) < 0)
                printf("ERROR occured in shared memory allocation.\n");
            if(shmid[1] = shmget(key[1],sizeof(int),0) < 0)
                printf("ERROR occured in shared memory allocation.\n");
            if(shmid[2] = shmget(key[2],sizeof(int),0) < 0)
                printf("ERROR occured in shared memory allocation.\n");

            base = (int*) shmat(shmid[1],NULL,0);
            head = (int*) shmat(shmid[2],NULL,0);
            shm = (int*) shmat(shmid[0],NULL,0);        
            
            if(*base < 0 or *head < 0 or *shm < 0)
                printf("ERROR occured in assigning address space to shared memory.\n");

            if(i < np)
                write_in_memory(shm,base,head,i,bufferSize);   
            else 
                read_from_memory(shm,base,head,i,bufferSize);

            if(shmdt(base) < 0 or shmdt(head) < 0 or shmdt(shm) < 0)
                printf("ERROR in deallocating address space to shared memory.\n");
            
            _exit(0);
        }
    }

    for(int i = 0; i < 3; i++)
        shmctl(shmid[i],IPC_RMID,NULL);
    sleep(100);
    kill(0,SIGTERM);
    return 0;
}

void write_in_memory(int* shm,int* base,int* head,int index,int bufferSize){
    int value = rand()%1000;
    time_t result;
    sleep(rand()%6);
    while((*head+1)%bufferSize  == *base){}
    shm[*head] = value;
    *head = (*head + 1)%bufferSize;
    result = std::time(NULL);
    cout << "Producer " << index << ": " << value << " time : " <<  ctime(&result) << "\n";
    return;
}

void read_from_memory(int * shm, int* base, int* head,int index,int bufferSize){
    time_t result;
    sleep(rand()%6);
    while(*head == *base){} 
    result = std::time(NULL);
    cout << "Consumer " << index << ": " << shm[*base] << " time : " <<  ctime(&result) << "\n";
    *base = (*base + 1)%bufferSize;
    return;
}
