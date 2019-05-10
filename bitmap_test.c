#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"



int main(int agc, char** argv) { 					//A. Main di prova per testare le funzioni di bitmap.c 
	printf("--------------- 1. TEST BITMAP.C --------------------\n\n");
  
	BitMap bitmap1;
	char bits1[8] = "00000001";
	bitmap1.entries = bits1;
	bitmap1.num_bits = 64;
	
	BitMap_print(&bitmap1);
  
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
	
	printf("\n--------------- 1.3 TEST BITMAP GET -------\n");
	
	int index_to_get1 = BitMap_get(&bitmap1,0,0);
	printf("Starts Looking from position:%i, bit with status:%i\n", 0, 0);
	printf("index got:%i\n", index_to_get1);
	int index_to_get2 = BitMap_get(&bitmap1,5,0);
	printf("Starts Looking from position:%i, bit with status:%i\n", 5, 0);
	printf("index got:%i\n", index_to_get2);
	
	printf("\n--------------- 1.4 TEST BITMAP SET -------\n");
	BitMap_set(&bitmap1,5,0);
	printf("Setting bit at pos:%i with status:%i\n", 5, 0);
	int index_to_get3 = BitMap_get(&bitmap1,5,0);
	printf("Starts Looking from position:%i, bit with status:%i\n", 5, 0);
	printf("index got:%i\n", index_to_get3);
	
	printf("\n--------------- 1.5 TEST BITMAP IS FREE BLOCK -------\n");
	
	int res1;
	res1 = BitMap_is_free_block(&bitmap1,5);
	printf("res1:%i\n", res1); 
}
