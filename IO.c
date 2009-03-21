#include "..\Z-OS\Z-OS.h"
#include "..\Z-OS\Devices\Partition.h"
#include "..\Z-OS\Devices\FileSystems.h"
#include "Structures.h"
#include "FAT.h"

Int16 FAT_ClusterChainIO(FATVolume* vol, Bool read, UInt32 firstCluster, UInt32 pos, UInt8* buffer, UInt32 bufLen)
{
	UInt32 bytsPerClus = vol->SecsPerClus * vol->BPB_BytsPerSec;
	// Divide by 512 (the number of bytes in a sector)
	UInt32 clusterBegin = (pos / (vol->SecsPerClus * vol->BPB_BytsPerSec)) + firstCluster;
	UInt32 i = clusterBegin;
	Int16 ret;
	pos -= (clusterBegin - firstCluster) * vol->SecsPerClus * vol->BPB_BytsPerSec;
	
	while (1)
	{
		UInt16 bytesToCopy;
		UInt32 entry;
		
		bytesToCopy = Min(bytsPerClus - pos,bufLen);
		if (read)
			ret = InternalReadPart(vol->Partition,pos + FAT_ClusterNumToByte(vol,i),buffer,bytesToCopy);
		else
			ret = InternalWritePart(vol->Partition,pos + FAT_ClusterNumToByte(vol,i),buffer,bytesToCopy);
		if (ret) return ret;
		bufLen -= bytesToCopy;
		
		// Have we finished?
		if (bufLen == 0) break;
		
		// Move to the next buffer
		ret = FAT_GetFATEntry(vol,i,&entry);
		if (ret) return ret;
		if (entry == FAT_GetEndOfCluster(vol)) break;
		i = entry;
		
		buffer += bytesToCopy;
		pos = 0;
	}
	return ErrorSuccess;
}

Int16 FAT_ReadFile(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen)
{
	Int16 ret;
	if (file->Data1)
	{
		FATFile* f = file->Data1;
		if (pos + bufLen > f->Entry.FileSize) return ErrorInvalidSeek;
		ret = FAT_ClusterChainIO(f->Volume,true,((UInt32)(f->Entry.FstClustLO)) & (((UInt32)(f->Entry.FstClustHI)) << 16),pos,buffer,bufLen);
		return ret;
	}
	return ErrorUnknown;
}

Int16 FAT_WriteFile(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen)
{
	Int16 ret;
	if (file->Data1)
	{
		FATFile* f = file->Data1;
		UInt32 bytsPerClus = f->Volume->SecsPerClus * f->Volume->BPB_BytsPerSec;
		//if (pos + bufLen > f->Entry.FileSize) return ErrorInvalidSeek;
		if (pos + bufLen > f->Entry.FileSize)
		{
			if ((f->Entry.FileSize / bytsPerClus) > ((pos + bufLen) / bytsPerClus))
			{
				//SetFATEntry(f->Volume,
			}
			f->Entry.FileSize = pos + bufLen;
		}	
		ret = FAT_ClusterChainIO(f->Volume,false,((UInt32)(f->Entry.FstClustLO)) & (((UInt32)(f->Entry.FstClustHI)) << 16),pos,buffer,bufLen);
		return ret;
	}
	return ErrorUnknown;
}
