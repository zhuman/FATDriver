#ifndef _STRUCTURES_FAT_HEADER_
#define _STRUCTURES_FAT_HEADER_

typedef struct
{
	union
	{
		__attribute__ ((packed)) UInt8 ByteArray[512];
		struct
		{
			__attribute__ ((packed)) UInt8 BS_jmpBoot[3];
			__attribute__ ((packed)) UInt8 BS_OEMName[8];
			__attribute__ ((packed)) UInt16 BPB_BytsPerSec;
			__attribute__ ((packed)) UInt8 BPB_SecsPerClus;
			__attribute__ ((packed)) UInt16 BPB_RsvdSecCnt;
			__attribute__ ((packed)) UInt8 BPB_NumFATs;
			__attribute__ ((packed)) UInt16 BPB_RootEntCnt;
			__attribute__ ((packed)) UInt16 BPB_TotSec16;
			__attribute__ ((packed)) UInt8 BPB_Media;
			__attribute__ ((packed)) UInt16 BPB_FATSz16;
			__attribute__ ((packed)) UInt16 BPB_SecPerTrk;
			__attribute__ ((packed)) UInt16 BPB_NumHeads;
			__attribute__ ((packed)) UInt32 BPB_HiddSec;
			__attribute__ ((packed)) UInt32 BPB_TotSec32;
			union
			{
				// FAT12/16
				struct
				{
					__attribute__ ((packed)) UInt8 BS_DrvNum;
					__attribute__ ((packed)) UInt8 BS_Reserved1;
					__attribute__ ((packed)) UInt8 BS_BootSig;
					__attribute__ ((packed)) UInt32 BS_VolID;
					__attribute__ ((packed)) UInt8 BS_VolLab[11];
					__attribute__ ((packed)) UInt8 BS_FilSysType[8];
				} FAT;
				// FAT32
				struct
				{
					__attribute__ ((packed)) UInt32 BPB_FATSz32;
					__attribute__ ((packed)) UInt16 BPB_ExtFlags;
					__attribute__ ((packed)) UInt16 BPB_FSVer;
					__attribute__ ((packed)) UInt32 BPB_RootClus;
					__attribute__ ((packed)) UInt16 BPB_FSInfo;
					__attribute__ ((packed)) UInt16 BPB_BkBootSec;
					__attribute__ ((packed)) UInt8 BPB_Reserved[12];
					__attribute__ ((packed)) UInt8 BS_DrvNum;
					__attribute__ ((packed)) UInt8 BS_Reserved1;
					__attribute__ ((packed)) UInt8 BS_BootSig;
					__attribute__ ((packed)) UInt32 BS_VolID;
					__attribute__ ((packed)) UInt8 BS_VolLab[11];
					__attribute__ ((packed)) UInt8 BS_FileSysTyp[8];
				} FAT32;
			};
		};
	};
} BootSector;

typedef struct
{
	__attribute__ ((packed)) UInt32 FSI_LeadSig; // 0x41615252
	__attribute__ ((packed)) UInt8 FSI_Reserved1[480];
	__attribute__ ((packed)) UInt32 FSI_StrucSig; // 0x61417272
	__attribute__ ((packed)) UInt32 FSI_Free_Count; // 0xFFFFFFFF if invalid
	__attribute__ ((packed)) UInt32 FSI_Nxt_Free; // 0xFFFFFFFF if invalid
	__attribute__ ((packed)) UInt8 FSI_Reserved2[12];
} Fat32FSInfo;

typedef struct
{
	enum
	{
		FAT12,
		FAT16,
		FAT32
	} Type;
	
	PartInternal* Partition;
	
	UInt32 FATSize;
	UInt32 TotSec;
	UInt32 DataSec;
	UInt32 CountOfClusters;
	UInt16 RootDirSectors;
	UInt16 FirstDataSector;
	UInt16 BPB_ResvdSecCnt;
	UInt16 BPB_BytsPerSec;
	
	UInt8* FATBuffer;
	UInt32 FATBufferSector;
	
	UInt32 FreeClusters;
	UInt32 NextFreeCluster;
	
} FATVolume;

#endif
