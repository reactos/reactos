
#ifndef __WINE_BASE64_CODEC_H__
#define __WINE_BASE64_CODEC_H__

/* Functions from base64_codec.c used elsewhere */
SECURITY_STATUS encodeBase64(PBYTE in_buf, int in_len, char* out_buf,
        int max_len, int *out_len) DECLSPEC_HIDDEN;

SECURITY_STATUS decodeBase64(char *in_buf, int in_len, BYTE *out_buf,
        int max_len, int *out_len) DECLSPEC_HIDDEN;

#endif /* __WINE_BASE64_CODEC_H__ */
