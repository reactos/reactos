#ifndef STLP_DIGITS_H
#define STLP_DIGITS_H

#include <string>

inline void 
#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
complete_digits(std::string &digits)
#else
complete_digits(string &digits)
#endif
{
  while (digits.size() < 2)
  {
    digits.insert(digits.begin(), '0');
  }
}

#endif
