/*
*******************************************************************************
*
*   Copyright (C) 2002, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <unicode/ustring.h>
#include <unicode/ubrk.h>

U_CFUNC int c_main(void);

void printTextRange(UChar* str, int32_t start, int32_t end)
{
  char    charBuf[1000];
  UChar   savedEndChar;

  savedEndChar = str[end];
  str[end] = 0;
  u_austrncpy(charBuf, str+start, sizeof(charBuf)-1);
  charBuf[sizeof(charBuf)-1]=0;
  printf("string[%2d..%2d] \"%s\"\n", start, end-1, charBuf); 
  str[end] = savedEndChar;
}



/* Print each element in order: */
void printEachForward( UBreakIterator* boundary, UChar* str) {
  int32_t end;
  int32_t start = ubrk_first(boundary);
  for (end = ubrk_next(boundary); end != UBRK_DONE; start = end, end =
	 ubrk_next(boundary)) {
    printTextRange(str, start, end );
  }
}


/* Print each element in reverse order: */
void printEachBackward( UBreakIterator* boundary, UChar* str) {
  int32_t start;
  int32_t end = ubrk_last(boundary);
  for (start = ubrk_previous(boundary); start != UBRK_DONE;  end = start,
	 start =ubrk_previous(boundary)) {
    printTextRange( str, start, end );
  }
}

/* Print first element */
void printFirst(UBreakIterator* boundary, UChar* str) {
  int32_t end;
  int32_t start = ubrk_first(boundary);
  end = ubrk_next(boundary);
  printTextRange( str, start, end );
}

/* Print last element */
void printLast(UBreakIterator* boundary, UChar* str) {
  int32_t start;
  int32_t end = ubrk_last(boundary);
  start = ubrk_previous(boundary);
  printTextRange(str, start, end );
}

/* Print the element at a specified position */

void printAt(UBreakIterator* boundary, int32_t pos , UChar* str) {
  int32_t start;
  int32_t end = ubrk_following(boundary, pos);
  start = ubrk_previous(boundary);
  printTextRange(str, start, end );
}

/* Creating and using text boundaries*/

int c_main( void ) {
  UBreakIterator *boundary;
  char           cStringToExamine[] = "Aaa bbb ccc. Ddd eee fff.";
  UChar          stringToExamine[sizeof(cStringToExamine)+1]; 
  UErrorCode     status = U_ZERO_ERROR;

  printf("\n\n"
	 "C Boundary Analysis\n"
	 "-------------------\n\n");

  printf("Examining: %s\n", cStringToExamine);
  u_uastrcpy(stringToExamine, cStringToExamine);
        
  /*print each sentence in forward and reverse order*/
  boundary = ubrk_open(UBRK_SENTENCE, "en_us", stringToExamine,
		       -1, &status);
  if (U_FAILURE(status)) {
    printf("ubrk_open error: %s\n", u_errorName(status));
    exit(1);
  }

  printf("\n----- Sentence Boundaries, forward: -----------\n"); 
  printEachForward(boundary, stringToExamine);
  printf("\n----- Sentence Boundaries, backward: ----------\n");
  printEachBackward(boundary, stringToExamine);
  ubrk_close(boundary);
    
  /*print each word in order*/
  boundary = ubrk_open(UBRK_WORD, "en_us", stringToExamine,
		       u_strlen(stringToExamine), &status);
  printf("\n----- Word Boundaries, forward: -----------\n"); 
  printEachForward(boundary, stringToExamine);
  printf("\n----- Word Boundaries, backward: ----------\n");
  printEachBackward(boundary, stringToExamine);
  /*print first element*/
  printf("\n----- first: -------------\n");
  printFirst(boundary, stringToExamine);
  /*print last element*/
  printf("\n----- last: --------------\n");
  printLast(boundary, stringToExamine);
  /*print word at charpos 10 */
  printf("\n----- at pos 10: ---------\n");
  printAt(boundary, 10 , stringToExamine);
    
  ubrk_close(boundary);

  printf("\nEnd of C boundary analysis\n");
  return 0;
}
