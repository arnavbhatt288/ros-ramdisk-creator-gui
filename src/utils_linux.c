
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <blkid/blkid.h>
#include <sys/statvfs.h>
#include <pthread.h>
#include <signal.h>
#include "common.h"
#include "utils_linux.h"

static pthread_t thread_id;

bool is_partition(char str[])
{
    return isdigit(str[strlen(str) - 2]);
}

bool copy_file(char* to, char* from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return false;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do
        {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        }
        while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return true;
    }

  out_error:
    perror("CopyFile failed!");

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    return false;
}

bool read_volume_sector(FileHandle* fhandle, long SectorNumber, void* SectorBuffer)
{
    int number_of_bytes_read;
    int file_position;
    
    file_position = lseek(fhandle->file, (SectorNumber* 512), SEEK_SET);
    
    if (file_position != (SectorNumber * 512))
    {
        perror("read_volume_sector() failed!");
        return false;
    }
    
    number_of_bytes_read = read(fhandle->file, SectorBuffer, 512);
    
    if (number_of_bytes_read != 512)
    {
        perror("read_volume_sector() failed!");
        return false;
    }
    
    return true;
}

bool write_volume_sector(FileHandle* fhandle, long SectorNumber, void* SectorBuffer)
{
    int number_of_bytes_written;
    int file_position;
    
    file_position = lseek(fhandle->file, (SectorNumber * 512), SEEK_SET);
    
    if (file_position != (SectorNumber * 512))
    {
        perror("write_volume_sector() failed!");
        return false;
    }
    
    number_of_bytes_written = write(fhandle->file, SectorBuffer, 512);
    
    if (number_of_bytes_written != 512)
    {
        perror("write_volume_sector() failed!");
        return false;
    }
    
    return true;
}

/* stolen from youknowwhat site with slight modications */
char *string_remove(char *str, const char *sub) {
    char *p, *q, *r;
    if (*sub && (q = r = strstr(str, sub)) != NULL) {
        int len = strlen(sub);
        while ((r = strstr(p = r + len, sub)) != NULL) {
            while (p < r)
                *q++ = *p++;
        }
        while ((*q++ = *p++) != '\0')
            continue;
    }
    return str;
}

int count_line(FILE *fp)
{
    char temp[128];
    int count_lines = 0;

    while (fgets(temp, 128, fp) != NULL)
    {
        //Count whenever new line is encountered
        if (is_partition(temp))
        {
            count_lines++;
        };
    }

    fseek(fp, 0, SEEK_SET);
    return count_lines;
}

void gather_partitions(utils_variables *pvariables)
{
    FILE *fp;
    char temp[128];
    int count = 0;

    fp = fopen("/proc/partitions", "r");

    if (fp == NULL)
    {
        printf("Error: File is unopenable.");
        return;
    }

    pvariables->partitions_amount = count_line(fp);
    char drives_tmp[pvariables->partitions_amount][128];

    while (fgets(temp, 128, fp) != NULL)
    {
        if (is_partition(temp))
        {
            char *ret;

            ret = strrchr(temp, ' ');
            ret++;
            ret[strlen(ret) - 1] = '\0';

            strcpy(drives_tmp[count ], "/dev/");
            strcat(drives_tmp[count], ret);
            count++;
        }
    }
    fclose(fp);

    pvariables->partitions = (char**) malloc(pvariables->partitions_amount * sizeof(char*));
    pvariables->partitions[0] = malloc(pvariables->partitions_amount * 12 * sizeof(char));
    for (count = 0; count < pvariables->partitions_amount; count++)
    {
        pvariables->partitions[count] = (char*) malloc(strlen(drives_tmp[count]) * sizeof(char));
        strcpy(pvariables->partitions[count], drives_tmp[count]);
    }
}

char* gather_mountpoint(char *partition)
{
    FILE *fp;
    char temp[128];
    char *tempptr, *tempptr2, *tempptr3, *tempptr4, *path = "";

    fp = fopen("/etc/mtab", "r");

    if (fp == NULL)
    {
        printf("Error: File is unopenable.");
        return NULL;
    }

    while (fgets(temp, 128, fp) != NULL)
    {
        tempptr = strstr(temp, partition);
        if (tempptr)
        {
            tempptr2 = strchr(tempptr, ' ' );
            tempptr2++;
            tempptr3 = strchr(tempptr2, ' ' );
            tempptr4 = string_remove(tempptr2, tempptr3);
            path = malloc(strlen(tempptr4));
            strcpy(path, tempptr4);
        }
    }

    return path;
}

void gather_partition_info(utils_variables *pvariables)
{
    blkid_probe pr;
    struct statvfs st;
    const char *label = "";
    const char *type = "";
    unsigned long long size = 0;

    pr = blkid_new_probe_from_filename(pvariables->partitions[pvariables->current_partition]);
    blkid_do_fullprobe(pr);
    blkid_probe_lookup_value(pr, "LABEL", &label, NULL);
    blkid_probe_lookup_value(pr, "TYPE", &type, NULL);

    pvariables->fs_type = malloc(strlen(type));
    pvariables->label = malloc(strlen(label));
    strcpy(pvariables->fs_type, type);
    strcpy(pvariables->label, label);

    blkid_free_probe(pr);

    pvariables->mounted_path = gather_mountpoint(pvariables->partitions[pvariables->current_partition]);

    if (pvariables->mounted_path != NULL && strlen(pvariables->mounted_path) != 0)
    {
        pvariables->is_partition_mounted = true;
        statvfs(pvariables->mounted_path, &st);
        size = st.f_bavail * st.f_bsize;
        pvariables->free_size = (double)size / (1024 * 1024);
    }
    else
    {
        pvariables->is_partition_mounted = false;
    }
}

void *thread_start_process(void *vargp)
{
    utils_variables *pvariables = (utils_variables *)vargp;
    
    strcpy(pvariables->status, "Installing Bootloader");
    pvariables->progress = 20;
    if (install_boot_sector(pvariables))
    {
        strcpy(pvariables->status, "Copying Files");
        if (copy_required_files(pvariables))
        {
            if (pvariables->generate_ini == 0)
            {
                strcpy(pvariables->status, "Generating freeldr.ini");
                if(!generate_freeldr_ini(pvariables))
                {
                    strcpy(pvariables->status, "Failed!");
                }
            }
            pvariables->progress = 100;
            strcpy(pvariables->status, "Success!");
        }
        else
        {
            strcpy(pvariables->status, "Failed!");
        }
    }
    else
    {
       strcpy(pvariables->status, "Failed!");
    }

    pthread_exit(NULL);
}

FileHandle* open_volume(char* VolumeName)
{
    FileHandle* fhandle = malloc(sizeof(*fhandle));
    printf("Opening volume %s\n", VolumeName);
    
    fhandle->file = open(VolumeName, O_RDWR | O_SYNC);
    
    if (fhandle->file < 0)
    {
        perror("open_volume() failed!");
        return NULL;
    }
    
    return fhandle;
}

void start_thread(utils_variables *pvariables)
{
    pthread_create(&thread_id, NULL, thread_start_process, (void *)pvariables);
    pvariables->is_thread_active = true;
}

void end_thread()
{
    pthread_cancel(thread_id);
}

void close_volume(FileHandle* fhandle)
{
    close(fhandle->file);
    fhandle->file = -1;
    free(fhandle);
}
