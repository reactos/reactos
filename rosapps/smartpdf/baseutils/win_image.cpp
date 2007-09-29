/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "base_util.h"
#include "win_image.h"
#define INITGUID
#include <initguid.h>
#include <Imaging.h>

#pragma comment(lib, "imaging")

/* TODO: those only work on wince 5.0 so we should have fallback for
   earlier versions */

struct win_image {
    IImage *    image;
    int         dx;
    int         dy;
};

win_image *win_image_from_file(const TCHAR *file_path)
{
    IImagingFactory* imageFactory;
    ImageInfo        info;
    win_image *      img = NULL;

    img = SAZ(win_image);
    if (!img)
        return NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT hr = CoCreateInstance (CLSID_ImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    IID_IImagingFactory, (LPVOID*)&imageFactory);

    if (FAILED(hr))
        goto Error;
    
    hr = imageFactory->CreateImageFromFile(file_path, &img->image);
    imageFactory->Release();
    if (FAILED(hr))
        goto Error;

    img->image->GetImageInfo(&info);

    img->dx = info.Width;
    img->dy = info.Height;

    return img;

Error:
    win_image_delete(img);
    return NULL;
}

win_image * win_image_from_buffer(void *buf, UINT buf_size)
{
    IImagingFactory* imageFactory;
    ImageInfo        info;
    win_image *      img = NULL;

    img = SAZ(win_image);
    if (!img)
        return NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT hr = CoCreateInstance (CLSID_ImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    IID_IImagingFactory, (LPVOID*)&imageFactory);

    if (FAILED(hr))
        goto Error;
    
    hr = imageFactory->CreateImageFromBuffer(buf, buf_size,  BufferDisposalFlagNone, &img->image);
    imageFactory->Release();
    if (FAILED(hr))
        goto Error;

    img->image->GetImageInfo(&info);

    img->dx = info.Width;
    img->dy = info.Height;

    return img;

Error:
    win_image_delete(img);
    return NULL;
}

win_image *win_image_from_resource(HMODULE hinst, int resource_id)
{
    HRSRC   hres;
    void *  buf;
    DWORD   res_size;
    HGLOBAL hdata;

    hres = FindResource(hinst, MAKEINTRESOURCE(resource_id), RT_RCDATA);
    if (!hres)
        return NULL;

    res_size = SizeofResource(hinst, hres);
    if (0 == res_size)
        return NULL;

    hdata = LoadResource(hinst, hres);
    if (!hdata)
        return NULL;

    buf = (void*)LockResource(hdata);
    if (!buf)
        return NULL;

    return win_image_from_buffer(buf, (UINT)res_size);
}

void win_image_delete(win_image *img)
{
    assert(img);
    if (!img)
        return;
    if (img->image)
        img->image->Release();
    free(img);
}
void win_image_blit_at(win_image *img, HDC dc, int x, int y)
{
    RECT r = {0};
    assert(img);
    if (!img)
        return;

    assert(img->image);
    if (!img->image)
        return;
    r.left = x;
    r.right = x + img->dx;
    r.top = y;
    r.bottom = y + img->dy;
    img->image->Draw(dc, &r, NULL);
}

