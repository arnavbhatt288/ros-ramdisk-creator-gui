@echo off
cl /nologo /W3 /O2 /fp:fast /Gm- /Feramdisk-creator.exe main.c gdi.c gui.c utils_win.c tinyfiledialogs.c common.c user32.lib gdi32.lib Advapi32.lib Msimg32.lib Comctl32.lib ComDlg32.lib Ole32.lib Shell32.lib /link /incremental:no
