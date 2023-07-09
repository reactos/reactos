#include <npdefs.h>
#include <netlib.h>

#define ADVANCE(p)    (p += IS_LEAD_BYTE(*p) ? 2 : 1)

#define SPN_SET(bits,ch)    bits[(ch)/8] |= (1<<((ch) & 7))
#define SPN_TEST(bits,ch)    (bits[(ch)/8] & (1<<((ch) & 7)))

