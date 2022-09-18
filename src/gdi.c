/* nuklear - 1.32.0 - public domain */
#define WIN32_LEAN_AND_MEAN

#ifdef _MSC_VER
    #pragma warning(disable:4996)
#endif

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>

#define WINDOW_WIDTH 340
#define WINDOW_HEIGHT 520

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDI_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_gdi.h"
#include "gdi.h"
#include "common.h"
#include "gui.h"
#include "utils_win.h"

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/

BOOL IsProcessElevated()
{
    BOOL fIsElevated = FALSE;
    HANDLE hToken = NULL;
    TOKEN_ELEVATION elevation;
    DWORD dwSize;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        goto Cleanup;
    }


    if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
    {    
        goto Cleanup;
    }

    fIsElevated = elevation.TokenIsElevated;

Cleanup:
    if (hToken)
    {
        CloseHandle(hToken);
        hToken = NULL;
    }
    return fIsElevated; 
}

void cleanup(utils_variables *pvariables)
{
    int i = 0;
    
    for (i = 0; i < pvariables->partitions_amount; i++)
    {
        free(pvariables->partitions[i]);
    }
    free(pvariables->partitions);
    free(pvariables->fs_type);
    free(pvariables->label);
}

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    if (nk_gdi_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

int start_program(void)
{
    GdiFont* font;
    struct nk_context *ctx;
    utils_variables variables;

    WNDCLASSW wc;
    ATOM atom;
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    DWORD exstyle = WS_EX_APPWINDOW;
    HWND wnd;
    HDC dc;
    int running = 1;
    int needs_refresh = 1;

    /* Check if the program has admin privileges */
    if (!IsProcessElevated())
    {
        MessageBox(NULL, "Run the program as administrator!", "Error", MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }
    else
    {
		pvariables->is_partition_mounted = true;
	}
    
    /* Struct initialization */
    memset(&variables, 0, sizeof variables);
    gather_partitions(&variables);
    gather_partition_info(&variables);
    strcpy(variables.status, "Standby...");
    variables.generate_ini = 1;
    variables.is_thread_active = false;


    /* Win32 */
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"NuklearWindowClass";
    atom = RegisterClassW(&wc);

    AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"ReactOS RAMDisk Creator",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);
    dc = GetDC(wnd);

    /* GUI */
    font = nk_gdifont_create("Arial", 14);
    ctx = nk_gdi_init(font, dc, WINDOW_WIDTH, WINDOW_HEIGHT);

    while (running)
    {
        /* Input */
        MSG msg;
        nk_input_begin(ctx);
        if (needs_refresh == 0) {
            if (GetMessageW(&msg, NULL, 0, 0) <= 0)
                running = 0;
            else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            needs_refresh = 1;
        } else needs_refresh = 0;

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            needs_refresh = 1;
        }
        nk_input_end(ctx);

        /* GUI */
        ctx->style.edit.border = 0;
        start_gui(ctx, WINDOW_WIDTH, WINDOW_HEIGHT, &variables);

        /* Draw */
        nk_gdi_render(nk_rgb(30,30,30));

        if (nk_window_is_closed(ctx, "Main"))
            break;
    }

    cleanup(&variables);
    nk_gdifont_del(font);
    ReleaseDC(wnd, dc);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

