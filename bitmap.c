#include "bitmap.h"


// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){
	BitMapEntryKey* key = (BitMapEntryKey*)malloc(sizeof(BitMapEntryKey));
	char offset = num % 8;		//A. offset del bit dentro l'array
	key->entry_num = num / 8;	//A. indice dell'array
	key->bit_num = offset;		
	return *key;
}

// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num){
	if(entry < 0 || bit_num < 0) return -1;
	
	int res = (entry*8) + bit_num; 
	return res;
}

// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bmap, int start, int status){
	return 0;
}

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status){
	return 0;
}
