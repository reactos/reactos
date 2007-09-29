#include <fitz.h>
#include <mupdf.h>

static fz_error *
loadnametreenode(fz_obj *tree, pdf_xref *xref, fz_obj *node)
{
	fz_error *error;
	fz_obj *names;
	fz_obj *kids;
	fz_obj *key;
	fz_obj *val;
	int i, len;

	error = pdf_resolve(&node, xref);
	if (error)
		return error;

	names = fz_dictgets(node, "Names");
	if (names)
	{
		error = pdf_resolve(&names, xref);
		if (error)
			goto cleanup;

		len = fz_arraylen(names) / 2;

		for (i = 0; i < len; ++i)
		{
			key = fz_arrayget(names, i * 2 + 0);
			val = fz_arrayget(names, i * 2 + 1);
			error = fz_dictput(tree, key, val);
			if (error)
			{
				fz_dropobj(names);
				goto cleanup;
			}
		}

		fz_dropobj(names);
	}

	kids = fz_dictgets(node, "Kids");
	if (kids)
	{
		error = pdf_resolve(&kids, xref);
		if (error)
			goto cleanup;

		len = fz_arraylen(kids);
		for (i = 0; i < len; ++i)
		{
			error = loadnametreenode(tree, xref, fz_arrayget(kids, i));
			if (error)
			{
				fz_dropobj(kids);
				goto cleanup;
			}
		}

		fz_dropobj(kids);
	}

	fz_dropobj(node);
	return nil;

cleanup:
	fz_dropobj(node);
	return error;
}

fz_error *
pdf_loadnametree(fz_obj **dictp, pdf_xref *xref, fz_obj *root)
{
	fz_error *error;
	fz_obj *tree;

	error = fz_newdict(&tree, 128);
	if (error)
		return error;

	error = loadnametreenode(tree, xref, root);
	if (error)
	{
		fz_dropobj(tree);
		return error;
	}

	fz_sortdict(tree);

	*dictp = tree;
	return nil;
}

fz_error *
pdf_loadnametrees(pdf_xref *xref)
{
	fz_error *error;
	fz_obj *names;
	fz_obj *dests;

	/* PDF 1.1 */
	dests = fz_dictgets(xref->root, "Dests");
	if (dests)
	{
		error = pdf_resolve(&dests, xref);
		if (error)
			return error;
		xref->dests = dests;
		return nil;
	}

	/* PDF 1.2 */
	names = fz_dictgets(xref->root, "Names");
	if (names)
	{
		error = pdf_resolve(&names, xref);
		if (error)
			return error;
		dests = fz_dictgets(names, "Dests");
		if (dests)
		{
			error = pdf_loadnametree(&xref->dests, xref, dests);
			if (error)
			{
				fz_dropobj(names);
				return error;
			}
		}
		fz_dropobj(names);
	}

	return nil;
}

