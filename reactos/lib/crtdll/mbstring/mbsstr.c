#include <crtdll/mbstring.h>
#include <crtdll/stdlib.h>

unsigned char *_mbsstr(const unsigned char *src1,const unsigned char *src2)
{
        int len;

        if(src2 ==NULL || *src2 == 0)
                return (unsigned char *)src1;

        len = _mbstrlen(src2);

        while(*src1)
        {
                if((*src1 == *src2) && (_mbsncmp(src1,src2,len) == 0))
                        return (unsigned char *)src1;
                src1 = (unsigned char *)_mbsinc(src1);
        }
        return NULL;
}
