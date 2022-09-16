
#ifdef _MSC_VER
    #pragma warning(disable:4996)
#endif

#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <tchar.h>

#include "common.h"
#include "utils_win.h"

static HANDLE hthread;

FileHandle* open_volume(LPCTSTR volume_name)
{
    TCHAR realvolumename[MAX_PATH];
    FileHandle* fhandle = GlobalAlloc(GPTR, sizeof(*fhandle));

    _tcscpy(realvolumename, _T("\\\\.\\"));
    _tcscat(realvolumename, volume_name);
    realvolumename[strlen(realvolumename) - 1] = '\0';

    _tprintf(_T("Opening volume \'%s\'\n"), volume_name);

    fhandle->file = CreateFile(realvolumename,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);

    if (fhandle->file == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("%s:%d: "), __FILE__, __LINE__);
        _tprintf(_T("Failed. Error code %ld.\n"), GetLastError());
        return NULL;
    }

    return fhandle;
}

bool read_volume_sector(FileHandle* fhandle, ULONG sector_number, PVOID sector_buffer)
{
    DWORD dwnumberofbytesread;
    DWORD dwfileposition;
    int bretval;

    //
    // FIXME: this doesn't seem to handle the situation
    // properly when sector_number is bigger than the
    // amount of sectors on the disk. Seems to me that
    // the call to SetFilePointer() should just give an
    // out of bounds error or something but it doesn't.
    //
    dwfileposition = SetFilePointer(fhandle->file, (sector_number * 512), NULL, FILE_BEGIN);
    if (dwfileposition != (sector_number * 512))
    {
        _tprintf(_T("%s:%d: "), __FILE__, __LINE__);
        _tprintf(_T("SetFilePointer() failed. Error code %ld.\n"), GetLastError());
        return false;
    }

    bretval = ReadFile(fhandle->file, sector_buffer, 512, &dwnumberofbytesread, NULL);
    if (!bretval || (dwnumberofbytesread != 512))
    {
        _tprintf(_T("%s:%d: "), __FILE__, __LINE__);
        _tprintf(_T("ReadFile() failed. Error code %ld.\n"), GetLastError());
        return false;
    }

    return true;
}

bool write_volume_sector(FileHandle* fhandle, ULONG sector_number, PVOID sector_buffer)
{
    DWORD dwnumberofbyteswritten;
    DWORD dwfileposition;
    int bretval;

    dwfileposition = SetFilePointer(fhandle->file, (sector_number * 512), NULL, FILE_BEGIN);
    if (dwfileposition != (sector_number * 512))
    {
        _tprintf(_T("%s:%d: "), __FILE__, __LINE__);
        _tprintf(_T("SetFilePointer() failed. Error code %ld.\n"), GetLastError());
        return false;
    }

    bretval = WriteFile(fhandle->file, sector_buffer, 512, &dwnumberofbyteswritten, NULL);
    if (!bretval || (dwnumberofbyteswritten != 512))
    {
        _tprintf(_T("%s:%d: "), __FILE__, __LINE__);
        _tprintf(_T("WriteFile() failed. Error code %ld.\n"), GetLastError());
        return false;
    }

    return true;
}

DWORD WINAPI thread_start_process(LPVOID lpParameter)
{
    utils_variables *pvariables = (utils_variables *)lpParameter;
    
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
                    goto exit;
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
    pvariables->is_thread_active = false;

exit:
    CloseHandle(hthread);
    return 0;
}

void close_volume(FileHandle* fhandle)
{
    CloseHandle(fhandle->file);
    GlobalFree(fhandle);
}

void end_thread()
{
    TerminateThread(hthread, 0);
    CloseHandle(hthread);
}

void gather_partitions(utils_variables *pvariables)
{
    DWORD dwcountdrives = 0;
    char szlogicaldrives[MAX_PATH] = { 0 };
    int count = 0;

    dwcountdrives = GetLogicalDriveStrings(MAX_PATH, szlogicaldrives);
    if (dwcountdrives > 0 && dwcountdrives <= MAX_PATH)
    {
        char* szSingleDrive = szlogicaldrives;
        while (*szSingleDrive)
        {
            if (GetDriveType(szSingleDrive) != 5)
            {
                pvariables->partitions_amount++;
            }
            szSingleDrive += strlen(szSingleDrive) + 1;
        }
    }

    pvariables->partitions = (char**)malloc(pvariables->partitions_amount * sizeof(char*));
    pvariables->partitions[0] = malloc(pvariables->partitions_amount * 3 * sizeof(char));

    if (dwcountdrives > 0 && dwcountdrives <= MAX_PATH)
    {
        char* szSingleDrive = szlogicaldrives;
        while (*szSingleDrive)
        {
            if (GetDriveType(szSingleDrive) != 5)
            {
                pvariables->partitions[count] = (char*)malloc(strlen(szSingleDrive) * sizeof(char));
                strcpy(pvariables->partitions[count], szSingleDrive);
                count++;
            }
            szSingleDrive += strlen(szSingleDrive) + 1;
        }
    }
}

void gather_partition_info(utils_variables *pvariables)
{
    char tempfs[MAX_PATH] = {0};
    char templabel[MAX_PATH] = {0};
    ULARGE_INTEGER ulF;

    ZeroMemory(&ulF, sizeof(ulF));
    GetDiskFreeSpaceEx(pvariables->partitions[pvariables->current_partition], NULL, NULL, &ulF);
    pvariables->free_size = (size_t)ulF.QuadPart / 1000000;

    GetVolumeInformation(pvariables->partitions[pvariables->current_partition], templabel, MAX_PATH, NULL, NULL, NULL, tempfs, MAX_PATH);
    pvariables->fs_type = malloc(strlen(tempfs));
    pvariables->label = malloc(strlen(templabel));
    strcpy(pvariables->fs_type, tempfs);
    strcpy(pvariables->label, templabel);
    pvariables->is_partition_mounted = pvariables->partitions[pvariables->current_partition];
}

void start_thread(utils_variables *pvariables)
{
    hthread = CreateThread(0, 0, thread_start_process, (LPVOID)pvariables, 0, 0);
    pvariables->is_thread_active = true;
}
