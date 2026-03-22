/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library - 拡張エディット / ヒストリ・補完リスト
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "PPD_DEF.H"
#include "PPD_EDL.H"
#include "VFS_STRU.H"
#include "CALC.H"
#pragma hdrstop

#define OnAllowTick 100 // 補完列挙を中止する時間
#define ThreadAllowTick 3000 // 補完列挙を中止する時間(スレッド使用時)
#define CompListItems 100 // 一覧表示する最大数

struct {
	DWORD showtime, stoptime;
} X_flto = { OnAllowTick, 0};

/*
	コンボボックスモード
						  入力時   表示後入力
		1:ヒストリ一覧		 -          -
		2:ファイル補完一覧	 -		頭文字検索
		3:ヒストリ補完一覧  一覧    一覧再作成
*/

TCHAR StrBTERR[] = T("BackupText err");
const TCHAR StrTimeOut[] = T("*** timeout ***");

#define ABS(n) (n >= 0 ? n : -n)

typedef struct {
	HWND hWnd;
	WNDPROC OrgProc;
	UINT msg;
	DWORD wParam; // ThreadID が格納される
	int items;
	int show;
} LISTADDINFO;

FILLTEXTFILE filltext_cmd = { T("PPXUCMD.TXT"), NULL, NULL, FALSE, 0/*, {0, 0}*/ };
FILLTEXTFILE filltext_path = { T("PPXUPATH.TXT"), NULL, NULL, FALSE, 0/*, {0, 0}*/ };
FILLTEXTFILE filltext_mask = { T("PPXUMASK.TXT"), NULL, NULL, FALSE, 0/*, {0, 0}*/ };

typedef struct {
	PPXAPPINFO info;
	LISTADDINFO *list;
	PPxEDSTRUCT *PES;
} SMODULECAPPINFO;

#define SMODE_PATH 0
#define SMODE_WORDPATH 1
#define SMODE_MASK 2

typedef struct {
	union {
		DWORD_PTR handle;
		FN_REGEXP *regexp;
	} x;
	volatile DWORD *ActiveListThreadID;
	TCHAR *first, *second;
	size_t firstlen, secondlen;
	DWORD startTick;
	DWORD edmode;
	DWORD ThreadID;
	int braket; // BRAKET_ に 1を足した値
} SEARCHPARAMS;
#define XH_BADHANDLE 1

void FreeFillTextFile(FILLTEXTFILE *filltext)
{
	if ( filltext->mem != NULL ){
		filltext->loading = TRUE;
		if ( filltext->ref > 0 ){
			int count = (500 / 50) + (filltext->ref * (200 / 50) );
			for (;;){
				Sleep(50);
				if ( filltext->ref <= 0 ) break;
				if ( --count <= 0 ) break;
			}
		}
		HeapFree(ProcHeap, 0, filltext->mem);
		filltext->mem = NULL;
		filltext->loading = FALSE;
		filltext->ref = 0;
	}
}

void CleanUpEdit(void)
{
	FreeFillTextFile(&filltext_cmd);
	FreeFillTextFile(&filltext_path);
	FreeFillTextFile(&filltext_mask);
}

void FreeBackupText(PPxEDSTRUCT *PES)
{
	if ( PES->list.OldString != NULL ){
		HeapFree(DLLheap, 0, PES->list.OldString);
		PES->list.OldString = NULL;
	}
}

void BackupText(HWND hWnd, PPxEDSTRUCT *PES)
{
	int len;

	FreeBackupText(PES);
	len = GetWindowTextLength(hWnd) + 1;
	PES->list.OldString = HeapAlloc(DLLheap, 0, TSTROFF(len));
	if ( PES->list.OldString == NULL ){
		PES->list.OldString = StrBTERR;
		XMessage(hWnd, NULL, XM_FaERRld, StrBTERR);
		return;
	}
	*PES->list.OldString = '\0';
	GetWindowText(hWnd, PES->list.OldString, len);
	SendMessage(hWnd, EM_GETSEL, (WPARAM)&PES->list.OldStart, (LPARAM)&PES->list.OldEnd);
}

static void RestoreBackupText(PPxEDSTRUCT *PES, HWND hEditWnd, HWND hListWnd)
{
	SendMessage(hListWnd, LB_SETCURSEL, (WPARAM)-1, 0);

	if ( PES->list.OldString == NULL ) return;
	SetWindowText(hEditWnd, PES->list.OldString);
	SendMessage(hEditWnd, EM_SETSEL, (WPARAM)PES->list.OldStart, (LPARAM)PES->list.OldEnd);
	if ( PES->flags & PPXEDIT_SYNTAXCOLOR ) RichSyntaxColor(PES);
}

static DWORD CheckSamelen(const TCHAR *str1, const TCHAR *str2)
{
	const TCHAR *s1 = str1, *s2 = str2;
	DWORD len = 0;

	while ( (*s1 != '\0') && (*s1 == *s2) ){
	#ifdef UNICODE
		s1++;
		s2++;
		len++;
	#else
		int clen = Chrlen(*s1);
		if ( (clen > 1) && (*(s1 + 1) != *(s2 + 1) ) ) break;
		s1 += clen;
		s2 += clen;
		len += clen;
	#endif
	}
	return len;
}

static BOOL MakeCompleteList(PPxEDSTRUCT *PES, HWND hListWnd)
{
	ECURSOR cursor;
	TCHAR *ptr;
	TCHAR buf[VFPS + MAX_PATH], buf2[VFPS + MAX_PATH];
	DWORD fmode;
	DWORD len, samelen;

	if ( GetWindowText(PES->hWnd, buf, TSIZEOF(buf)) >= (TSIZEOF(buf) - 1) ){
		return FALSE;
	}
	SendMessage(PES->hWnd, WM_SETREDRAW, FALSE, 0);
												// 編集文字列(全て)を取得
	GetEditSel(PES->hWnd, buf, &cursor);
	fmode = (PES->ED.cmdsearch & (CMDSEARCH_NOADDSEP | CMDSEARCH_MATCHFLAGS)) |
			((PES->list.WhistID & PPXH_COMMAND) ?
					CMDSEARCH_CURRENT : CMDSEARCH_OFF) |
			((PES->flags & PPXEDIT_SINGLEREF) ? 0 : CMDSEARCH_MULTI) |
			CMDSEARCH_EDITBOX | CMDSEARCH_ONE;

	if ( (PES->list.WhistID & (PPXH_DIR | PPXH_PPCPATH)) &&
		 IsTrue(GetCustDword(T("X_fdir"), TRUE)) ){
		setflag(fmode, CMDSEARCH_DIRECTORY);
	}
	// １回目検索
	PES->ED.FnameP = PES->list.OldString; // ここでリスト(backuptext)ができていること!

	ptr = SearchFileIned(&PES->ED, buf, &cursor, fmode | CMDSEARCHI_SAVEWORD);
	if ( ptr == NULL ){
		SetMessageForEdit(PES->hWnd, MES_EENF);
		SendMessage(PES->hWnd, WM_SETREDRAW, TRUE, 0);
		return FALSE;
	}
	// １回目結果を反映
	SendMessage(PES->hWnd, EM_SETSEL, cursor.start, cursor.end);
							// ※↑SearchFileIned 内で加工済み
	SendMessage(PES->hWnd, EM_REPLACESEL, 0, (LPARAM)ptr);
	SendMessage(PES->hWnd, EM_SETSEL, 0, 0);	// 表示開始桁を補正させる

	samelen = len = (DWORD)tstrlen(ptr);
#ifndef UNICODE
	if ( xpbug < 0 ) CaretFixToW(ptr, &len);
#endif
	PES->list.select.start = cursor.start;
	PES->list.select.end = cursor.start + len;

	SendMessage(PES->hWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(PES->hWnd, NULL, FALSE);
	tstrcpy(buf2, ptr);
	tstrcpy(buf, PES->ED.Fname);

	// ブラケット処理済みは、これ以上重複処理させない
	if ( PES->ED.cmdsearch & CMDSEARCHI_FINDBRAKET ){
		resetflag(fmode, CMDSEARCH_MULTI);
	}
	// ２回目検索
	cursor.start = 0;
	cursor.end   = tstrlen32(buf);
	ptr = SearchFileIned(&PES->ED, buf, &cursor, fmode);
	if ( ptr == NULL ){
		PostMessage(hListWnd, WM_CLOSE, 0, 0);
	}else{
		DWORD starttime;

		SendMessage(hListWnd, LB_ADDSTRING, 0, (LPARAM)buf2); // １回目
		SendMessage(hListWnd, LB_ADDSTRING, 0, (LPARAM)ptr); // ２回目
		samelen = CheckSamelen(buf2, ptr);
		starttime = GetTickCount();
		// ３回目以降検索
		for ( ; ; ){
			tstrcpy(buf, PES->ED.Fname);
			cursor.start = 0;
			cursor.end   = tstrlen32(buf);
			ptr = SearchFileIned(&PES->ED, buf, &cursor, fmode);
			if ( ptr == NULL ) break;
			SendMessage(hListWnd, LB_ADDSTRING, 0, (LPARAM)ptr);
			samelen = CheckSamelen(buf2, ptr);
			if ( (GetTickCount() - starttime) > OnAllowTick ){
				SendMessage(hListWnd, LB_ADDSTRING, 0, (LPARAM)T("more..."));
				break;
			}
		}
	}
	if ( PES->flags & CMDSEARCHI_SELECTPART ){
	#ifndef UNICODE
		if ( xpbug < 0 ) CaretFixToW(buf2, &samelen);
	#endif
	}else{
		samelen = len;
	}
	SendMessage(PES->hWnd, EM_SETSEL,
			PES->list.select.start + samelen, PES->list.select.start + len);
	SetMessageForEdit(PES->hWnd, NULL);
	SendMessage(hListWnd, LB_SETCURSEL, 0, 0);
	return TRUE;
}

static BOOL MakeFloatList(PPxEDSTRUCT *PES, EDITDIST dist)
{
	HWND hListWnd;
	int i;
	UINT msg;

	hListWnd = PES->list.hWnd;

	SendMessage(hListWnd, LB_RESETCONTENT, 0, 0);

	BackupText(PES->hWnd, PES);
	if ( PES->list.mode >= LIST_FILL ){
		return MakeCompleteList(PES, hListWnd);
	}

	// LIST_MANUAL
	msg = dist > 0 ? LB_ADDSTRING : LB_INSERTSTRING;
	UsePPx();
										// 編集中の内容をリストに入れる
	if ( dist != 0 ){
		const TCHAR *first;

		first = EnumHistory(PES->list.WhistID, 0);	// ヒストリ１つ目と重複？
		if ( first != NULL ){
			if ( tstricmp(PES->list.OldString, first) ){
				SendMessage(hListWnd, msg, 0, (LPARAM)PES->list.OldString);
			}
		}
	}
										// ヒストリ内容を登録:W:近い内容
	for ( i = 0 ; i < CompListItems ; i++ ){
		const TCHAR *histptrW;

		histptrW = EnumHistory(PES->list.WhistID, i);
		if ( histptrW == NULL ) break;
		#ifdef UNICODE
			if (ALIGNMENT_BITS(histptrW) & 1 ){
				SendUTextMessage_U(hListWnd, msg, 0, histptrW);
			}else
		#endif
				SendMessage(hListWnd, msg, 0, (LPARAM)histptrW);
	}
										// ヒストリ内容を追加登録:R:遠い内容
	for ( ; i < CompListItems ; i++ ){
		const TCHAR *histptrR;

		histptrR = EnumHistory(PES->list.RhistID & (WORD)~PES->list.WhistID, i);
		if ( histptrR == NULL ) break;
		#ifdef UNICODE
			if (ALIGNMENT_BITS(histptrR) & 1 ){
				SendUTextMessage_U(hListWnd, msg, 0, histptrR);
			}else
		#endif
				SendMessage(hListWnd, msg, 0, (LPARAM)histptrR);
	}
	if ( dist < 0 ){
		SendMessage(hListWnd, LB_SETCURSEL, SendMessage(hListWnd, LB_GETCOUNT, 0, 0) - 1, 0);
	}
	if ( dist > 0 ){
		SendMessage(hListWnd, LB_SETCURSEL, 0, 0);
	}
	FreePPx();
	return TRUE;
}

//------------------------------------- 一覧窓本体
static LRESULT CALLBACK TextListWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	PPxEDSTRUCT *PES;
	PES = (PPxEDSTRUCT *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch ( iMsg ){
		case WM_KILLFOCUS:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		case WM_KEYDOWN:
			if ( (wParam != VK_ESCAPE) && (wParam != VK_RETURN) && (wParam != VK_F4) ){
				break;
			}
			// ESC 等なら、WM_LBUTTONUP を処理して、窓を閉じる

		case WM_LBUTTONUP:
			SendMessage(PES->hWnd, WM_COMMAND, TMAKELPARAM(0, LBN_SELCHANGE), (LPARAM)hWnd);
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		// 「マウス ポインタをウィンドウに重ねたときにアクティブ」有効時、
		// アクティブにされるのを防止する
		case WM_MOUSEACTIVATE:
			if ( HIWORD(lParam) == WM_MOUSEMOVE ) return MA_NOACTIVATE;
			break;

		case WM_DESTROY:
			PES->list.hWnd = NULL;
			if ( PES->list.hSubWnd == NULL){
				PES->list.index = 0;
				PES->list.mode = 0;
				PES->list.ListFocus = LISTU_NOLIST;
			}
			break;

		case LB_ADDSTRING:
		case LB_INSERTSTRING:
			if ( wParam == 0 ) break;
			// 送信スレッド有効チェック
			if ( wParam != PES->ActiveListThreadID ) return LB_ERR;
			wParam = 0;
			break;
	}
	return CallWindowProc(PES->list.OrgProc, hWnd, iMsg, wParam, lParam);
}

static LRESULT CALLBACK ListSubWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	PPxEDSTRUCT *PES;
	PES = (PPxEDSTRUCT *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch ( iMsg ){
		case WM_KILLFOCUS:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		case WM_KEYDOWN:
			if ( (wParam != VK_ESCAPE) && (wParam != VK_RETURN) && (wParam != VK_F4) ){
				break;
			}
			// ESC 等なら、WM_LBUTTONUP を処理して、窓を閉じる

		case WM_LBUTTONUP:
			SendMessage(PES->hWnd, WM_COMMAND, TMAKELPARAM(0, LBN_SELCHANGE), (LPARAM)hWnd);
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		// 「マウス ポインタをウィンドウに重ねたときにアクティブ」有効時、
		// アクティブにされるのを防止する
		case WM_MOUSEACTIVATE:
			if ( HIWORD(lParam) == WM_MOUSEMOVE ) return MA_NOACTIVATE;
			break;

		case WM_DESTROY:
			PES->list.hSubWnd = NULL;
			if ( PES->list.hWnd == NULL ){
				PES->list.index = 0;
				PES->list.mode = 0;
				PES->list.ListFocus = LISTU_NOLIST;
			}
			break;

		case LB_ADDSTRING:
		case LB_INSERTSTRING:
			if ( wParam == 0 ) break;
			// 送信スレッド有効チェック
			if ( wParam != PES->ActiveListThreadID ) return LB_ERR;
			wParam = 0;
			break;
	}
	return CallWindowProc(PES->list.OrgSubProc, hWnd, iMsg, wParam, lParam);
}

int GetExtraSize(PPxEDSTRUCT *PES, RECT *box)
{
	RECT parentbox;
	HWND hParentWnd;
	int extray;

	hParentWnd = GetParent(PES->hWnd);
	if ( PES->flags & PPXEDIT_COMBOBOX ) hParentWnd = GetParent(hParentWnd);
	GetWindowRect(hParentWnd, &parentbox);
	extray = box->top - parentbox.top;
	if ( !(PES->flags & PPXEDIT_LINEEDIT) &&
		 (((box->bottom - box->top) * 3) < extray) ){
		return 0;
	}
	return box->top - parentbox.top;
}

static HWND CreateListWindowMain(PPxEDSTRUCT *PES, int direction)
{
	RECT box, deskbox;
	UINT ListHeight;
	HWND hListWnd;
	POINT pos;
	int extray = 0;

	ListHeight = (X_flst_height + 1) * PES->fontY;
	GetWindowRect(PES->hWnd, &box);

	if ( X_flst_uppos ) extray = GetExtraSize(PES, &box);

	GetDesktopRect(PES->hWnd, &deskbox);

	if ( direction > 0 ){ // 1: 下側配置
		if ( PES->flags & PPXEDIT_TEXTEDIT ){
			GetCaretPos(&pos);
			box.bottom = box.top + pos.y + PES->fontY;
		}
		if ( (box.bottom + (int)ListHeight) > deskbox.bottom ){
			ListHeight = deskbox.bottom - box.bottom;
			if ( ListHeight < (PES->fontY * 2) ) ListHeight = (PES->fontY * 2);
		}
	}else{ // 0:editbox -1:combobox 上側配置
		box.bottom = box.top - ListHeight - extray;
		if ( PES->flags & PPXEDIT_TEXTEDIT ){
			GetCaretPos(&pos);
			box.bottom += pos.y;
		}
		if ( box.bottom < deskbox.top ){
			ListHeight = box.top - deskbox.top;
			if ( ListHeight < (PES->fontY * 2) ) ListHeight = (PES->fontY * 2);
			box.bottom = box.top - ListHeight - extray;
		}
		PES->list.upperbottom = box.bottom + ListHeight;
	}
	box.right -= box.left;
	if ( X_flst_width != 0 ){
		int listwidth;

		listwidth = PES->fontY * X_flst_width / 2;
		if ( box.right < listwidth ) box.right = listwidth;
		if ( box.right > (deskbox.right - deskbox.left) ){
			box.right = deskbox.right - deskbox.left;
		}
		if ( (box.left + box.right) > deskbox.right ){
			box.left = deskbox.right - box.right;
		}
		if ( box.left < deskbox.left ){
			box.left = deskbox.left;
		}
	}

	if ( UseCompListModule != 0 ){
		PPXMODULEPARAM pmp;
		PPXMEXTLISTBOX els;

		els.hListWnd = PMELB_EI_HIST;
		els.Caption = NilStr;
		els.hParentWnd = PES->hWnd;
		els.hMenu = NULL;
		els.hInst = DLLhInst;
		els.lParam = NULL;
		els.x = box.left;
		els.y = box.bottom;
		els.width = box.right;
		els.height = ListHeight;
		els.type = 2 - direction;
		els.ExStyle = (WinType >= WINTYPE_2000) ? WS_EX_NOACTIVATE : 0;
		els.Style = WS_BORDER | WS_POPUP | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY;
		els.WhistID = PES->list.WhistID;
		els.RhistID = PES->list.RhistID;
		pmp.listbox = &els;

		if ( CallModule(PES->info, PPXMEVENT_EXLISTBOX, pmp, NULL) == PPXMRESULT_SKIP ){
			UseCompListModule = 0;
			hListWnd = CreateWindowEx(els.ExStyle,
					ListBoxClassName, NilStr,
					WS_BORDER | WS_POPUP | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY,
					box.left, box.bottom, box.right, ListHeight,
					PES->hWnd, NULL, DLLhInst, NULL);
			FixUxTheme(hListWnd, ListBoxClassName);
		}else{
			UseCompListModule = 1;
			hListWnd = els.hListWnd;
		}
	}else{
		hListWnd = CreateWindowEx(
				(WinType >= WINTYPE_2000) ? WS_EX_NOACTIVATE : 0,
				ListBoxClassName, NilStr,
/* ↓窓枠が太くなる分左右位置がずれる＆上下サイズを変えると高さが縮むので中止
			(WinType >= WINTYPE_10) ?
				(WS_THICKFRAME | WS_BORDER | WS_POPUP | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY) :
				(WS_BORDER | WS_POPUP | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY),
*/
				WS_BORDER | WS_POPUP | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY,
				box.left, box.bottom, box.right, ListHeight,
				PES->hWnd, NULL, DLLhInst, NULL);
		FixUxTheme(hListWnd, ListBoxClassName);
	}
	SetWindowLongPtr(hListWnd, GWLP_USERDATA, (LONG_PTR)PES);
	SendMessage(hListWnd, WM_SETFONT, SendMessage(PES->hWnd, WM_GETFONT, 0, 0), 0);
	PES->list.itemheight = (int)SendMessage(hListWnd, LB_GETITEMHEIGHT, 0, 0);

	if ( direction <= 0 ){	// 上側配置のとき、フォントサイズに合うよう高さが
							// 変化して位置ずれ起きることあるので修正
		GetWindowRect(hListWnd, &deskbox);
		ListHeight -= (deskbox.bottom - deskbox.top);
		if ( ListHeight != 0 ){
			SetWindowPos(hListWnd, NULL, box.left, box.bottom + ListHeight,
				0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
		}
/*		次の処理相当
		if ( (box.bottom + ListHeight) != deskbox.bottom ){
			SetWindowPos(hListWnd, NULL, box.left, (box.bottom + ListHeight) - (deskbox.bottom - deskbox.top),
				0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
		}
*/
	}
	return hListWnd;
}

void CreateMainListWindow(PPxEDSTRUCT *PES, int direction)
{
	PES->list.hWnd = CreateListWindowMain(PES, direction);
	PES->list.direction = direction;
	PES->list.ListFocus = LISTU_FOCUSMAIN;
	PES->list.OrgProc = (WNDPROC)SetWindowLongPtr
			(PES->list.hWnd, GWLP_WNDPROC, (LONG_PTR)TextListWndProc);
}

// 補助リスト(常に上に位置する)
void CreateSubListWindow(PPxEDSTRUCT *PES)
{
	PES->list.hSubWnd = CreateListWindowMain(PES, 0);
	PES->list.OrgSubProc = (WNDPROC)SetWindowLongPtr
			(PES->list.hSubWnd, GWLP_WNDPROC, (LONG_PTR)ListSubWndProc);
}

void FloatList(PPxEDSTRUCT *PES, EDITDIST dist)
{
	if ( PES->list.hWnd != NULL ){
		PostMessage(PES->list.hWnd, WM_CLOSE, 0, 0);
		return;
	}
	if ( (dist > EDITDIST_NEXT) || (dist < EDITDIST_BACK) ){ // EDITDIST_NEXT_FILL / EDITDIST_BACK_FILL
		PES->list.mode = LIST_FILL;
	}else{
		PES->list.mode = LIST_MANUAL;
	}
	CreateMainListWindow(PES, dist); // PES->list.hWnd 再生成
	if ( FALSE != MakeFloatList(PES, dist) ){
		ShowWindow(PES->list.hWnd, SW_SHOWNA);
	}else{
		PostMessage(PES->list.hWnd, WM_CLOSE, 0, 0);
	}
}

void FixListPosition(PPxEDSTRUCT *PES)
{
	RECT parentbox, listbox;
	POINT pos;
	int y, extray = 0;

	GetWindowRect(PES->hWnd, &parentbox);
	if ( X_flst_uppos ) extray = GetExtraSize(PES, &parentbox);

	GetWindowRect(PES->list.hWnd, &listbox);
	if ( X_flst_width != 0 ) parentbox.left = listbox.left;
	if ( PES->flags & PPXEDIT_TEXTEDIT ){
		GetCaretPos(&pos); // ※１
		if ( PES->list.direction > 0 ){ // 下配置
			y = parentbox.top + pos.y + PES->fontY;
		}else{ // 上配置
			PES->list.upperbottom = parentbox.top - extray;
			y = PES->list.upperbottom + pos.y - (listbox.bottom - listbox.top);
		}
	}else{
		if ( PES->list.direction > 0 ){ // 下配置
			y = parentbox.bottom;
		}else{ // 上配置
			PES->list.upperbottom = parentbox.top - extray;
			y = PES->list.upperbottom - (listbox.bottom - listbox.top);
		}
	}

	SetWindowPos(PES->list.hWnd, NULL, parentbox.left, y,
			0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

	if ( PES->list.hSubWnd != NULL ){
		GetWindowRect(PES->list.hSubWnd, &listbox);

		PES->list.upperbottom = parentbox.top - extray;
		y = PES->list.upperbottom - (listbox.bottom - listbox.top);
		if ( PES->flags & PPXEDIT_TEXTEDIT ){
			#pragma warning(suppress: 4701) // 同じPPXEDIT_TEXTEDITの条件※１で初期化
			y += pos.y;
		}
		SetWindowPos(PES->list.hSubWnd, NULL, parentbox.left, y,
				0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	}
}

// テキスト中のコマンド名の末尾を検索
static DWORD GetCommandEnd(const TCHAR *text)
{
	const TCHAR *p, *subp;
	DWORD wP;

	p = text;
	while ( (*p == ' ') || (*p == '\t') ) p++; // 行頭空白
	// コマンドをスキップ
	if ( *p == '\"' ){
		p++;
		while ( ((UTCHAR)*p >= ' ') && (*p != '\"') ) p++;
		if ( *p == '\"' ) p++;
	}else{
		while ( (UTCHAR)*p > ' ' ) p++;
	}

	for ( ; ; ){ // コマンド直後のオプションを選択範囲から除く
		subp = p;
		while ( (*subp == ' ') || (*subp == '\t') ) subp++;
		if ( !((*subp == '/') || (*subp == '-')) ) break;
		while ( (UTCHAR)*subp > ' ' ) subp++;
		p = subp;
	}

	if ( (*p == ' ') || (*p == '\t') ) p++; // コマンド区切りを１文字確保

	wP = p - text;
#ifndef UNICODE
	if ( xpbug < 0 ) CaretFixToW(text, &wP);
#endif
	return wP;
}

void ListSelect(PPxEDSTRUCT *PES, HWND hListWnd)
{
	TCHAR buf[0x1000], *p;
	HWND hWnd;
	WPARAM index;

	if ( (hListWnd != PES->list.hWnd) && (hListWnd != PES->list.hSubWnd) ){
		return;
	}
	index = (WPARAM)SendMessage(hListWnd, LB_GETCURSEL, 0, 0);
	if ( SendMessage(hListWnd, LB_GETTEXTLEN, index, 0) >= TSIZEOF(buf) ){
		return;
	}
	buf[0] = '\0';
	SendMessage(hListWnd, LB_GETTEXT, index, (LPARAM)buf);

	if ( (buf[0] == ';') && (buf[1] == '<') ){
		p = tstrstr(buf + 2, T(">;"));
		if ( p != NULL ){
			p += 2;
			if ( *p == ' ' ) p++;
			memmove(buf, p, TSTRSIZE(p));
		}
	}

	// コメント以降を削除
	for ( p = buf; *p != '\0'; p++ ){
		if ( (*p == ' ') || (*p == '\t') ){
			if ( *(p + 1) == ';' ){
				*p = '\0';
				break;
			}
		}
	}

	hWnd = PES->hWnd;
	if ( PES->list.mode < LIST_FILL ){
		SetWindowText(hWnd, buf);
		SendMessage(hWnd, EM_SETSEL, EC_LAST, EC_LAST);
		if ( PES->flags & PPXEDIT_SYNTAXCOLOR ) RichSyntaxColor(PES);
	}else{
		DWORD wP, lP;

		if ( PES->list.mode == LIST_FILLEXT ){
			SendMessage(hWnd, EM_GETSEL, (WPARAM)&wP, (LPARAM)&lP);
			// カーソルが置き換え位置より後なら、全て置きかえにする
			if ( (wP == lP) && (lP > PES->list.select.end) ){
				PES->list.select.end = EC_LAST;
				PES->list.mode = LIST_FILL;
			}else{ // カーソルが置き換え位置より前なら、拡張子を残す
				TCHAR *p;

				p = buf + FindExtSeparator(VFSFindLastEntry(buf));
				if ( *p == '.' ) *p = '\0';
			}
		}

		SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
		SendMessage(hWnd, EM_SETSEL, PES->list.select.start, PES->list.select.end);
		SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)buf);

		if ( (PES->list.WhistID != PPXH_COMMAND) || PES->list.select.start ){
			SendMessage(hWnd, EM_GETSEL, (WPARAM)&wP, (LPARAM)&PES->list.select.end);
			SendMessage(hWnd, EM_SETSEL, PES->list.select.end, PES->list.select.end);
		}else{ // コマンドなら、パラメータの部分を選択状態にする
			PES->list.select.end = EC_LAST;
			SendMessage(hWnd, EM_SETSEL, GetCommandEnd(buf), PES->list.select.end);
		}
		SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
		if ( PES->flags & PPXEDIT_SYNTAXCOLOR ) RichSyntaxColor(PES);
		InvalidateRect(hWnd, NULL, FALSE);
	}
}

ERRORCODE ListPageUpDown(PPxEDSTRUCT *PES, EDITDIST dist, BOOL search)
{
	if ( search && (PES->findrep != NULL) && (PES->findrep->findtext[0] != '\0') ){
		SearchStr(PES, dist);
		return NO_ERROR;
	}

	if ( PES->list.ListFocus != LISTU_NOLIST ){ // リスト表示中→リスト移動
		HWND hListWnd;
		WPARAM sendkeyID;

		hListWnd = (PES->list.ListFocus == LISTU_FOCUSSUB) ?
			PES->list.hSubWnd : PES->list.hWnd;

		sendkeyID = (dist > 0) ? VK_NEXT : VK_PRIOR;
		SendMessage(hListWnd, WM_KEYDOWN, sendkeyID, 0);
		SendMessage(hListWnd, WM_KEYUP, sendkeyID, 0);
		return NO_ERROR;
	}else if ( PES->flags & (PPXEDIT_COMBOBOX | PPXEDIT_TEXTEDIT) ){
		return ERROR_INVALID_FUNCTION;
	}else if ( PES->style & WS_VSCROLL ){
		FloatList(PES, dist);
	}
	return NO_ERROR;
}

BOOL ListUpDown(HWND hWnd, PPxEDSTRUCT *PES, int offset, int repeat)
{
	TCHAR nowline[CMDLINESIZE];

	if ( !(PES->flags & PPXEDIT_NOINCLIST) ){
		if ( PES->list.hWnd == NULL ){
			if ( PES->flags & PPXEDIT_TEXTEDIT ) return FALSE;	// PPe はリストがないときは処理しない
			if ( !(PES->style & WS_VSCROLL) && !IsRichMode(PES) ) return FALSE;	// スクロールバー無は未対応
		}
		// リスト表示時の処理 -------------------------------------------------
		if ( (PES->list.ListFocus == LISTU_FOCUSSUB) || // 第２リスト上？
			 ((PES->list.hWnd == NULL) && (PES->list.hSubWnd != NULL)) ){
			int index, count;

			index = SendMessage(PES->list.hSubWnd, LB_GETCURSEL, 0, 0);
			count = SendMessage(PES->list.hSubWnd, LB_GETCOUNT, 0, 0);
			if ( index < 0 ) index = count;
			index += offset;
			if ( index < 0 ){ // 上にはみ出す
				index = 0;
			}
			if ( index >= count ){
				PES->list.ListFocus = LISTU_FOCUSMAIN; // 第１リスト(編集行)へ
				if ( PES->list.OldString != NULL ){
					PES->list.select.end = PES->list.OldEnd;
					RestoreBackupText(PES, hWnd, PES->list.hSubWnd);
				}
				return TRUE;
			}
			SendMessage(PES->list.hSubWnd, LB_SETCURSEL, index, 0);
			SendMessage(hWnd, WM_COMMAND, TMAKELPARAM(0, LBN_SELCHANGE), (LPARAM)PES->list.hSubWnd);
			return TRUE;
		}
		if ( PES->list.hWnd != NULL ){	// 第１リスト表示中→リスト移動
			int nowindex, index, count;

			nowindex = SendMessage(PES->list.hWnd, LB_GETCURSEL, 0, 0);

			if ( (PES->flags & PPXEDIT_TEXTEDIT) && (SendMessage(PES->list.hWnd, LB_GETCOUNT, 0, 0) <= 0) ){
				if ( (offset > 0) ||
					 (SendMessage(PES->list.hSubWnd, LB_GETCOUNT, 0, 0) <= 0) ){
					CloseLineList(PES);
					return FALSE;
				}
			}

			index = nowindex + offset;
			if ( PES->list.direction < 0 ){ // 第１リスト-上側表示
				count = SendMessage(PES->list.hWnd, LB_GETCOUNT, 0, 0);
				if ( nowindex == -1 ){ // 未選択
					if ( offset < 0 ){ // ↑
						if ( PES->flags & PPXEDIT_COMBOBOX ){
							LRESULT cbindex;

							cbindex = SendMessage(GetParent(PES->hWnd), CB_GETCURSEL, 0, 0);
							if ( (cbindex != 0) && (cbindex != CB_ERR) ){
								return FALSE; // コンボボックスリストの操作中
							}
						}
						index = count - 1; // list.hWnd の最上位行へ

						PES->list.select.start = 0;
						PES->list.select.end = EC_LAST;

					}else{ // コンボボックスの操作へ
						return FALSE;
					}
					BackupText(hWnd, PES);
				}else if ( index == -1 ){ // 最上位行... 止まる
					index = 0;
				}else if ( index >= count ){ // 編集行へ
					if ( PES->list.OldString != NULL ){
						PES->list.select.end = PES->list.OldEnd;
						RestoreBackupText(PES, hWnd, PES->list.hWnd);
					}
					return TRUE;
				}
			}else{ // 第１リスト-下側表示
				if ( nowindex == -1 ) BackupText(hWnd, PES);
				if ( index < 0 ){ // -1(0-1):フォーカス解除 -2(-1-1):第2へ
					if ( (index == -2) && (PES->list.hSubWnd != NULL) ){
						PES->list.ListFocus = LISTU_FOCUSSUB; // 第２リストへ
						count = SendMessage(PES->list.hSubWnd, LB_GETCOUNT, 0, 0);
						SendMessage(PES->list.hSubWnd, LB_SETCURSEL, count - 1, 0);
						SendMessage(hWnd, WM_COMMAND, TMAKELPARAM(0, LBN_SELCHANGE), (LPARAM)PES->list.hSubWnd);
					}else{ // 編集行へ
						if ( PES->list.OldString != NULL ){
							PES->list.select.end = PES->list.OldEnd;
							RestoreBackupText(PES, hWnd, PES->list.hWnd);
						}
					}
					return TRUE;
				}else if ( SendMessage(PES->list.hWnd, LB_GETCOUNT, 0, 0) == 0 ){
					return FALSE;
				}
			}
			SendMessage(PES->list.hWnd, LB_SETCURSEL, index, 0);
			SendMessage(hWnd, WM_COMMAND, TMAKELPARAM(0, LBN_SELCHANGE), (LPARAM)PES->list.hWnd);
			return TRUE;
		}
	}

	// リスト表示なしの時の処理 -----------------------------------------------
	if ( PES->oldkey == 0 ){	// 新規サーチ？
		BackupText(hWnd, PES);
		PES->list.mode = PES->list.startmode =
				(*PES->list.OldString && SendMessage(hWnd, EM_GETMODIFY, 0, 0)) ?
				LIST_SEARCH : LIST_WRITE;
		PES->list.index = 0;
		repeat = 0;	// 初めての時はリピートを無視する
	}
	if ( PES->list.OldString == NULL ) return TRUE;	// バックアップ失敗時は不可
					// 現在の編集内容に戻ったとき、リピート時はサーチさせない
	if ( (PES->list.index == 0) &&
			(PES->list.mode == PES->list.startmode) && repeat ){
		return TRUE;
	}
	PES->list.index += offset;

	nowline[0] = '\0';
	GetWindowText(PES->hWnd, nowline, TSIZEOF(nowline));
	nowline[TSIZEOF(nowline) - 1] = '\0';

	UsePPx();
	for ( ; ; ){
		const TCHAR *hstr;

		if ( PES->list.index == 0 ){	// 一つ前の検索モードに戻る
			if ( PES->list.mode == PES->list.startmode ){
				SetWindowText(hWnd, PES->list.OldString);
				SendMessage(hWnd, EM_SETSEL, EC_LAST, EC_LAST);
				if ( PES->flags & PPXEDIT_SYNTAXCOLOR ) RichSyntaxColor(PES);
				break;
			}
			PES->list.mode--;
			PES->list.index = CountHistory(
					(PES->list.mode != LIST_WRITE) ?
							PES->list.RhistID : PES->list.WhistID) * -offset;
			continue;
		}

		hstr = EnumHistory( // ↓LIST_READ の方がいいかも？
					(PES->list.mode != LIST_WRITE) ?
							PES->list.RhistID : PES->list.WhistID,
					ABS(PES->list.index) - 1);
		if ( hstr == NULL ){
				// 読込みヒストリモード以外なら別のに切り換えて再トライ
			if ( PES->list.mode < LIST_READ ){
				PES->list.mode++;
				PES->list.index = offset;
				continue;
			}
				// 読込みヒストリモードならサーチ終了
			PES->list.index -= offset;
			break;
		}
		if ( PES->list.mode == LIST_SEARCH ){
			size_t len;

			len = tstrlen(PES->list.OldString);
			if ( (len > tstrlen(hstr)) || tstrnicmp(PES->list.OldString, hstr, len) ){
				PES->list.index += offset;
				continue;
			}
		}

		if ( tstrcmp(nowline, hstr) == 0 ){ // 重複を削除
			PES->list.index += offset;
			continue;
		}

		SetWindowText(hWnd, hstr);
		if ( PES->list.WhistID != PPXH_COMMAND ){
			SendMessage(hWnd, EM_SETSEL, EC_LAST, EC_LAST);
		}else{
			SendMessage(hWnd, EM_SETSEL, GetCommandEnd(hstr), EC_LAST);
		}
		if ( PES->flags & PPXEDIT_SYNTAXCOLOR ) RichSyntaxColor(PES);
		break;
	}
	FreePPx();
#if 0
	{
		TCHAR buf[100];

		thprintf(buf, TSIZEOF(buf), "Key:%d mode:%d index:%d",
				PES->oldkey, PES->list.mode, PES->list.index);
		SetWindowText(GetParentCaptionWindow(hWnd), buf);
	}
#endif
	return TRUE;
}

// 補完候補リストのインクリメンタルサーチ(X_flst_mode < X_fmode_auto1 のとき)
void ListSearch(HWND hWnd, PPxEDSTRUCT *PES, int oldlen)
{
	int index, limit, offset, nowlen;
	TCHAR text[0x1000];

	if ( PES->list.ListFocus == LISTU_NOLIST ) return;

	nowlen = GetWindowTextLength(hWnd);
	if ( (oldlen == nowlen) || (nowlen >= (TSIZEOF(text) - 1)) ) return;

	if ( PES->list.mode >= LIST_FILL ){
		PostMessage(PES->list.hWnd, WM_CLOSE, 0, 0);
		return;
	}
	if ( GetWindowText(hWnd, text, TSIZEOF(text)) == 0 ) return;

	limit = SendMessage(PES->list.hWnd, LB_GETCOUNT, 0, 0) - 1;
	if ( PES->list.direction > 0 ){
		index = 0;
		offset = 1;
	}else{
		index = limit;
		offset = -1;
	}
	for ( ; (index >= 0) && (index <= limit) ; index += offset ){
		int itemlen;
		TCHAR itemtext[0x800];

		itemlen = SendMessage(PES->list.hWnd, LB_GETTEXTLEN, index, 0);
		if ( (itemlen > nowlen) || (itemlen >= (TSIZEOF(itemtext) - 1))  ){
			continue; // 検索側が長いので無視できる
		}
		if ( SendMessage(PES->list.hWnd, LB_GETTEXT, index, (LPARAM)itemtext)
					== LB_ERR ){
			return;
		}
		#pragma warning(suppress: 6001 6054) // LB_GETTEXT で取得済み
		if ( tstrnicmp(text, itemtext, nowlen) == 0 ){
								// [↓]で選択できるように、一つ手前に移動する
			SendMessage(PES->list.hWnd, LB_SETTOPINDEX, index, 0);
			SendMessage(PES->list.hWnd, LB_SETCURSEL, index - 1, 0);
			break;
		}
	}
	return;
}

// 検索対象の長さを決定
#if 1 // 深いことはしない用
#define SMODEPARAM(smode)
static const TCHAR *CalcTargetWordWidth(const TCHAR *text, LPCTSTR *last)
{
	TCHAR code;
	const TCHAR *first;

	while ( (*text == ' ') || (*text == '\t') ) text++;
	first = text;

	while ( (code = *text) != '\0' ){
		if ( (code == '\r') || (code == '\n') ) break;
		text++;
	}
	*last = text;
	return first;
}
#else
#define SMODEPARAM(smode), smode
static const TCHAR *CalcTargetWordWidth(const TCHAR *text, LPCTSTR *last, int mode)
{
	TCHAR code;
	const TCHAR *first;
	int usebraket = 0;

	while ( (*text == ' ') || (*text == '\t') ) text++;
	first = text;
	if ( mode == SMODE_MASK ){ // マスク文字列用
		while ( (code = *text) != '\0' ){
			if ( (code == '\r') || (code == '\n') ) break;
			#if 0 // コメントを検索対象から外す時用
			if ( (code == ';') && ((text > first) && ((*(text - 1) == ' ') || (*(text - 1) == '\t')) ) ){
				break;
			}
			#endif
			text++;
		}
	}else while ( (code = *text) != '\0' ){ // コマンド・パス
		if ( (code == '\r') || (code == '\n') ) break;
		#if 0 // コメントを検索対象から外す時用
		if ( code == ';' ) break; // コメント
		#endif
		#if 0 // パス形式の時、最終エントリ以降に限定するとき
		if ( (code == '\\') && ((UTCHAR)*(text + 1) > ' ') ){
			first = text + 1;
		}else
		#endif
		#if 0 // 空白区切り以降を検索対象から外す
		if ( mode != SMODE_PATH ){
			// くくりのない空白区切り
			if ( ((code == ' ') || (code == '\t')) && !usebraket ) break;
			if ( code == '\"' ){
				if ( usebraket ) break; // くくりの終わり
					// くくりの始まり
				usebraket = 1;
				first = text;
			}
		}
		#endif
		#ifdef UNICODE
			text++;
		#else
			text += Chrlen(*text);
		#endif
	}
	*last = text;
	return first;
}
#endif

static TCHAR *tmemimem(const TCHAR *source, const TCHAR *sourcelast, const TCHAR *word, int wordlen)
{
	const TCHAR *src, *dest, *destlast;

	if ( wordlen == 0 ) return (TCHAR *)source;
	sourcelast -= (wordlen - 1);
	destlast = word + wordlen;
	while ( source <= sourcelast ){
		src = source;
		dest = word;
		for ( ; ; ){
			TCHAR s, d;

			s = *src++;
			if ( Isupper(s) ) s += (TCHAR)0x20;
			d = *dest++;
			if ( Isupper(d) ) d += (TCHAR)0x20;
			if ( s != d ) break;
			if ( dest >= destlast ) return (TCHAR *)source;
		}
		source++;
	}
	return NULL;
}

#if 0
#ifdef UNICODE
static LRESULT USEFASTCALL AddListItem_FixSub(LISTADDINFO *list, const TCHAR *text)
{
	TCHAR buf[0x1000];

	tstplimcpy(buf, text, 0x1000);
	return CallWindowProc(list->hWnd, list->OrgProc, list->msg, list->wParam, (LPARAM)bug);
}
#endif
#endif

static LRESULT USEFASTCALL AddListItem_Fix(LISTADDINFO *list, const TCHAR *text)
{
#if 1
	#ifdef UNICODE
		if ( ALIGNMENT_BITS(text) & 1 ){
			return SendUTextMessage_U(list->hWnd, list->msg, list->wParam, text);
		}else
	#endif
			return SendMessage(list->hWnd, list->msg, list->wParam, (LPARAM)text);
#else
	if ( list->OrgProc != NULL ){
	#ifdef UNICODE
		if ( ALIGNMENT_BITS(text) & 1 ){
			return AddListItem_FixSub(list, text);
		}else
	#endif
//			return CallWindowProc(list->OrgProc, list->hWnd, list->msg, list->wParam, (LPARAM)text);
	}else{
	#ifdef UNICODE
		if ( ALIGNMENT_BITS(text) & 1 ){
			return SendUTextMessage_U(list->hWnd, list->msg, list->wParam, text);
		}else
	#endif
			return SendMessage(list->hWnd, list->msg, list->wParam, (LPARAM)text);
	}
#endif
}

static LRESULT USEFASTCALL AddListItem(LISTADDINFO *list, const TCHAR *text)
{
#if 1
	return SendMessage(list->hWnd, list->msg, list->wParam, (LPARAM)text);
#else
	if ( list->OrgProc != NULL ){
		// さすがに自プロージャーの中から呼び出さないと使えないようだ
		return CallWindowProc(list->OrgProc, list->hWnd, list->msg, list->wParam, (LPARAM)text);
	}else{
		return SendMessage(list->hWnd, list->msg, list->wParam, (LPARAM)text);
	}
#endif
}

static LRESULT AddBraketTextToList(LISTADDINFO *list, const TCHAR *text, int braket)
{
	TCHAR buf[CMDLINESIZE], *p;

	if ( braket ){
		p = tstrchr(text, ' ');
		if ( (p != NULL) && (*(p + 1) != ';') ){	// 空白あり→ブラケット必要
			TCHAR *nline;

			nline = buf;
			if ( braket == (GWS_BRAKET_NONE + 1)) *nline++ = '\"'; // ブラケット無し
			tstplimcpy(nline, text, CMDLINESIZE - 4);
			if ( braket != (GWS_BRAKET_LEFTRIGHT + 1) ) tstrcat(nline, T("\"")); // 右ブラケット無し
			text = buf;
		}
	}
	return AddListItem_Fix(list, text);
}

#define tstrcpartcmp(src, static_find_str) memcmp(src, static_find_str, TSIZEOFSTR(static_find_str))

const TCHAR ShellDirRegStr[] = T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FolderDescriptions");
const TCHAR ShellDirNameStr[] = T("Name");

static void ShellDriveSearch(LISTADDINFO *list, const TCHAR *first, int braket)
{
	HKEY hShellDirKey, hSubKey;
	int count = 0;
	TCHAR KeyName[128], buf[VFPS];
	const TCHAR *search;
	DWORD rsize;
	FILETIME write;

	if ( (first[0] != 's') || (first[1] != 'h') || (first[2] != 'e') ){
		return;
	}
	search = NilStr;
	if ( first[3] != '\0' ){
		if ( first[3] != 'l' ) return;
		if ( first[4] != '\0' ){
			if ( first[4] != 'l' ) return;
			search = (first[5] == ':') ? (first + 6) : (first + 5);
		}
	}

	if ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, ShellDirRegStr, 0, KEY_READ, &hShellDirKey)){
		return;
	}
	for ( ; ; count++ ){					// 設定を取り出す ----------------
		rsize = TSIZEOF(KeyName);
		if ( ERROR_SUCCESS != RegEnumKeyEx(hShellDirKey, count, KeyName, &rsize, NULL, NULL, NULL, &write) ){
			break;
		}

		if ( ERROR_SUCCESS != RegOpenKeyEx(hShellDirKey, KeyName, 0, KEY_READ, &hSubKey)){
			continue;
		}

		rsize = TSIZEOF(KeyName);
		if ( ERROR_SUCCESS == RegQueryValueEx(hSubKey, ShellDirNameStr, NULL, NULL, (LPBYTE)KeyName, &rsize) ){
			if ( tstristr(KeyName, search) != NULL ){
				thprintf(buf, TSIZEOF(buf), T("shell:%s"), KeyName);
				if ( LB_ERR == AddBraketTextToList(list, buf, braket) ) break;

				list->items++;
			}
		}
		RegCloseKey(hSubKey);
	}
	RegCloseKey(hShellDirKey);
}

// auxの一覧作成
static void AuxSearch(LISTADDINFO *list, const TCHAR *first)
{
	int count, size;
	const TCHAR *search;
	TCHAR name[CUST_NAME_LENGTH], buf[CMDLINESIZE];

	if ( (first[0] != 'a') || (first[1] != 'u') || (first[2] != 'x')  ){
		return;
	}
	search = (first[3] == ':') ? (first + 4) : (first + 3);

	for ( count = 0 ; ; count++ ){
		size = EnumCustData(count, name, NULL, 0);
		if ( 0 > size ) break;

		if ( ((name[0] != 'M') && (name[0] != 'S')) ||
			 (tstrcpartcmp(name + 1, T("_aux")) != 0) ){
			continue;
		}
		if ( tstrstr(name, search) == NULL ) continue;

		if ( GetCustTable(name, T("base"), buf, sizeof(buf)) == NO_ERROR ){
			tstrreplace(buf, T(" %; "), T(" ; "));
		}else{
			thprintf(buf, TSIZEOF(buf), T("aux:%s"), name);
		}

		if ( LB_ERR == AddListItem(list, buf) ) break;

		list->items++;
	}
}

// 環境変数の一覧作成
static void EnvironmentStringsSearch(LISTADDINFO *list, TCHAR *first, size_t firstlen, int braket, WORD mode)
{
	TCHAR *envptr;
	const TCHAR *p;
	TCHAR name[64], buf[96], *dst, *maxptr;
	int itemcount = 0;
	BOOL pathmode = (mode == PPXH_COMMAND) ? FALSE : TRUE;

	if ( firstlen > 63 ) return;
	if ( firstlen ){
		first++;
		firstlen--;
	}
	if ( (*first == '\'') && firstlen ){
		first++;
		firstlen--;
	}
	memcpy(name, first, firstlen * sizeof(TCHAR) );
	name[firstlen] = '\0';

	p = envptr = GetEnvironmentStrings();
	if ( p == NULL ) return;
	for ( ; *p != '\0' ; p += tstrlen(p) + 1 ){
		if ( *p == '=' ) continue;
		if ( tstristr(p, name) != NULL ){
			dst = buf;
			maxptr = buf + TSIZEOF(buf) - 10;

			*dst++ = '%';
			if ( pathmode == FALSE ) *dst++ = '\'';
			for ( ;; ){ // 名前をコピー
				if ( *p == '=' ){
					p++;
					break;
				}
				*dst++ = *p++;
				if ( dst >= maxptr ) break;
			}
			*dst++ = (TCHAR)(pathmode ? '%' : '\'');

			*dst++ = ' ';
			*dst++ = ';';
			*dst++ = ' ';

			while ( *p != '\0' ){ // 内容をコピー
				*dst++ = *p++;
				if ( dst >= maxptr ) break;
			}
			*dst = '\0';

			if ( LB_ERR == AddBraketTextToList(list, buf, braket) ) break;
			itemcount++;
		}
	}
	list->items += itemcount;
	FreeEnvironmentStrings(envptr);
}

static void FreeExtraSearch(SEARCHPARAMS *spms)
{
	if ( spms->x.handle > XH_BADHANDLE ){
		if ( spms->edmode & (CMDSEARCH_WILDCARD | CMDSEARCH_REGEXP) ){
			FreeFN_REGEXP(spms->x.regexp);
			HeapFree(DLLheap, 0, spms->x.regexp);
		}else if ( spms->edmode & CMDSEARCH_ROMA ){
			SearchRomaString(NULL, NULL, 0, &spms->x.handle);
		}
	}
	spms->x.handle = 0;
}

const TCHAR WordSearchOption_REGEXP[] = T("o:xel,/"); // wildcard + regexp
const TCHAR WordSearchOption_AND[] = T("o:xel,&*"); // wildcard 用
const TCHAR WordSearchOption_ROMA[] = T("o:xel,&R:"); // wildcard + roma
const TCHAR WordSearchDummy[] = T("!*"); // 常に失敗用

static void MakeWordSearch(SEARCHPARAMS *spms, const TCHAR *searchstr)
{
	TCHAR text[0x1000];
	FN_REGEXP *fr;

	fr = spms->x.regexp;

	if ( tstrlen(searchstr) >= MAX_PATH ){
		MakeFN_REGEXP(fr, WordSearchDummy); // 常に失敗の内容にする
		return;
	}

	if ( spms->edmode & CMDSEARCH_REGEXP ){
		tstrcpy(text, WordSearchOption_REGEXP);
		if ( *searchstr == '/' ) searchstr++;
		tstrcpy(text + TSIZEOFSTR(WordSearchOption_REGEXP), searchstr);
	}else if ( spms->edmode & CMDSEARCH_ROMA ){
		tstrcpy(text, WordSearchOption_ROMA);
		tstrcpy(text + TSIZEOFSTR(WordSearchOption_ROMA), searchstr);
		tstrreplace(text + TSIZEOFSTR(WordSearchOption_ROMA), T("."), T(",&R:"));
	}else{
/*
		 // wildcard だけの時、and 指定が無いときは失敗に
		if ( (tstrlen(searchstr) >= MAX_PATH) ||
			 ((tstrchr(searchstr, '.') == NULL) && (spms->startTick != 3)) ){
			MakeFN_REGEXP(fr, WordSearchDummy); // 常に失敗の内容にする
			return;
		}
*/
		tstrcpy(text, WordSearchOption_AND);
		tstrcpy(text + TSIZEOFSTR(WordSearchOption_AND), searchstr);
		tstrreplace(text + TSIZEOFSTR(WordSearchOption_AND), T("."), T("*,&*"));
		tstrcat(text + TSIZEOFSTR(WordSearchOption_AND), T("*"));
	}
	MakeFN_REGEXP(fr, text);
}

static BOOL ExtraSearchString(const TCHAR *text, TCHAR *textlast, const TCHAR *searchstr, SEARCHPARAMS *spms)
{
	if ( ! (spms->edmode & (CMDSEARCH_ROMA | CMDSEARCH_WILDCARD | CMDSEARCH_REGEXP) ) ) return FALSE;
	if ( spms->x.handle == XH_BADHANDLE ) return FALSE;
	if ( spms->edmode & (CMDSEARCH_WILDCARD | CMDSEARCH_REGEXP) ){
		if ( spms->x.handle == 0 ){
			spms->x.handle = (DWORD_PTR)HeapAlloc(DLLheap, 0, sizeof(FN_REGEXP));
			MakeWordSearch(spms, searchstr);
		}

		if ( textlast != NULL ){
			TCHAR backup;
			BOOL result;

			backup = *textlast;
			*textlast = '\0';

			result = FilenameRegularExpression(text, spms->x.regexp);
			*textlast = backup;
			return result;
		}else{
			return FilenameRegularExpression(text, spms->x.regexp);
		}
	}else{ // CMDSEARCH_ROMA
		if ( textlast != NULL ){
			TCHAR backup;
			BOOL result;

			backup = *textlast;
			*textlast = '\0';

			result = SearchRomaString(text, searchstr, ISEA_FLOAT, &spms->x.handle);
			*textlast = backup;
			return result;
		}else{
			return SearchRomaString(text, searchstr, ISEA_FLOAT, &spms->x.handle);
		}
	}
}

BOOL ExtraEntrySearchString(const TCHAR *text, ESTRUCT *ED)
{
	SEARCHPARAMS spms;
	BOOL result;

	if ( ED->Fword[0] == '\0' ) return TRUE; // 空なので無条件で成功
	if ( ED->romahandle == XH_BADHANDLE ){ // 検索初期化失敗のときは簡易検索
		return (tstristr(text, ED->Fword) != NULL);
	}

	spms.ThreadID = 0;
	spms.edmode = ED->cmdsearch;
	spms.x.handle = ED->romahandle;
	spms.startTick = 3; // 識別用

	result = ExtraSearchString(text, NULL, ED->Fword, &spms);
	ED->romahandle = spms.x.handle;
	if ( spms.x.handle <= XH_BADHANDLE ){ // 検索初期化失敗のときは簡易検索
		spms.x.handle = XH_BADHANDLE;
		return (tstristr(text, ED->Fword) != NULL);
	}
	return result;
}

static BOOL CheckFillTimeout(LISTADDINFO *list, SEARCHPARAMS *spms)
{
	DWORD nowtick = GetTickCount();

	// 表示時間以内
	if ( list->show == 0 ){
		if ( (nowtick - spms->startTick) <= X_flto.showtime ) return FALSE;
		if ( list->items > 0 ){
			SendMessage(list->hWnd, WM_SETREDRAW, TRUE, 0);
			ShowWindow(list->hWnd, SW_SHOWNA);
			SendMessage(list->hWnd, WM_SETREDRAW, FALSE, 0);
			list->show = 60;
		}
	// インターバルに画面更新(表示ちらつき減少＋登録速度をVSYNC以上にするため)
	}else{
		if ( (nowtick - spms->startTick) <= (X_flto.showtime + list->show) ){
			return FALSE;
		}
		SendMessage(list->hWnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(list->hWnd, NULL, FALSE);
		UpdateWindow(list->hWnd);
		SendMessage(list->hWnd, WM_SETREDRAW, FALSE, 0);
		list->show += 60;
	}

	// TimeOut
	if ( (nowtick - spms->startTick) <= X_flto.stoptime ) return FALSE;
	AddListItem(list, StrTimeOut);
	return TRUE;
}

static BOOL TextReplaceSearch(TCHAR *text, TCHAR *buf)
{
	PXREGEXPS *rexps;
	TCHAR destbuf[CMDLINESIZE];
	const TCHAR *rtext;
	BOOL result;

	if ( FALSE == InitRegularExpression(&rexps, buf, SLASH_REQUIRE) ) return FALSE;
	rtext = RegularExpressionReplace(rexps, text, destbuf, CMDLINESIZE);
	if ( rtext == NULL ) rtext = destbuf;

	if ( (tstrcmp(text, rtext) != 0) && (tstrlen(rtext) < CMDLINESIZE) ){
		tstrcpy(buf, rtext);
		result = TRUE;
	}else{
		result = FALSE;
	}
	FreeRegularExpression(rexps);
	return result;
}

// テキストファイルから一覧作成
static void TextSearch(LISTADDINFO *list, FILLTEXTFILE *filltext, SEARCHPARAMS *spms SMODEPARAM(smode), int braket)
{
	TCHAR buf[CMDLINESIZE], buf2[VFPS];
	TCHAR *text;
	const TCHAR *checkfirst;
	TCHAR *first;
	TCHAR *dest;
	int itemcount = 0;
	size_t firstlen;
	int breakcheck = 100;

	if ( filltext->mem == NULL ){
		// マルチスレッドなので、imageptr が更新されても textptr が未設定のことがある→filltextの直接使用は危険
		TCHAR *imageptr = NULL, *textptr;

		if ( filltext->filename == NULL ) return; // ファイル名未設定

		if ( filltext->loading ){
			int count = 10;

			for (;;){
				Sleep(50);
				if ( filltext->loading == FALSE ) break;
				if ( --count == 0 ) break;
			}
		}

		if ( filltext->mem == NULL ){
			filltext->loading = TRUE;
			MakeUserfilename(buf, CUSTNAME, PPxRegPath);
			tstrcpy( FindLastEntryPoint(buf), filltext->filename );
			if ( LoadTextData(buf, &imageptr, &textptr, NULL, 0) != NO_ERROR ){
				BOOL cancel = FALSE;

				*(VFSFullPath(buf2, (TCHAR *)filltext->filename, DLLpath) + 3) = 'F';
				CopyFileExL(buf2, buf, NULL, NULL, &cancel, COPY_FILE_FAIL_IF_EXISTS);
				if ( LoadTextData(buf, &imageptr, &textptr, NULL, 0) != NO_ERROR ){
					filltext->loading = FALSE;
					return;
				}
			}
			// resetflag(PES->list.flags, LISTFLAG_STATE_LOADING);
			// ●textptr == -1 ... LoadTextDataで異常？(1.82)
			if ( (filltext->mem == NULL) && (textptr != (TCHAR *)-1) ){
				filltext->text = textptr;
				filltext->mem = imageptr;
			}else{ // ●別のスレッドに先を越された（クリティカルセッションを使う方向にした方がよい。）
				HeapFree(ProcHeap, 0, imageptr);
			}
			filltext->loading = FALSE;
		}
	}
	filltext->ref++;

	first = spms->first;
	firstlen = spms->firstlen;
	while ( (*first == '\\') && firstlen ){
		first++;
		firstlen--;
	}

	text = filltext->text;
	if ( text != NULL ) while ( *text != '\0' ){
		if ( (*text != ' ') && (*text != '\t') ){ // 行頭からのみ検索
			TCHAR *checklast;

			if ( (text[0] == ';') && (text[1] == '!') && (text[2] == '/') ){
				dest = buf;
				text += 2;
				for (;;){
					UTCHAR code;

					code = IsEOL((const TCHAR **)&text);
					if ( code == '\0' ) break; // EOL
					if ( dest < (buf + CMDLINESIZE - 1) ) *dest++ = code;
					text++;
				}
				*dest = '\0';
				if ( TextReplaceSearch(first, buf) ){
					if ( LB_ERR == AddBraketTextToList(list, buf, braket) ){
						goto enumbreak;
					}
					itemcount++;
					goto enumnext;
				}
			}

			checkfirst = CalcTargetWordWidth(text, (const TCHAR **) &checklast SMODEPARAM(smode));
/*
			{
				char ab[2000];
				memcpy(ab, checkfirst, checklast - checkfirst);
				ab[checklast - checkfirst] = '\0';
				XMessage(NULL, NULL, XM_DbgLOG, T("%s"), ab);


			}
*/
			if ( (tmemimem(checkfirst, checklast, first, firstlen) != NULL) ||
				 ExtraSearchString(checkfirst, checklast, first, spms) ){
				if ( spms->second == NULL ){ // １つめのみは前方一致
					// 一致したので一行取り出す
					dest = buf;
					for (;;){
						UTCHAR code;

						code = IsEOL((const TCHAR **)&text);
						if ( code == '\0' ) break; // EOL
						if ( dest < (buf + CMDLINESIZE - 1) ) *dest++ = code;
						text++;
					}
					*dest = '\0';

					if ( first == NilStrNC ){
						TCHAR *p = tstrrchr(buf, ';');

						if ( (p != NULL) && (p > buf) &&
							 ((UTCHAR)*(p - 1) <= ' ') ){
							*p = '\0';
						}
					}

					if ( LB_ERR == AddBraketTextToList(list, buf, braket) ){
						goto enumbreak;
					}
					itemcount++;
				// ２つめなら、完全単語一致
				}else{
					if ( (UTCHAR)*(text + firstlen) <= ' ' ){
					 	text += firstlen;
						if ( (*text == ' ') || (*text == '\t') ){
							text++;
							while ( (*text == ' ') || (*text == '\t') ) text++;
							if ( *text == ';' ){ // コメント表示
								text++;
								dest = buf;
								for (;;){
									UTCHAR code;

									code = IsEOL((const TCHAR **)&text);
									if ( code == '\0' ) break; // EOL
									if ( dest < (buf + CMDLINESIZE - 1) ) *dest++ = code;
									text++;
								}
								*dest = '\0';
								SetMessageForEdit(list->hWnd, buf);
							}
						}

						for ( ; ; ){
							// 次の行へ
							while ( IsEOL((const TCHAR **)&text) ) text++;
							if ( *text != '\0' ) text++;

							if ( (*text != ' ') && (*text != '\t') ) break;
							while ( (*text == ' ') || (*text == '\t') ) text++;

							if ( !tstrnicmp(spms->second, text, spms->secondlen) ){ // 前方一致したか
								// 一致したので取り出す
								dest = buf;
								for (;;){
									UTCHAR code;

									code = IsEOL((const TCHAR **)&text);
									if ( code == '\0' ) break; // EOL
									if ( dest < (buf + CMDLINESIZE - 1) ) *dest++ = code;
									text++;
								}
								*dest = '\0';

								if ( LB_ERR == AddBraketTextToList(list, buf, braket) ){
									goto enumbreak;
								}
								itemcount++;
							}
						}
					}
				}
			}else{
				text = checklast; // ここまでチェックしたのでスキップ
			}
		}
enumnext:
		// 次の行へ
		while ( IsEOL((const TCHAR **)&text) ) text++;
		if ( *text != '\0' ) text++;

		if ( --breakcheck == 0 ){
			breakcheck = 100;

			if ( filltext->loading ) break;
			if ( (spms->ThreadID != 0) &&
				 (spms->ThreadID != *spms->ActiveListThreadID) ){
				break;
			}
			if ( IsTrue(CheckFillTimeout(list, spms)) ) break;
		}
	}
enumbreak:
	list->items += itemcount;
	filltext->ref--;
	return;
}

// オプションの検索を行う
static void TextSearch2nd(LISTADDINFO *list, FILLTEXTFILE *filltext, SEARCHPARAMS *spms, TCHAR *firstWordPtr, size_t firstWordLen)
{
	SEARCHPARAMS spms2;

	spms2.x.handle = 0;
	spms2.ActiveListThreadID = spms->ActiveListThreadID;
	spms2.ThreadID = spms->ThreadID;
	spms2.first = firstWordPtr;
	spms2.firstlen = firstWordLen;
	spms2.second = spms->first;
	spms2.secondlen = spms->firstlen;
	spms2.startTick = spms->startTick;
	spms2.edmode = spms->edmode;
	TextSearch(list, filltext, &spms2 SMODEPARAM(SMODE_WORDPATH), 0);
	if ( spms2.x.handle != 0 ) FreeExtraSearch(&spms2);
}

// ディレクトリから一覧作成 ※ spms の first 内の文字列の firstlen 以降が破壊される
static void EntrySearch(PPxEDSTRUCT *PES, LISTADDINFO *list, SEARCHPARAMS *spms)
{
	int itemcount = 0;
	TCHAR *ptr, keyword[CMDLINESIZE];
	ESTRUCT ED;
	DWORD mode, useparent;

	ED.hF = NULL;
	ED.info= PES->info;
	ED.cmdsearch = 0;

	tstplimcpy(keyword, spms->first, TSIZEOF(keyword));

	if ( FindPathSeparator(keyword) == NULL ){ // パス区切りがなければ、親のを利用可能
		useparent = 1;
		ED.romahandle = spms->x.handle; // 親のを利用
	}else{ // 親の検索文字列はパス区切りが含まれるので、自前で用意
		useparent = 0;
		ED.romahandle = 0;
	}
	mode = spms->edmode | CMDSEARCH_NOADDSEP;
	for ( ; ; ){
		ptr = SearchFileInedMain(&ED, keyword, mode);
		if ( ptr == NULL ) break;

		if ( LB_ERR == AddBraketTextToList(list, ptr, spms->braket) ) break;
		itemcount++;

		if ( (spms->ThreadID != 0) &&
			 (spms->ThreadID != *spms->ActiveListThreadID) ){
			break;
		}
		if ( IsTrue(CheckFillTimeout(list, spms)) ) break;
		memmove(keyword, ptr, TSTRSIZE(ptr));
	}
	list->items += itemcount;
	if ( useparent ){
		spms->x.handle = ED.romahandle;
		ED.romahandle = 0; // 親で解放させる
	}
	SearchFileIned(&ED, NilStrNC, NULL, 0);
	return;
}

// ヒストリから一覧作成
static void HistorySearch(LISTADDINFO *list, SEARCHPARAMS *spms, WORD targethist SMODEPARAM(smode), int braket)
{
	int itemcount = 0, index;
	TCHAR textbuf[CMDLINESIZE];
	size_t histlen;

	for ( index = 0 ; ; index++ ){
		const TCHAR *hist;
		const TCHAR *checkfirst;
		TCHAR *checklast;

		UsePPx();
		hist = EnumHistory(targethist, index);
		if ( hist == NULL ){
			FreePPx();
			break;
		}

		if ( IsTrue(CheckFillTimeout(list, spms)) ) {
			FreePPx();
			break;
		}

		histlen = tstrlen(hist);
		if ( ! (spms->edmode & (CMDSEARCH_ROMA | CMDSEARCH_WILDCARD | CMDSEARCH_REGEXP) ) &&
			 (spms->firstlen > histlen) ){
			FreePPx();
			continue;
		}
		if ( histlen >= CMDLINESIZE ){
			FreePPx();
			continue;
		}
		tstrcpy(textbuf, hist);
		hist = textbuf;
		FreePPx();

		checkfirst = CalcTargetWordWidth(hist, (const TCHAR **)&checklast SMODEPARAM(smode));
		if ( tmemimem(checkfirst, checklast, spms->first, spms->firstlen) == NULL ){
			if ( ExtraSearchString(checkfirst, checklast, spms->first, spms) ){
			}else if ( (checkfirst == hist) || tstrnicmp(hist, spms->first, spms->firstlen) ){ // 前方一致
				continue;
			}
		}
		if ( LB_ERR == AddBraketTextToList(list, hist, braket) ) break;
		itemcount++;
	}
	list->items += itemcount;
	return;
}


static DWORD_PTR USECDECL SearchCReportModuleFunction(SMODULECAPPINFO *sinfo, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	if ( (cmdID == PPXCMDID_REPORTSEARCH) ||
		 (cmdID == PPXCMDID_REPORTSEARCH_FILE) ||
		 (cmdID == PPXCMDID_REPORTSEARCH_DIRECTORY) ){

		if ( LB_ERR == AddListItem(sinfo->list, uptr->str) ){
			return PPXARESULT(ERROR_PATH_NOT_FOUND);
		}
		sinfo->list->items++;
		return PPXA_NO_ERROR;
	}
#if 0
	if ( (sinfo->PES->info == NULL) || (sinfo->PES->info->Function) == NULL) ){
		return PPXARESULT(ERROR_PATH_NOT_FOUND);
	}
	return sinfo->PES->info->Function(sinfo->PES->info, cmdID, uptr);
#else // 別スレッドで実行中に親が廃棄されたときの対策が終わるまで
	return DummyPPxInfoFunc(&sinfo->info, cmdID, uptr);
#endif
}

// 検索モジュールを使って一覧作成
static void ModuleSearch(PPxEDSTRUCT *PES, LISTADDINFO *list, TCHAR *first, size_t firstlen)
{
	TCHAR msg[VFPS];
	PPXMSEARCHSTRUCT msearch;
	PPXMODULEPARAM pmp;
	SMODULECAPPINFO smca;
#ifndef UNICODE
	WCHAR keywordW[VFPS];
#endif
	if ( PES->list.flags & LISTFLAG_NOMODULE ) return;
	if ( firstlen >= VFPS ) return; // OVER_VFPS_MSG
	memcpy(msg, first, firstlen * sizeof(TCHAR));
	msg[firstlen] = '\0';

#ifndef UNICODE
	AnsiToUnicode(msg, keywordW, VFPS);
	msearch.keyword = keywordW;
#else
	msearch.keyword = msg;
#endif
	msearch.searchtype = PES->list.WhistID | PES->list.RhistID | PPXH_SEARCH_NAMEONLY;
	msearch.maxresults = PPXMSEARCH_SHORTRESULT;
	smca.info.Function = (PPXAPPINFOFUNCTION)SearchCReportModuleFunction;
	smca.info.Name = WC_EDIT;
	smca.info.RegID = NilStr;
	smca.info.hWnd = PES->hWnd;
	smca.list = list;
	smca.PES = PES;
	pmp.search = &msearch;
	CallModule(&smca.info, PPXMEVENT_SEARCH, pmp, NULL);
	return;
}

// エイリアスから一覧作成
static void AliasSearch(LISTADDINFO *list, SEARCHPARAMS *spms SMODEPARAM(smode))
{
	int itemcount = 0, index = 0;
	TCHAR keyword[CUST_NAME_LENGTH + 8], param[CMDLINESIZE];
	int offset = 0;

	if ( *spms->first == '%' ){
		keyword[0] = '%';
		keyword[1] = '\'';
		offset = 2;
	}

	while( EnumCustTable(index++, T("A_exec"), keyword + offset, param, sizeof(param)) >= 0){
		const TCHAR *checkfirst, *checklast;

		if ( IsTrue(CheckFillTimeout(list, spms)) ) break;

		if ( spms->firstlen <= tstrlen(keyword) ){
			checkfirst = CalcTargetWordWidth(keyword, &checklast SMODEPARAM(smode));
			if ( tmemimem(checkfirst, checklast, spms->first, spms->firstlen) == NULL ){
				// 前方一致
				if ( (checkfirst == keyword) || tstrnicmp(keyword, spms->first, spms->firstlen) ){
					continue;
				}
			}
		}else{
			continue;
		}
		if ( offset != 0 ) tstrcat(keyword, T("'"));
		thprintf(keyword + tstrlen(keyword), CUST_NAME_LENGTH, T(" ;%s"), param);
		if ( LB_ERR == AddListItem(list, keyword) ){
			break;
		}
		itemcount++;
	}
	list->items += itemcount;
	return;
}

static void AddCalcNumber(LISTADDINFO *list, TCHAR *first, size_t firstlen)
{
	TCHAR text[VFPS], result[VFPS + 16];
	int num;
	const TCHAR *ptr;

	if ( firstlen >= VFPS ) return; // OVER_VFPS_MSG
	memcpy(text, first, firstlen * sizeof(TCHAR));
	text[firstlen] = '\0';
	ptr = text;
	if ( CalcString(&ptr, &num) != CALC_NOERROR ) return;
	ptr = text;
	while ( *ptr ){
		TCHAR c;

		c = *ptr++;
		if ( (c == ' ') || Isdigit(c) ) continue;
		break;
	}
	thprintf(result, TSIZEOF(result), T("%d ;=%s"), num, text);
	AddListItem(list, result);
	list->items++;
	thprintf(result, TSIZEOF(result), T("%x ;(16)=%s"), num, text);
	AddListItem(list, result);
	list->items++;
}

static TCHAR *FindCommandParamPoint(TCHAR *line, DWORD cursorPos, size_t *secondPos)
{
	DWORD firstWordPos;
	TCHAR *tempP;
	TCHAR *p;

	if ( cursorPos == 0 ) return NULL;

	// カーソルが１番目の単語かを確認
	firstWordPos = 0;

	while ( line[firstWordPos] == ' ' ) firstWordPos++; // 単語前の空白をスキップ

	if ( firstWordPos >= cursorPos ) return NULL;
	// カーソルがコマンド区切りの直後なら最初の単語扱い
	tempP = line + cursorPos; // startP;

	while ( tempP > line ){ // カーソル直前が区切り文字かどうかを判定
		TCHAR c;

		c = *(tempP - 1);

		if ( c == ' ' ){
			tempP--;
			continue;
		}
		if ( (c == '\n') || (c == ':') || (c == '|') || (c == '&') ){
			return NULL;
		}
		break;
	}

	p = line + firstWordPos;
	while ( (UTCHAR)*p > ' ' ) p++;
	*secondPos = p - line - firstWordPos;

	return line + firstWordPos;
}

static void SettingValueSearch(LISTADDINFO *list, SEARCHPARAMS *spms, BOOL add, TCHAR *cmdmore)
{
	int index = 0;
	TCHAR buf[CMDLINESIZE], keyword[CMDLINESIZE], value[CMDLINESIZE];

	if ( cmdmore == NULL ) return;
	cmdmore++;

	while( EnumCustTable(index++, cmdmore, keyword, value, sizeof(value)) >= 0 ){
		if ( value[0] == '\0' ) continue;
		thprintf(buf, TSIZEOF(buf), T(";<%s>; %s"), keyword, value);
		if ( add ||
			(tstristr(buf, spms->first) != NULL) ||
			 ExtraSearchString(buf, NULL, spms->first, spms) ){
			AddListItem(list, buf);
			list->items++;
		}
	}
}

static void StringValueSearch(LISTADDINFO *list, PPxEDSTRUCT *PES, SEARCHPARAMS *spms, int type, BOOL add, TCHAR *cmdmore)
{
	TCHAR buf[CMDLINESIZE], *value, *ptr, *next;

	if ( cmdmore == NULL ) return;
	ptr = value = ThGetLongString(&PES->LocalStringValue, cmdmore + 1, buf, TSIZEOF(buf));
	if ( value == NULL ) return;

	for(;;){
		if ( type ){
			next = tstrchr(ptr, '\r');
			if ( next != NULL ){
				*next++ = '\0';
				if ( *next == '\n' ) next++;
			}
		}else{
			next = NULL;
		}

		if ( ptr[0] != '\0' ){
			if ( add ||
				(tstristr(ptr, spms->first) != NULL) ||
				 ExtraSearchString(ptr, NULL, spms->first, spms) ){
				AddListItem(list, ptr);
				list->items++;
			}
		}
		if ( next == NULL ) break;
		ptr = next;
	}
	if ( value != buf ) HeapFree(ProcHeap, 0, value);
}

static void AddMacros(LISTADDINFO *list, PPxEDSTRUCT *PES, SEARCHPARAMS *spms, int type, BOOL add)
{
	TCHAR buf[CMDLINESIZE], buf2[VFPS], *p;
	PPXCMDENUMSTRUCT work;

	switch(type){
		case 0:
			PPxEnumInfoFunc(PES->info, '1', tstpcpy(buf, T(";<%1>; ")), &work);
			break;

		case 1:
			PPxEnumInfoFunc(PES->info, PPXCMDID_STARTENUM, buf2, &work);
			PPxEnumInfoFunc(PES->info, 'C', tstpcpy(buf, T(";<%C>; ")), &work);
			PPxEnumInfoFunc(PES->info, PPXCMDID_ENDENUM, buf2, &work);
			break;

		case 2:
			p = tstpcpy(buf, T(";<%FDC>; "));
			PPxEnumInfoFunc(PES->info, PPXCMDID_STARTENUM, buf, &work);
			PPxEnumInfoFunc(PES->info, 'C', p, &work);
			PPxEnumInfoFunc(PES->info, PPXCMDID_ENDENUM, buf2, &work);
			PPxEnumInfoFunc(PES->info, '1', buf2, &work);
			if ( buf2[0] == '\0' ) return;
			VFSFullPath(NULL, p, buf2);
			break;

		case 3:
			PPxEnumInfoFunc(PES->info, '2', tstpcpy(buf, T(";<%2>; ")), &work);
			break;

		case 4:
			PP_ExtractMacro(NULL, PES->info, NULL, T("%~C"), tstpcpy(buf, T(";<%~C>; ")), XEO_NOEDIT);
			break;

		case 5:
			PP_ExtractMacro(NULL, PES->info, NULL, T("%~FDC"), tstpcpy(buf, T(";<%~FDC>; ")), XEO_NOEDIT);
			break;

		default:
			return;
	}
	if ( buf[0] == '\0' ) return;

	if ( add ||
		(tstristr(buf, spms->first) != NULL) ||
		 ExtraSearchString(buf, NULL, spms->first, spms) ){
		AddListItem(list, buf);
		list->items++;
	}
}

static int ComboListSearch(LISTADDINFO *list, SEARCHPARAMS *spms, int type, TCHAR *ppclist, DWORD *useppclist)
{
	TCHAR *listIDptr, *listPathPtr;
	int item = 0;
	TCHAR buf[VFPS];

	if ( ppclist == NULL ) return 0;

	listIDptr = ppclist;
	for ( ; *listIDptr != '\0';
			listIDptr = listPathPtr + tstrlen(listPathPtr) + 1){
		listPathPtr = listIDptr + tstrlen(listIDptr) + 1;

		if ( *listIDptr == '\t' ) continue;
		setflag(*useppclist, 1 << (listIDptr[2] - 'A'));

		if ( type == 0 ){
			thprintf(buf, TSIZEOF(buf), T(";<%cPPc[%s]>; %s"),
					*listIDptr, listIDptr + 2, listPathPtr);
		}else{
			thprintf(buf, TSIZEOF(buf), T("%s ;%cPPc %s"),
					listIDptr + 1,
					*listIDptr,
					listPathPtr);
		}

		if ( (tstristr(buf, spms->first) != NULL) ||
			 ExtraSearchString(buf, NULL, spms->first, spms) ){
			AddListItem(list, buf);
			item++;
		}
	}
	HeapFree(DLLheap, 0, ppclist);
	return item;
}

static void PPxListSearch(LISTADDINFO *list, SEARCHPARAMS *spms, int type)
{
	TCHAR buf[CMDLINESIZE];
	ShareX *sx;
	int i;
	DWORD useppclist = 0; // 現プロセスで使用しているPPcのID一覧

	// 自己一体化窓列挙
	if ( hProcessComboWnd != NULL ){
		list->items += ComboListSearch(list, spms , type,
			(TCHAR *)SendMessage(hProcessComboWnd, WM_PPXCOMMAND, KCW_ppclist, 0),
			&useppclist);
	}
	// 一体化窓列挙
	for ( i = 0 ; i < X_MaxComboID ; i++ ){
		HWND hComboWnd;
		LRESULT lr;
		TCHAR buf1[VFPS + 16];
		TCHAR *ppclist;
		DWORD size;
		ERRORCODE ec;

		hComboWnd = Sm->ppc.hComboWnd[i];
		if ( (hComboWnd == NULL) || (hComboWnd == BADHWND) || (hComboWnd == hProcessComboWnd) ){
			continue;
		}

		lr = SendMessage(hComboWnd, WM_PPXCOMMAND, TMAKEWPARAM(KCW_ppclist, 1), 0);
		if ( lr == 0 ) continue;

		thprintf(buf1, TSIZEOF(buf1), T("%%temp%%\\ppxl%d"), (int)lr);
		ExpandEnvironmentStrings(buf1, buf, TSIZEOF(buf));

		ppclist = NULL;
		ec = LoadFileImage(buf, 4, (char **)&ppclist, &size, LFI_ALWAYSLIMITLESS);
		DeleteFile(buf);
		if ( ec == NO_ERROR ){
			list->items += ComboListSearch(list, spms , type,
					ppclist, &useppclist);
		}
	}

	// 独立窓列挙
	UsePPx();
	sx = Sm->P;
	for ( i = 0 ; i < X_Mtask ; i++, sx++ ){
		TCHAR *name, ID;

		ID = sx->ID[0];
		if ( ID == '\0' ) continue;
		if ( (ID == 'C') && (useppclist & (1 << (sx->ID[2] - 'A'))) ){
			continue;
		}

		if ( type == 0 ){
			if ( (ID != 'C') || (sx->ID[1] != '_') ) continue;

			thprintf(buf, TSIZEOF(buf), T(";<%cPPc[%s]>; %s"),
				(sx->hWnd == Sm->ppc.hLastFocusWnd) ? '*' : ' ',
				sx->ID + 2,
				sx->path);
		}else{
			switch ( ID ){
				case 'B':
					name = T("PPb");
					break;
				case 'C':
					name = T("PPc");
					break;
				case 'V':
					name = T("PPv");
					break;
				case 'c':
					name = T("PPcust");
					break;
				default:
					continue;
			}
			if ( (ID == 'C') && (sx->ID[2] <= 'Z') ){
				thprintf(buf, TSIZEOF(buf), T("%c%s ;%c%s %s"),
						sx->ID[0], sx->ID + 2,
						(sx->hWnd == Sm->ppc.hLastFocusWnd) ? '*' : ' ',
						name,
						sx->path);
			}else {
				thprintf(buf, TSIZEOF(buf), T("%c%s ; %s"),
						sx->ID[0],
						(sx->ID[1] == '_') ? sx->ID + 1 : sx->ID,
						name);
			}
		}

		if ( (tstristr(buf, spms->first) != NULL) ||
			 ExtraSearchString(buf, NULL, spms->first, spms) ){
			AddListItem(list, buf);
			list->items++;
		}
	}
	FreePPx();
}

static void ListUserCombination(PPxEDSTRUCT *PES, LISTADDINFO *mainlist, LISTADDINFO *sublist, SEARCHPARAMS *spms, TCHAR *line)
{
	TCHAR cmdbuf[CMDLINESIZE], *cmdptr, *cmdnext, *cmdmore;
	LISTADDINFO *list;
	TCHAR *firstWordPtr;
	size_t firstWordLen;
	BOOL add;

	firstWordPtr = FindCommandParamPoint(line, spms->first - line /* nwP相当 */, &firstWordLen);

	ThGetString(&PES->LocalStringValue, LSV_user_comb, cmdbuf, TSIZEOF(cmdbuf) - 20);
	cmdptr = cmdbuf;
	for (;;){
		SkipSpace((const TCHAR **)&cmdptr);
		if ( *cmdptr == '^' ){
			cmdptr++;
			list = sublist;
		}else{
			list = mainlist;
		}
		if ( *cmdptr == '+' ){
			cmdptr++;
			add = TRUE;
		}else{
			add = FALSE;
		}
		cmdnext = tstrchr(cmdptr, ' ');
		if ( cmdnext != NULL ) *cmdnext = '\0';
		cmdmore = tstrchr(cmdptr, ':');
		if ( cmdmore != NULL ) *cmdmore = '\0';
		spms->startTick = GetTickCount();

		if ( *cmdptr == '1' ){ // 第１パラメータの時だけ有効
			if ( firstWordPtr == NULL ) cmdptr++;
		}else if ( *cmdptr == '2' ){ // 第２パラメータ以降の時だけ有効
			if ( firstWordPtr != NULL ) cmdptr++;
		}

		if ( tstrcmp(cmdptr, T("alias")) == 0 ){
			AliasSearch(list, spms SMODEPARAM(SMODE_WORDPATH));
		}else if ( tstrcmp(cmdptr, T("aux")) == 0 ){
			AuxSearch(list, spms->first);
		}else if ( tstrcmp(cmdptr, T("calc")) == 0 ){
			AddCalcNumber(list, spms->first, spms->firstlen);
		}else if ( tstrcmp(cmdptr, T("curdir")) == 0 ){
			AddMacros(list, PES, spms, 0, add);
		}else if ( tstrcmp(cmdptr, T("curname")) == 0 ){
			AddMacros(list, PES, spms, 1, add);
		}else if ( tstrcmp(cmdptr, T("curpath")) == 0 ){
			AddMacros(list, PES, spms, 2, add);
		}else if ( tstrcmp(cmdptr, T("pairdir")) == 0 ){
			AddMacros(list, PES, spms, 3, add);
		}else if ( tstrcmp(cmdptr, T("pairname")) == 0 ){
			AddMacros(list, PES, spms, 4, add);
		}else if ( tstrcmp(cmdptr, T("pairpath")) == 0 ){
			AddMacros(list, PES, spms, 5, add);
		}else if ( tstrcmp(cmdptr, T("ppclist")) == 0 ){
			PPxListSearch(list, spms, 0);
		}else if ( tstrcmp(cmdptr, T("ppxidlist")) == 0 ){
			PPxListSearch(list, spms, 1);
		}else if ( tstrcmp(cmdptr, T("cmd")) == 0 ){
			if ( firstWordPtr == NULL ){
				TextSearch(list, &filltext_cmd, spms, 0);
			}else{
				TextSearch2nd(list, &filltext_cmd, spms, firstWordPtr, firstWordLen);
			}
		}else if ( tstrcmp(cmdptr, T("cmd1")) == 0 ){
			TextSearch(list, &filltext_cmd, spms, 0);
		}else if ( tstrcmp(cmdptr, T("entry")) == 0 ){
			EntrySearch(PES, list, spms);
		}else if ( tstrcmp(cmdptr, T("env")) == 0 ){
			if ( *spms->first == '%' ){
				EnvironmentStringsSearch(list, spms->first, spms->firstlen, spms->braket, PES->list.WhistID);
			}
		}else if ( tstrcmp(cmdptr, T("vitem")) == 0 ){
			StringValueSearch(list, PES, spms, 0, add, cmdmore);
		}else if ( tstrcmp(cmdptr, T("vlist")) == 0 ){
			StringValueSearch(list, PES, spms, 1, add, cmdmore);
		}else if ( tstrcmp(cmdptr, T("clist")) == 0 ){
			SettingValueSearch(list, spms, add, cmdmore);
		}else if ( tstrcmp(cmdptr, T("hist")) == 0 ){
			WORD RhistID;

			RhistID = PES->list.RhistID;
			if ( cmdmore != NULL ){
				TINPUT_EDIT_OPTIONS options;

				cmdmore--; // cmdmore は "history" の分だけ戻っても問題なし
				cmdmore[0] = 'e';
				cmdmore[1] = ',';
				if ( IsTrue(GetEditMode((const TCHAR **)&cmdmore, &options)) ){
					RhistID = options.hist_readflags;
				}
			}
			HistorySearch(list, spms, RhistID SMODEPARAM(SMODE_PATH), 0);
		}else if ( tstrcmp(cmdptr, T("mask")) == 0 ){
			TextSearch(list, &filltext_mask, spms, 0);
		}else if ( tstrcmp(cmdptr, T("module")) == 0 ){
			ModuleSearch(PES, list, spms->first, spms->firstlen);
		}else if ( tstrcmp(cmdptr, T("path")) == 0 ){
			TextSearch(list, &filltext_path, spms, 0);
		}else if ( tstrcmp(cmdptr, T("shell")) == 0 ){
			ShellDriveSearch(list, spms->first, spms->braket);
		}else if ( tstrcmp(cmdptr, T("user")) == 0 ){
			if ( PES->list.filltext_user.mem != NULL ){
				if ( firstWordPtr == NULL ){
					TextSearch(list, &PES->list.filltext_user, spms, 0);
				}else{
					TextSearch2nd(list, &PES->list.filltext_user, spms, firstWordPtr, firstWordLen);
				}
			}
		}else if ( tstrcmp(cmdptr, T("user1")) == 0 ){
			TextSearch(list, &PES->list.filltext_user, spms, 0);
		}else if ( !Isdigit(*cmdptr) ){
			tstrcat(cmdptr, T(" - bad type"));
			SetMessageForEdit(PES->hWnd, cmdptr);
			return;
		}
		if ( cmdnext == NULL ) return;
		cmdptr = cmdnext + 1;
	}
}

static void ListHeightFix(PPxEDSTRUCT *PES, HWND hListWnd, int items, UINT msg, RECT *deskbox)
{
	RECT box;
	int height, newheight;

	GetWindowRect(hListWnd, &box);
	height = box.bottom - box.top;
	newheight = min(items, (int)X_flst_height) * PES->list.itemheight + (height % PES->list.itemheight);
	if ( msg == LB_ADDSTRING ){ // 下配置
		if ( (box.top + newheight) > deskbox->bottom ){
			newheight = deskbox->bottom - box.top;
		}
		if ( newheight == height ) return;
		SetWindowPos(hListWnd, NULL, 0, 0,
				box.right - box.left, newheight,
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}else{ // 上配置
		int newtop;

		newtop = PES->list.upperbottom - newheight;
		if ( newtop < deskbox->top ){
			newtop = deskbox->top;
			newheight = box.bottom - newtop;
		}
		if ( (newheight == height) && (box.top == newtop) ) return;
		SetWindowPos(hListWnd, NULL, box.left, newtop,
				box.right - box.left, newheight,
				SWP_NOACTIVATE | SWP_NOZORDER); // ここで行数に併せて高さ調整
		GetWindowRect(hListWnd, &box);
		height = box.bottom - box.top;
		if ( height != newheight ){
			SetWindowPos(hListWnd, NULL,
				box.left, box.top + (newheight - height), 0, 0,
				SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
		}
	}
}

typedef struct { // KeyStepFillMain に情報を渡した後、廃棄される構造体
	PPxEDSTRUCT *PES;
	HANDLE hReadyEvent;
	TCHAR *option;
} KEYSTEPFILLMAIN_INFO;
TCHAR KeyStepFill_listmode[] = T("");

static THREADEXRET THREADEXCALL KeyStepFillMain(KEYSTEPFILLMAIN_INFO *ksfinfo)
{
	HWND hWnd;
	ECURSOR cursor;
	TCHAR linebuf[CMDLINESIZE * 2], option[CMDLINESIZE], *line;

	int dbllist;
	TCHAR *startP;
	DWORD nwP;
	size_t len;
	size_t firstWordLen;
	LISTADDINFO mainlist, sublist;

	PPxEDSTRUCT *PES;
	BOOL listmode, threadmode;
	SEARCHPARAMS spms;
	RECT deskbox;

	PES = ksfinfo->PES;
	PES->ref++;

	PES->ActiveListThreadID = spms.ThreadID = GetCurrentThreadId();
	spms.ActiveListThreadID = &PES->ActiveListThreadID;

	option[0] = '\0';
	listmode = FALSE;
	if ( ksfinfo->option != NULL ){
		if ( ksfinfo->option == KeyStepFill_listmode ){
			listmode = TRUE;
		}else{
			tstrcpy(option, ksfinfo->option);
		}
	}
	if ( PES->list.flags & LISTFLAG_LISTMODE ) listmode = TRUE;

	if ( PES->flags & PPXEDIT_TEXTEDIT ){
		if ( PES->list.RhistID == 0 ){
			PES->list.RhistID = PPXH_COMMAND | PPXH_GENERAL | PPXH_DIR | PPXH_PATH | PPXH_SEARCH | PPXH_PPCPATH;
		}
		if ( PES->list.WhistID == 0 ){
			PES->list.WhistID = PPXH_GENERAL;
		}
	}

	if ( ksfinfo->hReadyEvent != NULL ){ // Sub thread mode
		THREADSTRUCT threadstruct = {T("Fill list"), XTHREAD_EXITENABLE | XTHREAD_TERMENABLE, NULL, 0, 0};

		PES->ListThreadCount++;
		SetEvent(ksfinfo->hReadyEvent); // 待機完了 / ksfinfo 解放指示
		PPxRegisterThread(&threadstruct);
		(void)CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
		threadmode = TRUE;
		mainlist.wParam = sublist.wParam = spms.ThreadID;
	}else{ // on thread mode
		threadmode = FALSE;
		mainlist.wParam = sublist.wParam = 0;
	}
	hWnd = PES->hWnd;
	spms.edmode = PES->ED.cmdsearch & CMDSEARCH_MATCHFLAGS;
	if ( X_flst_mode >= X_fmode_auto2ex ) spms.edmode |= CMDSEARCH_NOUNC;
	if ( PES->flags & PPXEDIT_TEXTEDIT ) spms.edmode |= CMDSEARCH_MULTI; // PPeのときは、1行のみの動作を無効にする。

	spms.braket = 0;
												// サイズチェック
	line = linebuf;
	linebuf[0] = '\0';
	if ( IsTrue(listmode) ){
		cursor.start = cursor.end = 0;
	}else{
		if ( GetWindowText(hWnd, linebuf, CMDLINESIZE) >= (CMDLINESIZE - 1) ){
			int getlen = GetWindowTextLength(hWnd);
			if ( (getlen >= 0x200000) || (WinType < WINTYPE_VISTA) ) goto fin2;
			line = (TCHAR *)HeapAlloc(DLLheap, 0, TSTROFF(getlen + CMDLINESIZE));
			if ( line == NULL ) goto fin2;
			GetWindowText(hWnd, line, getlen + CMDLINESIZE);
		}
												// 編集文字列(全て)を取得
		GetEditSel(hWnd, line, &cursor);
	}
	if ( option[0] != '\0' ){
		// line は常に CMDLINESIZE 以上の余裕を確保している
		tstrcpy(line + cursor.start, option);
		cursor.end = cursor.start + tstrlen(line + cursor.start);
	}

	spms.edmode |= CMDSEARCH_ONE | CMDSEARCH_EDITBOX |
			((PES->list.WhistID & PPXH_COMMAND) ?
					CMDSEARCH_CURRENT : CMDSEARCH_OFF) |
			((PES->flags & PPXEDIT_SINGLEREF) ? 0 : CMDSEARCH_MULTI);

	if ( (PES->list.WhistID & (PPXH_DIR | PPXH_PPCPATH)) &&
		 IsTrue(GetCustDword(T("X_fdir"), TRUE)) ){
		setflag(spms.edmode, CMDSEARCH_DIRECTORY);
	}

	PES->list.ListFocus = LISTU_FOCUSMAIN;
	mainlist.hWnd = PES->list.hWnd;
	mainlist.OrgProc = PES->list.OrgProc;
	mainlist.msg = (PES->list.direction >= 0) ? LB_ADDSTRING : LB_INSERTSTRING;
	mainlist.items = 0;
	mainlist.show = 0;
	dbllist = (X_flst_mode >= X_fmode_auto2) && !(PES->flags & PPXEDIT_COMBOBOX);
	if ( dbllist ){
		sublist.hWnd = PES->list.hSubWnd;
		sublist.OrgProc = PES->list.OrgSubProc;
		sublist.msg = LB_INSERTSTRING;
		sublist.items = 0;
		sublist.show = 0;
	}else{
		sublist = mainlist;
		sublist.show = 1;
	}

		// 検索対象の切り出し
	if ( !(spms.edmode & CMDSEARCH_MULTI) ){ // 単一パラメータ
		cursor.start = 0;
		if ( X_flst_mode < X_fmode_auto2ex ){
			cursor.end = tstrlen32(line);
		}else{
			line[cursor.end] = '\0';
		}
	}else{
										// 範囲選択がされていない時の抽出処理 -
		if ( cursor.start == cursor.end ){
			// 空白区切りの１語を検索対象にする
			spms.braket = GetWordStrings(line, &cursor, GWSF_SPLIT_PARAM) + 1;
		}else{
			spms.braket = GWS_BRAKET_NONE + 1;
		}
		line[cursor.end] = '\0';
	}

	PES->list.select.start = nwP = cursor.start;
	PES->list.select.end = cursor.end;
	if ( IsTrue(listmode) ){
		PES->list.select.end = GetWindowTextLength(hWnd);
	}
#ifndef UNICODE
	if ( xpbug < 0 ){
		if ( IsTrue(listmode) ){
			PES->list.select.end = EC_LAST; // 一覧時は末尾であれば良い
							// ※ CaretFixToW に必要な line が用意していない
		}else{
			CaretFixToW(line, &PES->list.select.end);
		}
	}
#endif
	startP = line + nwP;

	// 補完一覧の作成 ------------------------------
	// Tickオーバーフローは、補完がその場で終わるだけなので対処しない
	len = tstrlen(startP);

	spms.x.handle = 0;
	spms.first = startP;
	spms.firstlen = len;
	spms.second = 0;
	spms.secondlen = 0;
	spms.startTick = GetTickCount();

	if ( PES->list.flags & LISTFLAG_USER_COMB ){
		ListUserCombination(PES, &mainlist, &sublist, &spms, line);
	}else{
		if ( PES->list.filltext_user.mem != NULL ){
			TCHAR *firstWordPtr;

			firstWordPtr = FindCommandParamPoint(line, nwP, &firstWordLen);
			if ( firstWordPtr == NULL ){
				TextSearch(&mainlist, &PES->list.filltext_user, &spms SMODEPARAM(SMODE_PATH), 0);
			}else{
				TextSearch2nd(&sublist, &PES->list.filltext_user, &spms, firstWordPtr, firstWordLen);
			}
		}

		switch ( PES->list.WhistID ){
			case PPXH_COMMAND: {
				WORD histmask;
				TCHAR *firstWordPtr;

				AddCalcNumber(&sublist, startP, len);

				firstWordPtr = FindCommandParamPoint(line, nwP, &firstWordLen);
				if ( firstWordPtr == NULL ){ // 最初の単語ならコマンドの補完を行う
					HistorySearch(&sublist, &spms, PPXH_COMMAND SMODEPARAM(SMODE_WORDPATH), 0);
					AliasSearch(&sublist, &spms SMODEPARAM(SMODE_WORDPATH));
					TextSearch(&sublist, &filltext_cmd, &spms SMODEPARAM(SMODE_WORDPATH), 0);
					spms.startTick = GetTickCount();
					EntrySearch(PES, &mainlist, &spms);
					histmask = (WORD)~PPXH_COMMAND;
				}else{ // ２語目以降はパラメータ扱い
					TextSearch2nd(&sublist, &filltext_cmd, &spms, firstWordPtr, firstWordLen);

					spms.startTick = GetTickCount();
					EntrySearch(PES, &mainlist, &spms);
					HistorySearch(&sublist, &spms, PPXH_DIR_R SMODEPARAM(SMODE_WORDPATH), spms.braket);
					TextSearch(&mainlist, &filltext_path, &spms SMODEPARAM(SMODE_PATH), spms.braket);
					AliasSearch(&sublist, &spms SMODEPARAM(SMODE_WORDPATH));
					histmask = (WORD)~PPXH_DIR_R;
				}
				ModuleSearch(PES, &mainlist, startP, len);
				HistorySearch(&sublist, &spms, PES->list.RhistID & (WORD)~histmask SMODEPARAM(SMODE_PATH), 0);
				if ( *startP == '%' ){
					EnvironmentStringsSearch(&mainlist, startP, len, spms.braket, PES->list.WhistID);
				}
				if ( firstWordPtr != NULL ) AuxSearch(&mainlist, startP);
				ShellDriveSearch(&mainlist, startP, spms.braket);
				break;
			}
			case PPXH_MASK:
				HistorySearch(&mainlist, &spms, PPXH_MASK SMODEPARAM(SMODE_MASK), 0);
				TextSearch(&sublist, &filltext_mask, &spms SMODEPARAM(SMODE_MASK), 0);
				HistorySearch(&sublist, &spms, PES->list.RhistID & (WORD)~PPXH_MASK SMODEPARAM(SMODE_PATH), 0);
				break;

			case PPXH_GENERAL:
				AddCalcNumber(&sublist, startP, len);

			case PPXH_DIR:
			case PPXH_FILENAME:
			case PPXH_PATH:

			case PPXH_PPCPATH:
			case PPXH_PPVNAME:
				HistorySearch(&mainlist, &spms, PES->list.WhistID SMODEPARAM(SMODE_PATH), 0);
				TextSearch(&sublist, &filltext_path, &spms SMODEPARAM(SMODE_PATH), 0);
				spms.startTick = GetTickCount();
				EntrySearch(PES, &mainlist, &spms);
				ModuleSearch(PES, &mainlist, startP, len);
				HistorySearch(&sublist, &spms, PES->list.RhistID & (WORD)~PES->list.WhistID SMODEPARAM(SMODE_PATH), 0);
				if ( *startP == '%' ){
					EnvironmentStringsSearch(&mainlist, startP, len, spms.braket, PES->list.WhistID);
				}
				AuxSearch(&mainlist, startP);
				ShellDriveSearch(&mainlist, startP, spms.braket);
				break;

			default:
				HistorySearch(&mainlist, &spms, PES->list.RhistID SMODEPARAM(SMODE_WORDPATH), 0);
				break;
		}
	}
	if ( PES->ActiveListThreadID != spms.ThreadID ) goto fin;

	//-----------------------
	if ( mainlist.items + sublist.items ){ // 項目有り
		GetDesktopRect(PES->hWnd, &deskbox);
	}else{
		if ( PES->flags & PPXEDIT_TEXTEDIT ){ // 手動表示のときはダミー表示
			mainlist.items = 1;
			GetDesktopRect(PES->hWnd, &deskbox);
		}
	}
	if ( dbllist ){
		if ( sublist.items == 0 ){
			ShowWindow(sublist.hWnd, SW_HIDE);
		}else{
			if ( UseCompListModule > 0 ){
				SendMessage(sublist.hWnd, WM_PPXCOMMAND, TMAKEWPARAM(KE_editword, spms.firstlen), (LPARAM)spms.first);
			}
			ListHeightFix(PES, sublist.hWnd, sublist.items, sublist.msg, &deskbox);
			// 一番最後を表示し、カーソルを表示させない
			SendMessage(sublist.hWnd, LB_SETCURSEL, SendMessage(sublist.hWnd, LB_GETCOUNT, 0, 0) - 1, 0);
			SendMessage(sublist.hWnd, LB_SETCURSEL, (WPARAM)-1, 0);
			SendMessage(sublist.hWnd, WM_SETREDRAW, TRUE, 0);
			ShowWindow(sublist.hWnd, SW_SHOWNA);
		}
	}else{
		mainlist.items += sublist.items;
	}

	if ( mainlist.items == 0 ){
		ShowWindow(mainlist.hWnd, SW_HIDE);
		if ( sublist.items == 0 ) PES->list.ListFocus = LISTU_NOLIST;
	}else{
		if ( UseCompListModule > 0 ){
			SendMessage(mainlist.hWnd, WM_PPXCOMMAND, TMAKEWPARAM(KE_editword, spms.firstlen), (LPARAM)spms.first);
		}
		ListHeightFix(PES, mainlist.hWnd, mainlist.items, mainlist.msg, &deskbox);
		SendMessage(mainlist.hWnd, WM_SETREDRAW, TRUE, 0);
		ShowWindow(mainlist.hWnd, SW_SHOWNA);
	}

	BackupText(hWnd, PES);
	if ( (X_flst_mode >= X_fmode_auto2ex) && (PES->list.OldString != NULL) ){
		DWORD lP;

		lP = PES->list.OldEnd;
		CaretFixToA(PES->list.OldString, &lP);
		if ( *(PES->list.OldString + lP) == '.' ){
			PES->list.mode = PES->list.startmode = LIST_FILLEXT;
		}
	}
fin:
	if ( spms.x.handle != 0 ) FreeExtraSearch(&spms);
fin2:
	if ( PES->ActiveListThreadID == spms.ThreadID ) PES->ActiveListThreadID = 0;
	if ( line != linebuf ) HeapFree(DLLheap, 0, line);

	if ( threadmode == FALSE ){
		FreeEdStruct(PES);
	}else{
		PES->ListThreadCount--;
		FreeEdStruct(PES);
		PPxUnRegisterThread();
		CoUninitialize();
	}
	t_endthreadex(0);
}

void CancelListThread(PPxEDSTRUCT *PES)
{
	#define ThreadChkSleepTime 20
	int WaitTimer = 100 / ThreadChkSleepTime;

	PES->ActiveListThreadID = 0;

	for ( ;; ){
		if ( PES->ListThreadCount == 0 ) break;
		PeekMessageLoop(PES->hWnd);
		Sleep(ThreadChkSleepTime);
		if ( --WaitTimer <= 0 ) break;
	}
}

void KeyStepFill(PPxEDSTRUCT *PES, TCHAR *option)
{
	KEYSTEPFILLMAIN_INFO ksfinfo;
	MSG msginfo;

	if ( PES->list.flags & LISTFLAG_DISABLE ) return;

	ksfinfo.PES = PES;
	ksfinfo.option = option;
	ksfinfo.hReadyEvent = NULL;
	PES->ActiveListThreadID = 0;

	if ( IsTrue(PeekMessage(&msginfo, PES->hWnd, WM_CHAR, WM_CHAR, PM_NOREMOVE)) ){
		return; // 更に入力があるので、その時に処理する
	}
	if ( IsTrue(PeekMessage(&msginfo, PES->hWnd, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE)) ){
		if ( (msginfo.wParam == VK_RETURN) || ((msginfo.wParam >= '0') && (msginfo.wParam <= 'Z')) ){
			return; // 更に入力があるので、その時に処理する
		}
	}
	if ( PES->ListThreadCount > LIST_THREAD_MAX ){
#ifndef RELEASE
		XMessage(NULL, NULL, XM_DbgLOG, T("> KeyStepFill ") T(DefineToStr(LIST_THREAD_MAX)) T(" Thread"));
		SetMessageForEdit(PES->hWnd, T("> KeyStepFill ") T(DefineToStr(LIST_THREAD_MAX)) T(" Thread"));
#endif
		return; // スレッド数が多すぎるので中止
	}

	// メインリストウィンドウ作成
	if ( PES->list.hWnd == NULL ){
		int direction;

		PES->list.mode = PES->list.startmode = LIST_FILL;
		direction = (PES->flags & PPXEDIT_COMBOBOX) ? -1 : 1;

		CreateMainListWindow(PES, direction);
	}else{
		SendMessage(PES->list.hWnd, WM_SETREDRAW, FALSE, 0);
		SendMessage(PES->list.hWnd, LB_RESETCONTENT, TRUE, 0);
	}
	// サブリストウィンドウ作成
	if ( (X_flst_mode >= X_fmode_auto2) &&
		 !(PES->flags & PPXEDIT_COMBOBOX) ){
		if ( PES->list.hSubWnd == NULL ){
			CreateSubListWindow(PES);
		}else{
			SendMessage(PES->list.hSubWnd, WM_SETREDRAW, FALSE, 0);
			SendMessage(PES->list.hSubWnd, LB_RESETCONTENT, TRUE, 0);
		}
	}

	if ( X_flto.stoptime == 0 ){
		GetCustData(T("X_flto"), &X_flto, sizeof(X_flto));

		#ifndef _WIN64
		if ( WinType < WINTYPE_2000 ) {
			X_flto.stoptime = OnAllowTick;
		}else
		#endif
		{
			X_flto.stoptime = ThreadAllowTick;
		}
	}

	#ifndef WINEGCC
	#ifndef _WIN64
	if ( WinType >= WINTYPE_2000 ) // 次の行に続く
	#endif
	{
		HANDLE hThread;

		ksfinfo.hReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		hThread = t_beginthreadex(NULL, 0,
				(THREADEXFUNC)KeyStepFillMain,
				&ksfinfo, 0, (DWORD *)&PES->ActiveListThreadID);
		if ( hThread != NULL ){
			WaitForSingleObject(ksfinfo.hReadyEvent, INFINITE);
			CloseHandle(ksfinfo.hReadyEvent);
			SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
			CloseHandle(hThread);
			return;
		}
		CloseHandle(ksfinfo.hReadyEvent);
		ksfinfo.hReadyEvent = NULL;
	}
	#endif
	KeyStepFillMain(&ksfinfo);
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
	if ( IsRichMode(PES) ) tstrreplace(ets.text, T("\r\n"), T("\n"));

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



#ifndef CB_GETCOMBOBOXINFO
#define CB_GETCOMBOBOXINFO 0x0164 // XP以降
#endif
typedef struct {
	DWORD cbSize;
	RECT rcItem;
	RECT rcButton;
	DWORD stateButton;
	HWND hwndCombo;
	HWND hwndItem;
	HWND hwndList;
} xCOMBOBOXINFO;

HWND PPxRegistExEditCombo(HWND hED, int maxlen, const TCHAR *defstr, WORD rHist, WORD wHist, int flags)
{
	int i;
	const TCHAR *p;
	POINT LP = {4, 4};
	HWND hRealED;
	LISTADDINFO list = {NULL, NULL, CB_ADDSTRING, 0, 0, 1};
	SEARCHPARAMS spms;
	xCOMBOBOXINFO cbi;

	SendMessage(hED, CB_LIMITTEXT, maxlen - 1, 0);
	if ( (defstr != NULL) && (*defstr != '\0') ){
		SendMessage(hED, CB_ADDSTRING, 0, (LPARAM)defstr);
		SendMessage(hED, CB_SETCURSEL, 0, 0);
	}
									// ヒストリ内容を登録
	UsePPx();
	for ( i = 0 ; i < CompListItems ; i++ ){
		if ( (p = EnumHistory((WORD)wHist, i)) == NULL ) break;
		SendMessage(hED, CB_ADDSTRING, 0, (LPARAM)p);
	};
	FreePPx();

	list.hWnd = hED;
	list.OrgProc = NULL;
	spms.x.handle= 0;
	spms.startTick = GetTickCount();
	spms.first = NilStrNC;
	spms.firstlen = 0;
	spms.second = NULL;
	spms.secondlen = 0;
	spms.ThreadID = 0;

	TextSearch(&list, &filltext_mask, &spms SMODEPARAM(SMODE_MASK), 0);
	UsePPx();
	for ( ; i < CompListItems ; i++ ){
		if ( (p = EnumHistory((WORD)(rHist & ~wHist), i)) == NULL ) break;
		SendMessage(hED, CB_ADDSTRING, 0, (LPARAM)p);
	};
	FreePPx();

	cbi.cbSize = sizeof(cbi);
	cbi.hwndItem = NULL;
	if ( (SendMessage(hED, CB_GETCOMBOBOXINFO, 0, (LPARAM)&cbi) != 0) && (cbi.hwndItem != NULL) ){
		hRealED = cbi.hwndItem;
	}else for ( ; ; ){ // XP より前は座標で取得する
		hRealED = ChildWindowFromPoint(hED, LP);
		if ( hRealED == NULL ) break;
		if ( hRealED == hED ){
			LP.x += 2;
			LP.y += 2;
			continue;
		}
		break;
	}
	if ( flags & PPXEDIT_INSTRSEL ){ // 範囲選択を後回しで行う
		PostMessage(hRealED, WM_PPXCOMMAND, KE_setselectCB, 0);
	}
	return hRealED;
}
