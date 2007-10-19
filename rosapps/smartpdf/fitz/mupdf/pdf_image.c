/*
 * TODO: this needs serious cleaning up, and error checking.
 */

#include "fitz.h"
#include "mupdf.h"

void pdf_dropimage(fz_image *fzimg)
{
	pdf_image *img = (pdf_image*)fzimg;
	fz_dropbuffer(img->samples);
	if (img->mask)
		fz_dropimage(img->mask);
}

fz_error *
pdf_loadinlineimage(pdf_image **imgp, pdf_xref *xref,
		fz_obj *rdb, fz_obj *dict, fz_stream *file)
{
	fz_error *error;
	pdf_image *img;
	fz_filter *filter;
	fz_obj *f;
	fz_obj *cs;
	fz_obj *d;
	int ismask;
	int i;

	img = fz_malloc(sizeof(pdf_image));
	if (!img)
		return fz_outofmem;

	pdf_logimage("load inline image %p {\n", img);

	img->super.refs = 1;
	img->super.loadtile = pdf_loadtile;
	img->super.drop = pdf_dropimage;
	img->super.n = 0;
	img->super.a = 0;
	img->indexed = nil;
	img->usecolorkey = 0;
	img->mask = nil;

	img->super.w = fz_toint(fz_dictgetsa(dict, "Width", "W"));
	img->super.h = fz_toint(fz_dictgetsa(dict, "Height", "H"));
	img->bpc = fz_toint(fz_dictgetsa(dict, "BitsPerComponent", "BPC"));
	ismask = fz_tobool(fz_dictgetsa(dict, "ImageMask", "IM"));
	d = fz_dictgetsa(dict, "Decode", "D");
	cs = fz_dictgetsa(dict, "ColorSpace", "CS");

	pdf_logimage("size %dx%d %d\n", img->super.w, img->super.h, img->bpc);

	if (ismask)
	{
		pdf_logimage("is mask\n");
		img->super.cs = nil;
		img->super.n = 0;
		img->super.a = 1;
		img->bpc = 1;
	}

	if (cs)
	{
		img->super.cs = nil;

		if (fz_isname(cs))
		{
			fz_obj *csd = fz_dictgets(rdb, "ColorSpace");
			if (csd)
			{
				fz_obj *cso = fz_dictget(csd, cs);
				img->super.cs = pdf_finditem(xref->store, PDF_KCOLORSPACE, cso);
				if (img->super.cs)
					fz_keepcolorspace(img->super.cs);
			}
		}

		if (!img->super.cs)
		{
			/* XXX danger! danger! does this resolve? */
			error = pdf_loadcolorspace(&img->super.cs, xref, cs);
			if (error)
				return error;
		}

		if (!strcmp(img->super.cs->name, "Indexed"))
		{
			pdf_logimage("indexed\n");
			img->indexed = (pdf_indexed*)img->super.cs;
			img->super.cs = img->indexed->base;
		}

		pdf_logimage("colorspace %s\n", img->super.cs->name);

		img->super.n = img->super.cs->n;
		img->super.a = 0;
	}

	if (fz_isarray(d))
	{
		pdf_logimage("decode array\n");
		if (img->indexed)
			for (i = 0; i < 2; i++)
				img->decode[i] = fz_toreal(fz_arrayget(d, i));
		else
			for (i = 0; i < (img->super.n + img->super.a) * 2; i++)
				img->decode[i] = fz_toreal(fz_arrayget(d, i));
	}
	else
	{
		if (img->indexed)
			for (i = 0; i < 2; i++)
				img->decode[i] = i & 1 ? (1 << img->bpc) - 1 : 0;
		else
			for (i = 0; i < (img->super.n + img->super.a) * 2; i++)
				img->decode[i] = i & 1;
	}

	if (img->indexed)
		img->stride = (img->super.w * img->bpc + 7) / 8;
	else
		img->stride = (img->super.w * (img->super.n + img->super.a) * img->bpc + 7) / 8;

	/* load image data */

	f = fz_dictgetsa(dict, "Filter", "F");
	if (f)
	{
		fz_stream *tempfile;

		error = pdf_buildinlinefilter(&filter, dict);
		if (error)
			return error;

		if (filter == nil)
		    goto thereisnofilter;

		error = fz_openrfilter(&tempfile, filter, file);
		if (error)
			return error;

		i = fz_readall(&img->samples, tempfile);
		if (i < 0)
			return fz_ioerror(tempfile);

		fz_dropfilter(filter);
		fz_dropstream(tempfile);
	}
	else
	{
thereisnofilter:
		error = fz_newbuffer(&img->samples, img->super.h * img->stride);
		if (error)
			return error;

		i = fz_read(file, img->samples->bp, img->super.h * img->stride);
		if (i < 0)
			return fz_ioerror(file);

		img->samples->wp += img->super.h * img->stride;
	}

	/* 0 means opaque and 1 means transparent, so we invert to get alpha */
	if (ismask)
	{
		unsigned char *p;
		for (p = img->samples->bp; p < img->samples->ep; p++)
			*p = ~*p;
	}

	pdf_logimage("}\n");

	*imgp = img;
	return nil;
}

static void
loadcolorkey(int *colorkey, int bpc, int indexed, fz_obj *obj)
{
	int scale = 1;
	int i;

	pdf_logimage("keyed mask\n");

	if (!indexed)
	{
		switch (bpc)
		{
		case 1: scale = 255; break;
		case 2: scale = 85; break;
		case 4: scale = 17; break;
		case 8: scale = 1; break;
		}
	}

	for (i = 0; i < fz_arraylen(obj); i++)
		colorkey[i] = fz_toint(fz_arrayget(obj, i)) * scale;
}

/* TODO error cleanup */
fz_error *
pdf_loadimage(pdf_image **imgp, pdf_xref *xref, fz_obj *dict, fz_obj *ref)
{
	fz_error *error;
	pdf_image *img;
	pdf_image *mask;
	int ismask;
	fz_obj *obj;
	fz_obj *sub;
	int i;

	int w, h, bpc;
	int n = 0;
	int a = 0;
	int usecolorkey = 0;
	fz_colorspace *cs = nil;
	pdf_indexed *indexed = nil;
	int stride;

	if ((*imgp = pdf_finditem(xref->store, PDF_KIMAGE, ref)))
	{
		fz_keepimage((fz_image*)*imgp);
		return nil;
	}

	img = fz_malloc(sizeof(pdf_image));
	if (!img)
		return fz_outofmem;

	pdf_logimage("load image %d %d (%p) {\n", fz_tonum(ref), fz_togen(ref), img);

	/*
	 * Dimensions, BPC and ColorSpace
	 */

	w = fz_toint(fz_dictgets(dict, "Width"));
	h = fz_toint(fz_dictgets(dict, "Height"));
	bpc = fz_toint(fz_dictgets(dict, "BitsPerComponent"));

	pdf_logimage("size %dx%d %d\n", w, h, bpc);

	cs = nil;
	obj = fz_dictgets(dict, "ColorSpace");
	if (obj)
	{
		cs = pdf_finditem(xref->store, PDF_KCOLORSPACE, obj);
		if (cs)
			fz_keepcolorspace(cs);
		else
		{
			error = pdf_resolve(&obj, xref);
			if (error)
				return error;

			error = pdf_loadcolorspace(&cs, xref, obj);
			if (error)
				return error;

			fz_dropobj(obj);
		}

		if (!strcmp(cs->name, "Indexed"))
		{
			pdf_logimage("indexed\n");
			indexed = (pdf_indexed*)cs;
			cs = indexed->base;
		}
		n = cs->n;
		a = 0;

		pdf_logimage("colorspace %s\n", cs->name);
	}

	/*
	 * ImageMask, Mask and SoftMask
	 */

	mask = nil;

	ismask = fz_tobool(fz_dictgets(dict, "ImageMask"));
	if (ismask)
	{
		pdf_logimage("is mask\n");
		bpc = 1;
		n = 0;
		a = 1;
	}

	obj = fz_dictgets(dict, "SMask");
	if (fz_isindirect(obj))
	{
		pdf_logimage("has soft mask\n");

		error = pdf_loadindirect(&sub, xref, obj);
		if (error)
			return error;

		error = pdf_loadimage(&mask, xref, sub, obj);
		fz_dropobj(sub);
		if (error)
			return error;

		if (mask->super.cs != pdf_devicegray)
			return fz_throw("syntaxerror: SMask must be DeviceGray");

		mask->super.cs = 0;
		mask->super.n = 0;
		mask->super.a = 1;
	}

	obj = fz_dictgets(dict, "Mask");
	if (fz_isindirect(obj))
	{
		error = pdf_loadindirect(&sub, xref, obj);
		if (error)
			return error;
		if (fz_isarray(sub))
		{
			usecolorkey = 1;
			loadcolorkey(img->colorkey, bpc, indexed != nil, sub);
		}
		else
		{
			pdf_logimage("has mask\n");
			error = pdf_loadimage(&mask, xref, sub, obj);
			if (error)
				return error;
		}
		fz_dropobj(sub);
	}
	else if (fz_isarray(obj))
	{
		usecolorkey = 1;
		loadcolorkey(img->colorkey, bpc, indexed != nil, obj);
	}

	/*
	 * Decode
	 */

	obj = fz_dictgets(dict, "Decode");
	if (fz_isarray(obj))
	{
		pdf_logimage("decode array\n");
		if (indexed)
			for (i = 0; i < 2; i++)
				img->decode[i] = fz_toreal(fz_arrayget(obj, i));
		else
			for (i = 0; i < (n + a) * 2; i++)
				img->decode[i] = fz_toreal(fz_arrayget(obj, i));
	}
	else
	{
		if (indexed)
			for (i = 0; i < 2; i++)
				img->decode[i] = i & 1 ? (1 << bpc) - 1 : 0;
		else
			for (i = 0; i < (n + a) * 2; i++)
				img->decode[i] = i & 1;
	}

	/*
	 * Load samples
	 */

	if (indexed)
		stride = (w * bpc + 7) / 8;
	else
		stride = (w * (n + a) * bpc + 7) / 8;

	error = pdf_loadstream(&img->samples, xref, fz_tonum(ref), fz_togen(ref));
	if (error)
	{
		/* TODO: colorspace? */
		fz_free(img);
		return error;
	}

	if (img->samples->wp - img->samples->bp < stride * h)
	{
		/* TODO: colorspace? */
		fz_dropbuffer(img->samples);
		fz_free(img);
		return fz_throw("syntaxerror: truncated image data");
	}

	/* 0 means opaque and 1 means transparent, so we invert to get alpha */
	if (ismask)
	{
		unsigned char *p;
		for (p = img->samples->bp; p < img->samples->ep; p++)
			*p = ~*p;
	}

	/*
	 * Create image object
	 */
    img->super.refs = 1;
	img->super.loadtile = pdf_loadtile;
	img->super.drop = pdf_dropimage;
	img->super.cs = cs;
	img->super.w = w;
	img->super.h = h;
	img->super.n = n;
	img->super.a = a;
	img->indexed = indexed;
	img->stride = stride;
	img->bpc = bpc;
	img->mask = (fz_image*)mask;
	img->usecolorkey = usecolorkey;

	pdf_logimage("}\n");

	error = pdf_storeitem(xref->store, PDF_KIMAGE, ref, img);
	if (error)
	{
		fz_dropimage((fz_image*)img);
		return error;
	}

	*imgp = img;
	return nil;
}

static void
maskcolorkey(fz_pixmap *pix, int *colorkey)
{
	unsigned char *p = pix->samples;
	int i, k, t;
	for (i = 0; i < pix->w * pix->h; i++)
	{
		t = 1;
		for (k = 1; k < pix->n; k++)
			if (p[k] < colorkey[k * 2 - 2] || p[k] > colorkey[k * 2 - 1])
				t = 0;
		if (t)
			for (k = 0; k < pix->n; k++)
				p[k] = 0;
		p += pix->n;
	}
}

static void
maskcolorkeyindexed(fz_pixmap *ind, fz_pixmap *pix, int *colorkey)
{
	unsigned char *s = ind->samples;
	unsigned char *d = pix->samples;
	int i, k;
	for (i = 0; i < pix->w * pix->h; i++)
	{
		if (s[0] >= colorkey[0] && s[0] <= colorkey[1])
			for (k = 0; k < pix->n; k++)
				d[k] = 0;
		d[0] = 255;
		s += ind->n;
		d += pix->n;
	}
}

fz_error *
pdf_loadtile(fz_image *img, fz_pixmap *tile)
{
	pdf_image *src = (pdf_image*)img;
	void (*tilefunc)(unsigned char*,int,unsigned char*, int, int, int, int);
	fz_error *error;

	assert(tile->n == img->n + 1);
	assert(tile->x >= 0);
	assert(tile->y >= 0);
	assert(tile->x + tile->w <= img->w);
	assert(tile->y + tile->h <= img->h);

	switch (src->bpc)
	{
	case 1: tilefunc = fz_loadtile1; break;
	case 2: tilefunc = fz_loadtile2; break;
	case 4: tilefunc = fz_loadtile4; break;
	case 8: tilefunc = fz_loadtile8; break;
	default:
		return fz_throw("rangecheck: unsupported bit depth: %d", src->bpc);
	}

	if (src->indexed)
	{
		fz_pixmap *tmp;
		int x, y, k, i;
		int bpcfact = 1;

		error = fz_newpixmap(&tmp, tile->x, tile->y, tile->w, tile->h, 1);
		if (error)
			return error;

		switch (src->bpc)
		{
		case 1: bpcfact = 255; break;
		case 2: bpcfact = 85; break;
		case 4: bpcfact = 17; break;
		case 8: bpcfact = 1; break;
		}

		tilefunc(src->samples->rp, src->stride,
				tmp->samples, tmp->w,
				tmp->w, tmp->h, 0);

		for (y = 0; y < tile->h; y++)
		{
			for (x = 0; x < tile->w; x++)
			{
				tile->samples[(y * tile->w + x) * tile->n] = 255;
				i = tmp->samples[y * tile->w + x] / bpcfact;
				i = CLAMP(i, 0, src->indexed->high);
				for (k = 0; k < src->indexed->base->n; k++)
				{
					tile->samples[(y * tile->w + x) * tile->n + k + 1] =
						src->indexed->lookup[i * src->indexed->base->n + k];
				}
			}
		}

		if (src->usecolorkey)
			maskcolorkeyindexed(tmp, tile, src->colorkey);

		fz_droppixmap(tmp);
	}

	else
	{
		tilefunc(src->samples->rp, src->stride,
				tile->samples, tile->w * tile->n,
				img->w * (img->n + img->a), img->h, img->a ? 0 : img->n);
		if (src->usecolorkey)
			maskcolorkey(tile, src->colorkey);
		fz_decodetile(tile, !img->a, src->decode);
	}

	return nil;
}

