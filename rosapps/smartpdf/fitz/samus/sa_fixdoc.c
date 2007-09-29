/*
 * FixedDocumentSequence and FixedDocument parts.
 */

#include "fitz.h"
#include "samus.h"

typedef struct sa_fixdoc_s sa_fixdoc;
typedef struct sa_fixpage_s sa_fixpage;

struct sa_fixdocseq_s
{
	int count;	/* pages */
	sa_fixdoc *docs;
};

struct sa_fixdoc_s
{
	int count;
	sa_fixpage *pages;
	sa_fixdoc *next;
};

struct sa_fixpage_s
{
	char *part;
	int width;
	int height;
	/* char **linktargets; */
	sa_fixpage *next;
};

/*
 * Debugging
 */

static void
sa_debugfixdoc(sa_fixdoc *doc, int i)
{
	sa_fixpage *page = doc->pages;
	printf("  FixedDocument\n  {\n");
	while (page)
	{
		printf("    FixedPage %d w=%d h=%d %s\n", i,
				page->width, page->height, page->part);
		page = page->next;
		i ++;
	}
	printf("  }\n");
}

void
sa_debugfixdocseq(sa_fixdocseq *seq)
{
	sa_fixdoc *doc = seq->docs;
	int i = 0;
	printf("FixedDocumentSequence count=%d\n{\n", seq->count);
	while (doc)
	{
		sa_debugfixdoc(doc, i);
		i += doc->count;
		doc = doc->next;
	}
	printf("}\n");
}

/*
 * Free data structures
 */

static void
sa_dropfixpage(sa_fixpage *page)
{
	sa_fixpage *next;
	while (page)
	{
		next = page->next;
		fz_free(page->part);
		fz_free(page);
		page = next;
	}
}

static void
sa_dropfixdoc(sa_fixdoc *doc)
{
	sa_fixdoc *next;
	while (doc)
	{
		next = doc->next;
		if (doc->pages)
			sa_dropfixpage(doc->pages);
		fz_free(doc);
		doc = next;
	}
}

void
sa_dropfixdocseq(sa_fixdocseq *seq)
{
	sa_dropfixdoc(seq->docs);
	fz_free(seq);
}

/*
 * Load FixedDocument
 */
static fz_error *
sa_loadfixdoc(sa_fixdoc **docp, sa_package *pack, char *part)
{
	fz_error *error;
	fz_stream *stm;
	sa_xmlparser *parser;
	sa_xmlitem *item;
	sa_fixdoc *doc;
	sa_fixpage *page;
	sa_fixpage *last;
	char buf[1024];

	page = nil;
	last = nil;

	error = sa_openpart(&stm, pack, part);
	if (error)
		return error;

	doc = fz_malloc(sizeof(sa_fixdoc));
	if (!doc)
	{
		error = fz_outofmem;
		goto cleanupstm;
	}

	doc->count = 0;
	doc->pages = nil;
	doc->next = nil;

	error = sa_openxml(&parser, stm, 0);
	if (error)
		goto cleanupdoc;

	item = sa_xmlnext(parser);
	while (item)
	{
		if (!strcmp(sa_xmlname(item), "FixedDocument"))
		{
			sa_xmldown(parser);
			item = sa_xmlnext(parser);
			while (item)
			{
				if (!strcmp(sa_xmlname(item), "PageContent"))
				{
					char *src = sa_xmlatt(item, "Source");
					char *w = sa_xmlatt(item, "Width");
					char *h = sa_xmlatt(item, "Height");

					if (!w) w = "0";
					if (!h) h = "0";

					if (src)
					{
						sa_resolvepath(buf, part, src, sizeof buf);
						page = fz_malloc(sizeof(sa_fixpage));
						if (!page)
						{
							error = fz_outofmem;
							goto cleanupxml;
						}

						page->part = fz_strdup(buf);
						if (!page->part)
						{
							fz_free(page);
							error = fz_outofmem;
							goto cleanupxml;
						}

						page->width = atoi(w);
						page->height = atoi(h);

						if (last)
							last->next = page;
						else
							doc->pages = page;
						doc->count ++;
						last = page;
					}
				}
				item = sa_xmlnext(parser);
			}
			sa_xmlup(parser);
		}
		item = sa_xmlnext(parser);
	}

	sa_closexml(parser);
	fz_dropstream(stm);
	*docp = doc;
	return nil;

cleanupxml:
	sa_closexml(parser);
cleanupdoc:
	sa_dropfixdoc(doc);
cleanupstm:
	fz_dropstream(stm);
	return error;
}

/*
 * Load FixedDocumentSequence
 */
fz_error *
sa_loadfixdocseq(sa_fixdocseq **seqp, sa_package *pack, char *part)
{
	fz_error *error;
	fz_stream *stm;
	sa_xmlparser *parser;
	sa_xmlitem *item;
	sa_fixdocseq *seq;
	sa_fixdoc *doc;
	sa_fixdoc *last;
	char buf[1024];

	seq = nil;
	last = nil;

	error = sa_openpart(&stm, pack, part);
	if (error)
		return error;

	seq = fz_malloc(sizeof(sa_fixdocseq));
	if (!seq)
	{
		error = fz_outofmem;
		goto cleanupstm;
	}

	seq->count = 0;
	seq->docs = nil;

	error = sa_openxml(&parser, stm, 0);
	if (error)
		goto cleanupseq;

	item = sa_xmlnext(parser);
	while (item)
	{
		if (!strcmp(sa_xmlname(item), "FixedDocumentSequence"))
		{
			sa_xmldown(parser);
			item = sa_xmlnext(parser);
			while (item)
			{
				if (!strcmp(sa_xmlname(item), "DocumentReference"))
				{
					char *src = sa_xmlatt(item, "Source");
					if (src)
					{
						sa_resolvepath(buf, part, src, sizeof buf);
						error = sa_loadfixdoc(&doc, pack, buf);
						if (error)
							goto cleanupxml;
						if (last)
							last->next = doc;
						else
							seq->docs = doc;
						seq->count += doc->count;
						last = doc;
					}
				}
				item = sa_xmlnext(parser);
			}
			sa_xmlup(parser);
		}
		item = sa_xmlnext(parser);
	}

	sa_closexml(parser);
	fz_dropstream(stm);
	*seqp = seq;
	return nil;

cleanupxml:
	sa_closexml(parser);
cleanupseq:
	sa_dropfixdocseq(seq);
cleanupstm:
	fz_dropstream(stm);
	return error;
}

/*
 * Accessors
 */

int
sa_getpagecount(sa_fixdocseq *seq)
{
	return seq->count;
}

char *
sa_getpagepart(sa_fixdocseq *seq, int idx)
{
	sa_fixdoc *doc = seq->docs;
	int cur = 0;

	if (idx < 0 || idx >= seq->count)
		return nil;

	while (doc)
	{
		sa_fixpage *page = doc->pages;
		while (page)
		{
			if (idx == cur)
				return page->part;
			page = page->next;
			cur ++;
		}
		doc = doc->next;
	}

	return nil;
}

