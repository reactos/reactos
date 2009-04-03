#ifndef s_addr

typedef struct in_addr
{
        union
        {
                struct {UCHAR s_b1,s_b2,s_b3,s_b4;} S_un_b;
                struct {USHORT s_w1,s_w2;} S_un_w;
                ULONG S_addr;
        } S_un;
} IN_ADDR, *PIN_ADDR, FAR *LPIN_ADDR;

#define s_addr  S_un.S_addr
#define s_host  S_un.S_un_b.s_b2
#define s_net   S_un.S_un_b.s_b1
#define s_imp   S_un.S_un_w.s_w2
#define s_impno S_un.S_un_b.s_b4
#define s_lh    S_un.S_un_b.s_b3

#endif

