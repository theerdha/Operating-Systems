#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <time.h>
#include <fstream>
#include <time.h>

using namespace std;

#define L 0.5

//////// Utitlity FUnctions ////////

// Generates Random Numbers from Exponential Distribution
double expo_dist(double lambda){
    double u,v = 20;
	while(!(v >= 0 && v <= 10))
   	{
		 u = rand() / (RAND_MAX + 1.0);
		 v = -log(1- u) / lambda;
	}
    return v;
}

//Compare
int max(int a, int b)
{
    if(a > b) return a;
	else return b;
}

//Checks if All the entries of a vector are zero or not
bool Allzeros(vector<int> v)
{
	for(int i = 0; i < v.size(); i++)
	{
		if(v[i] != 0)return false;
	}
	
	return true;
}

//Checks if it is the case that no process has currently arrived.
bool allNotArrive(vector<int> v,vector<int> r,int time)
{
	for(int i = 0; i < v.size(); i++)
	{
		if(v[i] <= time && r[i] != 0)return false;
	}
	return true;
}

// checks for the presence of an element in vector
int isIn( vector<int> v,int index)
{
	for(int i = 0; i < v.size(); i++)
	{
		if(v[i] == index)return i;
	}
	return -1;
}

// Returns the process with minimum remaining burst time
int minRem(vector<int> v,int index,vector<int> a)
{
	int minVal = 1000000;
	int minIndex = -1;
	for(int i = 0; i < v.size(); i++)
	{
		if(v[i] < minVal && v[i] != 0 && a[i] <= index)
		{
			minVal = v[i];
			minIndex = i ;
		}
	}
	return minIndex;
}

////////////// Simulates First Come First Serve Scheduling Algorithm ////////////////////
///// Inputs : Arrival Time, CPU Burst Time
///// Ouput : Average ATN

double FCFS( vector<int> arrT, vector<int> BT)
{
	cout << endl << "FCFS......" << endl << endl;
	vector<int> finishT;
	finishT.push_back(BT[0]);
	int FT;
	for(int i = 1; i < arrT.size(); i++)
	{
		FT = BT[i] + max(finishT[i-1],arrT[i]);
		finishT.push_back(FT);
	}
	
	double ATN = 0;
	for(int i = 0 ; i < arrT.size(); i++)
	{
		ATN += (finishT[i] - arrT[i]);
	}
	
	for(int i = 0 ; i < arrT.size(); i++)
	{
		cout << "finsh : " << i + 1 << " : " << finishT[i] << endl;
	}
	
	ATN = ATN/arrT.size();
	return ATN;
}

////////////// Simulates Preemtive version of Shortest Job First Scheduling Algorithm ////////////////////
///// Inputs : Arrival Time, CPU Burst Time
///// Ouput : Average ATN

double PremtiveSJF( vector<int> arrT, vector<int> BT)
{
	cout << endl << "PremtiveSJF......" << endl << endl;
	vector<int> remT;
	vector<int> finishT(arrT.size());
	vector<bool> finishMarker(arrT.size());
	int currProcess = 0;
	
	for(int i = 0; i < arrT.size(); i++)
	{
		remT.push_back(BT[i]);
		finishMarker[i] = false;
	}
	
	int index = 0;
	
	while(!Allzeros(remT))
	{

		index ++;
		if(currProcess != -1)remT[currProcess] --;

		// Checks when a new process arrives or current process is over
		if(isIn(arrT,index) >= 0 || (currProcess != -1 && remT[currProcess] == 0))
		{
			currProcess = minRem(remT,index, arrT);
			if(currProcess == -1) continue;
		}
		
		for(int i = 0; i < arrT.size(); i++)
		{
			/// To mark completed processes
			if(remT[i] == 0 && !finishMarker[i])
			{
				finishT[i] = index;
				finishMarker[i] = true;
			}
		}
		
	}
	
	// To calculate average ATN
	double ATN = 0;
	for(int i = 0 ; i < arrT.size(); i++)
	{
		ATN += (finishT[i] - arrT[i]);
	}
	
	for(int i = 0 ; i < arrT.size(); i++)
	{
		cout << "finsh : " << i + 1 << " : " << finishT[i] << endl;
	}
	
	ATN = ATN/arrT.size();
	return ATN;
}

////////////// Simulates Round Robin Scheduling Algorithm ////////////////////
///// Inputs : Time Quantum, Arrival Time, CPU Burst Time
///// Ouput : Average ATN

double RoundRobin(int TimeQuantum, vector<int> arrT, vector<int> BT)
{
	cout << endl << "RoundRobin with Quantum : "  << TimeQuantum <<" ....." << endl << endl;
	int time = 0;
	vector<int> remT;
	vector<int> finishT(arrT.size());
	int currProcess = 0;
	
	for(int i = 0; i < arrT.size(); i++)
	{
		remT.push_back(BT[i]);
	}
	
	while(!Allzeros(remT))
	{
		//Simulates time		
		time ++;

		if(currProcess != -1)
		{
			remT[currProcess] --;
			if(remT[currProcess] == 0)
			{
				finishT[currProcess] = time;
			}
		}
		
		// On a time quatum interrupt or completion of a process

		if(time % TimeQuantum == 0 || currProcess != -1 && remT[currProcess] == 0)
		{
			for(int i = 0; i < arrT.size(); i++)
			{
				// Chooses the next process
				currProcess = (currProcess + 1) % arrT.size();
				if(remT[currProcess] != 0 && arrT[currProcess] <= time) break;
			}
			
			if(allNotArrive(arrT,remT,time))currProcess = -1;			
		
		}		

	}
	double ATN = 0;

	// Computes average ATN

	for(int i = 0 ; i < arrT.size(); i++)
	{
		ATN += (finishT[i] - arrT[i]);
	}
	
	for(int i = 0 ; i < arrT.size(); i++)
	{
		cout << "finsh : " << i + 1 << " : " << finishT[i] << endl;
	}
	
	ATN = ATN/arrT.size();
	return ATN;
}

int main()
{
    int N; 
    cout << "Enter number of process to run : ";
    cin >> N;

	//FIle handling
	ofstream myfile;
    myfile.open ("Processtables.txt",std::ios_base::app);
	myfile << "Processes for N = " << N << endl << endl;
    double avgfcfs = 0,avgsjf = 0,avgrr1 = 0,avgrr2 = 0,avgrr5 = 0;

    // Number of test cases
	int r = 10;
	srand (time(NULL));
    while(r--){

		
        vector <int> arrivalTime;
        vector <int> burstTime;
        arrivalTime.push_back(0);

        for(int i = 0; i < N; i++)
        {
            burstTime.push_back(rand()%20 + 1);
        }

        int inter_arrival_time;

        for(int i = 1; i < N; i++)
        {
            inter_arrival_time = ((int)expo_dist(L)) % 11;
            arrivalTime.push_back(arrivalTime[i - 1] + inter_arrival_time);
        }

		myfile << "Case "<< 10 - r << endl << endl; 

		// Stores the Arrival time and CPU burst time for processes
        for(int i = 0; i < N; i++)
        {
            myfile << "Process " << i + 1 << "		";
            myfile << "Arrival time : " << arrivalTime[i] << "		";
            myfile << "CPU burst time : " << burstTime[i] << endl;
        } 
		myfile << endl;

		// Computes average on 10 test cases

        avgfcfs += FCFS(arrivalTime,burstTime);	
        avgsjf += PremtiveSJF(arrivalTime,burstTime);	
        avgrr1 += RoundRobin(1,arrivalTime,burstTime);	
        avgrr2 += RoundRobin(2,arrivalTime,burstTime);	
        avgrr5 += RoundRobin(5,arrivalTime,burstTime);	
    }
    cout << "\nAVG ATN fcfs: " << avgfcfs/10.0 << endl;
    cout << "AVG ATN sjf: " << avgsjf/10.0 << endl;
    cout << "AVG ATN rr1: " << avgrr1/10.0 << endl;
    cout << "AVG ATN rr2: " << avgrr2/10.0 << endl;
    cout << "AVG ATN rr5: " << avgrr5/10.0 << endl;
	myfile.close();
    return 0 ;
}


