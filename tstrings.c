//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#include "tstrings.h"


#ifdef UNICODE
	#if defined(DEFINE_tstpcpy) && !defined(DEFINE_stpcpyW)
		#define DEFINE_stpcpyW
	#endif
	#if defined(DEFINE_tstplimcpy) && !defined(DEFINE_stplimcpyW)
		#define DEFINE_stplimcpyW
	#endif
#else
	#if defined(DEFINE_tstpcpy) && !defined(DEFINE_stpcpy)
		#define DEFINE_stpcpy
	#endif
	#if defined(DEFINE_tstplimcpy) && !defined(DEFINE_stplimcpy)
		#define DEFINE_stplimcpy
	#endif
#endif

// strcpy Œn ---------------------------------------------------------------
#ifdef DEFINE_stpcpyW
WCHAR *stpcpyW(WCHAR *deststr, const WCHAR *srcstr)
{
	WCHAR *destptr = deststr;
	const WCHAR *srcptr = srcstr;

	for(;;){
		WCHAR code;

		code = *srcptr;
		*destptr = code;
		if ( code == '\0' ) return destptr;
		srcptr++;
		destptr++;
	}
}
#endif

#if !defined(__BORLANDC__) && (defined(DEFINE_stpcpyA) || (!defined(UNICODE) && defined(DEFINE_tstpcpy)) )
char *stpcpyA(char *deststr, const char *srcstr)
{
	char *destptr = deststr;
	const char *srcptr = srcstr;

	for(;;){
		char code;

		code = *srcptr;
		*destptr = code;
		if ( code == '\0' ) return destptr;
		srcptr++;
		destptr++;
	}
}
#endif

#if 0
void tstrlimcpy(TCHAR *dest, const TCHAR *src, DWORD maxlength)
{
	TCHAR code;
	const TCHAR *destmax;

	destmax = dest + maxlength - 1;
	while( dest < destmax ){
		code = *src++;
		if ( code == '\0' ) break;
		*dest++ = code;
	}
	*dest = '\0';
}
#endif

#if defined(DEFINE_stplimcpy) || (!defined(UNICODE) && defined(DEFINE_tstplimcpy))
char *stplimcpy(char *deststr, const char *srcstr, size_t maxlength)
{
	char code;
	const char *destmax;

	destmax = deststr + maxlength - 1;
	while( deststr < destmax ){
		code = *srcstr++;
		if ( code == '\0' ) break;
		*deststr++ = code;
	}
	*deststr = '\0';
	return deststr;
}
#endif

#if defined(DEFINE_stplimcpyW) || (defined(UNICODE) && defined(DEFINE_tstplimcpy))
WCHAR *stplimcpyW(WCHAR *deststr, const WCHAR *srcstr, size_t maxlength)
{
	WCHAR code;
	const WCHAR *destmax;

	destmax = deststr + maxlength - 1;
	while( deststr < destmax ){
		code = *srcstr++;
		if ( code == '\0' ) break;
		*deststr++ = code;
	}
	*deststr = '\0';
	return deststr;
}
#endif

#ifdef DEFINE_tstpmaxcpy
TCHAR *tstpmaxcpy(TCHAR *deststr, const TCHAR *srcstr, const TCHAR *destmax)
{
	TCHAR code;

	destmax--;
	for (;;){
		code = *srcstr++;
		if ( deststr < destmax ){
			if ( code == '\0' ) break;
			*deststr++ = code;
		}else{
			*deststr = '\0';
			return (code != '\0') ? NULL : deststr;
		}
	}
	*deststr = '\0';
	return deststr;
}
#endif

// strchr Œn ---------------------------------------------------------------
// ‚P•¶Žš‘O‚Ì‘SŠp^”¼Šp‚Ì”»’f(UNICODE”Å‚ÍƒTƒƒQ[ƒg‚ð–³Ž‹‚µ‚Äƒ}ƒNƒ‚Å 1 ‚ð•Ô‚·
#if defined(DEFINE_bchrlen) && !defined(UNICODE)
int bchrlen(char *str, int off)
{
	int size = 0;
	char *max_str;

	max_str = str + off;

	while( (str < max_str) && ((size = Chrlen(*str)) != 0) ) str += size;
	return size;
}
#endif

// "|"‚ÌˆÊ’u‚ð’T‚·(ANSIŒÀ’èAUNICOODE”Å‚Íƒ}ƒNƒ)
#if defined(DEFINE_SearchVLINE) && !defined(UNICODE)
char *SearchVLINE(const char *str)
{
	char type;

	for ( ;; ){
		type = *str;
		if ( type == '\0' ) return NULL;
		if ( type == '|' ) return (char *)str;
		str += (char)Chrlen(type);
	}
}
#endif

#if defined(WINEGCC) && (defined(strchrW) || (defined(UNICODE) && defined(DEFINE_tstrchr)) )
WCHAR *strchrW(const WCHAR *text, WCHAR findchar)
{
	const WCHAR *ptr;

	ptr = text;
	while ( *ptr != '\0' ){
		if ( *ptr == findchar ) return (WCHAR *)ptr;
		ptr++;
	}
	return NULL;
}
#endif

#if defined(WINEGCC) && (defined(strrchrW) || (defined(UNICODE) && defined(DEFINE_tstrrchr)) )
WCHAR *strrchrW(const WCHAR *text, WCHAR findchar)
{
	const WCHAR *ptr;

	ptr = text + strlenW(text);
	while ( ptr > text ){
		ptr--;
		if ( *ptr == findchar ) return (WCHAR *)ptr;
	}
	return NULL;
}
#endif

// strstr Œn ---------------------------------------------------------------
#ifdef DEFINE_tstristr
TCHAR *tstristr(const TCHAR *target, const TCHAR *findstr)
{
	size_t len, flen;
	const TCHAR *p, *maxptr;

	flen = tstrlen(findstr);
	len = tstrlen(target);
	maxptr = target + len - flen;

#ifdef UNICODE
	for ( p = target ; p <= maxptr ; p++ ){
#else
	for ( p = target ; p <= maxptr ; p += Chrlen(*p) ){
#endif
		if ( !tstrnicmp(p, findstr, flen) ){
			return (TCHAR *)p;
		}
	}
	return NULL;
}
#endif

#if defined(WINEGCC) && defined(DEFINE_strstrW)
WCHAR *strstrW(const WCHAR *text1, const WCHAR *text2)
{
	size_t len1, len2;
	const WCHAR *ptr;

	len1 = strlenW(text1);
	len2 = strlenW(text2);
	if ( len1 < len2 ) return NULL;
	len1 -= (len2 - 1);
	ptr = text1;
	while ( len1 ){
		if ( memcmp(ptr, text2, len2 * sizeof(WCHAR)) == 0 ) return (WCHAR *)ptr;
		ptr++;
		len1--;
	}
	return NULL;
}
#endif

#ifdef DEFINE_memsearch
LONG_PTR memsearch(const char *image, size_t imagesize, const char *key, size_t keysize)
{
	const char *imageptr;

	if ( imagesize < keysize ) return -1;
	imagesize -= keysize;
	imageptr = image;
	for (;;){
		if ( memcmp(imageptr, key, keysize) == 0 ) return (LONG_PTR)(imageptr - image);
		if ( imagesize <= 0 ) return -1;
		imagesize--;
		imageptr++;
	}
}
#endif

// •¶Žš—ñ’uŠ· ---------------------------------------------------------------
#ifdef DEFINE_tstrreplace
void tstrreplace(TCHAR *text, const TCHAR *targetword, const TCHAR *replaceword)
{
	TCHAR *p;
	size_t tlen, rlen;

	if ( (p = tstrstr(text, targetword)) == NULL ) return;
	tlen = tstrlen(targetword);
	rlen = tstrlen(replaceword);

	for (;;){
		if ( tlen != rlen ) memmove(p + rlen, p + tlen, TSTRSIZE(p + tlen));
		memcpy(p, replaceword, TSTROFF(rlen));
		text = p + rlen;
		if ( (p = tstrstr(text, targetword)) != NULL ) continue;
		break;
	}
}
#endif
#ifdef DEFINE_strreplace
void strreplace(char *text, const char *targetword, const char *replaceword)
{
	char *p;
	size_t tlen, rlen;

	if ( (p = strstr(text, targetword)) == NULL ) return;
	tlen = strlen(targetword);
	rlen = strlen(replaceword);

	for (;;){
		if ( tlen != rlen ) memmove(p + rlen, p + tlen, strlen(p + tlen) + 1);
		memcpy(p, replaceword, rlen);
		text = p + rlen;
		if ( (p = strstr(text, targetword)) != NULL ) continue;
		break;
	}
}
#endif
