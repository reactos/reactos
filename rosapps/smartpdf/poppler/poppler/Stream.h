//========================================================================
//
// Stream.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
// Copyright 2006: Krzysztof Kowalczyk (http://blog.kowalczyk.info)
//
//========================================================================

#ifndef STREAM_H
#define STREAM_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <stdio.h>
#include <assert.h>
#include "goo/gtypes.h"
#include "Object.h"

class Decrypt;
class BaseStream;

//------------------------------------------------------------------------

enum StreamKind {
  strFile,
  strASCIIHex,
  strASCII85,
  strLZW,
  strRunLength,
  strCCITTFax,
  strDCT,
  strFlate,
  strJBIG2,
  strJPX,
  strWeird			// internal-use stream types
};

enum StreamColorSpaceMode {
  streamCSNone,
  streamCSDeviceGray,
  streamCSDeviceRGB,
  streamCSDeviceCMYK
};

//------------------------------------------------------------------------
// Stream (base class)
//------------------------------------------------------------------------

class Stream {
public:

  Stream();

  virtual ~Stream();

  // Reference counting.
  int incRef() { return ++ref; }
  int decRef() { return --ref; }

  // Get kind of stream.
  virtual StreamKind getKind() = 0;

  // Reset stream to beginning.
  virtual void reset() = 0;

  // Close down the stream.
  virtual void close();

  // Get next char from stream.
  virtual int getChar() = 0;

  // Peek at next char in stream.
  virtual int lookChar() = 0;

  // Get next char from stream without using the predictor.
  // This is only used by StreamPredictor.
  virtual int getRawChar();

  // Get next line from stream.
  virtual char *getLine(char *buf, int size);

  // Get current position in file.
  virtual int getPos() = 0;

  // Go to a position in the stream.  If <dir> is negative, the
  // position is from the end of the file; otherwise the position is
  // from the start of the file.
  virtual void setPos(Guint pos, int dir = 0) = 0;

  // Get PostScript command for the filter(s).
  virtual GooString *getPSFilter(int psLevel, char *indent);

  // Does this stream type potentially contain non-printable chars?
  virtual GBool isBinary(GBool last = gTrue) = 0;

  // Get the BaseStream of this stream.
  virtual BaseStream *getBaseStream() = 0;

  // Get the dictionary associated with this stream.
  virtual Dict *getDict() = 0;

  // Is this an encoding filter?
  virtual GBool isEncoder() { return gFalse; }

  // Get image parameters which are defined by the stream contents.
  virtual void getImageParams(int * /*bitsPerComponent*/,
                              StreamColorSpaceMode * /*csMode*/) {}

  // Add filters to this stream according to the parameters in <dict>.
  // Returns the new stream.
  Stream *addFilters(Object *dict);

  static const int NO_SIZE_LIMIT = -1;

  virtual GBool hasGetBuf() = 0;
  virtual GBool getBuf(char **bufOut, int *bufSizeOut, int maxSize = NO_SIZE_LIMIT) = 0;
  virtual void  ungetBuf(int sizeToGoBack) = 0;

private:

  Stream *makeFilter(char *name, Stream *str, Object *params);

  int ref;			// reference count
};

//------------------------------------------------------------------------
// BaseStream
//
// This is the base class for all streams that read directly from a file.
//------------------------------------------------------------------------

class BaseStream: public Stream {
public:

  BaseStream(Object *dictA);
  virtual ~BaseStream();
  virtual Stream *makeSubStream(Guint start, GBool limited,
                                Guint length, Object *dict) = 0;
  virtual void setPos(Guint pos, int dir = 0) = 0;
  virtual GBool isBinary(GBool last = gTrue) { return last; }
  virtual BaseStream *getBaseStream() { return this; }
  virtual Dict *getDict() { return dict.getDict(); }

  // Get/set position of first byte of stream within the file.
  virtual Guint getStart() = 0;
  virtual void moveStart(int delta) = 0;

  // Set decryption for this stream.
  virtual void doDecryption(Guchar *fileKey, int keyLength,
                            int objNum, int objGen);

protected:

  Decrypt *decrypt;

private:

  Object dict;
};

//------------------------------------------------------------------------
// FilterStream
//
// This is the base class for all streams that filter another stream.
//------------------------------------------------------------------------

class FilterStream: public Stream {
public:

  FilterStream(Stream *strA);
  virtual ~FilterStream();
  virtual void close();
  virtual int getPos() { return str->getPos(); }
  virtual void setPos(Guint pos, int dir = 0);
  virtual BaseStream *getBaseStream() { return str->getBaseStream(); }
  virtual Dict *getDict() { return str->getDict(); }

  // override those in specific filters when you add support for getBuf()
  virtual GBool hasGetBuf() { return gFalse; }
  virtual GBool getBuf(char **bufOut, int *bufSizeOut, int maxSize) {
    assert(0);
    return gFalse;
  }
  virtual void  ungetBuf(int sizeToGoBack) {
    assert(0);
  }

protected:

  Stream *str;
};

//------------------------------------------------------------------------
// ImageStream
//------------------------------------------------------------------------

class ImageStream {
public:

  // Create an image stream object for an image with the specified
  // parameters.  Note that these are the actual image parameters,
  // which may be different from the predictor parameters.
  ImageStream(Stream *strA, int widthA, int nCompsA, int nBitsA);

  ~ImageStream();

  // Reset the stream.
  void reset();

  // Gets the next pixel from the stream.  <pix> should be able to hold
  // at least nComps elements.  Returns false at end of file.
  GBool getPixel(Guchar *pix);

  // Returns a pointer to the next line of pixels.  Returns NULL at
  // end of file.
  Guchar *getLine();

  // Skip an entire line from the image.
  void skipLine();

private:

  Stream *str;			// base stream
  int width;			// pixels per line
  int nComps;			// components per pixel
  int nBits;			// bits per component
  int nVals;			// components per line
  Guchar *imgLine;		// line buffer
  int imgIdx;			// current index in imgLine
};

//------------------------------------------------------------------------
// StreamPredictor
//------------------------------------------------------------------------

class StreamPredictor {
public:

  // Create a predictor object.  Note that the parameters are for the
  // predictor, and may not match the actual image parameters.
  StreamPredictor(Stream *strA, int predictorA,
                  int widthA, int nCompsA, int nBitsA);

  ~StreamPredictor();

  GBool isOk() { return ok; }

  int lookChar();
  int getChar();

private:

  GBool getNextLine();

  Stream *str;			// base stream
  int predictor;		// predictor
  int width;			// pixels per line
  int nComps;			// components per pixel
  int nBits;			// bits per component
  int nVals;			// components per line
  int pixBytes;			// bytes per pixel
  int rowBytes;			// bytes per line
  Guchar *predLine;		// line buffer
  int predIdx;			// current index in predLine
  GBool ok;
};

//------------------------------------------------------------------------
// FileStream
//------------------------------------------------------------------------

#define fileStreamBufSize 256

class FileStream: public BaseStream {
public:

  FileStream(FILE *fA, Guint startA, GBool limitedA,
             Guint lengthA, Object *dictA);
  virtual ~FileStream();
  virtual Stream *makeSubStream(Guint startA, GBool limitedA,
                                Guint lengthA, Object *dictA);
  virtual StreamKind getKind() { return strFile; }
  virtual void reset();
  virtual void close();
  virtual int getChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr++ & 0xff); }
  virtual int lookChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr & 0xff); }
  virtual int getPos() { return bufPos + (bufPtr - buf); }
  virtual void setPos(Guint pos, int dir = 0);
  virtual Guint getStart() { return start; }
  virtual void moveStart(int delta);

  virtual GBool hasGetBuf();
  virtual GBool getBuf(char **bufOut, int *bufSizeOut, int maxSize);
  virtual void  ungetBuf(int sizeToGoBack);

private:

  GBool fillBuf();

  FILE *f;
  Guint start;
  GBool limited;
  Guint length;
  char buf[fileStreamBufSize];
  char *bufPtr;
  char *bufEnd;
  Guint bufPos;
  int savePos;
  GBool saved;
};

//------------------------------------------------------------------------
// MemStream
//------------------------------------------------------------------------

class MemStream: public BaseStream {
public:

  MemStream(char *bufA, Guint startA, Guint lengthA, Object *dictA);
  virtual ~MemStream();
  virtual Stream *makeSubStream(Guint start, GBool limited,
                                Guint lengthA, Object *dictA);
  virtual StreamKind getKind() { return strWeird; }
  virtual void reset();
  virtual void close();
  virtual int getChar()
    { return (bufPtr < bufEnd) ? (*bufPtr++ & 0xff) : EOF; }
  virtual int lookChar()
    { return (bufPtr < bufEnd) ? (*bufPtr & 0xff) : EOF; }
  virtual int getPos() { return (int)(bufPtr - buf); }
  virtual void setPos(Guint pos, int dir = 0);
  virtual Guint getStart() { return start; }
  virtual void moveStart(int delta);
  virtual void doDecryption(Guchar *fileKey, int keyLength,
                            int objNum, int objGen);

  virtual GBool hasGetBuf() { return gFalse; }
  virtual GBool getBuf(char **bufOut, int *bufSizeOut, int maxSize) {
    assert(0);
    return gFalse;
  }
  virtual void  ungetBuf(int sizeToGoBack) { assert(0); }

private:

  char *buf;
  Guint start;
  Guint length;
  char *bufEnd;
  char *bufPtr;
  GBool needFree;
};

//------------------------------------------------------------------------
// EmbedStream
//
// This is a special stream type used for embedded streams (inline
// images).  It reads directly from the base stream -- after the
// EmbedStream is deleted, reads from the base stream will proceed where
// the BaseStream left off.  Note that this is very different behavior
// that creating a new FileStream (using makeSubStream).
//------------------------------------------------------------------------

class EmbedStream: public BaseStream {
public:

  EmbedStream(Stream *strA, Object *dictA, GBool limitedA, Guint lengthA);
  virtual ~EmbedStream();
  virtual Stream *makeSubStream(Guint start, GBool limitedA,
                                Guint lengthA, Object *dictA);
  virtual StreamKind getKind() { return str->getKind(); }
  virtual void reset() {}
  virtual int getChar();
  virtual int lookChar();
  virtual int getPos() { return str->getPos(); }
  virtual void setPos(Guint pos, int dir = 0);
  virtual Guint getStart();
  virtual void moveStart(int delta);

  virtual GBool hasGetBuf();
  virtual GBool getBuf(char **bufOut, int *bufSizeOut, int maxSize);
  virtual void  ungetBuf(int sizeToGoBack);

private:

  Stream *str;
  GBool limited;
  Guint length;
};

//------------------------------------------------------------------------
// ASCIIHexStream
//------------------------------------------------------------------------

class ASCIIHexStream: public FilterStream {
public:

  ASCIIHexStream(Stream *strA);
  virtual ~ASCIIHexStream();
  virtual StreamKind getKind() { return strASCIIHex; }
  virtual void reset();
  virtual int getChar()
    { int c = lookChar(); buf = EOF; return c; }
  virtual int lookChar();
  virtual GooString *getPSFilter(int psLevel, char *indent);
  virtual GBool isBinary(GBool last = gTrue);

private:

  int buf;
  GBool eof;
};

//------------------------------------------------------------------------
// ASCII85Stream
//------------------------------------------------------------------------

class ASCII85Stream: public FilterStream {
public:

  ASCII85Stream(Stream *strA);
  virtual ~ASCII85Stream();
  virtual StreamKind getKind() { return strASCII85; }
  virtual void reset();
  virtual int getChar()
    { int ch = lookChar(); ++index; return ch; }
  virtual int lookChar();
  virtual GooString *getPSFilter(int psLevel, char *indent);
  virtual GBool isBinary(GBool last = gTrue);

private:

  int c[5];
  int b[4];
  int index, n;
  GBool eof;
};

//------------------------------------------------------------------------
// LZWStream
//------------------------------------------------------------------------

class LZWStream: public FilterStream {
public:

  LZWStream(Stream *strA, int predictor, int columns, int colors,
            int bits, int earlyA);
  virtual ~LZWStream();
  virtual StreamKind getKind() { return strLZW; }
  virtual void reset();
  virtual int getChar();
  virtual int lookChar();
  virtual int getRawChar();
  virtual GooString *getPSFilter(int psLevel, char *indent);
  virtual GBool isBinary(GBool last = gTrue);

private:

  StreamPredictor *pred;	// predictor
  int early;			// early parameter
  GBool eof;			// true if at eof
  int inputBuf;			// input buffer
  int inputBits;		// number of bits in input buffer
  struct {			// decoding table
    int length;
    int head;
    Guchar tail;
  } table[4097];
  int nextCode;			// next code to be used
  int nextBits;			// number of bits in next code word
  int prevCode;			// previous code used in stream
  int newChar;			// next char to be added to table
  Guchar seqBuf[4097];		// buffer for current sequence
  int seqLength;		// length of current sequence
  int seqIndex;			// index into current sequence
  GBool first;			// first code after a table clear

  GBool processNextCode();
  void clearTable();
  int getCode();
};

//------------------------------------------------------------------------
// RunLengthStream
//------------------------------------------------------------------------

class RunLengthStream: public FilterStream {
public:

  RunLengthStream(Stream *strA);
  virtual ~RunLengthStream();
  virtual StreamKind getKind() { return strRunLength; }
  virtual void reset();
  virtual int getChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr++ & 0xff); }
  virtual int lookChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr & 0xff); }
  virtual GooString *getPSFilter(int psLevel, char *indent);
  virtual GBool isBinary(GBool last = gTrue);

private:

  char buf[128];		// buffer
  char *bufPtr;			// next char to read
  char *bufEnd;			// end of buffer
  GBool eof;

  GBool fillBuf();
};

//------------------------------------------------------------------------
// CCITTFaxStream
//------------------------------------------------------------------------

struct CCITTCodeTable;

class CCITTFaxStream: public FilterStream {
public:

  CCITTFaxStream(Stream *strA, int encodingA, GBool endOfLineA,
                 GBool byteAlignA, int columnsA, int rowsA,
                 GBool endOfBlockA, GBool blackA);
  virtual ~CCITTFaxStream();
  virtual StreamKind getKind() { return strCCITTFax; }
  virtual void reset();
  virtual int getChar()
    { int c = lookChar(); buf = EOF; return c; }
  virtual int lookChar();
  virtual GooString *getPSFilter(int psLevel, char *indent);
  virtual GBool isBinary(GBool last = gTrue);

private:

  int encoding;			// 'K' parameter
  GBool endOfLine;		// 'EndOfLine' parameter
  GBool byteAlign;		// 'EncodedByteAlign' parameter
  int columns;			// 'Columns' parameter
  int rows;			// 'Rows' parameter
  GBool endOfBlock;		// 'EndOfBlock' parameter
  GBool black;			// 'BlackIs1' parameter
  GBool eof;			// true if at eof
  GBool nextLine2D;		// true if next line uses 2D encoding
  int row;			// current row
  int inputBuf;			// input buffer
  int inputBits;		// number of bits in input buffer
  short *refLine;		// reference line changing elements
  int b1;			// index into refLine
  short *codingLine;		// coding line changing elements
  int a0;			// index into codingLine
  int outputBits;		// remaining ouput bits
  int buf;			// character buffer

  short getTwoDimCode();
  short getWhiteCode();
  short getBlackCode();
  short lookBits(int n);
  void eatBits(int n) { if ((inputBits -= n) < 0) inputBits = 0; }
};

#ifndef ENABLE_LIBJPEG
//------------------------------------------------------------------------
// DCTStream
//------------------------------------------------------------------------

// DCT component info
struct DCTCompInfo {
  int id;			// component ID
  int hSample, vSample;		// horiz/vert sampling resolutions
  int quantTable;		// quantization table number
  int prevDC;			// DC coefficient accumulator
};

struct DCTScanInfo {
  GBool comp[4];		// comp[i] is set if component i is
                                //   included in this scan
  int numComps;			// number of components in the scan
  int dcHuffTable[4];		// DC Huffman table numbers
  int acHuffTable[4];		// AC Huffman table numbers
  int firstCoeff, lastCoeff;	// first and last DCT coefficient
  int ah, al;			// successive approximation parameters
};

// DCT Huffman decoding table
struct DCTHuffTable {
  Guchar firstSym[17];		// first symbol for this bit length
  Gushort firstCode[17];	// first code for this bit length
  Gushort numCodes[17];		// number of codes of this bit length
  Guchar sym[256];		// symbols
};

class DCTStream: public FilterStream {
public:

  DCTStream(Stream *strA);
  virtual ~DCTStream();
  virtual StreamKind getKind() { return strDCT; }
  virtual void reset();
  virtual int getChar();
  virtual int lookChar();
  virtual GooString *getPSFilter(int psLevel, char *indent);
  virtual GBool isBinary(GBool last = gTrue);
  Stream *getRawStream() { return str; }

private:

  GBool progressive;		// set if in progressive mode
  GBool interleaved;		// set if in interleaved mode
  int width, height;		// image size
  int mcuWidth, mcuHeight;	// size of min coding unit, in data units
  int bufWidth, bufHeight;	// frameBuf size
  DCTCompInfo compInfo[4];	// info for each component
  DCTScanInfo scanInfo;		// info for the current scan
  int numComps;			// number of components in image
  int colorXform;		// need YCbCr-to-RGB transform?
  GBool gotJFIFMarker;		// set if APP0 JFIF marker was present
  GBool gotAdobeMarker;		// set if APP14 Adobe marker was present
  int restartInterval;		// restart interval, in MCUs
  Gushort quantTables[4][64];	// quantization tables
  int numQuantTables;		// number of quantization tables
  DCTHuffTable dcHuffTables[4];	// DC Huffman tables
  DCTHuffTable acHuffTables[4];	// AC Huffman tables
  int numDCHuffTables;		// number of DC Huffman tables
  int numACHuffTables;		// number of AC Huffman tables
  Guchar *rowBuf[4][32];	// buffer for one MCU (non-progressive mode)
  int *frameBuf[4];		// buffer for frame (progressive mode)
  int comp, x, y, dy;		// current position within image/MCU
  int restartCtr;		// MCUs left until restart
  int restartMarker;		// next restart marker
  int eobRun;			// number of EOBs left in the current run
  int inputBuf;			// input buffer for variable length codes
  int inputBits;		// number of valid bits in input buffer

  void restart();
  GBool readMCURow();
  void readScan();
  GBool readDataUnit(DCTHuffTable *dcHuffTable,
                     DCTHuffTable *acHuffTable,
                     int *prevDC, int data[64]);
  GBool readProgressiveDataUnit(DCTHuffTable *dcHuffTable,
                                DCTHuffTable *acHuffTable,
                                int *prevDC, int data[64]);
  void decodeImage();
  void transformDataUnit(Gushort *quantTable,
                         int dataIn[64], Guchar dataOut[64]);
  int readHuffSym(DCTHuffTable *table);
  int readAmp(int size);
  int readBit();
  GBool readHeader();
  GBool readBaselineSOF();
  GBool readProgressiveSOF();
  GBool readScanInfo();
  GBool readQuantTables();
  GBool readHuffmanTables();
  GBool readRestartInterval();
  GBool readJFIFMarker();
  GBool readAdobeMarker();
  GBool readTrailer();
  int readMarker();
  int read16();
};
#endif

#ifndef ENABLE_ZLIB
//------------------------------------------------------------------------
// FlateStream
//------------------------------------------------------------------------

#define flateWindow          32768    // buffer size
#define flateMask            (flateWindow-1)
#define flateMaxHuffman         15    // max Huffman code length
#define flateMaxCodeLenCodes    19    // max # code length codes
#define flateMaxLitCodes       288    // max # literal codes
#define flateMaxDistCodes       30    // max # distance codes

// Huffman code table entry
struct FlateCode {
  Gushort len;			// code length, in bits
  Gushort val;			// value represented by this code
};

struct FlateHuffmanTab {
  FlateCode *codes;
  int maxLen;
};

// Decoding info for length and distance code words
struct FlateDecode {
  int bits;			// # extra bits
  int first;			// first length/distance
};

class FlateStream: public FilterStream {
public:

  FlateStream(Stream *strA, int predictor, int columns,
              int colors, int bits);
  virtual ~FlateStream();
  virtual StreamKind getKind() { return strFlate; }
  virtual void reset();
  virtual int getChar();
  virtual int lookChar();
  virtual int getRawChar();
  virtual GooString *getPSFilter(int psLevel, char *indent);
  virtual GBool isBinary(GBool last = gTrue);

  virtual GBool hasGetBuf();
  virtual GBool getBuf(char **bufOut, int *bufSizeOut, int maxSize);
  virtual void  ungetBuf(int sizeToGoBack);

private:

  StreamPredictor *pred;	// predictor
  Guchar buf[flateWindow];	// output data buffer
  int index;			// current index into output buffer
  int remain;			// number valid bytes in output buffer
  int codeBuf;			// input buffer
  int codeSize;			// number of bits in input buffer
  int				// literal and distance code lengths
    codeLengths[flateMaxLitCodes + flateMaxDistCodes];
  FlateHuffmanTab litCodeTab;	// literal code table
  FlateHuffmanTab distCodeTab;	// distance code table
  GBool compressedBlock;	// set if reading a compressed block
  int blockLen;			// remaining length of uncompressed block
  GBool endOfBlock;		// set when end of block is reached
  GBool eof;			// set when end of stream is reached

  static int			// code length code reordering
    codeLenCodeMap[flateMaxCodeLenCodes];
  static FlateDecode		// length decoding info
    lengthDecode[flateMaxLitCodes-257];
  static FlateDecode		// distance decoding info
    distDecode[flateMaxDistCodes];
  static FlateHuffmanTab	// fixed literal code table
    fixedLitCodeTab;
  static FlateHuffmanTab	// fixed distance code table
    fixedDistCodeTab;

  void readSome();
  GBool startBlock();
  void loadFixedCodes();
  GBool readDynamicCodes();
  void compHuffmanCodes(int *lengths, int n, FlateHuffmanTab *tab);
  int getHuffmanCodeWord(FlateHuffmanTab *tab);
  int getCodeWord(int bits);
};
#endif

//------------------------------------------------------------------------
// EOFStream
//------------------------------------------------------------------------

class EOFStream: public FilterStream {
public:

  EOFStream(Stream *strA);
  virtual ~EOFStream();
  virtual StreamKind getKind() { return strWeird; }
  virtual void reset() {}
  virtual int getChar() { return EOF; }
  virtual int lookChar() { return EOF; }
  virtual GooString *getPSFilter(int /*psLevel*/, char * /*indent*/)  { return NULL; }
  virtual GBool isBinary(GBool /*last = gTrue*/) { return gFalse; }
};

//------------------------------------------------------------------------
// FixedLengthEncoder
//------------------------------------------------------------------------

class FixedLengthEncoder: public FilterStream {
public:

  FixedLengthEncoder(Stream *strA, int lengthA);
  ~FixedLengthEncoder();
  virtual StreamKind getKind() { return strWeird; }
  virtual void reset();
  virtual int getChar();
  virtual int lookChar();
  virtual GooString *getPSFilter(int /*psLevel*/, char * /*indent*/) { return NULL; }
  virtual GBool isBinary(GBool /*last = gTrue*/);
  virtual GBool isEncoder() { return gTrue; }

private:

  int length;
  int count;
};

//------------------------------------------------------------------------
// ASCIIHexEncoder
//------------------------------------------------------------------------

class ASCIIHexEncoder: public FilterStream {
public:

  ASCIIHexEncoder(Stream *strA);
  virtual ~ASCIIHexEncoder();
  virtual StreamKind getKind() { return strWeird; }
  virtual void reset();
  virtual int getChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr++ & 0xff); }
  virtual int lookChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr & 0xff); }
  virtual GooString *getPSFilter(int /*psLevel*/, char * /*indent*/) { return NULL; }
  virtual GBool isBinary(GBool /*last = gTrue*/) { return gFalse; }
  virtual GBool isEncoder() { return gTrue; }

private:

  char buf[4];
  char *bufPtr;
  char *bufEnd;
  int lineLen;
  GBool eof;

  GBool fillBuf();
};

//------------------------------------------------------------------------
// ASCII85Encoder
//------------------------------------------------------------------------

class ASCII85Encoder: public FilterStream {
public:

  ASCII85Encoder(Stream *strA);
  virtual ~ASCII85Encoder();
  virtual StreamKind getKind() { return strWeird; }
  virtual void reset();
  virtual int getChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr++ & 0xff); }
  virtual int lookChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr & 0xff); }
  virtual GooString *getPSFilter(int /*psLevel*/, char * /*indent*/) { return NULL; }
  virtual GBool isBinary(GBool /*last = gTrue*/) { return gFalse; }
  virtual GBool isEncoder() { return gTrue; }

private:

  char buf[8];
  char *bufPtr;
  char *bufEnd;
  int lineLen;
  GBool eof;

  GBool fillBuf();
};

//------------------------------------------------------------------------
// RunLengthEncoder
//------------------------------------------------------------------------

class RunLengthEncoder: public FilterStream {
public:

  RunLengthEncoder(Stream *strA);
  virtual ~RunLengthEncoder();
  virtual StreamKind getKind() { return strWeird; }
  virtual void reset();
  virtual int getChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr++ & 0xff); }
  virtual int lookChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr & 0xff); }
  virtual GooString *getPSFilter(int /*psLevel*/, char * /*indent*/) { return NULL; }
  virtual GBool isBinary(GBool /*last = gTrue*/) { return gTrue; }
  virtual GBool isEncoder() { return gTrue; }

private:

  char buf[131];
  char *bufPtr;
  char *bufEnd;
  char *nextEnd;
  GBool eof;

  GBool fillBuf();
};

#endif
