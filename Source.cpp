#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <deque>
#include <stdlib.h>
#include "BTB.h"

using namespace std;


/*sim <S> <N> <BLOCKSIZE> <L1_size> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <tracefile>

<PC> <operation type> <dest reg #> <src1 reg #> <src2 reg #> <mem address>

<seq_no> fu{<op_type>} src{<src1>,<src2>} dst{<dst>} IF{<begin-cycle>,<duration>} ID{<begin-cycle>,<duration>} IS{…} EX{…} WB{…}*/
int main(int argc, char *argv[])
{
	int S = atoi(argv[1]);
	int N = atoi(argv[2]);
	int BLOCKSIZE = atoi(argv[3]);
	int L1_SIZE = atoi(argv[4]);
	int L1_ASSOC = atoi(argv[5]);
	int L2_SIZE = atoi(argv[6]);
	int L2_ASSOC = atoi(argv[7]);
	char *trace_file = argv[8];

	ifstream myfile;
	myfile.open(trace_file);

	unsigned long int PC, mem_address;
	int operation_type;
	int dest_reg, src1_reg, src2_reg;
	int currentseq = 0;
	int noofinstructions = 0;
	int cyclecounter = 0, seqcounter = 0, issuecounter = 0, issuetemp = 0, exectemp = 0, IFcyctemp = 0, structemp = 0;
	int finalcycles = 0;

	int L1cache_miss_hit, L2cache_miss_hit;
	int instructions_ended = 0;
	int L1cache_accesses = 0, L2cache_accesses = 0, L1cache_misses = 0, L2cache_misses = 0;

	/*Read the whole file to get the total number of instructions*/
	while (myfile >> hex >> PC >> dec >> operation_type >> dec >> dest_reg >> dec >> src1_reg >> dec >> src2_reg >> hex >> mem_address)
	{
		/*
		if (myfile.eof())
		{
			break;
		}
		*/
		++noofinstructions;
	}
	myfile.close();
	myfile.clear();

	/*register file RMT renaming map table*/
	typedef struct  
	{
		int ready; //1: Ready, 0: Not-ready
		int tag;
		//int tagvalue;
	}regfile;

	regfile regfilematrix[128];

	for (int i = 0; i < 128; ++i)
	{
		regfilematrix[i].ready = 1;//1:ready and 0:not ready
		regfilematrix[i].tag = 0;
	}

	
#if 0
	enum instruction_state { IF, ID, IS, EX, WB };
	struct fakeROB
	{
		unsigned long int PC, mem_address;
		int operation_type, tag;
		int dest_reg, src1_reg, src2_reg;
		int src1_ready, src2_ready, src1_tag, src2_tag;
		instruction_state state;
		int IFbegin, IFcycle, IDbegin, IDcycle, ISbegin, IScycle, EXbegin, EXcycle, WBbegin, WBcycle;
		int EXcycletimer;
	};
#endif

	fakeROB *fakeROBmatrix = new fakeROB[noofinstructions];
	/*Define 2 queue one for dispatch of size 2N and one for scheduling in reservation stations of size S*/
	deque<fakeROB> dispatchqueue;
	deque<fakeROB> decodequeue;
	deque<fakeROB> schedulingqueue;
	deque<fakeROB> execqueue;
	//fakeROB temp;

	BTB L1cache = BTB(L1_SIZE, L1_ASSOC, BLOCKSIZE);
	BTB L2cache = BTB(L2_SIZE, L2_ASSOC, BLOCKSIZE);

	/*FETCH the entire instruction data into fakeROB one dimension struct array*/
	myfile.open(trace_file);
	//while (myfile >> hex >> PC >> operation_type >> dest_reg >> src1_reg >> src2_reg >> hex >> mem_address)
	while (myfile >> hex >> PC >> dec >> operation_type >> dec >> dest_reg >> dec >> src1_reg >> dec >> src2_reg >> hex >> mem_address)
	{
		/*
		if (myfile.eof())
		{
		break;
		}
		*/
		fakeROBmatrix[currentseq].tag = currentseq;
		fakeROBmatrix[currentseq].PC = PC;
		fakeROBmatrix[currentseq].operation_type = operation_type;
		if (operation_type == 0)
		{
			fakeROBmatrix[currentseq].EXcycletimer = 1; //latency or no of cycles needed for EX for this operation type
		}
		if (operation_type == 1)
		{
			fakeROBmatrix[currentseq].EXcycletimer = 2; //latency or no of cycles needed for EX for this operation type
		}
		if (operation_type == 2)
		{
			fakeROBmatrix[currentseq].EXcycletimer = 5; //latency or no of cycles needed for EX for this operation type
		}
		fakeROBmatrix[currentseq].dest_reg = dest_reg;
		fakeROBmatrix[currentseq].src1_reg = src1_reg;
		if (src1_reg == -1)
		{
			fakeROBmatrix[currentseq].src1_ready = -1;
		}
		fakeROBmatrix[currentseq].src1_tag = 0;
		fakeROBmatrix[currentseq].src2_reg = src2_reg;
		if (src2_reg == -1)
		{
			fakeROBmatrix[currentseq].src2_ready = -1;
		}
		fakeROBmatrix[currentseq].src2_tag = 0;
		fakeROBmatrix[currentseq].mem_address = mem_address;
		fakeROBmatrix[currentseq].state = IF;
		fakeROBmatrix[currentseq].IFbegin = 0;
		fakeROBmatrix[currentseq].IFcycle = 0;
		fakeROBmatrix[currentseq].IDbegin = 0;
		fakeROBmatrix[currentseq].IDcycle = 0;
		fakeROBmatrix[currentseq].ISbegin = 0;
		fakeROBmatrix[currentseq].IScycle = 0;
		fakeROBmatrix[currentseq].EXbegin = 0;
		fakeROBmatrix[currentseq].EXcycle = 0;
		fakeROBmatrix[currentseq].WBbegin = 0;
		fakeROBmatrix[currentseq].WBcycle = 0;
		++currentseq;
		//break;
	}


	/*start the processing of instructions*/
	do{
		/*if all the instructions are in WB state then come out of the do while loop*/
		
		instructions_ended = 0;
		/*
		if (structemp < noofinstructions)
		{
			for (int i = structemp; i < (structemp + 100); ++i)
			{
				if (fakeROBmatrix[i].state == WB)
				{
					++instructions_ended;
				}
			}
		}
		if (instructions_ended == 100)
		{
			for (int i = structemp; i < (structemp + 100); ++i)
			{
				delete fakeROBmatrix[i];
			}
		}
		*/
		
		for (int i = 0; i < noofinstructions; ++i)
		{
			if (fakeROBmatrix[i].state == WB)
			{
				++instructions_ended;
			}
		}
		
		if (instructions_ended == noofinstructions)
		{
			break;
		}
		
		/*tackle the exec queue by checking whether the exec timer is finished or  not*/
		exectemp = execqueue.size();
		for (int k = 0; k < exectemp; ++k)
		{
			for (int i = 0; i < execqueue.size(); ++i)//erases firstmost completes EX instr each iteration
			{
				if ((execqueue[i].EXcycletimer == 0) && (execqueue[i].state == EX))//process only those instructions which are in EX state and leave those in WB state
				{
					//Update Register File if tag matches
					if (regfilematrix[execqueue[i].dest_reg].tag == execqueue[i].tag)
					{
						regfilematrix[execqueue[i].dest_reg].ready = 1;
						regfilematrix[execqueue[i].dest_reg].tag = 0;
					}
					//Wakeup dependent instructions
					for (int j = 0; j < schedulingqueue.size(); j++)
					{
						//if (ROB[j].state == IS){
						if ((schedulingqueue[j].src1_ready == 0) && (schedulingqueue[j].src1_tag == execqueue[i].tag))
						{
							schedulingqueue[j].src1_ready = 1;
							schedulingqueue[j].src1_tag = 0;
						}
						if ((schedulingqueue[j].src2_ready == 0) && (schedulingqueue[j].src2_tag == execqueue[i].tag))
						{
							schedulingqueue[j].src2_ready = 1;
							schedulingqueue[j].src2_tag = 0;
						}
					}
					
					execqueue[i].WBbegin = cyclecounter;
					++execqueue[i].WBcycle;
					finalcycles = execqueue[i].WBbegin + 1;
					execqueue[i].state = WB;
					fakeROBmatrix[execqueue[i].tag].IFbegin = execqueue[i].IFbegin;
					fakeROBmatrix[execqueue[i].tag].IFcycle = execqueue[i].IFcycle;
					fakeROBmatrix[execqueue[i].tag].IDbegin = execqueue[i].IDbegin;
					fakeROBmatrix[execqueue[i].tag].IDcycle = execqueue[i].IDcycle;
					fakeROBmatrix[execqueue[i].tag].ISbegin = execqueue[i].ISbegin;
					fakeROBmatrix[execqueue[i].tag].IScycle = execqueue[i].IScycle;
					fakeROBmatrix[execqueue[i].tag].EXbegin = execqueue[i].EXbegin;
					fakeROBmatrix[execqueue[i].tag].EXcycle = execqueue[i].EXcycle;
					fakeROBmatrix[execqueue[i].tag].WBbegin = execqueue[i].WBbegin;
					fakeROBmatrix[execqueue[i].tag].WBcycle = execqueue[i].WBcycle;
					fakeROBmatrix[execqueue[i].tag].state = execqueue[i].state;
					execqueue.erase(execqueue.begin() + i);
					break;
				}
			}
		}

		for (int i = 0; i < execqueue.size(); ++i)
		{
		//else
		//{
			--execqueue[i].EXcycletimer;
			++execqueue[i].EXcycle;
		//}
		}

		/*check the scheduling queue and issue the instructions which are ready and increase the IScycles if they are not ready*/
		issuecounter = 0;
		//issuetemp = 0;
		for (int l = 0; l < S; ++l)//checking the scheduling queue S times whether all the instructions are processed correctly or not
		{
			if (issuecounter < N)//issue the instructions only when the issue counter < N
			{
				for (int i = 0; i < schedulingqueue.size(); ++i) //check for the first instruction to be issued and increase the issued counter
				{
					if (((schedulingqueue[i].src1_ready == 1) && (schedulingqueue[i].src2_ready == 1)) ||
						((schedulingqueue[i].src1_ready == 1) && (schedulingqueue[i].src2_ready == -1)) ||
						((schedulingqueue[i].src1_ready == -1) && (schedulingqueue[i].src2_ready == 1)) ||
						((schedulingqueue[i].src1_ready == -1) && (schedulingqueue[i].src2_ready == -1)))
					{
						++issuecounter;
						execqueue.push_back(schedulingqueue[i]);
						//issuetemp = i;//for the last iteration record the i value so that the remaining instructions greater than i have to be stalled if the issue counter reaches N
						execqueue.back().EXbegin = cyclecounter;
						//++execqueue.back().EXcycle;
						//--execqueue.back().EXcycletimer;
						//execqueue.back().state = EX;
						//schedulingqueue.erase(schedulingqueue.begin() + i);//remove the issued element from scheduling queue to make place
						if (L1_SIZE != 0)
						{
							if (((execqueue.back().operation_type) == 2) && ((execqueue.back().mem_address) != 0))
							{
								++L1cache_accesses;
								L1cache_miss_hit = L1cache.Read(execqueue.back().mem_address);//returns 0 for hit and 1 for miss
								if (L1cache_miss_hit == 0) //for hit in L1 cache
								{
									execqueue.back().EXcycletimer = 5;
									fakeROBmatrix[execqueue.back().tag].EXcycletimer = 5;
								}
								if (L1cache_miss_hit == 1)//for miss in L1 cache
								{
									++L1cache_misses;
									if (L2_SIZE != 0)
									{
										++L2cache_accesses;
										L2cache_miss_hit = L2cache.Read(execqueue.back().mem_address);//returns 0 for hit and 1 for miss
										if (L2cache_miss_hit == 0)//for hit in L2 cache
										{
											execqueue.back().EXcycletimer = 10;
											fakeROBmatrix[execqueue.back().tag].EXcycletimer = 10;
										}
										if (L2cache_miss_hit == 1)//for miss in L2 cache
										{
											++L2cache_misses;
											execqueue.back().EXcycletimer = 20;
											fakeROBmatrix[execqueue.back().tag].EXcycletimer = 20;
										}
									}
									if (L2_SIZE == 0)//if L2 cache is not there cache miss penalty is 20 cycles
									{
										execqueue.back().EXcycletimer = 20;
										fakeROBmatrix[execqueue.back().tag].EXcycletimer = 20;
									}
								}
							}
						}
						++execqueue.back().EXcycle;
						--execqueue.back().EXcycletimer;
						execqueue.back().state = EX;
						schedulingqueue.erase(schedulingqueue.begin() + i);//remove the issued element from scheduling queue to make place
						break;//after the firstmost instruction in the schedulign queue is processed,sent to exec queue and destroyed break it so that it starts next iteration of the queue to encounter the first most instr to be processed
					}//end of if loop for checking to issue or not
				}
			}
			if (issuecounter == N) //break the first loop < S when issue counter = N
			{
				for (int j = 0; j < schedulingqueue.size(); ++j) //instr from issue temp till the end of queue are untouched and new set of queue formed after erasing the processed instr
				{
					++schedulingqueue[j].IScycle;//if not issued though they are ready issue cycles increase
				}
				break;
			}
		}
		if (issuecounter < N)//if less than N instructions are issued,from 0 till N-1 as there are no enough ready instructions 
		{
			for (int j = 0; j < schedulingqueue.size(); ++j) //increase the cycles for the rest of instr
			{
				++schedulingqueue[j].IScycle;//issue cycles increase for rest of instructions
			}
		}


		/*fill the scheduling queue from dispatch queue of size S*/
		for (int i = 0; i < S; ++i)
		{
			if ((schedulingqueue.size() == S) && (decodequeue.size() != 0) )
			{
				for (int j = 0; j < decodequeue.size(); ++j)
				{
					++decodequeue[j].IDcycle; //dispatching stalls in ID if the scheduling queue is full
				}
				break;
			}
			if (decodequeue.size() == 0)
			{
				break;
			}
			schedulingqueue.push_back(decodequeue.front());
			decodequeue.pop_front();//destroying front object and thus decrease the size of container by 1
			schedulingqueue.back().ISbegin = cyclecounter;
			++schedulingqueue.back().IScycle;
			schedulingqueue.back().state = IS;
			if (schedulingqueue.back().src1_reg != -1)
			{
				schedulingqueue.back().src1_ready = regfilematrix[schedulingqueue.back().src1_reg].ready;
				schedulingqueue.back().src1_tag = regfilematrix[schedulingqueue.back().src1_reg].tag;
			}
			if (schedulingqueue.back().src2_reg != -1)
			{
				schedulingqueue.back().src2_ready = regfilematrix[schedulingqueue.back().src2_reg].ready;
				schedulingqueue.back().src2_tag = regfilematrix[schedulingqueue.back().src2_reg].tag;
			}
			if (schedulingqueue.back().dest_reg != -1)
			{
				regfilematrix[schedulingqueue.back().dest_reg].ready = 0;
				regfilematrix[schedulingqueue.back().dest_reg].tag = schedulingqueue.back().tag;
			}
		}
		

		/*fill the decode queue*/
		for (int i = 0; i < N; ++i)
		{
			if ((decodequeue.size() == N) && (dispatchqueue.size() != 0)) //stop fetching if the dispatch queue is full or the instructions are finished
			//if (decodequeue.size() + dispatchqueue.size() == (2*N))
			{
				for (int j = 0; j < dispatchqueue.size(); ++j) //insert stalls for rest of the instructions in dispatch/fetch queue
				{
					++dispatchqueue[j].IDcycle;//insert stalls if the decode queue is full though there are instructions to dispatch
				}
				break;
			}
			if (dispatchqueue.size() == 0)
			{
				break;
			}
			//temp = dispatchqueue.pop_front();
			decodequeue.push_back(dispatchqueue.front());
			dispatchqueue.pop_front();//destroying front object and thus decrease the size of dispatch fetch container by 1
			//decodequeue.back().IDbegin = cyclecounter;
			decodequeue.back().IDbegin = (decodequeue.back().IFbegin) + 1;
			++decodequeue.back().IDcycle;
			decodequeue.back().state = ID;
		}

		/*fetch the instructions into dispatch queue of size 2N*/
		for (int i = 0; i < N; ++i)
		{
			if ((dispatchqueue.size() == N) || (seqcounter == noofinstructions)) //stop fetching if the dispatch queue is full or the instructions are finished
			{
				break;
			}
						
			dispatchqueue.push_back(fakeROBmatrix[seqcounter]);
			dispatchqueue.back().IFbegin = cyclecounter;
			IFcyctemp = dispatchqueue.back().IFcycle;
			IFcyctemp = IFcyctemp + 1;
			dispatchqueue.back().IFcycle = IFcyctemp; //will increment IF cycles
			dispatchqueue.back().state = IF;
			//cout << dispatchqueue.back().IFbegin << dispatchqueue.back().IFcycle << dispatchqueue.back().state;
			//fakeROBmatrix[seqcounter].IFbegin = cyclecounter;
			++seqcounter;
		}
		++cyclecounter;//do while loop will start with the cycle counter value of 0
	} while (1); //write a break instruction if the WB is completed for the last instruction

	myfile.close();

	for (int i = 0; i < noofinstructions; ++i)
	{
		cout << fakeROBmatrix[i].tag << " " << "fu{" << fakeROBmatrix[i].operation_type << "} ";
		cout << "src{" << fakeROBmatrix[i].src1_reg << "," << fakeROBmatrix[i].src2_reg << "} ";
		cout << "dst{" << fakeROBmatrix[i].dest_reg << "} ";
		cout << "IF{" << fakeROBmatrix[i].IFbegin << "," << fakeROBmatrix[i].IFcycle << "} ";
		cout << "ID{" << fakeROBmatrix[i].IDbegin << "," << fakeROBmatrix[i].IDcycle << "} ";
		cout << "IS{" << fakeROBmatrix[i].ISbegin << "," << fakeROBmatrix[i].IScycle << "} ";
		cout << "EX{" << fakeROBmatrix[i].EXbegin << "," << fakeROBmatrix[i].EXcycle << "} ";
		cout << "WB{" << fakeROBmatrix[i].WBbegin << "," << fakeROBmatrix[i].WBcycle << "}" << endl;
	}

	if (L1_SIZE != 0)
	{
		cout << "L1 CACHE CONTENTS" << endl;
		cout << "a. number of accesses :" << dec << L1cache_accesses << endl;
		cout << "b. number of misses :" << dec << L1cache_misses << endl;
		L1cache.getprint();
		cout << endl;
	}

	if (L2_SIZE != 0)
	{
		cout << "L2 CACHE CONTENTS" << endl;
		cout << "a. number of accesses :" << dec << L2cache_accesses << endl;
		cout << "b. number of misses :" << dec << L2cache_misses << endl;
		L2cache.getprint();
		cout << endl;
	}

	cout << "CONFIGURATION\n";
	cout << " superscalar bandwidth (N) = " << dec << N << endl;
	cout << " dispatch queue size (2*N) = " << dec << 2 * N << endl;
	cout << " schedule queue size (S)   = " << dec << S << endl;
	cout << "RESULTS" << endl;
	cout << " number of instructions = " << dec << noofinstructions << endl;
	cout << " number of cycles       = " << dec << finalcycles << endl;
	printf(" IPC                    = %0.2f\n", float(noofinstructions) / finalcycles);
	
	
	return 0;
	system("pause");
}
