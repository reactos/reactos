 
#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))
#define ROUND_UP ROUNDUP
#define ROUND_DOWN ROUNDDOWN
#define PAGE_ROUND_DOWN(x) (((ULONG)x)&(~(PAGE_SIZE-1)))
#define PAGE_ROUND_UP(x) ( (((ULONG)x)%PAGE_SIZE) ? ((((ULONG)x)&(~(PAGE_SIZE-1)))+PAGE_SIZE) : ((ULONG)x) )
#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))
#define RtlRosMin(X,Y) (((X) < (Y))? (X) : (Y))
#define RtlRosMin3(X,Y,Z) (((X) < (Y)) ? RtlRosMin(X,Z) : RtlRosMin(Y,Z))
#define KEBUGCHECKEX(a,b,c,d,e) DbgPrint("KeBugCheckEx at %s:%i\n",__FILE__,__LINE__), KeBugCheckEx(a,b,c,d,e)
#define KEBUGCHECK(a) DbgPrint("KeBugCheck at %s:%i\n",__FILE__,__LINE__), KeBugCheck(a)
#define EXPORTED __declspec(dllexport)
#define IMPORTED __declspec(dllimport)
