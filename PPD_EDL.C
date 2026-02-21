/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library					拡張エディット
-----------------------------------------------------------------------------*/
#pragma setlocale("Japanese")
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include <richedit.h>
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#define PPEFINDREPLACESTRUCT
#include "PPD_EDL.H"
#include "VFS_STRU.H"
#pragma hdrstop

#ifdef WINEGCC
	#undef FINDMSGSTRING // Wine 5.0 でおかしい
	#define FINDMSGSTRING  T("commdlg_FindReplace")
#endif

#ifndef CFM_BACKCOLOR
  #define CFM_SMALLCAPS	0x0040
  #define CFM_ALLCAPS	0x0080
  #define CFM_HIDDEN	0x0100
  #define CFM_OUTLINE	0x0200
  #define CFM_SHADOW	0x0400
  #define CFM_EMBOSS	0x0800
  #define CFM_IMPRINT	0x1000
  #define CFM_DISABLED	0x2000
  #define CFM_REVISED	0x4000

  #define CFM_BACKCOLOR 0x04000000
  #define CFM_LCID		0x02000000
  #define CFM_UNDERLINETYPE	0x00800000
  #define CFM_WEIGHT	0x00400000
  #define CFM_SPACING	0x00200000
  #define CFM_KERNING	0x00100000
  #define CFM_STYLE		0x00080000
  #define CFM_ANIMATION	0x00040000
  #define CFM_REVAUTHOR	0x00008000

  #define CFE_LINK		0x0020
  #define CFE_DISABLED	CFM_DISABLED
  #define CFE_SUBSCRIPT	0x00010000
  #define CFE_SUPERSCRIPT	0x00020000

  #define CFE_AUTOBACKCOLOR	CFM_BACKCOLOR

  #define CFM_EFFECTS2 (CFM_EFFECTS | CFM_DISABLED | CFM_SMALLCAPS | CFM_ALLCAPS \
					| CFM_HIDDEN  | CFM_OUTLINE | CFM_SHADOW | CFM_EMBOSS \
					| CFM_IMPRINT | CFM_DISABLED | CFM_REVISED \
					| CFM_SUBSCRIPT | CFM_SUPERSCRIPT | CFM_BACKCOLOR)
  #define CFM_ALL (CFM_EFFECTS | CFM_SIZE | CFM_FACE | CFM_OFFSET | CFM_CHARSET)
  #define CFM_ALL2	 (CFM_ALL | CFM_EFFECTS2 | CFM_BACKCOLOR | CFM_LCID \
					| CFM_UNDERLINETYPE | CFM_WEIGHT | CFM_REVAUTHOR \
					| CFM_SPACING | CFM_KERNING | CFM_STYLE | CFM_ANIMATION)

  #define PFM_EFFECTS (PFM_RTLPARA | PFM_KEEP | PFM_KEEPNEXT | PFM_TABLE \
					| PFM_PAGEBREAKBEFORE | PFM_NOLINENUMBER  \
					| PFM_NOWIDOWCONTROL | PFM_DONOTHYPHEN | PFM_SIDEBYSIDE \
					| PFM_TABLE)
  #define PFM_ALL (PFM_STARTINDENT | PFM_RIGHTINDENT | PFM_OFFSET	| \
					 PFM_ALIGNMENT   | PFM_TABSTOPS    | PFM_NUMBERING | \
					 PFM_OFFSETINDENT| PFM_RTLPARA)
  #define PFM_ALL2	(PFM_ALL | PFM_EFFECTS | PFM_SPACEBEFORE | PFM_SPACEAFTER \
					| PFM_LINESPACING | PFM_STYLE | PFM_SHADING | PFM_BORDER \
					| PFM_NUMBERINGTAB | PFM_NUMBERINGSTART | PFM_NUMBERINGSTYLE)
#endif

#ifndef IMF_DUALFONT
  #define IMF_DUALFONT  0x0080
#endif

#ifndef SPF_SETDEFAULT
  #define SPF_SETDEFAULT  0x0004
  #define EM_SETEDITSTYLE  (WM_USER + 204)
#endif
#ifndef SES_EXTENDBACKCOLOR
  #define SES_EXTENDBACKCOLOR  4
#endif
#define EM_SHOWSCROLLBAR (WM_USER + 96)
#define ES_DISABLENOSCROLL 0x00002000

#ifndef ST_UNICODE
#define ST_UNICODE 8
#endif

#ifndef ST_DEFAULT
#define ST_DEFAULT 0
#define ST_KEEPUNDO 1
#define ST_SELECTION 2
#define ST_NEWCHARS 4

typedef struct
{
	DWORD flags;
	UINT codepage;
} SETTEXTEX;

#define GT_SELECTION	2
#define EM_SETTEXTEX			(WM_USER + 97)
#endif
#ifndef GT_NOHIDDENTEXT
#define GT_NOHIDDENTEXT 8
#endif

#define TwipToPixel(twip, dpi) (((twip) * 1440) / (dpi))
#define PixelToTwip(twip, dpi) (((twip) * (dpi)) / 1440)

const TCHAR StrPPxED[] = T("PPxExEdit");
const TCHAR StrReload[] = MES_QRLT;

#define INMENU_BREAK B31 // 次の段へ

#define INUM_SEL_START 4
#define INUM_SEL_END 5

static const IID xIID_ITextDocument = {0x8CC497C0, 0xA1DF, 0x11CE, {0x80,0x98,0x00,0xAA,0x00,0x47,0xBE,0x5D} };

int Cursor_TopOfWindow = -13;
int Cursor_EndOfWindow = -14;
int Cursor_TopOfText = -15;
int Cursor_EndOfText = -16;
int Cursor_LogUp[] = {-18, 0, -1, 0, 0, 0};
int Cursor_LogDown[] = {-18, 0, 1, 0, 0, 0};

#ifndef UNICODE
int xpbug = 0;		// XP で EM_?ETSEL が WIDE に必ずなる bug なら負
#endif
int USEFASTCALL PPeFill(PPxEDSTRUCT *PES);
int USEFASTCALL PPeDefaultMenu(PPxEDSTRUCT *PES);
ERRORCODE PPedCommand(PPxEDSTRUCT *PES, PPECOMMANDPARAM *param);
ERRORCODE PPedExtCommand(PPxEDSTRUCT *PES, PPECOMMANDPARAM *param, const TCHAR *command);
void InitExEdit(PPxEDSTRUCT *PES);
int X_esrx = 0; // 検索で正規表現を使う

const TCHAR FINDMSGSTRINGstr[] = FINDMSGSTRING;
UINT ReplaceDialogMessage = 0xff77ff77;

const TCHAR CRCD_CR[] = T("CR");
const TCHAR CRCD_CRLF[] = T("CRLF");
const TCHAR CRCD_LF[] = T("LF");
const TCHAR *CRCD_list[] = {CRCD_CRLF, CRCD_CR, CRCD_LF};

typedef struct {
	TCHAR *text;
	TCHAR textbuf[0x410];
	int len;
} EditTextStruct;


#define SYNTAXCOLOR_NOCOLOR SYNTAXCOLOR_count

EDITCOLORS SynColor[SYNTAXCOLOR_count] = { DEFAULT_EDITCOLORS };

void InitSynColor(PPxEDSTRUCT *PES)
{
	COLORREF *col = &SynColor[0].text;
	int i;

	setflag(PES->flags, PPXEDIT_SYNTAXCOLOR);
	PES->SyntaxType = PES->list.WhistID;

	if ( col[1] == (C_S_AUTO - 1) ){
		col[1] = C_AUTO;
		GetCustData(T("C_csyh"), col, sizeof(SynColor));
		for ( i = 0; i < (SYNTAXCOLOR_count * 2); i++, col++){
			*col = GetSchemeColor(*col, C_S_AUTO);
		}
	}
}

void RichChangeSynColor(PPxEDSTRUCT *PES, EDITCOLORS *ec)
{
	CHARFORMAT2 chfmt;

	if ( (ec->text >= C_S_AUTO) && (ec->back >= C_S_AUTO) ) return;

	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = 0;
	chfmt.dwEffects = 0;
	if ( ec->text < C_S_AUTO ){
		setflag(chfmt.dwMask, CFM_COLOR);
		chfmt.crTextColor = ec->text;
	}
	if ( ec->back < C_S_AUTO ){
		setflag(chfmt.dwMask, CFM_BACKCOLOR);
		chfmt.crBackColor = ec->back;
	}
	SendMessage(PES->hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&chfmt);
}

void RichChangeFontStyle(PPxEDSTRUCT *PES, DWORD mask, DWORD effect)
{
	CHARFORMAT chfmt;

	if ( !(PES->flags & PPXEDIT_RICHEDIT) ) return;

	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = mask;
	SendMessage(PES->hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&chfmt);
	chfmt.dwEffects ^= effect;
	SendMessage(PES->hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&chfmt);
}

void RichChangeFontColor(PPxEDSTRUCT *PES)
{
	CHARFORMAT chfmt;

	if ( !(PES->flags & PPXEDIT_RICHEDIT) ) return;

	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = CFM_COLOR;
	SendMessage(PES->hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&chfmt);
	if ( (chfmt.dwEffects & CFE_AUTOCOLOR) || (chfmt.crTextColor == C_WindowText) ){
		resetflag(chfmt.dwEffects, CFE_AUTOCOLOR);
		chfmt.crTextColor = C_RED;
	}else{
		chfmt.crTextColor = C_WindowText;
	}
	SendMessage(PES->hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&chfmt);
}

void RichChangeBackColor(PPxEDSTRUCT *PES)
{
	CHARFORMAT2 chfmt;

	if ( !(PES->flags & PPXEDIT_RICHEDIT) ) return;

	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = CFM_BACKCOLOR;
	SendMessage(PES->hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&chfmt);
	resetflag(chfmt.dwEffects, CFE_AUTOBACKCOLOR);
	if ( chfmt.crBackColor != C_YELLOW ){
		chfmt.crBackColor = C_YELLOW;
	}else{
		chfmt.crBackColor = C_WindowBack;
	}
	chfmt.dwMask = CFM_BACKCOLOR;
	SendMessage(PES->hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&chfmt);
}

void SetSelectCombbox(HWND hEditWnd)
{
	TCHAR textbuf[CMDLINESIZE];
	HWND hComboWnd;

	hComboWnd = GetParent(hEditWnd);
	GetWindowText(hEditWnd, textbuf, CMDLINESIZE);
	textbuf[CMDLINESIZE - 1] = '\0';
	SendMessage(hComboWnd, CB_DELETESTRING, 0, 0);
	SetWindowTextWithSelect(hEditWnd, textbuf);
	GetWindowText(hEditWnd, textbuf, CMDLINESIZE);
	SendMessage(hComboWnd, CB_INSERTSTRING, 0, (LPARAM)textbuf);
}

BOOL OpenEditText(PPxEDSTRUCT *PES, EditTextStruct *ets, int margin)
{
	int sizelen;

	// GetWindowTextLength は内部で WM_GETTEXTLENGTH を使用しているが、
	// 同一プロセス内特化なので早い。CallWindowProc なら更に早い
	// GetWindowText, SetWindowTextも同様
	sizelen = Edit_GetWindowTextLength(PES) + margin + 1;
	if ( sizelen <= TSIZEOF(ets->textbuf) ){
		ets->text = ets->textbuf;
		ets->len = Edit_GetWindowText(PES, ets->text, sizelen);
		return TRUE;
	}else{
		ets->text = (TCHAR *)HeapAlloc(DLLheap, 0, TSTROFF(sizelen));
		if ( ets->text == NULL ){
			ets->text = ets->textbuf;
			ets->len = 0;
			ets->textbuf[0] = '\0';
			return FALSE;
		}
		ets->len = Edit_GetWindowText(PES, ets->text, sizelen);
		if ( ets->len > 0 ) return TRUE;
		HeapFree(DLLheap, 0, ets->text);
		ets->text = ets->textbuf;
		ets->textbuf[0] = '\0';
		return FALSE;
	}
}

void CloseEditText(EditTextStruct *ets)
{
	if ( ets->text == ets->textbuf ) return;
	HeapFree(DLLheap, 0, ets->text);
}

void RichSyntaxColor(PPxEDSTRUCT *PES)
{
	CHARFORMAT2 chfmt;
	ECURSOR cursor;
	EditTextStruct ets;
	BYTE *etsattr;
	BYTE *attrptr, *ptr, nowattr;
	int width;

	if ( !(PES->flags & PPXEDIT_RICHEDIT) ) return;
	if ( OpenEditText(PES, &ets, 0) == FALSE ) return;
	if ( ets.len == 0 ) return; // Close 不要
	etsattr = (BYTE *)HeapAlloc(DLLheap, 0, ets.len + 1);
	memset(etsattr, SYNTAXCOLOR_NOCOLOR, ets.len + 1);

	GetEditSel(PES->hWnd, ets.text, &cursor);
	tstrreplace(ets.text, T("\r\n"), T("\n"));

	SendMessage(PES->hWnd, WM_SETREDRAW, FALSE, 0);
	if ( PES->RichEdit != NULL ) RichEdit_Suspend_Undo(PES->RichEdit);

	// 一度色を戻す
	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = CFM_COLOR | CFM_BACKCOLOR;
	chfmt.dwEffects = 0;
	chfmt.crTextColor = C_WindowText;
	chfmt.crBackColor = C_WindowBack;
	SendMessage(PES->hWnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&chfmt);

	CommandSyntaxColor((WORD)PES->SyntaxType, ets.text, etsattr);

	// 色を反映させる
	width = ets.len;
	attrptr = etsattr;
	nowattr = *attrptr;
	ptr = attrptr + 1;
	for (;;){
		if ( (width == 0) || (nowattr != *ptr) ){
			if ( attrptr < ptr ){
				if ( nowattr < SYNTAXCOLOR_NOCOLOR ){
					SetEditSel(PES->hWnd, ets.text, ptr - etsattr, attrptr - etsattr);
					RichChangeSynColor(PES, SynColor + nowattr);
				}
			}
			if ( width == 0 ) break;
			attrptr = ptr;
			nowattr = *ptr;
		}
		ptr++;
		width--;
	}

	// 検索ハイライト
	if ( (PES->findrep != NULL) && (PES->findrep->findtext[0] != '\0') ){
		TCHAR *fptr, *nptr;
		int offset, len;

		len = tstrlen(PES->findrep->findtext);
		fptr = ets.text;
		for (;;){
			nptr = tstristr(fptr, PES->findrep->findtext);
			if ( nptr == NULL ) break;
			offset = nptr - ets.text;
			SetEditSel(PES->hWnd, ets.text, offset, offset + len);
			RichChangeSynColor(PES, &SynColor[SYNTAXCOLOR_keyword]);
			fptr = nptr + 1;
		}
	}

	HeapFree(DLLheap, 0, etsattr);
	CloseEditText(&ets);
	SetEditSel(PES->hWnd, ets.text, cursor.start, cursor.end);
	if ( PES->RichEdit != NULL ) RichEdit_Resume_Undo(PES->RichEdit);
	SendMessage(PES->hWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(PES->hWnd, NULL, FALSE);
}

void SetRichSyntaxColor(PPxEDSTRUCT *PES)
{
	if ( !(PES->flags & PPXEDIT_RICHEDIT) ) return;
	PES->flags ^= PPXEDIT_SYNTAXCOLOR;
	if ( PES->flags & PPXEDIT_SYNTAXCOLOR ){
		X_csyh = 1;
		InitSynColor(PES);
		RichSyntaxColor(PES);
	}else{
		X_csyh = 0;
	}
}


// 文字オフセットがカーソルであるかを判定。※フォーカスが無いときは失敗する
BOOL IsEditCursorPos(PPxEDSTRUCT *PES, DWORD coff)
{
	POINT pos;
	DWORD coffset;

	GetCaretPos(&pos); // DPI仮想化未対応

	if ( !(PES->flags & PPXEDIT_RICHEDIT) || (OSver.dwMajorVersion < 5) ){
		coffset = (DWORD)SendMessage(PES->hWnd, EM_CHARFROMPOS, 0, TMAKELPARAM(pos.x, pos.y));
	}else{
		coffset = (DWORD)SendMessage(PES->hWnd, EM_CHARFROMPOS, 0, (LPARAM)&pos);
	}

	if ( LOWORD(coffset) != LOWORD(coff) ) return FALSE; // x下位が一致しない
	if ( HIWORD(coff) == 0 ) return TRUE; // x下位だけで判定確定

	// y下位が一致(EM_LINEFROMCHAR が1余分にある時もある)
	return (DWORD)(LOWORD(SendMessage(PES->hWnd, EM_LINEFROMCHAR, coff, 0)) - HIWORD(coffset)) <= 1;
#if 0
	POINT pos;
	DWORD wpos;

	GetCaretPos(&pos);
	wpos = SendMessage(PES->hWnd, EM_POSFROMCHAR, (WPARAM)coff, 0);

	return (LOWORD(pos.x) == LOWORD(wpos)) && (LOWORD(pos.y) == HIWORD(wpos));
#endif
}

int GetSelAndGetCursorLeftSel(PPxEDSTRUCT *PES, int *inums)
{
	SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&inums[INUM_SEL_START], (LPARAM)&inums[INUM_SEL_END]);
	if ( inums[INUM_SEL_START] == inums[INUM_SEL_END] ) return INUM_SEL_START; // 選択無し
	switch (inums[3]){ // カーソル位置による
		case 1: // 左
			return INUM_SEL_START;
		case 2: // 右
			return INUM_SEL_END;
		case 3: // 移動方向による 左に移動なら左
			return (inums[1] <= 0) ? INUM_SEL_START : INUM_SEL_END;
		default: // case 0: カーソル位置
			return IsEditCursorPos(PES, (DWORD)inums[INUM_SEL_START]) ? INUM_SEL_START : INUM_SEL_END;
	}
}

// Y 移動量に相当する文字オフセットを求める
int CursorYMove(HWND hWnd, int deltaY, int nowW)
{
	int nowY, newY;

	if ( deltaY == 0 ) return 0;
	nowY = (nowW != 0) ? (int)SendMessage(hWnd, EM_LINEFROMCHAR, (WPARAM)nowW, 0) : 0;
	newY = nowY + deltaY;
	if ( newY < 0 ) newY = 0;
	return	(int)SendMessage(hWnd, EM_LINEINDEX, (WPARAM)newY, 0) -
			((nowW != 0) ? (int)SendMessage(hWnd, EM_LINEINDEX, (WPARAM)nowY, 0) : 0);
}

int FixCursorXmove(HWND hWnd, int start, int offset)
{
	int nowpos = start;
	const WCHAR *EditText;

#ifdef UNICODE
	BOOL uselocalheap = FALSE;
	HLOCAL hEd;

	EditText = NULL;
	hEd = (HLOCAL)SendMessage(hWnd, EM_GETHANDLE, 0, 0);
	if ( hEd != NULL ){
		EditText = LocalLock(hEd);
		if ( EditText != NULL ) uselocalheap = TRUE;
	}
	if ( EditText == NULL )
#endif
	{
		DWORD len;

		len = GetWindowTextLength(hWnd);
		EditText = HeapAlloc(DLLheap, 0, TSTROFF(len + 1));
		GetWindowText(hWnd, (TCHAR *)EditText, len + 1);
	}
	if ( EditText == NULL ){
		nowpos += offset;
		if ( nowpos < 0 ) nowpos = 0;
#ifndef UNICODE
	}else if ( xpbug >= 0 ){ // ANSI版
		if ( xpbug ) CaretFixToA((const char *)EditText, (DWORD *)&nowpos);
		if ( offset >= 0 ){ // 正
			for (;offset > 0;){
				BYTE c;

				c = (BYTE)*((char *)EditText + nowpos);
				if ( c == '\0' ) break;
				if ( c == '\r' ){
					nowpos++;
					continue;
				}
				if ( IskanjiA(c) ){
					nowpos++;
					c = (BYTE)*((char *)EditText + nowpos);
					if ( c == '\0' ) break;
				}
				nowpos++;
				offset--;
			}
		}else for (;offset < 0;){ // 負
			BYTE c;

			if ( nowpos == 0 ) break;
			if ( nowpos > 1 ){
				c = (BYTE)*((char *)EditText + nowpos - 2);
				if ( c == '\r' ){
					nowpos--;
					continue;
				}
				if ( IskanjiA(c) ){
					nowpos--;
				}
			}
			nowpos--;
			offset++;
		}
		if ( xpbug ) CaretFixToW((const char *)EditText, (DWORD *)&nowpos);
#endif
	// UNICODE 版
	}else if ( offset >= 0 ){ // 正
		for (;offset > 0;){
			WCHAR c, c2;

			c = *(EditText + nowpos);
			if ( c == '\0' ) break;
			c2 = *(EditText + nowpos + 1);
			if ( (c == '\r') ||
				 ((c >= 0xd800) && (c < 0xdc00)) || // サロゲートペア1
				 ((c >= 0x180b) && (c <= 0x180d)) || // FVS
				 ((c >= 0xfe00) && (c <= 0xfe0f)) || // VS1-VS16
				// U+E0100?U+E01EF // VS17-VS256
//				 ((c2 >= 0x93a) && (c2 <= 0x94f)) || //
				 ((c2 >= 0x300) && (c2 <= 0x36f))  // ダイアクリティカル
				){
				nowpos++;
				continue;
			}
			nowpos++;
			offset--;
		}
	}else for (;offset < 0;){ // 負
		WCHAR c, c2;

		if ( nowpos == 0 ) break;
		if ( nowpos > 1 ){
			c = *(EditText + nowpos - 2);
			c2 = *(EditText + nowpos - 1);
			if ((c == '\r') ||
				((c >= 0xd800) && (c < 0xdc00)) || // サロゲートペア
				((c >= 0x180b) && (c <= 0x180d)) || // FVS
				((c >= 0xfe00) && (c <= 0xfe0f)) || // VS1-VS16
				// U+E0100?U+E01EF // VS17-VS256
//				 ((c2 >= 0x93a) && (c2 <= 0x94f)) || //
				 ((c2 >= 0x300) && (c2 <= 0x36f))  // ダイアクリティカル
				){
				nowpos--;
				continue;
			}
		}
		nowpos--;
		offset++;
	}
#ifdef UNICODE
	if ( uselocalheap != FALSE ){
		LocalUnlock(hEd);
	}else
#endif
	if ( EditText != NULL ){
		HeapFree(DLLheap, 0, (void *)EditText);
	}
	return nowpos;
}

int USEFASTCALL PPeSelectExtensionMain(PPxEDSTRUCT *PES, int mode)
{
	ECURSOR cursor, cursorRange;
	TCHAR *p;
	DWORD extoffset;
	HWND hWnd = PES->hWnd;

	EditTextStruct ets;

	if ( OpenEditText(PES, &ets, 0) == FALSE ) return 0;
	if ( ets.len == 0 ) return FALSE; // len == 0 の時はClose不要
												// 編集文字列(全て)を取得
	GetEditSel(hWnd, ets.text, &cursor);
	if ( PES->flags & PPXEDIT_RICHEDIT ){
		tstrreplace(ets.text, T("\r\n"), T("\n"));
	}
	if ( PES->flags & PPXEDIT_SINGLEREF ){	// １項目のみ
		cursorRange.start = 0;
		cursorRange.end = ets.len;
	}else{	// 複数項目
		cursorRange = cursor;
		GetWordStrings(ets.text, &cursorRange, GWSF_SPLIT_PARAM);
	}
	// 拡張子の位置を確定
	p = FindLastEntryPoint(ets.text + cursorRange.start);
	cursorRange.start = p - ets.text;
	p += FindExtSeparator(p);
	extoffset = p - ets.text;

	if ( mode < -7 ){
		if ( mode == -8 ){ // 名前のみ
			cursor.start = cursor.end;
		}else{  // if ( mode == -9 ) // 拡張子のみ
			cursor.start = cursorRange.start;
			cursor.end = extoffset;
		}
	}

	if ( cursor.start == cursor.end ){ // 未選択→名前指定
		cursor.start = cursorRange.start;
		cursor.end = extoffset;
	}else if ( cursor.start == cursorRange.start ){ //全範囲指定 or 名前指定
		if ( cursor.end > extoffset ){ // 全範囲指定→名前指定
			cursor.end = extoffset;
		}else{	// 名前指定→拡張子指定
			if ( ets.text[extoffset] == '.' ) extoffset++;
			cursor.start = extoffset;
			cursor.end = cursorRange.end;
		}
	}else{ // その他→全範囲指定
		cursor.start = cursorRange.start;
		cursor.end = cursorRange.end;
	}
	SetEditSel(hWnd, ets.text, cursor.start, cursor.end);
	CloseEditText(&ets);
	return 0;
}

int USEFASTCALL PPeSelectExtension(PPxEDSTRUCT *PES)
{
	return PPeSelectExtensionMain(PES, 0);
}

int GetPageInfo(PPxEDSTRUCT *PES, BOOL getheight)
{
	RECT box;

	CallWindowProc(PES->hOldED, PES->hWnd, EM_GETRECT, 0, (LPARAM)&box);
	if ( PES->exstyle & WS_EX_CLIENTEDGE ){
		box.top++;
		box.left++;
	}
	#pragma warning(suppress:6001) // 異常でも無視できる
	return getheight ?
			((box.bottom - box.top) / PES->fontY) :
			((box.right - box.left) / PES->fontX);
}

BOOL USEFASTCALL PPeLogicalLinePos(PPxEDSTRUCT *PES, EditLPos *elp)
{
	DWORD len;
	EditTextStruct ets;

	TCHAR *textptr; // 参照位置
	BOOL rely = FALSE;
	int curindex;

	if ( OpenEditText(PES, &ets, 0) == FALSE ) return FALSE;
	len = ets.len;
	if ( len == 0 ) return FALSE; // len == 0 の時はClose不要
	if ( PES->flags & PPXEDIT_RICHEDIT ){
		tstrreplace(ets.text, T("\r\n"), T("\n"));
	}

	textptr = ets.text;

	if ( elp->mode & EditLPos_SET_RANGE ){
		curindex = GetSelAndGetCursorLeftSel(PES, elp->inums);
		if ( curindex == INUM_SEL_END ) elp->mode ^= EditLPos_CURSOR_ENDSEL;
	}

	if ( elp->mode & EditLPos_RELCURSOR ){
		TCHAR *line; // 行頭
		int y = 0;
		DWORD posC, firstC, lastC;

		// 現在カーソルの x,y を求める
		SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&firstC, (LPARAM)&lastC);
		if ( elp->mode & EditLPos_CURSOR_ENDSEL ) firstC = lastC;
		#ifndef UNICODE
			if ( xpbug < 0 ) CaretFixToA(ets.text, &firstC);
		#endif
		line = textptr;
		posC = 0;
		for(;;){
			TCHAR chr;

			if ( posC >= firstC ) break;
			chr = *textptr;
			if ( chr == '\0' ) break;
			textptr++;
			posC++;
			if ( chr == '\r' ){
				if ( *textptr == '\n' ){
					textptr++;
					posC++;
				}
				y++;
				line = textptr;
				continue;
			}
			if ( chr == '\n' ){
				y++;
				line = textptr;
				continue;
			}
		}
		if ( elp->mode & EditLPos_SET_POS ){
			elp->x += textptr - line;
			elp->y += y;
			if ( elp->y != 0 ) rely = TRUE;
		}else if ( elp->mode & EditLPos_GET_L_CURSORPOS ){
			elp->x = textptr - line;
			elp->y = y;
		}
//		XMessage(NULL, NULL, XM_DbgDIA, T("> %d %d"),elp->x,elp->y);
	}

	if ( elp->mode & EditLPos_COUNT_POS ){
		int y = 0;

		textptr = ets.text;
		for(;;){
			TCHAR chr;

			if ( y >= elp->y ){
//				XMessage(NULL, NULL, XM_DbgDIA, T("f %d %d"),y,textptr - ets.text);
				// ●文字単位化済んでいない
				if ( elp->mode & EditLPos_SET ){
					DWORD lp;

					if ( rely ){
						int offx;

						offx = elp->x;
						for(; offx > 0; offx--, textptr++ ){
							TCHAR chr;

							chr = *textptr;
							if ( (chr == '\0') || (chr == '\r') || (chr == '\n') ){
								break;
							}
						}
					}else{
						textptr += elp->x;
					}
					lp = textptr - ets.text;
					#ifndef UNICODE
						if ( xpbug < 0 ) CaretFixToW(ets.text, &lp);
					#endif

					if ( elp->mode & EditLPos_SET_RANGE ){
						elp->inums[curindex] = lp;
						SendMessage(PES->hWnd, EM_SETSEL, (WPARAM)elp->inums[curindex ^ 1 /* 4/5 → 5/4 */], (LPARAM)elp->inums[curindex]);
					}else{
						SendMessage(PES->hWnd, EM_SETSEL, (WPARAM)lp, (LPARAM)lp);
					}
				}
				break;
			}

			chr = *textptr;
			if ( chr == '\0' ) break;
			textptr++;
			if ( chr == '\r' ){
				if ( *textptr == '\n' ){
					textptr++;
				}
				y++;
				continue;
			}
			if ( chr == '\n' ){
				y++;
				continue;
			}
		}
		if ( elp->mode & EditLPos_SET_POS ){
			elp->y = y;
		}
	}

	if ( elp->mode & EditLPos_GET_TEXT ){
		elp->textbuf = ets.text;
		elp->textptr = textptr;
	}else{
		CloseEditText(&ets);
	}
	return TRUE;
}

void PPeMoveLogicalLine(PPxEDSTRUCT *PES, int *inums, int mode)
{
	EditLPos elp = { NULL, NULL, 0, 0, 0, NULL};

	elp.mode = mode;
	elp.x = inums[1];
	elp.y = inums[2];
	elp.inums = inums;
	if ( inums[0] == -17 ){
		elp.x = (elp.x > 0) ? (elp.x - 1) : 0;
		elp.y--;
	}
	PPeLogicalLinePos(PES, &elp);
	Edit_SendMessageMsg(PES, EM_SCROLLCARET);
}

int PPeGetLogicalPos(PPxEDSTRUCT *PES, int mode)
{
	EditLPos elp = { NULL, NULL, EditLPos_GET_CURSORPOS, 0, 0, NULL};

	if ( mode >= 4 ){
		elp.mode = EditLPos_COUNT_POS | EditLPos_SET_POS;
		elp.y = 0x7fffffff;
		mode = 1;
	}else if ( mode >= 2 ){
		elp.mode = EditLPos_GET_CURSORPOS | EditLPos_CURSOR_ENDSEL;
		mode -= 2;
	}
	if ( PPeLogicalLinePos(PES, &elp) == FALSE ) return 0;
	return mode ? elp.y : elp.x;
}

void SendEditKey(PPxEDSTRUCT *PES, int keycode, int repeat)
{
	BYTE keytable[256], oldtable[256];

	(void)GetKeyboardState(keytable);
	memcpy(oldtable, keytable, 256);
	keytable[VK_SHIFT] = 0;
	keytable[VK_LSHIFT] = 0;
	keytable[VK_RSHIFT] = 0;
	keytable[VK_CONTROL] = 0;
	keytable[VK_LCONTROL] = 0;
	keytable[VK_RCONTROL] = 0;
	keytable[VK_MENU] = 0;
	keytable[VK_LMENU] = 0;
	keytable[VK_RMENU] = 0;
	SetKeyboardState(keytable);

	for ( ; repeat > 0; repeat-- ){
		CallWindowProc(PES->hOldED, PES->hWnd, WM_KEYDOWN, (WPARAM)keycode, 0);
		CallWindowProc(PES->hOldED, PES->hWnd, WM_KEYUP, (WPARAM)keycode, B30 | B31);
	}

	SetKeyboardState(oldtable);
}

void EditMoveCursor(PPxEDSTRUCT *PES, int keyInc, int keyDec, int delta)
{
	if ( delta < 0 ){
		keyInc = keyDec;
		delta = -delta;
	}
	SendEditKey(PES, keyInc, delta);
}

void EditMoveCursorDispRel(PPxEDSTRUCT *PES, int *inums)
{
	int curindex;
	DWORD newindex;

	curindex = GetSelAndGetCursorLeftSel(PES, inums);
	newindex = inums[curindex] + CursorYMove(PES->hWnd, inums[2], inums[curindex]);
	newindex = FixCursorXmove(PES->hWnd, newindex, inums[1]);
	if ( inums[0] == -1 ){
		inums[1] = newindex;
		if ( inums[1] == inums[curindex] ) return;
		SendMessage(PES->hWnd, EM_SETSEL, (WPARAM)inums[1], (LPARAM)inums[1]);
	}else{
		inums[curindex] = newindex;
		SendMessage(PES->hWnd, EM_SETSEL, (WPARAM)inums[curindex ^ 1 /* 4/5 → 5/4 */], (LPARAM)inums[curindex]);
	}
}

void EditSelectWord(PPxEDSTRUCT *PES, int mode)
{
	EditTextStruct ets;

	TCHAR *textbuf;
	ECURSOR cursor;
	int braket;
	DWORD len;

	// 「"」追加することあり
	if ( OpenEditText(PES, &ets, 1) == FALSE ) return;
	len = ets.len;
	if ( len == 0 ) return;
	textbuf = ets.text;

	GetEditSel(PES->hWnd, textbuf, &cursor);
	if ( PES->flags & PPXEDIT_RICHEDIT ){
		tstrreplace(ets.text, T("\r\n"), T("\n"));
	}
	braket = GetWordStrings(textbuf, &cursor, GWSF_SPLIT_PARAM);
	if ( mode == -5 ){
		if ( braket >= GWS_BRAKET_LEFT ) cursor.start--;
		if ( braket >= GWS_BRAKET_LEFTRIGHT ){
			#ifndef UNICODE
				textbuf[cursor.end] = '\"';
				textbuf[cursor.end + 1] = '\0';
			#endif
			cursor.end++;
		}
	}
	SetEditSel(PES->hWnd, textbuf, cursor.start, cursor.end);
	CloseEditText(&ets);
}

// *cursor
DWORD_PTR EditCursorCommand(PPxEDSTRUCT *PES, PPXAPPINFOUNION *uptr)
{
	switch ( uptr->inums[0] ){ // 正は親PPxの操作用に確保
		case -1: // 表示相対移動、選択解除
		case -2: // 表示相対移動、選択あり
			EditMoveCursorDispRel(PES, uptr->inums);
			break;

		case -3: { // 表示絶対移動、選択解除
			int w;

			w = uptr->nums[1] + CursorYMove(PES->hWnd, uptr->inums[2], 0);
			SendMessage(PES->hWnd, EM_SETSEL, (WPARAM)w, (LPARAM)w);
			break;
		}
		case -4: // 表示絶対移動、選択あり
			SendMessage(PES->hWnd, EM_SETSEL,
				(WPARAM)(uptr->nums[1] + CursorYMove(PES->hWnd, uptr->inums[2], 0)),
				(LPARAM)(uptr->nums[3] + CursorYMove(PES->hWnd, uptr->inums[4], 0)));
			break;

		case -5:	// カーソル位置選択(「"」含む)
		case -6:	// カーソル位置選択(「"」除く)
			EditSelectWord(PES, uptr->inums[0]);
			break;

		case -7: // カーソル位置ファイル名の選択トグル
		case -8: // カーソル位置ファイル名の名前部選択
		case -9: // カーソル位置ファイル名の拡張子部選択
			PPeSelectExtensionMain(PES, uptr->inums[0]);
			break;

		case -11: // ページ移動 / 補完一覧リストのページ移動 兼用
		case -12: // ページ移動 / 補完一覧リストのページ移動 / キーワード検索 兼用
			if ( ListPageUpDown(PES, (uptr->inums[1] >= 0) ? EDITDIST_NEXT : EDITDIST_BACK, (uptr->inums[0] == -12) ) != ERROR_INVALID_FUNCTION ){
				break;
			}
			// -10 へ
		case -10: // ページ移動
			EditMoveCursor(PES, VK_NEXT, VK_PRIOR, uptr->inums[1]);
			break;

		case -13: // Top of window
			JumptoLine(PES->hWnd,
					(int)SendMessage(PES->hWnd, EM_GETFIRSTVISIBLELINE, 0, 0) );
			break;

		case -14: // End of window
			JumptoLine(PES->hWnd,
					(int)SendMessage(PES->hWnd, EM_GETFIRSTVISIBLELINE, 0, 0) +
						GetPageInfo(PES, TRUE) - 1);
			break;

		case -15: // Top of text
			SendMessage(PES->hWnd, EM_SETSEL, 0, 0);
			SendMessage(PES->hWnd, EM_SCROLLCARET, 0, 0);
			break;

		case -16: // End of text
#ifndef UNICODE
			// Win9x は EC_LAST, EC_LAST だと、移動しない。0, EC_LAST や 0x7fff, 0x7fff だと移動する
			if ( OSver.dwPlatformId != VER_PLATFORM_WIN32_NT ){
				SendMessage(PES->hWnd, EM_SETSEL, 0x7fff, 0x7fff);
			}else // 次の SendMessage を実行
#endif
			SendMessage(PES->hWnd, EM_SETSEL, EC_LAST, EC_LAST);
			SendMessage(PES->hWnd, EM_SCROLLCARET, 0, 0);
			break;

		case -17: // 行番号移動
			PPeMoveLogicalLine(PES, uptr->inums, EditLPos_SET_POS);
			break;

		case -18: // 論理相対移動、選択解除
			PPeMoveLogicalLine(PES, uptr->inums, EditLPos_RELCURSOR | EditLPos_SET_POS);
			break;

		case -19: // 論理相対移動、選択あり
			PPeMoveLogicalLine(PES, uptr->inums, EditLPos_RELCURSOR | EditLPos_SET_POS | EditLPos_SET_RANGE);
			break;

		case -20: // 論理絶対移動、選択解除
			PPeMoveLogicalLine(PES, uptr->inums, EditLPos_SET_POS);
			break;

		case -21: // 論理絶対移動、選択あり
			PPeMoveLogicalLine(PES, uptr->inums, EditLPos_SET_POS | EditLPos_SET_POS);
			break;

		case -22: // Window Left
			SendEditKey(PES, VK_HOME, 1);
			break;

		case -23: // Window Right
			SendEditKey(PES, VK_END, 1);
			break;

		// -24 論理 home
		// -25 論理 end

		default:
			if ( (PES->info == NULL) || (PES->info->Function == NULL) ){
				return PPXA_INVALID_FUNCTION;
			}
			return PPxInfoFunc(PES->info, PPXCMDID_MOVECSR, uptr);
	}
	return PPXA_NO_ERROR;
}

void ListHScrollMain(HWND hListWnd, int dest)
{
	RECT listbox;
	int width;
//	SCROLLINFO sinfo;

	if ( hListWnd == NULL ) return;
	GetWindowRect(hListWnd, &listbox);
	listbox.right -= listbox.left;
	width = (int)SendMessage(hListWnd, LB_GETHORIZONTALEXTENT, 0, 0);
	width = width + (listbox.right * 2 * dest) / 3;
	if ( width < listbox.right ) width = listbox.right;
	SendMessage(hListWnd, LB_SETHORIZONTALEXTENT, (WPARAM)width, 0);

	SendMessage(hListWnd, WM_HSCROLL, (dest > 0) ? SB_PAGERIGHT : SB_PAGELEFT, 0);
/*
	sinfo.cbSize = sizeof(sinfo);
	sinfo.fMask = SIF_POS;
	GetScrollInfo(hListWnd, SB_HORZ, &sinfo);
	sinfo.nPos = width - listbox.right;
	SetScrollInfo(hListWnd, SB_HORZ, &sinfo, TRUE);
*/
}

void ListWidthMain(HWND hListWnd, int dest)
{
	RECT listbox;
	int width, page;

	if ( hListWnd == NULL ) return;
	GetWindowRect(hListWnd, &listbox);
	listbox.right -= listbox.left;
	listbox.bottom -= listbox.top;
	page = (int)SendMessage(hListWnd, LB_GETITEMHEIGHT, 0, 0) * 10;
	if ( dest > 1 ){
		width = dest;
	}else{
		width = listbox.right + page * dest;
	}
	if ( width < page ) width = page;
	SetWindowPos(hListWnd, NULL, 0, 0, width, listbox.bottom,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
}


void ListHScroll(PPxEDSTRUCT *PES, int dest)
{
	ListHScrollMain(PES->list.hWnd, dest);
	ListHScrollMain(PES->list.hSubWnd, dest);
}

void ListWidth(PPxEDSTRUCT *PES, int dest)
{
	ListWidthMain(PES->list.hWnd, dest);
	ListWidthMain(PES->list.hSubWnd, dest);
}

BOOL SearchStrCheck(PPxEDSTRUCT *PES, int mode)
{
	if ( (PES->findrep == NULL) || (PES->findrep->findtext[0] == '\0') ){
		return FALSE;
	}
	SearchStr(PES, mode);
	return TRUE;
}

void PPeReplaceStrCommand(PPxEDSTRUCT *PES, FINDREPLACE *freplace)
{
	DWORD firstC, lastC;

	if ( freplace->Flags & FR_FINDNEXT ){
		SearchStr(PES, (freplace->Flags & FR_DOWN) ? EDITDIST_NEXT_WITHNOW : EDITDIST_BACK );
	}else if ( freplace->Flags & FR_REPLACE ){
		SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&firstC, (LPARAM)&lastC);
		if ( firstC != lastC ){
			TEXTSEL ts;

			if ( SelectEditStrings(PES, &ts, TEXTSEL_CHAR) == FALSE ) return;

			if ( X_esrx ?
				(IsTrue(SearchStr(PES, EDITDIST_REPLACE_TEST))) :
				(tstricmp(ts.word, freplace->lpstrFindWhat) == 0) ){
				SendMessage(PES->hWnd, EM_REPLACESEL, 0, (LPARAM)freplace->lpstrReplaceWith);
			}
			FreeTextselStruct(&ts);
		}
		SearchStr(PES, EDITDIST_NEXT_WITHNOW);
	}else if ( freplace->Flags & FR_REPLACEALL ){ // はじめから最後まで
		SendMessage(PES->hWnd, WM_SETREDRAW, FALSE, 0);
		SendMessage(PES->hWnd, EM_SETSEL, 0, 0);
		for ( ;; ){
			if ( SearchStr(PES, EDITDIST_NEXT_WITHNOW) == FALSE ) break;
			SendMessage(PES->hWnd, EM_REPLACESEL, 1, (LPARAM)freplace->lpstrReplaceWith); // replace後、カーソルは置換文字列の末尾に移動する
		}
		SendMessage(PES->hWnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(PES->hWnd, NULL, FALSE);
		if ( freplace->lCustData != 0 ){
			PostMessage((HWND)freplace->lCustData, WM_CLOSE, 0, 0);
		}
	}
}

void PPeReplaceStr(PPxEDSTRUCT *PES)
{
	HMODULE hCOMDLG32;
	DefineWinAPI(HWND, ReplaceText, (LPFINDREPLACE));
	HWND hReplaceWnd;
	FINDREPLACE freplace;

	if ( PES->findrep == NULL ){
		if ( InitPPeFindReplace(PES) == FALSE ) return;
	}

	hCOMDLG32 = LoadSystemDLL(SYSTEMDLL_COMDLG32);
	if ( hCOMDLG32 == NULL ) return;
	GETDLLPROCT(hCOMDLG32, ReplaceText);
	if ( DReplaceText == NULL ) return;

	ReplaceDialogMessage = RegisterWindowMessage(FINDMSGSTRINGstr);

	freplace.lStructSize = sizeof(FINDREPLACE);
	freplace.hwndOwner = PES->hWnd;
	freplace.Flags = FR_DOWN | FR_HIDEMATCHCASE | FR_HIDEWHOLEWORD;
	#pragma warning(suppress:6001 6011) // InitPPeFindReplace で初期化
	freplace.lpstrFindWhat = PES->findrep->findtext;
	freplace.lpstrReplaceWith = PES->findrep->replacetext;
	freplace.wFindWhatLen = VFPS;
	freplace.wReplaceWithLen = VFPS;

	hReplaceWnd = DReplaceText(&freplace);
	if ( hReplaceWnd != NULL ){ // 置換ダイアログが出ている間のループ
		MSG msg;

		freplace.lCustData = (LPARAM)hReplaceWnd;
		while( (int)GetMessage(&msg, NULL, 0, 0) > 0 ){
			if ( DialogKeyProc(&msg) ) continue;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if ( !IsWindow(hReplaceWnd) || (freplace.Flags & FR_DIALOGTERM) ){
				// ダイアログを既に閉じた
				hReplaceWnd = NULL;
				break;
			}
		}
		if ( hReplaceWnd != NULL ) DestroyWindow(hReplaceWnd);
	}
	FreeLibrary(hCOMDLG32);
//	SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)T("\"\""));
}

typedef struct {
	HWND hParentWnd;
	RECT boxEdit, boxDialog, boxDesk;
	int delta;
} WindowExpandInfoStruct;

BOOL CALLBACK EnumChildExpandProc(HWND hWnd, LPARAM lParam)
{
	RECT box;
	POINT pos;

	GetWindowRect(hWnd, &box);
	if ( (box.top > ((WindowExpandInfoStruct *)lParam)->boxEdit.top) &&
		 (GetParent(hWnd) == ((WindowExpandInfoStruct *)lParam)->hParentWnd) ){
		pos.x = box.left;
		pos.y = box.top;
		ScreenToClient( ((WindowExpandInfoStruct *)lParam)->hParentWnd, &pos);

		SetWindowPos(hWnd, NULL,
				pos.x, pos.y + ((WindowExpandInfoStruct *)lParam)->delta,
				0, 0,
				SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);
	}
	return TRUE;
}

void EditBoxExpand(PPxEDSTRUCT *PES, WindowExpandInfoStruct *eis)
{
	int oldflags = PES->flags, dialogheight;

	/* ウィンドウ調整中に、再調整が発生しないよう、一時的に停止。
		※ DeferWindowPos は、親子関係のウィンドウ変更をまとめて行えない模様 */
	resetflag(PES->flags, PPXEDIT_LINE_MULTI | PPXEDIT_LINE_MULTILINE);

	// ダイアログが下にはみ出すときは、上に移動させる
	if ( (eis->boxDialog.bottom + eis->delta) >= eis->boxDesk.bottom ){
		int diff = eis->boxDialog.bottom + eis->delta - eis->boxDesk.bottom;

		if ( (eis->boxDialog.top >= eis->boxDesk.top) && ((eis->boxDialog.top - diff) < eis->boxDesk.top) ){
			diff = eis->boxDialog.top - eis->boxDesk.top;
		}
		if ( diff <= 0 ) return;
		eis->boxDialog.top -= diff;
		eis->boxDialog.bottom -= diff;

		SetWindowPos(eis->hParentWnd, NULL,
				eis->boxDialog.left, eis->boxDialog.top, 0, 0,
				SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);
	}

	dialogheight = eis->boxDialog.bottom - eis->boxDialog.top + eis->delta;
	if ( dialogheight < GetSystemMetrics(SM_CYCAPTION) ){ // ●仮調整 1.68+4
		dialogheight = GetSystemMetrics(SM_CYCAPTION) * 3;
	}

	// IDE_INPUT_LINE edit box
	SetWindowPos(PES->hWnd, NULL, 0, 0,
			eis->boxEdit.right - eis->boxEdit.left,
			eis->boxEdit.bottom,
			SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);

	// IDE_INPUT_LINE より下のコントロールを修正
	EnumChildWindows(eis->hParentWnd, EnumChildExpandProc, (LPARAM)eis);

	// Dialog box
	SetWindowPos(eis->hParentWnd, NULL, 0, 0,
			eis->boxDialog.right - eis->boxDialog.left, dialogheight,
			SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);

	if ( PES->FloatBar.hWnd != NULL ){
		SetWindowPos(PES->FloatBar.hWnd, NULL,
				eis->boxEdit.left, eis->boxEdit.top + eis->boxEdit.bottom,
				0, 0,
				SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}
	PES->flags = oldflags;
}

void ChangeFontSize(PPxEDSTRUCT *PES, int delta)
{
	HWND hWnd = PES->hWnd;
	HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	HFONT hNewFont;
	LOGFONT lfont;
	LONG oldheight;
	BOOL firstcreate = FALSE;

	GetObject(hFont, sizeof(LOGFONT), &lfont);
	if ( PES->OrgFontY == 0 ){
		PES->OrgFontY = lfont.lfHeight;
		firstcreate = TRUE;
	}
	oldheight = lfont.lfHeight;
	if ( delta == -9 ){
		lfont.lfHeight = PES->OrgFontY;
	}else{
		if ( lfont.lfHeight > 0 ){
			lfont.lfHeight += delta * 4;
			if ( lfont.lfHeight < 5 ) lfont.lfHeight = 5;
		}else{
			lfont.lfHeight -= delta * 4;
			if ( lfont.lfHeight > -5 ) lfont.lfHeight = -5;
		}
	}
	hNewFont = CreateFontIndirect(&lfont);
	SendMessage(hWnd, WM_SETFONT, (WPARAM)hNewFont, 0);
	if ( !firstcreate ) DeleteObject(hFont);

	if ( PES->flags & PPXEDIT_LINEEDIT ) {
		WindowExpandInfoStruct eis;

		eis.delta = lfont.lfHeight - oldheight;
		if ( lfont.lfHeight < 0 ) eis.delta = -eis.delta;

		eis.hParentWnd = GetParent(hWnd);
		GetDesktopRect(hWnd, &eis.boxDesk);
		GetWindowRect(hWnd, &eis.boxEdit);
		GetWindowRect(eis.hParentWnd, &eis.boxDialog);
		eis.boxEdit.bottom += -eis.boxEdit.top + eis.delta;
		EditBoxExpand(PES, &eis);
		hWnd = eis.hParentWnd;
	}
	InvalidateRect(hWnd, NULL, TRUE);

	if ( PES->flags & PPXEDIT_RICHEDIT ){ // タブの再設定が必要
		PPeSetTab(PES, PES->tab);
	}
}

PPXINMENU escmenu[] = {
	{ K_c | 'O',	MES_MCOP },
	{ K_F12,		T("&Save as...")},
//	{ KE_en,		T("&New Files")},
//	{ ,				T("&Read Files")},
//	{ K_c | 'L',	T("&Load File")}, // 差し替え読み込み
	{ KE_ea,		T("&Append to file") },
//	{ ,				T("&Path Rename")}, // filename を変更、保存はしない
	{ KE_ed,		T("&Duplicate PPe")},
//	{ KE_eu,		T("&Undo Edit")}, // 再読み込み
	{ KE_ei,		T("&Insert from file")},
//	{ ,				T("Close all(&X)")},
	{ K_c | 'Q',	MES_MEED}, // T("edit menu")},
	{ K_s | K_F2,	T("%G\"MEST|settings menu\"(&G)")},
	{PPXINMENY_SEPARATE, NULL },
	{ KE_kp	,		T("&Print by PPv")}, // 印刷
	{PPXINMENY_SEPARATE, NULL },
	{ K_s | K_F1,	T("&Help")},
	{PPXINMENY_SEPARATE, NULL },
	{ KE_er,		T("&Run as admin")},
	{ KE_ee,		T("&Exec command")},
	{ KE_ec,		MES_TABC },
//	{ KE_eq,		T("&Quit")},
	{ 0, NULL }
};

PPXINMENU f2menu[] = {
	{ KE_qj,	MES_SJLN},
//	{ KE_2b,	T("&Block Top/End")},
//	{ KE_2l,	T("Restore &Line")},
//	{ KE_2s,	T("&Screen Lines")}, // 行数変更
//	{ KE_kr,	T("&Read Only")},
	{ KE_qv,	MES_MERO}, //T("&View Mode")},
	{ KE_2t,	T("&TAB stop...")},
	{ KE_2c,	T("&Char code...")},
	{ KE_2r,	T("&Return code...")},
	{ KE_2p,	MES_MEWW}, // T("&Word wrap")},
	{ K_c | K_s | 'G',	T("Syntax highlight (&G)")}, // T("&Word wrap")},
	{ KE_2x,	MES_MERS}, // T("Regexp serach(&X)")},
//	{ KE_2a,	T("&Auto Save")},
	{ 0, NULL }
};

PPXINMENU kmenu[] = {
	{ KE_kp,	T("&Print by PPv")},
//	{ KE_ku,	T("&Undo Paste")},
//	{ KE_kc,	T("Paste(&Copy)")},
//	{ KE_kk,	T("Copy Line/Bloc&k")},
	{ KE_kd,	MES_MEDL}, // T("&Duplicate line")},
	{ KE_kz,	MES_SWCZ},
//	{ KE_kr,	T("&Read only")},
//	{ KE_kb,	T("Column &Block")},
//	{ KE_ky,	T("&Y clear stack")},
//	{ KE_k0,	T("Set Marker #&0")},
//	{ KE_k1,	T("Set Marker #&1")},
//	{ KE_k2,	T("Set Marker #&2")},
//	{ KE_k3,	T("Set Marker #&3")},
//	{ KE_k4,	T("Set Marker #&4")},
	{ 0, NULL }
};

PPXINMENU qmenu[] = {
//	{ KE_qh,	T("Cut word left(&H)")},
//	{ KE_qt,	T("Cu&t BOL")},
//	{ KE_qy,	T("Cut EOL(&Y)")},
	{ KE_qu,	T("%G\"SWCA|Word case\"(&U)")},
	{ KE_kz,	MES_SWCZ},
	{ KE_kd,	MES_MEDL}, // T("&Duplicate line")},
	{ KE_qv,	MES_MERO}, //T("&View Mode")},
//	{ KE_ql,	T("Restore &Line")},
//	{ KE_qf,	T("Set &Find string")},
//	{ KE_qo,	T("Replece Next(&O)")},
//	{ KE_qp,	T("&Put File Name")},
	{ K_c | K_s | 'I', T("%G\"VIMF|Insert selected filename...\"(&N)")},
	{ K_s | K_F7,	T("%G\"MEFS|&Insert find str\"\tShift+F7")},

	{ KE_qj,	MES_SJLN},
	{ KE_qr,	MES_SJTT}, // T("Top of text(&R)")},
	{ KE_qc,	MES_SJTE}, // T("End of text(&C)")},
//	{ KE_qp,	T("Last &Position")},
//	{ KE_qo,	T("Replace Next(&O)")},
//	{ KE_qlw,	T("Left of Window(&[)")},
//	{ KE_qrw,	T("Right of Window(&])")},
//	{ KE_qs,	T("Top of Line(&S)")},
//	{ KE_qd,	T("End of Line(&D)")},
	{ KE_qe,	MES_SJWT}, // T("Top of Window(&E)")},
	{ KE_qx,	MES_SJWE}, // T("End of Window(&X)")},
//	{ KE_qk,	T("Jump Brac&ket")},
//	{ KE_qm,	T("Set &Marker #0")},
//	{ KE_q0,	T("Jump to Marker #&0")},
//	{ KE_q1,	T("Jump to Marker #&1")},
//	{ KE_q2,	T("Jump to Marker #&2")},
//	{ KE_q3,	T("Jump to Marker #&3")},
//	{ KE_q4,	T("Jump to Marker #&4")},
//	{ KE_qb,	T("&Block Top/End")},
	{ 0, NULL }
};

//abcdefg  j lmnopq stu w  z
PPXINMENU amenu[] = {
	{ K_c | 'Z',	T("%G\"JMUN|Undo\"(&U)\tCtrl+Z")},
	{PPXINMENY_SEPARATE, NULL},
	{ K_c | 'X',	T("%G\"JMCU|Cut\"(&T)\tCtrl+X")},
	{ K_c | 'C',	T("%G\"JMCL|Copy\"(&C)\tCtrl+C")},
	{ K_c | 'V',	T("%G\"JMPA|Paste\"(&P)\tCtrl+V")},
	{ K_del,		T("%G\"JMDE|Delete\"(&D)\tDelete")},
	{PPXINMENY_SEPARATE, NULL},
	{ K_c | 'A',	T("%G\"JMAL|Select All\"(&A)\tCtrl+A")},

	{ KE_defmenu | INMENU_BREAK,	T("%G\"MEDM|Original menu\"(&B)\tShift+F10")},
	{ K_c | ']',	T("%G\"MEMD|File &menu\"\tCtrl+]")},
	{ K_c | 'Q',	T("%G\"MEED|Edit menu\"\tCtrl+&Q")},
	{ K_s | K_F2,	T("%G\"MEST|Settings menu\"(&S)\tShift+F2")},
	{PPXINMENY_SEPARATE, NULL},
	{ KE_qu,		T("%G\"SWCA|Word case transfer\"(&W)\tCtrl+Q-U")},
	{ KE_kz,		T("%G\"SWCZ|&Zen/han transfer\"\tCtrl+K-Z")},
	{ KE_er,		T("&Run as admin\tESC-R")},
//	{ KE_ee,		T("&Execute command\tESC-E")},

	{ KE_qj | INMENU_BREAK,	T("%G\"SJLN|&Jump to Line\"\tCtrl+Q-J")},
	{ K_c | 'F',	T("%G\"VCHF|Find\"(&F)\tCtrl+F")},
	{PPXINMENY_SEPARATE, NULL},
	{ K_raw | K_s | K_c | 'P', T("Path List(&N)\tCtrl+Shift+P")},
	{ K_raw | K_s | K_c | 'L', T("PPc &List\tCtrl+Shift+L")},
	{ K_c | K_s | 'D', T("F&older dialog...\tCtrl+Shift+D")},
	{ K_c | K_s | 'I', T("%G\"VIMF|Insert selected filename...\"(&G)\tCtrl+Shift+I")},
	{ 0, NULL }
};

const TCHAR amenu2str[] = T("&Insert");
PPXINMENU amenu2[] = {
	{ K_c | 'N',	T("File&name\tCtrl+N")},
	{ K_c | 'P',	T("Full&path Filename\tCtrl+P")},
	{ K_c | 'E',	T("Filename without &ext\tCtrl+E")},
	{ K_c | 'T',	T("File ext\tCtrl+&T")},
	{ K_c | 'R',	T("Filename on curso&r\tCtrl+&R")},
	{ K_c | '0',	T("PPx path\tCtrl+&0")},
	{ K_c | '1',	T("Current path\tCtrl+&1")},
	{ K_c | '2',	T("Pair path\tCtrl+&2")},
	{ K_F5,			T("now &Date\tF5")},
	{ 0, NULL }
};

PPXINMENU returnmenu[] = {
	{ VTYPE_CRLF + 1,	T("C&R LF(Windows)")},
	{ VTYPE_CR + 1,		T("&CR")},
	{ VTYPE_LF + 1,		T("&LF(Unix)")},
	{ 0, NULL }
};

const TCHAR charmenustr_lcp[] = T("&local codepage");
const TCHAR charmenustr_other[] = T("&other...");

PPXINMENU charmenu[charmenu_items] = {
	{ CP__US,			T("US(&A)")},
	{ CP__LATIN1,		T("&Latin1")},
	{ VTYPE_SYSTEMCP,	T("&Shift_JIS")},
	{ VTYPE_EUCJP,		T("&EUC-JP")},
	{ CP__UTF16L,		T("UTF-1&6")},
	{ VTYPE_UNICODE,	T("&UTF-16LE(BOM)")},
	{ CP__UTF16B,		T("UTF-1&6BE")},
	{ VTYPE_UNICODEB,	T("UTF-1&6BE(BOM)")},
	{ CP_UTF8,			T("UTF-&8")},
	{ VTYPE_UTF8,		T("UTF-8(&BOM)")},
	{ 0, NULL }, //	{ VTYPE_SYSTEM/CP_UTF8,	charmenustr_lcp},
	{ 0, NULL }, //	{ VTYPE_OTHER		, T("codepage : %d")}
	{ 0, NULL }
};

typedef struct {
	PPXAPPINFO info;
	PPxEDSTRUCT *PES;
} EDITMODULEINFOSTRUCT;
const TCHAR EditInfoName[] = WC_EDIT;
const TCHAR GetFileExtsStr[] = T("All Files\0*.*\0\0");

LRESULT EdPPxWmCommand(PPxEDSTRUCT *PES, HWND hWnd, WPARAM wParam, LPARAM lParam);
void GetPPePopupPositon(PPxEDSTRUCT *PES, POINT *pos);

#define WIDEDELTA 32

#define CLOSELIST_NONE			0 // リストは表示されていなかった
#define CLOSELIST_MANUALLIST	1 // 手動表示リストを閉じた
#define CLOSELIST_AUTOLIST		2 // 自動リストを閉じた

DefineWinAPI(HRESULT, SHAutoComplete, (HWND hwndEdit, DWORD dwFlags)) = NULL;
#ifndef SHACF_FILESYSTEM
#define SHACF_AUTOAPPEND_FORCE_ON	0x40000000
#define SHACF_AUTOSUGGEST_FORCE_ON	0x10000000
#define SHACF_FILESYSTEM			1
#define SHACF_USETAB				8
#endif

LOADWINAPISTRUCT SHLWAPIDLL[] = {
	LOADWINAPI1(SHAutoComplete),
	{NULL, NULL}
};
#define FloatBarID 12266
const TCHAR FloatClassStr[] = T("PPxFloatBar");
const TCHAR FloatBarNameMouseStr[] = T("B_flm");
const TCHAR FloatBarNamePenStr[] = T("B_flp");
const TCHAR FloatBarNameTouchStr[] = T("B_flt");

LRESULT CALLBACK FloatProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PPxEDSTRUCT *PES;

	PES = (PPxEDSTRUCT *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if ( PES == NULL ){
		if ( message == WM_CREATE ){
			PES = (PPxEDSTRUCT *)(((CREATESTRUCT *)lParam)->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)PES);
			return 0;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	switch (message){
		case WM_COMMAND:
			if ( (HWND)lParam == PES->FloatBar.hToolBarWnd ){
				const TCHAR *ptr;

				ptr = GetToolBarCmd((HWND)lParam, &PES->FloatBar.Cmd, LOWORD(wParam));
				if ( ptr != NULL ){
					if ( ((UTCHAR)ptr[0] == EXTCMD_KEY) &&
						 (*(WORD *)(ptr + 1) == 'X') ){
						PostMessage(hWnd, WM_CLOSE, 0, 0);
					}else{
						PPECOMMANDPARAM param = {0, 0};

						PPedExtCommand(PES, &param, ptr);
					}
				}
				return 0;
			}
			break;

		case WM_WINDOWPOSCHANGED: {
			RECT box;

			GetClientRect(hWnd, &box);
			SetWindowPos(PES->FloatBar.hToolBarWnd, NULL,
					0, box.top, box.right, 32, SWP_NOACTIVATE | SWP_NOZORDER);
			break;
		}

		case WM_DESTROY:
			CloseToolBar(PES->FloatBar.hToolBarWnd);
			PES->FloatBar.hToolBarWnd = NULL;
			ThFree(&PES->FloatBar.Cmd);

			PES->FloatBar.hWnd = NULL;
			break;

		case WM_MOUSEACTIVATE:
			return MA_NOACTIVATE; // 非アクティブ
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

#define BARSPACE 2
void InitFloatBar(PPxEDSTRUCT *PES, int posX)
{
	UINT ID = FloatBarID;
	RECT box, editbox;
	LPARAM pointtype = GetMessageExtraInfo();
	const TCHAR *FloatBarNamePtr;
	int minheight, posY;
	HWND hParentWnd;

	if ( PES->FloatBar.Cmd.size == 1 ) return;

	if ( (pointtype & POINTTYPE_SIG_MASK) == POINTTYPE_TOUCH ){
		minheight = 32 - BARSPACE;
		FloatBarNamePtr = FloatBarNameTouchStr;
		if ( (pointtype & POINTTYPE_TOUCH_MASK) == POINTTYPE_TOUCH_PEN ){ //pen
			if ( (X_pmc[pmc_pen] & TOUCH_EDIT_COMMAND_BAR) ){ // 負論理注意
				PES->FloatBar.Cmd.size = 1;
				return;
			}
			if ( IsExistCustData(FloatBarNamePenStr) ){
				FloatBarNamePtr = FloatBarNamePenStr;
			}
		}else{ //touch
			if ( (X_pmc[pmc_touch] & TOUCH_EDIT_COMMAND_BAR) ){ // 負論理注意
				PES->FloatBar.Cmd.size = 1;
				return;
			}
		}
	}else{
		if ( (X_pmc[pmc_mouse] & TOUCH_EDIT_COMMAND_BAR) ){ // 負論理注意
			PES->FloatBar.Cmd.size = 1;
			return;
		}

		minheight = 16 - BARSPACE;
		FloatBarNamePtr = FloatBarNameMouseStr;
	}

	if ( !IsExistCustData(FloatBarNamePtr) ){
		PES->FloatBar.Cmd.size = 1;
		return;
	}

	if ( DLLWndClass.item.FloatMenu == 0 ){
		WNDCLASS FloatClass;

		FloatClass.style = 0;
		FloatClass.lpfnWndProc = FloatProc;
		FloatClass.cbClsExtra = 0;
		FloatClass.cbWndExtra = 0;
		FloatClass.hInstance = DLLhInst;
		FloatClass.hIcon = NULL;
		FloatClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		FloatClass.hbrBackground = NULL;
		FloatClass.lpszMenuName = NULL;
		FloatClass.lpszClassName = FloatClassStr;
		DLLWndClass.item.FloatMenu = RegisterClass(&FloatClass);
	}

	// ツールバー親
	PES->FloatBar.hWnd = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
		FloatClassStr, NilStr, WS_POPUP,
		0, 0, 0, 0, PES->hWnd, NULL, DLLhInst, (LPVOID)PES);

	ThInit(&PES->FloatBar.Cmd);
	PES->FloatBar.hToolBarWnd = CreateToolBar(&PES->FloatBar.Cmd, PES->FloatBar.hWnd, &ID, FloatBarNamePtr, DLLpath, 0);

	// ツールバー本体の位置調整と大きさ調整
	SetWindowPos(PES->FloatBar.hToolBarWnd, NULL,
			0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

	SendMessage(PES->FloatBar.hToolBarWnd, TB_GETITEMRECT,
		SendMessage(PES->FloatBar.hToolBarWnd, TB_BUTTONCOUNT, 0, 0) - 1,
		(LPARAM)(RECT *)&box);

	minheight = (minheight * GetMonitorDPI(PES->hWnd)) / DEFAULT_WIN_DPI;
	if ( box.bottom < minheight ){
		box.bottom = minheight;
		SendMessage(PES->FloatBar.hToolBarWnd, TB_SETBUTTONSIZE, 0, TMAKELPARAM(minheight, minheight));
	}
	box.bottom += BARSPACE;

	GetWindowRect(PES->hWnd, &editbox);

	posX = editbox.left + posX - box.right / 2;
	if ( posX < editbox.left ) posX = editbox.left;
	posY = editbox.bottom; // 通常の場所: EDIT の直下
	hParentWnd = GetParentCaptionWindow(PES->hWnd);
	if ( hParentWnd != PES->hWnd ){
		RECT tmpbox, deskbox;

		GetWindowRect(hParentWnd, &tmpbox);
		if ( (tmpbox.bottom - posY) < (int)PES->fontY * 3 ) posY = tmpbox.bottom - 4;
		if ( PES->flags & PPXEDIT_COMBOBOX ){
			posY = editbox.top - box.bottom;
		}

		GetDesktopRect(hParentWnd, &deskbox);
		if ( (posX + box.right) > deskbox.right ){
			posX = deskbox.right - box.right;
		}
		if ( posX < deskbox.left ) posX = deskbox.left;
		if ( (posY + box.bottom) > deskbox.bottom ){ // 下にはみ出すならEDIT上
			posY = editbox.top - box.bottom;
		}
	}

	SetWindowPos(PES->FloatBar.hWnd, NULL,
			posX, posY,  box.right, box.bottom,
			SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOZORDER);

	ShowWindow(PES->FloatBar.hWnd, SW_SHOWNOACTIVATE);
}

void WMGestureEdit(HWND hWnd, WPARAM wParam/*, LPARAM lParam*/)
{
/*
	if ( TouchMode == 0 ){
		GetCustData(T("X_pmc"), &X_pmc, sizeof(X_pmc));
		if ( X_pmc[pmc_mode] < 0 ){
			TouchMode = ~X_pmc[pmc_touch];
			PPxCommonCommand(hWnd, 0, K_E_TABLET);
		}
	}else{
*/
	{
		switch ( wParam ){
			case GID_TWOFINGERTAP:
				PostMessage(hWnd, WM_PPXCOMMAND, K_apps, 0);
				break;

			case GID_PRESSANDTAP:
				PostMessage(hWnd, WM_PPXCOMMAND, K_apps, 0);
				break;
		}
	}
}

void PPeRunas(PPxEDSTRUCT *PES, TCHAR *cmdline)
{
	TCHAR path[VFPS];
	TCHAR exefile[VFPS];
	const TCHAR *ptr;
	PPXCMDENUMSTRUCT work;

	PPxEnumInfoFunc(PES->info, '1', path, &work);
	ptr = cmdline;
	GetLineParamS(&ptr, exefile, TSIZEOF(exefile));
	if ( PPxShellExecute(PES->hWnd, T("RUNAS"), exefile, ptr, path, 0, cmdline) != NULL ){
		WriteHistory(PPXH_COMMAND, cmdline, 0, NULL);
		PostMessage(GetParentCaptionWindow(PES->hWnd), WM_CLOSE, 0, 0);
	}else{
		if ( GetLastError() != ERROR_CANCELLED ){
			XMessage(PES->hWnd, NULL, XM_GrERRld, cmdline);
		}
	}
}

void WideWindowByKey(PPxEDSTRUCT *PES, int offsetW, int offsetH)
{
	RECT box;
	HWND hWnd;

	if ( !(PES->flags & (PPXEDIT_LINEEDIT | PPXEDIT_ENABLE_SIZE_CHANGE)) ){
		return;
	}
	hWnd = GetParentCaptionWindow(PES->hWnd);
	GetWindowRect(hWnd, &box);

	box.right -= box.left;
	if ( (offsetW < 0) && (box.right < (WIDEDELTA * 2)) ) offsetW = 0;

	box.bottom -= box.top;
	if ( (!(PES->flags & PPXEDIT_ENABLE_HEIGHT_CHANGE)) ||
		 ((offsetH < 0) && (box.bottom < (int)(PES->fontY * 2))) ){
		offsetH = 0;
	}
	if ( offsetW || offsetH ){
		box.right += offsetW * WIDEDELTA;
		box.bottom += offsetH * PES->fontY;

		SetWindowPos(hWnd, NULL, 0, 0,
				box.right, box.bottom, SWP_NOZORDER | SWP_NOMOVE);
	}
}

#define ZENRATE (2 / sizeof(TCHAR)) // Multibyte の時は 2(半→全で2倍), UNICODE のときは 1
void USEFASTCALL PPeConvertZenHanMain(PPxEDSTRUCT *PES, HWND hWnd, int mode)
{
	TEXTSEL ts;
	TCHAR defbuf[CMDLINESIZE * ZENRATE], *buf;
	TCHAR *buflast;
	const TCHAR *first;
	size_t wordlen;

	if ( SelectEditStrings(PES, &ts, TEXTSEL_WORD) == FALSE ) return;

	// 簡易全半検出
	for ( first = ts.word ; (UTCHAR)*first < ' ' ; first++ );

	if ( mode < 0 ){
	#ifdef UNICODE
		mode = ((*first >= L' ') && (*first <= L'~')) || ((*first >= L'｡') && (*first <= L'ﾟ'));
	#else
		mode = !IskanjiA(*first);
	#endif
	}
	wordlen = tstrlen(ts.word);

	if ( wordlen < (CMDLINESIZE - 1) ){
		buf = defbuf;
	}else{
		buf = HeapAlloc(DLLheap, 0, TSTROFF(wordlen * ZENRATE));
	}
	if ( buf != NULL ){
		// 変換
		if ( mode ){
			buflast = Strsd(buf, ts.word);
		}else{
			buflast = Strds(buf, ts.word);
		}
#ifdef UNICODE
		if ( memcmp(buf, ts.word, TSTROFF(buflast - buf)) == 0 )
#else
		if ( (buflast - buf) == wordlen )
#endif
		{ // 変換に失敗→反対方向に変換
			if ( !mode ){
				buflast = Strsd(buf, ts.word);
			}else{
				buflast = Strds(buf, ts.word);
			}
		}
		SendMessage(hWnd, EM_REPLACESEL, 1, (LPARAM)buf);
		if ( ts.cursororg.start != ts.cursororg.end ){
			ts.cursororg.end = buflast - buf;
#ifndef UNICODE
			if ( xpbug < 0 ) CaretFixToW(buf, &ts.cursororg.end);
#endif
			ts.cursororg.end += ts.cursororg.start;
		}
		SendMessage(hWnd, EM_SETSEL, ts.cursororg.start, ts.cursororg.end);
		if ( buf != defbuf ) HeapFree(DLLheap, 0, buf);
	}
	FreeTextselStruct(&ts);
}

// *find
void PPeFind(PPxEDSTRUCT *PES, const TCHAR *param)
{
	DWORD replaceFlags = 0;
	const TCHAR *more;
	TCHAR buf[CMDLINESIZE];
	UTCHAR code;
	int dist = EDITDIST_NEXT;

	if ( PES->findrep == NULL ){
		if ( InitPPeFindReplace(PES) == FALSE ) return;
	}
	while( '\0' != (code = GetOptionParameter(&param, buf, CONSTCAST(TCHAR **, &more))) ){
		if ( code == '-' ){
			if ( !tstrcmp( buf + 1, T("REPLACE")) ){
				tstplimcpy(PES->findrep->replacetext, more, VFPS);
				if ( replaceFlags == 0 ) replaceFlags = FR_REPLACE;
				continue;
			}
			if ( !tstrcmp( buf + 1, T("ALL")) ){
				replaceFlags = FR_REPLACEALL;
				continue;
			}
			/*
			if ( !tstrcmp( buf + 1, T("FORWARD")) ){
				dist = EDITDIST_NEXT;
				continue;
			}
			*/
			if ( !tstrcmp( buf + 1, T("BACK")) || !tstrcmp( buf + 1, T("PREVIOUS")) ){
				dist = EDITDIST_BACK;
				continue;
			}
			if ( !tstrcmp( buf + 1, T("DIALOG")) ){
				dist = EDITDIST_DIALOG;
				continue;
			}
			if ( !tstrcmp( buf + 1, T("REGEXP")) ){
				X_esrx = CheckParamOnOff(&more);
				continue;
			}
			XMessage(NULL, NULL, XM_GrERRld, StrOptionError, buf);
		}else{
			#pragma warning(suppress:6001 6011) // InitPPeFindReplace で初期化
			tstplimcpy(PES->findrep->findtext, buf, VFPS);
		}
	}
	if ( replaceFlags == 0 ){
		SearchStr(PES, dist);
	}else{
		if ( dist == EDITDIST_DIALOG ){
			PPeReplaceStr(PES);
		}else{
			FINDREPLACE freplace;

			freplace.hwndOwner = PES->hWnd;
			freplace.Flags = FR_DOWN | FR_HIDEMATCHCASE | FR_HIDEWHOLEWORD | replaceFlags;
			freplace.lpstrFindWhat = PES->findrep->findtext;
			freplace.lpstrReplaceWith = PES->findrep->replacetext;
//			freplace.wFindWhatLen = VFPS;
//			freplace.wReplaceWithLen = VFPS;
			freplace.lCustData = 0;
			PPeReplaceStrCommand(PES, &freplace);
		}
	}
}

BOOL SetHistorySettings(PPxEDSTRUCT *PES, const TCHAR *more)
{
	BOOL result;
	TINPUT_EDIT_OPTIONS options;

	result = GetEditMode(&more, &options);
	if ( IsTrue(result) ){
		PES->list.RhistID = options.hist_readflags;
		PES->list.WhistID = HistWriteTypeflag[options.hist_writetype];
		PES->flags = (PES->flags & ~(TIEX_REFTREE | TIEX_SINGLEREF)) |
				TinputTypeflags[options.hist_writetype];
		if ( options.flags & TINPUT_EDIT_OPTIONS_single_param ){
			setflag(PES->flags, TIEX_SINGLEREF);
		}
	}
	return result;
}

void SetMatchMode(PPxEDSTRUCT *PES, int mode)
{
	if ( mode >= 2 ){ // 部分一致
		setflag(PES->ED.cmdsearch, CMDSEARCH_FLOAT);
		if ( mode == 3 ){ // 部分一致 + 選択
			setflag(PES->flags, CMDSEARCHI_SELECTPART | PPXEDIT_LISTCOMP);
			if ( X_flst_mode == X_fmode_off ) X_flst_mode = X_fmode_manual;
		}else if ( mode == 4 ){ // migemo
			setflag(PES->ED.cmdsearch, CMDSEARCH_ROMA);
		}else if ( mode == 5 ){ // and
			setflag(PES->ED.cmdsearch, CMDSEARCH_WILDCARD);
		}else if ( mode == 6 ){ // and + migemo
			setflag(PES->ED.cmdsearch, CMDSEARCH_WILDCARD | CMDSEARCH_ROMA);
		}else if ( mode == 7 ){ // regexp
			setflag(PES->ED.cmdsearch, CMDSEARCH_REGEXP);
		}
	}
}

// *completelist
void EditCompleteListCommand(PPxEDSTRUCT *PES, const TCHAR *param)
{
	TCHAR *more, *option = NULL;
	TCHAR buf[CMDLINESIZE], text[CMDLINESIZE];
	UTCHAR code;

	BOOL setmode = FALSE;

	text[0] = '\0';
	while( '\0' != (code = GetOptionParameter(&param, buf, &more)) ){
		if ( code == '-' ){
			if ( !tstrcmp( buf + 1, T("CLOSE")) ){
				CloseLineList(PES);
				return;
			}

			if ( !tstrcmp( buf + 1, T("RELOAD")) ){
				CleanUpEdit();
				continue;
			}

			if ( !tstrcmp( buf + 1, T("FREE")) ){
				CancelListThread(PES);
				CloseLineList(PES);
				CleanUpEdit();
				PES->ListThreadCount = 0;
				continue;
			}

			if ( !tstrcmp( buf + 1, T("SET")) ){
				setmode = TRUE;
				continue;
			}

			if ( !tstrcmp( buf + 1, T("SHOW")) ){
				setmode = FALSE;
				tstrcpy(text, more);
				option = text;
				continue;
			}

			if ( !tstrcmp( buf + 1, T("HISTORY")) ){
				if ( SetHistorySettings(PES, more) ){
					resetflag(PES->list.flags, LISTFLAG_USER_COMB);
					continue;
				}else{
					return;
				}
			}

			if ( !tstrcmp( buf + 1, T("LIST")) ){
				int mode;

				if ( SkipSpace((const TCHAR **)&more) == '\0' ){
					mode = 9; // -list:on 相当
				}else{
					mode = GetStringListCommand(more,
							T("disable\0") T("file\0") T("api\0") T("single\0") T("double\0") T("doubleex\0")
							T("paramchange\0") T("allchange\0") T("off\0") T("on\0"));
				}
				if ( mode < 0 ){
					XMessage(PES->hWnd, T("*completelist"), XM_GrERRld, StrOptionError, more);
				}else if ( mode < 6 ){ // disable, file, api, single, double/ex
					CancelListThread(PES);
					CloseLineList(PES);
					X_flst_mode = mode;
					if ( X_flst_mode == X_fmode_off ){
						resetflag(PES->flags, PPXEDIT_LISTCOMP);
					}else{
						setflag(PES->flags, PPXEDIT_LISTCOMP);
					}
					continue;
				}else if ( mode == 6 ){ // paramchange
					resetflag(PES->flags, PPXEDIT_SINGLEREF);
					continue;
				}else if ( mode == 7 ){ // allchange
					setflag(PES->flags, PPXEDIT_SINGLEREF);
					continue;
				}else{
					if ( mode == 8 ){ // -list:off
						resetflag(PES->list.flags, LISTFLAG_LISTMODE);
					}else{ // mode == 9 -list:on
						setflag(PES->list.flags, LISTFLAG_LISTMODE);
						option = KeyStepFill_listmode;
					}
					continue;
				}
			}

			if ( !tstrcmp( buf + 1, T("MODULE")) ){
				if ( CheckParamOnOff((const TCHAR **)&more) != 0 ){
					resetflag(PES->list.flags, LISTFLAG_NOMODULE);
				}else{
					setflag(PES->list.flags, LISTFLAG_NOMODULE);
				}
				continue;
			}

			if ( !tstrcmp( buf + 1, T("MATCH")) ){
				resetflag(PES->ED.cmdsearch, CMDSEARCH_MATCHFLAGS);
				SetMatchMode(PES, GetIntNumber((const TCHAR **)&more));
				if ( SkipSpace((const TCHAR **)&more) == ',' ){
					setflag(PES->flags, CMDSEARCHI_SELECTPART | PPXEDIT_LISTCOMP);
					if ( X_flst_mode == X_fmode_off ) X_flst_mode = X_fmode_manual;
				}
				continue;
			}

			// keystepfillだとキーを押すと消えてしまう。
/*
			if ( !tstrcmp( buf + 1, T("ADDITEM")) ){
				KeyStepFill(PES, option);
				if ( PES->list.hWnd == NULL ){
					int direction;

					PES->list.mode = PES->list.startmode = LIST_FILL;
					direction = (PES->flags & PPXEDIT_COMBOBOX) ? -1 : 1;

					CreateMainListWindow(PES, direction);
				}

				if ( PES->list.hWnd != NULL ){
					SendMessage(PES->list.hWnd, LB_INSERTSTRING, 0, (LPARAM)more);
					ShowWindow(PES->list.hWnd, SW_SHOWNA);
				}
				return;
			}
*/
			if ( !tstrcmp( buf + 1, T("DETAIL")) ){
				if ( *more == '\0' ){
					resetflag(PES->list.flags, LISTFLAG_USER_COMB);
				}else{
					setflag(PES->list.flags, LISTFLAG_USER_COMB);
					ThSetString(&PES->LocalStringValue, LSV_user_comb, more);
				}
				continue;
			}

			if ( !tstrcmp( buf + 1, T("SCROLL")) ){
				ListWidth(PES, GetIntNumber((const TCHAR **)&more));
				return;
			}

			if ( !tstrcmp( buf + 1, T("WIDE")) ){
				ListWidth(PES, GetIntNumber((const TCHAR **)&more));
				return;
			}

			if ( !tstrcmp( buf + 1, T("FILE")) ){
				ERRORCODE result;

				{ // file 使用の終了待ち
					#define ThreadChkSleepTime 20
					int WaitTimer = 1000 / ThreadChkSleepTime;
					PES->ActiveListThreadID = 0;
					for ( ;; ){
						if ( PES->ListThreadCount == 0 ) break;
						PeekMessageLoop(PES->hWnd);
						Sleep(ThreadChkSleepTime);
						if ( --WaitTimer <= 0 ) break;
					}
				}

				FreeFillTextFile(&PES->list.filltext_user);
				if ( *more == '\0' ) return;


				PES->list.filltext_user.loading = TRUE;
				VFSFullPath(NULL, more, DLLpath);

				if ( (result = LoadTextData(more, &PES->list.filltext_user.mem, &PES->list.filltext_user.text, NULL, 0)) != NO_ERROR ){
					PES->list.filltext_user.loading = FALSE;
					ErrorPathBox(PES->hWnd, T("*completelist -file"), more, result);
					return;
				}
				PES->list.filltext_user.loading = FALSE;
				continue;
			}
		}
		XMessage(PES->hWnd, T("*completelist"), XM_GrERRld, StrOptionError, buf);
		return;
	}
	if ( IsTrue(setmode) ){
		if ( option == KeyStepFill_listmode ){
			setflag(PES->list.flags, LISTFLAG_LISTMODE);
		}
		return;
	}

	KeyStepFill(PES, option);
}

void EditTreeCommand(PPxEDSTRUCT *PES, const TCHAR *param)
{
	TCHAR cmdstr[VFPS];

	if ( PES->hTreeWnd == NULL ){
		GetLineParamS(&param, cmdstr, TSIZEOF(cmdstr));
		if ( !tstrcmp(cmdstr, T("off")) ) return; // close 済み
		PPeTreeWindow(PES);
		if ( cmdstr[0] == '\0' ) return;
	}
	if ( PES->hTreeWnd != NULL ){
		SendMessage(PES->hTreeWnd, VTM_TREECOMMAND, 0, (LPARAM)param); // 整形前を渡す
	}
}

BOOL EditModeParam(EDITMODESTRUCT *ems, const TCHAR *param, const TCHAR *more)
{
	// codepage
	if ( tstrcmp(param, T("SYSTEM")) == 0 ){
		ems->codepage = VTYPE_SYSTEMCP;
	}else if ( (tstrcmp(param, T("UNICODE")) == 0) || (tstrcmp(param, T("UTF16")) == 0) ){
		ems->codepage = CP__UTF16L;
	}else if ( (tstrcmp(param, T("UNICODEB")) == 0) || (tstrcmp(param, T("UTF16BE")) == 0) ){
		ems->codepage = CP__UTF16B;
	}else if ( tstrcmp(param, T("UNICODEBOM")) == 0 ){
		ems->codepage = VTYPE_UNICODE;
	}else if ( tstrcmp(param, T("UNICODEBBOM")) == 0 ){
		ems->codepage = VTYPE_UNICODEB;
	}else if ( tstrcmp(param, T("EUC")) == 0 ){
		ems->codepage = VTYPE_EUCJP;
	}else if ( tstrcmp(param, T("UTF8")) == 0 ){
		ems->codepage = CP_UTF8;
	}else if ( tstrcmp(param, T("UTF8BOM")) == 0 ){
		ems->codepage = VTYPE_UTF8;
	}else if ( tstrcmp(param, T("SJIS")) == 0 ){
		ems->codepage = (GetACP() == CP__SJIS) ? VTYPE_SYSTEMCP : CP__SJIS;
	}else if ( (tstrcmp(param, T("IBM")) == 0) || (tstrcmp(param, T("US")) == 0) ){
		ems->codepage = CP__US;
	}else if ( (tstrcmp(param, T("ANSI")) == 0) || (tstrcmp(param, T("LATIN1")) == 0)){
		ems->codepage = CP__LATIN1;
	}else if ( tstrcmp(param, T("CODEPAGE")) == 0 ){
		ems->codepage = GetNumber((const TCHAR **)&more);
	// crcode
	}else if ( tstrcmp(param, CRCD_CR) == 0 ){
		ems->crcode = VTYPE_CR;
	}else if ( tstrcmp(param, CRCD_CRLF) == 0 ){
		ems->crcode = VTYPE_CRLF;
	}else if ( tstrcmp(param, CRCD_LF) == 0 ){
		ems->crcode = VTYPE_LF;
	// tabwidth
	}else if ( tstrcmp(param, T("TAB")) == 0 ){
		ems->tabwidth = GetNumber((const TCHAR **)&more);
	}else{
		return FALSE;
	}
	return TRUE;
}

void PutOptionFlags(TCHAR *dest, DWORD flags, const TCHAR *flagletters, int maxbits)
{
	int i;

	for ( i = 0; i < maxbits; i++ ){
		if ( flags & (1 << i) ) *dest++ = *(flagletters + i);
	}
	*dest = '\0';
}

typedef struct {
	const char *data;
	LONG left;
} RtfStreamInStruct;

DWORD CALLBACK RtfStreamIn(RtfStreamInStruct *rsi, LPBYTE pBuffer, LONG cb, LONG *pcb)
{
	*pcb = (cb >= rsi->left) ? rsi->left : cb;
	memcpy(pBuffer, rsi->data, *pcb);
	rsi->data += *pcb;
	rsi->left -= *pcb;
	return NO_ERROR;
}

void EditInsertRtf(PPxEDSTRUCT *PES, const TCHAR *param) // *insertrtf
{
	EDITSTREAM es;
	RtfStreamInStruct rsi;

	rsi.data = (const char *)param;
	rsi.left = (LONG)TSTRLENGTH(param);
	es.dwCookie = (DWORD_PTR)&rsi;
	es.dwError = NO_ERROR;
	es.pfnCallback = (EDITSTREAMCALLBACK)RtfStreamIn;
#ifdef UNICODE
	SendMessage(PES->hWnd, EM_STREAMIN, SF_RTF | SF_UNICODE | SFF_SELECTION, (LPARAM)&es);
#else
	SendMessage(PES->hWnd, EM_STREAMIN, SF_RTF, (LPARAM)&es);
#endif
}

void EditSetTextProperties(PPxEDSTRUCT *PES, const TCHAR *param) // *textprop
{
	TCHAR buf[CMDLINESIZE], code, *more;
	CHARFORMAT2 chfmt;
	PARAFORMAT2 prfmt;
	BOOL rangedefault = FALSE;
	WPARAM range = 0;

	if ( !(PES->flags & PPXEDIT_RICHEDIT) ) return;

	memset(&chfmt, 0, sizeof(chfmt));
	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = CFM_ALL2;
	SendMessage(PES->hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&chfmt);

	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = 0;

	memset(&prfmt, 0, sizeof(prfmt));
	prfmt.cbSize = sizeof(prfmt);

	for ( ;; ){
		code = GetOptionParameter(&param, buf, &more);
		if ( code == '\0' ) break;
		if ( code == '-' ){
			if ( !tstrcmp(buf + 1, T("SELECT")) ){
				setflag(range, SCF_SELECTION);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("DEFAULT")) ){
				rangedefault = TRUE;
				continue;
			}
			if ( !tstrcmp(buf + 1, T("WORD")) ){
				setflag(range, SCF_WORD);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("ALL")) ){
				setflag(range, SCF_ALL);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("SIZE")) ){
				chfmt.yHeight = (LONG)GetNumber((const TCHAR **)&more);
				if ( *more == 'p' ){
					if ( *(more + 1) == 't' ){
						chfmt.yHeight *= 20;
					}else if ( (*(more + 1) == 'i') || (*(more + 1) == 'x') ){
						chfmt.yHeight = TwipToPixel(chfmt.yHeight, 96);
					}
				}
				setflag(chfmt.dwMask, CFM_SIZE);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("TEXTCOLOR")) ){
				chfmt.crTextColor = GetColor((const TCHAR **)&more, TRUE);
				resetflag(chfmt.dwEffects, CFE_AUTOCOLOR);
				setflag(chfmt.dwMask, CFM_COLOR);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("BACKCOLOR")) ){
				chfmt.crBackColor = GetColor((const TCHAR **)&more, TRUE);
				resetflag(chfmt.dwEffects, CFE_AUTOBACKCOLOR);
				setflag(chfmt.dwMask, CFM_BACKCOLOR);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("FACE")) ){
				tstplimcpy(chfmt.szFaceName, more, LF_FACESIZE);
				setflag(chfmt.dwMask, CFM_FACE);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("YOFFSET")) ){
				chfmt.yOffset = (LONG)GetNumber((const TCHAR **)&more);
				setflag(chfmt.dwMask, CFM_OFFSET);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("UNDERLINE")) ){
				chfmt.bUnderlineType = (BYTE)GetNumber((const TCHAR **)&more);
				setflag(chfmt.dwMask, CFM_UNDERLINETYPE);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("BOLD")) ){
				setflag(chfmt.dwMask, CFM_BOLD);
				chfmt.dwEffects = (chfmt.dwEffects & ~CFE_BOLD) |
						(GetNumber((const TCHAR **)&more) ? CFE_BOLD : 0);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("DISABLE")) ){
				setflag(chfmt.dwMask, CFM_DISABLED);
				chfmt.dwEffects = (chfmt.dwEffects & ~CFE_DISABLED) |
						(GetNumber((const TCHAR **)&more) ? CFE_DISABLED : 0);
				continue;
			}
			/*
			if ( !tstrcmp(buf + 1, T("EMBOSS")) ){
				setflag(chfmt.dwMask, CFM_EMBOSS);
				chfmt.dwEffects = (chfmt.dwEffects & ~CFE_EMBOSS) |
						(GetNumber((const TCHAR **)&more) ? CFE_EMBOSS : 0);
				continue;
			}
			*/
			/*
			if ( !tstrcmp(buf + 1, T("IMPRINT")) ){
				setflag(chfmt.dwMask, CFM_IMPRINT);
				chfmt.dwEffects = (chfmt.dwEffects & ~CFE_IMPRINT) |
						(GetNumber((const TCHAR **)&more) ? CFE_IMPRINT : 0);
				continue;
			}
			*/
			if ( !tstrcmp(buf + 1, T("ITALIC")) ){
				setflag(chfmt.dwMask, CFM_ITALIC);
				chfmt.dwEffects = (chfmt.dwEffects & ~CFE_ITALIC) |
						(GetNumber((const TCHAR **)&more) ? CFE_ITALIC : 0);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("LINK")) ){
				setflag(chfmt.dwMask, CFM_LINK);
				chfmt.dwEffects = (chfmt.dwEffects & ~CFE_LINK) |
						(GetNumber((const TCHAR **)&more) ? CFE_LINK : 0);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("STRIKEOUT")) ){
				setflag(chfmt.dwMask, CFM_STRIKEOUT);
				chfmt.dwEffects = (chfmt.dwEffects & ~CFE_STRIKEOUT) |
						(GetNumber((const TCHAR **)&more) ? CFE_STRIKEOUT : 0);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("SUBSCRIPT")) ){
				setflag(chfmt.dwMask, CFM_SUBSCRIPT);
				chfmt.dwEffects = (chfmt.dwEffects & ~CFE_SUBSCRIPT) |
						(GetNumber((const TCHAR **)&more) ? CFE_SUBSCRIPT : 0);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("SUPERSCRIPT")) ){
				setflag(chfmt.dwMask, CFM_SUPERSCRIPT);
				chfmt.dwEffects = (chfmt.dwEffects & ~CFE_SUPERSCRIPT) |
						(GetNumber((const TCHAR **)&more) ? CFE_SUPERSCRIPT : 0);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("NUMBERING")) ){
				setflag(prfmt.dwMask, PFM_NUMBERING);
				prfmt.wNumbering = (WORD)GetNumber((const TCHAR **)&more);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("NUMBERINGSTART")) ){
				setflag(prfmt.dwMask, PFM_NUMBERINGSTART);
				prfmt.wNumberingStart = (WORD)GetNumber((const TCHAR **)&more);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("STARTINDENT")) ){
				setflag(prfmt.dwMask, PFM_STARTINDENT);
				prfmt.dxStartIndent = GetNumber((const TCHAR **)&more);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("RIGHTINDENT")) ){
				setflag(prfmt.dwMask, PFM_RIGHTINDENT);
				prfmt.dxRightIndent = GetNumber((const TCHAR **)&more);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("NEXTINDENT")) ){
				setflag(prfmt.dwMask, PFM_OFFSET);
				prfmt.dxOffset = GetNumber((const TCHAR **)&more);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("ALIGNMENT")) ){
				setflag(prfmt.dwMask, PFM_ALIGNMENT);
				prfmt.wAlignment = (WORD)GetNumber((const TCHAR **)&more);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("SPACEBEFORE")) ){
				setflag(prfmt.dwMask, PFM_SPACEBEFORE);
				prfmt.dySpaceBefore = GetNumber((const TCHAR **)&more);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("SPACEAFTER")) ){
				setflag(prfmt.dwMask, PFM_SPACEAFTER);
				prfmt.dySpaceAfter = GetNumber((const TCHAR **)&more);
				continue;
			}
			if ( !tstrcmp(buf + 1, T("LINESPACE")) ){
				setflag(prfmt.dwMask, PFM_LINESPACING);
				prfmt.bLineSpacingRule = (BYTE)GetNumber((const TCHAR **)&more);
				continue;
			}

			XMessage(PES->hWnd, NULL, XM_GrERRld, MES_EUOP, buf);
			continue;
		}
	}
	if ( chfmt.dwMask != 0 ){
		SendMessage(PES->hWnd, EM_SETCHARFORMAT, range, (LPARAM)&chfmt);
		if ( (range != SCF_DEFAULT) && rangedefault ){
			SendMessage(PES->hWnd, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&chfmt);
		}
	}
	if ( prfmt.dwMask != 0 ){
		SendMessage(PES->hWnd, EM_SETPARAFORMAT, 0, (LPARAM)&prfmt);
	}
/*
	int mode = 3;

	if ( tstrcmp(buf, T("cursor")) == 0 ){
		mode = 1;
	}else if ( tstrcmp(buf, T("back")) == 0 ){
		mode = 2;
	}
	if ( mode & 1 ){ // cursor color
		clr = (*param > ' ') ? GetColor(&param, TRUE) : C_AUTO;
	}
*/
}

void EditGetTextProperties(PPxEDSTRUCT *PES, PPXMDLFUNCSTRUCT *funcparam) // %*textprop
{
	CHARFORMAT2 chfmt;
	PARAFORMAT2 prfmt;

	funcparam->dest[0] = '\0';
	if ( !(PES->flags & PPXEDIT_RICHEDIT) ) return;

	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = CFM_ALL2;
	SendMessage(PES->hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&chfmt);

	prfmt.cbSize = sizeof(prfmt);
	prfmt.dwMask = PFM_ALL2;
	SendMessage(PES->hWnd, EM_GETPARAFORMAT, 0, (LPARAM)&prfmt);

	thprintf(funcparam->dest, CMDLINESIZE,
		T("Effects: %x\r\n")
		T("CharSet: %d(2:SYMBOL 128:SJIS 129:HANGLE 134:GB2312)\r\n")
		T("PitchAndFamily: %d\r\n")
		T("font name: %s\r\n")
		T("font weight: %d\r\n")
		T("font size: %d pt, %d twip, %d pix\r\n")
		T("[font spacing]: %d twip\r\n")
		T("font underline: %d\r\n")
//		T("font underline: %d  color: %d\r\n")
		T("Offset Y: %d twip\r\n")
		T("Color: Text:%06x Back: %06x\r\n")
		T("lcid: %x\r\n")
		T("Kerning: %x\r\n")
		T("RevAuthor: %d\r\n")
		T("[Style]: %x\r\n")
		T("[Animation]: %d\r\n")
//		T("user define: %d\r\n")
		T("Numbering: %d\r\n")
		T("Indent: start:%d next:%d right:%d (twip)\r\n")
		T("Alignment: %d\r\n")
		T("TabCount: %d\r\n")
		T("SpaceBefore: %d\r\n")
		T("SpaceAfter: %d\r\n")
		T("LineSpacing: %d\r\n")
//		T("Style: %d\r\n")
		T("LineSpacingRule: %d\r\n")
//		T("bOutlineLevel / CRC: %d\r\n")
		T("ShadingWeight: %d\r\n")
		T("ShadingStyle: %d\r\n")
		T("NumberingStart: %d\r\n")
		T("NumberingStyle: %d\r\n")
		T("NumberingTab: %d\r\n")
		T("BorderSpace: %d\r\n")
		T("BorderWidth: %d\r\n")
		T("Borders: %d\r\n")
		,
		chfmt.dwEffects,
		chfmt.bCharSet,
		chfmt.bPitchAndFamily,
		chfmt.szFaceName,
		chfmt.wWeight,
		chfmt.yHeight / 20, chfmt.yHeight, PixelToTwip(chfmt.yHeight, 96),
		chfmt.sSpacing,
		chfmt.bUnderlineType,
//		chfmt.bUnderlineType, chfmt.bUnderlineColor,
		chfmt.yOffset,
		chfmt.crTextColor, chfmt.crBackColor,
		chfmt.lcid,
		chfmt.wKerning,
		chfmt.bRevAuthor,
		chfmt.sStyle,
		chfmt.bAnimation
//		chfmt.dwCookie
		,
		prfmt.wNumbering,
		prfmt.dxStartIndent, prfmt.dxOffset, prfmt.dxRightIndent,
		prfmt.wAlignment,
		prfmt.cTabCount,
		prfmt.dySpaceBefore,
		prfmt.dySpaceAfter,
		prfmt.dyLineSpacing,
//		prfmt.sStyle,          // wine で sStype とタイプミスしているので除外
		prfmt.bLineSpacingRule,
//		prfmt.bOutlineLevel or bCRC,
		prfmt.wShadingWeight,
		prfmt.wShadingStyle,
		prfmt.wNumberingStart,
		prfmt.wNumberingStyle,
		prfmt.wNumberingTab,
		prfmt.wBorderSpace,
		prfmt.wBorderWidth,
		prfmt.wBorders
	);
}

void CursorTextFunction(PPxEDSTRUCT *PES, PPXMDLFUNCSTRUCT *funcparam) // %*cursortext
{
	EditTextStruct ets;
	ECURSOR cursor;
	int len;

	funcparam->dest[0] = '\0';
	if ( OpenEditText(PES, &ets, 0) == FALSE ) return;
	GetEditSel(PES->hWnd, ets.text, &cursor);
	if ( PES->flags & PPXEDIT_RICHEDIT ){
		tstrreplace(ets.text, T("\r\n"), T("\n"));
	}
	GetWordStrings(ets.text, &cursor, (*funcparam->optparam == 'o') ? GWSF_SPLIT_PARAM : 0);

	len = tstrlen(ets.text + cursor.start);
	if ( len >= CMDLINESIZE ){
		TCHAR *longbuf;
		longbuf = HeapAlloc(ProcHeap, 0, TSTROFF(len + 1));
		if ( longbuf != NULL ){
			funcparam->dest = longbuf;
			tstrcpy(funcparam->dest, ets.text + cursor.start);
		}else{
			tstplimcpy(funcparam->dest, ets.text + cursor.start, CMDLINESIZE);
		}
	}else{
		tstrcpy(funcparam->dest, ets.text + cursor.start);
	}
	CloseEditText(&ets);
}

// *jumpline
void EditJumpLineCommand(PPxEDSTRUCT *PES, const TCHAR *param)
{
	int line, col = 0;
	EditLPos elp = { NULL, NULL, EditLPos_GET_CURSORPOS, 0, 0, NULL};
	TCHAR buf[20], pre;

	PPeLogicalLinePos(PES, &elp);

	if ( param == NULL ){
		param = buf;
		Numer32ToString(buf, elp.y + 1);
//		line = SendMessage(PES->hWnd, EM_LINEFROMCHAR, (WPARAM)-1, 0);
//		Numer32ToString(buf, line + 1);
		if ( tInput(PES->hWnd, MES_TLNO, buf, 10, PPXH_NUMBER, PPXH_NUMBER) <= 0 ){
			return;
		}
	}
	pre = TinyCharUpper(*param);
	if ( pre == 'L' ){
		param++;
	}else if ( pre == 'Y' ){
		param++;
	}
	line = GetIntNumber(&param);
	if ( SkipSpace(&param) == ',' ){
		param++;
		col = GetIntNumber(&param);
	}
	if ( (line != 0) || (col != 0) ){
		int CursorCmd[] = {0, 0, 0, 0, 0, 0};
		if ( pre == 'Y' ){
			if ( col == 0 ){
				JumptoLine(PES->hWnd, line - 1);
			}else{
				CursorCmd[0] = -3;
				CursorCmd[1] = (col > 0) ? col - 1 : 0;
				CursorCmd[2] = (line > 0) ? line - 1 : 0;
				EditCursorCommand(PES, (PPXAPPINFOUNION *)&CursorCmd);
			}
		}else{
			CursorCmd[0] = -17;
			CursorCmd[1] = (col > 0) ? col : elp.x + 1;
			CursorCmd[2] = (line > 0) ? line : elp.y + 1;
			EditCursorCommand(PES, (PPXAPPINFOUNION *)&CursorCmd);
		}
	}
}


DWORD CALLBACK RtfStreamOut(ThSTRUCT *th, LPBYTE pBuffer, LONG cb, LONG *pcb)
{
	if ( ThAppend(th, pBuffer, cb) == FALSE ) return ERROR_NOT_ENOUGH_MEMORY;
	*pcb = cb;
	return NO_ERROR;
}

ERRORCODE EditRtfTextFunction(PPxEDSTRUCT *PES, PPXMDLFUNCSTRUCT *fparam) // %*rtftext
{
	EDITSTREAM es;
	ThSTRUCT th;

	ThInit(&th);
	es.dwCookie = (DWORD_PTR)&th;
	es.dwError = NO_ERROR;
	es.pfnCallback = (EDITSTREAMCALLBACK)RtfStreamOut;
#ifdef UNICODE
	SendMessage(PES->hWnd, EM_STREAMOUT, SF_RTF | SF_UNICODE | SFF_SELECTION, (LPARAM)&es);
#else
	SendMessage(PES->hWnd, EM_STREAMOUT, SF_RTF | SFF_SELECTION, (LPARAM)&es);
#endif
	if ( th.top < TSTROFF(CMDLINESIZE) ){
		memcpy(fparam->dest, th.bottom, th.top);
		fparam->dest[th.top / sizeof(TCHAR)] = '\0';
		ThFree(&th);
	}else{
		fparam->dest = (TCHAR *)th.bottom;
		ThAppend(&th, NilStr, sizeof(NilStr));
	}
	return PPXA_NO_ERROR;
}

const TCHAR Hist_flags[] = T("gnd\x3xu\x6\x7vshfcmp");

// %*editprop
void EditGetPropFunction(PPxEDSTRUCT *PES, PPXMDLFUNCSTRUCT *fparam)
{
	WPARAM wParam;
	LPARAM lParam;
	TCHAR param[64];

	fparam->dest[0] = '\0';
	GetCommandParameter(&fparam->optparam, param, TSIZEOF(param));
	switch ( param[0] ){
		case 'c':
			if ( tstrcmp(param + 1, T("lass")) == 0 ){ // class
				GetClassName(PES->hWnd, fparam->dest, CMDLINESIZE);
			}else{ // codepade
				Numer32ToString(fparam->dest, PES->CharCP);
			}
			break;

		case 'd': // displaytop
			Numer32ToString(fparam->dest,
				(DWORD)SendMessage(PES->hWnd, EM_GETFIRSTVISIBLELINE, 0, 0) + 1);
			break;

		case 'f': // flags, findtext
			if ( tstrcmp(param + 1, T("indtext")) == 0 ){ // findtext
				tstrcpy(fparam->dest, (PES->findrep != NULL) ? PES->findrep->findtext : NilStr);
			}else{ // flags
				Numer32ToString(fparam->dest, PES->flags);
			}
			break;

		case 'h': // history
			PutOptionFlags(fparam->dest, PES->list.RhistID, Hist_flags, TSIZEOFSTR(Hist_flags));
			break;

		case 'l': // list / line,lines,loglines / liststatus
			if ( tstrcmp(param + 1, T("ist")) == 0 ){ // list
				Int32ToString(fparam->dest, PES->list.ListFocus);
			}else if ( tstrcmp(param + 1, T("iststatus")) == 0 ){ // liststatus
				thprintf(fparam->dest, CMDLINESIZE,
					T("list1: %s   list2: %s\n")
					T("focus: %d\n")
					T("threads: %d/") T(DefineToStr(LIST_THREAD_MAX)) T("\n")
					T("user list: %s"),
					(PES->list.hWnd != NULL) ? T("use") : T("close"),
					(PES->list.hSubWnd != NULL) ? T("use") : T("close"),
					PES->list.ListFocus,
					PES->ListThreadCount,
					(PES->list.filltext_user.mem != NULL) ? T("on") : T("off")
				);
			}else if ( tstrcmp(param + 1, T("oglines")) == 0 ){ // loglines
				Numer32ToString(fparam->dest, (DWORD)PPeGetLogicalPos(PES, 4) + 1);
			}else { // line,lines
				Numer32ToString(fparam->dest, (DWORD)SendMessage(PES->hWnd, EM_GETLINECOUNT, 0, 0));
			}
			break;

		case 'm': // modify / mode
			if ( tstrcmp(param, T("modify")) == 0 ){ // modify
				Numer32ToString(fparam->dest, (DWORD)SendMessage(PES->hWnd, EM_GETMODIFY, 0, 0));
			}else{ // mode
				fparam->dest[0] = (TCHAR)((PES->flags & PPXEDIT_TEXTEDIT) ? 'T' : 'L');
				fparam->dest[1] = (TCHAR)((PES->flags & PPXEDIT_LINE_MULTI) ? 'M' : ' ');
				Numer32ToString(fparam->dest + 2, PES->flags);
			}
			break;

		case 'n': // name
			ThGetString(&PES->LocalStringValue, LSV_filename, fparam->dest, VFPS);
			break;

		case 'p': // pagex / pagey
			Numer32ToString(fparam->dest,
					GetPageInfo(PES, (tstrcmp(param, T("pagex")) != 0)) );
			break;

		case 'r':
			if ( tstrcmp(param + 1, T("eplacetext")) == 0 ){ // replacetext
				tstrcpy(fparam->dest, (PES->findrep != NULL) ? PES->findrep->replacetext : NilStr);
			}else{ // returncode
				tstrcpy(fparam->dest, CRCD_list[PES->CrCode]);
			}
			break;

		case 's': // start / startline / startlogx startlogy
		case 'e': // end / endline
			if ( tstrcmp(param + 1, T("tartlogcolumn")) == 0 ){ // startlogcolumn
				wParam = PPeGetLogicalPos(PES, 0) + 1;
			}else if ( tstrcmp(param + 1, T("tartlogline")) == 0 ){ // startlogline
				wParam = PPeGetLogicalPos(PES, 1) + 1;
			}else if ( tstrcmp(param + 1, T("ndlogcolumn")) == 0 ){ // endlogcolumn
				wParam = PPeGetLogicalPos(PES, 2) + 1;
			}else if ( tstrcmp(param + 1, T("ndlogline")) == 0 ){ // endlogline
				wParam = PPeGetLogicalPos(PES, 3) + 1;
			}else if ( tstrcmp(param, T("startlogy")) == 0 ){
				wParam = PPeGetLogicalPos(PES, 1);
			}else if ( tstrcmp(param, T("startlogx")) == 0 ){
				wParam = PPeGetLogicalPos(PES, 0);
			}else if ( tstrcmp(param, T("style")) == 0 ){
				wParam = PES->style;
			}else if ( tstrcmp(param, T("edittype")) == 0 ){
				if ( PES->flags & PPXEDIT_COMBOBOX ){
					tstrcpy(fparam->dest, T("combobox"));
				}else if ( PES->flags & PPXEDIT_RICHEDIT ){
					tstrcpy(fparam->dest, T("richedit"));
				}else{
					tstrcpy(fparam->dest, T("edit"));
				}
				break;
			}else if ( tstrcmp(param, T("exstyle")) == 0 ){
				wParam = PES->exstyle;
			}else{ // start / end
				SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&wParam, (LPARAM)&lParam);
				if ( param[0] == 'e' ) wParam = lParam;
				if ( tstrchr(param, 'l') != NULL ){
					wParam = SendMessage(PES->hWnd, EM_LINEFROMCHAR, wParam, 0) + 1;
				}
			}
			Numer32ToString(fparam->dest, (DWORD)wParam);
			break;

		case 't': // tab
			Numer32ToString(fparam->dest, PES->tab);
			break;

		case 'w': // whistory
			PutOptionFlags(fparam->dest, PES->list.WhistID, Hist_flags, TSIZEOFSTR(Hist_flags));
			break;

		default:
			tstrcpy(fparam->dest, T("?"));
	}
}

#define LineBreak_TOGGLE	-1 // toggle
#define LineBreak_NOWRAP	0 // off
#define LineBreak_CHAR	1 // on / char
#define LineBreak_WORD	2 // word
void ChangeLineBreak(PPxEDSTRUCT *PES, int mode)
{
	PPxEDSTRUCT *NewPES;
	HWND hNewWnd, hParentWnd;
	UINT newstyle;
	RECT box;
	EditTextStruct ets;

	if ( PES->flags & PPXEDIT_COMBOBOX ) return; // コンボボックスは不可

	if ( mode < 0 ){ // LineBreak_TOGGLE
		mode = (PES->style & ES_AUTOHSCROLL) ? LineBreak_CHAR : LineBreak_NOWRAP;
	}
	if ( mode == LineBreak_NOWRAP ){
		if ( PES->style & ES_AUTOHSCROLL ) return; // 変更不要
		newstyle = PES->style | ES_AUTOHSCROLL | WS_HSCROLL;
	}else{
		if ( !(PES->style & ES_AUTOHSCROLL) ) return; // 変更不要
		newstyle = PES->style & ~(ES_AUTOHSCROLL | WS_HSCROLL);
	}
	// 情報待避
	hParentWnd = GetParent(PES->hWnd);
	GetClientRect(hParentWnd, &box);

	hNewWnd = CreateWindowEx(PES->exstyle, EditClassName, NilStr, newstyle,
			 box.left, box.top, box.right - box.left, box.bottom - box.top,
			 hParentWnd, CHILDWNDID(IDE_PPEMAIN), DLLhInst, 0);
	if ( hNewWnd == NULL ) return; // 変更に失敗

	if ( SendMessage(hParentWnd, WM_PPXCOMMAND, KE_ChangeHWND, (LPARAM)hNewWnd) != (LRESULT)hNewWnd ){ // 変更に対応しているか？
		DestroyWindow(hNewWnd);
		return;
	}

	// 新Windowを設定
	NewPES = HeapAlloc(DLLheap, 0, sizeof(PPxEDSTRUCT));
	if ( NewPES == NULL ) {
		DestroyWindow(hNewWnd);
		return; // 失敗
	}
	*NewPES = *PES;

	// 設定の移植
	FixUxTheme(hNewWnd, EditClassName);
	SendMessage(hNewWnd, WM_SETFONT, (WPARAM)
			SendMessage(PES->hWnd, WM_GETFONT, 0, 0), 0);

	NewPES->style = newstyle;
	NewPES->hWnd = hNewWnd;
	SendMessage(hNewWnd, EM_LIMITTEXT, SendMessage(PES->hWnd, EM_GETLIMITTEXT, 0, 0), 0);
	InitExEdit(NewPES);

	// テキストの移植
	if ( IsTrue(OpenEditText(PES, &ets, 0)) ){
		SetWindowText(hNewWnd, ets.text);
		CloseEditText(&ets);
	}else{
		DestroyWindow(hNewWnd);
		HeapFree(DLLheap, 0, (void*)NewPES);
		return; // 失敗
	}

	// 拡張部分が機能するように最終設定
	SendMessage(hNewWnd, EM_SETMODIFY,
			SendMessage(PES->hWnd, EM_GETMODIFY, 0, 0), 0);
	NewPES->hOldED = (WNDPROC)
			SetWindowLongPtr(hNewWnd, GWLP_WNDPROC, (LONG_PTR)EDsHell);

	ShowWindow(hNewWnd, SW_SHOW);

	// 旧Windowを破棄
	PostMessage(PES->hWnd, WM_PPXCOMMAND, KE_scrapwindow, 0);
}

#if 0
void EditKeyCommand(PPxEDSTRUCT *PES, const TCHAR *param) // *key
{
	TCHAR buf[CMDLINESIZE];
	const TCHAR *ptr;
	int key;

	for ( ;; ){
		if ( GetCommandParameter(&param, buf, TSIZEOF(buf)) == 0 ) return;
		if ( tstrcmp(buf, "remove") == 0 ){
			RemoveCharKey(PES->hWnd);
			return;
		}
		ptr = buf;
		key = GetKeyCode(&ptr);
		if ( key > 0 ) {
			if ( key == K_cr ) key = '\n';
			if ( key & K_v ) {
				CallWindowProc(PES->hOldED, PES->hWnd, WM_KEYDOWN, key & 0xff, 0);
				CallWindowProc(PES->hOldED, PES->hWnd, WM_KEYUP, key & 0xff, B30 | B31);
			}else{
				CallWindowProc(PES->hOldED, PES->hWnd, WM_CHAR, key & 0xff, 0);
			}
		}
	}
}
#endif

void EditSetModeCommand(PPxEDSTRUCT *PES, const TCHAR *param) // *editmode
{
	TCHAR buf[CMDLINESIZE], code, *more;
	EDITMODESTRUCT ems = {EDITMODESTRUCT_DEFAULT};

	for ( ;; ){
		code = GetOptionParameter(&param, buf, &more);
		if ( code == '\0' ) break;
		if ( code == '-' ){
			if ( IsTrue(EditModeParam(&ems, buf + 1, more)) ){
				if ( ems.codepage > 0 ) PES->CharCP = ems.codepage;
				if ( ems.crcode >= 0 ) PES->CrCode = ems.crcode;
				if ( ems.tabwidth >= 0 ){
					PPeSetTab(PES, ems.tabwidth);
					ems.tabwidth = -1;
				}
			}else if ( !tstrcmp(buf + 1, T("LEAVECANCEL")) ){
				SendMessage(GetParent(PES->hWnd), WM_PPXCOMMAND, KE_setLeaveCancel, 1);
			}else if ( !tstrcmp(buf + 1, T("LINEBREAK")) ){
				if ( SkipSpace((const TCHAR **)&more) < ' ' ){
					ChangeLineBreak(PES, LineBreak_TOGGLE);
				}else if ( *more == 'c' ){
					ChangeLineBreak(PES, LineBreak_CHAR);
//				}else if ( *more == 'w' ){
//					ChangeLineBreak(PES, LineBreak_WORD);
				}else{
					ChangeLineBreak(PES,
							((*more == 'o') && (*(more + 1) == 'f')) ?
							LineBreak_NOWRAP : LineBreak_CHAR);
				}
			}else if ( !tstrcmp(buf + 1, T("SYNTAX")) ){
				if ( PES->flags & PPXEDIT_RICHEDIT ){
					if ( *more == '\0' ){
						PES->flags ^= PPXEDIT_SYNTAXCOLOR;
						if ( PES->flags & PPXEDIT_SYNTAXCOLOR ){
							InitSynColor(PES);
							RichSyntaxColor(PES);
						}
					}else if ( tstrcmp(more, T("clear")) == 0 ){
						CHARFORMAT2 chfmt;

						resetflag(PES->flags, PPXEDIT_SYNTAXCOLOR);
						chfmt.cbSize = sizeof(chfmt);
						chfmt.dwMask = CFM_COLOR | CFM_BACKCOLOR;
						chfmt.dwEffects = 0;
						chfmt.crTextColor = C_WindowText;
						chfmt.crBackColor = C_WindowBack;
						SendMessage(PES->hWnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&chfmt);
					}else if ( tstrcmp(more, T("on")) == 0 ){
						if ( !(PES->flags & PPXEDIT_SYNTAXCOLOR) ){
							InitSynColor(PES);
						}
						RichSyntaxColor(PES);
					}else if ( tstrcmp(more, T("off")) == 0 ){
						resetflag(PES->flags, PPXEDIT_SYNTAXCOLOR);
					}else if ( tstrcmp(more, T("once")) == 0 ){
						RichSyntaxColor(PES);
					}else{
						BOOL result;
						TINPUT_EDIT_OPTIONS options;

						if ( !(PES->flags & PPXEDIT_SYNTAXCOLOR) ){
							InitSynColor(PES);
						}
						result = GetEditMode((const TCHAR **)&more, &options);
						if ( IsTrue(result) ){
							PES->SyntaxType = HistWriteTypeflag[options.hist_writetype];
						}
						RichSyntaxColor(PES);
					}
				}
			}else if ( !tstrcmp(buf + 1, T("TABKEY")) ){
				setflag(PES->flags, PPXEDIT_TABCOMP);
				if ( (*more == 'o') && (*(more + 1) == 'f') ){
					resetflag(PES->flags, PPXEDIT_TABCOMP);
				}
			}else if ( !tstrcmp(buf + 1, T("ALLKEY")) ){
				setflag(PES->flags, PPXEDIT_WANTALLKEY);
				if ( (*more == 'o') && (*(more + 1) == 'f') ){
					resetflag(PES->flags, PPXEDIT_WANTALLKEY);
				}
			}else if ( !tstrcmp(buf + 1, T("MODIFY")) ){
				switch ( SkipSpace((const TCHAR **)&more) ){
					case 'q': // query
						resetflag(PES->flags, PPXEDIT_DISABLE_CHECKSAVE);
						break;

					case 's':
						if ( more[1] == 'a' ){ // save
							setflag(PES->flags, PPXEDIT_DISABLE_CHECKSAVE | PPXEDIT_SAVE_BYCLOSE);
						}else{ // silent / save
							setflag(PES->flags, PPXEDIT_DISABLE_CHECKSAVE);
						}
						break;

					case 'w': // write
						if ( PES->style & ES_READONLY ){
							SendMessage(PES->hWnd, EM_SETREADONLY, FALSE, 0);
							resetflag(PES->style, ES_READONLY);
						}
						break;

					case 'r': // readonly
						if ( !(PES->style & ES_READONLY) ){
							SendMessage(PES->hWnd, EM_SETREADONLY, TRUE, 0);
							setflag(PES->style, ES_READONLY);
						}
						break;

					case 'c': // clear
						XEditClearModify(PES);
						break;

					default: // modify
						XEditSetModify(PES);
				}
			}else if ( !tstrcmp( buf + 1, T("HISTORY")) ){
				if ( SetHistorySettings(PES, more) ){
					SendMessage(GetParent(PES->hWnd), WM_PPXCOMMAND, KE_setWType, (LPARAM)PES->list.WhistID );
				}else{
					break;
				}
			}else{
				XMessage(PES->hWnd, NULL, XM_GrERRld, MES_EUOP, buf);
			}
			continue;
		}
		if ( SetHistorySettings(PES, buf) ){
			SendMessage(GetParent(PES->hWnd), WM_PPXCOMMAND, KE_setWType, (LPARAM)PES->list.WhistID );
		}else{
			break;
		}
	}
}

void GetCursorLocate(PPxEDSTRUCT *PES, DWORD *start, DWORD *end)
{
	ECURSOR cursor = {0, 0};

#ifdef UNICODE
	SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&cursor.start, (LPARAM)&cursor.end);
#else
	EditTextStruct ets;

	if ( (xpbug < 0) && OpenEditText(PES, &ets, 0) ){
		GetEditSel(PES->hWnd, ets.text, &cursor);
		CloseEditText(&ets);
	}else{
		SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&cursor.start, (LPARAM)&cursor.end);
	}
#endif
	if ( start != NULL ) *start = cursor.start;
	if ( end != NULL ) *end = cursor.end;
}

ERRORCODE EditPPeKeyCommand(PPxEDSTRUCT *PES, DWORD key)
{
	PPECOMMANDPARAM param;
	ERRORCODE result;

	param.key = (WORD)(DWORD)key;
	param.repeat = 0;

	result = PPedCommand(PES, &param);
	if ( (result == ERROR_INVALID_FUNCTION) &&
		 ((key & (K_v | K_ex | K_internal)) == K_v) ) { // 未実行の仮想キー？
		CallWindowProc(PES->hOldED, PES->hWnd, WM_KEYDOWN, key & 0xff, 0);
		CallWindowProc(PES->hOldED, PES->hWnd, WM_KEYUP, key & 0xff, B30 | B31);
	}
	return result;
}

// 一行編集時に使用する
DWORD_PTR USECDECL EditInfoFunc(PPXAPPINFO *info, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	PPxEDSTRUCT *PES;

	PES = ((EDITMODULEINFOSTRUCT *)info)->PES;
	switch (cmdID){
		case PPXCMDID_PPXCOMMAD:
			EditPPeKeyCommand(PES, uptr->key);
			return 0;

		case PPXCMDID_REQUIREKEYHOOK:
			PES->KeyHookEntry = FUNCCAST(CALLBACKMODULEENTRY, uptr);
			break;

		case PPXCMDID_POPUPPOS:
			GetPPePopupPositon(PES, (POINT *)uptr);
			break;

		case PPXCMDID_SETPOPLINE:
			if ( *uptr->str ){
				SetMessageForEdit(PES->hWnd, uptr->str);
			}else{
				SetMessageForEdit(PES->hWnd, NULL);
			}
			break;

		case PPXCMDID_CSRX:
			GetCursorLocate(PES, &uptr->num, NULL);
			break;

		case PPXCMDID_CSRY:
			GetCursorLocate(PES, NULL, &uptr->num);
			break;

		case PPXCMDID_CSRLOCATE:
			GetCursorLocate(PES, &uptr->nums[0], &uptr->nums[1]);
			uptr->nums[2] = uptr->nums[3] = 0;
			break;

		case PPXCMDID_CSRSETLOCATE: {
			int wp, lp;
			wp = uptr->nums[0];
			if ( wp < 0 ) wp = EC_LAST;
			lp = uptr->nums[1];
			if ( lp < 0 ){
				lp = (lp == -2) ? wp : EC_LAST;
			}
			SendMessage(PES->hWnd, EM_SETSEL, (WPARAM)uptr->nums[0], (LPARAM)lp );
			break;
		}

		case PPXCMDID_CSRRECT:
			GetPPePopupPositon(PES, (POINT *)uptr);
			uptr->nums[2] = 1;
			uptr->nums[3] = PES->fontY;
			break;

		case PPXCMDID_GETREQHWND:
			if ( uptr != NULL ){
				switch ( uptr->str[0] ){
					case '\0':
						return (DWORD_PTR)PES->hWnd;

					case 'F':
						return (DWORD_PTR)PES->FloatBar.hWnd;

					case 'L':
						return (uptr->str[1] == 'S') ? (DWORD_PTR)PES->list.hSubWnd : (DWORD_PTR)PES->list.hWnd;
					case 'P':
						if ( (PES->info == NULL) || (PES->info->Function == NULL) ) return (DWORD_PTR)NULL;
						return PPxInfoFunc(PES->info, PPXCMDID_GETREQHWND, (TCHAR *)(uptr->str + 1) );
					case 'T':
						return (DWORD_PTR)PES->hTreeWnd;
				}
			}
			return (DWORD_PTR)NULL;

		case PPXCMDID_MOVECSR:
			return EditCursorCommand(PES, uptr);

		case PPXCMDID_GETCONTROLVARIABLESTRUCT:
			return (DWORD_PTR)&PES->LocalStringValue;

		case PPXCMDID_FUNCTION:
			if ( !tstrcmp(uptr->funcparam.param, T("TEXTPROP")) ){
				EditGetTextProperties(PES, &uptr->funcparam);
				return PPXA_NO_ERROR;
			}
			if ( !tstrcmp(uptr->funcparam.param, T("EDITPROP")) ){
				EditGetPropFunction(PES, &uptr->funcparam);
				return PPXA_NO_ERROR;
			}
			if ( !tstrcmp(uptr->funcparam.param, T("CURSORTEXT")) ){
				CursorTextFunction(PES, &uptr->funcparam);
				return PPXA_NO_ERROR;
			}
			if ( !tstrcmp(uptr->funcparam.param, T("EDITTEXT")) ){
				int len;
				len = Edit_GetWindowTextLength(PES);
				if ( len >= CMDLINESIZE ){
					TCHAR *longbuf;
					longbuf = HeapAlloc(ProcHeap, 0, TSTROFF(len + 1));
					if ( longbuf != NULL ){
						uptr->funcparam.dest = longbuf;
					}else{
						len = CMDLINESIZE - 1;
					}
				}
				Edit_GetWindowText(PES, uptr->funcparam.dest, len + 1);
				uptr->funcparam.dest[len] = '\0';
				return PPXA_NO_ERROR;
			}
			if ( !tstrcmp(uptr->funcparam.param, T("RTFTEXT")) ){
				return EditRtfTextFunction(PES, &uptr->funcparam);
			}
			if ( (PES->info == NULL) || (PES->info->Function == NULL) ){
				return PPXA_INVALID_FUNCTION;
			}
			return PPxInfoFunc(PES->info, cmdID, uptr);

		case PPXCMDID_COMMAND:{
			const TCHAR *param = uptr->str + tstrlen(uptr->str) + 1;
#if 0
			if ( !tstrcmp(uptr->str, T("KEY")) ){
				EditKeyCommand(PES, param);
				break;
			}
#endif
			if ( !tstrcmp(uptr->str, T("TREE")) ){
				EditTreeCommand(PES, param);
				break;
			}
/*
			if ( !tstrcmp(uptr->str, T("COLOR")) ){
				EditColorCommand(PES, param);
				break;
			}
*/
			if ( !tstrcmp(uptr->str, T("EDITMODE")) ){
				EditSetModeCommand(PES, param);
				break;
			}
			if ( !tstrcmp(uptr->str, T("TEXTPROP")) ){
				EditSetTextProperties(PES, param);
				break;
			}
			if ( !tstrcmp(uptr->str, T("RTFTEXT")) ){
				EditInsertRtf(PES, param);
				break;
			}
			if ( !tstrcmp(uptr->str, T("JUMPLINE")) ){
				EditJumpLineCommand(PES, param);
				break;
			}
			if ( !tstrcmp(uptr->str, T("SETCAPTION")) ){
				SetWindowText(GetParentCaptionWindow(PES->hWnd), param);
				break;
			}
			if ( !tstrcmp(uptr->str, T("COMPLETELIST")) ){
				EditCompleteListCommand(PES, param);
				break;
			}
			if ( !tstrcmp(uptr->str, T("DEFAULTMENU")) ){
				PPeDefaultMenu(PES);
				break;
			}
			if ( !tstrcmp(uptr->str, T("REPLACEFILE")) ){
				OpenFromFile(PES, PPE_OPEN_MODE_CMDOPEN, param);
				break;
			}
			if ( !tstrcmp(uptr->str, T("INSERTFILE")) ){
				OpenFromFile(PES, PPE_OPEN_MODE_CMDINSERT, param);
				break;
			}
			if ( !tstrcmp(uptr->str, T("ZENHAN")) ){
				int mode = -1;
				TCHAR c = *param;

				if ( (c == 'z') || (c == '1') ) mode = 1;
				if ( (c == 'h') || (c == '0') ) mode = 0;
				PPeConvertZenHanMain(PES, PES->hWnd, mode);
				break;
			}
			if ( !tstrcmp(uptr->str, T("FIND")) ){
				PPeFind(PES, param);
				break;
			}
			// default へ
		}

		default:
			if ( (LONG_PTR)PES->info < 0x10000 ){
				if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
				return PPXA_INVALID_FUNCTION;
			}
			if ( (PES->info == NULL) || (PES->info->Function == NULL) ){
				return PPXA_INVALID_FUNCTION;
			}
			return PPxInfoFunc(PES->info, cmdID, uptr);
	}
	return PPXA_NO_ERROR;
}

int CallCloseEvent(PPxEDSTRUCT *PES)
{
	EDITMODULEINFOSTRUCT ppxa;
	PPXMODULEPARAM pmp = {NULL};

	ppxa.info.Name = EditInfoName;
	ppxa.info.RegID = PES->info->RegID;
	ppxa.info.Function = (PPXAPPINFOFUNCTION)EditInfoFunc;
	ppxa.info.hWnd = PES->hWnd;
	ppxa.PES = PES;

	return CallModule(&ppxa.info, PPXMEVENT_DESTROY, pmp, NULL);
}

int CallEditKeyHook(PPxEDSTRUCT *PES, WORD key)
{
	PPXMKEYHOOKSTRUCT keyhookinfo;
	PPXMODULEPARAM pmp;
	EDITMODULEINFOSTRUCT ppxa;

	keyhookinfo.key = key;
	pmp.keyhook = &keyhookinfo;
	ppxa.info.Name = EditInfoName;
	ppxa.info.RegID = PES->info->RegID;
	ppxa.info.Function = (PPXAPPINFOFUNCTION)EditInfoFunc;
	ppxa.info.hWnd = PES->hWnd;
	ppxa.PES = PES;

	return CallModule(&ppxa.info, PPXMEVENT_KEYHOOK, pmp, PES->KeyHookEntry);
}

ERRORCODE EditExtractMacro(PPxEDSTRUCT *PES, const TCHAR *param, TCHAR *extract, int flags)
{
	EDITMODULEINFOSTRUCT ppxa;

	ppxa.info.Name = EditInfoName;
	ppxa.info.RegID = PES->info->RegID;
	ppxa.info.Function = (PPXAPPINFOFUNCTION)EditInfoFunc;
	ppxa.info.hWnd = PES->hWnd;
	ppxa.PES = PES;
	return PP_ExtractMacro(ppxa.info.hWnd, &ppxa.info, NULL, param, extract, flags);
}

LRESULT CmdSelText(PPxEDSTRUCT *PES, TCHAR *strbuf)
{
	TEXTSEL ts;
	size_t wordlen;

	if ( SelectEditStrings(PES, &ts, TEXTSEL_ALL) == FALSE ) return NO_ERROR;
	wordlen = tstrlen(ts.word);
	if ( wordlen >= (CMDLINESIZE - 1) ){
		TCHAR *longparam;

		wordlen = (wordlen + 1) * sizeof(TCHAR);
		longparam = HeapAlloc(ProcHeap, 0, wordlen );
		if ( longparam != NULL ) memcpy(longparam, ts.word, wordlen);
		FreeTextselStruct(&ts);
		return (LRESULT)longparam;
	}
	tstrcpy(strbuf, ts.word);
	FreeTextselStruct(&ts);
	return NO_ERROR;
}

HMENU MakePopupMenus(PPXINMENU *menus, DWORD check)
{
	HMENU hMenu;
	TCHAR strbuf[0x200];

	hMenu = CreatePopupMenu();
	while ( menus->key ){
		if ( menus->str != NULL ){
			const TCHAR *str;

			if ( *menus->str == '%' ){
				PP_ExtractMacro(NULL, NULL, NULL, menus->str, strbuf, 0);
				str = strbuf;
			}else{
				str = MessageText(menus->str);
			}

			AppendMenu(hMenu,
					((menus->key == check) ? MF_CHECKED : 0) |
					((menus->key & INMENU_BREAK) ? MF_ES | MF_MENUBARBREAK : MF_ES),
					menus->key & ~INMENU_BREAK, str);
		}else{
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		}
		menus++;
	}
	return hMenu;
}

void GetPPePopupPositon(PPxEDSTRUCT *PES, POINT *pos)
{
	if ( PES->mousepos ){
		GetCursorPos(pos);
	}else{
		GetCaretPos(pos);
		pos->y += PES->fontY;
		ClientToScreen(PES->hWnd, pos);
	}
}

void PPeMenu(PPxEDSTRUCT *PES, PPECOMMANDPARAM *param, PPXINMENU *menus)
{
	POINT pos;
	HMENU popup;
	WORD key;

	GetPPePopupPositon(PES, &pos);
	popup = MakePopupMenus(menus, 0);
	if ( menus == amenu ){
		AppendMenu(popup, MF_EPOP, (UINT_PTR)MakePopupMenus(amenu2, 0), amenu2str);
	}
	if ( menus == f2menu ){
		if ( !(PES->style & ES_AUTOHSCROLL) ){
			CheckMenuItem(popup, KE_2p, MF_BYCOMMAND | MF_CHECKED );
		}
		if ( PES->flags & PPXEDIT_SYNTAXCOLOR ){
			CheckMenuItem(popup, K_s | K_c | 'G', MF_BYCOMMAND | MF_CHECKED );
		}
		if ( X_esrx ) CheckMenuItem(popup, KE_2x, MF_BYCOMMAND | MF_CHECKED );
		if ( PES->style & ES_READONLY ){
			CheckMenuItem(popup, KE_qv, MF_BYCOMMAND | MF_CHECKED );
		}
	}
	key = (WORD)TrackPopupMenu(popup, TPM_TDEFAULT, pos.x, pos.y, 0, PES->hWnd, NULL);
	DestroyMenu(popup);
	if ( key != 0 ){
		param->key = (WORD)(key | K_raw);
		PPedCommand(PES, param);
	}
}

void USEFASTCALL PPeClip(PPxEDSTRUCT *PES, UINT mode)
{
	ECURSOR cursor = {0, 0};
	TCHAR *text;
	DWORD len;
	GETTEXTEX textex;

	if ( !(PES->flags & PPXEDIT_RICHEDIT) ){
		CallWindowProc(PES->hOldED, PES->hWnd, mode, 0, 0);
		return;
	}

	SendMessage(PES->hWnd, EM_GETSEL, (WPARAM)&cursor.start, (LPARAM)&cursor.end);

	if ( cursor.start == cursor.end ) return;
/*
	if ( cursor.start == cursor.end ){
		len = GetWindowTextLength(PES->hWnd) + 1;
		text = HeapAlloc(DLLheap, 0, TSTROFF(len));
		if ( text == NULL ) return;
		GetWindowText(PES->hWnd, text, len);
	}
*/
	len = (cursor.end - cursor.start) + 1;
	#ifdef UNICODE
		textex.codepage = CP__UTF16L;
	#else
		textex.codepage = CP_ACP;
		len *= 2; // cursor が文字単位なので、MultiByte のために倍にする
	#endif
	text = HeapAlloc(DLLheap, 0, TSTROFF(len));
	if ( text == NULL ) return;

	textex.cb = TSTROFF(len);
	textex.flags = GT_NOHIDDENTEXT | GT_SELECTION;
	textex.lpDefaultChar = NULL;
	textex.lpUsedDefChar = NULL;
	SendMessage(PES->hWnd, EM_GETTEXTEX, (WPARAM)&textex, (LPARAM)text);

	ClipTextData(PES->hWnd, text);
	HeapFree(DLLheap, 0, text);

	if ( mode == WM_CUT ){
		SETTEXTEX setex;
	#ifdef UNICODE
		setex.flags = ST_KEEPUNDO | ST_SELECTION | ST_UNICODE;
		setex.codepage = CP__UTF16L;
	#else
		setex.flags = ST_KEEPUNDO | ST_SELECTION;
		setex.codepage = CP_ACP;
	#endif
		CallWindowProc(PES->hOldED, PES->hWnd, EM_SETTEXTEX, (WPARAM)&setex, (LPARAM)NilStr);
	}
}

	// ^V
int USEFASTCALL PPePaste(PPxEDSTRUCT *PES)
{
	if ( !(PES->flags & PPXEDIT_TEXTEDIT) && IsTrue(OpenClipboard2(PES->hWnd)) ){
		HGLOBAL hGMem;
		size_t maxlen;
		BOOL dopaste;

		// 一行編集のときは、改行を除去する / ShellIDlist も利用可能に
		dopaste = FALSE;
		maxlen = SendMessage(PES->hWnd, EM_GETLIMITTEXT, 0, 0);
		hGMem = GetClipboardData(CF_TTEXT);
		if ( hGMem != NULL ){
			TCHAR *src, *srcmax, *dest;
			size_t len;
			TCHAR *textp;

			src = GlobalLock(hGMem);
			#pragma warning(suppress: 6011 6387) // GlobalLock は失敗しないと見なす
			len = tstrlen(src);
			if ( len >= maxlen ) len = maxlen;
			textp = HeapAlloc(DLLheap, 0, TSTROFF(len + 1));
			if ( textp != NULL ){
				srcmax = src + len;
				dest = textp;
				while ( src < srcmax ){
					if ( (*src != '\r') && (*src != '\n') ){
						*dest++ = *src;
					}
					if ( src == srcmax ) break;
					src++;
				}
				// richedit か 改行編集した場合は自前ペーストに
				if ( (PES->flags & PPXEDIT_RICHEDIT) ||
					 ((size_t)(dest - textp) < len) ){
					*dest = '\0';
					SendMessage(PES->hWnd, EM_REPLACESEL, TRUE, (LPARAM)textp);
					dopaste = TRUE;
				}

				HeapFree(DLLheap, 0, textp);
			}
			GlobalUnlock(hGMem);
		}else if ( (hGMem = GetClipboardData(RegisterClipboardFormat(CFSTR_SHELLIDLIST))) != NULL ){
			TCHAR text[CMDLINESIZE];

			GetTextFromCF_SHELLIDLIST(text, CMDLINESIZE, hGMem, FALSE);
			SendMessage(PES->hWnd, EM_REPLACESEL, 1, (LPARAM)text);
			dopaste = TRUE;
		}
		CloseClipboard();
		if ( IsTrue(dopaste) ) return 0;
	}
	// 独自処理をしなかったのでもとの処理を行う
	CallWindowProc(PES->hOldED, PES->hWnd, WM_PASTE, 0, 0);
	return 0;
}

		// ^S
int USEFASTCALL PPeSaveFile(PPxEDSTRUCT *PES)
{
	FileSave(PES, EDL_FILEMODE_NODIALOG);
	return 0;
}
		// F12 , ESC-S
int USEFASTCALL PPeSaveAsFile(PPxEDSTRUCT *PES)
{
	FileSave(PES, EDL_FILEMODE_DIALOG);
	return 0;
}

int USEFASTCALL PPePPcListMain(PPxEDSTRUCT *PES, int mincount)
{
	HMENU hMenu;
	int count, id;
	DWORD id2 = 1;
	RECT box;
	TCHAR path[VFPS + 8];

	hMenu = CreatePopupMenu();
	count = GetPPxList(hMenu, GetPPcList_Path, NULL, &id2);
	if ( count >= mincount ){
		GetWindowRect(PES->hWnd, &box);
		id = TrackPopupMenu(hMenu, TPM_TDEFAULT, box.left, box.bottom, 0, PES->hWnd, NULL);
		if ( id ){
			TCHAR *sep;

			path[0] = '\0';
			GetMenuString(hMenu, id, path, TSIZEOF(path), MF_BYCOMMAND);
			if ( PES->flags & PPXEDIT_SINGLEREF ){
				SendMessage(PES->hWnd, EM_SETSEL, 0, EC_LAST);
			}
			sep = tstrchr(path, ':'); // "&A: path"
			if ( sep != NULL ){
				SendMessage(PES->hWnd, EM_REPLACESEL, 1, (LPARAM)(sep + 2));
			}
		}
	}
	DestroyMenu(hMenu);
	return 0;
}

int USEFASTCALL PPePPcList(PPxEDSTRUCT *PES)
{
	return PPePPcListMain(PES, 1);
}

int USEFASTCALL PPePPcListMin3(PPxEDSTRUCT *PES)
{
	return PPePPcListMain(PES, 3);
}

			// ESC-A
int USEFASTCALL PPeAppendFile(PPxEDSTRUCT *PES)
{
	FileSave(PES, EDL_FILEMODE_APPEND);
	return 0;
}

			// ^O , ESC-O
int USEFASTCALL PPeOpenFileCmd(PPxEDSTRUCT *PES)
{
	FileOpen(PES, PPE_OPEN_MODE_OPEN);
	return 0;
}

// ESC-I
int USEFASTCALL PPeInsertFile(PPxEDSTRUCT *PES)
{
	FileOpen(PES, PPE_OPEN_MODE_INSERT);
	return 0;
}

							// ESC-C
int USEFASTCALL PPeCloseFile(PPxEDSTRUCT *PES)
{
	HWND hWnd = PES->hWnd;

	if ( EdPPxWmCommand(PES, hWnd, KE_closecheck, 0) ){
		SendMessage(hWnd, EM_SETMODIFY, FALSE, 0);
		PostMessage(GetParentCaptionWindow(hWnd), WM_CLOSE, 0, 0);
	}
	return 0;
}
//----------------------------------------------- Tab/Ins によるファイル名補完
int USEFASTCALL PPeFillMain(PPxEDSTRUCT *PES)
{
	EditTextStruct ets;
	ECURSOR cursor;
	TCHAR *ptr;
	DWORD mode;
	HWND hWnd;

	mode = PES->ED.cmdsearch & CMDSEARCH_MATCHFLAGS;

	if ( OpenEditText(PES, &ets, 0) == FALSE ) return 0;
	if ( PES->flags & PPXEDIT_RICHEDIT ){
		tstrreplace(ets.text, T("\r\n"), T("\n"));
	}
												// 編集文字列(全て)を取得
	hWnd = PES->hWnd;
	GetEditSel(hWnd, ets.text, &cursor);
	SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
	mode |= ((PES->list.WhistID & PPXH_COMMAND) ?
					CMDSEARCH_CURRENT : CMDSEARCH_OFF) |
			((PES->flags & PPXEDIT_SINGLEREF) ? 0 : CMDSEARCH_MULTI) |
			CMDSEARCH_EDITBOX;

	if ( (PES->list.WhistID & (PPXH_DIR | PPXH_PPCPATH)) &&
		 IsTrue(GetCustDword(T("X_fdir"), TRUE)) ){
		setflag(mode, CMDSEARCH_DIRECTORY);
	}

	ptr = SearchFileIned(&PES->ED, ets.text, &cursor, mode);
	if ( ptr != NULL ){
		size_t len;

		SendMessage(hWnd, EM_SETSEL, cursor.start, cursor.end);
								// ※↑SearchFileIned 内で加工済み
		SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)ptr);
		SendMessage(hWnd, EM_SETSEL, 0, 0);	// 表示開始桁を補正させる

		len = tstrlen(ptr);
		if ( len && (*(ptr + len - 1) == '\"') ) len--;
#ifndef UNICODE
		if ( xpbug < 0 ) CaretFixToW(ptr, (DWORD *)&len);
#endif
		SendMessage(hWnd, EM_SETSEL, cursor.start + len, cursor.start + len);
		SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(hWnd, NULL, FALSE);
		SetMessageForEdit(PES->hWnd, NULL);
	}else{
		SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
		SetMessageForEdit(PES->hWnd, MES_EENF);
	}
	CloseEditText(&ets);
	return 0;
}

int USEFASTCALL PPeFillInit(PPxEDSTRUCT *PES)
{
	if ( PES->flags & PPXEDIT_LISTCOMP ){
		if ( PES->list.mode >= LIST_FILL ){
			if ( PES->list.ListFocus != LISTU_NOLIST ){
				PES->oldkey2 = 1;
				if ( ListUpDown(PES->hWnd, PES, 1, 0) == FALSE ) return 0;
				return 0;
			}
		}
		FloatList(PES, EDITDIST_NEXT_FILL);
		return 0;
	}
	// 一覧無し補完
	return PPeFillMain(PES);
}

int USEFASTCALL PPeFillIns(PPxEDSTRUCT *PES)
{
	resetflag(PES->ED.cmdsearch, CMDSEARCH_FLOAT);
	return PPeFillInit(PES);
}

int USEFASTCALL PPeTabChar(PPxEDSTRUCT *PES)
{
	if ( !(PES->flags & PPXEDIT_TABCOMP) ) return 1; // TAB割当て機能無効
	return 0;
}

int USEFASTCALL PPeShiftTab(PPxEDSTRUCT *PES)
{
	if ( PES->flags & (PPXEDIT_TEXTEDIT | PPXEDIT_LINE_MULTILINE) ) return 1;	// マルチなら本来のTAB
	if ( !(PES->flags & PPXEDIT_TABCOMP) ){
		HWND hWnd = PES->hWnd;

		if ( PES->flags & PPXEDIT_LINE_MULTI ) return 1; // 複数行一行編集のとき
		SetFocus(GetNextDlgTabItem(GetParentCaptionWindow(hWnd), hWnd, TRUE));
		return 0;
	}
	if ( (PES->list.hWnd != NULL) && (PES->list.ListFocus == LISTU_FOCUSMAIN) ){
		PES->oldkey2 = 1;
		ListUpDown(PES->hWnd, PES, (PES->list.direction >= 0) ? -1 : 1, 0);
		return 0;
	}
	if ( (PES->list.hSubWnd != NULL) && (PES->list.ListFocus == LISTU_FOCUSSUB) ){
		PES->oldkey2 = 1;
		ListUpDown(PES->hWnd, PES, (PES->list.direction >= 0) ? -1 : 1, 0);
		return 0;
	}
	return 0;
}

int USEFASTCALL PPeFillTab(PPxEDSTRUCT *PES)
{
	DWORD X_ltab[2] = Default_X_ltab;

	if ( PES->flags & (PPXEDIT_TEXTEDIT | PPXEDIT_LINE_MULTILINE)){
		if ( PES->flags & PPXEDIT_WANTALLKEY ){
			PostMessage(PES->hWnd, WM_CHAR, (WPARAM)'\t', 0);
		}
		return 1;	// PPeなら本来のTAB
	}
	if ( !(PES->flags & PPXEDIT_TABCOMP) ){
		HWND hWnd = PES->hWnd;

		if ( PES->flags & PPXEDIT_LINE_MULTI ) return 1; // 複数行一行編集のとき
		SetFocus(GetNextDlgTabItem(GetParentCaptionWindow(hWnd), hWnd, FALSE));
		return 0;
	}
	// 補完
	GetCustData(T("X_ltab"), &X_ltab, sizeof(X_ltab));
	if ( X_ltab[1] == 0 ){
		if ( X_flst_part && (X_ltab[0] < 2) ) X_ltab[0] = 2;
		resetflag(PES->ED.cmdsearch, CMDSEARCH_MATCHFLAGS);
		SetMatchMode(PES, X_ltab[0] );
	}
	return PPeFillInit(PES);
}

int USEFASTCALL DeleteSub(PPxEDSTRUCT *PES, int mode, int key)
{
	TEXTSEL ts;

	if ( SelectEditStrings(PES, &ts, mode) == FALSE ) return 1;
	PushTextStack((key == VK_DELETE) ? (TCHAR)B0 : (TCHAR)0, ts.word);
	CallWindowProc(PES->hOldED, PES->hWnd, WM_KEYDOWN, key, 0);
	if ( (key == VK_DELETE) &&
		 (mode != TEXTSEL_CHAR) &&
		 ( (PES->list.mode >= LIST_FILL) ||
		 ((X_flst_mode >= X_fmode_auto1) &&
		  !(PES->flags & PPXEDIT_NOINCLIST) &&
		  (PES->list.hWnd == NULL))) ){
		KeyStepFill(PES, NULL);
		if ( PES->flags & PPXEDIT_SYNTAXCOLOR ) RichSyntaxColor(PES);
	}
	FreeTextselStruct(&ts);
	return 0;
}

int USEFASTCALL PPeBackSpace(PPxEDSTRUCT *PES)
{
	return DeleteSub(PES, TEXTSEL_BACK, VK_BACK);
}

int USEFASTCALL PPeDeleteBackLine(PPxEDSTRUCT *PES)
{
	return DeleteSub(PES, TEXTSEL_BEFORE, VK_BACK);
}

int USEFASTCALL PPeDeleteBackWord(PPxEDSTRUCT *PES)
{
	TEXTSEL ts;

	if ( SelectEditStrings(PES, &ts, TEXTSEL_BEFOREWORD) == FALSE ) return 1;
	PushTextStack((TCHAR)0, ts.word);
	CallWindowProc(PES->hOldED, PES->hWnd, EM_REPLACESEL, 1, (LPARAM)NilStr);
	FreeTextselStruct(&ts);
	return 0;
}

int USEFASTCALL PPeDeleteAfterLine(PPxEDSTRUCT *PES)
{
	return DeleteSub(PES, TEXTSEL_AFTER, VK_DELETE);
}

int USEFASTCALL PPeDeleteAfterWord(PPxEDSTRUCT *PES)
{
	return DeleteSub(PES, TEXTSEL_WORD, VK_DELETE);
}

int USEFASTCALL PPeDelete(PPxEDSTRUCT *PES)
{
	int result = DeleteSub(PES, TEXTSEL_CHAR, VK_DELETE);

	if ( !( PES->flags & PPXEDIT_TEXTEDIT ) || (PES->list.ListFocus != LISTU_NOLIST) ){
		// X_fmode_autoX の自動補完機能付きの一覧表示
		if ( (PES->list.mode >= LIST_FILL) ||
			 ((X_flst_mode >= X_fmode_auto1) &&
			  !(PES->flags & PPXEDIT_NOINCLIST) &&
			  (PES->list.hWnd == NULL)) ){
			KeyStepFill(PES, NULL);
			if ( PES->flags & PPXEDIT_SYNTAXCOLOR ) RichSyntaxColor(PES);
		// 補完候補リストのインクリメンタルサーチ
		}else if ( PES->flags & PPXEDIT_SUGGEST ){
			ListSearch(PES->hWnd, PES, -1);
		}
	}
	return result;
}

int USEFASTCALL PPeGetFileName(PPxEDSTRUCT *PES)
{
	TCHAR buf[VFPS], path[VFPS];
	PPXCMDENUMSTRUCT work;
	HMODULE hCOMDLG32;
	impGetOpenFileName DGetOpenFileName;
	HWND hWnd = PES->hWnd;
	OPENFILENAME ofile = {sizeof(ofile), NULL, NULL, GetFileExtsStr, NULL, 0, 0,
		NULL, VFPS, NULL, 0, NULL, NULL, OFN_HIDEREADONLY | OFN_SHAREAWARE,
		0, 0, WildCard_All, 0, NULL, NULL OPENFILEEXTDEFINE };

	ofile.lpstrTitle = MessageText(MES_TSFN);
	PPxEnumInfoFunc(PES->info, '1', path, &work);
	hCOMDLG32 = LoadSystemDLL(SYSTEMDLL_COMDLG32);
	if ( hCOMDLG32 == NULL ) return 0;
	GETDLLPROCT(hCOMDLG32, GetOpenFileName);
	if ( DGetOpenFileName == NULL ) return 0;
	tstrcpy(buf, NilStr);  // 例えば、d:\\winnt\\*.*
	ofile.hwndOwner = hWnd;
	ofile.lpstrFile = buf;
	ofile.lpstrInitialDir = path;
	if ( !DGetOpenFileName(&ofile) ){
		FreeLibrary(hCOMDLG32);
		return 0;
	}
	FreeLibrary(hCOMDLG32);
	if ( PES->flags & PPXEDIT_SINGLEREF ){
		SendMessage(hWnd, EM_SETSEL, 0, EC_LAST);
	}else{
		DWORD wP, lP;

		SendMessage(hWnd, EM_GETSEL, (WPARAM)&wP, (LPARAM)&lP);
		if ( tstrchr(buf, ' ') != NULL ){ // 空白があるので、「"|"」の|に挿入
			SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)T("\"\""));
			SendMessage(hWnd, EM_SETSEL, wP + 1, wP + 1);
			PostMessage(hWnd, WM_KEYDOWN, VK_RIGHT, 0);
			PostMessage(hWnd, WM_KEYUP, VK_RIGHT, 0);
		}
	}
	SendMessage(hWnd, EM_REPLACESEL, 1, (LPARAM)buf);
	return 0;
}

int USEFASTCALL PPeDuplicate(PPxEDSTRUCT *PES)
{
	EditTextStruct ets;

	OpenEditText(PES, &ets, 0);
	PPEui(PES->hWnd, T("DupText"), ets.text);
	CloseEditText(&ets);
	return 0;
}

			// ^[K][Z] 変換 ----------------------
int USEFASTCALL PPeConvertZenHan(PPxEDSTRUCT *PES)
{
	PPeConvertZenHanMain(PES, PES->hWnd, -1);
	return 0;
}

//----------------------------------------------- ^Q-[U] 大小変換
int USEFASTCALL PPeConvertCase(PPxEDSTRUCT *PES)
{
	TEXTSEL ts;
	TCHAR *q;
	int f = 0;

	if ( SelectEditStrings(PES, &ts, TEXTSEL_WORD) == FALSE ) return 0;
	for ( q = ts.word ; *q ; q++ ){
		if ( Isalpha(*q) ){
			f = Isupper(*q) ? 1 : 2;
			break;
		}
#ifndef UNICODE
		if ( IskanjiA(*q) ){
			if ( (BYTE)*q == 0x82 ){
				BYTE c;
				c = (BYTE)*(q + 1);
				if ( (c >= 0x60) && (c <= 0x79) ){
					f = 1;
					break;
				}else if ( (c >= 0x81) && (c <= 0x9A) ){
					f = 2;
					break;
				}
			}
			q++;
		}
#endif
	}
	if ( f == 1 ) Strlwr(ts.word);
	if ( f == 2 ) Strupr(ts.word);
	SendMessage(PES->hWnd, EM_REPLACESEL, 1, (LPARAM)ts.word);
	SendMessage(PES->hWnd, EM_SETSEL, ts.cursororg.start, ts.cursororg.end);
	FreeTextselStruct(&ts);
	return 0;
}

#ifdef UNICODE
	#define DEF_CHAR_CODE_OPT T("-utf16")
#else
	#define DEF_CHAR_CODE_OPT T("-system")
#endif

int USEFASTCALL PPePrintByPPv(PPxEDSTRUCT *PES)
{
	EditTextStruct ets;
	DWORD size;
	TCHAR filename[VFPS], cmd[CMDLINESIZE];

	MakeTempEntry(VFPS, filename, FILE_ATTRIBUTE_NORMAL);

	if ( IsTrue(OpenEditText(PES, &ets, 0)) && (ets.len > 0) ){
		HANDLE hFile;

		hFile = CreateFileL(filename, GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL);
		if ( hFile != INVALID_HANDLE_VALUE ){
			WriteFile(hFile, ets.text, TSTROFF(ets.len), &size, NULL);
			CloseHandle(hFile);

			thprintf(cmd, TSIZEOF(cmd), T("%%Os *ppv ") DEF_CHAR_CODE_OPT T(" \"%s\" -k %%%%K\"@^P@Q\""), filename);
			EditExtractMacro(PES, cmd, NULL, 0);
			DeleteFile(filename);
		}
		CloseEditText(&ets);
	}else{
		XMessage(NULL, NULL, XM_GrERRld, T("no data"));
	}
	return 0;
}

int USEFASTCALL PPeDeleteLine(PPxEDSTRUCT *PES)
{
	HWND hWnd = PES->hWnd;

	if ( PES->flags & PPXEDIT_TEXTEDIT ){
		DWORD line, wP, lP;

#ifndef UNICODE
		if ( xpbug < 0 ) return 0;
#endif
		line = SendMessage(hWnd, EM_LINEFROMCHAR, (WPARAM)-1, 0);
		wP = SendMessage(hWnd, EM_LINEINDEX, (WPARAM)line, 0);
		lP = SendMessage(hWnd, EM_LINEINDEX, (WPARAM)line + 1, 0);
		SendMessage(hWnd, EM_SETSEL, wP, lP);
	}else{
		SendMessage(hWnd, EM_SETSEL, 0, EC_LAST);
	}
	return DeleteSub(PES, TEXTSEL_CHAR, VK_DELETE);
}

int USEFASTCALL PPeUndoChar(PPxEDSTRUCT *PES)
{
	TCHAR mode, buf[0x1000];
	DWORD wP, lP;
	HWND hWnd = PES->hWnd;

	PopTextStack(&mode, buf);
	if ( buf[0] == '\0' ) return 0;

	SendMessage(hWnd, EM_GETSEL, (WPARAM)&wP, (LPARAM)&lP);
	SendMessage(hWnd, EM_SETSEL, wP, wP);
	SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)buf);
	if ( !(mode & B0) ){
		lP = tstrlen32(buf);
#ifndef UNICODE
		if ( xpbug < 0 ) CaretFixToW(buf, &lP);
#endif
		wP += lP;
	}
	SendMessage(hWnd, EM_SETSEL, wP, wP);
	return 0;
}

int USEFASTCALL PPeDuplicateLine(PPxEDSTRUCT *PES)
{
	DWORD line, wP, lP, index;
	TCHAR buf[0x6000];
	DWORD len;
	HWND hWnd = PES->hWnd;

	SendMessage(hWnd, EM_GETSEL, (WPARAM)&wP, (LPARAM)&lP);

	*(WORD *)buf = (WORD)TSIZEOF(buf) - 4;
	line = SendMessage(hWnd, EM_LINEFROMCHAR, (WPARAM)-1, 0);
	len = SendMessage(hWnd, EM_GETLINE, (WPARAM)line, (LPARAM)buf);

	if ( PES->flags & PPXEDIT_TEXTEDIT ){
		buf[len++] = '\r';
		buf[len++] = '\n';
	}
	buf[len] = '\0';

	index = SendMessage(hWnd, EM_LINEINDEX, (WPARAM)line + 1, 0);
	SendMessage(hWnd, EM_SETSEL, index, index);
	SendMessage(hWnd, EM_REPLACESEL, 1, (LPARAM)buf);
	SendMessage(hWnd, EM_SETSEL, wP, lP);
	return 0;
}

int USEFASTCALL PPeGetFullPath(PPxEDSTRUCT *PES)
{
	PPXCMD_F fbuf;
	PPXCMDENUMSTRUCT work;

	PPxEnumInfoFunc(PES->info, PPXCMDID_STARTENUM, fbuf.dest, &work);
	fbuf.source = FullPathMacroStr;
	fbuf.dest[0] = '\0';
	Get_F_MacroData(PES->info, &fbuf, &work);
	SendMessage(PES->hWnd, EM_REPLACESEL, 1, (LPARAM)fbuf.dest);
	PPxEnumInfoFunc(PES->info, PPXCMDID_ENDENUM, fbuf.dest, &work);
	return 0;
}

int USEFASTCALL PPeUnSelect(PPxEDSTRUCT *PES)
{
	DWORD lPos, rPos;
	HWND hWnd = PES->hWnd;

	SendMessage(hWnd, EM_GETSEL, (WPARAM)&lPos, (LPARAM)&rPos);
	if ( lPos == rPos ) return 0; // 選択していない
	if ( IsEditCursorPos(PES, lPos) ){ // 左がカーソル？
		rPos = lPos;
	}
	SendMessage(hWnd, EM_SETSEL, (WPARAM)rPos, (LPARAM)rPos);
	return 0;
}

int USEFASTCALL PPeDefaultMenu(PPxEDSTRUCT *PES)
{
	SendMessage(PES->hWnd, WM_RBUTTONDOWN, MK_SHIFT, 0);
	SendMessage(PES->hWnd, WM_RBUTTONUP, MK_SHIFT, 0);
	return 0;
}

const KEYCOMMANDS ppecommands[] = {
	{K_c | K_s | 'A',	PPeUnSelect},

	{K_c | 'O',			PPeOpenFileCmd},
	{K_c | 'S',			PPeSaveFile},
	{K_c | 'U',			PPeUndoChar},
	{K_c | 'V',			PPePaste},

	{K_s | K_c | 'F',	PPeGetFileName},
	{K_s | K_c | 'L',	PPePPcList},
	{K_s | K_c | '3',	PPePPcListMin3},

	{KE_ea,				PPeAppendFile},
	{KE_ei,				PPeInsertFile},
	{KE_ec,				PPeCloseFile},
	{KE_ed,				PPeDuplicate},

	{KE_kp,				PPePrintByPPv},
	{KE_kd,				PPeDuplicateLine},
	{KE_kz,				PPeConvertZenHan},

	{KE_qu,				PPeConvertCase},

	{KE_defmenu,		PPeDefaultMenu},

	{K_tab,				PPeFillTab},
	{K_s | K_tab,		PPeShiftTab},
	{K_ins,				PPeFillIns},
//	{K_s | ' ',			PPeFill},	0.35 廃止
	{'\t',				PPeTabChar},
	{K_s | '\t',		PPeTabChar},

	{K_bs,				PPeBackSpace},
	{K_del,				PPeDelete},
	{K_c | K_del,		PPeDeleteAfterWord},
	{K_s | K_del,		PPeDeleteAfterLine},
	{K_s | K_bs,		PPeDeleteBackWord}, // PPeDeleteBackLine が本来だが、使えないことが多いので。
	{K_c | 0x7f,		PPeDeleteBackLine }, // PPeDeleteBackWord が本来
//	{K_c | K_bs,		PPeDeleteBackWord}, // WM_CHAR で 0x7f が出力
	{K_c | 'Y',			PPeDeleteLine},

//	{K_c | 'P',			PPeGetFullPath},
	{K_a | 'P',			PPeGetFullPath},

	{K_F2,				PPeSelectExtension},
	{K_F12,				PPeSaveAsFile},
	{K_c | K_s | 'S',	PPeSaveAsFile},
	{0, NULL}
};

void XEditSetModify(PPxEDSTRUCT *PES)
{
	SendMessage(PES->hWnd, EM_SETMODIFY, TRUE, 0);
	SendMessage(GetParent(PES->hWnd), WM_COMMAND, TMAKEWPARAM(0, EN_UPDATE), 0);
}

void XEditClearModify(PPxEDSTRUCT *PES)
{
	SendMessage(PES->hWnd, EM_SETMODIFY, FALSE, 0);
	SendMessage(GetParent(PES->hWnd), WM_PPXCOMMAND, KE_clearmodify, 0);
}


void InsertTime(PPxEDSTRUCT *PES, WORD key)
{
	TCHAR buf[64];

	GetNowTime(buf, (key == K_F5) && !(PES->flags & PPXEDIT_TEXTEDIT) );
	SendMessage(PES->hWnd, EM_REPLACESEL, 1, (LPARAM)buf);
}

// 一行から文字列を抽出。line は破壊され、文字列末尾に'\0'が付加される
PPXDLL int PPXAPI GetWordStrings(TCHAR *line, ECURSOR *cursor, DWORD flags)
{
	int start, bcnt = 0, bk C4701CHECK;
	int tmpb;
	int braket = GWS_BRAKET_NONE;
											// 「"」で括っているか判定 --------
											// カーソルまでの「"」の数を求める
	start = cursor->start;
	for ( tmpb = 0 ; tmpb <= start ; tmpb++ ){
		if ( line[tmpb] == '\"' ){
						// カーソル(の直前)が「"」で、
						// その次が空白の場合は、先頭か、
						// 末尾かの判断を行う
			if ( ( (tmpb + 1) >= start) &&
				 ((UTCHAR)line[tmpb + 1] <= (UTCHAR)' ') ){
				if ( bcnt & 1 ) break;	// 奇数→末尾括り→カウントせず
			}
			bk = tmpb;
			bcnt++;
		}
	}
	if ( bcnt & 1 ){	// 奇数→括りあり→前後の括りを検出
		TCHAR *p;

		start = bk + 1; // C4701ok, bcnt & 1 が満たされるときはbk設定済
		p = tstrchr(line + start, '\"');
		if ( p == NULL ){
			cursor->end = start + tstrlen32(line + start);
			braket = GWS_BRAKET_LEFT;
		}else{
			cursor->end = p - line;
			braket = GWS_BRAKET_LEFTRIGHT;
		}
	}else{				// 偶数→括り無し→前後の空白を検出
		while ( start &&
				((UTCHAR)line[start - 1] > (UTCHAR)' ') ){
			start--;
		}
		while ((UTCHAR)line[cursor->end] > (UTCHAR)' ') (cursor->end)++;

		if ( flags & GWSF_SPLIT_PARAM ){
			TCHAR c;
			TCHAR *p;

			p = line + start;
			c = *p;
			if ( ((c == '-') || (c == '/')) &&
				 Isalpha(line[start + 1]) ){ // -option:param 形式なら param に
				p += 2;
				for (;;){
					c = *p;
					if ( c == ':' ){
						if ( (cursor->start >= (DWORD)(p - line + 1)) && // カーソルが : 以降
							 (memcmp((char *)(TCHAR *)(line + start + 1), T("shell"), TSTROFF(5)) != 0) ){ // -shell: ではない
							start = p - line + 1;
						}
						break;
					}
					if ( !Isalpha(c) ) break;
					p++;
				}
			}
		}

	}
	line[cursor->end] = '\0';
	cursor->start = start;
	return braket;
}

void PPeSetCharCode(PPxEDSTRUCT *PES)
{
	POINT pos;
	HMENU hMenu;
	int index, oof = charmenu_other;
	PPXINMENU menus[charmenu_items];
	TCHAR otherstr[20], filename[VFPS];
	UINT cp;

	InitEditCharCode(PES);

	GetPPePopupPositon(PES, &pos);
	memcpy(menus, charmenu, sizeof(charmenu));
	if ( GetACP() != CP__SJIS ){
		menus[charmenu_sjis].key = CP__SJIS;
		menus[oof].key = VTYPE_SYSTEMCP;
		menus[oof].str = charmenustr_lcp;
		oof++;
	}
	cp = (PES->CharCP < VTypeToCPlist_max) ? VTypeToCPlist[PES->CharCP] : PES->CharCP;
	thprintf(otherstr, TSIZEOF(otherstr), T("codepage %d..."), cp);
	menus[oof].key = VTYPE_OTHER;
	menus[oof].str = otherstr;

	hMenu = MakePopupMenus(menus, PES->CharCP);
	index = TrackPopupMenu(hMenu, TPM_TDEFAULT, pos.x, pos.y, 0, PES->hWnd, NULL);
	DestroyMenu(hMenu);
	if ( index != 0 ){
		if ( index == VTYPE_OTHER ){
			const TCHAR *ptr;

			Numer32ToString(otherstr, cp);
			if ( tInput(PES->hWnd, T("codepage"), otherstr, 10, PPXH_NUMBER, PPXH_NUMBER) <= 0 ){
				return;
			}
			ptr = otherstr;
			index = GetIntNumber(&ptr);
			if ( index == 0 ) return;
		}
		PES->CharCP = (UINT)index;
		ThGetString(&PES->LocalStringValue, LSV_filename, filename, VFPS);
		if ( (filename[0] != '\0') && (SendMessage(PES->hWnd, EM_GETMODIFY, 0, 0) == FALSE) && (PMessageBox(PES->hWnd, StrReload, T("PPe"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) == IDYES) ){ // 指定した文字コードで再読み込み
			DWORD memsize;
			TCHAR *textimage = NULL;

			if ( NO_ERROR == LoadFileImage(filename, 0x40, (char **)&textimage, &memsize, NULL) ){
				SendMessage(PES->hWnd, WM_SETREDRAW, FALSE, 0);
				OpenMainFromMem(PES, PPE_OPEN_MODE_OPEN, NULL, textimage, memsize, PES->CharCP);
				HeapFree(ProcHeap, 0, textimage);
				SendMessage(PES->hWnd, WM_SETREDRAW, TRUE, 0);
				InvalidateRect(PES->hWnd, NULL, TRUE);
			}
		}else{
			XEditSetModify(PES);
		}
	}
}

void PPeSetReturnCode(PPxEDSTRUCT *PES)
{
	POINT pos;
	HMENU hMenu;
	int index;

	GetPPePopupPositon(PES, &pos);
	hMenu = MakePopupMenus(returnmenu, PES->CrCode + 1);
	index = TrackPopupMenu(hMenu, TPM_TDEFAULT, pos.x, pos.y, 0, PES->hWnd, NULL);
	DestroyMenu(hMenu);
	if ( index ){
		PES->CrCode = index - 1;
		XEditSetModify(PES);
	}
}

void PPeSetTab(PPxEDSTRUCT *PES, int usetab)
{
	int count = 0;
	UINT tab = 8;
	TCHAR keyword[CUST_NAME_LENGTH], param[VFPS];

	if ( usetab > 0 ){
		tab = usetab;
	}else if ( usetab < 0 ){
		TCHAR filename[VFPS], *entry;

		ThGetString(&PES->LocalStringValue, LSV_filename, filename, VFPS);
		entry = VFSFindLastEntry(filename);
		while( EnumCustTable(count++, T("XV_tab"), keyword, param, sizeof(param)) >= 0){
			FN_REGEXP fn;
			const TCHAR *p;
			int ctab;

			p = keyword;
			ctab = GetIntNumber(&p);
			MakeFN_REGEXP(&fn, param);
			if ( FilenameRegularExpression(entry, &fn) ){
				tab = ctab;
				FreeFN_REGEXP(&fn);
				break;
			}
			FreeFN_REGEXP(&fn);
		}
	}else{ // == 0
		const TCHAR *ptr;

		Numer32ToString(param, PES->tab);
		if ( tInput(PES->hWnd, MES_TTAB, param, 10, PPXH_NUMBER, PPXH_NUMBER) <= 0 ){
			return;
		}
		ptr = param;
		tab = GetIntNumber(&ptr);
		if ( tab <= 0 ) return;
	}
	PES->tab = tab;
	tab <<= 2 ; // ダイアログボックス単位に変換(x4)
	SendMessage(PES->hWnd, EM_SETTABSTOPS, 1, (LPARAM)&tab);
	InvalidateRect(PES->hWnd, NULL, FALSE);
}

void PPeGetString(PPxEDSTRUCT *PES, TCHAR cmd)
{
	TCHAR buf[VFPS];
	PPXCMDENUMSTRUCT work;

	PPxEnumInfoFunc(PES->info, PPXCMDID_STARTENUM, buf, &work);
	if ( PES->info == &PPxDefInfo ){
		switch (cmd){
			case '0': {	// 自分自身へのパス
				TCHAR *p;

				GetModuleFileName(NULL, buf, MAX_PATH);
				p = FindLastEntryPoint(buf);
				*p = '\0';
				break;
			}

			case '1':
				GetCurrentDirectory(VFPS, buf);
				break;

			case 'C':
			case 'R':
				ThGetString(&PES->LocalStringValue, LSV_filename, buf, VFPS);
				break;

			default:
				PPxEnumInfoFunc(PES->info, cmd, buf, &work);
		}
	}else{
		PPxEnumInfoFunc(PES->info, cmd, buf, &work);
	}

	SendMessage(PES->hWnd, EM_REPLACESEL, 1, (LPARAM)buf);
	PPxEnumInfoFunc(PES->info, PPXCMDID_ENDENUM, buf, &work);
}

void CloseUpperList(PPxEDSTRUCT *PES)
{
	if ( PES->list.hWnd == NULL ) return;
	if ( PES->list.direction > 0 ) return; // 下向きなので無視
	if ( SendMessage(PES->list.hWnd, LB_GETCURSEL, 0, 0) >= 0 ) return; // 選択中
	PostMessage(PES->list.hWnd, WM_CLOSE, 0, 0);
}

int CloseLineList(PPxEDSTRUCT *PES)
{
	int result = CLOSELIST_NONE;

	if ( PES->list.hSubWnd != NULL ){
		if ( SendMessage(PES->list.hSubWnd, LB_GETCOUNT, 0, 0) > 0 ){
			result = (PES->list.mode == LIST_MANUAL) ?
					CLOSELIST_MANUALLIST : CLOSELIST_AUTOLIST;
		}
		DestroyWindow(PES->list.hSubWnd);
	}
	if ( PES->list.hWnd != NULL ){
		if ( SendMessage(PES->list.hWnd, LB_GETCOUNT, 0, 0) > 0 ){
			result = (PES->list.mode == LIST_MANUAL) ?
					CLOSELIST_MANUALLIST : CLOSELIST_AUTOLIST;
		}
		DestroyWindow(PES->list.hWnd);
	}
	return result;
}
void USEFASTCALL EnterFix(PPxEDSTRUCT *PES)
{
	TCHAR buf[CMDLINESIZE];

	if ( PES->flags & (PPXEDIT_TEXTEDIT | PPXEDIT_LINE_MULTILINE) ) return;

	if ( (PES->list.WhistID != 0) && (PES->flags & PPXEDIT_WANTENTER) ){
		buf[0] = '\0';
		Edit_GetWindowText(PES, buf, TSIZEOF(buf));
		buf[TSIZEOF(buf) - 1] = '\0';
		WriteHistory(PES->list.WhistID, buf, 0, NULL);
	}
	if ( !(PES->flags & PPXEDIT_WANTALLKEY) ){
		CloseLineList(PES);

		// WM_CHAR の 13 (Enter) を廃棄
		RemoveCharKey(PES->hWnd);
	}
}

/*-----------------------------------------------------------------------------
	拡張エディットボックスコマンド処理部
-----------------------------------------------------------------------------*/
ERRORCODE PPedExtCommand(PPxEDSTRUCT *PES, PPECOMMANDPARAM *param, const TCHAR *command)
{
	const WORD *ptr;

	if ( param->key == K_cr ) EnterFix(PES);
	// コマンド実行
	if ( (UTCHAR)command[0] == EXTCMD_CMD ){
		return EditExtractMacro(PES, command + 1, NULL, 0);
	}

	// キー実行
	ptr = (WORD *)(((UTCHAR)command[0] == EXTCMD_KEY) ? (command + 1) : command);
	param->key = *ptr;
	if ( param->key == 0 ) return NO_ERROR;

	for( ; *(++ptr) ; param->key = *ptr ){ // 最後以外のキーを実行
		if ( PES->AKey != NULL ){
			if ( SendMessage(GetParent(PES->hWnd), WM_PPXCOMMAND, param->key, 0) == ERROR_SEEK ){
				continue;
			}
		}
		EdPPxWmCommand(PES, PES->hWnd, param->key, 0);
//		PPedCommand(PES, param);
	}
	// 最後のキーを実行
	if ( PES->AKey != NULL ){
		if ( SendMessage(GetParent(PES->hWnd), WM_PPXCOMMAND, param->key, 0) == ERROR_SEEK ){
			return NO_ERROR;
		}
	}
	EdPPxWmCommand(PES, PES->hWnd, param->key, 0);
	return NO_ERROR;
}

BOOL USEFASTCALL CtrlESCFix(PPxEDSTRUCT *PES)
{
	MSG msg;
	HWND hParentWnd = GetParent(PES->hWnd);

	if ( hParentWnd == NULL ) return FALSE;
	if ( PES->flags & PPXEDIT_WANTENTER ){
		PostMessage(hParentWnd, WM_COMMAND, TMAKELPARAM(0, VK_ESCAPE), (LPARAM)PES->hWnd);
		return TRUE;
	}
	// ダイアログで ESC を押したあとに WM_CHAR が来るケース対応
	if ( PeekMessage(&msg, hParentWnd, WM_CLOSE, WM_CLOSE, PM_NOREMOVE) ){
		return TRUE; // 終了しかけなのでメニュー表示しない
	}
	return FALSE;
}

// 未実行=ERROR_INVALID_FUNCTION , 実行有り=NO_ERROR
ERRORCODE PPedCommand(PPxEDSTRUCT *PES, PPECOMMANDPARAM *param)
{
	TCHAR buf[CMDLINESIZE], *bufp;

	PES->oldkey = PES->oldkey2;
	if ( !(param->key & K_raw) ){
		PutKeyCode(buf, param->key);
		if ( ((PES->AKey != NULL) && (NULL != (bufp = GetCustValue(PES->AKey, buf, buf, sizeof(buf)))) ) ||
			 (NULL != (bufp = GetCustValue(StrK_edit, buf, buf, sizeof(buf)))) ){
			ERRORCODE cmdresult;

			cmdresult = PPedExtCommand(PES, param, bufp);
			if ( bufp != buf ) HeapFree(ProcHeap, 0, bufp);
			return cmdresult;
		}
	}
	PES->oldkey2 = 0;
	resetflag(param->key, K_raw);	// エイリアスビットを無効にする
	{
		const KEYCOMMANDS *cms = ppecommands;

		while ( cms->key ){
			if ( cms->key == param->key ) return cms->func(PES);
			cms++;
		}
	}

	switch (param->key) {
case K_c | K_s | K_v | 'A': // RichEdit 全て大文字表示の切り替え
	break;

case K_c | 'J':			// ^[J]
	if ( PES->flags & (PPXEDIT_TEXTEDIT | PPXEDIT_LINE_MULTI | PPXEDIT_LINE_MULTILINE) ){
		SendMessage(PES->hWnd, EM_REPLACESEL, 1, (LPARAM)T("\r\n"));
	}
	break;
case K_c | 'M':			// ^[M]
	if ( PES->flags & (PPXEDIT_TEXTEDIT | PPXEDIT_LINE_MULTILINE) ){
		SendMessage(PES->hWnd, EM_REPLACESEL, 1, (LPARAM)T("\r\n"));
	}else{
		PostMessage(GetParentCaptionWindow(PES->hWnd), WM_COMMAND, TMAKELPARAM(IDOK, BN_CLICKED), 0);
	}
	break;
//-----------------------------------------------
case K_c | 'W':			// ^[W]
	if ( PES->flags & PPXEDIT_TEXTEDIT ){
		PPeCloseFile(PES);
		break;
	}else{
		return ERROR_INVALID_FUNCTION;
	}
//-----------------------------------------------
case K_c | K_s | 'N':			// ^\[N]
	EditExtractMacro(PES, T("*ppe"), NULL, 0);
	break;
//-----------------------------------------------
case K_c | 'N':			// ^[N]
case K_a | 'C':			// &[C]
	PPeGetString(PES, 'C');
	break;
//-----------------------------------------------
case K_c | 'E':			// ^[E]
case K_a | 'X':			// &[X]
	PPeGetString(PES, 'X');
	break;
//-----------------------------------------------
case K_c | 'T':			// ^[T]
case K_a | 'T':			// &[T]
	PPeGetString(PES, 'T');
	break;
//-----------------------------------------------
case K_c | 'R':			// ^[R]
case K_a | 'R':			// &[R]
	PPeGetString(PES, 'R');
	break;
//-----------------------------------------------
case K_c | 'P':	// ^[P]
	if ( PES->flags & PPXEDIT_TEXTEDIT ){
		PPePrintByPPv(PES);
	}else{
		PPeGetFullPath(PES);
	}
	break;
case K_s | K_c | 'P':	// ^\[P]
	EditExtractMacro(PES,
		(PES->flags & PPXEDIT_SINGLEREF) ?
			T("*replace %M_pjump") : T("*insert %M_pjump"), NULL, 0);
	break;
//------------------------------------------------------
case K_c | K_v | '0':	// ^[0]
case K_c | '0':			// ^[0]
case K_a | '0':			// &[0]
	PPeGetString(PES, '0');
	break;
//-----------------------------------------------
case K_c | K_v | '1':	// ^[1]
case K_c | '1':			// ^[1]
case K_a | '1':			// &[1]
case K_a | 'Q':			// &[Q]
	PPeGetString(PES, '1');
	break;
//-----------------------------------------------
case K_c | K_v | '2':	// ^[2]
case K_c | '2':			// ^[2]
case K_a | '2':			// &[2]
case K_a | 'W':			// &[W]
	PPeGetString(PES, '2');
	break;
//-----------------------------------------------
case K_s | K_c | 'D':	// ^\[D] ディレクトリ
	PPeTreeWindow(PES);
	break;
//-----------------------------------------------
case KE_RefButton:		// 参照ボタン
case K_s | K_c | 'I':	// ^\[I] ファイル取得
	param->key = (WORD)((PES->flags & PPXEDIT_REFTREE) ?
			(K_raw | K_s | K_c | 'D') : (K_raw | K_s | K_c | 'F'));
	PPedCommand(PES, param);
	break;
//-----------------------------------------------
case K_c | 'A':			// ^[A] すべてを選択
	SendMessage(PES->hWnd, EM_SETSEL, 0, EC_LAST);
	break;
//-----------------------------------------------
case K_s | K_up:		// \[↑]
	if ( !(PES->flags & PPXEDIT_NOINCLIST) ) return ERROR_INVALID_FUNCTION;
	// K_up へ
case K_up:				// [↑]
	PES->oldkey2 = 1;
	if ( ListUpDown(PES->hWnd, PES, -1, param->repeat) == FALSE ) return ERROR_INVALID_FUNCTION;
	break;
//-----------------------------------------------
case K_s | K_dw:		// \[↓]
	if ( !(PES->flags & PPXEDIT_NOINCLIST) ) return ERROR_INVALID_FUNCTION;
	// K_dw へ
case K_dw:				// [↓]
	PES->oldkey2 = 1;
	if ( ListUpDown(PES->hWnd, PES, 1, param->repeat) == FALSE ) return ERROR_INVALID_FUNCTION;
	break;
//-----------------------------------------------
case K_F3:
	SearchStr(PES, EDITDIST_NEXT);
	break;
case K_s | K_F3:
	SearchStr(PES, EDITDIST_BACK);
	break;
case K_F5:
case K_s | K_F5:
	InsertTime(PES, param->key);
	break;
case K_F6:
case K_c | 'F':
	SearchStr(PES, EDITDIST_DIALOG);
	break;
case K_F7:
	PPeReplaceStr(PES);
	break;
case K_s | K_F7:
	if ( PES->findrep == NULL ) break;
	SendMessage(PES->hWnd, EM_REPLACESEL, 1,
			(LPARAM)((PES->findrep->replacetext[0] != '\0') ?
				PES->findrep->replacetext : PES->findrep->findtext));
	break;
//-----------------------------------------------
case K_s | K_Pup:		// \[PgUp]
case K_Pup:				// [PgUp]
	return ListPageUpDown(PES, EDITDIST_BACK, FALSE);
//-----------------------------------------------
case K_s | K_Pdw:		// \[PgDw]
case K_Pdw:				// [PgDw]
	return ListPageUpDown(PES, EDITDIST_NEXT, FALSE);
//-----------------------------------------------
case K_e | K_lf:			// ~[←]
	ListHScroll(PES, -1);
	break;
case K_e | K_ri:			// ~[→]
	ListHScroll(PES, 1);
	break;
case K_e | K_c | K_lf:		// ~^[←]
	ListWidth(PES, -1);
	break;
case K_e | K_c | K_ri:		// ~^[→]
	ListWidth(PES, 1);
	break;

case K_a | K_home:			// &[home]
	CenterWindow(GetParentCaptionWindow(PES->hWnd));
	break;
case K_a | K_up:			// &[↑]
	MoveWindowByKey(GetParentCaptionWindow(PES->hWnd), 0, -1);
	break;
case K_a | K_dw:			// &[↓]
	MoveWindowByKey(GetParentCaptionWindow(PES->hWnd), 0, 1);
	break;
case K_a | K_lf:			// &[←]
	MoveWindowByKey(GetParentCaptionWindow(PES->hWnd), -1, 0);
	break;
case K_a | K_ri:			// &[→]
	MoveWindowByKey(GetParentCaptionWindow(PES->hWnd), 1, 0);
	break;
case K_a | K_s | K_up:		// &\[↑]
	WideWindowByKey(PES, 0, -1);
	break;
case K_a | K_s | K_dw:		// &\[↓]
	WideWindowByKey(PES, 0, 1);
	break;
case K_a | K_s | K_lf:		// &\[←]
	WideWindowByKey(PES, -1, 0);
	break;
case K_a | K_s | K_ri:		// &\[→]
	WideWindowByKey(PES, 1, 0);
	break;
//-----------------------------------------------
case K_s | K_esc:	// \[ESC]
	if ( PES->flags & PPXEDIT_TEXTEDIT ){
		HWND hPWnd;

		hPWnd = GetParentCaptionWindow(PES->hWnd);
		ShowWindow(hPWnd, SW_MINIMIZE);
		PostMessage(hPWnd, WM_LBUTTONUP, 0, MAX32);
		PostMessage(hPWnd, WM_RBUTTONUP, 0, MAX32);
	}
	break;
case K_F4:
	if ( PES->list.ListFocus != LISTU_NOLIST ){
		CloseLineList(PES);
		break;
	}else{
		if ( !(PES->flags & PPXEDIT_SUGGEST) ) return ERROR_INVALID_FUNCTION;
		if ( PES->flags & PPXEDIT_TEXTEDIT ){
			if ( PES->list.RhistID == 0 ){
				PES->list.RhistID = PPXH_COMMAND | PPXH_GENERAL | PPXH_DIR | PPXH_PATH | PPXH_SEARCH | PPXH_PPCPATH;
			}
			if ( PES->list.WhistID == 0 ){
				PES->list.WhistID = PPXH_GENERAL;
			}
			KeyStepFill(PES, NULL);
		}else if ( X_flst_mode >= X_fmode_auto2 ){
			KeyStepFill(PES, KeyStepFill_listmode);
		}else{
			FloatList(PES, EDITDIST_NEXT);
		}
	}
	return NO_ERROR;
//------------------------------------
case K_esc:			// [ESC]
	// WM_CHAR の 27 (Enter) を廃棄
	RemoveCharKey(PES->hWnd);
	if ( !(PES->flags & PPXEDIT_TEXTEDIT) || (PES->flags & PPXEDIT_LINE_MULTILINE) ){ // ダイアログ時
		if ( CloseLineList(PES) != CLOSELIST_NONE ) break;
		if ( PES->flags & PPXEDIT_WANTALLKEY ){
			PostMessage(GetParent(PES->hWnd), WM_COMMAND, IDCANCEL, 0);
			return NO_ERROR;
		}else{
			if ( CtrlESCFix(PES) ) break;
			return ERROR_INVALID_FUNCTION;
		}
	}
	if ( CtrlESCFix(PES) ) break;
//	if ( CloseLineList(PES) != CLOSELIST_NONE ) break;
//	return ERROR_INVALID_FUNCTION;	// デフォルト処理を行わせる

case K_c | ']':		// ^[]]
case K_F1:			// [F1]
case '\x1b': /*K_esc だと WM_CHAR で閉じて、メニュー表示できない */	// [ESC]
	if ( CloseLineList(PES) != CLOSELIST_NONE ) break;
	if ( param->key == '\x1b' ){
		if ( CtrlESCFix(PES) ) break;
	}
	PPeMenu(PES, param, escmenu);
	break;
//------------------------------------
case K_s | K_F1:
	PPxHelp(PES->hWnd, HELP_CONTEXT, IDH_PPE);
	break;
//------------------------------------
case K_s | K_F2:			// \[F2]
	PPeMenu(PES, param, f2menu);
	break;
//------------------------------------
case K_c | 'K':				// ^[K]
	PPeMenu(PES, param, kmenu);
	break;
//------------------------------------
case K_c | 'Q':				// ^[Q]
	PPeMenu(PES, param, qmenu);
	break;
//------------------------------------
case K_apps:
case K_a | ' ':				// &[ ]
	PPeMenu(PES, param, amenu);
	break;
//------------------------------------
case KE_ee:					// ESC-exec
	buf[0] = '\0';
	if ( tInput(PES->hWnd, MES_TSHL, buf, TSIZEOF(buf),
				PPXH_COMMAND, PPXH_COMMAND) > 0 ){
		EditExtractMacro(PES, buf, NULL, 0);
	}
	break;
//------------------------------------
case KE_er:					// ESC-Run as admin
	if ( Edit_GetWindowText(PES, buf, TSIZEOF(buf)) < (TSIZEOF(buf) - 1) ){
		PPeRunas(PES, buf);
	}
	break;
//------------------------------------
//case KE_eq:		// ESC-Quit
//case KE_ex:		// ESC-X CloseAll
//	PostMessage(GetParentCaptionWindow(PES->hWnd), WM_CLOSE, 0, 0);
//	break;
//-----------------------------------------------
case KE_qj:		// ^Q-J Jump to Line
	if ( PES->flags & PPXEDIT_TEXTEDIT ){
		EditJumpLineCommand(PES, NULL);
	}
	break;
//------------------------------------
case K_home:
	if ( !(PES->flags & PPXEDIT_LINE_MULTI) ) return ERROR_INVALID_FUNCTION;
	// K_c | K_Pup へ
case KE_qr:	// Top of text
	if ( SearchStrCheck(PES, EDITDIST_BACK) ) break;
// case K_c | K_home: edit control割当て済み
	 // ※ inum[1]以降は使用しないこと前提
	EditCursorCommand(PES, (PPXAPPINFOUNION *)&Cursor_TopOfText);
	break;

//-----------------------------------------------
case K_end:
	if ( !(PES->flags & PPXEDIT_LINE_MULTI) ) return ERROR_INVALID_FUNCTION;
	// K_c | K_Pdw へ
	if ( SearchStrCheck(PES, EDITDIST_NEXT) ) break;
case KE_qc:	// Top of text
// case K_c | K_end: edit control割当て済み
	 // ※ inum[1]以降は使用しないこと前提
	EditCursorCommand(PES, (PPXAPPINFOUNION *)&Cursor_EndOfText);
	break;

//-----------------------------------------------
case K_c | K_up:	// one logical line up
	if ( SearchStrCheck(PES, EDITDIST_BACK) ) break;
	EditCursorCommand(PES, (PPXAPPINFOUNION *)&Cursor_LogUp);
	break;
case K_c | K_Pup:	// Top of window
	if ( SearchStrCheck(PES, EDITDIST_BACK) ) break;
case KE_qe:	// Top of Window
	if ( PES->style & ES_MULTILINE ){ // ※ inum[1]以降は使用しないこと前提
		EditCursorCommand(PES, (PPXAPPINFOUNION *)&Cursor_TopOfWindow);
	}
	break;
//-----------------------------------------------
case K_c | K_dw:	// one logical line down
	if ( SearchStrCheck(PES, EDITDIST_NEXT) ) break;
	EditCursorCommand(PES, (PPXAPPINFOUNION *)&Cursor_LogDown);
	break;
case K_c | K_Pdw:	// End of window
	if ( SearchStrCheck(PES, EDITDIST_NEXT) ) break;
case KE_qx:	// End of Window
	if ( PES->style & ES_MULTILINE ){ // ※ inum[1]以降は使用しないこと前提
		EditCursorCommand(PES, (PPXAPPINFOUNION *)&Cursor_EndOfWindow);
	}
	break;

case KE_2t:	// tabstop
	PPeSetTab(PES, 0);
	break;

case KE_2r:	// Return code
	PPeSetReturnCode(PES);
	break;

case KE_2p:
	ChangeLineBreak(PES, -1);
	break;

case KE_2x:
	X_esrx = !X_esrx;
	break;

case KE_2c:	// Char code
	PPeSetCharCode(PES);
	break;

case KE_qv:	// Q-View mode
	SendMessage(PES->hWnd, EM_SETREADONLY,
			(PES->style & ES_READONLY) ? FALSE : TRUE, 0);
	PES->style = GetWindowLong(PES->hWnd, GWL_STYLE);
	break;

case K_c | K_v | 'C': // RichEdit はこのコードも反応するので無効化する
case K_c | K_v | 'X': // RichEdit はこのコードも反応するので無効化する
case K_c | K_v | 'V': // RichEdit はこのコードも反応するので無効化する
case K_c | K_v | 'Z': // RichEdit はこのコードも反応するので無効化する
case K_c | K_v | 'Q': // RichEdit はこのコードも反応するので無効化する
	break;

case K_c | K_s | 'G':
	SetRichSyntaxColor(PES);
	break;

case K_c | K_s | 'B':
	RichChangeFontStyle(PES, CFM_BOLD, CFE_BOLD);
	break;
case K_c | K_s | 'H':
	RichChangeBackColor(PES);
	break;
case K_c | K_s | 'R':
	RichChangeFontColor(PES);
	break;
case K_c | K_s | 'U':
	RichChangeFontStyle(PES, CFM_UNDERLINE, CFE_UNDERLINE);
	break;

// case K_c | 'V': // PPePaste で処理済み
case K_c | K_s | 'V':
	CallWindowProc(PES->hOldED, PES->hWnd, WM_PASTE, 0, 0);
	break;
case K_c | 'C':
	PPeClip(PES, WM_COPY);
	break;
case K_c | 'X':
	PPeClip(PES, WM_CUT);
	break;
case K_c | 'Z':
	CallWindowProc(PES->hOldED, PES->hWnd, WM_UNDO, 0, 0);
	break;
case K_c | K_s | 'Z':
	CallWindowProc(PES->hOldED, PES->hWnd, EM_REDO, 0, 0);
	break;

case K_a | K_del: {
	if ( Edit_GetWindowText(PES, buf, TSIZEOF(buf)) < (TSIZEOF(buf) - 1) ){
		if ( DeleteHistory(PES->list.WhistID, buf) || DeleteHistory(PES->list.RhistID, buf) ){
			Edit_SetWindowText(PES, NilStr);
			SetMessageForEdit(PES->hWnd, T("Delete history"));
			if ( PES->list.hWnd != NULL ){
				int index;

				index = SendMessage(PES->list.hWnd, LB_GETCURSEL, 0, 0);
				if ( index >= 0 ){
					SendMessage(PES->list.hWnd, LB_DELETESTRING, index, 0);
					SendMessage(PES->list.hWnd, LB_SETCURSEL, index, 0);
				}
			}
			if ( PES->list.hSubWnd != NULL ){
				int index;

				index = SendMessage(PES->list.hSubWnd, LB_GETCURSEL, 0, 0);
				if ( index >= 0 ){
					SendMessage(PES->list.hSubWnd, LB_DELETESTRING, index, 0);
					SendMessage(PES->list.hSubWnd, LB_SETCURSEL, index, 0);
				}
			}
		}
	}
	break;
}

//----------------------------------------------- Zoom in
case K_c | K_v | VK_ADD:
case K_c | K_v | VK_OEM_PLUS:
	ChangeFontSize(PES, 1);
	break;
//----------------------------------------------- Zoom out
case K_c | K_v | VK_SUBTRACT:
case K_c | K_v | VK_OEM_MINUS:
	ChangeFontSize(PES, -1);
	break;
//----------------------------------------------- Zoom mode change
case K_c | K_v | VK_NUMPAD0:
	ChangeFontSize(PES, -9);
	break;

case K_c | K_s | K_v | VK_ADD:
case K_c | K_s | K_v | VK_OEM_PLUS: // US[=/+] JIS[;/+]
	ChangeOpaqueWindow(PES->hWnd, 1);
	break;
case K_c | K_s | K_v | VK_SUBTRACT:
case K_c | K_s | K_v | VK_OEM_MINUS: // US[-/_] JIS[-/=]
	ChangeOpaqueWindow(PES->hWnd, -1);
	break;

case K_a | K_cr:
	if ( PES->flags & PPXEDIT_LINE_MULTILINE ){
		PostMessage(GetParent(PES->hWnd), WM_COMMAND, IDOK, 0);
	}
	break;

case K_cr:
	if ( PES->flags & PPXEDIT_WANTENTER ){
		PostMessage(GetParent(PES->hWnd), WM_COMMAND, TMAKELPARAM(0, VK_RETURN), (LPARAM)PES->hWnd);
		break;
	}
	// 再度 enter を入力し、閉じたりさせる(WM_GETDLGCODEを一旦通過しているので、ダイアログを閉じたりすることは現時点ではもうできないため)
	if ( CloseLineList(PES) == CLOSELIST_AUTOLIST ){
		if ( !( PES->flags & PPXEDIT_TEXTEDIT ) ){
			PostMessage(PES->hWnd, WM_KEYDOWN, VK_RETURN, 0);
			RemoveCharKey(PES->hWnd); // WM_CHAR の 13 (Enter) を廃棄
		}
		break;
	}
	if ( PES->flags & PPXEDIT_WANTALLKEY ){
		if ( PES->flags & PPXEDIT_TEXTEDIT ){
			PostMessage(PES->hWnd, WM_CHAR, 13, 0);
		}else{
			PostMessage(GetParent(PES->hWnd), WM_COMMAND, IDOK, 0);
		}
	}
	return ERROR_INVALID_FUNCTION;

	// default へ
//-----------------------------------------------
default:
	return PPxCommonCommand(PES->hWnd, 0, param->key);
//-----------------------------------------------
	}
	return NO_ERROR; // 実行済み
}

LRESULT EditSetFont(PPxEDSTRUCT *PES, HFONT hFont, LPARAM lParam)
{
	CHARFORMAT2 chfmt;
	LOGFONT lfont;

	PES->hEditFont = hFont;
	GetHeight(PES, hFont);
	if ( !(PES->flags & PPXEDIT_RICHEDIT) ){
		return CallWindowProc(PES->hOldED, PES->hWnd, WM_SETFONT, (WPARAM)hFont, lParam);
	}

	// フォント設定
	GetObject(hFont, sizeof(LOGFONT), &lfont);
	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = CFM_SIZE | CFM_FACE | CFM_CHARSET | CFM_LCID;
	if ( lfont.lfHeight < 0 ) lfont.lfHeight = -lfont.lfHeight;
	chfmt.yHeight = TwipToPixel(lfont.lfHeight, 96/*GetMonitorDPI(PES->hWnd)*/);
	if ( chfmt.yHeight < 0 ) chfmt.yHeight = -chfmt.yHeight;
	tstrcpy(chfmt.szFaceName, lfont.lfFaceName);
	chfmt.bCharSet = lfont.lfCharSet;
	chfmt.lcid = GetUserDefaultLCID();
	chfmt.bPitchAndFamily = lfont.lfPitchAndFamily;

	// SCF_DEFAULT は 0 なので別個に指定
	SendMessage(PES->hWnd, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&chfmt);
	SendMessage(PES->hWnd, EM_SETCHARFORMAT, SPF_SETDEFAULT | SCF_SELECTION | SCF_ALL, (LPARAM)&chfmt);

	if ( lParam != 0 ) InvalidateRect(PES->hWnd, NULL, TRUE);
	return 0;
}

void FreeEdStruct(PPxEDSTRUCT *PES)
{
	if ( --PES->ref > 0 ) return;
	HeapFree(DLLheap, 0, PES);
}

// LineMulti 時は、スクロールバーをそのまま借用できないので、WM_NCLBUTTONDOWN で処理する
void LineMulti_VScrollDown(HWND hWnd, int fontY, LPARAM lParam)
{
	POINT pos;

	LPARAMtoPOINT(pos, lParam);
	ScreenToClient(hWnd, &pos);
	EDsHell(hWnd, WM_VSCROLL, ( pos.y <= (fontY / 2) ) ? SB_LINEUP : SB_LINEDOWN, 0);
}

void SetDropFiles(HWND hWnd, HDROP hDrop)
{
	TCHAR name[VFPS];

	DragQueryFile(hDrop, 0, name, TSIZEOF(name));
	DragFinish(hDrop);
	SetWindowText(hWnd, name);
}

// WM_SYSKEYDOWN で英数字キーの処理を行ったとき、後続の WM_SYSCHAR を
// 処理しないため警告音が出る問題に対処する
void SkipAltLetterKeyDef(HWND hWnd)
{
	MSG WndMsg;

	if ( IsTrue(PeekMessage(&WndMsg, hWnd, WM_SYSCHAR, WM_SYSCHAR, PM_NOREMOVE)) ){
		if ( IsalnumA(WndMsg.wParam) ){ // 該当したら WM_SYSCHAR を破棄
			PeekMessage(&WndMsg, hWnd, WM_SYSCHAR, WM_SYSCHAR, PM_REMOVE);
		}
	}
}

void LineExpand(HWND hWnd, PPxEDSTRUCT *PES)
{
	DWORD line, deskline, lineY;
	WindowExpandInfoStruct eis;

	line = Edit_SendMessageMsg(PES, EM_GETLINECOUNT);
	if ( PES->caretLY == line ) return;

	eis.hParentWnd = GetParent(hWnd);
	GetDesktopRect(hWnd, &eis.boxDesk);

	lineY = PES->fontY;
	if ( PES->flags & PPXEDIT_RICHEDIT ) lineY += PES->fontYext;

	deskline = (eis.boxDesk.bottom - eis.boxDesk.top) / lineY;
	if ( deskline < 1 ) deskline = 1;
	if ( deskline > 3 ) deskline--;
	if ( deskline > 20 ) deskline /= 2;
	if ( line > deskline ){
		line = deskline;
		if ( PES->caretLY == line ) return;
	}

	GetWindowRect(hWnd, &eis.boxEdit);
	GetWindowRect(eis.hParentWnd, &eis.boxDialog);

	eis.delta = (line - PES->caretLY) * lineY;
	eis.boxEdit.bottom += -eis.boxEdit.top + eis.delta;

	if ( eis.boxEdit.bottom < (int)lineY ){ // １行未満なので補正
		eis.delta += lineY - eis.boxEdit.bottom;
		eis.boxEdit.bottom = lineY;
	}

	PES->caretLY = line;
	EditBoxExpand(PES, &eis);

	if ( (PES->flags & PPXEDIT_RICHEDIT) && (PES->list.hWnd != NULL) ){
		PostMessage(hWnd, WM_PPXCOMMAND, KE_changepos, 0);
	}

	InvalidateRect(eis.hParentWnd, NULL, TRUE);
}

LRESULT CharProc(PPxEDSTRUCT *PES, HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if ( !(PES->flags & PPXEDIT_TEXTEDIT) || (PES->list.ListFocus != LISTU_NOLIST) ){ // 一行編集特有
		// X_fmode_autoX の自動補完機能付きの一覧表示
		if ( ( (wParam != '\t') && (wParam != '\xd') && (wParam != '\x1b')) && // TAB/CR/ESCでない
				((PES->list.mode >= LIST_FILL) ||
				 ((X_flst_mode >= X_fmode_auto1) &&
				  !(PES->flags & PPXEDIT_NOINCLIST) &&
				  (PES->list.hWnd == NULL)) ) )
		{
			LRESULT lr;

			lr = CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);
			KeyStepFill(PES, NULL);
			if ( PES->flags & PPXEDIT_SYNTAXCOLOR ) RichSyntaxColor(PES);
			return lr;

		// 補完候補リストのインクリメンタルサーチ
		}else if ( PES->flags & PPXEDIT_SUGGEST ){
			int oldlen;
			LRESULT lr;

			if ( wParam == '\t' ) return 1;
			oldlen = Edit_GetWindowTextLength(PES);
			lr = CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);
			ListSearch(hWnd, PES, oldlen);
			return lr;
		}
	}
	// ※PPXEDIT_LINECSRは、ES_MULTILINE のときしか有効になっていない
	if ( PES->flags & (PPXEDIT_LINECSR | PPXEDIT_LINE_MULTI | PPXEDIT_LINE_MULTILINE) ){
		LRESULT lr = CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);

		if ( PES->flags & (PPXEDIT_LINE_MULTI | PPXEDIT_LINE_MULTILINE) ){
			LineExpand(hWnd, PES);
		}else{
			LineCursor(PES, iMsg);
		}
		return lr;
	}
	return CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);
}

/*-----------------------------------------------------------------------------
	拡張エディットボックスメッセージ処理
-----------------------------------------------------------------------------*/
LRESULT CALLBACK EDsHell(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	PPxEDSTRUCT *PES;
	PPECOMMANDPARAM cparam;

	PES = (PPxEDSTRUCT *)GetProp(hWnd, StrPPxED);
	if ( PES == NULL ) return DefWindowProc(hWnd, iMsg, wParam, lParam);
	switch (iMsg){
		case WM_CTLCOLORLISTBOX:
			if ( !(ThemeColors.ExtraDrawFlags & (EDF_WINDOW_TEXT | EDF_WINDOW_BACK)) ){
				return DefWindowProc(hWnd, iMsg, wParam, lParam);
			}
			return ControlWindowColor(wParam);

		case WM_NCLBUTTONDOWN:
			if ( (PES->flags & PPXEDIT_LINE_MULTI) && (wParam == HTVSCROLL) ){
				LineMulti_VScrollDown(hWnd, PES->fontY, lParam);
				return 0;
			}
			goto nextproc;

		case WM_IME_NOTIFY:
			// ウィンドウの位置が変化したときに通知される→
			// リストウィンドウの位置変更に利用する
			if ( (wParam == IMN_SETCOMPOSITIONWINDOW) &&
				 (PES->list.hWnd != NULL) ){
				PostMessage(hWnd, WM_PPXCOMMAND, KE_changepos, 0);
			}
			goto nextproc;

		case WM_DROPFILES:		// ドロップファイルの名前取込
			SetDropFiles(hWnd, (HDROP)wParam);
			goto nextproc;

		case WM_KILLFOCUS:
			if ( PES->FloatBar.hWnd != NULL ) DestroyWindow(PES->FloatBar.hWnd);
		// WM_LBUTTONDOWN へ続く
		case WM_LBUTTONDOWN:
									// ヒストリなどのリストがあるなら閉じる
			if ((PES->list.hWnd != NULL) && ((HWND)wParam != PES->list.hWnd) ){
				PostMessage(PES->list.hWnd, WM_CLOSE, 0, 0);
			}
			if ((PES->list.hSubWnd != NULL) && ((HWND)wParam != PES->list.hSubWnd) ){
				PostMessage(PES->list.hSubWnd, WM_CLOSE, 0, 0);
			}
			break; // line cursor 処理有り

		case WM_MOUSEWHEEL:
			if ( wParam & MK_CONTROL ){
				if ( wParam & MK_SHIFT ){
					ChangeOpaqueWindow(hWnd, (wParam & B31) ? -1 : 1);
				}else{
					ChangeFontSize(PES, (wParam & B31) ? -1 : 1);
				}
				return 0;
			}

			if ( PES->flags & PPXEDIT_COMBOBOX ){
				return SendMessage(GetParent(hWnd), WM_MOUSEWHEEL, wParam, lParam);
			}

			if ( PES->list.ListFocus != LISTU_NOLIST ){
				return SendMessage( (PES->list.ListFocus == LISTU_FOCUSSUB) ?
					PES->list.hSubWnd : PES->list.hWnd,
					WM_MOUSEWHEEL, wParam, lParam);
			}

			if ( !(PES->flags & PPXEDIT_TEXTEDIT) || (PES->list.ListFocus != LISTU_NOLIST) ){ // ヒストリ参照
				if ( HISHORTINT(wParam) >= 0 ){
					cparam.key = K_up;
				}else{
					cparam.key = K_dw;
				}
				cparam.repeat = 0;
				PES->mousepos = TRUE;
				PPedCommand(PES, &cparam);
				return 0;
			}
			goto nextproc;

		case WM_MOUSEACTIVATE:
			if ( PES->flags & PPXEDIT_PANEMODE ){ //要アクティブ化検出
				// アクティブ化のみ
				if ( GetFocus() == NULL ){
					// アクティブ時のクリック検出を無効にする
					return MA_ACTIVATEANDEAT; // アクティブ & マウス メッセージを廃棄
				}else{
					return MA_ACTIVATE; // アクティブ継続
				}
			}
			goto nextproc;

		case WM_COMMAND:
			if ( HIWORD(wParam) == LBN_SELCHANGE ){	// リスト選択？
				ListSelect(PES, (HWND)lParam);
			}
			goto nextproc;

		case WM_MENUCHAR:
		case WM_MENUSELECT:
		case WM_MENUDRAG:
		case WM_MENURBUTTONUP:
			return PPxMenuProc(hWnd, iMsg, wParam, lParam);

		case WM_LBUTTONUP:
			if ( PES->FloatBar.hWnd == NULL ){
				InitFloatBar(PES, LOSHORTINT(lParam));
			}
			goto nextproc;

		case WM_RBUTTONUP:
			// ctrl/shift有りなら元の処理を行う
			if ( (wParam & (MK_SHIFT | MK_CONTROL)) != 0 ) goto nextproc;
			cparam.key = K_a | ' ';
			cparam.repeat = 0;
			PES->mousepos = TRUE;
			PPedCommand(PES, &cparam);
			return 0;

		case WM_SYSKEYDOWN:
														// Alt 系の使用を禁止
			if ( !(PES->flags & PPXEDIT_USEALT) &&
				 ((WORD)wParam >= '0') &&
				 ((WORD)wParam <= VK_DIVIDE)){
				goto nextproc;
			}
			// WM_KEYDOWN へ
		case WM_KEYDOWN:
			if ( PES->list.flags & LISTFLAG_DISABLE ) goto nextproc;

			cparam.key = (WORD)(wParam | GetShiftKey() | K_v);
			cparam.repeat = lParam & B30;
			PES->mousepos = FALSE;

#ifndef WINEGCC
			/* ※ WM_GETDLGCODE の処理で、リストがないと WM_SYSCHAR がこなく、
			 リストがあると WM_SYSCHAR が来るため、調整する */
			if ( (cparam.key & K_a) && IsalnumA(cparam.key) &&
				 !( PES->flags & PPXEDIT_TEXTEDIT ) &&
				 (PES->list.ListFocus == LISTU_NOLIST) ){
				resetflag(cparam.key, K_v);
			}
#endif
			if ( PES->flags & PPXEDIT_WANTALLKEY ){
				if ( ((wParam == VK_RETURN) &&
					  ((PES->flags & (PPXEDIT_WANTENTER | PPXEDIT_TEXTEDIT)) != (PPXEDIT_WANTENTER | PPXEDIT_TEXTEDIT)) ) ||
					(wParam == VK_TAB) || (wParam == VK_ESCAPE) ){
					RemoveCharKey(PES->hWnd); // WM_CHAR の各キーを廃棄
				}
			}

			if ( PES->KeyHookEntry != NULL ){
				if ( CallEditKeyHook(PES, cparam.key) != PPXMRESULT_SKIP ){
					return 0;
				}
			}

			if ( PPedCommand(PES, &cparam) != ERROR_INVALID_FUNCTION ){
				if ( (cparam.key & K_a) &&
					 (T_CHRTYPE[(unsigned char)(cparam.key)] & (T_IS_DIG | T_IS_UPP)) ){
					SkipAltLetterKeyDef(hWnd);
				}
				return 0;
			}

			if ( wParam == VK_MENU ){ // [ALT] はメニュー移行を禁止
				if ( GetCustDword(T("X_alt"), 1) ) return 0;
			}
			break; // line cursor 処理有り

		#ifdef UNICODE
		case WM_IME_CHAR:
			#ifdef UNICODE
				if ( PES->list.flags & LISTFLAG_DISABLE ) goto nextproc;
				// WM_CHAR → iMsg にすると WM_CHAR を通過してしまう
				return CharProc(PES, hWnd, WM_CHAR, wParam, lParam);
			#else // Win9x, WinNTで漢字が入力できないため、休止中
				unsigned char charcode;

				if ( PES->list.flags & LISTFLAG_DISABLE ) goto nextproc;
				charcode = (BYTE)((wParam >> 8) & 0xff);
				if ( charcode != '\0' ){
					CharProc(PES, hWnd, WM_CHAR, charcode, lParam);
				}
				return CharProc(PES, hWnd, WM_CHAR, wParam & 0xff, lParam);
			#endif
		#endif

		case WM_IME_COMPOSITION:
			if ( !(PES->flags & PPXEDIT_RICHEDIT) ||
				 !(lParam & GCS_RESULTSTR) ){
				return CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);
			}else{
				LRESULT lr;

				lr = CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);
				KeyStepFill(PES, NULL);
				if ( PES->flags & PPXEDIT_SYNTAXCOLOR ) RichSyntaxColor(PES);
				return lr;
			}

		case WM_SYSCHAR:
			if ( !(PES->flags & PPXEDIT_USEALT) ) goto nextproc; // Alt 系の使用を禁止
		//	WM_CHAR へ

		case WM_CHAR:
			if ( PES->list.flags & LISTFLAG_DISABLE ) goto nextproc;
			if ( (WORD)wParam < 0x80 ){ // 英数字ならカスタマイズ処理する
				cparam.key = FixCharKeycode((WORD)wParam);
				cparam.repeat = lParam & B30;
				PES->mousepos = FALSE;

				if ( PES->KeyHookEntry != NULL ){
					if ( CallEditKeyHook(PES, cparam.key) != PPXMRESULT_SKIP ){
						return 0;
					}
				}
				if ( PPedCommand(PES, &cparam) != ERROR_INVALID_FUNCTION ){
					return 0;
				}
			}
			return CharProc(PES, hWnd, iMsg, wParam, lParam);

		case WM_CLOSE:
			setflag(PES->list.flags, LISTFLAG_DISABLE);
			CancelListThread(PES);
			return CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);

		case WM_DESTROY: {
			WNDPROC hOldED;

			setflag(PES->list.flags, LISTFLAG_DISABLE);
			CancelListThread(PES);

			if ( PES->flags & PPXEDIT_WANTEVENT ){
				if ( IsExistCustTable(StrK_edit, StrCLOSEEVENT) ||
					 ((PES->AKey != NULL) && IsExistCustTable(PES->AKey, StrCLOSEEVENT)) ){
					EditPPeKeyCommand(PES, K_E_CLOSE);
				}
				CallCloseEvent(PES);
			}

			if ( PES->list.ListFocus != LISTU_NOLIST ) CloseLineList(PES);
			FreeBackupText(PES);

			SearchFileIned(&PES->ED, NilStrNC, NULL, 0);
			hOldED = PES->hOldED;

			FreeFillTextFile(&PES->list.filltext_user);
			if ( PES->findrep != NULL ){
				HeapFree(ProcHeap, 0, PES->findrep);
			}
			if ( PES->FloatBar.hWnd != NULL ){
				DestroyWindow(PES->FloatBar.hWnd);
			}
			ThFree(&PES->LocalStringValue);
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)hOldED );
			RemoveProp(hWnd, StrPPxED);
			if ( PES->RichEdit != NULL ){
				 PES->RichEdit->lpVtbl->Release(PES->RichEdit);
				 PES->RichEdit = NULL;
			}
			PES->info = &DummyPPxAppInfo;
			FreeEdStruct(PES);
			return CallWindowProc(hOldED, hWnd, iMsg, wParam, lParam);
		}

		case WM_SETFONT:
			return EditSetFont(PES, (HFONT)wParam, lParam);

		case WM_GETFONT: {
			LRESULT lr;

			lr = CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);
			if ( lr != 0 ) return lr;
			return (LRESULT)PES->hEditFont;
		}

		case WM_GETDLGCODE: {
			LRESULT retcode;

			if ( PES->list.ListFocus != LISTU_NOLIST ){
				return DLGC_WANTALLKEYS;
			}
			retcode = 0;
			if ( PES->flags & PPXEDIT_TABCOMP ) retcode = DLGC_WANTTAB;
			if ( PES->flags & PPXEDIT_WANTALLKEY ){
				if ( PES->flags & PPXEDIT_LINE_MULTILINE ){
					return DLGC_WANTALLKEYS;
				}
				retcode = DLGC_WANTALLKEYS | DLGC_BUTTON;
			}
			return CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam) | retcode;
		}

		case WM_VSCROLL:
			if ( PES->flags & PPXEDIT_TEXTEDIT ) goto nextproc;
			SetFocus(hWnd);
			switch ( LOWORD(wParam) ){
				case SB_LINEUP:
					if ( X_flst_mode >= X_fmode_auto2 ){
						if ( PES->list.ListFocus != LISTU_NOLIST ){
							CloseLineList(PES);
						}else{
							KeyStepFill(PES, KeyStepFill_listmode);
						}
					}else{
						FloatList(PES, EDITDIST_BACK);
					}
					break;

				case SB_LINEDOWN:
					FloatList(PES, EDITDIST_NEXT);
					break;

				// default:
			}
			return 0;

		case WM_ENABLE:
			if ( (BOOL)wParam == FALSE ) ClosePPeTreeWindow(PES);
			goto nextproc;

		case WM_GESTURE:
			WMGestureEdit(hWnd, wParam/*, lParam*/);
			goto nextproc;

		case WM_COPYDATA:
			return PPWmCopyData(PES->info, (COPYDATASTRUCT *)lParam);

		default:
			if ( iMsg == WM_PPXCOMMAND ){
				LRESULT result;

				result = EdPPxWmCommand(PES, hWnd, wParam, lParam);
				if ( result != (LRESULT)MAX32 ) return result;
				goto nextproc;
			}else if ( iMsg == ReplaceDialogMessage ){
				PPeReplaceStrCommand(PES, (FINDREPLACE *)lParam);
				goto nextproc;
			}
			break;
	}
	// ※PPXEDIT_LINECSRは、ES_MULTILINE のときしか有効になっていない
	if ( (PES->flags & (PPXEDIT_LINECSR | PPXEDIT_LINE_MULTI | PPXEDIT_LINE_MULTILINE)) &&
		// 後、edit message と wm_paint の時に使用する
		 ( ((iMsg >= EM_SETSEL) && (iMsg <= EM_SETREADONLY)) ||
		   (iMsg == WM_PAINT) ||
		   (iMsg == WM_KEYDOWN) ||
		   (iMsg == WM_LBUTTONDOWN) ) ){
		LRESULT lr = CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);

		if ( PES->flags & (PPXEDIT_LINE_MULTI | PPXEDIT_LINE_MULTILINE) ){
			LineExpand(hWnd, PES);
		}else{
			LineCursor(PES, iMsg);
		}
		return lr;
	}
nextproc:
	return CallWindowProc(PES->hOldED, hWnd, iMsg, wParam, lParam);
}

void InsertAndSelect(HWND hWnd, const TCHAR *text, BOOL spacebraket)
{
	DWORD wP, lP;

	SendMessage(hWnd, EM_GETSEL, (WPARAM)&wP, (LPARAM)&lP);
	lP = tstrlen32(text);
#ifndef UNICODE
	if ( xpbug < 0 ) CaretFixToW(text, &lP);
#endif
	if ( spacebraket && (tstrchr(text, ' ') != NULL) ){
		SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)T("\"\""));
		SendMessage(hWnd, EM_SETSEL, wP + 1, wP + 1);
		lP += 2;
	}
	SendMessage(hWnd, EM_REPLACESEL, 1, (LPARAM)text);
	SendMessage(hWnd, EM_SETSEL, wP, wP + lP);
}

// 戻り値が MAX32 のときは、何もしなかった扱いになる。→ラインカーソル処理有り
LRESULT EdPPxWmCommand(PPxEDSTRUCT *PES, HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch ( LOWORD(wParam) ){
		case KE_insertsel: // *insertsel 本体
			InsertAndSelect(hWnd, (const TCHAR *)lParam, FALSE);
			break;

		case KE_insert: // *insert 本体
			SendMessage(hWnd, EM_REPLACESEL, 1, lParam);
			break;

		case KE_replace:
			Edit_SetWindowText(PES, (TCHAR *)lParam);
			Edit_SendMessage(PES, EM_SETSEL, 0, EC_LAST);
			break;

		case KE_closecheck:
			if ( (PES->flags & PPXEDIT_DISABLE_CHECKSAVE) ||
				 (Edit_SendMessageMsg(PES, EM_GETMODIFY) == FALSE) ){
				if ( (PES->flags & (PPXEDIT_SAVE_BYCLOSE | PPXEDIT_DISABLE_CHECKSAVE)) == (PPXEDIT_SAVE_BYCLOSE | PPXEDIT_DISABLE_CHECKSAVE) ){ // 強制保存
					if ( FileSave(PES, EDL_FILEMODE_NODIALOG) == FALSE ){
						break; // 保存失敗したので中止
					}
				}
				return 1; // 無視 || 変更無し→そのまま終了を許可
			}else{
				int result;

				result = PMessageBox(hWnd, MES_QSAV, T("PPe"),
					MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON1);
				if ( result == IDYES ){
					if ( FileSave(PES, EDL_FILEMODE_NODIALOG) ) return 1;
				}else if ( result == IDNO ){
					return 1;
				}
			}
			return 0;

		case KE_openfile:
			return OpenFromFile(PES, PPE_OPEN_MODE_OPEN, (const TCHAR *)lParam);

		case KE_opennewfile:
			return OpenFromFile(PES, PPE_OPEN_MODE_OPENNEW, (const TCHAR *)lParam);
		case KE_excmdopen:
			return OpenFromFile(PES, HIWORD(wParam), (const TCHAR *)lParam);

		case KE_seltext:
			return CmdSelText(PES, (TCHAR *)lParam);

		case KE__FREE: {
			WNDPROC hOldED;

			SearchFileIned(&PES->ED, NilStrNC, NULL, 0);
			hOldED = PES->hOldED;
			RemoveProp(hWnd, StrPPxED);
			FreeEdStruct(PES);
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)hOldED);
			return NO_ERROR;
		}

		case KE_setkeya:
			if ( wParam != KE_firstevent_enable ){
				PES->AKey = (const TCHAR *)lParam;
			}
			if ( wParam != KE_setkeya ){
				setflag(PES->flags, PPXEDIT_WANTEVENT); // K_E_CLOSE 送信対応
				if ( IsExistCustTable(StrK_edit, StrFIRSTEVENT) ||
					 ((PES->AKey != NULL) && IsExistCustTable(PES->AKey, StrFIRSTEVENT)) ){
					PostMessage(PES->hWnd, WM_PPXCOMMAND, K_E_FIRST, 0);
				}
			}
			return NO_ERROR;

		case KTN_close:
			ClosePPeTreeWindow(PES);
//		case KTN_focus: へ
		case KTN_escape:
		case KTN_focus:
			SetFocus(hWnd);
			break;

		case KTN_select:
		case KTN_selected:
			if ( PES->flags & PPXEDIT_SINGLEREF ){
				Edit_SetWindowText(PES, (TCHAR *)lParam);
				Edit_SendMessage(PES, EM_SETSEL, EC_LAST, EC_LAST);
			}else{
				InsertAndSelect(hWnd, (const TCHAR *)lParam, TRUE);
			}
			if ( LOWORD(wParam) == KTN_selected ){
				ClosePPeTreeWindow(PES);
				SetFocus(hWnd);
			}
			break;

		case KE_closeUList:
			CloseUpperList(PES);
			break;

		case KE_scrapwindow:
			RemoveProp(hWnd, StrPPxED);
			FreeEdStruct(PES);
			DestroyWindow(hWnd);
			return NO_ERROR; // CallWindowProc を実行させない

		case KE_changepos:
			FixListPosition(PES);
			break;

		case KE_setselectCB:
			SetSelectCombbox(hWnd);
			break;

		case K_EXECUTE:
			EditExtractMacro(PES, (TCHAR *)lParam, NULL, 0);
			break;

		default:
			PES->mousepos = FALSE;
			return EditPPeKeyCommand(PES, (DWORD)wParam);
	}
	return MAX32;
}

//------------------------------------
void RegisterSHAutoComplete(HWND hRealED)
{
	if ( DSHAutoComplete == NULL ){
		if ( LoadSystemWinAPI(SYSTEMDLL_SHLWAPI, SHLWAPIDLL) == NULL ){
			return;
		}
	}
	#pragma warning(suppress:6001 6011) // LoadSystemWinAPI で初期化
	DSHAutoComplete(hRealED, SHACF_FILESYSTEM | // SHACF_USETAB | ←これを付けると、W2kでBS/DELの挙動がおかしくなる？
			SHACF_AUTOAPPEND_FORCE_ON | SHACF_AUTOSUGGEST_FORCE_ON);
}

// | で範囲選択可能な SetWindowText
BOOL SetWindowTextWithSelect(HWND hEditWnd, const TCHAR *defstr)
{
	DWORD firstC, lastC;
	TCHAR *firstp = SearchVLINE(defstr), *lastp;

	if ( firstp == NULL ){
		SetWindowText(hEditWnd, defstr);
		return FALSE;
	}else{
		TCHAR strbuf[CMDLINESIZE];

		tstplimcpy(strbuf, defstr, CMDLINESIZE);
		firstp = strbuf + (firstp - defstr);
		memmove( firstp , firstp + 1 , TSTRSIZE(firstp + 1) );

		lastp = SearchVLINE(strbuf);
		if ( lastp != NULL ){
			memmove( lastp , lastp + 1 , TSTRSIZE(lastp + 1) );
		}else{
			lastp = firstp;
		}

		SetWindowText(hEditWnd, strbuf);
		firstC = firstp - strbuf;
		lastC = lastp - strbuf;
#ifndef UNICODE
		if ( xpbug == 0 ){
			xpbug = IsWindowUnicode(hEditWnd) ? -1 : 1;
		}
		if ( xpbug < 0 ){
			CaretFixToW(strbuf, (DWORD *)&firstC);
			CaretFixToW(strbuf, (DWORD *)&lastC);
		}
#endif
		SendMessage(hEditWnd, EM_SETSEL, (WPARAM)firstC, (LPARAM)lastC);
		return TRUE;
	}
}

/*
	Action 例
		ダブルクリックによる範囲選択:	WB_LEFT - WB_RIGHT
		表示のための改行判定:			WB_ISDELIMITER
		単語移動(Ctrl+left)				WB_LEFT - WB_RIGHT
		単語移動(Ctrl+right)			WB_ISDELIMITER - WB_RIGHT - WB_RIGHT
*/
int wordbreak_action_right = 0; // WB_RIGHT の繰返し回数

int SearchLeftWord(LPWSTR lpch, int ichCurrent)
{
	// 単語部分をスキップ
	if ( (ichCurrent > 0) && (*(lpch + ichCurrent - 1) <= ' ') ){
		for (;;){
			if ( ichCurrent <= 0 ) return 0;
			ichCurrent--;
			if ( *(lpch + ichCurrent) > ' ' ) break;
		}
	}
	// 区切り部分をスキップ
	for (;;){
		if ( ichCurrent <= 0 ) return 0;
		if ( *(lpch + ichCurrent - 1) <= ' ' ) return ichCurrent;
		ichCurrent--;
	}
}

int SearchLeftSeparator(LPWSTR lpch, int ichCurrent, TCHAR sepchr)
{
	// 単語部分をスキップ
	if ( (ichCurrent > 0) && (*(lpch + ichCurrent - 1) == sepchr) ){
		for (;;){
			if ( ichCurrent <= 0 ) return 0;
			ichCurrent--;
			if ( *(lpch + ichCurrent) != sepchr ) break;
		}
	}
	// 区切り部分をスキップ
	for (;;){
		if ( ichCurrent <= 0 ) return 0;
		if ( *(lpch + ichCurrent - 1) == sepchr ) return ichCurrent;
		ichCurrent--;
	}
}

int SearchRightWord(LPWSTR lpch, int ichCurrent, int cch)
{
	// 区切り部分をスキップ
	if ( *(lpch + ichCurrent) <= ' ' ){
		for (;;){
			if ( ichCurrent >= cch ) return cch;
			if ( *(lpch + ichCurrent + 1) > ' ' ) break;
			ichCurrent++;
		}
		return ichCurrent;
	}
	// 単語部分をスキップ
	for (;;){
		if ( ichCurrent >= cch ) return cch;
		if ( *(lpch + ichCurrent) <= ' ' ) return ichCurrent;
		ichCurrent++;
	}
}

int SearchRightSeparator(LPWSTR lpch, int ichCurrent, int cch, TCHAR sepchr)
{
	// 区切り部分をスキップ
	if ( *(lpch + ichCurrent) == sepchr ){
		for (;;){
			if ( ichCurrent >= cch ) return cch;
			if ( *(lpch + ichCurrent + 1) != sepchr ) break;
			ichCurrent++;
		}
		return ichCurrent;
	}
	// 単語部分をスキップ
	for (;;){
		if ( ichCurrent >= cch ) return cch;
		if ( *(lpch + ichCurrent) == sepchr ) return ichCurrent;
		ichCurrent++;
	}
}

// RichEdit 用の WB action
#ifndef WB_CLASSIFY
#define WB_CLASSIFY 		3
#define WB_MOVEWORDLEFT 	4
#define WB_MOVEWORDRIGHT	5
#define WB_LEFTBREAK		6
#define WB_RIGHTBREAK		7
#endif

// Edit: XP以降の新コントロールは UNICODE 専用
// RichEdit: V2 以降は UNICODE 専用

int CALLBACK LineEditWordBreakProcW(LPWSTR lpch, int ichCurrent, int cch, int action)
{
	int newpos;

	switch ( action ){
		case WB_LEFT: // EDIT
			wordbreak_action_right = 0;
			if ( ichCurrent == 0 ) return 0;
			newpos = SearchLeftWord(lpch, ichCurrent);
			if ( newpos != 0 ) return newpos;
			newpos = SearchLeftSeparator(lpch, ichCurrent, '\\');
			if ( newpos != 0 ) return newpos;
			newpos = SearchLeftSeparator(lpch, ichCurrent, '/');
			if ( newpos != 0 ) return newpos;
			return SearchLeftSeparator(lpch, ichCurrent, '.');

		// WB_RIGHT は WB_ISDELIMITER が 常に TRUE のため、常に2回呼ばれるので
		// 2回目は動作をスキップする
		case WB_RIGHT: // EDIT
			if ( wordbreak_action_right++ == 1 ) return ichCurrent;
			if ( ichCurrent >= cch ) return cch;
			newpos = SearchRightWord(lpch, ichCurrent, cch);
			if ( newpos < cch ) return newpos;
			newpos = SearchRightSeparator(lpch, ichCurrent, cch, '\\');
			if ( newpos < cch ) return newpos;
			newpos = SearchRightSeparator(lpch, ichCurrent, cch, '/');
			if ( newpos < cch ) return newpos;
			return SearchRightSeparator(lpch, ichCurrent, cch, '.');
	}
	wordbreak_action_right = 0;
	//	WB_ISDELIMITER ... 常に区切りにすることで、桁折り時の WordBreak を調整
	return TRUE;
}

int CALLBACK LineRichEditWordBreakProcW(LPWSTR lpch, int ichCurrent, int cch, int action)
{
	int newpos;

	switch ( action ){
		case WB_MOVEWORDLEFT: // RichEdit
		case WB_LEFT: // EDIT
			wordbreak_action_right = 0;
			if ( ichCurrent == 0 ) return 0;
			newpos = SearchLeftWord(lpch, ichCurrent);
			if ( newpos != 0 ) return newpos;
			newpos = SearchLeftSeparator(lpch, ichCurrent, '\\');
			if ( newpos != 0 ) return newpos;
			newpos = SearchLeftSeparator(lpch, ichCurrent, '/');
			if ( newpos != 0 ) return newpos;
			return SearchLeftSeparator(lpch, ichCurrent, '.');

		case WB_MOVEWORDRIGHT: // RichEdit
		#ifndef UNICODE
			cch /= 2;
		#endif
			if ( *(lpch + ichCurrent) <= ' ' ){
				for (;;){
					if ( ichCurrent >= cch ) return cch;
					ichCurrent++;
					if ( *(lpch + ichCurrent) > ' ' ) break;
				}
			}
			if ( ichCurrent >= cch ) return cch;
			newpos = SearchRightWord(lpch, ichCurrent, cch);
			if ( newpos < cch ) return newpos;
			newpos = SearchRightSeparator(lpch, ichCurrent, cch, '\\');
			if ( newpos < cch ) return newpos;
			newpos = SearchRightSeparator(lpch, ichCurrent, cch, '/');
			if ( newpos < cch ) return newpos;
			return SearchRightSeparator(lpch, ichCurrent, cch, '.');

		// WB_RIGHT は WB_ISDELIMITER が 常に TRUE のため、常に2回呼ばれるので
		// 2回目は動作をスキップする
		case WB_RIGHT: // EDIT
			return ichCurrent;

		case WB_LEFTBREAK: // RichEdit 表示時
			return 0;
	}
	wordbreak_action_right = 0;
	//	WB_ISDELIMITER ... 常に区切りにすることで、桁折り時の WordBreak を調整
	return TRUE;
}

//#define EM_SETWORDBREAKPROCEX	(WM_USER + 81)

// ●D2D ではEM_SETWORDBREAKPROCが使えない？

void InitExEdit(PPxEDSTRUCT *PES)
{
	if ( PES->style & ES_MULTILINE ){
		if ( PES->flags & PPXEDIT_LINE_MULTI ){
			PES->caretLY = 1;
			if ( WinType >= WINTYPE_VISTA ){
				SendMessage(PES->hWnd, EM_SETWORDBREAKPROC, 0,
					(LPARAM)((PES->flags & PPXEDIT_RICHEDIT) ?
						LineRichEditWordBreakProcW : LineEditWordBreakProcW));
			}
		}else{
			setflag(PES->flags, PPXEDIT_TEXTEDIT);
			if ( GetCustDword(T("X_ucsr"), 1) ){
				setflag(PES->flags, PPXEDIT_LINECSR);
			}
			if ( (WinType >= WINTYPE_VISTA) &&
				 (PES->flags & PPXEDIT_NOWORDBREAK) ){
				SendMessage(PES->hWnd, EM_SETWORDBREAKPROC, 0,
					(LPARAM)((PES->flags & PPXEDIT_RICHEDIT) ?
						LineRichEditWordBreakProcW : LineEditWordBreakProcW));
			}
		}
	}
	SetProp(PES->hWnd, StrPPxED, (HANDLE)PES);
}

void InitRichMode(PPxEDSTRUCT *PES, HWND hRealED, HFONT hEditFont)
{
	CHARFORMAT2 chfmt;
//	PARAFORMAT2 paragfmt;
	LOGFONT lfont;
	TCHAR classname[128];
	IUnknown *Unknown;

	GetClassName(hRealED, classname, TSIZEOF(classname));
	if ( TinyCharLower(classname[0]) != 'r' ) return;
	setflag(PES->flags, PPXEDIT_RICHEDIT);
	PES->fontYext = 4;
	PES->hEditFont = hEditFont;
	PES->RichEdit = NULL;

	SendMessage(hRealED, EM_GETOLEINTERFACE, 0, (LPARAM)&Unknown);
	if ( Unknown != NULL ){
		if ( FAILED(Unknown->lpVtbl->QueryInterface(Unknown, &xIID_ITextDocument, (void **)&PES->RichEdit)) ){
			PES->RichEdit = NULL;
		}
		Unknown->lpVtbl->Release(Unknown);
	}

	// フォント設定
	// ASCII フォントと日本語フォントを共通にする
	// IMF_AUTOFONT ?
	SendMessage(hRealED, EM_SETLANGOPTIONS, 0,
			SendMessage(hRealED, EM_GETLANGOPTIONS, 0, 0) & ~IMF_DUALFONT );

	// 規定フォントを取得
	if ( hEditFont == NULL ){
		chfmt.cbSize = sizeof(chfmt);
		chfmt.dwMask = CFM_SIZE | CFM_FACE | CFM_CHARSET | CFM_LCID;
		SendMessage(hRealED, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&chfmt);
	}else{
		GetObject(hEditFont, sizeof(LOGFONT), &lfont);
		chfmt.yHeight = TwipToPixel(lfont.lfHeight, 96);
		tstrcpy(chfmt.szFaceName, lfont.lfFaceName);
		chfmt.bCharSet = lfont.lfCharSet;
		chfmt.bPitchAndFamily = lfont.lfPitchAndFamily;
	}
	if ( chfmt.yHeight < 0 ) chfmt.yHeight = -chfmt.yHeight;

	// 背景色設定
	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwEffects = CFE_AUTOCOLOR;
	if ( ThemeColors.ExtraDrawFlags & (EDF_WINDOW_TEXT | EDF_WINDOW_BACK) ){
		SendMessage(hRealED, EM_SETBKGNDCOLOR, 0, (LPARAM)C_WindowBack);
		chfmt.dwEffects = 0;
		chfmt.crTextColor = C_WindowText;
	}

	// フォント設定
	chfmt.dwMask = CFM_COLOR | CFM_SIZE | CFM_FACE | CFM_CHARSET | CFM_LCID;
	chfmt.lcid = GetUserDefaultLCID();
		// SCF_DEFAULT は 0 なので別個に指定
	SendMessage(hRealED, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&chfmt);
	SendMessage(hRealED, EM_SETCHARFORMAT, SPF_SETDEFAULT | SCF_SELECTION | SCF_ALL, (LPARAM)&chfmt);
#if 0
	if ( chfmt.bCharSet == SHIFTJIS_CHARSET ){
		chfmt.bCharSet = ANSI_CHARSET;
		chfmt.lcid = LCID_ENGLISH;
		SendMessage(hRealED, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&chfmt);
		SendMessage(hRealED, EM_SETCHARFORMAT, SPF_SETDEFAULT | SCF_SELECTION | SCF_ALL, (LPARAM)&chfmt);
}
#endif
	{	// 行間の算出。本当は twip 単位っぽいので、行数が嵩むと誤差が出る。
		PARAFORMAT2 prfmt;
		prfmt.cbSize = sizeof(prfmt);
		prfmt.dwMask = PFM_LINESPACING;
		SendMessage(PES->hWnd, EM_GETPARAFORMAT, 0, (LPARAM)&prfmt);

		#ifdef UNICODE
			#define YEXT 5
		#else
			#define YEXT 4
		#endif

		switch ( prfmt.bLineSpacingRule ){
			case 0:
				PES->fontYext = YEXT; // 6pt?
				break;

			case 1:
				PES->fontYext = YEXT / 2; // 3pt?
				break;

			case 2:
				PES->fontYext = YEXT * 2; // 12pt?
				break;

			case 3:
				PES->fontYext = TwipToPixel(prfmt.dyLineSpacing, 96);
				if ( PES->fontYext < YEXT ) PES->fontYext = YEXT;
				break;

			case 4:
				PES->fontYext = TwipToPixel(prfmt.dyLineSpacing, 96);
				break;

			case 5:
				PES->fontYext = (prfmt.dyLineSpacing * PES->fontY) / 20;
				break;
		}
	}

	SendMessage(hRealED, EM_SETOPTIONS, ECOOP_OR, ECO_NOHIDESEL);
	SendMessage(hRealED, EM_SETTARGETDEVICE, 0, 0); // 右端で折り返す

	if ( PES->flags & (PPXEDIT_LINE_MULTI | PPXEDIT_LINE_MULTILINE) ){
		SendMessage(hRealED, EM_SETOPTIONS, ECOOP_AND, ~(ECO_AUTOHSCROLL | ECO_AUTOVSCROLL));
		SendMessage(hRealED, EM_SHOWSCROLLBAR, SB_VERT, FALSE); // ●垂直スクロールバーを消す操作をすると、なぜか常時垂直が表示されるようになる。
	}
	if ( PES->flags & PPXEDIT_SUGGEST ){
		SetWindowLong(PES->hWnd, GWL_STYLE,
				GetWindowLong(PES->hWnd, GWL_STYLE) |
					ES_DISABLENOSCROLL | WS_VSCROLL);
		// クライアント領域を再計算させて、スクロールバーを表示させる
		SetWindowPos(PES->hWnd, NULL, 0, 0, 0, 0,
				SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	}

	if ( X_csyh < 0 ){
		X_csyh = 0;
		GetCustData(T("X_csyh"), &X_csyh, sizeof(X_csyh));
		((COLORREF *)&SynColor)[1] = C_S_AUTO - 1;
	}
}

/*-----------------------------------------------------------------------------
	拡張エディットボックスを登録する
-----------------------------------------------------------------------------*/
PPXDLL HWND PPXAPI PPxRegistExEdit(PPXAPPINFO *info, HWND hEditWnd, int maxlen, const TCHAR *defstr, WORD rHist, WORD wHist, int flags)
{
	PPxEDSTRUCT *PES;
	HWND hRealED;
	DWORD X_ltab[2] = Default_X_ltab;
	HFONT hEditFont;

	GetCustData(T("X_ltab"), &X_ltab, sizeof(X_ltab));
	if ( X_ltab[0] != 0 ) setflag(flags, PPXEDIT_TABCOMP);
	GetCustData(T("X_flst"), &X_flst, sizeof(X_flst));
	if ( X_flst_mode != X_fmode_off ) setflag(flags, PPXEDIT_LISTCOMP);

	if ( !(flags & PPXEDIT_COMBOBOX) ){	// 対象がエディットボックス ===========
		if ( maxlen != 0 ){
			#ifndef UNICODE
			if ( (maxlen > 0x8000) && (WinType == WINTYPE_9x) ){
				SendMessage(hEditWnd, EM_LIMITTEXT, 0x8000 - 1, 0);
			}else
			#endif
			{
				SendMessage(hEditWnd, EM_LIMITTEXT, maxlen - 1, 0);
			}
		}
		if ( (defstr != NULL) && (*defstr != '\0') ){
			if ( flags & PPXEDIT_INSTRSEL ){
				SetWindowTextWithSelect(hEditWnd, defstr);
			}else{
				SetWindowText(hEditWnd, defstr);
				SendMessage(hEditWnd, EM_SETSEL, 0, EC_LAST);
			}
		}
		hRealED = hEditWnd;
	}else{								// 対象がコンボボックス ===============
		hRealED = PPxRegistExEditCombo(hEditWnd, maxlen, defstr, rHist, wHist, flags);
	}
#ifndef UNICODE
	if ( xpbug == 0 ){
		xpbug = IsWindowUnicode(hRealED) ? -1 : 1;
	}
#endif
	PES = GetProp(hRealED, StrPPxED);
	if ( PES == NULL ){					// 作業領域を確保 ---------------------
		PES = HeapAlloc(DLLheap, HEAP_ZERO_MEMORY, sizeof(PPxEDSTRUCT));
		if ( PES == NULL ) return NULL;
		PES->ref = 1;
		PES->hWnd = hRealED;
//		PES->hTreeWnd = NULL;
//		PES->hOldED = NULL;
//		PES->AKey = NULL;
//		PES->KeyHookEntry = NULL;
//		PES->findstring[0] = '\0';
		PES->ED.hF = NULL;

		if ( X_ltab[1] == 0 ){
			if ( X_flst_part && (X_ltab[0] < 2) ) X_ltab[0] = 2;
			X_ltab[1] = X_ltab[0];
		}
		SetMatchMode(PES, X_ltab[1]);
		FixUxTheme(hRealED, EditClassName);
	}	// PES != NULL ... 登録済み→設定の変更のみ行う

										// プロージャを設定 -------------------
	PES->info = PES->ED.info = (info != NULL) ? info : &PPxDefInfo;
//	PES->list.index = 0;
	PES->list.WhistID = wHist;
	PES->list.RhistID = rHist;
	PES->list.OldString = NULL;
	PES->list.ListFocus = LISTU_NOLIST;
	PES->list.flags = 0;
//	PES->list.filltext_user.filename = NULL;
//	PES->list.filltext_user.mem = NULL;
//	PES->list.filltext_user.text = NULL;
//	PES->list.filltext_user.ref = 0;

	PES->flags = flags;
	PES->oldkey2 = 0;

	hEditFont = (HFONT)SendMessage(hRealED, WM_GETFONT, 0, 0);
	if ( hEditFont == NULL ){
		hEditFont = (HFONT)SendMessage(GetParent(hRealED), WM_GETFONT, 0, 0);
	};
	GetHeight(PES, hEditFont);

	PES->tab = 8 * 4;
	PES->CrCode = VTYPE_CRLF;
	PES->CharCP = 0; // InitEditCharCode() で初期化される

	PES->style = GetWindowLong(PES->hWnd, GWL_STYLE);
	PES->exstyle = GetWindowLong(PES->hWnd, GWL_EXSTYLE);
	PES->list.hWnd = NULL;
	PES->list.hSubWnd = NULL;

	if ( wHist != 0 ) setflag(PES->flags, PPXEDIT_SUGGEST);

	if ( X_uxt[2] > 0 ) InitRichMode(PES, hRealED, hEditFont);

	GetCustData(T("X_pmc"), &X_pmc, sizeof(X_pmc));
	InitExEdit(PES);

	if ( PES->hOldED == NULL ){
		PES->hOldED = (WNDPROC)
				SetWindowLongPtr(hRealED, GWLP_WNDPROC, (LONG_PTR)EDsHell);
/*
		if ( PES->hOldED == NULL ){
			XMessage(hEditWnd, NULL, XM_GrERRld, T("PPxRegistExEdit error"));
		}
*/
		if ( X_csyh || (PES->flags & PPXEDIT_SYNTAXCOLOR) ){
			InitSynColor(PES);
			RichSyntaxColor(PES);
		}

		if ( !(PES->flags & PPXEDIT_TEXTEDIT) &&
			 (X_flst_mode == X_fmode_api) ){
			// SHAutoComplete はカレントディレクトリで相対パスを補完する
			RegisterSHAutoComplete(hRealED);
		}
		if ( flags & PPXEDIT_WANTEVENT ){
			if ( IsExistCustTable(StrK_edit, StrFIRSTEVENT) ){
				PostMessage(hRealED, WM_PPXCOMMAND, K_E_FIRST, 0);
			}
		}
	}
	return hRealED;
}
