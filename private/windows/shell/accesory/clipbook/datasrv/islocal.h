
/******************************************************************************

                        I S L O C A L   H E A D E R

    Name:       islocal.h
    Date:       1/20/94
    Creator:    John Fu

    Description:
        This is the header file for islocal.c

******************************************************************************/



void HexDumpBytes(
    char        *pv,
    unsigned    cb);


void PrintSid(
    PSID sid);


BOOL IsUserLocal (
    HCONV hConv);
