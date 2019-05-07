#include "simplefs.h"
#include "bitmap.h"
#include <stdio.h>

int main(int agc, char** argv) { 						//A. Main di prova per testare le funzioni di bitmap.c e diskdriver.c
	//printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
	//printf("DataBlock size %ld\n", sizeof(FileBlock));
	//printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
	//printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));
  
	printf("--------------- 1. TEST BITMAP.C --------------------\n\n");
  
	printf("--------------- 1.1 TEST BITMAP BLOCK TO INDEX -------\n");
  
	int i;
  
	for(i=0; i<32; i++){
		BitMapEntryKey key1 = BitMap_blockToIndex(i);
		printf("Block:%i -> Byte:%i - Bit:%i\n",i, key1.entry_num, key1.bit_num);
	}
  
	printf("\n--------------- 1.2 TEST BITMAP INDEX TO BLOCK -------\n");
	
	int j;
	for(i=0; i<3; i++){
		for(j=0; j<3; j++){
			int block = BitMap_indexToBlock(i,j);
			printf("Byte:%i - Bit:%i -> Block:%i\n", i, j, block);
		}
	}
}
