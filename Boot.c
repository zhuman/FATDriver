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
	
	funcs.Detect = DetectFS;
	funcs.MountDevice = MountDevice;
	funcs.UnmountDevice = UnmountDevice;
	funcs.FileExists = FileExists;
	funcs.GetFile = GetFile;
	funcs.OpenFile = OpenFile;
	funcs.SetFile = SetFile;
	funcs.DeleteFile = DeleteFile;
	funcs.RenameFile = RenameFile;
	funcs.ReadFile = ReadFile;
	funcs.WriteFile = WriteFile;
	
	RegisterFileSystem(info,funcs);
	
	puts("FAT inited successfully.\r\n");
}
