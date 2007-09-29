#include <fitz.h>
#include <mupdf.h>

struct stuff
{
	fz_obj *resources;
	fz_obj *mediabox;
	fz_obj *cropbox;
	fz_obj *rotate;
};

static fz_error *
loadpagetree(pdf_xref *xref, pdf_pagetree *pages,
	struct stuff inherit, fz_obj *obj, fz_obj *ref)
{
	fz_error *error;
	fz_obj *type;
	fz_obj *kids;
	fz_obj *kref, *kobj;
	fz_obj *inh;
	int i;

	type = fz_dictgets(obj, "Type");

	if (strcmp(fz_toname(type), "Page") == 0)
	{
		if (inherit.resources && !fz_dictgets(obj, "Resources"))
		{
			pdf_logpage("inherit resources (%d)\n", pages->cursor);
			error = fz_dictputs(obj, "Resources", inherit.resources);
			if (error) return error;
		}

		if (inherit.mediabox && !fz_dictgets(obj, "MediaBox"))
		{
			pdf_logpage("inherit mediabox (%d)\n", pages->cursor);
			error = fz_dictputs(obj, "MediaBox", inherit.mediabox);
			if (error) return error;
		}

		if (inherit.cropbox && !fz_dictgets(obj, "CropBox"))
		{
			pdf_logpage("inherit cropbox (%d)\n", pages->cursor);
			error = fz_dictputs(obj, "CropBox", inherit.cropbox);
			if (error) return error;
		}

		if (inherit.rotate && !fz_dictgets(obj, "Rotate"))
		{
			pdf_logpage("inherit rotate (%d)\n", pages->cursor);
			error = fz_dictputs(obj, "Rotate", inherit.rotate);
			if (error) return error;
		}

		pages->pref[pages->cursor] = fz_keepobj(ref);
		pages->pobj[pages->cursor] = fz_keepobj(obj);
		pages->cursor ++;
	}

	else if (strcmp(fz_toname(type), "Pages") == 0)
	{
		inh = fz_dictgets(obj, "Resources");
		if (inh) inherit.resources = inh;

		inh = fz_dictgets(obj, "MediaBox");
		if (inh) inherit.mediabox = inh;

		inh = fz_dictgets(obj, "CropBox");
		if (inh) inherit.cropbox = inh;

		inh = fz_dictgets(obj, "Rotate");
		if (inh) inherit.rotate = inh;

		kids = fz_dictgets(obj, "Kids");
		error = pdf_resolve(&kids, xref);
		if (error)
			return error;

		pdf_logpage("subtree %d {\n", fz_arraylen(kids));

		for (i = 0; i < fz_arraylen(kids); i++)
		{
			kref = fz_arrayget(kids, i);

			error = pdf_loadindirect(&kobj, xref, kref);
			if (error) { fz_dropobj(kids); return error; }

            if (kobj == obj)
            {
                /* prevent infinite recursion possible in maliciously crafted PDFs */
                fz_dropobj(kids);
                return fz_throw("corrupted pdf file");
            }
            else
            {
			    error = loadpagetree(xref, pages, inherit, kobj, kref);
			    fz_dropobj(kobj);
			    if (error) { fz_dropobj(kids); return error; }
            }
		}

		fz_dropobj(kids);

		pdf_logpage("}\n");
	}

	return nil;
}

void
pdf_debugpagetree(pdf_pagetree *pages)
{
	int i;
	printf("<<\n  /Type /Pages\n  /Count %d\n  /Kids [\n", pages->count);
	for (i = 0; i < pages->count; i++) {
		printf("    ");
		fz_debugobj(pages->pref[i]);
		printf("\t%% page %d\n", i + 1);
	}
	printf("  ]\n>>\n");
}

fz_error *
pdf_loadpagetree(pdf_pagetree **pp, pdf_xref *xref)
{
	fz_error *error;
	struct stuff inherit;
	pdf_pagetree *p = nil;
	fz_obj *catalog = nil;
	fz_obj *pages = nil;
	fz_obj *trailer;
	fz_obj *ref;
	int count;

	inherit.resources = nil;
	inherit.mediabox = nil;
	inherit.cropbox = nil;
	inherit.rotate = nil;

	trailer = xref->trailer;

	ref = fz_dictgets(trailer, "Root");
	error = pdf_loadindirect(&catalog, xref, ref);
	if (error) goto cleanup;

	ref = fz_dictgets(catalog, "Pages");
	error = pdf_loadindirect(&pages, xref, ref);
	if (error) goto cleanup;

	ref = fz_dictgets(pages, "Count");
	count = fz_toint(ref);

	p = fz_malloc(sizeof(pdf_pagetree));
	if (!p) { error = fz_outofmem; goto cleanup; }

	pdf_logpage("load pagetree %p {\n", p);
	pdf_logpage("count %d\n", count);

	p->pref = nil;
	p->pobj = nil;
	p->count = count;
	p->cursor = 0;

	p->pref = fz_malloc(sizeof(fz_obj*) * count);
	if (!p->pref) { error = fz_outofmem; goto cleanup; }

	p->pobj = fz_malloc(sizeof(fz_obj*) * count);
	if (!p->pobj) { error = fz_outofmem; goto cleanup; }

	error = loadpagetree(xref, p, inherit, pages, ref);
	if (error) goto cleanup;

	fz_dropobj(pages);
	fz_dropobj(catalog);

	pdf_logpage("}\n", count);

	*pp = p;
	return nil;

cleanup:
	if (pages) fz_dropobj(pages);
	if (catalog) fz_dropobj(catalog);
	if (p) {
		fz_free(p->pref);
		fz_free(p->pobj);
		fz_free(p);
	}
	return error;
}

int
pdf_getpagecount(pdf_pagetree *pages)
{
	return pages->count;
}

fz_obj *
pdf_getpageobject(pdf_pagetree *pages, int p)
{
	if (p < 0 || p >= pages->count)
		return nil;
	return pages->pobj[p];
}

void
pdf_droppagetree(pdf_pagetree *pages)
{
	int i;

	pdf_logpage("drop pagetree %p\n", pages);

	for (i = 0; i < pages->count; i++) {
		if (pages->pref[i])
			fz_dropobj(pages->pref[i]);
		if (pages->pobj[i])
			fz_dropobj(pages->pobj[i]);
	}

	fz_free(pages->pref);
	fz_free(pages->pobj);
	fz_free(pages);
}

