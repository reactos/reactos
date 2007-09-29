#include <fitz.h>
#include <mupdf.h>

#define noUSECAL

static void initcs(fz_colorspace *cs, char *name, int n,
	void(*to)(fz_colorspace*,float*,float*),
	void(*from)(fz_colorspace*,float*,float*),
	void(*drop)(fz_colorspace*))
{
	strlcpy(cs->name, name, sizeof cs->name);
	cs->refs = 1;
	cs->convpixmap = pdf_convpixmap;
	cs->convcolor = pdf_convcolor;
	cs->toxyz = to;
	cs->fromxyz = from;
	cs->drop = drop;
	cs->n = n;
}

/*
 * CalGray
 */

struct calgray
{
	fz_colorspace super;
	float white[3];
	float black[3];
	float gamma;
};

static void graytoxyz(fz_colorspace *fzcs, float *gray, float *xyz)
{
	struct calgray *cs = (struct calgray *) fzcs;
	xyz[0] = pow(gray[0], cs->gamma) * cs->white[0];
	xyz[1] = pow(gray[0], cs->gamma) * cs->white[1];
	xyz[2] = pow(gray[0], cs->gamma) * cs->white[2];
}

static void xyztogray(fz_colorspace *fzcs, float *xyz, float *gray)
{
	struct calgray *cs = (struct calgray *) fzcs;
	float r = pow(xyz[0], 1.0 / cs->gamma) / cs->white[0];
	float g = pow(xyz[1], 1.0 / cs->gamma) / cs->white[1];
	float b = pow(xyz[2], 1.0 / cs->gamma) / cs->white[2];
	gray[0] = r * 0.3 + g * 0.59 + b * 0.11;
}

/*
 * CalRGB
 */

struct calrgb
{
	fz_colorspace super;
	float white[3];
	float black[3];
	float gamma[3];
	float matrix[9];
	float invmat[9];
};

static void rgbtoxyz(fz_colorspace *fzcs, float *rgb, float *xyz)
{
	struct calrgb *cs = (struct calrgb *) fzcs;
	float a = pow(rgb[0], cs->gamma[0]) * cs->white[0];
	float b = pow(rgb[1], cs->gamma[1]) * cs->white[1];
	float c = pow(rgb[2], cs->gamma[2]) * cs->white[2];
	xyz[0] = a * cs->matrix[0] + b * cs->matrix[1] + c * cs->matrix[2];
	xyz[1] = a * cs->matrix[3] + b * cs->matrix[4] + c * cs->matrix[5];
	xyz[2] = a * cs->matrix[6] + b * cs->matrix[7] + c * cs->matrix[8];
}

static void xyztorgb(fz_colorspace *fzcs, float *xyz, float *rgb)
{
	struct calrgb *cs = (struct calrgb *) fzcs;
	float a = xyz[0] * cs->invmat[0] + xyz[1] * cs->invmat[1] + xyz[2] * cs->invmat[2];
	float b = xyz[0] * cs->invmat[3] + xyz[1] * cs->invmat[4] + xyz[2] * cs->invmat[5];
	float c = xyz[0] * cs->invmat[6] + xyz[1] * cs->invmat[7] + xyz[2] * cs->invmat[8];
	rgb[0] = pow(a, 1.0 / cs->gamma[0]) / cs->white[0];
	rgb[1] = pow(b, 1.0 / cs->gamma[1]) / cs->white[1];
	rgb[2] = pow(c, 1.0 / cs->gamma[2]) / cs->white[2];
}

/*
 * DeviceCMYK piggybacks on DeviceRGB
 */

static void devicecmyktoxyz(fz_colorspace *cs, float *cmyk, float *xyz)
{
	float rgb[3];
	rgb[0] = 1.0 - MIN(1.0, cmyk[0] + cmyk[3]);
	rgb[1] = 1.0 - MIN(1.0, cmyk[1] + cmyk[3]);
	rgb[2] = 1.0 - MIN(1.0, cmyk[2] + cmyk[3]);
	rgbtoxyz(pdf_devicergb, rgb, xyz);
}

static void xyztodevicecmyk(fz_colorspace *cs, float *xyz, float *cmyk)
{
	float rgb[3];
	float c, m, y, k;
	xyztorgb(pdf_devicergb, xyz, rgb);
	c = 1.0 - rgb[0];
	m = 1.0 - rgb[0];
	y = 1.0 - rgb[0];
	k = MIN(c, MIN(m, y));
	cmyk[0] = c - k;
	cmyk[1] = m - k;
	cmyk[2] = y - k;
	cmyk[3] = k;
}

/*
 * CIE Lab
 */

struct cielab
{
	fz_colorspace super;
	float white[3];
	float black[3];
	float range[4];
};

static inline float fung(float x)
{
	if (x >= 6.0 / 29.0)
		return x * x * x;
	return (108.0 / 841.0) * (x - (4.0 / 29.0));
}

static inline float invg(float x)
{
	if (x > 0.008856)
		return pow(x, 1.0 / 3.0);
	return (7.787 * x) + (16.0 / 116.0);
}

static void labtoxyz(fz_colorspace *fzcs, float *lab, float *xyz)
{
	struct cielab *cs = (struct cielab *) fzcs;
	float lstar, astar, bstar, l, m, n;
	float tmp[3];
	tmp[0] = lab[0] * 100;
	tmp[1] = lab[1] * 200 - 100;
	tmp[2] = lab[2] * 200 - 100;
	lstar = tmp[0];
	astar = MAX(MIN(tmp[1], cs->range[1]), cs->range[0]);
	bstar = MAX(MIN(tmp[2], cs->range[3]), cs->range[2]);
	l = (lstar + 16.0) / 116.0 + astar / 500.0;
	m = (lstar + 16.0) / 116.0;
	n = (lstar + 16.0) / 116.0 - bstar / 200.0;
	xyz[0] = fung(l) * cs->white[0];
	xyz[1] = fung(m) * cs->white[1];
	xyz[2] = fung(n) * cs->white[2];
}

static void xyztolab(fz_colorspace *fzcs, float *xyz, float *lab)
{
	struct cielab *cs = (struct cielab *) fzcs;
	float tmp[3];
	float yyn = xyz[1] / cs->white[1];
	if (yyn < 0.008856)
		tmp[0] = 116.0 * yyn * (1.0 / 3.0) - 16.0;
	else
		tmp[0] = 903.3 * yyn;
	tmp[1] = 500 * (invg(xyz[0]/cs->white[0]) - invg(xyz[1]/cs->white[1]));
	tmp[2] = 200 * (invg(xyz[1]/cs->white[1]) - invg(xyz[2]/cs->white[2]));
	lab[0] = tmp[0] / 100.0;
	lab[1] = (tmp[1] + 100) / 200.0;
	lab[2] = (tmp[2] + 100) / 200.0;
}

/*
 * Define global Device* colorspaces as Cal*
 */

static struct calgray kdevicegray =
{
	{ -1, "DeviceGray", 1, pdf_convpixmap, pdf_convcolor, graytoxyz, xyztogray, nil },
	{ 1.0000, 1.0000, 1.0000 },
	{ 0.0000, 0.0000, 0.0000 },
	1.0000
};

static struct calrgb kdevicergb =
{
	{ -1, "DeviceRGB", 3, pdf_convpixmap, pdf_convcolor, rgbtoxyz, xyztorgb, nil },
	{ 1.0000, 1.0000, 1.0000 },
	{ 0.0000, 0.0000, 0.0000 },
	{ 1.0000, 1.0000, 1.0000 },
	{ 1,0,0, 0,1,0, 0,0,1 },
	{ 1,0,0, 0,1,0, 0,0,1 },
};

static fz_colorspace kdevicecmyk =
{
	-1, "DeviceCMYK", 4, pdf_convpixmap, pdf_convcolor, devicecmyktoxyz, xyztodevicecmyk, nil
};

static struct cielab kdevicelab =
{
	{ -1, "Lab", 3, fz_stdconvpixmap, fz_stdconvcolor, labtoxyz, xyztolab, nil },
	{ 1.0000, 1.0000, 1.0000 },
	{ 0.0000, 0.0000, 0.0000 },
	{ -100, 100, -100, 100 },
};

static fz_colorspace kdevicepattern =
{
	-1, "Pattern", 0, nil, nil, nil, nil, nil
};

fz_colorspace *pdf_devicegray = &kdevicegray.super;
fz_colorspace *pdf_devicergb = &kdevicergb.super;
fz_colorspace *pdf_devicecmyk = &kdevicecmyk;
fz_colorspace *pdf_devicelab = &kdevicelab.super;
fz_colorspace *pdf_devicepattern = &kdevicepattern;

/*
 * Colorspace parsing
 */

#ifdef USECAL

static fz_error *
loadcalgray(fz_colorspace **csp, pdf_xref *xref, fz_obj *dict)
{
	fz_error *error;
	struct calgray *cs;
	fz_obj *tmp;

	error = pdf_resolve(&dict, xref);
	if (error)
		return error;

	cs = fz_malloc(sizeof(struct calgray));
	if (!cs)
		return fz_outofmem;

	pdf_logrsrc("load CalGray\n");

	initcs((fz_colorspace*)cs, "CalGray", 1, graytoxyz, xyztogray, nil);

	cs->white[0] = 1.0;
	cs->white[1] = 1.0;
	cs->white[2] = 1.0;

	cs->black[0] = 0.0;
	cs->black[1] = 0.0;
	cs->black[2] = 0.0;

	cs->gamma = 1.0;

	tmp = fz_dictgets(dict, "WhitePoint");
	if (fz_isarray(tmp))
	{
		cs->white[0] = fz_toreal(fz_arrayget(tmp, 0));
		cs->white[1] = fz_toreal(fz_arrayget(tmp, 1));
		cs->white[2] = fz_toreal(fz_arrayget(tmp, 2));
	}

	tmp = fz_dictgets(dict, "BlackPoint");
	if (fz_isarray(tmp))
	{
		cs->black[0] = fz_toreal(fz_arrayget(tmp, 0));
		cs->black[1] = fz_toreal(fz_arrayget(tmp, 1));
		cs->black[2] = fz_toreal(fz_arrayget(tmp, 2));
	}

	tmp = fz_dictgets(dict, "Gamma");
	if (fz_isreal(tmp))
		cs->gamma = fz_toreal(tmp);

	fz_dropobj(dict);

	*csp = (fz_colorspace*) cs;
	return nil;
}

static fz_error *
loadcalrgb(fz_colorspace **csp, pdf_xref *xref, fz_obj *dict)
{
	fz_error *error;
	struct calrgb *cs;
	fz_obj *tmp;
	int i;

	error = pdf_resolve(&dict, xref);
	if (error)
		return error;

	cs = fz_malloc(sizeof(struct calrgb));
	if (!cs)
		return fz_outofmem;

	pdf_logrsrc("load CalRGB\n");

	initcs((fz_colorspace*)cs, "CalRGB", 3, rgbtoxyz, xyztorgb, nil);

	cs->white[0] = 1.0;
	cs->white[1] = 1.0;
	cs->white[2] = 1.0;

	cs->black[0] = 0.0;
	cs->black[1] = 0.0;
	cs->black[2] = 0.0;

	cs->gamma[0] = 1.0;
	cs->gamma[1] = 1.0;
	cs->gamma[2] = 1.0;

	cs->matrix[0] = 1.0; cs->matrix[1] = 0.0; cs->matrix[2] = 0.0;
	cs->matrix[3] = 0.0; cs->matrix[4] = 1.0; cs->matrix[5] = 0.0;
	cs->matrix[6] = 0.0; cs->matrix[7] = 0.0; cs->matrix[8] = 1.0;

	tmp = fz_dictgets(dict, "WhitePoint");
	if (fz_isarray(tmp))
	{
		cs->white[0] = fz_toreal(fz_arrayget(tmp, 0));
		cs->white[1] = fz_toreal(fz_arrayget(tmp, 1));
		cs->white[2] = fz_toreal(fz_arrayget(tmp, 2));
	}

	tmp = fz_dictgets(dict, "BlackPoint");
	if (fz_isarray(tmp))
	{
		cs->black[0] = fz_toreal(fz_arrayget(tmp, 0));
		cs->black[1] = fz_toreal(fz_arrayget(tmp, 1));
		cs->black[2] = fz_toreal(fz_arrayget(tmp, 2));
	}

	tmp = fz_dictgets(dict, "Gamma");
	if (fz_isarray(tmp))
	{
		cs->gamma[0] = fz_toreal(fz_arrayget(tmp, 0));
		cs->gamma[1] = fz_toreal(fz_arrayget(tmp, 1));
		cs->gamma[2] = fz_toreal(fz_arrayget(tmp, 2));
	}

	tmp = fz_dictgets(dict, "Matrix");
	if (fz_isarray(tmp))
	{
		for (i = 0; i < 9; i++)
			cs->matrix[i] = fz_toreal(fz_arrayget(tmp, i));
	}

	fz_invert3x3(cs->invmat, cs->matrix);

	fz_dropobj(dict);

	*csp = (fz_colorspace*) cs;
	return nil;
}

static fz_error *
loadlab(fz_colorspace **csp, pdf_xref *xref, fz_obj *dict)
{
	fz_error *error;
	struct cielab *cs;
	fz_obj *tmp;

	error = pdf_resolve(&dict, xref);
	if (error)
		return error;

	cs = fz_malloc(sizeof(struct cielab));
	if (!cs)
		return fz_outofmem;

	pdf_logrsrc("load Lab\n");

	initcs((fz_colorspace*)cs, "Lab", 3, labtoxyz, xyztolab, nil);

	cs->white[0] = 1.0;
	cs->white[1] = 1.0;
	cs->white[2] = 1.0;

	cs->black[0] = 0.0;
	cs->black[1] = 0.0;
	cs->black[2] = 0.0;

	cs->range[0] = -100;
	cs->range[1] = 100;
	cs->range[2] = -100;
	cs->range[3] = 100;

	tmp = fz_dictgets(dict, "WhitePoint");
	if (fz_isarray(tmp))
	{
		cs->white[0] = fz_toreal(fz_arrayget(tmp, 0));
		cs->white[1] = fz_toreal(fz_arrayget(tmp, 1));
		cs->white[2] = fz_toreal(fz_arrayget(tmp, 2));
	}

	tmp = fz_dictgets(dict, "BlackPoint");
	if (fz_isarray(tmp))
	{
		cs->black[0] = fz_toreal(fz_arrayget(tmp, 0));
		cs->black[1] = fz_toreal(fz_arrayget(tmp, 1));
		cs->black[2] = fz_toreal(fz_arrayget(tmp, 2));
	}

	tmp = fz_dictgets(dict, "Range");
	if (fz_isarray(tmp))
	{
		cs->range[0] = fz_toreal(fz_arrayget(tmp, 0));
		cs->range[1] = fz_toreal(fz_arrayget(tmp, 1));
		cs->range[2] = fz_toreal(fz_arrayget(tmp, 2));
		cs->range[3] = fz_toreal(fz_arrayget(tmp, 3));
	}

	fz_dropobj(dict);

	*csp = (fz_colorspace*) cs;
	return nil;
}

#endif

/*
 * ICCBased
 */

static fz_error *
loadiccbased(fz_colorspace **csp, pdf_xref *xref, fz_obj *ref)
{
	fz_error *error;
	fz_obj *dict;
	int n;

	pdf_logrsrc("load ICCBased\n");

	error = pdf_loadindirect(&dict, xref, ref);
	if (error)
		return error;

	n = fz_toint(fz_dictgets(dict, "N"));

	fz_dropobj(dict);

	switch (n)
	{
	case 1: *csp = pdf_devicegray; return nil;
	case 3: *csp = pdf_devicergb; return nil;
	case 4: *csp = pdf_devicecmyk; return nil;
	}

	return fz_throw("syntaxerror: ICCBased must have 1, 3 or 4 components");
}

/*
 * Separation and DeviceN
 */

struct separation
{
	fz_colorspace super;
	fz_colorspace *base;
	pdf_function *tint;
};

static void separationtoxyz(fz_colorspace *fzcs, float *sep, float *xyz)
{
	struct separation *cs = (struct separation *)fzcs;
	fz_error *error;
	float alt[FZ_MAXCOLORS];

	error = pdf_evalfunction(cs->tint, sep, fzcs->n, alt, cs->base->n);
	if (error)
	{
		fz_warn("separation: %s", error->msg);
		fz_droperror(error);
		xyz[0] = 0;
		xyz[1] = 0;
		xyz[2] = 0;
		return;
	}

	cs->base->toxyz(cs->base, alt, xyz);
}

static void
dropseparation(fz_colorspace *fzcs)
{
	struct separation *cs = (struct separation *)fzcs;
	fz_dropcolorspace(cs->base);
	pdf_dropfunction(cs->tint);
}

static fz_error *
loadseparation(fz_colorspace **csp, pdf_xref *xref, fz_obj *array)
{
	fz_error *error;
	struct separation *cs;
	fz_obj *nameobj = fz_arrayget(array, 1);
	fz_obj *baseobj = fz_arrayget(array, 2);
	fz_obj *tintobj = fz_arrayget(array, 3);
	fz_colorspace *base;
	pdf_function *tint;
	int n;

	pdf_logrsrc("load Separation {\n");

	if (fz_isarray(nameobj))
		n = fz_arraylen(nameobj);
	else
		n = 1;

	pdf_logrsrc("n = %d\n", n);

	error = pdf_resolve(&baseobj, xref);
	if (error)
		return error;
	error = pdf_loadcolorspace(&base, xref, baseobj);
	fz_dropobj(baseobj);
	if (error)
		return error;

	error = pdf_loadfunction(&tint, xref, tintobj);
	if (error)
	{
		fz_dropcolorspace(base);
		return error;
	}

	cs = fz_malloc(sizeof(struct separation));
	if (!cs)
	{
		pdf_dropfunction(tint);
		fz_dropcolorspace(base);
		return fz_outofmem;
	}

	initcs((fz_colorspace*)cs,
		n == 1 ? "Separation" : "DeviceN", n,
		separationtoxyz, nil, dropseparation);

	cs->base = base;
	cs->tint = tint;

	pdf_logrsrc("}\n");

	*csp = (fz_colorspace*)cs;
	return nil;
}

/*
 * Indexed
 */

#if 0
static void
indexedtoxyz(fz_colorspace *fzcs, float *ind, float *xyz)
{
	pdf_indexed *cs = (pdf_indexed *)fzcs;
	float alt[FZ_MAXCOLORS];
	int i, k;
	i = ind[0] * 255;
	i = CLAMP(i, 0, cs->high);
	for (k = 0; k < cs->base->n; k++)
		alt[k] = cs->lookup[i * cs->base->n + k] / 255.0;
	cs->base->toxyz(cs->base, alt, xyz);
}
#endif

static void
dropindexed(fz_colorspace *fzcs)
{
	pdf_indexed *cs = (pdf_indexed *)fzcs;
	if (cs->base) fz_dropcolorspace(cs->base);
	if (cs->lookup) fz_free(cs->lookup);
}

static fz_error *
loadindexed(fz_colorspace **csp, pdf_xref *xref, fz_obj *array)
{
	fz_error *error;
	pdf_indexed *cs;
	fz_obj *baseobj = fz_arrayget(array, 1);
	fz_obj *highobj = fz_arrayget(array, 2);
	fz_obj *lookup = fz_arrayget(array, 3);
	fz_colorspace *base;
	int n;

	pdf_logrsrc("load Indexed {\n");

	error = pdf_resolve(&baseobj, xref);
	if (error)
		return error;
	error = pdf_loadcolorspace(&base, xref, baseobj);
	fz_dropobj(baseobj);
	if (error)
		return error;

	pdf_logrsrc("base %s\n", base->name);

	cs = fz_malloc(sizeof(pdf_indexed));
	if (!cs)
	{
		fz_dropcolorspace(base);
		return fz_outofmem;
	}

	initcs((fz_colorspace*)cs, "Indexed", 1, nil, nil, dropindexed);

	cs->base = base;
	cs->high = fz_toint(highobj);

	n = base->n * (cs->high + 1);

	cs->lookup = fz_malloc(n);
	if (!cs->lookup)
	{
		fz_dropcolorspace((fz_colorspace*)cs);
		return fz_outofmem;
	}

	if (fz_isstring(lookup) && fz_tostrlen(lookup) == n)
	{
		unsigned char *buf;
		int i;

		pdf_logrsrc("string lookup\n");

		buf = fz_tostrbuf(lookup);
		for (i = 0; i < n; i++)
			cs->lookup[i] = buf[i];
	}

	if (fz_isindirect(lookup))
	{
		fz_buffer *buf;
		int i;

		pdf_logrsrc("stream lookup\n");

		error = pdf_loadstream(&buf, xref, fz_tonum(lookup), fz_togen(lookup));
		if (error)
		{
			fz_dropcolorspace((fz_colorspace*)cs);
			return error;
		}

		for (i = 0; i < n && i < (buf->wp - buf->rp); i++)
			cs->lookup[i] = buf->rp[i];

		fz_dropbuffer(buf);
	}

	pdf_logrsrc("}\n");

	*csp = (fz_colorspace*)cs;
	return nil;
}

/*
 * Parse and create colorspace from PDF object.
 */

fz_error *
pdf_loadcolorspace(fz_colorspace **csp, pdf_xref *xref, fz_obj *obj)
{
	if (fz_isname(obj))
	{
		if (!strcmp(fz_toname(obj), "DeviceGray"))
			*csp = pdf_devicegray;
		else if (!strcmp(fz_toname(obj), "DeviceRGB"))
			*csp = pdf_devicergb;
		else if (!strcmp(fz_toname(obj), "DeviceCMYK"))
			*csp = pdf_devicecmyk;
		else if (!strcmp(fz_toname(obj), "G"))
			*csp = pdf_devicegray;
		else if (!strcmp(fz_toname(obj), "RGB"))
			*csp = pdf_devicergb;
		else if (!strcmp(fz_toname(obj), "CMYK"))
			*csp = pdf_devicecmyk;
		else if (!strcmp(fz_toname(obj), "Pattern"))
			*csp = pdf_devicepattern;
		else
			return fz_throw("unknown colorspace: %s", fz_toname(obj));
		return nil;
	}

	else if (fz_isarray(obj))
	{
		fz_obj *name = fz_arrayget(obj, 0);

		if (fz_isname(name))
		{
			if (!strcmp(fz_toname(name), "CalCMYK"))
				*csp = pdf_devicecmyk;

#ifdef USECAL
			else if (!strcmp(fz_toname(name), "CalGray"))
				return loadcalgray(csp, xref, fz_arrayget(obj, 1));
			else if (!strcmp(fz_toname(name), "CalRGB"))
				return loadcalrgb(csp, xref, fz_arrayget(obj, 1));
			else if (!strcmp(fz_toname(name), "Lab"))
				return loadlab(csp, xref, fz_arrayget(obj, 1));
#else
			else if (!strcmp(fz_toname(name), "CalGray"))
				*csp = pdf_devicegray;
			else if (!strcmp(fz_toname(name), "CalRGB"))
				*csp = pdf_devicergb;
			else if (!strcmp(fz_toname(name), "Lab"))
				*csp = pdf_devicelab;
#endif

			else if (!strcmp(fz_toname(name), "ICCBased"))
				return loadiccbased(csp, xref, fz_arrayget(obj, 1));

			else if (!strcmp(fz_toname(name), "Indexed"))
				return loadindexed(csp, xref, obj);

			else if (!strcmp(fz_toname(name), "I"))
				return loadindexed(csp, xref, obj);

			else if (!strcmp(fz_toname(name), "Separation"))
				return loadseparation(csp, xref, obj);

			else if (!strcmp(fz_toname(name), "DeviceN"))
				return loadseparation(csp, xref, obj);

			/* load base colorspace instead */
			else if (!strcmp(fz_toname(name), "Pattern"))
			{
				fz_error *error;

				obj = fz_arrayget(obj, 1);
				if (!obj)
				{
					*csp = pdf_devicepattern;
					return nil;
				}

				error = pdf_resolve(&obj, xref);
				if (error)
					return error;
				error = pdf_loadcolorspace(csp, xref, obj);
				fz_dropobj(obj);
				return error;
			}

			else
				return fz_throw("syntaxerror: unknown colorspace %s", fz_toname(name));

			return nil;
		}
	}

	return fz_throw("syntaxerror: could not parse color space");
}

