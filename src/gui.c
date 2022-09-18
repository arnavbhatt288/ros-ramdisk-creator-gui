
#define countof(array) (sizeof(array) / sizeof(array[0]))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "nuklear.h"
#include "common.h"
#include "gui.h"
#include "tinyfiledialogs.h"

#ifdef _WIN32
    #ifdef _MSC_VER
        #pragma warning(disable:4996)
    #endif
    #include <windows.h>
    #include "utils_win.h"
#elif __linux__
    #include <strings.h>
    #include "utils_linux.h"
#endif

void start_gui(struct nk_context *ctx, int width, int height, utils_variables *pvariables)
{
    static bool close_program = false;
    static char *temp;
    static int current_partition;

    current_partition = pvariables->current_partition;

    if (nk_begin(ctx, "Main", nk_rect(0, 0, (float)width, (float)height), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_begin(ctx, NK_STATIC, 30, 4);
        {
            nk_layout_row_push(ctx, 100);
            nk_label(ctx, "Select a drive:", NK_TEXT_LEFT);

            nk_layout_row_push(ctx, 100);
            pvariables->current_partition = nk_combo(ctx, (const char **)pvariables->partitions, pvariables->partitions_amount, pvariables->current_partition, 25, nk_vec2(100,100));
            if (current_partition != pvariables->current_partition)
            {
                gather_partition_info(pvariables);
            }
            
            nk_layout_row_push(ctx, 50);
            if (nk_button_label(ctx, "Reload"))
            {
                pvariables->partitions_amount = 0;
                gather_partitions(pvariables);

                if ((pvariables->current_partition + 1) > pvariables->partitions_amount)
                {
                    pvariables->current_partition = 0;
                    gather_partition_info(pvariables);
                }
            }
        }
        nk_layout_row_end(ctx);

        nk_layout_row_static(ctx, 108, 332, 1);
        if (nk_group_begin(ctx, "Partition Info", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE))
        {
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
            {
                nk_layout_row_push(ctx, 120);
                nk_label(ctx, "File System:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 120);
                nk_label(ctx, pvariables->fs_type, NK_TEXT_RIGHT);
            }
            nk_layout_row_end(ctx);
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
            {
                nk_layout_row_push(ctx, 120);
                nk_label(ctx, "Label:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 120);
                nk_label(ctx, pvariables->label, NK_TEXT_RIGHT);
            }
            nk_layout_row_end(ctx);

            nk_group_end(ctx);
        }

        nk_layout_row_static(ctx, 30, 110, 1);
        nk_checkbox_label(ctx, "Generate INI", &pvariables->generate_ini);

        nk_layout_row_static(ctx, 30, 110, 1);
        nk_label(ctx, "Select bootcd.iso:", NK_TEXT_LEFT);

        nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
        {
            nk_layout_row_push(ctx, 260);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, pvariables->path[BOOTCD_PATH], &pvariables->path_len[BOOTCD_PATH], 260, nk_filter_default);
            nk_layout_row_push(ctx, 10);
            nk_label(ctx, "", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 50);
            if (nk_button_label(ctx, "Open"))
            {
                temp = gather_path_of_file("Select a bootcd.iso", ".iso files", BOOTCD_PATH, true);
                strcpy(pvariables->path[BOOTCD_PATH], temp);
                pvariables->path_len[BOOTCD_PATH] = strlen(pvariables->path[BOOTCD_PATH]);
            }
        }
        nk_layout_row_end(ctx);
        
        nk_layout_row_static(ctx, 30, 110, 1);
        nk_label(ctx, "Select livecd.iso:", NK_TEXT_LEFT);

        nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
        {
            nk_layout_row_push(ctx, 260);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, pvariables->path[LIVECD_PATH], &pvariables->path_len[LIVECD_PATH], 260, nk_filter_default);
            nk_layout_row_push(ctx, 10);
            nk_label(ctx, "", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 50);
            if (nk_button_label(ctx, "Open"))
            {
                temp = gather_path_of_file("Select a livecd.iso", ".iso files", LIVECD_PATH, true);
                strcpy(pvariables->path[LIVECD_PATH], temp);
                pvariables->path_len[LIVECD_PATH] = strlen(pvariables->path[LIVECD_PATH]);
            }
        }
        nk_layout_row_end(ctx);

        nk_layout_row_static(ctx, 30, 110, 1);
        nk_label(ctx, "Select freeldr.sys:", NK_TEXT_LEFT);

        nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
        {
            nk_layout_row_push(ctx, 260);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, pvariables->path[FREELDRSYS_PATH], &pvariables->path_len[FREELDRSYS_PATH], 260, nk_filter_default);
            nk_layout_row_push(ctx, 10);
            nk_label(ctx, "", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 50);
            if (nk_button_label(ctx, "Open"))
            {
                temp = gather_path_of_file("Select a freeldr.sys", ".sys files", LIVECD_PATH, false);
                strcpy(pvariables->path[FREELDRSYS_PATH], temp);
                pvariables->path_len[FREELDRSYS_PATH] = strlen(pvariables->path[FREELDRSYS_PATH]);
            }
        }
        nk_layout_row_end(ctx);

        nk_layout_row_static(ctx, 15, 0, 1);
        nk_label(ctx, "", NK_TEXT_LEFT);
        nk_layout_row_static(ctx, 20, 330, 1);
        nk_progress(ctx, &pvariables->progress, 100, nk_false);
        nk_layout_row_static(ctx, 15, 0, 1);
        nk_label(ctx, "", NK_TEXT_LEFT);

        nk_layout_space_begin(ctx, NK_STATIC, 30, 2);
        {
            nk_layout_space_push(ctx, nk_rect(120, 0, 100, 30));
            if (!pvariables->is_thread_active)
            {
                if (nk_button_label(ctx, "Start"))
                {
                    if (pvariables->free_size < 128)
                    {
                        tinyfd_messageBox("Error", "Partition size is too small", "ok", "error", 1);
                    }
                    else if (pvariables->path_len[FREELDRSYS_PATH] == 0 || (pvariables->path_len[BOOTCD_PATH] == 0 && pvariables->path_len[LIVECD_PATH] == 0))
                    {
                        tinyfd_messageBox("Error", "Please select required files", "ok", "error", 1);
                    }
                    if (!pvariables->is_partition_mounted)
                    {
                        tinyfd_messageBox("Error", "Partition is not mounted", "ok", "error", 1);
                    }
                    else if (tinyfd_messageBox("Confirmation", "Are you sure to continue?", "yesno", "exclamation", 1))
                    {
                        start_thread(pvariables);
                    }
                }
            }
            else
            {
                if (nk_button_label(ctx, "Stop"))
                {
                    end_thread();
                    strcpy(pvariables->status, "Stopped");
                    pvariables->is_thread_active = false;
                }
            }
            nk_layout_space_push(ctx, nk_rect(230, 0, 100, 30));
            if (nk_button_label(ctx, "Exit"))
            {
                close_program = true;
            }
        }
        nk_layout_space_end(ctx);

        nk_layout_row_static(ctx, 30, 160, 1);
        nk_label(ctx, pvariables->status, NK_TEXT_LEFT);
    }
    nk_end(ctx);
    
    if (close_program)
    {
        nk_window_close(ctx, "Main");
    }
}
