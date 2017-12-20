/*
	vfdzip.c

	Virtual Floppy Drive for Windows
	Driver control library
	Zip compressed floppy image handling

	Copyright (C) 2003-2005 Ken Kato
*/

#ifdef __cplusplus
#pragma message(__FILE__": Compiled as C++ for testing purpose.")
#endif	// __cplusplus

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "vfdtypes.h"
#include "vfdio.h"
#include "vfdlib.h"

#ifndef __REACTOS__
#define ZLIB_WINAPI
#else
#define Z_SOLO
#define ZLIB_INTERNAL
#endif
#include "zlib.h"

#ifdef VFD_NO_ZLIB
#pragma message("ZIP image support is disabled.")

DWORD ExtractZipInfo(
	HANDLE			hFile,
	ULONG			*pSize)
{
	UNREFERENCED_PARAMETER(hFile);
	UNREFERENCED_PARAMETER(pSize);
	return ERROR_NOT_SUPPORTED;
}

DWORD ExtractZipImage(
	HANDLE			hFile,
	PUCHAR			*pBuffer,
	PULONG			pLength)
{
	UNREFERENCED_PARAMETER(hFile);
	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(pLength);
	return ERROR_NOT_SUPPORTED;
}

#else	// VFD_NO_ZLIB

#ifdef _DEBUG
static const char *ZLIB_ERROR(int err)
{
	switch (err) {
	case Z_OK			: return "Z_OK";
	case Z_STREAM_END	: return "Z_STREAM_END";
	case Z_NEED_DICT	: return "Z_NEED_DICT";
	case Z_ERRNO		: return "Z_ERRNO";
	case Z_STREAM_ERROR : return "Z_STREAM_ERROR";
	case Z_DATA_ERROR	: return "Z_DATA_ERROR";
	case Z_MEM_ERROR	: return "Z_MEM_ERROR";
	case Z_BUF_ERROR	: return "Z_BUF_ERROR";
	case Z_VERSION_ERROR: return "Z_VERSION_ERROR";
	default				: return "unknown";
	}
}
#endif	// _DEBUG

voidpf zcalloc OF((voidpf opaque, unsigned items, unsigned size))
{
	UNREFERENCED_PARAMETER(opaque);
	return LocalAlloc(LPTR, items * size);
}

void   zcfree  OF((voidpf opaque, voidpf ptr))
{
	UNREFERENCED_PARAMETER(opaque);
	LocalFree(ptr);
}

#define ZIP_LOCAL_SIGNATURE			0x04034b50

#define ZIP_FLAG_ENCRYPTED			0x01

#define ZIP_FLAG_DEFLATE_NORMAL		0x00
#define ZIP_FLAG_DEFLATE_MAX		0x02
#define ZIP_FLAG_DEFLATE_FAST		0x04
#define ZIP_FLAG_DEFLATE_SUPER		0x06
#define ZIP_FLAG_DEFLATE_MASK		0x06

#define ZIP_FLAG_SIZE_IN_DESC		0x08

#define ZIP_METHOD_STORED			0
#define ZIP_METHOD_SHRUNK			1
#define ZIP_METHOD_REDUCED1			2
#define ZIP_METHOD_REDUCED2			3
#define ZIP_METHOD_REDUCED3			4
#define ZIP_METHOD_REDUCED4			5
#define ZIP_METHOD_IMPLODED			6
#define ZIP_METHOD_TOKENIZED		7
#define ZIP_METHOD_DEFLATED			8
#define ZIP_METHOD_DEFLATE64		9
#define ZIP_METHOD_PKWARE_IMP		10
#define ZIP_METHOD_RESERVED			11
#define ZIP_METHOD_BZIP2			12

#pragma pack(1)

typedef struct _zip_local_file_header {
	ULONG	header_signature;
	USHORT	required_version;
	USHORT	general_flags;
	USHORT	compression_method;
	USHORT	last_mod_time;
	USHORT	last_mod_date;
	ULONG	crc32;
	ULONG	compressed_size;
	ULONG	uncompressed_size;
	USHORT	file_name_length;
	USHORT	extra_field_length;
	CHAR	file_name[1];
	// followed by extra field data, then compressed data
}
ZIP_HEADER, *PZIP_HEADER;

//
//	Check if the file is ZIP compressed
//
DWORD ExtractZipInfo(
	HANDLE			hFile,
	ULONG			*pSize)
{
	ZIP_HEADER		zip_hdr;
	DWORD			result;
	DWORD			ret;

	if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) != 0) {
		ret = GetLastError();

		VFDTRACE(0,
			("SetFilePointer() - %s\n",
			SystemMessage(ret)));

		return ret;
	}

	if (!ReadFile(hFile, &zip_hdr, sizeof(zip_hdr), &result, NULL)) {
		ret = GetLastError();

		VFDTRACE(0,
			("ReadFile() - %s\n",
			SystemMessage(ret)));

		return ret;
	}

	if (result != sizeof(zip_hdr)							||
		zip_hdr.header_signature != ZIP_LOCAL_SIGNATURE		||
		zip_hdr.compression_method != ZIP_METHOD_DEFLATED	||
		(zip_hdr.general_flags & ZIP_FLAG_ENCRYPTED)) {

		VFDTRACE(0,
			("[VFD] Invalid ZIP file\n"));

		return ERROR_INVALID_DATA;
	}

	//	correct (and supported) ZIP header detected

	*pSize = zip_hdr.uncompressed_size;

	return ERROR_SUCCESS;
}

//
//	Extract original data from IMZ file
//
DWORD ExtractZipImage(
	HANDLE			hFile,
	PUCHAR			*pBuffer,
	PULONG			pLength)
{
	UCHAR			buf[VFD_BYTES_PER_SECTOR];
	DWORD			result;
	DWORD			ret;

	PZIP_HEADER		zip_hdr;
	ULONG			compressed;
	ULONG			uncompressed;
	PUCHAR			file_cache;
	z_stream		stream;
	int				zlib_ret;

	VFDTRACE(0,
		("[VFD] VfdExtractImz - IN\n"));

	*pBuffer = NULL;
	*pLength = 0;

	//
	//	Read PKZIP local file header of the first file in the file
	//	-- An IMZ file actually is just a ZIP file with a different
	//	extension, which contains a single floppy image file
	//
	if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) != 0) {
		ret = GetLastError();

		VFDTRACE(0,(
			"SetFilePointer - %s", SystemMessage(ret)));;

		return ret;
	}

	if (!ReadFile(hFile, buf, VFD_BYTES_PER_SECTOR, &result, NULL) ||
		result < VFD_BYTES_PER_SECTOR) {

		ret = GetLastError();

		VFDTRACE(0,(
			"ReadFile - %s", SystemMessage(ret)));;

		return ret;
	}

	zip_hdr = (PZIP_HEADER)buf;

	//	check local file header signature

	if (zip_hdr->header_signature != ZIP_LOCAL_SIGNATURE) {

		VFDTRACE(0,
			("[VFD] PKZIP header signature not found.\n"));

		return ERROR_INVALID_DATA;
	}

	//	check compression method

	if (zip_hdr->compression_method != Z_DEFLATED) {

		VFDTRACE(0,
			("[VFD] Bad PKZIP compression method.\n"));

		return ERROR_NOT_SUPPORTED;
	}

	if (zip_hdr->general_flags & 0x01) {
		//	encrypted zip not supported

		VFDTRACE(0,
			("[VFD] PKZIP encrypted.\n"));

		return ERROR_NOT_SUPPORTED;
	}

	//	check uncompressed image size

	compressed = zip_hdr->compressed_size;
	uncompressed = zip_hdr->uncompressed_size;

	switch (uncompressed) {
	case VFD_SECTOR_TO_BYTE(320):
	case VFD_SECTOR_TO_BYTE(360):
	case VFD_SECTOR_TO_BYTE(640):
	case VFD_SECTOR_TO_BYTE(720):
	case VFD_SECTOR_TO_BYTE(1280):
	case VFD_SECTOR_TO_BYTE(1440):
	case VFD_SECTOR_TO_BYTE(1640):
	case VFD_SECTOR_TO_BYTE(2400):
	case VFD_SECTOR_TO_BYTE(2880):
	case VFD_SECTOR_TO_BYTE(3360):
	case VFD_SECTOR_TO_BYTE(3444):
	case VFD_SECTOR_TO_BYTE(5760):
		break;

	default:
		VFDTRACE(0,
			("[VFD] Unsupported image size %lu.\n",
			uncompressed));

		return ERROR_NOT_SUPPORTED;
	}

	//	check local file header length
	//	-- Just for simplicity, the compressed data must start in the
	//	first sector in the file:  this is not a problem in most cases.

	if (FIELD_OFFSET(ZIP_HEADER, file_name) +
		zip_hdr->file_name_length +
		zip_hdr->extra_field_length >= VFD_BYTES_PER_SECTOR) {

		VFDTRACE(0,
			("[VFD] PKZIP header too long.\n"));

		return ERROR_NOT_SUPPORTED;
	}

	//	allocate memory to store uncompressed data

	file_cache = (PUCHAR)LocalAlloc(LPTR, uncompressed);

	if (!file_cache) {

		VFDTRACE(0,
			("[VFD] Failed to allocate file cache.\n"));

		return ERROR_OUTOFMEMORY;
	}

	//	initialize the zlib stream

	ZeroMemory(&stream, sizeof(stream));

	//	set initial input data information

	stream.next_in		= (PUCHAR)zip_hdr->file_name +
		zip_hdr->file_name_length + zip_hdr->extra_field_length;

	stream.avail_in		= VFD_BYTES_PER_SECTOR -
		FIELD_OFFSET(ZIP_HEADER, file_name) -
		zip_hdr->file_name_length - zip_hdr->extra_field_length;

	//	set output buffer information

	stream.next_out		= file_cache;
	stream.avail_out	= uncompressed;

	zlib_ret = inflateInit2(&stream, -MAX_WBITS);

	//	negative MAX_WBITS value passed to the inflateInit2() function
	//	indicates that there is no zlib header.
	//	In this case inflate() function requires an extra "dummy" byte
	//	after the compressed stream in order to complete decompression
	//	and return Z_STREAM_END.  However, both compressed and uncompressed
	//	data size are already known from the pkzip header, Z_STREAM_END
	//	is not absolutely necessary to know the completion of the operation.

	if (zlib_ret != Z_OK) {
		LocalFree(file_cache);

		VFDTRACE(0,
			("[VFD] inflateInit2() failed - %s.\n",
			ZLIB_ERROR(zlib_ret)));

		return ERROR_INVALID_FUNCTION;
	}

	for (;;) {

		//	uncompress current block

		zlib_ret = inflate(&stream, Z_NO_FLUSH);

		if (zlib_ret != Z_OK) {
			if (zlib_ret == Z_STREAM_END) {
				ret = ERROR_SUCCESS;
			}
			else {
				VFDTRACE(0,
					("[VFD] inflate() failed - %s.\n",
					ZLIB_ERROR(zlib_ret)));

				ret = ERROR_INVALID_FUNCTION;
			}
			break;
		}

		if (stream.total_out >= uncompressed) {
			//	uncompress completed - no need to wait for Z_STREAM_END
			//	(inflate() would return Z_STREAM_END on the next call)
			ret = ERROR_SUCCESS;
			break;
		}

		if (stream.total_in >= compressed) {
			//	somehow there is not enought compressed data
			ret = ERROR_INVALID_FUNCTION;
			break;
		}

		//	read next block from file

		if (!ReadFile(hFile, buf, VFD_BYTES_PER_SECTOR, &result, NULL) ||
			result <= 0) {

			ret = GetLastError();

			VFDTRACE(0,
				("[VFD] Read compressed data - %s.\n",
				SystemMessage(ret)));
			break;
		}

		stream.avail_in = result;
		stream.next_in	= buf;
	}

	//	cleanup the zlib stream

	inflateEnd(&stream);

	//	set the return information

	if (ret == ERROR_SUCCESS) {
		*pBuffer	= file_cache;
		*pLength	= uncompressed;
	}
	else {
		LocalFree(file_cache);
	}

	VFDTRACE(0,
		("[VFD] VfdExtractImz - OUT\n"));

	return ret;
}

#endif	//	VFD_NO_ZLIB
