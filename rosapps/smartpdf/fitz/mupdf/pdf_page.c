#include <fitz.h>
#include <mupdf.h>

static fz_error *
runone(pdf_csi *csi, pdf_xref *xref, fz_obj *rdb, fz_obj *stmref)
{
	fz_error *error;
	fz_stream *stm;

	pdf_logpage("simple content stream\n");

	error = pdf_openstream(&stm, xref, fz_tonum(stmref), fz_togen(stmref));
	if (error)
		return error;

	error = pdf_runcsi(csi, xref, rdb, stm);

	fz_dropstream(stm);

	return error;
}

/* we need to combine all sub-streams into one for pdf_runcsi
 * to deal with split dictionaries etc.
 */
static fz_error *
runmany(pdf_csi *csi, pdf_xref *xref, fz_obj *rdb, fz_obj *list)
{
	fz_error *error;
	fz_stream *file;
	fz_buffer *big;
	fz_buffer *one;
	fz_obj *stm;
	int n;
	int i;

	pdf_logpage("multiple content streams: %d\n", fz_arraylen(list));

	error = fz_newbuffer(&big, 32 * 1024);
	if (error)
		return error;

	error = fz_openwbuffer(&file, big);
	if (error)
		goto cleanup0;

	for (i = 0; i < fz_arraylen(list); i++)
	{
		/* TODO dont use loadstream here */

		stm = fz_arrayget(list, i);
		error = pdf_loadstream(&one, xref, fz_tonum(stm), fz_togen(stm));
		if (error)
			goto cleanup1;

		n = fz_write(file, one->rp, one->wp - one->rp);

		fz_dropbuffer(one);

		if (n == -1)
		{
			error = fz_ioerror(file);
			goto cleanup1;
		}

		fz_printstr(file, " ");
	}

	fz_dropstream(file);

	error = fz_openrbuffer(&file, big);
	if (error)
		goto cleanup0;

	error = pdf_runcsi(csi, xref, rdb, file);

	fz_dropstream(file);
	fz_dropbuffer(big);

	return error;

cleanup1:
	fz_dropstream(file);
cleanup0:
	fz_dropbuffer(big);
	return error;
}

static fz_error *
loadpagecontents(fz_tree **treep, pdf_xref *xref, fz_obj *rdb, fz_obj *ref)
{
	fz_error *error;
	fz_obj *obj;
	pdf_csi *csi;

	error = pdf_newcsi(&csi, 0);
	if (error)
		return error;

	if (fz_isindirect(ref))
	{
		error = pdf_loadindirect(&obj, xref, ref);
		if (error)
			return error;

		if (fz_isarray(obj))
		{
			if (fz_arraylen(obj) == 1)
				error = runone(csi, xref, rdb, fz_arrayget(obj, 0));
			else
				error = runmany(csi, xref, rdb, obj);
		}
		else
			error = runone(csi, xref, rdb, ref);

		fz_dropobj(obj);
		if (error)
			goto cleanup;
	}

	else if (fz_isarray(ref))
	{
		if (fz_arraylen(ref) == 1)
			error = runone(csi, xref, rdb, fz_arrayget(ref, 0));
		else
			error = runmany(csi, xref, rdb, ref);
	}

	*treep = csi->tree;
	csi->tree = nil;
	error = nil;

cleanup:
	pdf_dropcsi(csi);
	return error;
}

fz_error *pdf_getpageinfo(pdf_xref *xref, fz_obj *dict, fz_rect *bboxp, int *rotatep)
{
    fz_rect bbox;
    int rotate;
    fz_obj *obj;
    fz_error *error;

    obj = fz_dictgets(dict, "CropBox");
    if (!obj)
        obj = fz_dictgets(dict, "MediaBox");

    if (fz_isindirect(obj)) {
        fz_obj* obj2;
        error = pdf_loadindirect(&obj2, xref, obj);
        if (error)
            return error;
        obj = obj2;
    }

    if (!fz_isarray(obj))
        return fz_throw("syntaxerror: Page missing MediaBox");
    bbox = pdf_torect(obj);

    pdf_logpage("bbox [%g %g %g %g]\n", bbox.x0, bbox.y0, bbox.x1, bbox.y1);

    obj = fz_dictgets(dict, "Rotate");
    rotate = 0;
    if (fz_isint(obj))
        rotate = fz_toint(obj);

    pdf_logpage("rotate %d\n", rotate);

    if (bboxp)
        *bboxp = bbox;
    if (rotatep)
        *rotatep = rotate;
    return nil;
}

fz_error *
pdf_loadpage(pdf_page **pagep, pdf_xref *xref, fz_obj *dict)
{
	fz_error *error;
	fz_obj *obj;
	pdf_page *page;
	fz_obj *rdb;
	pdf_comment *comments = nil;
	pdf_link *links = nil;
	fz_tree *tree;
	fz_rect bbox;
	int rotate;

	pdf_logpage("load page {\n");

	/*
	 * Sort out page media
	 */
	error = pdf_getpageinfo(xref, dict, &bbox, &rotate);
	if (error)
		return error;

	/*
	 * Load annotations
 	 */

	obj = fz_dictgets(dict, "Annots");
	if (obj)
	{
		error = pdf_resolve(&obj, xref);
		if (error)
			return error;
		error = pdf_loadannots(&comments, &links, xref, obj);
		fz_dropobj(obj);
		if (error)
			return error;
	}

	/*
	 * Load resources
	 */

	obj = fz_dictgets(dict, "Resources");
	if (!obj)
		return fz_throw("syntaxerror: Page missing Resources");
	error = pdf_resolve(&obj, xref);
	if (error)
		return error;
	error = pdf_loadresources(&rdb, xref, obj);
	fz_dropobj(obj);
	if (error)
		return error;

	/*
	 * Interpret content stream to build display tree
 	 */

	obj = fz_dictgets(dict, "Contents");

	error = loadpagecontents(&tree, xref, rdb, obj);
	if (error) {
		fz_dropobj(rdb);
		return error;
	}

	pdf_logpage("optimize tree\n");
	error = fz_optimizetree(tree);
	if (error) {
		fz_dropobj(rdb);
		return error;
	}

	/*
	 * Create page object
	 */

	page = *pagep = fz_malloc(sizeof(pdf_page));
	if (!page) {
		fz_droptree(tree);
		fz_dropobj(rdb);
		return fz_outofmem;
	}

	page->mediabox.x0 = MIN(bbox.x0, bbox.x1);
	page->mediabox.y0 = MIN(bbox.y0, bbox.y1);
	page->mediabox.x1 = MAX(bbox.x0, bbox.x1);
	page->mediabox.y1 = MAX(bbox.y0, bbox.y1);
	page->rotate = rotate;
	page->resources = rdb;
	page->tree = tree;

	page->comments = comments;
	page->links = links;

	pdf_logpage("} %p\n", page);

	return nil;
}

void
pdf_droppage(pdf_page *page)
{
	pdf_logpage("drop page %p\n", page);
/*
	if (page->comments)
		pdf_dropcomment(page->comments);
*/
	if (page->links)
		pdf_droplink(page->links);
	fz_dropobj(page->resources);
	fz_droptree(page->tree);
	fz_free(page);
}

