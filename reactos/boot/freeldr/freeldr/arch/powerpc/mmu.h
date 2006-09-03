#ifndef FREELDR_MMU_H
#define FREELDR_MMU_H

int GetMSR();
int GetPhys( int addr );
int GetSR(int n);
void GetBat( int bat, int inst, int *batHi, int *batLo );
int GetSDR1();
int BatHit( int bath, int batl, int virt );
int BatTranslate( int bath, int batl, int virt );
/* translate address */
int PpcVirt2phys( int virt, int inst );

#endif/*FREELDR_MMU_H*/
