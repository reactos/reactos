#include "../pdlparse/headers.hxx"

extern "C"
HINSTANCE MwMainwinInitLite(int argc, char *argv[], void *lParam);

extern "C" 
HINSTANCE mainwin_init(int argc, char *argv[])
{ 
    return MwMainwinInitLite( argc, argv, NULL ); 
}
