#ifndef _BOOT_FAT_HEADER_
#define _BOOT_FAT_HEADER_

Bool FAT_DetectFS(PartInternal* device);
Int16 FAT_MountDevice(PartInternal* device);
Bool FAT_FileExists(PartInternal* device, char* path);
Int16 FAT_GetFile(FileInternal* file, char** reparse);
Int16 FAT_OpenFile(FileInternal* file, FileMode mode);
Int16 FAT_SetFile(FileInternal* file);
Int16 FAT_DeleteFile(FileInternal* file);
Int16 FAT_RenameFile(FileInternal* file, char* path);
Int16 FAT_ReadFile(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen);
Int16 FAT_WriteFile(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen);
Int16 FAT_UnmountDevice(PartInternal* device, Bool suprise);

#endif
