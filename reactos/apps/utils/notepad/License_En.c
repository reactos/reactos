#include "windows.h"
#include "license.h"

static CHAR LicenseCaption_En[] = "LICENSE";
static CHAR License_En[] = "\
You may without charge, royalty or other payment, copy and\
 distribute copies of this work and derivative works of this work\
 in source or binary form provided that: (1)\
 you appropriately publish on each copy an appropriate copyright\
 notice; (2) faithfully reproduce all prior copyright notices\
 included in the original work (you may also add your own\
 copyright notice); and (3) agree to indemnify and hold all prior\
 authors, copyright holders and licensors of the work harmless\
 from and against all damages arising from use of the work.\
\n\
You may distribute sources of derivative works of the work\
 provided that (1) (a) all source files of the original work that\
 have been modified, (b) all source files of the derivative work\
 that contain any party of the original work, and (c) all source\
 files of the derivative work that are necessary to compile, link\
 and run the derivative work without unresolved external calls and\
 with the same functionality of the original work (\"Necessary\
 Sources\") carry a prominent notice explaining the nature and date\
 of the modification and/or creation.  You are encouraged to make\
 the Necessary Sources available under this license in order to\
 further the development and acceptance of the work.\
\n\
EXCEPT AS OTHERWISE RESTRICTED BY LAW, THIS WORK IS PROVIDED\
 WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES OF ANY KIND, INCLUDING\
 BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF FITNESS FOR A\
 PARTICULAR PURPOSE, MERCHANTABILITY OR TITLE.  EXCEPT AS\
 OTHERWISE PROVIDED BY LAW, NO AUTHOR, COPYRIGHT HOLDER OR\
 LICENSOR SHALL BE LIABLE TO YOU FOR DAMAGES OF ANY KIND, EVEN IF\
 ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.";

static CHAR NoWarrantyCaption_En[] = "NO WARRANTY";
static CHAR NoWarranty_En[] = "\
EXCEPT AS OTHERWISE RESTRICTED BY LAW, THIS WORK IS PROVIDED\
 WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES OF ANY KIND, INCLUDING\
 BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF FITNESS FOR A\
 PARTICULAR PURPOSE, MERCHANTABILITY OR TITLE.  EXCEPT AS\
 OTHERWISE PROVIDED BY LAW, NO AUTHOR, COPYRIGHT HOLDER OR\
 LICENSOR SHALL BE LIABLE TO YOU FOR DAMAGES OF ANY KIND, EVEN IF\
 ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.";

LICENSE WineLicense_En = {License_En, LicenseCaption_En,
                          NoWarranty_En, NoWarrantyCaption_En};

