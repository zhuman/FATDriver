#include "..\Z-OS\Z-OS.h"
#include "..\Z-OS\Devices\Partition.h"
#include "..\Z-OS\Devices\FileSystems.h"
#include "Structures.h"

Bool FileExists(PartInternal* device, char* path)
{
	return ErrorUnimplemented;
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
