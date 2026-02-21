/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library							文字列操作
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "PPD_DEF.H"
#pragma hdrstop

#define DEFINE_memsearch
#define DEFINE_tstrreplace
#define DEFINE_tstpmaxcpy
#define DEFINE_tstplimcpy
#define DEFINE_tstpcpy
#define DEFINE_stpcpyA
#define DEFINE_tstrchr
#define DEFINE_tstrrchr
#define DEFINE_tstristr
#define DEFINE_bchrlen
#define DEFINE_SearchVLINE
#define DEFINE_strstrW

#include "tstrings.c"

#ifndef UNICODE
const BYTE ZHtbl[] =
	"　！”＃＄％＆’（）＊＋，－．／"	// 2x 000
	"０１２３４５６７８９：；＜＝＞？"	// 3x 020
	"＠ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯ"	// 4x 040
	"ＰＱＲＳＴＵＶＷＸＹＺ［￥］＾＿"	// 5x 080
	"‘ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏ"	// 6x 0a0
	"ｐｑｒｓｔｕｖｗｘｙｚ｛｜｝～〓"	// 7x 0c0
	"〓。「」、・ヲァィゥェォャュョッ"	// ax 0e0
	"ーアイウエオカキクケコサシスセソ"	// bx 100
	"タチツテトナニヌネノハヒフヘホマ"	// cx 120
	"ミムメモヤユヨラリルレロワン゛゜"	// dx 140

	"ガギグゲゴザジズゼゾダヂヅデドバ"	// etc 160
	"ビブベボパピプペポ";				//     180-191
#else
const WCHAR ZHtbl[] = L"。「」、・ヲァィゥェォャュョッーアイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワン゛゜ガギグゲゴザジズゼゾダヂヅデドバビブベボパピプペポ";
#endif

// 1bytes 文字 → 2bytes 文字変換 ---------------------------------------------
PPXDLL TCHAR * PPXAPI Strsd(TCHAR *dststr, const TCHAR *srcstr)
{
	const UTCHAR *src;
	UTCHAR *dst, code;

	src = (const UTCHAR *)srcstr;
	dst = (UTCHAR *)dststr;

	for ( ; (code = *src) != '\0' ; src++ ){
		#ifdef UNICODE
		if ( code == L' ' ){
			*dst++ = L'　';
			continue;
		}
		if ( code == L'\\' ){
			*dst++ = L'￥';
			continue;
		}
		if ( (code >= L'!') && (code <= L'~') ){
			*dst++ = (UTCHAR)(*src + (L'！' - L'!'));
			continue;
		}
		if ( (code >= L'｡') && (code <= L'ﾟ') ){
			*dst = (UTCHAR)ZHtbl[code - L'｡'];
			if ( *(src + 1) == L'ﾞ' ){
				if ( (code >= L'ｶ') && (code <= L'ﾄ') ){
					src++;
					(*dst)++;
				}else if ( (code >= L'ﾊ') && (code <= L'ﾎ') ){
					src++;
					(*dst)++;
				}else if (code == L'ｳ' ){
					src++;
					*dst = L'ヴ';
				}
			}else if ( (*(src + 1) == L'ﾟ') &&
						(code >= L'ﾊ') && (code <= L'ﾎ') ){
				src++;
				*dst += (UTCHAR)2;
			}
			dst++;
			continue;
		}
		#else
		const BYTE *p;

		if ( (code >= 0x20) && (code < 0x80) ){
			p = &ZHtbl[(code - 0x20) << 1];
			*dst++ = *p++;
			*dst++ = *p;
			continue;
		}
		if ( (code >= 0xa0) && (code < 0xe0) ){

			p = &ZHtbl[(code - 0x20 - 0x20) << 1];
			if ( *(src + 1) == (BYTE)'ﾞ' ){
				if ( (code >= (BYTE)'ｶ') && (code <= (BYTE)'ﾄ') ){
					src++;
					p += 0x2a * 2;
				}else if ( (code >= (BYTE)'ﾊ') && (code <= (BYTE)'ﾎ') ){
					src++;
					p += (0x2a - 5) * 2;
				}else if (code == (BYTE)'ｳ' ){
					src++;
					p = (BYTE *)"ヴ";
				}
			}else if ( (*(src + 1) == (BYTE)'ﾟ') &&
						(code >= (BYTE)'ﾊ') && (code <= (BYTE)'ﾎ') ){
				src++;
				p += 0x2a * 2;
			}
			*dst++ = *p++;
			*dst++ = *p;
			continue;
		}
		if ( IskanjiA(code) && *(src + 1) ) *dst++ = *src++;
		#endif
		*dst++ = *src;
	}
	*dst = '\0';
	return (TCHAR *)dst;
}

// 2bytes 文字 → 1bytes 文字変換 ---------------------------------------------
PPXDLL TCHAR * PPXAPI Strds(TCHAR *dststr, const TCHAR *srcstr)
{
	const UTCHAR *src;
	UTCHAR *dst;

	src = (const UTCHAR *)srcstr;
	dst = (UTCHAR *)dststr;
	for ( ; *src ; src++ ){
		#ifdef UNICODE
		if ( *src == L'ー' ){
			*dst++ = L'ｰ';
			continue;
		}
		if ( *src == L'・' ){
			*dst++ = L'･';
			continue;
		}
		if ( *src == L'￥' ){
			*dst++ = L'\\';
			continue;
		}
		if ( (*src >= L'！') && (*src <= L'～') ){
			*dst++ = (UTCHAR)(*src - (L'！' - L'!'));
			continue;
		}
		if ( (*src >= L'　') && (*src <= L'。') ){
			if ( *src == L'　' ){
				*dst++ = L' ';
			}else{
				*dst++ = *src == L'。' ? L'｡' : L'､';
			}
			continue;
		}
		if ( (*src >= L'「') && (*src <= L'」') ){
			*dst++ = (WCHAR)(*src + (L'｢' - L'「'));
			continue;
		}

		if ( (*src >= L'゛') && (*src <= L'ヴ') ){
			WCHAR c, off;
			const WCHAR *p;

			if ( *src == L'ヴ' ){
				*dst++ = L'ｳ';
				*dst++ = L'ﾞ';
				continue;
			}

			c = *src;
			for ( p = ZHtbl ; *p ; p++ ){
				if ( *p != c ) continue;
				off = (WCHAR)(p - ZHtbl);
				if ( off < 0x3f ){
					c = (WCHAR)(L'｡' + off);
					break;
				}
				if ( off < (0x3f + 20) ){
					if ( off >= (0x3f + 15) ) off += (WCHAR)5;
					*dst++ = (WCHAR)(L'｡' + off - 0x3f + 21);
					c = L'ﾞ';
					break;
				}
				*dst++ = (WCHAR)(L'｡' + off - 0x3f -20 + 41);
				c = L'ﾟ';
				break;
			}
			*dst++ = c;
			continue;
		}
		#else
		const BYTE *p;
		BYTE c1, c2;

		if ( IskanjiA(*src) ){
			if ( (*src >= 0x81) && (*src <= 0x83) ){
				c1 = *src;
				c2 = *(src + 1);
				for( p = ZHtbl ; *p ; p += 2){
					if ( (*(p + 1) == c2 ) && (*p == c1) ){
						c1 = (BYTE)(((p - ZHtbl) >> 1) + 0x20);
						c2 = 0;
						if ( c1 >= (BYTE)0x80 ) c1 += (BYTE)0x20;
						if ( c1 >= 0xe0 ){
							if ( c1 < 0xf4 ){
								c1 -= (c1 < 0xef) ? (BYTE)0x2a : (BYTE)0x25;
								c2 = 'ﾞ';
							}else{
								c1 -= (BYTE)0x2a;
								c2 = 'ﾟ';
							}
						}
						src++;
						*dst++ = c1;
						if (c2) *dst++ = c2;
						goto next;
					}
				}
				if ( (c1 == (BYTE)0x83) && (c2 == (BYTE)0x94) ){ // ヴ
					src++;
					*dst++ = 'ｳ';
					*dst++ = 'ﾞ';
					goto next;
				}
			}
			*dst++ = *src++;
		}
		#endif
		*dst++ = *src;
#ifndef UNICODE
next:
;	// cl.exe は必須？
#endif
	}
	*dst = 0;
	return (TCHAR *)dst;
}

// 漢字対応小文字化 -----------------------------------------------------------
PPXDLL TCHAR * PPXAPI Strlwr(TCHAR *str)
{
	TCHAR *ptr, type;

	for ( ptr = str ; *ptr ; ptr++){
	#ifdef UNICODE
		type = *ptr;
		if ( (type >= 'A') && (type <= 'Z') ){
			*ptr += (TCHAR)('a' - 'A');
			continue;
		}
		if ( (type >= L'Ａ') && (type <= L'Ｚ') ){
			*ptr += (TCHAR)(L'ａ' - L'Ａ');
			continue;
		}
	#else
		type = T_CHRTYPE[(unsigned char)(*ptr)];
		if ( type & T_IS_KNJ ){			// 漢字の場合の処理
			unsigned char code;

			code = *(unsigned char *)ptr++;
			if ( !code ) break;
			if ( (code == 0x82) &&	((unsigned char)*ptr >= 0x60) &&
									((unsigned char)*ptr <= 0x79) ){
				*ptr += (char)0x21;
			}
		}else{
			if (type & T_IS_UPP) *ptr += (char)0x20;
		}
	#endif
	}
	return ptr;
}

// 漢字対応大文字化 -----------------------------------------------------------
PPXDLL TCHAR * PPXAPI Strupr(TCHAR *str)
{
	TCHAR *ptr, type;

	for ( ptr = str ; *ptr ; ptr++){
	#ifdef UNICODE
		type = *ptr;
		if ( (type >= 'a') && (type <= 'z') ){
			*ptr -= (TCHAR)('a' - 'A');
			continue;
		}
		if ( (type >= L'ａ') && (type <= L'ｚ') ){
			*ptr -= (TCHAR)(L'ａ' - L'Ａ');
			continue;
		}
	#else
		type = T_CHRTYPE[(unsigned char)(*ptr)];
		if ( type & T_IS_KNJ ){			// 漢字の場合の処理
			unsigned char code;

			code = *(unsigned char *)ptr++;
			if ( !code ) break;
			if ( (code == 0x82) &&	((unsigned char)*ptr >= 0x81) &&
									((unsigned char)*ptr <= 0x9A) ){
				*ptr -= (char)0x21;
			}
		}else{
			if (type & T_IS_LOW) *ptr -= (char)0x20;
		}
	#endif
	}
	return ptr;
}

/*-----------------------------------------------------------------------------
	空白を読み飛ばす(書き換え有り)
-----------------------------------------------------------------------------*/
TCHAR *SkipSpaceAndFix(TCHAR *str)
{
	while( *str != '\0' ){
		if ( *str == ' ' ){
			str++;
			continue;
		}
		if ( *str == '\t' ){
			*str++ = ' ';
			continue;
		}
		if ( (*str == '\r') || (*str == '\n') ){
			*str = '\0';
			if ( (*(str + 1) == '\r') || (*(str + 1) == '\n') ) str++;
		}
		break;
	}
	return str;
}

const TCHAR *EscapeMacrochar(const TCHAR *string, TCHAR *buf)
{
	const TCHAR *src;

	src = tstrchr(string, '%');
	if ( src == NULL ) return string;
	tstrcpy(buf, string);
	tstrreplace(buf, T("%"), T("%%"));
	return buf;
}

typedef struct {
	void *buffer;
	int buflen;
	const TCHAR *src;
	TCHAR *dest;
	int left, columns;
	TCHAR space;
} THPRINTF_STRUCT;


#define thprintf_dest(buffer) ((TCHAR *)(char *)(((ThSTRUCT *)(buffer))->bottom + ((ThSTRUCT *)(buffer))->top))
#define thprintf_left(buffer) ((size_t)(((ThSTRUCT *)(buffer))->size - ((ThSTRUCT *)(buffer))->top) / sizeof(TCHAR) - 1)
#define thprintf_set_top(buffer, dest) (((ThSTRUCT *)(buffer))->top = (char *)(dest) - ((ThSTRUCT *)(buffer))->bottom)

#if defined(__WINE__) && defined(_WIN64)
	// x64 の gcc + libc の場合は、va_list が配列のため、キャストが必要
	// mingw の場合は、配列では無い
	#define VA_LIST_POINTER(arglist) ((va_list *)(arglist))
#else
	#define VA_LIST_POINTER(arglist) (&(arglist))
#endif

int thprintf_expand_main(void *buffer, TCHAR *dest, int reqsize)
{
	*dest = '\0';
	thprintf_set_top(buffer, dest);

	if ( ThSize((ThSTRUCT *)buffer, TSTROFF(reqsize)) != FALSE ){
	//	printf("[thprintf_expand realloc:%d]", ((ThSTRUCT *)buffer)->size);
		return thprintf_left(buffer);
	}
	// printf("[thprintf_expand realloc x]");
	return 0;
}

#define thprintf_check_left(buffer, buflen, dest, left, reqlen, message) { \
	if ( reqlen > left ){ \
		int nleft; \
		if ( ((buflen) > 0) || ((nleft = thprintf_expand_main(buffer, dest, reqlen)) == 0) ){ \
			XMessage(NULL, NULL, XM_DbgLOG, T("thprintf over 2: %s"), message);\
			reqlen = left; \
		}else{ \
			left = nleft; \
			dest = thprintf_dest(buffer); \
		} \
	} \
}

#define thprintf_spacing(buffer, buflen, dest, left, srclength, columns, space, message){ \
	if ( srclength < columns ){ \
		thprintf_check_left(buffer, buflen, dest, left, columns, message); \
		columns -= srclength; \
		if ( columns > 0 ){ \
			left -= columns; \
			for (;;){ \
				*dest++ = space; \
				if ( --columns == 0 ) break; \
			} \
		} \
	} \
}

#define HEX_LEN 8
#define HEX64_POINTERLEN 4 // PML4 のときは 56bit
#define HEX64_UPLEN 16
int thprintf_hex32(DWORD num, TCHAR high, TCHAR *buf)
{
	int part;

	for ( part = HEX_LEN; ; ) {
		UTCHAR chr;

		chr = (UTCHAR)(num & 0xf);
		buf[--part] = (TCHAR)(UTCHAR)(chr + ((chr >= 10) ? high : '0'));
		if ( num < 16 ) break;
		num >>= 4;
	}
	return part;
}

int thprintf_hex64(UINTHL num, TCHAR high, TCHAR *buf, int uplen)
{
	DWORD numL = num.s.H;
	int hpart = uplen, lpart;

	// 上位 DWORD
	if ( numL != 0 ){
		if ( numL & 0xffff0000 ){ // PML5 以降 64bit or 0xfxxx...
			hpart = uplen = HEX_LEN;
		}
		for ( ; ; ) {
			UTCHAR chr;

			chr = (UTCHAR)(numL & 0xf);
			buf[--hpart] = (TCHAR)(UTCHAR)(chr + ((chr >= 10) ? high : '0'));
			if ( numL < 16 ) break;
			numL >>= 4;
		}
	}

	// 下位 DWORD
	numL = num.s.L;
	for ( lpart = HEX_LEN; ; ) {
		UTCHAR chr;

		chr = (UTCHAR)(numL & 0xf);
		buf[--lpart + uplen] = (TCHAR)(UTCHAR)(chr + ((chr >= 10) ? high : '0'));
		if ( numL < 16 ) break;
		numL >>= 4;
	}

	// 調整
	buf[HEX64_UPLEN] = (TCHAR)uplen;
	if ( hpart != uplen ){ // 上位あり
		for ( ; lpart; ) buf[--lpart + uplen] = '0';
		return hpart;
	}else{ // 下位のみ
		return lpart + uplen;
	}
}

int thprintf_extra(THPRINTF_STRUCT *ts, t_va_list *arglist) // %Mx
{
	TCHAR buf[VFPS];
	TCHAR c;
	int length;

	c = *ts->src++;
	if ( (c == 'd') || (c == 'u') ){ // %Md 単位型 UINT32
		DWORD num;

		num = t_va_arg(*arglist, DWORD);
		length = FormatNumber(buf, XFN_DECIMALPOINT | XFN_MINKILO, ts->columns ? ts->columns : ts->left, num, 0) - buf;
	}else if ( (c == 'D') || (c == 'U') ){ // %Md 単位型 UINT32
		DWORD num;

		num = t_va_arg(*arglist, DWORD);
		length = FormatNumber(buf, XFN_DECIMALPOINT | XFN_MINKILO | XFN_HEXUNIT, ts->columns ? ts->columns : ts->left, num, 0) - buf;
	}else if ( (c == 'L') && ((*ts->src == 'd') || (*ts->src == 'u')) ){ // %MLd 単位型 UINT64
		UINTHL num;

		ts->src++;
		num.rawdata = t_va_arg(*arglist, INTHL_RAWDATA);
		length = FormatNumber(buf, XFN_DECIMALPOINT | XFN_MINKILO,
				ts->columns ? ts->columns : ts->left, num.s.L, num.s.H) - buf;
	}else if ( (c == 'L') && ((*ts->src == 'D') || (*ts->src == 'U')) ){ // %MLd 単位型 UINT64(1024)
		UINTHL num;

		ts->src++;
		num.rawdata = t_va_arg(*arglist, INTHL_RAWDATA);
		length = FormatNumber(buf, XFN_DECIMALPOINT | XFN_MINKILO | XFN_HEXUNIT,
				ts->columns ? ts->columns : ts->left, num.s.L, num.s.H) - buf;
	}else if ( c == 'F' ){ // %MF FILETIME (UTC)
		FILETIME *utc, local;
		SYSTEMTIME st;

		utc = t_va_arg(*arglist, FILETIME *);
		if ( (utc->dwLowDateTime | utc->dwHighDateTime) == 0 ){ // 未定義
			memset(&st, 0, sizeof(st));
		}else{
			FileTimeToLocalFileTime(utc, &local);
			FileTimeToSystemTime(&local, &st);
		}
		length = thprintf(buf, 64, T("%04d-%02d-%02d %02d:%02d:%02d.%03d"),
				st.wYear, st.wMonth, st.wDay,
				st.wHour, st.wMinute, st.wSecond, st.wMilliseconds) - buf;
	}else if ( c == 'm' ){ // %Mm Win32 error message
		PPErrorMsg(buf, (ERRORCODE)t_va_arg(*arglist, ERRORCODE));
		length = tstrlen(buf);
	/*
	}else if ( c == 's' ){ // %Ms MessageText 処理あり文字列
		const TCHAR *str;

		str = t_va_arg(*arglist, const TCHAR *);
		length = (str != NULL) ? tstrlen(str) : 0;
		thprintf_spacing(ts->buffer, ts->buflen, ts->dest, ts->left, length, ts->columns, ts->space, NilStr);
		thprintf_check_left(ts->buffer, ts->buflen, ts->dest, ts->left, length, NilStr)
		memcpy(ts->dest, str, TSTROFF(length));
		ts->dest += length;
		ts->left -= length;
		return length;
	*/
	}else{
		*ts->dest++ = '%';
		*ts->dest++ = 'M';
		*ts->dest++ = c;
		return 0;
	}
	thprintf_spacing(ts->buffer, ts->buflen, ts->dest, ts->left, length, ts->columns, ts->space, NilStr);
	thprintf_check_left(ts->buffer, ts->buflen, ts->dest, ts->left, length, NilStr)
	memcpy(ts->dest, buf, TSTROFF(length));
	ts->dest += length;
	ts->left -= length;
	return length;
}


PPXDLL TCHAR * USECDECL thprintf(_Out_writes_opt_z_(buflen) void *buffer, int buflen, _In_z_ const TCHAR *message, ...)
{
	t_va_list arglist;
	TCHAR *result;

	t_va_start(arglist, message);
	result = thvprintf(buffer, buflen, message, arglist);
	t_va_end(arglist);
	return result;
}

PPXDLL TCHAR * PPXAPI thvprintf(_Out_writes_opt_z_(buflen) void *buffer, int buflen, _In_z_ const TCHAR *message, t_va_list arglist)
{
	const TCHAR *src = message;
	TCHAR buf[64];
	TCHAR *dest;
	int left, columns;
	TCHAR space;
	BOOL separator;

	if ( buflen > 0 ){
		dest = (TCHAR *)buffer;
		left = buflen - 1;
	}else{
		ThAllocCheck((ThSTRUCT *)buffer);
		dest = thprintf_dest(buffer);
		left = thprintf_left(buffer);
	}
	for(;;){
		TCHAR c;
		int length;

		c = src[0];
		if ( c == '\0' ) break;
		if ( left <= 0 ){
			if ( (buflen > 0) || (left = thprintf_expand_main(buffer, dest, tstrlen(src))) == 0 ){
				XMessage(NULL, NULL, XM_DbgLOG, T("thprintf over 1: %s"), message);
				break;
			}
			dest = thprintf_dest(buffer);
		}
		src++;
		if ( c != '%' ){
			*dest++ = c;
			left--;
			continue;
		}
		// '%' エスケープ
		c = *src++;
		columns = 0;
		space = ' ';
		separator = FALSE;

		if ( c == '\'' ){ // ' 桁区切り
			separator = TRUE;
			c = *src++;
		}

		if ( (UTCHAR)(c - '0') < 10 ){ // 数値...フィールド幅
			if ( c == '0' ) space = '0';
			for(;;){
				columns = columns * 10 + (c - '0');
				c = *src++;
				if ( (UTCHAR)(c - '0') >= 10 ) break;
			}
		}

		if ( c == 's' ){ // %s  TCHAR 文字列
			const TCHAR *str;

			str = t_va_arg(arglist, const TCHAR *);
			length = (str != NULL) ? tstrlen(str) : 0;
			thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
			thprintf_check_left(buffer, buflen, dest, left, length, message);
			memcpy(dest, str, TSTROFF(length));
			dest += length;
			left -= length;
			continue;
		}else if ( c == 'd' ){ // %d  int32
			int num;
			BOOL minus;

			num = t_va_arg(arglist, int);

			if ( num < 0 ){
				left--;
				num = -num;
				if ( columns > 0 ) columns--;
				if ( space == ' ' ){
					minus = TRUE;
				}else{
					*dest++ = '-';
					minus = FALSE;
				}
			}else{
				minus = FALSE;
			}
			if ( separator ){
				length = FormatNumber(buf, XFN_SEPARATOR | XFN_DECIMALPOINT, columns ? columns : left, num, 0) - buf;
			}else{
				length = Numer32ToString(buf, (DWORD)num) - buf;
			}
			thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
			thprintf_check_left(buffer, buflen, dest, left, length, message);
			if ( minus ) *dest++ = '-';
			memcpy(dest, buf, TSTROFF(length));
			dest += length;
			left -= length;
			continue;
		}else if ( c == 'u' ){ // %u  UINT32
			DWORD num;

			num = t_va_arg(arglist, DWORD);
			if ( separator ){
				length = FormatNumber(buf, XFN_SEPARATOR | XFN_DECIMALPOINT, columns ? columns : left, num, 0) - buf;
			}else{
				length = Numer32ToString(buf, (DWORD)num) - buf;
			}
		}else if ( c == 'c' ){ // %c  1 TCHAR
			*dest++ = (TCHAR)t_va_arg(arglist, int); // char / word の x86/x64アライメントは int 相当
			left--;
			continue;
		}else if ( c == '%' ){ // %%  % 自身
			*dest++ = c;
			left--;
			continue;
		}else if ( (c == 'x') || (c == 'X') || (c == 'p') ){
			int part;
			int length;

			if ( c == 'p' ){ // %p  UINT32/UINT64 pointer(%08X)
				#ifdef _WIN64
					UINTHL numHL;

					if ( columns == 0 ) columns = 12;
					numHL.HL = t_va_arg(arglist, unsigned __int64);
					part = thprintf_hex64(numHL, (TCHAR)('A' - 10),
							buf, HEX64_POINTERLEN);
					length = HEX_LEN + buf[HEX64_UPLEN] - part;
				#else // WIN32
					if ( columns == 0 ) columns = 8;
					part = thprintf_hex32(t_va_arg(arglist, DWORD), (TCHAR)('A' - 10), buf);
					length = HEX_LEN - part;
				#endif
				space = '0';
			}else{ // %x %X  UINT32 hex
				part = thprintf_hex32(t_va_arg(arglist, DWORD),
					(c == 'x') ? (TCHAR)('a' - 10) : (TCHAR)('A' - 10), buf);
				length = 8 - part;
			}

			thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
			thprintf_check_left(buffer, buflen, dest, left, length, message);
			memcpy(dest, buf + part, TSTROFF(length));
			dest += length;
			left -= length;
			continue;
		}else if ( c == 'L' ){ // long double / (独自) long long prefix
			c = *src++;
			if ( c == 'd' ){ // %Ld  int64
				int length;
				UINTHL num;
				BOOL minus;

				num.rawdata = t_va_arg(arglist, INTHL_RAWDATA);
			#if _INTEGRAL_MAX_BITS >= 64
				if ( (__int64)num.HL < 0 ){
					left--;
					num.HL = (unsigned __int64)-(__int64)num.HL;
					if ( columns > 0 ) columns--;
					if ( space == ' ' ){
						minus = TRUE;
					}else{
						*dest++ = '-';
						minus = FALSE;
					}
				}else{
					minus = FALSE;
				}
			#else
				if ( (LONG)num.s.H < 0 ){
					left--;
					num.s.H = ~num.s.H;
					if ( num.s.L == 0 ) num.s.H++;
					num.s.L = ~num.s.L;
					num.s.L++;
					if ( columns > 0 ) columns--;
					if ( space == ' ' ){
						minus = TRUE;
					}else{
						*dest++ = '-';
						minus = FALSE;
					}
				}else{
					minus = FALSE;
				}
			#endif
				if ( separator ){
					length = FormatNumber(buf, XFN_SEPARATOR | XFN_DECIMALPOINT, columns ? columns : left, num.s.L, num.s.H) - buf;
				}else{
					length = Numer64ToString(buf, num) - buf;
				}
				thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
				thprintf_check_left(buffer, buflen, dest, left, length, message);
				if ( minus ) *dest++ = '-';
				memcpy(dest, buf, TSTROFF(length));
				dest += length;
				left -= length;
				continue;
			}else if ( c == 'u' ){ // %Lu  UINT64
				UINTHL num;

				num.rawdata = t_va_arg(arglist, INTHL_RAWDATA);
				if ( separator ){
					length = FormatNumber(buf, XFN_SEPARATOR | XFN_DECIMALPOINT, columns ? columns : left, num.s.L, num.s.H) - buf;
				}else{
					length = Numer64ToString(buf, num) - buf;
				}
			}else if ( (c == 'x') || (c == 'X') ){ // %Lx %LX  UINT64 hex
				int part;
				int length;
				UINTHL numHL;

				numHL.rawdata = t_va_arg(arglist, INTHL_RAWDATA);
				part = thprintf_hex64(numHL,
						(c == 'x') ? (TCHAR)('a' - 10) : (TCHAR)('A' - 10),
						buf, HEX_LEN);
				length = HEX_LEN + buf[HEX64_UPLEN] - part;

				thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
				thprintf_check_left(buffer, buflen, dest, left, length, message);
				memcpy(dest, buf + part, TSTROFF(length));
				dest += length;
				left -= length;
				continue;
			}else if ( c == 's' ){ // %Ls  UNICODE strings
			#ifdef UNICODE
				const WCHAR *strW;

				strW = t_va_arg(arglist, const WCHAR *);
				length = strlenW(strW);
				thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
				thprintf_check_left(buffer, buflen, dest, left, length, message);
				memcpy(dest, strW, TSTROFF(length));
			#else
				const WCHAR *strW;

				strW = t_va_arg(arglist, const WCHAR *);
				length = WideCharToMultiByte(CP_ACP, 0, strW, -1, NULL, 0, NULL, NULL);
				thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
				thprintf_check_left(buffer, buflen, dest, left, length, message);
				length = WideCharToMultiByte(CP_ACP, 0, strW, -1, dest, length, NULL, NULL);
				if ( length > 0 ) length--;
			#endif
				dest += length;
				left -= length;
				continue;
			}else{
				*dest++ = '%';
				*dest++ = 'L';
				*dest++ = c;
				continue;
			}
		}else if ( c == 'M' ){ // 特殊な形式
			THPRINTF_STRUCT ts;

			ts.buffer = buffer;
			ts.buflen = buflen;
			ts.src = src;
			ts.dest = dest;
			ts.left = left;
			ts.columns = columns;
			ts.space = space;
			thprintf_extra(&ts, VA_LIST_POINTER(arglist));
			src = ts.src;
			dest = ts.dest;
			left = ts.left;
			continue;
		}else if ( c == 'h' ){ // ANSI指定
			c = *src++;
			if ( c == 's' ){ // %hs  ansi string
			#ifdef UNICODE
				const char *strA;

				strA = t_va_arg(arglist, const char *);
				length = MultiByteToWideChar(CP_ACP, 0, strA, -1, NULL, 0);
				thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
				thprintf_check_left(buffer, buflen, dest, left, length, message);
				length = MultiByteToWideChar(CP_ACP, 0, strA, -1, dest, length);
				if ( length > 0 ) length--;
			#else
				const char *str;

				str = t_va_arg(arglist, const char *);
				length = strlen(str);
				thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
				thprintf_check_left(buffer, buflen, dest, left, length, message);
				memcpy(dest, str, TSTROFF(length));
			#endif
				dest += length;
				left -= length;
				continue;
			}else{
				*dest++ = '%';
				*dest++ = 'h';
				*dest++ = c;
				continue;
			}
		}else if ( c == '\0' ){ // 末尾 %
			*dest++ = '%';
			break;
		}else{ // 不明
			*dest++ = c;
			continue;
		}
		thprintf_spacing(buffer, buflen, dest, left, length, columns, space, message);
		thprintf_check_left(buffer, buflen, dest, left, length, message);
		memcpy(dest, buf, TSTROFF(length));
		dest += length;
		left -= length;
		continue;

//		if ( c == 'a' ) // 浮動小数点 e
//		if ( c == 'A' ) // 浮動小数点 E
//		if ( c == 'c' ) // (ISO)char / (MS)TCHAR  1 文字
//		if ( c == 'C' ) // (ISO)wchar / (MS)現在と異なる char  1 文字
//		if ( c == 'd' ) // int
//		if ( c == 'e' ) // 浮動小数点 e
//		if ( c == 'E' ) // 浮動小数点 E
//		if ( c == 'f' ) // 浮動小数点 e
//		if ( c == 'F' ) // 浮動小数点 E
//		if ( c == 'g' ) // 浮動小数点 e
//		if ( c == 'G' ) // 浮動小数点 E
//		if ( c == 'h' ) // char prefix
//		if ( c == 'l' ) // wchar prefix
//		if ( c == 'i' ) // 符号付き数値
//		if ( c == 'o' ) // 符号無し 8進数
//		if ( c == 'n' ) // 指定位置までの文字数を保存する
//		if ( c == 'm' ) // (glibc)エラーコードに対応したメッセージ出力
//		if ( c == 'p' ) // ポインタ値(16進値)
//		if ( c == 's' ) // (ISO)char / (MS)TCHAR  文字列
//		if ( c == 'S' ) // (ISO)wchar / (MS)現在と異なる char  文字列
//		if ( c == 'u' ) // UINT
//		if ( c == 'x' ) // 符号無し 16進数 小文字
//		if ( c == 'X' ) // 符号無し 16進数 大文字
//		if ( c == 'Z' ) // (MS)xxx_STRING struct 文字列
// 未採用 フラグ文字 # - + 空白 I
// 未採用 長さ修飾子 h:(d等)short, (MS c)char, hh:(d等)char,
//		I:(MS)pointer/size_t,(glibc)ロケール依存数字 I32:(MS), I64:(MS),
//		j:intmax, l:(MS)long double, (d等)long int, (c)wchar,  L:long double,
//		ll:long long int, q, t:pointer, w:(MS c,s)wchar, z:size_t,
//		 *(引数で指定した長さ)
// 未採用 変換指定子 m

	}
	*dest = '\0';
	if ( buflen <= 0 ){
		thprintf_set_top(buffer, dest);
		// ADD 相当にするには、 (top) \0 を \0 (top) \0 にする
		if ( buflen == THP_ADD ){
			if ( (((ThSTRUCT *)buffer)->top + sizeof(TCHAR) * 2) < ((ThSTRUCT *)buffer)->size ){
				ThSize((ThSTRUCT *)buffer, sizeof(TCHAR) * 2);
				dest = thprintf_dest(buffer);
			}
			((ThSTRUCT *)(buffer))->top += sizeof(TCHAR);
			*(dest + 1) = '\0';
		}
	}
	return dest;
}


typedef struct {
	TCHAR *ptr;
	TCHAR endbraket;
} BRAKETPAIRS;

#define BRAKETPAIRS_MAX 16

// ●Multibyte の場合、漢字も1文字なので、着色位置にずれが生じる 2.07
#define SetSyntaxAttr(etsattr, start, end, attr) memset((etsattr) + (start), (attr), (end) - (start))

#define SYNTAXPATH_DIRPATH 0
#define SYNTAXPATH_FILEPATH 1
#define SYNTAXPATH_CMDLINE 2
#define SYNTAXPATH_FILENAME 3

void SyntaxPathColor(BYTE *attr, TCHAR *text, TCHAR *ptr, TCHAR *ptrmax, int mode)
{
	TCHAR sep, *pathptr, *ext = NULL;
	int drivemode;

	sep = *ptr;
	if ( (sep == '\"') || (sep == '\'') ){
		ptr++;
	}else{
		sep = '\0';
	}

	pathptr = VFSGetDriveType(ptr, &drivemode, NULL);
	if ( (pathptr == NULL) || (pathptr == ptr) ){ // 相対指定
		if ( mode == SYNTAXPATH_CMDLINE ) return;
		if ( mode != SYNTAXPATH_FILENAME ){
			SetSyntaxAttr(attr, ptr - text, ptrmax - text, SYNTAXCOLOR_path_part);
		}
		pathptr = ptr;
	}else{ // 絶対指定のドライブ部分
		if ( drivemode == VFSPT_UNC ){
			TCHAR *entry;

			entry = FindPathSeparator(pathptr);
			if ( entry == NULL ) entry = pathptr + tstrlen(pathptr);
			// *\\server*\share\path
			SetSyntaxAttr(attr, ptr - text, entry - text, SYNTAXCOLOR_path_drive);
			if ( *entry != '\0' ){
				// \\server*\*share\path
				SetSyntaxAttr(attr, entry - text, entry - text + 1, SYNTAXCOLOR_path_separater);
				pathptr = entry + 1;

				entry = FindPathSeparator(pathptr);
				if ( entry == NULL ) entry = pathptr + tstrlen(pathptr);
				// \\server\*share*\path
				SetSyntaxAttr(attr, pathptr - text, entry - text, SYNTAXCOLOR_path_drive);
			}
			pathptr = entry;
		}else{
			// *c:*\path
			SetSyntaxAttr(attr, ptr - text, pathptr - text, SYNTAXCOLOR_path_drive);
			if ( (*pathptr != '\\') && (*pathptr != '/') && (pathptr < ptrmax) ){
				SetSyntaxAttr(attr, pathptr - text, ptrmax - text, SYNTAXCOLOR_path_part);
			}
		}
	}

	for (;;){
		TCHAR chr;
		int ec;

		if ( pathptr >= ptrmax ) break;
		chr = *pathptr;
		if ( chr == '\0' ) break;
		if ( chr == sep ) break;
		if ( chr == '.' ){
			ext = pathptr++;
			continue;
		}
		if ( (chr == '/') || (chr == '\\') ){
			if ( mode == SYNTAXPATH_FILENAME ){
				ec = SYNTAXCOLOR_illigal;
			}else{
				ec = SYNTAXCOLOR_path_separater;
			}
			ext = NULL;
		}else if ( (chr == '\"') || (chr == '*') || (chr == ':') || (chr == '<') || (chr == '>') || (chr == '?') || (chr == '|') ){
			ec = SYNTAXCOLOR_illigal;
		}else{
			#ifdef UNICODE
				pathptr++;
			#else
				pathptr += Chrlen(chr);
			#endif
			continue;
		}
		SetSyntaxAttr(attr, pathptr - text, pathptr - text + 1, ec);
		pathptr++;
	}
	if ( (ext != NULL) && (mode != SYNTAXPATH_DIRPATH) ){
		SetSyntaxAttr(attr, ext - text, pathptr - text, SYNTAXCOLOR_path_part);
	}
}

void SyntaxCommandColor(BYTE *attr, TCHAR *text, TCHAR *ptr, int commandpos)
{
	int ec = SYNTAXCOLOR_command;
	TCHAR bkchr;

	if ( commandpos >= (ptr - text) ) return;
	bkchr = *ptr;
	*ptr = '\0';
	if ( IsExistCustTable(T("A_exec"), text + commandpos) ){
		ec = SYNTAXCOLOR_alias;
	}
	*ptr = bkchr;
	SetSyntaxAttr(attr, commandpos, ptr - text, ec);
	SyntaxPathColor(attr, text, text + commandpos, ptr, SYNTAXPATH_CMDLINE);
}

TCHAR *SyntaxBraketColor(BYTE *attr, TCHAR *text, TCHAR *ptr, TCHAR endbraket)
{
	int level = 0;
	BRAKETPAIRS bp[BRAKETPAIRS_MAX];
	TCHAR chr;

	bp[level].ptr = ptr;
	bp[level].endbraket = endbraket;
	ptr++;

	for (;;){
		chr = *ptr;
		if ( chr == '\0' ) break;
		if ( chr == bp[level].endbraket ){
			int offset;

			offset = bp[level].ptr - text;
			SetSyntaxAttr(attr, offset, offset + 1, SYNTAXCOLOR_braket1 + (level & 3));
			offset = ptr - text;
			SetSyntaxAttr(attr, offset, offset + 1, SYNTAXCOLOR_braket1 + (level & 3));
			ptr++;
			if ( --level < 0 ) break;
			continue;
		}
		if ( (chr == ')') || (chr == ']') || (chr == '}') ){
			int offset;

			offset = ptr - text;
			SetSyntaxAttr(attr, offset, offset + 1, SYNTAXCOLOR_illigal);
		}else if ( level < (BRAKETPAIRS_MAX - 1) ){
			if ( chr == '(' ){
				level++;
				bp[level].ptr = ptr;
				bp[level].endbraket = ')';
			}else
			if ( chr == '{' ){
				level++;
				bp[level].ptr = ptr;
				bp[level].endbraket = '}';
			}else
			if ( chr == '[' ){
				level++;
				bp[level].ptr = ptr;
				bp[level].endbraket = ']';
			}
		}
		#ifdef UNICODE
			ptr++;
		#else
			ptr += Chrlen(chr);
		#endif
	}
	while ( level >= 0 ){
		int offset;

		offset = bp[level].ptr - text;
		SetSyntaxAttr(attr, offset, offset + 1, SYNTAXCOLOR_illigal);
		level--;
	}
	return ptr;
}

PPXDLL void PPXAPI CommandSyntaxColor(WORD SyntaxType, TCHAR *etstext, BYTE *etsattr)
{
	TCHAR *ptr;

	if ( SyntaxType & (PPXH_DIR | PPXH_PATH | PPXH_PPCPATH | PPXH_PPVNAME | PPXH_FILENAME) ){	// パス
		int mode;

		if ( SyntaxType & PPXH_DIR ){	// パス
			mode = SYNTAXPATH_DIRPATH;
		}else if ( SyntaxType & (PPXH_PPCPATH | PPXH_PPVNAME | PPXH_PATH) ){	// パス
			mode = SYNTAXPATH_FILEPATH;
		}else{
			mode = SYNTAXPATH_FILENAME;
		}
		SyntaxPathColor(etsattr, etstext, etstext, etstext + tstrlen(etstext), mode);
	}else if ( SyntaxType & (PPXH_MASK | PPXH_SEARCH) ){	// 検索
		TCHAR chr;

		ptr = etstext;
		for (;;){
			chr = *ptr;
			if ( chr == '\0' ) break;
			if ( chr == '\t' ){
				SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + 1, SYNTAXCOLOR_illigal);
				ptr++;
				continue;
			}
			if ( chr == ' ' ){
				ptr++;
				continue;
			}
#ifdef UNICODE
			if ( chr == L'　' ){
				SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + 1, SYNTAXCOLOR_illigal);
				ptr++;
				continue;
			}
#endif
			if ( (chr == '*') || (chr == '?') || (chr == '/') ){
				SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + 1, SYNTAXCOLOR_path_separater);
				ptr++;
				continue;
			}

			if ( (chr == ';') || (chr == ',') ){
				SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + 1, SYNTAXCOLOR_command_separater);
				ptr++;
				continue;
			}
			ptr++;
		}
	}else if ( SyntaxType & (PPXH_COMMAND) ){	// コマンドライン
		int commandpos = -1; // -1 コマンド前空白 0≦ コマンド -2 コマンド終わり
		BOOL sepok = FALSE; // "/' による括り中
		TCHAR *lastcheckbraket = etstext; // ( ) チェック済みの末端
		TCHAR *wordstart; // 単語の開始場所
		TCHAR chr;

		wordstart = ptr = etstext;
		for (;;){
			chr = *ptr;
			if ( chr == '\0' ) break;
			if ( chr == '\t' ){
				SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + 1, SYNTAXCOLOR_illigal);
				ptr++;
				wordstart = ptr;
				continue;
			}

			if ( chr == '%' ){
				int len = 2;
				const TCHAR *ptr2;
				int ec;

				if ( *(ptr + 1) == '%' ){
					if ( commandpos == -1 ){
						commandpos = ptr - etstext;
					}
					ptr += 2;
					continue;
				}

				if ( *(ptr + 1) == ':' ){
					if ( commandpos >= 0 ){
						SyntaxCommandColor(etsattr, etstext, ptr, commandpos);
					}
					SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + 2, SYNTAXCOLOR_command_separater);
					ptr += 2;
					commandpos = -1;
					continue;
				}

				if ( *(ptr + 1) == 'O' ){
					ptr2 = ptr + 2;
					for (;;){
						if ( Isalpha(*ptr2) || (*ptr2 == '+') || (*ptr2 == '-') ){
							ptr2++;
							continue;
						}
						break;
					}
					len = ptr2 - ptr;
					ec = SYNTAXCOLOR_macro_letter;
				}else{
					if ( commandpos == -1 ){
						commandpos = ptr - etstext;
					}
					if ( (*(ptr + 1) == '\"') || (*(ptr + 1) == '\'') ){
						len = 1;
					}

					ptr2 = ptr + 1;
					if ( *ptr2 == '*' ){
						ptr2++;
						while ( Isalnum(*ptr2) ) ptr2++;
						len = ptr2 - ptr;
						ec = SYNTAXCOLOR_functions;
					}else{
						ec = SYNTAXCOLOR_macro_letter;
					}
				}

				SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + len, ec);
				ptr += len;
				continue;
			}

			if ( !sepok ){
				if ( chr == ' ' ){
					if ( commandpos >= 0 ){
						SyntaxCommandColor(etsattr, etstext, ptr, commandpos);
						commandpos = -2;
					}
					ptr++;
					wordstart = ptr;
					continue;
				}
#ifdef UNICODE
				if ( chr == L'　' ){
					SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + 1, SYNTAXCOLOR_illigal);
					ptr++;
					continue;
				}
#endif

				if ( ptr == wordstart ){
					if ( chr == '-' ){
						TCHAR *ptr2;

						for ( ptr2 = ptr + 1; *ptr2 != '\0'; ptr2++){
							if ( Isalnum(*ptr2) ) continue;
							if ( *ptr2 == '-' ) continue;
							if ( (*ptr2 == ':') || (*ptr2 == '=') ){
								ptr2++;
								break;
							}
							break;
						}
						SetSyntaxAttr(etsattr, ptr - etstext, ptr2 - etstext, SYNTAXCOLOR_options);
						ptr = ptr2;
						continue;
					}else{
						TCHAR *pathptr;

						pathptr = VFSGetDriveType(ptr, NULL, NULL);
						if ( pathptr != NULL ){
							for (;;){
								TCHAR chr;

								chr = *pathptr;
								if ( (UTCHAR)chr <= ' ' ) break;
								if ( (chr == '\"') || (chr == '\'') ) break;
								pathptr++;
							}
							SyntaxPathColor(etsattr, etstext, ptr, pathptr, SYNTAXPATH_CMDLINE);
						}
					}
				}

				if ( (chr == '<') || (chr == '>') ){
					TCHAR *p;

					p = ptr++;
					while ( (p > etstext) && Isdigit(*(p - 1)) ) p--;
					if ( *ptr == '&' ){
						ptr++;
						while ( Isdigit(*ptr) ) ptr++;
					}
					if ( commandpos >= 0 ){
						SyntaxCommandColor(etsattr, etstext, p, commandpos);
					}
					SetSyntaxAttr(etsattr, p - etstext, ptr - etstext, SYNTAXCOLOR_command_separater);
					commandpos = -1;
					continue;
				}

				if ( (chr == '&') || (chr == '|') /*|| (chr == ';')*/ ){
					if ( commandpos >= 0 ){
						SyntaxCommandColor(etsattr, etstext, ptr, commandpos);
					}
					SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + 1, SYNTAXCOLOR_command_separater);
					ptr += 1;
					commandpos = -1;
					continue;
				}
			}

			if ( (chr == '\r') || (chr == '\n') ){
				if ( commandpos >= 0 ){
					SyntaxCommandColor(etsattr, etstext, ptr, commandpos);
				}
				commandpos = -1;
				ptr += 1;
				continue;
			}

			if ( commandpos == -1 ) commandpos = ptr - etstext;

			if ( (chr == '\"') || (chr == '\'') ){
				TCHAR sep;
				int len;
				const TCHAR *ptr2;

				if ( sepok ){
					sepok = FALSE;
					ptr++;
					continue;
				}
				sepok = TRUE;
				sep = *ptr;
				ptr2 = ptr + 1;
				for (;;){
					if ( *ptr2 == '\0' ) break;
					if ( *ptr2 == sep ){
						ptr2++;
						break;
					}
					ptr2++;
				}
				len = ptr2 - ptr;

				if ( commandpos >= 0 ){
					SyntaxCommandColor(etsattr, etstext, ptr + len, commandpos);
					commandpos = -2;
				}else{
					SetSyntaxAttr(etsattr, ptr - etstext, ptr - etstext + len, SYNTAXCOLOR_strings);
					SyntaxPathColor(etsattr, etstext, ptr, ptr + len, SYNTAXPATH_CMDLINE);
				}
				ptr++;
				continue;
			}

			if ( ptr >= lastcheckbraket ){
				if ( chr == '(' ){
					lastcheckbraket = SyntaxBraketColor(etsattr, etstext, ptr, ')');
					ptr++;
					continue;
				}
				if ( chr == '{' ){
					lastcheckbraket = SyntaxBraketColor(etsattr, etstext, ptr, '}');
					ptr++;
					continue;
				}
				if ( chr == '[' ){
					lastcheckbraket = SyntaxBraketColor(etsattr, etstext, ptr, ']');
					ptr++;
					continue;
				}

				if ( (chr == ')') || (chr == ']') || (chr == '}') ){
					int offset;

					offset = ptr - etstext;
					SetSyntaxAttr(etsattr, offset, offset + 1, SYNTAXCOLOR_illigal);
				}
			}
			#ifdef UNICODE
				ptr++;
			#else
				ptr += Chrlen(chr);
			#endif
		}
		if ( commandpos >= 0 ){
			SyntaxCommandColor(etsattr, etstext, ptr, commandpos);
		}
	}
}
