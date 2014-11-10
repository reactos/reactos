KIRQL irql;
KAFFINITY affinity;

#include <pshpack1.h>
typedef struct
{
	unsigned char		rID[4]	;					//4	0
	unsigned int 		rLen	;						//4	4
	unsigned char 	wID[4] ;						//4	8
	unsigned char 	fID[4] ;						//4	12
	unsigned int 		fLen ;							//4	16
	unsigned short 		wFormatTag ;				//2	18
	unsigned short 	nChannels ;				//2	20
	unsigned int 		nSamplesPerSec ;		//2 22
	unsigned int 		nAvgBytesPerSec ;	//2	24
	unsigned short 	nBlockAlign ;			//2	26
	unsigned short 	FormatSpecific ;		//2	28
	unsigned char 	dID[4] ;						//4	30
	unsigned int 		dLen ;
	unsigned char* 	data;
}WAVE_HDR;
#include <poppack.h>

void sb16_play(WAVE_HDR* wave);
void dump_wav(WAVE_HDR* wave);
BOOLEAN playRoutine(PKINTERRUPT Interrupt,PVOID ServiceContext);
