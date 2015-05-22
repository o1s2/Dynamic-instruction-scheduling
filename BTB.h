#ifndef BTB_H_INCLUDED
#define BTB_H_INCLUDED

class BTB
{
public:
	BTB(int cachesize = 0, int assoc = 0, int blocksize = 0);
	~BTB();
	int Read(unsigned long int pcaddress);
	void getprint(void);

private:
	int BTBcachesize, BTBassoc;
	typedef struct
	{
		unsigned long int tagaddress;
		int LRU_counter;
		int validbit;
		//int dirtybit;
	}cacheblock;
	cacheblock **BTBmatrix; //dynamic 2D array of struct
	int Blocksize, indexcount, indexbits, LRUtemp, tagnotfound, emptynotfound, Blockoffset;
	unsigned long int pc_address;
	unsigned long int pctemp, pctag, pcindextemp;
	int pcindex;
};

#endif 
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
