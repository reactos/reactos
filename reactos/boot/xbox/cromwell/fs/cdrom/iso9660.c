/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2004-07-22 "Edgar Hucek"<hostmaster@ed-soft.at>  Created
*/

#include "boot.h"
#include "iso_fs.h"

int isupper( int ch )
{
	return (unsigned int)(ch - 'A') < 26u;
}

strip_blank(char *buffer, unsigned char len) {
	unsigned char i;
	for(i = len; i > 0; i--) {
		if(buffer[i] != ' ') {
			break;
		} else {
			buffer[i] = 0;
		}
	}
}

int iso9660_name_translate(char *translated, char *old, unsigned len) {
  int i;
  
  for (i = 0; i < len; i++) {
    unsigned char c = old[i];
    if (!c)
      break;
    
    /* lower case */
    if (isupper(c)) c = tolower(c);	
    
    /* Drop trailing '.;1' (ISO 9660:1988 7.5.1 requires period) */
    if (c == '.' && i == len - 3 && old[i + 1] == ';' && old[i + 2] == '1')
      break;
    
    /* Drop trailing ';1' */
    if (c == ';' && i == len - 2 && old[i + 1] == '1')
      break;
    
    /* Convert remaining ';' to '.' */
    if (c == ';')
      c = '.';
    
    translated[i] = c;
  }
  translated[i] = '\0';
  return i;
}

unsigned long read_dir(int driveId, struct iso_directory_record *dir_read, char *search, char *filename, struct iso_directory_record *dir_found) {
	char *buffer;
	struct iso_directory_record *dir;
	unsigned long read_size;
	unsigned long offset;
	unsigned char dir_length;
	unsigned long sect = 0;
	char *newfilename;
	int i;

	offset = *((unsigned long *)(dir_read->extent));
	read_size = *((unsigned long *)(dir_read->size));
	buffer = (char *)malloc(read_size);
	if(!buffer) {
		return 0;
	}
	
	for(i = 0; i < (read_size >> ISOFS_BLOCK_BITS); i++) {
		BootIdeReadSector(driveId, &buffer[i * ISOFS_BLOCK_SIZE], offset , 0, ISOFS_BLOCK_SIZE);
		offset++;
	}
	
	newfilename = (char *)malloc(1024);
	memset(newfilename, 0x0, 1024);
		
	offset=0;
	while(offset < read_size) {
		dir = (struct iso_directory_record *)&buffer[offset];
		dir_length = *((unsigned char *)(dir->length));
		if(!dir_length) {
			/* Skip to next sector */
			offset = (offset + ISOFS_BLOCK_SIZE) & ~ (ISOFS_BLOCK_SIZE - 1);
			continue;
		}
		if(dir->name[0] != 0 && dir->name[0] != 1) {
			sprintf(newfilename, "%s/", filename);
			iso9660_name_translate(newfilename + strlen(newfilename),
				 dir->name, (unsigned char) dir->name_len[0]);
//			printk("Read : Sector %d Filename %s %d\n", 
//					*((unsigned long *)(dir->extent)),  newfilename,
//					(unsigned char)dir->ext_attr_length[0]);
		}
		
		if(strlen(newfilename) <= strlen(search)) {
			if(memcmp(newfilename, search, strlen(search)) == 0) {
				sect = *((unsigned long *)(dir->extent));
				memcpy(dir_found, dir, sizeof(struct iso_directory_record));
//				printk("Found : Sector %d Directory %s Filename %s  %d %d \n", 
//					sect, newfilename, search, 
//					strlen(newfilename), strlen(search));
//				free(newfilename);
//				free(buffer);
				return sect;
			}
		}
		if((*((char *)(dir->flags)) & IS_DIR) && (*((unsigned char *)(dir->name_len)) > 1)) {
//			printk("Directory %s Filename %s\n", newfilename, search);
			if (strlen(newfilename) < strlen(search) &&
			    0 == memcmp(search, newfilename, strlen(newfilename)) &&
			    '/' == search[strlen(newfilename)]) {
				sect = read_dir(driveId, dir, search, newfilename, dir_found);
				return sect;
			}
		}
		offset+=dir_length;
		sect = 0;
	}
	
	free(buffer);
	free(newfilename);

	return sect;
}

void read_file(int driveId, struct iso_directory_record *dir_read, char *buffer) {
	unsigned long read_size;
	unsigned long offset;
	int i;
	char *tmpbuff;

	offset = *((unsigned long *)(dir_read->extent));
	tmpbuff = (char *) malloc(ISO_BLOCKSIZE);
	
	read_size = *((unsigned long *)(dir_read->size));	
	if(read_size <= ISO_BLOCKSIZE) {
		read_size = ISO_BLOCKSIZE;
	} else {
		read_size+=(ISO_BLOCKSIZE - (read_size % ISO_BLOCKSIZE));
	}
	
//	printk("         read_file sector %d %d\n", offset, read_size);
	
	for(i = 0; i < (read_size >> ISOFS_BLOCK_BITS) ; i++) {
		memset(tmpbuff, 0x0, ISO_BLOCKSIZE);
		BootIdeReadSector(driveId, tmpbuff, offset , 0, ISO_BLOCKSIZE);
		offset++;
		if(((i+1) * ISO_BLOCKSIZE) > read_size) {
			memcpy(&buffer[i * ISO_BLOCKSIZE], tmpbuff, (i * ISO_BLOCKSIZE) - *((unsigned long *)(dir_read->size)));
		} else {
			memcpy(&buffer[i * ISO_BLOCKSIZE], tmpbuff, ISO_BLOCKSIZE);
		}
	}
	free(tmpbuff);
}

int BootIso9660GetFile(int driveId, char *szcPath, unsigned char *pbaFile, unsigned int dwFileLengthMax) {
	struct iso_primary_descriptor *pvd;
	struct iso_directory_record *rootd;
	unsigned long read_size;
	unsigned long offset;
	struct iso_directory_record *dir;
	
	read_size = ISOFS_BLOCK_SIZE;

	pvd = (struct iso_primary_descriptor *)malloc(sizeof(struct iso_primary_descriptor));
	memset(pvd,0x0,sizeof(struct iso_primary_descriptor));
	dir = (struct iso_directory_record *)malloc(sizeof(struct iso_directory_record));
	memset(dir,0x0,sizeof(struct iso_directory_record));
	
	if(BootIdeReadSector(driveId, pvd, 16 , 0, ISO_BLOCKSIZE)) {
//		printk("BootIso9660GetFile : Error read Sector\n");
		free(pvd);
		free(dir);
		return -1;
	}

	rootd = (struct iso_directory_record *)&pvd->root_directory_record;
	offset = read_dir(driveId, rootd, szcPath, "", dir);
	
	if(offset > 0) {
		if(*((unsigned long *)(dir->size)) > dwFileLengthMax) {
			free(pvd);
			free(dir);
			return -1;
		}
		read_file(driveId, dir, pbaFile);
		return *((unsigned long *)(dir->size));
	} else {
		free(pvd);
		free(dir);
		return -1;
	}
}

