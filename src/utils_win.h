#ifndef UTILS_WIN_H
#define UTILS_WIN_H

typedef struct
{
    HANDLE file;
} FileHandle;

FileHandle* open_volume(LPCTSTR volume_name);
bool copy_file(char* to, char* from);
bool read_volume_sector(FileHandle* fhandle, ULONG sector_number, PVOID sector_buffer);
bool write_volume_sector(FileHandle* fhandle, ULONG sector_number, PVOID sector_buffer);
void close_volume(FileHandle* fhandle);
void gather_partitions(utils_variables *pvariables);
void gather_partition_info(utils_variables *pvariables);
void start_thread(utils_variables *pvariables);
void end_thread();

#endif