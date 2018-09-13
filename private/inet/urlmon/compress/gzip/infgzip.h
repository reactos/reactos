//
// infgzip.h
//
// Gzip additions to inflate
//

// decompressing
BOOL ReadGzipHeader(t_decoder_context *context);
BOOL ReadGzipFooter(t_decoder_context *context);
void DecoderInitGzipVariables(t_decoder_context *context);
ULONG GzipCRC32(ULONG crc, const BYTE *buf, ULONG len);
