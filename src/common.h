
#ifndef COMMON_H
#define COMMON_H

#define BOOTCD_PATH 0
#define LIVECD_PATH 1
#define FREELDRSYS_PATH 2


typedef struct
{
    bool is_partition_mounted;
    bool is_thread_active;
    char **partitions;
    char path[3][4096];
    char *fs_type;
    char *label;
#ifdef __linux__
    char *mounted_path;
#endif
    char status[64];
    double free_size;
    int current_partition;
    int generate_ini;
    int partitions_amount;
    int path_len[3];
    size_t progress;
} utils_variables;

bool install_boot_sector(utils_variables *pvariables);
bool copy_required_files(utils_variables *pvariables);
bool generate_freeldr_ini(utils_variables *pvariables);
char *gather_path_of_file(const char *title, const char *desc, int which_path, bool is_iso);

#endif
