/* Cromwell IDE driver code - GNU GPL */
#include "boot.h"

tsHarddiskInfo tsaHarddiskInfo[2];  // static struct stores data about attached drives

//Methods
int BootIdeInit(void);
int BootIdeReadSector(int nDriveIndex, void * pbBuffer, unsigned int block, int byte_offset, int n_bytes) ;
int BootIdeReadData(unsigned uIoBase, void * buf, size_t size);
