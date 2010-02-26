#pragma once

#include <stdio.h>
#include <rsym.h>

#include "config.h"
#include "revision.h"
#include "stat.h"
#include "list.h"


struct lineinfo_struct
{
    int     valid; 
    char    file1[LINESIZE];
    char    func1[NAMESIZE];
    int     nr1;
    char    file2[LINESIZE];
    char    func2[NAMESIZE];
    int     nr2;
};

typedef struct lineinfo_struct LINEINFO;

extern SUMM summ;
extern REVINFO revinfo;
extern LIST cache;
extern FILE *logFile;
extern LINEINFO lastLine;
extern LIST sources;

/* EOF */
