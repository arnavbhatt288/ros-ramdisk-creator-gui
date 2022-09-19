#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    #include "gdi.h"
#elif __linux__
    #include "x11.h"
#endif

#ifdef _WIN32
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    return start_program();
}
#elif __linux__
int main()
{
    return start_program();
}
#endif
