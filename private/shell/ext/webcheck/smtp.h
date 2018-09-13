//
// SMTP - Simple Mail Transfer Protocol 
//
// Julian Jiggins, January 13th 1997
//

//
// SMTPSendMessage -
//		loads winsock, 
//		connects to the specified server on the SMTP port 25
//		sends the szMessage to the szAddress
//
BOOL SMTPSendMessage(
	char * szServer, 
	char * szToAddress, 
	char * szFromAddress, 
	char * szMessage);