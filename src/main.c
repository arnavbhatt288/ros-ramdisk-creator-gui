#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include "gdi.h"
#elif __linux__
    #include "x11.h"
#endif

int main()
{
    return start_program();
}
