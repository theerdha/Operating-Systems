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
char page_table[64][4];
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
		x1 = (int *)(page_table[vir]);
		b1 = *x1;
		b2 = b1 % 8;
		// if((page_table[vir][3] | 0xFB) == 0xFF) break;
	    if(b2 >= 4) break;
	}
	*phy = b1/8;
	return vir;
}

int NRU(int * phy)
{
	int * x1; int b1,b2,b3,vir = -1;
	if(PAGE_FAULTS % 10 == 0)
	{
		for(int i = 0; i < 64; i++) 
		{
			x1 = (int *)(page_table[i]);
			b1 = *x1;
			b1 -= 1;
			*x1 = b1;
		}
	}

	vector<int> category0;vector<int> category1;vector<int> category2;vector<int> category3;
	for(int i = 0; i < 64; i++) 
	{
		x1 = (int *)(page_table[i]);
		b2 = *x1;
		b3 = b2 % 8;
		b1 = b2 % 4;
		if(b3 >= 4)
		{
			if(b1 == 0)category0.push_back(i);
			else if(b1 == 1)category2.push_back(i);
			else if(b1 == 2)category1.push_back(i);
			else category3.push_back(i);
		}
	}

	if(category0.size() != 0) vir = category0[(rand() % (category0.size()))];
	else if(category1.size() != 0) vir = category1[(rand() % (category1.size()))];
	else if(category2.size() != 0) vir = category2[(rand() % (category2.size()))];
	else vir = category3[(rand() % (category3.size()))];
	
	x1 = (int *)(page_table[vir]);
	b1 = *x1;
	b1 /= 8;
	*phy = b1;
	return vir;
}

int LRU(int * phy)
{
	int vir,b1;
	int * x1;
	vir = page_LRU.front();
	page_LRU.pop_front();
	x1 = (int *)(page_table[vir]);
	b1 = *x1;
	int b3 = b1/8;
	int b2 = b1 % 8;
	if(b2 >= 4)
	{
		*phy = b3;
	}
	return vir;
}

int FIFO(int * phy)
{
	int vir,b1;
	int *x1;
	// int min = 10000; 
	// int minindex;
	// for(int i = 0; i < frameLine.size(); i++)
	// {
	// 	if(frameLine[i] < min)
	// 	{
	// 		min = frameLine[i];
	// 		minindex = i;
	// 	}
	// }
	// *phy = minindex;
	vir = page_FIFO.front();
	page_FIFO.pop();
	
	x1 = (int *)(page_table[vir]);
	b1 = *x1;
	int b3 = b1/8;
	int b2 = b1 % 8;
	*phy = b3;
	
	return vir;
}

int SecChance(int * phy)
{
	int x;int vir,b1,b2;
	int *x1;

	while(1)
	{
		x = page_FIFO.front();
		x1 = (int *)(page_table[x]);
		b1 = *x1;
		b2 = b1 % 2;
		if(b2 == 0)
		{
			vir = x;
			*phy = b1/8;
			page_FIFO.pop();
			break;	
		}
		else
		{
			b1 -= 1;
			*x1 = b1;
			page_FIFO.push(page_FIFO.front());
			page_FIFO.pop();
		} 
		
	}
	return vir;
}

void ReadWrite(int num, char rw,int LINE_NUMBER,int algo)
{
	//page table entry is not valid(valid == 0)
	int * aa;
	int aaa;
	aa = (int *)(page_table[num]);
	aaa = *aa;
	aaa %= 8;
	
	//if((page_table[num][3] | 0xFB) == 0xFB)
	if(aaa < 4)
	{
		PAGE_FAULTS ++;	
		int phy = freeframe();
		if(phy != -1)
		{
			EXEC_CYCLE ++;
			cout << LINE_NUMBER << ": "<< "Load frame " <<  phy << " in page "<< num << endl;
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

			int * bb;
			int bbb;
			bb = (int *)(page_table[vir]);
			bbb = *bb;
			bbb %= 4;

			//if((page_table[vir][3] | 0xFD) == 0xFF) cout << "OUT" << vir  << " "<< phy <<endl;
			if(bbb >= 2)
			{
				cout << LINE_NUMBER << ": "<<"OUT " << vir  << " "<< phy <<endl;
				EXEC_CYCLE += 3000;	
			} 
			cout << LINE_NUMBER << ": "<< "IN " << num  << " "<< phy <<endl;
			cout << LINE_NUMBER << ": "<< "MAP " << num  << " "<< phy <<endl;
			cout << LINE_NUMBER << ": "<< "access memory" << endl;
		}
		
		//////copy the physical frame number into pagetable
		int *x;
		int b,c;
		x = (int *)(page_table[num]);
		b = *x;
		if((b % 8) < 4) c = 8 * phy + (b % 8) + 4;
		else c = 8 * phy + (b % 8);
		//cout << "c " << c << endl;
		//frameLine[phy] = LINE_NUMBER;
		page_FIFO.push(num);
		*x = c; 
		///page_table[num][3] = page_table[num][3] | 0x04; //valid bit is set to 1

	}
	else
	{
		EXEC_CYCLE ++;
		cout <<LINE_NUMBER << ": "<< "access memory" << endl;
	} 
	
	if(rw == '1')
	{
		int *x1;
		int b1;
		x1 = (int *)(page_table[num]);
		b1 = *x1;
		if((b1 % 4 ) < 2)b1 = b1 + 2; 
		*x1 = b1;
		//page_table[num][3] |= 0x02; // dirty bit is set one
	}
	
	int *x2;
	int b2;
	x2 = (int *)(page_table[num]);
	b2 = *x2;
	if((b2 % 2) < 1)b2 = b2 + 1; 
	*x2 = b2;

	page_LRU.remove(num);
	page_LRU.push_back(num);
	
	//page_table[num][3] |= 0x01; // referenced bit is set to 1
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
				//ReadWrite(page_number,line[0]);
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
		x = (int *)page_table[i];
		*x = 0;
	}
}

int main()
{
	cin >> FRAMES;
	srand (time(NULL));
	parse_page();

	Initialize();
	cout << endl << "FIFO PR" << endl;
	for(int i = 0; i < IR_TABLE.size(); i++) ReadWrite(IR_TABLE[i].page,IR_TABLE[i].rw,IR_TABLE[i].line_no,1);
	cout << "PAGE FAULTS = " << PAGE_FAULTS << " PAGE TRANSFERs = " << PAGE_TRANSFER << " EXEC CYCLES = " << EXEC_CYCLE << endl;

	Initialize();
	cout << endl << "Ranndom PR" << endl;
	for(int i = 0; i < IR_TABLE.size(); i++) ReadWrite(IR_TABLE[i].page,IR_TABLE[i].rw,IR_TABLE[i].line_no,2);
	cout << "PAGE FAULTS = " << PAGE_FAULTS << " PAGE TRANSFERs = " << PAGE_TRANSFER << " EXEC CYCLES = " << EXEC_CYCLE << endl;

	Initialize();
	cout << endl << "LRU PR" << endl;
	for(int i = 0; i < IR_TABLE.size(); i++) ReadWrite(IR_TABLE[i].page,IR_TABLE[i].rw,IR_TABLE[i].line_no,3);
	cout << "PAGE FAULTS = " << PAGE_FAULTS << " PAGE TRANSFERs = " << PAGE_TRANSFER << " EXEC CYCLES = " << EXEC_CYCLE << endl;

	Initialize();
	cout << endl << "NRU PR" << endl;
	for(int i = 0; i < IR_TABLE.size(); i++) ReadWrite(IR_TABLE[i].page,IR_TABLE[i].rw,IR_TABLE[i].line_no,4);
	cout << "PAGE FAULTS = " << PAGE_FAULTS << " PAGE TRANSFERs = " << PAGE_TRANSFER << " EXEC CYCLES = " << EXEC_CYCLE << endl;

	Initialize();
	cout << endl << "second chance PR" << endl;
	for(int i = 0; i < IR_TABLE.size(); i++) ReadWrite(IR_TABLE[i].page,IR_TABLE[i].rw,IR_TABLE[i].line_no,5);
	cout << "PAGE FAULTS = " << PAGE_FAULTS << " PAGE TRANSFERs = " << PAGE_TRANSFER << " EXEC CYCLES = " << EXEC_CYCLE << endl;
	
	return 0;	
}
