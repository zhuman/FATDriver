#ifndef _BOOT_FAT_HEADER_
#define _BOOT_FAT_HEADER_

Bool DetectFS(PartInternal* device);
Int16 MountDevice(PartInternal* device);
Bool FileExists(PartInternal* device, char* path);
Int16 GetFile(FileInternal* file, char** reparse);
Int16 OpenFile(FileInternal* file, FileMode mode);
Int16 SetFile(FileInternal* file);
Int16 DeleteFile(FileInternal* file);
Int16 RenameFile(FileInternal* file, char* path);
Int16 ReadFile(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen);
Int16 WriteFile(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen);
Int16 UnmountDevice(PartInternal* device, Bool suprise);

#endif
