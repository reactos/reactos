
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command 
 * FILE:            
 * PURPOSE:         
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org) 
 */

#include "net.h"

INT cmdStart(INT argc, CHAR **argv )
{
   char *string;
   long size = 100*sizeof(char);

   if (argc>4)
   {
	  help();
	  return 0;
   }
  
   if (argc==2)
   {
      string = (char *) malloc(size);
      if (string != NULL)
      {
         sprintf(string,"rpcclient -c \"service enum\"");
         system(string);
         free(string);
      }
      return 0;
   }
    
   if (argc==3)
   {
	  start_service(argv[1]);
      return 0;
   }
   
   return 0;
}


INT start_service(CHAR *service)
{  
 
  CHAR *srvlst;
  LONG pos=0;
  LONG old_pos=0;
  LONG row_size=0;
  LONG size=0;

  CHAR *row; /* we assume display name can max be 20 row and each row is 80 char */
  
  
  /* Get the size for  srvlst */
  myCreateProcessStartGetSzie("rpcclient -c \"service enum\"", &size);
  if (size==0)
  {
    return 0;
  }

  srvlst = (CHAR *) malloc(size);
  if (srvlst == NULL)
  {
	  return 0;
  }
  /* Get the server list */
  myCreateProcessStart("rpcclient -c \"service enum\"", srvlst, size);

  
  /* scan after display name */
  while (pos<size)
  {	
		old_pos = pos;

		if (1 == row_scanner_service(srvlst, &pos, size, service, NULL))
		{
		  row_size = (pos - old_pos)+32; /* 32 buffer for command */
		  pos = old_pos;
		  row = (CHAR *) malloc(row_size*sizeof(CHAR));
		  if (row == NULL)
	      {
		    return 0;
		  }   
		  memset(row,0,row_size*sizeof(CHAR));
		  if (1 == row_scanner_service(srvlst, &pos, size, service, &row[28]))
		  {
		     /* 
			    display name found 
		        now we can start the service 
			  */
			                
			  memcpy(row,"rpcclient -c \"service start %s\"\"",28*sizeof(CHAR));
			  row_size = strlen(row);
			  row[row_size] = '\"';			 
              system(row);
		  }
		  free(row);
		}
  }
  
  free(srvlst);
  return 0;
}
