#include "vga.h"

//------------------------------------------------------------------------
// Sets the value of a character in the character buffer
//------------------------------------------------------------------------
int setchar(char c, char attr, unsigned int x, unsigned int y)
{
    if(y > 24)
        return -1;
    if(x > 79)
        return -1;
    unsigned short* charbuf = (unsigned short*)0xb8000;
    charbuf[y*80+x] = (attr << 8) + c;
    return 0;
}
