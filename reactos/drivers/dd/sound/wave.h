KIRQL irql;
KAFFINITY affinity;

typedef struct
{	
	unsigned char		rID[4]	__attribute__((packed));					//4	0
	unsigned int 		rLen	__attribute__((packed));						//4	4
	unsigned char 	wID[4] __attribute__((packed));						//4	8
	unsigned char 	fID[4] __attribute__((packed));						//4	12
	unsigned int 		fLen __attribute__((packed));							//4	16
	unsigned short 		wFormatTag __attribute__((packed));				//2	18
	unsigned short 	nChannels __attribute__((packed));				//2	20
	unsigned int 		nSamplesPerSec __attribute__((packed));		//2 22
	unsigned int 		nAvgBytesPerSec __attribute__((packed));	//2	24
	unsigned short 	nBlockAlign __attribute__((packed));			//2	26
	unsigned short 	FormatSpecific __attribute__((packed));		//2	28
	unsigned char 	dID[4] __attribute__((packed));						//4	30
	unsigned int 		dLen __attribute__((packed));
	unsigned char* 	data;
}WAVE_HDR;

void sb16_play(WAVE_HDR* wave);
void dump_wav(WAVE_HDR* wave);
BOOLEAN playRoutine(PKINTERRUPT Interrupt,PVOID ServiceContext);
