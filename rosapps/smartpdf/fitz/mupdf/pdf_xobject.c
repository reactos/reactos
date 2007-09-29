#include <fitz.h>
#include <mupdf.h>

fz_error *
pdf_loadxobject(pdf_xobject **formp, pdf_xref *xref, fz_obj *dict, fz_obj *ref)
{
	fz_error *error;
	pdf_xobject *form;
	fz_obj *obj;

	if ((*formp = pdf_finditem(xref->store, PDF_KXOBJECT, ref)))
	{
		pdf_keepxobject(*formp);
		return nil;
	}

	form = fz_malloc(sizeof(pdf_xobject));
	if (!form)
		return fz_outofmem;

	form->refs = 1;
	form->resources = nil;
	form->contents = nil;

	pdf_logrsrc("load xobject %d %d (%p) {\n", fz_tonum(ref), fz_togen(ref), form);

	obj = fz_dictgets(dict, "BBox");
	form->bbox = pdf_torect(obj);

	pdf_logrsrc("bbox [%g %g %g %g]\n",
		form->bbox.x0, form->bbox.y0,
		form->bbox.x1, form->bbox.y1);

	obj = fz_dictgets(dict, "Matrix");
	if (obj)
		form->matrix = pdf_tomatrix(obj);
	else
		form->matrix = fz_identity();

	pdf_logrsrc("matrix [%g %g %g %g %g %g]\n",
		form->matrix.a, form->matrix.b,
		form->matrix.c, form->matrix.d,
		form->matrix.e, form->matrix.f);

	obj = fz_dictgets(dict, "Resources");
	if (obj)
	{
		error = pdf_resolve(&obj, xref);
		if (error)
		{
			pdf_dropxobject(form);
			return error;
		}
		error = pdf_loadresources(&form->resources, xref, obj);
		fz_dropobj(obj);
		if (error)
		{
			pdf_dropxobject(form);
			return error;
		}
	}

	error = pdf_loadstream(&form->contents, xref, fz_tonum(ref), fz_togen(ref));
	if (error)
	{
		pdf_dropxobject(form);
		return error;
	}

	pdf_logrsrc("stream %d bytes\n", form->contents->wp - form->contents->rp);

	pdf_logrsrc("}\n");

	error = pdf_storeitem(xref->store, PDF_KXOBJECT, ref, form);
	if (error)
	{
		pdf_dropxobject(form);
		return error;
	}

	*formp = form;
	return nil;
}

pdf_xobject *
pdf_keepxobject(pdf_xobject *xobj)
{
	xobj->refs ++;
	return xobj;
}

void
pdf_dropxobject(pdf_xobject *xobj)
{
	if (--xobj->refs == 0)
	{
		if (xobj->contents) fz_dropbuffer(xobj->contents);
		if (xobj->resources) fz_dropobj(xobj->resources);
		fz_free(xobj);
	}
}

