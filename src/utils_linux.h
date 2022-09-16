
#ifndef UTILS_LINUX_H
#define UTILS_LINUX_H

typedef struct
{
    int file;
} FileHandle;

bool copy_file(char* to, char* from);
bool read_volume_sector(FileHandle* fhandle, long SectorNumber, void* SectorBuffer);
bool write_volume_sector(FileHandle* fhandle, long SectorNumber, void* SectorBuffer);
FileHandle* open_volume(char* VolumeName);
void close_volume(FileHandle* fhandle);
void gather_partitions(utils_variables *pvariables);
void gather_partition_info(utils_variables *pvariables);
void start_thread(utils_variables *pvariables);
void end_thread();

#endif
