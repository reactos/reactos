/*
 *  Notepad (search.c)
 *  Copyright (C) 1999 by Marcel Baur
 *  To be distributed under the Wine license
 *
 *  This file features Heuristic Boyer-Moore Text Search
 *
 *  Always:   - Buf is the Buffer containing the whole text
 *  =======   - SP is the Search Pattern, which has to be found in Buf.
 *
 */

#include <windows.h>
 
 #define CHARSETSIZE 255
  
 int delta[CHARSETSIZE];
 
 /* rightmostpos: return rightmost position of ch in szSP (or -1) */
 int rightmostpos(char ch, LPSTR szSP, int nSPLen) {
    int i = nSPLen;
    while ((i>0) & (szSP[i]!=ch)) i--;
    return(i);
 }
 
 /* setup_delta: setup delta1 cache */
 void setup_delta(LPSTR szSP, int nSPLen) {
    int i;
    
    for (i=0; i<CHARSETSIZE; i++) {
       delta[i] = nSPLen;
    }

    for (i=0; i<nSPLen; i++) {
       delta[(int)szSP[i]] = (nSPLen - rightmostpos(szSP[i], szSP, nSPLen));
    }
 }

 int bm_search(LPSTR szBuf, int nBufLen, LPSTR szSP, int nSPLen) {
    int i = nSPLen;
    int j = nSPLen;
    
    do {
       if ((szBuf[i] = szSP[j])) {
         i--; j--;
       } else {
         if ((nSPLen-j+1) > delta[(int)szBuf[i]]) {
           i+= (nSPLen-j+1);
         } else {
           i+= delta[(int)szBuf[i]];
         }
       }
    } while (j>0 && i<=nBufLen);
    return(i+1);
 }
 
