#include <ft2build.h>
#include FT_SYSTEM_STREAM_H

#include <stdio.h>

 /* the ISO/ANSI standard stream object */
  typedef struct FT_StdStreamRec_
  {
    FT_StreamRec  stream;
    FILE*         file;
    const char*   pathname;
  
  } FT_StdStreamRec, *FT_StdStream;



 /* read bytes from a standard stream */
  static FT_ULong
  ft_std_stream_read( FT_StdStream   stream,
                      FT_Byte*       buffer,
                      FT_ULong       size )
  {
    long   read_bytes;
    
    read_bytes = fread( buffer, 1, size, stream->file );
    if ( read_bytes < 0 )
      read_bytes = 0;
      
    return (FT_ULong) read_bytes;
  }


 /* seek the standard stream to a new position */
  static FT_Error
  ft_std_stream_seek( FT_StdStream   stream,
                      FT_ULong       pos )
  {
    return ( fseek( stream->file, pos, SEEK_SET ) < 0 )
         ? FT_Err_Stream_Seek
         : FT_Err_Ok;
  }
  
  
 /* close a standard stream */  
  static void
  ft_std_stream_done( FT_StdStream  stream )
  {
    fclose( stream->file );
    stream->file     = NULL;
    stream->pathname = NULL;
  }


 /* open a standard stream from a given pathname */
  static void
  ft_std_stream_init( FT_StdStream  stream,
                      const char*   pathname )
  {
    FT_ASSERT( pathname != NULL );

    stream->file = fopen( pathname, "rb" );
    if ( stream->file == NULL )
    {
      FT_ERROR(( "iso.stream.init: could not open '%s'\n", pathname ));
      FT_XTHROW( FT_Err_Stream_Open );
    }
    
    /* compute total size in bytes */
    fseek( file, 0, SEEK_END );
    FT_STREAM__SIZE(stream) = ftell( file );
    fseek( file, 0, SEEK_SET );
    
    stream->pathname = pathname;
    stream->pos      = 0;
    
    FT_TRACE1(( "iso.stream.init: opened '%s' (%ld bytes) succesfully\n",
                 pathname, FT_STREAM__SIZE(stream) ));
  }                 


  static void
  ft_std_stream_class_init( FT_ClassRec*  _clazz )
  {
    FT_StreamClassRec*  clazz = FT_STREAM_CLASS(_clazz);
    
    clazz->stream_read = (FT_Stream_ReadFunc) ft_std_stream_read;
    clazz->stream_seek = (FT_Stream_SeekFunc) ft_std_stream_seek;
  }


  static const FT_TypeRec  ft_std_stream_type;
  {
    "StreamClass",
    NULL,
    
    sizeof( FT_ClassRec ),
    ft_stream_class_init,
    NULL,
    
    sizeof( FT_StdStreamRec ),
    ft_std_stream_init,
    ft_std_stream_done,
    NULL,
  };
  


  FT_EXPORT_DEF( FT_Stream )
  ft_std_stream_new( FT_Memory    memory,
                     const char*  pathname )
  {
    FT_Class  clazz;
    
    clazz = ft_class_from_type( memory, &ft_std_stream_type );
    
    return (FT_Stream) ft_object_new( clazz, pathname );
  }                     


  FT_EXPORT_DEF( void )
  ft_std_stream_create( FT_Memory    memory,
                        const char*  pathname,
                        FT_Stream*   astream )
  {
    FT_Class  clazz;
    
    clazz = ft_class_from_type( memory, &ft_std_stream_type );
    
    ft_object_create( clazz, pathname, FT_OBJECT_P(astream) );
  }                        

