#include "..\Z-OS\Z-OS.h"
#include "..\Z-OS\Devices\Partition.h"
#include "..\Z-OS\Devices\FileSystems.h"
#include "Structures.h"
#include "Public.h"
#include "FAT.h"

Bool FAT_DetectFS(PartInternal* device)
{
	BootSector* bootSector = zmalloc(sizeof(BootSector));
	Int16 ret = False;
	
	puts("FAT DetectFS called.\r\n");
	
	if (!bootSector) return False;
	
	if ((ret = InternalReadPart(device,0,(UInt8*)bootSector,sizeof(BootSector))))
	{
		zfree(bootSector);
		return False;
	}
	puts("Read partition boot sector sucessfully.\r\n");
	if (!bootSector)
	{
		puts("BootSector is null.\r\n");
		return False;
	}
	if (bootSector->ByteArray[510] == 0x55 && bootSector->ByteArray[511] == 0xAA)
	{
		/*if (bootSector->BPB_TotSec16)
		{
			ret = (bootSector->FAT.BS_BootSig == 0x29) ? True : False;
			if (ret) puts("32-bit BS_BootSig = True\r\n");
		}
		else
		{
			ret = (bootSector->FAT32.BS_BootSig == 0x29) ? True : False;
			if (ret) puts("16-bit BS_BootSig = True\r\n");
		}*/
		ret = true;
	}
	else
	{
		puts("0xAA55 not detected\r\n");
		printf("0x%X and 0x%X were detected instead.\r\n",bootSector->ByteArray[510],bootSector->ByteArray[511]);
		ret = False;
	}
	
	if (ret) puts("FAT detected.\r\n");
	else puts("FAT not detected.\r\n");
	
	zfree(bootSector);
	
	puts("Returning from DetectFS.\r\n");
	
	return ret;
}

//extern void CloseVirtualFile(void);

// Calculates the type of FAT (12/16/32)
static void FAT_CalcFATType(FATVolume* vol, BootSector*  bootSector)
{
	vol->DataSec = vol->TotSec - (bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * vol->FATSize) + vol->RootDirSectors);
	
	vol->CountOfClusters = (UInt32)(vol->DataSec) / (UInt32)(bootSector->BPB_SecsPerClus);
	
	if (vol->CountOfClusters < 4085) vol->Type = FAT12;
	else if (vol->CountOfClusters < 65525) vol->Type = FAT16;
	else vol->Type = FAT32;
}

static void FAT_CalcFATSize(FATVolume* vol, BootSector* bootSector)
{
	// Round the division up
	div_t RootDirSectorsDiv = div(((bootSector->BPB_RootEntCnt * 32) + (bootSector->BPB_BytsPerSec - 1)), bootSector->BPB_BytsPerSec);
	vol->RootDirSectors = RootDirSectorsDiv.quot + (RootDirSectorsDiv.rem ? 1 : 0);
	
	vol->FirstDataSector = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * vol->FATSize) + vol->RootDirSectors;
}

static void FAT_DebugMount(FATVolume* vol, BootSector* bootSector)
{
	char OEMName[9] = {0};
	char VolLabel[12] = {0};
	
	// Copy over the strings
	memcpy(OEMName,bootSector->BS_OEMName,8);
	if (bootSector->BPB_TotSec16) memcpy(VolLabel,bootSector->FAT.BS_VolLab,11);
	else memcpy(VolLabel,bootSector->FAT32.BS_VolLab,11);
	
	puts("Listing FAT mount data:\r\n");
	switch (vol->Type)
	{
		case FAT12: puts("FAT12 volume\r\n"); break;
		case FAT16: puts("FAT16 volume\r\n"); break;
		case FAT32: puts("FAT32 volume\r\n"); break;
	}
	printf("Sector Size: %d bytes\r\n",bootSector->BPB_BytsPerSec);
	if (bootSector->BPB_TotSec32)
	{
		printf("TotalSectors: %ld\r\n",bootSector->BPB_TotSec32);
	}
	else
	{
		printf("TotalSectors: %d\r\n",bootSector->BPB_TotSec16);
	}
	printf("ClusterCount: %lu\r\n",vol->CountOfClusters);
	printf("Data Sectors: %lu\r\n",vol->DataSec);
	if (vol->Type == FAT32)
	{
		printf("RootDirFirstClus: %lu\r\n",vol->RootDirFirstClus);
	}
	else
	{
		printf("RootDirSectors: %u\r\n",vol->RootDirSectors);
	}
	printf("FirstDataSector: %u\r\n",vol->FirstDataSector);
	printf("Sectors per Cluster: %d\r\n",bootSector->BPB_SecsPerClus);
	printf("OEM Name: %s\r\nVolume Label: %s\r\n",OEMName,VolLabel);
}

Int16 FAT_MountDevice(PartInternal* device)
{
	BootSector* bootSector;
	FATVolume* vol;
	Int16 ret;
	
	if (!(bootSector = zmalloc(sizeof(BootSector)))) return ErrorOutOfMemory;
	if (!(vol = zmalloc(sizeof(FATVolume))))
	{
		zfree(bootSector);
		return ErrorOutOfMemory;
	}
	
	// Read in the boot sector and check the signature
	if ((ret = InternalReadPart(device,0,(UInt8*)bootSector,sizeof(BootSector)))) { zfree(bootSector); zfree(vol); return ErrorUnknownFS; }
	if (!(bootSector->ByteArray[510] == 0x55 && bootSector->ByteArray[511] == 0xAA)) { zfree(bootSector); zfree(vol); return ErrorUnknownFS; }
	
	vol->Partition = device;
	vol->BPB_ResvdSecCnt = bootSector->BPB_RsvdSecCnt;
	vol->BPB_BytsPerSec = bootSector->BPB_BytsPerSec;
	vol->BPB_RootEntCnt = bootSector->BPB_RootEntCnt;
	vol->SecsPerClus = bootSector->BPB_SecsPerClus;
	
	if (bootSector->BPB_FATSz16) vol->FATSize = bootSector->BPB_FATSz16;
	else vol->FATSize = bootSector->FAT32.BPB_FATSz32;
	
	if (bootSector->BPB_TotSec16) vol->TotSec = bootSector->BPB_TotSec16;
	else vol->TotSec = bootSector->BPB_TotSec32;
	
	// Calculate some required statistics about the volume
	puts("Calling CalcFATSize...\r\n");
	FAT_CalcFATSize(vol,bootSector);
	
	// Calculate the volume type and some more statistics
	puts("Calling CalcFATType...\r\n");
	FAT_CalcFATType(vol,bootSector);
	
	// Create a FAT buffer and prepare for reading
	puts("Calling LoadFAT...\r\n");
	ret = FAT_LoadFAT(vol);
	
	// TODO: Create a free cluster list:
	//puts("Calling CalcFreeClusters...\r\n");
	//ret = FAT_CalcFreeClusters(vol,bootSector);
	
	// Print some useful information
	puts("Calling DebugMount...\r\n");
	FAT_DebugMount(vol,bootSector);
	
	device->Data1 = vol;
	zfree(bootSector);
	
	puts("Finished mounting volume.\r\n");
	return ret;
}

Int16 FAT_UnmountDevice(PartInternal* device, Bool suprise)
{
	return ErrorUnimplemented;
}
