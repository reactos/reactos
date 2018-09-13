/*
   File : DConv.H
   Header file for Data Conversion of  RegWiz Registry Information 

      

*/
#ifndef __DCONV__
#define __DCONV__

#ifdef __cplusplus
extern "C" 
{
#endif

int PrepareRegWizTxbuffer(HINSTANCE hIns, char *tcTxBuf, DWORD * pRetLen); 
DWORD OemTransmitBuffer(HINSTANCE hIns,char *sztxBuffer,DWORD * pRetLen);

#ifdef __cplusplus
}
#endif

#endif
