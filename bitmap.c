#include "bitmap.h"


// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){
	BitMapEntryKey key;
	char offset = num % 8;
	key.entry_num = num / 8;	//Alessandro: indice dell'array
	key.bit_num = offset;		//Alessandro: offset del bit dentro l'array
	return key;
}

// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num){
	return 0;
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
