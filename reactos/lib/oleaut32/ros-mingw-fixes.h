
/* problems with decVal member of VARIANT union in MinGW headers */
#undef V_DECIMAL
#define V_DECIMAL(X) (X->__VARIANT_NAME_1.decVal)

