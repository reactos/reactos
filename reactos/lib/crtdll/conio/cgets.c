#include <msvcrt/conio.h>
#include <msvcrt/stdlib.h>


char *_cgets(char *string)
{
  unsigned len = 0;
  unsigned int maxlen_wanted;
  char *sp;
  int c;
  /*
   * Be smart and check for NULL pointer.
   * Don't know wether TURBOC does this.
   */
  if (!string)
    return(NULL);
  maxlen_wanted = (unsigned int)((unsigned char)string[0]);
  sp = &(string[2]);
  /* 
   * Should the string be shorter maxlen_wanted including or excluding
   * the trailing '\0' ? We don't take any risk.
   */
  while(len < maxlen_wanted-1)
  {
    c=_getch();
    /*
     * shold we check for backspace here?
     * TURBOC does (just checked) but doesn't in cscanf (thats harder
     * or even impossible). We do the same.
     */
    if (c == '\b')
    {
      if (len > 0)
      {
	_cputs("\b \b");		/* go back, clear char on screen with space
				   and go back again */
	len--;
	sp[len] = '\0';		/* clear the character in the string */
      }
    }
    else if (c == '\r')
    {
      sp[len] = '\0';
      break;
    }
    else if (c == 0)
    {
      /* special character ends input */
      sp[len] = '\0';
      _ungetch(c);		/* keep the char for later processing */
      break;
    }
    else
    {
      sp[len] = _putch(c);
      len++;
    }
  }
  sp[maxlen_wanted-1] = '\0';
  string[1] = (char)((unsigned char)len);
  return(sp);   
}    


