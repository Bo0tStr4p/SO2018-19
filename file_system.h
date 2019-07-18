#pragma once
#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"

//Use it to start up the file system
DirectoryHandle* FileSystem_StartUp(const char* filename, DiskDriver* disk, SimpleFS* simple_fs);
