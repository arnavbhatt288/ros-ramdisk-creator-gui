#define countof(array) (sizeof(array) / sizeof(array[0]))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "data.h"
#include "common.h"
#include "tinyfiledialogs.h"

#ifdef _WIN32
    #ifdef _MSC_VER
        #pragma warning(disable:4996)
        #define strncasecmp _strnicmp
        #define strcasecmp _stricmp
    #endif

    #include <windows.h>
    #include "utils_win.h"
#else
	#include <strings.h>
    #include "utils_linux.h"
#endif

bool create_ini(char *pacPath, char *pacTopic, char *pacItem, char *pacValue)
{
    int iFoundTopic;
    int iFoundItem;
    int iError;
    int iItemLength;
    int iValueLength;
    char acTopicHeading[80];
    char acItemHeading[80];
    char acIniLine[160];
    char acIniPath[160];
    char acTempPath[160];
    FILE *pFIniFile;
    FILE *pFTempIni;

    iError = true;

    strcpy(acIniPath, pacPath);

    strcpy(acTempPath, pacPath);
    strcat(acTempPath, "temp");

    // add brackets to topic
    strcpy(acTopicHeading, "[");
    strcat(acTopicHeading, pacTopic);
    strcat(acTopicHeading, "]\n");

    strcpy(acItemHeading, pacItem);
    strcat(acItemHeading, "=");

    iItemLength = strlen(acItemHeading);
    iValueLength = strlen(pacValue);

    iFoundTopic = 0;
    iFoundItem = 0;

    if ((pFTempIni = fopen (acTempPath, "w")) == NULL)
    {
        printf ("Could not open temp ini file to write settings\n");
        iError = false;
        return(iError);
    }

    // try to open current config file for reading
    if ((pFIniFile = fopen (acIniPath, "r")) != NULL)
    {
        // read a line from the config file until EOF
        while (fgets(acIniLine, 159, pFIniFile) != NULL)
        {
            // the item has been found so continue reading file to end
            if (iFoundItem == 1)
            {
                fputs (acIniLine, pFTempIni);
                continue;
            }
            // topic has not been found yet
            if (iFoundTopic == 0)
            {
                if (strcmp(acTopicHeading, acIniLine) == 0)
                {
                    // found the topic
                    iFoundTopic = 1;
                }
                fputs (acIniLine, pFTempIni);
                continue;
            }
            // the item has not been found yet
            if ((iFoundItem == 0) && (iFoundTopic == 1))
            {
                if ( strncmp (acItemHeading, acIniLine, iItemLength) == 0)
                {
                    // found the item
                    iFoundItem = 1;
                    if (iValueLength > 0)
                    {
                        fputs (acItemHeading, pFTempIni);
                        fputs (pacValue, pFTempIni);
                        fputs ("\n", pFTempIni);
                    }
                    continue;
                }
                // if newline or [, the end of the topic has been reached
                // so add the item to the topic
                if ((acIniLine[0] == '\n') || (acIniLine[0] == '['))
                {
                    iFoundItem = 1;
                    if ( iValueLength > 0)
                    {
                        fputs(acItemHeading, pFTempIni);
                        fputs(pacValue, pFTempIni);
                        fputs("\n", pFTempIni);
                    }
                    fputs("\n", pFTempIni);
                    if(acIniLine[0] == '[')
                    {
                        fputs(acIniLine, pFTempIni);
                    }
                    continue;
                }
                // if the item has not been found, write line to temp file
                if(iFoundItem == 0)
                {
                    fputs (acIniLine, pFTempIni);
                    continue;
                }
            }
        }
        fclose(pFIniFile);
    }
    // still did not find the item after reading the config file
    if (iFoundItem == 0)
    {
        // config file does not exist
        // or topic does not exist so write to temp file
        if (iValueLength > 0)
        {
            if (iFoundTopic == 0)
            {
                fputs(acTopicHeading, pFTempIni);
            }
            fputs(acItemHeading, pFTempIni);
            fputs(pacValue, pFTempIni);
            fputs("\n\n", pFTempIni);
        }
    }

    fclose(pFTempIni);

    //delete the ini file
    remove(acIniPath);

    // rename the temp file to ini file
    rename(acTempPath, acIniPath);

    return iError;
}

bool copy_required_files(utils_variables *pvariables)
{
    bool ret = false;
    char temp_path[512];

#ifdef _WIN32
    sprintf(temp_path, "%sfreeldr.sys", pvariables->partitions[pvariables->current_partition]);
    ret = CopyFile(pvariables->path[FREELDRSYS_PATH], temp_path, TRUE);
    pvariables->progress = 40;

    if (pvariables->path_len[BOOTCD_PATH] != 0)
    {
        sprintf(temp_path, "%sbootcd.iso", pvariables->partitions[pvariables->current_partition]);
        ret = CopyFile(pvariables->path[BOOTCD_PATH], temp_path, TRUE);
        pvariables->progress = 60;
    }
    if (pvariables->path_len[LIVECD_PATH] != 0)
    {
        sprintf(temp_path, "%slivecd.iso", pvariables->partitions[pvariables->current_partition]);
        ret = CopyFile(pvariables->path[LIVECD_PATH], temp_path, TRUE);
        pvariables->progress = 80;
    }
#else
    sprintf(temp_path, "%s/freeldr.sys", pvariables->mounted_path);
    ret = copy_file(temp_path, pvariables->path[FREELDRSYS_PATH]);
    pvariables->progress = 40;

    if (pvariables->path_len[BOOTCD_PATH] != 0)
    {
        sprintf(temp_path, "%s/bootcd.iso", pvariables->mounted_path);
        ret = copy_file(temp_path, pvariables->path[BOOTCD_PATH]);
        pvariables->progress = 60;
    }
    if (pvariables->path_len[LIVECD_PATH] != 0)
    {
        sprintf(temp_path, "%s/livecd.iso", pvariables->mounted_path);
        ret = copy_file(temp_path, pvariables->path[LIVECD_PATH]);
        pvariables->progress = 80;
    }
#endif

    return ret;
}

bool generate_freeldr_ini(utils_variables *pvariables)
{
    bool ret = false;
    char temp_path[512];
    int i = 0;
    
#ifdef _WIN32
    sprintf(temp_path, "%sfreeldr.ini", pvariables->partitions[pvariables->current_partition]);
#else
    sprintf(temp_path, "%s/freeldr.ini", pvariables->mounted_path);
#endif

    for (i = 0; i < countof(MainIniData); i++)
    {
        ret = create_ini(temp_path, MainIniData[i].pacTopic, MainIniData[i].pacItem, MainIniData[i].pacValue);
    }

    if (pvariables->path_len[BOOTCD_PATH] != 0)
    {
        for (i = 0; i < countof(BootCDIniData); i++)
        {
            ret = create_ini(temp_path, BootCDIniData[i].pacTopic, BootCDIniData[i].pacItem, BootCDIniData[i].pacValue);
        }
    }
    
    if (pvariables->path_len[LIVECD_PATH] != 0)
    {
        for (i = 0; i < countof(LiveCDIniData); i++)
        {
            ret = create_ini(temp_path, LiveCDIniData[i].pacTopic, LiveCDIniData[i].pacItem, LiveCDIniData[i].pacValue);
        }
    }

    return ret;
}

bool install_boot_sector(utils_variables *pvariables)
{
    unsigned char BootSectorBuffer[512];
    FileHandle* fhandle;
    
    fhandle = open_volume(pvariables->partitions[pvariables->current_partition]);
    if (fhandle == NULL)
    {
        tinyfd_messageBox("Error", "Error opening the partition", "ok", "error", 1);
        return false;
    }

    if (!read_volume_sector(fhandle, 0, BootSectorBuffer))
    {
        tinyfd_messageBox("Error", "Error reading the partition", "ok", "error", 1);
        return false;
    }

    if (strcasecmp(pvariables->fs_type, "fat32") == 0 || strcasecmp(pvariables->fs_type, "vfat") == 0)
    {
        //
        // Update the BPB in the new boot sector
        //
        memcpy((fat32_data+3), (BootSectorBuffer+3), 87 /*fat32 BPB length*/);

        //
        // Write out new boot sector
        //
        if (!write_volume_sector(fhandle, 0, fat32_data))
        {
            tinyfd_messageBox("Error", "Error writing to the partition", "ok", "error", 1);
            return false;
        }

        //
        // Write out new extra sector
        //
        if (!write_volume_sector(fhandle, 14, (fat32_data+512)))
        {
            tinyfd_messageBox("Error", "Error writing to the partition", "ok", "error", 1);
            return false;
        }
    }

	else if (strcasecmp(pvariables->fs_type, "fat") == 0)
    {
        //
        // Update the BPB in the new boot sector
         //
        memcpy((fat_data+3), (BootSectorBuffer+3), 59 /*fat BPB length*/);

        //
        // Write out new boot sector
        //
        if (!write_volume_sector(fhandle, 0, fat_data))
        {
            tinyfd_messageBox("Error", "Error writing to the partition", "ok", "error", 1);
            return false;
        }
    }

    else
    {
        tinyfd_messageBox("Error", "Unknown filesystem", "ok", "error", 1);
        return false;
    }

    close_volume(fhandle);
    return true;
}

char *gather_path_of_file(const char *title, const char *desc, int which_path, bool is_iso)
{
    const char *iso_filter[] = { "*.iso", "*.ISO" };
    const char *sys_filter[] = { "*.sys", "*.SYS" };
    char *dest = NULL;
    
    if (is_iso)
    {
        dest = tinyfd_openFileDialog(
                        title,
                        "",
                        2,
                        iso_filter,
                        desc,
                        0);
    }
    else
    {
        dest = tinyfd_openFileDialog(
                        title,
                        "",
                        2,
                        sys_filter,
                        desc,
                        0);
    }
    
    if (dest == NULL)
    {
        dest = "";
    }
    
    return dest;
}
