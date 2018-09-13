/*
 * api_int.h
 *
 * Internal API function prototypes and flags
 *
 * The api.h which is given to decompression clients is hand-created from this file.
 */
// flags for CreateCompression() and CreateDeCompression()
#define COMPRESSION_FLAG_DEFLATE    0 
#define COMPRESSION_FLAG_GZIP       1 

#define COMPRESSION_FLAG_DO_GZIP      COMPRESSION_FLAG_GZIP
#define DECOMPRESSION_FLAG_DO_GZIP    COMPRESSION_FLAG_GZIP

// Initialise global DLL compression data
HRESULT	WINAPI InitCompression(VOID);

// Initialise global DLL decompression data
HRESULT	WINAPI InitDecompression(VOID);

// Free global compression data
VOID    WINAPI DeInitCompression(VOID);

// Free global decompression data
VOID    WINAPI DeInitDecompression(VOID);

// Create a new compression context
HRESULT	WINAPI CreateCompression(PVOID *context, ULONG flags);

// Compress data
HRESULT WINAPI Compress(
	PVOID				context,            // compression context
	CONST BYTE *		input_buffer,       // input buffer
	LONG				input_buffer_size,  // size of input buffer
	PBYTE				output_buffer,      // output buffer
	LONG				output_buffer_size, // size of output buffer
	PLONG				input_used,         // amount of input buffer used
	PLONG				output_used,        // amount of output buffer used
	INT					compression_level   // compression level (1...10)
);

// Reset compression state (for compressing new file)
HRESULT	WINAPI ResetCompression(PVOID context);

// Destroy compression context
VOID	WINAPI DestroyCompression(PVOID context);

// Create a decompression context
HRESULT WINAPI CreateDecompression(PVOID *context, ULONG flags);

// Decompress data
HRESULT WINAPI Decompress(
	PVOID				void_context,
	CONST BYTE *		input, 
	LONG				input_size,
	BYTE *				output, 
	LONG				output_size,
	PLONG				input_used,
	PLONG				output_used
);

HRESULT	WINAPI ResetDecompression(PVOID void_context);

VOID	WINAPI DestroyDecompression(PVOID void_context);
