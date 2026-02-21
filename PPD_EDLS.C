/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library						拡張エディット
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "PPD_DEF.H"
#include "PPD_EDL.H"
#pragma hdrstop

TCHAR SEPSPACE[] = CAPTIONSEPARATOR; // タイトルバーに表示するメッセージの表示間隔
TCHAR SEPSPACEFMT[] = CAPTIONSEPARATOR T("%s");
#define SEPSPACElen TSIZEOFSTR(CAPTIONSEPARATOR)

PPXDLL void PPXAPI SetMessageOnCaption(HWND hWnd, const TCHAR *message)
{
	TCHAR buf[CMDLINESIZE], *p;

	buf[0] = '\0';
	GetWindowText(hWnd, buf, TSIZEOF(buf));
	p = tstrstr(buf, SEPSPACE);	// 区切りを求める
	if ( message != NULL ){
		message = MessageText(message);
		if ( p == NULL ) p = buf + tstrlen(buf);
		thprintf(p, TSIZEOF(buf), SEPSPACEFMT, message);
	}else{
		if ( p == NULL ) return;
		if ( *(p + SEPSPACElen) == '@' ){
			*(p + SEPSPACElen) = ' ';
		}else{
			*p = '\0';
		}
	}
	SetWindowText(hWnd, buf);
}

void USEFASTCALL SetMessageForEdit(HWND hWnd, const TCHAR *message)
{
	HWND hParentWnd = GetParent(hWnd);

	if ( GetWindowLongPtr(hParentWnd, GWL_STYLE) & (WS_CAPTION & ~WS_BORDER) ){
		SetMessageOnCaption(hParentWnd, message);
	}
}

TCHAR *AllocEditTextBuf(HWND hWnd, int *len)
{
	TCHAR *ptr;

	*len = GetWindowTextLength(hWnd);
	if ( *len > MB ){
		XMessage(hWnd, NULL, XM_GrERRld, MES_EOLT);
		return NULL;
	}
	ptr = HeapAlloc(DLLheap, 0, TSTROFF(*len + 1));
	if ( ptr == NULL ){
		XMessage(hWnd, NULL, XM_GrERRld, MES_ENOM);
	}
	return ptr;
}

#ifndef UNICODE
// Multi(ANSI) 仕様→Wide 仕様 に修正
void USEFASTCALL CaretFixToW(const char *str, DWORD *of)
{
	int i;
	const char *last;

	last = str + *of;
	for ( i = 0 ; str < last ; i++ ){
		if ( *str == '\0' ) break;
		if ( IskanjiA( *str ) ) str++;
		str++;
	}
	*of = i;
}

// Wide 仕様から→ Multi(ANSI) 仕様に修正
void USEFASTCALL CaretFixToA(const char *str, DWORD *of)
{
	int i;
	const char *ptr;

	if ( xpbug >= 0 ) return;

	ptr = str;
	for ( i = *of ; i ; i-- ){
		if ( *ptr == '\0' ) break;
		if ( IskanjiA( *ptr ) ) ptr++;
		ptr++;
	}
	*of = ptr - str;
}
#endif

#ifndef UNICODE
void GetEditSel(HWND hWnd, const TCHAR *buf, ECURSOR *cursor)
{
	SendMessage(hWnd, EM_GETSEL, (WPARAM)&cursor->start, (LPARAM)&cursor->end);
	if ( xpbug < 0 ){
		CaretFixToA(buf, &cursor->start);
		CaretFixToA(buf, &cursor->end);
	}
}

void SetEditSel(HWND hWnd, const TCHAR *buf, DWORD start, DWORD end)
{
	if ( xpbug < 0 ){
		CaretFixToW(buf, &start);
		CaretFixToW(buf, &end);
	}
	SendMessage(hWnd, EM_SETSEL, start, end);
}
#endif

BOOL InitPPeFindReplace(PPxEDSTRUCT *PES)
{
	PES->findrep = (struct PPeFindReplace *)HeapAlloc(DLLheap, 0, sizeof(struct PPeFindReplace));
	if ( PES->findrep == NULL ) return FALSE;

	PES->findrep->findtext[0] = '\0';
	PES->findrep->replacetext[0] = '\0';
	return TRUE;
}

BOOL SearchStr(PPxEDSTRUCT *PES, int mode)
{
	TCHAR *edittext, *maxptr, *ptr, *find = NULL, *findstr;
	int len;
	DWORD slen;
	ECURSOR cursor;
	PXREGEXPS *rexps;

	if ( PES->findrep == NULL ){
		if ( InitPPeFindReplace(PES) == FALSE ) return FALSE;
	}
	#pragma warning(suppress:6001 6011) // InitPPeFindReplace で初期化
	findstr = PES->findrep->findtext;

	if ( (mode == EDITDIST_DIALOG) || (findstr[0] == '\0') ){
		DWORD firstC, lastC;

		SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&firstC, (LPARAM)&lastC);
		if ( firstC != lastC ){
			TEXTSEL ts;

			if ( IsTrue(SelectEditStrings(PES, &ts, TEXTSEL_CHAR)) ){
				tstplimcpy(findstr, ts.word, VFPS);
				FreeTextselStruct(&ts);
			}
		}

		if ( 0 >= tInput(PES->hWnd, (mode >= 0) ? MES_TFNX : MES_TFPV,
				findstr, TSIZEOF(PES->findrep->findtext) - 1,
				PPXH_SEARCH, PPXH_SEARCH) ){
			return FALSE;
		}
	}
	if ( findstr[0] == '\0' ) return FALSE;

	edittext = AllocEditTextBuf(PES->hWnd, &len);
	if ( edittext == NULL ) return FALSE;
	GetWindowText(PES->hWnd, edittext, len + 1);

	if ( PES->flags & PPXEDIT_RICHEDIT ){
		tstrreplace(edittext, T("\r\n"), T("\n"));
		len = tstrlen32(edittext);
	}

	slen = tstrlen32(findstr);

	SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&cursor.start, (LPARAM)&cursor.end);
	cursor.end -= cursor.start;
	CaretFixToA(edittext, &cursor.start);

	if ( X_esrx ){
		size_t hitlen, lasthitpos = 0xffffffff, lasthitlen = 0;
		TCHAR findstr2[VFPS];

		tstrcpy(findstr2, findstr);
		if ( FALSE == InitRegularExpression(&rexps, findstr2, SLASH_SEARCH) ){
			XMessage(PES->hWnd, NULL, XM_GrERRld, T("regexp error"));
			return FALSE;
		}

		hitlen = len;
		ptr = edittext;
		if ( mode >= 0 ){ // 進む
			hitlen -= cursor.start;
			ptr += cursor.start;
		}
		for ( ;; ){
			find = RegularExpressionSearch(rexps, ptr, hitlen, &hitlen);
			if ( find != NULL ){
				if ( mode >= 0 ){ // 進む
					if ( mode == EDITDIST_REPLACE_TEST ){
						if ( find != (edittext + cursor.start) ) find = NULL;
					}else{
						if ( (find == (edittext + cursor.start)) && ((mode != (EDITDIST_NEXT_WITHNOW)) || (cursor.end != 0)) ){
							ptr = find + hitlen;
							hitlen = len - (find + hitlen - edittext);
							continue;
						}
					}
					slen = hitlen;
				}else{
					if ( find < (edittext + cursor.start) ){
						if ( lasthitpos == (find - edittext) ) break;
						lasthitpos = find - edittext;
						lasthitlen = hitlen;

						ptr = find + hitlen;
						hitlen = len - lasthitpos - hitlen;
						continue;
					}
				}
			}
			break;
		}
		if ( mode < 0 ){
			if ( lasthitlen == 0 ){
				find = NULL;
			}else{
				find = edittext + lasthitpos;
				slen = lasthitlen;
			}
		}
	}else{
		maxptr = edittext + len;

		if ( mode >= 0 ){ // 進む
			maxptr -= slen;
			ptr = edittext + cursor.start;
#ifdef UNICODE
			for ( ; ptr <= maxptr ; ptr++ )
#else
			for ( ; ptr <= maxptr ; ptr += Chrlen(*ptr) )
#endif
			{
				if ( tstrnicmp(ptr, findstr, slen) == 0 ){
					if ( ptr == (edittext + cursor.start) ){
						if ( (mode != EDITDIST_NEXT_WITHNOW) || (cursor.end != 0) ){
							continue; // 先頭に一致する文字列があるとき、EDITDIST_NEXT なら無視、EDITDIST_NEXT_WITHNOW なら範囲選択してなければ無視
						}
					}
					find = ptr;
					break;
				}
			}
		}else{ // 戻る
			for ( ptr = edittext + cursor.start - 1 ; ptr >= edittext ; ptr-- ){
				if ( tstrnicmp(ptr, findstr, slen) == 0 ){
					find = ptr;
					break;
				}
			}
		}
	}

	if ( find != NULL ){
		cursor.start = find - edittext;
		SetEditSel(PES->hWnd, edittext, cursor.start, cursor.start + slen);
		SendMessage(PES->hWnd, EM_SCROLLCARET, 0, 0);
		HeapFree(DLLheap, 0, edittext);
		SetMessageForEdit(PES->hWnd, NULL);
		return TRUE;
	}else{
		HeapFree(DLLheap, 0, edittext);
		SetMessageForEdit(PES->hWnd, MES_EENF);
		return FALSE;
	}
}

const TCHAR *SkipPathName(const TCHAR *pathptr)
{
	for (;;){
		TCHAR code;

		code = *pathptr;
		if ( (code == '\0') || (code == '/') || (code == '\\') ) return pathptr;
		#ifndef UNICODE
			if ( IskanjiA(code) ){
				pathptr++;
				if ( *pathptr == '\0' ) return pathptr;
			}
		#endif
		pathptr++;
	}
}

// ファイル検索処理 -----------------------------------------------------------
BOOL SearchFileInedInit(ESTRUCT *ED, TCHAR *str, WIN32_FIND_DATA *ff)
{
	PPXCMDENUMSTRUCT work;
	TCHAR spath[CMDLINESIZE], cdir[CMDLINESIZE], word[MAX_PATH];
	TCHAR *slashp, *sepp;
	DWORD length;

	length = tstrlen32(str);
	if ( length >= VFPS ) return FALSE; // OVER_VFPS_MSG 全体が長すぎ

	while ( (slashp = tstrchr(str, '/')) != NULL ) *slashp = '\\';

	tstrcpy(spath, str);
	sepp = FindLastEntryPoint(spath);
	if ( (length - (sepp - spath)) >= 256 ) return FALSE; // ファイル名長すぎ(これ以上長いと APIで落ちる)

	tstrcpy(word, sepp);
	*sepp = '*';
	*(sepp + 1) = '\0';

	tstrcpy(ED->Fname, str);

	if ( spath[0] == '%' ){
		if ( spath[1] == '\'' ){ // エイリアスを展開する
			PP_ExtractMacro(NULL, NULL, NULL, spath, cdir, XEO_DISPONLY);
		}else{ // 環境変数を展開する
			if ( VFPS <= ExpandEnvironmentStrings(spath, cdir, VFPS) ){
				tstrcpy(cdir, spath);
			}
		}
		tstrcpy(spath, cdir);
	}

	if ( (ED->info != NULL) && (ED->info->Function != NULL) ){
		PPxEnumInfoFunc(ED->info, '1', cdir, &work);
	}else{
		cdir[0] = '\0';
	}
	#pragma warning(suppress: 6054) // PPxEnumInfoFunc で取得済み
	if ( NULL == VFSFullPath(NULL, spath, cdir) ) return FALSE;
	if ( (spath[0] == '\\') && (spath[1] == '\\') ){ // UNC でサーバを検索させないように調整する
		const TCHAR *pathp;

		pathp = SkipPathName(spath + 2); // 「\\server」は検索対象外
		if ( *pathp == '\0' ) return FALSE;
		pathp = SkipPathName(pathp + 1); // 「\\server\share」は検索対象外
		if ( *pathp == '\0' ) return FALSE;
		if ( ED->cmdsearch & CMDSEARCH_NOUNC ){
			size_t len;
			TCHAR code;

		 // CMDSEARCH_NOUNC の場合、検索対象が現在パス以外のときは検索しない
			len = pathp - spath;
			if ( memicmp(spath, cdir, len * sizeof(TCHAR)) != 0 ){
				return FALSE;
			}
			code = cdir[len];
			if ( (code != '\0') && (code != '/') && (code != '\\') ){
				return FALSE;
			}
		}
	}

	ED->hF = FindFirstFileL(spath, ff);
	if ( ED->hF == INVALID_HANDLE_VALUE ){
		ED->hF = NULL;
		return FALSE;
	}

	tstrcpy(ED->Fsrc, spath);
	ED->Fword = ED->Fsrc + tstrlen(ED->Fsrc) + 1;
	tstrcpy(ED->Fword, word);
									// 検索文字列からパスを分離
	ED->FnameP = FindLastEntryPoint(ED->Fname);
	return TRUE;
}

void ClearSearchFileIned(ESTRUCT *ED)
{
	if ( ED->hF != NULL ){
		FindClose(ED->hF);
		ED->hF = NULL;
	}
	if ( ED->romahandle != 0 ){
		if ( ED->romahandle > 1 ){
			if ( ED->cmdsearch & (CMDSEARCH_WILDCARD | CMDSEARCH_REGEXP) ){
				FreeFN_REGEXP((FN_REGEXP *)ED->romahandle);
				HeapFree(DLLheap, 0, (void *)ED->romahandle);
			}else if ( ED->cmdsearch & CMDSEARCH_ROMA ){
				SearchRomaString(NULL, NULL, 0, &ED->romahandle);
			}
		}
		ED->romahandle = 0;
	}
}

TCHAR *SearchFileInedMain(ESTRUCT *ED, TCHAR *str, int flags)
{
	WIN32_FIND_DATA ff;
	int limit = 3;

	for ( ; ; ){
		if ( ED->hF != NULL ){					// 前回に検索を行っている場合…
			if ( (flags & CMDSEARCH_MAKELIST) || (tstrcmp(str, ED->Fname) == 0) ){	// 検索直後なら検索続行
				if ( FindNextFile(ED->hF, &ff) == FALSE ){
												// 最後までやったので最初から
					FindClose(ED->hF);
					ED->hF = NULL;
/*					現在、うまく伝達できないので休止中
					if ( !(flags & CMDSEARCH_ONE) ){
						SetMessageForEdit(ED->info->hWnd, MES_EENF);
					}
*/
					if ( (flags & CMDSEARCH_ONE) &&
							!(ED->cmdsearch & CMDSEARCH_CURRENT) ){
						goto searchfail;
					}

					if ( flags & CMDSEARCH_CURRENT ){	// コマンド検索切替
						ED->cmdsearch ^= CMDSEARCH_CURRENT;
					}else{
						limit--;
					}

					limit--;
					if ( limit <= 0 ) goto searchfail;
					ED->hF = FindFirstFileL(ED->Fsrc, &ff);
					if ( ED->hF == INVALID_HANDLE_VALUE ){
						ED->hF = NULL;
//						SetMessageForEdit(ED->info->hWnd, T("retry error"));
					 	goto searchfail;
					}
				}
			}else{							// 検索直後でないなら新規検索指定
				ClearSearchFileIned(ED);
				if ( flags & CMDSEARCHI_FINDFIRST ) goto searchfail;
			}
		}
		if ( ED->hF == NULL ){					// 新規検索
			ED->cmdsearch = flags & ~CMDSEARCHI_FINDFIRST;
			#pragma warning(suppress: 4701) // ff は out 専用
			if ( SearchFileInedInit(ED, str, &ff) == FALSE ) goto searchfail;
			setflag(flags, CMDSEARCHI_FINDFIRST);
		}
						// ファイル列挙に成功したため、内容一致を調べる -------
		if ( ED->cmdsearch & CMDSEARCH_DIRECTORY ){ // dir 属性？
			#pragma warning(suppress:4701 6001)
			if ( !(ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) continue;
		}
		// 部分/roma一致
		if ( ED->cmdsearch & CMDSEARCH_MATCHFLAGS ){
			if ( ED->cmdsearch & (CMDSEARCH_ROMA | CMDSEARCH_WILDCARD | CMDSEARCH_REGEXP) ){
				if ( ExtraEntrySearchString(ff.cFileName, ED) == FALSE ){
					continue;
				}
			}else{ // CMDSEARCH_FLOAT
				if ( tstristr(ff.cFileName, ED->Fword) == NULL ) continue;
			}
		}else{ // 前方一致
			TCHAR bkchr, *nametail;

			nametail = ff.cFileName + tstrlen(ED->Fword);
			bkchr = *nametail;
			*nametail = '\0';
			if ( tstricmp(ff.cFileName, ED->Fword) != 0 ) continue;
			*nametail = bkchr;
		}
											// 検索成功 -----------------------
		if ( ED->cmdsearch & CMDSEARCH_CURRENT ){
			TCHAR buf[0x400];
			FN_REGEXP fn;
			int result;

			if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) continue;

			buf[0] = '\0';
			ExpandEnvironmentStrings(T("%PATHEXT%"), buf,
					TSIZEOF(buf) - TSIZEOF(EXTPATHEXT) - 1);
			if ( buf[0] != '\0' ) tstrcat(buf, T(";"));
			tstrcat(buf, T(EXTPATHEXT));
			MakeFN_REGEXP(&fn, buf);
			result = FinddataRegularExpression(&ff, &fn);
			FreeFN_REGEXP(&fn);
			if ( result ){
				break;
			}else{
				continue;
			}
		}

		if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
			if ( IsRelativeDirectory(ff.cFileName) ) continue;
			if ( !(ED->cmdsearch & CMDSEARCH_NOADDSEP) ){
				tstrcat(ff.cFileName, T("\\"));
			}
		}
		break;
	};

	tstrcpy(ED->FnameP, ff.cFileName);
	return ED->Fname;

searchfail:
	if ( !(flags & CMDSEARCH_ONE) ) XBeep(XB_NiERR);
	return NULL;
}

PPXDLL TCHAR * PPXAPI SearchFileIned(ESTRUCT *ED, TCHAR *line, ECURSOR *cursor, int flags)
{
	TCHAR *ptr;
	int braket = GWS_BRAKET_NONE;
	DWORD nwP;

	if ( flags & CMDSEARCH_MAKELIST ){
		if ( ED->hF == NULL ){
			if ( !(flags & CMDSEARCH_MULTI) ){
				cursor->start = 0;
				cursor->end   = tstrlen32(line);
			}else{
										// 範囲選択がされていない時の抽出処理 -
				if ( cursor->start == cursor->end ){
					braket = GetWordStrings(line, cursor, GWSF_SPLIT_PARAM);
					if ( braket != GWS_BRAKET_NONE ){
						setflag(ED->cmdsearch, CMDSEARCHI_BRAKET);
					}
				}
				line[cursor->end] = '\0';
			}
		}else if ( line[cursor->start] == '\"' ){
			braket = GWS_BRAKET_LEFT;
		}
		nwP = cursor->start;
	}else{
		if ( !(flags & CMDSEARCH_MULTI) ){
			if ( *line == '\0' ){
				ClearSearchFileIned(ED);
				return NULL;
			}
			cursor->start = 0;
			cursor->end = tstrlen32(line);
		}else{
										// 範囲選択がされていない時の抽出処理 -
			if ( cursor->start == cursor->end ){
				braket = GetWordStrings(line, cursor, GWSF_SPLIT_PARAM);
			}
			line[cursor->end] = '\0';
		}
		nwP = cursor->start;
#ifndef UNICODE
		if ( (flags & CMDSEARCH_EDITBOX ) && (xpbug < 0) ){
			CaretFixToW(line, &cursor->start);
			CaretFixToW(line, &cursor->end);
		}
#endif
		if ( flags & CMDSEARCHI_SAVEWORD ){
			if ( braket ) setflag( flags, CMDSEARCHI_FINDBRAKET );
			if ( ED->FnameP == NULL ) return NULL;
			tstrcpy(ED->FnameP, line + nwP);
		}
	}
	ptr = SearchFileInedMain(ED, line + nwP, flags);

	if ( ptr == NULL ) return NULL;	// 検索失敗
									// 検索成功
	if ( flags & CMDSEARCH_MULTI ){
		if ( flags & CMDSEARCH_MAKELIST ){
			if ( (braket >= GWS_BRAKET_LEFT) && (line[cursor->start] != '\"') ) cursor->start--;
			if ( (ED->cmdsearch & CMDSEARCHI_BRAKET) || (tstrchr(ptr, ' ') != NULL) ){
				TCHAR *nline;

				nline = line;
				*nline++ = '\"';
				nline = tstpcpy(nline, ptr);
				tstrcpy(nline, T("\""));
				ptr = line;
			}
		}else if ( tstrchr(ptr, ' ') != NULL ){	// 空白あり→ブラケット必要
			TCHAR *nline;

			nline = line;
			if ( (braket == GWS_BRAKET_NONE) ) *nline++ = '\"'; // ブラケット無し
			nline = tstpcpy(nline, ptr);
			if ( braket != GWS_BRAKET_LEFTRIGHT ) tstrcpy(nline, T("\"")); // 右ブラケット無し
			ptr = line;
		}
	}
	return ptr;
}

/*-----------------------------------------------------------------------
	テキストスタック(新しいものほど後ろに配置される)

	1st:削除した方法
			0:EOD
			B0	0:BS	1:DEL
			B1	1=選択状態
			B7	1
	2nd:削除した文字列
----------------------------------------------------------------------*/
// テキストスタックに保存 -----------------------------------------------------
PPXDLL void PPXAPI PushTextStack(TCHAR mode, TCHAR *text)
{
	TCHAR *p, *b;
	int size;	// 保存に必要な大きさ

	if ( text == NULL ) return;
	size = tstrlen32(text) + 1 + 1 + 1;	// 文字列 + 削除方法 + EOD
	if ( size <= 3 ) return; // 空
	if ( size > TextStackSize ) return; // 入りきらない

	UsePPx();
	b = NULL;							// 保存する場所を決める
	for ( p = Sm->TextStack ; *p ; p += tstrlen(p) + 1){
		if ( !b && ( (p - Sm->TextStack) >= size) ){
			b = p;
		}
	}
										// 容量チェック
	if ( (TextStackSize - (p - Sm->TextStack)) < size ){
		if ( b != NULL ){
			memmove(Sm->TextStack, b, TSTROFF(p - b));
			p -= b - Sm->TextStack;
		}else{
			p = Sm->TextStack;
		}
	}
	*p = (UTCHAR)(mode | B7);
	tstrcpy(p + 1, text);
	*(p + size - 1) = 0;
	FreePPx();
}

// テキストスタックから取り出す -----------------------------------------------
PPXDLL void PPXAPI PopTextStack(TCHAR *mode, TCHAR *text)
{
	TCHAR *ptr, *b;
										// 末尾を捜す
	UsePPx();
	for ( b = ptr = Sm->TextStack ; *ptr ; ptr += tstrlen(ptr) + 1 ) b = ptr;
	*mode = (*b) & (TCHAR)~B7;
	if (*b){
		tstrcpy(text, b + 1);
		*b = '\0';
	}else{
		*text = '\0';
	}
	FreePPx();
}

void FreeTextselStruct(TEXTSEL *ts)
{
	if ( (ts->text == ts->textbuf) || (ts->text == NULL) ) return;
	HeapFree(DLLheap, 0, ts->text);
}

// 一行用
BOOL USEFASTCALL SelectEditStringsS(PPxEDSTRUCT *PES, TEXTSEL *ts, int mode)
{
	ts->textlength = GetWindowTextLength(PES->hWnd) + 1;
	if ( ts->textlength >= SELBUFSIZE ){
		ts->text = HeapAlloc(DLLheap, 0, ts->textlength * sizeof(TCHAR) );
		if ( ts->text == NULL ) return FALSE;
	}else{
		ts->textlength = SELBUFSIZE;
	}
	GetWindowText(PES->hWnd, ts->text, ts->textlength);
	SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&ts->cursor.start, (LPARAM)&ts->cursor.end);
	ts->cursororg = ts->cursor;
#ifndef UNICODE
	if ( xpbug < 0 ){						// XP bug 回避
		CaretFixToA(ts->text, &ts->cursor.start);
		CaretFixToA(ts->text, &ts->cursor.end);
	}
#endif

	if ( ts->cursor.start < ts->cursor.end ){			// 範囲選択あり
		ts->text[ts->cursor.end] = '\0';
		ts->word = ts->text + ts->cursor.start;
		return TRUE;
	}else{						// 範囲選択なし
		TCHAR *top, *end;

		top = end = ts->text + ts->cursor.start;
		switch ( mode ){
			case TEXTSEL_CHAR:	// Del
				if ( *end == 0xd ){
					end++;
					if ( *end == 0xa ) end++;
				}else{
					#ifdef UNICODE
						end++;
					#else
						end += Chrlen(*end);
					#endif
				}
				break;
			case TEXTSEL_BACK:	// BS
				if ( (ts->text < top) && ( *(top - 1) == 0xa ) ){
					top--;
					if ( (ts->text < top) && ( *(top - 1) == 0xd ) ) top--;
				}else{
					top -= bchrlen(ts->text, top - ts->text);
				}
				break;
			case TEXTSEL_WORD:	// Word
				while( (UTCHAR)*end > ' ' ) end++;
				break;
			case TEXTSEL_BEFORE:	// カーソルより前
				top = ts->text;
				break;
			case TEXTSEL_AFTER:	// カーソルより後ろ
				end += tstrlen(end);
				break;
			case TEXTSEL_BEFOREWORD:	// Word
				if ( (ts->text < top) && ((UTCHAR)*(top - 1) > ' ') ){
					while( (ts->text < top) && ((UTCHAR)*(top - 1) > ' ') ) top--;
				}else{
					while( (ts->text < top) && ((UTCHAR)*(top - 1) <= ' ') ) top--;
				}
				break;
//			case TEXTSEL_ALL:	// 全選択
		}
		*end = 0;

		SetEditSel(PES->hWnd, ts->text, top - ts->text, end - ts->text);
		ts->word = top;
		return TRUE;
	}
}

// マルチライン用
BOOL USEFASTCALL SelectEditStringsM(PPxEDSTRUCT *PES, TEXTSEL *ts, int mode)
{
	size_t line, topline, buflength;
	LRESULT wLP, wBP;
	TCHAR *destptr;
	TCHAR tmpbuf[SELBUFSIZE];
	DWORD pos;
#ifndef UNICODE
	DWORD pos2;
#endif
	ts->textlength = GetWindowTextLength(PES->hWnd) + 1;
	if ( ts->textlength >= SELBUFSIZE ){
		ts->text = HeapAlloc(DLLheap, 0, ts->textlength * sizeof(TCHAR) );
		if ( ts->text == NULL ) return FALSE;
	}else{
		ts->textlength = SELBUFSIZE;
	}

	SendMessage(PES->hWnd, EM_GETSEL,
			(WPARAM)&ts->cursor.start, (WPARAM)&ts->cursor.end);
	ts->cursororg = ts->cursor;
									// 読み込み位置を決定 -----------------
	if ( ts->cursor.start != ts->cursor.end ){			// 範囲選択あり
		line = SendMessage(PES->hWnd, EM_LINEFROMCHAR,
				(WPARAM)ts->cursor.start, 0);
		topline = SendMessage(PES->hWnd, EM_LINEFROMCHAR,
				(WPARAM)ts->cursor.end, 0);
	}else{						// 範囲選択なし
		line = SendMessage(PES->hWnd, EM_LINEFROMCHAR, (WPARAM)-1, 0);
		if ( (mode == TEXTSEL_BACK) && line ) line--;
		topline = line + 1;
	}
									// 行単位で読み込み -----------------------
	destptr = ts->text;
	buflength = ts->textlength;

	wBP = wLP = SendMessage(PES->hWnd, EM_LINEINDEX, (WPARAM)line, 0);
	if ( (LONG_PTR)wLP >= 0 ) do {
		size_t len;

		*(WORD *)tmpbuf = TSIZEOF(tmpbuf) - 2 - 1; // 改行+Nil分追加
		// 行テキストと文字数(xpbug影響なし)を取得
		len = SendMessage(PES->hWnd, EM_GETLINE, (WPARAM)line, (LPARAM)tmpbuf);
		// len=0 (読み込み失敗の可能性あり)でも、続行する必要あり
		if ( buflength < len ){
			FreeTextselStruct(ts);
			return FALSE;	// バッファ不足
		}
		buflength -= len;
		memcpy(destptr, tmpbuf, TSTROFF(len));

		// 次の行へ & 改行があれば改行を追加
		#ifndef UNICODE
		if ( xpbug < 0 ){
			size_t i = 0;
			while ( i < len ){
				wLP++;
				i += IskanjiA( *(destptr + i) ) ? 2 : 1;
			}
			destptr += len;
			len = wLP;
		}else{
			destptr += len;
			len += wLP;
		}
		#else
			destptr += len;
			len += wLP;
		#endif
		// len : 次の行を示す wLP (改行がないので、ずれが起きる)
		line++;
		wLP = SendMessage(PES->hWnd, EM_LINEINDEX, (WPARAM)line, 0);
		if ( (LONG_PTR)wLP < 0 ) break;				// これ以上行がない

		if ( len < (size_t)wLP ){	// 改行が隠れている
			if ( buflength < 3 ){
				FreeTextselStruct(ts);
				return FALSE;	// バッファ不足
			}
			buflength -= 2;
			*destptr++ = '\r';
			*destptr++ = '\n';
		}
	}while( line <= topline );
	*destptr = '\0';

									// 範囲選択 ---------------------------
	// 頭出し
	pos = ts->cursor.start - wBP;
#ifndef UNICODE
	CaretFixToA(ts->text, &pos);
#endif
	destptr = ts->text + pos;
	if ( ts->cursor.start < ts->cursor.end ){	// 範囲選択あり
		pos = ts->cursor.end - ts->cursor.start;
#ifndef UNICODE
		CaretFixToA(destptr, &pos);
#endif
		if ( (destptr + pos) >= (ts->text + ts->textlength) ){
			FreeTextselStruct(ts);
			return FALSE;
		}
		*(destptr + pos) = '\0';
		ts->word = destptr;
	}else{						// 範囲選択なし
		TCHAR *top, *end;

		top = end = destptr;
		switch ( mode ){
			case TEXTSEL_CHAR:	// Del
				if ( *end == 0xd ){
					end++;
					if ( *end == 0xa ) end++;
				}else{
					#ifdef UNICODE
						end++;
					#else
						end += Chrlen(*end);
					#endif
				}
				break;
			case TEXTSEL_BACK:	// BS
				if ( (ts->text < top) && ( *(top - 1) == 0xa ) ){
					top--;
					if ( (ts->text < top) && ( *(top - 1) == 0xd ) ) top--;
				}else{
					top -= bchrlen(ts->text, top - ts->text);
				}
				break;
			case TEXTSEL_WORD:	// Word
				while( (UTCHAR)*end > ' ' ) end++;
				break;
			case TEXTSEL_BEFORE:	// カーソルより前
				top = ts->text;
				break;
			case TEXTSEL_AFTER:	// カーソルより後ろ
				while ( *end ){
					if ( *end == '\r' ) break;
					end++;
				}
				break;
			case TEXTSEL_BEFOREWORD:	// Word
				if ( (ts->text < top) && ((UTCHAR)*(top - 1) > ' ') ){
					while( (ts->text < top) && ((UTCHAR)*(top - 1) > ' ') ) top--;
				}else{
					while( (ts->text < top) && ((UTCHAR)*(top - 1) <= ' ') ) top--;
				}
				break;
//			case TEXTSEL_ALL:	// 全選択
		}
		*end = '\0';

		#ifndef UNICODE
			pos = end - ts->text;
			pos2 = top - ts->text;
			if ( xpbug < 0 ){
				CaretFixToW(ts->text, &pos);
				CaretFixToW(ts->text, &pos2);
			}
			SendMessage(PES->hWnd, EM_SETSEL, pos2 + wBP, pos + wBP);
		#else
			SendMessage(PES->hWnd, EM_SETSEL,
					top - ts->text + wBP, end - ts->text + wBP);
		#endif
		ts->word = top;
	}
	return TRUE;
}

BOOL SelectEditStrings(PPxEDSTRUCT *PES, TEXTSEL *ts, int mode)
{
	BOOL result;

	ts->text = ts->textbuf;
	SendMessage(PES->hWnd, WM_SETREDRAW, FALSE, 0);
	if ( PES->flags & PPXEDIT_TEXTEDIT ){
		result = SelectEditStringsM(PES, ts, mode);	// マルチライン用
	}else{
		result = SelectEditStringsS(PES, ts, mode);	// 一行用
	}
	SendMessage(PES->hWnd, WM_SETREDRAW, TRUE, 0);
	return result;
}

void GetHeight(PPxEDSTRUCT *PES, HFONT hFont)
{
	HDC hDC;
	HGDIOBJ hOldFont C4701CHECK;	//一時保存用
	TEXTMETRIC tm;

	hDC = GetWindowDC(PES->hWnd);
	if ( hFont != NULL ) hOldFont = SelectObject(hDC, hFont);
	GetTextMetrics(hDC, &tm);
	if ( hFont != NULL ) SelectObject(hDC, hOldFont); // C4701ok
	ReleaseDC(PES->hWnd, hDC);

	PES->fontX = tm.tmAveCharWidth ? tm.tmAveCharWidth : 2;
	PES->fontY = tm.tmHeight ? tm.tmHeight : 2;
}

void LineCursor(PPxEDSTRUCT *PES, DWORD mes)
{
	POINT pos;
	DWORD line;
	HDC hDC;

	if ( GetFocus() != PES->hWnd ) return;
	if ( PES->flags & PPXEDIT_RICHEDIT ) return; // 仮

	line = CallWindowProc(PES->hOldED, PES->hWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
	GetCaretPos(&pos);
	if ( (PES->caretY != pos.y) || (line != PES->caretLY) || (mes == WM_PAINT)){
		HBRUSH hBrush;
		RECT box;
		int topbackup;

		CallWindowProc(PES->hOldED, PES->hWnd, EM_GETRECT, 0, (LPARAM)&box);
		if ( PES->exstyle & WS_EX_CLIENTEDGE ){
			box.top++;
			box.left++;
		}

		hDC = GetWindowDC(PES->hWnd);
		hBrush = (HBRUSH)SendMessage(GetParent(PES->hWnd), WM_CTLCOLOREDIT, (WPARAM)hDC, (LPARAM)PES->hWnd);
		#pragma warning(suppress:6001) // EM_GETRECT で読み込み
		topbackup = box.top;
		box.top += PES->caretY + (PES->caretLY - line + 1) * PES->fontY - 1 + PES->fontYext;
		box.bottom = box.top + 1;
		FillBox(hDC, &box, hBrush);

		box.top = topbackup + pos.y + PES->fontY - 1 + PES->fontYext;
		box.bottom = box.top + 1;
		hBrush = CreateSolidBrush(GetTextColor(hDC));
		FillBox(hDC, &box, hBrush);
		DeleteObject(hBrush);

		ReleaseDC(PES->hWnd, hDC);
		PES->caretY = pos.y;
		PES->caretLY = line;
	}
}

//=============================================================================
// システムフック処理
//-----------------------------------------------------------------------------
// システムフックハンドラ
//-----------------------------------------------------------------------------
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if ( nCode == HCBT_CREATEWND ){
		TCHAR buf[MAX_PATH];

		GetClassName((HWND)wParam, buf, TSIZEOF(buf));
		if ( tstricmp(buf, EditClassName) == 0 ){
			PPxRegistExEdit(NULL, (HWND)wParam, 0, NULL, 0, 0, PPXEDIT_WANTEVENT);
		}
	}
	return CallNextHookEx(Sm->hhookCBT, nCode, wParam, lParam);
}

#pragma argsused
BOOL CALLBACK PPxUnHookEditChild(HWND hWnd, LPARAM lParam)
{
	TCHAR buf[MAX_PATH];
	UnUsedParam(lParam);

	if ( GetClassName((HWND)hWnd, buf, TSIZEOF(buf)) ){
		if ( tstricmp(buf, EditClassName) == 0 ){
			SendMessage(hWnd, WM_PPXCOMMAND, KE__FREE, 0);
		}
	}
	return TRUE;
}

BOOL CALLBACK PPxUnHookEditSub(HWND hWnd, LPARAM lParam)
{
	EnumChildWindows(hWnd, PPxUnHookEditChild, lParam);
	return PPxUnHookEditChild(hWnd, lParam);
}
//------------------------------------ 設定処理
PPXDLL BOOL PPXAPI PPxHookEdit(int local)
{
	if ( local < 0 ){				// フック解放
		if ( Sm->hhookCBT ){
			EnumWindows(PPxUnHookEditSub, 0);
			UsePPx();
			UnhookWindowsHookEx(Sm->hhookCBT);
			Sm->hhookCBT = NULL;
			FreePPx();
			PostMessage(HWND_BROADCAST, WM_NULL, 0, 0);
			return TRUE;
		}
	}else if ( Sm->hhookCBT == NULL ){	// フック設定
		UsePPx();
		Sm->hhookCBT = SetWindowsHookEx(WH_CBT, CBTProc, DLLhInst,
				local ? GetCurrentThreadId() : 0);
		FreePPx();
		PostMessage(HWND_BROADCAST, WM_NULL, 0, 0);
		return TRUE;
	}
	return FALSE;
}
