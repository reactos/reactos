#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include "sscommon.h"


/******************************Public*Routine******************************\
* HsvToRgb
*
* HSV to RGB color space conversion.  From pg. 593 of Foley & van Dam.
*
\**************************************************************************/

void 
ss_HsvToRgb(float h, float s, float v, RGBA *color )
{
    float i, f, p, q, t;

    // set alpha value, so caller doesn't have to worry about undefined value
    color->a = 1.0f;

    if (s == 0.0f)     // assume h is undefined
        color->r = color->g = color->b = v;
    else {
        if (h >= 360.0f)
            h = 0.0f;
        h = h / 60.0f;
        i = (float) floor(h);
        f = h - i;
        p = v * (1.0f - s);
        q = v * (1.0f - (s * f));
        t = v * (1.0f - (s * (1.0f - f)));
        switch ((int)i) {
        case 0:
            color->r = v;
            color->g = t;
            color->b = p;
            break;
        case 1:
            color->r = q;
            color->g = v;
            color->b = p;
            break;
        case 2:
            color->r = p;
            color->g = v;
            color->b = t;
            break;
        case 3:
            color->r = p;
            color->g = q;
            color->b = v;
            break;
        case 4:
            color->r = t;
            color->g = p;
            color->b = v;
            break;
        case 5:
            color->r = v;
            color->g = p;
            color->b = q;
            break;
        default:
            break;
        }
    }
}
