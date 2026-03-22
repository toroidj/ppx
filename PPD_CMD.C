/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library						マクロ実行処理
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#include "CALC.H"
#pragma hdrstop
#define DEFCMDNAME
#include "PPXCMDS.C"

#define ISCMDNULREQUEIERSIZE TSTROFF(2)

#ifndef SC_MONITORPOWER
#define SC_MONITORPOWER 0xF170
#endif

BOOL EditExtractText(EXECSTRUCT *Z);
void EnumEntries(EXECSTRUCT *Z);
void ZInitSentence(EXECSTRUCT *Z);
void Freeoldsrc(EXECSTRUCT *Z);

#define ZMC_BREAK	FALSE
#define ZMC_CONTINUE	TRUE
BOOL ZMacroCharacter(EXECSTRUCT *Z);

#define HistType_GENERAL 0 // g 相当
#define HistType_PATHFIX 5 // f 相当

const TCHAR ExecuteDefaultTitle[] = MES_TEXE;
const TCHAR Title_ValueName[] = T(StringVariable_Command_Title);
const TCHAR ResponseName_ValueName[] = T(StringVariable_Command_Response);

#define GetDestOffset(Z) (((Z)->ExtendDst.top / sizeof(TCHAR)) + ((Z)->dst - (Z)->DstBuf))

#ifndef GW_ENABLEDPOPUP
#define GW_ENABLEDPOPUP 6
#endif

typedef struct {
	HWND hFoundWnd;
	const TCHAR *caption;
} SEARCHWINDOW;

void CmdSkip(EXECSTRUCT *Z) // *skip
{
	for (;;){
		TCHAR chr;
		chr = *Z->src;
		if ( chr == '\0' ) break;
		if ( ((chr == '\n') || (chr == '\r')) && !(Z->flags & XEO_INRETURN) ){
			do {
				Z->src++;
			} while ( (*Z->src == '\n') || (*Z->src == '\r') );
			ZInitSentence(Z);
			return;
		}
		Z->src++;
	}
	Z->command = CID_FILE_EXEC;
}

TCHAR *SearchLabel(const TCHAR *text, const TCHAR *key, size_t keylen)
{
	TCHAR *ptr, chr;

	for (;;){
		ptr = tstrstr(text, key);
		if ( ptr != NULL ){
			chr = ptr[keylen];
			if ( ((UTCHAR)chr <= ' ') || (chr == '%') ) break;
			text = ptr + keylen;
			continue;
		}else{
			break;
		}
	}
	return ptr;
}

BOOL CmdGoto(EXECSTRUCT *Z, const TCHAR *param) // *goto
{
	TCHAR *ptr;
	size_t keylen;

	ZInitSentence(Z);
	if ( GetAsyncKeyState(VK_PAUSE) & KEYSTATE_FULLPUSH ){ // 中断チェック
		if ( PMessageBox(Z->hWnd, MES_AbortCheck, NULL, MB_QYES) == IDOK ){
			BreakAction(Z);
			return FALSE;
		}
	}

	SkipSpace(&Z->src);
	ptr = Z->dst + 2;
	for (;;){
		TCHAR chr;

		chr = *Z->src;
		if ( ((UTCHAR)chr <= ' ') || (chr == '%') ) break;
		*ptr++ = chr;
		Z->src++;
		if ( ptr > (Z->dst + 128) ) break;
	}
	keylen = ptr - Z->dst;
	*ptr = '\0';
	if ( Z->dst[2] == '\0' ){
		Z->dst[0] = '\0';
	}else{
		Z->dst[0] = '%';
		Z->dst[1] = 'm';
	}
	ptr = SearchLabel(Z->src, Z->dst, keylen); // 現在の src 内で前方検索
	if ( ptr != NULL ){
		Z->src = ptr;
		return TRUE;
	}

	if ( Z->oldsrc != NULL ){ // 挿入 src 内で検索
		ptr = SearchLabel((TCHAR *)(&Z->oldsrc[1]), Z->dst, keylen);
		if ( ptr != NULL ){
			Z->src = ptr;
			return TRUE;
		}
	}

	ptr = SearchLabel(param, Z->dst, keylen); // 元コマンドラインで検索
	if ( ptr != NULL ){
		Z->src = ptr;
		Freeoldsrc(Z);
		return TRUE;
	}
	XMessage(NULL, NULL, XM_GrERRld, T("*goto: %s not found."), Z->dst);
	Z->result = ERROR_INVALID_PARAMETER;
	return FALSE;
}

PPXDLL WORD PPXAPI GetHistoryType(const TCHAR **param)
{
	const TCHAR *ptr;
	WORD historytype = 0;

	for ( ;; ){
		TCHAR code;

		code = **param;
		if (((UTCHAR)code < 'a') ||((ptr = tstrchr(HistTypes, code)) == NULL)){
			return historytype;
		}
		historytype |= HistWriteTypeflag[ptr - HistTypes];
		(*param)++;
	}
}

// " を "" に加工する(DstBuf用)
void ZQuotationDoubler_Buf(EXECSTRUCT *Z)
{
	TCHAR *src_q;
	DWORD top, count; // 「"」の数
	int dstlen;

	src_q = tstrchr(Z->dst, '\"');
	if ( src_q == NULL ) return;

	// " による保存領域の増分を算出
	count = 1;
	for (;;){
		src_q = tstrchr(src_q + 1, '\"');
		if ( src_q == NULL ) break;
		count++;
	}
	dstlen = tstrlen(Z->dst);
	if ( Z->dst != Z->DstBuf ){ // Z->DstBuf に納まるか確認
		if ( (Z->dst + dstlen + count) <= (Z->DstBuf + CMDLINESIZE - 2) ){
			tstrreplace(Z->dst, T("\""), T("\"\""));
			return;
		}
	}
	Z->dst += dstlen;

	// ExtendDst を拡張できるか確認
	if ( StoreLongParam(Z, 0) == FALSE ){
		Z->result = RPC_S_STRING_TOO_LONG;
		return;
	}

	top = Z->ExtendDst.top;
	if ( ThSize(&Z->ExtendDst, (count + 1) * sizeof(TCHAR) ) == FALSE ){
		Z->result = RPC_S_STRING_TOO_LONG;
		return;
	}

	// 拡張
	tstrreplace(ThPointerT(&Z->ExtendDst, top) - dstlen, T("\""), T("\"\""));
	Z->ExtendDst.top += count * sizeof(TCHAR);
}

// " を "" に加工する(ExtendDst 用)
void ZQuotationDoubler_Ext(EXECSTRUCT *Z)
{
	TCHAR *src, *src_q;
	DWORD top, count; // 「"」の数
	int dstlen;

	top = Z->ExtendDst.top;
	src = ThPointerT(&Z->ExtendDst, top);
	dstlen = tstrlen(src);

	if ( !Z->func.quotation ){
		Z->ExtendDst.top += dstlen * sizeof(TCHAR);
		return;
	}

	src_q = tstrchr(src, '\"');
	if ( src_q == NULL ){
		Z->ExtendDst.top += dstlen * sizeof(TCHAR);
		return;
	}

	// " による保存領域の増分を算出
	count = 1;
	for (;;){
		src_q = tstrchr(src_q + 1, '\"');
		if ( src_q == NULL ) break;
		count++;
	}

	// ExtendDst を拡張できるか確認
	if ( ThSize(&Z->ExtendDst, (dstlen + count + 1) * sizeof(TCHAR) ) == FALSE ){
		Z->result = RPC_S_STRING_TOO_LONG;
		return;
	}

	// 拡張
	tstrreplace(ThPointerT(&Z->ExtendDst, top), T("\""), T("\"\""));
	Z->ExtendDst.top += (dstlen + count) * sizeof(TCHAR);
}

void SetLongDestMain(EXECSTRUCT *Z, const TCHAR *text, int length)
{
	if ( IsTrue(StoreLongParam(Z, length)) ){
		if ( Z->func.quotation ){
			DWORD oldtop;

			oldtop = Z->ExtendDst.top;
			ThCatString(&Z->ExtendDst, text);
			Z->ExtendDst.top = oldtop;;
			ZQuotationDoubler_Ext(Z);
		}else{
			ThCatString(&Z->ExtendDst, text);
		}
	}else{
		Z->result = RPC_S_STRING_TOO_LONG;
	}
}

void SetLongDest(EXECSTRUCT *Z, const TCHAR *text, int length)
{
	if ( length < 0 ) length = tstrlen(text);

	if ( length < CMDLINESIZE ){
		if ( !Z->func.quotation ){
			Z->dst = tstpcpy(Z->dst, text);
		}else{
			tstrcpy(Z->dst, text);
			ZQuotationDoubler_Buf(Z);
			Z->dst += tstrlen(Z->dst);
		}
	}else{
		SetLongDestMain(Z, text, length);
	}
}

/*-----------------------------------------------------------------------------
	パラメータ入手関数の定義
-----------------------------------------------------------------------------*/
PPXDLL void PPXAPI PPxRegGetIInfo(PPXAPPINFO *ptr)
{
	PPxDefAppInfo = ptr;
	if ( ptr != NULL ){
		PPxDefInfo.Name = ptr->Name;
		PPxDefInfo.RegID = ptr->RegID;
		PPxDefInfo.hWnd = ptr->hWnd;
	}
}
#pragma argsused
DWORD_PTR USECDECL PPxDefInfoFunc(PPXAPPINFO *ppxa, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	if ( PPxDefAppInfo != NULL ){
		DWORD_PTR result = PPxDefAppInfo->Function(PPxDefAppInfo, cmdID, uptr);
		if ( result != PPXA_INVALID_FUNCTION ) return result;
	}

	switch (cmdID){
		case '0': {	// 自分自身へのパス
			TCHAR *p;

			GetModuleFileName(NULL, uptr->enums.buffer, MAX_PATH);
			p = FindLastEntryPoint(uptr->enums.buffer);
			*p = '\0';
			return PPXA_NO_ERROR;
		}

		case '1':
			GetCurrentDirectory(VFPS, uptr->enums.buffer);
			return PPXA_NO_ERROR;
	}

	if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
	return PPXA_INVALID_FUNCTION;
}
//-----------------------------------------------------------------------------
BOOL GetEditMode(const TCHAR **param, TINPUT_EDIT_OPTIONS *options)
{
	const TCHAR *ptr;
	TCHAR chr;
	WORD rhistflags = 0;

	// ダイアログ種類
	options->flags = 0;
	switch ( **param ){ // HistTypes との衝突に注意
		case 'R':
			options->flags = TINPUT_EDIT_OPTIONS_use_refline;
			(*param)++;
			break;

		case 'O':
			options->flags = TINPUT_EDIT_OPTIONS_use_optionbutton;
			(*param)++;
			break;

		case 'E':
			options->flags = TINPUT_EDIT_OPTIONS_use_refext;
			(*param)++;
			break;
	}
	if ( **param == 'S' ){
		setflag(options->flags, TINPUT_EDIT_OPTIONS_single_param);
		(*param)++;
	}

	// 書き込みヒストリ種類
	ptr = tstrchr(HistTypes, **param);
	if ( (ptr == NULL) || (*ptr == '\0') ){
		PPErrorBox(NULL, NULL, ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	// ※ %eg の場合は、0 … 指定無しになる
	options->hist_writetype = (BYTE)(ptr - HistTypes);
	(*param)++;


	// 読み込みヒストリ種類
	chr = SkipSpace(param);
	if ( chr != ',' ){ // 指定無し
		if ( !Isalpha(chr) ){
			rhistflags = HistReadTypeflag[ptr - HistTypes];
		}else{
			PPErrorBox(NULL, NULL, ERROR_INVALID_PARAMETER);
			return FALSE;
		}
	}else{
		for ( ;; ){
			(*param)++;
			ptr = tstrchr(HistTypes, **param);
			if ( (ptr == NULL) || (*ptr == '\0') ) break;
			setflag(rhistflags, HistWriteTypeflag[ptr - HistTypes]);
		}
	}
	options->hist_readflags = rhistflags;
	return TRUE;
}

void SetEditMode(EXECSTRUCT *Z)
{
	resetflag(Z->status, ST_EDITHIST);
	if ( IsTrue(GetEditMode(&Z->src, &Z->edit.options)) ){
		setflag(Z->status, ST_EDITHIST);
	}else{
		Z->result = ERROR_INVALID_PARAMETER;
	}
}

void SetTInputOptionFlags(TINPUT *tinput, TINPUT_EDIT_OPTIONS *options)
{
	if ( options->flags & TINPUT_EDIT_OPTIONS_UI_mask ){
		if ( options->flags & TINPUT_EDIT_OPTIONS_use_optionbutton ){
			setflag(tinput->flag, TIEX_USEOPTBTN);
		}else{ // use_refline / use_refext
			if ( options->flags & TINPUT_EDIT_OPTIONS_use_refext ){
				setflag(tinput->flag, TIEX_REFEXT);
			}
			setflag(tinput->flag, TIEX_USEREFLINE);
		}
	}
	if ( options->flags & TINPUT_EDIT_OPTIONS_single_param ){
		setflag(tinput->flag, TIEX_SINGLEREF);
	}
}

void LineEscape_charcode(EXECSTRUCT *Z, INT_PTR code)
{
	if ( code == 0 ) return;

#ifdef UNICODE
	if ( code < 0x10000 ){
		*Z->dst++ = (WORD)code;
	}else{
		*Z->dst++ = (WORD)(DWORD)((code >> 10) + (0xd800 - (0x10000 >> 10)));
		*Z->dst++ = (WORD)(DWORD)((code & 0x3ff) | 0xdc00);
	}
#else
	*Z->dst++ = (char)(unsigned char)code;
#endif
	if ( *Z->src == ';' ) Z->src++;
}

void LineEscape(EXECSTRUCT *Z) // %b 行末エスケープ・文字挿入
{
	TCHAR c;

	c = *Z->src;
	switch ( c ){
		case '\r':
		case '\n':
			for (;;){
				*Z->dst++ = c;
				c = *(++Z->src);
				if ( (c != '\r') && (c != '\n') ) break;
			}
			break;

		case 'n':
			*Z->dst++ = '\r';
		case 'l':
			*Z->dst++ = '\n';
			Z->src++;
			break;

		case 'r':
			*Z->dst++ = '\r';
			Z->src++;
			break;

		case 't':
			Z->src++;
			*Z->dst++ = '\t';
			break;

		case 'h':
		case 'x':
			Z->src++;
			LineEscape_charcode(Z, GetHexNumber(&Z->src));
			break;
#if 0
		case '\"':
			*Z->dst++ = '\"';
		case '@':
			if ( Z->flags & XEO_INRETURN ) {
				setflag(Z->flags, XEO_INRETURN);
			}else{
				resetflag(Z->flags, XEO_INRETURN);
			}
			Z->src++;
			break;

		case '\'':
			Z->src++;
			for (;;){
				c = *Z->src;
				if ( c == '\0' ) break;
				if ( (c == '%') && (Z->src[1] == 'b') && (Z->src[2] == '\'')){
					Z->src += 3;
					break;
				}
				*Z->dst++ = c;
				Z->src++;

				if ( (Z->dst - Z->DstBuf) >= (CMDLINESIZE - 1) ){ // 長さ制限
					if ( StoreLongParam(Z, 0) == FALSE ){
						Z->result = RPC_S_STRING_TOO_LONG;
						break;
					}
				}
			}
			break;
#endif
		default:
			LineEscape_charcode(Z, GetNumber(&Z->src));
			break;
	}
}

// extract に結果を保存する
void SetLongParamToParam(EXECSTRUCT *Z, LONGEXTRACTPARAM *extract)
{
	if ( (Z->flags & XEO_EXTRACTLONG) &&
		 (extract->longtext.id == extract) ){ // 保存可能
		ThCatString(&Z->ExtendDst, Z->DstBuf);
		extract->longtext.th = Z->ExtendDst;
		Z->ExtendDst.bottom = NULL; // 呼び出し元で解放させる
		Z->result = ERROR_PARTIAL_COPY;
	}else{ // 保存できないので先頭だけ。
		Z->result = RPC_S_STRING_TOO_LONG;
		tstplimcpy(extract->text, (Z->ExtendDst.top != 0) ?
				(TCHAR *)Z->ExtendDst.bottom : Z->DstBuf, CMDLINESIZE);
	}
}

void ZErrorFix(EXECSTRUCT *Z)
{
	if ( Z->extract == NULL ) return;
	if ( Z->result != ERROR_EX_RETURN ){
		*Z->extract = '\0';
		return;
	}
	if ( (Z->ExtendDst.top != 0) ||
		 ((Z->dst - Z->DstBuf) >= (CMDLINESIZE - 1)) ){ // 長さ制限
		SetLongParamToParam(Z, (LONGEXTRACTPARAM *)Z->extract);
	}else{
		tstrcpy(Z->extract, Z->DstBuf);
	}
}

BOOL USEFASTCALL ZExecAndCheckError(EXECSTRUCT *Z) // %&
{
	setflag(Z->flags, XEO_SEQUENTIAL);
	if ( Z->flags & XEO_DISPONLY ){		// 表示のみ
		*Z->dst++ = ':';
		return ZMC_CONTINUE;
	}				// 実行
	ZExec(Z);
	if ( Z->result != NO_ERROR ){
		ZErrorFix(Z); // *return xxx %& は対応していない
		return ZMC_BREAK;
	}
	if ( Z->ExitCode != 0 ){
		Z->result = ERROR_CANCELLED;
		return ZMC_BREAK;
	}
	ZInitSentence(Z);
	return ZMC_CONTINUE;
}

void USEFASTCALL ZStringOnDir(EXECSTRUCT *Z) // %S
{
	if ( !(Z->status & ST_CHKSDIRREF) ){ // 属性判定をしていないならここで。
		if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_ENUMATTR, Z->dst, &Z->EnumInfo) &&
			(*(DWORD *)Z->dst & FILE_ATTRIBUTE_DIRECTORY) ){
			setflag(Z->status, ST_SDIRREF | ST_CHKSDIRREF);
		}else{
			setflag(Z->status, ST_CHKSDIRREF);
		}
	}

	if ( SkipSpace(&Z->src) == '\"' ) Z->src++;
	while ( *Z->src && (*Z->src != '\"') ){
		if ( Z->status & ST_SDIRREF ){ // サブディレクトリがあるときだけ複写
			*Z->dst++ = *Z->src++;
		}else{
			Z->src++;
		}
	}
	if ( *Z->src == '\"' ) Z->src++;
}

void USEFASTCALL ZAddSeparator(EXECSTRUCT *Z){	// "%\"
	TCHAR *buftop;
	UTCHAR c;

	setflag(Z->status, ST_PATHITEM);
	buftop = Z->DstBuf;
#ifndef UNICODE
	if ( (Z->command >= CID_USERNAME) || (Z->func.off != 0) ){
		buftop = Z->dst;
		while ( buftop > Z->DstBuf ){
			if ( *(buftop - 1) == '\0' ) break;
			buftop--;
		}
	}
#endif
	if ( Z->dst > buftop ){
		c = (UTCHAR)*(Z->dst - 1);
#ifndef UNICODE
		if ( (c  > ' ') && (c != '\"') ){
			CatPath(NULL, buftop, NilStr);
			Z->dst += tstrlen(Z->dst);
		}
		return;
#endif
	}else if ( Z->ExtendDst.top > 0 ){
		c = (UTCHAR)*(TCHAR *)(ThStrLastT(&Z->ExtendDst) - 1);
#ifndef UNICODE
		if ( (c  > ' ') && (c != '\"') ){
			if ( ThSize(&Z->ExtendDst, 1 * sizeof(TCHAR) ) == FALSE ){
				Z->result = RPC_S_STRING_TOO_LONG;
				return;
			}
			CatPath(NULL, (TCHAR *)Z->ExtendDst.bottom , NilStr);
			if ( (UTCHAR)*(TCHAR *)(ThStrLastT(&Z->ExtendDst) - 1) != '\0' ){
				Z->ExtendDst.top += sizeof(TCHAR);
			}
		}
		return;
#endif
	}else{
		return;
	}
#ifdef UNICODE
	if ( (c  > ' ') && (c != '\"') && (c != '\\') ){
		*Z->dst++ = '\\';
	}
#endif
}

void USEFASTCALL SetTitleMacro(EXECSTRUCT *Z)
{
	TCHAR *maxp, *titlep;

	titlep = ThAllocString(&Z->StringVariable, Title_ValueName, VFPS);
	maxp = titlep + VFPS - 1;
	for (;;){
		UTCHAR c;

		c = *Z->src;
		if ( c < ' ' ) break;
		Z->src++;
		if ( c == '\"' ) break;
		if ( titlep < maxp ) *titlep++ = c;
	}
	*titlep = '\0';
}

TCHAR * USEFASTCALL GetZCurDir(EXECSTRUCT *Z)
{
	if ( Z->curdir[0] == '\0' ){
		if ( !PPxEnumInfoFunc(Z->Info, '1', Z->curdir, &Z->EnumInfo) ){
			GetCurrentDirectory(VFPS, Z->curdir);
		}
	}
	return Z->curdir;
}

DWORD GetFmacroOption(const TCHAR **string)
{
	DWORD flags = 0;
	const TCHAR *p;
	TCHAR c;

	p = *string;
	for ( ;; p++ ){
		c = *p;
		if ( (c < 'B') || (c > 'X') ) break;
		switch ( c ){
//			case 'A': setflag(flags, FMOPT_EX_FILEATTR);	continue;
			case 'B': setflag(flags, FMOPT_BLANKET);	continue;
			case 'C': setflag(flags, FMOPT_FILENAME);	continue;
			case 'D': setflag(flags, FMOPT_DIR);		continue;
			case 'H': setflag(flags, FMOPT_DRIVE);		continue;
//			case 'I': setflag(flags, FMOPT_IDL);		continue;
			case 'K': setflag(flags, FMOPT_SEP_BACKSLASH); continue;
			case 'L': setflag(flags, FMOPT_SEP_SLASH);	continue;
			case 'M': setflag(flags, FMOPT_MARK);		continue;
			case 'N': setflag(flags, FMOPT_NOBLANKET);	continue;
			case 'P': setflag(flags, FMOPT_LASTSEPARATOR); continue;
			case 'R': setflag(flags, FMOPT_REALPATH);	continue;
			case 'S': setflag(flags, FMOPT_USESFN);		continue;
			case 'T': setflag(flags, FMOPT_FILEEXT);	continue;
			case 'U': setflag(flags, FMOPT_UNIQUE);		continue;
			case 'V': setflag(flags, FMOPT_ENABLEVFS);	continue;
//			case 'W': setflag(flags, FMOPT_EX_WRITEDATE);	continue;
			case 'X': setflag(flags, FMOPT_FILENAME | FMOPT_FILENOEXT); continue;
//			case 'Z': setflag(flags, FMOPT_EX_FILESIZE);	continue;
		}
		break;
	}
	*string = p;
	return flags;
}

void GetFmacroString(DWORD flag, TCHAR *src, TCHAR *dest)
{
	DWORD attr;

	if ( flag & FMOPT_USESFN ) GetShortPathName(src, src, VFPS);
	if ( flag & (FMOPT_FILEEXT | FMOPT_FILENOEXT) ){
		attr = GetFileAttributesL(src); // ※１ dir により拡張子処理をするかどうか
	}
	if ( (flag & (FMOPT_FILENAME | FMOPT_DIR)) !=
			(FMOPT_FILENAME | FMOPT_DIR) ){ // フルパスでないときの処理

		if ( flag & (FMOPT_DRIVE | FMOPT_FILEEXT) ){

			if ( flag & FMOPT_FILEEXT ){	// 拡張子
				TCHAR *p;

				p = VFSFindLastEntry(src);
				#pragma warning(suppress: 4701) // ここは、"flag & FMOPT_FILEEXT"のため、必ず※１済み
				if ( (attr == BADATTR) || !(attr & FILE_ATTRIBUTE_DIRECTORY) ||
						GetCustDword(T("XC_sdir"), 0) ){
					if ( *p != '\0' ) p++;
					p = tstrrchr(p, '.');
					if ( p == NULL ){
						*src = '\0';
					}else{
						tstrcpy(src, p + 1);
					}
				}else{
					*src = '\0';
				}
			}else{							// ドライブ名
				TCHAR *p, *q, *r;
				int mode;

				p = VFSGetDriveType(src, &mode, NULL);
				if ( p != NULL ){
					if ( mode == VFSPT_DRIVE ){
						*p = '\0';
					}else if ( mode == VFSPT_UNC ){
						q = FindPathSeparator(p);
						if ( q != NULL ){
							r = FindPathSeparator(q + 1);
							if ( r != NULL ) *r = '\0';
						}
					}
				}
			}
		}else{ // ファイル名 or ディレクトリ
			TCHAR *xp, xpc;

			xp = VFSFindLastEntry(src);
			if ( xp != src ){
				if ( flag & FMOPT_DIR ){ // ディレクトリ
					*xp = '\0';
				}else{ // ファイル名
					xpc = *xp;
					tstrcpy(src, (xpc == '\\') || (xpc == '/') ? xp + 1 : xp);
				}
			}
		}
	}
	if ( flag & FMOPT_FILENOEXT ){
		#pragma warning(suppress: 6001) // 頭でGetFileAttributesを使用済み
		if ( (attr == BADATTR) || !(attr & FILE_ATTRIBUTE_DIRECTORY) ||
				GetCustDword(T("XC_sdir"), 0) ){
			TCHAR *lastentry;

			lastentry = VFSFindLastEntry(src);
			*(lastentry + FindExtSeparator(lastentry)) = '\0';
		}
	}
	if ( flag & FMOPT_LASTSEPARATOR ) CatPath(NULL, src, NilStr);

	if ( !(flag & FMOPT_NOBLANKET) &&
		 ((flag & FMOPT_BLANKET) || tstrchr(src, ' ') || tstrchr(src, ',')) ){
		thprintf(dest, CMDLINESIZE, T("\"%s\""), src);
	}else{
		tstrcpy(dest, src);
	}
}

void GetPopupPoint(EXECSTRUCT *Z, POINT *pos)
{
	if ( Z->posptr == NULL ){
		if ( PPxInfoFunc(Z->Info, PPXCMDID_POPUPPOS, pos) ) return;
		if ( (Z->hWnd != NULL) && IsWindowVisible(Z->hWnd) ){
			pos->x = 0;
			pos->y = 0;
			ClientToScreen(Z->hWnd, pos);
		}else{
			GetCursorPos(pos);
		}
	}else{
		*pos = *Z->posptr;
	}
}

BOOL infoExtCheck(EXECSTRUCT *Z)
{
	TCHAR buf[64];

	if ( (PPxEnumInfoFunc(Z->Info, PPXCMDID_ENUMATTR, buf, &Z->EnumInfo) == 0) ||
		!(*(DWORD *)buf & FILE_ATTRIBUTE_DIRECTORY) ||
		GetCustDword(T("XC_sdir"), 0) ){
		return TRUE; // 拡張子あり扱い
	}
	return FALSE;
}

void DefaultCmd(EXECSTRUCT *Z, DWORD cmdID, TCHAR *dest)
{
	TCHAR buf[VFPS];

	switch( cmdID ){
		case '1':
			GetCurrentDirectory(VFPS, dest);
			break;

		case '0': {	// 自分自身へのパス
			TCHAR *p;

			GetModuleFileName(NULL, dest, MAX_PATH);
			p = FindLastEntryPoint(dest);
			*p = '\0';
			break;
		}

//		case 'C':
		case 'R':
			PPxEnumInfoFunc(Z->Info, 'C', dest, &Z->EnumInfo);
			break;

		case 'X':
			// 'Y' へ
		case 'Y': {
			TCHAR *p;

			PPxEnumInfoFunc(Z->Info, 'C', dest, &Z->EnumInfo);
			if ( infoExtCheck(Z) ){
				p = VFSFindLastEntry(dest);
				if ( *p != '\0' ) p++;
				p = tstrrchr(p, '.');
				if ( p != NULL ) *p = '\0';
			}
			break;
		}

		case 'T':
			// 'T' へ
		case 't': {
			TCHAR *p;

			if ( !infoExtCheck(Z) ){
				dest[0] = '\0';
				break;
			}

			PPxEnumInfoFunc(Z->Info, 'C', buf, &Z->EnumInfo);
			p = VFSFindLastEntry(buf);
			if ( *p != '\0' ) p++;
			p = tstrrchr(p, '.');
			if ( p == NULL ){
				dest[0] = '\0';
			}else{
				tstrcpy(dest, p + 1);
			}
			break;
		}
	}
	return;
}

// %F
void Get_F_MacroData(PPXAPPINFO *info, PPXCMD_F *fbuf, PPXCMDENUMSTRUCT *work)
{
	DWORD flags;
	const TCHAR *p;
	TCHAR buf[VFPS];

	p = fbuf->source;
	if ( PPxEnumInfoFunc(info, 'F', (TCHAR *)fbuf, work) ) return;

	flags = GetFmacroOption(&p);
	fbuf->source = p;
	if ( !PPxEnumInfoFunc(info, '1', fbuf->dest, work) ){
		GetCurrentDirectory(VFPS, fbuf->dest);
	}
	PPxEnumInfoFunc(info, 'C', buf, work);
	if ( buf[0] == '\0' ){
		if ( flags & (FMOPT_FILENAME | FMOPT_USESFN | FMOPT_FILEEXT) ){
			fbuf->dest[0] = '\0';
			return;
		}
		buf[0] = '?';
		buf[1] = '\0';
	}
	VFSFullPath(NULL, buf, fbuf->dest);
	GetFmacroString(flags, buf, fbuf->dest);
}

// %M[$][&][@][:][<header>][[?[?]]name][,[!]shortcut]
// // $ :マクロ文字展開無し (CmdPack で使用)
// & : 特殊環境変数 %s"Menu_Index" に内容を保存 (CmdPack で使用)
// @ : pjump用追加項目 (PPC_PathJump で使用)
// ? : 動的メニューを下層    ?? : 動的メニューを挿入
// : : %M:M_pjump 等、マクロ文字展開無し
// ! : メニュー表示無しで選択
BOOL USEFASTCALL MenuCmd(EXECSTRUCT *Z)
{
	TCHAR cid[MAX_PATH], *ptr;
	TCHAR param[CMDLINESIZE];
	TCHAR def[VFPS], buf[MAX_PATH], c;
	int flags = 0;

	if ( Z->flags & XEO_DISPONLY ){
		*Z->dst++ = '%';
		*Z->dst++ = *(Z->src - 1);
		return FALSE;
	}
	/*
	if ( *Z->src == '$' ){
		Z->src++;
		flags = MENUFLAG_NOEXTACT;
	}
	*/
	if ( *Z->src == '&' ){
		Z->src++;
		setflag(flags, MENUFLAG_SETINDEX);
	}else if ( flags == 0 ){
		Z->src--;
	}
	ptr = cid;
	while( (UTCHAR)(c = *Z->src) >= (UTCHAR)'0' ){ // " や ' や , を含めない
		*ptr++ = c;
		Z->src++;
	}
	*ptr = '\0';
	def[0] = '\0';
	if ( *Z->src == ',' ){
		Z->src++;
		if ( *Z->src == '!' ){
			Z->src++;
			setflag(flags, MENUFLAG_SELECT);
		}
		GetLineParamS(&Z->src, def, TSIZEOF(def));
	}
	if ( cid[1] != 'E' ){				// メニュー ...........................
		if ( MenuCommand(Z, cid, def, flags) == FALSE ) return TRUE;
		if ( Z->result != NO_ERROR ) return TRUE;
	}else{								// 拡張子判別 .........................
		TCHAR *TypeName = NULL;

		if ( def[0] == '\0' ){ // shortcut 指定無し
			PPxEnumInfoFunc(Z->Info, 'C', buf, &Z->EnumInfo);
			if ( buf[0] == '\0' ){
				Z->result = ERROR_NO_MORE_ITEMS;
				return TRUE;
			}
			CatPath(def, GetZCurDir(Z), buf);

			if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_ENUMATTR, buf, &Z->EnumInfo) && (*(DWORD *)buf & FILE_ATTRIBUTE_DIRECTORY) ){
				TypeName = (TCHAR *)FILE_ATTRIBUTE_DIRECTORY;
			}
		}

		if ( (*(cid + 2) != ':') ?
				(PP_GetExtCommand(def, cid + 1, param, TypeName) == PPEXTRESULT_FILE)
			  : (NO_ERROR == GetCustTable(cid + 3, def, param, TSTROFF(CMDLINESIZE)))
		){
			const TCHAR *newsrc;

			newsrc = param;
			if ( (UTCHAR)*newsrc == EXTCMD_CMD ){
				newsrc++;
			}else{
				if ( (UTCHAR)*newsrc == EXTCMD_KEY ) newsrc++;
				while( *((WORD *)newsrc) ){
					ERRORCODE result;

					result = (ERRORCODE)PPxInfoFunc(Z->Info, PPXCMDID_PPXCOMMAD, (void *)newsrc);
					if ( result > 1 ){ // NO_ERROR, ERROR_INVALID_FUNCTION 以外はエラー
						if ( (result == ERROR_CANCELLED) || !(Z->flags & XEO_IGNOREERR) ){
							break;
						}
					}
					newsrc += sizeof(WORD) / sizeof(TCHAR);
				}
			}
			InsertSrc(Z, newsrc);
		}else{
			Z->result = ERROR_NO_MORE_ITEMS;
		}
		return TRUE;
	}
	return FALSE;
}

#define ERROR_EXTRACT_LONG RPC_S_STRING_TOO_LONG
#define OFFSET_EXTRACT_LONG 16
PPXDLL LRESULT PPXAPI AnswerExtractCall(PPXAPPINFO *info, WPARAM wParam, LPARAM lParam)
{
	TCHAR *mapptr, *cmdptr, extractbuf[CMDLINESIZE];
	ERRORCODE result;
	DWORD dstlen = CMDLINESIZE;
	ERRORCODE recvresult = NO_ERROR;

	mapptr = MapViewOfFile((HANDLE)lParam, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if ( mapptr == NULL ) return NO_ERROR;
	if ( HIWORD(wParam) == (sizeof(DWORD) / sizeof(TCHAR)) ){
		dstlen = *(DWORD *)mapptr;
		cmdptr = mapptr + sizeof(DWORD) / sizeof(TCHAR);
	}else{
		cmdptr = mapptr;
	}
	PP_InitLongParam(extractbuf);
	result = PP_ExtractMacro(info->hWnd, info, NULL, cmdptr, extractbuf, XEO_EXTRACTEXEC | XEO_EXTRACTLONG);

	if ( result == ERROR_PARTIAL_COPY ){
		TCHAR *longresult = PP_GetLongParamRAW(extractbuf);
		size_t resultlen = tstrlen(longresult);

		if ( resultlen >= dstlen ){
			HANDLE hFile;

			tstrcpy(mapptr, T("<length over>"));
			MakeTempEntry(MAX_PATH, mapptr + OFFSET_EXTRACT_LONG, FILE_ATTRIBUTE_NORMAL);
			hFile = CreateFileL(mapptr + OFFSET_EXTRACT_LONG, GENERIC_WRITE,
					FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS,
					FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			if ( hFile != INVALID_HANDLE_VALUE ){
				DWORD wsize;

				if ( IsTrue(WriteFile(hFile, longresult, resultlen * sizeof(TCHAR), &wsize, NULL)) ){
					recvresult = ERROR_EXTRACT_LONG;
				}
				CloseHandle(hFile);
			}
		}else{
			tstrcpy(mapptr, PP_GetLongParamRAW(extractbuf));
		}
		PP_FreeLongParamRAW(extractbuf);
	}else{
		tstrcpy(mapptr, extractbuf);
	}

	UnmapViewOfFile(mapptr);
	CloseHandle((HANDLE)lParam);
	return recvresult;
}

TCHAR *RecvLongExtract(TCHAR *mapptr)
{
	HANDLE hFile;
	TCHAR *buf = NULL;

	hFile = CreateFileL(mapptr + OFFSET_EXTRACT_LONG, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile != INVALID_HANDLE_VALUE ){
		DWORD size;

		size = GetFileSize(hFile, NULL);
		buf = HeapAlloc(ProcHeap, 0, size + sizeof(TCHAR));
		if ( buf != NULL ){
			if ( ReadFile(hFile, buf, size, &size, NULL) == FALSE ) size = 0;
			buf[size / sizeof(TCHAR)] = '\0';
		}
		CloseHandle(hFile);
	}
	DeleteFileL(mapptr + OFFSET_EXTRACT_LONG);
	return buf;
}

PPXDLL const TCHAR * PPXAPI SendExtractCall(HWND hTargetWnd, const TCHAR *src)
{
	DWORD pid;
	HANDLE hMap, hSendMap, hProcess;
	TCHAR *mapptr, *recvptr;
	DWORD srclen, dstlen, mapsize, offlen;
	LRESULT sendresult;
	TCHAR *buf;

	srclen = tstrlen32(src) + 1;
	dstlen = CMDLINESIZE;
	offlen = 0;
	srclen += offlen;
	mapsize = max(srclen, dstlen) * sizeof(TCHAR);
	if ( mapsize < TSTROFF(CMDLINESIZE) ) mapsize = TSTROFF(CMDLINESIZE);

	hMap = CreateFileMapping(INVALID_HANDLE_VALUE,
			NULL, PAGE_READWRITE, 0, mapsize, NULL);
	if ( hMap == NULL ){
		PPErrorBox(NULL, NULL, PPERROR_GETLASTERROR);
		return NULL;
	}
	mapptr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, mapsize);
	if ( mapptr == NULL ){
		PPErrorBox(NULL, NULL, PPERROR_GETLASTERROR);
		CloseHandle(hMap);
		return NULL;
	}
	GetWindowThreadProcessId(hTargetWnd, &pid);
	hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid);
	DuplicateHandle(GetCurrentProcess(), hMap,
			hProcess, &hSendMap, 0, FALSE, DUPLICATE_SAME_ACCESS);
	tstrcpy(mapptr + offlen, src);
	sendresult = SendMessage(hTargetWnd, WM_PPXCOMMAND, TMAKEWPARAM(K_EXTRACT, offlen), (LPARAM)hSendMap);
	CloseHandle(hSendMap);
	CloseHandle(hProcess);

	recvptr = NULL;
	if ( sendresult == ERROR_EXTRACT_LONG ){
		buf = RecvLongExtract(mapptr);
		if ( buf != NULL ) recvptr = buf;
	}else{
		buf = HeapAlloc(ProcHeap, 0, CMDLINESIZE * sizeof(TCHAR));
		if ( buf != NULL ){
			tstrcpy(buf, mapptr);
			recvptr = buf;
		}
	}
	UnmapViewOfFile(mapptr);
	CloseHandle(hMap);
	return recvptr;
}

void ExtractPPxCall(HWND hTargetWnd, EXECSTRUCT *Z, const TCHAR *macroparam)
{
	DWORD pid;
	HANDLE hMap, hSendMap, hProcess;
	TCHAR *mapptr, *recvptr;
	DWORD srclen, dstlen, mapsize, offlen;
	LRESULT sendresult;

	srclen = tstrlen32(macroparam) + 1;
	dstlen = 4 * KB / sizeof(TCHAR);
	offlen = sizeof(DWORD) / sizeof(TCHAR);

	srclen += offlen;
	mapsize = max(srclen, dstlen) * sizeof(TCHAR);
//	if ( mapsize < TSTROFF(CMDLINESIZE) ) mapsize = TSTROFF(CMDLINESIZE);

	hMap = CreateFileMapping(INVALID_HANDLE_VALUE,
			NULL, PAGE_READWRITE, 0, mapsize, NULL);
	if ( hMap == NULL ){
		PPErrorBox(Z->hWnd, NULL, PPERROR_GETLASTERROR);
		return;
	}
	mapptr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, mapsize);
	if ( mapptr == NULL ){
		CloseHandle(hMap);
		PPErrorBox(Z->hWnd, NULL, PPERROR_GETLASTERROR);
		return;
	}
	GetWindowThreadProcessId(hTargetWnd, &pid);
	hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid);
	DuplicateHandle(GetCurrentProcess(), hMap,
			hProcess, &hSendMap, 0, FALSE, DUPLICATE_SAME_ACCESS);
	*(DWORD *)mapptr = dstlen;

	tstrcpy(mapptr + offlen, macroparam);
	sendresult = SendMessage(hTargetWnd, WM_PPXCOMMAND, TMAKEWPARAM(K_EXTRACT, offlen), (LPARAM)hSendMap);
	CloseHandle(hSendMap);
	CloseHandle(hProcess);

	recvptr = mapptr;
	if ( sendresult == ERROR_EXTRACT_LONG ){
		TCHAR *buf = RecvLongExtract(mapptr);

		if ( buf != NULL ) recvptr = buf;
	}
	SetLongDest(Z, recvptr, -1);
	if ( recvptr != mapptr ) HeapFree(ProcHeap, 0, recvptr);
	UnmapViewOfFile(mapptr);
	CloseHandle(hMap);
	return;
}

void GetValue(EXECSTRUCT *Z, DWORD cmdID, TCHAR *dest)
{
	if ( PPxEnumInfoFunc(Z->Info, cmdID, dest, &Z->EnumInfo) ) return;
	DefaultCmd(Z, cmdID, dest);
}

void ZGetName(EXECSTRUCT *Z, TCHAR *dest, TCHAR cmd)
{
	TCHAR buf[VFPS + sizeof(TCHAR) * 2];

	if ( cmd == '\0' ) cmd = *(Z->src - 1);
	switch ( cmd ){
		case 'F': { // %F
			((PPXCMD_F *)buf)->source = Z->src;
			((PPXCMD_F *)buf)->dest[0] = '\0';

			Get_F_MacroData(Z->Info, (PPXCMD_F *)buf, &Z->EnumInfo);

			if ( Z->src < ((PPXCMD_F *)buf)->source ){
				while( Z->src < ((PPXCMD_F *)buf)->source ){
					if ( (*Z->src == 'C') || (*Z->src == 'X') ){
						setflag(Z->flags, XEO_EXECMARK);
					}
					Z->src++;
				}
			}else{
				while( Isalpha(*Z->src) ){
					if ( (*Z->src == 'C') || (*Z->src == 'X') ){
						setflag(Z->flags, XEO_EXECMARK);
					}
					Z->src++;
				}
			}
			tstrcpy(dest, ((PPXCMD_F *)buf)->dest);
			break;
		}
		case 'C': // %C
		case 'X': // %X
			setflag(Z->status, ST_PATHITEM);
		// %T へ
		case 'T': // %T
								// カレントのファイルの属性を入手
			GetValue(Z, cmd, dest);
			if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_ENUMATTR, buf, &Z->EnumInfo) &&
					(*(DWORD *)buf & FILE_ATTRIBUTE_DIRECTORY) ){
				if ( (Z->flags & XEO_DIRWILD) && ( cmd == 'C' ) ){
					CatPath(NULL, dest, T("*.*"));
				}
				setflag(Z->status, ST_SDIRREF | ST_CHKSDIRREF);
			}else{
				setflag(Z->status, ST_CHKSDIRREF);
			}
			setflag(Z->flags, XEO_EXECMARK);
			break;

		default: // その他は未定義扱い
			*dest = '\0';
	}
}

void USEFASTCALL convertslash(EXECSTRUCT *Z, TCHAR *path)
{
	if ( !(Z->flags & (XEO_PATHSLASH | XEO_PATHESCAPE)) ) return;

	while ( *path ){
		if ( Ismulti(*path) ){
			path++;
			if ( *path == '\0' ) break;
		}else{
			switch ( *path ){
				case '\\':
					*path = '/';
					break;

				case '[':
				case ']':
					if ( !(Z->flags & XEO_PATHESCAPE) ) break;
					memmove(path + 1, path, TSTRSIZE(path));
					*path++ = '\\';
					break;
			}
		}
		path++;
	}
}

BOOL CreateResponseFile(EXECSTRUCT *Z)
{
	TCHAR atr[16], *p, buf[VFPS + 4]; // +4 は、「""」 + α分
	const TCHAR *oldsrc = NULL; // 書式指定
	HANDLE hFile;
	DWORD size;
	BOOL addall = TRUE;
	BOOL writebom = FALSE;
	#ifdef UNICODE
	char bufA[VFPS * 3 + 4];
	#endif
	UINT codepage = CP_ACP;

	if ( Z->flags & XEO_DISPONLY ){		// 表示のみ
		tstrcpy(Z->dst, T("ResFile"));
		Z->dst += 7;
		return TRUE;
	}

	if ( *Z->src == '*' ){
		Z->src++;
		addall = FALSE;
	}
	if ( *Z->src == '8' ){
		Z->src++;
		codepage = CP_UTF8;
	}
	if ( *Z->src == 'U' ){
		Z->src++;
		codepage = CP_PPX_UCF2;
	}
	if ( *Z->src == 'B' ){
		Z->src++;
		writebom = TRUE;
	}

	if ( ThGetString(&Z->StringVariable, ResponseName_ValueName, Z->dst, VFPS) != NULL ){ // 実行(作成済み)
		Z->dst += tstrlen(Z->dst);
		while ( Isalpha(*Z->src) ) Z->src++; // 書式設定をスキップ
		return TRUE;
	}

	#ifdef UNICODE
	if ( (Z->command == 'u') && (codepage == CP_ACP) ){ // %u の場合は、文字コード補正が必要
		UN_DLL *uD;
		int i;

		p = Z->DstBuf;
		GetCommandParameter((const TCHAR **)&p, buf, TSIZEOF(buf));

		uD = undll_list;
		for ( i = 0 ; i < undll_items ; i++, uD++ ){
			if ( tstricmp(uD->DllName, buf) ) continue;

			if ( uD->hadd == NULL ){
				if ( LoadUnDLL(uD) == FALSE ) break;
			}

			if ( uD->UnarcW != NULL ){
				if ( uD->flags & UNDLLFLAG_RESPONSE_UTF8 ){
					codepage = CP_UTF8;
				}else{
					codepage = CP_PPX_UCF2;
				}
			}else if ( uD->SetUnicodeMode(TRUE) ){
				uD->SetUnicodeMode(FALSE);
				codepage = CP_UTF8;
			}
			if ( codepage != CP_ACP ){
				p = tstrchr(Z->DstBuf, ',');
				if ( p != NULL ){
					p++;
					memmove(p + 2, p, TSTRSIZE(p));
					Z->dst += 2;
					p[0] = '!';
					p[1] = (TCHAR)((codepage == CP_PPX_UCF2) ? '2' : '8');
				}
			}
			break;
		}
	}
	#endif

	if ( Isalpha(*Z->src) ) oldsrc = Z->src + 1;

										// 実行(新規作成)
	MakeTempEntry(MAX_PATH, Z->dst, FILE_ATTRIBUTE_NORMAL);
	hFile = CreateFileL(Z->dst, GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS,
			FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ){
		Z->result = GetLastError();
		return FALSE;
	}
	PPxInfoFunc(Z->Info, PPXCMDID_GETTMPFILENAME, Z->dst);
	ThSetString(&Z->StringVariable, ResponseName_ValueName, Z->dst);
	if ( tstrchr(Z->dst, ' ') == NULL ){
		Z->dst += tstrlen(Z->dst);
	}else{
		Z->dst = thprintf(Z->dst, CMDLINESIZE, T("\"%s\""), ThGetString(&Z->StringVariable, ResponseName_ValueName, NULL, 0));
	}
	buf[0] = '\"';

	setflag(Z->status, ST_CHKSDIRREF);

	if ( writebom ){
		if ( codepage == CP_PPX_UCF2 ){
			WriteFile(hFile, UCF2HEADER, UCF2HEADERSIZE, &size, NULL);
		}else{
			WriteFile(hFile, UTF8HEADER, UTF8HEADERSIZE, &size, NULL);
		}
	}

	for ( ; ; ){
		if ( oldsrc != NULL ){ // 書式指定有り
			Z->src = oldsrc;
			ZGetName(Z, buf, '\0');
			// 蛇足コードだった
//			if ( !buf[0] || ((buf[0] == '\"') && (buf[1] == '\"')) ){ // 空欄
//				break;
//			}
			convertslash(Z, buf);
			p = buf;
		}else{ // 書式指定無し…%C
			GetValue(Z, 'C', buf + 1);
			if ( buf[1] == '\0' ) break;
			convertslash(Z, buf + 1);
							// 空白あり なら「"」で括る
			if ( tstrchr(buf + 1, ' ') ){
				p = buf;
				tstrcat(p, T("\""));
			}else{
				p = buf + 1;
			}
		}
		if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_ENUMATTR, atr, &Z->EnumInfo) &&
					(*(DWORD *)atr & FILE_ATTRIBUTE_DIRECTORY) ){
			if ( addall ){
				if ( *p == '\"' ){
					*(p + tstrlen(p) - 1) = '\0';
					CatPath(NULL, p, T("*\""));
				}else{
					CatPath(NULL, p, WildCard_All);
				}
			}
			setflag(Z->status, ST_SDIRREF);
		}
		#ifdef UNICODE
			if ( codepage != CP_PPX_UCF2 ){
				if ( 0 >= WideCharToMultiByteU8(codepage, 0, p, -1, bufA, sizeof(bufA), NULL, NULL) ){
					strcpy(bufA, "<long>");
				}
				if ( WriteFile(hFile, bufA, strlen32(bufA), &size, NULL) == FALSE ) break;
				if ( WriteFile(hFile, "\r\n", 2, &size, NULL) == FALSE ) break;
			}else{
				if ( WriteFile(hFile, p, TSTRLENGTH32(p), &size, NULL) == FALSE ) break;
				if ( WriteFile(hFile, L"\r\n", 4, &size, NULL) == FALSE ) break;
			}
		#else
			if ( codepage == CP_UTF8 ){
				WCHAR bufW[VFPS];

				AnsiToUnicode(p, bufW, TSIZEOF(bufW));
				if ( 0 >= WideCharToMultiByteU8(codepage, 0, bufW, -1, buf + 1, sizeof(buf) - 2, NULL, NULL) ){
					strcpy(buf + 1, "<long>");
				}
				p = buf + 1;
			}
			if ( WriteFile(hFile, p, strlen(p), &size, NULL) == FALSE ) break;
			if ( WriteFile(hFile, "\r\n", 2, &size, NULL) == FALSE ) break;
		#endif
		if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_NEXTENUM, buf + 1, &Z->EnumInfo)== 0){
			setflag(Z->status, ST_LOOPEND);
			break;
		}
	}
	CloseHandle(hFile);
	return TRUE;
}


const TCHAR *ZGetTitleName(EXECSTRUCT *Z)
{
	const TCHAR *title;

	title = ThGetString(&Z->StringVariable, Title_ValueName, NULL, 0);
	if ( title == NULL ) title = MessageText(ExecuteDefaultTitle);
	return title;
}


BOOL ZTinput(EXECSTRUCT *Z, TINPUT *tinput)
{
	int inputtype;
	WORD rhisttype;

	if ( !(tinput->flag & TIEX_Z_HIST_SETTINGED) ){
		if ( Z->status & ST_EDITHIST ){ // ※ %eg の場合は 0…指定無しになる
			inputtype = Z->edit.options.hist_writetype;
			rhisttype = Z->edit.options.hist_readflags;

			SetTInputOptionFlags(tinput, &Z->edit.options);
		}else if ( Z->status & ST_PATHITEM ){
			inputtype = HistType_PATHFIX;
			rhisttype = PPXH_NAME_R;
		}else{
			inputtype = HistType_GENERAL;
			rhisttype = PPXH_GENERAL_R;
		}
		tinput->hRtype = rhisttype;
		tinput->hWtype = HistWriteTypeflag[inputtype];
		setflag(tinput->flag, TinputTypeflags[inputtype]);
	}
	tinput->hOwnerWnd = Z->hWnd;
	tinput->info = Z->Info;

	if ( tInputEx(tinput) <= 0 ){
		Z->result = ERROR_CANCELLED;
		return FALSE;
	}
	return TRUE;
}

HWND GetPPcPairWindow(PPXAPPINFO *ppxa)
{
	HWND hWnd;
	int i;

	if ( ppxa != NULL ){
		hWnd = (HWND)PPxInfoFunc(ppxa, PPXCMDID_PAIRWINDOW, &i);
		if ( hWnd != NULL ) return hWnd;
	}

	// 見つからない…アクティブ PPc を探す
	UsePPx();
	i = Sm->ppc.LastFocusID;
	if ( CheckPPcID(i) == FALSE ){
		for ( i = 0 ; i < X_Mtask ; i++ ){
			if ( IsTrue(CheckPPcID(i)) ) break;
		}
	}
	if ( i < X_Mtask ) hWnd = Sm->P[i].hWnd;
	FreePPx(); // 一旦、専有を解除する
	if ( i >= X_Mtask ) return NULL;
	return (HWND)SendMessage(hWnd, WM_PPXCOMMAND, KC_GETSITEHWND, (LPARAM)KC_GETSITEHWND_PAIR);
}

int GetPPxRegID(const TCHAR **src)
{
	int IDindex; // 0～X_Mtask 未満なら有効な値
	TCHAR *bufp, buf[REGEXTIDSIZE];
	int offset;

	// A_BCDEF 形式を用意する
	bufp = buf;
	offset = 1; // ※Isalpha(**src)==FALSEのときは値不一致だが、無視できる
	if ( Isalpha(**src) ){ // 1字目
		*bufp++ = TinyCharUpper(*((*src)++));
		// 2字目('X_X'のとき)
		if ( **src == '_' ){
			*bufp++ = '_';
			(*src)++;
			offset++;
		}
		for ( ; offset < (REGEXTIDSIZE - 1) ; offset++ ){ // 残りのID部分を抽出
			if ( !Isalpha(**src) ) break;
			*bufp++ = TinyCharUpper(*((*src)++));
		}
	}
	*bufp = '\0';

	if ( Isalpha(**src) ){ // 余分な文字
		while ( Isalpha(**src) ) (*src)++;
		return -1;
	}

	if ( buf[0] == '\0' ) return -1;
	UsePPx();
	if ( (buf[0] == 'C') && (buf[1] == '\0') ){ // 1文字指定 PPc
/*
		HWND hFocusWnd = Sm->ppc.hLastFocusWnd;

		if ( (hFocusWnd != NULL) && IsWindow(hFocusWnd) ){
			FreePPx();
			return hFocusWnd;
		}
*/
		IDindex = Sm->ppc.LastFocusID;
		if ( CheckPPcID(IDindex) == FALSE ){
			for ( IDindex = 0 ; IDindex < X_Mtask ; IDindex++ ){
				if ( IsTrue(CheckPPcID(IDindex)) ) break;
			}
		}
	}else if ( buf[1] == '\0' ){ // 各種1文字指定
		for ( IDindex = 0 ; IDindex < X_Mtask ; IDindex++ ){ // 使用できるPPxを検索
			if ( (Sm->P[IDindex].ID[0] == buf[0]) && (Sm->P[IDindex].ID[1] == '_') ){
				break;
			}
		}
	}else{
		IDindex = SearchPPx(buf);
		if ( IDindex < 0 ){
			buf[3] = '\0';
			buf[2] = buf[1];
			buf[1] = '_';
			IDindex = SearchPPx(buf);
		}
	}

	if ( (IDindex < 0) || (IDindex >= X_Mtask) ){
		IDindex = -1;
	}
	FreePPx();
	return IDindex;
}

// BADHWND ... 該当ウィンドウがない
// NULL ... ID指定がされていない
PPXDLL HWND PPXAPI GetPPxhWndFromID(PPXAPPINFO *ppxa, const TCHAR **src, TCHAR *path)
{
	int IDindex; // 0～X_Mtask 未満なら有効な値
	TCHAR *bufp, buf[REGEXTIDSIZE], code;
	int combosite = -2, offset;
	HWND hComboWnd;

	code = **src;
	if ( code == '~' ){ // 反対窓
		HWND hPairWnd;

		(*src)++;
		hPairWnd = GetPPcPairWindow(ppxa);
		if ( hPairWnd == NULL ) return BADHWND;
		return hPairWnd;
	}

	if ( !Isalpha(code) ){
		if ( code == '-' ){ // 特殊
			(*src)++;
		}else{
			return NULL; // 指定なし
		}
	}

	// A_BCDEF 形式を用意する
	bufp = buf;
	offset = 1; // ※Isalpha(**src)==FALSEのときは値不一致だが、無視できる
	if ( Isalpha(**src) ){ // 1字目
		*bufp++ = TinyCharUpper(*((*src)++));
		// 2字目('X_X'のとき)
		if ( **src == '_' ){
			*bufp++ = '_';
			(*src)++;
			offset++;
		}
		for ( ; offset < (REGEXTIDSIZE - 1) ; offset++ ){ // 残りのID部分を抽出
			if ( !Isalpha(**src) ) break;
			*bufp++ = TinyCharUpper(*((*src)++));
		}
	}
	*bufp = '\0';

	if ( code == '-' ){ // 特殊
		if ( ppxa != NULL ){
			HWND hWnd = (HWND)PPxInfoFunc(ppxa, PPXCMDID_GETREQHWND, buf);
			if ( hWnd != NULL ) return hWnd;
		}
		if ( buf[0] != 'C' ) return BADHWND;
		// -C を C 扱いで参照する
	}

	if ( offset >= REGIDSIZE ){ // C_Zxyy 形式？
		HWND hWnd;

		if ( buf[1] == '_' ){
			buf[1] = buf[0];
			bufp = buf + 1;
		}else{
			bufp = buf;
		}
		hWnd = Sm->ppc.hComboWnd[bufp[2] - 'A'];
		if ( (hWnd == hProcessComboWnd) && (ppxa != NULL) ){
			HWND hGetWnd = (HWND)PPxInfoFunc(ppxa, PPXCMDID_COMBOGETHWNDEX, bufp);
			if ( hGetWnd != NULL ) return hGetWnd;
		}
		{
			LPARAM lParam;

			if ( (hWnd == NULL) || (hWnd == BADHWND) ) return BADHWND;
			// Cza[aaa]の[]部分をエンコード.字数が足りなくても \0 があるので ok
			lParam = (DWORD)(BYTE)bufp[3] |
					((DWORD)(BYTE)bufp[4] << 8) |
					((DWORD)(BYTE)bufp[5] << 16) |
					((DWORD)(BYTE)bufp[6] << 24);
			hWnd = (HWND)SendMessage(hWnd, WM_PPXCOMMAND, KCW_GetIDWnd, lParam);
		}
		if ( hWnd == NULL ) return BADHWND;
		return hWnd;
	}

	if ( **src == '#' ){ // 一体化指定
		TCHAR c;

		if ( buf[0] != 'C' ) return BADHWND;

		if ( (buf[1] == 'B') && Isalpha(buf[2]) ){ // 任意のcombo 'CBX'
			hComboWnd = Sm->ppc.hComboWnd[buf[2] - 'A'];
		}else{ // default
			hComboWnd = hProcessComboWnd;
			if ( hComboWnd == NULL ) hComboWnd = Sm->ppc.hComboWnd[0];
		}
		if ( (hComboWnd == NULL) || (hComboWnd == BADHWND) ) return BADHWND;

		(*src)++;
		c = upper(**src);
		if ( c == 'C' ){ // 現在窓
			(*src)++;
			combosite = KC_GETSITEHWND_CURRENT;
		}else if ( c == 'L' ){ // 左側
			(*src)++;
			combosite = KC_GETSITEHWND_LEFT;
		}else if ( c == 'R' ){ // 右側
			(*src)++;
			combosite = KC_GETSITEHWND_RIGHT;
		}else if ( c == '~' ){ // 反対窓
			(*src)++;
			combosite = KC_GETSITEHWND_PAIR;
		}else if ( Isdigit(c) ){ // 左からのペイン位置
			combosite = KC_GETSITEHWND_LEFTENUM + (int)GetDigitNumber(src);
		}else{
			combosite = KC_GETSITEHWND_BASEWND;
		}
	}

	if ( Isalpha(**src) ){ // 余分な文字
		while ( Isalpha(**src) ) (*src)++;
		return BADHWND;
	}

	if ( combosite != -2 ){ // 一体化窓関連取得
		HWND hWnd;

		hWnd = (HWND)SendMessage(hComboWnd, WM_PPXCOMMAND, KC_GETSITEHWND, (LPARAM)combosite);
		if ( hWnd == NULL ) return BADHWND;
		if ( path == NULL ) return hWnd;

		UsePPx();
		if ( hWnd == NULL ){
			IDindex = -1;
		}else{
			for ( IDindex = 0 ; IDindex < X_Mtask ; IDindex++ ){
				if ( Sm->P[IDindex].hWnd == hWnd ) break;
			}
		}
	}else{ // 通常窓取得
		if ( buf[0] == '\0' ) return NULL;
		UsePPx();
		if ( (buf[0] == 'C') && (buf[1] == '\0') ){ // 1文字指定 PPc
			HWND hFocusWnd = Sm->ppc.hLastFocusWnd;

			if ( (hFocusWnd != NULL) && IsWindow(hFocusWnd) ){
				FreePPx();
				return hFocusWnd;
			}

			IDindex = Sm->ppc.LastFocusID;
			if ( CheckPPcID(IDindex) == FALSE ){
				for ( IDindex = 0 ; IDindex < X_Mtask ; IDindex++ ){
					if ( IsTrue(CheckPPcID(IDindex)) ) break;
				}
			}
		}else if ( buf[1] == '\0' ){ // 各種1文字指定
			for ( IDindex = 0 ; IDindex < X_Mtask ; IDindex++ ){ // 使用できるPPxを検索
				if ( (Sm->P[IDindex].ID[0] == buf[0]) && (Sm->P[IDindex].ID[1] == '_') ){
					break;
				}
			}
		}else{
			IDindex = SearchPPx(buf);
			if ( IDindex < 0 ){
				buf[3] = '\0';
				buf[2] = buf[1];
				buf[1] = '_';
				IDindex = SearchPPx(buf);
			}
		}
	}

	if ( (IDindex >= 0) && (IDindex < X_Mtask) ){
		HWND hWnd = Sm->P[IDindex].hWnd;

		if ( path != NULL ) tstrcpy(path, Sm->P[IDindex].path);
		FreePPx();
		return hWnd;
	}
	FreePPx();
	return BADHWND;
}

PPXDLL void PPXAPI ForceSetForegroundWindow(HWND hWnd)
{
	DWORD inactiveTID, activeTID;
	HWND hTargetWnd, hParentWnd;

	// 非アクティブ時でもフォーカス変更可能な状態にする
	inactiveTID	= GetCurrentThreadId();
	activeTID	= GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	AttachThreadInput(inactiveTID, activeTID, TRUE);
	if ( WinType >= WINTYPE_2000 ){ // ダイアログがあれば、そちらを対象に
		if ( IsWindowEnabled(hWnd) == FALSE ){ // 子ウィンドウがないと、別のウィンドウを示すので、対策
			hWnd = GetWindow(hWnd, GW_ENABLEDPOPUP);
		}
	}
	// 親が最小化・非表示になっている場合は元に戻す
	hTargetWnd = hWnd;
	while ( (hParentWnd = GetParent(hTargetWnd)) != NULL ){
		hTargetWnd = hParentWnd;
	}
	if ( IsIconic(hTargetWnd) || !IsWindowVisible(hTargetWnd) ){
		DWORD_PTR sendresult;

		SendMessageTimeout(hTargetWnd, WM_SYSCOMMAND, SC_RESTORE, 0xffff0000,
				SMTO_ABORTIFHUNG, 300, &sendresult);
	}
	// アクティブ化
	SetForegroundWindow(hWnd);
	AttachThreadInput(inactiveTID, activeTID, FALSE);
}

void USEFASTCALL ZGetPPvCursorPos(EXECSTRUCT *Z)
{
	DWORD cmdid = *(Z->src - 1);

	switch ( *Z->src ){
		case 'H': // %lH %LH
			cmdid += 0x100;
			// 'V'へ
		case 'V': // %lV %LV
			Z->src++;
			break;
		//default:
	}
	PPxEnumInfoFunc(Z->Info, cmdid, Z->dst, &Z->EnumInfo);
	Z->dst += tstrlen(Z->dst);
}

void USEFASTCALL ZGetWndCaptionMacro(EXECSTRUCT *Z)
{
	HWND nhWnd;
	int len;

	nhWnd = GetCaptionWindow(Z->hWnd);
	*Z->dst = '\0';
	len = (int)SendMessage(nhWnd, WM_GETTEXT, VFPS, (LPARAM)Z->dst); // 別プロセスを参照する可能性あり
	Z->dst[len] = '\0';
	if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
	Z->dst += tstrlen(Z->dst);
}

// *string / %s 特殊環境変数
BOOL ZStringVariable(EXECSTRUCT *Z, const TCHAR **src, int mode)
{
	TCHAR *name, namebuf[CMDLINESIZE], *str;
	ThSTRUCT *thStringValue = &ProcessStringValue;

	for (;;){
		switch ( *(*src)++ ){
			case 'g': // 展開処理
				mode = StringVariable_extract;
				continue;

			case 'p': // プロセス内
				break;

			case 'i': {// ID内
				ThSTRUCT *thWndSV = (ThSTRUCT *)PPxInfoFunc(Z->Info, PPXCMDID_GETWNDVARIABLESTRUCT, NULL);
				if ( thWndSV != NULL ) thStringValue = thWndSV;
				break;
			}
/*
			case 'r': {// 親コマンドライン内
				ThSTRUCT *thParentSV = (ThSTRUCT *)PPxInfoFunc(Z->Info, PPXCMDID_GETEXTRACTVARIABLESTRUCT, NULL);
				if ( thParentSV != NULL ) thStringValue = thParentSV;
				break;
			}
*/
			case 'e': {// Edit/Treeコントロール内
				ThSTRUCT *thEditSV = (ThSTRUCT *)PPxInfoFunc(Z->Info, PPXCMDID_GETCONTROLVARIABLESTRUCT, NULL);
				if ( thEditSV != NULL ) thStringValue = thEditSV;
				break;
			}

//			case 'c': // セル内
//				break;

			case 'o': // コマンドライン内(コマンドラインローカル)
				thStringValue = &Z->StringVariable;
				break;

			case 'v': {// コマンドライン内(コマンドライングローバル)
				DWORD_PTR result;

				result = Z->Info->Function(Z->Info, PPXCMDID_GETEXTRACTVARIABLESTRUCT, NULL);
				thStringValue = (result != PPXA_INVALID_FUNCTION) ?
						(ThSTRUCT *)result : &Z->StringVariable;
				break;
			}

			case 'u': // _User 内
				thStringValue = NULL;
				break;

			default: { // デフォルト
				ThSTRUCT *thSV = (ThSTRUCT *)PPxInfoFunc(Z->Info, PPXCMDID_GETCONTROLVARIABLESTRUCT, NULL);
				if ( thSV != NULL ){
					thStringValue = thSV;
				}else{
					thSV = (ThSTRUCT *)PPxInfoFunc(Z->Info, PPXCMDID_GETWNDVARIABLESTRUCT, NULL);
					if ( thSV != NULL ) thStringValue = thSV;
				}
				(*src)--;
				break;
			}

		}
		break;
	}
	if ( SkipSpace(src) == ',' ) (*src)++;
	if ( SkipSpace(src) == '\0' ){
		HMENU hPopupMenu;
		int index = 0;

		hPopupMenu = CreatePopupMenu();
		for (;;){
			if ( thStringValue != NULL ){
				if ( FALSE == ThEnumString(thStringValue, index++, Z->dst, namebuf, TSIZEOF(namebuf)) ){
					break;
				}
			}else{
				if ( EnumCustTable(index++, T("_User"), Z->dst, namebuf, TSIZEOF(namebuf)) <= 0 ){
					break;
				}
			}
			namebuf[1000] = '\0';
			thprintf(Z->dst + tstrlen(Z->dst), CMDLINESIZE, T("=%s"), namebuf);
			AppendMenuString(hPopupMenu, index + 1, Z->dst);
		}
		*Z->dst = '\0';
		index = TTrackPopupMenu(Z, hPopupMenu, NULL);
		if ( index ){
			GetMenuString(hPopupMenu, index, Z->dst, CMDLINESIZE, MF_BYCOMMAND);
			Z->dst += tstrlen(Z->dst);
		}else{
			Z->result = ERROR_CANCELLED;
		}
		DestroyMenu(hPopupMenu);
		return index ? ZMC_CONTINUE : ZMC_BREAK;
	}
	if ( mode == StringVariable_command ){
		name = (TCHAR *)*src; // const TCHAR * を TCHAR * に強制変換
	}else{
		TCHAR sep;

		name = namebuf;
		sep = **src;
		if ( (sep != '\"') && (sep != '\'') ){
			GetCommandParameter(src, name, TSIZEOF(namebuf));
		}else{
			TCHAR *dest = name;

			(*src)++;
			while ( (UTCHAR)**src >= ' ' ){
				if ( **src == sep ){
					(*src)++;
					break;
				}
				*dest++ = *(*src)++;
			}
			*dest = '\0';
		}
	}
	str = tstrchr(name, '=');
	if ( str == NULL ){ // read
		const TCHAR *getptr;

		*Z->dst = '\0';
		Z->dst[CMDLINESIZE - 2] = '\0';
		if ( thStringValue != NULL ){
			getptr = ThGetString(thStringValue, name, Z->dst, CMDLINESIZE);
		}else{
			GetCustTable(T("_User"), name, Z->dst, TSTROFF(CMDLINESIZE));
		}
		if ( Z->dst[CMDLINESIZE - 2] == '\0' ){ // short ver.
			if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
		}else{ // long ver.
			if ( thStringValue != NULL ){
				SetLongDestMain(Z, getptr, 0);
			}else{
				if ( IsTrue(StoreLongParam(Z, 0)) ){
					int newsize;

					newsize = GetCustTableSize(T("_User"), name);
					if ( ThSize(&Z->ExtendDst, newsize) == FALSE ){
						Z->result = RPC_S_STRING_TOO_LONG;
					}else{
						GetCustTable(T("_User"), name, ThStrLastT(&Z->ExtendDst), newsize);
						ZQuotationDoubler_Ext(Z);
					}
				}
			}
		}
		if ( mode == StringVariable_extract ){
			TCHAR *longdst;

			PP_InitLongParam(namebuf);
			Z->result = PP_ExtractMacro(Z->hWnd, Z->Info, NULL, Z->dst, namebuf, XEO_EXTRACTLONG);
			if ( Z->result == ERROR_PARTIAL_COPY ){
				Z->result = NO_ERROR;
				longdst = PP_GetLongParamRAW(namebuf);
				SetLongDestMain(Z, longdst, 0);
				PP_FreeLongParamRAW(namebuf);
			}else{
				Z->dst = tstpcpy(Z->dst, namebuf);
			}
		}else{
			Z->dst += tstrlen(Z->dst);
		}
	}else{ // store
		const TCHAR *value;

		value = str + 1;
		while ( (str > name) && (*(str - 1) == ' ') ) str--;
		*str = '\0';
		if ( thStringValue != NULL ){
			ThSetString(thStringValue, name, value);
		}else{
			SetCustTable(T("_User"), name, value, TSTRSIZE(value));
		}
	}
	return ZMC_CONTINUE;
}

BOOL CALLBACK SearchWindowProc(HWND hWnd, LPARAM lParam)
{
	TCHAR caption[CMDLINESIZE];

	if ( GetWindowText(hWnd, caption, TSIZEOF(caption) - 1) == 0 ) return TRUE;
	if ( tstrstr(caption, ((SEARCHWINDOW *)lParam)->caption) != NULL ){
		((SEARCHWINDOW *)lParam)->hFoundWnd = hWnd;
		return FALSE;
	}
	return TRUE;
}

PPXDLL HWND PPXAPI GetWindowHandleByText(PPXAPPINFO *ppxa, const TCHAR *param)
{
	HWND hTargetWnd;
	SEARCHWINDOW sw;
	const TCHAR *ptr;
	TCHAR letter1, letter2;
									// PPx 指定あり？
	ptr = param;
	hTargetWnd = GetPPxhWndFromID(ppxa, &ptr, NULL);
	if ( (hTargetWnd != NULL) && (hTargetWnd != BADHWND) ){
		return hTargetWnd;
	}
	letter1 = upper(*param);
	letter2 = *(param + 1);
	if ( letter1 == '#' ){ // #数値 によるハンドル指定
		ptr = param + 1;
		hTargetWnd = (HWND)GetDigitNumber(&ptr);
		if ( (hTargetWnd != NULL) && (*ptr == '\0') ) return hTargetWnd;
	}else if ( ((letter1 == 'C') || (letter1 == 'V')) &&
				((Isalpha(letter2) && ((*(param + 2) == '\0'))) || (letter2 == '\0')) ){
		return NULL;
	}
									// ウィンドウサーチ
	sw.hFoundWnd = NULL;
	sw.caption = param;
	EnumWindows(SearchWindowProc, (LPARAM)&sw);
	return sw.hFoundWnd;
}

// オプションを取得 -----------------------------------------------------------
void ZReadOption(EXECSTRUCT *Z)
{
	TCHAR c;
	int i;

	while ( (c = *Z->src) != '\0' ){
		Z->src++;
		if ( c == ',' ) break;
		for ( i = 0 ; i < XEO_STRLENGTH ; i++ ){
			if ( c == XEO_OptionString[i] ){
				c = *Z->src;
				switch(c){
					case '-':
						Z->src++;
						resetflag(Z->flags, 1 << i);
						break;
					case '+':
						Z->src++;
					default:
						setflag(Z->flags, 1 << i);
						break;
				}
				break;
			}
		}
		if ( i >= XEO_STRLENGTH ){
			Z->src--;
			break;
		}
	}
}

void InsertSrc(EXECSTRUCT *Z, const TCHAR *newsrc)
{
	OLDSRCSTRUCT *next;
	int length;

	SkipSpace(&newsrc);
	if ( *newsrc == '\0' ) return;

	if ( Z->func.quotation ){
		const TCHAR *srcptr;

		length = 1;
		srcptr = newsrc;
		for (;;){
			TCHAR code;
			code = *srcptr++;
			if ( code == '\0' ) break;
			if ( code == '\"' ){
				length += 2;
			}else{
				length++;
			}
		}
	}else{
		length = tstrlen(newsrc) + 1;
	}

	next = HeapAlloc(DLLheap, 0, sizeof(OLDSRCSTRUCT) + TSTROFF(length));
	if ( next == NULL ) return;
	next->src = Z->src;
	next->backptr = Z->oldsrc;
	Z->oldsrc = next;
	Z->src = (TCHAR *)&next[1];
	memcpy((TCHAR *)&next[1], newsrc, TSTROFF(length));
	if ( Z->func.quotation ) tstrreplace((TCHAR *)&next[1], T("\""), T("\"\""));
}

void Freeoldsrc(EXECSTRUCT *Z)
{
	while ( Z->oldsrc != NULL ){
		OLDSRCSTRUCT *back;

		back = Z->oldsrc;
		Z->oldsrc = back->backptr;
		HeapFree(DLLheap, 0, back);
	}
}

void LoadHistory(EXECSTRUCT *Z, WORD historytype) // %H
{
	const TCHAR *ptr;
	WORD tmphistorytype;

	tmphistorytype = GetHistoryType(&Z->src);
	if ( tmphistorytype != 0 ) historytype = tmphistorytype;

	UsePPx();
	ptr = EnumHistory(historytype, (int)GetDigitNumber(&Z->src));
	if ( ptr != NULL ){
		tstrcpy(Z->dst, ptr);
		if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
		Z->dst += tstrlen(Z->dst);
	}
	FreePPx();
	return;
}

void ZGetMessageText(EXECSTRUCT *Z)
{
	const TCHAR *srcp;
	TCHAR *dstp, *maxdst;

	if ( *Z->src == '\"' ) Z->src++;

	srcp = MessageText(Z->src);
	dstp = Z->dst;
	maxdst = Z->DstBuf + CMDLINESIZE - 1;

	if ( (srcp == Z->src) || (srcp == (Z->src + MESTEXTIDLEN + 1)) ){
		for (;;){
			TCHAR c;

			c = *srcp;
			if ( c == '\0' ) break;
			srcp++;
			if ( c == '\"' ) break;
			if ( dstp >= maxdst ) break;
			*dstp++ = c;
		}
		Z->src = srcp;
	}else{
		for (;;){
			TCHAR c;

			c = *srcp;
			if ( c == '\0' ) break;
			srcp++;
			if ( dstp >= maxdst ) break;
			*dstp++ = c;
		}
		for (;;){
			TCHAR c;

			c = *Z->src;
			if ( c == '\0' ) break;
			Z->src++;
			if ( c == '\"' ) break;
		}
	}

	*dstp = '\0';
	Z->dst = dstp;
}

void ZExpandAliasWithExtract(EXECSTRUCT *Z)
{
	TCHAR buf[CMDLINESIZE];
	TCHAR *ptr;
	TCHAR *longdst;
	DWORD size;
	int flags = 0;

	ptr = buf;
	if ( *Z->src == '\'' ) Z->src++;
	while ( ((UTCHAR)*Z->src >= ' ') && (*Z->src != '\'') ) *ptr++ = *Z->src++;
	if ( *Z->src == '\'' ) Z->src++;
	*ptr = '\0';

	size = (DWORD)(CMDLINESIZE - (Z->dst - Z->DstBuf));
	if ( GetCustTable(T("A_exec"), buf, Z->dst, TSTROFF(size)) < 0 ){
		if ( GetEnvironmentVariable(buf, Z->dst, (DWORD)CMDLINESIZE) == 0 ){
			*Z->dst = '\0';
		}
	}
/*
	if ( (*Z->dst == '*') &&
		 (Z->dst == Z->DstBuf) && (Z->ExtendDst.top == 0) &&
		 (Z->command == CID_FILE_EXEC) &&
		 !(Z->flags & XEO_DISPONLY) ){
		flags = XEO_EXTRACTEXEC;
	}
*/
	PP_InitLongParam(buf);
	Z->result = PP_ExtractMacro(Z->hWnd, Z->Info, NULL, Z->dst, buf, flags | XEO_EXTRACTLONG);
	if ( Z->result == ERROR_PARTIAL_COPY ){
		Z->result = NO_ERROR;
		longdst = PP_GetLongParamRAW(buf);
		SetLongDestMain(Z, longdst, 0);
		PP_FreeLongParamRAW(buf);
	}else{
		Z->dst = tstpcpy(Z->dst, buf);
	}
}

void ZExpandAlias(EXECSTRUCT *Z)
{
	TCHAR name[CMDLINESIZE], *namep;
	LONG_PTR leftlen;

	namep = name;
	while ( ((UTCHAR)*Z->src >= ' ') && (*Z->src != '\'') ) *namep++ = *Z->src++;
	if ( *Z->src == '\'' ) Z->src++;
	*namep = '\0';

	leftlen = CMDLINESIZE - (Z->dst - Z->DstBuf);
	if ( leftlen <= 0 ) leftlen = 1;
	Z->dst[leftlen - 1] = '\0'; // 目印
	if ( NO_ERROR != GetCustTable(T("A_exec"), name, Z->dst, TSTROFF(leftlen)) ){
		int envlen;

		envlen = GetEnvironmentVariable(name, Z->dst, (DWORD)leftlen);
		if ( envlen >= leftlen ){
			if ( StoreLongParam(Z, envlen) == FALSE ) return;
			if ( ThSize(&Z->ExtendDst, TSTROFF(envlen)) == FALSE ){
				Z->result = RPC_S_STRING_TOO_LONG;
				return;
			}
			GetEnvironmentVariable(name, ThStrLastT(&Z->ExtendDst), envlen);
			ZQuotationDoubler_Ext(Z);
		}else{
			if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
		}
	}else if ( Z->dst[leftlen - 1] != '\0' ){ // 目印破損→足りない
		int newsize;

		newsize = GetCustTableSize(T("A_exec"), name);
		if ( StoreLongParam(Z, newsize) == FALSE ) return;
		if ( ThSize(&Z->ExtendDst, newsize) == FALSE ){
			Z->result = RPC_S_STRING_TOO_LONG;
			return;
		}
		GetCustTable(T("A_exec"), name, ThStrLastT(&Z->ExtendDst), newsize);
		ZQuotationDoubler_Ext(Z);
	}else{
		if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
	}
	Z->dst += tstrlen(Z->dst);
}

void GetPPxHWND(EXECSTRUCT *Z) // %N
{
	HWND hWnd;

	if ( *Z->src == '.' ){
		Z->src++;
		hWnd = GetParent(Z->hWnd);
	}else{
		hWnd = GetPPxhWndFromID(Z->Info, &Z->src, NULL);
		if ( hWnd == BADHWND ) return; // 該当無し
		if ( hWnd == NULL ) hWnd = Z->hWnd; // ID指定無し
	}
	if ( hWnd != NULL ) Z->dst = Numer32ToString(Z->dst, (DWORD)(DWORD_PTR)hWnd);
	return;
}

void GetPPxID(EXECSTRUCT *Z) // %n
{
	int len;

	if ( *Z->src != '#' ){ // 各窓のID
		// 一体化窓の CZabc 形式
		if ( (Z->Info->RegID[0] == 'C') && (Z->Info->RegID[2] == 'Z') ){
			*Z->dst = '\0';
			PPxInfoFunc(Z->Info, PPXCMDID_GETSUBID, Z->dst);
			if ( Z->dst[0] != '\0' ){
				Z->dst += tstrlen(Z->dst);
				return;
			}
		}
		// C_A → CA
		len = tstpcpy(Z->dst, Z->Info->RegID) - Z->dst;
		if ( Z->dst[1] == '_' ){
			Z->dst[1] = Z->dst[2];
			len--;
		}
	}else{ // 一体化窓
		Z->src++;
		if ( PPxInfoFunc(Z->Info, PPXCMDID_COMBOIDNAME, Z->dst) == 0 ) return;
		len = tstrlen32(Z->dst);
	}

	Z->dst += len;
	*Z->dst = '\0';
}

void GetPPxPath(EXECSTRUCT *Z) // %D
{
	HWND hWnd;

	*Z->dst = '\0';
	hWnd = GetPPxhWndFromID(Z->Info, &Z->src, Z->dst); // Z->dst に path が保存

	if ( hWnd == NULL ){ // デフォルト指定
		tstrcpy(Z->dst, GetZCurDir(Z));
	}else if ( hWnd == BADHWND ){ // 該当無し
		return;
	}else if ( *Z->dst == '\0' ){
		ExtractPPxCall(hWnd, Z, T("%1"));
	}
	Z->dst += tstrlen(Z->dst);
	return;
}

int FindSearch(const TCHAR *structs[], int structmax, const TCHAR *str)
{
	int mini, maxi;

	mini = 0;
	maxi = structmax;
	while ( mini < maxi ){
		int mid, result;

		mid = (mini + maxi) / 2;
		result = tstrcmp(structs[mid], str);
		if ( result < 0 ){
			mini = mid + 1;
		}else if ( result > 0 ){
			maxi = mid;
		}else{
			return mid;
		}
	}
	return -1;
}

void GetCursorFileName(EXECSTRUCT *Z) // %R, %Y, %t
{
	DWORD attr;

	GetValue(Z, *(Z->src - 1), Z->dst);
								// カーソルエントリのファイルの属性を入手
	if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_CSRATTR, (TCHAR *)&attr, &Z->EnumInfo) && (attr & FILE_ATTRIBUTE_DIRECTORY) ){
		setflag(Z->status, ST_SDIRREF | ST_CHKSDIRREF);
	}else{
		setflag(Z->status, ST_CHKSDIRREF);
	}
	Z->dst += tstrlen(Z->dst);
}

DWORD USEFASTCALL GetLongParamMaxLen(EXECSTRUCT *Z)
{
	if ( Z->status & ST_LONGRESULT ){
		return Z->LongResultLen;
	}else if ( ((Z->command == CID_FILE_EXEC) || // 実行(CID_FILE_EXEC) のとき
			(Z->command == CID_RUN) ||
			(Z->command == CID_LAUNCH) ) &&
		 !(Z->flags & (XEO_DISPONLY | XEO_NOEDIT)) ){
		return CMDEXE_LENGTH;
	}else if ( (Z->command == CID_SET) || // 環境変数は 32K-1 が最大値
			(Z->command == CID_LINECUST) ||
			(Z->command == CID_CUSTOMIZE) ||
			(Z->command == CID_SETCUST) ){
		return 0x7fc0; // 約32k
	}else if ( (Z->flags & XEO_EXTRACTLONG) ||
			(Z->command == 'I') ||
			(Z->command == 'Q') ||
			(Z->command == CID_MSGBOX) ||
			(Z->command == CID_MESSAGEBOX) ||
			(Z->command == CID_INSERT) ||
			(Z->command == CID_INSERTSEL) ||
			(Z->command == CID_REPLACE) ||
			(Z->command == CID_STRING) ||
			(Z->command == CID_LINEMESSAGE) ||
			(Z->command == CID_CLIPTEXT) ){
		return 0x6fffffff;
	}else{
		return CMDLINESIZE;
	}
}

BOOL StoreLongParam(EXECSTRUCT *Z, DWORD addlen)
{
	DWORD size;

	if ( (Z->ExtendDst.top + (Z->dst - Z->DstBuf) + addlen) >= GetLongParamMaxLen(Z) ){
		return FALSE;
	}

	setflag(Z->status, ST_EXTENDDST);
	// 長いパラメータに対応しているので、保存
	*Z->dst = '\0';
	size = TSTROFF32(Z->dst - Z->DstBuf + 1);
	ThAppend(&Z->ExtendDst, Z->DstBuf, size);
	Z->ExtendDst.top -= TSTROFF(1); // '\0' 分減らす
	Z->dst = Z->DstBuf;
	*Z->dst = '\0';
	return TRUE;
}

DWORD GetModuleNameHash(const TCHAR *src, TCHAR *dest)
{
	DWORD hash = 0;

	while( *src != '\0' ){
		TCHAR c;

		c = upper(*src++);
		*dest++ = c;
		hash = (DWORD)(hash << 6) | (DWORD)(hash >> (32 - 6)) | (DWORD)c;
	}
	*dest = '\0';
	return hash | CID_USERNAME;
}

void ZMemo(EXECSTRUCT *Z) // %m
{
	if ( *Z->src == '\"' ){
		Z->src++;
		for (;;){
			UTCHAR c;

			c = (UTCHAR)*Z->src;
			if ( c < ' ' ) break;
			Z->src++;
			if ( c == '\"' ) break;
		}
	}else{
		while ( (UTCHAR)(*Z->src) > ' ' ) Z->src++;
	}
}

void GetPairMacro(EXECSTRUCT *Z) // %~
{
	TCHAR *dstp;
	HWND hPairWnd;

	dstp = Z->dst;
	*dstp++ = '%';
	while ( Isalnum(*Z->src) || (*Z->src == '#') ) *dstp++ = *Z->src++;
	*dstp = '\0';
	hPairWnd = GetPPcPairWindow(Z->Info);
	if ( hPairWnd != NULL ) ExtractPPxCall(hPairWnd, Z, Z->dst);
}

void RestoreSrc(EXECSTRUCT *Z)
{
	OLDSRCSTRUCT *back;

	back = Z->oldsrc;
	Z->src = back->src;
	Z->oldsrc = back->backptr;
	HeapFree(DLLheap, 0, back);
}

#define GCS_NOCMD 0
#define GCS_CMD 1
#define GCS_GOTO 2
int GetCommandString(EXECSTRUCT *Z)
{
	TCHAR cmdname[CmdFuncMaxLen], *dest, firstchar;
	const TCHAR *src;
	int i;
									// コマンドを切り出す
	dest = cmdname;
	firstchar = SkipSpace(&Z->src);
	src = Z->src;
	if ( firstchar == '*' ){
		*dest++ = firstchar;
		src++;
	}
	for ( i = 0 ; i < (int)(TSIZEOF(cmdname) - 1) ; i++ ){
		if ( !Isalnum(*src) ) break;
		*dest++ = *src++;
	}
	*dest = '\0';
									// *が付加→コマンド
	if ( cmdname[0] == '*' ){
		DWORD namehash;
		int cmdid;

		if ( dest == (cmdname + 1) ) return GCS_NOCMD; // 文字列無し
		SkipSpace(&src);
		Z->src = src;

		namehash = GetModuleNameHash(cmdname + 1, cmdname + 1);
		cmdid = FindSearch(CmdName, CID_COUNTS, cmdname + 1);
		if ( cmdid >= 0 ){
			if ( cmdid == (CID_GOTO - CID_MINID) ) return GCS_GOTO;
			if ( cmdid != (CID_SKIP - CID_MINID) ){
				Z->command = cmdid + CID_MINID;
			}else{
				CmdSkip(Z);
			}
		}else{
			Z->command = namehash;
			tstrcpy(Z->dst, cmdname + 1);
			// tstrlen(cmdname + 1) + 1 の最適化結果↓
			Z->dst += tstrlen(cmdname);
			*Z->dst = '\0';
		}
		return GCS_CMD;
	}
									// 記号類が付加→コマンドでない
	if ( dest == cmdname ) return GCS_NOCMD; // 文字列無し
	if ( (UTCHAR)*src <= ' ' ){
		if ( NO_ERROR == GetCustTable(T("A_exec"), cmdname, Z->dst,
				TSTROFF(CMDLINESIZE) - TSTROFF(Z->dst - Z->DstBuf)) ){ // エイリアス有り
			Z->src = src;
			InsertSrc(Z, Z->dst);
			return GCS_CMD;
		}
	}
	return GCS_NOCMD;
}

#if 0
int ZGetOneParameter(EXECSTRUCT *Z, TCHAR separator)
{
	int stringoff;
	BOOL quotation = FALSE;
	const TCHAR *src;

	if ( Z->ExtendDst.top != 0 ) StoreLongParam(Z, 0);
	stringoff = Z->ExtendDst.top + (Z->dst - Z->DstBuf) * sizeof(TCHAR);

	src = Z->src;
	SkipSpace(&Z->src);
	for (;;){
		TCHAR code;
		BOOL result;

		code = *src;
		if ( code == '\0' ){ // src の nested を戻す / 関数の途中なのでエラー
			if ( Z->oldsrc == NULL ){
				if ( !(Z->flags & XEO_DISPONLY) ){
					XMessage(NULL, NULL, XM_GrERRld, MES_EFUT);
				}
				Z->result = ERROR_INVALID_FUNCTION;
				break;
			}
			RestoreSrc(Z);
			src = Z->src;
			continue;
		}

		if ( (Z->dst - Z->DstBuf) >= (CMDLINESIZE - 1) ){ // 長さ制限
			if ( StoreLongParam(Z, 0) == FALSE ){
				Z->result = RPC_S_STRING_TOO_LONG;
				break;
			}
		}

		if ( code == '\"' ){ // " チェック
			src++;
			if ( quotation ){
				if ( *src != '\"' ){
					quotation = FALSE;
					SkipSpace(&src);
				}else{ // "" → " に置換
					*Z->dst++ = '\"';
					src++;
				}
			}else{
				quotation = TRUE;
			}
			continue;
		}

		if ( quotation == FALSE ){
			if ( code == ')' ) break; // 関数末端
			if ( code == separator ) break; // 区切り
		}

		// ※改行はパラメータ内に含める
		if ( code != '%' ){				// '%' 以外はそのまま複写
			*Z->dst++ = code;
			src++;
			continue;
		}
		Z->src = src;
		result = ZMacroCharacter(Z);
		src = Z->src;
		if ( (result == ZMC_BREAK) || (Z->result != NO_ERROR) ) break;
	}
	Z->src = src;
	*Z->dst++ = '\0';
	return (Z->result == NO_ERROR) ? stringoff : -1;
}

ERRORCODE ZFunctionRegExp(EXECSTRUCT *Z)
{
	const TCHAR *long_result;
	TCHAR *src, *pattern;
	PXREGEXPS *rexps;
	int stringoff, regexpoff;
	EXECFUNC oldfunc;

	oldfunc = Z->func;

	if ( SkipSpace(&Z->src) != '(' ) goto error; // 括弧無し
	Z->src++;
	Z->func.quotation = FALSE;

	stringoff = ZGetOneParameter(Z, ',');
	if ( stringoff < 0 ) goto error;
	if ( *Z->src != ',' ) goto error_1;
	Z->src++;
	regexpoff = ZGetOneParameter(Z, ')');
	if ( (regexpoff < 0) || (*Z->src != ')') ) goto error;
	Z->src++;

	if ( Z->ExtendDst.top == 0 ){
		src = Z->DstBuf + stringoff / sizeof(TCHAR);
		pattern = Z->DstBuf + regexpoff / sizeof(TCHAR);
	}else{
		if ( StoreLongParam(Z, 0) == FALSE ){
			Z->result = RPC_S_STRING_TOO_LONG;
			return Z->result;
		}
		src = (TCHAR *)(Z->ExtendDst.bottom + stringoff);
		pattern = (TCHAR *)(Z->ExtendDst.bottom + regexpoff);
	}

	if ( FALSE == InitRegularExpression(&rexps, pattern, SLASH_REQUIRE) ){
		goto error;
	}
	Z->dst = src;
	long_result = RegularExpressionReplace(rexps, src, Z->dst, CMDLINESIZE);
	if ( long_result != NULL ){
		if ( long_result == Z->dst ){
			Z->dst += tstrlen(Z->dst);
		}else{
			if ( IsTrue(StoreLongParam(Z, 0)) ){
				ThCatString(&Z->ExtendDst, long_result);
			}
		}
	}
	FreeRegularExpression(rexps);
	Z->func = oldfunc;
	return NO_ERROR;

error_1:
	if ( Z->ExtendDst.top == 0 ){
		src = Z->DstBuf + stringoff / sizeof(TCHAR);
	}else{
		if ( StoreLongParam(Z, 0) == FALSE ){
			Z->result = RPC_S_STRING_TOO_LONG;
			return Z->result;
		}
		src = (TCHAR *)(Z->ExtendDst.bottom + stringoff);
	}
	if ( *src == '?' ){
		if ( *Z->src == ')' ) Z->src++;
		Z->dst = GetRegularExpressionName(src);
		return NO_ERROR;
	}
error:
	XMessage(NULL, NULL, XM_GrERRld, T("%*regexp(src,regexp)"));
	Z->result = ERROR_INVALID_PARAMETER;
	Z->func = oldfunc;
	return ERROR_INVALID_PARAMETER;
}
#endif

ERRORCODE ZFunction(EXECSTRUCT *Z)
{
	TCHAR *dst;
	const TCHAR *olds;
	EXECFUNC oldfunc;

	for ( dst = Z->dst ; Isalnum(*Z->src) ; ) *dst++ = *Z->src++;
	*dst++ = '\0'; // 関数名末端
	*dst = '\0'; // パラメータ末端

//	if ( tstrcmp(Z->dst, T("re")) == 0 ) return ZFunctionRegExp(Z);

	oldfunc = Z->func;
	Z->func.off = Z->ExtendDst.top + (Z->dst - Z->DstBuf) * sizeof(TCHAR) + 1;
	Z->dst = dst;

	olds = Z->src;
	if ( SkipSpace(&Z->src) != '(' ){ // 括弧無し
		Z->src = olds;
		ExecuteFunction(Z);
		Z->func.off = oldfunc.off;
		return Z->result;
	}

	// 括弧有り
	Z->func.quotation = FALSE;
	Z->src++;

	for (;;){
		if ( *Z->src == '\0' ){ // src の nested を戻す
			if ( Z->oldsrc == NULL ){
				if ( !(Z->flags & XEO_DISPONLY) ){
					XMessage(NULL, NULL, XM_GrERRld, MES_EFUT);
				}
				Z->result = ERROR_INVALID_FUNCTION;
				break;
			}
			RestoreSrc(Z);
			continue;
		}

		if ( (Z->dst - Z->DstBuf) >= (CMDLINESIZE - 1) ){ // 長さ制限
			if ( StoreLongParam(Z, 0) == FALSE ){
				Z->result = RPC_S_STRING_TOO_LONG;
				break;
			}
		}

		// 関数内解析
		if ( *Z->src == '\"' ){ // " チェック
			*Z->dst++ = *Z->src++;
			if ( Z->func.quotation ){
				if ( *Z->src != '\"' ){
					Z->func.quotation = FALSE;
				}else{ // "" → " に置換
					*Z->dst++ = '\"';
					Z->src++;
					continue;
				}
			}else{
				Z->func.quotation = TRUE;
				continue;
			}
		}
		if ( (*Z->src == ')') && (Z->func.quotation == FALSE) ){ // 関数末端
			Z->src++;
			Z->func.quotation = oldfunc.quotation;
			ExecuteFunction(Z);
			break;
		}

		// ※改行はパラメータ内に含める

		if ( *Z->src != '%' ){				// '%' 以外はそのまま複写
			*Z->dst++ = *Z->src++;
			continue;
		}
		if ( ZMacroCharacter(Z) == ZMC_BREAK ) break;
		if ( Z->result != NO_ERROR ) break;
	}
	Z->func = oldfunc;
	return Z->result;
}

void ZBlockEscape(EXECSTRUCT *Z)
{
	TCHAR code;
	int count = 0;

	Z->src++;
	for (;;){
		code = *Z->src;
		if ( code == '\0' ){ // src の nested を戻す
			if ( Z->oldsrc == NULL ) break;
			RestoreSrc(Z);
			continue;
		}
		if ( (Z->dst - Z->DstBuf) >= (CMDLINESIZE - 1) ){ // 長さ制限
			if ( StoreLongParam(Z, 0) == FALSE ){
				Z->result = RPC_S_STRING_TOO_LONG;
				return;
			}
		}
		if ( code == '\"' ){
			if ( Z->func.quotation ) *Z->dst++ = '\"';
		}
		if ( code != '%' ){				// '%' 以外はそのまま複写
			*Z->dst++ = code;
			Z->src++;
			continue;
		}
		code = *(Z->src + 1);
		Z->src += 2;
		if ( code == ')' ){
			if ( count <= 0 ) return;
			count--;
		}
		*Z->dst++ = '%';
		*Z->dst++ = code;
		if ( code == '(' ) count++;
		continue;
	}
}

// TRUE:解釈継続、FALSE:解釈終了
BOOL ZMacroCharacter(EXECSTRUCT *Z)	// マクロ % 解析
{
	Z->src++;
	if ( *Z->src == '(' ){	// % 展開抑制
		ZBlockEscape(Z);
		return ZMC_CONTINUE;
	}
	if ( *Z->src == ')' ){	// % 展開抑制解除
		if ( !(Z->flags & XEO_DISPONLY) ){
			XMessage(NULL, NULL, XM_GrERRld, MES_EFUT);
		}
		return ZMC_BREAK;
	}
	Z->edit.EdOffset = -1;
	resetflag(Z->status, ST_PATHITEM | ST_PATHFIX);
	if ( *Z->src == '!' ){		// %!	部分編集 ------------------
		Z->edit.EdOffset = GetDestOffset(Z);
		Z->src++;
	}else if ( *Z->src == '$' ){	// %$	キャッシュ付き部分編集 ---
		setflag(Z->status, ST_USECACHE);
		Z->edit.EdOffset = GetDestOffset(Z);
		Z->src++;
		Z->edit.cache.srcptr = Z->src;
	}
	*Z->dst = '\0';
	switch( *Z->src++ ){
						//----- NULL	文末 --------------------------
		case '\0':
			Z->src--;
			break;
						//----- %"		タイトル変更 ------------------
		case '\"':
			SetTitleMacro(Z);
			break;
						//----- %#		対象ファイル名 ----------------
		case '#':
			EnumEntries(Z);
			break;
						//----- %'		A_exec / 環境変数参照 ---------
		case '\'':
			ZExpandAlias(Z);
			break;
						//----- %*		関数・関数モジュール
		case '*':
			if ( ZFunction(Z) == NO_ERROR ) break;
			return ZMC_BREAK;

		case '0':		//----- %0		PPx ディレクトリ --------------
		case '1':			//	%1	カーソル位置のディレクトリ
		case '2':			//	%2	カーソルの反対位置ディレクトリ
		case '3':			//	%3	*pack 内で使用
		case '4':			//	%4	*pack 内で使用
			setflag(Z->status, ST_PATHITEM);
			GetValue(Z, *(Z->src - 1), Z->dst);
			Z->dst += tstrlen(Z->dst);
			break;
						//----- %&	終了コードで判断 ----------------
		case '&':
			return ZExecAndCheckError(Z);
						//----- %:		コマンド区切り ----------------
		case ':':
			if ( Z->flags & XEO_DISPONLY ){		// 表示のみ
				*Z->dst++ = ':';
				break;
			}else{					// 実行
				ZExec(Z);
				if ( Z->result != NO_ERROR ) { ZErrorFix(Z); return ZMC_BREAK; }
				ZInitSentence(Z);
				return ZMC_CONTINUE;
			}

		case 'C':		// %C		対象ファイル名 ------------
		case 'X':		// %X	対象ファイル拡張子なし --------
		case 'T':		// %T	対象ファイル拡張子のみ --------
			ZGetName(Z, Z->dst, '\0');
			Z->dst += tstrlen(Z->dst);
			break;
						//----- %D		PPxのパス取得 ------------
		case 'D':
			GetPPxPath(Z);
			break;
						//----- %E		パラメータ入力 ----------------
		case 'E':
			Z->edit.EdOffset = GetDestOffset(Z);
			break;
						//----- %F		 ----------------
		case 'F':
			ZGetName(Z, Z->dst, '\0');
			if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
			Z->dst += tstrlen(Z->dst);
			break;
						//----- %G		 ----------------
		case 'G':
			ZGetMessageText(Z);
			break;
						//----- %H		ヒストリー呼び出し ------------
		case 'H':
			LoadHistory(Z, PPXH_GENERAL | PPXH_COMMAND);
			break;

		case 'I':		//	%I	情報メッセージボックス
		case 'J':		//	%J	エントリ移動
		case 'K':		//	%K	内蔵コマンド
		case 'Q':		//	%Q	確認メッセージボックス
		case 'Z':		//	%Z	ShellExecute で実行
		case 'j':		//	%j	パスジャンプ
		case 'k':		//	%k	内蔵コマンド
		case 'v':		//	%v	PPV で表示
		case 'z':		//	%z	ShellContextMenu で実行
			if ( Z->flags & XEO_DISPONLY ){	// 表示
				*Z->dst++ = '%';
				*Z->dst++ = *(Z->src - 1);
			}else{
				ZExec(Z);
				if ( Z->result != NO_ERROR ) { ZErrorFix(Z); return ZMC_BREAK; }
				ZInitSentence(Z);
				Z->command = *(Z->src - 1);
				SkipSpace(&Z->src);
			}
			break;

		case 'L':		//	%Ln	PPv の論理行数 ----------------
		case 'l':		//	%ln	PPv の表示行数 ----------------
			ZGetPPvCursorPos(Z);
			break;
						//----- %M		補助メニュー／拡張子判別 ------
		case 'M':
			if ( MenuCmd(Z) ) return ZMC_CONTINUE;
			break;
						//----- %N		PPxのHWND取得 ------------
		case 'N':
			GetPPxHWND(Z);
			break;
						//----- %O		オプション再設定 --------------
		case 'O':
			ZReadOption(Z);
			break;
						//----- %P		パス・ファイル名入力 ----------
		case 'P':
			setflag(Z->status, ST_PATHITEM | ST_PATHFIX);
			Z->edit.EdOffset = GetDestOffset(Z);
			break;

		case 'R':		//----- %R		カーソル位置ファイル名 --------
		case 'Y':			//	%Y	カーソル位置の拡張子なし ------
			setflag(Z->status, ST_PATHITEM);
			// %t へ
		case 't':			//	%t	カーソル位置の拡張子のみ ------
			GetCursorFileName(Z);
			break;
						//------ %S		ディレクトリがある場合の文字列-
		case 'S':
			ZStringOnDir(Z);
			break;
						//----- %W		Window Caption取得 ------------
		case 'W':
			ZGetWndCaptionMacro(Z);
			break;
						//----- %\		\ の付加 ----------------------
		case '\\':
			ZAddSeparator(Z);
			break;
						//----- %@		レスポンスファイル 対応 %C ----
		case '@':
			*Z->dst++ = '@';
			// %a へ
		case 'a':		//----- %a		レスポンスファイル 対応 %C ----
			setflag(Z->status, ST_PATHITEM);
			if ( CreateResponseFile(Z) == FALSE ) return ZMC_BREAK;
			break;
						//----- %b		行末エスケープ・文字挿入 ------
		case 'b':
			LineEscape(Z);
			break;
						//----- %e		一行編集の設定変更 ------------
		case 'e':
			SetEditMode(Z);
			break;
						//----- %g		A_exec展開
		case 'g':
			ZExpandAliasWithExtract(Z);
			break;
						//----- %h		ヒストリー呼び出し ------------
		case 'h':
			LoadHistory(Z, PPXH_PPCPATH);
			break;
						//----- %m		追加情報(何もしない) ----------
		case 'm':
			ZMemo(Z);
			return ZMC_CONTINUE;
						//----- %n		RegID 取得 ------------
		case 'n':
			GetPPxID(Z);
			break;
						//----- %s		特殊環境変数
		case 's':
			return ZStringVariable(Z, &Z->src, StringVariable_function);
						//----- %u	UnXXX を実行 ------------------
		case 'u':
			if ( Z->flags & XEO_DISPONLY ){	// 表示
				*Z->dst++ = '%';
				*Z->dst++ = 'u';
			}else{
				ZExec(Z);
				if ( Z->result != NO_ERROR ){
					ZErrorFix(Z);
					return ZMC_BREAK;
				}
				ZInitSentence(Z);
				Z->command = 'u';
			}
			break;
						//----- %{		部分編集指定開始 --------------
		case '{':
			Z->edit.EdOffset = -1; // %$ での編集を無効にする
			Z->edit.EdPartStart = GetDestOffset(Z);
			Z->edit.CsrStart = 0;
			Z->edit.CsrEnd = -1;
			break;
						//----- %|		カーソル位置指定 --------------
		case '|':
			if ( Z->edit.CsrEnd == -1 ){	// 1st:指定位置にカーソル
				Z->edit.CsrStart = Z->edit.CsrEnd = GetDestOffset(Z) - Z->edit.EdPartStart;
			}else{					// 2nd:指定範囲を選択
				Z->edit.CsrStart = Z->edit.CsrEnd;
				Z->edit.CsrEnd = GetDestOffset(Z) - Z->edit.EdPartStart;
			}
			break;
						//----- %}		部分編集指定終了 --------------
		case '}':
			Z->edit.EdOffset = Z->edit.EdPartStart;
			break;
						//----- %~		反対窓内容取得 ----------------
		case '~':
			GetPairMacro(Z);
			break;
						//----- 未定義	その文字自身 ------------------
		default:
			*Z->dst++ = *(Z->src - 1);
	}

	if ( (Z->edit.EdOffset >= 0) && !(Z->flags & (XEO_DISPONLY | XEO_NOEDIT)) ){
		if ( EditExtractText(Z) == FALSE ) return ZMC_BREAK; // %!x 部分編集
	}
	return ZMC_CONTINUE;
}

// Z の初期化 -----------------------------------------------------------------
void ZInitSentence(EXECSTRUCT *Z)
{
	Z->command = CID_FILE_EXEC;
	Z->dst = Z->DstBuf;
	Z->edit.EdPartStart = 0;
	Z->edit.CsrStart = 0;
	Z->edit.CsrEnd = -1;
	Z->func.off = 0;
	Z->func.quotation = FALSE;
}

/*-----------------------------------------------------------------------------
	hWnd		親ウィンドウ
	pos			ウィンドウ表示する時に用いるスクリーン座標。NULLならhWndの(0, 0)
	param		パラメータ(ASCIIZ)
	extract		展開先。必要なければ NULL でよい。
	flag		各種設定
	戻り値		NO_ERROR:正常終了
				259(ERROR_NO_MORE_ITEMS):空欄終了
				ERROR_CANCELLED :キャンセル
-----------------------------------------------------------------------------*/
PPXDLL ERRORCODE PPXAPI PP_ExtractMacro(HWND hWnd, PPXAPPINFO *ParentInfo, POINT *pos, const TCHAR *param, TCHAR *extract, int flags)
{
	EXECSTRUCT Z;
										// 最初の初期化 -----------------------
	ThInit(&Z.ExtendDst);
	ThInit(&Z.ExpandCache);
	ThInit(&Z.StringVariable);
	Z.hWnd = hWnd;
	Z.posptr = pos;
	Z.Info = (ParentInfo != NULL) ? ParentInfo : &PPxDefInfo;
	Z.oldsrc = NULL;
	Z.extract = extract;
	Z.status = 0;
	Z.result = NO_ERROR;
	Z.curdir[0] = '\0';
	Z.useppb = -1;
	Z.edit.cache.hash = MAX32;

	PPxEnumInfoFunc(Z.Info, PPXCMDID_STARTENUM, Z.DstBuf, &Z.EnumInfo);
#ifdef WINEGCC
	setflag(flags, XEO_NOUSEPPB);
#endif
	for (;;){		// コマンドライン初期化 ----------------
		Z.flags = flags;
		Z.status = Z.status & ST_MASK_FIRSTCMDLINE;
		Z.src = param;
		Z.ExtendDst.top = 0;
		ZInitSentence(&Z);
										// 1文の頭 : 解析開始 -----------------
		for (;;){
			if ( *Z.src == '\0' ){ // src の nested を戻す
				if ( Z.oldsrc == NULL ) break;
				RestoreSrc(&Z);
				continue;
			}
											// *コマンド / A_exec コマンド
			if ( (Z.dst == Z.DstBuf) &&
				 (Z.ExtendDst.top == 0) &&
				 (Z.command == CID_FILE_EXEC) &&
				 !(Z.flags & XEO_DISPONLY) &&
				 ((extract == NULL) || (Z.flags & XEO_EXTRACTEXEC)) ){
				int cmdr = GetCommandString(&Z);
				if ( cmdr != GCS_NOCMD ){
					if ( cmdr == GCS_GOTO ){
						if ( CmdGoto(&Z, param) == FALSE ) break;
					}
					continue;
				}
			}else if ( (Z.dst - Z.DstBuf) >= (CMDLINESIZE - 1) ){ // 長さ制限
				if ( StoreLongParam(&Z, 0) == FALSE ){
					Z.result = RPC_S_STRING_TOO_LONG;
					PPErrorBox(Z.hWnd, NULL, RPC_S_STRING_TOO_LONG);
					break;
				}
			}

			if ( *Z.src == '\"' ) Z.func.quotation = !Z.func.quotation;

			if ( ((UTCHAR)(*Z.src) < ' ') &&			// 改行
				 ((*Z.src == '\n') || (*Z.src == '\r')) &&
				 !(Z.flags & XEO_INRETURN) ){
				do {
					Z.src++;
				} while ( (*Z.src == '\n') || (*Z.src == '\r') );
				if ( Z.flags & XEO_DISPONLY ){		// 表示のみ
					*Z.dst++ = ':';
				}else{					// 実行
					*Z.dst = '\0';
					ZExec(&Z);
					if ( Z.result != NO_ERROR ) {
						ZErrorFix(&Z);
						goto execbreak;
					}
					ZInitSentence(&Z);
				}
				continue;
			}

			if ( *Z.src != '%' ){				// '%' 以外はそのまま複写
				*Z.dst++ = *Z.src++;
				continue;
			}
			if ( ZMacroCharacter(&Z) == ZMC_BREAK ) break;
			if ( Z.result == NO_ERROR ) continue;
			if ( extract != NULL ) *extract = '\0';
			goto execbreak;
		} // 全文の展開が完了
		*Z.dst = '\0';

		if ( extract != NULL ){	// 展開処理
			if ( Z.result != NO_ERROR ){
				*extract = '\0';
				break; // エラー有り
			}
			if ( (Z.ExtendDst.top != 0) ||
				 ((Z.dst - Z.DstBuf) >= CMDLINESIZE) ){ // 長さ制限
				SetLongParamToParam(&Z, (LONGEXTRACTPARAM *)extract);
			}else{
				memcpy(extract, Z.DstBuf, (Z.dst - Z.DstBuf + 1) * sizeof(TCHAR) );
			}
			break;
		}						// 実行処理
		if ( Z.result != NO_ERROR ) break; // エラー有り
		ZExec(&Z);
		if ( Z.result != NO_ERROR ) break; // ZErrorFix 不要 (*return処理不要)

		// 次のマークを処理するか判断
		if ( (Z.flags & XEO_EXECMARK) && !(Z.flags & XEO_NOEXECMARK) ){
			if ( !(Z.status & (ST_NO_NEXTENUM | ST_LOOPEND)) ){
				if ( PPxEnumInfoFunc(Z.Info, PPXCMDID_NEXTENUM, Z.DstBuf, &Z.EnumInfo) == 0 ){
					break;
				}
			}else{ // %#, %@ は既に PPXCMDID_NEXTENUM を実行済み
				if ( Z.status & ST_LOOPEND ) break;
			}
			// 未処理マークがあるので再度コマンドラインを解析
			setflag(Z.status, ST_2ndLOOP);
			Freeoldsrc(&Z);

			if ( Z.result == NO_ERROR ) continue;
			if ( Z.result == RPC_S_STRING_TOO_LONG ){
				PPErrorBox(Z.hWnd, NULL, RPC_S_STRING_TOO_LONG);
			}
		}
		break;
	}
execbreak:
	PPxEnumInfoFunc(Z.Info, PPXCMDID_ENDENUM, Z.DstBuf, &Z.EnumInfo);

	if ( Z.flags & (XEO_DELTEMP | XEO_DOWNCSR) ){
		if ( (extract == NULL) && (Z.flags & XEO_DELTEMP) ){
			const TCHAR *ResName;

			ResName = ThGetString(&Z.StringVariable, ResponseName_ValueName, NULL, 0);
			if ( ResName != NULL ) DeleteFileL(ResName);
		}
		if ( (Z.flags & XEO_DOWNCSR) && (Z.result == NO_ERROR) ){
			PostMessage(Z.hWnd, WM_PPXCOMMAND, K_raw | K_dw, 0);
		}
	}

	ThFree(&Z.StringVariable);
	ThFree(&Z.ExpandCache); // %M のキャッシュを解放
	ThFree(&Z.ExtendDst);
	Freeoldsrc(&Z);
	if ( Z.useppb != -1 ) FreePPb(Z.hWnd, Z.useppb);
	return Z.result;
}
/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
BOOL IsCmdNul(TCHAR *cmdbuf)
{
	if ( cmdbuf[0] == '\0' ) return TRUE;
	if ( cmdbuf[1] == '\0' ){
		if ( ((UTCHAR)cmdbuf[0] == EXTCMD_CMD) ||
			 ((UTCHAR)cmdbuf[0] == EXTCMD_KEY) ){
			return TRUE;
		}
	}
	return FALSE;
}

PPXDLL int PPXAPI PP_GetExtCommand(const TCHAR *src, const TCHAR *ID, TCHAR *cmdbuf, TCHAR *TypeName)
{
	VFSFILETYPE vft;
	int count = 0;
	TCHAR kword[CUST_NAME_LENGTH], name[VFPS];
	BYTE *filebuf;
	const TCHAR *namep, *extp;
	DWORD hsize = 0;
	HANDLE hFile;
	FN_REGEXP fn;
	int result = PPEXTRESULT_FILE;
	BOOL dir = FALSE;
	ERRORCODE errresult;

	cmdbuf[0] = '\0';
	filebuf = HeapAlloc(DLLheap, 0, VFS_check_size);
	if ( filebuf == NULL ) return PPEXTRESULT_NONE;

										// ファイルの内容で判別 ---------------
	vft.flags = VFSFT_TYPE | VFSFT_STRICT;
	vft.type[0] = '\0';

	if ( TypeName != NULL ){
		if ( TypeName == (TCHAR *)FILE_ATTRIBUTE_DIRECTORY ){
			TypeName = NULL;
			dir = TRUE;
		}else if ( tstrstr(src, T(".{")) != NULL ){
			result = PPEXTRESULT_CLSID;
		}
	}

	hFile = CreateFileL(src, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile != INVALID_HANDLE_VALUE ){
		hsize = ReadFileHeader(hFile, filebuf, VFS_check_size);
		CloseHandle(hFile);

		if ( (hsize != 0) && (TypeName != NULL) ){ // ファイルならディレクトリかどうかの調査も行う
			if ( (hsize > 100) && (*filebuf == 0x4c) &&
					(*(filebuf + 1) == 0) && (*(filebuf + 2) == 0) ){
				result = PPEXTRESULT_LINK;
			}else{
				tstrcpy(name, src);

				if ( VFSCheckDir(name, filebuf, hsize, NULL) ){
					result = PPEXTRESULT_VFSDIR;
				}
			}
		}
	}

	errresult = VFSGetFileType(src, (char *)filebuf, hsize, &vft);
	HeapFree(DLLheap, 0, filebuf);

	namep = FindLastEntryPoint(src);
	extp = namep + FindExtSeparator(namep);

	if ( errresult == NO_ERROR ){
		if ( TypeName != NULL ) tstrcpy(TypeName, vft.type);

		if ( GetCustDword(T("X_exts"), 0) == 0 ){
			if ( *extp == '.' ){				// 拡張子あり
				thprintf(name, TSIZEOF(name), T("%s%s"), vft.type, extp);
				if ( NO_ERROR == GetCustTable(ID, name, cmdbuf, TSTROFF(CMDLINESIZE)) ){
					if ( !IsCmdNul(cmdbuf) ) return result; // 空欄なら以降の名前判別へ
				}
			}

			if ( NO_ERROR == GetCustTable(ID, vft.type, cmdbuf, TSTROFF(CMDLINESIZE)) ){
				if ( !IsCmdNul(cmdbuf) ) return result; // 空欄なら以降の名前判別へ
			}
			vft.type[0] = '\0'; // 内容判別できなかったので名前判別へ
		}
	}
										// ファイル名・拡張子の抽出 -----------
	if ( *extp == '.' ){				// 拡張子あり
		extp++;
		if ( TypeName != NULL ) tstrcpy(TypeName, extp);
	}else{								// 拡張子なし
		tstrcpy(name, namep);
		tstrcat(name, T("."));
		namep = name;
		if ( TypeName != NULL ) tstrcpy(TypeName, name);
	}
										// ファイル名で判別 -------------------
	for ( ; EnumCustTable(count, ID, kword, cmdbuf, ISCMDNULREQUEIERSIZE) >= 0 ; count++ ){ // 限定
		if ( kword[0] == ':' ){ // ファイル内容種別だった
							// 種別で判定済み or 種別無しなので次へ
			if ( vft.type[0] == '\0' ) continue;
			if ( !IsCmdNul(cmdbuf) && !tstricmp(kword, vft.type) ) goto enumhit;
		}else if ( kword[0] == '/' ){ // ワイルドカード指定
			int fnresult;

			MakeFN_REGEXP(&fn, kword + 1);
			fnresult = FilenameRegularExpression(namep, &fn);
			FreeFN_REGEXP(&fn);
			if ( fnresult != FRRESULT_NO ) goto enumhit;
		}
		if ( tstrchr(kword, '.') ){			// 名前で判別
			if ( !tstricmp(kword, namep) ) goto enumhit;
		}else{								// 拡張子で判別
			if ( !tstricmp(kword, extp) ) goto enumhit;
		}
	}
	if ( (*extp == '\0') &&
		 (NO_ERROR == GetCustTable(ID, T("."), cmdbuf, TSTROFF(CMDLINESIZE))) ){
		return result;
	}
	if ( dir && (NO_ERROR == GetCustTable(ID, T(":DIR"), cmdbuf, TSTROFF(CMDLINESIZE))) ){
		return result;
	}
	if ( NO_ERROR == GetCustTable(ID, WildCard_All, cmdbuf, TSTROFF(CMDLINESIZE)) ){
		return result;
	}
	if ( result != PPEXTRESULT_FILE ) return -result;
	return PPEXTRESULT_NONE;

enumhit:
	EnumCustTable(count, ID, kword, cmdbuf, TSTROFF(CMDLINESIZE));
	return result;
}

// %# 本体
void EnumEntries(EXECSTRUCT *Z)
{
	BOOL useopt = FALSE;
	const TCHAR *oldsrc;
	const TCHAR *ptr;
	TCHAR separator = ' ', buf[VFPS];
	int maxentries = 0xfffffff;
	DWORD maxlen;

	oldsrc = Z->src;
	if ( Isdigit(*oldsrc) ) maxentries = GetDigitNumber(&oldsrc);

	if ( (UTCHAR)*oldsrc > ' ' ){
		if ( !Isalnum(*oldsrc) ){
			separator = *oldsrc++;
		}else if ( *oldsrc == 'n' ){
			separator = '\n';
			oldsrc++;
			setflag(Z->flags, XEO_INRETURN);
		}
	}
	if ( (UTCHAR)*oldsrc > ' ' ){
		useopt = TRUE;
		oldsrc++;
	}
											// 最大長を決める
	maxlen = GetLongParamMaxLen(Z) - (1 + 3);
	// 後段の文字列が残るように末尾を決定
	ptr = Z->src;
	while ( ((UTCHAR)(*ptr) >= ' ') || ((*ptr != '\n') && (*ptr == '\r')) ){ // 改行以外
		if ( *ptr == '%' ){
			if ( *(ptr + 1) == '%' ){
				ptr++;
			}else if ( *(ptr + 1) == ':' ){
				break; // コマンド区切り
			}else {
				DWORD dstlength;

				maxlen -= 100; // 仮の確保幅
				dstlength = Z->dst - Z->DstBuf;
				if ( maxlen < dstlength ){
					maxlen = dstlength;
					break;
				}
			}
		}
		ptr++;
		maxlen--;
	}
	setflag(Z->status, ST_CHKSDIRREF);
	for ( ; ; ){
		size_t len;
		size_t dstlen;

		Z->src = oldsrc;
		if ( IsTrue(useopt) ){
			if ( (*(oldsrc - 1) == 'F') &&
				 PPxEnumInfoFunc(Z->Info, PPXCMDID_ENUMATTR, buf, &Z->EnumInfo) &&
				 (*(DWORD *)buf & FILE_ATTRIBUTE_DIRECTORY) ){
				setflag(Z->status, ST_SDIRREF);
			}
			ZGetName(Z, buf, '\0'); // ST_SDIRREF 判定は、%F のときにはない
			if ( Z->func.quotation != FALSE ) tstrreplace(buf, T("\""), T("\"\""));
		}else{
			ZGetName(Z, buf, 'C'); // ST_SDIRREF 判定内蔵
		}

		len = tstrlen32(buf);
		dstlen = (Z->dst - Z->DstBuf) + len;
		if ( (Z->ExtendDst.top + dstlen ) >= maxlen ) break; // 大きさ越える
		if ( dstlen >= CMDLINESIZE ){
			if ( StoreLongParam(Z, 0) == FALSE ) break; // これ以上展開しない
		}
		if ( (buf[0] != '\"') && (tstrchr(buf, separator)) != NULL ){
			Z->dst = thprintf(Z->dst, CMDLINESIZE, T("\"%s\""), buf);
		}else{
			tstrcpy(Z->dst, buf);
			Z->dst += len;
		}
		if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_NEXTENUM, buf, &Z->EnumInfo) == 0 ){
			setflag(Z->status, ST_LOOPEND);
			break;
		}
		if ( --maxentries == 0 ) break;
		if ( separator == '\n' ){
			*Z->dst++ = '\r';
			*Z->dst++ = '\n';
		}else{
			*Z->dst++ = separator;
		}
		*Z->dst = '\0';
	}
	setflag(Z->status, ST_NO_NEXTENUM);
}

DWORD GetCacheHash(EXECSTRUCT *Z)
{
	const TCHAR *plast, *srcptr;
	DWORD hash = 0;

	srcptr = Z->edit.cache.srcptr;
	plast = Z->src;
	if ( (plast - srcptr) > CMDLINESIZE ){
		size_t len = tstrlen(srcptr);
		if ( len < (size_t)(plast - srcptr) ){
			plast = srcptr + len;
		}
	}
	for ( ; srcptr < plast ; srcptr++ ) hash += *srcptr;
	return hash;
}

// 部分編集を行う
BOOL EditExtractText(EXECSTRUCT *Z)
{
	TINPUT tinput;
	DWORD hash C4701CHECK;

	if ( Z->ExtendDst.top == 0 ){
		*Z->dst = '\0';
		tinput.buff = Z->DstBuf + Z->edit.EdOffset;
		tinput.size = CMDLINESIZE - ToSIZE32_T(Z->dst - Z->DstBuf) - 1;
	}else{
		if ( StoreLongParam(Z, 0) == FALSE ) return FALSE;
		tinput.size = Z->ExtendDst.size / sizeof(TCHAR) - Z->edit.EdOffset;
		if ( tinput.size >= 0x8000 ) tinput.size = 0x7fff;
		tinput.buff = (TCHAR *)Z->ExtendDst.bottom + Z->edit.EdOffset;
	}

	if ( (Z->status & ST_USECACHE) &&
		 (Z->edit.cache.hash == (hash = GetCacheHash(Z))) ){
		// キャッシュが使用できる
		ThGetString(&Z->StringVariable, EditCache_ValueName, tinput.buff, tinput.size);
	}else{
		tinput.title	= ZGetTitleName(Z);
		tinput.flag		= TIEX_USESELECT | TIEX_USEINFO;
		tinput.firstC	= Z->edit.CsrStart;
		tinput.lastC	= Z->edit.CsrEnd;
		if ( ZTinput(Z, &tinput) == FALSE ) return FALSE;
		// 1.79+1 で ST_PATHFIX と ST_PATHFIX / ST_PATHITEM に分離してみた
		if ( Z->status & ST_PATHFIX ){
			VFSFixPath(NULL, tinput.buff, GetZCurDir(Z), VFSFIX_VFPS);
		}

		if ( Z->status & ST_USECACHE ){
			Z->edit.cache.hash = hash; // C4701ok
			ThSetString(&Z->StringVariable, EditCache_ValueName, tinput.buff);
		}
	}
	if ( Z->ExtendDst.top != 0 ){
		Z->ExtendDst.top = (Z->edit.EdOffset + tstrlen(tinput.buff)) * sizeof(TCHAR);
	}else{
		Z->dst = tinput.buff + tstrlen(tinput.buff);
	}
	resetflag(Z->status, ST_USECACHE);
	Z->edit.EdPartStart = GetDestOffset(Z);
	Z->edit.CsrStart = 0;
	Z->edit.CsrEnd = -1;
	return TRUE;
}
