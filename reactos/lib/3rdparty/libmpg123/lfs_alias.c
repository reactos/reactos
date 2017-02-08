/*
	lfs_alias: Aliases to the small/native API functions with the size of long int as suffix.

	copyright 2010-2013 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis

	Use case: Client code on Linux/x86-64 that defines _FILE_OFFSET_BITS to 64,
	which is the only choice on that platform anyway. It should be no-op, but
	prompts the platform-agnostic header of mpg123 to define API calls with the
	corresponding suffix. This file provides the names for this case. It's cruft,
	but glibc does it, too -- so people rely on it.
	Oh, and it also caters for the lunatics that define _FILE_OFFSET_BITS=32 on
	32 bit platforms. In addition, it's needed for platforms that always have
	off_t /= long, and clients still insisting on defining _FILE_OFFSET_BITS.

	Depending on use case, the aliases map to 32 (small) or 64 bit (large) offset
	functions, to the ones from libmpg123 or the ones from lfs_wrap.
	
	So, two basic cases:
	1. mpg123_bla_32 alias for mpg123_bla (native)
	2. mpg123_bla    alias for mpg123_bla_32 (wrapper)
	Same for 64 bits. Confusing, I know. It sucks.

	Note that the mpg123 header is _not_ used here to avoid definition with whacky off_t.
	The aliases are always about arguments of native alias_t type. This can be off_t, but
	on Linux/x86, this is long int. The off_t declarations in mpg123.h confuse things,
	so reproduce definitions for the wrapper functions in that case. The definitions are
	pulled by an inline Perl script in any case ... no need to copy anything manually!
	As a benefit, one can skip undefining possible largefile namings.
*/

#include "config.h"

/* Hack for Solaris: Some system headers included from compat.h might force _FILE_OFFSET_BITS. Need to follow that here.
   Also, want it around to have types defined. */
#include "compat.h"

#ifndef LFS_ALIAS_BITS
#error "I need the count of alias bits here."
#endif

#define MACROCAT_REALLY(a, b) a ## b
#define MACROCAT(a, b) MACROCAT_REALLY(a, b)

/* This is wicked switchery: Decide which way the aliases are facing. */

#if _FILE_OFFSET_BITS+0 == LFS_ALIAS_BITS

/* The native functions have suffix, the aliases not. */
#define NATIVE_SUFFIX MACROCAT(_, _FILE_OFFSET_BITS)
#define NATIVE_NAME(func) MACROCAT(func, NATIVE_SUFFIX)
#define ALIAS_NAME(func) func

#else

/* The alias functions have suffix, the native ones not. */
#define ALIAS_SUFFIX MACROCAT(_, LFS_ALIAS_BITS)
#define ALIAS_NAME(func) MACROCAT(func, ALIAS_SUFFIX)
#define NATIVE_NAME(func) func

#endif

/* Copy of necessary definitions, actually just forward declarations. */
struct mpg123_handle_struct;
typedef struct mpg123_handle_struct mpg123_handle;


/* Get attribute_align_arg, to stay safe. */
#include "abi_align.h"

/*
	Extract the list of functions we need wrappers for, pregenerating the wrappers for simple cases (inline script for nedit):
perl -ne '
if(/^\s*MPG123_EXPORT\s+(\S+)\s+(mpg123_\S+)\((.*)\);\s*$/)
{
	my $type = $1;
	my $name = $2;
	my $args = $3;
	next unless ($type =~ /off_t/ or $args =~ /off_t/ or ($name =~ /open/ and $name ne mpg123_open_feed));
	$type =~ s/off_t/lfs_alias_t/g;
	my @nargs = ();
	$args =~ s/off_t/lfs_alias_t/g;
	foreach my $a (split(/,/, $args))
	{
		$a =~ s/^.*\s\**([a-z_]+)$/$1/;
		push(@nargs, $a);
	}
	my $nargs = join(", ", @nargs);
	$nargs = "Human: figure me out." if($nargs =~ /\(/);
	print <<EOT

$type NATIVE_NAME($name)($args);
$type attribute_align_arg ALIAS_NAME($name)($args)
{
	return NATIVE_NAME($name)($nargs);
}
EOT

}' < mpg123.h.in
*/

int NATIVE_NAME(mpg123_open)(mpg123_handle *mh, const char *path);
int attribute_align_arg ALIAS_NAME(mpg123_open)(mpg123_handle *mh, const char *path)
{
	return NATIVE_NAME(mpg123_open)(mh, path);
}

int NATIVE_NAME(mpg123_open_fd)(mpg123_handle *mh, int fd);
int attribute_align_arg ALIAS_NAME(mpg123_open_fd)(mpg123_handle *mh, int fd)
{
	return NATIVE_NAME(mpg123_open_fd)(mh, fd);
}

int NATIVE_NAME(mpg123_open_handle)(mpg123_handle *mh, void *iohandle);
int attribute_align_arg ALIAS_NAME(mpg123_open_handle)(mpg123_handle *mh, void *iohandle)
{
	return NATIVE_NAME(mpg123_open_handle)(mh, iohandle);
}

int NATIVE_NAME(mpg123_decode_frame)(mpg123_handle *mh, lfs_alias_t *num, unsigned char **audio, size_t *bytes);
int attribute_align_arg ALIAS_NAME(mpg123_decode_frame)(mpg123_handle *mh, lfs_alias_t *num, unsigned char **audio, size_t *bytes)
{
	return NATIVE_NAME(mpg123_decode_frame)(mh, num, audio, bytes);
}

int NATIVE_NAME(mpg123_framebyframe_decode)(mpg123_handle *mh, lfs_alias_t *num, unsigned char **audio, size_t *bytes);
int attribute_align_arg ALIAS_NAME(mpg123_framebyframe_decode)(mpg123_handle *mh, lfs_alias_t *num, unsigned char **audio, size_t *bytes)
{
	return NATIVE_NAME(mpg123_framebyframe_decode)(mh, num, audio, bytes);
}

lfs_alias_t NATIVE_NAME(mpg123_framepos)(mpg123_handle *mh);
lfs_alias_t attribute_align_arg ALIAS_NAME(mpg123_framepos)(mpg123_handle *mh)
{
	return NATIVE_NAME(mpg123_framepos)(mh);
}

lfs_alias_t NATIVE_NAME(mpg123_tell)(mpg123_handle *mh);
lfs_alias_t attribute_align_arg ALIAS_NAME(mpg123_tell)(mpg123_handle *mh)
{
	return NATIVE_NAME(mpg123_tell)(mh);
}

lfs_alias_t NATIVE_NAME(mpg123_tellframe)(mpg123_handle *mh);
lfs_alias_t attribute_align_arg ALIAS_NAME(mpg123_tellframe)(mpg123_handle *mh)
{
	return NATIVE_NAME(mpg123_tellframe)(mh);
}

lfs_alias_t NATIVE_NAME(mpg123_tell_stream)(mpg123_handle *mh);
lfs_alias_t attribute_align_arg ALIAS_NAME(mpg123_tell_stream)(mpg123_handle *mh)
{
	return NATIVE_NAME(mpg123_tell_stream)(mh);
}

lfs_alias_t NATIVE_NAME(mpg123_seek)(mpg123_handle *mh, lfs_alias_t sampleoff, int whence);
lfs_alias_t attribute_align_arg ALIAS_NAME(mpg123_seek)(mpg123_handle *mh, lfs_alias_t sampleoff, int whence)
{
	return NATIVE_NAME(mpg123_seek)(mh, sampleoff, whence);
}

lfs_alias_t NATIVE_NAME(mpg123_feedseek)(mpg123_handle *mh, lfs_alias_t sampleoff, int whence, lfs_alias_t *input_offset);
lfs_alias_t attribute_align_arg ALIAS_NAME(mpg123_feedseek)(mpg123_handle *mh, lfs_alias_t sampleoff, int whence, lfs_alias_t *input_offset)
{
	return NATIVE_NAME(mpg123_feedseek)(mh, sampleoff, whence, input_offset);
}

lfs_alias_t NATIVE_NAME(mpg123_seek_frame)(mpg123_handle *mh, lfs_alias_t frameoff, int whence);
lfs_alias_t attribute_align_arg ALIAS_NAME(mpg123_seek_frame)(mpg123_handle *mh, lfs_alias_t frameoff, int whence)
{
	return NATIVE_NAME(mpg123_seek_frame)(mh, frameoff, whence);
}

lfs_alias_t NATIVE_NAME(mpg123_timeframe)(mpg123_handle *mh, double sec);
lfs_alias_t attribute_align_arg ALIAS_NAME(mpg123_timeframe)(mpg123_handle *mh, double sec)
{
	return NATIVE_NAME(mpg123_timeframe)(mh, sec);
}

int NATIVE_NAME(mpg123_index)(mpg123_handle *mh, lfs_alias_t **offsets, lfs_alias_t *step, size_t *fill);
int attribute_align_arg ALIAS_NAME(mpg123_index)(mpg123_handle *mh, lfs_alias_t **offsets, lfs_alias_t *step, size_t *fill)
{
	return NATIVE_NAME(mpg123_index)(mh, offsets, step, fill);
}

int NATIVE_NAME(mpg123_set_index)(mpg123_handle *mh, lfs_alias_t *offsets, lfs_alias_t step, size_t fill);
int attribute_align_arg ALIAS_NAME(mpg123_set_index)(mpg123_handle *mh, lfs_alias_t *offsets, lfs_alias_t step, size_t fill)
{
	return NATIVE_NAME(mpg123_set_index)(mh, offsets, step, fill);
}

int NATIVE_NAME(mpg123_position)( mpg123_handle *mh, lfs_alias_t frame_offset, lfs_alias_t buffered_bytes, lfs_alias_t *current_frame, lfs_alias_t *frames_left, double *current_seconds, double *seconds_left);
int attribute_align_arg ALIAS_NAME(mpg123_position)( mpg123_handle *mh, lfs_alias_t frame_offset, lfs_alias_t buffered_bytes, lfs_alias_t *current_frame, lfs_alias_t *frames_left, double *current_seconds, double *seconds_left)
{
	return NATIVE_NAME(mpg123_position)(mh, frame_offset, buffered_bytes, current_frame, frames_left, current_seconds, seconds_left);
}

lfs_alias_t NATIVE_NAME(mpg123_length)(mpg123_handle *mh);
lfs_alias_t attribute_align_arg ALIAS_NAME(mpg123_length)(mpg123_handle *mh)
{
	return NATIVE_NAME(mpg123_length)(mh);
}

int NATIVE_NAME(mpg123_set_filesize)(mpg123_handle *mh, lfs_alias_t size);
int attribute_align_arg ALIAS_NAME(mpg123_set_filesize)(mpg123_handle *mh, lfs_alias_t size)
{
	return NATIVE_NAME(mpg123_set_filesize)(mh, size);
}

int NATIVE_NAME(mpg123_replace_reader)(mpg123_handle *mh, ssize_t (*r_read) (int, void *, size_t), lfs_alias_t (*r_lseek)(int, lfs_alias_t, int));
int attribute_align_arg ALIAS_NAME(mpg123_replace_reader)(mpg123_handle *mh, ssize_t (*r_read) (int, void *, size_t), lfs_alias_t (*r_lseek)(int, lfs_alias_t, int))
{
	return NATIVE_NAME(mpg123_replace_reader)(mh, r_read, r_lseek);
}

int NATIVE_NAME(mpg123_replace_reader_handle)(mpg123_handle *mh, ssize_t (*r_read) (void *, void *, size_t), lfs_alias_t (*r_lseek)(void *, lfs_alias_t, int), void (*cleanup)(void*));
int attribute_align_arg ALIAS_NAME(mpg123_replace_reader_handle)(mpg123_handle *mh, ssize_t (*r_read) (void *, void *, size_t), lfs_alias_t (*r_lseek)(void *, lfs_alias_t, int), void (*cleanup)(void*))
{
	return NATIVE_NAME(mpg123_replace_reader_handle)(mh, r_read, r_lseek, cleanup);
}

