#include "..\Z-OS\Z-OS.h"
#include "..\Z-OS\Devices\Partition.h"
#include "..\Z-OS\Devices\FileSystems.h"
#include "Boot.h"
#include "Structures.h"

void InitFAT(void)
{
	FileSystemInfo info;
	FileSystemFuncs funcs;
	
	strcpy(info.Name,"FAT");
	info.HasSecurity = False;
	info.IsCaseSensitive = False;
	
	funcs.Detect = FAT_DetectFS;
	funcs.MountDevice = FAT_MountDevice;
	funcs.UnmountDevice = FAT_UnmountDevice;
	funcs.FileExists = FAT_FileExists;
	funcs.GetFile = FAT_GetFile;
	funcs.OpenFile = FAT_OpenFile;
	funcs.SetFile = FAT_SetFile;
	funcs.DeleteFile = FAT_DeleteFile;
	funcs.RenameFile = FAT_RenameFile;
	funcs.ReadFile = FAT_ReadFile;
	funcs.WriteFile = FAT_WriteFile;
	
	RegisterFileSystem(info,funcs);
	
	puts("FAT inited successfully.\r\n");
}
