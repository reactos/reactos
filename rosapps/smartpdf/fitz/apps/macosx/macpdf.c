#include <Carbon/Carbon.h>

#include <fitz.h>
#include <mupdf.h>
#include "pdfapp.h"

#define gDefaultFilename "/Users/giles/projects/ghostscript/tiger.pdf"

#define kViewClassID CFSTR("com.artofcode.ghostpdf.View")
#define kViewPrivate 'MU_v'

/* the pdfapp abstraction currently uses magic callbacks, so we have
   to use a a global state for our own data, or subclass pdfapp_t and
   do a lot of casting */

/* pdfapp callbacks - error handling */
void winwarn(pdfapp_t *pdf, char *msg)
{
	fprintf(stderr, "ghostpdf warning: %s\n", msg);
}

void winerror(pdfapp_t *pdf, char *msg)
{
	fprintf(stderr, "ghostpdf error: %s\n", msg);
	exit(1);
}

/* pdfapp callbacks - drawing */

void wintitle(pdfapp_t *pdf, char *title)
{
    /* set new document title */
}

void winresize(pdfapp_t *pdf, int w, int h)
{
    /* pdfapp as been asked to shrinkwrap the document image;
       we're called to actually apply the new window size. */
}

void winconvert(pdfapp_t *pdf, fz_pixmap *image)
{
    /* notification that page drawing is complete */
    /* do nothing */
}

void winrepaint(pdfapp_t *pdf)
{
    /* image needs repainting */
    HIViewSetNeedsDisplay((HIViewRef)pdf->userdata, true);
}

char* winpassword(pdfapp_t *pdf, char *filename)
{
    /* prompt user for document password */
    return NULL;
}

void winopenuri(pdfapp_t *pdf, char *s)
{
    /* user clicked on an external uri */
    /* todo: launch browser and/or open a new window if it's a PDF */
}

void wincursor(pdfapp_t *pdf, int curs)
{
    /* cursor status change notification */
}

void windocopy(pdfapp_t *pdf)
{
    /* user selected some text; copy it to the clipboard */
}

static OSStatus
view_construct(EventRef inEvent)
{
    OSStatus err;
    pdfapp_t *pdf;

    pdf = (pdfapp_t *)malloc(sizeof(pdfapp_t));
    require_action(pdf != NULL, CantMalloc, err = memFullErr);
    
    pdfapp_init(pdf);

    err = GetEventParameter(inEvent, kEventParamHIObjectInstance,
			    typeHIObjectRef, NULL, sizeof(HIObjectRef), NULL,
			    (HIObjectRef *)&pdf->userdata);
    require_noerr(err, ParameterMissing);
    err = SetEventParameter(inEvent, kEventParamHIObjectInstance,
			    typeVoidPtr, sizeof(pdfapp_t *), &pdf);

 ParameterMissing:
    if (err != noErr)
	free(pdf);

 CantMalloc:
    return err;
}

static OSStatus
view_destruct(EventRef inEvent, pdfapp_t *pdf)
{
    pdfapp_close(pdf);
    free(pdf);
    return noErr;
}

static OSStatus
view_initialize(EventHandlerCallRef inCallRef, EventRef inEvent,
		   pdfapp_t *pdf)
{
    OSStatus err;
    HIRect bounds;
    HIViewRef view = (HIViewRef)pdf->userdata;
    
    err = CallNextEventHandler(inCallRef, inEvent);
    require_noerr(err, TroubleInSuperClass);

    HIViewGetBounds (view, &bounds);
    pdf->scrw = bounds.size.width;
    pdf->scrh = bounds.size.height;
    
    pdfapp_open(pdf, gDefaultFilename);
    
 TroubleInSuperClass:
    return err;
}

static void
cgcontext_set_rgba(CGContextRef ctx, unsigned int rgba)
{
    const double norm = 1.0 / 255;
    CGContextSetRGBFillColor(ctx,
			     ((rgba >> 24) & 0xff) * norm,
			     ((rgba >> 16) & 0xff) * norm,
			     ((rgba >> 8) & 0xff) * norm,
			     (rgba & 0xff) * norm);
}

static void
draw_rect(CGContextRef ctx, double x0, double y0, double x1, double y1,
	      unsigned int rgba)
{
    HIRect rect;

    cgcontext_set_rgba(ctx, rgba);
    rect.origin.x = x0;
    rect.origin.y = y0;
    rect.size.width = x1 - x0;
    rect.size.height = y1 - y0;
    CGContextFillRect(ctx, rect);
}

static OSStatus
view_draw(EventRef inEvent, pdfapp_t *pdf)
{
    OSStatus err;
    CGContextRef gc;
    CGDataProviderRef provider;
    CGImageRef image;
    CGColorSpaceRef colorspace;
    CGRect rect;

    err = GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef,
			    NULL, sizeof(CGContextRef), NULL, &gc);
    require_noerr(err, cleanup);

    colorspace = CGColorSpaceCreateDeviceRGB();
    provider = CGDataProviderCreateWithData(NULL, pdf->image->samples,
					    pdf->image->w * pdf->image->h * 4,
					    NULL);
    image = CGImageCreate(pdf->image->w, pdf->image->h,
			  8, 32, pdf->image->w * 4,
			  colorspace, kCGImageAlphaNoneSkipFirst, provider,
			  NULL, 0, kCGRenderingIntentDefault);

    rect.origin.x = 0;
    rect.origin.y = 0;
    rect.size.width = pdf->image->w;
    rect.size.height = pdf->image->h;
    HIViewDrawCGImage(gc, &rect, image);

    CGColorSpaceRelease(colorspace);
    CGDataProviderRelease(provider);

 cleanup:
    return err;
}

static OSStatus
view_get_data(EventRef inEvent, pdfapp_t *pdf)
{
    OSStatus err;
    OSType tag;
    Ptr ptr;
    Size outSize;

    /* Probably could use a bit more error checking here, for type
       and size match. Also, just returning a viewctx seems a
       little hacky. */
    err = GetEventParameter(inEvent, kEventParamControlDataTag, typeEnumeration,
			    NULL, sizeof(OSType), NULL, &tag);
    require_noerr(err, ParameterMissing);

    err = GetEventParameter(inEvent, kEventParamControlDataBuffer, typePtr,
			    NULL, sizeof(Ptr), NULL, &ptr);

    if (tag == kViewPrivate) {
	*((pdfapp_t **)ptr) = pdf;
	outSize = sizeof(pdfapp_t *);
    } else
	err = errDataNotSupported;

    if (err == noErr)
	err = SetEventParameter(inEvent, kEventParamControlDataBufferSize, typeLongInteger,
				sizeof(Size), &outSize);

 ParameterMissing:
    return err;
}

static OSStatus
view_set_data(EventRef inEvent, pdfapp_t *pdf)
{
    OSStatus err;
    Ptr ptr;
    OSType tag;

    err = GetEventParameter(inEvent, kEventParamControlDataTag, typeEnumeration,
			    NULL, sizeof(OSType), NULL, &tag);
    require_noerr(err, ParameterMissing);

    err = GetEventParameter(inEvent, kEventParamControlDataBuffer, typePtr,
			    NULL, sizeof(Ptr), NULL, &ptr);
    require_noerr(err, ParameterMissing);

    /* we think we don't use this */
    err = errDataNotSupported;

 ParameterMissing:
    return err;
}

static OSStatus
view_hittest(EventRef inEvent, pdfapp_t *pdf)
{
    OSStatus err;
    HIPoint where;
    HIRect bounds;
    ControlPartCode part;

    err = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint,
			    NULL, sizeof(HIPoint), NULL, &where);
    require_noerr(err, ParameterMissing);

    err = HIViewGetBounds(pdf->userdata, &bounds);
    require_noerr(err, ParameterMissing);

    if (CGRectContainsPoint(bounds, where))
	part = 1;
    else
	part = kControlNoPart;
    err = SetEventParameter(inEvent, kEventParamControlPart,
			    typeControlPartCode, sizeof(ControlPartCode),
			    &part);
    printf("hittest %g, %g!\n", where.x, where.y);

 ParameterMissing:
    return err;
}


pascal OSStatus
view_handler(EventHandlerCallRef inCallRef,
		EventRef inEvent,
		void* inUserData )
{
    OSStatus err = eventNotHandledErr;
    UInt32 eventClass = GetEventClass(inEvent);
    UInt32 eventKind = GetEventKind(inEvent);
    pdfapp_t *pdf = (pdfapp_t *)inUserData;

    switch (eventClass) {
    case kEventClassHIObject:
	switch (eventKind) {
	case kEventHIObjectConstruct:
	    err = view_construct(inEvent);
	    break;
	case kEventHIObjectInitialize:
	    err = view_initialize(inCallRef, inEvent, pdf);
	    break;
	case kEventHIObjectDestruct:
	    err = view_destruct(inEvent, pdf);
	    break;
	}
	break;
    case kEventClassControl:
	switch (eventKind) {
	case kEventControlInitialize:
	    err = noErr;
	    break;
	case kEventControlDraw:
	    err = view_draw(inEvent, pdf);
	    break;
	case kEventControlGetData:
	    err = view_get_data(inEvent, pdf);
	    break;
	case kEventControlSetData:
	    err = view_set_data(inEvent, pdf);
	    break;
	case kEventControlHitTest:
	    err = view_hittest(inEvent, pdf);
	    break;
	    /*...*/
	}
	break;
    }
    return err;
}

OSStatus
view_register(void)
{
    OSStatus err = noErr;
    static HIObjectClassRef view_ClassRef = NULL;

    if (view_ClassRef == NULL) {
	EventTypeSpec eventList[] = {
	    { kEventClassHIObject, kEventHIObjectConstruct },
	    { kEventClassHIObject, kEventHIObjectInitialize },
	    { kEventClassHIObject, kEventHIObjectDestruct },

	    { kEventClassControl, kEventControlActivate },
	    { kEventClassControl, kEventControlDeactivate },
	    { kEventClassControl, kEventControlDraw },
	    { kEventClassControl, kEventControlHiliteChanged },
	    { kEventClassControl, kEventControlHitTest },
	    { kEventClassControl, kEventControlInitialize },
	    { kEventClassControl, kEventControlGetData },
	    { kEventClassControl, kEventControlSetData },
	};
	err = HIObjectRegisterSubclass(kViewClassID,
				       kHIViewClassID,
				       NULL,
				       view_handler,
				       GetEventTypeCount(eventList),
				       eventList,
				       NULL,
				       &view_ClassRef);
    }
    return err;
}

OSStatus view_create(
	WindowRef			inWindow,
	const HIRect*		inBounds,
	HIViewRef*			outView)
{
    OSStatus err;
    EventRef event;

    err = view_register();
    require_noerr(err, CantRegister);

    err = CreateEvent(NULL, kEventClassHIObject, kEventHIObjectInitialize,
		      GetCurrentEventTime(), 0, &event);
    require_noerr(err, CantCreateEvent);

    if (inBounds != NULL) {
	err = SetEventParameter(event, 'Boun', typeHIRect, sizeof(HIRect),
				inBounds);
	require_noerr(err, CantSetParameter);
    }

    err = HIObjectCreate(kViewClassID, event, (HIObjectRef*)outView);
    require_noerr(err, CantCreate);

    if (inWindow != NULL) {
	HIViewRef root;
	err = GetRootControl(inWindow, &root);
	require_noerr(err, CantGetRootView);
	err = HIViewAddSubview(root, *outView);
    }
 CantCreate:
 CantGetRootView:
 CantSetParameter:
 CantCreateEvent:
    ReleaseEvent(event);
 CantRegister:
    return err;
}



int main(int argc, char *argv[])
{
    IBNibRef nibRef;
    OSStatus err;
    WindowRef window;

    pdfapp_t pdf;
    
    fz_cpudetect();
    fz_accelerate();

    err = view_register();
    require_noerr(err, CantRegisterView);

    err = CreateNibReference(CFSTR("main"), &nibRef);
    printf("err = %d\n", (int)err);
    require_noerr(err, CantGetNibRef);

    err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"));
    require_noerr(err, CantSetMenuBar);
 
    err = CreateWindowFromNib(nibRef, CFSTR("MainWindow"), &window);
    require_noerr(err, CantCreateWindow);

    //openpdf(window, gDefaultFilename);

    DisposeNibReference(nibRef);

    pdfapp_init(&pdf);
    pdfapp_open(&pdf, gDefaultFilename);
    
    ShowWindow(window);
    RunApplicationEventLoop();

    pdfapp_close(&pdf);

 CantGetNibRef:
 CantSetMenuBar:
 CantCreateWindow:
 CantRegisterView:

    return err;
}
