#include <precomp.h>

_invalid_parameter_handler
_get_invalid_parameter_handler(void)
{
    _invalid_parameter_handler oldhandler;

    if (__pInvalidArgHandler)
    {
        oldhandler = DecodePointer(__pInvalidArgHandler);
    }
    else
    {
        oldhandler = NULL;
    }
    return oldhandler;
}
