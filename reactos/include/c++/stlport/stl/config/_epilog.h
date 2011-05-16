#if defined (_STLP_MSVC) || defined (__ICL)

#  pragma warning (pop)
#  pragma pack (pop)

#elif defined (__BORLANDC__)

#  pragma option pop
#  pragma option -w-pow     // -w-8062 Previous options and warnings not restored

#elif defined (__sgi) && !defined (__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)

#  pragma reset woff 1174
#  pragma reset woff 1375
#  pragma reset woff 1209
/* from iterator_base.h */
#  pragma reset woff 1183

#elif defined (__DECCXX)

#  ifdef __PRAGMA_ENVIRONMENT
#    pragma __environment __restore
#  endif

#elif defined (__IBMCPP__)

#  pragma info(restore)

#endif
