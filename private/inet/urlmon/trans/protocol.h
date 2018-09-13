// protocl.h
// This file contains templates and externs to things that are protocl specific



BOOL FBeginHttpTransaction(
   LPDLD lpDownload
   );

void TerminateHttpTransaction(
   LPDLD lpDownload
   );


BOOL FBeginFtpTransaction(
   LPDLD lpDownload
   );

void TerminateFtpTransaction(
   LPDLD lpDownload
   );


BOOL FBeginGopherTransaction(
   LPDLD lpDownload
   );

void TerminateGopherTransaction(
   LPDLD lpDownload
   );


BOOL FBeginFileTransaction(
   LPDLD lpDownload
   );

void TerminateFileTransaction(
   LPDLD lpDownload
   );

// BUGBUG couldn't we use the pdld to indicate whether this was restarted
// instead of using additional parameters?
BOOL FPreprocessHttpResponse(LPDLD	pdld, BOOL *lpfRestarted);
BOOL FPreprocessFtpResponse(LPDLD   lpDownload, BOOL *lpfRestarted);

extern char vszHttp[];
extern char vszFtp[];
extern char vszGopher[];
extern char vszFile[];
extern char vszLocal[];
extern char vszHttps[];
extern HINTERNET	vhSession;
