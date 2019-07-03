#pragma once
#include "bitmap.h"
#include "disk_driver.h"

#define MAX_BLOCKS 10

/*these are structures stored on disk*/

/*
// header, occupies the first portion of each block in the disk
// represents a chained list of blocks
typedef struct {
  int previous_block; // chained list (previous block)
  int next_block;     // chained list (next_block)
  int block_in_file; // position in the file, if 0 we have a file control block
} BlockHeader;
*/


//Usiamo questa struttura per memorizzare il nostro blocco Index 
//Quando effettuiamo le operazioni di read e write sul disco passiamo semplicemente
//un int come posizione all'interno del disco. Utilizzando un array di int andiamo
//a memorizzare i blocchi da leggere nell'ordine in cui sono presenti all'interno 
//dell'array
typedef struct BlockIndex BlockIndex;

struct BlockIndex{
  int previous; 				//Memorizziamo il predecessore
  int blocks[MAX_BLOCKS];	    //Inizialmente tutti vuoti, mettiamo tutto a -1					
  int next; 					//Memorizziamo il successore
};

// this is in the first block of a chain, after the header
typedef struct {
  int directory_block; // first block of the parent directory
  int block_in_disk;   // repeated position of the block on the disk
  char name[128];
  int written_bytes;   // Usata per memorizzare i byte effettivi all'interno di tutti i file blocks
  int  size_in_bytes;
  int size_in_blocks;
  int is_dir;          // 0 for file, 1 for dir
} FileControlBlock;

// this is the first physical block of a file
// it has a header
// an FCB storing file infos
// and can contain some data

/******************* stuff on disk BEGIN *******************/
typedef struct {
  BlockIndex index;
  FileControlBlock fcb;
} FirstFileBlock;

// this is one of the next physical blocks of a file
typedef struct {
  int index_block;
  int position; //Usiamo questo valore quando torniamo al blocco index per spostarci
  char  data[BLOCK_SIZE - sizeof(int) - sizeof(int)];
} FileBlock;

// this is the first physical block of a directory
typedef struct {
  BlockIndex index;
  FileControlBlock fcb;
  int num_entries;
  /*int file_blocks[ (BLOCK_SIZE
		   -sizeof(BlockIndex)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int) ];*/
} FirstDirectoryBlock;

// this is remainder block of a directory
typedef struct {
  int index_block;
  int position; //Usiamo questo valore quando torniamo al blocco index per spostarci
  int file_blocks[ (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int) ];
} DirectoryBlock;
/******************* stuff on disk END *******************/




  
typedef struct {
  DiskDriver* disk;
  // add more fields if needed
} SimpleFS;

// this is a file handle, used to refer to open files
typedef struct {
  SimpleFS* sfs;                   // pointer to memory file system structure
  FirstFileBlock* fcb;             // pointer to the first block of the file(read it)
  FirstDirectoryBlock* directory;  // pointer to the directory where the file is stored
  FileBlock* current_block;        // current block in the file
  int pos_in_file;                 // position of the cursor
} FileHandle;

typedef struct {
  SimpleFS* sfs;                   // pointer to memory file system structure
  FirstDirectoryBlock* dcb;        // pointer to the first block of the directory(read it)
  FirstDirectoryBlock* parent_dir;  // pointer to the parent directory (null if top level)
  DirectoryBlock* current_block;   // current block in the directory
  int pos_in_dir;                  // absolute position of the cursor in the directory
  int pos_in_block;                // relative position of the cursor in the block
} DirectoryHandle;

// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk);

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs);

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename);

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d);


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename);


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f);

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size);

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size);

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos);

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(DirectoryHandle* d, char* filename);


// Funzioni aggiuntive


// Funzione per creare un nuovo blocco di tipo index
BlockIndex create_block_index(int previous);

// Funzione per ottenere il blocco index da un file
BlockIndex* get_block_index_file(FileBlock* file, DiskDriver* disk);

// Funzione per ottenere il blocco index da una directory
BlockIndex* get_block_index_directory(DirectoryBlock* directory, DiskDriver* disk);

// Funzione che restituisce il blocco successivo file
FileBlock* get_next_block_file(FileBlock* file,DiskDriver* disk);

// Funzione che restituisce il blocco successivo directory
DirectoryBlock* get_next_block_directory(DirectoryBlock* directory,DiskDriver* disk);

//   Funzione per creare un nuovo file blocks collegandolo con il blocco index di riferimento.
//   Restituisce il numero del blocco del disk driver su cui fare write, in caso di errore -1.
//   Utile soprattutto in casi di scrittura.
//   In FileBlock* new andiamo a restituire il blocco, il quale verrà riempito con le informazioni
//   e successivamente scritto nel disco (tramite writeBlock) nel int restituito dalla funzione.

int create_next_file_block(FileBlock* current_block, FileBlock* new, DiskDriver* disk);

// Funzione per creare un nuovo directory block collegandolo con il blocco index di riferimento.
int create_next_directory_block(DirectoryBlock* current_block, DirectoryBlock* new, DiskDriver* disk);

void print_index_block(BlockIndex* index);

// Funzione per ottenere la posizione nel disco di un file block
int get_position_disk_file_block(FileBlock* file_block, DiskDriver* disk);

// Funzione per ottenere la posizione nel disco di un directory block
int get_position_disk_directory_block(DirectoryBlock* directory_block, DiskDriver* disk);

  

