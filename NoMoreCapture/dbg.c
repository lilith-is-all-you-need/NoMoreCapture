#define LILITH_DEBUG
#ifdef LILITH_DEBUG

#include "lilith_is_all_you_need.h"


void out(const wchar_t* str, int opt)
{
    int type = MB_SYSTEMMODAL;
    switch (opt)
    {
    case err:
        type |= MB_ICONERROR;
        break;
    case info:
        type |= MB_ICONINFORMATION;
        break;
    default:
        break;
    }
    MessageBoxW(NULL, str, L"Lilith Dbg opt", type);
}

#endif