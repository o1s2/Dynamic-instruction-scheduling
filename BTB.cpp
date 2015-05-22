#include <iostream>
#include <cstdlib>
#include <cmath>
#include "BTB.h"

using namespace std;

BTB::BTB(int cachesize, int assoc ,int blocksize)
{
	BTBcachesize = cachesize;
	BTBassoc = assoc;
	Blocksize = blocksize;
	Blockoffset = log2(Blocksize);
	if (BTBcachesize != 0)
	{
		if (BTBassoc != 0) //set associative
		{
			indexcount = BTBcachesize / (Blocksize * BTBassoc); //no of sets= size/(assoc*block)
			indexbits = log2(indexcount); //no of bits needed for index

			/*******Dynamic array of struct declare********/
			BTBmatrix = new cacheblock*[indexcount];
			for (int i = 0; i < indexcount; ++i)
			{
				BTBmatrix[i] = new cacheblock[BTBassoc];
			}
			/*********initialize struct values*********/
			for (int i = 0; i < indexcount; ++i)
			{
				for (int j = 0; j < BTBassoc; ++j)
				{
					BTBmatrix[i][j].tagaddress = 0;
					BTBmatrix[i][j].LRU_counter = BTBassoc - j - 1; //filling the blocks in order
					BTBmatrix[i][j].validbit = 0;
					//BTBmatrix[i][j].dirtybit = 0;
					//cout << "i \t" << i << "\t j \t" << j << "\t tag \t" << BTBmatrix[i][j].tagaddress << "\t lru \t" << BTBmatrix[i][j].LRU_counter << "\t valid \t" << BTBmatrix[i][j].validbit << endl;
				}
			}
		}

		if (BTBassoc == 0) //fully associative
		{
			BTBassoc = BTBcachesize / Blocksize;
			indexcount = 1;
			indexbits = 0;
			/*******Dynamic array of struct declare********/
			BTBmatrix = new cacheblock*[indexcount];
			for (int i = 0; i < indexcount; ++i)
			{
				BTBmatrix[i] = new cacheblock[BTBassoc];
			}
			/*********initialize struct values*********/
			for (int i = 0; i < indexcount; ++i)
			{
				for (int j = 0; j < BTBassoc; ++j)
				{
					BTBmatrix[i][j].tagaddress = 0;
					BTBmatrix[i][j].LRU_counter = BTBassoc - j - 1; //filling the blocks in order from 1st block to last block
					BTBmatrix[i][j].validbit = 0;
				}
			}
		}
	}
	else
	{
		indexcount = 0;
		/*******Dynamic array of struct declare********/
		BTBmatrix = NULL;
		/*
		for (int i = 0; i < indexcount; ++i)
		{
		BTBmatrix[i] = new cacheblock[BTBassoc];
		}
		*/
	}
}

BTB::~BTB()
{
	delete BTBmatrix;
}

int BTB::Read(unsigned long int pcaddress)
{
	pc_address = pcaddress;
	pctemp = pc_address >> Blockoffset; //eliminates right most bits equal to block offset
	pctag = pctemp >> indexbits; //gives tag address
	pcindextemp = 0;
	for (int k = 0; k < indexbits; ++k)
	{
		pcindextemp += (1 << k);
	}
	pcindex = pctemp & pcindextemp; //gives the index
	if (indexbits == 0) //if index bits are zero then pcindex needs to be 0 in order to access fully associative cache
	{
		pcindex = 0;
	}

	tagnotfound = 0;
	for (int j = 0; j < BTBassoc; ++j)
	{

		if (BTBmatrix[pcindex][j].tagaddress == pctag)//hit
		{
			//cout << "tag found" << endl;
			//cout << "pcindex \t" << pcindex << "\t j \t" << j << "\t tag \t" << BTBmatrix[pcindex][j].tagaddress << "\t lru \t" << BTBmatrix[pcindex][j].LRU_counter << "\t valid \t" << BTBmatrix[pcindex][j].validbit << endl;
			LRUtemp = BTBmatrix[pcindex][j].LRU_counter;
			for (int i = 0; i < BTBassoc; ++i)
			{
				if (BTBmatrix[pcindex][i].LRU_counter < LRUtemp)
				{
					++BTBmatrix[pcindex][i].LRU_counter; //increment the LRU counters which are less than current LRU hit counter
				}
			}
			BTBmatrix[pcindex][j].LRU_counter = 0;
			return 0; //0 for hit
			break;
		}
		else
		{
			++tagnotfound; //if tagnotfound is equal to assoc then its a miss
		}
	}

	if (tagnotfound == BTBassoc) //miss then update the buffer with new address according to LRU counters and invalid bits
	{
		//cout << "tag not found" << endl;

		emptynotfound = 0;
		for (int j = 0; j < BTBassoc; ++j)
		{
			if ((BTBmatrix[pcindex][j].validbit == 0) && (BTBmatrix[pcindex][j].LRU_counter == (BTBassoc - 1)))//filling the empty blocks
			{
				//cout << "empty block" << endl;
				//cout << "pcindex \t" << pcindex << "\t j \t" << j << "\t tag \t" << BTBmatrix[pcindex][j].tagaddress << "\t lru \t" << BTBmatrix[pcindex][j].LRU_counter << "\t valid \t" << BTBmatrix[pcindex][j].validbit << endl;
				BTBmatrix[pcindex][j].tagaddress = pctag;
				LRUtemp = BTBmatrix[pcindex][j].LRU_counter;
				for (int i = 0; i < BTBassoc; ++i)
				{
					if (BTBmatrix[pcindex][i].LRU_counter < LRUtemp)
					{
						++BTBmatrix[pcindex][i].LRU_counter; //increment the LRU counters which are less than current LRU hit counter
					}
				}
				BTBmatrix[pcindex][j].LRU_counter = 0;//making the counter to most recently used
				BTBmatrix[pcindex][j].validbit = 1; //as the empty blocks are filled making setting the validbit
				break;
			}
			else
			{
				++emptynotfound;//if there are no empty blocks this will be equal to assoc
			}
		}


		if (emptynotfound == BTBassoc) //if there are no invalid blocks fill the blocks according to the LRU counter
		{
			for (int j = 0; j < BTBassoc; ++j)
			{
				if (BTBmatrix[pcindex][j].LRU_counter == (BTBassoc - 1))
				{
					//cout << "no empty block" << endl;
					//cout << "pcindex \t" << pcindex << "\t j \t" << j << "\t tag \t" << BTBmatrix[pcindex][j].tagaddress << "\t lru \t" << BTBmatrix[pcindex][j].LRU_counter << "\t valid \t" << BTBmatrix[pcindex][j].validbit << endl;
					BTBmatrix[pcindex][j].tagaddress = pctag;
					LRUtemp = BTBmatrix[pcindex][j].LRU_counter;
					for (int i = 0; i < BTBassoc; ++i)
					{
						if (BTBmatrix[pcindex][i].LRU_counter < LRUtemp)
						{
							++BTBmatrix[pcindex][i].LRU_counter; //increment the LRU counters which are less than current LRU hit counter
						}
					}
					BTBmatrix[pcindex][j].LRU_counter = 0;//making the counter to most recently used
					break;
				}
			}
		}
		return 1;//1 for miss
	}

}



void BTB::getprint(void)
{
	for (int l = 0; l < indexcount; ++l)
	{
		cout << "set " << dec << l << " :";
		for (int m = 0; m < BTBassoc; ++m)
		{
			cout << hex << BTBmatrix[l][m].tagaddress << "    ";
		}
		cout << endl;
	}

}