// start of
// structure of symbol file
///////////////////////////////////////////////////
typedef struct tagLoaderSymbolHeader
{
	ULONG dwMagic,dwSize;
	char Copyright[256];
	char ModuleName[256];
	ULONG NumberOfSymbols;
	ULONG Reserved; // for future extension
}LOADERSYMBOLHEADER,*PLOADERSYMBOLHEADER;

typedef struct tagLoaderSymbolRecord
{
	ULONG ModuleNameLength;
	ULONG NameLength;
	ULONG Address;
	ULONG Type;
	ULONG Class;
}LOADERSYMBOLRECORD,*PLOADERSYMBOLRECORD;

typedef struct tagLoaderSymbolFile
{
	LOADERSYMBOLHEADER LoaderSymbolHeader;		// file header
	LOADERSYMBOLRECORD LoaderSymbolRecord[1];	// symbol records (symbol + source file)
}LOADERSYMBOLFILE,*PLOADERSYMBOLFILE;

typedef struct tagLoaderSymbolPool
{
	ULONG NumberOfFiles;
	ULONG SizeOfThisHeap;
	LOADERSYMBOLHEADER LoaderSymbolHeader;		// file header
	LOADERSYMBOLRECORD LoaderSymbolRecord[1];	// symbol records (symbol + source file)
}LOADERSYMBOLPOOL,*PLOADERSYMBOLPOOL;

// end of
// structure of symbol file
///////////////////////////////////////////////////
