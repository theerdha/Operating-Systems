#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <sstream>
#include <queue>
#include <list>

using namespace std;

typedef struct INSTR
{
	int line_no;
	int page;
	char rw;
}instruction; 

vector<instruction> IR_TABLE;
int page_table[64];
vector<int> freeList; //--> 1 : free , 0 : occupied
vector<int> frameLine; //--> -1 : free , else line number at which it is loaded
queue <int> page_FIFO;
list <int> page_LRU;

int FRAMES;
int PAGE_FAULTS = 0, PAGE_TRANSFER = 0, EXEC_CYCLE = 0;

int freeframe()
{
	for(int i = 0; i < freeList.size(); i++)
	{
		if(freeList[i] == 1) return i;
	}
	return -1;
}

int RandomPR(int * phy)
{
	int vir,b1,b2;
	int *x1;
	while(1)
	{
		vir = rand() % 64;
		// if((page_table[vir][3] | 0xFB) == 0xFF) break;
	    if((page_table[vir] % 8) >= 4) break;
	}
	*phy = page_table[vir]/8;
	return vir;
}

int NRU(int * phy)
{
	int vir = -1;
	if(PAGE_FAULTS % 10 == 0)
	{
		for(int i = 0; i < 64; i++) 
		{
			if((page_table[i]) % 2 == 1)page_table[i] -= 1;
			
		}
	}

	vector<int> category0;vector<int> category1;vector<int> category2;vector<int> category3;
	for(int i = 0; i < 64; i++) 
	{
		
		if((page_table[i] % 8) >= 4)
		{
			if((page_table[i] % 4) == 0)category0.push_back(i);
			else if((page_table[i] % 4) == 1)category2.push_back(i);
			else if((page_table[i] % 4) == 2)category1.push_back(i);
			else category3.push_back(i);
		}
	}

	if(category0.size() != 0) vir = category0[(rand() % (category0.size()))];
	else if(category1.size() != 0) vir = category1[(rand() % (category1.size()))];
	else if(category2.size() != 0) vir = category2[(rand() % (category2.size()))];
	else vir = category3[(rand() % (category3.size()))];
	
	*phy = page_table[vir]/8;
	return vir;
}

int LRU(int * phy)
{
	int vir;
	vir = page_LRU.front();
	page_LRU.pop_front();
	if((page_table[vir] % 8) >= 4)
	{
		*phy = page_table[vir] /8;
	}
	return vir;
}

int FIFO(int * phy)
{
	int vir,b1;
	vir = page_FIFO.front();
	page_FIFO.pop();
	*phy = page_table[vir] / 8;
	return vir;
}

int SecChance(int * phy)
{
	int x, vir;

	while(1)
	{
		x = page_FIFO.front();
		
		if((page_table[x] % 2) == 0)
		{
			vir = x;
			*phy = page_table[x]/8;
			page_FIFO.pop();
			break;	
		}
		else
		{
			page_table[x] -= 1;
			page_FIFO.push(page_FIFO.front());
			page_FIFO.pop();
		} 
		
	}
	return vir;
}

void ReadWrite(int num, char rw,int LINE_NUMBER,int algo)
{
	//page table entry is not valid(valid == 0)
	
	if((page_table[num] % 8) < 4)
	{
		PAGE_FAULTS ++;	
		int phy = freeframe();
		if(phy != -1)
		{
			cout << LINE_NUMBER << ": "<< "MAP " <<  num << " "<< phy << endl;
			EXEC_CYCLE += 251;
			cout << LINE_NUMBER << ": "<<"access memory" << endl;
			freeList[phy] = 0;
		}

		else
		{
			// Victim page selection algorithm returns vir,phy
			PAGE_TRANSFER ++;
			int vir;
			if(algo == 1)vir = FIFO(&phy);
			else if(algo == 2)vir = RandomPR(&phy);
			else if(algo == 3)vir = LRU(&phy);
			else if(algo == 4)vir = NRU(&phy);
			else vir = SecChance(&phy);
			EXEC_CYCLE += 3501;
			cout << LINE_NUMBER << ": "<<"UNMAP " << vir  << " "<< phy << endl;
			page_table[vir] -= 4;

			if(page_table[vir] % 4 >= 2) //if dirty
			{
				cout << LINE_NUMBER << ": "<<"OUT " << vir  << " "<< phy <<endl;
				EXEC_CYCLE += 3000;	
			} 
			cout << LINE_NUMBER << ": "<< "IN " << num  << " "<< phy <<endl;
			cout << LINE_NUMBER << ": "<< "MAP " << num  << " "<< phy <<endl;
			cout << LINE_NUMBER << ": "<< "access memory" << endl;
		}
		
		//////copy the physical frame number into pagetable
		if((page_table[num] % 8) < 4) page_table[num] = 8 * phy + (page_table[num] % 8) + 4;
		else page_table[num] = 8 * phy + (page_table[num] % 8);
		page_FIFO.push(num);

	}
	else
	{
		EXEC_CYCLE ++;
		cout <<LINE_NUMBER << ": "<< "access memory" << endl;
	} 
	// dirty bit is set one
	if(rw == '1')
		if((page_table[num] % 4 ) < 2)page_table[num] = page_table[num] + 2; 
	
	// referenced bit is set to 1
	if((page_table[num] % 2) < 1)page_table[num] = page_table[num] + 1; 

	page_LRU.remove(num);
	page_LRU.push_back(num);
	return;
}

void parse_page()
{
	string line;
	int LINE_NUMBER = 0;
	ifstream myfile ("example.txt");
	if (myfile.is_open())
	{
		while ( getline (myfile,line) )
		{
			LINE_NUMBER ++;
			if(line[0] == '#')continue;
			else 
			{
				instruction a;
				string r = line.substr(2,2);
				stringstream s(r);
				int page_number;
				s >> page_number;
				a.page = page_number;
				a.rw = line[0];
				a.line_no = LINE_NUMBER;
				IR_TABLE.push_back(a);
			}
		}
		myfile.close();
	}
	return;
}

void Initialize()
{

	freeList.clear(); //--> 1 : free , 0 : occupied
	frameLine.clear(); //--> -1 : free , else line number at which it is loaded
	while (!page_FIFO.empty())
    {
        page_FIFO.pop();
    }
    while (!page_LRU.empty())
    {
        page_LRU.pop_back();
    }

	PAGE_FAULTS = 0;PAGE_TRANSFER = 0; EXEC_CYCLE = 0;
	for(int i = 0; i < FRAMES; i++)
	{
		freeList.push_back(1);
		frameLine.push_back(-1);
	}

	int * x;
	//Initializing valid,dirty and reference bits to be 0
	for(int i = 0 ; i < 64; i++)
	{
		page_table[i] = 0;
	}
}

void data_gen(int page_refs,int work_set,float prob_loc_ref, float prob_r)
{
	ofstream myfile;
	myfile.open ("example.txt");
	vector<int> recent_5(work_set);
	float r;
	int rw;
	int page,index;
	for(int i = 0 ; i < work_set; i++)
	{
		recent_5[i] = rand() % 64;
		r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		if(r <= prob_r) rw = 0; else rw = 1;
		myfile << rw << " " << recent_5[i] << endl;
	}

	for(int i = work_set; i < page_refs; i++)
	{
		r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		if(r <= prob_loc_ref)
		{
			index = rand() % work_set;
			page = recent_5[index];	
		}
		else
		{
			page = rand() % 64;
		}
		r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		if(r <= prob_r) rw = 0; else rw = 1;
		recent_5[i % work_set] = page;
		myfile << rw << " " << page << endl;
	}
	myfile.close();
	return;

}

int main()
{
	cin >> FRAMES;
	srand (time(NULL));
	data_gen(500,10,0.7,0.6);
	//parse_page();

	int count  = 1;


	// while(count <= 5)
	// {
	// 	Initialize();
	// 	if(count == 1)cout << endl << "FIFO PR" << endl;
	// 	else if(count == 2) cout << endl << "Ranndom PR" << endl;
	// 	else if(count == 3) cout << endl << "LRU PR" << endl;
	// 	else if(count == 4) cout << endl << "NRU PR" << endl;
	// 	else cout << endl << "second chance PR" << endl;
	// 	for(int i = 0; i < IR_TABLE.size(); i++)
	// 	{
	// 		ReadWrite(IR_TABLE[i].page,IR_TABLE[i].rw,IR_TABLE[i].line_no,count);
	// 		cout << "valid pages are : ";
	// 		for(int j = 0; j < 64; j++)
	// 		{
	// 			if( (page_table[j] % 8) >= 4) cout << j << " ";
	// 		}	
	// 		cout << endl;
	// 	} 
	// 	cout << "PAGE FAULTS = " << PAGE_FAULTS << " PAGE TRANSFERs = " << PAGE_TRANSFER << " EXEC CYCLES = " << EXEC_CYCLE << endl;
	// 	count ++;	
	// }
	
	return 0;	
}
