/*
 * Swiss army knife for manipulating and debugging PDFs.
 *
 * There are a few major modes of operation:
 *
 *   show -- pretty-print objects and streams
 *   draw -- render pages to bitmap
 *   clean -- simple rewrite of pdf file
 *   edit -- edit pages (impose and copy operations)
 */

#include "fitz.h"
#include "mupdf.h"

/*
 * Common operations.
 * Parse page selectors.
 * Load and decrypt a PDF file.
 * Select pages.
 */

char *srcname = "(null)";
pdf_xref *src = nil;
pdf_pagetree *srcpages = nil;

void die(fz_error *eo)
{
    fflush(stdout);
    fprintf(stderr, "%s:%d: %s(): %s\n", eo->file, eo->line, eo->func, eo->msg);
    fflush(stderr);
    abort();
}

void closesrc(void)
{
	if (srcpages)
	{
		pdf_droppagetree(srcpages);
		srcpages = nil;
	}

	if (src)
	{
		if (src->store)
		{
			pdf_dropstore(src->store);
			src->store = nil;
		}
		pdf_closexref(src);
		src = nil;
	}

	srcname = nil;
}

void opensrc(char *filename, char *password, int loadpages)
{
	fz_error *error;

	closesrc();

	srcname = filename;

	error = pdf_newxref(&src);
	if (error)
		die(error);

	error = pdf_loadxref(src, filename);
	if (error)
	{
		fz_warn("trying to repair");
		error = pdf_repairxref(src, filename);
		if (error)
			die(error);
	}

	error = pdf_decryptxref(src);
	if (error)
		die(error);

	if (src->crypt)
	{
		error = pdf_setpassword(src->crypt, password);
		if (error)
			die(error);
	}

	if (loadpages)
	{
		error = pdf_loadpagetree(&srcpages, src);
		if (error)
			die(error);
	}
}

void preloadobjstms(void)
{
        fz_error *error;
        fz_obj *obj;
        int i;

        for (i = 0; i < src->len; i++)
        {
                if (src->table[i].type == 'o')
                {
                        error = pdf_loadobject(&obj, src, i, 0);
                        if (error) die(error);
                        fz_dropobj(obj);
                }
        }
}

/* --------------------------------------------------------------------- */

/*
 * Debug print parts of the PDF.
 */

int showbinary = 0;
int showdecode = 0;
int showcolumn;

void showusage(void)
{
	fprintf(stderr, "usage: pdftool show [-bd] <file> [xref] [trailer] [object numbers]\n");
	fprintf(stderr, "  -b  \tprint streams as raw binary data\n");
	fprintf(stderr, "  -d  \tdecode streams\n");
	exit(1);
}

void showtrailer(void)
{
	if (!src)
		die(fz_throw("no file specified"));
	printf("trailer\n");
	fz_debugobj(src->trailer);
	printf("\n\n");
}

void showxref(void)
{
	if (!src)
		die(fz_throw("no file specified"));
	pdf_debugxref(src);
	printf("\n");
}

void showsafe(unsigned char *buf, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		if (buf[i] == '\r' || buf[i] == '\n') {
			putchar('\n');
			showcolumn = 0;
		}
		else if (buf[i] < 32 || buf[i] > 126) {
			putchar('.');
			showcolumn ++;
		}
		else {
			putchar(buf[i]);
			showcolumn ++;
		}
		if (showcolumn == 79) {
			putchar('\n');
			showcolumn = 0;
		}
	}
}

void showstream(int num, int gen)
{
	fz_error *error;
	fz_stream *stm;
	unsigned char buf[2048];
	int n;

	showcolumn = 0;

	if (showdecode)
		error = pdf_openstream(&stm, src, num, gen);
	else
		error = pdf_openrawstream(&stm, src, num, gen);
	if (error)
		die(error);

	while (1)
	{
		n = fz_read(stm, buf, sizeof buf);
		if (n == 0)
			break;
		if (n < 0)
			die(fz_ioerror(stm));

		if (showbinary)
			fwrite(buf, 1, n, stdout);
		else
			showsafe(buf, n);
	}

	fz_dropstream(stm);
}

void showobject(int num, int gen)
{
	fz_error *error;
	fz_obj *obj;

	if (!src)
		die(fz_throw("no file specified"));

	error = pdf_loadobject(&obj, src, num, gen);
	if (error)
		die(error);

	printf("%d %d obj\n", num, gen);
	fz_debugobj(obj);
	printf("\n");

	if (pdf_isstream(src, num, gen))
	{
		printf("stream\n");
		showstream(num, gen);
		printf("endstream\n");
	}

	printf("endobj\n\n");

	fz_dropobj(obj);
}

void
showmain(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "bd")) != -1)
	{
		switch (c)
		{
		case 'b': showbinary ++; break;
		case 'd': showdecode ++; break;
		default:
			showusage();
			break;
		}
	}

	if (optind == argc)
		showusage();

	opensrc(argv[optind++], "", 0);

	if (optind == argc)
		showtrailer();

	while (optind < argc)
	{
		if (!strcmp(argv[optind], "trailer"))
			showtrailer();
		else if (!strcmp(argv[optind], "xref"))
			showxref();
		else
			showobject(atoi(argv[optind]), 0);
		optind++;
	}
}

/* --------------------------------------------------------------------- */

/*
 * Clean tool.
 * Rewrite PDF.
 * Garbage collect.
 * Decompress streams.
 * Encrypt or decrypt.
 */

void
cleanusage(void)
{
	fprintf(stderr,
			"usage: pdftool clean [options] input.pdf [outfile.pdf]\n"
			"  -d -\tpassword for decryption\n"
			"  -g  \tgarbage collect unused objects\n"
			"  -x  \texpand compressed streams\n"
			"  -e  \tencrypt output\n"
			"    -u -\tset user password for encryption\n"
			"    -o -\tset owner password\n"
			"    -p -\tset permissions (combine letters 'pmca')\n"
			"    -n -\tkey length in bits: 40 <= n <= 128\n");
	exit(1);
}

void
cleanexpand(void)
{
	fz_error *error;
	fz_obj *stmobj;
	fz_buffer *buf;
	fz_obj *stmlen;
	int i, gen;

	for (i = 0; i < src->len; i++)
	{
		if (src->table[i].type == 'n')
		{
			gen = src->table[i].gen;

			if (pdf_isstream(src, i, gen))
			{
				error = pdf_loadobject(&stmobj, src, i, gen);
				if (error) die(error);

				error = pdf_loadstream(&buf, src, i, gen);
				if (error) die(error);

				fz_dictdels(stmobj, "Filter");
				fz_dictdels(stmobj, "DecodeParms");

				error = fz_newint(&stmlen, buf->wp - buf->rp);
				if (error) die(error);
				error = fz_dictputs(stmobj, "Length", stmlen);
				if (error) die(error);
				fz_dropobj(stmlen);

				pdf_updateobject(src, i, gen, stmobj);
				pdf_updatestream(src, i, gen, buf);

				fz_dropobj(stmobj);
			}
		}
	}
}

void
cleanmain(int argc, char **argv)
{
	int doencrypt = 0;
	int dogarbage = 0;
	int doexpand = 0;
	pdf_crypt *encrypt = nil;
	char *infile;
	char *outfile = "out.pdf";
	char *userpw = "";
	char *ownerpw = "";
	unsigned perms = 0xfffff0c0;	/* nothing allowed */
	int keylen = 40;
	char *password = "";
	fz_error *error;
	int c;

	while ((c = getopt(argc, argv, "d:egn:o:p:u:x")) != -1)
	{
		switch (c)
		{
		case 'p':
			/* see TABLE 3.15 User access permissions */
			perms = 0xfffff0c0;
			if (strchr(optarg, 'p')) /* print */
				perms |= (1 << 2) | (1 << 11);
			if (strchr(optarg, 'm')) /* modify */
				perms |= (1 << 3) | (1 << 10);
			if (strchr(optarg, 'c')) /* copy */
				perms |= (1 << 4) | (1 << 9);
			if (strchr(optarg, 'a')) /* annotate / forms */
				perms |= (1 << 5) | (1 << 8);
			break;
		case 'd': password = optarg; break;
		case 'e': doencrypt ++; break;
		case 'g': dogarbage ++; break;
		case 'n': keylen = atoi(optarg); break;
		case 'o': ownerpw = optarg; break;
		case 'u': userpw = optarg; break;
		case 'x': doexpand ++; break;
		default: cleanusage(); break;
		}
	}

	if (argc - optind < 1)
		cleanusage();

	infile = argv[optind++];
	if (argc - optind > 0)
		outfile = argv[optind++];

	opensrc(infile, password, 0);

	if (doencrypt)
	{
		fz_obj *id = fz_dictgets(src->trailer, "ID");
		if (!id)
		{
			error = fz_packobj(&id, "[(ABCDEFGHIJKLMNOP)(ABCDEFGHIJKLMNOP)]");
			if (error)
				die(error);
		}
		else
			fz_keepobj(id);

		error = pdf_newencrypt(&encrypt, userpw, ownerpw, perms, keylen, id);
		if (error)
			die(error);

		fz_dropobj(id);
	}

	if (doexpand)
		cleanexpand();

	if (dogarbage)
	{
		preloadobjstms();
		pdf_garbagecollect(src);
	}

	error = pdf_savexref(src, outfile, encrypt);
	if (error)
		die(error);

	if (encrypt)
		pdf_dropcrypt(encrypt);

	pdf_closexref(src);
}


/* --------------------------------------------------------------------- */

/*
 * Draw pages to PPM bitmaps.
 */

enum { DRAWPNM, DRAWTXT, DRAWXML };

fz_renderer *drawgc = nil;
int drawmode = DRAWPNM;
char *drawpattern = "out%0.3d.pnm";
pdf_page *drawpage = nil;
float drawzoom = 1.0;
int drawrotate = 0;
int drawbands = 1;
int drawcount = 0;

void
drawusage(void)
{
	fprintf(stderr,
		"usage: pdftool draw [options] [file.pdf pages ... ]\n"
			"  -b -\tdraw page in N bands\n"
			"  -d -\tpassword for decryption\n"
			"  -o -\tpattern (%%d for page number) for output file\n"
			"  -r -\tresolution in dpi\n"
			"  -t  \tutf-8 text output instead of graphics\n"
			"  -x  \txml dump of display tree\n"
			"  example:\n"
			"    pdftool draw -o out%%0.3d.pnm a.pdf 1-3,5,9-\n");
	exit(1);
}

void
drawloadpage(int pagenum)
{
	fz_error *error;
	fz_obj *pageobj;

	pageobj = pdf_getpageobject(srcpages, pagenum - 1);
	error = pdf_loadpage(&drawpage, src, pageobj);
	if (error)
		die(error);

	fprintf(stderr, "draw %s page %d mediabox [ %g %g %g %g ] rotate %d\n",
			srcname, pagenum,
			drawpage->mediabox.x0, drawpage->mediabox.y0,
			drawpage->mediabox.x1, drawpage->mediabox.y1,
			drawpage->rotate);
}

void
drawfreepage(void)
{
	pdf_droppage(drawpage);
	drawpage = nil;
}

void
drawpnm(int pagenum)
{
	fz_error *error;
	fz_matrix ctm;
	fz_irect bbox;
	fz_pixmap *pix;
	char namebuf[256];
	char buf[256];
	int x, y, w, h, b, bh;
	int fd;

	drawloadpage(pagenum);

	ctm = fz_identity();
	ctm = fz_concat(ctm, fz_translate(0, -drawpage->mediabox.y1));
	ctm = fz_concat(ctm, fz_scale(drawzoom, -drawzoom));
	ctm = fz_concat(ctm, fz_rotate(drawrotate + drawpage->rotate));

	bbox = fz_roundrect(fz_transformaabb(ctm, drawpage->mediabox));
	w = bbox.x1 - bbox.x0;
	h = bbox.y1 - bbox.y0;
	bh = h / drawbands;

	if (drawpattern)
	{
		sprintf(namebuf, drawpattern, drawcount++);
		fd = open(namebuf, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (fd < 0)
			die(fz_throw("ioerror: could not open file '%s'", namebuf));
	}
	else
		fd = 1;

	sprintf(buf, "P6\n%d %d\n255\n", w, h);
	write(fd, buf, strlen(buf));

	error = fz_newpixmap(&pix, bbox.x0, bbox.y0, w, bh, 4);
	if (error)
		die(error);

	memset(pix->samples, 0xff, pix->h * pix->w * pix->n);

	for (b = 0; b < drawbands; b++)
	{
		if (drawbands > 1)
			fprintf(stderr, "drawing band %d / %d\n", b + 1, drawbands);

		error = fz_rendertreeover(drawgc, pix, drawpage->tree, ctm);
		if (error)
			die(error);

		for (y = 0; y < pix->h; y++)
		{
			unsigned char *src = pix->samples + y * pix->w * 4;
			unsigned char *dst = src;

			for (x = 0; x < pix->w; x++)
			{
				dst[x * 3 + 0] = src[x * 4 + 1];
				dst[x * 3 + 1] = src[x * 4 + 2];
				dst[x * 3 + 2] = src[x * 4 + 3];
			}

			write(fd, dst, pix->w * 3);

			memset(src, 0xff, pix->w * 4);
		}

		pix->y += bh;
		if (pix->y + pix->h > bbox.y1)
			pix->h = bbox.y1 - pix->y;
	}

	fz_droppixmap(pix);

	if (drawpattern)
		close(fd);

	drawfreepage();
}

void
drawtxt(int pagenum)
{
	fz_error *error;
	pdf_textline *line;
	fz_matrix ctm;

	drawloadpage(pagenum);

	ctm = fz_concat(
			fz_translate(0, -drawpage->mediabox.y1),
			fz_scale(drawzoom, -drawzoom));

	error = pdf_loadtextfromtree(&line, drawpage->tree, ctm);
	if (error)
		die(error);

	pdf_debugtextline(line);
	pdf_droptextline(line);

	drawfreepage();
}

void
drawxml(int pagenum)
{
	drawloadpage(pagenum);
	fz_debugtree(drawpage->tree);
	drawfreepage();
}

void
drawpages(char *pagelist)
{
	int page, spage, epage;
	char *spec, *dash;

	if (!src)
		drawusage();

	spec = strsep(&pagelist, ",");
	while (spec)
	{
		dash = strchr(spec, '-');

		if (dash == spec)
			spage = epage = 1;
		else
			spage = epage = atoi(spec);

		if (dash)
		{
			if (strlen(dash) > 1)
				epage = atoi(dash + 1);
			else
				epage = pdf_getpagecount(srcpages);
		}

		if (spage > epage)
			page = spage, spage = epage, epage = page;

		for (page = spage; page <= epage; page++)
		{
			if (page < 1 || page > pdf_getpagecount(srcpages))
				continue;
			switch (drawmode)
			{
			case DRAWPNM: drawpnm(page); break;
			case DRAWTXT: drawtxt(page); break;
			case DRAWXML: drawxml(page); break;
			}
		}

		spec = strsep(&pagelist, ",");
	}
}

void
drawmain(int argc, char **argv)
{
	fz_error *error;
	char *password = "";
	int c;

	while ((c = getopt(argc, argv, "b:d:o:r:tx")) != -1)
	{
		switch (c)
		{
		case 'b': drawbands = atoi(optarg); break;
		case 'd': password = optarg; break;
		case 'o': drawpattern = optarg; break;
		case 'r': drawzoom = atof(optarg) / 72.0; break;
		case 't': drawmode = DRAWTXT; break;
		case 'x': drawmode = DRAWXML; break;
		default:
			drawusage();
			break;
		}
	}

	if (optind == argc)
		drawusage();

	error = fz_newrenderer(&drawgc, pdf_devicergb, 0, 1024 * 512);
	if (error)
		die(error);

	while (optind < argc)
	{
		if (strstr(argv[optind], ".pdf"))
			opensrc(argv[optind], password, 1);
		else
			drawpages(argv[optind]);
		optind++;
	}

	closesrc();

	fz_droprenderer(drawgc);
}

/* --------------------------------------------------------------------- */

/*
 * Edit tool.
 * Copy or impose pages from other pdf files into output pdf.
 */

/* for each source pdf, build a list of objects to transplant.
 * for each source pdf, do the transplants at the end of object collecting.
 * build a new page tree structure for output.
 * change page nodes into xobjects for over and n-up modes.
 * create new page nodes.
 * create new page tree.
 */

enum { COPY, OVER, NUP2, NUP4, NUP8 };

pdf_xref *editxref = nil;
fz_obj *editpagelist = nil;
fz_obj *editmodelist = nil;
fz_obj *editobjects = nil;
int editmode = COPY;

void
editusage(void)
{
	fprintf(stderr, "usage: pdftool edit [-o file.pdf] [mode file.pdf pages ... ]\n");
	fprintf(stderr, "  mode is one of: copy over 2up 4up 8up\n");
	fprintf(stderr, "  pages is a comma separated list of ranges\n");
	fprintf(stderr, "  example:\n");
	fprintf(stderr, "    pdftool edit -o output.pdf copy one.pdf 1-3,5,9 two.pdf 1-\n");
	exit(1);
}

void
editcopy(int pagenum)
{
	fz_error *error;
	fz_obj *obj;
	fz_obj *ref;
	fz_obj *num;

	printf("copy %s page %d\n", srcname, pagenum);

	ref = srcpages->pref[pagenum - 1];
	obj = pdf_getpageobject(srcpages, pagenum - 1);

	fz_dictdels(obj, "Parent");
	// fz_dictdels(obj, "B");
	// fz_dictdels(obj, "PieceInfo");
	// fz_dictdels(obj, "Metadata");
	// fz_dictdels(obj, "Annots");
	// fz_dictdels(obj, "Tabs");

	pdf_updateobject(src, fz_tonum(ref), fz_togen(ref), obj);

	error = fz_arraypush(editobjects, ref);
	if (error)
		die(error);

	error = fz_newint(&num, editmode);
	if (error)
		die(error);

	error = fz_arraypush(editmodelist, num);
	if (error)
		die(error);

	fz_dropobj(num);
}

void
editflushobjects(void)
{
	fz_error *error;
	fz_obj *results;
	int i;

	error = pdf_transplant(editxref, src, &results, editobjects);
	if (error)
		die(error);

	for (i = 0; i < fz_arraylen(results); i++)
	{
		error = fz_arraypush(editpagelist, fz_arrayget(results, i));
		if (error)
			die(error);
	}

	fz_dropobj(results);
}

void
editflushpagetree(void)
{

	/* TODO: merge pages where editmode != COPY by turning them into XObjects
	   and creating a new page object with resource dictionary and content
	   stream placing the xobjects on the page. */
}

void
editflushcatalog(void)
{
	fz_error *error;
	int rootnum, rootgen;
	int listnum, listgen;
	fz_obj *listref;
	fz_obj *obj;
	int i;

	/* Create page tree and add back-links */

	error = pdf_allocobject(editxref, &listnum, &listgen);
	if (error)
		die(error);

	error = fz_packobj(&obj, "<</Type/Pages/Count %i/Kids %o>>",
			fz_arraylen(editpagelist),
			editpagelist);
	if (error)
		die(error);

	pdf_updateobject(editxref, listnum, listgen, obj);

	fz_dropobj(obj);

	error = fz_newindirect(&listref, listnum, listgen);
	if (error)
		die(error);

	for (i = 0; i < fz_arraylen(editpagelist); i++)
	{
		int num = fz_tonum(fz_arrayget(editpagelist, i));
		int gen = fz_togen(fz_arrayget(editpagelist, i));

		error = pdf_loadobject(&obj, editxref, num, gen);
		if (error)
			die(error);

		error = fz_dictputs(obj, "Parent", listref);
		if (error)
			die(error);

		pdf_updateobject(editxref, num, gen, obj);

		fz_dropobj(obj);
	}

	/* Create catalog */

	error = pdf_allocobject(editxref, &rootnum, &rootgen);
	if (error)
		die(error);

	error = fz_packobj(&obj, "<</Type/Catalog/Pages %r>>", listnum, listgen);
	if (error)
		die(error);

	pdf_updateobject(editxref, rootnum, rootgen, obj);

	fz_dropobj(obj);

	/* Create trailer */

	error = fz_packobj(&editxref->trailer, "<</Root %r>>", rootnum, rootgen);
	if (error)
		die(error);
}

void
editpages(char *pagelist)
{
	int page, spage, epage;
	char *spec, *dash;

	if (!src)
		editusage();

	spec = strsep(&pagelist, ",");
	while (spec)
	{
		dash = strchr(spec, '-');

		if (dash == spec)
			spage = epage = 1;
		else
			spage = epage = atoi(spec);

		if (dash)
		{
			if (strlen(dash) > 1)
				epage = atoi(dash + 1);
			else
				epage = pdf_getpagecount(srcpages);
		}

		if (spage > epage)
			page = spage, spage = epage, epage = page;

		for (page = spage; page <= epage; page++)
		{
			if (page < 1 || page > pdf_getpagecount(srcpages))
				continue;
			editcopy(page);
		}

		spec = strsep(&pagelist, ",");
	}
}

void
editmain(int argc, char **argv)
{
	char *outfile = "out.pdf";
	fz_error *error;
	int c;

	while ((c = getopt(argc, argv, "o:")) != -1)
	{
		switch (c)
		{
		case 'o':
			outfile = optarg;
			break;
		default:
			editusage();
			break;
		}
	}

	if (optind == argc)
		editusage();

	error = pdf_newxref(&editxref);
	if (error)
		die(error);

	error = pdf_initxref(editxref);
	if (error)
		die(error);

	error = fz_newarray(&editpagelist, 100);
	if (error)
		die(error);

	error = fz_newarray(&editmodelist, 100);
	if (error)
		die(error);

	while (optind < argc)
	{
		if (strstr(argv[optind], ".pdf"))
		{
			if (editobjects)
				editflushobjects();

			opensrc(argv[optind], "", 1);

			error = fz_newarray(&editobjects, 100);
			if (error)
				die(error);
		}
		else if (!strcmp(argv[optind], "copy"))
			editmode = COPY;
		else if (!strcmp(argv[optind], "over"))
			editmode = OVER;
		else if (!strcmp(argv[optind], "2up"))
			editmode = NUP2;
		else if (!strcmp(argv[optind], "4up"))
			editmode = NUP4;
		else if (!strcmp(argv[optind], "8up"))
			editmode = NUP8;
		else
			editpages(argv[optind]);
		optind++;
	}

	if (editobjects)
		editflushobjects();

	closesrc();

	editflushpagetree();
	editflushcatalog();

	error = pdf_savexref(editxref, outfile, nil);
	if (error)
		die(error);

	pdf_closexref(editxref);
}

/* --------------------------------------------------------------------- */

/*
 * Main!
 */

void
mainusage(void)
{
	fprintf(stderr, "usage: pdftool <command> [options...]\n");
	fprintf(stderr, "  command is one of: show, draw, clean, edit\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	if (argc >= 2)
	{
		optind = 2;
		if (!strcmp(argv[1], "show"))
			showmain(argc, argv);
		else if (!strcmp(argv[1], "draw"))
			drawmain(argc, argv);
		else if (!strcmp(argv[1], "clean"))
			cleanmain(argc, argv);
		else if (!strcmp(argv[1], "edit"))
			editmain(argc, argv);
		else
			mainusage();
	}
	else
		mainusage();
	return 0;
}

