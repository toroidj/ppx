/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library		コマンドライン解析
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include "PPX.H"
#include "PPD_DEF.H"
#pragma hdrstop

/*-----------------------------------------------------------------------------
	行末(\0,\a,\d)か？

RET:	==0 :EOL	!=0 :Code
-----------------------------------------------------------------------------*/
UTCHAR USEFASTCALL IsEOL(const TCHAR **str)
{
	UTCHAR code;

	code = **str;
	if ( code == '\0' ) return '\0';
	if ( code == '\xd' ){
		if ( *(*str + 1) == '\xa' ) (*str)++;
		return '\0';
	}
	if ( code == '\xa' ){
		if ( *(*str + 1) == '\xd' ) (*str)++;
		return '\0';
	}
	return code;
}
/*-----------------------------------------------------------------------------
	空白(space, tab）をスキップする

RET:	==0 :EOL	!=0 :Code
-----------------------------------------------------------------------------*/
PPXDLL UTCHAR PPXAPI SkipSpace(LPCTSTR *str)
{
	UTCHAR code;

	for ( ; ; ){
		code = IsEOL(str);
		if ( (code != ' ') && (code != '\t') ) break;
		(*str)++;
	}
	return code;
}

PPXDLL BOOL PPXAPI NextParameter(LPCTSTR *str)
{
	UTCHAR code;

	if ( IsEOL(str) == '\0' ) return FALSE;
	code = SkipSpace(str);
	if ( code != ',' ){ // 空白のみの区切り？
		return (code != '\0');
	}
	(*str)++;
	SkipSpace(str);
	return TRUE;
}

/*-----------------------------------------------------------------------------
	一つ分の空白区切りパラメータを抽出する。先頭と末尾の空白は除去する

 param	抽出先
 RET	先頭の文字(何もなかったら 0)
-----------------------------------------------------------------------------*/
PPXDLL UTCHAR PPXAPI GetLineParamS(LPCTSTR *str, TCHAR *param, size_t length)
{
	UTCHAR code, bottom;
	const TCHAR *src;
	TCHAR *dst, *dstmax;

	src = *str;
	dst = param;
	dstmax = dst + length - 1;
	bottom = code = SkipSpace(&src);
	if ( code == '\0' ){
		*str = src;
		*dst = '\0';
		return '\0';
	}
	if ( code == '\"' ){
		GetQuotedParameter(&src, dst, dstmax);
	}else{
		do {
			src++;
			if ( code <= ' ' ) break;
			if ( code == '\"' ){
				if ( dst < dstmax ) *dst++ = code;
				while( '\0' != (code = IsEOL(&src)) ){
					src++;
					if ( dst < dstmax ) *dst++ = code;
					if ( (code == '\"') && (*src != '\"') ) break;
				}
			}else{
				if ( dst < dstmax ) *dst++ = code;
			}
		}while( '\0' != (code = IsEOL(&src)) );
		*dst = '\0';
	}
	*str = src;
	return bottom;
}

// 1.86 で GetLineParamS に移行
PPXDLL UTCHAR PPXAPI GetLineParam(LPCTSTR *str, TCHAR *param)
{
	return GetLineParamS(str, param, CMDLINESIZE);
}
/*-----------------------------------------------------------------------------
	パラメータを入手して、オプションかどうかを判別する
	オプションの開始文字は、「/」又は「-」

 optionname オプション名 \0 オプションのパラメータ (要CMDLINESIZE)
 optionparameter オプションのパラメータを示す位置
 RET	== '-':found
-----------------------------------------------------------------------------*/
PPXDLL UTCHAR PPXAPI GetOptionParameter(LPCTSTR *commandline, TCHAR *optionname, TCHAR **optionparameter)
{
	UTCHAR code;

	if ( optionparameter != NULL ) *optionparameter = NilStrNC;
	code = SkipSpace(commandline);
	if ( (code == '-') || (code == '/') ){
		const TCHAR *src;
		TCHAR *dest, *maxptr;

		dest = optionname;
		*dest++ = code;
		maxptr = dest + CMDLINESIZE - 1;
		src = *commandline + 1;
		for ( ;; ){
			code = *src;
			if ( Isalnum(code) == FALSE ) break;
			if ( dest < maxptr ) *dest++ = code;
			src++;
		}
		*dest = '\0';
		Strupr(optionname + 1);
		if ( (*src == ':') && (dest < maxptr) ){ // オプションのパラメータ取得
			if ( optionparameter != NULL ){
				*optionparameter = dest + 1;
				src++;
				dest++;
			}else{
				*dest++ = *src++;
			}
			GetLineParamS(&src, dest, maxptr - dest + 1);
		}else{
			if ( optionparameter != NULL ) *optionparameter = dest;
		}
		*commandline = src;
		return '-';
	}else{
		return GetLineParamS(commandline, optionname, CMDLINESIZE);
	}
}

/* 1.17 で GetOptionParameter に移行 1.86 廃止
PPXDLL UTCHAR PPXAPI GetOption(LPCTSTR *commandline, TCHAR *param)
{
	return GetOptionParameter(commandline, param, NULL);
}
*/
PPXDLL void PPXAPI GetQuotedParameter(LPCTSTR *commandline, TCHAR *param, TCHAR *parammax)
{
	const TCHAR *src, *srcfirst, *srclast;
	TCHAR *dest;

	dest = param;
	srcfirst = src = *commandline + 1; // " をスキップ
	for ( ; ; ){
		TCHAR code;

		code = *src;
		if ( (code == '\0') || (code == '\r') || (code == '\n') ){
			srclast = src;
			break;
		}
		if ( code != '\"' ){
			src++;
			continue;
		}
		// " を見つけた場合の処理
		if ( *(src + 1) != '\"' ){	// "" エスケープ?
			srclast = src++; // 単独 " … ここで終わり
			break;
		}
		// エスケープ処理
		{
			size_t copylength;

			copylength = src - srcfirst + 1; // " 分追加
			if ( (dest + copylength) >= parammax ){ // buffer overflow?
				// " が 1文字エスケープされるので ">=" でok
				srclast = src;
				break; // 後でもう一度 overflow 判定させる
			}
			memcpy(dest, srcfirst, TSTROFF(copylength));
			dest += copylength;
			srcfirst = (src += 2); // " x 2
			continue;
		}
	}
	*commandline = src;
	{
		size_t srclength;

		srclength = srclast - srcfirst;
		if ( (dest + srclength) > parammax ){ // buffer overflow?
			tstrcpy(param, T("<flow!!>"));
			*(parammax - 1) = '\xff';
		}else{
			memcpy(dest, srcfirst, TSTROFF(srclength));
			*(dest + srclength) = '\0';
		}
	}
}

// ,/改行 区切りのパラメータを１つ取得する ※PPC_SUB.Cにも
// "～" の文字列か , 改行 までの文字列(両端の空白は除去)
UTCHAR GetCommandParameter(LPCTSTR *commandline, TCHAR *param, size_t paramlen)
{
	const TCHAR *src;
	TCHAR *dest, *destmax;
	UTCHAR firstcode, code;

	firstcode = SkipSpace(commandline);
	if ( (firstcode == '\0') || (firstcode == ',') ){ // パラメータ無し
		*param = '\0';
		return firstcode;
	}
	dest = param;
	destmax = dest + paramlen - 1;
	if ( firstcode == '\"' ){
		GetQuotedParameter(commandline, param, destmax);
		return *param;
	}
	src = *commandline + 1;
	code = firstcode;
	for ( ;; ){
		if ( dest < destmax ) *dest++ = code;
		code = *src;
		if ( (code == ',') || // (code == ' ') || 途中の空白はパラメータ扱い
			 ((code < ' ') && ((code == '\0') || // (code == '\t') || // tabは無視
							   (code == '\r') || (code == '\n'))) ){
			break;
		}
		src++;
	}
	while ( (dest > param) && (*(dest - 1) == ' ') ) dest--; // choptail
	*dest = '\0';
	*commandline = src;
	return firstcode;
}

UTCHAR GetCommandParameterDual(LPCTSTR *commandline, TCHAR *param, size_t paramlen)
{
	const TCHAR *src;
	TCHAR *dest, *destmax;
	UTCHAR firstcode, code;

	firstcode = SkipSpace(commandline);
	if ( (firstcode == '\0') || (firstcode == ',') ){ // パラメータ無し
		*param = '\0';
		return firstcode;
	}
	if ( firstcode == '\"' ) return GetLineParamS(commandline, param, paramlen);
	src = *commandline + 1;
	dest = param;
	destmax = dest + paramlen - 1;
	code = firstcode;
	for ( ;; ){
		if ( dest < destmax ) *dest++ = code;
		code = *src;
		if ( code == ' ' ){
			if ( tstrchr(src, ',') == NULL ) break;
			src++;
			continue;
		}
		if ( (code == ',') ||
			 ((code < ' ') && ((code == '\0') || // (code == '\t') ||
							   (code == '\r') || (code == '\n'))) ){
			break;
		}
		src++;
	}
	while ( (dest > param) && (*(dest - 1) == ' ') ) dest--;
	*dest = '\0';
	*commandline = src;
	return firstcode;
}

// 改行使用可能なパラメータ取得(空白区切り。「,」区切り不可)
void GetLfGetParam(const TCHAR **param, TCHAR *dest, DWORD destlength)
{
	const TCHAR *src = *param;
	TCHAR *destptr;

	destptr = dest;
	if ( SkipSpace(&src) == '\"' ){ // " 処理
		src++;
		for (;;){
			TCHAR code;

			code = *src;
			if ( code == '\0' ) goto end;
			if ( code == '\"' ){
				if ( *(src + 1) != '\"' ){ // 末尾？
					src++;
					break;
				}
				src++; // "" ... " 自身
			}
			if ( destlength ){
				*destptr++ = code;
				destlength--;
			}
			src++;
			continue;
		}
		dest = destptr; // 末尾空白調整位置を変更
	}

	for (;;){
		TCHAR code;

		code = *src;
		if ( code == '\0' ) break;

		if ( (UTCHAR)code <= ' ' ){
			break;
		}

		if ( code == '\"' ){
			if ( destlength ){
				*destptr++ = code;
				destlength--;
			}
			while( '\0' != (code = IsEOL(&src)) ){
				src++;
				if ( destlength ){
					*destptr++ = code;
					destlength--;
				}
				if ( (code == '\"') && (*src != '\"') ) break;
			}
			dest = destptr; // 末尾空白調整位置を変更
		}else{
			if ( destlength ){
				*destptr++ = code;
				destlength--;
			}
			src++;
		}
		continue;
	}
	while ( (destptr > dest) && (*(destptr - 1) == ' ') ) destptr--;
end:
	*param = src;
	*destptr = '\0';
}

// 改行対応 + 長さ制限可変の オプション取得
UTCHAR GetOptionParameterML(const TCHAR **commandline, TCHAR *optionname, TCHAR **optionparameter, size_t optionlength)
{
	UTCHAR code;

	if ( optionlength == 0 ) return '\0';
	if ( optionparameter != NULL ) *optionparameter = NilStrNC;
	code = SkipSpace(commandline);
	if ( (code == '-') || (code == '/') ){
		const TCHAR *src;
		TCHAR *dest, *maxptr;

		dest = optionname;
		*dest++ = code;
		maxptr = dest + optionlength - 1;
		src = *commandline + 1;
		for ( ;; ){
			code = *src;
			if ( Isalnum(code) == FALSE ) break;
			if ( dest < maxptr ) *dest++ = code;
			src++;
		}
		*dest = '\0';
		Strupr(optionname + 1);
		if ( (*src == ':') && (dest < maxptr) ){ // オプションのパラメータ取得
			if ( optionparameter != NULL ){
				*optionparameter = dest + 1;
				src++;
				dest++;
			}else{
				*dest++ = *src++;
			}
			GetLfGetParam((const TCHAR **)&src, dest, maxptr - dest + 1);
		}else{
			if ( optionparameter != NULL ) *optionparameter = dest;
		}
		*commandline = src;
		return '-';
	}else{
		GetLfGetParam(commandline, optionname, optionlength);
		return code;
	}
}

int GetStringListCommand(const TCHAR *param, const TCHAR *commands)
{
	const TCHAR *oldparam;
	int count;

	oldparam = param;
	count = GetIntNumber(&param);
	if ( oldparam != param ) return count;
	// count = 0 の状態

	while ( *commands ){
		if ( !tstrcmp(param, commands) ) return count;
		commands += tstrlen(commands) + 1;
		count++;
	}
	XMessage(NULL, NULL, XM_GrERRld, StrOptionError, param);
	return -1;
}
