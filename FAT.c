#include "..\Z-OS\Z-OS.h"
#include "..\Z-OS\Devices\Partition.h"
#include "..\Z-OS\Devices\FileSystems.h"
#include "Structures.h"

UInt32 FAT_GetEndOfCluster(FATVolume* vol)
{
	switch (vol->Type)
	{
		case FAT32: return 0x0FFFFFF8;
		case FAT16: return 0xFFF8;
		case FAT12: default: return 0x0FF8;
	}
}

UInt32 FAT_ClusterNumToSector(FATVolume* vol, UInt32 clus)
{
	return ((clus - 2) * vol->SecsPerClus) + vol->FirstDataSector - 1;
}

UInt32 FAT_ClusterNumToByte(FATVolume* vol, UInt32 clus)
{
	return (((clus - 2) * vol->SecsPerClus) + vol->FirstDataSector - 1) * vol->BPB_BytsPerSec;
}

Int16 FAT_ClusterNumToFATIndex(FATVolume* vol, UInt32 cluster, UInt16* offset, UInt16* sectorOffset, UInt32* sector)
{
	UInt32 FATOffset;
	if (cluster > vol->CountOfClusters) return ErrorInvalidCluster;
	
	if (vol->Type == FAT12)
	{
		FATOffset = cluster + (cluster >> 1);
    	
    	*offset = FATOffset;
    	*sector = vol->BPB_ResvdSecCnt + (FATOffset / vol->BPB_BytsPerSec);
    	*sectorOffset = FATOffset % vol->BPB_BytsPerSec;
	}
	else
	{
		if (vol->Type == FAT16) FATOffset = cluster * 2;
		else if (vol->Type == FAT32) FATOffset = cluster * 4;
		
		*offset = FATOffset;
		*sector = vol->BPB_ResvdSecCnt + (FATOffset / vol->BPB_BytsPerSec);
		*sectorOffset = FATOffset % vol->BPB_BytsPerSec;
	}
	return ErrorSuccess;
}

Int16 FAT_GetFATEntry(FATVolume* vol, UInt32 index, UInt32* entry)
{
	Int16 ret;
	UInt16 offset, sectorOffset;
	UInt32 sector;
	if ((ret = FAT_ClusterNumToFATIndex(vol,index,&offset,&sectorOffset,&sector))) return ret;
	
	// Load the buffer if needed
	if (vol->FATBufferSector != sector)
	{
		ret = InternalReadPart(vol->Partition,sector * vol->BPB_BytsPerSec,vol->FATBuffer,(vol->Type == FAT12) ? (vol->BPB_BytsPerSec * 2) : (vol->BPB_BytsPerSec));
		if (ret) return ret;
	}
	
	// Find and return the entry
	switch (vol->Type)
	{
		case FAT12:
			if (offset & 1)
				*entry = (UInt32)(*((UInt16*)&(vol->FATBuffer[sectorOffset])) >> 4); // Odd cluster
			else
				*entry = (UInt32)(*((UInt16*)&(vol->FATBuffer[sectorOffset])) & 0x0FFF); // Even cluster
		break;
		case FAT16:
			*entry = (UInt32)*((UInt16*)&(vol->FATBuffer[sectorOffset]));
		break;
		case FAT32:
			*entry = *((UInt32*)&(vol->FATBuffer[sectorOffset])) & 0x0FFFFFFF;
		break;
	}
	
	return ErrorSuccess;
}

Int16 FAT_SetFATEntry(FATVolume* vol, UInt32 index, UInt32 entry)
{
	Int16 ret;
	UInt16 offset, sectorOffset;
	UInt32 sector;
	if ((ret = FAT_ClusterNumToFATIndex(vol,index,&offset,&sectorOffset,&sector))) return ret;
	
	// Load the buffer if needed
	if (vol->FATBufferSector != sector)
	{
		if (vol->FATBufferDirty)
		{
			ret = InternalWritePart(vol->Partition,vol->FATBufferSector * vol->BPB_BytsPerSec,vol->FATBuffer,(vol->Type == FAT12) ? (vol->BPB_BytsPerSec * 2) : (vol->BPB_BytsPerSec));
			if (ret) return ret;
		}
		ret = InternalReadPart(vol->Partition,sector * vol->BPB_BytsPerSec,vol->FATBuffer,(vol->Type == FAT12) ? (vol->BPB_BytsPerSec * 2) : (vol->BPB_BytsPerSec));
		if (ret) return ret;
	}
	
	// Find and return the entry
	switch (vol->Type)
	{
		case FAT12:
			if (offset & 1)
			{
			    entry = entry << 4; // Odd cluster
			    *((UInt16*) &(vol->FATBuffer[sectorOffset])) = 
			        (*((UInt16*) &(vol->FATBuffer[sectorOffset]))) & 0x000F;
			}
			else
			{
			    entry = entry & 0x0FFF; // Even Cluster
			    *((UInt16*) &(vol->FATBuffer[sectorOffset])) = 
			        (*((UInt16*) &(vol->FATBuffer[sectorOffset]))) & 0xF000;
			}
			*((UInt16*) &(vol->FATBuffer[sectorOffset])) = 
			    (*((UInt16*) &(vol->FATBuffer[sectorOffset]))) | entry;
		break;
		case FAT16:
			*((UInt16*)&(vol->FATBuffer[sectorOffset])) = entry;
		break;
		case FAT32:
			*((UInt32*)&(vol->FATBuffer[sectorOffset])) &= entry | 0xF0000000;
		break;
	}
	vol->FATBufferDirty = true;
	
	return ErrorSuccess;
}

Int16 FAT_LoadFAT(FATVolume* vol)
{
	Int16 ret;
	UInt32 entry;
	
	// Init the FAT buffer to hold one FAT sector, or two for FAT12 volumes
	vol->FATBuffer = zmalloc((vol->Type == FAT12) ? (vol->BPB_BytsPerSec * 2) : vol->BPB_BytsPerSec);
	if (!(vol->FATBuffer)) return ErrorOutOfMemory;
	
	// Force the FAT buffer to load on the first call
	vol->FATBufferSector = 0xFFFFFFFF;
	
	// Check for volume problems
	if ((ret = FAT_GetFATEntry(vol,0,&entry))) return ret;
	if (vol->Type == FAT16)
	{
		if (!(entry & 0x8000)) puts("The volume was not unmounted properly on last use.\r\n");
		if (!(entry & 0x4000)) puts("Volume read/write errors were reported on last use.\r\n");
	}
	else if (vol->Type == FAT32)
	{
		if (!(entry & 0x08000000)) puts("The volume was not unmounted properly on last use.\r\n");
		if (!(entry & 0x04000000)) puts("Volume read/write errors were reported on last use. Run CheckDisk on the volume.\r\n");
	}
	
	return ErrorSuccess;
}

Int16 FAT_ScanFirstFree(FATVolume* vol)
{
	UInt32 i;
	Int16 ret;
	for (i = 2; i < vol->CountOfClusters; i++)
	{
		UInt32 entry;
		ret = FAT_GetFATEntry(vol,i,&entry);
		if (ret) return ret;
		if (!entry)
		{
			vol->NextFreeCluster = i;
			return ErrorSuccess;
		}
	}
	vol->NextFreeCluster = vol->CountOfClusters;
	return ErrorSuccess;
}

Int16 FAT_ScanFreeClusters(FATVolume* vol)
{
	Int16 ret;
	
	if ((ret = FAT_ScanFirstFree(vol))) return ret;
	if (vol->NextFreeCluster < vol->CountOfClusters)
	{
		UInt32 count = 0;
		UInt32 i;
		for (i = vol->NextFreeCluster; i < vol->CountOfClusters; i++)
		{
			UInt32 entry;
			if ((ret = FAT_GetFATEntry(vol,i,&entry))) return ret;
			if (!entry) count++;
		}
		vol->FreeClusters = count;
	}
	else
	{
		vol->FreeClusters = 0;
	}
	return ErrorSuccess;
}

Int16 FAT_CalcFreeClusters(FATVolume* vol, BootSector* bootSector)
{
	Int16 ret = ErrorSuccess;
	
	if (vol->Type == FAT32)
	{
		if (bootSector->FAT32.BPB_FSInfo > 0)
		{
			// FAT32 has an FSInfo block describing free cluster statistics
			Fat32FSInfo* info = zmalloc(sizeof(Fat32FSInfo));
			if (!info) return ErrorOutOfMemory;
			
			ret = InternalReadPart(vol->Partition,bootSector->FAT32.BPB_FSInfo * vol->BPB_BytsPerSec,(UInt8*)info,sizeof(Fat32FSInfo));
			if (ret)
			{
				zfree(info);
				return ret;
			}
			
			if (info->FSI_LeadSig != 0x41615252) return ErrorSignature;
			if (info->FSI_StrucSig != 0x61417272) return ErrorSignature;
			
			// Check the free cluster count
			if (info->FSI_Free_Count <= vol->CountOfClusters)
			{
				vol->FreeClusters = info->FSI_Free_Count;
				// Check the next free cluster index
				if (info->FSI_Nxt_Free < vol->CountOfClusters)
				{
					vol->NextFreeCluster = info->FSI_Nxt_Free;
				}
				else
				{
					ret = FAT_ScanFirstFree(vol);
				}
				zfree(info);
				return ret;
			}
		}
	}
	ret = FAT_ScanFreeClusters(vol);
	return ret;
}
