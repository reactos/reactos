#include <time.h>
#include <string>

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)

struct some_struct {
    std::string s; // if std not properly redefined, error will be here
};

#endif
