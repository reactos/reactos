
/******************************************************************************

                        S E C U R I T Y   H E A D E R

    Name:       security.h
    Date:       1/20/94
    Creator:    John Fu

    Description:
        This is the header file for shares.c

******************************************************************************/




BOOL GetTokenHandle(
    PHANDLE pTokenHandle);


PSECURITY_DESCRIPTOR MakeLocalOnlySD (void);


PSECURITY_DESCRIPTOR CurrentUserOnlySD (void);




#ifdef DEBUG

void HexDumpBytes(
    char        *pv,
    unsigned    cb);


void PrintSid(
    PSID    sid);


void PrintAcl(
    PACL    pacl);


void PrintSD(
    PSECURITY_DESCRIPTOR    pSD);


#else

#define HexDumpBytes(x,y)
#define PrintSid(x)
#define PrintAcl(x)
#define PrintSD(x)

#endif
