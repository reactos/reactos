#include "fitz.h"
#include "samus.h"

#include <expat.h>

#define XMLBUFLEN 4096

struct sa_xmlitem_s
{
	char *name;
	char **atts;
	sa_xmlitem *up;
	sa_xmlitem *down;
	sa_xmlitem *next;
};

struct sa_xmlparser_s
{
	fz_error *error;
	sa_xmlitem *root;
	sa_xmlitem *head;
	int nexted;
	int downed;
};

static void dropitem(sa_xmlitem *item)
{
	sa_xmlitem *next;
	while (item)
	{
		next = item->next;
		if (item->down)
			dropitem(item->down);
		fz_free(item);
		item = next;
	}
}

static void onopentag(void *zp, const char *name, const char **atts)
{
	struct sa_xmlparser_s *sp = zp;
	sa_xmlitem *item;
	sa_xmlitem *tail;
	int namelen;
	int attslen;
	int textlen;
	char *p;
	int i;

	if (sp->error)
		return;

	/* count size to alloc */

	namelen = strlen(name) + 1;
	attslen = sizeof(char*);
	textlen = 0;
	for (i = 0; atts[i]; i++)
	{
		attslen += sizeof(char*);
		textlen += strlen(atts[i]) + 1;
	}

	item = fz_malloc(sizeof(sa_xmlitem) + attslen + namelen + textlen);
	if (!item)
	{
		sp->error = fz_outofmem;
		return;
	}

	/* copy strings to new memory */

	item->atts = (char**) (((char*)item) + sizeof(sa_xmlitem));
	item->name = ((char*)item) + sizeof(sa_xmlitem) + attslen;
	p = ((char*)item) + sizeof(sa_xmlitem) + attslen + namelen;

	strcpy(item->name, name);
	for (i = 0; atts[i]; i++)
	{
		item->atts[i] = p;
		strcpy(item->atts[i], atts[i]);
		p += strlen(p) + 1;
	}

	item->atts[i] = 0;

	/* link item into tree */

	item->up = sp->head;
	item->down = nil;
	item->next = nil;

	if (!sp->head)
	{
		sp->root = item;
		sp->head = item;
		return;
	}

	if (!sp->head->down)
	{
		sp->head->down = item;
		sp->head = item;
		return;
	}

	tail = sp->head->down;
	while (tail->next)
		tail = tail->next;
	tail->next = item;
	sp->head = item;
}

static void onclosetag(void *zp, const char *name)
{
	struct sa_xmlparser_s *sp = zp;

	if (sp->error)
		return;

	if (sp->head)
		sp->head = sp->head->up;
}

static inline int isxmlspace(int c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void ontext(void *zp, const char *buf, int len)
{
	struct sa_xmlparser_s *sp = zp;
	int i;

	if (sp->error)
		return;

	for (i = 0; i < len; i++)
	{
		if (!isxmlspace(buf[i]))
		{
			char *tmp = fz_malloc(len + 1);
			const char *atts[] = {"", tmp, 0};
			if (!tmp)
			{
				sp->error = fz_outofmem;
				return;
			}
			memcpy(tmp, buf, len);
			tmp[len] = 0;
			onopentag(zp, "", atts);
			onclosetag(zp, "");
			fz_free(tmp);
			return;
		}
	}
}

fz_error *
sa_openxml(sa_xmlparser **spp, fz_stream *file, int ns)
{
	fz_error *error = nil;
	sa_xmlparser *sp;
	XML_Parser xp;
	char *buf;
	int len;

	sp = fz_malloc(sizeof(sa_xmlparser));
	if (!sp)
		return fz_outofmem;

	sp->error = nil;
	sp->root = nil;
	sp->head = nil;
	sp->downed = 0;
	sp->nexted = 0;

	if (ns)
		xp = XML_ParserCreateNS(nil, ns);
	else
		xp = XML_ParserCreate(nil);
	if (!xp)
	{
		fz_free(sp);
		return fz_outofmem;
	}

	XML_SetUserData(xp, sp);
	XML_SetParamEntityParsing(xp, XML_PARAM_ENTITY_PARSING_NEVER);

	XML_SetStartElementHandler(xp, onopentag);
	XML_SetEndElementHandler(xp, onclosetag);
	XML_SetCharacterDataHandler(xp, ontext);

	while (1)
	{
		buf = XML_GetBuffer(xp, XMLBUFLEN);

		len = fz_read(file, buf, XMLBUFLEN);
		if (len < 0)
		{
			error = fz_ioerror(file);
			goto cleanup;
		}

		if (!XML_ParseBuffer(xp, len, len == 0))
		{
			error = fz_throw("ioerror: xml: %s",
					XML_ErrorString(XML_GetErrorCode(xp)));
			goto cleanup;
		}

		if (sp->error)
		{
			error = sp->error;
			sp->error = nil;
			goto cleanup;
		}

		if (len == 0)
			break;
	}

	sp->head = nil;
	*spp = sp;
	return nil;

cleanup:
	if (sp->root)
		dropitem(sp->root);
	fz_free(sp);
	XML_ParserFree(xp);
	return error;
}

void
sa_closexml(sa_xmlparser *sp)
{
	if (sp->root)
		dropitem(sp->root);
	fz_free(sp);
}

static void indent(int n)
{
	while (n--)
		printf("  ");
}

void
sa_debugxml(sa_xmlitem *item, int level)
{
	int i;

	while (item)
	{
		indent(level);

		if (strlen(item->name) == 0)
			printf("%s\n", item->atts[1]);
		else
		{
			printf("<%s", item->name);

			for (i = 0; item->atts[i]; i += 2)
				printf(" %s=\"%s\"", item->atts[i], item->atts[i+1]);

			if (item->down)
			{
				printf(">\n");
				sa_debugxml(item->down, level + 1);
				indent(level);
				printf("</%s>\n", item->name);
			}
			else
				printf(" />\n");
		}

		item = item->next;
	}
}

sa_xmlitem *
sa_xmlnext(sa_xmlparser *sp)
{
	if (sp->downed)
		return nil;

	if (!sp->head)
	{
		sp->head = sp->root;
		return sp->head;
	}

	if (!sp->nexted)
	{
		sp->nexted = 1;
		return sp->head;
	}

	if (sp->head->next)
	{
		sp->head = sp->head->next;
		return sp->head;
	}

	return nil;
}

void
sa_xmldown(sa_xmlparser *sp)
{
	if (!sp->downed && sp->head && sp->head->down)
		sp->head = sp->head->down;
	else
		sp->downed ++;
	sp->nexted = 0;
}

void
sa_xmlup(sa_xmlparser *sp)
{
	if (!sp->downed && sp->head && sp->head->up)
		sp->head = sp->head->up;
	else
		sp->downed --;
}

char *
sa_xmlname(sa_xmlitem *item)
{
	return item->name;
}

char *
sa_xmlatt(sa_xmlitem *item, char *att)
{
	int i;
	for (i = 0; item->atts[i]; i += 2)
		if (!strcmp(item->atts[i], att))
			return item->atts[i + 1];
	return nil;
}

