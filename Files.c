#include "..\Z-OS\Z-OS.h"
#include "..\Z-OS\Devices\Partition.h"
#include "..\Z-OS\Devices\FileSystems.h"
#include "Structures.h"
#include "FAT.h"

const UInt8 ForbiddenFileNameChars[] = {0x22, 0x2A, 0x2B, 0x2C, 0x2F, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x5B, 0x5C, 0x5D, 0x7C, 0};

// Maps a short name to an 11-byte array such as the one in a directory entry
Int16 MapShortName(const unsigned char* shortName, unsigned char* entryArray)
{
	UInt16 i;
	UInt16 j;
	UInt16 len = strlen((char*)shortName);
	if (len > 12) return ErrorInvalidFileName;

	// Check for illegal characters
	for (i = 0; i < len; i++)
	{
		char str[2] = {0};
		str[0] = shortName[i];
		if (shortName[i] <= 0x20)
		{
			puts("Invalid character less than 0x20\r\n");
			return ErrorInvalidFileName;
		}
		if (strpbrk(str,(char*)ForbiddenFileNameChars))
		{
			printf("Invalid character %x\r\n",*strpbrk(str,(char*)ForbiddenFileNameChars));
			return ErrorInvalidFileName;
		}
	}
	
	// Fill the array with spaces
	memset(entryArray,0x20,11);
	
	// Fill in the 11-byte array
	for (i = 0, j = 0; i < 12; i++, j++)
	{
		if (!shortName[i]) break;
		
		// Once the period is reached, jump to the place for the extension in the array
		if (shortName[i] == '.')
		{
			if (i > 7) return ErrorInvalidFileName;
			puts("Reached period.\r\n");
			j = 8 - 1;
			continue;
		}
		entryArray[j] = shortName[i];
		
		// Make all letters uppercase
		if (shortName[i] >= 'a' && shortName[i] <= 'z') entryArray[j] -= 'a' - 'A';
		else if ((!j) && (shortName[i] == 0x5)) entryArray[j] = 0xE5;
	}
	return ErrorSuccess;
}

FATFile* FAT_FindFile(FATVolume* vol, char* path, UInt16 pathLen)
{
	FATFile* file = zmalloc(sizeof(FATFile));
	char* segmentStart;
	UInt16 segmentLength = 0;
	UInt16 i;
	
	if (!file) return null;
	file->Path = zmalloc(pathLen + 1);
	if (!(file->Path))
	{
		zfree(file);
		return null;
	}
	memcpy(file->Path,path,pathLen);
	if (vol->Type == FAT32)
	{
		file->Entry.FstClustLO = vol->RootDirFirstClus & 0xFFFF;
		file->Entry.FstClustHI = vol->RootDirFirstClus >> 16;
	}
	else
	{
		file->Entry.FstClustLO = 0;
		file->Entry.FstClustHI = 0;
		//file->Entry.FileSize = vol->BPB_BytsPerSec * vol->RootDirSectors;
		file->Entry.FileSize = vol->BPB_RootEntCnt * sizeof(DirEntry);
	}
	
	for (i = 0; i < pathLen; i++)
	{
		printf("Current char: %c\r\n",path[i]);
		if ((path[i] == '\\') || (i == pathLen - 1))
		{
			if (i == pathLen - 1) segmentLength++;
			puts("Segment started.\r\n");
			if (segmentLength > 12)
			{
				zfree(file);
				return null;
			}
			else if (segmentLength)
			{
				UInt16 j = 0; UInt16 k = 0;
				UInt32 firstCluster = (UInt32)(file->Entry.FstClustLO) & (((UInt32)(file->Entry.FstClustHI)) << 16);
				UInt32 entryCount = file->Entry.FileSize / sizeof(DirEntry);
				char* segmentShort = zmalloc(11);
				char* segmentOrig = zmalloc(13);
				
				if (!segmentShort || !segmentOrig)
				{
					if (segmentShort) zfree(segmentShort);
					if (segmentOrig) zfree(segmentOrig);
					zfree(file);
					return null;
				}
				
				puts("Mapping segment to short name...\r\n");
				
				// Convert the path segment into the format used for directory entries
				memcpy((void*)segmentOrig,(void*)segmentStart,segmentLength);
				puts("Path Segment:");
				puts(segmentOrig);
				puts("\r\n");
				if (MapShortName((UInt8*)segmentOrig,(UInt8*)segmentShort))
				{
					puts("Error in MapShortName.\r\n");
					zfree(file);
					zfree(segmentShort);
					zfree(segmentOrig);
					return null;
				}
				puts("Finding entry in directory.\r\n");
				
				for (j = 0; j < 11; j++) printf("%c",segmentShort[j]);
				j = 0;
				puts("\r\n");
				
				// Find the entry, if possible
				while (j < entryCount)
				{
					if (firstCluster)
					{
						puts("Using cluster chain IO.\r\n");
						FAT_ClusterChainIO(vol,true,firstCluster,j * sizeof(DirEntry),(UInt8*)&(file->Entry),sizeof(DirEntry));
					}
					else
					{
						puts("Reading partition directly.\r\n");
						// If this is a FAT12/16 volume and we are searching
						// the root directory, we can't use cluster chain IO
						printf("FirstDataSector: %u\r\nRootDirSectors: %u\r\nj: %d\r\n",vol->FirstDataSector,vol->RootDirSectors,j);
						InternalReadPart(vol->Partition,(vol->FirstDataSector - vol->RootDirSectors) * vol->BPB_BytsPerSec + j * sizeof(DirEntry),(UInt8*)&(file->Entry),sizeof(DirEntry));
						puts("Read partition successfully.\r\n");
					}
					if ((file->Entry.Name[0] != 0xE5) && (file->Entry.Name[0] > 0x20 || file->Entry.Name[0] == 0x5))
					{
						puts("Inspecting file entry:\r\n");
						for (k = 0; k < 11; k++) printf("%c",file->Entry.Name[k]);
						puts("\r\n");
						if (!memcmp(file->Entry.Name,segmentShort,11))
						{
							// This is the entry we were looking for from the start
							if (i == pathLen - 1)
							{
								puts("File found.\r\n");
								printf("File size: %ld\r\n",file->Entry.FileSize);
								printf("First cluster: %ld\r\n",(UInt32)(file->Entry.FstClustLO) & ((UInt32)(file->Entry.FstClustHI) << 16));
								zfree(segmentShort);
								zfree(segmentOrig);
								return file;
							}
							break;
						}
					}
					j++;
				}
				zfree(segmentShort);
				zfree(segmentOrig);
			}
			segmentLength = 0;
			segmentStart = &(path[i + 1]);
		}
		else
		{
			segmentLength++;
		}
	}
	
	return null;
}

Bool FAT_FileExists(PartInternal* device, char* path)
{
	if (device->FileSystem && device->Data1)
	{
		FATVolume* vol = device->Data1;
		FATFile* file = FAT_FindFile(vol,path,strlen(path));
		Int16 ret = file ? True : False;
		zfree(file);
		return ret;
	}
	return False;
}

// Gets a file's (or dir's) attributes
Int16 FAT_GetFile(FileInternal* file, char** reparse)
{
	FATFile* ffile;
	printf("FAT_GetFile(%s)\r\n",file->Name);
	ffile = FAT_FindFile((FATVolume*)(file->Partition->Data1),file->Name,strlen(file->Name));
	
	if (!ffile) return ErrorNotFound;
	
	file->Data1 = ffile;
	file->Attributes = ((ffile->Entry.Attr & ATTR_READ_ONLY) ? ReadOnly : 0) | 
						((ffile->Entry.Attr & ATTR_SYSTEM) ? System : 0) |
						((ffile->Entry.Attr & ATTR_HIDDEN) ? Hidden : 0) |
						((ffile->Entry.Attr & ATTR_ARCHIVE) ? Archive : 0);
	return ErrorSuccess;
}

// Opens a file for IO
Int16 FAT_OpenFile(FileInternal* file, FileMode mode)
{
	return ErrorSuccess;
}

// Sets file attributes
Int16 FAT_SetFile(FileInternal* file)
{
	return ErrorUnimplemented;
}

Int16 FAT_DeleteFile(FileInternal* file)
{
	return ErrorUnimplemented;
}

Int16 FAT_RenameFile(FileInternal* file, char* path)
{
	return ErrorUnimplemented;
}
