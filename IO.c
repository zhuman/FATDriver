#include "..\Z-OS\Z-OS.h"
#include "..\Z-OS\Devices\Partition.h"
#include "..\Z-OS\Devices\FileSystems.h"
#include "Structures.h"

Int16 ReadFile(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen)
{
	return ErrorUnimplemented;
}

Int16 WriteFile(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen)
{
	return ErrorUnimplemented;
}
