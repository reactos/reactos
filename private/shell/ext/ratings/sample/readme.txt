This DLL is an example of how to write a component which obtains ratings for
URLs from some external source.  The sample is quite primitive and merely
reads the ratings from a file, RATINGS.INI.  Obviously a real product would
have more work to do.

To use this component, copy SAMPLE.DLL into the system directory (usually
C:\WINDOWS\SYSTEM).  Then run "regsvr32 sample.dll" from there;  this will
install the appropriate registry keys.  Finally, create RATINGS.INI in the
Windows directory, with contents something like this:

[Ratings]
http://www.microsoft.com=l 0 n 0 s 0 v 0
http://www.playboy.com=l 4 n 4 s 3 v 0

[Allow]
http://www.playboy.com/allowme.htm=1

[Deny]
http://www.disney.com/denyanyway.htm=1

Then, when you browse to any sites listed in RATINGS.INI, the ratings provided
there will override any ratings found at the site itself.  The [Allow] and
[Deny] sections list URLs which should have access explicitly allowed or
denied, no matter what the user's rating restrictions are.  This simulates
an inclusion/exclusion list.

To modify this DLL for your own purposes:

The source is packaged as a Microsoft Developer Studio project.

Change the name of the component to something other than SAMPLE.DLL (this
should be done in the build environment, in the resource file version stamps,
in the .DEF file, and the string szDLLNAME in comobj.cpp).																	

Run the GUIDGEN utility (supplied with Microsoft Developer Studio, or the
Win32 SDK) to generate a CLSID for your component.  Replace the definitions
of CLSID_Sample and szOurGUID with this GUID.  Do not omit this step and just
use the GUID provided, otherwise your component may collide with that of
another ISV if they also omit this step.

Modify the code in getlabel.cpp, CSampleObtainRating::ObtainRating to retrieve
the rating for a URL from the source of your choosing.  Memory for the rating
label should be allocated using the IMalloc interface provided.

If WaitForSingleObject(hAbortEvent, 0) returns WAIT_OBJECT_0, then the user
has cancelled the download and any attempt to obtain a rating should also be
cancelled.  If obtaining the rating is a relatively quick operation (such as
reading from a local file) and is not easily divided into substeps for polling
the event, then this event can be ignored.

Also modify CSampleObtainRating::GetSortOrder to determine the priority of
your rating obtainer's ratings relative to other installable rating obtainers.
Rating obtainers with lower sort orders are called before those with higher
sort orders.  Thus, if Microsoft's label bureau obtainer (whose sort order
is 0x80000000) is called and obtains a rating for a site, then any obtainer
with a higher sort order will not even be called for that site.  If the bureau
was unable to obtain a rating, then further obtainers would be called until
a rating was found.

Microsoft's label bureau obtainer's sort order is 0x80000000.  If Microsoft
produces an obtainer which fetches from a local exclusion list, its sort order
will be 0xC0000000.  Note that a rating obtained from an installable DLL will
always override any rating found in the HTML document itself.

Install your DLL in the Windows system directory.  Run "regsvr32 <yourdll>.DLL"
or load it and call its DllRegisterServer entrypoint to install its registry
keys.  Currently the user must log off and back on (or restart the system) for
the installation of this component to take effect.
