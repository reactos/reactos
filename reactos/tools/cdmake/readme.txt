                              CD-ROM Maker
                           Philip J. Erdelsky

The CDMAKE utility converts files from DOS/Windows format to ISO9660
(CD-ROM) format.

First, gather all the files to be converted and put them into a single
base directory and its subdirectories, arranged just the way you want
them on the CD-ROM. Remember that ISO9660 allows subdirectories to be
nested only eight levels deep. Therefore, if the base directory is
C:\CDROM,

     C:\CDROM\D2\D3\D4\D5\D6\D7\D8\FOO.TXT is permitted, but

     C:\CDROM\D2\D3\D4\D5\D6\D7\D8\D9\FOO.TXT is forbidden.

Also, ISO9660 does not allow directories to have extensions, although
DOS does.

Finally, the characters in file and directory names and file extensions
must be letters, digits or underscores. Other punctuation marks
permitted by DOS/Windows are forbidden by ISO9660. You can use the -c
option to override this restriction, but the resulting CD-ROM may not be
readable on systems other than DOS/Windows.

Files in the base directory will be written to the root directory of the
CD-ROM image. All subdirectories of the base directory will appear as
subdirectories of the root directory of the CD-ROM image. Their
contents, and the contents of their subdirectories, down to the eighth
level, will be faithfully copied to the CD-ROM image.

System files will not be written to the CD-ROM image. Hidden files will
be written to the CD-ROM image, and will retain their hidden attributes.
Read-only files will be written, and will remain read-only on the
CD-ROM, but this does not distinguish them in any way, because on a
CD-ROM all files are read-only. The archive attribute will be lost.

File and directory date and time stamps will be preserved in the CD-ROM
image.

The utility is called up by a command line of the following form:

     CDMAKE  [-q] [-v] [-p] [-s N] [-m] [-b bootimage]  source  volume  image

     source        specifications of base directory containing all files to
                   be written to CD-ROM image

     volume        volume label

     image         image file or device

     -q            quiet mode - display nothing but error messages

     -v            verbose mode - display file information as files are
                   scanned and written - overrides -p option

     -p            show progress while writing

     -s N          abort operation before beginning write if image will be
                   larger than N megabytes (i.e. 1024*1024*N bytes)

     -m            accept punctuation marks other than underscores in
                   names and extensions

     -b bootimage  create bootable ElTorito CD-ROM using 'no emulation' mode


The utility makes three passes over the source files:

     (1) The scanning pass, in which the names and extensions are
         checked for validity, and the names, extensions, sizes, dates,
         times and attributes are recorded internally. The files are not
         actually read during this pass.

     (2) The layout pass, in which the sizes and positions of
         directories, files and other items in the CD-ROM image are
         determined.

     (3) The writing pass, in which the files are actually read and the
         CD-ROM image is actually written to the specified file or
         device. The image is always written sequentially.

If neither the -q nor the -v option is used, CDMAKE will display the
volume label, size, number of files and directories and the total bytes
in each at the end of the layout pass.

If the -p option is used, and is not overridden by the -v option, then
during the writing pass, CDMAKE will display the number of bytes still
to be written to the CD-ROM image, updating it frequently. The number
will decrease as the operation progresses, and will reach zero when the
operation is complete.

The operation of CDMAKE can be aborted by typing Ctrl-C when the utility
is displaying text of any kind.
