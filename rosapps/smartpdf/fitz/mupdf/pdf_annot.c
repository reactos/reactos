#include <fitz.h>
#include <mupdf.h>

static fz_error *
loadcomment(pdf_comment **commentp, pdf_xref *xref, fz_obj *dict)
{
	return nil;
}

fz_error *
pdf_newlink(pdf_link **linkp, fz_rect bbox, fz_obj *dest, pdf_linkkind kind)
{
	pdf_link *link;

	link = fz_malloc(sizeof(pdf_link));
	if (!link)
		return fz_outofmem;

	link->rect = bbox;
	link->dest = fz_keepobj(dest);
	link->kind = kind;
	link->next = nil;

	*linkp = link;
	return nil;
}

void
pdf_droplink(pdf_link *link)
{
	if (link->next)
		pdf_droplink(link->next);
	if (link->dest)
		fz_dropobj(link->dest);
	fz_free(link);
}

static fz_obj *
resolvedest(pdf_xref *xref, fz_obj *dest)
{
	if (fz_isname(dest))
	{
		dest = fz_dictget(xref->dests, dest);
		if (dest)
			pdf_resolve(&dest, xref); /* XXX */
		return resolvedest(xref, dest);
	}

	else if (fz_isstring(dest))
	{
		dest = fz_dictget(xref->dests, dest);
		if (dest)
			pdf_resolve(&dest, xref); /* XXX */
		return resolvedest(xref, dest);
	}

	else if (fz_isarray(dest))
	{
		return fz_arrayget(dest, 0);
	}

	else if (fz_isdict(dest))
	{
		dest = fz_dictgets(dest, "D");
		return resolvedest(xref, dest);
	}

	else if (fz_isindirect(dest))
		return dest;

	return nil;
}

fz_error *
pdf_loadlink(pdf_link **linkp, pdf_xref *xref, fz_obj *dict)
{
	fz_error *error;
	pdf_link *link;
	fz_obj *dest;
	fz_obj *action;
	fz_obj *obj;
	fz_rect bbox;
	pdf_linkkind kind;

	pdf_logpage("load link {\n");

	link = nil;
	dest = nil;

	obj = fz_dictgets(dict, "Rect");
	if (obj)
	{
		bbox = pdf_torect(obj);
		pdf_logpage("rect [%g %g %g %g]\n",
			bbox.x0, bbox.y0,
			bbox.x1, bbox.y1);
	}
	else
		bbox = fz_emptyrect;

	obj = fz_dictgets(dict, "Dest");
	if (obj)
	{
		error = pdf_resolve(&obj, xref);
		if (error)
			return error;
		dest = resolvedest(xref, obj);
		pdf_logpage("dest %d %d R\n", fz_tonum(dest), fz_togen(dest));
		fz_dropobj(obj);
	}

	kind = PDF_LUNKNOWN;
	action = fz_dictgets(dict, "A");
	if (action)
	{
		error = pdf_resolve(&action, xref);
		if (error)
			return error;

		obj = fz_dictgets(action, "S");
		if (!strcmp(fz_toname(obj), "GoTo"))
		{
			kind = PDF_LGOTO;
			dest = resolvedest(xref, fz_dictgets(action, "D"));
			pdf_logpage("action goto %d %d R\n", fz_tonum(dest), fz_togen(dest));
		}
		else if (!strcmp(fz_toname(obj), "URI"))
		{
			kind = PDF_LURI;
			dest = fz_dictgets(action, "URI");
			pdf_logpage("action uri %s\n", fz_tostrbuf(dest));
		}
		else
			pdf_logpage("action ... ?\n");

		fz_dropobj(action);
	}

	pdf_logpage("}\n");

	if (dest)
	{
		error = pdf_newlink(&link, bbox, dest, kind);
		if (error)
			return error;
		*linkp = link;
	}

	return nil;
}

fz_error *
pdf_loadannots(pdf_comment **cp, pdf_link **lp, pdf_xref *xref, fz_obj *annots)
{
	fz_error *error;
	pdf_comment *comment;
	pdf_link *link;
	fz_obj *subtype;
	fz_obj *obj;
	int i;

	comment = nil;
	link = nil;

	pdf_logpage("load annotations {\n");

	for (i = 0; i < fz_arraylen(annots); i++)
	{
		obj = fz_arrayget(annots, i);
		error = pdf_resolve(&obj, xref);
		if (error)
			goto cleanup;

		subtype = fz_dictgets(obj, "Subtype");
		if (!strcmp(fz_toname(subtype), "Link"))
		{
			pdf_link *temp = nil;

			error = pdf_loadlink(&temp, xref, obj);
			fz_dropobj(obj);
			if (error)
				goto cleanup;

			if (temp)
			{
				temp->next = link;
				link = temp;
			}
		}
		else
		{
			error = loadcomment(&comment, xref, obj);
			fz_dropobj(obj);
			if (error)
				goto cleanup;
		}
	}

	pdf_logpage("}\n");

	*cp = comment;
	*lp = link;
	return nil;

cleanup:
    if (link)
        pdf_droplink(link);
	return error;
}

