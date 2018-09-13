//
// defgzip.h
//
// Gzip additions to deflate
//

// compressing
void WriteGzipHeader(t_encoder_context *context, int compression_level);
void WriteGzipFooter(t_encoder_context *context);
void GzipCRCmemcpy(t_encoder_context *context, BYTE *dest, const BYTE *src, ULONG count);
void EncoderInitGzipVariables(t_encoder_context *context);
