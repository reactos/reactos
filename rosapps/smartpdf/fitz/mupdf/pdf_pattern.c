#include <fitz.h>
#include <mupdf.h>

pdf_pattern *
pdf_keeppattern(pdf_pattern *pat)
{
    assert(pat->refs > 0);
    pat->refs ++;
    return pat;
}

void
pdf_droppattern(pdf_pattern *pat)
{
    assert(pat->refs > 0);
    if (--pat->refs == 0)
    {
        if (pat->tree)
            fz_droptree(pat->tree);
        fz_free(pat);
    }
}

fz_error *
pdf_loadpattern(pdf_pattern **patp, pdf_xref *xref, fz_obj *dict, fz_obj *stmref)
{
    fz_error *error;
    pdf_pattern *pat;
    fz_stream *stm;
    fz_obj *resources;
    fz_obj *obj;
    pdf_csi *csi;

    *patp = pdf_finditem(xref->store, PDF_KPATTERN, stmref);
    if (*patp)
    {
        pdf_keeppattern(*patp);
        return nil;
    }

    pdf_logrsrc("load pattern %d %d {\n", fz_tonum(stmref), fz_togen(stmref));

    pat = fz_malloc(sizeof(pdf_pattern));
    if (!pat)
        return fz_outofmem;

    pat->refs = 1;
    pat->tree = nil;
    pat->ismask = fz_toint(fz_dictgets(dict, "PaintType")) == 2;
    pat->xstep = fz_toreal(fz_dictgets(dict, "XStep"));
    pat->ystep = fz_toreal(fz_dictgets(dict, "YStep"));

    pdf_logrsrc("mask %d\n", pat->ismask);
    pdf_logrsrc("xstep %g\n", pat->xstep);
    pdf_logrsrc("ystep %g\n", pat->ystep);

    obj = fz_dictgets(dict, "BBox");
    pat->bbox = pdf_torect(obj);

    pdf_logrsrc("bbox [%g %g %g %g]\n",
        pat->bbox.x0, pat->bbox.y0,
        pat->bbox.x1, pat->bbox.y1);

    obj = fz_dictgets(dict, "Matrix");
    if (obj)
        pat->matrix = pdf_tomatrix(obj);
    else
        pat->matrix = fz_identity();

    pdf_logrsrc("matrix [%g %g %g %g %g %g]\n",
        pat->matrix.a, pat->matrix.b,
        pat->matrix.c, pat->matrix.d,
        pat->matrix.e, pat->matrix.f);

    /*
     * Resources
     */

    obj = fz_dictgets(dict, "Resources");
    if (!obj) {
        error = fz_throw("syntaxerror: Pattern missing Resources");
        goto cleanup;
    }

    error = pdf_resolve(&obj, xref);
    if (error)
        goto cleanup;

    error = pdf_loadresources(&resources, xref, obj);

    fz_dropobj(obj);

    if (error)
        goto cleanup;

    /*
     * Content stream
     */

    pdf_logrsrc("content stream\n");

    error = pdf_newcsi(&csi, pat->ismask);
    if (error)
        goto cleanup;

    error = pdf_openstream(&stm, xref, fz_tonum(stmref), fz_togen(stmref));
    if (error)
        goto cleanup2;

    error = pdf_runcsi(csi, xref, resources, stm);

    fz_dropstream(stm);

    if (error)
        goto cleanup2;

    pat->tree = csi->tree;
    csi->tree = nil;

    pdf_dropcsi(csi);

    fz_dropobj(resources);

    pdf_logrsrc("optimize tree\n");
    fz_optimizetree(pat->tree);

    pdf_logrsrc("}\n");

    error = pdf_storeitem(xref->store, PDF_KPATTERN, stmref, pat);
    if (error)
        goto cleanup;

    *patp = pat;
    return nil;

cleanup2:
    pdf_dropcsi(csi);
cleanup:
    pdf_droppattern(pat);
    return error;
}

