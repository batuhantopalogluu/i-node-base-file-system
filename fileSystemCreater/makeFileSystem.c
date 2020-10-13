/* 
* * * * * * * * * * * * * * * * * * * * * *
*  BatuHan  TOPALOĞLU 1510244026  05 2020 *
* * * * * * * * * * * * * * * * * * * * * *
*/

#include "types.h"

#define ONE_MB_IN_BYTE 1048576

void superBlock(int blockSize,int iNodeNumber,int numOfTotalBlocks ,FILE * fpp);

int main(int argc, char const *argv[])
{
	
	if(argc != 4){
		printf("\nERR- Invalid parameters number \n\n\t-> makeFileSystem BLOCK_SIZE_IN_KB  NUMBER_OF_FREE_INODE   EMPTY_FILE_SYSTEM\n" );
	}
	
	FILE *fd;

	fd=fopen(argv[3],"w+b");
	
	if (!fd){
		printf("ERR- Unable to open file!");
		return -1;
	}

	int blockSize = (atoi(argv[1]) )* 1024;
	int numOfTotalBlocks = ceil((float)(ONE_MB_IN_BYTE / blockSize));
	int numberOfINodes = atoi(argv[2]);

	if(blockSize <= 0 || numberOfINodes <=0 || !(blockSize != 1 || blockSize != 2 || blockSize != 4 || 
	  blockSize != 8  || blockSize != 16 || blockSize != 32))
	{

		printf("ERR - invalid function parameters\n");
		close(fd);
		return -1;
	}
	int check = (numberOfINodes * (sizeof(bool) + (sizeof(int) * 14) +  20)) / blockSize;
	if(check > numOfTotalBlocks ){
		printf("Err- too big i node number\n");
		close(fd);
		return;
	}

	
	char temp[blockSize];

	memset(temp, 0, blockSize); 

	int i;
	for ( i = 0; i < numOfTotalBlocks; ++i)
		fwrite(temp, 1, blockSize, fd);

	superBlock(blockSize,numberOfINodes,numOfTotalBlocks,fd);
		
	fclose(fd);

	return 0;
}


void superBlock(int blockSize,int iNodeNumber,int numOfTotalBlocks ,FILE * fp){
	

    struct superBlock superB;
    superB.blockSize = blockSize;
    superB.iNodeNumber = iNodeNumber;
    superB.numOfTotalBlocks = numOfTotalBlocks;

    superB.numOfBlocksForINode = ceil(((float)(iNodeNumber * (sizeof(bool) + (sizeof(int) * 14) +  20) )) / blockSize);

    superB.numOfBlocksForSuperAndFSMG = ceil(((float)iNodeNumber + numOfTotalBlocks + (sizeof(int) * 9) ) / blockSize);


	superB.startBlockNumOfINodes = superB.numOfBlocksForSuperAndFSMG;

    superB.startBlockNumOfDataBlocks = superB.numOfBlocksForSuperAndFSMG + superB.numOfBlocksForINode;

    fseek(fp, 0, SEEK_SET );
		
	fwrite(&superB,(sizeof(int) * 7),1,fp);


/////  FREE SPACE MANAGEMENT  /////////////

	struct freeSpaceManagement fsmg;
	fsmg.freeBlockNum = superB.numOfTotalBlocks - superB.startBlockNumOfDataBlocks;
	
	fwrite(&(fsmg.freeBlockNum),sizeof(int),1,fp);
	fsmg.freeINodeBlocks = calloc(sizeof(char),iNodeNumber);
	int i;
	for(i = 0; i < superB.iNodeNumber;i++){
		fsmg.freeINodeBlocks[i] = 'f';
		fwrite(&(fsmg.freeINodeBlocks[i]),sizeof(char),1,fp);
	}

	fsmg.freeDataBlocks = calloc(sizeof(char),superB.numOfTotalBlocks);
	
	for (i = 0; i < superB.startBlockNumOfDataBlocks; i++){
		fsmg.freeDataBlocks[i] = 'u';
		fwrite(&(fsmg.freeDataBlocks[i]),sizeof(char),1,fp);

	}

	for(i = superB.startBlockNumOfDataBlocks; i < superB.numOfTotalBlocks;i++){
		fsmg.freeDataBlocks[i] = 'f';
		fwrite(&(fsmg.freeDataBlocks[i]),sizeof(char),1,fp);
	}

	
///////////  I NODES ////////////////////////


	fseek(fp, superB.startBlockNumOfINodes * blockSize, SEEK_SET );

	struct iNode inodes;

	inodes.isDir = false;
	inodes.size = -1;
	for(i = 0;i < 13 ;i++){
		inodes.blogs[i] = -2; 
	}
    struct tm *sTm;

	time_t now = time (0);
	sTm = localtime (&now);

	strftime (inodes.lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", sTm);
	
	int z = 0;
	for(;z < iNodeNumber;z++){

		fwrite(&(inodes.isDir), sizeof(bool), 1, fp);
		fwrite(&(inodes.size), sizeof(int), 1, fp);

		for(i = 0;i < 13 ;i++){
			fwrite(&(inodes.blogs[i]), sizeof(int), 1, fp);
		}
		
		fwrite(&inodes.lastModifiedTime,sizeof(char),20,fp);
	
	}


/////// ROOT DIRECTORY ///////

	// root i node
 	fseek(fp, superB.startBlockNumOfINodes * blockSize, SEEK_SET );

 	struct iNode ForRootDir;
 	ForRootDir.isDir = true;
 	ForRootDir.size = blockSize;   // empty dir

	sTm = localtime (&now);

	strftime (ForRootDir.lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", sTm);
	
 	ForRootDir.blogs[0] = 0;//superB.startBlockNumOfDataBlocks; // BLOCK NUMBER

 	fwrite(&(ForRootDir.isDir), sizeof(bool), 1, fp);
 	fwrite(&(ForRootDir.size), sizeof(int), 1, fp);

 	
 	fwrite(&(ForRootDir.blogs[0]), sizeof(int), 1, fp);

 	int m = -2;
 	for( i = 0 ; i < 12;i++)
 		fwrite(&m, sizeof(int), 1, fp);

 	fwrite(&ForRootDir.lastModifiedTime,sizeof(char),20,fp);

 
 	fseek(fp, superB.startBlockNumOfDataBlocks * blockSize, SEEK_SET );

 	struct dataBlockEntry rootEntry;
 	rootEntry.nodeId = 0;
 	strcpy(rootEntry.fileName,"..");

 	struct dataBlockEntry currentEntry;
 	currentEntry.nodeId = 0;
 	strcpy(currentEntry.fileName,".");

 	fwrite(&rootEntry, sizeof(struct dataBlockEntry), 1, fp);
 	fwrite(&currentEntry, sizeof(struct dataBlockEntry), 1, fp);

 	struct dataBlockEntry emptyEntry;
 	emptyEntry.nodeId = -1;
 	strcpy(emptyEntry.fileName,"\0");
 	
 	for(i = 0 ; i < (blockSize / 16) - 2; i++){

		fwrite(&emptyEntry, sizeof(struct dataBlockEntry), 1, fp); 		
 	}


 	// free block sayısını düşürür;
 	// inode free değil yapar ,node u kullanılan a çeker

 	fseek(fp, 7 * sizeof(int), SEEK_SET ); // free i node bool ın 0 ının adresş
 	int tempA = fsmg.freeBlockNum - 1;
 	fwrite(&tempA, sizeof(int), 1, fp);
 	
 	fseek(fp, ( 8 * sizeof(int)), SEEK_SET );

 	char temp = 'u';
 	fwrite(&temp, sizeof(char), 1, fp); // freeInodes[0]

 	//fseek(fp, superB.startBlockNumOfDataBlocks * superB.blockSize, SEEK_SET );
 	fseek(fp, (( 8 * sizeof(int))) + (sizeof(char) * superB.iNodeNumber) , SEEK_SET );
 	fwrite(&temp, sizeof(char), 1, fp);

	
	return ;
}