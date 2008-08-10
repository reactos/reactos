
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

INT cmdHelp(INT argc, CHAR **argv)
{
    if (argc>3)
    {
      return 0;
    }

    if (strcmp(argv[0],"ACCOUNTS")==0)
    {
        printf("ACCOUNTS\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"COMPUTER")==0)
    {
        printf("COMPUTER\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"CONFIG")==0)
    {
        printf("CONFIG\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"CONTINUE")==0)
    {
        printf("CONTINUE\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"FILE")==0)
    {
        printf("FILE\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"GROUP")==0)
    {
        printf("GROUP\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"HELP")==0)
    {
        printf("HELP\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"HELPMSG")==0)
    {
        printf("HELPMSG\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"LOCALGROUP")==0)
    {
        printf("LOCALGROUP\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"NAME")==0)
    {
        printf("NAME\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"PRINT")==0)
    {
        printf("PRINT\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"SEND")==0)
    {
        printf("SEND\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"SESSION")==0)
    {
        printf("SESSION\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"SHARE")==0)
    {
        printf("SHARE\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"START")==0)
    {
        printf("START\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"STATISTICS")==0)
    {
        printf("STATISTICS\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"STOP")==0)
    {
        printf("STOP\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"TIME")==0)
    {
        printf("TIME\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"USE")==0)
    {
        printf("USE\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"USER")==0)
    {
        printf("USER\n");
        printf("help text\n");
        return 0;
    }

    if (strcmp(argv[0],"VIEW")==0)
    {
        printf("VIEW\n");
        printf("help text\n");
        return 0;
    }

    help();
    return 0;
}

void help()
{
    printf("NET ACCOUNTS\n");
    printf("NET COMPUTER\n");
    printf("NET CONFIG\n");
    printf("NET CONFIG SERVER\n");
    printf("NET CONFIG WORKSTATION\n");
    printf("NET CONTINUE\n");
    printf("NET FILE\n");
    printf("NET GROUP\n");

    printf("NET HELP\n");
    printf("NET HELPMSG\n");
    printf("NET LOCALGROUP\n");
    printf("NET NAME\n");
    printf("NET PAUSE\n");
    printf("NET PRINT\n");
    printf("NET SEND\n");
    printf("NET SESSION\n");

    printf("NET SHARE\n");
    printf("NET START\n");
    printf("NET STATISTICS\n");
    printf("NET STOP\n");
    printf("NET TIME\n");
    printf("NET USE\n");
    printf("NET USER\n");
    printf("NET VIEW\n");
}
