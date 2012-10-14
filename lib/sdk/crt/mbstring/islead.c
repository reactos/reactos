#include <precomp.h>
#include <mbstring.h>

/*
 * @implemented
 */
int isleadbyte(int c)
{
    return _isctype( c, _MLEAD );

}
