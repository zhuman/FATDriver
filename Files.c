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

// Parses and traces a file path through each directory to the requested entry
// Returns null on any error or if the file cannot be found
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
		file->Entry.FileSize = vol->BPB_RootEntCnt * sizeof(DirEntry);
	}
	
	for (i = 0; i < pathLen; i++)
	{
		if ((path[i] == '\\') || (i == pathLen - 1))
		{
			if (i == pathLen - 1) segmentLength++;
			puts("Segment started.\r\n");
			if (segmentLength > 12)
			{
				puts("Bad file/folder name.\r\n");
				zfree(file);
				return null;
			}
			else if (segmentLength)
			{
				UInt32 j = 0; UInt16 k = 0;
				UInt32 firstCluster = (UInt32)(file->Entry.FstClustLO) | (((UInt32)(file->Entry.FstClustHI)) << 16);
				UInt32 entryCount = file->Entry.FileSize / sizeof(DirEntry);
				char* segmentShort = zmalloc(11);
				char* segmentOrig = zmalloc(13);
				
				// ErrorOutOfMemory check
				if (!segmentShort || !segmentOrig)
				{
					if (segmentShort) zfree(segmentShort);
					if (segmentOrig) zfree(segmentOrig);
					zfree(file);
					return null;
				}
				
				// Convert the path segment into the 11-byte format used for directory entries
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
				
				// Print out the 11-byte form of the name
				puts("Finding entry in directory:\r\n");
				for (j = 0; j < 11; j++) printf("%c",segmentShort[j]);
				j = 0;
				puts("\r\n");
				printf("entryCount: %lu\r\n",entryCount);
				
				// Find the entry, if possible
				for(;;)
				{
					if (firstCluster)
					{
						UInt32 bytesRemaining;
						puts("Using cluster chain IO.\r\n");
						if (FAT_ClusterChainIO(vol,true,firstCluster,j * sizeof(DirEntry),(UInt8*)&(file->Entry),sizeof(DirEntry),&bytesRemaining) || bytesRemaining)
						{
							zfree(file);
							zfree(segmentShort);
							zfree(segmentOrig);
							return null;
						}
					}
					else
					{
						if (j >= entryCount)
						{
							zfree(file);
							zfree(segmentShort);
							zfree(segmentOrig);
							return null;
						}
						puts("Reading partition directly.\r\n");
						// If this is a FAT12/16 volume and we are searching
						// the root directory, we can't use cluster chain IO
						//printf("FirstDataSector: %u\r\nRootDirSectors: %u\r\nj: %ld\r\n",vol->FirstDataSector,vol->RootDirSectors,j);
						InternalReadPart(vol->Partition,(((UInt32)(vol->FirstDataSector) - (UInt32)(vol->RootDirSectors)) * (UInt32)(vol->BPB_BytsPerSec)) + (j * sizeof(DirEntry)),(UInt8*)&(file->Entry),sizeof(DirEntry));
					}
					
					puts("Inspecting file entry:\r\n");
					for (k = 0; k < 11; k++) if (file->Entry.Name[k] >= 0x20) printf("%c",file->Entry.Name[k]);
					puts("\r\n");
					
					// If the entry is valid, compare its name to the current segment
					if ((file->Entry.Name[0] != 0xE5) && 												// Don't look at empty file entries
						(file->Entry.Name[0] > 0x20 || file->Entry.Name[0] == 0x5) && 					// Only look at valid file names
						((file->Entry.Attr & ATTR_LONG_NAME) != ATTR_LONG_NAME) && 						// Don't inspect long file names
						((file->Entry.Attr & ATTR_VOLUME_ID) != ATTR_VOLUME_ID) && 						// Don't check the volume ID
						(((file->Entry.Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY) || i == pathLen - 1))	// Check only for directories until the last segment
					{
						// If this entry's name matches the segment
						if (!memcmp(file->Entry.Name,segmentShort,11))
						{
							puts("Entry found.\r\n");
							printf("File size: %ld\r\n",file->Entry.FileSize);
							printf("First cluster: %ld\r\n",((UInt32)(file->Entry.FstClustLO)) | (((UInt32)(file->Entry.FstClustHI)) << 16));
							printf("Creation date: %x Time: %x Written date: %x Time: %x\r\n",file->Entry.CrtDate,file->Entry.CrtTime,file->Entry.WriteDate,file->Entry.WriteTime);
							
							for (k = 0; k < 32; k++) printf("%x ",((UInt8*)(&file->Entry))[k]);
							puts("\r\n");
							
							// If this is the last segment of the path, return the found file entry
							if (i == pathLen - 1)
							{
								puts("File found.\r\n");
								zfree(segmentShort);
								zfree(segmentOrig);
								return file;
							}
							else
							{
								// If the folder is empty, we can't go any farther
								if ((!(file->Entry.FstClustLO)) && (!(file->Entry.FstClustHI)))
								{
									puts("Empty folder found. Invalid file name.\r\n");
									zfree(segmentShort);
									zfree(segmentOrig);
									return null;
								}
								else
								{
									puts("On to the next segment...\r\n");
									break;
								}
							}
						}
					}
					else puts("Skipped entry.\r\n");
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
	FATFile* fatFile;
	printf("FAT_GetFile(%s)\r\n",file->Name);
	fatFile = FAT_FindFile((FATVolume*)(file->Partition->Data1),file->Name,strlen(file->Name));
	if (file)
	{
		file->Data1 = fatFile;
		file->FileLength = fatFile->Entry.FileSize;
		file->IsDirectory = fatFile->Entry.Attr & ATTR_DIRECTORY;
		file->Attributes = ((fatFile->Entry.Attr & ATTR_READ_ONLY) ? ReadOnly : 0) | 
								((fatFile->Entry.Attr & ATTR_HIDDEN) ? Hidden : 0) | 
								((fatFile->Entry.Attr & ATTR_SYSTEM) ? System : 0) | 
								((fatFile->Entry.Attr & ATTR_ARCHIVE) ? Archive : 0);
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidFileName;
	}
}

// Opens a file for IO
Int16 FAT_OpenFile(FileInternal* file, FileMode mode)
{
	FATFile* fatFile = file->Data1;
	if (fatFile)
	{
		return ErrorSuccess;
	}
	else
	{
		return ErrorUnknown;
	}
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
