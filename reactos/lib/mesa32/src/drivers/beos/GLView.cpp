/*
 * Mesa 3-D graphics library
 * Version:  6.1
 * 
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <assert.h>
#include <stdio.h>

extern "C" {

#include "glheader.h"
#include "version.h"
#include "buffers.h"
#include "bufferobj.h"
#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
#include "mtypes.h"
#include "texformat.h"
#include "texobj.h"
#include "teximage.h"
#include "texstore.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "swrast/s_trispan.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

}	// extern "C"

#include <interface/Screen.h>
#include <GLView.h>

// BeOS component ordering for B_RGBA32 bitmap format
#if B_HOST_IS_LENDIAN
	#define BE_RCOMP 2
	#define BE_GCOMP 1
	#define BE_BCOMP 0
	#define BE_ACOMP 3

	#define PACK_B_RGBA32(color) (color[BCOMP] | (color[GCOMP] << 8) | \
							(color[RCOMP] << 16) | (color[ACOMP] << 24))

	#define PACK_B_RGB32(color) (color[BCOMP] | (color[GCOMP] << 8) | \
  							(color[RCOMP] << 16) | 0xFF000000)
#else
	// Big Endian B_RGBA32 bitmap format
	#define BE_RCOMP 1
	#define BE_GCOMP 2
	#define BE_BCOMP 3
	#define BE_ACOMP 0

	#define PACK_B_RGBA32(color) (color[ACOMP] | (color[RCOMP] << 8) | \
							(color[GCOMP] << 16) | (color[BCOMP] << 24))

	#define PACK_B_RGB32(color) ((color[RCOMP] << 8) | (color[GCOMP] << 16) | \
  							(color[BCOMP] << 24) | 0xFF000000)
#endif

#define FLIP(coord) (LIBGGI_MODE(ggi_ctx->ggi_visual)->visible.y-(coord) - 1) 

const char * color_space_name(color_space space);

//
// This object hangs off of the BGLView object.  We have to use
// Be's BGLView class as-is to maintain binary compatibility (we
// can't add new members to it).  Instead we just put all our data
// in this class and use BGLVIew::m_gc to point to it.
//
class MesaDriver
{
friend class BGLView;
public:
	MesaDriver();
	~MesaDriver();
	
	void 		Init(BGLView * bglview, GLcontext * c, GLvisual * v, GLframebuffer * b);

	void 		LockGL();
	void 		UnlockGL();
	void 		SwapBuffers() const;
	status_t 	CopyPixelsOut(BPoint source, BBitmap *dest);
	status_t 	CopyPixelsIn(BBitmap *source, BPoint dest);

	void CopySubBuffer(GLint x, GLint y, GLuint width, GLuint height) const;
	void Draw(BRect updateRect) const;

private:
	MesaDriver(const MesaDriver &rhs);  // copy constructor illegal
	MesaDriver &operator=(const MesaDriver &rhs);  // assignment oper. illegal

	GLcontext * 	m_glcontext;
	GLvisual * 		m_glvisual;
	GLframebuffer *	m_glframebuffer;

	BGLView *		m_bglview;
	BBitmap *		m_bitmap;

	GLchan 			m_clear_color[4];  // buffer clear color
	GLuint 			m_clear_index;      // buffer clear color index
	GLint 			m_bottom;           // used for flipping Y coords
	GLuint 			m_width;
	GLuint			m_height;
	
   // Mesa Device Driver callback functions
   static void 		UpdateState(GLcontext *ctx, GLuint new_state);
   static void 		ClearIndex(GLcontext *ctx, GLuint index);
   static void 		ClearColor(GLcontext *ctx, const GLfloat color[4]);
   static void 		Clear(GLcontext *ctx, GLbitfield mask,
                                GLboolean all, GLint x, GLint y,
                                GLint width, GLint height);
   static void 		ClearFront(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                          GLint width, GLint height);
   static void 		ClearBack(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                         GLint width, GLint height);
   static void 		Index(GLcontext *ctx, GLuint index);
   static void 		Color(GLcontext *ctx, GLubyte r, GLubyte g,
                     GLubyte b, GLubyte a);
   static void 		SetBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                             GLenum mode);
   static void 		GetBufferSize(GLframebuffer * framebuffer, GLuint *width,
                             GLuint *height);
   static void		Error(GLcontext *ctx);
   static const GLubyte *	GetString(GLcontext *ctx, GLenum name);
   static void          Viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);

   // Front-buffer functions
   static void 		WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);
   static void 		WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);
   static void 		WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                      GLint x, GLint y,
                                      const GLchan color[4],
                                      const GLubyte mask[]);
   static void 		WriteRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    CONST GLubyte rgba[][4],
                                    const GLubyte mask[]);
   static void 		WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                        const GLint x[], const GLint y[],
                                        const GLchan color[4],
                                        const GLubyte mask[]);
   static void 		WriteCI32SpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  const GLuint index[], const GLubyte mask[]);
   static void 		WriteCI8SpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLubyte index[], const GLubyte mask[]);
   static void 		WriteMonoCISpanFront(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[]);
   static void 		WriteCI32PixelsFront(const GLcontext *ctx,
                                    GLuint n, const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[]);
   static void 		WriteMonoCIPixelsFront(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[]);
   static void 		ReadCI32SpanFront(const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[]);
   static void 		ReadRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 GLubyte rgba[][4]);
   static void 		ReadCI32PixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[]);
   static void 		ReadRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[]);

   // Back buffer functions
   static void 		WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);
   static void 		WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);
   static void 		WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[]);
   static void 		WriteRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[]);
   static void 		WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[]);
   static void 		WriteCI32SpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[]);
   static void 		WriteCI8SpanBack(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[]);
   static void 		WriteMonoCISpanBack(const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y, GLuint colorIndex,
                                   const GLubyte mask[]);
   static void 		WriteCI32PixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[]);
   static void 		WriteMonoCIPixelsBack(const GLcontext *ctx,
                                     GLuint n, const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[]);
   static void 		ReadCI32SpanBack(const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[]);
   static void 		ReadRGBASpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                GLubyte rgba[][4]);
   static void 		ReadCI32PixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLuint indx[], const GLubyte mask[]);
   static void 		ReadRGBAPixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[]);

};

//------------------------------------------------------------------
// Public interface methods
//------------------------------------------------------------------


//
// Input:  rect - initial rectangle
//         name - window name
//         resizingMode - example: B_FOLLOW_NONE
//         mode - usually 0 ?
//         options - Bitwise-OR of BGL_* tokens
//
BGLView::BGLView(BRect rect, char *name,
                 ulong resizingMode, ulong mode,
                 ulong options)
   : BView(rect, name, B_FOLLOW_ALL_SIDES, mode | B_WILL_DRAW | B_FRAME_EVENTS) //  | B_FULL_UPDATE_ON_RESIZE)
{
	// We don't support single buffering (yet): double buffering forced.
	options |= BGL_DOUBLE;

   const GLboolean rgbFlag = ((options & BGL_INDEX) == 0);
   const GLboolean alphaFlag = ((options & BGL_ALPHA) == BGL_ALPHA);
   const GLboolean dblFlag = ((options & BGL_DOUBLE) == BGL_DOUBLE);
   const GLboolean stereoFlag = false;
   const GLint depth = (options & BGL_DEPTH) ? 16 : 0;
   const GLint stencil = (options & BGL_STENCIL) ? 8 : 0;
   const GLint accum = (options & BGL_ACCUM) ? 16 : 0;
   const GLint index = (options & BGL_INDEX) ? 32 : 0;
   const GLint red = rgbFlag ? 8 : 0;
   const GLint green = rgbFlag ? 8 : 0;
   const GLint blue = rgbFlag ? 8 : 0;
   const GLint alpha = alphaFlag ? 8 : 0;

	m_options = options | BGL_INDIRECT;

   struct dd_function_table functions;
 
   if (!rgbFlag) {
      fprintf(stderr, "Mesa Warning: color index mode not supported\n");
   }

   // Allocate auxiliary data object
   MesaDriver * md = new MesaDriver();

   // examine option flags and create gl_context struct
   GLvisual * visual = _mesa_create_visual( rgbFlag,
                                            dblFlag,
                                            stereoFlag,
                                            red, green, blue, alpha,
                                            index,
                                            depth,
                                            stencil,
                                            accum, accum, accum, accum,
                                            1
                                            );

	// Initialize device driver function table
	_mesa_init_driver_functions(&functions);

	functions.GetString 	= md->GetString;
	functions.UpdateState 	= md->UpdateState;
	functions.GetBufferSize = md->GetBufferSize;
	functions.Clear 		= md->Clear;
	functions.ClearIndex 	= md->ClearIndex;
	functions.ClearColor 	= md->ClearColor;
	functions.Error			= md->Error;
        functions.Viewport      = md->Viewport;

	// create core context
	GLcontext *ctx = _mesa_create_context(visual, NULL, &functions, md);
	if (! ctx) {
         _mesa_destroy_visual(visual);
         delete md;
         return;
      }
   _mesa_enable_sw_extensions(ctx);
   _mesa_enable_1_3_extensions(ctx);
   _mesa_enable_1_4_extensions(ctx);
   _mesa_enable_1_5_extensions(ctx);


   // create core framebuffer
   GLframebuffer * buffer = _mesa_create_framebuffer(visual,
                                              depth > 0 ? GL_TRUE : GL_FALSE,
                                              stencil > 0 ? GL_TRUE: GL_FALSE,
                                              accum > 0 ? GL_TRUE : GL_FALSE,
                                              alphaFlag
                                              );

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext(ctx);
   _ac_CreateContext(ctx);
   _tnl_CreateContext(ctx);
   _swsetup_CreateContext(ctx);
   _swsetup_Wakeup(ctx);

   md->Init(this, ctx, visual, buffer );

   // Hook aux data into BGLView object
   m_gc = md;

   // some stupid applications (Quake2) don't even think about calling LockGL()
   // before using glGetString and friends... so make sure there is at least a
   // valid context.
   if (!_mesa_get_current_context()) {
      LockGL();
      // not needed, we don't have a looper yet: UnlockLooper();
   }

}


BGLView::~BGLView()
{
   // printf("BGLView destructor\n");
   MesaDriver * md = (MesaDriver *) m_gc;
   assert(md);
   delete md;
}

void BGLView::LockGL()
{
   MesaDriver * md = (MesaDriver *) m_gc;
   assert(md);
   md->LockGL();
}

void BGLView::UnlockGL()
{
   MesaDriver * md = (MesaDriver *) m_gc;
   assert(md);
   md->UnlockGL();
}

void BGLView::SwapBuffers()
{
	SwapBuffers(false);
}

void BGLView::SwapBuffers(bool vSync)
{
	MesaDriver * md = (MesaDriver *) m_gc;
	assert(md);
	md->SwapBuffers();

	if (vSync) {
		BScreen screen(Window());
		screen.WaitForRetrace();
	}
}


#if 0
void BGLView::CopySubBufferMESA(GLint x, GLint y, GLuint width, GLuint height)
{
   MesaDriver * md = (MesaDriver *) m_gc;
   assert(md);
   md->CopySubBuffer(x, y, width, height);
}
#endif

BView *	BGLView::EmbeddedView()
{
	return NULL;
}

status_t BGLView::CopyPixelsOut(BPoint source, BBitmap *dest)
{
	if (! dest || ! dest->Bounds().IsValid())
		return B_BAD_VALUE;

	MesaDriver * md = (MesaDriver *) m_gc;
	assert(md);
	return md->CopyPixelsOut(source, dest);
}

status_t BGLView::CopyPixelsIn(BBitmap *source, BPoint dest)
{
	if (! source || ! source->Bounds().IsValid())
		return B_BAD_VALUE;

	MesaDriver * md = (MesaDriver *) m_gc;
	assert(md);
	return md->CopyPixelsIn(source, dest);
}


void BGLView::ErrorCallback(unsigned long errorCode) // Mesa's GLenum is not ulong but uint!
{
	char msg[32];
	sprintf(msg, "GL: Error code $%04lx.", errorCode);
	// debugger(msg);
	fprintf(stderr, "%s\n", msg);
	return;
}

void BGLView::Draw(BRect updateRect)
{
   // printf("BGLView::Draw()\n");
   MesaDriver * md = (MesaDriver *) m_gc;
   assert(md);
   md->Draw(updateRect);
}

void BGLView::AttachedToWindow()
{
   BView::AttachedToWindow();

   // don't paint window background white when resized
   SetViewColor(B_TRANSPARENT_32_BIT);
}

void BGLView::AllAttached()
{
   BView::AllAttached();
//   printf("BGLView AllAttached\n");
}

void BGLView::DetachedFromWindow()
{
   BView::DetachedFromWindow();
}

void BGLView::AllDetached()
{
   BView::AllDetached();
//   printf("BGLView AllDetached");
}

void BGLView::FrameResized(float width, float height)
{
   return BView::FrameResized(width, height);
}

status_t BGLView::Perform(perform_code d, void *arg)
{
   return BView::Perform(d, arg);
}


status_t BGLView::Archive(BMessage *data, bool deep) const
{
   return BView::Archive(data, deep);
}

void BGLView::MessageReceived(BMessage *msg)
{
   BView::MessageReceived(msg);
}

void BGLView::SetResizingMode(uint32 mode)
{
   BView::SetResizingMode(mode);
}

void BGLView::Show()
{
   BView::Show();
}

void BGLView::Hide()
{
   BView::Hide();
}

BHandler *BGLView::ResolveSpecifier(BMessage *msg, int32 index,
                                    BMessage *specifier, int32 form,
                                    const char *property)
{
   return BView::ResolveSpecifier(msg, index, specifier, form, property);
}

status_t BGLView::GetSupportedSuites(BMessage *data)
{
   return BView::GetSupportedSuites(data);
}

void BGLView::DirectConnected( direct_buffer_info *info )
{
#if 0
	if (! m_direct_connected && m_direct_connection_disabled) 
		return; 

	direct_info_locker->Lock(); 
	switch(info->buffer_state & B_DIRECT_MODE_MASK) { 
	case B_DIRECT_START: 
		m_direct_connected = true;
	case B_DIRECT_MODIFY: 
		// Get clipping information 
		if (m_clip_list)
			free(m_clip_list); 
		m_clip_list_count = info->clip_list_count; 
		m_clip_list = (clipping_rect *) malloc(m_clip_list_count*sizeof(clipping_rect)); 
		if (m_clip_list) { 
			memcpy(m_clip_list, info->clip_list, m_clip_list_count*sizeof(clipping_rect));
			fBits = (uint8 *) info->bits; 
			fRowBytes = info->bytes_per_row; 
			fFormat = info->pixel_format; 
			fBounds = info->window_bounds; 
			fDirty = true; 
		} 
		break; 
	case B_DIRECT_STOP: 
		fConnected = false; 
		break; 
	} 
	direct_info_locker->Unlock(); 
#endif
}

void BGLView::EnableDirectMode( bool enabled )
{
   // TODO
}


//---- virtual reserved methods ----------

void BGLView::_ReservedGLView1() {}
void BGLView::_ReservedGLView2() {}
void BGLView::_ReservedGLView3() {}
void BGLView::_ReservedGLView4() {}
void BGLView::_ReservedGLView5() {}
void BGLView::_ReservedGLView6() {}
void BGLView::_ReservedGLView7() {}
void BGLView::_ReservedGLView8() {}

#if 0
// Not implemented!!!

BGLView::BGLView(const BGLView &v)
	: BView(v)
{
   // XXX not sure how this should work
   printf("Warning BGLView::copy constructor not implemented\n");
}

BGLView &BGLView::operator=(const BGLView &v)
{
   printf("Warning BGLView::operator= not implemented\n");
	return *this;
}
#endif

void BGLView::dither_front()
{
   // no-op
}

bool BGLView::confirm_dither()
{
   // no-op
   return false;
}

void BGLView::draw(BRect r)
{
   // XXX no-op ???
}

/* Direct Window stuff */
void BGLView::drawScanline( int x1, int x2, int y, void *data )
{
   // no-op
}

void BGLView::scanlineHandler(struct rasStateRec *state,
                              GLint x1, GLint x2)
{
   // no-op
}

void BGLView::lock_draw()
{
   // no-op
}

void BGLView::unlock_draw()
{
   // no-op
}

bool BGLView::validateView()
{
   // no-op
   return true;
}

// #pragma mark -

MesaDriver::MesaDriver()
{
   m_glcontext 		= NULL;
   m_glvisual		= NULL;
   m_glframebuffer 	= NULL;
   m_bglview 		= NULL;
   m_bitmap 		= NULL;

   m_clear_color[BE_RCOMP] = 0;
   m_clear_color[BE_GCOMP] = 0;
   m_clear_color[BE_BCOMP] = 0;
   m_clear_color[BE_ACOMP] = 0;

   m_clear_index = 0;
}


MesaDriver::~MesaDriver()
{
   _mesa_destroy_visual(m_glvisual);
   _mesa_destroy_framebuffer(m_glframebuffer);
   _mesa_destroy_context(m_glcontext);
   
   delete m_bitmap;
}


void MesaDriver::Init(BGLView * bglview, GLcontext * ctx, GLvisual * visual, GLframebuffer * framebuffer)
{
	m_bglview 		= bglview;
	m_glcontext 	= ctx;
	m_glvisual 		= visual;
	m_glframebuffer = framebuffer;

	MesaDriver * md = (MesaDriver *) ctx->DriverCtx;
	struct swrast_device_driver * swdd = _swrast_GetDeviceDriverReference( ctx );
	TNLcontext * tnl = TNL_CONTEXT(ctx);

	assert(md->m_glcontext == ctx );
	assert(tnl);
	assert(swdd);

	// Use default TCL pipeline
	tnl->Driver.RunPipeline = _tnl_run_pipeline;
 
	swdd->SetBuffer = this->SetBuffer;
}


void MesaDriver::LockGL()
{
	m_bglview->LockLooper();

   UpdateState(m_glcontext, 0);
   _mesa_make_current(m_glcontext, m_glframebuffer);
}


void MesaDriver::UnlockGL()
{
	if (m_bglview->Looper()->IsLocked())
		m_bglview->UnlockLooper();
   // Could call _mesa_make_current(NULL, NULL) but it would just
   // hinder performance
}


void MesaDriver::SwapBuffers() const
{
    _mesa_notifySwapBuffers(m_glcontext);

	if (m_bitmap) {
		m_bglview->LockLooper();
		m_bglview->DrawBitmap(m_bitmap);
		m_bglview->UnlockLooper();
	};
}


void MesaDriver::CopySubBuffer(GLint x, GLint y, GLuint width, GLuint height) const
{
   if (m_bitmap) {
      // Source bitmap and view's bitmap are same size.
      // Source and dest rectangle are the same.
      // Note (x,y) = (0,0) is the lower-left corner, have to flip Y
      BRect srcAndDest;
      srcAndDest.left = x;
      srcAndDest.right = x + width - 1;
      srcAndDest.bottom = m_bottom - y;
      srcAndDest.top = srcAndDest.bottom - height + 1;
      m_bglview->DrawBitmap(m_bitmap, srcAndDest, srcAndDest);
   }
}

status_t MesaDriver::CopyPixelsOut(BPoint location, BBitmap *bitmap)
{
	color_space scs = m_bitmap->ColorSpace();
	color_space dcs = bitmap->ColorSpace();

	if (scs != dcs && (scs != B_RGBA32 || dcs != B_RGB32)) {
		printf("CopyPixelsOut(): incompatible color space: %s != %s\n",
			color_space_name(scs),
			color_space_name(dcs));
		return B_BAD_TYPE;
	}
	
	// debugger("CopyPixelsOut()");
	
	BRect sr = m_bitmap->Bounds();
	BRect dr = bitmap->Bounds();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y); 
	
	uint8 *ps = (uint8 *) m_bitmap->Bits();
	uint8 *pd = (uint8 *) bitmap->Bits();
	uint32 *s, *d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32) sr.bottom; y++) {
		s = (uint32 *) (ps + y * m_bitmap->BytesPerRow());
		s += (uint32) sr.left;
		
		d = (uint32 *) (pd + (y + (uint32) (dr.top - sr.top)) * bitmap->BytesPerRow());
		d += (uint32) dr.left;
		
		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}

status_t MesaDriver::CopyPixelsIn(BBitmap *bitmap, BPoint location)
{
	color_space scs = bitmap->ColorSpace();
	color_space dcs = m_bitmap->ColorSpace();

	if (scs != dcs && (dcs != B_RGBA32 || scs != B_RGB32)) {
		printf("CopyPixelsIn(): incompatible color space: %s != %s\n",
			color_space_name(scs),
			color_space_name(dcs));
		return B_BAD_TYPE;
	}
	
	// debugger("CopyPixelsIn()");

	BRect sr = bitmap->Bounds();
	BRect dr = m_bitmap->Bounds();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y); 
	
	uint8 *ps = (uint8 *) bitmap->Bits();
	uint8 *pd = (uint8 *) m_bitmap->Bits();
	uint32 *s, *d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32) sr.bottom; y++) {
		s = (uint32 *) (ps + y * bitmap->BytesPerRow());
		s += (uint32) sr.left;
		
		d = (uint32 *) (pd + (y + (uint32) (dr.top - sr.top)) * m_bitmap->BytesPerRow());
		d += (uint32) dr.left;
		
		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}


void MesaDriver::Draw(BRect updateRect) const
{
   if (m_bitmap)
      m_bglview->DrawBitmap(m_bitmap, updateRect, updateRect);
}


void MesaDriver::Error(GLcontext *ctx)
{
	MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
	if (md && md->m_bglview)
		md->m_bglview->ErrorCallback((unsigned long) ctx->ErrorValue);
}

void MesaDriver::UpdateState( GLcontext *ctx, GLuint new_state )
{
	struct swrast_device_driver *	swdd = _swrast_GetDeviceDriverReference( ctx );

	_swrast_InvalidateState( ctx, new_state );
	_swsetup_InvalidateState( ctx, new_state );
	_ac_InvalidateState( ctx, new_state );
	_tnl_InvalidateState( ctx, new_state );

	if (ctx->Color.DrawBuffer[0] == GL_FRONT) {
      /* read/write front buffer */
      swdd->WriteRGBASpan = MesaDriver::WriteRGBASpanFront;
      swdd->WriteRGBSpan = MesaDriver::WriteRGBSpanFront;
      swdd->WriteRGBAPixels = MesaDriver::WriteRGBAPixelsFront;
      swdd->WriteMonoRGBASpan = MesaDriver::WriteMonoRGBASpanFront;
      swdd->WriteMonoRGBAPixels = MesaDriver::WriteMonoRGBAPixelsFront;
      swdd->WriteCI32Span = MesaDriver::WriteCI32SpanFront;
      swdd->WriteCI8Span = MesaDriver::WriteCI8SpanFront;
      swdd->WriteMonoCISpan = MesaDriver::WriteMonoCISpanFront;
      swdd->WriteCI32Pixels = MesaDriver::WriteCI32PixelsFront;
      swdd->WriteMonoCIPixels = MesaDriver::WriteMonoCIPixelsFront;
      swdd->ReadRGBASpan = MesaDriver::ReadRGBASpanFront;
      swdd->ReadRGBAPixels = MesaDriver::ReadRGBAPixelsFront;
      swdd->ReadCI32Span = MesaDriver::ReadCI32SpanFront;
      swdd->ReadCI32Pixels = MesaDriver::ReadCI32PixelsFront;
   }
   else {
      /* read/write back buffer */
      swdd->WriteRGBASpan = MesaDriver::WriteRGBASpanBack;
      swdd->WriteRGBSpan = MesaDriver::WriteRGBSpanBack;
      swdd->WriteRGBAPixels = MesaDriver::WriteRGBAPixelsBack;
      swdd->WriteMonoRGBASpan = MesaDriver::WriteMonoRGBASpanBack;
      swdd->WriteMonoRGBAPixels = MesaDriver::WriteMonoRGBAPixelsBack;
      swdd->WriteCI32Span = MesaDriver::WriteCI32SpanBack;
      swdd->WriteCI8Span = MesaDriver::WriteCI8SpanBack;
      swdd->WriteMonoCISpan = MesaDriver::WriteMonoCISpanBack;
      swdd->WriteCI32Pixels = MesaDriver::WriteCI32PixelsBack;
      swdd->WriteMonoCIPixels = MesaDriver::WriteMonoCIPixelsBack;
      swdd->ReadRGBASpan = MesaDriver::ReadRGBASpanBack;
      swdd->ReadRGBAPixels = MesaDriver::ReadRGBAPixelsBack;
      swdd->ReadCI32Span = MesaDriver::ReadCI32SpanBack;
      swdd->ReadCI32Pixels = MesaDriver::ReadCI32PixelsBack;
    }
}


void MesaDriver::ClearIndex(GLcontext *ctx, GLuint index)
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   md->m_clear_index = index;
}


void MesaDriver::ClearColor(GLcontext *ctx, const GLfloat color[4])
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   CLAMPED_FLOAT_TO_CHAN(md->m_clear_color[BE_RCOMP], color[0]);
   CLAMPED_FLOAT_TO_CHAN(md->m_clear_color[BE_GCOMP], color[1]);
   CLAMPED_FLOAT_TO_CHAN(md->m_clear_color[BE_BCOMP], color[2]);
   CLAMPED_FLOAT_TO_CHAN(md->m_clear_color[BE_ACOMP], color[3]); 
   assert(md->m_bglview);
}


void MesaDriver::Clear(GLcontext *ctx, GLbitfield mask,
                               GLboolean all, GLint x, GLint y,
                               GLint width, GLint height)
{
   if (mask & DD_FRONT_LEFT_BIT)
		ClearFront(ctx, all, x, y, width, height);
   if (mask & DD_BACK_LEFT_BIT)
		ClearBack(ctx, all, x, y, width, height);

	mask &= ~(DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT);
	if (mask)
		_swrast_Clear( ctx, mask, all, x, y, width, height );

   return;
}


void MesaDriver::ClearFront(GLcontext *ctx,
                         GLboolean all, GLint x, GLint y,
                         GLint width, GLint height)
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);

   bglview->SetHighColor(md->m_clear_color[BE_RCOMP],
                         md->m_clear_color[BE_GCOMP],
                         md->m_clear_color[BE_BCOMP],
                         md->m_clear_color[BE_ACOMP]);
   bglview->SetLowColor(md->m_clear_color[BE_RCOMP],
                        md->m_clear_color[BE_GCOMP],
                        md->m_clear_color[BE_BCOMP],
                        md->m_clear_color[BE_ACOMP]);
   if (all) {
      BRect b = bglview->Bounds();
      bglview->FillRect(b);
   }
   else {
      // XXX untested
      BRect b;
      b.left = x;
      b.right = x + width;
      b.bottom = md->m_height - y - 1;
      b.top = b.bottom - height;
      bglview->FillRect(b);
   }

   // restore drawing color
#if 0
   bglview->SetHighColor(md->mColor[BE_RCOMP],
                         md->mColor[BE_GCOMP],
                         md->mColor[BE_BCOMP],
                         md->mColor[BE_ACOMP]);
   bglview->SetLowColor(md->mColor[BE_RCOMP],
                        md->mColor[BE_GCOMP],
                        md->mColor[BE_BCOMP],
                        md->mColor[BE_ACOMP]);
#endif
}


void MesaDriver::ClearBack(GLcontext *ctx,
                        GLboolean all, GLint x, GLint y,
                        GLint width, GLint height)
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   BBitmap *bitmap = md->m_bitmap;
   assert(bitmap);
   GLuint *start = (GLuint *) bitmap->Bits();
   const GLuint *clearPixelPtr = (const GLuint *) md->m_clear_color;
   const GLuint clearPixel = B_LENDIAN_TO_HOST_INT32(*clearPixelPtr);

   if (all) {
      const int numPixels = md->m_width * md->m_height;
      if (clearPixel == 0) {
         memset(start, 0, numPixels * 4);
      }
      else {
         for (int i = 0; i < numPixels; i++) {
             start[i] = clearPixel;
         }
      }
   }
   else {
      // XXX untested
      start += y * md->m_width + x;
      for (int i = 0; i < height; i++) {
         for (int j = 0; j < width; j++) {
            start[j] = clearPixel;
         }
         start += md->m_width;
      }
   }
}


void MesaDriver::SetBuffer(GLcontext *ctx, GLframebuffer *buffer,
                            GLenum mode)
{
   /* TODO */
	(void) ctx;
	(void) buffer;
	(void) mode;
}

void MesaDriver::GetBufferSize(GLframebuffer * framebuffer, GLuint *width,
                            GLuint *height)
{
   GET_CURRENT_CONTEXT(ctx);
   if (!ctx)
		return;

   MesaDriver * md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);

   BRect b = bglview->Bounds();
   *width = (GLuint) b.IntegerWidth() + 1; // (b.right - b.left + 1);
   *height = (GLuint) b.IntegerHeight() + 1; // (b.bottom - b.top + 1);
   md->m_bottom = (GLint) b.bottom;

   if (ctx->Visual.doubleBufferMode) {
      if (*width != md->m_width || *height != md->m_height) {
         // allocate new size of back buffer bitmap
         if (md->m_bitmap)
            delete md->m_bitmap;
         BRect rect(0.0, 0.0, *width - 1, *height - 1);
         md->m_bitmap = new BBitmap(rect, B_RGBA32);
      }
   }
   else
   {
      md->m_bitmap = NULL;
   }

   md->m_width = *width;
   md->m_height = *height;
}


void MesaDriver::Viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   /* poll for window size change and realloc software Z/stencil/etc if needed */
   _mesa_ResizeBuffersMESA();
}


const GLubyte *MesaDriver::GetString(GLcontext *ctx, GLenum name)
{
   switch (name) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa " MESA_VERSION_STRING " powered BGLView (software)";
      default:
         // Let core library handle all other cases
         return NULL;
   }
}


// Plot a pixel.  (0,0) is upper-left corner
// This is only used when drawing to the front buffer.
inline void Plot(BGLView *bglview, int x, int y)
{
   // XXX There's got to be a better way!
   BPoint p(x, y), q(x+1, y);
   bglview->StrokeLine(p, q);
}


void MesaDriver::WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   int flippedY = md->m_bottom - y;
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
            Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
         Plot(bglview, x++, flippedY);
      }
   }
}

void MesaDriver::WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgba[][3],
                                const GLubyte mask[])
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   int flippedY = md->m_bottom - y;
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
            Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
         Plot(bglview, x++, flippedY);
      }
   }
}

void MesaDriver::WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[])
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   int flippedY = md->m_bottom - y;
   bglview->SetHighColor(color[RCOMP], color[GCOMP], color[BCOMP]);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         Plot(bglview, x++, flippedY);
      }
   }
}

void MesaDriver::WriteRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
            Plot(bglview, x[i], md->m_bottom - y[i]);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
         Plot(bglview, x[i], md->m_bottom - y[i]);
      }
   }
}


void MesaDriver::WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[])
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   // plot points using current color
   bglview->SetHighColor(color[RCOMP], color[GCOMP], color[BCOMP]);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            Plot(bglview, x[i], md->m_bottom - y[i]);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         Plot(bglview, x[i], md->m_bottom - y[i]);
      }
   }
}


void MesaDriver::WriteCI32SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLuint index[], const GLubyte mask[] )
{
 	printf("WriteCI32SpanFront() not implemented yet!\n");
   // TODO
}

void MesaDriver::WriteCI8SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLubyte index[], const GLubyte mask[] )
{
 	printf("WriteCI8SpanFront() not implemented yet!\n");
   // TODO
}

void MesaDriver::WriteMonoCISpanFront( const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[] )
{
 	printf("WriteMonoCISpanFront() not implemented yet!\n");
   // TODO
}


void MesaDriver::WriteCI32PixelsFront( const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[] )
{
 	printf("WriteCI32PixelsFront() not implemented yet!\n");
   // TODO
}

void MesaDriver::WriteMonoCIPixelsFront( const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[] )
{
 	printf("WriteMonoCIPixelsFront() not implemented yet!\n");
   // TODO
}


void MesaDriver::ReadCI32SpanFront( const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[] )
{
 	printf("ReadCI32SpanFront() not implemented yet!\n");
  // TODO
}


void MesaDriver::ReadRGBASpanFront( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y, GLubyte rgba[][4] )
{
 	printf("ReadRGBASpanFront() not implemented yet!\n");
   // TODO
}


void MesaDriver::ReadCI32PixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
 	printf("ReadCI32PixelsFront() not implemented yet!\n");
   // TODO
}


void MesaDriver::ReadRGBAPixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[] )
{
 	printf("ReadRGBAPixelsFront() not implemented yet!\n");
   // TODO
}




void MesaDriver::WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
	MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
	BBitmap *bitmap = md->m_bitmap;

	assert(bitmap);

	int row = md->m_bottom - y;
	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = PACK_B_RGBA32(rgba[0]);
			pixel++;
			rgba++;
		};
	} else {
		while(n--) {
			*pixel++ = PACK_B_RGBA32(rgba[0]);
			rgba++;
		};
	};
 }


void MesaDriver::WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgb[][3],
                                const GLubyte mask[])
{
	MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
	BBitmap *bitmap = md->m_bitmap;

	assert(bitmap);

	int row = md->m_bottom - y;
	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = PACK_B_RGB32(rgb[0]);
			pixel++;
			rgb++;
		};
	} else {
		while(n--) {
			*pixel++ = PACK_B_RGB32(rgb[0]);
			rgb++;
		};
	};
}




void MesaDriver::WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    const GLchan color[4], const GLubyte mask[])
{
	MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
	BBitmap *bitmap = md->m_bitmap;

	assert(bitmap);

	int row = md->m_bottom - y;
	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	uint32 pixel_color = PACK_B_RGBA32(color);
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = pixel_color;
			pixel++;
		};
	} else {
		while(n--) {
			*pixel++ = pixel_color;
		};
	};
}


void MesaDriver::WriteRGBAPixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BBitmap *bitmap = md->m_bitmap;

	assert(bitmap);
#if 0
	while(n--) {
		if (*mask++) {
			int row = md->m_bottom - *y;
			uint8 * pixel = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + *x * 4;
			*((uint32 *) pixel) = PACK_B_RGBA32(rgba[0]);
		};
		x++;
		y++;
		rgba++;
	};
#else
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
            pixel[BE_RCOMP] = rgba[i][RCOMP];
            pixel[BE_GCOMP] = rgba[i][GCOMP];
            pixel[BE_BCOMP] = rgba[i][BCOMP];
            pixel[BE_ACOMP] = rgba[i][ACOMP];
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
         pixel[BE_RCOMP] = rgba[i][RCOMP];
         pixel[BE_GCOMP] = rgba[i][GCOMP];
         pixel[BE_BCOMP] = rgba[i][BCOMP];
         pixel[BE_ACOMP] = rgba[i][ACOMP];
      }
   }
#endif
}


void MesaDriver::WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      const GLchan color[4],
                                      const GLubyte mask[])
{
	MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
	BBitmap *bitmap = md->m_bitmap;

	assert(bitmap);

	uint32 pixel_color = PACK_B_RGBA32(color);
#if 0	
	while(n--) {
		if (*mask++) {
			int row = md->m_bottom - *y;
			uint8 * pixel = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + *x * 4;

			*((uint32 *) pixel) = pixel_color;
		};
		x++;
		y++;
	};
#else
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
         	GLubyte * ptr = (GLubyte *) bitmap->Bits()
            	+ ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
            *((uint32 *) ptr) = pixel_color;
         }
      }
   }
   else {
	  for (GLuint i = 0; i < n; i++) {
       	GLubyte * ptr = (GLubyte *) bitmap->Bits()
	           	+ ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
       *((uint32 *) ptr) = pixel_color;
      }
   }
#endif
}


void MesaDriver::WriteCI32SpanBack( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[] )
{
 	printf("WriteCI32SpanBack() not implemented yet!\n");
   // TODO
}

void MesaDriver::WriteCI8SpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[] )
{
  	printf("WriteCI8SpanBack() not implemented yet!\n");
  // TODO
}

void MesaDriver::WriteMonoCISpanBack( const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y,
                                   GLuint colorIndex, const GLubyte mask[] )
{
 	printf("WriteMonoCISpanBack() not implemented yet!\n");
   // TODO
}


void MesaDriver::WriteCI32PixelsBack( const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[] )
{
 	printf("WriteCI32PixelsBack() not implemented yet!\n");
   // TODO
}

void MesaDriver::WriteMonoCIPixelsBack( const GLcontext *ctx, GLuint n,
                                     const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[] )
{
 	printf("WriteMonoCIPixelsBack() not implemented yet!\n");
   // TODO
}


void MesaDriver::ReadCI32SpanBack( const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[] )
{
 	printf("ReadCI32SpanBack() not implemented yet!\n");
   // TODO
}


void MesaDriver::ReadRGBASpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y, GLubyte rgba[][4] )
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   const BBitmap *bitmap = md->m_bitmap;
   assert(bitmap);
   int row = md->m_bottom - y;
   const GLubyte *pixel = (GLubyte *) bitmap->Bits()
                        + (row * bitmap->BytesPerRow()) + x * 4;

   for (GLuint i = 0; i < n; i++) {
      rgba[i][RCOMP] = pixel[BE_RCOMP];
      rgba[i][GCOMP] = pixel[BE_GCOMP];
      rgba[i][BCOMP] = pixel[BE_BCOMP];
      rgba[i][ACOMP] = pixel[BE_ACOMP];
      pixel += 4;
   }
}


void MesaDriver::ReadCI32PixelsBack( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
 	printf("ReadCI32PixelsBack() not implemented yet!\n");
   // TODO
}


void MesaDriver::ReadRGBAPixelsBack( const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[] )
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   const BBitmap *bitmap = md->m_bitmap;
   assert(bitmap);

   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
	         rgba[i][RCOMP] = pixel[BE_RCOMP];
    	     rgba[i][GCOMP] = pixel[BE_GCOMP];
        	 rgba[i][BCOMP] = pixel[BE_BCOMP];
        	 rgba[i][ACOMP] = pixel[BE_ACOMP];
         };
      };
   } else {
      for (GLuint i = 0; i < n; i++) {
         GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
         rgba[i][RCOMP] = pixel[BE_RCOMP];
         rgba[i][GCOMP] = pixel[BE_GCOMP];
         rgba[i][BCOMP] = pixel[BE_BCOMP];
         rgba[i][ACOMP] = pixel[BE_ACOMP];
      };
   };
}

const char * color_space_name(color_space space)
{
#define C2N(a)	case a:	return #a

	switch (space) {
	C2N(B_RGB24);
	C2N(B_RGB32);
	C2N(B_RGBA32);
	C2N(B_RGB32_BIG);
	C2N(B_RGBA32_BIG);
	C2N(B_GRAY8);
	C2N(B_GRAY1);
	C2N(B_RGB16);
	C2N(B_RGB15);
	C2N(B_RGBA15);
	C2N(B_CMAP8);
	default:
		return "Unknown!";
	};

#undef C2N
};


