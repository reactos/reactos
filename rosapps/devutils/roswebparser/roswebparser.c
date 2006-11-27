/*
 * This is a standalone rc langues to xml parser 
 * do not use windows or linux specfiy syntax or functions
 * use only pure ansi C, this program is also runing on
 * linux apachie webserver and being use in ReactOS website
 *
 * CopyRight 20/9-2006  by Magnus Olsen (magnus@greatlord.com)
 * Licen GPL version 2.0
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define true 1
#define false 0


int ansiCodePage(int codepage, unsigned char *inBuffer, unsigned char *outBuffer, int Lenght);

int paraser1(unsigned char *buf, long buf_size, unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format, unsigned char *iso_type);

void find_str(unsigned char asc, unsigned char *buf, long *foundPos);
void find_str2(unsigned char *asc, unsigned char *buf, long *foundPos, unsigned char * output_resid, unsigned char *output_text );
void trim(unsigned char* buf);
void stringbugs(unsigned char *buf, int shift2);
void stringbugs2(unsigned char *buf, int shift2);

void ParserCMD1(unsigned char *text, unsigned char *output_resid, unsigned char *output_text, unsigned char *output_format);
void ParserCMD2(unsigned char *text, unsigned char *output_resid, unsigned char *output_text, unsigned char *output_format);
void ParserCMD3(unsigned char *text, unsigned char *output_resid, unsigned char *output_text, unsigned char *output_format);

void ParserComment(long *pos, unsigned char *buf, long buf_size, unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format);
void ParserLang(unsigned char *text, unsigned char *output_resid, unsigned char *output_text, unsigned char *output_format);
void ParserString(long *pos, unsigned char *buf, long buf_size,	unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format);
void ParserDialog(unsigned char *text, long *pos, unsigned char *buf, long buf_size, unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format);
void DialogCMDBuild1(unsigned char *output_resid, unsigned char *output_format, long pos, unsigned char * text);
void DialogCMDBuild2(unsigned char *output_resid, unsigned char *output_format, long pos, unsigned char * text);
void DialogCMDBuild3(unsigned char *output_resid, unsigned char *output_format, long pos, unsigned char * text);
void ParserAccelerators(long *pos, unsigned char *buf, long buf_size, unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format);
void ParserMenu(unsigned char *text, long *pos, unsigned char *buf, long buf_size,	unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format);

/*
  return -1 : No file found
  return -2 : Fail open file
  return -3 : Fail seek
  return -4 : Fail get size
  return -5 : Fail size of the file is 0 bytes
  return -6 : Fail malloc memory
  return -7 : Fail to read the file    
  return -8 : Fail to write to the file   
  return -9 : Fail to open write file   
*/

int main(int argc, char * argv[])
{
	FILE * fp;
	FILE * Outfp;
	unsigned char * buffer;
	unsigned char * output_text;
    unsigned char * output_resid;
	unsigned char * output_format;
	
	long buf_size;
	long buf_size_calc = 0;
    	
	if (argc!=4)
	{
        printf("Help\n");
		printf("%s inputfile iso-type\n\n",argv[0]);
		printf("example %s sv.rc 28591 sv.xml\n\n",argv[0]); 
		printf("Contry table\n"); 		
		printf("se (Swedish = Windows-28591 (Latin1 ISO-8859-1)\n"); 
		 
		return -1;
	}
	

	if ((fp = fopen(argv[1],"rb"))==NULL)	
	{
		printf("Fail open file %s by %s\n",argv[1],argv[0]);
		return -2;
	}


  
	fseek(fp,0,SEEK_END);
	if (ferror(fp) !=0) 
	{
		fclose(fp);
        printf("Fail seek\n");
        return -3;
	} 
	buf_size = ftell(fp);
	if (ferror(fp) !=0) 
	{
		fclose(fp);
        printf("Fail get size\n");
        return -4;
	}


	/* 
	   We make sure it is least 4 times + 2K biger 
	   for we can grow around 2-3 times biger 
	   so it better to make safe assume how
	   much memory we need
     */

	buf_size_calc = (buf_size*4) + 2048;
    
	fseek(fp,0,SEEK_SET);
	if (ferror(fp) !=0) 
	{
		fclose(fp);
        printf("Fail seek\n");
        return -3;
	} 

	if (buf_size==0)
	{
	   fclose(fp);
       printf("Fail size of the file is 0 bytes\n");
       return -5;
	}

	buffer =(char *)malloc(buf_size_calc);
	if (buffer == NULL)
	{
       fclose(fp);
       printf("Fail malloc memory\n");
       return -6; 
	}

	output_text =(char *)malloc(buf_size_calc);
	if (output_text == NULL)
	{
	   free(buffer);
       fclose(fp);
       printf("Fail malloc memory\n");
       return -6; 
	}

	output_resid =(char *)malloc(buf_size_calc);
	if (output_resid == NULL)
	{
	   free(buffer);
       free(output_text);
       fclose(fp);
       printf("Fail malloc memory\n");
       return -6; 
	}

	output_format =(char *)malloc(buf_size_calc);
	if (output_format == NULL)
	{
	   free(buffer);
       free(output_text);
	   free(output_resid);	   
       fclose(fp);
       printf("Fail malloc memory\n");
       return -6; 
	}
	    	
	//fread(buffer,1,buf_size,fp);
	fread(buffer,buf_size,1,fp);
	if (ferror(fp) !=0) 
	{
		fclose(fp);
        printf("Fail to read the file\n");
        return -7;
	}
	fclose(fp);

	/* Now we can write our parser */
	
	paraser1(buffer, buf_size, output_text, output_resid, output_format,"UTF-8");
	// printf ("%s",output_format);

	/* Now we convert to utf-8 */
	memset(output_resid,0,buf_size_calc);
	buf_size_calc = ansiCodePage(atoi(argv[2]), output_format, output_resid, strlen(output_format));

	if ((Outfp = fopen(argv[3],"wb"))  != NULL )
	{
         fwrite(output_resid,1,buf_size_calc,Outfp);
	     fclose(Outfp);
	}	
	

	
	 	
	if(buffer!=NULL)
     free(buffer);
    if(output_text!=NULL)
     free(output_text);
    if(output_resid!=NULL)
     free(output_resid);
    if(output_format!=NULL)
     free(output_format);
	

	return 0;	
}

int paraser1(unsigned char *buf, long buf_size,	unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format, unsigned char *iso_type)
{
   unsigned char *row; 
   long foundPos=0;
   long foundNextPos=0;
   long row_size=0;
   long pos=0;

   memset(output_text,0,buf_size);
   memset(output_resid,0,buf_size);
   memset(output_format,0,buf_size);

   sprintf(output_format,"<?xml version=\"1.0\" encoding=\"%s\"?>\n<resource>\n",iso_type);
   
   row = output_text;
   while(pos < buf_size)
   {
	  foundPos=0;
	  foundNextPos=0;
      row_size=0;

	  /* create a row string so we can easy scan it */
	  find_str('\n',&buf[pos],&foundPos);
	  
	  if (foundPos !=0)
	  {		     
		 row_size = foundPos - 1;

		 /* found a new row */
	     strncpy(row, &buf[pos], row_size);		   
		 pos+=foundPos;
		 if (foundPos >=2) 
		     row[row_size -1]=0;	

	   }
	   else
	   {          
		   row_size = buf_size - pos;

		   /* no new row found in the buffer */
           strncpy(row, &buf[pos], buf_size - pos);		   
		   pos= buf_size;
	   }
       
	   trim(row);		   
	   foundPos=0;	   

	   /* Detect Basic command and send it to own paraser */
	   if (*row==0)
		   continue;

	   if (strncmp("/*",row,2)==0) 
       {			 
		  ParserComment(&pos, buf, buf_size, output_text, output_resid, output_format);
		  continue;
	   }

	   if (strncmp("//",row,2)==0)
       {
          ParserComment(&pos, buf, buf_size, output_text, output_resid, output_format);
		  continue;
	   }
	   if (strncmp("#",row,1)==0)
	   {
          ParserComment(&pos, buf, buf_size, output_text, output_resid, output_format);
		  continue;
	   }

	   stringbugs(row,true);	
			   
	   if (foundPos == 0)
	   {
	        find_str2 ("LANGUAGE ",row,&foundPos,output_resid,output_text);
            if (foundPos != 0)  
            {			  
			  ParserLang("LANGUAGE", output_resid, output_text, output_format);
			  continue;
		    }
		}
	    	
		if (foundPos == 0)
		{
			find_str2 ("STRINGTABLE ",row,&foundPos,output_resid,output_text);
            if (foundPos != 0)  
            {			   
              ParserCMD3("STRINGTABLE", output_resid, output_text, output_format);
			  ParserString(&pos, buf, buf_size,	output_text, output_resid, output_format);
			  continue;
		    }
		}
        	
		if (foundPos == 0)
		{
			find_str2 (" DIALOGEX ",row,&foundPos,output_resid,output_text);
            if (foundPos != 0)  
            {
			  ParserCMD2("DIALOGEX", output_resid, output_text, output_format);
			  ParserDialog("DIALOGEX",&pos, buf, buf_size,	output_text, output_resid, output_format);
			  continue;
		    }
		}
		
		if (foundPos == 0)
		{
			find_str2 (" DIALOG ",row,&foundPos,output_resid,output_text);
            if (foundPos != 0)  
            {
              ParserCMD2("DIALOG", output_resid, output_text, output_format);
			  ParserDialog("DIALOG",&pos, buf, buf_size,	output_text, output_resid, output_format);
			  continue;
		    }
		}
        
		if (foundPos == 0)
		{
			find_str2 (" ACCELERATORS\0",row,&foundPos,output_resid,output_text);
            if (foundPos != 0)  
            {
			  ParserCMD1("ACCELERATORS", output_resid, output_text, output_format);
			  ParserAccelerators(&pos, buf, buf_size,	output_text, output_resid, output_format);
			  continue;
		    }
		}

		if (foundPos == 0)
		{
			find_str2 (" MENU\0",row,&foundPos,output_resid,output_text);
            if (foundPos != 0)  
            {
              ParserCMD1("MENU", output_resid, output_text, output_format);
			  ParserMenu("MENU",&pos, buf, buf_size,	output_text, output_resid, output_format);
			  continue;
		    }
		}

		
   } // end while 
   sprintf(output_format,"%s</resource>\n",output_format);
   return false;
}

/*
  ParserCMD
  data  
  input : IDM_MDIFRAME MENU DISCARDABLE LANG LANG_TAG LANG_TAG
  input : IDM_MDIFRAME MENU DISCARDABLE 
  input : IDM_MDIFRAME MENU 
  input : IDM_MDIFRAME ACCELERATORS DISCARDABLE LANG LANG_TAG LANG_TAG
  input : IDM_MDIFRAME ACCELERATORS DISCARDABLE 
  input : IDM_MDIFRAME ACCELERATORS 


  output : <obj type="MENU" rc_name="ID">DISCARDABLE</obj>
  output : <obj type="MENU" rc_name="ID">DISCARDABLE</obj>
  output : <obj type="MENU" rc_name="ID"></obj>
  output : <obj type="ACCELERATORS" rc_name="ID">DISCARDABLE</obj>
  output : <obj type="ACCELERATORS" rc_name="ID">DISCARDABLE</obj>
  output : <obj type="ACCELERATORS" rc_name="ID"></obj>

  param : output_resid = rc_name ID 
  param : output_text  = MENU DISCARDABLE LANG LANG_TAG LANG_TAG
  param : text  = type example MENU
  param : output_format  = xml data store buffer
*/

void ParserCMD1(unsigned char *text, unsigned char *output_resid, unsigned char *output_text, unsigned char *output_format)
{
  long le;  
  
  stringbugs(output_resid,false);
  stringbugs(output_text,false);
  
  le = strlen(text);

  if (strlen(output_text) == le)
  {
	 sprintf(output_format,"%s<group name=\"%s\">\n  <obj type=\"%s\" rc_name=\"%s\"></obj>\n",output_format,text,text,output_resid);
  }
  else if (output_text[le]==' ')
  {    
     sprintf(output_format,"%s<group name=\"%s\">\n  <obj type=\"%s\" rc_name=\"%s\">DISCARDABLE</obj>\n",output_format,text,text,output_resid);
  }

}

/*
  ParserCMD2
  data  
  input : IDM_MDIFRAME DIALOG DISCARDABLE  15, 13, 210, 63 LANG LANG_TAG LANG_TAG
  input : IDM_MDIFRAME DIALOG DISCARDABLE  15, 13, 210, 63
  input : IDM_MDIFRAME DIALOGEX DISCARDABLE  15, 13, 210, 63 LANG LANG_TAG LANG_TAG
  input : IDM_MDIFRAME DIALOGEX DISCARDABLE  15, 13, 210, 63


  output : <obj type="DIALOG" rc_name="ID" top="15" left="13" right="210" bottom="63">DISCARDABLE</obj>
  output : <obj type="DIALOG" rc_name="ID" top="15" left="13" right="210" bottom="63"></obj>
  output : <obj type="DIALOGEX" rc_name="ID" top="15" left="13" right="210" bottom="63">DISCARDABLE</obj>
  output : <obj type="DIALOGEX" rc_name="ID" top="15" left="13" right="210" bottom="63"></obj>


  param : output_resid = rc_name ID 
  param : output_text  =  DIALOG DISCARDABLE  15, 13, 210, 63 LANG LANG_TAG LANG_TAG
  param : text  = type example DIALOG
  param : output_format  = xml data store buffer
  
*/

void ParserCMD2(unsigned char *text, unsigned char *output_resid, unsigned char *output_text, unsigned char *output_format)
{
	long le;
	long flag = 0;
	
	stringbugs(output_resid,false);
    stringbugs(output_text,false);

	le=strlen(text);
	
	sprintf(output_format,"%s<group name=\"%s\">\n  <obj type=\"%s\" rc_name=\"%s\" ",output_format,text,text,output_resid);

    find_str2 (" DISCARDABLE ",output_text,&flag,output_resid,output_text);
	trim(output_resid);
	trim(output_text);
	if (flag==0)
	{
		*output_resid='\0'; /* not in use futer */
		flag=0; /*	DISCARDABLE off */
		sprintf(output_text,"%s",&output_text[le]);
		trim(output_text);		
	}
	else
	{
		*output_resid='\0'; /* not in use futer */
		flag=1;  /*	DISCARDABLE on */
		sprintf(output_text,"%s",&output_text[11]);
		trim(output_text);
	}
	
   /* data is looking like this 0 1 2 3 now */
	 
   trim(output_resid);
   trim(output_text);	
   find_str2 (" ",output_text,&flag,output_resid,output_text);
   trim(output_resid);
   trim(output_text);

   sprintf(output_format,"%sleft=\"%s\" ",output_format,output_resid);

   trim(output_resid);
   trim(output_text);	
   find_str2 (" ",output_text,&flag,output_resid,output_text);
   trim(output_resid);
   trim(output_text);

   sprintf(output_format,"%stop=\"%s\" ",output_format,output_resid);

   trim(output_resid);
   trim(output_text);	
   find_str2 (" ",output_text,&flag,output_resid,output_text);
   trim(output_resid);
   trim(output_text);

   if (flag==0)
      sprintf(output_format,"%swidth=\"%s\" height=\"%s\"></obj>\n",output_format,output_resid,output_text);
   else
      sprintf(output_format,"%swidth=\"%s\" height=\"%s\">DISCARDABLE</obj>\n",output_format,output_resid,output_text);
}

/*
  ParserCMD3
  data  
  input : STRINGTABLE DISCARDABLE LANG LANG_TAG LANG_TAG
  input : STRINGTABLE DISCARDABLE LANG 
  input : STRINGTABLE LANG LANG_TAG LANG_TAG
  input : STRINGTABLE 


  output : <obj type="STRINGTABLE">DISCARDABLE</obj>
  output : <obj type="STRINGTABLE">DISCARDABLE</obj>
  output : <obj type="STRINGTABLE"></obj>
  output : <obj type="STRINGTABLE"></obj>


  param : output_resid = empty
  param : output_text  =  DIALOG DISCARDABLE  15, 13, 210, 63 LANG LANG_TAG LANG_TAG
  param : text  = type example DIALOG
  param : output_format  = xml data store buffer
  
*/
void ParserCMD3(unsigned char *text, unsigned char *output_resid, unsigned char *output_text, unsigned char *output_format)
{  
  long foundPos=0;

  stringbugs(output_resid,false);
  stringbugs(output_text,false);

  find_str2 (" ",output_text,&foundPos,output_resid,output_text);
  trim(output_resid);
  trim(output_text);

   if(strncmp("STRINGTABLE",output_text,11))
      sprintf(output_format,"%s<group name=\"%s\">\n  <obj type=\"%s\">DISCARDABLE</obj>\n",output_format,output_text,output_resid);
   else
      sprintf(output_format,"%s<group name=\"%s\">\n  <obj type=\"%s\"></obj>\n",output_format,output_resid,output_text);

}
/*
  ParserLang
  data  
  input : LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US
  input : LANGUAGE LANG_ENGLISH SUBLANG_ENGLISH_US
  output : <obj type="LANG" sublang="sub">lang</obj>
  output : <obj type="LANG" sublang="sub">lang</obj>

  param : output_resid = not in use
  param : output_text  =   LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US
  param : text  = type example LANGUAGE
  param : output_format  = xml data store buffer
*/

void ParserLang(unsigned char *text, unsigned char *output_resid, unsigned char *output_text, unsigned char *output_format)
{
  long foundPos=0;
    
  stringbugs(output_resid,false);
  stringbugs(output_text,false);	
  
  sprintf(output_text,"%s",&output_text[strlen(text)+1]);

  /* split the lang into two string */ 
  find_str2 (" ",output_text,&foundPos,output_resid,output_text);
  trim(output_resid);
  trim(output_text);
  sprintf(output_format,"%s<group name=\"%s\">\n  <obj type=\"%s\" sublang=\"%s\">%s</obj>\n</group>\n",output_format,text,text,output_text,output_resid);
}


/*
  ParserComment
  data  
  input : / *  sadasdas asdasd  asdas ... * /  
  output :<obj type=\"COMMENT\"><![CDATA[ sadasdas asdasd  asdas ... ]]></obj>

  input : / *  sadasdas asdasd  asdas ... * /  
  output :<obj type=\"COMMENT\"><![CDATA[ sadasdas asdasd  asdas ... ]]></obj>

  input : #if x
  output :<obj type=\"COMMENT\"><![CDATA[#if x]]></obj>

  input : // hi
  output :<obj type=\"COMMENT\"><![CDATA[// hi]]></obj>
  
  param : pos = current buf position 
  param : buf = read in buffer from file rc
  param : buf_size  = buf max size
  param : output_text  = using internal instead alloc more memory
  param : output_resid = using internal instead alloc more memory
  param : output_format  = xml data store buffer
*/

void ParserComment(long *pos, unsigned char *buf, long buf_size, unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format)
{
  long foundPos=0;
  long foundNextPos=0;
  long row_size=0;
  unsigned char *row = output_text;
  

  row_size = strlen(row);
  if (strncmp("//",&row[0],2)==0)
  {     	   
       sprintf(output_format,"%s<group name=\"COMMENT\">\n  <obj type=\"COMMENT\"><![CDATA[%s]]></obj>\n</group>\n",output_format,row);
       return;
  }	   
  if (strncmp("#",&row[0],1)==0)
  {   
       sprintf(output_format,"%s<group name=\"COMMENT\">\n  <obj type=\"COMMENT\"><![CDATA[%s]]></obj>\n</group>\n",output_format,row);
       return;
  }

  for (foundNextPos=0;foundNextPos<row_size;foundNextPos++)
  {
       if (strncmp("*/",&row[foundNextPos],2)==0)
       {
		   //*pos=*pos+foundNextPos+1;
		   //*pos=(*pos- (row_size+foundNextPos+2))+foundNextPos+2;
           row[foundNextPos+2]='\0';
           sprintf(output_format,"%s<group name=\"COMMENT\">\n  <obj type=\"COMMENT\"><![CDATA[%s]]></obj>\n</group>\n",output_format,row);
           return;
       }	
	  
  }
  

  sprintf(output_format,"%s<group name=\"COMMENT\">\n  <obj type=\"COMMENT\"><![CDATA[%s\n",output_format,output_text);

 while(*pos < buf_size)
  {
	  foundPos=0;	  
      row_size=0;

	  /* create a row string so we can easy scan it */
	  find_str('\n',&buf[*pos],&foundPos);	  
	  if (foundPos !=0)
	  {		     
		 row_size = foundPos - 1;

		 /* found a new row */
	     strncpy(row, &buf[*pos], foundPos);		   
		 *pos+=foundPos;
		 if (foundPos >=2) 
		 {
		     row[row_size -1]=0;				
		 }

	   }
	   else
	   {          
		   row_size = buf_size - *pos;

		   /* no new row found in the buffer */
           strncpy(row, &buf[*pos], buf_size - *pos);		   
		   *pos= buf_size;
	  }	

       /* Search now after end of comment */ 	   	   
	   row_size=strlen(row);
	   for (foundNextPos=0;foundNextPos<row_size;foundNextPos++)
	   {
			if (strncmp("*/",&row[foundNextPos],2)==0)
			{
				row_size=row_size - (foundNextPos+2);
				*pos-=row_size;
                row[foundNextPos+2]='\0';
				sprintf(output_format,"%s%s]]></obj>\n</group>\n",output_format,row);
				return;
			}	   
	   }	   	          	
	   sprintf(output_format,"%s%s\n",output_format,row);
    }

}

/*
  ParserAccelerators
  data    
  input  : BEGIN
  input  : "^A", CMD_SELECT_ALL
  input  : END

  output : <obj type="ACCELERATORS" command="BEGIN" /> 
  output : <obj type="ACCELERATORS" rc_name="CMD_SEARCH"><![CDATA[^F]]></obj>
  output : <obj type="ACCELERATORS" command="END" />
  
  param : pos = current buf position 
  param : buf = read in buffer from file rc
  param : buf_size  = buf max size
  param : output_text  = using internal instead alloc more memory
  param : output_resid = using internal instead alloc more memory
  param : output_format  = xml data store buffer
*/
void ParserAccelerators(long *pos, unsigned char *buf, long buf_size, unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format)
{
  long foundPos=0;
  long foundNextPos=0;
  long row_size=0;
  char *row = output_text;
  int start=false;
  long le;

  while(*pos < buf_size)
  {
	  foundPos=0;	  
      row_size=0;

	  /* create a row string so we can easy scan it */
	  find_str('\n',&buf[*pos],&foundPos);	  
	  if (foundPos !=0)
	  {		     
		 row_size = foundPos - 1;

		 /* found a new row */
	     strncpy(row, &buf[*pos], foundPos);		   
		 *pos+=foundPos;
		 if (foundPos >=2) 
		 {
		     row[row_size -1]=0;				
		 }

	   }
	   else
	   {          
		   row_size = buf_size - *pos;

		   /* no new row found in the buffer */
           strncpy(row, &buf[*pos], buf_size - *pos);		   
		   *pos= buf_size;
	  }	   

	  stringbugs(row,true);	  
      if (start == false)
	  {
		if ((strcmp(row,"BEGIN")==0) || (strcmp(row,"{")==0))
		{
		   start=true;
		   sprintf(output_format,"%s  <obj type=\"ACCELERATORS\" command=\"BEGIN\" />\n",output_format);
		   
	    }
		continue;
	  }
	  		  
	  if ((strcmp(row,"END")==0) || (strcmp(row,"}")==0))
	  {
		  sprintf(output_format,"%s  <obj type=\"ACCELERATORS\" command=\"END\" />\n</group>\n",output_format);

		  *output_resid = '\0';
		  break;
	  }

	  foundPos=0;
	  foundNextPos=0;
	  find_str('"',row,&foundPos);
      find_str('"',&row[foundPos],&foundNextPos);

	  if ((foundPos!=0) && (foundNextPos!=0))
	  {	      
	     
         sprintf(output_format,"%s  <obj type=\"KEY\" rc_name=\"",output_format); 
		 le = strlen(output_format);
		 sprintf(output_format,"%s%s",output_format,&row[foundNextPos+foundPos]); 
         trim(&output_format[le]); 
		 row[foundNextPos+foundPos]='\0';
		 row[foundPos-1]=' ';
		 foundPos=0;
		 find_str('"',row,&foundPos);
		 if (foundPos!=0)
		 {
			 row[foundPos-1]=' ';
		 }
		 	 
		 trim(row);
		 sprintf(output_format,"%s\"><![CDATA[%s]]></obj>\n",output_format,row); 
	  }	  
  }
}

/*
  ParserString
  data    
  input  : BEGIN
  input  : IDS_HINT_BLANK  "text"  
  input  : END

  output : <obj type="STRINGTABLE" command="BEGIN" /> 
  output : <obj type="STRING" rc_name="rc_id">text</obj>
  output : <obj type="STRINGTABLE" command="END" />
  
  param : pos = current buf position 
  param : buf = read in buffer from file rc
  param : buf_size  = buf max size
  param : output_text  = using internal instead alloc more memory
  param : output_resid = using internal instead alloc more memory
  param : output_format  = xml data store buffer
*/
void ParserString(long *pos, unsigned char *buf, long buf_size,	unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format)
{
  long foundPos=0;  
  long row_size=0;
  unsigned char *row = output_text;
  int start=false;

  while(*pos < buf_size)
  {
	  foundPos=0;
	  row_size=0;

	  	  /* create a row string so we can easy scan it */
	  find_str('\n',&buf[*pos],&foundPos);	
	
	  if (foundPos !=0)
	  {		     
		 row_size = foundPos - 1;

		 /* found a new row */
	     strncpy(row, &buf[*pos], foundPos);		   
		 *pos+=foundPos;
		 if (foundPos >=2) 
		 {
		     row[row_size -1]=0;				
		 }

	   }
	   else
	   {          
		   row_size = buf_size - *pos;

		   /* no new row found in the buffer */
           strncpy(row, &buf[*pos], buf_size - *pos);		   
		   *pos= buf_size;
	  }	   
  
      stringbugs(row,true);	

	 
	  if (start == false)
	  {
		if ((strcmp(row,"BEGIN")==0) || (strcmp(row,"{")==0))
		{
			
			start=true;
			sprintf(output_format,"%s  <obj type=\"STRINGTABLE\" command=\"BEGIN\" />\n",output_format);		 
		}
			continue;
	  }
	 
	  if ((strcmp(row,"END")==0) || (strcmp(row,"}")==0))
	  {
			sprintf(output_format,"%s  <obj type=\"STRINGTABLE\" command=\"END\" />\n</group>\n",output_format);

			*output_resid = '\0';
			break;
	  }

	 

	  /* the split code here */
      foundPos=0;		   
	  find_str2 (" ",row,&foundPos,output_resid,output_text);
	  
	  if (foundPos != 0)  
      {		  		  
		  trim(output_text);
		  trim(output_resid);			  
		    
		  if (*output_resid!='\0')
              sprintf(output_format,"%s  <obj type=\"STRING\" rc_name=\"%s\"><![CDATA[%s]]></obj>\n",output_format,output_resid,output_text);
          else
	          sprintf(output_format,"%s  <obj type=\"STRING\" rc_name=\"%s\"></obj>\n",output_format,output_resid);
	  }

  }
}

/*
  ParserDialog
  data    
  
  input  : BEGIN
  output : <obj type="DIALOG" command="BEGIN" />
  output : <obj type="DIALOGEX" command="BEGIN" />

  input  : END
  output : <obj type="END" command="BEGIN" />
  output : <obj type="END" command="BEGIN" />

  input  : {
  output : <obj type="DIALOG" command="BEGIN" />
  output : <obj type="DIALOGEX" command="BEGIN" />

  input  : }
  output : <obj type="END" command="BEGIN" />
  output : <obj type="END" command="BEGIN" />

  input  : FONT 8, "MS Shell Dlg"  
  output : <obj type="FONT" size="8" name="MS Shell Dlg"></obj>

  input  : FONT 8, "MS Shell Dlg", 0, 0, 0x1
  output : <obj type="FONT" size="8" name="MS Shell Dlg">0  0  0x1</obj>
  
  input  : CONTROL         "",101,"Static",SS_SIMPLE | SS_NOPREFIX,3,6,150,10  
  output : <obj type="CONTROL" rc_name="IDC_ICON_ALIGN_1" prop="Button" style="BS_OWNERDRAW |BS_BOTTOM | WS_TABSTOP" top="57" left="25" right="46" bottom="44"><![CDATA[left/top right]]></obj>

   
  Builder1
  input  : DEFPUSHBUTTON   "&OK",1,158,6,47,14 xx
  input  : PUSHBUTTON      "&Cancel",2,158,23,47,14 xx
  input  : LTEXT           "&Tooltip Text:",IDC_LABEL1,7,44,40,8 xx
  input  : GROUPBOX        "&Display Mode",IDC_LABEL4,7,96,157,28 xx
  input  : ICON            "",IDC_PICTURE,173,101,21,20 xx

  input  : EDITTEXT        201,3,29,134,12,ES_AUTOHSCROLL  
  input  : LISTBOX         IDC_LIST, 4, 16, 104, 46, WS_TABSTOP
  input  : COMBOBOX        ID_EOLN,54,18,156,80,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP

  output : <obj type="COMBOBOX" rc_name="ID_ENCODING" top="54" left="0" right="156" bottom="80" style="CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP"></obj>
  output : <obj type="COMBOBOX" rc_name="ID_ENCODING" top="54" left="0" right="156" bottom="80"></obj>

  output : <obj type="GROUPBOX" rc_name="IDC_LABEL4" top="7" left="96" right="157" bottom="28" style="CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP"><![CDATA[&Display Mode]]></obj>
  output : <obj type="GROUPBOX" rc_name="IDC_LABEL4" top="7" left="96" right="157" bottom="28"><![CDATA[&Display Mode]]></obj>
    
  builder2
  input  : CAPTION "Execute"     
  input  : EXSTYLE WS_EX_APPWINDOW
  input  : STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU

  output : <obj type="STYLE">DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU</obj>

  param : pos = current buf position 
  param : buf = read in buffer from file rc
  param : buf_size  = buf max size
  param : output_text  = using internal instead alloc more memory
  param : output_resid = using internal instead alloc more memory
  param : output_format  = xml data store buffer
*/
void ParserDialog(unsigned char *text, long *pos, unsigned char *buf, long buf_size,	unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format)
{
  long foundPos=0;
  long foundNextPos=0;
  long row_size=0;
  unsigned char *row = output_text; 
  long commandfound=0;
  long le;

  *output_resid='\0';
  le=0;

  while(*pos < buf_size)
  {
	  foundPos=0;	  
      row_size=0;

	  /* create a row string so we can easy scan it */
	  find_str('\n',&buf[*pos],&foundPos);	  
	  if (foundPos !=0)
	  {		     
		 row_size = foundPos - 1;

		 /* found a new row */
	     strncpy(row, &buf[*pos], foundPos);		   
		 *pos+=foundPos;
		 if (foundPos >=2) 
		 {
		     row[row_size -1]=0;				
		 }

	   }
	   else
	   {          
		   row_size = buf_size - *pos;

		   /* no new row found in the buffer */
           strncpy(row, &buf[*pos], buf_size - *pos);		   
		   *pos= buf_size;
	  }	   

	  //stringbugs(row,true);
	  trim(row);
     
	  if ((strcmp(row,"BEGIN")==0) || (strcmp(row,"{")==0))
	     commandfound=1;
	  if ((strcmp(row,"END")==0) || (strcmp(row,"}")==0))
	     commandfound=2;

	  if (strncmp("STYLE ",row,6)==0)
	     commandfound=3;
	  if (strncmp("CAPTION ",row,8)==0)
	     commandfound=3;
	  if (strncmp("FONT ",row,5)==0)
	     commandfound=3;
	  if (strncmp("CONTROL ",row,8)==0)
	     commandfound=3;
	  if (strncmp("EDITTEXT ",row,9)==0)
	     commandfound=3;
	  if (strncmp("DEFPUSHBUTTON ",row,14)==0)
	     commandfound=3;
	  if (strncmp("PUSHBUTTON ",row,11)==0)
	     commandfound=3;
	  if (strncmp("LTEXT ",row,6)==0)
	     commandfound=3;
	  if (strncmp("GROUPBOX ",row,9)==0)
	     commandfound=3;
	  if (strncmp("ICON ",row,5)==0)
	     commandfound=3;
	  if (strncmp("EXSTYLE ",row,8)==0)
	     commandfound=3;
	  if (strncmp(row,"LISTBOX ",8)==0) 		  
         commandfound=3;
	  if (strncmp(row,"COMBOBOX ",9)==0) 		  
         commandfound=3;

      if ((*output_resid!=0) && (commandfound!=0))
	  {
		  /* Builder 1*/
          if (strncmp(output_resid,"LTEXT ",6)==0)		  
             DialogCMDBuild1(output_resid, output_format, 5, "LTEXT");
          if (strncmp(output_resid,"GROUPBOX ",9)==0)
              DialogCMDBuild1(output_resid, output_format, 8, "GROUPBOX");
		  if (strncmp(output_resid,"DEFPUSHBUTTON ",14)==0)
			  DialogCMDBuild1(output_resid, output_format, 13, "DEFPUSHBUTTON");
          if (strncmp(output_resid,"PUSHBUTTON ",11)==0)
			  DialogCMDBuild1(output_resid, output_format, 10, "PUSHBUTTON");
		  if (strncmp("ICON ",output_resid,5)==0)
			  DialogCMDBuild1(output_resid, output_format, 4, "ICON");
		  if (strncmp("EDITTEXT ",output_resid,9)==0)
			  DialogCMDBuild1(output_resid, output_format, 8, "EDITTEXT");
		  if (strncmp("LISTBOX ",output_resid,8)==0)
			  DialogCMDBuild1(output_resid, output_format, 7, "LISTBOX");
		  if (strncmp("COMBOBOX ",output_resid,9)==0)
			  DialogCMDBuild1(output_resid, output_format, 8, "COMBOBOX");
          
		  /* Builder 2*/
		  if (strncmp("STYLE ",output_resid,6)==0)
		      DialogCMDBuild2(output_resid, output_format, 5, "STYLE");
		  if (strncmp("EXSTYLE ",output_resid,8)==0)
		      DialogCMDBuild2(output_resid, output_format, 7, "EXSTYLE");
		  if (strncmp("CAPTION ",output_resid,8)==0)
			  DialogCMDBuild2(output_resid, output_format, 7, "CAPTION");
		  if (strncmp("CONTROL ",output_resid,8)==0)
			  DialogCMDBuild3(output_resid, output_format, 7, "CONTROL");

		  /* no builder */
		  if (strncmp(output_resid,"FONT ",5)==0) 
		  {
             stringbugs(output_resid,true);
			 /* FONT */
			 sprintf(output_resid,"%s",&output_resid[5]);
			 trim(output_resid);
			 sprintf(output_format,"%s  <obj type=\"FONT\" size=\"",output_format);
			 le = strlen(output_format);
             sprintf(output_format,"%s%s",output_format,output_resid);
             
			 foundPos=0;
			 find_str('\"',output_resid,&foundPos);			 
			 output_format[le+foundPos-1]='\0';
			 trim(&output_format[le]);
			 sprintf(output_format,"%s\" name=",output_format);
             le = strlen(output_format);
			 sprintf(output_format,"%s%s",output_format,&output_resid[foundPos-1]);
             
			 foundNextPos=0;
			 find_str('\"',&output_resid[foundPos],&foundNextPos);
			 output_format[le+foundPos+foundNextPos-1]='\0';
			 trim(&output_format[le+foundPos]);
			 if (output_resid[foundPos+foundNextPos]=='\0')
			 {				  
				sprintf(output_format,"%s></obj>\n",output_format);
			 }
			 else
			 {
				 sprintf(output_format,"%s\">%s</obj>\n",output_format,&output_resid[foundPos]);
			 }

             *output_resid=0;
		  }

	    *output_resid='\0';
	  }
	 		 
	  if (commandfound==1)
	  {
		  sprintf(output_format,"%s  <obj type=\"%s\" command=\"BEGIN\" />\n",output_format,text);
	  }
	  if (commandfound==2)
	  {
		  sprintf(output_format,"%s  <obj type=\"%s\" command=\"END\" />\n</group>\n",output_format,text);
		  break;
	  }

	  sprintf(output_resid,"%s%s",output_resid,row);
	  commandfound=0;      
  }

}
//////////////////////////
/*
  ParserDialog
  data    
  
  input  : BEGIN
  output : <obj type="DIALOG" command="BEGIN" />
  output : <obj type="DIALOGEX" command="BEGIN" />

  input  : END
  output : <obj type="END" command="BEGIN" />
  output : <obj type="END" command="BEGIN" />

  input  : {
  output : <obj type="DIALOG" command="BEGIN" />
  output : <obj type="DIALOGEX" command="BEGIN" />

  input  : }
  output : <obj type="END" command="BEGIN" />
  output : <obj type="END" command="BEGIN" />
 
  param : pos = current buf position 
  param : buf = read in buffer from file rc
  param : buf_size  = buf max size
  param : output_text  = using internal instead alloc more memory
  param : output_resid = using internal instead alloc more memory
  param : output_format  = xml data store buffer
*/
void ParserMenu(unsigned char *text, long *pos, unsigned char *buf, long buf_size, unsigned char * output_text, unsigned char * output_resid, unsigned char * output_format)
{
  long foundPos=0;
  long foundNextPos=0;
  long row_size=0;
  unsigned char *row = output_text; 
  long commandfound=0;
  long le;
  long count=0;

  *output_resid='\0';
  le=0;

  while(*pos < buf_size)
  {
	  foundPos=0;	  
      row_size=0;

	  /* create a row string so we can easy scan it */
	  find_str('\n',&buf[*pos],&foundPos);	  
	  if (foundPos !=0)
	  {		     
		 row_size = foundPos - 1;

		 /* found a new row */
	     strncpy(row, &buf[*pos], foundPos);		   
		 *pos+=foundPos;
		 if (foundPos >=2) 
		 {
		     row[row_size -1]=0;				
		 }

	   }
	   else
	   {          
		   row_size = buf_size - *pos;

		   /* no new row found in the buffer */
           strncpy(row, &buf[*pos], buf_size - *pos);		   
		   *pos= buf_size;
	  }	   

	  //stringbugs(row,true);
	  stringbugs2(row,true);
     
	  if ((strcmp(row,"BEGIN")==0) || (strcmp(row,"{")==0))
	     commandfound=1;
	  if ((strcmp(row,"END")==0) || (strcmp(row,"}")==0))
	     commandfound=2;

	  if (strncmp("POPUP ",row,6)==0)
	     commandfound=3;
	  if (strncmp("MENUITEM ",row,8)==0)
	     commandfound=3;
	 

      if ((*output_resid!=0) && (commandfound!=0))
	  {		   
          if (strncmp(output_resid,"POPUP ",6)==0)		  
		  {
			 sprintf(output_resid,"%s",&output_resid[5]);   
             trim(output_resid);			 
			 sprintf(output_format,"%s<obj type=\"POPUP\"><![CDATA[%s]]></obj>\n",output_format,output_resid);   
             *output_resid='\0'; 
		  }

		  if (strncmp(output_resid,"MENUITEM ",9)==0)
		  {
              sprintf(output_resid,"%s",&output_resid[8]);			  
			  trim(output_resid);
			  if (strcmp(output_resid,"SEPARATOR")==0)
			  {
				 sprintf(output_format,"%s<obj type=\"MENUITEMSEPERATOR\"></obj>\n",output_format);   
				 *output_resid='\0';
			  }
			  else
			  {
				  foundPos=0;
				  foundNextPos=0;
				  find_str('"',output_resid,&foundPos);
				  find_str('"',&output_resid[foundPos],&foundNextPos);
                  
				  stringbugs(&output_resid[foundPos+foundNextPos],true);

				  if ((foundPos+foundNextPos)==0)
				  {
					 sprintf(output_format,"%s<obj type=\"MENUITEM\" rc_name=\"%s\"></obj>\n",output_format,&output_resid[foundPos+foundNextPos]);   
				  }
				  else
				  {
				    sprintf(output_format,"%s<obj type=\"MENUITEM\" rc_name=\"%s\">",output_format,&output_resid[foundPos+foundNextPos]);   
					
					output_resid[foundPos+foundNextPos]='\0';
					trim(output_resid);
						                   
                    sprintf(output_format,"%s<![CDATA[%s]]></obj>\n",output_format,output_resid);    
				  }
                   


                 *output_resid='\0';
			  }
		  }

      
		 
	    *output_resid='\0';
	  }
	 		 
	  if (commandfound==1)
	  {
		  count++;
		  if (count==1)
		      sprintf(output_format,"%s<obj type=\"%s\" command=\"BEGIN\" />\n",output_format,text);
		  else
			  sprintf(output_format,"%s<obj type=\"POPUP\" command=\"BEGIN\" />\n",output_format);

		  *output_resid='\0';
	  }
	  if (commandfound==2)
	  {
		  count--;
		  *output_resid='\0';

		  if (count<1)
		      sprintf(output_format,"%s<obj type=\"%s\" command=\"END\" />\n",output_format,text);
		  else
			  sprintf(output_format,"%s<obj type=\"POPUP\" command=\"END\" />\n",output_format);
		  
		  if (count<1)
		  {
			  sprintf(output_format,"%s</group>\n",output_format);
			  break;
		  }
	  }

	  sprintf(output_resid,"%s%s",output_resid,row);
	  commandfound=0;      
  }

}

void stringbugs(unsigned char *buf, int shift2)
{
  long foundPos=0;
  long foundNextPos=0;
  long t=0;
   		  
  /* remove , */
  if (shift2== false)
  {
	for (t=0;t<strlen(buf);t++)
	{  
	  if (foundPos==0)
	  {
	     if (strncmp(",",&buf[t],1)==0)
	     {
		     buf[t]=' ';
	     }

	     if (strncmp("\"",&buf[t],1)==0)
	     {
		     buf[t]=' ';
	     }	  
	  
	     if (strncmp("/*",&buf[t],2)==0)
	     {
		      foundPos=t;
			  buf[t]=' ';
			  buf[t+1]=' ';
	     }
	  }
	  else
	  {
		 if (strncmp("*/",&buf[t],2)==0)
	     {
		   buf[t]=' ';
		   buf[t+1]=' ';
		   foundPos=0;
	     }
		 else
		 {
           buf[t]=' ';
		 }
	  }	  	 
	}
  }
  else
  {
    /* shift */
	for (t=0;t<strlen(buf);t++)
	{  
	  if ((foundPos==0) && (foundNextPos==0))
	  {
	     if (strncmp(",",&buf[t],1)==0)
	     {
		     buf[t]=' ';
	     }

	     if (strncmp("\"",&buf[t],1)==0)
	     {
		    foundNextPos=t; 
	     }	  
	  
	     if (strncmp("/*",&buf[t],2)==0)
	     {
		      foundPos=t;
			  buf[t]=' ';
			  buf[t+1]=' ';
	     }
	  }
	  else
	  {
	     if (foundPos!=0)
		 {
			if (strncmp("*/",&buf[t],2)==0)
			{
				buf[t]=' ';
				buf[t+1]=' ';
				foundPos=0;
			}
			else
			{
				buf[t]=' ';
			}
		 }
		 if (foundNextPos!=0)
		 {
			if (strncmp("\"",&buf[t],1)==0)
			{				
				foundNextPos=0;
			}			
		 }
	  }
	}
  }

  trim(buf);
  /* have remove all wrong syntax */   
}

void stringbugs2(unsigned char *buf, int shift2)
{
  long foundPos=0;
  long foundNextPos=0;
  long t=0;
   		  
  /* remove , */
  if (shift2== false)
  {
	for (t=0;t<strlen(buf);t++)
	{  
	  if (foundPos==0)
	  {

	     if (strncmp("\"",&buf[t],1)==0)
	     {
		     buf[t]=' ';
	     }	  
	  
	     if (strncmp("/*",&buf[t],2)==0)
	     {
		      foundPos=t;
			  buf[t]=' ';
			  buf[t+1]=' ';
	     }
	  }
	  else
	  {
		 if (strncmp("*/",&buf[t],2)==0)
	     {
		   buf[t]=' ';
		   buf[t+1]=' ';
		   foundPos=0;
	     }
		 else
		 {
           buf[t]=' ';
		 }
	  }	  	 
	}
  }
  else
  {
    /* shift */
	for (t=0;t<strlen(buf);t++)
	{  
	  if ((foundPos==0) && (foundNextPos==0))
	  {

	     if (strncmp("\"",&buf[t],1)==0)
	     {
		    foundNextPos=t; 
	     }	  
	  
	     if (strncmp("/*",&buf[t],2)==0)
	     {
		      foundPos=t;
			  buf[t]=' ';
			  buf[t+1]=' ';
	     }
	  }
	  else
	  {
	     if (foundPos!=0)
		 {
			if (strncmp("*/",&buf[t],2)==0)
			{
				buf[t]=' ';
				buf[t+1]=' ';
				foundPos=0;
			}
			else
			{
				buf[t]=' ';
			}
		 }
		 if (foundNextPos!=0)
		 {
			if (strncmp("\"",&buf[t],1)==0)
			{				
				foundNextPos=0;
			}			
		 }
	  }
	}
  }

  trim(buf);
  /* have remove all wrong syntax */   
}



void trim(unsigned char* buf)
{
  size_t le;
  
  if (buf==NULL)
	  return;
  if (*buf==0)
	  return;

  le=strlen(buf);
  
  
  while(le>0)
  {
    
    if (isspace(buf[le-1])!=0)
	{	
		buf[le-1]=0;
        le=strlen(buf); 
	}
	else
	{
		break;
	}
  }

  le=strlen(buf);
  while(le>0)
  {
    if (isspace(buf[0])!=0)
	{
		strncpy(&buf[0],&buf[1],le-1);
		buf[le-1]=0;
        le=strlen(buf); 
	}
	else
	{
		break;
	}
  }
}
void find_str(unsigned char asc,unsigned char *buf, long *foundPos)
{
  int t;
  size_t le;  

  le=strlen(buf);

   for (t=0;t<le;t++)
   {
		 if (buf[t]==asc) 
		 {		 
			*foundPos =  *foundPos+t+1;
			break ;
		 }
  }

  /* for end of line the \ is a special case */
  if ((asc == '\n') && (foundPos!=0) && (buf[t-2]=='\\')) 
  {		 
     long extra=t+1;

     find_str(asc, &buf[extra], foundPos);         
  }

}

void find_str2(unsigned char *asc, unsigned char *buf, long *foundPos, 
			   unsigned char * output_resid, unsigned char *output_text)
{
  int t=0;
  size_t le;
  size_t lec;

  le=strlen(buf);
  lec=strlen(asc);

  if ((lec==0) || (le==0))
  {
	  return;
  }

   for (t=0;t<le;t++)
   {
		 if (strncmp(&buf[t],asc,lec)==0) 
		 {		 
		    long softfoundPos=0;
			
			 *foundPos =  *foundPos+t+lec;
			 softfoundPos = *foundPos;
						
			 strncpy(output_resid, &buf[0], t);
			 output_resid[t]=0;
		    
			 strncpy(output_text, &buf[t], le-t);
			 output_text[ le-t ]=0;
			
			break ;
		 }
  }
}


void DialogCMDBuild1(unsigned char *output_resid, unsigned char *output_format, long pos, unsigned char * text)
{
 
  
  unsigned char extra[1000];
  long foundPos=0;
  long foundNextPos=0;
  long le;
  long size;
 
  stringbugs(output_resid,true);
  sprintf(output_resid,"%s",&output_resid[pos]);
  trim(output_resid);
 
  find_str('"',output_resid,&foundPos);
  find_str('"',&output_resid[foundPos],&foundNextPos);
 
  if ((foundPos!=0) && (foundPos!=0))
  {
      strcpy(extra,&output_resid[foundPos+foundNextPos]);
      trim(extra);
  
      output_resid[foundPos+foundNextPos]='\0';        
      trim(output_resid);
  }
  else
  {	  
	  strcpy(extra,output_resid);
	  *output_resid='\0';  
  }
  // \0
  sprintf(output_format,"%s  <obj type=\"%s\" rc_name=\"%s",output_format,text,extra);  
  foundPos=0;  
  find_str(' ',extra,&foundPos);
  le = (strlen(output_format) - strlen(extra))+foundPos-1;
  output_format[le]='\0';
  sprintf(extra,"%s",&extra[foundPos]);
  trim(extra);

  /* top */
  // \0
  sprintf(output_format,"%s\" left=\"%s",output_format,extra);  
  foundPos=0;  
  find_str(' ',extra,&foundPos);
  le = (strlen(output_format) - strlen(extra))+foundPos-1;
  output_format[le]='\0';
  sprintf(extra,"%s",&extra[foundPos]);
  trim(extra);

  /* left */
  // \0
  sprintf(output_format,"%s\" top=\"%s",output_format,extra);  
  foundPos=0;  
  find_str(' ',extra,&foundPos);
  le = (strlen(output_format) - strlen(extra))+foundPos-1;
  output_format[le]='\0';
  sprintf(extra,"%s",&extra[foundPos]);
  trim(extra);

  /* right */
  // \0
  sprintf(output_format,"%s\" width=\"%s",output_format,extra);  
  foundPos=0;  
  find_str(' ',extra,&foundPos);
  le = (strlen(output_format) - strlen(extra))+foundPos-1;
  output_format[le]='\0';
  sprintf(extra,"%s",&extra[foundPos]);
  trim(extra);

  /* bottom */
  // \0
  sprintf(output_format,"%s\" height=\"%s",output_format,extra);  
  foundPos=0;  
  find_str(' ',extra,&foundPos);
  if (foundPos!=0)
  {
      le = (strlen(output_format) - strlen(extra))+foundPos-1;
	  size = strlen(&output_format[le]);
	  output_format[le]='\0';
	  sprintf(extra,"%s",&output_format[le+1]);
	  trim(extra);
	 
     /* style */
	 size = strlen(output_format) + strlen(extra) + 9; 
	 sprintf(output_format,"%s\" style=\"%s",output_format,extra);  	 
	 output_format[size]='\0';
	 foundPos=0;  
     find_str(' ',extra,&foundPos);

    if (*output_resid!='\0')
    {
        sprintf(output_format,"%s\"><![CDATA[%s]]></obj>\n",output_format,output_resid);   
    }
    else
    {
	   sprintf(output_format,"%s\"></obj>\n",output_format);   
    }
  } 
  else
  {
    if (*output_resid!='\0')
        sprintf(output_format,"%s\" style=\"\"><![CDATA[%s]]></obj>\n",output_format,output_resid);   
    else
	    sprintf(output_format,"%s\" style=\"\"></obj>\n",output_format);   
  }

  *output_resid='\0';
}
 

void DialogCMDBuild2(unsigned char *output_resid, unsigned char *output_format, long pos, unsigned char * text)
{   
  long le;

  stringbugs(output_resid,true);
  sprintf(output_resid,"%s",&output_resid[pos]);
  trim(output_resid);
   
  le = strlen(output_resid);
  if (*output_resid=='"')
      *output_resid=' ';
  if (output_resid[le-1]=='"')
      output_resid[le-1]=' ';

  trim(output_resid);    
  sprintf(output_format,"%s  <obj type=\"%s\"><![CDATA[%s]]></obj>\n",output_format,text,output_resid);  
  *output_resid='\0';  
}

// input  : CONTROL         "",101,"Static",SS_SIMPLE | SS_NOPREFIX,3,6,150,10  
void DialogCMDBuild3(unsigned char *output_resid, unsigned char *output_format, long pos, unsigned char * text)
{
  long foundPos=0;
  long foundNextPos=0;
  long le;  
  long count=0;
  long save1;
  long save2;
 
  sprintf(output_resid,"%s",&output_resid[pos]);
  trim(output_resid);
 
  find_str('"',output_resid,&foundPos);
  find_str('"',&output_resid[foundPos],&foundNextPos);

  save1=foundPos;
  save2=foundNextPos;
  
  sprintf(output_format,"%s  <obj type=\"%s\" rc_name=\"",output_format,text);
  
  le=strlen(output_format);
  count=foundNextPos+foundPos;
  if (output_resid[count]==',')
      output_resid[count]=' ';
  foundPos=0;
  find_str(',',&output_resid[count],&foundPos);
  sprintf(output_format,"%s%s\"",output_format,&output_resid[count]);   
  output_format[le+foundPos]='\0';  
  stringbugs(&output_format[le],false);
  count+=foundPos;

  /* prop */  
  sprintf(output_format,"%s\" prop=\"",output_format); 
  le=strlen(output_format);
  sprintf(output_format,"%s%s",output_format,&output_resid[count]);   

  if (output_resid[count]==',')
      output_resid[count]=' ';
  foundPos=0;
  find_str(',',&output_resid[count],&foundPos);
  output_format[le+foundPos]='\0'; 
  stringbugs(&output_format[le],false);
  count+=foundPos;

  /* style */  
  sprintf(output_format,"%s\" style=\"",output_format); 
  le=strlen(output_format);
  sprintf(output_format,"%s%s",output_format,&output_resid[count]);   

  if (output_resid[count]==',')
      output_resid[count]=' ';
  foundPos=0;
  find_str(',',&output_resid[count],&foundPos);
  output_format[le+foundPos]='\0'; 
  stringbugs(&output_format[le],false);
  count+=foundPos;

  /* top */  
  sprintf(output_format,"%s\" left=\"",output_format); 
  le=strlen(output_format);
  sprintf(output_format,"%s%s",output_format,&output_resid[count]);   

  if (output_resid[count]==',')
      output_resid[count]=' ';
  foundPos=0;
  find_str(',',&output_resid[count],&foundPos);
  output_format[le+foundPos]='\0'; 
  stringbugs(&output_format[le],false);
  count+=foundPos;

  /* left */  
  sprintf(output_format,"%s\" top=\"",output_format); 
  le=strlen(output_format);
  sprintf(output_format,"%s%s",output_format,&output_resid[count]);   

  if (output_resid[count]==',')
      output_resid[count]=' ';
  foundPos=0;
  find_str(',',&output_resid[count],&foundPos);
  output_format[le+foundPos]='\0'; 
  stringbugs(&output_format[le],false);
  count+=foundPos;

  /* right */  
  sprintf(output_format,"%s\" width=\"",output_format); 
  le=strlen(output_format);
  sprintf(output_format,"%s%s",output_format,&output_resid[count]);   

  if (output_resid[count]==',')
      output_resid[count]=' ';
  foundPos=0;
  find_str(',',&output_resid[count],&foundPos);
  output_format[le+foundPos]='\0'; 
  stringbugs(&output_format[le],false);
  count+=foundPos;

  /* bottom */
  sprintf(output_format,"%s\" height=\"",output_format);
  le=strlen(output_format);
  sprintf(output_format,"%s%s",output_format,&output_resid[count]);   
  stringbugs(&output_format[le],false);

  /* string */      
  output_resid[save1+save2]='\0';  
  stringbugs(output_resid,true);
 
  
  if (*output_resid!='\0')
      sprintf(output_format,"%s\"><![CDATA[%s]]></obj>\n",output_format,output_resid);
  else
	  sprintf(output_format,"%s\"></obj>\n",output_format);

  *output_resid='\0';
}

