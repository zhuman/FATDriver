#include "..\Z-OS\Z-OS.h"
#include "..\Z-OS\Devices\Partition.h"
#include "..\Z-OS\Devices\FileSystems.h"
#include "Structures.h"

FATFile* FindFile(FATVolume* vol, char* path)
{
	FATFile* file = zmalloc(sizeof(FATFile));
	int i;
	int pathLen = strlen(path);
	if (!file) return null;
	if (pathLen == 1 && path[0] == '\\')
	{
		if (vol->Type == FAT32)
		{
			//vol->
			//vol->FirstDataSector - vol->RootDirSectors
		}	
	}
	else
	{	
		for (i = pathLen - 1; i > 0; i--)
		{
			if (path[i] == '\\')
			{
				
			}
		}
	}
}

Bool FileExists(PartInternal* device, char* path)
{
	if (device->FileSystem && device->Data1)
	{
		FATVolume* vol = device->Data1;
		
	}
}

// Gets a file's (or dir's) attributes
Int16 GetFile(FileInternal* file, char** reparse)
{
	return ErrorUnimplemented;
}

// Opens a file for IO
Int16 OpenFile(FileInternal* file, FileMode mode)
{
	return ErrorUnimplemented;
}

// Sets file attributes
Int16 SetFile(FileInternal* file)
{
	return ErrorUnimplemented;
}

Int16 DeleteFile(FileInternal* file)
{
	return ErrorUnimplemented;
}

Int16 RenameFile(FileInternal* file, char* path)
{
	return ErrorUnimplemented;
}
