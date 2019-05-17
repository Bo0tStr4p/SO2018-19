#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"



int main(int agc, char** argv) { 					//A. Main di prova per testare le funzioni di bitmap.c 
	
	printf("\n*************************************************************\n");
	printf("\t\t1. TEST BITMAP.C\n");
	printf("*************************************************************\n\n");
	printf("\n*************************************************************\n");
	printf("\t\t1.1 Test BitMap struct\n");
	printf("*************************************************************\n\n");
  
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
	
	printf("\n*************************************************************\n");
	printf("\t\t1.2 Test BitMap_is_free_block\n");
	printf("*************************************************************\n\n");
	
	
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
	
	printf("\n*************************************************************\n");
	printf("\t\t1.3 Test BitMap_blockToIndex\n");
	printf("*************************************************************\n\n");
	
	printf("key6, block:6 to index\n");
	BitMapEntryKey key6 = BitMap_blockToIndex(6);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",6, key6.entry_num, key6.bit_num);
	
	printf("key13, block:13 to index\n");
	BitMapEntryKey key13 = BitMap_blockToIndex(13);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",13, key13.entry_num, key13.bit_num);
	
	printf("key22, block:22 to index\n");
	BitMapEntryKey key22= BitMap_blockToIndex(22);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",22, key22.entry_num, key22.bit_num);
	
	printf("key28, block:28 to index\n");
	BitMapEntryKey key28 = BitMap_blockToIndex(28);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",28, key28.entry_num, key28.bit_num);
	
	printf("key32, block:32 to index\n");
	BitMapEntryKey key32 = BitMap_blockToIndex(32);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",32, key32.entry_num, key32.bit_num);
	
	printf("key47, block:47 to index\n");
	BitMapEntryKey key47 = BitMap_blockToIndex(47);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",47, key47.entry_num, key47.bit_num);
	
	printf("key50, block:50 to index\n");
	BitMapEntryKey key50 = BitMap_blockToIndex(50);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",50, key50.entry_num, key50.bit_num);
	
	printf("key66, block:66 to index\n");
	BitMapEntryKey key66 = BitMap_blockToIndex(66);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",66, key66.entry_num, key66.bit_num);
	
	printf("key71, block:71 to index\n");
	BitMapEntryKey key71 = BitMap_blockToIndex(71);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",71, key71.entry_num, key71.bit_num);
	
	printf("key120, block:120 to index\n");
	BitMapEntryKey key120 = BitMap_blockToIndex(120);
	printf("Block:%i -> Byte:%i - Bit:%i\n\n",120, key120.entry_num, key120.bit_num);
	
	printf("\n*************************************************************\n");
	printf("\t\t1.4 Test BitMap_indexToBlock\n");
	printf("*************************************************************\n\n");
  
	printf("Byte:0 - Bit:0 to Block -->\n");
	int block0 = BitMap_indexToBlock(0,0);
	printf("Block: %i | Expected: 0\n\n", block0);
	
	printf("Byte:1 - Bit:0 to Block -->\n");
	int block8 = BitMap_indexToBlock(1,0);
	printf("Block: %i | Expected: 8\n\n", block8);
	
	printf("Byte:2 - Bit:2 to Block -->\n");
	int block18 = BitMap_indexToBlock(2,2);
	printf("Block: %i | Expected: 18\n\n", block18);
	
	printf("Byte:2 - Bit:6 to Block -->\n");
	int block22 = BitMap_indexToBlock(2,6);
	printf("Block: %i | Expected: 22\n\n", block22);
	
	printf("Byte:3 - Bit:6 to Block -->\n");
	int block30 = BitMap_indexToBlock(3,6);
	printf("Block: %i | Expected: 30\n\n", block30);
	
	printf("Byte:4 - Bit:5 to Block -->\n");
	int block37 = BitMap_indexToBlock(4,5);
	printf("Block: %i | Expected: 37\n\n", block37);
	
	printf("Byte:5 - Bit:2 to Block -->\n");
	int block42 = BitMap_indexToBlock(5,2);
	printf("Block: %i | Expected: 42\n\n", block42);
	
	printf("Byte:5 - Bit:7 to Block -->\n");
	int block47 = BitMap_indexToBlock(5,7);
	printf("Block: %i | Expected: 47\n\n", block47);
	
	printf("Byte:8 - Bit:6 to Block -->\n");
	int block70 = BitMap_indexToBlock(8,6);
	printf("Block: %i | Expected: 70\n\n", block70);
	
	printf("\n*************************************************************\n");
	printf("\t\t1.5 Test BitMap_get\n");
	printf("*************************************************************\n\n");
  
	int index_to_get1 = BitMap_get(&bitmap1,2,0);
	printf("Starts from position: %i, bit with status: %i\n", 2, 0);
	printf("index got: %i | Expected: 2 \n\n", index_to_get1);
	
	int index_to_get2 = BitMap_get(&bitmap1,2,1);
	printf("Starts from position: %i, bit with status: %i\n", 2, 1);
	printf("index got: %i | Expected: 4 \n\n", index_to_get2);
	
	int index_to_get3 = BitMap_get(&bitmap1,7,1);
	printf("Starts from position: %i, bit with status: %i\n", 7, 1);
	printf("index got: %i | Expected: 12 \n\n", index_to_get3);
	
	int index_to_get4 = BitMap_get(&bitmap1,20,0);
	printf("Starts from position: %i, bit with status: %i\n", 20, 0);
	printf("index got: %i | Expected: 22 \n\n", index_to_get4);
	
	int index_to_get5 = BitMap_get(&bitmap1,26,0);
	printf("Starts from position: %i, bit with status: %i\n", 26, 0);
	printf("index got: %i | Expected: 26 \n\n", index_to_get5);
	
	int index_to_get6 = BitMap_get(&bitmap1,29,1);
	printf("Starts from position: %i, bit with status: %i\n", 29, 1);
	printf("index got: %i | Expected: 29 \n\n", index_to_get6);
	
	int index_to_get7 = BitMap_get(&bitmap1,32,1);
	printf("Starts from position: %i, bit with status: %i\n", 32, 1);
	printf("index got: %i | Expected: 36 \n\n", index_to_get7);
	
	int index_to_get8 = BitMap_get(&bitmap1,56,0);
	printf("Starts from position: %i, bit with status: %i\n", 56, 0);
	printf("index got: %i | Expected: 57 \n\n", index_to_get8);
	
	int index_to_get9 = BitMap_get(&bitmap2,2,0);
	printf("Starts from position: %i, bit with status: %i\n", 2, 0);
	printf("index got: %i | Expected: 2 \n\n", index_to_get9);
	
	int index_to_get10 = BitMap_get(&bitmap2,2,1);
	printf("Starts from position: %i, bit with status: %i\n", 2, 1);
	printf("index got: %i | Expected: 4 \n\n", index_to_get10);
	
	int index_to_get11 = BitMap_get(&bitmap2,7,0);
	printf("Starts from position: %i, bit with status: %i\n", 7, 0);
	printf("index got: %i | Expected: 7 \n\n", index_to_get11);
	
	int index_to_get12 = BitMap_get(&bitmap2,20,0);
	printf("Starts from position: %i, bit with status: %i\n", 20, 0);
	printf("index got: %i | Expected: 22 \n\n", index_to_get12);
	
	int index_to_get13 = BitMap_get(&bitmap2,26,0);
	printf("Starts from position: %i, bit with status: %i\n", 26, 0);
	printf("index got: %i | Expected: 26 \n\n", index_to_get13);
	
	int index_to_get14 = BitMap_get(&bitmap2,29,1);
	printf("Starts from position: %i, bit with status: %i\n", 29, 1);
	printf("index got: %i | Expected: 29 \n\n", index_to_get14);
	
	int index_to_get15 = BitMap_get(&bitmap2,32,1);
	printf("Starts from position: %i, bit with status: %i\n", 32, 1);
	printf("index got: %i | Expected: 32 \n\n", index_to_get15);
	
	int index_to_get16 = BitMap_get(&bitmap2,60,0);
	printf("Starts from position: %i, bit with status: %i\n", 60, 0);
	printf("index got: %i | Expected: 62 \n\n", index_to_get16);
	
	printf("\n*************************************************************\n");
	printf("\t\t1.6 Test BitMap_set\n");
	printf("*************************************************************\n\n");
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 0, 1);
	BitMap_set(&bitmap1,0,1);
	printf("Getting bit -->\n");
	int get1 = BitMap_is_free_block(&bitmap1,0);
	printf("bitmap1 - Block:0 - Status:%i | Expected: -1 (Full)\n\n", get1);
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 5, 0);
	BitMap_set(&bitmap1,5,0);
	printf("Getting bit -->\n");
	int get2 = BitMap_is_free_block(&bitmap1,5);
	printf("bitmap1 - Block:5 - Status:%i | Expected: 0 (Free)\n\n", get2);
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 9, 1);
	BitMap_set(&bitmap1,9,1);
	printf("Getting bit -->\n");
	int get3 = BitMap_is_free_block(&bitmap1,9);
	printf("bitmap1 - Block:9 - Status:%i | Expected: -1 (Full)\n\n", get3);
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 21, 0);
	BitMap_set(&bitmap1,21,0);
	printf("Getting bit -->\n");
	int get4 = BitMap_is_free_block(&bitmap1,21);
	printf("bitmap1 - Block:21 - Status:%i | Expected: 0 (Free)\n\n", get4);
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 45, 0);
	BitMap_set(&bitmap1,45,0);
	printf("Getting bit -->\n");
	int get5 = BitMap_is_free_block(&bitmap1,45);
	printf("bitmap1 - Block:45 - Status:%i | Expected: 0 (Free)\n\n", get5);
	
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 0, 1);
	BitMap_set(&bitmap2,0,1);
	printf("Getting bit -->\n");
	int get6 = BitMap_is_free_block(&bitmap2,0);
	printf("bitmap2 - Block:0 - Status:%i | Expected: -1 (Full)\n\n", get6);
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 5, 0);
	BitMap_set(&bitmap2,5,0);
	printf("Getting bit -->\n");
	int get7 = BitMap_is_free_block(&bitmap2,5);
	printf("bitmap2 - Block:5 - Status:%i | Expected: 0 (Free)\n\n", get7);
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 12, 0);
	BitMap_set(&bitmap2,12,0);
	printf("Getting bit -->\n");
	int get8 = BitMap_is_free_block(&bitmap2,12);
	printf("bitmap2 - Block:12 - Status:%i | Expected: 0 (Free)\n\n", get8);
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 34, 1);
	BitMap_set(&bitmap2,34,1);
	printf("Getting bit -->\n");
	int get9 = BitMap_is_free_block(&bitmap2,34);
	printf("bitmap2 - Block:34 - Status:%i | Expected: -1 (Full)\n\n", get9);
	
	printf("Setting bit at pos: %i with status: %i in bitmap1\n", 45, 0);
	BitMap_set(&bitmap2,45,0);
	printf("Getting bit -->\n");
	int get10 = BitMap_is_free_block(&bitmap2,45);
	printf("bitmap2 - Block:45 - Status:%i | Expected: 0 (Free)\n\n", get10);
	
	
}
