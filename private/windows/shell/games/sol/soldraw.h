

/* CarDDRaw structure  */
typedef struct _cddr
{
	HDC hdc;
	INT x;
	INT y;
	INT cd;
	INT mode;
	DWORD rgbBgnd;
} CDDR;



#define drwInit 		1
#define drwDrawCard 	2
#define drwClose 		3
