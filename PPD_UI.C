/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library			UI 関係
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "PPD_DEF.H"
#pragma hdrstop

HMODULE hWinmm = NULL;
HMODULE hShcore = NULL;

DefineWinAPI(HRESULT, DwmGetWindowAttribute, (HWND, DWORD, PVOID, DWORD)) = NULL;

typedef enum xMONITOR_DPI_TYPE {
	xMDT_EFFECTIVE_DPI = 0,
	xMDT_ANGULAR_DPI = 1,
	xMDT_RAW_DPI = 2,
	xMDT_DEFAULT = xMDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

DefineWinAPI(HMONITOR, MonitorFromWindow, (HWND hwnd, DWORD dwFlags)) = NULL;
DefineWinAPI(BOOL, GetMonitorInfoA, (HMONITOR hMonitor, LPMONITORINFO lpmi));
DefineWinAPI(BOOL, GetDpiForMonitor, (HMONITOR hMonitor, enum xMONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY)) = NULL;
DefineWinAPI(BOOL, PlaySound, (LPCTSTR, HMODULE, DWORD));
DefineWinAPI(BOOL, FlashWindowEx, (PFLASHWINFO pfwi));
DefineWinAPI(BOOL, FlashWindow, (HWND hWnd, BOOL invert)) = NULL;

LOADWINAPISTRUCT MONITORDLL[] = {
	LOADWINAPI1(MonitorFromWindow),
	LOADWINAPI1(GetMonitorInfoA),
	{NULL, NULL}
};

struct { // ボタンメニューの誤表示抑制用の状態記憶
	DWORD tick; // メニュー表示完了後時間経過確認
	UINT ButtonID; // 使用したボタンのID
} ButtonMenuState = {0, 0};

HANDLE hConsoleStdin;
ValueWinAPI(GetConsoleMode);
ValueWinAPI(SetConsoleMode);
ValueWinAPI(ReadConsole);
ValueWinAPI(ReadConsoleInput);
ValueWinAPI(PeekConsoleInput);
ValueWinAPI(SetConsoleCtrlHandler);
LOADWINAPISTRUCT ConsoleAPI[] = {
	LOADWINAPI1(GetConsoleMode),
	LOADWINAPI1(SetConsoleMode),
	LOADWINAPI1T(ReadConsole),
	LOADWINAPI1T(ReadConsoleInput),
	LOADWINAPI1T(PeekConsoleInput),
	LOADWINAPI1(SetConsoleCtrlHandler),
	{NULL, NULL}
};

#define BeepType_Max 9
const UINT BeepType[BeepType_Max] = {
	MB_ICONHAND,		// 致命的エラー表示
	MB_ICONHAND,		// 一般エラー表示
	MB_OK,				// 重要でないエラー表示
	MB_ICONEXCLAMATION,	// 重要な警告

	MB_ICONEXCLAMATION,	// 重要でない警告
	MB_ICONASTERISK,	// 結果を表示する情報
	MB_ICONASTERISK,	// 好意で出力する情報/作業完了
	MB_OK,				// その他
	0xFFFFFFFF			// 範囲外
};

const TCHAR *MessageType[] = {
// XM_FaERRld～
	T("Err! Dlg"), T("Err  Dlg"), T("Err? Dlg"), T("Warn!Dlg"), T("Warn Dlg"),
	T("Result D"), T("Info Dlg"), T("Etc  Dlg"),
// XM_FaERRl～
	T("Err! Mes"), T("Err  Mes"), T("Err? Mes"), T("Warn!Mes"), T("Warn Mes"),
	T("Info"), T("Info"), T("Etc"),
// unuse, XM_FaERRd～
	T("Unknown0"), T("Fault by"), T("Error by"), T("Unknown1"), T("Unknown2"),
	T("Unknown3"), T("MsgCon  "), T("MsgConWa"),
// XM_DbgDIA～
	T("DebugDlg"), T("DebugSnd"), T("DebugLog"), T("Dump Dlg"), T("Dump Log"),
	T("DebugCon"), T("DebugCoW"), T("Unknown8")};

#define GetMsgBoxDefButton(style) ((style >> 8) & 3)

typedef struct {
	const TCHAR *text;
	UINT ID;
} MESSAGEITEMS;

const MESSAGEITEMS MessageYesNo[] = {
	{MES_IMYE, IDYES},
	{MES_IMNO, IDNO},
	{NULL, 0}
};

const MESSAGEITEMS MessageYesNoCancel[] = {
	{MES_IMYE, IDYES},
	{MES_IMNO, IDNO},
	{MES_IMTC, IDCANCEL},
	{NULL, 0}
};

const MESSAGEITEMS MessageOkCancel[] = {
	{MES_IMYK, IDOK},
	{MES_IMCA, IDCANCEL},
	{NULL, 0}
};

const MESSAGEITEMS MessageOk[] = {
	{MES_IMOK, IDOK},
	{NULL, 0}
};

const MESSAGEITEMS MessageAbortRetryIgnore[] = {
	{MES_IMAB, IDABORT},
	{MES_IMRE, IDRETRY},
	{MES_IMIG, IDIGNORE},
	{NULL, 0}
};

const MESSAGEITEMS MessageRetryCancel[] = {
	{MES_IMRE, IDRETRY},
	{MES_IMTC, IDCANCEL},
	{NULL, 0}
};

const MESSAGEITEMS MessageAddAbortRetryIgnore[] = {
	{MES_7063, IDB_QDADDLIST},
	{MES_IMAB, IDABORT},
	{MES_IMRE, IDRETRY},
	{MES_IMIG, IDIGNORE},
	{NULL, 0}
};

const MESSAGEITEMS MessageStartCancel[] = {
	{MES_IMST, IDYES},
	{MES_IMCA, IDCANCEL},
	{NULL, 0}
};

const MESSAGEITEMS MessageYesNoIgnoreCancel[] = {
	{MES_IMYE, IDYES},
	{MES_IMNO, IDNO},
	{MES_IMSK, IDIGNORE},
	{MES_IMTC, IDCANCEL},
	{NULL, 0}
};

const MESSAGEITEMS MessageRetryCancel_NoTrans[] = { // UsePPx で使うため、多言語化処理は無し
	{T("&Retry"), IDRETRY},
	{T("&Cancel"), IDCANCEL},
	{NULL, 0}
};

const MESSAGEITEMS MessageAbortRetryRenameIgnore[] = {
	{MES_IMAB, IDABORT},
	{MES_IMRE, IDRETRY},
	{MES_7068, IDX_FOP_ADDNUMDEST},
	{MES_IMIG, IDIGNORE},
	{NULL, 0}
};

const MESSAGEITEMS MessageAbortOption[] = {
	{MES_IMAB, IDABORT},
	{MES_7009, IDB_DIALOG_OPTION},
	{NULL, 0}
};

const MESSAGEITEMS *MessageTypes[] = {
	MessageOk, MessageOkCancel, MessageAbortRetryIgnore,
	MessageYesNoCancel, MessageYesNo, MessageRetryCancel,
	MessageAbortRetryIgnore/* CancelTryContinue */, //ここまでがWindows標準定義
	MessageAddAbortRetryIgnore,	//  7, MB_PPX_ADDABORTRETRYIGNORE
	MessageStartCancel,			//  8, MB_PPX_STARTCANCEL
	MessageYesNoIgnoreCancel,	//  9, MB_PPX_YESNOIGNORECANCEL
	MessageRetryCancel_NoTrans,	// 10, MB_PPX_USEPPXCHECKOKCANCEL
	MessageAbortRetryRenameIgnore,	// 11, MB_PPX_ABORTRETRYRENAMEIGNORE
	MessageAbortOption			// 12, MB_PPX_ABORTOPTION
};

const TCHAR MessageAllReactionStr[] = MES_7062;
const TCHAR CriticalTitle[] = T("Paper Plane xUI");

HMODULE hIEframe = NULL;
const TCHAR VistaBitmapBar[] = T("IDB_TB_SH_DEF_16");

PPXDLL void PPXAPI XBeep(UINT type)
{
	if ( X_beep > 0xff ){
		X_beep = 0xaf;
		GetCustData(T("X_beep"), &X_beep, sizeof(X_beep));
	}
	if ( X_beep & (1 << type) ){
		if ( type >= BeepType_Max ) type = BeepType_Max - 1;
		MessageBeep(BeepType[type]);
	}
}

BOOL PPxFlashWindow(HWND hWnd, int mode)
{
	HWND hParentWnd;

	for (;;){ // キャプションがある親を探す
		hParentWnd = GetParent(hWnd);
		if ( hParentWnd == NULL ) break;
		hWnd = hParentWnd;
	}

	if ( DFlashWindow == NULL ){
		HMODULE hUser32;

		hUser32 = GetModuleHandleA(User32DLL);
		#pragma warning(disable:4245) // User32.dll は常に読み込み済み
		GETDLLPROC(hUser32, FlashWindow);
		GETDLLPROC(hUser32, FlashWindowEx);
		#pragma warning(default:4245)
	}
	if ( mode == PPXFLASH_STOP ){
		DFlashWindow(hWnd, FALSE);
		return TRUE;
	}

	if ( DFlashWindowEx != NULL ){
		FLASHWINFO fwinfo;

		fwinfo.cbSize = sizeof(fwinfo);
		fwinfo.hwnd = hWnd;
		fwinfo.dwFlags = FLASHW_ALL;
		fwinfo.uCount = 5;
		fwinfo.dwTimeout = 500;
		DFlashWindowEx(&fwinfo);
		return FALSE;
	}else{
		DFlashWindow(hWnd, TRUE);
		return TRUE;
	}
}

void PrintToConsole(const TCHAR *text)
{
	DWORD size;
#ifdef UNICODE
	char buf[0x1000];
	DWORD length;

	length = UnicodeToAnsi(text, buf, sizeof(buf));
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), buf, length, &size, NULL);
#else
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), text, strlen(text), &size, NULL);
#endif
}

PPXDLL void USECDECL XMessage(HWND hWnd, const TCHAR *title, UINT type, const TCHAR *message, ...)
{
	ThuSTRUCT thu;
	DWORD flag;
	t_va_list argptr;
	UINT icon;

	ThuInit(&thu);
	flag = 1 << type;
	//------------------------------------------------------ 表示内容を作成する
	t_va_start(argptr, message);
	if ( flag & 0x67ffffff ){
		thvprintf(&thu.th, THP_CAT, MessageText(message), argptr);
		message = ThuStrT(&thu);
	}else if ( type <= XM_DUMPLOG ){
		DWORD size;
		const BYTE *srcp;
		int i;

		size = t_va_arg(argptr, DWORD);
		srcp = (const BYTE *)message;
		if ( type == XM_DUMPLOG ) thprintf(&thu.th, THP_CAT, T("%s"), title);
		while( size ){
			thprintf(&thu.th, THP_CAT, T("\r\n%p :"), srcp);
			for ( i = 0 ; i < 16 ; i++ ){
				thprintf(&thu.th, THP_CAT, T(" %02X"), *srcp++);
				size--;
				if ( size == 0 ) break;
			}
		}
		message = ThuStrT(&thu);
	}else{
		message = T("Unknown type");
	}
	t_va_end(argptr);
	//----------------------------------------------------- アイコン設定 ------
	if ( type < 16 ){
		icon = BeepType[type & 7];
	}else if ( type == 16 ){
		icon = MB_ICONSTOP;
	}else if ( type < 24 ){
		icon = MB_ICONEXCLAMATION;
	}else{
		icon = MB_ICONINFORMATION;
	}
	//---------------------------------------------------------- ログ出力を行う
	if ( X_log & flag ){
		TCHAR temp[MAX_PATH];
		DWORD size;
		HANDLE hFile;
		SYSTEMTIME ltime;

		temp[0] = '\0';
		GetCustData(T("X_save"), &temp, sizeof(temp));
		if ( temp[0] == '\0' ) tstrcpy(temp, DLLpath);
		VFSFixPath(NULL, temp, DLLpath, VFSFIX_FULLPATH | VFSFIX_REALPATH);
		CatPath(NULL, temp, T("PPX.LOG"));
		GetLocalTime(&ltime);

		hFile = CreateFileL(temp, GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL);
		if ( hFile != INVALID_HANDLE_VALUE ){
			DWORD len;
			#ifdef UNICODE
			if ( GetLastError() == NO_ERROR ){
				WriteFile(hFile, UCF2HEADER, UCF2HEADERSIZE, &size, NULL);
			}
			#endif
			size = 0;							// 末尾に移動 -----------------
			SetFilePointer(hFile, 0, (PLONG)&size, FILE_END);
												// 時刻を出力 -----------------
			size = TSTROFF(thprintf(temp, TSIZEOF(temp),
					T("%4d-%02d-%02d %02d:%02d:%02d.%03d (%s) "),
					ltime.wYear, ltime.wMonth, ltime.wDay,
					ltime.wHour, ltime.wMinute, ltime.wSecond,
					ltime.wMilliseconds, MessageType[type]) - temp);
			WriteFile(hFile, temp, size, &size, NULL);
												// 内容を出力 -----------------
			#pragma warning(suppress:6001 6054) // XM_DUMPLOG, size = 0 の時に。無視
			len = TSTRLENGTH32(message);
			if ( (len > 4) && (message[(len / sizeof(TCHAR)) - 2] == '\r') ){
				len -= sizeof(TCHAR) * 2; // 末尾改行を除去
			}
			WriteFile(hFile, message, len, &size, NULL);
												// 改行 -------------------
			WriteFile(hFile, T("\r\n"), TSTROFF(2), &size, NULL);
			CloseHandle(hFile);
		}
	}
	if ( B23 & flag ){
		SendMessage(hWnd, WM_PPXCOMMAND, K_WINDDOWLOG, (LPARAM)message);
	}
	//----------------------------------------------------- ダイアログ表示/Beep
	/* 0110 1001 0110 1111 0000 0000 1111 1111 */
	if ( 0x696f00ff & flag ){
		if ( (ConsoleMode < ConsoleMode_ConsoleOnly) && !(flag & 0x60600000) ){
			PMessageBox(hWnd, message, (title != NULL) ? title : PPxName,
				MB_APPLMODAL | MB_OK | icon);
		}else{
			if ( title != NULL ){
				PrintToConsole(title);
				PrintToConsole(T(": "));
			}
			PrintToConsole(message);
		}
	/* 0001 0110 0000 0000 1111 1111 0000 0000 ダイアログがない種類だけbeep */
	}else if ( X_beep & flag & 0x1600ff00 ){
		MessageBeep(icon);
	}
	ThuFree(&thu);
	return;
}

HMONITOR GetMonitorHandle(HWND hBaseWnd, int mode)
{
	if ( DMonitorFromWindow == NULL ){
		LoadWinAPI(User32DLL, NULL, MONITORDLL, LOADWINAPI_GETMODULE);
	}
	if ( DMonitorFromWindow == NULL ) return NULL;
	if ( mode == GMH_INWINDOW ){
		return DMonitorFromWindow(hBaseWnd, MONITOR_DEFAULTTONEAREST);
	}
	return NULL;
}

PPXDLL void PPXAPI GetDesktopRect(HWND hWnd, RECT *desktop)
{
	if ( GetSystemMetrics(SM_CMONITORS) > 1 ){	// マルチモニタ
		if ( DMonitorFromWindow == NULL ){
			LoadWinAPI(User32DLL, NULL, MONITORDLL, LOADWINAPI_GETMODULE);
		}
		if ( DMonitorFromWindow != NULL ){
			HMONITOR hMonitor;
			MONITORINFO MonitorInfo;

			hMonitor = DMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
			MonitorInfo.cbSize = sizeof(MonitorInfo);
			DGetMonitorInfoA(hMonitor, &MonitorInfo);
			*desktop = MonitorInfo.rcWork;
		}
	}else{										// シングルモニタ
		SystemParametersInfo(SPI_GETWORKAREA, 0, desktop, 0);
	}
}

UINT USEFASTCALL GetGDIdpi(HWND hWnd)
{
	HDC hDC;
	UINT dpi;

	hDC = GetDC(hWnd);
	dpi = GetDeviceCaps(hDC, LOGPIXELSX);
	ReleaseDC(hWnd, hDC);
	return dpi;
}

UINT GetMonitorDPI(HWND hWnd)
{
	UINT dpiX, dpiY;

	if ( hWnd == NULL ) hWnd = GetFocus();
	if ( (DGetDpiForMonitor == NULL) && (hShcore == NULL) ){
		if ( DMonitorFromWindow == NULL ){
			LoadWinAPI(User32DLL, NULL, MONITORDLL, LOADWINAPI_GETMODULE);
		}
		hShcore = LoadSystemDLL(SYSTEMDLL_SHCORE);
		if ( hShcore != NULL ){
			GETDLLPROC(hShcore, GetDpiForMonitor);
		}else{
			hShcore = INVALID_VALUE(HMODULE);
		}
	}
	if ( (DGetDpiForMonitor != NULL) && (DMonitorFromWindow != NULL) ){
		HMONITOR hMonitor;

		hMonitor = DMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		if ( hMonitor != NULL ){
			DGetDpiForMonitor(hMonitor, xMDT_EFFECTIVE_DPI, &dpiX, &dpiY);
			return dpiX;
		}
	}
	return GetGDIdpi(hWnd);
}

const LOGFONT DefFontInfo[2] = { Default_F_mes , Default_F_unfix };
const TCHAR *DefFontInfoJA[2] = { Default_F_mes_JA , Default_F_unfix_JA };

const TCHAR *PPxFontList[] = {
	T("F_mes"),
	T("F_dlg"),
	T("F_fix"),
	T("F_unfix"),
	T("F_tree"),
	T("F_ctrl"),
};

const TCHAR StatusFontPath[] = T("Control Panel\\Desktop\\WindowMetrics");
const TCHAR StatusFontName[] = T("StatusFont");

PPXDLL void PPXAPI GetPPxFont(int type, DWORD dpi, LOGFONTWITHDPI *font)
{
	NONCLIENTMETRICS ncm;

	font->dpi = 0;
	if ( NO_ERROR != GetCustData(PPxFontList[type], font, sizeof(LOGFONTWITHDPI)) ){
		font->dpi = DEFAULT_WIN_DPI;
		switch ( type ){
			case PPXFONT_F_fix:
				GetPPxFont(PPXFONT_F_mes, dpi, font);
				return;

			case PPXFONT_F_tree:
				font->font.lfFaceName[0] = '\0';
				return;

			case PPXFONT_F_ctrl:
				ncm.cbSize = sizeof(ncm);
				// SystemParametersInfoForDpi
				SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
				font->font = ncm.lfStatusFont;
				// SPI_GETNONCLIENTMETRICS の dpi を取得
				font->dpi = GetGDIdpi(NULL);
				break;

			default: {
				int ftype = (type == PPXFONT_F_unfix) ? 1 : 0;

				font->font = DefFontInfo[ftype];
				if ( LOWORD(GetUserDefaultLCID()) == LCID_JAPANESE ){
					tstrcpy(font->font.lfFaceName, DefFontInfoJA[ftype]);
				}
			}
		}
	}

	// 高さ指定がなければ適当に設定
	if ( font->font.lfHeight == 0 ){
		// dialog は dpi に影響しないポイント指定なので、同じく変化しないレジストリの値を利用
		if ( (type == PPXFONT_F_dlg) &&
			GetRegString(HKEY_CURRENT_USER, StatusFontPath, StatusFontName,
				(TCHAR *)&ncm.lfStatusFont,
				sizeof(ncm.lfStatusFont) / sizeof(TCHAR)) ){ // F_dlg
			font->font.lfHeight = ncm.lfStatusFont.lfHeight;
			if ( font->font.lfHeight == 0 ){
				font->font.lfHeight = T_DEFAULTFONTSIZE;
			}
		}else if ( type <= PPXFONT_F_fix ){ // F_mes, F_dlg, F_fix
			// SPI_GETNONCLIENTMETRICS の値は、現在の DPI と違うタイミングで変化する
			ncm.cbSize = sizeof(ncm);
			// SystemParametersInfoForDpi
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
			font->font.lfHeight = ncm.lfStatusFont.lfHeight;
			if ( font->font.lfHeight == 0 ){
				font->font.lfHeight = T_DEFAULTFONTSIZE;
			}

			if ( type != PPXFONT_F_dlg ){ // フォントを少し大きくする
				// ※PPXFONT_F_dlgはpoint表記のため、元から少し大きくなる
				font->font.lfHeight = (font->font.lfHeight * 125) / 100;
			}
		}else{
			font->font.lfHeight = T_DEFAULTFONTSIZE;
		}
	}

	if ( dpi == 0 ) return;

	if ( X_dss == DSS_NOLOAD ){
		X_dss = DSS_DEFFLAGS;
		GetCustData(T("X_dss"), &X_dss, sizeof(X_dss));
	}

	if ( X_dss & DSS_FONT ){ // 画面密度自動スケーリング
		if ( font->dpi == 0 ) font->dpi = DEFAULT_WIN_DPI;
		if ( type != PPXFONT_F_dlg ){
			if ( dpi != font->dpi ){
				font->font.lfHeight = (font->font.lfHeight * (int)dpi) / (int)font->dpi;
				font->font.lfWidth  = (font->font.lfWidth  * (int)dpi) / (int)font->dpi;
			}
		}else{ // PPXFONT_F_dlg
			if ( (WinType >= WINTYPE_10) && (OSver.dwBuildNumber < WINBUILD_10_RS2) ){
				// Win7 以前は、フォント設定のptが変更される 8は未チェック
				// Win10 RS2 以降は、マニフェストの PerMonitorV2 で処理
				DWORD GDIdpi = GetGDIdpi(NULL);

				if ( dpi != GDIdpi ){
					font->font.lfHeight = (font->font.lfHeight * (int)dpi) / (int)GDIdpi;
					font->font.lfWidth  = (font->font.lfWidth  * (int)dpi) / (int)GDIdpi;
				}
			}
		}
	}

	if ( (X_dss & DSS_MINFONT) || ((TouchMode & TOUCH_LARGEWIDTH) && (type == PPXFONT_F_ctrl)) ){ // 最小値自動スケーリング
		int minHeight = (type != PPXFONT_F_dlg) ? // 約8pt
				(PPX_FONT_MIN_PIXEL * dpi) / DEFAULT_WIN_DPI :
				PPX_FONT_MIN_PT;

		if ( (TouchMode & TOUCH_LARGEWIDTH) && (type == PPXFONT_F_ctrl) ){
			if ( X_tous[tous_TEXTSIZE] < 0 ){
				X_tous[tous_TEXTSIZE] = GUI_TEXT_MINSIZE;
				GetCustData(T("X_tous"), &X_tous, sizeof(X_tous));
			}
			minHeight = CalcMinDpiSize(dpi, X_tous[tous_TEXTSIZE]);
		}

		if ( font->font.lfHeight < 0 ){
			if ( font->font.lfHeight > -minHeight ){
				font->font.lfHeight = -minHeight;
				font->font.lfWidth  = 0;
			}
		}else{
			if ( font->font.lfHeight < minHeight ){
				font->font.lfHeight = minHeight;
				font->font.lfWidth  = 0;
			}
		}
	}
}

// 指定した方向へウィンドウをデスクトップの 1/32 だけ移動させる ---------------
PPXDLL void PPXAPI MoveWindowByKey(HWND hWnd, int offx, int offy)
{
	int x, y, stepX, stepY, fixX = 0, fixY = 0;
	RECT box, desktop;

	hWnd = GetCaptionWindow(hWnd);
	GetWindowRect(hWnd, &box);

	if ( DDwmGetWindowAttribute == NULL ){
		HANDLE hDWMAPI = GetModuleHandle(T("DWMAPI.DLL"));

		if ( hDWMAPI != NULL ){
			GETDLLPROC(hDWMAPI, DwmGetWindowAttribute);
		}
		if ( DDwmGetWindowAttribute == NULL ){
			DDwmGetWindowAttribute = INVALID_VALUE(impDwmGetWindowAttribute);
		}
	}
	if ( (DDwmGetWindowAttribute != INVALID_VALUE(impDwmGetWindowAttribute)) &&
		SUCCEEDED(DDwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &desktop, sizeof(desktop))) ){
		if ( desktop.left > box.left ){ // 実体の枠がウィンドウの枠より小さい（Windows8/10）
			fixX = box.left - desktop.left;
			box.left -= fixX;
			fixY = box.top - desktop.top;
			box.top -= fixY;
		}
	}
	GetDesktopRect(hWnd, &desktop);
											// X 移動
	stepX = (desktop.right - desktop.left) >> 5;
	if ( offx == 0 ){
		x = box.left;
	}else{
		x = ((box.left / stepX) + offx) * stepX;
	}
	stepY = (desktop.bottom - desktop.top) >> 5;
	if ( offy == 0 ){
		y = box.top;
	}else{
		y = ((box.top / stepY) + offy) * stepY;
	}

	if ( GetCustDword(T("X_mmon"), 0) == 0 ){
		if ( (x + box.right - box.left) <= (desktop.left + stepX) ){
			x = desktop.left + stepX - (box.right - box.left);
		}
		if ( x >= (desktop.right - stepX)) x = desktop.right - stepX;

		if ( (y + box.bottom - box.top) <= (desktop.top + stepY) ){
			y = desktop.top + stepY - (box.bottom - box.top);
		}
		if ( y >= (desktop.bottom - stepY)) y = desktop.bottom - stepY;
	}
	SetWindowPos(hWnd, NULL, x + fixX, y + fixY, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

const TCHAR StrHelpWebPage[] = T(TOROsWEB) T("/") T(HTMLHELP_WEBPAGE);
const TCHAR StrHelpWebText[] = T(TOROsWEB) T("/") T(HTMLHELP_TEXT);

PPXDLL void PPXAPI PPxHelp(HWND hWnd, UINT command, DWORD_PTR data)
{
	TCHAR path[VFPS], temp[CMDLINESIZE], basepath[VFPS];
	int offlinefile = 0;
		#define OLF_FRAME B0 // + 強制フレーム
		#define OLF_FILESC B1 // - 強制file:///
		#define OLF_INDEX B2 // index 有り
		#define OLF_WORDS B3 // words 有り
		#define OLF_TEXT B4 // help 有り
	HANDLE result;
	TCHAR *anchor, *bp;

	if ( (data == 0) && (command != HELP_FINDER) ) return;
	if ( (data == MAX16) || (data == MAX32) ) return;

	basepath[0] = '\0';
	GetCustTable(StrCustOthers, T("helpdir"), basepath, sizeof(basepath));
	if ( basepath[0] == '+' ){
		offlinefile = OLF_FRAME;
		bp = basepath + 1;
	}else if ( basepath[0] == '-' ){
		offlinefile = OLF_FILESC;
		bp = basepath + 1;
	}else{
		bp = basepath;
	}
	VFSFullPath(basepath, bp, DLLpath);

	// _others:winhelp が示す Winhelp32.exe を直接使用
	temp[0] = '\0';
	GetCustTable(StrCustOthers, T("winhelp"), temp, sizeof(temp));
	if ( temp[0] != '\0' ){
		// HELP_CONTEXT / HELP_CONTEXTPOPUP は winhlp32 -i IDH_PPC としないとだめ。
		if ( command == HELP_KEY ){
			thprintf(path, TSIZEOF(path), T("-k%s %s\\%s"), (TCHAR *)data, basepath, T(WINHELPFILE));
		}else{
			const TCHAR *ikey = NULL;
			if ( command == HELP_CONTEXT ){
				#define HELPCONTIDS(name) case name: ikey = T(#name); break;
				switch (data){
					HELPCONTIDS(IDH_PPC)
					HELPCONTIDS(IDH_PPE)
					HELPCONTIDS(IDH_PPV)
					HELPCONTIDS(IDH_PPB)
					HELPCONTIDS(IDD_INFO)
					HELPCONTIDS(IDD_GENERAL)
					HELPCONTIDS(IDD_EXT)
					HELPCONTIDS(IDD_KEYD)
					HELPCONTIDS(IDD_MOUSED)
					HELPCONTIDS(IDD_BARD)
					HELPCONTIDS(IDD_MENUD)
					HELPCONTIDS(IDD_COLOR)
					HELPCONTIDS(IDD_ADDON)
					HELPCONTIDS(IDD_ETCTREE)
				}
			}
			if ( ikey != NULL ){
				thprintf(path, TSIZEOF(path), T("-i%s %s\\%s"), ikey, basepath, T(WINHELPFILE));
			}else{
				thprintf(path, TSIZEOF(path), T("%s\\%s"), basepath, T(WINHELPFILE));
			}
		}
		if ( NULL != PPxShellExecute(hWnd, NULL, temp, path, basepath, 0, path) ){
			return;
		}
	}

	if ( WinType < WINTYPE_VISTA ){ // help file を使用
		CatPath(path, basepath, T(WINHELPFILE));
		if ( GetFileAttributesL(path) != BADATTR ){
			WinHelp(hWnd, path, command, data);
			return;
		}
	}

	// html help を開く
	CatPath(path, basepath, T(HTMLHELP_INDEX));
	if ( GetFileAttributesL(path) != BADATTR ){
		offlinefile += OLF_INDEX;
	}
	CatPath(path, basepath, T(HTMLHELP_WORDS));
	if ( GetFileAttributesL(path) != BADATTR ){
		offlinefile += OLF_WORDS;
	}
	CatPath(path, basepath, T(HTMLHELP_TEXT));
	if ( GetFileAttributesL(path) != BADATTR ){
		offlinefile += OLF_TEXT;
	}else{  // online
		// Win8 以降ならフレーム有りにする
		tstrcpy(path, (!(offlinefile & OLF_FILESC) && (WinType >= WINTYPE_8)) ? StrHelpWebPage : StrHelpWebText);
	}

	anchor = path + tstrlen(path);
	if ( (command == HELP_CONTEXT) || (command == HELP_CONTEXTPOPUP) ){
		thprintf(anchor, 32, T("#%d"), (int)data);
	}else if ( command == HELP_KEY ){
		thprintf(anchor, 128, T("#%s"), (TCHAR *)data);
	}else if ( offlinefile >= OLF_TEXT ){
		anchor = tstpcpy(path, StrHelpWebPage);
	}

	if ( offlinefile >= OLF_TEXT ){ // offile file あり
		if ( (offlinefile & OLF_FRAME) ||
			 (!(offlinefile & OLF_FILESC) && (offlinefile >= (OLF_TEXT | OLF_INDEX))) ){ // help/wordsがあるのでフレーム有りにする
			DWORD size;
			HANDLE hFile;
			#ifdef UNICODE
				#define HELP_CHAR "utf-8"
				char HELP_BASEPATH[VFPS];
				char HELP_ANCHOR[64];
				WideCharToMultiByteU8(CP_UTF8, 0, basepath, -1, HELP_BASEPATH, VFPS, NULL, NULL);
				WideCharToMultiByteU8(CP_UTF8, 0, anchor, -1, HELP_ANCHOR, 64, NULL, NULL);
			#else
				#define HELP_CHAR "Shift_JIS"
				#define HELP_BASEPATH basepath
				#define HELP_ANCHOR anchor
			#endif
			size = wsprintfA((char *)temp,
				"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3.org/TR/html4/frameset.dtd\"><html lang=\"ja\">"
				"<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=" HELP_CHAR "\">"
				"<title>PPx help</title></head>\r\n"
				"<frameset cols=\"20%%,*\">"
				"<frame src=\"file:///%s\\%s\" name=\"index\">"
				"<frame src=\"file:///%s\\ppxhelp.html%s\" name=\"main\">"
				"<noframes><body><a href=\"ppxhelp.html%s\"></a></body></noframes></frameset></html>",
				HELP_BASEPATH,
				(offlinefile & OLF_WORDS) ? HTMLHELP_WORDS : HTMLHELP_INDEX,
				HELP_BASEPATH, HELP_ANCHOR, HELP_ANCHOR);
#if 1
			if ( ProcTempPath[0] == '\0' ){
				MakeTempEntry(MAX_PATH, NULL, 0); // ProcTempPath を作成
			}
			CatPath(path, ProcTempPath, T("ppxhelp.html"));
#else
			tstrcpy(path, T("ppxhelp.html"));
			MakeTempEntry(path, tempfile, FILE_ATTRIBUTE_LABEL);
#endif
			hFile = CreateFileL(path, GENERIC_WRITE,
					FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL, NULL);
			if ( hFile != INVALID_HANDLE_VALUE ){
				WriteFile(hFile, temp, size, &size, NULL);
				CloseHandle(hFile);
			}
		}else{
			thprintf(temp, TSIZEOF(temp), T("file:///%s"), path);
			tstrcpy(path, temp);
		}
	}

	PP_ExtractMacro(hWnd, NULL, NULL, T("%g'browser'"), temp, 0);
	if ( temp[0] == '\0' ){
		result = PPxShellExecute(hWnd, NULL, path, NilStr, basepath, 0, path);
	}else{
		if ( tstrchr(path, ' ') != NULL ){
			TCHAR path2[VFPS];

			tstrcpy(path2, path);
			thprintf(path, TSIZEOF(path), T("\"%s\""), path2);
		}
		result = PPxShellExecute(hWnd, NULL, temp, path, basepath, 0, path);
	}
	if ( result == NULL ) PopupErrorMessage(hWnd, NULL, path);
}

BOOL PPxDialogHelp(HWND hDlg)
{
	DWORD ID;

	ID = (DWORD)SendMessage(hDlg, WM_COMMAND, IDQ_GETDIALOGID, 0);
	if ( ID == 0 ) return FALSE;
	PPxHelp(hDlg, HELP_CONTEXT, ID);
	return TRUE;
}

// window / dialog 兼用
LRESULT ControlWindowColor(WPARAM wParam)
{
	SetTextColor((HDC)wParam, C_WindowText);
	SetBkColor((HDC)wParam, C_WindowBack);
	if ( hWindowBackBrush != NULL ) return (LRESULT)hWindowBackBrush;
	InitSysColors();
	hWindowBackBrush = CreateSolidBrush(C_WindowBack);
	return (LRESULT)hWindowBackBrush;
}

// dialog 専用
INT_PTR ControlDialogColor(WPARAM wParam)
{
	if ( !(ThemeColors.ExtraDrawFlags & (EDF_DIALOG_TEXT | EDF_DIALOG_BACK)) ){
		return 0;
	}
	SetTextColor((HDC)wParam, C_DialogText);
	SetBkColor((HDC)wParam, C_DialogBack);

	return (LRESULT)hDialogBackBrush; // WM_CTLCOLORDLG は DWLP_MSGRESULT を使わなくてもよい
}

#pragma argsused
PPXDLL INT_PTR PPXAPI PPxDialogHelper(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UnUsedParam(wParam); UnUsedParam(lParam);

	switch (message){
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLOREDIT:
			if ( !(ThemeColors.ExtraDrawFlags & (EDF_WINDOW_TEXT | EDF_WINDOW_BACK)) ){
				return FALSE;
			}
			return ControlWindowColor(wParam);

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
			return ControlDialogColor(wParam);

		case WM_DESTROY:
			SetIMEDefaultStatus(hDlg);
			return FALSE;

		case WM_CLOSE:
			EndDialog(hDlg, 0);
			break;

		case WM_HELP:
//		case WM_CONTEXTMENU:
			PPxDialogHelp(hDlg);
			break;
		default:
			if ( message == WM_PPXCOMMAND ) return ERROR_INVALID_FUNCTION;
			return FALSE;
	}
	return TRUE;
}

BOOL OpenClipboard2(HWND hWnd)
{
	int trycount = 6;

	for (;;){
		if ( IsTrue(OpenClipboard(hWnd)) ) return TRUE;
		if ( --trycount == 0 ) return FALSE;
		Sleep(20);
	}
}

void ClipTextData(HWND hWnd, const TCHAR *text)
{
	INT_PTR len;
	HGLOBAL hG;

	len = TSTRSIZE(text);
	hG = GlobalAlloc(GMEM_MOVEABLE, len);
	if ( hG == NULL ) return;
	memcpy(GlobalLock(hG), text, len);
	GlobalUnlock(hG);
	OpenClipboard2(hWnd);
	EmptyClipboard();
	SetClipboardData(CF_TTEXT, hG);
	CloseClipboard();
}

#define CLIPMSGTEXTSIZE 0x4000
#define CLIPMSGTITLESIZE 0x800

#pragma warning(suppress:6262) // スタックを使いすぎだが、現在は無視
void ClipMessageBox(HWND hDlg)
{
	TCHAR text[CLIPMSGTEXTSIZE], *ptr;
	HMENU hPopupMenu;
	POINT pos;
	int index;

	hPopupMenu = CreatePopupMenu();
	AppendMenuString(hPopupMenu, 1, MES_MBCP);
	GetCursorPos(&pos);
	index = TrackPopupMenu(hPopupMenu, TPM_TDEFAULT, pos.x, pos.y, 0, hDlg, NULL);
	DestroyMenu(hPopupMenu);
	if ( index > 0 ){
		text[0] = '\0';
		GetWindowText(hDlg, text, CLIPMSGTITLESIZE);
		ptr = text + tstrlen(text);
		*ptr++ = '\r';
		*ptr++ = '\n';
		GetDlgItemText(hDlg, IDS_MSGBOX_TEXT, ptr, CLIPMSGTEXTSIZE - CLIPMSGTITLESIZE);
		ClipTextData(hDlg, text);
	}
}

void PlayWave(const TCHAR *name)
{
	TCHAR path[VFPS];

	if ( name[0] == '\0' ) name = NULL; // 再生停止
	if ( hWinmm == NULL ){
		hWinmm = LoadSystemDLL(SYSTEMDLL_WINMM);
		if ( hWinmm != NULL ) GETDLLPROCT(hWinmm, PlaySound);
	}
	if ( hWinmm != NULL ){
		if ( name != NULL ) VFSFullPath(path, CONSTCAST(TCHAR *, name), DLLpath);
		if ( IsTrue(DPlaySound(name, NULL,
				SND_FILENAME | SND_NOWAIT | SND_ASYNC)) ){
			return;
		}
	}
	MessageBeep(MB_ICONASTERISK);
}

#ifndef ENABLE_INSERT_MODE
	#define ENABLE_INSERT_MODE 0x0020
	#define ENABLE_QUICK_EDIT_MODE 0x0040
	#define ENABLE_EXTENDED_FLAGS 0x0080
#endif
#ifndef ENABLE_AUTO_POSITION
	#define ENABLE_AUTO_POSITION 0x0100
#endif

DWORD UseConsoleKey(void)
{
	DWORD oldmode;

	DGetConsoleMode(hConsoleStdin, &oldmode);
	DSetConsoleMode(hConsoleStdin, ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS | ENABLE_AUTO_POSITION);
	return oldmode;
}

void FreeConsoleKey(DWORD oldmode)
{
	DSetConsoleMode(hConsoleStdin, oldmode);
}

int ReadConsoleKey(void)
{
	INPUT_RECORD cin;
	DWORD read;
	int key;

	for (;;){
		cin.EventType = 0;
		DReadConsoleInput(hConsoleStdin, &cin, 1, &read);
		if ( cin.EventType == KEY_EVENT ){
			if ( cin.Event.KeyEvent.bKeyDown == FALSE ) continue;
			#ifdef UNICODE
				key = cin.Event.KeyEvent.uChar.UnicodeChar;
				if ( key > 0xff ){
					key = (key & 0xff) | ((key & 0xff00) << 8);
				}
			#else
				key = cin.Event.KeyEvent.uChar.AsciiChar;
				if ( key >= 0xffff80 ) key &= 0xff;	// S-JIS の処理
			#endif
			if ( key == 0 ) key = K_v | cin.Event.KeyEvent.wVirtualKeyCode;
			if ( cin.Event.KeyEvent.dwControlKeyState &
					(RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED) ){
				if ( key < 32 ) key += '@';
				key |= K_c;
			}
			return key;
		}
	}
}

int PMessageBoxConsole(const TCHAR *text, const TCHAR *title, UINT style)
{
	TCHAR buf[CMDLINESIZE];
	int type, bcount, defbutton, cancelbutton, key;
	const MESSAGEITEMS *mi, *mtype; // メッセージボックスボタンの種類
	DWORD old;
	BOOL draw = TRUE;

	thprintf(buf, TSIZEOF(buf), T("\r\n[%Ms]\r\n%Ms\r\n"), title, text);
	PrintToConsole(buf);

	type = style & 0xf;
	if ( type >= MB_PPX_MAX ) type = 0;
	mtype = MessageTypes[type];
	defbutton = GetMsgBoxDefButton(style);

	old = UseConsoleKey();
	for (;;){
		if ( draw ){
			TCHAR bk1, bk2;
			draw = FALSE;
			bcount = 0;
			cancelbutton = -1;
			PrintToConsole(T("\r"));
			for ( mi = mtype ; mi->text != NULL ; mi++ ){
				if ( bcount == defbutton ){
					bk1 = '[';
					bk2 = ']';
				}else{
					bk1 = bk2 = ' ';
				}
				thprintf(buf, TSIZEOF(buf),
						((mi + 1)->text == NULL) ? T("%c%Ms%c") : T("%c%Ms%c/ "),
						bk1, mi->text, bk2);
				if ( mi->ID == IDCANCEL ) cancelbutton = bcount;
				PrintToConsole(buf);
				bcount++;
			}
		}
		key = ReadConsoleKey();
		switch ( key ){
			case K_esc:
			case 27:
				if ( cancelbutton >= 0 ) return mtype[cancelbutton].ID;
				break;

			case K_cr:
			case 13:
			case ' ':
				return mtype[defbutton].ID;

			case K_lf:
			case K_c | 'B':
				if ( defbutton > 0 ){
					defbutton--;
					draw = TRUE;
					continue;
				}
				break;

			case K_ri:
			case K_c | 'F':
				if ( defbutton < (bcount - 1) ){
					defbutton++;
					draw = TRUE;
					continue;
				}
				break;

			default:
				key = TinyCharUpper(key);
				for ( mi = mtype ; mi->text != NULL ; mi++ ){
					const TCHAR *ptr;
					int prefix;
					ptr = tstrchr(mi->text, '&');
					if ( ptr == NULL ) ptr = SearchVLINE(mi->text);
					prefix = TinyCharUpper((ptr == NULL) ? mi->text[0] : ptr[1]);
					if ( prefix == key ){
						PrintToConsole(T("\r\n"));
						FreeConsoleKey(old);
						return mi->ID;
					}
				}
		}
	}
}

HWND GetWindowOnProcessMonitor(BOOL *visible)
{
	HWND hWnd, hResult;

	hResult = GetForegroundWindow();
	hWnd = GetWindow(hResult, GW_HWNDFIRST);
	for (;;){
		DWORD procid;

		if ( hWnd == NULL ){
			*visible = FALSE;
			return hResult;
		}
		GetWindowThreadProcessId(hWnd, &procid);
		if ( procid == GetCurrentProcessId() ){
			if ( IsWindowVisible(hWnd) ){
				return hWnd;
			}else{
				hResult = hWnd;
			}
		}
		hWnd = GetWindow(hWnd, GW_HWNDNEXT);
	}
}

BOOL MessageBoxInitDialog(HWND hDlg, MESSAGEDATA *md)
{
	HDC hDC;
	NONCLIENTMETRICS ncm;
	HFONT hOldFont;	//一時保存用
	int spaceH; // 各種基準につかう間隔(１行分の高さ)
	int WindowWide; // ウィンドウの幅
	const MESSAGEITEMS *mi, *mtype; // メッセージボックスボタンの種類
	int TextHeight;	// テキスト表示用空間の高さ
	int TextAreaLeft;
	int IconType; // アイコンの種類 0==未使用
	int IconWidth; // アイコンの幅(空白含む)
	int IconSize; // アイコンの幅(アイコンのみの大きさ)
	int ButtonCount = 0; // ボタンの数
	int ButtonWidth = 0; // ボタンの幅
	int ButtonHeight; // ボタンの高さ
	RECT msgbox = {0, 0, 0, 0};	// テキストの大きさ
	RECT buttonbox = {0, 0, 0, 0};	// ボタンの大きさ
	RECT tempbox = {0, 0, 0, 0};
	RECT deskbox;
	HWND hPWnd, hDpiWnd;
	int dpi, gdi_dpi, minHeight;
	BOOL longheight = FALSE; // TRUE の時は、static でなく、 edit を使う
	BOOL VisibleParentWnd = TRUE; // 位置の目安にするWindowが表示されているか

	md->hOldForegroundWnd = NULL;
	SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)md);
	SetWindowText(hDlg, md->title);
/* ここでは変更できない
	if ( md->style & MB_PPX_ENABLE_MINIMIZE ){
		SetWindowLongPtr(hDlg, GWL_STYLE, GetWindowLongPtr(hDlg, GWL_STYLE) | WS_MINIMIZEBOX);
	}
*/
	// ダイアログボックスフォントを作成
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

	hPWnd = GetParentCaptionWindow(hDlg);
	if ( hPWnd == NULL ) hPWnd = GetWindowOnProcessMonitor(&VisibleParentWnd);

	hDpiWnd = ((hPWnd != NULL) && !(md->style & MB_PPX_NOCENTER)) ? hPWnd : hDlg;
	GetDesktopRect(hDpiWnd, &deskbox);
	dpi = GetMonitorDPI(hDpiWnd);
	hDC = GetDC(hDpiWnd);
	gdi_dpi = GetDeviceCaps(hDC, LOGPIXELSX);

	ncm.lfMessageFont.lfHeight = (ncm.lfMessageFont.lfHeight * dpi) / gdi_dpi;
	minHeight = (PPX_FONT_MIN_PIXEL * dpi) / gdi_dpi; // 約8pt

	if ( ncm.lfMessageFont.lfHeight < 0 ){
		if ( ncm.lfMessageFont.lfHeight > -minHeight ){
			ncm.lfMessageFont.lfHeight = -minHeight;
			ncm.lfMessageFont.lfWidth  = 0;
		}
	}else{
		if ( ncm.lfMessageFont.lfHeight < minHeight ){
			ncm.lfMessageFont.lfHeight = minHeight;
			ncm.lfMessageFont.lfWidth  = 0;
		}
	}

	md->hDlgFont = CreateFontIndirect(&ncm.lfMessageFont);
	hOldFont = SelectObject(hDC, md->hDlgFont);

	{	// メッセージボックスの種類を決定
		int type;

		type = md->style & 0xf;
		if ( type >= MB_PPX_ADDABORTRETRYIGNORE ){
			setflag(md->style, MB_PPX_EXMSGBOX);
		}
		if ( type >= MB_PPX_MAX ) type = 0;
		mtype = MessageTypes[type];
	}
										// 大きさを計算 ------------
		// ボタンの大きさを決定
	for ( mi = mtype ; mi->text != NULL ; mi++ ){
		ButtonCount++;
		buttonbox.right = 0;
		DrawText(hDC, MessageText(mi->text), -1, &buttonbox, DT_CALCRECT | DT_NOCLIP);
		if ( ButtonWidth < buttonbox.right ) ButtonWidth = buttonbox.right;
	}
	spaceH = buttonbox.bottom;
	if ( ButtonWidth < (spaceH * 4) ) ButtonWidth = spaceH * 4;
	ButtonWidth += spaceH * 2;
	WindowWide = (ButtonWidth + spaceH) * ButtonCount + spaceH;

	IconType = (md->style >> 4) & 7;
	if ( IconType ){
	 	IconSize = GetSystemMetrics(SM_CXICON);
	 	IconWidth = IconSize + spaceH / 2; // アイコンサイズを決定
	}else{
	 	IconSize = 0;
		IconWidth = 0;
	}
	{
		int textmaxW, maxW_desk, maxW_font, maxH_desk, maxH_font;

		DrawText(hDC, md->text, -1, &msgbox, DT_CALCRECT | DT_NOCLIP |
				DT_EXPANDTABS | DT_NOPREFIX | DT_LEFT | DT_EDITCONTROL );

		// テキストの幅調整
		maxW_desk = deskbox.right - deskbox.left - IconWidth - buttonbox.bottom * 2;
		maxW_font = ncm.lfMessageFont.lfHeight * 100;
		if ( maxW_font < 0 ) maxW_font = -maxW_font;
		if ( maxW_desk > maxW_font ) maxW_desk = maxW_font;
		if ( msgbox.right > maxW_desk ){
			msgbox.right = maxW_desk;
			DrawText(hDC, md->text, -1, &msgbox, DT_CALCRECT | DT_WORDBREAK |
				DT_EXPANDTABS | DT_NOPREFIX | DT_LEFT | DT_EDITCONTROL);
		}
		textmaxW = msgbox.right + spaceH * (1 + 1) + IconWidth;
		if ( WindowWide < textmaxW ) WindowWide = textmaxW;
		TextAreaLeft = ( ButtonCount == 1 ) ?
				(WindowWide - (textmaxW - spaceH * (1 + 1)) ) / 2 : spaceH;

		// テキストの高さ調整
		maxH_desk = deskbox.bottom - deskbox.top - buttonbox.bottom * 10;
		maxH_font = ncm.lfMessageFont.lfHeight;
		if ( maxH_font < 0 ) maxH_font = -maxH_font;
		if ( maxH_desk < maxH_font ) maxH_desk = maxH_font;
		if ( msgbox.bottom > maxH_desk ){
			msgbox.bottom = maxH_desk;
			longheight = TRUE;
		}
		if ( msgbox.bottom < IconSize ){
			TextHeight = IconSize + spaceH * 2;
		}else{
			TextHeight = msgbox.bottom + spaceH * 2;
		}
	}
										// 作成 ------------
	if ( IconType ){ // アイコン
		HWND hWnd;

		hWnd = CreateWindowEx(WS_EX_NOPARENTNOTIFY, StaticClassName, NilStr,
				SS_ICON | WS_CHILD | WS_VISIBLE, TextAreaLeft, spaceH,
				IconSize, IconSize, hDlg, CHILDWNDID(20), DLLhInst, 0);
		if ( hWnd != NULL ){
			SendMessage(hWnd, WM_SETFONT, (WPARAM)md->hDlgFont, 0);
			SendMessage(hWnd, STM_SETICON,
					(WPARAM)LoadImage(NULL,
					MAKEINTRESOURCE((int)(INT_PTR)IDI_HAND + IconType - 1),
					IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED),
					0);
		}
	}
	{				// テキスト
		HWND hWnd;
		LPCTSTR classname;
		DWORD style;

#ifndef SS_EDITCONTROL
#define SS_EDITCONTROL 0x2000
#endif
		if ( longheight == FALSE ){
			classname = StaticClassName;
			style = SS_LEFT | SS_NOPREFIX | SS_EDITCONTROL | WS_CHILD | WS_VISIBLE;
		}else{
			classname = EditClassName;
			style = ES_LEFT | ES_MULTILINE | ES_NOHIDESEL | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP;
		}

		hWnd = CreateWindowEx(WS_EX_NOPARENTNOTIFY, classname, NilStr, style,
				TextAreaLeft + IconWidth, (TextHeight - msgbox.bottom) / 2,
				msgbox.right, msgbox.bottom, hDlg, CHILDWNDID(IDS_MSGBOX_TEXT),
				DLLhInst, 0);

		if ( hWnd != NULL ){
			FixUxTheme(hWnd, classname);
			SendMessage(hWnd, WM_SETFONT, (WPARAM)md->hDlgFont, 0);

			// CreateWindowEx で md->text を直接指定すると、32K（予想）で
			// 作成に失敗するので// 後で設定し直す。
			// また、Win9x では 32K 以内にする必要あり。
			#ifdef UNICODE
				SetWindowText(hWnd, md->text);
			#else
				if ( (OSver.dwPlatformId != VER_PLATFORM_WIN32_NT) &&
					 (strlen(md->text) >= 0x7f00) ){
					char buf[0x2000];

					tstplimcpy(buf, md->text, sizeof(buf));
					SetWindowText(hWnd, buf);
				}else{
					SetWindowText(hWnd, md->text);
				}
			#endif
		}
	}
	if ( md->style & (MB_PPX_EXMSGBOX | MB_PPX_ALLCHECKBOX) ){
		setflag(md->style, MB_PPX_EXMSGBOX);
		if ( md->style & MB_PPX_ALLCHECKBOX ){	// チェックボックス
			HWND hWnd;
			const TCHAR *caption;

			caption = MessageText(MessageAllReactionStr);
			DrawText(hDC, caption, -1, &tempbox, DT_CALCRECT |
					DT_NOCLIP | DT_NOPREFIX | DT_LEFT | DT_EDITCONTROL);
			hWnd = CreateWindowEx(WS_EX_NOPARENTNOTIFY, ButtonClassName, caption,
					BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
					TextAreaLeft + IconWidth / 2, TextHeight,
					tempbox.right + spaceH * 2, tempbox.bottom,
					hDlg, CHILDWNDID(IDX_QDALL), DLLhInst, 0);
			if ( hWnd != NULL ){
				FixUxTheme(hWnd, ButtonClassName);
				SendMessage(hWnd, WM_SETFONT, (WPARAM)md->hDlgFont, 0);
					TextHeight += tempbox.bottom + spaceH;
			}
		}
	}
	SelectObject(hDC, hOldFont);
	ReleaseDC(hDpiWnd, hDC);
	{				// ボタン
		int left, defbutton;

		defbutton = GetMsgBoxDefButton(md->style);
		ButtonHeight = buttonbox.bottom + spaceH;
		if ( ButtonCount < 2 ){
			left = (WindowWide - ButtonWidth) / 2;
		}else{
			left = WindowWide - (ButtonWidth + spaceH) * ButtonCount;
		}
		for ( mi = mtype ; mi->text != NULL ; mi++ ){
			HWND hWnd;
			DWORD bstyle;

			bstyle = (defbutton != 0) ? BS_PUSHBUTTON | BS_CENTER | WS_CHILD | WS_VISIBLE | WS_TABSTOP : BS_PUSHBUTTON | BS_CENTER | WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON;
			hWnd = CreateWindowEx(WS_EX_NOPARENTNOTIFY, ButtonClassName,
					MessageText(mi->text), bstyle,
					left, TextHeight, ButtonWidth, ButtonHeight, hDlg,
					(HMENU)(LONG_PTR)(int)mi->ID, DLLhInst, 0);
			if ( hWnd != NULL ){
				FixUxTheme(hWnd, ButtonClassName);
				SendMessage(hWnd, WM_SETFONT, (WPARAM)md->hDlgFont, 0);
				if ( defbutton == 0 ) SetFocus(hWnd);
			}
			left += ButtonWidth + spaceH;
			defbutton--;
		}
	}
										// ウィンドウの大きさを調整 -----------
	GetWindowRect(hDlg, &msgbox);
	GetClientRect(hDlg, &buttonbox);
	WindowWide += (msgbox.right - msgbox.left) - buttonbox.right;
	TextHeight += ButtonHeight + spaceH + ((msgbox.bottom - msgbox.top) - buttonbox.bottom);

	SetWindowPos(hDlg, NULL, 0, 0, WindowWide, TextHeight,
			SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
	if ( !(md->style & MB_PPX_NOCENTER) ){
		if ( VisibleParentWnd ){ // 表示基準ウィンドウの中央
			MoveCenterWindow(hDlg, hPWnd);
		}else{ // 基準がないので該当モニタの中央
			SetWindowPos(hDlg, NULL,
				((deskbox.right - deskbox.left) - WindowWide) / 2,
				((deskbox.bottom - deskbox.top) - TextHeight) / 2,
				0, 0, SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER);
		}
	}
	if ( md->style & MB_PPX_AUTORETRY ){
		SetTimer(hDlg, TIMERID_MSGBOX_AUTORETRY, TIME_MSGBOX_AUTORETRY, md->autoretryfunc);
	}
										// フォーカスを設定 -----------
	ActionInfo(hDlg, NULL, AJI_SHOW, T("msg"));
	InvalidateRect(hDlg, NULL, FALSE);
	return FALSE;
}

INT_PTR CALLBACK MessageBoxDxProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( iMsg ){
		case WM_INITDIALOG:
			return MessageBoxInitDialog(hDlg, (MESSAGEDATA *)lParam);

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLOREDIT:
			return ControlDialogColor(wParam);

		case WM_ACTIVATE: {
			MESSAGEDATA *md;

			md = (MESSAGEDATA *)GetWindowLongPtr(hDlg, DWLP_USER);
			if ( LOWORD(wParam) == WA_INACTIVE ){
				md->hOldForegroundWnd = NULL; // 非アクティブになった
			}else{
				md->hOldForegroundWnd = (HWND)lParam; // 初めてのアクティブなら値が有効、2回目以降はNULLらしい
			}
			return FALSE;
		}

		case WM_CONTEXTMENU:
			ClipMessageBox(hDlg);
			break;

		case WM_COMMAND:
			if ( ((LOWORD(wParam) >= IDOK) && (LOWORD(wParam) <= IDNO)) ||
				 (LOWORD(wParam) == IDB_QDADDLIST) ||
				 (LOWORD(wParam) == IDX_FOP_ADDNUMDEST) ){
				MESSAGEDATA *md;

				md = (MESSAGEDATA *)GetWindowLongPtr(hDlg, DWLP_USER);
				if ( md->style & MB_PPX_ALLCHECKBOX ){
					if ( IsDlgButtonChecked(hDlg, IDX_QDALL) ){
						setflag(wParam, ID_PPX_CHECKED);
					}
				}
				if ( md->hOldForegroundWnd != NULL ){
					SetForegroundWindow(md->hOldForegroundWnd);
					SetActiveWindow(md->hOldForegroundWnd);
				}
				EndDialog(hDlg, LOWORD(wParam));
			}
			break;

		case WM_DESTROY: {
			MESSAGEDATA *md;

			md = (MESSAGEDATA *)GetWindowLongPtr(hDlg, DWLP_USER);
			if ( md->style & MB_PPX_AUTORETRY ){
				KillTimer(hDlg, TIMERID_MSGBOX_AUTORETRY);
			}
			DeleteObject(md->hDlgFont);
			if ( md->hOldFocusWnd != NULL ) SetFocus(md->hOldFocusWnd);
			break;
		}
		default:
			return FALSE;
	}
	return TRUE;

}

PPXDLL int PPXAPI PMessageBox(HWND hWnd, const TCHAR *text, const TCHAR *title, UINT style)
{
#if UseTMessageBox
	MESSAGEDATA md;

	InitSysColors();
	if ( title == NULL ) title = PPxName;
	md.title = (style & MB_PPX_PLAIN_TITLE) ? title : MessageText(title);

	if ( text == NULL ){
		text = (style & MB_ICONQUESTION) ? MES_QABO : NilStr;
	}
	md.text = (style & MB_PPX_PLAIN_TEXT) ? text : MessageText(text);

	md.style = style & ~(MB_PPX_PLAIN_TITLE | MB_PPX_PLAIN_TEXT);
	md.hOldFocusWnd = GetFocus();
	if ( hWnd == NULL ) hWnd = md.hOldFocusWnd;

	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		return PMessageBoxConsole(text, title, style);
	}
	return (int)DialogBoxParam(DLLhInst, MAKEINTRESOURCE(IDD_NULL), hWnd, MessageBoxDxProc, (LPARAM)&md);
#else
	if ( title == NULL ) title = PPxName;
	title = (style & MB_PPX_PLAIN_TITLE) ? title : MessageText(title);

	if ( text == NULL ){
		text = (style & MB_ICONQUESTION) ? MES_QABO : NilStr;
	}
	text = (style & MB_PPX_PLAIN_TEXT) ? text : MessageText(text);
	style = style & 0xffffff; // B24 以降は PPx 独自設定なので除去
	return MessageBox(hWnd, text, title, style);
#endif
}

int PMessageBoxEx(HWND hWnd, MESSAGEDATA *md)
{
	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		return PMessageBoxConsole(md->text, md->title, md->style);
	}
	return (int)DialogBoxParam(DLLhInst, MAKEINTRESOURCE(IDD_NULL), hWnd, MessageBoxDxProc, (LPARAM)md);
}

// PMessageBox / XMessage が使えない時用
int CriticalMessageBox(const TCHAR *text, const TCHAR *title, UINT style)
{
	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		return PMessageBoxConsole(text, title, style);
	}
	return MessageBox(GetActiveWindow(), text, title, style);
}

int CriticalMessageBoxOk(const TCHAR *text)
{
	return CriticalMessageBox(text, CriticalTitle, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
}

COLORREF DarkPalette[16] = {
	0x808080, 0xff4040, 0x40ff40, 0xffff40,
	0x7070ff, 0xff40ff, 0x40ffff, 0xe0e0e0,
	0xa0a0a0, 0xff8080, 0x80ff80, 0xffff80,
	0x8080ff, 0xff00ff, 0x80ffff, 0xffffff
};

BOOL LoadResBMP(HTBMP *hTbmp, HINSTANCE hInst, LPCTSTR name, int bright)
{
	DWORD size;
	HRSRC hRsrc;

	hTbmp->dibfile = NULL;
	hTbmp->hPalette = NULL;
	hTbmp->info = NULL;
	hTbmp->bm = NULL;

	hRsrc = FindResource(hInst, name, RT_RCDATA);
	if ( hRsrc == NULL ) return FALSE;
	size = SizeofResource(hInst, hRsrc);
	hTbmp->dibfile = HeapAlloc(GetProcessHeap(), 0, size);
	if ( hTbmp->dibfile == NULL ) return FALSE;
	memcpy(hTbmp->dibfile, LockResource(LoadResource(hInst, hRsrc)), size);
	if ( X_uxt_color == UXT_DARK ){
		memcpy((BYTE *)((BYTE *)hTbmp->dibfile + 0x36), DarkPalette, sizeof(DarkPalette));
	}
	return InitBMP(hTbmp, NilStr, size, bright);
}

void FixToolBarBitmap(HWND hWnd, HTBMP *hTbmp, TBADDBITMAP *tbab, SIZE *bmpsize, int IconSize)
{
	HPALETTE hOldPal C4701CHECK;
	HBITMAP hBMP;
	HDC hDC;
	int sizedelta;

	hDC = GetDC(hWnd);
	if ( hTbmp->hPalette != NULL ){
		hOldPal = SelectPalette(hDC, hTbmp->hPalette, FALSE);
		RealizePalette(hDC);
	}

	hBMP = CreateDIBitmap(hDC, hTbmp->DIB, CBM_INIT,
		hTbmp->bits, (BITMAPINFO *)hTbmp->DIB, DIB_RGB_COLORS);
	if ( hTbmp->hPalette != NULL ){
		SelectPalette(hDC, hOldPal, FALSE);  // C4701ok
	}
	ReleaseDC(hWnd, hDC);
	tbab->hInst = NULL;
	tbab->nID   = (UINT_PTR)hBMP;

	// 必要とするサイズと大きく異なるなら、サイズ調整
	sizedelta = (IconSize / 2) - hTbmp->size.cy;
	if ( sizedelta < 0 ) sizedelta = -sizedelta;
	if ( sizedelta > (hTbmp->size.cy / 3) ){
		hTbmp->size.cx = (hTbmp->size.cx * IconSize) / (hTbmp->size.cy * 2);
		hTbmp->size.cy = IconSize / 2;

		tbab->nID = (UINT_PTR)CopyImage(hBMP, IMAGE_BITMAP, hTbmp->size.cx, hTbmp->size.cy, LR_COPYDELETEORG | LR_COPYRETURNORG);
	}
	*bmpsize = hTbmp->size;
	FreeBMP(hTbmp);
	return;
}


#define MAKEBUFSIZE 32
typedef struct {
	WCHAR destbuf[MAKEBUFSIZE], *dest;
	HDC hMDC;
	HFONT hFont;
	TEXTMETRIC tm;
	HGDIOBJ hOldFont;
	int baseX;
	int locate;
	int iconsize;
	BOOL changefont;
	LOGFONTWITHDPI usefont;
} TEXTMAKERSTRUCT;

void FillIcon(TEXTMAKERSTRUCT *tms, int width, COLORREF color)
{
	RECT box;
	HBRUSH hBack;

	box.left = tms->baseX;
	box.top = 0;
	box.right = tms->baseX + width;
	box.bottom = tms->iconsize;
	hBack = CreateSolidBrush(color);
	FillRect(tms->hMDC, &box, hBack);
	DeleteObject(hBack);
}

void DrawChar(TEXTMAKERSTRUCT *tms)
{
	int length;
	SIZE textsize;
	int x, y;

	length = tms->dest - tms->destbuf;
	if ( length == 0 ) return;

	if ( tms->changefont ){
		tms->changefont = FALSE;
		if ( tms->hFont != NULL ){
			SelectObject(tms->hMDC, tms->hOldFont);
			DeleteObject(tms->hFont);
		}
		tms->hFont = CreateFontIndirect(&tms->usefont.font);
		tms->hOldFont = SelectObject(tms->hMDC, tms->hFont);
		GetTextMetrics(tms->hMDC, &tms->tm);
		tms->tm.tmHeight -= tms->tm.tmInternalLeading;
	}

	GetTextExtentPoint32W(tms->hMDC, tms->destbuf, length, &textsize);
	x = (tms->iconsize - textsize.cx) / 2;
	y = (tms->iconsize - tms->tm.tmHeight) / 2;
	switch ( tms->locate ){
		case 1: // 上
			y = 0;
			break;

		case 2: // 右上
			x = x * 2;
			y = 0;
			break;

		case 3: // 右
			x = x * 2;
			break;

		case 4: // 右下
			x = x * 2;
			y = y * 2;
			break;

		case 5: // 下
			y = y * 2;
			break;

		case 6: // 左下
			x = 0;
			y = y * 2;
			break;

		case 7: // 左
			x = 0;
			break;

		case 8: // 左上
			x = y = 0;
			break;
//		default: /* case 0: 中央(大) case 9: 中央(小) )*/
	}

	if ( x < 0 ) x = 0;
	if ( y < 0 ) y = 0;
	TextOutW(tms->hMDC, tms->baseX + x, y , tms->destbuf, length);
	tms->dest = tms->destbuf;
}

PPXDLL HBITMAP PPXAPI CreateScriptBitmap(const TCHAR **source, UINT iconsize, UINT iconwidth, int icontype)
{
	TEXTMAKERSTRUCT tms;
	BITMAPINFO bmpinfo;
	HDC hDC;
	HGDIOBJ hOldBmp;
	void *lpBits;
	COLORREF defcolor;
	HBITMAP hBmp;
	const TCHAR *script = *source;

	InitSysColors();

	hDC = GetDC(NULL);
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biWidth = iconwidth;
	bmpinfo.bmiHeader.biHeight = iconsize;
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = (WORD)((OSver.dwMajorVersion < 6) ? 24 : 32);
	bmpinfo.bmiHeader.biCompression = BI_RGB;
	bmpinfo.bmiHeader.biSizeImage = 0;
	bmpinfo.bmiHeader.biClrUsed = 0;
	bmpinfo.bmiHeader.biClrImportant = 0;

	hBmp = CreateDIBSection(hDC, &bmpinfo, DIB_RGB_COLORS, &lpBits, NULL, 0);
	if ( hBmp == NULL ) goto end;

	tms.hMDC = CreateCompatibleDC(hDC);
	hOldBmp = SelectObject(tms.hMDC, hBmp);
	SetBkMode(tms.hMDC, TRANSPARENT);

	tms.hFont = NULL;
	tms.baseX = 0;
	tms.locate = 0;
	tms.iconsize = iconsize;
	tms.dest = tms.destbuf;
	tms.changefont = TRUE;

	if ( icontype == ICONTYPE_FILEICON ){
		GetPPxFont(PPXFONT_F_mes, 0, &tms.usefont);
		defcolor = GetSchemeColor(C_S_MES, 0);
		FillIcon(&tms, bmpinfo.bmiHeader.biWidth, GetSchemeColor(C_S_BACK, 0));
	}else { // ICONTYPE_TOOLBAR
		tms.usefont.font.lfWidth = 0;
		tms.usefont.font.lfEscapement = 0;
		tms.usefont.font.lfOrientation = 0;
		tms.usefont.font.lfWeight = FW_NORMAL;
		tms.usefont.font.lfItalic = FALSE;
		tms.usefont.font.lfUnderline = FALSE;
		tms.usefont.font.lfStrikeOut = FALSE;
		tms.usefont.font.lfCharSet = DEFAULT_CHARSET;
		tms.usefont.font.lfOutPrecision = OUT_DEFAULT_PRECIS;
		tms.usefont.font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		tms.usefont.font.lfQuality = DEFAULT_QUALITY;
		tms.usefont.font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		tstrcpy(tms.usefont.font.lfFaceName, T("Segoe MDL2 Assets"));

		defcolor = GetRValue(C_DialogBack) + GetGValue(C_DialogBack) + GetBValue(C_DialogBack);
		if ( defcolor < 0x140 ){
			defcolor = 0xd0d0d0;
		}else if ( defcolor < 0x180 ){
			defcolor = 0xffffff;
		}else if ( defcolor < 0x1a0 ){
			defcolor = 0x000000;
		}else{
			defcolor = 0x202020;
		}
		FillIcon(&tms, bmpinfo.bmiHeader.biWidth, C_DialogBack);
	}

	tms.usefont.font.lfHeight = iconsize;
	SetTextColor(tms.hMDC, defcolor);

	for (;;){
		TCHAR chr;

		chr = *script;
		if ( chr == '\0' ) break;
		script++;
		if ( chr == '>' ) break;
		if ( (UTCHAR)chr <= ' ' ) continue;
		switch (chr){
			case ',': // 次のアイコン描画
				DrawChar(&tms);
				tms.baseX += iconsize;
				continue;

			case 'z': // アイコンサイズ(ここでは解釈しない)
			case '#': // デフォルト画像指定(ここでは解釈しない)
				GetDigitNumber(&script);
				continue;

			case '\'': // 文字列
				for (;;){
					chr = *script;
					if ( chr == '\0' ) break;
						script++;
					if ( chr == '\'' ) break;
					if ( tms.dest < (tms.destbuf + MAKEBUFSIZE - 1) ){
						*tms.dest++ = (WCHAR)chr;
					}
				}
				continue;

			case 'b': { // 背景色
				TCHAR sep, all;
				COLORREF color;

				DrawChar(&tms);
				sep = all = *script;
				if ( all == 'a' ) sep = *(++script);
				if ( (sep == 'd') || (sep == 'g') ){
					color = C_DialogBack;
					script++;
				}else{
					if ( sep == '\'' ) script++;
					color = GetColor(&script, TRUE);
					if ( (sep == '\'') && (*script == sep) ) script++;
				}
				FillIcon(&tms, (all == 'a') ?
						bmpinfo.bmiHeader.biWidth - tms.baseX : iconsize,
						color);
				continue;
			}

			case 'c': { // 文字色
				TCHAR sep;
				COLORREF color;

				DrawChar(&tms);
				sep = *script;
				if ( sep == 'g' ){
					color = C_DialogText;
					script++;
				}else if ( sep == 'd' ){
					color = defcolor;
					script++;
				}else{
					if ( sep == '\'' ) script++;
					color = GetColor(&script, TRUE);
					if ( (sep == '\'') && (*script == sep) ) script++;
				}
				SetTextColor(tms.hMDC, color);
				continue;
			}

			case 'F': // 記号フォント指定
				tms.usefont.font.lfCharSet = SYMBOL_CHARSET;
				// 'f' へ
			case 'f': { // フォント指定
				TCHAR chr, *dest;

				DrawChar(&tms);
				chr = *script;
				if ( chr == '\'' ) script++;
				dest = tms.usefont.font.lfFaceName;
				for (;;){
					chr = *script;
					if ( chr == '\0' ) break;
					script++;
					if ( chr == '\'') break;
					if ( dest < (tms.usefont.font.lfFaceName + LF_FACESIZE - 1) ){
						*dest++ = chr;
					}
				}
				*dest = '\0';
				tms.changefont = TRUE;
				continue;
			}

			case 'u': { // unicode 文字コード指定
				int code;

				code = 0;
				for (;;){
					chr = *script;
					if ( Isdigit(chr) ){
						code = (code * 10) + chr - '0';
						script++;
					}else{
						break;
					}
				}
				if ( code >= 0x10000 ){ // サロゲートペア
					if ( tms.dest >= (tms.destbuf + MAKEBUFSIZE - 1) ) continue;
					*tms.dest++ = (WCHAR)(DWORD)((code >> 10) + (0xd800 - (0x10000 >> 10)));
					code = (int)(DWORD)((code & 0x3ff) | 0xdc00);
				}
				if ( tms.dest >= (tms.destbuf + MAKEBUFSIZE - 1) ) continue;
				*tms.dest++ = (WCHAR)code;
				continue;
			}

			case 'p': { // 描画位置
				int newlocate;

				DrawChar(&tms);
				newlocate = *script++ - '0';
				if ( tms.locate != newlocate ){
					if ( tms.locate == 0 ){ // 全体→部分に
						tms.changefont = TRUE;
						tms.usefont.font.lfHeight = iconsize / 2;
					}else if ( newlocate == 0 ){ // 部分→全体に
						tms.changefont = TRUE;
						tms.usefont.font.lfHeight = iconsize;
					}
					tms.locate = newlocate;
				}
				continue;
			}

			default:
				if ( tms.dest < (tms.destbuf + MAKEBUFSIZE - 1) ){
					*tms.dest++ = (WCHAR)chr;
				}
		}
	}
	DrawChar(&tms);
	if ( tms.hFont != NULL ){
		SelectObject(tms.hMDC, tms.hOldFont);
		DeleteObject(tms.hFont);
	}
	SelectObject(tms.hMDC, hOldBmp);
	DeleteDC(tms.hMDC);

	if ( (icontype == ICONTYPE_TOOLBAR_O) &&
		 (bmpinfo.bmiHeader.biBitCount == 32) ){
		BYTE *ptr;
		size_t length;
		ptr = (BYTE *)lpBits + 3;
		length = bmpinfo.bmiHeader.biWidth * bmpinfo.bmiHeader.biHeight;
		while ( length > 0 ){
			*ptr = 0xff;
			ptr += sizeof(COLORREF);
			length--;
		}
	}

	*source = script;
end:
	ReleaseDC(NULL, hDC);
	return hBmp;
}

#define ToolBarScript_icons 48
const TCHAR ToolBarScript[] =
	T("u59179,") // T("u11207,") // 前に戻る
	T("u59178,") // T("u11208,") // 次に進む
	T("u59188,") //  T("u9734,") // お気に入り
	T("u59188p2u59152p0,") // T("u9734p2u10010p0,") // お気に入りに追加
	T("u59575,") // T("u128479,") // フォルダツリー
	T("u59590,") // T("u9988,") // 切り取り
	T("u59592,") // T("u128458,") // コピー
	T("u59263,") // T("u128203,") // 貼り付け
	T("u59303,") // T("u8634,") // 前に戻る
	T("u59302,") // T("u8635,") // やり直す
	T("u59213,") // T("u10060,") // 削除
	T("u59331p2u59152p0,") // 新規ファイル
	T("u59448,") // T("u128449,") // フォルダを開く
	T("u60356,") // T("u128427,") // 上書き保存 / ドライブ選択
	T("u59169,") // T("u128441p1p0u128270,") // 検索
	T("u59718,") // T("u128441p1p0u10004,") // プロパティ
	T("u59543,") // T("?,") // ヘルプ
	T("u59169,") // T("u128270,") // 検索
	T("u59169,") // T("u128270,") // エクスプローラで検索
	T("u59209,") // T("u128424,") // 印刷
	T("u59165,") // T("u128479,") // 表示形式
	T("u59165,") // T("u128479,") // 表示形式
	T("u59165,") // T("u128479,") // 表示形式
	T("u59165,") // T("u128479,") // 表示形式
	T("p3u59600p8Ap6Z,") // T("p3u129067p8Ap6Z,") // 名前順
	T("p3u59600p8u59331p6u128441,") // T("p3u129067p8u128441p6u128458,") // 大きさ順
	T("p0u59600,") // T("p3u129067p7u128338,") // 日時順
	T("u59600,") // T("p3u129067p7u128441p0,") // ソート
	T("u59210,") // T("u128449p1u8682p0,") // 親へ移動
	T("u59598,") // T("u128436p2u10010p0,") // ドライブ追加
	T("u59597,") // T("u128436p2-p0,") // ドライブ削除
	T("u59636,") // T("u128448p2u10010p0,") // 新規フォルダ
	T("u59165,") // T("u128479,") // 表示形式
	T("u59543,") // 不明
	T("u59543,") // 不明
	T("u59264,") // T("u128241,") // 電話帳
	T("u59626,") // T("u128242,") // 携帯電話
	T("u59165,") // T("u128479,") // 表示形式
	T("u59165,") // T("u128479,") // 表示形式
	T("u59165,") // T("u128479,") // 表示形式
	T("u59165,") // T("u128479,") // 表示形式
	T("u59331p2u59152p0,") // T("u128441p2u10010p0,") // 新規ファイル
	T("u59331p2u59152p0,") // T("u128441p2u10010p0,") // 新規ファイル
	T("u59575,") // T("u128479,") // フォルダツリー
	T("u59614,") // T("p7u128449p3u128449p9-,") // 移動
	T("u62483,") // T("p7u128449p3u128449p9+p0,") // コピー
	T("u59663,") // T("u128295,") // カスタマイズ
	T("u59663"); // T("u128295,") // カスタマイズ

#define USEWINRES 1
PPXDLL void PPXAPI LoadToolBarBitmap(HWND hWnd, const TCHAR *bmpname, TBADDBITMAP *tbab, SIZE *bmpsize)
{
	int IconSize; // デスクトップアイコンのサイズ(ツールバーサイズの２倍)
	DWORD monitordpi;
	HTBMP hTbmp;
	int d;
	int mode = 0;
#if USEWINRES
	HDC hDC;
	int videocolorbits;

	hDC = GetDC(hWnd);
	videocolorbits = GetDeviceCaps(hDC, BITSPIXEL);
	ReleaseDC(hWnd, hDC);
#endif
	// bitmap の高さを決定
	bmpsize->cy = 16;
	IconSize = GetSystemMetrics(SM_CXICON);
	monitordpi = GetMonitorDPI(hWnd);
	if ( (bmpname != NULL) && (bmpname[0] == '<') ){
		const TCHAR *s;

		s = bmpname + 1;
		if ( *s == 'z' ){
			s++;
			d = GetDigitNumber(&s);
			if ( d > 0 ) IconSize = d * 2;
		}
		if ( *s == '#' ){
			s++;
			mode = GetDigitNumber(&s);
		}
	}
	if ( (X_dss & DSS_ICON) && (monitordpi > 105) ){
		int minsize = CalcMinDpiSize(monitordpi, X_tous[tous_BARSIZE]);

		IconSize = (IconSize * (int)monitordpi) / DEFAULT_WIN_DPI;
		if ( IconSize < minsize ) IconSize = minsize;
	}
	if ( bmpname == NULL ){
		bmpsize->cy = IconSize / 2;
		return;
	}

	// bmpファイル有り
	if ( (bmpname[0] != '<') || (mode == 2) ){
		if ( bmpname[0] != '\0' ){
			if ( IsTrue(LoadBMP(&hTbmp, bmpname, BMPFIX_TOOLBAR)) ){
				FixToolBarBitmap(hWnd, &hTbmp, tbab, bmpsize, IconSize);
				return;
			}
		}
#if USEWINRES
		// Windows から取得
		if ( videocolorbits > 16 ){
			HBITMAP hBMP;
			if ( WinType <= WINTYPE_VISTA ){
										// Windows XP
				if ( WinType < WINTYPE_VISTA ){
					hBMP = LoadBitmap(hShell32, MAKEINTRESOURCE(217));
					if ( hBMP != NULL ){
						bmpsize->cx = 752;
						tbab->hInst = NULL;
						tbab->nID   = (UINT_PTR)hBMP;
						return;
					}
				}else{
										// Windows Vista
					hBMP = LoadBitmap(hShell32, VistaBitmapBar);
					if ( hBMP != NULL ){
						bmpsize->cx = 768;
						tbab->hInst = NULL;
						tbab->nID   = (UINT_PTR)hBMP;
						return;
					}
				}
			}else{
										// Windows 7, 8, 10
				if ( hIEframe == NULL ){
					hIEframe = LoadLibraryEx(T("IEFRAME.DLL"), NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
				}
				if ( hIEframe != NULL ){
					if ( IconSize >= 40 ){
						hBMP = LoadBitmap(hIEframe, MAKEINTRESOURCE(215));
						if ( hBMP != NULL ){
							bmpsize->cx = 1152;
							bmpsize->cy = 24;
							tbab->hInst = NULL;
							tbab->nID   = (UINT_PTR)hBMP;
							if ( IconSize != (24 * 2) ){
//								UINT gdidpi;

								bmpsize->cx = (1152 * IconSize) / (24 * 2);
								bmpsize->cy = IconSize / 2;
/*
							gdidpi = GetGDIdpi(NULL);
		if ( (OSver.dwBuildNumber >= WINBUILD_10_RS2) &&
			((monitordpi == gdidpi) && (gdidpi != DEFAULT_WIN_DPI)) ){ // Win10 RS2 以降は、既に補正がかかっているので、其の分調整
			bmpsize->cx = (bmpsize->cx * DEFAULT_WIN_DPI) / gdidpi;
			bmpsize->cy = (bmpsize->cy * DEFAULT_WIN_DPI) / gdidpi;
		}
*/
								tbab->nID = (UINT_PTR)CopyImage(hBMP, IMAGE_BITMAP, bmpsize->cx, bmpsize->cy, LR_COPYDELETEORG | LR_COPYRETURNORG);
							}
							return;
						}
					}
					hBMP = LoadBitmap(hIEframe, MAKEINTRESOURCE(217));
					if ( hBMP != NULL ){
						bmpsize->cx = 768;
						tbab->hInst = NULL;
						tbab->nID   = (UINT_PTR)hBMP;
						return;
					}
				}
			}
		}
#endif
	}

	if ( (mode != 1) && ((OSver.dwMajorVersion >= 10) || (bmpname[0] == '<')) ){ // Windows 10/11 以降
		const TCHAR *script = ToolBarScript;
		int iconsize = IconSize / 2;

		if ( (mode != 3) && ((bmpname[0] == '<') && ((bmpname[1] != '\0' ) && (bmpname[1] != '>'))) ){
			script = bmpname + 1;
		}

		bmpsize->cy = iconsize;
		bmpsize->cx = iconsize * ToolBarScript_icons;

		tbab->hInst = NULL;
		tbab->nID = (UINT_PTR)CreateScriptBitmap(&script, iconsize, bmpsize->cx, ICONTYPE_TOOLBAR);
		return;
	}

	// 内蔵BMP
	if ( IsTrue(LoadResBMP(&hTbmp, DLLhInst, MAKEINTRESOURCE(TOOLBARIMAGE), BMPFIX_TOOLBAR)) ){
		FixToolBarBitmap(hWnd, &hTbmp, tbab, bmpsize, IconSize);
		return;
	}

	// ※tbab->nID に直接 HBITMAP を載せると、紫色の背景色変換が効かない
	tbab->hInst = DLLhInst;
	tbab->nID   = TOOLBARIMAGE;
	bmpsize->cx = 752;
}

const TCHAR TOOLBARPROCNAME[] = T("bmpproc");

typedef struct {
	WNDPROC OldProc;
	HBITMAP barbitmap;
} ToolBarProcStruct;

LRESULT CALLBACK ToolBarProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	ToolBarProcStruct *tbps;
	WNDPROC OldProc;

	tbps = (ToolBarProcStruct *)GetProp(hWnd, TOOLBARPROCNAME);
	if ( tbps == NULL ) return DefWindowProc(hWnd, iMsg, wParam, lParam);
	OldProc = tbps->OldProc;
	if ( iMsg == WM_DESTROY ){
		RemoveProp(hWnd, TOOLBARPROCNAME);
		DeleteObject(tbps->barbitmap);
		HeapFree(DLLheap, 0, tbps);
	}
	return CallWindowProc(OldProc, hWnd, iMsg, wParam, lParam);
}

struct cmdstrstruct { // BCC で "\x9f～" ができない対策
	TCHAR precode;
	TCHAR str[3];
} BarAddMenuString = {EXTCMD_CMD, T("%M")};

// style を追加可能に。 WS_MINIMIZE : ボタンに画像を使用しない
PPXDLL HWND PPXAPI CreateToolBar(ThSTRUCT *thCmd, HWND hParentWnd, UINT *ID, const TCHAR *custname, const TCHAR *currentpath, DWORD style)
{
	HWND hTBWnd;
	TBADDBITMAP tbab;
	TCHAR imagepath[VFPS], *p;
	SIZE bmpsize;
	int buttons;
	int index = 0;
	TBBUTTON *tb, *tbp;
	TCHAR MenuString[16];
	TCHAR tiptext[CMDLINESIZE];
	TOOLBARCUSTTABLESTRUCT tbs;
	int bsize;
	ThSTRUCT buttontext;
	int textindex = 0;
	char *tabledata = (char *)&tbs;
	DWORD tablesize = sizeof(tbs);

	ThInit(&buttontext);

	buttons = CountCustTable(custname);
	if ( buttons <= 0 ) return NULL;
	imagepath[0] = '\0';
								// ツールバーに登録するボタンの配列を作成する
	tbp = tb = HeapAlloc(ProcHeap, 0, sizeof(TBBUTTON) * buttons);
	if ( tbp == NULL ) return NULL;

	if ( custname[0] == 'M' ){
		tabledata = (char *)tbs.text;
		tablesize = sizeof(tbs.text);
		tstrcpy(MenuString + (sizeof(WORD) / sizeof(TCHAR)), (TCHAR *)&BarAddMenuString);
	}

	for ( ; ; ){
		bsize = EnumCustTable(index++, custname, tiptext, tabledata, tablesize);
		if ( bsize < 0 ) break;

		if ( tstrcmp(tiptext, T("@")) == 0 ){ // Bitmap
			TCHAR *name;

			name = (custname[0] == 'B') ? tbs.text + 1 : tbs.text;
			if ( *name != '<' ){
				VFSFixPath(imagepath, name, currentpath, VFSFIX_FULLPATH | VFSFIX_REALPATH);
			}else{
				tstrcpy(imagepath, name);
			}
			buttons--;
			continue;
		}

		tbp->fsState = TBSTATE_ENABLED;
		tbp->bReserved[0] = 0;

		if ( (tiptext[0] == '-') && (tiptext[1] == '-') &&
			 ((tiptext[2] == '\0') || (tiptext[2] == '.')) ){ // --. 区切り線
			tbp->iBitmap = 0;
			tbp->fsStyle = TBSTYLE_SEP;
			tbp++;
			continue;
		}

		tbp->idCommand = (*ID)++;

		if ( custname[0] == 'B' ){
			tbp->iBitmap = tbs.index;
			tbp->fsStyle = TBSTYLE_BUTTON;

			if ( (UTCHAR)tbs.text[0] != EXTCMD_CMD ){
				switch(*(WORD *)(&tbs.text[1])){
					case K_raw | K_c | K_lf:
					case K_raw | K_c | K_ri:
					case K_raw | K_bs:
						tbp->fsStyle = TBSTYLE_DROPDOWN;
						break;
					//default: なし
				}
			}

			ThAppend(thCmd, &bsize, sizeof(WORD)); // Size登録
			tbp->dwData = thCmd->top;	// Cmdの位置を記憶
			ThAppend(thCmd, tbs.text, bsize);	// Cmd登録

			PP_ExtractMacro(hParentWnd, NULL, NULL, tiptext, tiptext, XEO_DISPONLY);

			p = tstrchr(tiptext, '/'); // バー表示用のテキストがあるか
			if ( p == NULL ){
				p = tiptext;
				tbp->iString = 0;
			}else{
				*p++ = '\0';	// チップテキスト抽出
				ThAddString(&buttontext, tiptext);
				tbp->iString = textindex++;
				setflag(tbp->fsStyle, BTNS_SHOWTEXT | TBSTYLE_AUTOSIZE);
			}
			ThAddString(thCmd, p);
		}else{ // Menu
			DWORD extsize;

			tbp->iBitmap = -2; // I_IMAGENONE
			tbp->fsStyle = TBSTYLE_BUTTON | BTNS_SHOWTEXT | TBSTYLE_AUTOSIZE;

			extsize = (tbs.text[0] == '?') ? // ?メニュー
					sizeof(BarAddMenuString) - sizeof(TCHAR) : // Nil 除く
					sizeof(BarAddMenuString) - 3 * sizeof(TCHAR); // %M + Nil 除く
			*(WORD *)MenuString = (WORD)(DWORD)(bsize + extsize);

			tbp->dwData = thCmd->top + sizeof(WORD); // Cmdの位置を記憶
			ThAppend(thCmd, MenuString, sizeof(WORD) + extsize); // bsize + 追加登録
			ThAppend(thCmd, tabledata, bsize);	// Cmd登録

			PP_ExtractMacro(hParentWnd, NULL, NULL, tiptext, tiptext, XEO_DISPONLY);
			ThAddString(&buttontext, tiptext);
			tbp->iString = textindex++;

			ThAddString(thCmd, tiptext);
		}
		tbp++;
	}
	if ( style & WS_MINIMIZE ){
		tbab.nID = 0;
	}else{						// ツールバーのイメージを取得する
		LoadToolBarBitmap(hParentWnd, imagepath, &tbab, &bmpsize);
	}
								// ツールバーウィンドウを作成する
	LoadCommonControls(ICC_BAR_CLASSES);
	// 編集可能にする : CCS_ADJUSTABLE | TBSTYLE_ALTDRAG
	style |= WS_CHILD | CCS_NOPARENTALIGN | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER;
	if ( textindex ) setflag(style, TBSTYLE_LIST);	// テキスト表示に必要
	hTBWnd = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, style & ~WS_MINIMIZE,
		0, 0, 0, 0, hParentWnd, CHILDWNDID(IDW_GENTOOLBAR), GetModuleHandle(NULL), NULL);
	if ( hTBWnd != NULL ){
		FixUxTheme(hTBWnd, TOOLBARCLASSNAME);
//		if ( X_dss & DSS_COMCTRL ) SendMessage(hTBWnd, CCM_DPISCALE, TRUE, 0); // GDI スケール用
		SendMessage(hTBWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

		if ( textindex ){
			SendMessage(hTBWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
			// buttontext は、ThAddString により必ず末尾が \0\0 になっている
			SendMessage(hTBWnd, TB_ADDSTRING, 0, (LPARAM)buttontext.bottom);
			ThFree(&buttontext);
		}else{
			SendMessage(hTBWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
		}
		if ( tbab.nID != 0 ){
			ToolBarProcStruct *tbps;

			SendMessage(hTBWnd, TB_SETBITMAPSIZE, 0, TMAKELPARAM(bmpsize.cy, bmpsize.cy));
			SendMessage(hTBWnd, TB_ADDBITMAP, bmpsize.cx / bmpsize.cy, (WPARAM)&tbab);

			tbps = HeapAlloc(DLLheap, 0, sizeof(ToolBarProcStruct));
			tbps->barbitmap = (HBITMAP)tbab.nID;
			tbps->OldProc = (WNDPROC)
				SetWindowLongPtr(hTBWnd, GWLP_WNDPROC, (LONG_PTR)ToolBarProc);
			SetProp(hTBWnd, TOOLBARPROCNAME, (HANDLE)tbps);
		}
		// 使用ビットマップはウィンドウ廃棄時廃棄する？
		SendMessage(hTBWnd, TB_ADDBUTTONS, buttons, (LPARAM)tb);
		SendMessage(hTBWnd, TB_AUTOSIZE, 0, 0);
		ShowWindow(hTBWnd, SW_SHOW);
	}
	HeapFree(ProcHeap, 0, tb);
	return hTBWnd;
}
PPXDLL BOOL PPXAPI SetToolBarTipText(HWND hToolbarWnd, ThSTRUCT *thCmd, NMHDR *nmh)
{
	BYTE *p;
	TBBUTTON tb;
	UINT ID;

//	if ( nmh->hwndFrom != TabCtrl_GetToolTips(hToolbarWnd) ) return FALSE;
	ID = (UINT)SendMessage(hToolbarWnd, TB_COMMANDTOINDEX, nmh->idFrom, 0);
	if ( (int)ID < 0 ) return FALSE;
	if ( !SendMessage(hToolbarWnd, TB_GETBUTTON, ID, (LPARAM)&tb) ) return FALSE;
	if ( (thCmd->bottom == NULL) || (tb.dwData == 0) || (tb.dwData >= thCmd->top) ) return FALSE;
	p = (BYTE *)(thCmd->bottom + tb.dwData);
	((LPTOOLTIPTEXT)nmh)->lpszText = (TCHAR *)(BYTE *)(p + *(WORD *)(p - 2));
	((LPTOOLTIPTEXT)nmh)->hinst = NULL;
	return TRUE;
}

PPXDLL TCHAR * PPXAPI GetToolBarCmd(HWND hToolbarWnd, ThSTRUCT *thCmd, UINT ID)
{
	TBBUTTON tb;

	ID = (UINT)SendMessage(hToolbarWnd, TB_COMMANDTOINDEX, ID, 0);
	if ( (int)ID < 0 ) return NULL;
	if ( !SendMessage(hToolbarWnd, TB_GETBUTTON, ID, (LPARAM)&tb) ) return NULL;
	SendMessage(hToolbarWnd, TB_GETBUTTON, ID, (LPARAM)&tb);
	return (TCHAR *)(BYTE *)(thCmd->bottom + tb.dwData);
}

void PopupErrorMessage(HWND hWnd, const TCHAR *title, const TCHAR *msg)
{
	DWORD Pid = 0;

	GetWindowThreadProcessId(hWnd, &Pid);

	if ( (Pid == GetCurrentProcessId()) && (SendMessage(hWnd, WM_PPXCOMMAND, TMAKEWPARAM(K_SETPOPMSG, POPMSG_MSG), (LPARAM)msg) != 1) ){
		XMessage(hWnd, title, XM_GrERRld, T("%s"), msg);
	}
}

void FillBox(HDC hDC, const RECT *box, HBRUSH hbr)
{
	HGDIOBJ hOldObj;

	hOldObj = SelectObject(hDC, hbr);
	PatBlt(hDC, box->left, box->top, box->right - box->left, box->bottom - box->top, PATCOPY);
	SelectObject(hDC, hOldObj);
}

//-----------------------------------------------------------------------------
#define CCH_RM_SESSION_KEY (sizeof(GUID) * 2)
#define CCH_RM_MAX_APP_NAME 255
#define CCH_RM_MAX_SVC_NAME 63
#define RmRebootReasonNone 0
typedef struct {
	DWORD dwProcessId;
	FILETIME ProcessStartTime;
} RM_UNIQUE_PROCESS;

typedef struct {
	RM_UNIQUE_PROCESS Process;
	WCHAR strAppName[CCH_RM_MAX_APP_NAME + 1];
	WCHAR strServiceShortName[CCH_RM_MAX_SVC_NAME + 1];
	int /*RM_APP_TYPE*/ ApplicationType;
	ULONG AppStatus;
	DWORD TSSessionId;
	BOOL bRestartable;
} RM_PROCESS_INFO;

DefineWinAPI(ERRORCODE, RmStartSession, (DWORD *pSessionHandle, DWORD dwSessionFlags, WCHAR *strSessionKey));
DefineWinAPI(ERRORCODE, RmRegisterResources, (DWORD dwSessionHandle, UINT nFiles, LPCWSTR *rgsFileNames, UINT nApplications, RM_UNIQUE_PROCESS rgApplications[], UINT nServices, LPCWSTR *rgsServiceNames));
DefineWinAPI(ERRORCODE, RmGetList, (DWORD dwSessionHandle, UINT *pnProcInfoNeeded, UINT *pnProcInfo, RM_PROCESS_INFO *rgAffectedApps, LPDWORD lpdwRebootReasons));
DefineWinAPI(ERRORCODE, RmEndSession, (DWORD dwSessionHandle));

LOADWINAPISTRUCT RSTRTMGRDLL[] = {
	LOADWINAPI1(RmStartSession),
	LOADWINAPI1(RmRegisterResources),
	LOADWINAPI1(RmGetList),
	LOADWINAPI1(RmEndSession),
	{NULL, NULL}
};

PPXDLL int PPXAPI GetAccessApplications(const TCHAR *checkpath, TCHAR *text)
{
	WCHAR SessionKey[CCH_RM_SESSION_KEY + 1];
	DWORD hRm;
	int infocount = 0;
	HANDLE hRSTRTMGR = LoadSystemWinAPI(SYSTEMDLL_RSTRTMGR, RSTRTMGRDLL);
	BOOL header = TRUE;

#ifdef UNICODE
	#define path checkpath
#else
	WCHAR path[VFPS];
#endif
	*text = '\0';
	if ( hRSTRTMGR == NULL ) return -1;

	if ( *checkpath == '\1' ){
		header = FALSE;
		checkpath++;
	}
#ifndef UNICODE
	AnsiToUnicode(checkpath, path, VFPS);
#endif

	strcpyW(SessionKey, L"PPxVFS");
	if ( NO_ERROR == DRmStartSession(&hRm, 0, SessionKey) ){
		LPCWSTR PathPtr = path;

		if ( NO_ERROR == DRmRegisterResources(hRm, 1, &PathPtr, 0, NULL, 0, NULL) ){
			UINT pnProcInfoNeeded, pnProcInfo = 0;
			DWORD lpdwRebootReasons = RmRebootReasonNone;

			if ( ERROR_MORE_DATA == DRmGetList(hRm, &pnProcInfoNeeded,
					&pnProcInfo, NULL, &lpdwRebootReasons) ){ // 数計算
				RM_PROCESS_INFO *processInfo;

				infocount = pnProcInfo = pnProcInfoNeeded;
				processInfo = HeapAlloc(ProcHeap, HEAP_ZERO_MEMORY,
						sizeof(RM_PROCESS_INFO) * pnProcInfoNeeded);

				if ( processInfo != NULL ) {
					if ( (pnProcInfo > 0) &&
						 (NO_ERROR == DRmGetList(hRm, &pnProcInfoNeeded,
							&pnProcInfo, processInfo, &lpdwRebootReasons)) ) {
						DWORD i;
						TCHAR *textmax = text + VFPS;

						if ( header ){
							tstrcpy(text, MessageText(MES_ADHD));
							text += tstrlen(text);
						}
						for ( i = 0; i < pnProcInfo; i++ ) {
							if ( i != 0 ) *text++ = '\r';
							text = thprintf(text, 16, T(" %d: "), processInfo[i].Process.dwProcessId);
							strcpyWToT(text, processInfo[i].strAppName, min(MAX_PATH, (textmax - text)) );
							text += tstrlen(text);
							if (text >= textmax) break;
						}
						if ( infocount - i ){
							thprintf(text, MAX_PATH, MessageText(MES_ACET), infocount - i);
						}
					}
					HeapFree(ProcHeap, 0, processInfo);
				}
			}
		}
		DRmEndSession(hRm);
	}
	FreeLibrary(hRSTRTMGR);
	return infocount;
}

// C_Scheme1 は再帰が起きるため使用できない
#define PRESETCOLOR(Color, newcolor) {if ( Color >= C_Scheme2_MIN ) Color = GetSchemeColor(Color, newcolor);}
#define DEFWINCOLOR(Color, Type) {if ( Color >= C_Scheme2_MIN ) Color = GetSchemeColor(Color, GetSysColor(Type));}

void InitSysColors_main(void)
{
	ThemeColors.ExtraDrawFlags = 0;
	GetCustData(T("X_vclr"), &X_vclr, sizeof(X_vclr));
	GetCustData(T("C_win"), &ThemeColors.c, sizeof(ThemeColors.c));
/*
	if ( C_WindowText == C_AUTO ){ // Ver1.76 カラーテーマの設定ミス対策
		if ( C_WindowBack == 0x202020 ) C_WindowText = C_Win_Dark.WindowText;
		if ( C_WindowBack == 0xffffff ) C_WindowText = C_BLACK;
	}
*/
	if ( X_uxt_color >= UXT_MINPRESET ){
		const C_WIN_COLORS *cw;

		cw = (X_uxt_color == UXT_LIGHT) ? &C_Win_Light : &C_Win_Dark; // UXT_LIGHT / UXT_DARK 切り替え
		PRESETCOLOR(C_FrameHighlight, cw->FrameHighlight);
		PRESETCOLOR(C_FrameFace, cw->FrameFace);
		PRESETCOLOR(C_FrameShadow, cw->FrameShadow);
		PRESETCOLOR(C_WindowBack, cw->WindowBack);
		PRESETCOLOR(C_DialogBack, cw->DialogBack);
		PRESETCOLOR(C_WindowText, cw->WindowText);

		PRESETCOLOR(C_GrayText, cw->GrayText);
		PRESETCOLOR(C_DialogText, cw->DialogText);
		PRESETCOLOR(C_EdgeLine, cw->EdgeLine);
		PRESETCOLOR(C_FocusBack, cw->FocusBack);
		ThemeColors.ExtraDrawFlags = EDF_WINDOW_TEXT | EDF_WINDOW_BACK | EDF_DIALOG_TEXT | EDF_DIALOG_BACK;
	}else{
		DEFWINCOLOR(C_FrameHighlight, COLOR_BTNHIGHLIGHT);
		DEFWINCOLOR(C_FrameFace, COLOR_BTNFACE);
		DEFWINCOLOR(C_FrameShadow, COLOR_BTNSHADOW);
		if ( C_WindowBack == C_AUTO ){
			C_WindowBack = GetSysColor(COLOR_WINDOW);
		}else{
			setflag(ThemeColors.ExtraDrawFlags, EDF_WINDOW_BACK);
		}
		if ( C_DialogBack == C_AUTO ){
			C_DialogBack = GetSysColor(COLOR_BTNFACE);
		}else{
			setflag(ThemeColors.ExtraDrawFlags, EDF_DIALOG_BACK);
		}
		if ( C_WindowText == C_AUTO ){
			C_WindowText = GetSysColor(COLOR_WINDOWTEXT);
		}else{
			setflag(ThemeColors.ExtraDrawFlags, EDF_WINDOW_TEXT);
		}
		DEFWINCOLOR(C_GrayText, COLOR_GRAYTEXT);
		if ( C_DialogText == C_AUTO ){
			C_DialogText = GetSysColor(COLOR_WINDOWTEXT);
			if ( X_uxt_color >= UXT_MINMODIFY ){
				setflag(ThemeColors.ExtraDrawFlags, EDF_DIALOG_BACK);
			}
		}else{
			setflag(ThemeColors.ExtraDrawFlags, EDF_DIALOG_TEXT);
		}
		PRESETCOLOR(C_EdgeLine, COLOR_LIGHT_EDGE_LINE);
		PRESETCOLOR(C_FocusBack, COLOR_LIGHT_FOCUSBACK);
	}
	DEFWINCOLOR(C_HighlightText, COLOR_HIGHLIGHTTEXT);
	DEFWINCOLOR(C_HighlightBack, COLOR_HIGHLIGHT);
#undef PRESETCOLOR
#undef DEFDARKCOLOR
	if ( ThemeColors.ExtraDrawFlags & EDF_DIALOG_BACK ){
		hDialogBackBrush = CreateSolidBrush(C_DialogBack);
	}
}

void FreeSysColors(void)
{
	if ( hFrameHighlight != NULL ){
		DeleteObject(hFrameHighlight);
		hFrameHighlight = NULL;
	}
	if ( hFrameFace != NULL ){
		DeleteObject(hFrameFace);
		hFrameFace = NULL;
	}
	if ( hFrameShadow != NULL ){
		DeleteObject(hFrameShadow);
		hFrameShadow = NULL;
	}
	if ( hHighlightBack != NULL ){
		DeleteObject(hHighlightBack);
		hHighlightBack = NULL;
	}
	if ( hEdgeLine != NULL ){
		DeleteObject(hEdgeLine);
		hEdgeLine = NULL;
	}
	if ( hDialogBackBrush != NULL ){
		DeleteObject(hDialogBackBrush);
		hDialogBackBrush = NULL;
	}
	if ( hWindowBackBrush != NULL ){
		DeleteObject(hWindowBackBrush);
		hWindowBackBrush = NULL;
	}
	if ( hButtonBMP != NULL ){
		DeleteObject(hButtonBMP);
		hButtonBMP = NULL;
	}
	C_FrameFace = C_AUTO;
}

HBRUSH GetFrameHighlightBrush(void)
{
	if ( hFrameHighlight != NULL ) return hFrameHighlight;
	InitSysColors();
	hFrameHighlight = CreateSolidBrush(C_FrameHighlight);
	return hFrameHighlight;
}

HBRUSH GetFrameFaceBrush(void)
{
	if ( hFrameFace != NULL ) return hFrameFace;
	InitSysColors();
	hFrameFace = CreateSolidBrush(C_FrameFace);
	return hFrameFace;
}

HBRUSH GetFrameShadowBrush(void)
{
	if ( hFrameShadow != NULL ) return hFrameShadow;
	if ( C_FrameFace == C_AUTO ) InitSysColors_main();
	hFrameShadow = CreateSolidBrush(C_FrameShadow);
	return hFrameShadow;
}

HBRUSH GetHighlightBackBrush(void)
{
	if ( hHighlightBack != NULL ) return hHighlightBack;
	InitSysColors();
	hHighlightBack = CreateSolidBrush(C_HighlightBack);
	return hHighlightBack;
}

HBRUSH GetEdgeLineBrush(void)
{
	if ( hEdgeLine != NULL ) return hEdgeLine;
	InitSysColors();
	hEdgeLine = CreateSolidBrush(C_EdgeLine);
	return hEdgeLine;
}

HBRUSH GetDialogBackBrush(void)
{
	if ( hDialogBackBrush != NULL ) return hDialogBackBrush;
	InitSysColors();
	hDialogBackBrush = CreateSolidBrush(C_DialogBack);
	return hDialogBackBrush;
}

// 常に EDGE_RAISED
PPXDLL void PPXAPI DrawSeparateLine(HDC hDC, const RECT *box, UINT flags)
{
	RECT facebox, edgebox;

	edgebox = facebox = *box;
	if ( flags & BF_TOP ){
		edgebox.bottom = facebox.top = edgebox.top + 1;
		FillBox(hDC, &edgebox, GetFrameHighlightBrush());
		edgebox.bottom = box->bottom;
	}
	if ( (flags & BF_BOTTOM) && ((facebox.bottom - facebox.top) >= 3) ){
		edgebox.top = facebox.bottom = edgebox.bottom - 1;
		FillBox(hDC, &edgebox, GetFrameShadowBrush());
	}
	if ( flags & BF_LEFT ){
		edgebox.right = facebox.left = edgebox.left + 1;
		FillBox(hDC, &edgebox, GetFrameHighlightBrush());
		edgebox.right = box->right;
	}
	if ( (flags & BF_RIGHT) && ((facebox.right - facebox.left) >= 3) ){
		edgebox.left = facebox.right = edgebox.right - 1;
		FillBox(hDC, &edgebox, GetFrameShadowBrush());
	}
	FillBox(hDC, &facebox, GetFrameFaceBrush());
}

ERRORCODE IsShowButtonMenu(UINT buttonid)
{
	if ( (buttonid != 0) && (ButtonMenuState.ButtonID != buttonid) ){ // 押したボタンが異なるのでok
		ButtonMenuState.ButtonID = buttonid;
		return NO_ERROR;
	}
	ButtonMenuState.ButtonID = buttonid;
	if ( (GetTickCount() - ButtonMenuState.tick) >= SKIPBUTTONMENUTIME ){
		 return NO_ERROR; // 十分に時間経過したのでok
	}
	ButtonMenuState.ButtonID = 0; // 一度押したので次は許可する
	return ERROR_CANCELLED;
}

void EndButtonMenu(void)
{
	ButtonMenuState.tick = (GetAsyncKeyState(VK_LBUTTON) & KEYSTATE_PUSH) ? GetTickCount() : 0;
}

#ifndef WM_UAHDRAWMENU
  #define WM_UAHDRAWMENU 0x0091
  #define WM_UAHDRAWMENUITEM 0x0092
  #define WM_UAHMEASUREMENUITEM  0x0094
#endif

typedef struct {
	HMENU hmenu;
	HDC hdc;
	DWORD dwFlags;
} UAHMENU;

typedef struct
{
	struct { DWORD cx, cy; } rgsize[4];
} UAHMENUITEMMETRICS;

typedef struct
{
	DWORD rgcx[4];
	DWORD fUpdateMaxWidths; // use bits 2;
} UAHMENUPOPUPMETRICS;

typedef struct
{
	int iPosition;
	UAHMENUITEMMETRICS umim;
	UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

typedef struct
{
	DRAWITEMSTRUCT dis;
	UAHMENU um;
	UAHMENUITEM umi;
} UAHDRAWMENUITEM;

#ifndef OBJID_MENU
typedef struct {
  DWORD cbSize;
  RECT  rcBar;
  HMENU hMenu;
  HWND  hwndMenu;
  BOOL  fBarFocused : 1;
  BOOL  fFocused : 1;
  BOOL  fUnused : 30;
} xMENUBARINFO;

#define OBJID_MENU ((LONG)0xFFFFFFFD)
#else
 #define xMENUBARINFO MENUBARINFO
#endif

DefineWinAPI(HRESULT, DrawThemeTextEx, (HANDLE, HDC, int, int, LPCWSTR, int, DWORD, LPRECT, const void *)) = NULL;

DefineWinAPI(BOOL, GetMenuBarInfo, (HWND, LONG, LONG, xMENUBARINFO *));

#define ODS_SELECTED  0x0001
#define ODS_GRAYED    0x0002
#define ODS_DISABLED  0x0004
#define ODS_CHECKED   0x0008
#define ODS_FOCUS     0x0010
#define ODS_HOTLIGHT  0x0040
#define ODS_INACTIVE  0x0080
#define ODS_NOACCEL   0x0100
#define MPI_NORMAL 1
#define MPI_HOT 2
#define MPI_DISABLED 3

#define DT_HIDEPREFIX  0x00100000
#define DTT_TEXTCOLOR  (1UL << 0)
#define DTT_BORDERCOLOR  (1UL << 1)
#define DTT_COMPOSITED  (1UL << 13)
#define DTT_GLOWSIZE  (1UL << 11)

#define MENU_BARITEM 8
#define MBI_NORMAL 1

typedef struct
{
	DWORD  dwSize;
	DWORD  dwFlags;
	COLORREF crText;
	COLORREF crBorder;
	COLORREF crShadow;
	int  iTextShadowType;
	POINT  ptShadowOffset;
	int  iBorderSize;
	int  iFontPropId;
	int  iColorPropId;
	int  iStateId;
	BOOL  fApplyOverlay;
	int  iGlowSize;
	void *pfnDrawTextCallback;
	LPARAM  lParam;
} DTTOPTS;

struct {
	HBRUSH hBack, hHover, hSelected;
} DarkMenuBrush = {NULL, NULL, NULL};

const WCHAR ThemeMenu[] = L"Menu";

PPXDLL LRESULT PPXAPI DarkMenuWndProc(HWND hWnd, HANDLE *hMenuTheme, UINT message, WPARAM wParam, LPARAM lParam)
{
	if ( message == WM_UAHDRAWMENU ){
		if ( DarkMenuBrush.hBack == NULL ){
			DarkMenuBrush.hBack = CreateSolidBrush(C_DialogBack);
			DarkMenuBrush.hHover = CreateSolidBrush(C_FocusBack);
			DarkMenuBrush.hSelected = DarkMenuBrush.hHover;
		}
		if ( DGetMenuBarInfo == NULL ){
			GETDLLPROC(GetModuleHandleA(User32DLL), GetMenuBarInfo);
		}

		if ( DDrawThemeTextEx == NULL ){
			HMODULE hModule = GetModuleHandle(T("UxTheme.dll"));
			if ( hModule != NULL ){
				GETDLLPROC(hModule, OpenThemeData);
				GETDLLPROC(hModule, CloseThemeData);
				GETDLLPROC(hModule, DrawThemeTextEx);
			}
		}

		if ( DGetMenuBarInfo != NULL ){
			RECT rcWindow;
			xMENUBARINFO mbi;

			mbi.cbSize = sizeof(mbi);
			DGetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);
			GetWindowRect(hWnd, &rcWindow);
			mbi.rcBar.left -= rcWindow.left;
			mbi.rcBar.right -= rcWindow.left;
			mbi.rcBar.top -= rcWindow.top;
			mbi.rcBar.bottom -= rcWindow.top;
			FillRect(((UAHMENU*)lParam)->hdc, &mbi.rcBar, DarkMenuBrush.hBack);
			return 0;
		}
	}else if ( (message == WM_UAHDRAWMENUITEM) && (DDrawThemeTextEx != NULL) ){
		if ( *hMenuTheme == NULL ){
			*hMenuTheme = DOpenThemeData(hWnd, ThemeMenu);
		}

		if ( *hMenuTheme != NULL ){
			UAHDRAWMENUITEM *udi = (UAHDRAWMENUITEM*)lParam;
			HBRUSH *hBackBrush = &DarkMenuBrush.hBack;
			WCHAR MenuItem[128];
			MENUITEMINFOW mii;
			DTTOPTS opts;

//			SetBkColor(udi->um.hdc, WinColors.c.DialogBack);
			if ( udi->dis.itemState & ODS_HOTLIGHT ){
				hBackBrush = &DarkMenuBrush.hHover;
//				SetBkColor(udi->um.hdc, WinColors.c.FocusBack);
			}
			if ( udi->dis.itemState & ODS_SELECTED ){
				hBackBrush = &DarkMenuBrush.hSelected;
//				SetBkColor(udi->um.hdc, WinColors.c.FocusBack);
			}
			/*
			if ( (udi->dis.itemState & ODS_GRAYED) ||
				 (udi->dis.itemState & ODS_DISABLED) ){
				iTextStateID = MPI_DISABLED;
			}
			if ( udi->dis.itemState & ODS_NOACCEL ){
				dwFlags |= DT_HIDEPREFIX;
			}
			*/
			FillRect(udi->um.hdc, &udi->dis.rcItem, *hBackBrush);

			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = MenuItem;
			mii.cch = (sizeof(MenuItem) / sizeof(WCHAR)) - 1;
			GetMenuItemInfoW(udi->um.hmenu, udi->umi.iPosition, TRUE, &mii);

			opts.dwSize = sizeof(opts);
			opts.dwFlags = DTT_TEXTCOLOR | DTT_COMPOSITED;
			opts.crText = C_DialogText;
			DDrawThemeTextEx(*hMenuTheme, udi->um.hdc, MENU_BARITEM, MBI_NORMAL,
					MenuItem, mii.cch, DT_CENTER | DT_SINGLELINE | DT_VCENTER,
					&udi->dis.rcItem, &opts);
/*
			SetTextColor(udi->um.hdc, WinColors.c.DialogText);
			DrawTextW(udi->um.hdc, MenuItem, mii.cch, &udi->dis.rcItem, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
*/
			return 0;
		}
	}else if ( (message == WM_NCPAINT) || (message == WM_NCACTIVATE) ){
			RECT rcWindow, rcClient;
			HDC hdc;
			LRESULT lr;

			lr = DefWindowProc(hWnd, message, wParam, lParam);
	 		GetClientRect(hWnd, &rcClient);
	 		ClientToScreen(hWnd, (POINT *)&rcClient);
			GetWindowRect(hWnd, &rcWindow);
			rcClient.left -= rcWindow.left;
			rcClient.right += rcClient.left;
			rcClient.bottom = rcClient.top - rcWindow.top;
			rcClient.top = rcClient.bottom - 1;
			hdc = GetWindowDC(hWnd);
			FillRect(hdc, &rcClient, DarkMenuBrush.hBack);
			ReleaseDC(hWnd, hdc);
			return lr;

	}else if ( (message == WM_THEMECHANGED) && (*hMenuTheme != NULL) ){
		DCloseThemeData(*hMenuTheme);
		*hMenuTheme = NULL;

	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

// 明るさを 0-255000 で表現
#define ColorBrightR 299
#define ColorBrightG 587
#define ColorBrightB 114
#define ColorBright(color) (((int)GetRValue(color) * ColorBrightR) + ((int)GetGValue(color) * ColorBrightG) + ((int)GetBValue(color) * ColorBrightB))
#define ColorVisibleBright 50000
#define ColorVisibleBoost 20000
void VisibledTextColor(COLORREF *text_color, COLORREF back_color)
{
	int text_bright, back_bright, diff_bright;
	int r, g, b;

	text_bright = ColorBright(*text_color);
	back_bright = ColorBright(back_color);

	diff_bright = text_bright - back_bright;
	if ( (diff_bright >= ColorVisibleBright) ||
		 (diff_bright <= -ColorVisibleBright) ){
		return;
	}
//	XMessage(NULL, NULL, XM_DbgLOG, T("color visib %x %d"),*text_color,diff_bright);
	if ( diff_bright > 0 ){ // 文字の明るさ不足
		diff_bright = ColorVisibleBright - diff_bright;
		if ( diff_bright < ColorVisibleBoost ) diff_bright = ColorVisibleBoost;
		r = (int)GetRValue(*text_color) + (diff_bright / ColorBrightR);
		if ( r > 255 ) r = 255;
		g = (int)GetGValue(*text_color) + (diff_bright / ColorBrightG);
		if ( g > 255 ) g = 255;
		b = (int)GetBValue(*text_color) + (diff_bright / ColorBrightB);
		if ( b > 255 ) b = 255;
	}else{ // 文字の暗さ不足
		diff_bright = -ColorVisibleBright - diff_bright;
		if ( diff_bright > -ColorVisibleBoost ) diff_bright = -ColorVisibleBoost;
		r = (int)GetRValue(*text_color) + (diff_bright / ColorBrightR);
		if ( r < 0 ) r = 0;
		g = (int)GetGValue(*text_color) + (diff_bright / ColorBrightG);
		if ( g < 0 ) g = 0;
		b = (int)GetBValue(*text_color) + (diff_bright / ColorBrightB);
		if ( b < 0 ) b = 0;
	}
	*text_color = RGB(r, g, b);
}
