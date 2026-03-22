/*-----------------------------------------------------------------------------
	Paper Plane xUI	customizer						GUI 関係
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <commctrl.h>
#include <prsht.h>
#include <wincon.h>
#include "PPX.H"
#include "PPCUST.H"
#pragma hdrstop

#if !NODLL
#define DEFINE_tstpcpy
#define DEFINE_tstristr
#define DEFINE_tstrreplace
#define DEFINE_tstplimcpy
#include "tstrings.c"
#endif

#define IDT_PROP_TAB 12320 // プロパティシートのタブ
#define WM_APP_POSCHECK (WM_APP + 401) // プロパティの表示位置調整

const TCHAR StrCustTitle[] = T("PPx Customizer");
#if !NODLL
const TCHAR GetFileExtsStr[] = T("All Files\0*.*\0\0");
#endif
#define PROPPAGE	10	// 全ページ数

BOOL Restore = FALSE;	// カスタマイズ内容をもとに戻す必要があれば真
TCHAR Backup[VFPS];	// カスタマイズのバックアップ名
BOOL X_chidc = TRUE; // コンソールを隠すか
HWND hConsoleWnd;
LCID UseLcid = 0;		// 表示言語
#if NODLL
extern DWORD X_dss;
extern int X_uxt[X_uxt_items];
#else
DWORD X_dss = DSS_NOLOAD;	// 画面自動スケーリング
int X_uxt[X_uxt_items] = {X_uxt_defvalue};
#endif
TCHAR *FirstTypeName = NULL;
TCHAR *FirstItemName = NULL;

TCHAR PPcustRegID[REGIDSIZE] = T(PPCUST_REGID) T(" ");

const TCHAR backup_option[] = T("-nocomment -nosort");

#define DIALOGMARGIN 4
UINT PropMoveTarget[] = {IDOK, IDCANCEL, IDHELP, 12321, 0}; // プロパティシートの位置調整対象

THEME_COLORS WinColors;
HBRUSH hControlBackBrush;
WNDPROC OldPropWndProc = NULL; // Prop 表示位置調整に使う 旧 Prop Window Proc
WNDPROC OldPropDarkDlgProc = NULL; // Dark mode時に使用する 旧 Prop Dialog Proc
WNDPROC OldPropTabWndProc = NULL; // Dark mode時に使用する 旧 Tab Window Proc

#define EPFLAG_MOVEX B0 // 右に移動
#define EPFLAG_WIDE B1 // 幅を拡げる
#define EPFLAG_MOVEY B2 // 下に移動
#define EPFLAG_TALL B3 // 高さを拡げる
#define EPFLAG_MOVEY_PART B4 // 下の方にあるときだけ、下に移動
#define EPFLAG_LIMIT B5 // 最大 5/4
typedef struct {
	UINT id; // 対象コントロールID
	UINT flags; // 調整方法
} ExpandListStruct;

ExpandListStruct ExpandList[] = { // ダイアログ幅拡張時の対象
	{IDS_INFO, EPFLAG_WIDE | EPFLAG_TALL},
	{IDT_GENERAL, EPFLAG_TALL},
	{IDT_GENERALITEM, EPFLAG_WIDE | EPFLAG_TALL},
	{IDE_FIND, EPFLAG_WIDE | EPFLAG_MOVEY},
	{IDB_FIND, EPFLAG_MOVEX | EPFLAG_MOVEY},
	{IDB_TEST, EPFLAG_MOVEX | EPFLAG_MOVEY},
	{IDV_EXTYPE, EPFLAG_TALL},
	{IDV_ALCLIST, EPFLAG_WIDE | EPFLAG_TALL},
	{IDB_MEUP, EPFLAG_MOVEY},
	{IDB_MEDW, EPFLAG_MOVEY},
	{IDB_MEKEY, EPFLAG_MOVEY},
	{IDB_MECKEYS, EPFLAG_MOVEY},
	{IDS_EXITEM, EPFLAG_MOVEY},
	{IDS_EXTYPE, EPFLAG_MOVEY},
	{IDE_EXTYPE, EPFLAG_MOVEY_PART},
	{IDE_EXITEM, EPFLAG_MOVEY},
	{IDS_EXITEML, EPFLAG_MOVEY},
	{IDB_TB_DELITEM, EPFLAG_MOVEX | EPFLAG_MOVEY},
	{IDE_ALCCMT, /*EPFLAG_WIDE |*/ EPFLAG_MOVEY},
	{IDB_ALCCMT, /*EPFLAG_MOVEX | */ EPFLAG_MOVEY},
	{IDB_ALCKEY, EPFLAG_MOVEY},
	{IDC_ALCKEYG, EPFLAG_MOVEY},
	{IDC_ALCKEYS, EPFLAG_WIDE | EPFLAG_MOVEY},
	{IDB_TB_SETITEM, EPFLAG_MOVEX | EPFLAG_MOVEY},
	{IDC_ALCMOUSEB, EPFLAG_MOVEY},
	{IDC_ALCMOUSET, EPFLAG_MOVEY},
	{IDB_ALCMOUSEL, EPFLAG_MOVEY},
	{IDB_ALCMOUSEU, EPFLAG_MOVEY},
	{IDB_ALCMOUSED, EPFLAG_MOVEY},
	{IDB_ALCMOUSER, EPFLAG_MOVEY},
	{IDG_ALCCMD, EPFLAG_WIDE | EPFLAG_MOVEY},
	{IDG_ALCCMD2, EPFLAG_MOVEY},
	{IDE_ALCCMD, EPFLAG_WIDE | EPFLAG_MOVEY},
	{IDB_ALCCMDLIST, EPFLAG_MOVEX | EPFLAG_MOVEY},
	{IDB_ALCCMDI, EPFLAG_MOVEX | EPFLAG_MOVEY},
	{IDE_AOSMASK, EPFLAG_WIDE},
	{IDL_BLIST, EPFLAG_MOVEY},
	{IDB_BREF, EPFLAG_MOVEY},
	{IDB_HMCHARC, EPFLAG_MOVEY},
	{IDB_HMBACKC, EPFLAG_MOVEY},
	{IDL_GCOLOR, EPFLAG_WIDE | EPFLAG_LIMIT},
//	{IDB_GCSEL, EPFLAG_MOVEX},
//	{IDL_GCTYPE, EPFLAG_TALL},
//	{IDL_GCITEM, EPFLAG_TALL}, // 高さが増えすぎる
//	{IDB_GCADD, EPFLAG_MOVEX},
	{IDB_ADDDEFEXT, EPFLAG_MOVEX},
	{IDB_ADDSETEXT, EPFLAG_MOVEX},
	{IDB_ETCEDIT, EPFLAG_MOVEX | EPFLAG_MOVEY},
	{IDL_AOSUSIE, EPFLAG_TALL},
	{IDE_AOSINFO, EPFLAG_WIDE | EPFLAG_MOVEY},
	{IDL_EXITEM, EPFLAG_WIDE | EPFLAG_TALL},
	{IDB_REGPPC, EPFLAG_MOVEX},
	{IDB_REGOPEN, EPFLAG_MOVEX},
	{IDB_UNREGPPC, EPFLAG_MOVEX},
	{IDB_TESTKEY, EPFLAG_MOVEX},
	{IDX_CONSOLE, EPFLAG_MOVEX},
	{0, 0}
};

TCHAR *keynames[] = { T("Space"), T("Page Up"), T("Page Down"), T("End"), T("Home"),
	T("←"), T("↑"), T("→"), T("↓")};
// delete ins space

const TCHAR ImmersiveShellPatg[] = T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell");
const TCHAR TabletMode[] = T("TabletMode");

const TCHAR PropNameInitSheet[] = T("iniSht");

//------------------------------------------------------------------ シート管理
typedef struct {	// 各ページの情報を保存するための構造体
										// あらかじめ初期化する必要があるもの
	int rID;		// リソースID
	DLGPROC proc;	// コールバック関数、NULL なら使わない
} PAGEINFO;

//-------------------------------------
// ※ページの増減があったら、FindKeyword を調整する必要がある。

PAGEINFO PageInfoJpn[PROPPAGE] = {
	{IDD_INFO, FilePage},
	{IDD_GENERAL, GeneralPage}, // ここだけ専用ページ
	{IDD_EXT, ExtPage},
	{IDD_KEYD, KeyPage},
	{IDD_MOUSED, MousePage},
	{IDD_MENUD, MenuPage},
	{IDD_BARD, BarPage},
	{IDD_COLOR, ColorPage},
	{IDD_ADDON, AddonPage},
	{IDD_ETCTREE, EtcPage},
};

PAGEINFO PageInfoEng[PROPPAGE] = {
	{IDD_INFO, FilePage},
	{IDD_GENERALE, GeneralPage},
	{IDD_EXT, ExtPage},
	{IDD_KEYD, KeyPage},
	{IDD_MOUSED, MousePage},
	{IDD_MENUD, MenuPage},
	{IDD_BARD, BarPage},
	{IDD_COLOR, ColorPage},
	{IDD_ADDON, AddonPage},
	{IDD_ETCTREE, EtcPage},
};

void HideConsoleWindow(void)
{
	DWORD TabletModeValue = 0;

	GetRegString(HKEY_CURRENT_USER, ImmersiveShellPatg, TabletMode, (TCHAR *)&TabletModeValue, TSIZEOF(TabletModeValue));
	if ( TabletModeValue != 0 ){
		hConsoleWnd = NULL; // TabletMode時はウィンドウが隠れてしまうので無効に
	}else{
		DefineWinAPI(HWND, GetConsoleWindow, (void));

		GETDLLPROC(GetModuleHandle(T("Kernel32.DLL")), GetConsoleWindow);
		if ( DGetConsoleWindow != NULL ){
			hConsoleWnd = DGetConsoleWindow();
		}else{
			TCHAR temp[32];
			int i;

			thprintf(temp, TSIZEOF(temp), T("+%X)"), GetCurrentThreadId());
			SetConsoleTitle(temp);
			for( i = 0 ; i < 30 ; i++ ){
				if ( (hConsoleWnd = FindWindow(NULL, temp)) != NULL ) break;
				Sleep(100);
			}
		}
	}

	GetCustData(T("X_chidc"), &X_chidc, sizeof(X_chidc));
	if ( IsTrue(X_chidc) && (hConsoleWnd != NULL) ){
		ShowWindow(hConsoleWnd, SW_HIDE);
	}
}

void ShowConsoleWindow(HWND hFocus)
{
	if ( IsTrue(X_chidc) && (hConsoleWnd != NULL) ){
		ShowWindow(hConsoleWnd, SW_SHOW);
		SetForegroundWindow(hFocus);
	}
}

void CustDialog(int Template, DLGPROC dlgprc)
{
	HWND hForeWnd;

	hForeWnd = GetForegroundWindow();
	HideConsoleWindow();
	X_uxt_color = PPxCommonExtCommand(K_UxTheme, KUT_INIT);
	PPxDialogBoxParam(hInst, MAKEINTRESOURCE(Template), hForeWnd, dlgprc, 0);
	ShowConsoleWindow(hForeWnd);
}

void GUILoadCust(void)
{
	GetCustData(T("X_LANG"), &UseLcid, sizeof(UseLcid));
	if ( UseLcid == 0 ) UseLcid = LOWORD(GetUserDefaultLCID());
	WinColors.ExtraDrawFlags = KUTS_COLORS;
	WinColors.c.FrameHighlight = sizeof(WinColors);
	PPxCommonExtCommand(K_UxTheme, (WPARAM)&WinColors);
	hControlBackBrush = (HBRUSH)PPxCommonExtCommand(K_DRAWCCBACK, 0);
}

void Changed(HWND hWnd)
{
	PropSheet_Changed(GetParent(hWnd), hWnd);
	Restore = TRUE;
	GUILoadCust();
}
void Test(void)
{
	PPxPostMessage(WM_PPXCOMMAND, K_Lcust, GetTickCount());
	GUILoadCust();
}
#if !NODLL
void USEFASTCALL SetDlgFocus(HWND hDlg, int id)
{
	SetFocus(GetDlgItem(hDlg, id));
}

void EnableDlgWindow(HWND hDlg, int id, BOOL state)
{
	HWND hControlWnd;

	hControlWnd = GetDlgItem(hDlg, id);
	if ( hControlWnd == NULL ) return;
	EnableWindow(hControlWnd, state);
}
#endif

void EraseBack(HWND hWnd, WPARAM wParam)
{
	RECT box;

	GetClientRect(hWnd, &box);
	FillRect((HDC)wParam, &box, hControlBackBrush);
}

int GetListCursorIndex(WPARAM wParam, LPARAM lParam, int *indexptr)
{
	int index;

	if ( HIWORD(wParam) != LBN_SELCHANGE ) return 0;
	index = (int)SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
	if ( index == LB_ERR ) return 0;
	if ( *indexptr == index ) return -1;
	*indexptr = index;
	return 1;
}

INT_PTR CALLBACK KeyInputDlgBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message){
		case WM_INITDIALOG:
			LocalizeDialogText(hDlg, IDD_KEYINPUT);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
			SetDlgItemText(hDlg, IDP_KEYINPUT_W, (TCHAR *)lParam);
			return TRUE;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDX_KEYINPUT_C:
					SetDlgFocus(hDlg, IDP_KEYINPUT_W);
					break;

				case IDOK:{
					TCHAR *key;

					key = (TCHAR *)GetWindowLongPtr(hDlg, DWLP_USER);
					GetDlgItemText(hDlg, IDP_KEYINPUT_W, key, 32);
					if ( key[0] && (key[tstrlen(key) - 1] == ')') ){
						if ( IsDlgButtonChecked(hDlg, IDX_KEYINPUT_C) ){
							tstrcpy(key, tstrchr(key, '(') + 1);
							key[tstrlen(key) - 1] = '\0';
						}else{
							*tstrchr(key, '(') = '\0';
						}
					}
					EndDialog(hDlg, 1);
					break;
				}

				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;
			}
			break;

		default:
			return PPxDialogHelper(hDlg, message, wParam, lParam);
	}
	return TRUE;
}

int KeyInput(HWND hWnd, TCHAR *string)
{
	return (int)PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_KEYINPUT), hWnd, KeyInputDlgBox, (LPARAM)string);
}

void SetKeyText(HWND hWnd, WORD Vkey, WORD CharKey)
{
	TCHAR CharKeyText[0x100], VKeyText[0x100];
	TCHAR keytext[0x100];

	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)Vkey);
	if ( CharKey == 0 ){
		PutKeyCode(keytext, Vkey);
	}else{
		PutKeyCode(VKeyText, Vkey);
		PutKeyCode(CharKeyText, CharKey);
		thprintf(keytext, TSIZEOF(keytext), T("%s(%s)"), CharKeyText, VKeyText);
	}
	SetWindowText(hWnd, keytext);

	MakeKeyDetailText(Vkey, keytext, FALSE);
	SetDlgItemText(GetParent(hWnd), IDS_EXITEM, keytext);
}

LRESULT CALLBACK KeyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message){
		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS | DLGC_BUTTON;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
			WORD VKey;

			VKey = (WORD)(wParam | GetShiftKey() | K_v);
			SetKeyText(hWnd, VKey, 0);
			break;
		}

		case WM_SYSCHAR:
		case WM_CHAR: {
			WORD CharKey, VKey;

			CharKey = (WORD)wParam;
			if ( CharKey < 0x7f ){
				if ( IslowerA(CharKey) ) CharKey -= (WORD)0x20;	// 小文字
				if ( IsalnumA(CharKey) || (CharKey <= 0x20) ){
					CharKey |= (WORD)GetShiftKey();
				}else{
					CharKey |= (WORD)(GetShiftKey() & ~K_s);
				}
				VKey = (WORD)GetWindowLongPtr(hWnd, GWLP_USERDATA);
																// ctrl + char
				if ( (CharKey & K_c) && ((CharKey & 0xff) < 0x20) ){
					if ( !IsupperA(VKey) ) break; // Enter とか Break とかを無視
					CharKey += (WORD)0x40;
				}
				if ( (CharKey & 0xff) >= ' ' ){
					SetKeyText(hWnd, VKey, CharKey);
				}
			}
			break;
		}
		case WM_ERASEBKGND: {
			if ( X_uxt_color < UXT_MINPRESET ){
				return DefWindowProc(hWnd, message, wParam, lParam);
			}else{
				EraseBack(hWnd, wParam);
				return 1;
			}
		}

		case WM_PAINT: {
			PAINTSTRUCT ps;
			int size;
			TCHAR buf[0x100];
			HGDIOBJ hOldFont;

			BeginPaint(hWnd, &ps);
			size = GetWindowText(hWnd, buf, TSIZEOF(buf));
			if ( size != 0 ){
				hOldFont = SelectObject(ps.hdc,
						(HFONT)SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0));
				SetTextColor(ps.hdc, C_WindowText);
				SetBkColor(ps.hdc, C_WindowBack);
				TextOut(ps.hdc, 4, 1, buf, size);
				SelectObject(ps.hdc, hOldFont);
			}
			EndPaint(hWnd, &ps);
			break;
		}
		case WM_SETTEXT:
			InvalidateRect(hWnd, NULL, TRUE);
//			default へ
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void MakeOneKeyDetailText(int key, TCHAR *dest)
{
	resetflag(key, K_e | K_a | K_c | K_s | K_raw );
	if ( key & K_v ){
		TCHAR *name;

		key &= 0xff;
		switch ( key ){
			case VK_INSERT: // Num Insert になるので置き換え
				name = T("Insert");
				break;
			case VK_DELETE:
				name = T("Delete");
				break;
			case VK_LWIN:
				name = T("LeftWin");
				break;
			case VK_RWIN:
				name = T("RightWin");
				break;
			case VK_APPS:
				name = T("Application");
				break;
			case VK_NUMLOCK:
				name = T("NumLock");
				break;
			default:
				if ( (key >= 0x20) && (key <= 0x28) ){
					name = keynames[key - 0x20];
					break;
				}
				*dest = '\0';
				GetKeyNameText(MapVirtualKey(key, 0) << 16, dest, 64);
				if ( *dest == '\0' ) thprintf(dest, 16, T("V_H%X"), key);
				return;
		}
		tstrcpy(dest, name);
	}else{
		if ( key == ' ' ){
			tstrcpy(dest, T("Space"));
		}else{
			*dest++ = (TCHAR)(key & 0x7f);
			*dest = '\0';
		}
	}
}

void MakeKeyDetailText(int key, TCHAR *dest, BOOL extype)
{
	if ( key & K_e ){
		int X_es = 0x1d;

		if ( extype ){
			GetCustData(T("X_es"), &X_es, sizeof(X_es));
			MakeOneKeyDetailText(X_es, dest);
		}else{
			tstrcpy(dest, T("Win"));
		}
		dest += tstrlen(dest);
		*dest++ = '+';
	}
	if ( key & K_a ) dest = tstpcpy(dest, T("Alt + "));
	if ( key & K_c ) dest = tstpcpy(dest, T("Ctrl + "));
	if ( key & K_s ) dest = tstpcpy(dest, T("Shift + "));
	MakeOneKeyDetailText(key, dest);
}

//-------------------------------------------------------------------
INT_PTR CALLBACK DlgSheetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, UINT id)
{
	switch (message){
		case WM_SETTINGCHANGE:
			if ( (wParam == 0) && (lParam != 0) && (*(char *)lParam == 'I') ){ // "ImmersiveColorSet" の簡易判定
				//※ Dark modeになっているとき、「I」になっていることがあるため
				if ( PPxCommonExtCommand(K_UxTheme, KUT_SETTINGCHANGE) ){
					ChangeCustEnv(hDlg);
				}
			}
			break;

		case WM_CONTEXTMENU:
			if ( (HWND)wParam == hDlg ) break;
		case WM_HELP:
			PPxHelp(hDlg, HELP_CONTEXT, id);
			break;

		case WM_NOTIFY:
			#define NHPTR ((NMHDR *)lParam)
			switch (NHPTR->code){
				case PSN_APPLY:
					if ( IsTrue(Restore) ){
						Restore = FALSE;
						Test();
						Print(T("*backup..."));
						CustDump(backup_option, Backup);
					}
					break;

				case PSN_HELP:
					PPxHelp(NHPTR->hwndFrom, HELP_CONTEXT, id);
					break;

				default:
					return FALSE;
			}
			break;
			#undef NHPTR
		case WM_CLOSE:	// ※PPxDialogHelper では WM_CLOSE で EndDialogするから
			break;

		case WM_ERASEBKGND: {
			if ( X_uxt_color < UXT_MINPRESET ){
				return DefWindowProc(hDlg, message, wParam, lParam);
			}else{
				EraseBack(hDlg, wParam);
				return 1;
			}
		}

		default:	// 特になし
			return PPxDialogHelper(hDlg, message, wParam, lParam);
	}
	return TRUE;
}

void FixPropPos(HWND hPropWnd)
{
	RECT DeskBox, PropBox;
	const TCHAR *ref = T("C");
	BOOL change = FALSE;
	WINPOS WinPos;

	#define FixPropPos_dummy 0xcc
	WinPos.show = FixPropPos_dummy; // 検出用ダミー
	if ( NO_ERROR != GetCustTable(T("_WinPos"), PPcustRegID, &WinPos, sizeof(WinPos)) ){
		SetWindowPos(hPropWnd, NULL, WinPos.pos.left, WinPos.pos.top, 0, 0,
				SWP_NOZORDER | SWP_NOSIZE);
	}
						// 基準デスクトップを決定。PPcがあればそのデスクトップ
	GetDesktopRect(hPropWnd, &DeskBox);
	if ( WinPos.show == FixPropPos_dummy ){
		HWND hParentWnd;

		hParentWnd = GetPPxhWndFromID(NULL, &ref, NULL);
		if ( (hParentWnd != NULL) && (hParentWnd != BADHWND) && IsWindow(hParentWnd) ){
			GetDesktopRect(hParentWnd, &DeskBox);
		}
	}

	GetWindowRect(hPropWnd, &PropBox);

	// プロパティシートを画面内に移動
	if ( PropBox.right > DeskBox.right ){
		PropBox.left -= PropBox.right - DeskBox.right;
		change = TRUE;
	}
	if ( PropBox.left < DeskBox.left ){
		PropBox.left = DeskBox.left;
		change = TRUE;
	}

	if ( PropBox.bottom > DeskBox.bottom ){
		PropBox.top -= PropBox.bottom - DeskBox.bottom;
		change = TRUE;
	}
	if ( PropBox.top < DeskBox.top ){
		PropBox.top = DeskBox.top;
		change = TRUE;
	}

	if ( change ){
		SetWindowPos(hPropWnd, NULL, PropBox.left, PropBox.top, 0, 0,
				SWP_NOZORDER | SWP_NOSIZE);
	}
}

BOOL FixControlSize(HWND hWnd, SIZE *lsize, RECT *basebox)
{
	RECT box;

	if ( hWnd == NULL ) return FALSE;
	GetWindowRect(hWnd, &box);
	if ( (basebox == NULL) || (box.right < basebox->right) || (box.bottom < basebox->bottom) ){
		SetWindowPos(hWnd, NULL, 0, 0,
				box.right - box.left + lsize->cx,
				box.bottom - box.top + lsize->cy,
				SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);
		return TRUE;
	}
	return FALSE;
}

void InitSheetControls(HWND hDlg, UINT baseControlID)
{
	HWND hBaseWnd, hPropWnd;
	RECT DialogBox, DeskBox, BaseBox, TempBox;
	POINT pos;
	SIZE lsize = { 0, 0 };
	UINT *ctrlID;
	int ExtendWidth, ExtendHeight;

	hPropWnd = GetParent(hDlg);

	// 右下のコントロールが表示されないときは、表示されるように大きさ調整する
	if ( baseControlID == 0 ) return;
	hBaseWnd = GetDlgItem(hDlg, baseControlID);
	if ( hBaseWnd == NULL ) return;

	GetWindowRect(hDlg, &DialogBox);
	GetWindowRect(hBaseWnd, &BaseBox);
	GetDesktopRect(hPropWnd, &DeskBox);

	if ( (OSver.dwMajorVersion >= 5) &&
		 (GetProp(hBaseWnd, PropNameInitSheet) == NULL) ){
		int DialogAWidth, DialogMaxWidth, DialogExtendWidth;
		int DialogAHeight, DialogMaxHeight, DialogExtendHeight;

		SetProp(hBaseWnd, PropNameInitSheet, INVALID_HANDLE_VALUE);

		// 大まかなダイアログサイズを算出
		DialogAWidth = BaseBox.right - DialogBox.left;
		DialogAWidth += DialogAWidth / 10; // ダイアログの大きさを少し小さく
		DialogAHeight = BaseBox.bottom - DialogBox.top;
		DialogAHeight += DialogAHeight / 10; // ダイアログの大きさを少し小さく
		// 大まかなダイアログボックス幅 x 1.3 <= デスクトップ幅まで拡張
		DialogMaxWidth = DialogAWidth + DialogAWidth / 3;
		DialogExtendWidth = DeskBox.right - DeskBox.left;
		if ( DialogAWidth > DialogExtendWidth ){
			DialogExtendWidth = DialogAWidth;
		}
		// 大まかなダイアログボックス高 x 1.2 <= デスクトップ高まで拡張
		DialogMaxHeight = DialogAHeight + DialogAHeight / 5;
		DialogExtendHeight = DeskBox.bottom - DeskBox.top;
		if ( DialogAHeight > DialogExtendHeight ){
			DialogExtendHeight = DialogAHeight;
		}
		if ( (DialogAWidth < DialogExtendWidth) ||
			 (DialogAHeight < DialogExtendHeight) ){
			ExpandListStruct *exl;

			if ( DialogExtendWidth >= DialogMaxWidth ){
				DialogExtendWidth = DialogMaxWidth;
			}
			ExtendWidth = DialogExtendWidth - DialogAWidth;
			BaseBox.right += ExtendWidth;
			if ( DialogExtendHeight >= DialogMaxHeight ){
				DialogExtendHeight = DialogMaxHeight;
			}
			ExtendHeight = DialogExtendHeight - DialogAHeight;
			BaseBox.bottom += ExtendHeight;
			ExtendHeight -= 4;
			if ( ExtendHeight < 0 ) ExtendHeight = 0;

			for ( exl = ExpandList ; exl->id != 0 ; exl++ ){
				HWND hWnd;

				hWnd = GetDlgItem(hDlg, exl->id);
				if ( hWnd == NULL ) continue;

				GetWindowRect(hWnd, &TempBox);
				pos.x = TempBox.left - DialogBox.left;
				pos.y = TempBox.top - DialogBox.top;
				if ( exl->flags & EPFLAG_MOVEX ) pos.x += ExtendWidth;
				if ( exl->flags & EPFLAG_WIDE ){
					if ( exl->flags & EPFLAG_LIMIT ){
						int wid;
						wid = (TempBox.right - TempBox.left) / 4;
						TempBox.right += (wid < ExtendWidth) ? wid : ExtendWidth;
						PostMessage(hDlg, WM_COMMAND, TMAKEWPARAM(IDL_GCOLOR, IDL_GCOLOR), 0);
					}else{
						TempBox.right += ExtendWidth;
					}
				}
				if ( exl->flags & EPFLAG_MOVEY ) pos.y += ExtendHeight;
				if ( exl->flags & EPFLAG_TALL ) TempBox.bottom += ExtendHeight;
				if ( exl->flags & EPFLAG_MOVEY_PART ){ // マウス割当て以外のみ
					if ( pos.y > (DialogAHeight / 5) ){
						pos.y += ExtendHeight;
					}
				}
				SetWindowPos(hWnd, NULL, pos.x, pos.y,
						TempBox.right - TempBox.left,
						TempBox.bottom - TempBox.top,
						SWP_NOACTIVATE | SWP_NOZORDER);
			}
		}
	}
	if ( (DialogBox.right >= BaseBox.right) &&
		 (DialogBox.bottom >= BaseBox.bottom) ){
		return; // 大きさ調整の必要なし
	}
	// 足りない大きさを算出
	if ( DialogBox.right < BaseBox.right ){
		lsize.cx = BaseBox.right + DIALOGMARGIN - DialogBox.right;
		DialogBox.right += lsize.cx;
	}
	if ( DialogBox.bottom < BaseBox.bottom ){
		lsize.cy = BaseBox.bottom + DIALOGMARGIN - DialogBox.bottom;
		DialogBox.bottom += lsize.cy;
	}
	FixControlSize(hDlg, &lsize, &DialogBox); // プロパティシートの大きさ修正
	FixControlSize(GetDlgItem(hPropWnd, IDT_PROP_TAB), &lsize, &DialogBox); // tab control
	FixControlSize(hPropWnd, &lsize, NULL);	// 親ウィンドウの大きさ修正

	GetWindowRect(hPropWnd, &TempBox); // プロパティシートを画面内に移動
	if ( TempBox.right > DeskBox.right ){
		TempBox.left -= TempBox.right - DeskBox.right;
		if ( TempBox.left < DeskBox.left ) TempBox.left = DeskBox.left;
		SetWindowPos(hPropWnd, NULL, TempBox.left, TempBox.top,
				0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}
	if ( TempBox.bottom > DeskBox.bottom ){
		TempBox.top -= TempBox.bottom - DeskBox.bottom;
		if ( TempBox.top < DeskBox.top ) TempBox.top = DeskBox.top;
		SetWindowPos(hPropWnd, NULL, TempBox.left, TempBox.top,
				0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}

	// 各種ボタンの位置修正
	for ( ctrlID = PropMoveTarget ; *ctrlID != 0 ; ctrlID++ ){
		HWND hWnd;

		hWnd = GetDlgItem(hPropWnd, *ctrlID);
		if ( hWnd == NULL ) continue;
		GetWindowRect(hWnd, &TempBox);
		pos.x = TempBox.left;
		pos.y = TempBox.top;
		ScreenToClient(hPropWnd, &pos);
		SetWindowPos(hWnd, NULL, pos.x + lsize.cx, pos.y + lsize.cy,
				0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
	}

	InvalidateRect(hPropWnd, NULL, TRUE);
}

LRESULT CALLBACK ProcWndHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
/*
	if ( uMsg == WM_DESTROY ){

	WINPOS wpos = {{0, 0, 0, 0}, 0, 0};

	GetWindowRect(hWnd, &wpos.pos);
	SetCustTable(T("_WinPos"), PPcustRegID, &wpos, sizeof(wpos));

		SavePropPos(hWnd);
		break;
	}
*/
	if ( uMsg == WM_APP_POSCHECK ){
		FixPropPos(hWnd);
		InitSheetControls((HWND)lParam, wParam);
		// ProcWndHookProc は用済みなので元に戻す
		// ※戻しても、しばらくは ProcWndHookProc が呼び出されるので注意
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)OldPropWndProc);
		return 1;
	}
	return CallWindowProc(OldPropWndProc, hWnd, uMsg, wParam, lParam);
}

void SetTabTitles(HWND hPropWnd)
{
	HWND hTabWnd;
	int i;
	TC_ITEM tabitem;

	hTabWnd = GetDlgItem(hPropWnd, IDT_PROP_TAB);
	if ( hTabWnd == NULL ) return;

	for ( i = 0 ; i < PROPPAGE ; i++ ){
		TCHAR id[256];

		tabitem.mask = TCIF_TEXT;
		thprintf(id, TSIZEOF(id), T("%4X|"), PageInfoEng[i].rID);
		tabitem.pszText = (TCHAR *)MessageText(id);
		if ( tabitem.pszText[0] != '\0' ){
			TabCtrl_SetItem(hTabWnd, i, &tabitem);
		}
	}
}

#pragma argsused
int CALLBACK PropSheetCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam)
{
	UnUsedParam(lParam);

	if ( uMsg == PSCB_INITIALIZED ){
		SetTabTitles(hWnd);
		OldPropWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)ProcWndHookProc);
	}
	return 0;
}

INT_PTR CALLBACK DefSheetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DlgSheetProc(hDlg, message, wParam, lParam, 0);
}

TCHAR PPcustMainThreadName[] = T("PPcust");

void GUIcustomizer(int startpage, const TCHAR *param)
{
	PROPSHEETHEADER head;
	PROPSHEETPAGE page[PROPPAGE];
	PAGEINFO *pinfo = PageInfoEng;
	TCHAR temp[VFPS];
	TCHAR firstitem[CMDLINESIZE];
	int i;
	WNDCLASS wcClass;
	THREADSTRUCT threadstruct = {PPcustMainThreadName, XTHREAD_ROOT, NULL, 0, 0};

	PPxRegisterThread(&threadstruct);
	OSver.dwOSVersionInfoSize = sizeof(OSver);
	GetVersionEx(&OSver);
	SetErrorMode(SEM_FAILCRITICALERRORS); // 致命的エラーを取得可能にする
	(void)CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	LoadCommonControls(ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES);

	hInst = GetModuleHandle(NULL);
	X_uxt_color = PPxCommonExtCommand(K_UxTheme, KUT_INIT);
	PPxCommonExtCommand(K_SETAPPID, 'c');

	GUILoadCust();
	if ( UseLcid == LCID_PPXDEF ) pinfo = PageInfoJpn;

#ifndef WINEGCC				// コンソールウィンドウが開いているか確認する
	HideConsoleWindow();

#else
	X_chidc = FALSE;
	hConsoleWnd = NULL;
#endif
	SetConsoleTitle(T("PPcust log"));

	if ( OSver.dwMajorVersion >= 6 ){
		GetCustData(T("X_dss"), &X_dss, sizeof(X_dss));
	}

	if ( (param != NULL) && (*param >= ' ') ){
		GetLineParamS(&param, firstitem, TSIZEOF(firstitem));
		FirstTypeName = firstitem;
		if ( *FirstTypeName == 'X' ){ // general
			startpage = 1;
		}else{ // menu
			FirstItemName = tstrchr(firstitem, ':');
			if ( FirstItemName != NULL ) *FirstItemName++ = '\0';
			startpage = 5;
		}
	}
	if ( startpage >= 0 ){
		wcClass.style			= 0;
		wcClass.lpfnWndProc		= KeyProc;
		wcClass.cbClsExtra		= 0;
		wcClass.cbWndExtra		= 0;
		wcClass.hInstance		= hInst;
		wcClass.hIcon			= LoadIcon(hInst, MAKEINTRESOURCE(IC_CUST));
		wcClass.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wcClass.hbrBackground	= WNDCLASSBRUSH(COLOR_WINDOW + 1);
		wcClass.lpszMenuName	= NULL;
		wcClass.lpszClassName	= T(PPXKEYCLASS);
											// クラスを登録する
		RegisterClass(&wcClass);
												// 初期設定
		head.dwSize		= sizeof(PROPSHEETHEADER);
		head.dwFlags	= PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_HASHELP | PSH_USECALLBACK;
		head.hwndParent	= NULL;
		head.hInstance	= hInst;
		UNIONNAME(head, pszIcon) = MAKEINTRESOURCE(IC_CUST);
		head.pszCaption	= StrCustTitle;
		head.nPages		= PROPPAGE;
		UNIONNAME2(head, nStartPage) = startpage;
		UNIONNAME3(head, ppsp) = page;
		head.pfnCallback = PropSheetCallbackProc;

		for ( i = 0 ; i < PROPPAGE ; i++, pinfo++ ){
			page[i].dwSize		= sizeof(PROPSHEETPAGE);
			page[i].hInstance	= hInst;
			page[i].dwFlags	= PSP_HASHELP | PSP_DLGINDIRECT;
			UNIONNAME(page[i], pResource) = GetDialogTemplate(NULL, hInst, MAKEINTRESOURCE(pinfo->rID));
			page[i].pfnDlgProc	= pinfo->proc ? pinfo->proc : DefSheetProc;
			page[i].lParam		= (LPARAM)i;
		}
		MakeTempEntry(VFPS, Backup, FILE_ATTRIBUTE_NORMAL);
		thprintf(temp, TSIZEOF(temp), T("*Customize backup file:%s\r\n"), Backup);
		Print(temp);
		Print(T("*backup..."));
		CustDump(backup_option, Backup);
//		PPxHookEdit(1);	// 入れると異常動作するため保留
		PropertySheet(&head);
//		PPxHookEdit(-1);
//		page[i].pResource の HeapFree を省略
	}else{
		PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DISPFOMAT),
				PPcGetWindow(0, CGETW_GETFOCUS),
				DispFormatDialogBox, (LPARAM)NULL);
	}

	if ( startpage >= 0 ){
		ShowConsoleWindow(PPcGetWindow(0, CGETW_GETFOCUS));
		if ( Restore ){
			Print(T("*Customize restoring...") TNL);
			PPxCommonExtCommand(K_CLEARCUSTOMIZE, ~K_CLEARCUSTOMIZE);
			CustStore(Backup, PPXCUSTMODE_temp_RESTORE, NULL);
		}
		DeleteFile(Backup);
		Print(T("*Deleted customize backup file."));
	}else{
		ShowConsoleWindow(hConsoleWnd);
	}

	if ( PPxGetHWND(PPcustRegID) != NULL ){ // この中は１回だけ呼び出される
		PPxRegist(NULL, PPcustRegID, PPXREGIST_FREE);
	}
	PPxCommonCommand(NULL, 0, K_CLEANUP);
	PPxUnRegisterThread();
	CoUninitialize();
}
#if !NODLL
void ShowDlgWindow(const HWND hDlg, const UINT id, BOOL show)
{
	ShowWindow(GetDlgItem(hDlg, id), show ? SW_SHOW : SW_HIDE);
}
#endif

TCHAR *LoadTextResourceData(HINSTANCE hModule, LPCTSTR rname)
{
	TCHAR *mem;
	HRSRC hres;
	DWORD size;

	hres = FindResource(hModule, rname, RT_RCDATA);
	if ( hres == NULL ) return NULL;
	#ifdef UNICODE
	{
		char *rc;
		DWORD rsize;
		UINT cp;

		rsize = SizeofResource(hModule, hres);
		rc = LockResource(LoadResource(hModule, hres));
		cp = IsValidCodePage(CP__SJIS) ? CP__SJIS : CP_ACP;
		size = MultiByteToWideChar(cp, 0, rc, rsize, NULL, 0);
		mem = HeapAlloc(GetProcessHeap(), 0, TSTROFF(size) + 16);
		if ( mem == NULL ) return NULL;
		size = MultiByteToWideChar(cp, 0, rc, rsize, mem, size);
		mem[size] = '\0';
	}
	#else
		size = SizeofResource(hModule, hres);
		if ( (GetACP() == CP_UTF8) && IsValidCodePage(CP__SJIS) ){
			char *rc;
			DWORD wsize;
			WCHAR *tmpptr;

			rc = LockResource(LoadResource(hModule, hres));
			wsize = MultiByteToWideChar(CP__SJIS, 0, rc, size, NULL, 0);
			tmpptr = HeapAlloc(GetProcessHeap(), 0, (wsize * sizeof(WCHAR)) + 16);
			if ( tmpptr == NULL ) return NULL;
			wsize = MultiByteToWideChar(CP__SJIS, 0, rc, size, tmpptr, wsize);
			size = WideCharToMultiByte(CP_UTF8, 0, tmpptr, wsize, NULL, 0, NULL, NULL);
			mem = HeapAlloc(GetProcessHeap(), 0, size + 16);
			if ( mem == NULL ) return NULL; // tmpptr がリークするが無視
			size = WideCharToMultiByte(CP_UTF8, 0, tmpptr, wsize, mem, size, NULL, NULL);
		}else{
			mem = HeapAlloc(GetProcessHeap(), 0, size + 16);
			if ( mem == NULL ) return NULL;
			memcpy(mem, LockResource(LoadResource(hModule, hres)), size);
		}
		mem[size] = '\0';
	#endif
	return mem;
}

TCHAR *LoadTextResource(HINSTANCE hModule, LPCTSTR rname)
{
	TCHAR *ptr, *text, c;

	text = LoadTextResourceData(hModule, rname);
	if ( text == NULL ) return NULL;

	ptr = text;
	for ( ;; ){
		c = *ptr;
		if ( (c == '\0') || (c == '\r') || (c == '\n') ) break;
		ptr++;
	}
	if ( c != '\0' ){
		if ( (c == '\r') && (*(ptr + 1) == '\n') ){ // cr lf → 1文字削る
			TCHAR *dst;

			dst = ptr;
			for ( ;; ){
				c = *ptr++;
				if ( c == '\r' ){
					ptr++;
					if ( *ptr == '\n' ) ptr++;
					*dst++ = '\0';
					continue;
				}
				*dst++ = c;
				if ( c == '\0' ) break;
			}
		}else{ // cr / lf
			for ( ;; ){
				ptr = tstrchr(ptr, c);
				if ( ptr == NULL ) break;
				*ptr++ = '\0';
			}
		}
	}
	return text;
}

void InitWndIcon(HWND hDlg, UINT baseControlID)
{
	if ( PPcustRegID[2] == ' ' ){ // ウィンドウのアイコンを設定する
		HWND hPropWnd = GetParent(hDlg);

		PPcustRegID[2] = 'A';
		PPxRegist(hPropWnd, PPcustRegID, PPXREGIST_NORMAL);

		SetClassLongPtr(hPropWnd, GCLP_HICON,
				(LONG_PTR)LoadIcon(hInst, MAKEINTRESOURCE(IC_CUST)));
		PostMessage(hPropWnd, WM_APP_POSCHECK, (WPARAM)baseControlID, (LPARAM)hDlg); // ダイアログを移動
	}else{
		InitSheetControls(hDlg, baseControlID);
	}
}

#ifndef UNICODE
const WCHAR *GetBannerText(const WCHAR *text)
{
	const WCHAR *newtext;

	if ( UseLcid == LCID_PPXDEF ) return text;
	newtext = text + strlenW(text) + 1;
	return (*newtext != '\0') ? newtext : text;
}
#endif

void GetControlText(HWND hDlg, UINT ID, TCHAR *text, DWORD len)
{
	TCHAR *texttop, *textlast;
	size_t toplen;

	text[0] = '\0';
	GetDlgItemText(hDlg, ID, text, len);
	texttop = text;
	for (;;){
		UTCHAR chr;

		chr = (UTCHAR)*texttop;
		if ( (chr == '\0') || (chr > ' ') ) break;
		texttop++;
	}
	toplen = tstrlen(texttop);
	if ( texttop > text ) memmove(text, texttop, (toplen + 1) * sizeof(TCHAR));
	textlast = text + toplen;
	while ( textlast > text ){
		if ( (UTCHAR)*(textlast - 1) > ' ' ) break;
		textlast--;
		*textlast = '\0';
	}
}

#if NODLL
extern void FillBox(HDC hDC, const RECT *box, HBRUSH hbr);
#else
void FillBox(HDC hDC, const RECT *box, HBRUSH hbr)
{
	HGDIOBJ hOldObj;

	hOldObj = SelectObject(hDC, hbr);
	PatBlt(hDC, box->left, box->top,
			box->right - box->left, box->bottom - box->top,
			PATCOPY);
	SelectObject(hDC, hOldObj);
}
#endif

// タブは左から右に順にUnselectのを描画し、最後にSelectedを描画する
void DrawTab(DRAWITEMSTRUCT *dis)
{
	TC_ITEM tie;
	TCHAR buf[CMDLINESIZE];
	RECT box;
	COLORREF oldfc, oldbc;
	HBRUSH hBack;
	COLORREF col;

	tie.mask = TCIF_TEXT;
	tie.pszText = buf;
	tie.cchTextMax = CMDLINESIZE;
	if ( TabCtrl_GetItem(dis->hwndItem, dis->itemID, &tie) == FALSE ) return;

	oldfc = SetTextColor(dis->hDC, C_DialogText);
	col = (dis->itemState & ODS_SELECTED) ? C_FocusBack : C_DialogBack;
	hBack = CreateSolidBrush(col);
	FillBox(dis->hDC, &dis->rcItem, hBack);
	DeleteObject(hBack);
	oldbc = SetBkColor(dis->hDC, col);

	box = dis->rcItem; // TCM_GETITEMRECT で得られるRECTより左右上下2pixずつ小さい
	box.left += 6;
	box.top += 2;
	#pragma warning(suppress: 6054) // TabCtrl_GetItem で取得
	DrawText(dis->hDC, buf, tstrlen32(buf), &box, DT_LEFT | DT_NOPREFIX);
	SetTextColor(dis->hDC, oldfc);
	SetBkColor(dis->hDC, oldbc);
}

void PaintTab(HWND hWnd)
{
	PAINTSTRUCT ps;
	DRAWITEMSTRUCT dis;
	DWORD maxindex;
	UINT cursor;
	HFONT hOldFont;
	HBRUSH hFrameBrush;

	BeginPaint(hWnd, &ps);

	dis.hwndItem = hWnd;
	dis.hDC = ps.hdc;

	hOldFont = SelectObject(ps.hdc, (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0));
	hFrameBrush = CreateSolidBrush(C_EdgeLine);
	SetBkColor(ps.hdc, C_DialogBack);
	cursor = TabCtrl_GetCurSel(hWnd);
	maxindex = TabCtrl_GetItemCount(hWnd);
	for ( dis.itemID = 0 ; dis.itemID < maxindex ; dis.itemID++ ){
		TabCtrl_GetItemRect(hWnd, dis.itemID, &dis.rcItem);
		FrameRect(ps.hdc, &dis.rcItem, hFrameBrush);
		dis.rcItem.left++;
		dis.rcItem.right--;
		if ( dis.itemID == cursor ){
			dis.itemState = ODS_SELECTED;
		}else{
			dis.rcItem.top++;
			dis.rcItem.bottom--;
			dis.itemState = 0;
		}
		DrawTab(&dis);
	}
	DeleteObject(hFrameBrush);
	SelectObject(ps.hdc, hOldFont);
	EndPaint(hWnd, &ps);
}

// Propperty sheet の Tab control の拡張用 (タブの背景表示)
LRESULT CALLBACK PropTabHookProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if ( (X_uxt_color >= UXT_MINMODIFY) && (hControlBackBrush != NULL) ){
		if ( message == WM_ERASEBKGND ){
			EraseBack(hWnd, wParam);
			return 1;
		}else if ( message == WM_PAINT ){
			PaintTab(hWnd);
			return 0;
		}
	}
	return CallWindowProc(OldPropTabWndProc, hWnd, message, wParam, lParam);
}

// Propperty sheet の Dialog proc の拡張用 (ダイアログ背景と、タブのアイテム表示)
LRESULT CALLBACK PropDarkProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch ( message ){
		case WM_CTLCOLORDLG:
			return PPxDialogHelper(hWnd, message, wParam, lParam);

		case WM_DRAWITEM:
			if ( wParam == IDT_PROP_TAB ) DrawTab((DRAWITEMSTRUCT *)lParam);
			return 1;
	}
	return CallWindowProc(OldPropDarkDlgProc, hWnd, message, wParam, lParam);
}

void InitPropSheetsUxtheme(HWND hDlg)
{
	HWND hWnd, hTabWnd;

	hWnd = hDlg;
	if ( X_uxt_color >= UXT_MINMODIFY ){
		if ( OldPropDarkDlgProc == NULL ){
			hWnd = GetParent(hDlg);
			hTabWnd = GetDlgItem(hWnd, IDT_PROP_TAB);

			OldPropDarkDlgProc = (WNDPROC)SetWindowLongPtr(hWnd, DWLP_DLGPROC, (LONG_PTR)PropDarkProc);
			if ( hTabWnd != NULL ){
				OldPropTabWndProc = (WNDPROC)SetWindowLongPtr(hTabWnd, GWLP_WNDPROC, (LONG_PTR)PropTabHookProc);
				SetWindowLongPtr(hTabWnd, GWL_STYLE, GetWindowLongPtr(hTabWnd, GWL_STYLE) | TCS_OWNERDRAWFIXED);
			}
		}
		FixUxTheme(hWnd, NilStr);
	}
	LocalizeDialogText(hWnd, 0);
}

#if !NODLL
void RemoveControlKeydown(HWND hWnd)
{
	MSG WndMsg;

	if ( IsTrue(PeekMessage(&WndMsg, hWnd, WM_CHAR, WM_CHAR, PM_NOREMOVE)) ){
		if ( WndMsg.wParam <= ' ' ){
			PeekMessage(&WndMsg, hWnd, WM_CHAR, WM_CHAR, PM_REMOVE);
		}
	}
}
#endif

int CustTrackPopupMenu(HWND hDlg, HMENU hMenu, POINT *pos)
{
	RemoveControlKeydown(hDlg);
	return TrackPopupMenu(hMenu, TPM_TDEFAULT,
			pos->x, pos->y, 0, hDlg, NULL);
}

int ButtonTrackPopupMenu(HWND hDlg, HMENU hMenu, HWND hTargetButton)
{
	RECT box;

	RemoveControlKeydown(hDlg);
	GetWindowRect(hTargetButton, &box);
	return TrackPopupMenu(hMenu, TPM_TDEFAULT,
			box.left, box.bottom, 0, hDlg, NULL);
}

// \r\n → \n に変換 & 末尾改行の除去　編集コマンドラインを保存する前に使用
void FixCutReturnCode(TCHAR *text)
{
	TCHAR *src, *dest;

	src = tstrchr(text, '\r');
	if ( src == NULL ){
		dest = text + tstrlen(text);
	}else{ // \r を除去
		dest = src;
		src++;
		while ( *src ){
			if ( *src != '\r' ) *dest++ = *src;
			src++;
		}
	}
	while ( (dest > text) && (*(dest - 1) == '\n') ) dest--; //末尾の空行を削除
	*dest = '\0';
}

