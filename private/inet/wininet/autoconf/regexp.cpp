#include <regexp.h>

BOOL test_match(int m, LPSTR target, int pattern[])     /* m = length of target */
{
    char    match[INTERNET_MAX_URL_LENGTH];
    int     i = -1;     /* Will be advanced to 0 */
    int     j = 0;      /* i = index to pattern, j = index to target */

advance:
    ++i;
    if (j > m)
        return FALSE;

    switch (pattern[i]) {
    case PAT_START:  if (j != 0) return FALSE; match[i] = 0; goto advance;
    case PAT_END:    if (target[j] == 0) return TRUE; else goto retreat;
    case PAT_STAR:   match[i] = j = m; goto advance;
    case PAT_QUES:   if (j < m) goto match_one; else goto retreat;
    case PAT_AUGDOT: if (target[j] == '.') goto match_one;
             else if (target[j] == 0) goto match_zero;
             else goto retreat;
    case PAT_AUGQUES: if (target[j] && target[j] != '.')
            goto match_one; else goto match_zero;
    case PAT_AUGSTAR: if (target[j] && target[j] != '.') 
            goto match_one; else goto retreat;
    default:          if (target[j] == pattern[i])
            goto match_one; else goto retreat;
    }
match_one: match[i] = (char) ++j; goto advance;
match_zero: match[i] = (char) j; goto advance;

retreat:
    --i;
    switch (pattern[i]) {
    case PAT_START:  return FALSE;
    case PAT_END:    return FALSE;     /* Cannot happen */
    case PAT_STAR:   if (match[i] == match[i-1]) goto retreat;
             j = --match[i]; goto advance;
    case PAT_QUES:   goto retreat;
    case PAT_AUGDOT: goto retreat;
    case PAT_AUGQUES: if (match[i] == match[i-1]) goto retreat;
             j = --match[i]; goto advance;
    case PAT_AUGSTAR: goto retreat;
    default:          goto retreat;
    }
}

BOOL parse_pattern(LPSTR s, int pattern[])
{
    int i = 1;

    pattern[0] = PAT_START; /* Can be hard-coded into pattern[] */
    for (;;) {
    switch (*s) {
        case '*':   pattern[i] = PAT_STAR; break;
        case '?':   pattern[i] = PAT_QUES; break;
        case '^':
        switch (*++s) {
        case '.': pattern[i] = PAT_AUGDOT; break;
        case '?': pattern[i] = PAT_AUGQUES; break;
        case '*': pattern[i] = PAT_AUGSTAR; break;
        default: return FALSE;
        }
        break;
        case 0: pattern[i] = PAT_END; return TRUE;
        default:    pattern[i] = *s; break;
    }
    if (++i >= INTERNET_MAX_URL_LENGTH) return FALSE;
    ++s;
    }
}

BOOL match( LPSTR target, LPSTR regexp) 
{
    int *pattern;
    BOOL result;

    pattern = new int[INTERNET_MAX_URL_LENGTH];

    if (!target || (pattern==NULL))
        return FALSE;

    if (!parse_pattern(regexp,pattern)) 
        return FALSE;
    if (lstrlen(target) >= INTERNET_MAX_URL_LENGTH) 
        return FALSE;

    result = test_match(lstrlen(target),target,pattern);
    delete [] pattern;
    return result;
}
