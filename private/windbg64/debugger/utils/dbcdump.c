#include <windows.h>
#include <stdio.h>

#include <cvinfo.h>

#include <od.h>
#include <odp.h>

typedef struct {
    DBC     dbc;
    LPCSTR  lszDbc;
    BOOL    fRequest;
} DBCINFO;

#define DECL_DBC(name, fRequest, dbct) { dbc##name, "dbc" #name, fRequest },

DBCINFO rgdbcinfo[] = {
#include "dbc.h"
};

#define NDBC (sizeof(rgdbcinfo)/sizeof(DBCINFO))

__cdecl
main(
    int argc,
    char **argv
    )
{

    int i;
    for (i = 0; i < NDBC; i++) {

        printf("%-20s %02x %3d %s\n",
            rgdbcinfo[i].lszDbc,
            rgdbcinfo[i].dbc,
            rgdbcinfo[i].dbc,
            rgdbcinfo[i].fRequest ? "TRUE" : "FALSE"
            );
    }
    return 0;
}
