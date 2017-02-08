#define noargs() 1 
# define onearg(foo) foo 
 # define  twoargs( x , y ) x y 
	#	define	threeargs(	a	,	b	,	c	) a b c 
noargs ( ) 
onearg ( 2 ) 
twoargs ( 3 , 4 ) 
threeargs ( 5 , 6 , 7 ) 
