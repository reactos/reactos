#ifndef DO_DEBUG_VERTS
#define DO_DEBUG_VERTS 0
#endif 

#ifndef PRINT_VERTEX
#define PRINT_VERTEX(x) 
#endif

#if defined(USE_X86_ASM)
#define COPY_DWORDS( j, vb, vertsize, v )				\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (j), "=D" (vb), "=S" (__tmp)	\
			      : "0" (vertsize),				\
			        "D" ((long)vb),				\
			        "S" ((long)v) );			\
} while (0)
#else
#define COPY_DWORDS( j, vb, vertsize, v )				\
do {									\
   for ( j = 0 ; j < vertsize ; j++ )					\
      vb[j] = ((GLuint *)v)[j];						\
   vb += vertsize;							\
} while (0)
#endif



#if HAVE_QUADS
static __inline void TAG(quad)( CTX_ARG,
				VERTEX *v0,
				VERTEX *v1,
				VERTEX *v2,
				VERTEX *v3 )
{
   GLuint vertsize = GET_VERTEX_DWORDS();
   GLuint *vb = (GLuint *)ALLOC_VERTS( 4, vertsize);
   GLuint j;

   if (DO_DEBUG_VERTS) {
      fprintf(stderr, "%s\n", __FUNCTION__);
      PRINT_VERTEX(v0);
      PRINT_VERTEX(v1);
      PRINT_VERTEX(v2);
      PRINT_VERTEX(v3);
   }
      
   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
   COPY_DWORDS( j, vb, vertsize, v3 );
}
#else
static __inline void TAG(quad)( CTX_ARG,
				VERTEX *v0,
				VERTEX *v1,
				VERTEX *v2,
				VERTEX *v3 )
{
   GLuint vertsize = GET_VERTEX_DWORDS();
   GLuint *vb = (GLuint *)ALLOC_VERTS(  6, vertsize);
   GLuint j;

   if (DO_DEBUG_VERTS) {
      fprintf(stderr, "%s\n", __FUNCTION__);
      PRINT_VERTEX(v0);
      PRINT_VERTEX(v1);
      PRINT_VERTEX(v2);
      PRINT_VERTEX(v3);
   }
 
   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v3 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
   COPY_DWORDS( j, vb, vertsize, v3 );
}
#endif


static __inline void TAG(triangle)( CTX_ARG,
				    VERTEX *v0,
				    VERTEX *v1,
				    VERTEX *v2 )
{
   GLuint vertsize = GET_VERTEX_DWORDS();
   GLuint *vb = (GLuint *)ALLOC_VERTS( 3, vertsize);
   GLuint j;

   if (DO_DEBUG_VERTS) {
      fprintf(stderr, "%s\n", __FUNCTION__);
      PRINT_VERTEX(v0);
      PRINT_VERTEX(v1);
      PRINT_VERTEX(v2);
   }

   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
}


#if HAVE_LINES
static __inline void TAG(line)( CTX_ARG,
				VERTEX *v0,
				VERTEX *v1 )
{
   GLuint vertsize = GET_VERTEX_DWORDS();
   GLuint *vb = (GLuint *)ALLOC_VERTS( 2, vertsize);
   GLuint j;

   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
}
#endif

#if HAVE_POINTS
static __inline void TAG(point)( CTX_ARG,
				 VERTEX *v0 )
{
   GLuint vertsize = GET_VERTEX_DWORDS();
   GLuint *vb = (GLuint *)ALLOC_VERTS( 1, vertsize);
   int j;

   COPY_DWORDS( j, vb, vertsize, v0 );
}
#endif


static void TAG(fast_clipped_poly)( GLcontext *ctx, const GLuint *elts,
				    GLuint n )
{
   LOCAL_VARS
   GLuint vertsize = GET_VERTEX_DWORDS();
   GLuint *vb = (GLuint *)ALLOC_VERTS( (n-2) * 3, vertsize );
   const GLuint *start = (const GLuint *)VERT(elts[0]);
   int i,j;

   if (DO_DEBUG_VERTS) {
      fprintf(stderr, "%s\n", __FUNCTION__);
      PRINT_VERTEX(VERT(elts[0]));
      PRINT_VERTEX(VERT(elts[1]));
   }

   for (i = 2 ; i < n ; i++) {
      if (DO_DEBUG_VERTS) {
	 PRINT_VERTEX(VERT(elts[i]));
      }

      COPY_DWORDS( j, vb, vertsize, VERT(elts[i-1]) );
      COPY_DWORDS( j, vb, vertsize, VERT(elts[i]) );
      COPY_DWORDS( j, vb, vertsize, start );
   }
}

