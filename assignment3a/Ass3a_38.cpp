#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <time.h>

using namespace std;

#define L 0.5

double expo_dist(double lambda){
    double u;
    u = rand() / (RAND_MAX + 1.0);
    return -log(1- u) / lambda;
}

int max(int a, int b)
{
    if(a > b) return a;
	else return b;
}


bool Allzeros(vector<int> v)
{
	for(int i = 0; i < v.size(); i++)
	{
		if(v[i] != 0)return false;
	}
	
	return true;
}

int isIn( vector<int> v,int index)
{
	for(int i = 0; i < v.size(); i++)
	{
		if(v[i] == index)return i;
	}
	return -1;
}

int minRem(vector<int> v,int index,vector<int> a)
{
	int minVal = 1000000;
	int minIndex = 0;
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
		
		if(isIn(arrT,index) >= 0 || remT[currProcess] == 0)
		{
			currProcess = minRem(remT,index, arrT);
		}
		index ++;
		remT[currProcess] --;
		for(int i = 0; i < arrT.size(); i++)
		{
			if(remT[i] == 0 && !finishMarker[i])
			{
				finishT[i] = index;
				finishMarker[i] = true;
			}
		}
		
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
		time ++;

		remT[currProcess] --;
		if(remT[currProcess] == 0)
		{
			finishT[currProcess] = time;
		}

		if(time % TimeQuantum == 0 || remT[currProcess] == 0)
		{
			while(!Allzeros(remT))
			{
				currProcess = (currProcess + 1) % arrT.size();
				if(remT[currProcess] != 0 && arrT[currProcess] <= time) break;
			}
		}		

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

int main()
{
    int N; 
    cout << "Enter number of process to run : ";
    cin >> N;
    double avgfcfs = 0,avgsjf = 0,avgrr1 = 0,avgrr2 = 0,avgrr5 = 0;
    int r = 10;
    while(r--){

        vector <int> arrivalTime;
        vector <int> burstTime;
        arrivalTime.push_back(0);
        srand (time(NULL));

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

        for(int i = 0; i < N; i++)
        {
            cout << "Process " << i + 1 << endl;
            cout << "Arrival time : " << arrivalTime[i] << endl;
            cout << "CPU burst time : " << burstTime[i] << endl;
        } 


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
    return 0 ;
}

