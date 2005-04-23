#define MAX_DOUBLE 1.7976931348623158e+308
#define MIN_DOUBLE  2.2250738585072014e-308

double _xnextafter( double __x, double __y )
{
	double_t *x = ( double_t *)&__x;
	
	double __e;
	double_t *e = (double_t *)&__e;
	
	
	if ( _isnan(__x) || _isinf(__x) )
		return __x;
	
	if ( _isnan(__y) )
		return __y;


	// don't go to infinity just by adding

	if ( _isinf(__y) && fabs(__x) >= MAX_DOUBLE  )
		return __x;

	if ( !_isinf(__y) && fabs(__x - __y) <= MIN_DOUBLE  )
		return __y; 




	e->mantissal = 1;
	e->mantissah = 0;
	if ( x->exponent >= 53  ) 
		e->exponent = x->exponent - 52;
	else
		e->exponent = 1;

	if ( fabs(__x) < fabs(__y) )
		e->sign = 0;
	else
		e->sign = 1;



    return __x+__e;



}
