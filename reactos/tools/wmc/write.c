/*
 * Wine Message Compiler output generation
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
 *
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "wmc.h"
#include "utils.h"
#include "lang.h"
#include "write.h"

/*
 * The binary resource layout is as follows:
 *
 *         +===============+
 * Header  |    NBlocks    |
 *         +===============+
 * Block 0 |    Low ID     |
 *         +---------------+
 *         |    High ID    |
 *         +---------------+
 *         |    Offset     |---+
 *         +===============+   |
 * Block 1 |    Low ID     |   |
 *         +---------------+   |
 *         |    High ID    |   |
 *         +---------------+   |
 *         |    Offset     |------+
 *         +===============+   |  |
 *         |               |   |  |
 *        ...             ...  |  |
 *         |               |   |  |
 *         +===============+ <-+  |
 * B0 LoID |  Len  | Flags |      |
 *         +---+---+---+---+      |
 *         | b | l | a | b |      |
 *         +---+---+---+---+      |
 *         | l | a | \0| \0|      |
 *         +===============+      |
 *         |               |      |
 *        ...             ...     |
 *         |               |      |
 *         +===============+      |
 * B0 HiID |  Len  | Flags |      |
 *         +---+---+---+---+      |
 *         | M | o | r | e |      |
 *         +---+---+---+---+      |
 *         | b | l | a | \0|      |
 *         +===============+ <----+
 * B1 LoID |  Len  | Flags |
 *         +---+---+---+---+
 *         | J | u | n | k |
 *         +---+---+---+---+
 *         | \0| \0| \0| \0|
 *         +===============+
 *         |               |
 *        ...             ...
 *         |               |
 *         +===============+
 *
 * All Fields are aligned on their natural boundaries. The length
 * field (Len) covers both the length of the string and the header
 * fields (Len and Flags). Strings are '\0' terminated. Flags is 0
 * for normal character strings and 1 for unicode strings.
 */

static char str_header[] =
	"/* This file is generated with wmc version " WMC_FULLVERSION ". Do not edit! */\n"
	"/* Source : %s */\n"
	"/* Cmdline: %s */\n"
	"/* Date   : %s */\n"
	"\n"
        ;

static char *dup_u2c(int cp, const WCHAR *uc)
{
	int len = unistrlen(uc);
	char *cptr = xmalloc(len+1);
//	const union cptable *cpdef = find_codepage(cp);
//	if(!cpdef)
//		internal_error(__FILE__, __LINE__, "Codepage %d not found (vanished?)", cp);
//	if((len = cp_wcstombs(cpdef, 0, uc, unistrlen(uc)+1, cptr, len+1, NULL, NULL)) < 0)
	if((len = WideCharToMultiByte(cp, 0, uc, unistrlen(uc)+1, cptr, len+1, NULL, NULL)) < 0)
		internal_error(__FILE__, __LINE__, "Buffer overflow? code %d.", len);
	return cptr;
}

static void killnl(char *s, int ddd)
{
	char *tmp;
	tmp = strstr(s, "\r\n");
	if(tmp)
	{
		if(ddd && tmp - s > 3)
		{
			tmp[0] = tmp[1] = tmp[2] = '.';
			tmp[3] = '\0';
		}
		else
			*tmp = '\0';
	}
	tmp = strchr(s, '\n');
	if(tmp)
	{
		if(ddd && tmp - s > 3)
		{
			tmp[0] = tmp[1] = tmp[2] = '.';
			tmp[3] = '\0';
		}
		else
			*tmp = '\0';
	}
}

static int killcomment(char *s)
{
	char *tmp = s;
	int b = 0;
	while((tmp = strstr(tmp, "/*")))
	{
		tmp[1] = 'x';
		b++;
	}
	tmp = s;
	while((tmp = strstr(tmp, "*/")))
	{
		tmp[0] = 'x';
		b++;
	}
	return b;
}

void write_h_file(const char *fname)
{
	node_t *ndp;
	char *cptr;
	char *cast;
	FILE *fp;
	token_t *ttab;
	int ntab;
	int i;
	int once = 0;
	int idx_en = 0;

	fp = fopen(fname, "w");
	if(!fp)
	{
		perror(fname);
		exit(1);
	}
	cptr = ctime(&now);
	killnl(cptr, 0);
	fprintf(fp, str_header, input_name ? input_name : "<stdin>", cmdline, cptr);
	fprintf(fp, "#ifndef __WMCGENERATED_%08lx_H\n", now);
	fprintf(fp, "#define __WMCGENERATED_%08lx_H\n", now);
	fprintf(fp, "\n");

	/* Write severity and facility aliases */
	get_tokentable(&ttab, &ntab);
	fprintf(fp, "/* Severity codes */\n");
	for(i = 0; i < ntab; i++)
	{
		if(ttab[i].type == tok_severity && ttab[i].alias)
		{
			cptr = dup_u2c(WMC_DEFAULT_CODEPAGE, ttab[i].alias);
			fprintf(fp, "#define %s\t0x%x\n", cptr, ttab[i].token);
			free(cptr);
		}
	}
	fprintf(fp, "\n");

	fprintf(fp, "/* Facility codes */\n");
	for(i = 0; i < ntab; i++)
	{
		if(ttab[i].type == tok_facility && ttab[i].alias)
		{
			cptr = dup_u2c(WMC_DEFAULT_CODEPAGE, ttab[i].alias);
			fprintf(fp, "#define %s\t0x%x\n", cptr, ttab[i].token);
			free(cptr);
		}
	}
	fprintf(fp, "\n");

	/* Write the message codes */
	fprintf(fp, "/* Message definitions */\n");
	for(ndp = nodehead; ndp; ndp = ndp->next)
	{
		switch(ndp->type)
		{
		case nd_comment:
			cptr = dup_u2c(WMC_DEFAULT_CODEPAGE, ndp->u.comment+1);
			killnl(cptr, 0);
			killcomment(cptr);
			if(*cptr)
				fprintf(fp, "/* %s */\n", cptr);
			else
				fprintf(fp, "\n");
			free(cptr);
			break;
		case nd_msg:
			if(!once)
			{
				/*
				 * Search for an english text.
				 * If not found, then use the first in the list
				 */
				once++;
				for(i = 0; i < ndp->u.msg->nmsgs; i++)
				{
					if(ndp->u.msg->msgs[i]->lan == 0x409)
					{
						idx_en = i;
						break;
					}
				}
				fprintf(fp, "\n");
			}
			fprintf(fp, "/* MessageId  : 0x%08x */\n", ndp->u.msg->realid);
			cptr = dup_u2c(ndp->u.msg->msgs[idx_en]->cp, ndp->u.msg->msgs[idx_en]->msg);
			killnl(cptr, 0);
			killcomment(cptr);
			fprintf(fp, "/* Approx. msg: %s */\n", cptr);
			free(cptr);
			cptr = dup_u2c(WMC_DEFAULT_CODEPAGE, ndp->u.msg->sym);
			if(ndp->u.msg->cast)
				cast = dup_u2c(WMC_DEFAULT_CODEPAGE, ndp->u.msg->cast);
			else
				cast = NULL;
			switch(ndp->u.msg->base)
			{
			case 8:
				if(cast)
					fprintf(fp, "#define %s\t((%s)0%oL)\n\n", cptr, cast, ndp->u.msg->realid);
				else
					fprintf(fp, "#define %s\t0%oL\n\n", cptr, ndp->u.msg->realid);
				break;
			case 10:
				if(cast)
					fprintf(fp, "#define %s\t((%s)%dL)\n\n", cptr, cast, ndp->u.msg->realid);
				else
					fprintf(fp, "#define %s\t%dL\n\n", cptr, ndp->u.msg->realid);
				break;
			case 16:
				if(cast)
					fprintf(fp, "#define %s\t((%s)0x%08xL)\n\n", cptr, cast, ndp->u.msg->realid);
				else
					fprintf(fp, "#define %s\t0x%08xL\n\n", cptr, ndp->u.msg->realid);
				break;
			default:
				internal_error(__FILE__, __LINE__, "Invalid base for number print");
			}
			free(cptr);
			if(cast)
				free(cast);
			break;
		default:
			internal_error(__FILE__, __LINE__, "Invalid node type %d", ndp->type);
		}
	}
	fprintf(fp, "\n#endif\n");
	fclose(fp);
}

static void write_rcbin(FILE *fp)
{
	lan_blk_t *lbp;
	token_t *ttab;
	int ntab;
	int i;

	get_tokentable(&ttab, &ntab);

	for(lbp = lanblockhead; lbp; lbp = lbp->next)
	{
		char *cptr = NULL;
		fprintf(fp, "LANGUAGE 0x%x, 0x%x\n", lbp->lan & 0x3ff, lbp->lan >> 10);
		for(i = 0; i < ntab; i++)
		{
			if(ttab[i].type == tok_language && ttab[i].token == lbp->lan)
			{
				if(ttab[i].alias)
					cptr = dup_u2c(WMC_DEFAULT_CODEPAGE, ttab[i].alias);
				break;
			}
		}
		if(!cptr)
			internal_error(__FILE__, __LINE__, "Filename vanished for language 0x%0x", lbp->lan);
		fprintf(fp, "1 MESSAGETABLE \"%s.bin\"\n", cptr);
		free(cptr);
	}
}

static char *make_string(WCHAR *uc, int len, int codepage)
{
	char *str = xmalloc(7*len + 1);
	char *cptr = str;
	int i;
	int b;

	if(!codepage)
	{
		*cptr++ = ' ';
		*cptr++ = 'L';
		*cptr++ = '"';
		for(i = b = 0; i < len; i++, uc++)
		{
			int n;
			if(*uc < 0x100)
			{
				if(isprint(*uc))
				{
					*cptr++ = (char)*uc;
					b++;
				}
				else
				{
					switch(*uc)
					{
					case '\a': *cptr++ = '\\'; *cptr++ = 'a'; b += 2; break;
					case '\b': *cptr++ = '\\'; *cptr++ = 'b'; b += 2; break;
					case '\f': *cptr++ = '\\'; *cptr++ = 'f'; b += 2; break;
					case '\n': *cptr++ = '\\'; *cptr++ = 'n'; b += 2; break;
					case '\r': *cptr++ = '\\'; *cptr++ = 'r'; b += 2; break;
					case '\t': *cptr++ = '\\'; *cptr++ = 't'; b += 2; break;
					case '\v': *cptr++ = '\\'; *cptr++ = 'v'; b += 2; break;
					case '\\': *cptr++ = '\\'; *cptr++ = '\\'; b += 2; break;
					case '"':  *cptr++ = '\\'; *cptr++ = '"'; b += 2; break;
					default:
						n = sprintf(cptr, "\\x%04x", *uc & 0xffff);
						cptr += n;
						b += n;
					}
				}
			}
			else
			{
				n = sprintf(cptr, "\\x%04x", *uc & 0xffff);
				cptr += n;
				b += n;
			}
			if(i < len-1 && b >= 72)
			{
				*cptr++ = '"';
				*cptr++ = ',';
				*cptr++ = '\n';
				*cptr++ = ' ';
				*cptr++ = 'L';
				*cptr++ = '"';
				b = 0;
			}
		}
		len = (len + 3) & ~3;
		for(; i < len; i++)
		{
			*cptr++ = '\\';
			*cptr++ = 'x';
			*cptr++ = '0';
			*cptr++ = '0';
			*cptr++ = '0';
			*cptr++ = '0';
		}
		*cptr++ = '"';
		*cptr = '\0';
	}
	else
	{
		char *tmp = xmalloc(2*len+1);
		char *cc = tmp;
//		const union cptable *cpdef = find_codepage(codepage);

//		assert(cpdef != NULL);
//		if((i = cp_wcstombs(cpdef, 0, uc, unistrlen(uc)+1, tmp, 2*len+1, NULL, NULL)) < 0)
		if((i = WideCharToMultiByte(codepage, 0, uc, unistrlen(uc)+1, tmp, 2*len+1, NULL, NULL)) < 0)
			internal_error(__FILE__, __LINE__, "Buffer overflow? code %d.", i);
		*cptr++ = ' ';
		*cptr++ = '"';
		for(i = b = 0; i < len; i++, cc++)
		{
			int n;
			if(isprint(*cc))
			{
				*cptr++ = *cc;
				b++;
			}
			else
			{
				switch(*cc)
				{
				case '\a': *cptr++ = '\\'; *cptr++ = 'a'; b += 2; break;
				case '\b': *cptr++ = '\\'; *cptr++ = 'b'; b += 2; break;
				case '\f': *cptr++ = '\\'; *cptr++ = 'f'; b += 2; break;
				case '\n': *cptr++ = '\\'; *cptr++ = 'n'; b += 2; break;
				case '\r': *cptr++ = '\\'; *cptr++ = 'r'; b += 2; break;
				case '\t': *cptr++ = '\\'; *cptr++ = 't'; b += 2; break;
				case '\v': *cptr++ = '\\'; *cptr++ = 'v'; b += 2; break;
				case '\\': *cptr++ = '\\'; *cptr++ = '\\'; b += 2; break;
				case '"':  *cptr++ = '\\'; *cptr++ = '"'; b += 2; break;
				default:
					n = sprintf(cptr, "\\x%02x", *cc & 0xff);
					cptr += n;
					b += n;
				}
			}
			if(i < len-1 && b >= 72)
			{
				*cptr++ = '"';
				*cptr++ = ',';
				*cptr++ = '\n';
				*cptr++ = ' ';
				*cptr++ = '"';
				b = 0;
			}
		}
		len = (len + 3) & ~3;
		for(; i < len; i++)
		{
			*cptr++ = '\\';
			*cptr++ = 'x';
			*cptr++ = '0';
			*cptr++ = '0';
		}
		*cptr++ = '"';
		*cptr = '\0';
		free(tmp);
	}
	return str;
}


static char *make_bin_string(WCHAR *uc, int len, int *retlen, int codepage)
{
	char *str = xmalloc(7 * len + 1);
	int i;
	int b;

	if(!codepage)
	{
		WCHAR *cptr = (WCHAR*)str;

		for(i = b = 0; i < len; i++, uc++)
		{
			*cptr++ = *uc;
		}
		len = (len + 1) & ~1;
		for(; i < len; i++)
		{
			*cptr++ = 0;
		}
		*retlen = len * 2;
	}
	else
	{
		char *tmp = xmalloc(2*len+1);
		char *cc = tmp;
		char *cptr = str;
//		const union cptable *cpdef = find_codepage(codepage);

//		assert(cpdef != NULL);
//		if((i = cp_wcstombs(cpdef, 0, uc, unistrlen(uc)+1, tmp, 2*len+1, NULL, NULL)) < 0)
		if((i = WideCharToMultiByte(codepage, 0, uc, unistrlen(uc)+1, tmp, 2*len+1, NULL, NULL)) < 0)
			internal_error(__FILE__, __LINE__, "Buffer overflow? code %d.", i);
		for(i = b = 0; i < len; i++, cc++)
		{
			*cptr++ = *cc;
			b++;
		}
		len = (len + 3) & ~3;
		for(; i < len; i++)
		{
			*cptr++ = 0;
		}
		free(tmp);
		*retlen = len;
	}
	return str;
}

static void write_rcinline(FILE *fp)
{
	lan_blk_t *lbp;
	int i;
	int j;

	for(lbp = lanblockhead; lbp; lbp = lbp->next)
	{
		unsigned offs = 4 * (lbp->nblk * 3 + 1);
		fprintf(fp, "\n1 MESSAGETABLE\n");
		fprintf(fp, "LANGUAGE 0x%x, 0x%x\n", lbp->lan & 0x3ff, lbp->lan >> 10);
		fprintf(fp, "{\n");
		fprintf(fp, " /* NBlocks    */ 0x%08xL,\n", lbp->nblk);
		for(i = 0; i < lbp->nblk; i++)
		{
			fprintf(fp, " /* Lo,Hi,Offs */ 0x%08xL, 0x%08xL, 0x%08xL,\n",
					lbp->blks[i].idlo,
					lbp->blks[i].idhi,
					offs);
			offs += lbp->blks[i].size;
		}
		for(i = 0; i < lbp->nblk; i++)
		{
			block_t *blk = &lbp->blks[i];
			for(j = 0; j < blk->nmsg; j++)
			{
				char *cptr;
				int l = blk->msgs[j]->len;
				char *comma = j == blk->nmsg-1  && i == lbp->nblk-1 ? "" : ",";
				cptr = make_string(blk->msgs[j]->msg, l, unicodeout ? 0 : blk->msgs[j]->cp);
				fprintf(fp, "\n /* Msg 0x%08x */ 0x%04x, 0x000%c,\n",
					blk->idlo + j,
					(unicodeout ? (l*2+3)&~3 : (l+3)&~3)+4,
					unicodeout ? '1' : '0');
				fprintf(fp, "%s%s\n", cptr, comma);
				free(cptr);
			}
		}
		fprintf(fp, "}\n");
	}
}

void write_rc_file(const char *fname)
{
	FILE *fp;
	char *cptr;

	fp = fopen(fname, "w");
	if(!fp)
	{
		perror(fname);
		exit(1);
	}
	cptr = ctime(&now);
	killnl(cptr, 0);
	fprintf(fp, str_header, input_name ? input_name : "<stdin>", cmdline, cptr);

	if(rcinline)
		write_rcinline(fp);
	else
		write_rcbin(fp);
	fclose(fp);
}

void write_bin_files(void)
{
  lan_blk_t *lbp;
  token_t *ttab;
  int ntab;
  int i;
  int j;
  char fname[16];
  FILE *fp;

  get_tokentable(&ttab, &ntab);

  for (lbp = lanblockhead; lbp; lbp = lbp->next)
    {
      unsigned offs = 4 * (lbp->nblk * 3 + 1); 

      char *cptr = NULL;
      for(i = 0; i < ntab; i++)
	{
	  if (ttab[i].type == tok_language && ttab[i].token == lbp->lan)
	    {
	      if (ttab[i].alias)
		cptr = dup_u2c(WMC_DEFAULT_CODEPAGE, ttab[i].alias);
	      break;
	    }
	}

      if (!cptr)
	internal_error(__FILE__, __LINE__, "Filename vanished for language 0x%0x", lbp->lan);
      sprintf(fname, "%s.bin", cptr);
      free(cptr);

      fp = fopen(fname, "wb");
      if (!fp)
	{
	  perror(fname);
	  exit(1);
	}

      /* NBlocks */
      fwrite(&lbp->nblk, sizeof(unsigned long), 1, fp);
      for(i = 0; i < lbp->nblk; i++)
	{
	  /* Lo */
	  fwrite(&lbp->blks[i].idlo, sizeof(unsigned long), 1, fp);
	  /* Hi */
	  fwrite(&lbp->blks[i].idhi, sizeof(unsigned long), 1, fp);
	  /* Offs */
	  fwrite(&offs, sizeof(unsigned long), 1, fp);
	  offs += lbp->blks[i].size;
	}

      for (i = 0; i < lbp->nblk; i++)
	{
	  block_t *blk = &lbp->blks[i];
	  for (j = 0; j < blk->nmsg; j++)
	    {
	      char *cptr;
	      int l = blk->msgs[j]->len;
	      int retlen = 0;

	      unsigned short length = (unicodeout ? (l*2+3)&~3 : (l+3)&~3)+4;
	      unsigned short flags = unicodeout ? 1 : 0;

	      /* Lo */
	      fwrite(&length, sizeof(unsigned short), 1, fp);
	      /* Hi */
	      fwrite(&flags, sizeof(unsigned short), 1, fp);

	      /* message text */
	      cptr = make_bin_string(blk->msgs[j]->msg, l, &retlen, unicodeout ? 0 : blk->msgs[j]->cp);
	      fwrite(cptr, retlen, 1, fp);

	      free(cptr);
	    }
	}

      fclose(fp);
    }
}

/* EOF */
