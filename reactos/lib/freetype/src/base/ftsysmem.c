#include <ft2build.h>
#include FT_SYSTEM_MEMORY_H

  static FT_Memory
  ft_memory_new_default( FT_ULong  size )
  {
    return (FT_Memory) ft_malloc( size );
  }
  
  static void
  ft_memory_destroy_default( FT_Memory  memory )
  {
    ft_free( memory );
  }

  
 /* notice that in normal builds, we use the ISO C library functions */
 /* 'malloc', 'free' and 'realloc' directly..                        */
 /*                                                                  */
  static const FT_Memory_FuncsRec  ft_memory_funcs_default_rec = 
  {
    (FT_Memory_CreateFunc)  ft_memory_new_iso,
    (FT_Memory_DestroyFunc) ft_memory_destroy_iso,
    (FT_Memory_AllocFunc)   ft_malloc,
    (FT_Memory_FreeFunc)    ft_free,
    (FT_Memory_ReallocFunc) ft_realloc
  };
  
  FT_APIVAR_DEF( const FT_Memory_Funcs )
  ft_memory_funcs_default = &ft_memory_funcs_defaults_rec;
