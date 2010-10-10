#ifndef FREELDR_MMU_H
#define FREELDR_MMU_H

int GetDEC();
int GetMSR();
int GetPhys( paddr_t addr );
int GetPhysHalf( paddr_t addr );
int GetPhysByte( paddr_t addr );
void SetPhys( paddr_t addr, int val );
void SetPhysHalf( paddr_t addr, int val );
void SetPhysByte( paddr_t addr, int val );
int GetSR(int n);
void SetSR(int n, int val);
void GetBat( int bat, int inst, int *batHi, int *batLo );
void SetBat( int bat, int inst, int batHi, int batLo );
int GetSDR1();
void SetSDR1( int newsdr );
int BatHit( int bath, int batl, int virt );
int BatTranslate( int bath, int batl, int virt );
/* translate address */
int PpcVirt2phys( vaddr_t virt, int inst );
int PtegNumber( vaddr_t virt, int hfun );
#endif/*FREELDR_MMU_H*/
