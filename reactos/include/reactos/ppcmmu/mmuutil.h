#ifndef FREELDR_MMU_H
#define FREELDR_MMU_H

extern int mmu_handle;

int GetDEC();
int GetMSR();
int GetPhys( int addr );
int GetPhysHalf( int addr );
int GetPhysByte( int addr );
void SetPhys( int addr, int val );
void SetPhysHalf( int addr, int val );
void SetPhysByte( int addr, int val );
int GetSR(int n);
void SetSR(int n, int val);
void GetBat( int bat, int inst, int *batHi, int *batLo );
void SetBat( int bat, int inst, int batHi, int batLo );
int GetSDR1();
void SetSDR1( int newsdr );
int BatHit( int bath, int batl, int virt );
int BatTranslate( int bath, int batl, int virt );
/* translate address */
int PpcVirt2phys( int virt, int inst );
int InsertPageEntry( int virt, int phys, int slot, int sdr );

#endif/*FREELDR_MMU_H*/
