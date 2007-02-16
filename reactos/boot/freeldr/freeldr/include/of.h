#ifndef FREELDR_OF_H
#define FREELDR_OF_H

#define OF_FAILED 0
#define ERR_NOT_FOUND 0xc0000010

#define REV(x) (((x)>>24)&0xff)|(((x)>>8)&0xff00)|(((x)<<24)&0xff000000)|(((x)<<8)&0xff0000)

typedef int (*of_proxy)
    ( int table_off, void *arg1, void *arg2, void *arg3, void *arg4 );
typedef long jmp_buf[100];

extern of_proxy ofproxy;

int ofw_finddevice( const char *name );
int ofw_getprop( int package, const char *name, void *buffer, int size );
int ofw_open( const char *name );
int ofw_write( int handle, const char *buffer, int size );
int ofw_read( int handle, const char *buffer, int size );
void ofw_print_string(const char *);
void ofw_print_number(int);
void ofw_exit();

int setjmp( jmp_buf buf );
int longjmp( jmp_buf buf, int retval );

#endif/*FREELDR_OF_H*/
