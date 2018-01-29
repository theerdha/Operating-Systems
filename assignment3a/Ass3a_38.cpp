#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <time.h>

using namespace std;

#define L 0.05

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

double FCFS( vector<int> arrT, vector<int> BT)
{
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

double PremtiveSJF( vector<int> arrT, vector<int> BT)
{
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

int main()
{
	int N; cin >> N;
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
	
	cout << "ATN in FCFS : " << FCFS(arrivalTime,burstTime) << endl;	
	cout << "ATN in premtive SJF : " << PremtiveSJF(arrivalTime,burstTime) << endl;	
	return 0 ;
}

