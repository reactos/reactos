/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#ifndef _S3V_TEX_H
#define _S3V_TEX_H

#define TEX_DEBUG_ON 0

extern void s3vUpdateTexLRU( s3vContextPtr vmesa, s3vTextureObjectPtr t );

#if TEX_DEBUG_ON
#define DEBUG_TEX(str) printf str
#else
#define DEBUG_TEX(str) /* str */
#endif

#define _TEXFLUSH 1 /* flush before uploading */
#define _TEXLOCK  1 /* lock before writing new texures to card mem */
					/* 	if you turn it on you will gain stability and image
						quality, but you will loose performance (~10%) */
#define _TEXFALLBACK 0 /* fallback to software for -big- textures (slow) */
					/* turning this off, you will lose some tex (e.g. mountains
					   on tuxracer) but you will increase average playability */

#define _TEXALIGN 0x00000007

#endif
