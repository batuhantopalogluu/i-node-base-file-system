/* 
* * * * * * * * * * * * * * * * * * * * * *
*  BatuHan  TOPALOĞLU 1510244026  05 2020 *
* * * * * * * * * * * * * * * * * * * * * *
*/

/*
	Kendime NOT : Bir çok yerde benimde gözüme çarpan kod tekrarları var o kısımlar beninde içime sinmedi.
*/


#include "types.h"

FILE *fp;

struct iNode * iNodes;
char * freeInodes;
char * freeBlocks;

char * usedInodesFSCK;  // bu liste fsck fonskiyonu çağrıldığında i nodelar gezilerek hesaplanır. 
						//freeInodes arrayinden bağımsız olduğu için fsck kontrollünde kullanılır.
char * usedBlockFSCK;   // bu liste fsck fonskiyonu çağrıldığında i nodelar tarafında kullanılan 
						// blok numaralarını tutarak free block listesine karşı kontrol elemanı olarak
						// kullanılır.

char * pathForLnSym;

int blockSize;
int iNodeNumber;
int numOfTotalBlocks;
int numOfBlocksForINode;		
int numOfBlocksForSuperAndFSMG;
int firstBlockNumOfINodes;
int firstBlockNumOfDataBlocks;
int freeBlockNum;

int hardLinkVar ;	  

char * writeReadFileName;

struct dataBlockEntry * currentDirBlock;

time_t rawtime;
struct tm *info;

int signDataBlock(char sign,int dataBlockId);	
void readRootDir(FILE *fp);

void initializeDirDataBlock(int dataBlockId,int currentNodeId,int parentNodeId);
int readDirDataBlockFromDisk(int dataBlockNumber);
int writeBackDirBataBLock(int dataBlockNumber);
void initializeINode(int nodeId,int dataBlockNumber,int blogNum);
int traverseSystem(int startingINodeId,bool dirOrNot, const char * path,int opCode,int hdVar);

int signFreeNodes(char sign,int freeNodeId);
void functionFsck();
void functionDumpe2fs();



char passChar = '\0';
int passNoedeId = -1; 

int main(int argc, char const *argv[])
{	
	int op = -1; 

	/*

opCode :

	hata 	-1
	list  	 0
	mkdir 	 1
	rmdir    2
	write    4 
	read     5
	del      6 
	dumpe2fs 7
	ln       8
	fsck  	 9
	lnsym    10

	*/

	if(argc == 1){
		printf("ERR- \n\t	HELP \n");
		return -1;
	}
	else if(argc == 3){

		if(strcmp(argv[2],"dumpe2fs") == 0) {

			op = 7; 
		}
		else if(strcmp(argv[2],"fsck") == 0) {

			op = 9; 
		}
	}
	else if(argc == 4){
		if(strcmp(argv[2],"list") == 0) {

			op = 0; 
		}
		else if(strcmp(argv[2],"mkdir") == 0) {

			op = 1; 
		}
		else if(strcmp(argv[2],"rmdir") == 0) {

			op = 2; 
		}
		else if(strcmp(argv[2],"del") == 0) {

			op = 6; 
		}
	}else if(argc == 5){ /// write / read

		if(strcmp(argv[2],"write") == 0) {

			op = 4; 
		}
		else if(strcmp(argv[2],"read") == 0) {

			op = 5; 
		}
		else if(strcmp(argv[2],"ln") == 0) {

			op = 8; 
		}
		else if(strcmp(argv[2],"lnsym") == 0) {

			op = 10; 
		}
	}
	if(op == -1){
		printf("ERR - invalid function call\n");
		return -1;
	}

	
	fp = fopen(argv[1],"rb+");
	
	if (!fp){
		printf("ERR- Unable to open file!\n");
		return -1;
	}
	
	fread(&blockSize,   sizeof(int), 1, fp);
	fread(&iNodeNumber, sizeof(int), 1, fp);
	fread(&numOfTotalBlocks, sizeof(int), 1, fp);
	fread(&numOfBlocksForINode,   sizeof(int), 1, fp);
	fread(&numOfBlocksForSuperAndFSMG, sizeof(int), 1, fp);
	fread(&firstBlockNumOfINodes,   sizeof(int), 1, fp);
	fread(&firstBlockNumOfDataBlocks, sizeof(int), 1, fp);
	fread(&freeBlockNum, sizeof(int), 1, fp);

	freeInodes = calloc(sizeof(char),iNodeNumber); 
	memset(freeInodes, 0, iNodeNumber); 
	freeBlocks = calloc(sizeof(char),numOfTotalBlocks);  
	memset(freeBlocks, 0, numOfTotalBlocks); 

	int i ;

	for(i = 0; i < iNodeNumber;i++){			//fsmg da tutalan free i node listesi diskten sisteme çekilir.
		fread(&freeInodes[i], 1,1 , fp);
	}
	for(i = 0; i < numOfTotalBlocks;i++){       //fsmg da tutalan free blok listesi diskten sisteme çekilir.
		fread(&freeBlocks[i], 1,1 , fp);
	}

	currentDirBlock =  calloc(16,blockSize/16);  // Bir directory bloğu ile işlem yapmam gerektiğinde, o bloğu diskten sisteme
												 // alıp üzerinde işlemler yaptıktan sonra tekrar diske yazıyorum. Bu blok onun için.

	iNodes = calloc(sizeof(struct iNode),iNodeNumber);	// Program başladığında i nodeları diskten sisteme alır. İşlemler bu i node listesi
	                                                 	// üzerinden yapılır. Program biterken i node listesi tekrar diske yazılır.

	
	fseek(fp, firstBlockNumOfINodes * blockSize, SEEK_SET );

	for(i = 0 ; i < iNodeNumber;i++){
		
		fread(&(iNodes[i].isDir), sizeof(bool), 1, fp);
	
		fread(&(iNodes[i].size), sizeof(int), 1, fp);
			
		int j;
		for(j = 0;j <  13;j++){
			
			fread(&(iNodes[i].blogs[j]), sizeof(int), 1, fp);
		}

		fread(&(iNodes[i].lastModifiedTime),sizeof(char),20,fp);
	}

	hardLinkVar = -1;									

	if(op == 7){									   // operasayona göre traverse fonksiyonu veya gerekli diğer fonksiyonlar çağrılır.
		functionDumpe2fs();
	}
	else if(op == 9){
		functionFsck();
	}
	else if(op == 0){

		traverseSystem(0,true,argv[3],0,-1);
	}
	else if (op == 1 || op == 2 || op == 6){	       // bu operasyonlar sisteme ekleme veya çıkarma yaptıkları için değişkenler diske yazılır

		traverseSystem(0,true,argv[3],op,-1);
		writeInodesToDisk();
		writeFreesToDisk();	
	}
	else if(op == 4 || op == 5 || op == 8 || op == 10){ // bu operasyonlar sisteme ekleme veya çıkarma yaptıkları için değişkenler diske yazılır

		writeReadFileName = calloc(sizeof(char),strlen(argv[4]));
		memset(writeReadFileName,0,strlen(argv[4]));
		strcpy(writeReadFileName,argv[4]);

		traverseSystem(0,true,argv[3],op,-1);

		writeInodesToDisk();
		writeFreesToDisk();
		free(writeReadFileName);
	}

	free(freeInodes);
	free(freeBlocks);
	free(iNodes);
	free(currentDirBlock);

	close(fp);

	return 0;
}

/*
*   DOUBLE UYUMLU DEĞİL DÜZELT +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
void functionFsck(){

	printf("\n !!! liste uzunlukları fazla olduğu için kıyaslamalar parçalara ayrılayarak alt alta yazılmıştır.\n"); 
	printf(" İlk olarak i nodelara ait sağlama liste basılmıştır. Onun bitimde bloklara ait liste yer almaktadır.\n\n");
	printf("-> i node sağlaması için üstteki liste fsck fonksiyonı tarafından free space management'tan bağımsız olarak hesaplanmıştır.\n");
	printf("  Altındaki liste direkt diskte fsmg tarafında tutulan free i nodes listesidir.\n\n");
	
	printf("-> block sağlaması için üstteki liste fsck fonksiyonı tarafından free space management'tan bağımsız olarak hesaplanmıştır.\n");
	printf("  Altındaki liste direkt diskte fsmg tarafında tutulan free blok listesidirr.\n\n");


	usedInodesFSCK = calloc(sizeof(bool),iNodeNumber); 		    //   Kullanılan i nodelar fsck için freeInodes arrayinden bağımsız
	memset(usedInodesFSCK, 0,sizeof(bool ) * iNodeNumber); 	    //   olarak hesaplanır.

	usedBlockFSCK = calloc(sizeof(bool),numOfTotalBlocks); 		//   Kullanılan i nodelar fsck için freeInodes arrayinden bağımsız
	memset(usedBlockFSCK, 0,sizeof(bool ) * numOfTotalBlocks); 	//   olarak hesaplanır.
	
	int i,j,z;
	
	j = numOfBlocksForINode + numOfBlocksForSuperAndFSMG;       //super block ve i nodeları tutmak için kullanılan blocklar işaretlenir.
	
	for(i = 0 ; i < j; i++ )
		usedBlockFSCK[i] = true;
	

	for(i = 0; i < iNodeNumber; i++){

		if(iNodes[i].size != -1){
			usedInodesFSCK[i] = 1;								//  bir i node'un size bilgisine bakarak kullanıp kullanılmadığını çıkarımı 
		}														//  yapılarak işaretlenir.		

		for(j = 0; j < 13;j++){
		
			if(iNodes[i].blogs[j] != -2){

				usedBlockFSCK[iNodes[i].blogs[j]] = true;      // i nodeların ilk 10 bloğu direk erişimdir. Eğer bunlara block atanmışsa o blok
															   // işaretlenir.
			
				if(j == 10){  								   // single indirect erişim olan bloklarda işaretlenir.

					fseek(fp, (firstBlockNumOfDataBlocks + iNodes[i].blogs[j]) * blockSize, SEEK_SET );

					for(z = 0; z < blockSize / sizeof(int); z++){
						int temp;
						fread(&temp, sizeof(int),1 , fp);
						if(temp != -1){
							usedBlockFSCK[temp] = true;
						}

					}
				}
				else if(j == 11){
					/*

					double

					*/
				} 				
			}		
		}
	}

	int f = iNodeNumber;
	i = 0;
	int u,k;
	j = 0;

	printf("\n>>>> i-nodes :\n");
	
	while(1){
		
		if(f >= 25)  u = 25;
		else         u = f;

		printf("\n--------------------------------------------------------------------------------------------------------------------\n");
		
		printf("i-node ids     : ");
		
		for(k = 0;k < u ; k++)
			printf("%-4d",i+k );
		
		printf("\ni-nodes in use : ");
		
		for(k = 0;k < u ; k++){
			printf("%-4d",usedInodesFSCK[i+k]);
		}
		
		printf("\nfree i-nodes   : ");
		
		for(k = 0;k < u ; k++){
			
			
			if(freeInodes[i+k] == 'f') 	  printf("%-4d",1);
			else 						  printf("%-4d",0);

		}	
		
		printf("\n--------------------------------------------------------------------------------------------------------------------\n");

		f -= u;
		if(f == 0){
			break;
		}
		i += 25;
		j++;
	}

	printf("\n\n\n\n>>>> blocks :\n");

	f = numOfTotalBlocks;
	i = 0;
	j = 0;


		while(1){
		
		if(f >= 25) u = 25;
		else u = f;

		printf("\n--------------------------------------------------------------------------------------------------------------------\n");
		
		printf("block ids     : ");
		
		for(k = 0;k < u ; k++)
			printf("%-4d",i+k );
		
		printf("\nblocks in use : ");
		
		for(k = 0;k < u ; k++){
			printf("%-4d",usedBlockFSCK[i+k]);
		}
		printf("\nfree blocks   : ");
		
		for(k = 0;k < u ; k++){
			
			if(freeBlocks[i+k] == 'u') 	  printf("%-4d",0);

			else  printf("%-4d",1);

		}	
		
		printf("\n--------------------------------------------------------------------------------------------------------------------\n");

		f -= u;
		if(f == 0){
			break;
		}
		i += 25;
		j++;
	}
	free(usedInodesFSCK);
	free(usedBlockFSCK);
	return;
}


void functionDumpe2fs(){

	printf("\n-Superblock\n\n\tBlock size of files (byte)          : %d\n", blockSize);
	printf("\ti-node number of fs                 : %d\n", iNodeNumber);
	printf("\tblock number of fs   	     	    : %d\n", numOfTotalBlocks);
	printf("\tNumber of blocks for inode array    : %d\n", numOfBlocksForINode);
	printf("\tNumber of blocks for superblock     : %d\n", numOfBlocksForSuperAndFSMG);
	printf("\tinitial block number of i-nodes     : %d\n", firstBlockNumOfINodes);
	printf("\tinitial block number of data blocks : %d\n", firstBlockNumOfDataBlocks);
	
	printf("\n-Free space management\n\n\tFree blocks number of fs            :  %-7d       used:  %-7d\n", freeBlockNum,numOfTotalBlocks-freeBlockNum);
 
	int i;

	int counter1 = 0;
	for(i = 0; i < iNodeNumber; i++){
		if(freeInodes[i] == 'f'){
			counter1++;
		}
	}

	printf("\tFree i-node number of fs            :  %-7d       used:  %-7d\n\n", counter1,iNodeNumber-counter1);
	
	int j,zz,k,kk,l;
	char *Str[2] ={"File","Dir"};
	char name [14] = "root";

	for(i = 0; i < iNodeNumber; i++){		
		int tepp = 0;	

		if(freeInodes[i] != 'f'){
				
			printf("-------------------------------------------------------\n");
			printf(" node id  :  %-6d   mode  :  %-6s   name  :  ",i,Str[iNodes[i].isDir]);
			
			giveNameById(i);

			printf("\ndata block ids : \n");

			for(k = 0;k < 10 ;k++){
				if(iNodes[i].blogs[k] != -2) printf(" %-5d ",iNodes[i].blogs[k] );
			}
			printf("\n");
			if(iNodes[i].blogs[10] != -2){		// single indirect link

				fseek(fp, ((firstBlockNumOfDataBlocks + iNodes[i].blogs[10]) * blockSize), SEEK_SET );
				
				int position;
					
				for(kk = 0 ; kk < blockSize/ sizeof(int);kk++){
			
					fread(&position, sizeof(int),1 , fp);
					
					if(position != -1){
						printf(" %-5d ",position );
						tepp++;
						if(tepp % 10 == 0) printf("\n");
					}
				}
			}
			tepp++;
			if(iNodes[i].blogs[11] != -2){			// double indirect link

				int position;
					
				for(kk = 0 ; kk < blockSize/ sizeof(int);kk++){

					fseek(fp, ((firstBlockNumOfDataBlocks + iNodes[i].blogs[11]) * blockSize) + (kk *sizeof(int)), SEEK_SET );
					fread(&position, sizeof(int),1 , fp);
					
					if(position != -1){
						
						printf(" %-5d ",position );

						fseek(fp, (firstBlockNumOfDataBlocks + position) * blockSize, SEEK_SET );

						int positionDouble;

						for(l = 0 ; l < blockSize/ sizeof(int);l++){
			
							fread(&positionDouble, sizeof(int),1 , fp);

							if(positionDouble != -1){
								printf(" %-5d ",positionDouble );
								tepp++;
								if(tepp % 10 == 0) printf("\n");
							}
						}

						tepp++;
						if(tepp % 10 == 0) printf("\n");
					}
				}
			}
			printf("\n\n");
		}
	}

	return ;
}

giveNameById(int id){

	int i,j,zz;

	for(i = 0; i < iNodeNumber;i++){

		if(freeInodes[i] != 'f'  && iNodes[i].isDir == true){

			for(j = 0; j < 10; j++){

				if(iNodes[i].blogs[j] != -2){

					
					if(readDirDataBlockFromDisk(iNodes[i].blogs[j]) == -1){
						printf("ERR - \n");
						return; 
					}
					
					for(zz = 0; zz < (blockSize / 16); zz++ ){

						if(currentDirBlock[zz].nodeId == id){
										
							if(strcmp(currentDirBlock[zz].fileName,".") != 0 && strcmp(currentDirBlock[zz].fileName,"..") != 0)
								printf(" %-14s ",currentDirBlock[zz].fileName );
						}
					}
				}
			}
		}
	}

}

int writeInodesToDisk(){

	fseek(fp, firstBlockNumOfINodes * blockSize, SEEK_SET );

	int i;
	for(i = 0 ; i < iNodeNumber;i++){
		
		fwrite(&(iNodes[i].isDir), sizeof(bool), 1, fp);
	
		fwrite(&(iNodes[i].size), sizeof(int), 1, fp);
		
		int j;
		for(j = 0;j <  13;j++){
			fwrite(&(iNodes[i].blogs[j]), sizeof(int), 1, fp);
		}	

		fwrite(&(iNodes[i].lastModifiedTime),sizeof(char),20,fp);
	}


	return 1;
}

int writeFreesToDisk(){

	fseek(fp, 0, SEEK_SET );
	int i;
	fwrite(&blockSize,   sizeof(int), 1, fp);
	fwrite(&iNodeNumber, sizeof(int), 1, fp);
	fwrite(&numOfTotalBlocks, sizeof(int), 1, fp);
	fwrite(&numOfBlocksForINode,   sizeof(int), 1, fp);
	fwrite(&numOfBlocksForSuperAndFSMG, sizeof(int), 1, fp);
	fwrite(&firstBlockNumOfINodes,   sizeof(int), 1, fp);
	fwrite(&firstBlockNumOfDataBlocks, sizeof(int), 1, fp);
	fwrite(&freeBlockNum, sizeof(int), 1, fp);

	for(i = 0; i < iNodeNumber;i++){
		fwrite(&(freeInodes[i]),sizeof(char),1 , fp);
	}
	for(i = 0; i < numOfTotalBlocks;i++){
		fwrite(&(freeBlocks[i]),sizeof(char),1 , fp);
	}
	return 1;
}

int checkFileNameSizeValidaty(const char * path){

	char * pathTemp  = calloc(sizeof(char),strlen(path));
	strcpy(pathTemp,path);

	char* token = strtok(pathTemp, "/");

	while (token != NULL) {
		if(strlen(token) > 14){
			printf("Err - invalid '%s' file or directory name\n",token );
			return -1;
		}
		token = strtok(NULL, "/");
	}
	free(pathTemp);


}

void printDirContent(int iNodeId,int type,int id2){

	int j,z;
	int size = 0;

	for(j = 0; j < 10;j++){
		
		if(iNodes[iNodeId].blogs[j] != -2 && iNodes[iNodeId].blogs[j] < numOfTotalBlocks){
	
			if(readDirDataBlockFromDisk(iNodes[iNodeId].blogs[j]) == -1){
				printf("ERR - \n");
				return; 
			}

			for(z = 0; z < (blockSize / 16); z++ ){
			
				if(currentDirBlock[z].nodeId != -1 && currentDirBlock[z].nodeId< iNodeNumber 
					&& currentDirBlock[z].nodeId >= 0 && freeInodes[currentDirBlock[z].nodeId] != 'f' ){
				
					if( type == 0 && 
						strcmp(currentDirBlock[z].fileName,".") != 0   &&
						strcmp(currentDirBlock[z].fileName,"..") != 0  &&
						freeInodes[currentDirBlock[z].nodeId] != 'a'
						){
		
						size += iNodes[currentDirBlock[z].nodeId].size;
						printf("%d\t%s\t%s\n",iNodes[currentDirBlock[z].nodeId].size,
							iNodes[currentDirBlock[z].nodeId].lastModifiedTime,
							currentDirBlock[z].fileName);
					}
					else if(type == 0 && 
						strcmp(currentDirBlock[z].fileName,".") != 0   &&
						strcmp(currentDirBlock[z].fileName,"..") != 0  &&
						freeInodes[currentDirBlock[z].nodeId] == 'a'
						){
		

						printf("%d\t%s\t%s -> ",iNodes[currentDirBlock[z].nodeId].size,
							iNodes[currentDirBlock[z].nodeId].lastModifiedTime,
							currentDirBlock[z].fileName);
		
						int tempsize =  iNodes[currentDirBlock[z].nodeId].size;
						int blokNum = iNodes[currentDirBlock[z].nodeId].blogs[0];

		
						fseek(fp, (firstBlockNumOfDataBlocks + blokNum) * blockSize, SEEK_SET );

						char * tempStr = calloc(sizeof(char),tempsize);
						fread(tempStr, tempsize, 1, fp);	
						printf("%s\n",tempStr );
						free(tempStr);

					}
					else if(type == 1){


						if(currentDirBlock[z].nodeId == id2){
							printf("%s\n",currentDirBlock[z].fileName );
						}
					
					}
					else if(type == 2 && currentDirBlock[z].nodeId == id2){
						

						printf("%s -> ",currentDirBlock[z].fileName );
						int tempsize =  iNodes[currentDirBlock[z].nodeId].size;
						int blokNum = iNodes[currentDirBlock[z].nodeId].blogs[0];

						fseek(fp, (firstBlockNumOfDataBlocks + blokNum) * blockSize, SEEK_SET );

						char * tempStr = calloc(sizeof(char),tempsize);
						fread(tempStr, tempsize, 1, fp);	

						printf("%s\n",tempStr );
						free(tempStr);
						return;
					}
				}

			}
		}
	}
	printf("\ntotal : %d \n",(int)(size / 1024) );
}

int removeDataBlockEntry(int parentINodeId,char * name){

	int cc,zz;
	for(cc = 0; cc < 10;cc++){
					
		if(iNodes[parentINodeId].blogs[cc] != -2){

			if(readDirDataBlockFromDisk(iNodes[parentINodeId].blogs[cc]) == -1){
				printf("ERR - \n");
				return; 
			}

			for(zz = 0; zz < (blockSize / 16); zz++ ){

				if(strcmp(currentDirBlock[zz].fileName,name) == 0){
								
					currentDirBlock[zz].nodeId = -1;
					strcpy(currentDirBlock[zz].fileName,"\0");
 	
					writeBackDirBataBLock(iNodes[parentINodeId].blogs[cc]);

					return 1;
				}
			}
		}
	}	
	return -1;	
}

int removeFileContentFromDisc(int iNodeId, int operation){

	int j,zz;

	for(j = 0; j < 10;j++){
			
		if(iNodes[iNodeId].blogs[j] != -2){
		
			if(operation == 5 || operation == 4){
				
				if(readDirDataBlockFromDisk(iNodes[iNodeId].blogs[j]) == -1){
					printf("ERR - \n");
					return; 
				}

				for(zz = 0; zz < (blockSize / 16); zz++ ){

					if(operation == 5){
						if(currentDirBlock[zz].nodeId != -1 &&
							!(strcmp(currentDirBlock[zz].fileName,".") == 0  ||
							strcmp(currentDirBlock[zz].fileName,"..") == 0)
							){						
							return -1;
						}
					}
					if(operation == 4){

					}
				}
			}

			if(operation == 6){

				signDataBlock('f',iNodes[iNodeId].blogs[j]);
				freeBlockNum++;

			}

		}
	}
	if(operation == 6 && (iNodes[iNodeId].blogs[10] != -2)){

		// single indrect erişim
		signDataBlock('f',iNodes[iNodeId].blogs[j]);    // blog 10 u free et
		freeBlockNum++;
		
		fseek(fp, (firstBlockNumOfDataBlocks + iNodes[iNodeId].blogs[10]) * blockSize, SEEK_SET );

		// blog 10 un içerisinde id leri olan data blockları free et

		int position;
		for(j = 0 ; j < blockSize / sizeof(int); j++){
			fread(&position, sizeof(int),1 , fp);
			if(position != -1){

				signDataBlock('f',position);
				freeBlockNum++;

			}
		}	
		// end of single indirect erişim
	}
	if(operation == 6 && (iNodes[iNodeId].blogs[11] != -2)){    //double indirect link
  
		int position,l,kk;

		signDataBlock('f',iNodes[iNodeId].blogs[11]);    // blog 11 u free et
		freeBlockNum++;

		for(kk = 0 ; kk < blockSize/ sizeof(int);kk++){

			fseek(fp, ((firstBlockNumOfDataBlocks + iNodes[iNodeId].blogs[11]) * blockSize) + (kk *sizeof(int)), SEEK_SET );
			fread(&position, sizeof(int),1 , fp);
			
			if(position != -1){

				signDataBlock('f',position);    
				freeBlockNum++;
				
				fseek(fp, (firstBlockNumOfDataBlocks + position) * blockSize, SEEK_SET );

				int positionDouble;

				for(l = 0 ; l < blockSize/ sizeof(int);l++){
	
					fread(&positionDouble, sizeof(int),1 , fp);

					if(positionDouble != -1){
						signDataBlock('f',positionDouble);
						freeBlockNum++;
						
					}
				}
			}
		}
	}
	return 1;
}




/*
* HER İŞİ YAPAN FONKSİYON İSVİÇRE ÇAKISI GİBİ
*/
int traverseSystem(int startingINodeId,bool dirOrNot, const char * path,int opCode,int hdVar){

	if(freeInodes[startingINodeId] == 'f')
		return -1; 

	if(iNodes[startingINodeId].isDir != dirOrNot)
		return -2;

	char * pathTemp  = calloc(sizeof(char),strlen(path));
	memset(pathTemp,0,strlen(path) *sizeof(char));
	strcpy(pathTemp,path);

	if(checkFileNameSizeValidaty(path) == -1){
		return -1;
	}
	
	char * token = strtok(pathTemp, "/");

	if(token == NULL && opCode == 0){
		printDirContent(0,0,-1);
		return 0;
	}
	if(token == NULL && opCode == 6){
		printf("del : this is root dir\n");
		return 0;
	}
	if(token == NULL && opCode == 1){
		printf("mkdir: cannot create directory ‘%s’: File exists\n",path );
		return 0;
	}
	if(token == NULL && opCode == 2){
		printf("rmdir: failed to remove ‘%s’: Directory not empty\n",path );
		return 0;
	}
	if(token == NULL && (opCode == 8 || opCode == 10)){
		printf("ln: '%s' invalid path\n",path );
		return 0;
	}
	if(token == NULL && (opCode == 4 || opCode == 5)){
		printf("write/read: '%s' invalid path\n",path );
		return 0;
	}
	char * Temp  = calloc(sizeof(char),strlen(path));
	memset(Temp,0,strlen(path) *sizeof(char));
	
	strcpy(Temp,path);

	int pId;
	int parentNodeId = startingINodeId;

	while (1) {

		int j = 0;
		int returnNodeId = checkDirEntry(startingINodeId,token, &j);
	
		strcpy(Temp,token);
		token = strtok(NULL, "/");

		//  write
			
	
		/////////////////////////   L   /////   N   //////////////////////////////////////////////////////////////////////
		if(opCode == 8 || opCode == 10){

			parentNodeId = startingINodeId;

			if(returnNodeId == -1){

				if(opCode == 8) printf("ln: failed to access ‘%s’: No such file or directory\n",path);
				else if(opCode == 10) printf("lnsym: failed to access ‘%s’: No such file or directory\n",path);
				
			}
			else if(returnNodeId != -1 && token != NULL){
		
				parentNodeId = startingINodeId;
				startingINodeId = returnNodeId;
				continue;
			}
			else if(returnNodeId != -1 && token == NULL){
				
				if(opCode == 8){
						
					hardLinkVar = returnNodeId;
					traverseSystem(0,true,writeReadFileName,111,-1);
				}
				else if(opCode == 10 ){

					pathForLnSym = calloc(sizeof(char),strlen(path));
					strcpy(pathForLnSym,path);

					traverseSystem(0,true,writeReadFileName,222,-1);

					free(pathForLnSym);	
				}
			}

			free(pathTemp);
		    free(Temp);
			return 1;
		}
		///// yardımcı operasyonlar ln ve lnsym için /////////////////////////////////////////////////////////////////////
		else if(opCode == 111 || opCode == 222){     // link yardımcı operasyonu

			if(returnNodeId == -1 && token == NULL){

				if(opCode == 111){						// hard link

					if(freeInodes[hardLinkVar] == 'u'){
					
						freeInodes[hardLinkVar] = 'h';  // hard link flag 
					}
					else if(freeInodes[hardLinkVar] != 'u' &&freeInodes[hardLinkVar] != 'a'){
					
						freeInodes[hardLinkVar]++;
					}

					int blockNumJ = 0;
					int entryNumZ = returnFreeBlockNum(parentNodeId, &blockNumJ);					
					
					if(readDirDataBlockFromDisk(iNodes[parentNodeId].blogs[blockNumJ]) == -1){
						
						printf("ERR - readDirDataBlockFromDisk\n");
						free(pathTemp);
						free(Temp);
						return; 
					}

					time( &rawtime );
					info = localtime( &rawtime );

					strftime (iNodes[parentNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);

					currentDirBlock[entryNumZ].nodeId = hardLinkVar;
													
					strcpy(currentDirBlock[entryNumZ].fileName,Temp);

					if(writeBackDirBataBLock(iNodes[parentNodeId].blogs[blockNumJ]) == -1){
						printf("ERR - \n");
					}

					hardLinkVar = -1;
				}
				else if(opCode == 222){           		// sembolik link     
			
					int blockNumJ = 0;
					int entryNumZ = returnFreeBlockNum(parentNodeId, &blockNumJ);					
					
					if(readDirDataBlockFromDisk(iNodes[parentNodeId].blogs[blockNumJ]) == -1){
						
						printf("ERR - readDirDataBlockFromDisk\n");
						free(pathTemp);
						free(Temp);
						return; 
					}

					int newNodeId = getFreeINodeid();	
			
					currentDirBlock[entryNumZ].nodeId = newNodeId;

					signFreeNodes('a',newNodeId); 

					strcpy(currentDirBlock[entryNumZ].fileName,Temp);


					if(writeBackDirBataBLock(iNodes[parentNodeId].blogs[blockNumJ]) == -1){
						printf("ERR - \n");
					}

					time( &rawtime );
					info = localtime( &rawtime );

					strftime (iNodes[parentNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);
					strftime (iNodes[newNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);
				
					iNodes[newNodeId].isDir = false;
					iNodes[newNodeId].size = strlen(pathForLnSym);

					int newDataBlockId = getFreeDataBlockID();
					if(newDataBlockId == -1){
						printf("ERR - SPACE/DISK FULL\n");
						break;
					}

					signDataBlock('u',newDataBlockId);
					freeBlockNum --;
														
					iNodes[newNodeId].blogs[0] = newDataBlockId;	
					
					int jj;
					for(jj = 1; jj < 13 ; jj++){
						iNodes[newNodeId].blogs[jj] = -2;
					} 

					fseek(fp, (firstBlockNumOfDataBlocks + newDataBlockId) * blockSize, SEEK_SET );						
				
					fwrite(pathForLnSym,strlen(pathForLnSym),1,fp);

				}

			}
			else if(returnNodeId == -1 && token != NULL){
				if(opCode == 111) 	   printf("ln: '%s' invalid path\n",path);
				else if(opCode == 222) printf("lnsym: '%s' invalid path\n",path);
			}
			else if(returnNodeId != -1 && token != NULL){

				parentNodeId = returnNodeId;
				startingINodeId = returnNodeId;
				continue;
			}
			else if(returnNodeId != -1 && token == NULL){
				if(opCode == 111) 	   printf("ln: '%s' file alreadt exist\n",path);
				else if(opCode == 222) printf("lnsym: '%s' file alreadt exist\n",path);
			}

			free(pathTemp);
			free(Temp);
				
			return 1;
		}

		/////////////////////////   D   /////   E   ////   L   /////////////////////////////////////////////////////////// *
		else if(opCode == 6){

			parentNodeId = startingINodeId;

			if(returnNodeId == -1){

				if(hdVar == 555)
					return -2;
				
				printf("del: '%s' no such file or directory \n",path);
			

			}
			else if(returnNodeId != -1 && token != NULL){
		
				parentNodeId = startingINodeId;
				startingINodeId = returnNodeId;
				continue;
			}
			else if(returnNodeId != -1 && token == NULL){

				if(hdVar == 555){ /// semblolik link
					free(pathTemp);
		    		free(Temp);
					return 1;
				}
				
				if(iNodes[returnNodeId].isDir == true){
					printf("del - '%s' is not a file name. It is a directory\n",path);
					return -2;
				}

				if(hdVar == 1){					/// hard link

					passChar = freeInodes[returnNodeId];
					passNoedeId = returnNodeId;

					time( &rawtime );
					info = localtime( &rawtime );

					strftime (iNodes[parentNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);

					removeFileContentFromDisc(returnNodeId,6);
						
					if(removeDataBlockEntry(parentNodeId,/*returnNodeId*/Temp) == -1){ // directory bilgiler parent directory den silinir.
						
						printf("ERR - in remove from disc\n");
						free(pathTemp);
			    		free(Temp);
						return -1;
					}
										
					signFreeNodes('f',returnNodeId);

				}
		
				else if(freeInodes[returnNodeId] == 'u') { // eğer bu dosya linkli değilse
					
					time( &rawtime );
					info = localtime( &rawtime );

					strftime (iNodes[parentNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);

					removeFileContentFromDisc(returnNodeId,6);
						
					if(removeDataBlockEntry(parentNodeId,/*returnNodeId*/Temp) == -1){ // directory bilgiler parent directory den silinir.
						
						printf("ERR - in remove from disc\n");
						free(pathTemp);
			    		free(Temp);
						return -1;
					}
										
					signFreeNodes('f',returnNodeId);
				
				}
				else if(freeInodes[returnNodeId] >= 'h'){
				
					if(freeInodes[returnNodeId] == 'h'){
						freeInodes[returnNodeId] = 'u';
					}
					else if (freeInodes[returnNodeId] != 'h' && freeInodes[returnNodeId] != 'a'){
						freeInodes[returnNodeId]--;	
					}

					time( &rawtime );
					info = localtime( &rawtime );

					strftime (iNodes[parentNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);

					if(removeDataBlockEntry(parentNodeId,Temp) == -1){ // directory bilgiler parent directory den silinir.
						
						printf("ERR - in remove from disc\n");
						free(pathTemp);
			    		free(Temp);
						return -1;
					}

				}
				else if(freeInodes[returnNodeId] == 'a'){ // silinecek dosya sembolic linkli ise 

					
					signDataBlock('f',iNodes[returnNodeId].blogs[0]);
					freeBlockNum++;

					signFreeNodes('f',returnNodeId);

					time( &rawtime );
					info = localtime( &rawtime );

					strftime (iNodes[parentNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);


					if(removeDataBlockEntry(parentNodeId,Temp) == -1){ // directory bilgiler parent directory den silinir.
						
						printf("ERR - in remove from disc\n");
						free(pathTemp);
			    		free(Temp);
						return -1;
					}
				}
			}

			free(pathTemp);
		    free(Temp);

		    return 1;

		}

        /////////////////////////   R   /////   M   ////   D   ////   I   ////   R   /////////////////////////////////////////
		else if(opCode == 2){

			parentNodeId = startingINodeId;

			if(returnNodeId == -1){

				printf("rmdir: '%s' no such file or directory\n",path);
				
			}
			else if(returnNodeId != -1 && token != NULL){

			//	printf("RECURSİVE call : %d  : %s\n",parentNodeId,token);
				
				parentNodeId = startingINodeId;
				startingINodeId = returnNodeId;
				continue;
			}
			else if(returnNodeId != -1 && token == NULL){
			
				if(iNodes[returnNodeId].isDir != true){
					printf("rmdir: '%s' is not a directory\n",path);
					return -1;
				}
				
				/////
				time( &rawtime );
				info = localtime( &rawtime );

				strftime (iNodes[parentNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);
				///
				if(removeFileContentFromDisc(returnNodeId,5) == -1){

					printf("rmdir: failed to remove ‘%s’: Directory not empty \n",path );
					free(pathTemp);
					free(Temp);
					return -1;

				}		

				if(removeDataBlockEntry(parentNodeId,Temp) == -1){ // directory bilgiler parent directory den silinir.
					printf("rmdir: '%s' remove from disc\n",path);
					free(pathTemp);
		    		free(Temp);
					return -1;
				}
			
				signDataBlock('f',iNodes[returnNodeId].blogs[0]);
				freeBlockNum++;
				signFreeNodes('f',returnNodeId);

			}
			free(pathTemp);
		    free(Temp);

		    return 1;
		}

		/////////////////////////   R   /////   E   ////   A   ////   D   ////////////////////////////////////////////////////
		else if(opCode == 5){     

			parentNodeId = startingINodeId;

			if(returnNodeId == -1){

				printf("read: '%s' no such file or directory\n",path);
				
			}
			else if(returnNodeId != -1 && token != NULL){
		
				parentNodeId = startingINodeId;
				startingINodeId = returnNodeId;
				continue;
			}
			else if(returnNodeId != -1 && token == NULL){
			
			
				if(freeInodes[returnNodeId] == 'a'){		// eğer dosya sembolik linkli ise

					int tempsize = iNodes[returnNodeId].size; 
					int blokNum = iNodes[returnNodeId].blogs[0];

					fseek(fp, (firstBlockNumOfDataBlocks + blokNum) * blockSize, SEEK_SET );

					char * tempStr = calloc(sizeof(char),tempsize);
					fread(tempStr, tempsize, 1, fp);	
								
					traverseSystem(0,true,tempStr,5,-1);

					free(tempStr);
					return 1;

				}
				if(iNodes[returnNodeId].isDir == true){				    // klasör adı girilirse
					printf("read - '%s' is directory name not a file\n",path );
					return -1;
				}
				int fd1 = open(writeReadFileName,O_WRONLY | O_CREAT  | O_EXCL,0777);

				if(fd1 == -1){
					printf("read: '%s' this files already exist\n", writeReadFileName);
					close(fd1);
					return 1;
				}

				int fileSize = iNodes[returnNodeId].size;
				int stepNumber = fileSize / blockSize;
				int lastSizes = fileSize - (stepNumber * blockSize);

				char * MYStr = calloc(blockSize , sizeof(char));
				bzero(MYStr,sizeof(char) *blockSize);
			
				int gg;
				int position = 0;
				int counter2 = 0;

				for(gg = 0 ; gg < 13;gg++){
					
					if(iNodes[returnNodeId].blogs[gg] != -2 ){          // direct block kullanılıyorsa

						if(gg < 10){

							fseek(fp, (firstBlockNumOfDataBlocks + iNodes[returnNodeId].blogs[gg]) * blockSize, SEEK_SET );
							if(stepNumber > 0){
								fread(MYStr, blockSize,1 , fp);
								write(fd1, MYStr, blockSize); 
								stepNumber --;	
							}
							else{
								fread(MYStr, lastSizes,1 , fp);
								write(fd1, MYStr, lastSizes); 
								break;
							}
							counter2++;
						}
						else if(gg == 10){							 	// single indirect kullanım
							
							int kk;
							for(kk = 0 ; kk < blockSize/ sizeof(int);kk++){

								fseek(fp, ((firstBlockNumOfDataBlocks + iNodes[returnNodeId].blogs[10]) * blockSize) + (kk *sizeof(int)), SEEK_SET );

								fread(&position, sizeof(int),1 , fp);
								
								if(position != -1){

									fseek(fp, (firstBlockNumOfDataBlocks + position) * blockSize, SEEK_SET );

									if(stepNumber > 0){
										fread(MYStr, blockSize,1 , fp);
										write(fd1, MYStr, blockSize); 
										stepNumber --;	
									}
									else{
										fread(MYStr, lastSizes,1 , fp);
										write(fd1, MYStr, lastSizes); 
										break;
									}
								}
							}
						}
						else if(gg == 11){								// double indirect kullanım

							int positiond1;
							int tepp = 0;		
							int kk,l;
							for(kk = 0 ; kk < blockSize/ sizeof(int);kk++){

								fseek(fp, ((firstBlockNumOfDataBlocks + iNodes[returnNodeId].blogs[11]) * blockSize) + (kk *sizeof(int)), SEEK_SET );
								fread(&positiond1, sizeof(int),1 , fp);
								
								if(positiond1 != -1){
													
									int positionDouble;

									for(l = 0 ; l < blockSize/sizeof(int);l++){
										
										fseek(fp, (firstBlockNumOfDataBlocks + positiond1) * blockSize + (l * sizeof(int)), SEEK_SET );
										fread(&positionDouble, sizeof(int),1 , fp);

										if(positionDouble != -1){
																				
											fseek(fp, (firstBlockNumOfDataBlocks + positionDouble) * blockSize, SEEK_SET );

											if(stepNumber > 0){
												fread(MYStr, blockSize,1 , fp);
												write(fd1, MYStr, blockSize); 
												stepNumber --;	
											}
											else{
												fread(MYStr, lastSizes,1 , fp);
												write(fd1, MYStr, lastSizes); 
												break;
											}
										}
									}
								}
							}
						}
					}
				}
		
				free(MYStr);
				close(fd1);
			}
			free(pathTemp);
		    free(Temp);
			return 1;
		}

		/////////////////////////   W   /////   R   ////   I   ////   T   ////   E   /////////////////////////////////////////
		else if(opCode == 4){  		
		
			if(returnNodeId == -1 && token == NULL){

		
				int fd1 = open(writeReadFileName,O_RDONLY,0777);
			
				if(fd1 == -1){
					printf("ERR - opening file : %s\n", writeReadFileName);
					free(pathTemp);
					free(Temp);
					close(fd1);
					return -1;
				}

				off_t fsize;

				fsize = lseek(fd1, 0, SEEK_END);
			
				lseek(fd1, 0, SEEK_SET);

				if(checkSizeIsValid((int)fsize) < 0){

					printf("write : there is no space for '%s' in disc\n",path );
					free(pathTemp);
					free(Temp);
					close(fd1);
					return -1;
				
				}

				int blockNumJ = 0;
				int entryNumZ = returnFreeBlockNum(parentNodeId, &blockNumJ);					
				
				if(readDirDataBlockFromDisk(iNodes[parentNodeId].blogs[blockNumJ]) == -1){
					printf("ERR - \n");
					free(pathTemp);
					free(Temp);
					return -1; 
				}

				int newNodeId;
				if(hdVar == 22){
					newNodeId = passNoedeId;
					signFreeNodes(passChar,newNodeId);
				}
				else{ 
					newNodeId = getFreeINodeid();	
					signFreeNodes('u',newNodeId);
				}
				currentDirBlock[entryNumZ].nodeId = newNodeId;
					
				strcpy(currentDirBlock[entryNumZ].fileName,Temp);

				if(writeBackDirBataBLock(iNodes[parentNodeId].blogs[blockNumJ]) == -1){
					printf("ERR - \n");
				}
				int c = 0;
				for( ; c < 13;c++){
					iNodes[newNodeId].blogs[c] = -2;						
				}

				int readd = 0 ,writed = 0;
				char * MYStr = calloc(blockSize , sizeof(char));
			
				bzero(MYStr,sizeof(char) *blockSize);

				int sizeOfFile = 0;
				iNodes[newNodeId].isDir = false;
				time( &rawtime );
				info = localtime( &rawtime );

				strftime (iNodes[newNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);
				strftime (iNodes[parentNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);
		
				int k = 0;
				int blockCounter = 0;
				
				while((readd = read(fd1,MYStr,blockSize)) > 0){
					
				
					int newDataBlockId = getFreeDataBlockID();
					if(newDataBlockId == -1){
						printf("ERR - SPACE/DISK FULL\n");
						break;
					}
					sizeOfFile +=readd;
					signDataBlock('u',newDataBlockId);
					freeBlockNum --;
																
					if(k < 10){											// direct erişim

						fseek(fp, (firstBlockNumOfDataBlocks + newDataBlockId) * blockSize, SEEK_SET );						
						iNodes[newNodeId].blogs[blockCounter] = newDataBlockId;	
						blockCounter++;								    //direct links
						fwrite(MYStr,readd,1,fp);
							
					}
					else if(k == 10 ){									// single indirect initial etme 
						
						iNodes[newNodeId].blogs[10] = newDataBlockId;	
						
 						int newBlockId = indirectWrite1(newDataBlockId);
						if(newBlockId == -1 ){

							printf("ERR - SPACE/DISK single FULL\n");
							free(pathTemp);
							free(Temp);
							close(fd1);
							free(MYStr);
							return -1;

						}
												
						fseek(fp, (firstBlockNumOfDataBlocks + newBlockId) * blockSize, SEEK_SET );

						fwrite(MYStr,readd,1,fp);

					}
					else if(k > 10 && k < ((blockSize / sizeof(int) + 10)) ){    // single indirect erişim

						fseek(fp,( (firstBlockNumOfDataBlocks + iNodes[newNodeId].blogs[10]) * blockSize) + (sizeof(int) * (k - 10 )), SEEK_SET );
						
						fwrite(&newDataBlockId,sizeof(int),1,fp);

						fseek(fp, (firstBlockNumOfDataBlocks + newDataBlockId) * blockSize, SEEK_SET );

						fwrite(MYStr,readd,1,fp);
					}
					else if(k == (blockSize / sizeof(int)) + 10 ){    // double indirect link initial etme

						iNodes[newNodeId].blogs[11] = newDataBlockId;					

						int newBlockId = indirectWrite1(newDataBlockId);
						if(newBlockId == -1 ){

							printf("ERR - SPACE/DISK FULL double 1\n");
							free(pathTemp);
							free(Temp);
							free(MYStr);
							close(fd1);
							return -1;

						}
						int newBlockId2 = indirectWrite1(newBlockId);
						if(newBlockId2 == -1 ){

							printf("ERR - SPACE/DISK FULL double 2\n");
							free(pathTemp);
							free(Temp);
							close(fd1);
							free(MYStr);
							return -1;

						}
						fseek(fp, (firstBlockNumOfDataBlocks + newBlockId2) * blockSize, SEEK_SET );
						
						fwrite(MYStr,readd,1,fp);

									
					}
																			 // double indirect link erişim
					else if (k > ((blockSize / sizeof(int)) + 10 ) &&  ((k - 10)  < ((blockSize / sizeof(int)) * (blockSize / sizeof(int))))){ 

						
						int jumpNum = ((firstBlockNumOfDataBlocks +  iNodes[newNodeId].blogs[11]) * blockSize) + 
							(((int)(k - ( (blockSize / sizeof(int)) + 10) ) / (blockSize / sizeof(int) )) * sizeof(int));
						
						fseek(fp, jumpNum, SEEK_SET );
						
						int pos;
						
						fread(&pos,sizeof(int),1,fp);

						if(pos == -1){
							
							fseek(fp, jumpNum, SEEK_SET );

							int newBlockId = getFreeDataBlockID();
							if(newBlockId == -1){
								return -1;
							}
							signDataBlock('u',newBlockId);
							freeBlockNum --;

							fwrite(&newBlockId,sizeof(int),1,fp);
							
							fseek(fp,( (firstBlockNumOfDataBlocks + newBlockId) * blockSize), SEEK_SET );							
							int c;
							int eksi1 = -1;
							
							for(c = 1; c < blockSize/sizeof(int);c++){
								
								fwrite(&eksi1,sizeof(int),1,fp);
							}
							fseek(fp, jumpNum, SEEK_SET );
							fread(&pos,sizeof(int),1,fp);
						}
						// blok alınmış

						fseek(fp,( (firstBlockNumOfDataBlocks + pos) * blockSize), SEEK_SET );

						int dd,posDD;
						for(dd = 0 ; dd < blockSize / sizeof(int); dd++){
							fread(&posDD,sizeof(int),1,fp);
							if(posDD == -1){
								break;
							}
						}

						fseek(fp,(((firstBlockNumOfDataBlocks + pos) * blockSize) + (sizeof(int) * dd)), SEEK_SET );
							
						fwrite(&newDataBlockId,sizeof(int),1,fp);
					
						fseek(fp, (firstBlockNumOfDataBlocks + newDataBlockId) * blockSize, SEEK_SET );

						fwrite(MYStr,readd,1,fp);

					}
									
					bzero(MYStr,sizeof(char) * blockSize);
			
					k++;
				}
				int ik;
				for( ik = k; ik < 10;ik++){
					iNodes[newNodeId].blogs[ik] = -2;						
				}
	
				iNodes[newNodeId].size = sizeOfFile;			
				
				free(MYStr);
				close(fd1);
			}
			else if(returnNodeId == -1 && token != NULL){
				printf("write: '%s' invalid path \n",path);
			}
			else if(returnNodeId != -1 && token != NULL){

				parentNodeId = returnNodeId;
				startingINodeId = returnNodeId;
				continue;
			}
			else if(returnNodeId != -1 && token == NULL){		// eğer dosya eskiden de bulunuyorsa eski dosya silinir yenisi yazılır
				
				

				if(freeInodes[returnNodeId] == 'a'){		// eğer dosya sembolik linkli ise
				
					int tempsize = iNodes[returnNodeId].size; 
					int blokNum = iNodes[returnNodeId].blogs[0];

					fseek(fp, (firstBlockNumOfDataBlocks + blokNum) * blockSize, SEEK_SET );

					char * tempStr = calloc(sizeof(char),tempsize);
					fread(tempStr, tempsize, 1, fp);	
					
					int linkIsBroken = traverseSystem(0,true,tempStr,6,555);
					if(linkIsBroken == -2){
						printf("lnsym : '%s' Broken Link\n",tempStr );
					}
					else{
						traverseSystem(0,true,tempStr,6,1);      // eski halini halini sil
					
						traverseSystem(0,true,tempStr,4,22);		// yeni hali yazılır
					}

					
					free(tempStr);
					free(Temp);
					free(pathTemp);
					return 1;

				}else{

				traverseSystem(0,true,path,6,1);      // eski halini halini sil
					
				traverseSystem(0,true,path,4,22);		// yeni hali yazılır
				}
			}

			free(pathTemp);
			free(Temp);
				
			return 1;
		}

		/////////////////////////   L   /////   I   ////   S   ////   T   ///////////////////////////////////////////////////
		else if (opCode == 0){ 			

			parentNodeId = startingINodeId;

			if(returnNodeId == -1 ){
				printf("ls: cannot access '%s': No such file or directory\n",path);
			}
			else if(returnNodeId != -1 && token != NULL){
		
				parentNodeId = startingINodeId;
				startingINodeId = returnNodeId;
				continue;
			}
			else if(returnNodeId != -1 && token == NULL){  // path dosya ise
		
				if(iNodes[returnNodeId].isDir != true){


					if(freeInodes[returnNodeId] == 'a'){

						printDirContent(parentNodeId,2,returnNodeId);
						
					}
					else{
						
						printDirContent(parentNodeId,1,returnNodeId);
					}
				}
				else 
					printDirContent(returnNodeId,0,-1);		 // path dir ise
			}

			free(pathTemp);
			free(Temp);
			
			return 1;
			
		}
		/////////////////////////   M   /////   K   ////   D   ////   I   ////   R   /////////////////////////////////////////
		else if(opCode == 1){					

			if(returnNodeId == -1 && token == NULL){

				if(strcmp(Temp,".") == 0 || strcmp(Temp,"..") == 0){
					printf("mkdir: cannot create directory ‘%s’: invalid directory name\n",path );
					free(pathTemp);
					free(Temp);
					return 1;	

				}
				
				int blockNumJ = 0;
				int entryNumZ = returnFreeBlockNum(parentNodeId, &blockNumJ);	
				if(entryNumZ == -1){
					printf("Err - this directory full, total number of entry %d\n",(blockSize/ 16) * 10 );
					return -1;
				}				
				
				if(readDirDataBlockFromDisk(iNodes[parentNodeId].blogs[blockNumJ]) == -1){
					printf("ERR - \n");
					free(pathTemp);
					free(Temp);
					return; 
				}

				iNodes[parentNodeId].size = (blockNumJ + 1) * blockSize;

				int newNodeId = getFreeINodeid();
	
				currentDirBlock[entryNumZ].nodeId = newNodeId;
					
				signFreeNodes('u',newNodeId);
			
				strcpy(currentDirBlock[entryNumZ].fileName,Temp);
				
				time( &rawtime );
				info = localtime( &rawtime );

				strftime (iNodes[parentNodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);
				
				if(writeBackDirBataBLock(iNodes[parentNodeId].blogs[blockNumJ]) == -1){
					printf("ERR - \n");
				}

				int newDataBlockId = getFreeDataBlockID();
			
				signDataBlock('u',newDataBlockId);

				freeBlockNum --;

				initializeINode(newNodeId,newDataBlockId,blockNumJ);		
							
				initializeDirDataBlock(newDataBlockId,newNodeId,parentNodeId);	

				free(pathTemp);
				free(Temp);
				return 1;
			}
			else if(returnNodeId == -1 && token != NULL){
				printf("mkdir: cannot create directory ‘%s’: No such file or directory\n",path);
				free(pathTemp);
				free(Temp);
				return 1;
			}
			else if(returnNodeId != -1 && token == NULL){
				printf("mkdir: cannot create directory ‘%s’: File exists\n",path );
				free(pathTemp);
				free(Temp);
				return 1;
			
			}
			else if(returnNodeId != -1 && token != NULL){

				parentNodeId = returnNodeId;
				startingINodeId = returnNodeId;
				continue;
			}
		}

	}
}


int checkSizeIsValid(int sizeOfFile){

	int tempFBN = freeBlockNum;
	int i;

	for(i = 0 ; i < 10 ;i++){				    // direct

		sizeOfFile -= blockSize;

		tempFBN--;

		if(sizeOfFile <= 0 && tempFBN >= 0)
			return 1;
		

		if(tempFBN == 0 && sizeOfFile > 0 )
			return -1;

	}

	tempFBN --;
	for(i = 0 ; i < blockSize / sizeof(int);i++){ // single indirect

		sizeOfFile -= blockSize;

		tempFBN--;

		if(sizeOfFile <= 0 && tempFBN >= 0)
			return 1;

		if(tempFBN == 0 && sizeOfFile > 0 )
			return -1;

	}
	tempFBN --;
	int j;
	for(i = 0 ; i < blockSize / sizeof(int);i++){ //double indirect

		tempFBN --;
		for(j = 0 ; j < blockSize / sizeof(int);j++){
			tempFBN --;
			sizeOfFile -= blockSize;
			
			if(sizeOfFile <= 0 && tempFBN >= 0)
				return 1;

			if(tempFBN == 0 && sizeOfFile > 0 )
				return -1;
		}
	
	}

 	return -2;
}

int indirectWrite1(int newDataBlockId){

	fseek(fp, (firstBlockNumOfDataBlocks + newDataBlockId) * blockSize, SEEK_SET );

	int newBlockId = getFreeDataBlockID();
	if(newBlockId == -1){
		return -1;
	}
	signDataBlock('u',newBlockId);
	freeBlockNum --;

	fwrite(&newBlockId,sizeof(int),1,fp);
	
	int c;
	int eksi1 = -1;
	
	for(c = 1; c < blockSize/sizeof(int);c++){
		
		fwrite(&eksi1,sizeof(int),1,fp);
	}
	return newBlockId;
}

int returnFreeBlockNum(int iNodeId, int *returnJ){

	int j,z;

	for(j = 0; j < 10;j++){
		
		if(iNodes[iNodeId].blogs[j] != -2){
		
			if(readDirDataBlockFromDisk(iNodes[iNodeId].blogs[j]) == -1){
				printf("ERR - \n");
				return; 
			}

			for(z = 0; z < (blockSize / 16); z++ ){

				if(currentDirBlock[z].nodeId == -1 ){
						*returnJ = j;
						return z;  // files i node id
				}
			}
		}
		else{		// dir in blokunda de yeni directory eklenecek yer kalmamaıs durumu

			int newDataBlockId = getFreeDataBlockID();

			iNodes[iNodeId].blogs[j] = newDataBlockId;

			signDataBlock('u',newDataBlockId);

			freeBlockNum --;

			*returnJ = j;

			return 0;
		}
	}		

	return -1;   
}

int checkDirEntry(int iNodeId,char * fileName, int *returnJ){

	int j,z;

	for(j = 0; j < 10;j++){
		
		if(iNodes[iNodeId].blogs[j] != -2){

			if(readDirDataBlockFromDisk(iNodes[iNodeId].blogs[j]) == -1){
				printf("ERR - \n");
				return; 
			}

			for(z = 0; z < (blockSize / 16); z++ ){

				if(strcmp(currentDirBlock[z].fileName,fileName) == 0 ){
						*returnJ = j;
						return currentDirBlock[z].nodeId;  // files i node id
				}
			}
		}
	}		

	return -1;   // 
}

int writeBackDirBataBLock(int dataBlockNumber){

	if(dataBlockNumber >= numOfTotalBlocks){
		return -1; 
	}
	fseek(fp, (firstBlockNumOfDataBlocks + dataBlockNumber) * blockSize, SEEK_SET );

	int i;
	
	for(i = 0 ; i < blockSize / 16 ; i++){
		
		fwrite(&(currentDirBlock[i]), sizeof(struct dataBlockEntry), 1, fp);		
	}

	return 1;
}

int readDirDataBlockFromDisk(int dataBlockNumber){

	if(dataBlockNumber >= numOfTotalBlocks){
		return -1; 
	}

	fseek(fp, (firstBlockNumOfDataBlocks + dataBlockNumber) * blockSize, SEEK_SET );

	int i;
	
	for(i = 0 ; i < blockSize / 16 ; i++){
		
		fread(&(currentDirBlock[i]), sizeof(struct dataBlockEntry), 1, fp);	
	}

	return 1;
}

void initializeDirDataBlock(int dataBlockId,int currentNodeId,int parentNodeId){

	readDirDataBlockFromDisk(dataBlockId);

	currentDirBlock[0].nodeId = parentNodeId;
	strcpy(currentDirBlock[0].fileName,"..");

	currentDirBlock[1].nodeId = currentNodeId;
	strcpy(currentDirBlock[1].fileName,".");

	int i;
 	 	
 	for(i = 2 ; i < (blockSize / 16) ; i++){

		currentDirBlock[i].nodeId = -1;
		strcpy(currentDirBlock[i].fileName,"\0");
 	}

	writeBackDirBataBLock(dataBlockId);

}

void initializeINode(int nodeId,int dataBlockNumber,int blogNum){

	iNodes[nodeId].isDir = true;
	iNodes[nodeId].size = blockSize;
	int i;

	iNodes[nodeId].blogs[blogNum] = dataBlockNumber;

	
 	for( i = 1 ; i < 13;i++){
 		iNodes[nodeId].blogs[i] = -2;
 	}

	time( &rawtime );
	info = localtime( &rawtime );

	strftime (iNodes[nodeId].lastModifiedTime, sizeof(char) * 20, "%B %d %I:%M", info);

	return;
}

int getFreeDataBlockID(){

	int i;
	for(i = 0; i < numOfTotalBlocks; i++){
		if(freeBlocks[i] == 'f'){
			return i;
		}
	}
	return -1;

}

/*
* -1 bütün i nodelar dolu ise
*/
int getFreeINodeid(){
	
	int i;
	for(i = 0; i < iNodeNumber; i++){
		if(freeInodes[i] == 'f'){
			return i;
		}
	}
	return -1;
}

int signFreeNodes(char sign,int freeNodeId){

	int i;
	if(sign =='f'){
		iNodes[freeNodeId].size = -1;
		for(i = 0;i < 13;i++){
			iNodes[freeNodeId].blogs[i] = -2;
		}
	}

	if(freeInodes[freeNodeId] != sign){
		freeInodes[freeNodeId] = sign;
		return 1;
	}
	return -1;
}

int signDataBlock(char sign,int dataBlockId){

	if(freeBlocks[dataBlockId] != sign){
		freeBlocks[dataBlockId] = sign;
		return 1;
	}
	return -1;

}