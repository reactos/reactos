#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* 
 Unicode                   UTF8
0x00000000 - 0x0000007F: 0xxxxxxx
0x00000080 - 0x000007FF: 110xxx xx 10xx xxxx
0x00000800 - 0x0000FFFF: 1110xxxx  10xxxx xx 10xx xxxx
0x00010000 - 0x001FFFFF: 11110x xx 10xx xxxx 10xxxx xx 10xx xxxx
0x00200000 - 0x03FFFFFF: 111110xx  10xxxx xx 10xx xxxx 10xxxx xx 10xx xxxx
0x04000000 - 0x7FFFFFFF: 1111110x  10xx xxxx 10xxxx xx 10xx xxxx 10xxxx xx 10xx xxxx

0000 0      1001 9
0001 1      1010 A
0010 2      1011 B
0011 3      1100 C
0100 4      1101 D 
0101 5      1110 E
0110 6      1111 F
0111 7
1000 8
*/
void printUTF8(long long u) {
  long long m;
  if(u<0x80) {
    printf("%c", (unsigned char)u);
  }
  else if(u<0x800) {
    m = ((u&0x7c0)>>6) | 0xc0;
    printf("%c", (unsigned char)m);
    m = (u&0x3f) | 0x80;
    printf("%c", (unsigned char)m);
  }
  else if(u<0x10000) {
    m = ((u&0xf000)>>12) | 0xe0;
    printf("%c",(unsigned char)m);
    m = ((u&0xfc0)>>6) | 0x80;
    printf("%c",(unsigned char)m);
    m = (u & 0x3f) | 0x80;
    printf("%c",(unsigned char)m);
  }
  else if(u<0x200000) {
    m = ((u&0x1c0000)>>18) | 0xf0;
    printf("%c", (unsigned char)m);
    m = ((u& 0x3f000)>>12) | 0x80;
    printf("%c", (unsigned char)m);
    m = ((u& 0xfc0)>>6) | 0x80;
    printf("%c", (unsigned char)m);
    m = (u&0x3f) | 0x80;
    printf("%c", (unsigned char)m);
  }
  else if(u<0x4000000){
    m = ((u&0x3000000)>>24) | 0xf8;
    printf("%c", (unsigned char)m);
    m = ((u&0xfc0000)>>18) | 0x80;
    printf("%c", (unsigned char)m);
    m = ((u&0x3f000)>>12) | 0x80;
    printf("%c", (unsigned char)m);
    m = ((u&0xfc00)>>6) | 0x80;
    printf("%c", (unsigned char)m);
    m = (u&0x3f) | 0x80;
    printf("%c", (unsigned char)m);
  }
  else {
    m = ((u&0x40000000)>>30) | 0xfc;
    printf("%c", (unsigned char)m);
    m = ((u&0x3f000000)>>24) | 0x80;
    printf("%c", (unsigned char)m);
    m = ((u&0xfc0000)>>18) | 0x80;
    printf("%c", (unsigned char)m);
    m = ((u&0x3f000)>>12) | 0x80;
    printf("%c", (unsigned char)m);
    m = ((u&0xfc0)>>6) | 0x80;
    printf("%c", (unsigned char)m);
    m = (u&0x3f)| 0x80;
    printf("%c", (unsigned char)m);
  }
}

int main() {
  int i,j;
  long long n1, n2;
  unsigned char b1[15], b2[15];
  unsigned char buf[1024];
  i=0;
  while(fgets(buf, 1024, stdin)) {
    //    printf("read %s\n", buf);
    for(i=0;i<strlen(buf); i++) 
      if(buf[i]=='U') {
	if(buf[i+1] == '+') {
	  n1 = strtoll(buf+i+2,0,16);
	  printf("U+%05x", n1);
	  printUTF8(n1);printf("|");
	}
      }
    printf("\n");
  }
}

