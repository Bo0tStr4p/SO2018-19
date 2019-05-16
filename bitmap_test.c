#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"



int main(int agc, char** argv) { 					//A. Main di prova per testare le funzioni di bitmap.c 
	
	printf("\n****************************\n");
	printf("1. TEST BITMAP.C\n");
	printf("****************************\n\n");
	printf("\n*****************************\n");
	printf("1.1 Test BitMap struct\n");
	printf("*****************************\n\n");
  
	printf("This is BitMap bitmap1:\n\n");
  
	BitMap bitmap1;
	char bits1[8] = "00000001";
	bitmap1.entries = bits1;
	bitmap1.num_bits = 64;
	BitMap_print(&bitmap1);
	
	printf("\nThis is BitMap bitmap2:\n\n");
	
	BitMap bitmap2;
	char bits2[8] = "10011001";
	bitmap2.entries = bits2;
	bitmap2.num_bits = 64;
	BitMap_print(&bitmap2);
	
	printf("\n*****************************\n");
	printf("1.2 Test BitMap_is_free_block\n");
	printf("*****************************\n\n");
	
	
	int status10;
	status10 = BitMap_is_free_block(&bitmap1,0);
	printf("bitmap1 - Block:0 - Status:%i | Expected: 0 (Free)\n", status10);
	int status15;
	status15 = BitMap_is_free_block(&bitmap1,5);
	printf("bitmap1 - Block:5 - Status:%i | Expected: -1 (Full)\n", status15);
	int status19;
	status19 = BitMap_is_free_block(&bitmap1,9);
	printf("bitmap1 - Block:9 - Status:%i | Expected: 0 (Free)\n", status19);
	int status117;
	status117 = BitMap_is_free_block(&bitmap1,17);
	printf("bitmap1 - Block:17 - Status:%i | Expected: 0 (Free)\n", status117);
	int status121;
	status121 = BitMap_is_free_block(&bitmap1,21);
	printf("bitmap1 - Block:21 - Status:%i | Expected: -1 (Full)\n", status121);
	int status132;
	status132 = BitMap_is_free_block(&bitmap1,32);
	printf("bitmap1 - Block:32 - Status:%i | Expected: 0 (Free)\n", status132);
	int status145;
	status145 = BitMap_is_free_block(&bitmap1,45);
	printf("bitmap1 - Block:45 - Status:%i | Expected: -1 (Full)\n", status145);
	int status152;
	status152 = BitMap_is_free_block(&bitmap1,52);
	printf("bitmap1 - Block:52 - Status:%i | Expected: -1 (Full)\n", status152);
	int status159;
	status159 = BitMap_is_free_block(&bitmap1,59);
	printf("bitmap1 - Block:59 - Status:%i | Expected: 0 (Free)\n", status159);
	int status162;
	status162 = BitMap_is_free_block(&bitmap1,62);
	printf("bitmap1 - Block:62 - Status:%i | Expected: 0 (Free)\n", status162);
	
	printf("\n");
	
	int status20;
	status20 = BitMap_is_free_block(&bitmap2,0);
	printf("bitmap2 - Block:0 - Status:%i | Expected: -1 (Full)\n", status20);
	int status25;
	status25 = BitMap_is_free_block(&bitmap2,5);
	printf("bitmap2 - Block:5 - Status:%i | Expected: -1 (Full)\n", status25);
	int status29;
	status29 = BitMap_is_free_block(&bitmap2,9);
	printf("bitmap2 - Block:9 - Status:%i | Expected: 0 (Free)\n", status29);
	int status217;
	status217 = BitMap_is_free_block(&bitmap2,17);
	printf("bitmap2 - Block:17 - Status:%i | Expected: 0 (Free)\n", status217);
	int status221;
	status221 = BitMap_is_free_block(&bitmap2,21);
	printf("bitmap2 - Block:21 - Status:%i | Expected: -1 (Full)\n", status221);
	int status232;
	status232 = BitMap_is_free_block(&bitmap2,32);
	printf("bitmap2 - Block:39 - Status:%i | Expected: -1(Full)\n", status232);
	int status245;
	status245 = BitMap_is_free_block(&bitmap2,45);
	printf("bitmap2 - Block:45 - Status:%i | Expected: -1 (Full)\n", status245);
	int status252;
	status252 = BitMap_is_free_block(&bitmap2,52);
	printf("bitmap2 - Block:52 - Status:%i | Expected: -1 (Full)\n", status252);
	int status259;
	status259 = BitMap_is_free_block(&bitmap2,59);
	printf("bitmap2 - Block:59 - Status:%i | Expected: 0 (Free)\n", status259);
	int status262;
	status262 = BitMap_is_free_block(&bitmap2,62);
	printf("bitmap2 - Block62 - Status:%i | Expected: 0 (Free)\n", status262);
	
	printf("\n*****************************\n");
	printf("1.3 Test BitMap_BlockToIndex\n");
	printf("*****************************\n\n");
	
	printf("key6, block:1 to index\n");
	BitMapEntryKey key6 = BitMap_blockToIndex(6);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",6, key6.entry_num, key6.bit_num);
	
	printf("key28, block:28 to index\n");
	BitMapEntryKey key28 = BitMap_blockToIndex(28);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",28, key28.entry_num, key28.bit_num);
	
	printf("key47, block:47 to index\n");
	BitMapEntryKey key47 = BitMap_blockToIndex(47);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",47, key47.entry_num, key47.bit_num);
	
	printf("key13, block:13 to index\n");
	BitMapEntryKey key13 = BitMap_blockToIndex(13);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",13, key13.entry_num, key13.bit_num);
	
	printf("key66, block:66 to index\n");
	BitMapEntryKey key66 = BitMap_blockToIndex(66);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",66, key66.entry_num, key66.bit_num);
	
	printf("key71, block:71 to index\n");
	BitMapEntryKey key71 = BitMap_blockToIndex(71);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",71, key71.entry_num, key71.bit_num);
	
	printf("key120, block:120 to index\n");
	BitMapEntryKey key120 = BitMap_blockToIndex(120);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",120, key120.entry_num, key120.bit_num);
	
  
	
	printf("\n--------------- 1.4 TEST BITMAP INDEX TO BLOCK -------\n");
	
	int i,j;
	for(i=0; i<3; i++){
		for(j=0; j<3; j++){
			int block = BitMap_indexToBlock(i,j);
			printf("Byte:%i - Bit:%i -> Block:%i\n", i, j, block);
		}
	}
	
	printf("\n--------------- 1.5 TEST BITMAP GET -------\n");
	
	int index_to_get1 = BitMap_get(&bitmap1,0,0);
	printf("Starts Looking from position:%i, bit with status:%i\n", 0, 0);
	printf("index got:%i\n", index_to_get1);
	int index_to_get2 = BitMap_get(&bitmap1,5,0);
	printf("Starts Looking from position:%i, bit with status:%i\n", 5, 0);
	printf("index got:%i\n", index_to_get2);
	
	printf("\n--------------- 1.6 TEST BITMAP SET -------\n");
	BitMap_set(&bitmap1,5,0);
	printf("Setting bit at pos:%i with status:%i\n", 5, 0);
	int index_to_get3 = BitMap_get(&bitmap1,5,0);
	printf("Starts Looking from position:%i, bit with status:%i\n", 5, 0);
	printf("index got:%i\n", index_to_get3);
	
	
}
