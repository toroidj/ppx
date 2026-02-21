/*-----------------------------------------------------------------------------
	Paper Plane xUI	 Virtual File System				～ List File 処理 ～
-----------------------------------------------------------------------------*/
#define ONVFSDLL		// VFS.H の DLL export 指定
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "PPD_DEF.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "VFS_FF.H"
#pragma hdrstop

const TCHAR TypeName_ListFile[] = T("List");

BOOL GetISODateTime(const TCHAR **lineptr, FILETIME *fftime)
{
	SYSTEMTIME stime;
	FILETIME lftime;
#define TZ_UTC 0
#define TZ_LOCAL 1
	int TimeZoneMode = TZ_LOCAL;
	TCHAR TimeZoneSig;
	DWORD num;
	const TCHAR *aptr;

	const TCHAR *ptr = *lineptr;

						// 日付
	stime.wYear = (WORD)fftime->dwLowDateTime;
	ptr++;

	num = GetNumber((const TCHAR **)&ptr );
	stime.wMonth = (WORD)num;
	if ( (*ptr == '-') || (*ptr == '/') ){
		ptr++;
	} else{
		return FALSE;
	}
	num = GetNumber((const TCHAR **)&ptr );
	stime.wDay = (WORD)num;
					// 時刻(option)
	aptr = ptr;
	if ( *ptr == 'T' ){ // ISO
		ptr++;
	} else{
		if ( *ptr == ' ' ) ptr++;
	}
	num = GetNumber((const TCHAR **)&ptr );
	if ( *ptr != ':' ){
		ptr = aptr;
		stime.wHour = stime.wMinute = stime.wSecond = 0;
	} else{
		stime.wHour = (WORD)num;
		ptr++;
		num = GetNumber((const TCHAR **)&ptr );
		stime.wMinute = (WORD)num;
		if ( *ptr == ':' ){
			ptr++;
			num = GetNumber((const TCHAR **)&ptr );
			stime.wSecond = (WORD)num;
			if ( *ptr == '.' ){ // micro/nano second ?
				ptr++;
				GetNumber((const TCHAR **)&ptr );
			}
		} else{
			stime.wSecond = 0;
		}
		if ( *ptr == 'Z' ){
			TimeZoneMode = TZ_UTC;
			ptr++; // ISO
		} else if ( (*ptr == '+') || (*ptr == '-') ){
			TimeZoneSig = *ptr++;
			TimeZoneMode = GetNumber((const TCHAR **)&ptr ) * 60;
			if ( *ptr == ':' ) ptr++;
			// 0x23c346 = 600,000,000 / 0x100
			TimeZoneMode = (TimeZoneMode + GetNumber((const TCHAR **)&ptr )) * 0x23c346;
		}
	}
	stime.wMilliseconds = 0;
	SystemTimeToFileTime( &stime, &lftime );
	switch ( TimeZoneMode ){
		case TZ_UTC:
			*fftime = lftime;
			break;
		case TZ_LOCAL:
			LocalFileTimeToFileTime( &lftime, fftime );
			break;
		default:
		{ // TimeZoneMode に (FILETIME/0x100) の値が入っている
			DWORD tl = TimeZoneMode << 8;
			if ( TimeZoneSig == '+' ){
				SubDD( lftime.dwLowDateTime, lftime.dwHighDateTime, tl, TimeZoneMode >> 24 );
			} else{
				AddDD( lftime.dwLowDateTime, lftime.dwHighDateTime, tl, TimeZoneMode >> 24 );
			}
			*fftime = lftime;
		}
	}
	*lineptr = ptr;
	return TRUE;
}

void ReadFileTime(const TCHAR **ptr, FILETIME *ft)
{
	TCHAR c;

	GetNumberFILETIME(ptr, ft);
	c = **ptr;
	if ( c == '.' ){
		(*ptr)++;
		ft->dwHighDateTime = ft->dwLowDateTime;
		ft->dwLowDateTime = (DWORD)GetNumber(ptr);
	}else if ( (c == '-') || (c == '/') ){
		GetISODateTime(ptr, ft);
	}
}

TCHAR NextJSONchar(TCHAR **ptr, TCHAR *ptrmax)
{
	for ( ;; ){
		TCHAR c;

		if ( *ptr >= ptrmax ) return '\0';
		c = **ptr;
		if ( (c >= '\1') && (c <= ' ') ){
			(*ptr)++;
			continue;
		}
		return c;
	}
}

void SkipJSONObject(TCHAR **ptr, TCHAR *ptrmax)
{
	TCHAR c;

	c = NextJSONchar(ptr, ptrmax);
	if ( c == '\0' ) return;
	if ( c == '\"' ){ // 文字列
		(*ptr)++;
		for ( ;; ){
			if ( *ptr >= ptrmax ) return;
			c = **ptr;
			if ( c == '\"' ){ // 末尾
				(*ptr)++;
				return;
			}
			if ( (c == '\\') && (*(*ptr + 1) == '\"') ){ // ESC
				(*ptr) += 2;
				continue;
			}
			(*ptr)++;
		}
	}else if ( c == '{' ){ // オブジェクト
		(*ptr)++;
		for ( ;; ){
			c = NextJSONchar(ptr, ptrmax);
			if ( c == '\0' ) return;

			if ( (c == '\"') || (c == '{') || (c == '[') ){ // 文字列／オブジェクト／配列
				SkipJSONObject(ptr, ptrmax);
				continue;
			}
			if ( c == '}' ){ // オブジェクト終わり
				(*ptr)++;
				return;
			}
			(*ptr)++;
		}
	}else if ( c == '[' ){ // 配列
		(*ptr)++;
		for ( ;; ){
			c = NextJSONchar(ptr, ptrmax);
			if ( c == '\0' ) return;

			if ( (c == '\"') || (c == '{') || (c == '[') ){ // 文字列／オブジェクト／配列
				SkipJSONObject(ptr, ptrmax);
				continue;
			}
			if ( c == ']' ){ // 配列終わり
				(*ptr)++;
				return;
			}
			(*ptr)++;
		}
	}else{ // 値(数値、文字列単語)
		for ( ;; ){
			c = NextJSONchar(ptr, ptrmax);
			if ( c == '\0' ) return;

			if ( (c == '\"') || (c == ']') || (c == '}') || (c == ',') ){
				return;
			}
			(*ptr)++;
		}
	}
}

// 戻り値は ptr が示す文字。ptr 位置の文字は \0 が書き込まれていることがある
TCHAR GetJSONObject(TCHAR **ptr, TCHAR *ptrmax, TCHAR **value)
{
	TCHAR *dest, c;

	c = NextJSONchar(ptr, ptrmax);
	if ( c == '\0' ) return c;
	if ( c == '\"' ){ // 文字列(\0 を許容する)
		*value = dest = ++(*ptr);
		for ( ;; ){
			if ( *ptr >= ptrmax ){
				c = '\0';
				break;
			}
			c = **ptr;
			if ( c == '\"' ){ // 末尾
				(*ptr)++;
				c = **ptr;
				break;
			}
			if ( c == '\\' ){ // ESC
				c = *(*ptr + 1);
				if ( (c == '\"') || (c == '/') || (c == '\\') ){ // '"' '/' '\'
					(*ptr)++;
				}else if ( c == 'u' ){
					int d = 4, n = 0;
					(*ptr) += 2;

					for ( ; ; ){
						TCHAR Ctype;
						UTCHAR uc;

						uc = **ptr;
#ifdef UNICODE
						if ( uc > 'f' ) break;
#endif
						Ctype = T_CHRTYPE[uc];
						if ( !(Ctype & (T_IS_DIG | T_IS_HEX)) ) break;	// 0-9,A-F,a-f ではない
						if ( Ctype & T_IS_LOW ) uc = (UTCHAR)(uc - 0x20);	// 小文字を大文字に
						if ( !(Ctype & T_IS_DIG) ) uc = (UTCHAR)(uc - 7);	// A-F の処理
						n = (n << 4) + (UTCHAR)(uc - '0');
						if ( --d == 0 ) break;
						(*ptr)++;
					}
#ifdef UNICODE
					c = (TCHAR)n;
#else
					{
						WCHAR uchar;

						uchar = (WCHAR)n;
						dest += WideCharToMultiByte(CP_ACP, 0, &uchar, 1, dest, 6 - d, NULL, NULL);
					}
					(*ptr)++;
					continue;
#endif
				}else if ( c == 'b' ){
					(*ptr)++;
					c = '\x8';
				}else if ( c == 'f' ){
					(*ptr)++;
					c = '\xc';
				}else if ( c == 'n' ){
					(*ptr)++;
					c = '\xa';
				}else if ( c == 'r' ){
					(*ptr)++;
					c = '\xd';
				}else if ( c == 't' ){
					(*ptr)++;
					c = '\x9';
				}else{
					(*ptr)++;
					*dest++ = '\\';
				}
			}
			(*ptr)++;
			*dest++ = c;
		}
	}else if ( (c == '{') || (c == '[') ){ // オブジェクト・配列は対応していないのでスキップ
		*value = dest = *ptr;
		SkipJSONObject(ptr, ptrmax);
		dest = *ptr;
		c = **ptr;
	}else{ // 値(数値、文字列単語)
		*value = dest = *ptr;
		for ( ;; ){
			if ( *ptr >= ptrmax ){
				c = '\0';
				break;
			}
			c = **ptr;
			if ( (c == '\"') || (c == ']') || (c == '}') || (c == ',') ){
				break;
			}
			(*ptr)++;
			*dest++ = c;
		}
	}
	*dest = '\0';
	return c;
}

TCHAR *GetLFname(TCHAR **src, TCHAR *dest, int maxlength)
{
	TCHAR *srcptr, *maxdest, *resultptr = NULL, chr;

	srcptr = *src + 1;
	maxdest = dest + maxlength - 1;
	for (;;){
		chr = *srcptr;
		if ( chr == '\0' ) break;
		srcptr++;
		if ( chr == '\"' ) break;
		if ( dest < maxdest ) *dest++ = chr;
	}
	*dest = '\0';
	if ( dest == maxdest ){
		resultptr = *src + 1;
		if ( *(srcptr - 1) == '\"' ) *(srcptr - 1) = '\0';
	}
	*src = srcptr;
	return resultptr;
}

TCHAR *FixListOneLine(FF_LFILE *list)
{
	TCHAR *ptr, *bottom;
	const TCHAR *maxptr = list->maxptr;

	bottom = ptr = list->readptr;
	while ( ptr < maxptr ){
		bottom = ptr = SkipSpaceAndFix(ptr);
		if ( *ptr == '\0' ){		// 空行?
			ptr++;
			continue;
		}
		if ( *ptr == ';'){
			ptr++;
			while( *ptr ){
				if ( (*ptr == '\r') || (*ptr == '\n') ){
					ptr++;
					break;
				}
				ptr++;
			}
			continue;
		}
		while( *ptr ){		// コメントの除去
			if ( (*ptr == '\r') || (*ptr == '\n') ){
				*ptr++ = '\0';
				if ( (*ptr == '\r') || (*ptr == '\n') ) ptr++;
				break;
			}
			ptr++;
		}
		if ( *bottom != '\0' ) break;
	}
	list->readptr = ptr;
	return bottom;
}

BOOL GetListLine(VFSFINDFIRST *VFF, WIN32_FIND_DATA *findfile)
{
	TCHAR *line;
	INTHL ihl;
	FF_LFILE *list = &VFF->v.LFILE;

	if ( list->readptr >= list->maxptr ) goto EOL;

	memset(findfile, 0, (BYTE *)findfile->cFileName - (BYTE *)findfile + sizeof(TCHAR));
	// findfile->cFileName[0] = '\0'; // ↑で初期化済み
	findfile->cAlternateFileName[0] = '\0';
	list->extra.mask = 0;
	list->longname = NULL;
	list->comment = NULL;

	if ( list->json ){
		TCHAR *ptr, *ptrmax, c;
		TCHAR *key, *value;

		ptr = list->readptr;
		ptrmax = list->maxptr;

		// XMessage(NULL, NULL, XM_DbgDIA, T("%x %s"),ptr,ptr);
		for (;;){ // オブジェクト開始を確認
			c = NextJSONchar(&ptr, ptrmax);
			if ( c == '\0' ) goto EOLJ;
			ptr++;
			if ( c == '{' ){
				TCHAR *ptr2;
				ptr2 = ptr;
				if ( NextJSONchar(&ptr2, ptrmax) == '}' ){ // 空オブジェクト
					ptr = ptr2 + 1;
					continue;
				}
				break;
			}
		}
		for (;;){
			TCHAR *keylast;
						// key
			c = NextJSONchar(&ptr, ptrmax);
			if ( c == '\0' ) break;
			if ( c == ',' ){
				ptr++;
				continue;
			}
			if ( c == '}' ){
				ptr++;
				break;
			}
			if ( c == '\"' ){
				ptr++;
				key = ptr;
				for ( ;; ){
					if ( ptr >= ptrmax ) goto EOLJ;
					if ( *ptr == '\"' ){
						keylast = ptr++;
						break;
					}
					ptr++;
				}
			}else{
				key = ptr;
				for ( ;; ){
					if ( ptr >= ptrmax ) goto EOLJ;
					if ( !Isalnum(*ptr) ){
						keylast = ptr;
						break;
					}
					ptr++;
				}
			}
			if ( NextJSONchar(&ptr, ptrmax) != ':' ) break;
			ptr++;
			*keylast = '\0';

			c = GetJSONObject(&ptr, ptrmax, &value);
			if ( value == NULL ) break;
			// XMessage(NULL, NULL, XM_DbgLOG, T("%s : %s , [%c]%s"),key ,value, c, *ptr=='\0' ? ptr+ 1:ptr);

			switch (*key){ // 各種エントリ追加情報 x:xxx
				case 'N': { // N:"MAX_PATH" filename
					TCHAR *p;

					p = tstplimcpy(findfile->cFileName, value, MAX_PATH);
					if ( (p - findfile->cFileName) >= (MAX_PATH - 1) ){
						list->longname = value;
						findfile->cFileName[0] = '>';
					}else{
						list->longname = NULL;
					}
					break;
				}
				case 'Y': // Y:"8.3" dos name
					tstplimcpy(findfile->cAlternateFileName, value, 14);
					break;

				case 'A': // A:x	属性
					findfile->dwFileAttributes |=
						GetNumber((const TCHAR **)&value) &
						~(FILE_ATTRIBUTEX_LF_MARK | B26/*FILE_ATTRIBUTEX_LF_COMMENT、互換用*/ );
					break;

				case 'C': // C:"iso時刻" 作成時刻
					ReadFileTime((const TCHAR **)&value, &findfile->ftCreationTime);
					break;
				case 'L': // L:"iso時刻" 最終アクセス時刻
					ReadFileTime((const TCHAR **)&value, &findfile->ftLastAccessTime);
					break;
				case 'W': // W:"iso時刻" 更新時刻
					ReadFileTime((const TCHAR **)&value, &findfile->ftLastWriteTime);
					break;
				case 'S': // S:"x.x" サイズ
					EX_LITTLE_ENDIAN GetNumberHL((const TCHAR **)&value, &ihl);
					if ( SkipSpace((const TCHAR **)&value) == '.' ){
						value++;
						findfile->nFileSizeHigh = ihl.s.L;
						findfile->nFileSizeLow = (DWORD)GetNumber((const TCHAR **)&value);
					}else{
						EX_LITTLE_ENDIAN
						findfile->nFileSizeHigh = ihl.s.H;
						findfile->nFileSizeLow = ihl.s.L;
					}
					break;

				case 'R': // R:"n.n" dwReserved0/1
					findfile->dwReserved0 = (DWORD)GetNumber((const TCHAR **)&value);
					if ( SkipSpace((const TCHAR **)&value) == '.' ) value++;
					findfile->dwReserved1 = (DWORD)GetNumber((const TCHAR **)&value);
					break;
				case 'M':	// M:0|1|-1 マーク
					if ( *value == '1' ){
						setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_LF_MARK);
						setflag(list->extra.mask, FODE_MARK_ON);
					}else if ( *value == '0' ){
						setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
						setflag(list->extra.mask, FODE_MARK_OFF);
					}else if ( *value == '-' ){
						value++;
						setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
						setflag(list->extra.mask, FODE_MARK_TOGGLE);
					}
					break;
				case 'H':	// H:n ハイライト
					setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
					setflag(list->extra.mask, FODE_HIGHLIGHT);
					list->extra.highlight = (BYTE)GetNumber((const TCHAR **)&value);
					break;
				case 'X':	// X:n 拡張子色
					setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
					setflag(list->extra.mask, FODE_COLOR);
					list->extra.extC = (COLORREF)GetNumber((const TCHAR **)&value);
					break;
				case 'T':	// T:… コメント
					if ( (VFF->cb != NULL) && (VFF->cb->EntryInfo != NULL) ){
						if ( *ptr == '\0' ) ptr++;
						VFF->cb->EntryInfo(VFF->cb, key, value, ptr - value);
					}else{
						setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
						setflag(list->extra.mask, FODE_COMMENT);
						list->comment = value;
					}
					break;
				default:	// 未定義
					if ( (VFF->cb != NULL) && (VFF->cb->EntryInfo != NULL) ){
						if ( *ptr == '\0' ) ptr++;
						VFF->cb->EntryInfo(VFF->cb, key, value, ptr - value);
					}
					break;
			}
			if ( c == '\0' ) break;
			if ( c == '}' ){
				ptr++;
				break;
			}
			if ( *ptr == '\0' ) ptr++;
		}
		list->readptr = ptr;
		return TRUE;

	EOLJ:;
		list->readptr = ptr;
		SetLastError(ERROR_NO_MORE_FILES);
		return FALSE;
	}
	// 独自形式・リスト形式
	line = FixListOneLine(list);

	if ( SkipSpace((const TCHAR **)&line) != '\"' ){ // " なし…ファイル名のみ
		TCHAR *dest, *destmax;
		const TCHAR *lineptr;

		if ( *line == '\0' ) goto EOL;
		if ( GetFileSTAT(line, findfile) == FALSE ){
			TCHAR *last;

			findfile->dwFileAttributes = FILE_ATTRIBUTEX_NODETAIL;
			last = VFSFindLastEntry(line);
			if ( ((*last == '\\') || (*last == '/')) && (*(last + 1) == '\0') ){
				findfile->dwFileAttributes = FILE_ATTRIBUTEX_NODETAIL | FILE_ATTRIBUTE_DIRECTORY;
			}
		}
		dest = findfile->cFileName;
		destmax = dest + MAX_PATH - 1;
		lineptr = line;
		while ( *lineptr != '\0' ){
			*dest++ = *lineptr++;
			if ( dest >= destmax ){
				list->longname = line;
				findfile->cFileName[0] = FINDOPTION_LONGNAME;
				tstplimcpy(findfile->cFileName + 1, line, MAX_PATH);
				break;
			}
		}
		*dest = '\0';
	}else{	// " あり…各種属性有り
		TCHAR c;

		// ファイル名
		list->longname = GetLFname(&line, findfile->cFileName, MAX_PATH);
		if ( list->longname != NULL ) findfile->cFileName[0] = FINDOPTION_LONGNAME;
		if ( SkipSpace((const TCHAR **)&line) != ',' ){
			if ( GetFileSTAT(line, findfile) == FALSE ){
				TCHAR *last;

				findfile->dwFileAttributes = FILE_ATTRIBUTEX_NODETAIL;
				last = VFSFindLastEntry(line);
				if ( ((*last == '\\') || (*last == '/')) && (*(last + 1) == '\0') ){
					findfile->dwFileAttributes = FILE_ATTRIBUTEX_NODETAIL | FILE_ATTRIBUTE_DIRECTORY;
				}
			}
			return TRUE;
		}
		// ,["短いファイル名"]
		line++;
		c = SkipSpace((const TCHAR **)&line);
		if ( c == '\"' ){	// 単独 " … SFN 8.3
			GetLFname(&line, findfile->cAlternateFileName, 13);
			c = SkipSpace((const TCHAR **)&line);
			if ( c != ',' ) return TRUE;
			line++;
		}
		// 各種エントリ追加情報 x:xxx
		while ( *line != '\0' ){
			TCHAR ck, *key;

			c = SkipSpace((const TCHAR **)&line);
			key = line;
			for (;;){
				ck = *(line + 1);
				if ( !Isalnum(ck) ) break;
				line++;
			}
			if ( ck != ':' ) break;
			line += 2;
			switch (c){	// A:x	属性
				case 'A':
					findfile->dwFileAttributes |=
						GetNumber((const TCHAR **)&line) &
						~(FILE_ATTRIBUTEX_LF_MARK | B26/*FILE_ATTRIBUTEX_LF_COMMENT、互換用*/ );
					break;
				case 'C':	// C:x.x 作成時刻
					ReadFileTime((const TCHAR **)&line, &findfile->ftCreationTime);
					break;
				case 'L':	// L:x.x 最終アクセス時刻
					ReadFileTime((const TCHAR **)&line, &findfile->ftLastAccessTime);
					break;
				case 'W':	// W:x.x 更新時刻
					ReadFileTime((const TCHAR **)&line, &findfile->ftLastWriteTime);
					break;
				case 'S':	// S:x.x サイズ
					EX_LITTLE_ENDIAN GetNumberHL((const TCHAR **)&line, &ihl);
					if ( SkipSpace((const TCHAR **)&line) == '.' ){
						line++;
						findfile->nFileSizeHigh = ihl.s.L;
						findfile->nFileSizeLow = (DWORD)GetNumber((const TCHAR **)&line);
					}else{
						EX_LITTLE_ENDIAN
						findfile->nFileSizeHigh = ihl.s.H;
						findfile->nFileSizeLow = ihl.s.L;
					}
					break;
				case 'R':	// R:n.n dwReserved0/1
//					if ( *line == '0' ) line++;
					findfile->dwReserved0 = (DWORD)GetNumber((const TCHAR **)&line);
					if ( SkipSpace((const TCHAR **)&line) == '.' ) line++;
					findfile->dwReserved1 = (DWORD)GetNumber((const TCHAR **)&line);
					break;
				case 'M':	// M:0|1|-1 マーク
					if ( *line == '1' ){
						setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_LF_MARK);
						setflag(list->extra.mask, FODE_MARK_ON);
					}else if ( *line == '0' ){
						setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
						setflag(list->extra.mask, FODE_MARK_OFF);
					}else if ( *line == '-' ){
						line++;
						setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
						setflag(list->extra.mask, FODE_MARK_TOGGLE);
					}
					while ( Isdigit(*line) ) line++;
					break;
				case 'H':	// H:n ハイライト
					setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
					setflag(list->extra.mask, FODE_HIGHLIGHT);
					list->extra.highlight = (BYTE)GetNumber((const TCHAR **)&line);
					break;
				case 'X':	// X:n 拡張子色
					setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
					setflag(list->extra.mask, FODE_COLOR);
					list->extra.extC = (COLORREF)GetNumber((const TCHAR **)&line);
					break;

				case 'T': // T:"text" コメント
					if ( key[1] == ':' ){ // key 1文字
						if ( (VFF->cb != NULL) && (VFF->cb->EntryInfo != NULL) ){
							TCHAR *dest = line;

							GetCommandParameter((const TCHAR **)&line, dest, CMDLINESIZE);
							VFF->cb->EntryInfo(VFF->cb, T("T"), dest, tstrlen(dest) + 1);
						}else{
							UTCHAR c1, c2;

							c1 = *(line + 0);
							c2 = *(line + 1);
							if ( ((c1 == '\"') && (c2 != '\"') && (c2 >= ' '))
								|| ((c1 != ',') && (c1 >= ' ')) ){
								setflag(findfile->dwFileAttributes, FILE_ATTRIBUTEX_EXTRA);
								setflag(list->extra.mask, FODE_COMMENT);
								list->comment = line;
							}
						}
						break;
					}
					// default へ
				default: // 未定義
					if ( (VFF->cb != NULL) && (VFF->cb->EntryInfo != NULL) ){
						TCHAR *dest = line;
						*(line - 1) = '\0'; // key 末尾
						GetCommandParameter((const TCHAR **)&line, dest, CMDLINESIZE);
						VFF->cb->EntryInfo(VFF->cb, key, dest, tstrlen(dest) + 1);
					}
					break;
			}
			if ( SkipSpace((const TCHAR **)&line) != ',' ) break;
			line++;
		}
	}
	return TRUE;
EOL:
	SetLastError(ERROR_NO_MORE_FILES);
	return FALSE;
}

BOOL GetOptionData_ListFile(VFSFINDFIRST *VFF) // 追加情報の確認
{
	TCHAR *ptrmax = VFF->v.LFILE.maxptr - 2;
	TCHAR *ptr = VFF->v.LFILE.readptr, entrystart = (TCHAR)VFF->v.LFILE.json;

	if ( VFF->v.LFILE.LoadOption ) return FALSE; // 確認済み
	VFF->v.LFILE.LoadOption = TRUE;

	if ( VFF->v.LFILE.json ){
		while ( ptr < ptrmax ){
			TCHAR *key, *value, c;
			size_t valuelen;

			if ( *ptr == entrystart ) break;
			if ( *ptr <= ' ' ){
				ptr++;
				continue;
			}
			if ( *ptr != '\"' ){
				ptr++;
				continue;
			}

			// key
			key = ++ptr;
			for ( ;; ){
				if ( ptr >= ptrmax ) goto breakcheck;
				if ( *ptr == '\"' ){
					*ptr++ = '\0';
					break;
				}
				ptr++;
			}
			if ( NextJSONchar(&ptr, ptrmax) != ':' ) break;
			ptr++;

			c = NextJSONchar(&ptr, ptrmax);
			if ( c == '\0' ) break;
			if ( c == '[' ) break;

			GetJSONObject(&ptr, ptrmax, &value);
			if ( value == NULL ) break;
			if ( *ptr == '\0' ) ptr++;
			valuelen = ptr - value;
			//setflag(findfile->dwReserved1, WFD_R1_HEADDATA);

			if ( tstrcmp(key, T("Base")) == 0 ){
				VFF->v.LFILE.base = value;
			}
			if ( (VFF->cb) && (VFF->cb->PathInfo != NULL) ){
				VFF->cb->PathInfo(VFF->cb, key, value, valuelen);
			}
		}
		breakcheck: ;
		VFF->v.LFILE.readptr = ptr;
	}else{
		while ( VFF->v.LFILE.readptr < ptrmax ){
			TCHAR *ptr, *keysep, *value;
			const TCHAR *line;
			size_t valuelen;

			ptr = value = VFF->v.LFILE.readptr;
			if ( *ptr != ';' ) break;
			line = keysep = ++ptr;
			while ( ptr < ptrmax ){
				TCHAR c;

				c = *ptr;
				if ( c == '\0' ) break;
				if ( (c == '=') &&
					 ((VFF->v.LFILE.readptr - keysep) < 10) ) {
					*ptr = '\0';
					value = ptr + 1;
					keysep = NULL;
				}else if ( (c == '\r') || (c == '\n') ){
					*ptr++ = '\0';
					valuelen = ptr - value;
					if ( (*ptr == '\r') || (*ptr == '\n') ) ptr++;
					break;
				}
				ptr++;
			}
			VFF->v.LFILE.readptr = ptr;
			if ( keysep != NULL ) continue;

			if ( memcmp(line, T("Base"), 5 * sizeof(TCHAR)) == 0 ){
				VFF->v.LFILE.base = value;
			}

			if ( (VFF->cb) && (VFF->cb->PathInfo != NULL) ){
				VFF->cb->PathInfo(VFF->cb, line, value, valuelen);
			}
		}
	}
	return TRUE;
}


ERRORCODE InitFindFirstListFile(VFSFINDFIRST *VFF, const TCHAR *Fname, WIN32_FIND_DATA *findfile, BOOL json)
{
	ERRORCODE result;

	result = LoadTextData(Fname, &VFF->v.LFILE.mem, &VFF->v.LFILE.readptr, &VFF->v.LFILE.maxptr, 0);
	VFF->v.LFILE.json = '\0';
	VFF->v.LFILE.LoadOption = FALSE;
	if ( result == NO_ERROR ){
		TCHAR *ptrmax;

		VFF->v.LFILE.type = VFSDT_LFILE_TYPE_PARENT;
		VFF->v.LFILE.base = NilStr;
		VFF->v.LFILE.extra.size = sizeof(FOD_EXTRADATA);
		VFF->mode = VFSDT_LFILE;
		VFF->TypeName = TypeName_ListFile;
		SetDummydir(findfile, T("."));
		findfile->dwReserved0 = 0;
		findfile->dwReserved1 = 0;

		ptrmax = VFF->v.LFILE.maxptr - 2;
		if ( json ){ // JSON 形式の詳細を判定
			TCHAR *ptr = VFF->v.LFILE.readptr, c;

			VFF->v.LFILE.json = '[';
			c = NextJSONchar(&ptr, ptrmax);
			if ( c == '{' ){ // ヘッダ付きオブジェクト形式
				ptr++;
				setflag(findfile->dwReserved1, WFD_R1_SIGNATURE | WFD_R1_JSON | WFD_R1_JSON_OBJ);
			}else if ( c == '[' ){ // 配列形式
				ptr++;
				c = NextJSONchar(&ptr, ptrmax);
				setflag(findfile->dwReserved1, WFD_R1_JSON);
				VFF->v.LFILE.json = '{';
				if ( c == '{' ) goto breakcheck; // [0]がオブジェクト…ヘッダなし配列形式
				// [0]はシグネチャ
				setflag(findfile->dwReserved1, WFD_R1_SIGNATURE);

				SkipJSONObject(&ptr, ptrmax); // [0](シグネチャ)をスキップ
				c = NextJSONchar(&ptr, ptrmax);
				if ( c == ',' ){
					ptr++;
					c = NextJSONchar(&ptr, ptrmax);
				}
				if ( c == '{' ) ptr++; // [1] のオブジェクト先頭をスキップ
			}else{
				setflag(findfile->dwReserved1, WFD_R1_JSON);
			}
			breakcheck: ;
			VFF->v.LFILE.readptr = ptr;
		}
	}
	return result;
}
