#include <stdlib.h>

#include "bitmap.h"



// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){
	BitMapEntryKey* key = (BitMapEntryKey*)malloc(sizeof(BitMapEntryKey));
	key->entry_num = num / 8;	//A. indice dell'array
	char offset = num % 8;		//A. offset del bit dentro l'array
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
	if(start > bmap->num_bits || start < 0) return -1;			//A. se start è più grande del numero dei blocchi o è minore di 0, errore
	
	for(i=start; i<bmap->num_bits; i++){
		BitMapEntryKey key = BitMap_blockToIndex(i);
		int idx = key->entry_num;
		int bit = bmap->entries[idx] >> key->bit_num & 0x01;	//A. prendo il bit
		if(bit == status) return i;							
	}
	return -1;
}

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status){
	if(pos > bmap->num_bits || pos < 0) return -1;				//A. se pos è più grande del numero dei blocchi o è minore di 0, errore	
	BitMapEntryKey key = BitMap_blockToIndex(pos);
	
	unsigned char flag = 1 << key->bit_num;						//flag di controllo
	unsigned char bitToSet = bmap->entries[key->entry_num];		//bit da settare
	
	if(status){
		bmap->entries[key->entry_num] = bitToSet | flag;
		return bitToSet | flag;
	}
	else{
		bmap->entries[map.entry_num] = bitToSet & (~flag);       
        return bitToSet & (~flag);
	}
	
}
