

#pragma once

void WINAPI RtlUpperString(STRING*, const STRING*);
LONG WINAPI RtlCompareString(const STRING*,const STRING*,BOOLEAN);

typedef struct _PROCESS_ACCESS_TOKEN
{
    HANDLE Token;
    HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;
