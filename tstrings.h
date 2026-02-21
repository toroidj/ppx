//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#ifndef _tstrings_h_
#define _tstrings_h_
#ifdef __cplusplus
extern "C" {
#endif

// strcpy ån
#ifndef UNICODE
#define tstplimcpy(deststr, srcstr, maxlength) stplimcpy(deststr, srcstr, maxlength)
#else
#define tstplimcpy(deststr, srcstr, maxlength) stplimcpyW(deststr, srcstr, maxlength)
#endif

extern WCHAR *stpcpyW(WCHAR *deststr, const WCHAR *srcstr);
#ifdef _MSC_VER
extern char *stpcpy(char *deststr, const char *srcstr);
#endif
extern char *stplimcpy(char *deststr, const char *srcstr, size_t maxlength);
extern WCHAR *stplimcpyW(WCHAR *deststr, const WCHAR *srcstr, size_t maxlength);
// stplimcpy ÇÃí∑Ç≥êßå¿Ç™Ç†Ç¡ÇΩÇ∆Ç´ÅANULL Çï‘Ç∑
extern TCHAR *tstpmaxcpy(TCHAR *deststr, const TCHAR *srcstr, const TCHAR *destmax);

// strchr ån
#ifdef UNICODE
  #define bchrlen(str, off) 1
  #define SearchVLINE(str) strchrW(str, '|')
#else
  extern int bchrlen(char *str, int off);
  extern char *SearchVLINE(const char *str);
#endif
#ifdef WINEGCC
  extern WCHAR *strchrW(const WCHAR *text, WCHAR findchar);
  extern WCHAR *strrchrW(const WCHAR *text, WCHAR findchar);
#endif

// strstr ån
extern TCHAR *tstristr(const TCHAR *target, const TCHAR *findstr);
#ifdef WINEGCC
extern WCHAR *strstrW(const WCHAR *text1, const WCHAR *text2);
#endif
extern LONG_PTR memsearch(const char *image, size_t imagesize, const char *key, size_t keysize);

// ï∂éöóÒíuä∑
extern void tstrreplace(TCHAR *text, const TCHAR *targetword, const TCHAR *replaceword);
extern void tstrinsert(TCHAR *str, TCHAR *insertstr);
extern void tstrtrimhead(TCHAR *str, size_t len);

#ifdef __cplusplus
}
#endif
#endif
