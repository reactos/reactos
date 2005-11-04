/*
 * Copyright 2002 by Alan Hourihane, Sychdyn, North Wales, UK.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * Trident CyberBladeXP driver.
 *
 */

#include "trident_context.h"
#include "trident_lock.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"

static int first = 1;

typedef struct reg {
	int addr;
	int data;
} RegData;

RegData initRegData[]={
    {0x2804,  0x19980824},
    {0x2F70,  0x46455858},
    {0x2F74,    0x41584998},
    {0x2F00,    0x00000000},
    {0x2F04,    0x80000800},
    {0x2F08,    0x00550200},
    {0x2F40,    0x00000001},
    {0x2F40,    0x00000001},
    {0x2F44,    0x00830097},
    {0x2F48,    0x0087009F},
    {0x2F4C,    0x00BF0003},
    {0x2F50,    0xF00B6C1B},
    {0x2C04,    0x00000000},
    {0x2D00,    0x00000080},
    {0x2D00,    0x00000000},
    {0x2DD4,    0x00100000},
    {0x2DD4,    0x00100010},
    {0x2DD8,    0x00100000},
    {0x2DD8,    0x00100010},
    {0x2C88,    0xFFFFFFFF},
    {0x2C94 ,   0xFFFFFFFF},
    {0x281C,    0x00008000},
    {0x2C80,    0x00000000},
    {0x2C80,    0x00000000},
    {0x2C80 ,   0x00008000},
    {0x2C00  ,  0x00000000},
    {0x2C04  ,  0x00000000},
    {0x2C08  ,  0x00000000},
    {0x2C0C  ,  0x00000000},
    {0x2C10  ,  0x00000000},
    {0x2C14  ,  0x00000000},
    {0x2C18  ,  0x00000000},
    {0x2C1C  ,  0x00000000},
    {0x2C20  ,  0x00000000},
    {0x2C24  ,  0x00000000},
    {0x2C2C  ,  0x00000000},
    {0x2C30  ,  0x00000000},
    {0x2C34  ,  0x00000000},
    {0x2C38  ,  0x00000000},
    {0x2C3C  ,  0x00000000},
    {0x2C40  ,  0x00000000},
    {0x2C44  ,  0x00000000},
    {0x2C48  ,  0x00000000},
    {0x2C4C  ,  0x00000000},
    {0x2C50  ,  0x00000000},
    {0x2C54  ,  0x00000000},
    {0x2C58  ,  0x00000000},
    {0x2C5C  ,  0x00000000},
    {0x2C60  ,  0x00000000},
    {0x2C64  ,  0x00000000},
    {0x2C68  ,  0x00000000},
    {0x2C6C  ,  0x00000000},
    {0x2C70  ,  0x00000000},
    {0x2C74  ,  0x00000000},
    {0x2C78  ,  0x00000000},
    {0x2C7C  ,  0x00000000},
    {0x2C80  ,  0x00008000},
    {0x2C84  ,  0x00000000},
    {0x2C88  ,  0xFFFFFFFF},
    {0x2C8C  ,  0x00000000},
    {0x2C90  ,  0x00000000},
    {0x2C94  ,  0xFFFFFFFF},
    {0x2C98  ,  0x00000000},
    {0x2C9C  ,  0x00000000},
    {0x2CA0  ,  0x00000000},
    {0x2CA4   , 0x00000000},
    {0x2CA8   , 0x00000000},
    {0x2CAC  ,  0x00000000},
    {0x2CB0  ,  0x00000000},
    {0x2CB4  ,  0x00000000},
    {0x2CB8  ,  0x00000000},
    {0x2CBC  ,  0x00000000},
    {0x2CC0  ,  0x00000000},
    {0x2CC4  ,  0x00000000},
    {0x2CC8  ,  0x00000000},
    {0x2CCC  ,  0x00000000},
    {0x2CD0  ,  0x00000000},
    {0x2CD4  ,  0x00000000},
    {0x2CD8  ,  0x00000000},
    {0x2CDC  ,  0x00000000},
    {0x2CE0  ,  0x00000000},
    {0x2CE4  ,  0x00000000},
    {0x2CE8  ,  0x00000000},
    {0x2CEC  ,  0x00000000},
    {0x2CF0  ,  0x00000000},
    {0x2CF4  ,  0x00000000},
    {0x2CF8  ,  0x00000000},
    {0x2CFC  ,  0x00000000},
    {0x2D00  ,  0x00000000},
    {0x2D04  ,  0x00000000},
    {0x2D08  ,  0x00000000},
    {0x2D0C  ,  0x00000000},
    {0x2D10  ,  0x00000000},
    {0x2D14  ,  0x00000000},
    {0x2D18  ,  0x00000000},
    {0x2D1C  ,  0x00000000},
    {0x2D20  ,  0x00000000},
    {0x2D24  ,  0x00000000},
    {0x2D28  ,  0x00000000},
    {0x2D2C  ,  0x00000000},
    {0x2D30  ,  0x00000000},
    {0x2D34  ,  0x00000000},
    {0x2D38   , 0x00000000},
    {0x2D3C   , 0x00000000},
    {0x2D40   , 0x00000000},
    {0x2D44   , 0x00000000},
    {0x2D48   , 0x00000000},
    {0x2D4C   , 0x00000000},
    {0x2D50   , 0x00000000},
    {0x2D54  ,  0x00000000},
    {0x2D58  ,  0x00000000},
    {0x2D5C  ,  0x00000000},
    {0x2D60  ,  0x00000000},
    {0x2D64  ,  0x00000000},
    {0x2D68  ,  0x00000000},
    {0x2D6C  ,  0x00000000},
    {0x2D70   , 0x00000000},
    {0x2D74   , 0x00000000},
    {0x2D78   , 0x00000000},
    {0x2D7C   , 0x00000000},
    {0x2D80   , 0x00000000},
    {0x2D84   , 0x00000000},
    {0x2D88   , 0x00000000},
    {0x2D8C   , 0x00000000},
    {0x2D90   , 0x00000000},
    {0x2D94   , 0x00000000},
    {0x2D98   , 0x00000000},
    {0x2D9C   , 0x00000000},
    {0x2DA0   , 0x00000000},
    {0x2DA4   , 0x00000000},
    {0x2DA8   , 0x00000000},
    {0x2DAC   , 0x00000000},
    {0x2DB0   , 0x00000000},
    {0x2DB4   , 0x00000000},
    {0x2DB8   , 0x00000000},
    {0x2DBC   , 0x00000000},
    {0x2DC0   , 0x00000000},
    {0x2DC4   , 0x00000000},
    {0x2DC8   , 0x00000000},
    {0x2DCC   , 0x00000000},
    {0x2DD0   , 0x00000000},
    {0x2DD4   , 0x00100010},
    {0x2DD8   , 0x00100010},
    {0x2DDC   , 0x00000000},
    {0x2DE0   , 0x00000000},
    {0x2DE4   , 0x00000000},
    {0x2DE8   , 0x00000000},
    {0x2DEC   , 0x00000000},
    {0x2DF0   , 0x00000000},
    {0x2DF4   , 0x00000000},
    {0x2DF8   , 0x00000000},
    {0x2DFC   , 0x00000000},
    {0x2E00   , 0x00000000},
    {0x2E04   , 0x00000000},
    {0x2E08   , 0x00000000},
    {0x2E0C   , 0x00000000},
    {0x2E10   , 0x00000000},
    {0x2E14   , 0x00000000},
    {0x2E18   , 0x00000000},
    {0x2E1C   , 0x00000000},
    {0x2E20   , 0x00000000},
    {0x2E24   , 0x00000000},
    {0x2E28   , 0x00000000},
    {0x2E2C   , 0x00000000},
    {0x2E30   , 0x00000000},
    {0x2E34   , 0x00000000},
    {0x2E38   , 0x00000000},
    {0x2E3C   , 0x00000000},
    {0x2E40   , 0x00000000},
    {0x2E44   , 0x00000000},
    {0x2E48   , 0x00000000},
    {0x2E4C   , 0x00000000},
    {0x2E50   , 0x00000000},
    {0x2E54   , 0x00000000},
    {0x2E58   , 0x00000000},
    {0x2E5C   , 0x00000000},
    {0x2E60   , 0x00000000},
    {0x2E64   , 0x00000000},
    {0x2E68   , 0x00000000},
    {0x2E6C   , 0x00000000},
    {0x2E70   , 0x00000000},
    {0x2E74   , 0x00000000},
    {0x2E78   , 0x00000000},
    {0x2E7C   , 0x00000000},
    {0x2E80   , 0x00000000},
    {0x2E84   , 0x00000000},
    {0x2E88   , 0x00000000},
    {0x2E8C   , 0x00000000},
    {0x2E90   , 0x00000000},
    {0x2E94   , 0x00000000},
    {0x2E98   , 0x00000000},
    {0x2E9C   , 0x00000000},
    {0x2EA0   , 0x00000000},
    {0x2EA4   , 0x00000000},
    {0x2EA8   , 0x00000000},
    {0x2EAC   , 0x00000000},
    {0x2EB0   , 0x00000000},
    {0x2EB4   , 0x00000000},
    {0x2EB8   , 0x00000000},
    {0x2EBC   , 0x00000000},
    {0x2EC0   , 0x00000000},
    {0x2EC4   , 0x00000000},
    {0x2EC8   , 0x00000000},
    {0x2ECC   , 0x00000000},
    {0x2ED0   , 0x00000000},
    {0x2ED4   , 0x00000000},
    {0x2ED8   , 0x00000000},
    {0x2EDC   , 0x00000000},
    {0x2EE0   , 0x00000000},
    {0x2EE4    ,0x00000000},
    {0x2EE8    ,0x00000000},
    {0x2EEC   , 0x00000000},
    {0x2EF0   , 0x00000000},
    {0x2EF4   , 0x00000000},
    {0x2EF8   , 0x00000000},
    {0x2EFC   , 0x00000000},
    /*{0x2F60   , 0x00000000},*/
};

int initRegDataNum=sizeof(initRegData)/sizeof(RegData);

typedef union {
    unsigned int i;
    float        f;
} dmaBufRec, *dmaBuf;

void Init3D( tridentContextPtr tmesa )
{
   unsigned char *MMIO = tmesa->tridentScreen->mmio.map;
    int i;

    for(i=0;i<initRegDataNum;++i)
       MMIO_OUT32(MMIO, initRegData[i].addr, initRegData[i].data);
}

int DrawTriangle( tridentContextPtr tmesa)
{
   unsigned char *MMIO = tmesa->tridentScreen->mmio.map;
   dmaBufRec clr;

printf("DRAW TRI\n");
	Init3D(tmesa);

printf("ENGINE STATUS 0x%x\n",MMIO_IN32(MMIO, 0x2800));
    MMIO_OUT32(MMIO,  0x002800, 0x00000000 );
#if 0
    MMIO_OUT32(MMIO,  0x002368 , MMIO_IN32(MMIO,0x002368)|1 );
#endif

    MMIO_OUT32(MMIO, 0x002C00 , 0x00000014 );
#if 0
    MMIO_OUT32(MMIO, 0x002C04 , 0x0A8004C0 );
#else
    MMIO_OUT32(MMIO, 0x002C04 , 0x0A8000C0 );
#endif

#if 0
    MMIO_OUT32(MMIO, 0x002C08 , 0x00000000 );
    MMIO_OUT32(MMIO, 0x002C0C , 0xFFCCCCCC );
    MMIO_OUT32(MMIO, 0x002C10 , 0x3F800000 );
    MMIO_OUT32(MMIO, 0x002C14 , 0x3D0D3DCB );
    MMIO_OUT32(MMIO, 0x002C2C , 0x70000000 );
    MMIO_OUT32(MMIO, 0x002C24 , 0x00202C00 );
    MMIO_OUT32(MMIO, 0x002C28 , 0xE0002500 );
    MMIO_OUT32(MMIO, 0x002C30 , 0x00000000 );
    MMIO_OUT32(MMIO, 0x002C34 , 0xE0000000 );
    MMIO_OUT32(MMIO, 0x002C38 , 0x00000000 );
#endif

    MMIO_OUT32(MMIO, 0x002C50 , 0x00000000 );
    MMIO_OUT32(MMIO, 0x002C54 , 0x0C320C80 );    
    MMIO_OUT32(MMIO, 0x002C50 , 0x00000000 );
    MMIO_OUT32(MMIO, 0x002C54 , 0x0C320C80 );    
    MMIO_OUT32(MMIO, 0x002C80 , 0x20008258 );    
    MMIO_OUT32(MMIO, 0x002C84 , 0x20000320 );    
    MMIO_OUT32(MMIO, 0x002C94 , 0xFFFFFFFF );

#if 0
    MMIO_OUT32(MMIO, 0x002D00 , 0x00009009 );    
    MMIO_OUT32(MMIO, 0x002D38 , 0x00000000 );
    MMIO_OUT32(MMIO, 0x002D94 , 0x20002000 );
    MMIO_OUT32(MMIO, 0x002D50 , 0xf0000000 );
    MMIO_OUT32(MMIO, 0x002D80 , 0x24002000 );        
    MMIO_OUT32(MMIO, 0x002D98 , 0x81000000 );        
    MMIO_OUT32(MMIO, 0x002DB0 , 0x81000000 );        
    MMIO_OUT32(MMIO, 0x002DC8 , 0x808000FF );
    MMIO_OUT32(MMIO, 0x002DD4 , 0x02000200 );
    MMIO_OUT32(MMIO, 0x002DD8 , 0x02000200 );
    MMIO_OUT32(MMIO, 0x002D30 , 0x02092400 );    
    MMIO_OUT32(MMIO, 0x002D04 , 0x00102120 );    
    MMIO_OUT32(MMIO, 0x002D08 , 0xFFFFFFFF );
    MMIO_OUT32(MMIO, 0x002D0C , 0xF00010D0 );    
    MMIO_OUT32(MMIO, 0x002D10 , 0xC0000400 );
#endif

    MMIO_OUT32(MMIO, 0x002814,  0x00000000 );
#if 0
    MMIO_OUT32(MMIO, 0x002818 , 0x00036C20 );        
#else
    MMIO_OUT32(MMIO, 0x002818 , 0x00036020 );        
#endif
    MMIO_OUT32(MMIO, 0x00281C , 0x00098081 );	

printf("first TRI\n");
    clr.f = 5.0;
    MMIO_OUT32(MMIO, 0x002820 , clr.i );               
    clr.f = 595.0;
    MMIO_OUT32(MMIO, 0x002824 , clr.i );               
    clr.f = 1.0;
    MMIO_OUT32(MMIO, 0x002828 , clr.i );
    MMIO_OUT32(MMIO, 0x00282C , 0x00FF00 );        
#if 0
    clr.f = 0.0;
    MMIO_OUT32(MMIO, 0x002830 , clr.i );
    clr.f = 1.0;
    MMIO_OUT32(MMIO, 0x002834 , clr.i );
#endif

    clr.f = 5.0;
    MMIO_OUT32(MMIO, 0x002820 , clr.i );               
    clr.f = 5.0;
    MMIO_OUT32(MMIO, 0x002824 , clr.i );               
    clr.f = 1.0;
    MMIO_OUT32(MMIO, 0x002828 , clr.i );
    MMIO_OUT32(MMIO, 0x00282C , 0xFF0000 );        
#if 0
    clr.f = 0.0;
    MMIO_OUT32(MMIO, 0x002830 , clr.i );
    clr.f = 0.0;
    MMIO_OUT32(MMIO, 0x002834 , clr.i );
#endif

    clr.f = 395.0;
printf("0x%x\n",clr.i);
    MMIO_OUT32(MMIO, 0x002820 , clr.i );               
    clr.f = 5.0;
    MMIO_OUT32(MMIO, 0x002824 , clr.i );               
    clr.f = 1.0;
    MMIO_OUT32(MMIO, 0x002828 , clr.i );
    MMIO_OUT32(MMIO, 0x00282C , 0xFF );        
#if 0
    clr.f = 1.0;
    MMIO_OUT32(MMIO, 0x002830 , clr.i );
    clr.f = 0.0;
    MMIO_OUT32(MMIO, 0x002834 , clr.i );
#endif

printf("sec TRI\n");
    MMIO_OUT32(MMIO, 0x00281C , 0x00093980 );    
    clr.f = 395.0;
    MMIO_OUT32(MMIO, 0x002820 , clr.i );               
    clr.f = 595.0;
    MMIO_OUT32(MMIO, 0x002824 , clr.i );               
    clr.f = 1.0;
    MMIO_OUT32(MMIO, 0x002828 , clr.i );               
    MMIO_OUT32(MMIO, 0x00282C , 0x00FF00 );        
#if 0
    clr.f = 1.0;
    MMIO_OUT32(MMIO, 0x002830 , clr.i );
    clr.f = 1.0;
    MMIO_OUT32(MMIO, 0x002834 , clr.i );
#endif

#if 0
    MMIO_OUT32(MMIO,  0x002368 , MMIO_IN32(MMIO,0x002368)&0xfffffffe );
#endif

printf("fin TRI\n");

    return 0;  
}

static INLINE void trident_draw_point(tridentContextPtr tmesa, 
			     const tridentVertex *v0 )
{
   unsigned char *MMIO = tmesa->tridentScreen->mmio.map;
   (void) MMIO;
}

static INLINE void trident_draw_line( tridentContextPtr tmesa, 
			     const tridentVertex *v0,
			     const tridentVertex *v1 )
{
   unsigned char *MMIO = tmesa->tridentScreen->mmio.map;
   (void) MMIO;
}

static INLINE void trident_draw_triangle( tridentContextPtr tmesa,
				 const tridentVertex *v0,
				 const tridentVertex *v1, 
				 const tridentVertex *v2 )
{
}

static INLINE void trident_draw_quad( tridentContextPtr tmesa,
			    const tridentVertex *v0,
			    const tridentVertex *v1,
			    const tridentVertex *v2,
			    const tridentVertex *v3 )
{
   GLuint vertsize = tmesa->vertex_size;
   GLint coloridx = (vertsize > 4) ? 4 : 3;
   unsigned char *MMIO = tmesa->tridentScreen->mmio.map;
   int clr;
   float *ftmp = (float *)(&clr);

   if (tmesa->dirty)
	tridentUploadHwStateLocked( tmesa );
#if 0
	DrawTriangle(tmesa);
	exit(0);
#else
#if 1
	if (first) {
	Init3D(tmesa);
#if 0
	DrawTriangle(tmesa);
#endif
	first = 0;
	}
#endif
    
    LOCK_HARDWARE( tmesa );

    MMIO_OUT32(MMIO, 0x002C00 , 0x00000010 );
    MMIO_OUT32(MMIO, 0x002C04 , 0x029C00C0 );

    /* Z buffer */
    MMIO_OUT32(MMIO, 0x002C24 , 0x00100000 /*| (tmesa->tridentScreen->depthOffset)*/ );
    MMIO_OUT32(MMIO, 0x002C28 , 0xE0000000 | (tmesa->tridentScreen->depthPitch * 4) );

    /* front buffer */
    MMIO_OUT32(MMIO, 0x002C50 , 0x00000000 | (tmesa->drawOffset) );
    MMIO_OUT32(MMIO, 0x002C54 , 0x0C320000 | (tmesa->drawPitch * 4) );    

    /* clipper */
    MMIO_OUT32(MMIO, 0x002C80 , 0x20008000 | tmesa->tridentScreen->height );    
    MMIO_OUT32(MMIO, 0x002C84 , 0x20000000 | tmesa->tridentScreen->width );    

    /* writemask */
    MMIO_OUT32(MMIO, 0x002C94 , 0xFFFFFFFF );

if (vertsize == 4) {
    MMIO_OUT32(MMIO, 0x002818 , 0x0003A020 );        
    MMIO_OUT32(MMIO, 0x00281C , 0x00098021 );	

    *ftmp = v0->v.x;
    MMIO_OUT32(MMIO, 0x002820 , clr );               
    *ftmp = v0->v.y;
    MMIO_OUT32(MMIO, 0x002824 , clr );               
    *ftmp = v0->v.z;
    MMIO_OUT32(MMIO, 0x002828 , clr );
#if 0
    *ftmp = v0->v.w;
    MMIO_OUT32(MMIO, 0x00282C , clr );
#endif
    MMIO_OUT32(MMIO, 0x00282C , v0->ui[coloridx] );        

    *ftmp = v1->v.x;
    MMIO_OUT32(MMIO, 0x002820 , clr );               
    *ftmp = v1->v.y;
    MMIO_OUT32(MMIO, 0x002824 , clr );               
    *ftmp = v1->v.z;
    MMIO_OUT32(MMIO, 0x002828 , clr );
#if 0
    *ftmp = v1->v.w;
    MMIO_OUT32(MMIO, 0x00282C , clr );
#endif
    MMIO_OUT32(MMIO, 0x00282C , v1->ui[coloridx] );        

    *ftmp = v2->v.x;
    MMIO_OUT32(MMIO, 0x002820 , clr );               
    *ftmp = v2->v.y;
    MMIO_OUT32(MMIO, 0x002824 , clr );               
    *ftmp = v2->v.z;
    MMIO_OUT32(MMIO, 0x002828 , clr );
#if 0
    *ftmp = v2->v.w;
    MMIO_OUT32(MMIO, 0x00282C , clr );
#endif
    MMIO_OUT32(MMIO, 0x00282C , v2->ui[coloridx] );        

    MMIO_OUT32(MMIO, 0x00281C , 0x00093020 );    
    *ftmp = v3->v.x;
    MMIO_OUT32(MMIO, 0x002820 , clr );               
    *ftmp = v3->v.y;
    MMIO_OUT32(MMIO, 0x002824 , clr );               
    *ftmp = v3->v.z;
    MMIO_OUT32(MMIO, 0x002828 , clr );
#if 0
    *ftmp = v3->v.w;
    MMIO_OUT32(MMIO, 0x00282C , clr );
#endif
    MMIO_OUT32(MMIO, 0x00282C , v3->ui[coloridx] );        

}
#endif

    UNLOCK_HARDWARE( tmesa );
}
/***********************************************************************
 *                    Rasterization fallback helpers                   *
 ***********************************************************************/


/* This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.  
 */
#if 0
static void 
trident_fallback_quad( tridentContextPtr tmesa, 
		    const tridentVertex *v0, 
		    const tridentVertex *v1, 
		    const tridentVertex *v2, 
		    const tridentVertex *v3 )
{
   GLcontext *ctx = tmesa->glCtx;
   SWvertex v[4];
   trident_translate_vertex( ctx, v0, &v[0] );
   trident_translate_vertex( ctx, v1, &v[1] );
   trident_translate_vertex( ctx, v2, &v[2] );
   trident_translate_vertex( ctx, v3, &v[3] );
   _swrast_Quad( ctx, &v[0], &v[1], &v[2], &v[3] );
}
#endif

/* XXX hack to get the prototype defined in time... */
void trident_translate_vertex(GLcontext *ctx, const tridentVertex *src,
                              SWvertex *dst);

static void 
trident_fallback_tri( tridentContextPtr tmesa, 
		    const tridentVertex *v0, 
		    const tridentVertex *v1, 
		    const tridentVertex *v2 )
{
   GLcontext *ctx = tmesa->glCtx;
   SWvertex v[3];
   trident_translate_vertex( ctx, v0, &v[0] );
   trident_translate_vertex( ctx, v1, &v[1] );
   trident_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}

static void 
trident_fallback_line( tridentContextPtr tmesa,
		     const tridentVertex *v0,
		     const tridentVertex *v1 )
{
   GLcontext *ctx = tmesa->glCtx;
   SWvertex v[2];
   trident_translate_vertex( ctx, v0, &v[0] );
   trident_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void 
trident_fallback_point( tridentContextPtr tmesa, 
		      const tridentVertex *v0 )
{
   GLcontext *ctx = tmesa->glCtx;
   SWvertex v[1];
   trident_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}

/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do {						\
   if (DO_FALLBACK)				\
      tmesa->draw_tri( tmesa, a, b, c );	\
   else						\
      trident_draw_triangle( tmesa, a, b, c );	\
} while (0)

#define QUAD( a, b, c, d )			\
do {						\
   if (DO_FALLBACK) {				\
      tmesa->draw_tri( tmesa, a, b, d );	\
      tmesa->draw_tri( tmesa, b, c, d );	\
   } else 					\
      trident_draw_quad( tmesa, a, b, c, d );	\
} while (0)

#define LINE( v0, v1 )				\
do {						\
   if (DO_FALLBACK)				\
      tmesa->draw_line( tmesa, v0, v1 );	\
   else 					\
      trident_draw_line( tmesa, v0, v1 );	\
} while (0)

#define POINT( v0 )				\
do {						\
   if (DO_FALLBACK)				\
      tmesa->draw_point( tmesa, v0 );		\
   else 					\
      trident_draw_point( tmesa, v0 );		\
} while (0)

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define TRIDENT_OFFSET_BIT 	0x01
#define TRIDENT_TWOSIDE_BIT	0x02
#define TRIDENT_UNFILLED_BIT	0x04
#define TRIDENT_FALLBACK_BIT	0x08
#define TRIDENT_MAX_TRIFUNC	0x10


static struct {
   tnl_points_func	points;
   tnl_line_func	line;
   tnl_triangle_func	triangle;
   tnl_quad_func	quad;
} rast_tab[TRIDENT_MAX_TRIFUNC];


#define DO_FALLBACK (IND & TRIDENT_FALLBACK_BIT)
#define DO_OFFSET   (IND & TRIDENT_OFFSET_BIT)
#define DO_UNFILLED (IND & TRIDENT_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & TRIDENT_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA         1
#define HAVE_SPEC         1
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define VERTEX            tridentVertex
#define TAB               rast_tab

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (tmesa->verts + (e<<tmesa->vertex_stride_shift))

#define TRIDENT_COLOR( dst, src )                \
do {						\
   dst[0] = src[2];				\
   dst[1] = src[1];				\
   dst[2] = src[0];				\
   dst[3] = src[3];				\
} while (0)

#define TRIDENT_SPEC( dst, src )			\
do {						\
   dst[0] = src[2];				\
   dst[1] = src[1];				\
   dst[2] = src[0];				\
} while (0)

#define VERT_SET_RGBA( v, c )    TRIDENT_COLOR( v->ub4[coloroffset], c )
#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]

#define VERT_SET_SPEC( v, c )    if (havespec) TRIDENT_SPEC( v->ub4[5], c )
#define VERT_COPY_SPEC( v0, v1 ) if (havespec) COPY_3V(v0->ub4[5], v1->ub4[5])
#define VERT_SAVE_SPEC( idx )    if (havespec) spec[idx] = v[idx]->ui[5]
#define VERT_RESTORE_SPEC( idx ) if (havespec) v[idx]->ui[5] = spec[idx]

#define LOCAL_VARS(n)						\
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);		\
   GLuint color[n], spec[n];					\
   GLuint coloroffset = (tmesa->vertex_size == 4 ? 3 : 4);	\
   GLboolean havespec = (tmesa->vertex_size == 4 ? 0 : 1);	\
   (void) color; (void) spec; (void) coloroffset; (void) havespec;
/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/
#if 0
static const GLuint hw_prim[GL_POLYGON+1] = {
   B_PrimType_Points,
   B_PrimType_Lines,
   B_PrimType_Lines,
   B_PrimType_Lines,
   B_PrimType_Triangles,
   B_PrimType_Triangles,
   B_PrimType_Triangles,
   B_PrimType_Triangles,
   B_PrimType_Triangles,
   B_PrimType_Triangles
};
#endif

static void tridentResetLineStipple( GLcontext *ctx );
#if 0
static void tridentRasterPrimitive( GLcontext *ctx, GLuint hwprim );
#endif
static void tridentRenderPrimitive( GLcontext *ctx, GLenum prim );

#define RASTERIZE(x) /*if (tmesa->hw_primitive != hw_prim[x]) \
                        tridentRasterPrimitive( ctx, hw_prim[x] ) */
#define RENDER_PRIMITIVE tmesa->render_primitive
#define TAG(x) x
#define IND TRIDENT_FALLBACK_BIT
#include "tnl_dd/t_dd_unfilled.h"
#undef IND

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_TWOSIDE_BIT|TRIDENT_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_OFFSET_BIT|TRIDENT_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_TWOSIDE_BIT|TRIDENT_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_TWOSIDE_BIT|TRIDENT_OFFSET_BIT|TRIDENT_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_OFFSET_BIT|TRIDENT_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_TWOSIDE_BIT|TRIDENT_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_TWOSIDE_BIT|TRIDENT_OFFSET_BIT|TRIDENT_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_UNFILLED_BIT|TRIDENT_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_OFFSET_BIT|TRIDENT_UNFILLED_BIT|TRIDENT_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_TWOSIDE_BIT|TRIDENT_UNFILLED_BIT|TRIDENT_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (TRIDENT_TWOSIDE_BIT|TRIDENT_OFFSET_BIT|TRIDENT_UNFILLED_BIT|TRIDENT_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

static void init_rast_tab( void )
{
   init();
   init_offset();
   init_twoside();
   init_twoside_offset();
   init_unfilled();
   init_offset_unfilled();
   init_twoside_unfilled();
   init_twoside_offset_unfilled();
   init_fallback();
   init_offset_fallback();
   init_twoside_fallback();
   init_twoside_offset_fallback();
   init_unfilled_fallback();
   init_offset_unfilled_fallback();
   init_twoside_unfilled_fallback();
   init_twoside_offset_unfilled_fallback();
}


/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define VERT(x) (tridentVertex *)(tridentverts + (x << shift))
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++) 		\
      trident_draw_point( tmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   trident_draw_line( tmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   trident_draw_triangle( tmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   trident_draw_quad( tmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) tridentRenderPrimitive( ctx, x );
#undef LOCAL_VARS
#define LOCAL_VARS						\
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);		\
   const GLuint shift = tmesa->vertex_stride_shift;		\
   const char *tridentverts = (char *)tmesa->verts;		\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
   (void) elt;
#define RESET_STIPPLE	if ( stipple ) tridentResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) trident_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) trident_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"

/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/

static void tridentRenderClippedPoly( GLcontext *ctx, const GLuint *elts, 
				   GLuint n )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint prim = tmesa->render_primitive;

   /* Render the new vertices as an unclipped polygon. 
    */
   {
      GLuint *tmp = VB->Elts;
      VB->Elts = (GLuint *)elts;
      tnl->Driver.Render.PrimTabElts[GL_POLYGON]( ctx, 0, n, PRIM_BEGIN|PRIM_END );
      VB->Elts = tmp;
   }

   /* Restore the render primitive
    */
   if (prim != GL_POLYGON)
      tnl->Driver.Render.PrimitiveNotify( ctx, prim );
}

static void tridentRenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.Line( ctx, ii, jj );
}


/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

#define _TRIDENT_NEW_RENDER_STATE (_DD_NEW_LINE_STIPPLE |	\
			          _DD_NEW_LINE_SMOOTH |		\
			          _DD_NEW_POINT_SMOOTH |	\
			          _DD_NEW_TRI_SMOOTH |		\
			          _DD_NEW_TRI_UNFILLED |	\
			          _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			          _DD_NEW_TRI_OFFSET)		\


#define POINT_FALLBACK (DD_POINT_SMOOTH)
#define LINE_FALLBACK (DD_LINE_STIPPLE|DD_LINE_SMOOTH)
#define TRI_FALLBACK (DD_TRI_SMOOTH)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)


static void tridentChooseRenderState(GLcontext *ctx)
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (flags & (ANY_RASTER_FLAGS|ANY_FALLBACK_FLAGS)) {
      tmesa->draw_point = trident_draw_point;
      tmesa->draw_line = trident_draw_line;
      tmesa->draw_tri = trident_draw_triangle;

      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_LIGHT_TWOSIDE) index |= TRIDENT_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)        index |= TRIDENT_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)      index |= TRIDENT_UNFILLED_BIT;
      }

      /* Hook in fallbacks for specific primitives.
       */
      if (flags & (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)) {
	 if (flags & POINT_FALLBACK) tmesa->draw_point = trident_fallback_point;
	 if (flags & LINE_FALLBACK)  tmesa->draw_line = trident_fallback_line;
	 if (flags & TRI_FALLBACK)   tmesa->draw_tri = trident_fallback_tri;
	 index |= TRIDENT_FALLBACK_BIT;
      }
   }

   if (tmesa->RenderIndex != index) {
      tmesa->RenderIndex = index;

      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;
         
      if (tmesa->RenderIndex == 0) {
         tnl->Driver.Render.PrimTabVerts = trident_render_tab_verts;
         tnl->Driver.Render.PrimTabElts = trident_render_tab_elts;
      } else {
         tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
         tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
      }
      tnl->Driver.Render.ClippedLine = tridentRenderClippedLine;
      tnl->Driver.Render.ClippedPolygon = tridentRenderClippedPoly;
   }
}


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/



/* Determine the rasterized primitive when not drawing unfilled 
 * polygons.
 *
 * Used only for the default render stage which always decomposes
 * primitives to trianges/lines/points.  For the accelerated stage,
 * which renders strips as strips, the equivalent calculations are
 * performed in tridentrender.c.
 */
#if 0
static void tridentRasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   if (tmesa->hw_primitive != hwprim)
      tmesa->hw_primitive = hwprim;
}
#endif

static void tridentRenderPrimitive( GLcontext *ctx, GLenum prim )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   tmesa->render_primitive = prim;
}

static void tridentRunPipeline( GLcontext *ctx )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);

   if ( tmesa->new_state )
      tridentDDUpdateHWState( ctx );

   if (tmesa->new_gl_state) {
#if 0
      if (tmesa->new_gl_state & _NEW_TEXTURE)
	 tridentUpdateTextureState( ctx );
#endif

   if (!tmesa->Fallback) {
      if (tmesa->new_gl_state & _TRIDENT_NEW_VERTEX)
	 tridentChooseVertexState( ctx );
      
      if (tmesa->new_gl_state & _TRIDENT_NEW_RENDER_STATE)
	 tridentChooseRenderState( ctx );
   }
      
      tmesa->new_gl_state = 0;
   }

   _tnl_run_pipeline( ctx );
}

static void tridentRenderStart( GLcontext *ctx )
{
   /* Check for projective texturing.  Make sure all texcoord
    * pointers point to something.  (fix in mesa?)  
    */
   tridentCheckTexSizes( ctx );
}

static void tridentRenderFinish( GLcontext *ctx )
{
   if (0)
      _swrast_flush( ctx );	/* never needed */
}

static void tridentResetLineStipple( GLcontext *ctx )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   (void) tmesa;

   /* Reset the hardware stipple counter.
    */
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/


void tridentFallback( tridentContextPtr tmesa, GLuint bit, GLboolean mode )
{
   GLcontext *ctx = tmesa->glCtx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = tmesa->Fallback;

      _tnl_need_projected_coords( ctx, GL_FALSE );

   if (mode) {
      tmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 _swsetup_Wakeup( ctx );
	 tmesa->RenderIndex = ~0;
      }
   }
   else {
      tmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = tridentRenderStart;
	 tnl->Driver.Render.PrimitiveNotify = tridentRenderPrimitive;
	 tnl->Driver.Render.Finish = tridentRenderFinish;
	 tnl->Driver.Render.BuildVertices = tridentBuildVertices;
         tnl->Driver.Render.ResetLineStipple = tridentResetLineStipple;
	 tmesa->new_gl_state |= (_TRIDENT_NEW_RENDER_STATE|
				 _TRIDENT_NEW_VERTEX);
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/


void tridentDDInitTriFuncs( GLcontext *ctx )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }
   
   tmesa->RenderIndex = ~0;

   tnl->Driver.RunPipeline = tridentRunPipeline;
   tnl->Driver.Render.Start = tridentRenderStart;
   tnl->Driver.Render.Finish = tridentRenderFinish; 
   tnl->Driver.Render.PrimitiveNotify = tridentRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = tridentResetLineStipple;
   tnl->Driver.Render.BuildVertices = tridentBuildVertices;
}
