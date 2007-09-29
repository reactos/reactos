#include <fitz.h>
#include <mupdf.h>

fz_error *
pdf_loadshadefunction(fz_shade *shade, pdf_xref *xref, fz_obj *shading, float t0, float t1)
{
	fz_error *error;
	float t;
	fz_obj *obj;
	pdf_function *func;
	int i;

	obj = fz_dictgets(shading, "Function");
	if (obj)
	{
		shade->usefunction = 1;

		error = pdf_loadfunction(&func, xref, obj);
		if (error)
			return error;

		for (i = 0; i < 256; ++i)
		{
			t = t0 + (i / 256.0) * (t1 - t0);
			error = pdf_evalfunction(func, &t, 1, shade->function[i], shade->cs->n);
			if (error)
			{
				pdf_dropfunction(func);
				return error;
			}
		}

		pdf_dropfunction(func);
	}

	return nil;
}

void
pdf_setmeshvalue(float *mesh, int i, float x, float y, float t)
{
	mesh[i*3+0] = x;
	mesh[i*3+1] = y;
	mesh[i*3+2] = t;
}

static fz_error *
loadshadedict(fz_shade **shadep, pdf_xref *xref, fz_obj *dict, fz_obj *ref, fz_matrix matrix)
{
	fz_error *error;
	fz_shade *shade;
	fz_obj *obj;
	int type;
	int i;

	pdf_logshade("load shade dict %d %d {\n", fz_tonum(ref), fz_togen(ref));

	shade = fz_malloc(sizeof(fz_shade));
	if (!shade)
		return fz_outofmem;

	shade->refs = 1;
	shade->usebackground = 0;
	shade->usefunction = 0;
	shade->matrix = matrix;
	shade->bbox = fz_infiniterect;

	shade->meshlen = 0;
	shade->meshcap = 0;
	shade->mesh = nil;

	obj = fz_dictgets(dict, "ShadingType");
	type = fz_toint(obj);
	pdf_logshade("type %d\n", type);

	/* TODO: flatten indexed... */
	obj = fz_dictgets(dict, "ColorSpace");
	if (obj)
	{
		shade->cs = pdf_finditem(xref->store, PDF_KCOLORSPACE, obj);
		if (shade->cs)
			fz_keepcolorspace(shade->cs);
		else
		{
			error = pdf_resolve(&obj, xref);
			if (error)
				return error;
			error = pdf_loadcolorspace(&shade->cs, xref, obj);
			if (error)
				return error;
			fz_dropobj(obj);
		}
	}
	pdf_logshade("colorspace %s\n", shade->cs->name);

	obj = fz_dictgets(dict, "Background");
	if (obj)
	{
		pdf_logshade("background\n");
		shade->usebackground = 1;
		for (i = 0; i < shade->cs->n; i++)
			shade->background[i] = fz_toreal(fz_arrayget(obj, i));
	}

	obj = fz_dictgets(dict, "BBox");
	if (fz_isarray(obj))
	{
		shade->bbox = pdf_torect(obj);
		pdf_logshade("bbox [%g %g %g %g]\n",
			shade->bbox.x0, shade->bbox.y0,
			shade->bbox.x1, shade->bbox.y1);
	}

	switch(type)
	{
	case 1:
		error = pdf_loadtype1shade(shade, xref, dict, ref);
		if (error) goto cleanup;
		break;
	case 2:
		error = pdf_loadtype2shade(shade, xref, dict, ref);
		if (error) goto cleanup;
		break;
	case 3:
		error = pdf_loadtype3shade(shade, xref, dict, ref);
		if (error) goto cleanup;
		break;
	case 4:
		error = pdf_loadtype4shade(shade, xref, dict, ref);
		if (error) goto cleanup;
		break;
	case 5:
		error = pdf_loadtype5shade(shade, xref, dict, ref);
		if (error) goto cleanup;
		break;
	case 6:
		error = pdf_loadtype6shade(shade, xref, dict, ref);
		if (error) goto cleanup;
		break;
	case 7:
		error = pdf_loadtype7shade(shade, xref, dict, ref);
		if (error) goto cleanup;
		break;
	default:
		fz_warn("syntaxerror: unknown shading type: %d", type);
		break;
	};

	pdf_logshade("}\n");

	*shadep = shade;
	return nil;

cleanup:
	fz_dropshade(shade);
	return error;
}

fz_error *
pdf_loadshade(fz_shade **shadep, pdf_xref *xref, fz_obj *dict, fz_obj *ref)
{
	fz_error *error;
	fz_matrix mat;
	fz_obj *obj;
	fz_obj *shd;

	if ((*shadep = pdf_finditem(xref->store, PDF_KSHADE, ref)))
		return nil;

	/*
	 * Type 2 pattern dictionary
	 */
	if (fz_dictgets(dict, "PatternType"))
	{
		pdf_logshade("load shade pattern %d %d {\n", fz_tonum(ref), fz_togen(ref));

		obj = fz_dictgets(dict, "Matrix");
		if (obj)
		{
			mat = pdf_tomatrix(obj);
			pdf_logshade("matrix [%g %g %g %g %g %g]\n",
				mat.a, mat.b, mat.c, mat.d, mat.e, mat.f);
		}
		else
		{
			mat = fz_identity();
		}

		obj = fz_dictgets(dict, "ExtGState");
		if (obj)
		{
			pdf_logshade("extgstate ...\n");
		}

		obj = fz_dictgets(dict, "Shading");
		if (!obj)
			return fz_throw("syntaxerror: missing shading dictionary");

		shd = obj;
		error = pdf_resolve(&shd, xref);
		if (error)
			return error;
		error = loadshadedict(shadep, xref, shd, obj, mat);
		fz_dropobj(shd);
		if (error)
			return error;

		pdf_logshade("}\n");
	}

	/*
	 * Naked shading dictionary
	 */
	else
	{
		error = loadshadedict(shadep, xref, dict, ref, fz_identity());
		if (error)
			return error;
	}

	error = pdf_storeitem(xref->store, PDF_KSHADE, ref, *shadep);
	if (error)
	{
		fz_dropshade(*shadep);
		return error;
	}

	return nil;
}

