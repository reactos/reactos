#include <precomp.h>

_invalid_parameter_handler
_set_invalid_parameter_handler(_invalid_parameter_handler newhandler)
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

    if (newhandler)
    {
        __pInvalidArgHandler = EncodePointer(newhandler);
    }
    else
    {
        __pInvalidArgHandler = 0;
    }
    
    return oldhandler;
}
