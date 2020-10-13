#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>

struct iNode
{
 	bool isDir;        				
	int size;		   				 				
	int blogs[13]; 					// 10 direct pointers, 1 single indirect, 1 double indirect pointer 
	char lastModifiedTime[20];
};	


struct dataBlockEntry  
{
	short  nodeId;      // 2 BYTE
	char fileName[14];  // 14 byte dosy
};


struct superBlock
{
	int blockSize;
	int iNodeNumber;
	int numOfTotalBlocks;			
	int numOfBlocksForINode;		
	int numOfBlocksForSuperAndFSMG;
	int startBlockNumOfINodes;	
	int startBlockNumOfDataBlocks;
};


struct freeSpaceManagement
{
	int freeBlockNum;			
	char * freeINodeBlocks;
	char * freeDataBlocks;
};




#endif