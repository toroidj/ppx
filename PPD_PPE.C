/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library								PPx edit UI
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include <richedit.h>
#include "PPXVER.H"
#include "PPX.H"
#include "VFS.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#include "PPD_EDL.H"
#pragma hdrstop

//-------------------------------------- ウィンドウのスタイル
#define PPESTYLE	WS_OVERLAPPEDWINDOW
#define PPESTYLEEX	WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME

//-------------------------------------- グローバル変数定義
const TCHAR PPeName[] = T("PPe");

typedef struct {
	PPXAPPINFO info;		// PPx 共通情報 ※必ず先頭に配置
	HWND hEWnd;			// Edit Box の HWND
	HFONT hBoxFont;		// 等幅フォント
	int fontX, fontY;	// hBoxFont の寸法
	DWORD dpi;
	BOOL ShowModify;	// "*" 表示済み
	DYNAMICMENUSTRUCT DynamicMenu;
	TCHAR CurDir[VFPS]; // カレントディレクトリ
	TCHAR PosName[5]; // 窓の位置を記憶する名前
	HANDLE hMenuTheme;
} PPeditSTRUCT;

PPXINMENU PPebarFile[] = {
//	{ K_c | 'N'	, T("New\tCtrl+N")}, // 同じウィンドウでNew
	{ K_c | K_s | 'N'	, T("&New\tCtrl+Shift+N")},
	{ K_c | 'O'	, T("&Open...\tCtrl+O")},
	{ K_c | 'S'	, T("&Save\tCtrl+S")},
	{ K_c | K_s | 'S'		, T("Save &As...\tCtrl+Shift+S")},
	{ K_F1		, T("other &Menu\tF1")},
	{ KE_ed,		T("&Duplicate PPe")},
	{ KE_ei,		T("&Insert from file")},
	{PPXINMENY_SEPARATE, NULL },
//	{ , T("Page setup")}, // ページ設定
	{ KE_kp	,		T("&Print by PPv\tCtrl+P")}, // 印刷
	{PPXINMENY_SEPARATE, NULL },
	{ KE_ee,		T("&Exec command")},
	{PPXINMENY_SEPARATE, NULL },
	{ KE_ec	, T("E&xit\tAlt+F4")},
	{0, NULL}
};
PPXINMENU PPebarEdit[] = {
	{ K_c | 'Z'	, T("&Undo\tCtrl+Z")},
	{PPXINMENY_SEPARATE, NULL },
	{ K_c | 'X',	T("%G\"JMCU|Cut\"(&T)\tCtrl+X")},
	{ K_c | 'C',	T("%G\"JMCL|Copy\"(&C)\tCtrl+C")},
	{ K_c | 'V',	T("%G\"JMPA|Paste\"(&P)\tCtrl+V")},
	{ K_del,		T("%G\"JMDE|Delete\"(&D)\tDelete")},
	{PPXINMENY_SEPARATE, NULL },
//	{ K_c | 'E',	T("Web searchtCtrl+&E")},
	{ K_c | 'F',	T("&Find...\tCtrl+&F")},
	{ K_F3,			T("Find &Next\tF3")},
	{ K_s | K_F3,	T("Find &Prev\tShift + F3")},
	{ K_F7,			T("&Replace...\tF7")},
	{ KE_qj,		T("&Goto line\tCtrl+Q - J")},
	{PPXINMENY_SEPARATE, NULL },
	{ K_c | 'Q',	T("Edit menu\tCtrl+&Q")},
	{ K_c | 'K',	T("Menu&2\tCtrl+K")},
	{PPXINMENY_SEPARATE, NULL },
	{ K_c | 'A'		, T("select &All\tCtrl+A")},
	{ K_F5, T("Insert &date\tF5")},
	{0, NULL}
};
PPXINMENU PPebarView[] = {
	{ K_c | K_v | VK_ADD	, T("Zoom-&In\tCtrl+'+'")},	// Zoom-+
	{ K_c | K_v | VK_SUBTRACT	, T("Zoom-&Out\tCtrl+'-'")},	// Zoom--
	{ K_c | K_v | VK_NUMPAD0	, T("Zoom-Default\tCtrl+0")},	// Zoom-0
	{PPXINMENY_SEPARATE, NULL },
	{ K_s | K_F2, T("Settings\tShift+F2")},
	{ KE_2p,	T("&Word warp")}, // 書式-右端で折り返す
	{(DWORD_PTR)T("??charset"), NilStr},
	{(DWORD_PTR)T("??returnset"), NilStr},
	{(DWORD_PTR)T("??tabset"), NilStr},
	{(DWORD_PTR)T("??locate"), NilStr},
//	{ , T("Fonts")},	// 書式-フォント
	{0, NULL}
};
PPXINMENU PPebarHelp[] = {
	{ K_s | K_F1, T("&Help")},
	{PPXINMENY_SEPARATE, NULL },
	{ K_ex, T("Paper Plane eUI")},
	{ K_ex, T("Version ") T(FileProp_Version)},
	{ K_ex, T("(") T(__DATE__) T(",") RUNENVSTRINGS T(")")},
	{ K_ex, T("(c)TORO")},
	{0, NULL}
};

PPXINMENUBAR ppebar[] = {
	{T("&File"), PPebarFile},
	{T("&Edit"), PPebarEdit},
	{T("&View"), PPebarView},
	{T("&Help"), PPebarHelp},
	{NULL, NULL}
};

const TCHAR StrK_ppe[] = T("K_ppe");

/*-----------------------------------------------------------------------------
	桁と幅に合わせてウィンドウを調節する
-----------------------------------------------------------------------------*/
HFONT CreatePPeFont(int dpi)
{
	LOGFONTWITHDPI cursfont;

	GetPPxFont(PPXFONT_F_mes, dpi, &cursfont);
	return CreateFontIndirect(&cursfont.font);
}

void FixEditWindowRect(HWND hWnd)
{
	int mx, my;//, x, y;
	RECT rect;
	PPeditSTRUCT *PPES;

	PPES = (PPeditSTRUCT *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	my = (int)SendMessage(PPES->hEWnd, EM_GETLINECOUNT, 0, 0);
	if ( my < 10 ) my = 10;
	if ( my >= 30 ) my = 30;
/*	自動窓枠決定用のコード…使い心地が悪かった(^^;
	for ( y = 1 ; y <= my ; y++ ){
		x = SendMessage(PES->hEWnd, EM_LINEINDEX, y, 0 );// 先頭からのオフセット
		x = SendMessage(PES->hEWnd, EM_LINELENGTH, x, 0 );	// 桁数を得る
		if (x > mx) mx = x;
	}
*/
	mx = 80;

	GetWindowRect(PPES->hEWnd, &rect);
	rect.right	= rect.left + (mx + 1) * PPES->fontX + GetSystemMetrics(SM_CYHSCROLL);
	rect.bottom	= rect.top  + (my + 1) * PPES->fontY + GetSystemMetrics(SM_CXVSCROLL);
	AdjustWindowRectEx(&rect, PPESTYLE | WS_HSCROLL | WS_VSCROLL, TRUE, PPESTYLEEX);
	SetWindowPos(hWnd, NULL, rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			SWP_NOACTIVATE | SWP_NOZORDER);
}

DWORD_PTR USECDECL PPeInfoFunc(PPeditSTRUCT *PPES, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	if ( cmdID <= PPXCMDID_FILL ){
		*uptr->enums.buffer = '\0';
	}else if ( cmdID == PPXCMDID_PPXCOMMAD ){
		SendMessage(PPES->hEWnd, WM_PPXCOMMAND, uptr->key, 0);
	}else if ( cmdID == PPXCMDID_ADDEXMENU ){
		ADDEXMENUINFO *addmenu = (ADDEXMENUINFO *)uptr;
		PPxEDSTRUCT *PES = (PPxEDSTRUCT *)GetProp(PPES->hEWnd, StrPPxED);
		TCHAR buf[80];

		if ( PES == NULL ) return 0;
		if ( !tstrcmp(addmenu->exname, T("tabset")) ){
			thprintf(buf, TSIZEOF(buf), T("&tab: %u"), PES->tab);
			AppendMenuString(addmenu->hMenu, K_M | KE_2t, buf);
			return PPXCMDID_ADDEXMENU;
		}
		if ( !tstrcmp(addmenu->exname, T("charset")) ){
			UINT cp;
			const PPXINMENU *pi;
			InitEditCharCode(PES);

			cp = PES->CharCP;
			if ( cp == 0 ) cp = GetACP();
			if ( cp == CP__SJIS ) cp = VTYPE_SYSTEMCP;

			thprintf(buf, TSIZEOF(buf), T("&charset : %u"), cp);
			pi = charmenu;
			while ( pi->key ){
				if ( pi->key == cp ){
					thprintf(buf, TSIZEOF(buf), T("&Character: %s"), pi->str);
					tstrreplace(buf + 5, T("&"), NilStr);
					break;
				}
				pi++;
			}
			AppendMenuString(addmenu->hMenu, K_M | KE_2c, buf);
			return PPXCMDID_ADDEXMENU;
		}
		if ( !tstrcmp(addmenu->exname, T("returnset")) ){
			const TCHAR *ptr;
			if ( (PES->CrCode < 0) || (PES->CrCode >= 3) ){
				ptr = T("?");
			}else{
				ptr = returnmenu[PES->CrCode].str;
			}
			thprintf(buf, TSIZEOF(buf), T("&return: %s"), ptr);
			tstrreplace(buf + 5, T("&"), NilStr);
			AppendMenuString(addmenu->hMenu, K_M | KE_2r, buf);
			return PPXCMDID_ADDEXMENU;
		}
		if ( !tstrcmp(addmenu->exname, T("locate")) ){
			EditLPos elp = { NULL, NULL, EditLPos_GET_CURSORPOS, 0, 0, NULL};

			if ( PPeLogicalLinePos(PES, &elp) == FALSE ){
				tstrcpy(buf, T("line: ?"));
			}else{
				thprintf(buf, TSIZEOF(buf), T("line: %d  col: %d"), elp.y + 1, elp.x + 1);
				AppendMenuString(addmenu->hMenu, K_M | KE_qj, buf);
			}
			return PPXCMDID_ADDEXMENU;
		}
	}
	return PPXA_INVALID_FUNCTION;
}

const TCHAR SampleFontTextC[] = T("0123456789");

void GetTextMetricsWidth(HDC hDC, TEXTMETRIC *tm)
{
	int result;
	SIZE textsize;

	GetTextMetrics(hDC, tm);
	if ( tm->tmAveCharWidth < 1 ) tm->tmAveCharWidth = 1;

	// tmAveCharWidth が 実物と合っていないなら補正 例) Ricty Diminished 9pt
	GetTextExtentExPoint(hDC, SampleFontTextC, 1, 0, NULL, NULL, &textsize);
	if ( textsize.cx > tm->tmAveCharWidth ) tm->tmAveCharWidth = textsize.cx;

	// ※ TMPF_FIXED_PITCH は意味が反対になっている
	if ( !(tm->tmPitchAndFamily & TMPF_FIXED_PITCH) ) return; // 等幅フォント

	{ // 不等幅フォント
		int NumWidth[10], *Nptr, i, oldwidth = 0, newwidth, width;

		GetTextExtentExPoint(hDC, SampleFontTextC, 10, 0x7000, &result, NumWidth, &textsize);
		for ( i = 0, Nptr = NumWidth ; i <= 9 ; i++ ){
			newwidth = *Nptr++;
			width = newwidth - oldwidth;
			oldwidth = newwidth;
			if ( width > tm->tmAveCharWidth ) tm->tmAveCharWidth = width;
		}
	}
}

PPeditSTRUCT *PPeWmCreate(HWND hWnd, int editmode)
{
	PPeditSTRUCT *PPES;
	HDC hDC;
	HGDIOBJ hOldFont;
	TEXTMETRIC tm;
										// 作業領域を確保 -------------
	PPES = HeapAlloc(DLLheap, 0, sizeof(PPeditSTRUCT));
	if ( PPES == NULL ) return NULL;

	PPES->info.Function = (PPXAPPINFOFUNCTION)PPeInfoFunc;
	PPES->info.Name = PPeName;
	PPES->info.RegID = NilStr;
	PPES->info.hWnd = hWnd;

	PPES->CurDir[0] = '\0';
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)PPES);

										// EditBox を作成 -------------
	editmode = IsUseRichEdit(editmode);

	PPES->hEWnd = CreateWindowEx(WS_EX_ACCEPTFILES,
		RichEditClassname(editmode), NilStr,
		WS_CHILD | /*WS_HSCROLL |*/ WS_VSCROLL |
		/*ES_AUTOHSCROLL |*/ ES_AUTOVSCROLL | ES_NOHIDESEL |
		ES_LEFT | ES_MULTILINE | ES_WANTRETURN,	// ウインドウの形式
		0, 0, 0, 0, hWnd, CHILDWNDID(IDE_PPEMAIN), DLLhInst, 0);

	PPES->dpi = GetMonitorDPI(hWnd);
	PPES->ShowModify = FALSE;
	PPES->hBoxFont = CreatePPeFont(PPES->dpi);
	hDC = GetDC(hWnd);
	hOldFont = SelectObject(hDC, PPES->hBoxFont);
	GetTextMetricsWidth(hDC, &tm);
	PPES->fontX = tm.tmAveCharWidth;
	PPES->fontY = tm.tmHeight;
	PPES->PosName[0] = '\0';
	PPES->hMenuTheme = NULL;
	SelectObject(hDC, hOldFont);
	ReleaseDC(hWnd, hDC);
					// EditBox の Font を等幅に ※RichEdit時は反映されない
	SendMessage(PPES->hEWnd, WM_SETFONT, (WPARAM)PPES->hBoxFont, TRUE);
										// EditBox の拡張 -------------
	PPxRegistExEdit(NULL, PPES->hEWnd, 0x100000, NULL, 0, 0,
			PPXEDIT_USEALT | PPXEDIT_NOWORDBREAK |
			PPXEDIT_ENABLE_SIZE_CHANGE );
	if ( editmode != UXTRICH_NO ){
		SendMessage(PPES->hEWnd, EM_SETEVENTMASK, 0,
				SendMessage(PPES->hEWnd, EM_GETEVENTMASK , 0, 0) | ENM_CHANGE);
		SendMessage(PPES->hEWnd, WM_SETFONT, (WPARAM)PPES->hBoxFont, TRUE);
	}
										// K_ppe を登録 ---
	SendMessage(PPES->hEWnd, WM_PPXCOMMAND, KE_setkeya_firstevent, (LPARAM)StrK_ppe);
	ShowWindow(PPES->hEWnd, SW_SHOWNORMAL);
	return PPES;
}

void PPeWMDpiChanged(HWND hWnd, PPeditSTRUCT *PPES, WPARAM wParam, RECT *newpos)
{
	DWORD newDPI = HIWORD(wParam);
	HDC hDC;
	HFONT hNewFont, hOldFont;
	TEXTMETRIC	tm;

	if ( !(X_dss & DSS_ACTIVESCALE) ) return;
	if ( PPES->dpi == newDPI ) return; // 変更無し(起動時等)
	PPES->dpi = newDPI;

	hNewFont = CreatePPeFont(newDPI);
	SendMessage(PPES->hEWnd, WM_SETFONT, (WPARAM)hNewFont, TRUE);
	DeleteObject(PPES->hBoxFont);
	PPES->hBoxFont = hNewFont;
	hDC = GetDC(hWnd);
	hOldFont = SelectObject(hDC, PPES->hBoxFont);
	GetTextMetricsWidth(hDC, &tm);
	PPES->fontX = tm.tmAveCharWidth;
	PPES->fontY = tm.tmHeight;
	PPES->PosName[0] = '\0';
	SelectObject(hDC, hOldFont);
	ReleaseDC(hWnd, hDC);

	if ( newpos != NULL ){
		SetWindowPos(hWnd, NULL, newpos->left, newpos->top,
				newpos->right - newpos->left, newpos->bottom - newpos->top,
				SWP_NOACTIVATE | SWP_NOZORDER);
	}
	InvalidateRect(hWnd, NULL, TRUE);
}


void PPeFileOpen(HWND hWnd, HWND hEdWnd, const TCHAR *fname)
{
	if ( SendMessage(hEdWnd, WM_PPXCOMMAND, KE_openfile, (LPARAM)fname) == FALSE ){
		return;
	}
	SetWindowText(hWnd, fname);
	FixEditWindowRect(hWnd);
}

void PPeWmDROPFILES(HWND hWnd, PPeditSTRUCT *PPES, HDROP hDrop)
{
	TCHAR name[VFPS];

	DragQueryFile( hDrop, 0, name, TSIZEOF(name) );
	DragFinish(hDrop);
	PPeFileOpen(hWnd, PPES->hEWnd, name);
	SetForegroundWindow(hWnd);
}

void PPeClose(HWND hWnd, PPeditSTRUCT *PPES)
{
	WINPOS WinPos;

	if ( (PPES->PosName[0] != '\0') &&
		 !IsIconic(hWnd) /*&& IsWindowVisible(hWnd)*/ ){
		WinPos.show = 0;
		GetWindowRect(hWnd, &WinPos.pos);
		SetCustTable(T("_WinPos"), PPES->PosName, &WinPos, sizeof(WinPos));
	}
	if ( SendMessage(PPES->hEWnd, WM_PPXCOMMAND, KE_closecheck, 0) ){
		DestroyWindow(hWnd); // 窓の破棄(WM_DESTROY)
	}
}

void SetShowModify(PPeditSTRUCT *PPES, BOOL modify)
{
	TCHAR buf[CMDLINESIZE];

	PPES->ShowModify = modify;
	if ( GetWindowText(PPES->info.hWnd, buf, TSIZEOF(buf)) ){
		size_t len = tstrlen(buf);

		if ( modify ){
			if ( (len >= 2) && (buf[len - 1] == '*') ) return;
			tstrcpy(buf + len, T(" *"));
		} else {

			if ( (len < 2) || (buf[len - 2] != ' ') || (buf[len - 1] != '*') ){
				return;
			}
			buf[len - 2] = '\0';
		}
		SetWindowText(PPES->info.hWnd, buf);
	}
}

/*-----------------------------------------------------------------------------
	メインルーチン
-----------------------------------------------------------------------------*/
LRESULT CALLBACK PPeProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PPeditSTRUCT *PPES;

	PPES = (PPeditSTRUCT *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if ( PPES == NULL ) return DefWindowProc(hWnd, message, wParam, lParam);

	switch (message){
		case WM_CTLCOLOREDIT:
			if ( !(ThemeColors.ExtraDrawFlags & (EDF_WINDOW_TEXT | EDF_WINDOW_BACK)) ){
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
			return ControlWindowColor(wParam);

		case WM_MENUCHAR:
			return PPxMenuProc(hWnd, message, wParam, lParam);

/*
		case WM_NCCREATE:
			if ( (X_dss & DSS_COMCTRL) && (WinType >= WINTYPE_10) ){
				PPxCommonCommand(hWnd, 0, K_ENABLE_NC_SCALE);
			}
			return 1;
*/
		case WM_DROPFILES: // D&D 処理
			PPeWmDROPFILES(hWnd, PPES, (HDROP)wParam);
			break;

		case WM_SETFOCUS:
			SetFocus(PPES->hEWnd);
			break;

		case WM_SIZE:
			SetWindowPos(PPES->hEWnd, NULL, 0, 0,
					LOWORD(lParam), HIWORD(lParam),
					SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
			break;

		case WM_CLOSE:	// 終了要求
			PPeClose(hWnd, PPES);
			break;

		case WM_DESTROY:
													// 等幅フォントの破棄
			PostMessage(PPES->hEWnd, WM_SETFONT, 0, TRUE);
			DeleteObject(PPES->hBoxFont);
			FreeDynamicMenu(&PPES->DynamicMenu);
			HeapFree(DLLheap, 0, PPES);
			break;

		case WM_NCLBUTTONDBLCLK:
			switch(wParam){
				case HTBOTTOM:
				case HTBOTTOMLEFT:
				case HTBOTTOMRIGHT:
				case HTLEFT:
				case HTRIGHT:
				case HTTOP:
				case HTTOPLEFT:
				case HTTOPRIGHT:
					FixEditWindowRect(hWnd);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case K_Mc | 'W':
					PostMessage(hWnd, WM_CLOSE, 0, 0);
					break;

				default: {
					WORD cmd = LOWORD(wParam);

					if ( (cmd >= IDW_MENU) && (cmd <= IDW_MENUMAX) ){	// 0x4000～0x4fff Menu bar ========
						CommandDynamicMenu(&PPES->DynamicMenu, &PPES->info, wParam);
						return 0;
					}
					if ( cmd > K_M ){
						SendMessage(PPES->hEWnd, WM_PPXCOMMAND, wParam - K_M, 0);
						break;
					}
					if ( (HIWORD(wParam) == EN_CHANGE) &&
						 (PPES->ShowModify == FALSE) &&
						 ((HWND)lParam == PPES->hEWnd) ){
						SetShowModify(PPES, TRUE);
					}
				}
			}
			break;

		case WM_DPICHANGED:
			PPeWMDpiChanged(hWnd, PPES, wParam, (RECT *)lParam);
			break;

		case WM_INITMENU:
			DynamicMenu_InitMenu(&PPES->DynamicMenu, (HMENU)wParam, 1);
			break;

		case WM_INITMENUPOPUP:
			DynamicMenu_InitPopupMenu(&PPES->DynamicMenu, (HMENU)wParam, &PPES->info);
			break;

		default:
			if ( message == WM_PPXCOMMAND ){
				switch ( LOWORD(wParam) ){
					case KE_ChangeHWND:
						PPES->hEWnd = (HWND)lParam;
						return (LRESULT)lParam;

					case KE_getHWND:
						return (LRESULT)PPES->hEWnd;

					case KE_clearmodify:
						SetShowModify(PPES, FALSE);
						return 0;

					default:
						return ERROR_INVALID_FUNCTION;
				}
			}
			if ( X_uxt_color == UXT_DARK ){
				if ( ((message <= 0x94) && (message >= 0x91)) ||
//					 (message == WM_THEMECHANGED) ||
					 (message == WM_NCPAINT) || (message == WM_NCACTIVATE) ){
					return DarkMenuWndProc(hWnd, &PPES->hMenuTheme, message, wParam, lParam);
				}
			}
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
PPXDLL HWND PPXAPI PPEui(HWND hPWnd, const TCHAR *title, const TCHAR *text)
{
	PPeditSTRUCT *PES;
	HWND hPPeWnd, hFgWnd;
	int modify = 0, editmode = -1;
	DWORD textsize = 0;
	WINPOS WinPos;

	hFgWnd = hPWnd;
	if ( hFgWnd == BADHWND ) hFgWnd = GetFocus();
	InitSysColors();

	if ( text == PPE_TEXT_CMDMODE ){ // *edit, *ppe
		const TCHAR *param;
		TCHAR buf[CMDLINESIZE], code, *more;

		param = ((PPE_CMDMODEDATA *)title)->param;
		code = GetOptionParameter(&param, buf, &more);
		if ( (code == '-') && (tstrcmp(buf + 1, T("RICH")) == 0) ){
			editmode = 2;
		}
		if ( (code == '-') && (tstrcmp(buf + 1, T("NORICH")) == 0) ){
			editmode = 0;
		}
	}else if ( text != NULL ){
		if ( hPWnd != BADHWND ) textsize = TSTRSIZE(text);
	}else{
		FILE_STAT_DATA nowstat;
		if ( IsTrue(GetFileSTAT(title, &nowstat)) ){
			textsize = nowstat.nFileSizeLow;
		}
	}
	// ファイルサイズに応じて richedit を使用する
	if ( editmode < 0 ){
		if ( (X_uxt_ppe_rich == 1) ||
			 ((X_uxt_ppe_rich > 0) && (textsize >= (DWORD)X_uxt_ppe_rich))
	#ifndef _WIN64
			|| ((OSver.dwMajorVersion < 5) && (textsize >= 30000))
	#endif
		){
			editmode = 2;
		}
	}

	if ( DLLWndClass.item.PPe == 0 ){
		WNDCLASS wcClass;

		wcClass.style			= CS_BYTEALIGNCLIENT | CS_DBLCLKS;
		wcClass.lpfnWndProc		= PPeProc;
		wcClass.cbClsExtra		= 0;
		wcClass.cbWndExtra		= 0;
		wcClass.hInstance		= DLLhInst;
		wcClass.hIcon			= LoadIcon(DLLhInst, MAKEINTRESOURCE(Ic_PPE));
		wcClass.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wcClass.hbrBackground	= NULL;
		wcClass.lpszMenuName	= NULL;
		wcClass.lpszClassName	= T(PPeditWinClass);
		DLLWndClass.item.PPe = RegisterClass(&wcClass);
	}
										// ウィンドウを生成する ---------------
	hPPeWnd = CreateWindowEx(PPESTYLEEX, (LPCTSTR)DLLWndClass.item.PPe,
			MessageText(title), PPESTYLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, DLLhInst, NULL);
	if ( hPPeWnd == NULL ) return NULL;

	PES = PPeWmCreate(hPPeWnd, editmode);
	FixUxTheme(hPPeWnd, NULL);

	if ( hFgWnd != NULL ){ // 位置調整
		RECT deskbox;
		const TCHAR *deftext;

		GetWindowRect(hPPeWnd, &WinPos.pos);
		deftext = title;
		if ( (deftext[0] != '\0') && (deftext[1] != '\0') &&
			 (deftext[2] != '\0') && (deftext[3] != '\0') &&
			 (deftext[4] == '|') ){ // xxxx| なら、位置再現
			memcpy(PES->PosName, title, TSTROFF(4));
			PES->PosName[4] = '\0';
			if ( NO_ERROR == GetCustTable(T("_WinPos"), PES->PosName, &WinPos, sizeof(WinPos)) ){ // ※２
				modify = 2;
			}
		}
		GetDesktopRect(hFgWnd, &deskbox);
		if ( (WinPos.pos.left < deskbox.left) ||
			 (WinPos.pos.left >= deskbox.right) ){
			WinPos.pos.left = deskbox.left;
			modify |= 1;
		}
		if ( (WinPos.pos.top < deskbox.top) ||
			 (WinPos.pos.top >= deskbox.bottom) ){
			WinPos.pos.top = deskbox.top;
			modify |= 1;
		}
		if ( modify == 1 ){ // 位置調整のみ。サイズ調整の時は後で
			SetWindowPos(hPPeWnd, NULL, WinPos. pos.left, WinPos.pos.top, 0, 0,
					SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
		}
	}

	InitDynamicMenu(&PES->DynamicMenu, T("ME_menu"), ppebar);
	SetMenu(hPPeWnd, PES->DynamicMenu.hMenuBarMenu);

	if ( text == PPE_TEXT_CMDMODE ){ // *edit, *ppe
		if ( modify > 1 ){
			SetWindowPos(hPPeWnd, NULL,
					WinPos.pos.left, WinPos.pos.top,
					WinPos.pos.right - WinPos.pos.left,
					WinPos.pos.bottom - WinPos.pos.top,
					SWP_NOACTIVATE | SWP_NOZORDER);
		}
		SendMessage(PES->hEWnd, WM_PPXCOMMAND,
				TMAKEWPARAM(KE_excmdopen, ((modify > 1) ? PPE_OPEN_MODE_EXCMDOPENnoSIZE : PPE_OPEN_MODE_EXCMDOPENandSIZE)),
				(LPARAM)title);
	}else{
		if ( text == PPE_TEXT_OPENNEW ){
			if ( 0 == SendMessage(PES->hEWnd, WM_PPXCOMMAND, KE_opennewfile, (LPARAM)title) ){
				SetMessageOnCaption(hPPeWnd, MES_NEWF);
			}
		}else if ( text != NULL ){ // text あり ... 表示
			if ( hPWnd == BADHWND ){
				SendMessage(PES->hEWnd, WM_SETREDRAW, FALSE, 0);
				OpenMainFromMem(GetProp(PES->hEWnd, StrPPxED), PPE_OPEN_MODE_OPEN, NULL, text, TSTRLENGTH32(text), 0);
				SendMessage(PES->hEWnd, WM_SETREDRAW, TRUE, 0);
			}else{
				SetWindowText(PES->hEWnd, text);
			}
			SendMessage(PES->hEWnd, EM_SETMODIFY, FALSE, 0);
		}else{ // text == NULL ... ファイル読み込み
			PPeFileOpen(hPPeWnd, PES->hEWnd, title);
		}

		if ( modify > 1 ){
			SetWindowPos(hPPeWnd, NULL,
					WinPos.pos.left, WinPos.pos.top,
					WinPos.pos.right - WinPos.pos.left,
					WinPos.pos.bottom - WinPos.pos.top,
					SWP_NOZORDER | SWP_NOACTIVATE);
			MoveWindowByKey(hPPeWnd, 0, 0);
		}else{
			FixEditWindowRect(hPPeWnd);
		}
	}

	DragAcceptFiles(hPPeWnd, TRUE);
	ShowWindow(hPPeWnd, SW_SHOWNORMAL);
	if ( hFgWnd != NULL ) SetParent(hPPeWnd, NULL);
	return hPPeWnd;
}
