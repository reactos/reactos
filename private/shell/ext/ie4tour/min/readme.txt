Internet Explorer 4.0 Welcome Tour Minimal Install Source Readme.txt:
---------------------------------------------------------------------

The files contained in this directory and its sub-directories are specific
to the minimal install version of IE 4's Welcome Tour.

The files are broken into their functional groups via this directory structure.

	ie4tour\min	-	ie4tourm.dll build files
	ie4tour\min\Res	-	Resources bound into ie4tourm.dll
	ie4tour\min\Web	-	Content files that live on a website

The sources in min\Res are bound into ie4tourm.dll and used by welcome.exe to
run the tour from a PC where IE4 is installed and connected to the web.  These
sources provide the navigation controls for the tour.

The content files in min\Web form the remote content pulled to a PC by the
sources in ie4tourm.dll.  These files are content only and must reside on the
website pointed to by the links in the sources contained in ie4tourm.dll.

To distinguish the minimal install version from the std/full install version, the
build for these sources creates ie4tourM.dll.  When bound into .CABs for installation,
the appropriate file must be used and renamed to ie4tour.dll.
