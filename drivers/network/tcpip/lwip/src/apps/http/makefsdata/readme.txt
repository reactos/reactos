This directory contains a script ('makefsdata') to create C code suitable for
httpd for given html pages (or other files) in a directory.

There is also a plain C console application doing the same and extended a bit.

Usage: htmlgen [targetdir] [-s] [-i]s
   targetdir: relative or absolute path to files to convert
   switch -s: toggle processing of subdirectories (default is on)
   switch -e: exclude HTTP header from file (header is created at runtime, default is on)
   switch -11: include HTTP 1.1 header (1.0 is default)

  if targetdir not specified, makefsdata will attempt to
  process files in subdirectory 'fs'.

The C version of this program can optionally store the none-SSI files in
a compressed form in which they are also sent to the web client (which
must support the Deflate content encoding). Files that grow during compression
(due to being not compressible well), will stored umcompressed automatically.
In order to do so, compile the program with MAKEFS_SUPPORT_DEFLATE set to 1. You must
manually download minizip.c for this to work. As an alternative, you can additionally
define MAKEFS_SUPPORT_DEFLATE_ZLIB to use your system's zlib instead.
Compression of .html, .js, .css and .svg files usually yields very good compression
rates and is a great way of reducing your program's size.
