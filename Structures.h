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
	UInt16 SecsPerClus;
	
	UInt8* FATBuffer;
	UInt32 FATBufferSector;
	Bool FATBufferDirty;
	
	UInt32 FreeClusters;
	UInt32 NextFreeCluster;
	
} FATVolume;

// Records a time for a file
typedef struct
{
	__attribute__ ((packed)) UInt8 Seconds	: 5; // 0-29 (double to get the real second count)
	__attribute__ ((packed)) UInt8 Minutes	: 6; // 0-59
	__attribute__ ((packed)) UInt8 Hours	: 5; // 0-23
} FATTimeStamp;

// Records a date for a file
typedef struct
{
	__attribute__ ((packed)) UInt8 Day		: 5; // 1-31
	__attribute__ ((packed)) UInt8 Month	: 4; // 1-12
	__attribute__ ((packed)) UInt8 Year		: 7; // 0-127 (years since 1980)
} FATDateStamp;

// Stores file attributes
typedef enum
{
	ATTR_READ_ONLY	= 0x1,
	ATTR_HIDDEN		= 0x2,
	ATTR_SYSTEM		= 0x4,
	ATTR_VOLUME_ID	= 0x8,
	ATTR_DIRECTORY	= 0x10,
	ATTR_ARCHIVE	= 0x20,
	ATTR_LONG_NAME	= ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID
} FileAttrs;

// Stores a directory entry
typedef struct
{
	__attribute__ ((packed)) UInt8 Name[11];
	__attribute__ ((packed)) UInt8 Attr;
	__attribute__ ((packed)) UInt8 NTRes;
	__attribute__ ((packed)) UInt8 CrtTimeTenth;
	__attribute__ ((packed)) UInt16 CrtTime;
	__attribute__ ((packed)) UInt16 CrtDate;
	__attribute__ ((packed)) UInt16 LstAccDate;
	__attribute__ ((packed)) UInt16 FstClustHI;
	__attribute__ ((packed)) UInt16 WriteTime;
	__attribute__ ((packed)) UInt16 WriteDate;
	__attribute__ ((packed)) UInt16 FstClustLO;
	__attribute__ ((packed)) UInt32 FileSize;
} DirEntry;

// Stores a long name directory entry
typedef struct
{
	__attribute__ ((packed)) UInt8 Ord;
	__attribute__ ((packed)) UInt8 Name1[10];
	__attribute__ ((packed)) UInt8 Attr; // Must be ATTR_LONG_NAME
	__attribute__ ((packed)) UInt8 Type; // 0 for long name subcomponent
	__attribute__ ((packed)) UInt8 Chksum;
	__attribute__ ((packed)) UInt8 Name2[12];
	__attribute__ ((packed)) UInt16 FstClustLO; // Must be 0
	__attribute__ ((packed)) UInt8 Name3[3];
} LongDirEntry;

typedef struct
{
	FATVolume* Volume;
	char* Path;
	DirEntry Entry;
} FATFile;

#endif
