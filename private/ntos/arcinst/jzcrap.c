#include "precomp.h"
#pragma hdrstop

VOID
JzDeleteVariableSegment (
    PCHAR VariableName,
    ULONG Selection
    )
{
    PCHAR Variable;
    CHAR VariableValue[MAXIMUM_ENVIRONMENT_VALUE];
    ULONG Index;
    ULONG Count;
    BOOLEAN FirstSegment;

    if ((Variable = ArcGetEnvironmentVariable(VariableName)) == NULL) {
        return;
    }

    FirstSegment = TRUE;
    Index = 0;
    *VariableValue = 0;
    while (strchr(Variable,';') != NULL) {
        Count = strchr(Variable,';') - Variable;
        if (Index != Selection) {
            if (!FirstSegment) {
                strcat(VariableValue,";");
            }
            strncat(VariableValue, Variable, Count);
            FirstSegment = FALSE;
        }
        Variable += Count + 1;
        Index++;
    }

    if (Index != Selection) {
        if (!FirstSegment) {
            strcat(VariableValue,";");
        }
        strcat(VariableValue,Variable);
    }

    ArcSetEnvironmentVariable(VariableName, VariableValue);
    return;
}

PCHAR BootString[] = { "LOADIDENTIFIER",
                       "SYSTEMPARTITION",
                       "OSLOADER",
                       "OSLOADPARTITION",
                       "OSLOADFILENAME",
                       "OSLOADOPTIONS" };
