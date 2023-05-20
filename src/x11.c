
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_XLIB_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_xlib.h"
#include "x11.h"
#include "common.h"
#include "gui.h"
#include "utils_linux.h"

#define DTIME           20
#define WINDOW_WIDTH    340
#define WINDOW_HEIGHT   520

typedef struct XWindow XWindow;
struct XWindow {
    Display *dpy;
    Window root;
    Visual *vis;
    Colormap cmap;
    XWindowAttributes attr;
    XSetWindowAttributes swa;
    Window win;
    int screen;
    XFont *font;
    unsigned int width;
    unsigned int height;
    Atom wm_delete_window;
};

static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

static long
timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec/1000);
}

static int
cleanup(XWindow xw, utils_variables *pvariables)
{
    int i = 0;
    
    for (i = 0; i < pvariables->partitions_amount; i++)
    {
        free(pvariables->partitions[i]);
    }
    free(pvariables->partitions);
    free(pvariables->fs_type);
    free(pvariables->label);
    free(pvariables->mounted_path);

    nk_xfont_del(xw.dpy, xw.font);
    nk_xlib_shutdown();
    XUnmapWindow(xw.dpy, xw.win);
    XFreeColormap(xw.dpy, xw.cmap);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}

static void
sleep_for(long t)
{
    struct timespec req;
    const time_t sec = (int)(t/1000);
    const long ms = t - (sec * 1000);
    req.tv_sec = sec;
    req.tv_nsec = ms * 1000000L;
    while(-1 == nanosleep(&req, &req));
}


int
start_program(void)
{
    long dt;
    long started;
    long toto;
    int running = 1;
    XWindow xw;
    XSizeHints sh;
    struct nk_context *ctx;
    utils_variables variables;
    
    if (getuid())
    {
		printf("Run this program as root!\n");
		return 0;
	}
    
    /* Struct initialization */
    memset(&variables, 0, sizeof variables);
    gather_partitions(&variables);
    gather_partition_info(&variables);
    strcpy(variables.status, "Standby...");
    variables.generate_ini = 1;
    variables.is_thread_active = false;

    /* X11 */
    memset(&xw, 0, sizeof xw);
    xw.dpy = XOpenDisplay(NULL);
    if (!xw.dpy)
    {
        die("Could not open a display; perhaps $DISPLAY is not set?");
    }

    xw.root = DefaultRootWindow(xw.dpy);
    xw.screen = XDefaultScreen(xw.dpy);
    xw.vis = XDefaultVisual(xw.dpy, xw.screen);
    xw.cmap = XCreateColormap(xw.dpy,xw.root,xw.vis,AllocNone);

    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask| ButtonMotionMask |
        Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask|
        PointerMotionMask | KeymapStateMask | StructureNotifyMask;
    xw.win = XCreateWindow(xw.dpy, xw.root, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
        XDefaultDepth(xw.dpy, xw.screen), InputOutput,
        xw.vis, CWEventMask | CWColormap, &xw.swa);

    XGetWMNormalHints(xw.dpy, xw.root, &sh, &toto);
    sh.width = sh.min_width = sh.max_width = WINDOW_WIDTH;
    sh.height = sh.min_height = sh.max_height = WINDOW_HEIGHT;
    sh.flags = PSize | PMinSize | PMaxSize;
    XSetWMNormalHints(xw.dpy, xw.root, &sh);

    XStoreName(xw.dpy, xw.win, "ReactOS RAMDisk Creator");
    XMapWindow(xw.dpy, xw.win);
    xw.wm_delete_window = XInternAtom(xw.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(xw.dpy, xw.win, &xw.wm_delete_window, 1);
    XGetWindowAttributes(xw.dpy, xw.win, &xw.attr);
    xw.width = (unsigned int)xw.attr.width;
    xw.height = (unsigned int)xw.attr.height;

    /* GUI */
    xw.font = nk_xfont_create(xw.dpy, "fixed");
    ctx = nk_xlib_init(xw.font, xw.dpy, xw.screen, xw.win, xw.width, xw.height);

    while (running)
    {
        static int width = 0, height = 0;
        XEvent evt;
        started = timestamp();
        nk_input_begin(ctx);
        while (XPending(xw.dpy)) {
            XNextEvent(xw.dpy, &evt);
            if (evt.type == ClientMessage) return cleanup(xw, &variables);
            if (evt.type == ConfigureNotify)
            {
                width = evt.xconfigure.width;
                height = evt.xconfigure.height;
            }
            if (XFilterEvent(&evt, xw.win))
            {
                continue;
            }
            nk_xlib_handle_event(xw.dpy, xw.screen, xw.win, &evt);
        }
        nk_input_end(ctx);

        /* GUI */
        ctx->style.edit.border = 0;
        start_gui(ctx, width, height, &variables);

        /* Draw */
        XClearWindow(xw.dpy, xw.win);
        nk_xlib_render(xw.win, nk_rgb(30,30,30));
        XFlush(xw.dpy);

        /* Timing */
        dt = timestamp() - started;
        if (dt < DTIME)
            sleep_for(DTIME - dt);

        if (nk_window_is_closed(ctx, "Main"))
            break;
    }

    return cleanup(xw, &variables);
}
