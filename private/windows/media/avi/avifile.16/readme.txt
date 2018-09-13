Things to do in AVIFile:

Need handlers for:
	Quicktime files
	Targa files
	JPEG files
	MPEG files

Improve streaming interface

Speed improvements:
	Use GlobalDosAlloc for buffers?
	Read several frames at one time?

Current bugs:
	Error returns are lousy.


More things to do:
	Automatically generate open/save filter strings.  This would 
	require that we put something in the regdb to indicate which 
	handlers supported writing....

	Should there be some way to pass AVIFileOpen an HMMIO?  An
	IStorage or IStream?

	It would be nice to have some way to be notified when the contents
	of a stream change....  Look at IAdviseHolder

	ReadData and WriteData need to be renamed, because they
	confuse everybody.  Also, need better method for accessing
	DISP and LIST `INFO' chunks.

	Stream names need to be better supported.

	During long operations, we need some kind of status callback.

	Should everything have version numbers?

	"capabilities": Additional information via AVIFile/StreamInfo
	about what the handler can do

	Do handlers need parameters somehow?  Some way to configure
	stuff for a particular file format?  Am I deluding myself into
	thinking I can get away without this stuff?

	More thought is needed about "non-seekable" streams.  For
	instance: can a video capture device be thought of as a
	stream? 

	Can making a stream handler be made easier?  One thought: a
	C++ base class for a stream handler which people could
	override only the methods they want....

	How can you change the rectangle for a stream?
	Do we need an AVIStreamSetInfo command?


	Can we make streams be named?  Instead of opening streams by type 
	and number, people could enumerate the streams in a file and open
	them by name....  Looks more and more like IStorage....


	Everything needs to be renamed.  Functions shouldn't all
	start with "AVI".

	We need a new interface to handle change notifications.  Copy
	OLE Advise Holders, Advise Sinks.


	Marshal the AVIFile reader in the following way:
	Right before reading from an HMMIO, check the current task.
	Re-open the file for each new HTASK....

	Some way of getting an error string from an error value would
	be good.


	I don't really use the IEditStream interface for everything.
	I should make Vtbls for the editstrm.c implementation, and allow
	other handlers to support the stuff.

	How can you use ReadData/WriteData to deal with more than one
	chunk with the same ID?  How about INFO chunks?


Old Stuff that's done:

	Does AVIStreamWrite need some way of returning how much of the
	data was actually written?  (Particularly in the case where
	it's doing ACM compression, it may only be able to write out 
	whole blocks of data....)


	EditStreamSetName
	EditStreamClone

	AVIStreamInfo could use a different structure than an
	AVIStreamHeader.  In particular, the structure could lose
	some useless fields and gain the stream name.  Same with
	AVIFileInfo. 

	During long operations, we need some kind of status callback.	


	Look at using IAVIFile, IAVIStream on the clipboard along with
	IDataObject.  This seems to require some work to "marshal"
	the interface from app to app....

	Given an open IAVIFile handle, retrieve its "file format", that
	is, some user-readable name for what kind of file it is....

	It would be nice to be able to write to the compression streams

	The options dialog doesn't make me happy....



