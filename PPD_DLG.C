/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library						ダイアログ関連
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "PPD_DEF.H"
#pragma hdrstop

const TCHAR *ClassAtomString[] = {
	ButtonClassName, EditClassName, StaticClassName, ListBoxClassName,
	ScrollBarClassName, ComboBoxClassName
};

const TCHAR RichText2[] = T("riched20.dll");
#ifdef UNICODE
	const TCHAR RichText5[] = T("msftedit.dll");
	#ifdef _WIN64
		const WCHAR RichTextOffice[] =  L"C:\\Program Files\\Microsoft Office\\root\\vfs\\ProgramFilesCommonX64\\Microsoft Shared\\Office16\\RICHED20.DLL"; // msi install
		// MSPTLS.DLL も必要
		// C:\Program Files\WindowsApps\Microsoft.WindowsNotepad_11.*.*.*_x64__?????????????\Notepad も使用可能
	#else
		const TCHAR RichTextOffice[] = T("C:\\Program Files\\Microsoft Office\\root\\vfs\\ProgramFilesCommonX86\\Microsoft Shared\\Office16\\RICHED20.DLL");
	#endif
	// L"RichEdit"; // V1(95以降) Riched32.dll (UNICODE版無し、現在はRiched20.dllを呼び出す)
	const WCHAR RichTextClass2W[] =  L"RichEdit20W"; // V2(95,98),V3(Me,2000以降) Riched20.dll 750k
	const WCHAR RichTextClass5W[] =  L"RichEdit50W"; // V4.1(XP SP1, Office 2003)
	const WCHAR RichTextClassD2D[] = L"RichEditD2D"; // Office 16, notepad APP(V8)
#else // Multibyte 版は RichEdit V2(V3) まで
	const WCHAR RichTextClass2W[] =   L"RichEdit20A";
	const char RichTextClass2A[] =   "RichEdit20A";
//	const WCHAR RichTextClass5W[] =   L"RichEdit50W"; // ANSI版なし
//	const WCHAR RichTextClassD2D[] = L"RichEditD2D";  // ANSI版なし
#endif

void ShowErrorTip(HWND hWnd, const TCHAR *text, const TCHAR *path, ERRORCODE result)
{
	TCHAR buf[0x400];

	if ( path != NULL ) {
		thprintf(buf, TSIZEOF(buf), T("%s(%s): %Mm"), text, path, result);
	}else{
		thprintf(buf, TSIZEOF(buf), T("%s"), text);
	}
	XMessage(hWnd, NULL, XM_NiERRld, T("%s"), buf);
}

int IsUseRichEdit(int mode)
{
	if ( X_uxt_color == UXT_NA ) InitUnthemeCmd();
	if ( mode < 0 ) mode = X_uxt_rich;
	if ( (mode > UXTRICH_NO) && (hRichEdit == NULL) ){
	#ifdef UNICODE
		if ( mode == UXTRICH_DW ){ // D2D  カラー絵文字使用可能
			hRichEdit = LoadLibrary(RichTextOffice);
			if ( hRichEdit == NULL ){
				TCHAR path[MAX_PATH];
				WNDCLASS wc;

				path[0] = '\0';
				GetCustTable(StrCustOthers, T("RichEditPath"), path, sizeof(path));
				if ( path[0] == '\0' ){
					tstrcpy(path, RichText2);
				}else{
					VFSFullPath(path, (TCHAR *)RichText2, path);
				}
				hRichEdit = LoadLibrary(path);
				if ( hRichEdit != NULL ){
					if ( GetClassInfoW(hRichEdit, RichTextClassD2D, &wc) == FALSE ){
						FreeLibrary(hRichEdit);
						hRichEdit = NULL;
				//		ShowErrorTip(NULL, T("riched20.dll: not support D2D"), NULL, 0);
					}
				}else{
					ShowErrorTip(NULL, T("riched20.dll load error"), path, GetLastError());
				}
			}
		}
		if ( (mode >= UXTRICH_V4) && (hRichEdit == NULL) ){ // 2: V4.1  XP以降, GDI,BMP外の文字対応
			hRichEdit = LoadLibrary(RichText5);
			if ( X_uxt_rich > UXTRICH_V4 ) X_uxt_rich = UXTRICH_V4;
			mode = UXTRICH_V4;
		}
	#endif
		if ( hRichEdit == NULL ){ // 1: V2/V3(Me,2000以降)  GDI,BMP内のみ対応
			if ( X_uxt_rich > UXTRICH_V3 ) X_uxt_rich = UXTRICH_V3;
			mode = UXTRICH_V3;
			hRichEdit = LoadLibrary(RichText2);
		}
		if ( hRichEdit == NULL ){
			if ( X_uxt_rich > UXTRICH_NO ) X_uxt_rich = UXTRICH_NO;
			mode = UXTRICH_NO;
		}
	}
	return mode;
}

const TCHAR *RichEditClassname(int mode)
{
	mode = IsUseRichEdit(mode);
	if ( mode <= UXTRICH_NO ) return EditClassName;

	#ifdef UNICODE
	if ( mode == UXTRICH_DW ){
		return RichTextClassD2D;
	}else if ( mode == UXTRICH_V4 ){
		return RichTextClass5W;
	}else{ // UXTRICH_V3
		return RichTextClass2W;
	}
	#else
	return RichTextClass2A;
	#endif
}

		/* 桁折りできなくなる？
			if ( ((DLGITEMTEMPLATE *)(dpitem - sizeof(DLGITEMTEMPLATE)))->style & WS_VSCROLL ){
				((DLGITEMTEMPLATE *)(dpitem - sizeof(DLGITEMTEMPLATE)))->style |= ES_DISABLENOSCROLL;
			}
		*/
		/*
		if ( X_uxt_color == UXT_DARK ){ // 3D 境界線を無くす → 無くすと Ctrl + Enter が無効になる
			((DLGITEMTEMPLATE *)(dpitem - sizeof(DLGITEMTEMPLATE)))->style &= ~WS_BORDER;
		}
		*/

#if 0
{-------------------
//	XMessage(NULL, NULL, XM_DbgLOG, T("%d: atom %s"),i,ClassAtomString[*(WORD *)(dpitem + 2)-0x80] );
	if ( (i == 0) && (*(WORD *)(dpitem + sizeof(WORD)) == 0x81) ){ // EDIT

	#ifdef UNICODE
		const WCHAR *RTClassName;

		if ( X_uxt_rich == UXTRICH_DW ){
			RTClassName = RichTextClassD2D;
		}else{
			if ( X_uxt_rich == UXTRICH_V4 ){
				RTClassName = RichTextClass5W;
			}else{ // UXTRICH_V3
				RTClassName = RichTextClass2W;
			}
		}
	#else
		#define RTClassName RichTextClass2W
	#endif
		#define ES_DISABLENOSCROLL 0x00002000
		// ※ sizeof(RichTextClassW) で sizeof(RichTextClassD2D) を
		//    済ませているので文字数が変わった時は再実装(文字列
		//    サイズ取得と、DWORDアライメント調整) が必要
		#define RTClassSize sizeof(RichTextClass2W)
		newdialog = HeapReAlloc(ProcHeap, 0, dialog, dialogsize + RTClassSize + sizeof(WORD));
		if ( newdialog == NULL ) break;
		dpmax = newdialog + dialogsize;
		dpitem = newdialog + (dpitem - dialog);
		dialog = newdialog;
		memmove(dpitem + RTClassSize, dpitem + sizeof(WORD) * 2,
				dpmax - (dpitem + sizeof(WORD) * 2));
				memcpy(dpitem, RTClassName, RTClassSize);
		dialogsize += RTClassSize;
		dpitem += RTClassSize;
		dpmax += RTClassSize;
	}else{
		dpitem += sizeof(WORD) * 2;
	}
}
#endif

/*
プロパティシート
eng
	95-2000	MS Sans Serif
	XP		MS Shell Dlg
jpn
	95-98	ＭＳ Ｐゴシック
	2000-XP	MS UI Gothic
*/

PPXDLL LPDLGTEMPLATE PPXAPI GetDialogTemplate(HWND hParentWnd, HANDLE hinst, LPCTSTR lpszTemplate)
{
	BYTE *dialog, *dp, *dpitem;
	DLGTEMPLATE *dialog_res;
	HRSRC hRc;
	DWORD dialogsize, fontsize;
	int i;
	WCHAR fontdata[LF_FACESIZE + 1];
	UINT monitordpi, gdidpi;
	LOGFONTWITHDPI BoxFont;
	LCID forcust = 0;
									// テンプレートの読み込み
	hRc = FindResource(hinst, lpszTemplate, RT_DIALOG);
	if ( hRc == NULL ) return NULL;
	dialogsize = SizeofResource(hinst, hRc);
	dialog_res = LockResource(LoadResource(hinst, hRc));
#if 0
	dialog = HeapAlloc(ProcHeap, 0, dialogsize);
	memcpy(dialog, dialog_res, dialogsize);
#else
									// フォント情報の作成
	monitordpi = GetMonitorDPI(hParentWnd);
	GetPPxFont(PPXFONT_F_dlg, monitordpi, &BoxFont);
//	XMessage(NULL, NULL, XM_DbgLOG, T("Dialog point:%d"),BoxFont.font.lfHeight);

	if ( (dialog_res->style & DS_SETFONT) && // カスタマイザの時の特別処理
		 ((LONG_PTR)lpszTemplate >= ID_CUST_MIN) &&
		 ((LONG_PTR)lpszTemplate <= ID_CUST_MAX) ){
		DWORD custfont = 0;

		GetCustTable(StrCustOthers, T("custfont"), &custfont, sizeof(custfont));
		if ( custfont == 0 ){
			forcust = LOWORD(GetUserDefaultLCID());
			if ( forcust == LCID_JAPANESE ){
//				tstrcpy(BoxFont.font.lfFaceName, T("MS UI Gothic"));
				if ( ((LONG_PTR)lpszTemplate == IDD_GENERAL) ||
					 ((LONG_PTR)lpszTemplate == IDD_GENERALE) ){
					tstrcpy(BoxFont.font.lfFaceName, T("ＭＳ Ｐゴシック"));
				}else{
					tstrcpy(BoxFont.font.lfFaceName, T("ＭＳ ゴシック"));
				}
			}else if ( forcust == LCID_ENGLISH ){
				tstrcpy(BoxFont.font.lfFaceName, T("MS Sans Serif"));
			}else{
				forcust = 0;
			}
		}
	}

	// fontsize : lfFaceName の文字列長(\0含む) + word(fontsize)
#ifdef UNICODE
	fontsize = (strlenW(BoxFont.font.lfFaceName) + 2) * sizeof(WCHAR);
	memcpy(&fontdata[1], BoxFont.font.lfFaceName, fontsize - sizeof(WCHAR));
#else
	fontsize = (MultiByteToWideChar(CP_ACP, 0,
		BoxFont.font.lfFaceName, -1, &fontdata[1],
		sizeof(fontdata) / sizeof(WCHAR) - 2) + 1) * sizeof(WCHAR);
#endif
	dialog = HeapAlloc(ProcHeap, 0, dialogsize + fontsize + sizeof(WORD));
	memcpy(dialog, dialog_res, dialogsize);

	// dp = dialog + sizeof(DLGTEMPLATE) 相当
	dp = dialog + (sizeof(DWORD) * 2 + sizeof(WORD) * 5);

	if ( (X_dss & DSS_DIALOGREDUCE) &&
		 !((hinst == DLLhInst) &&
		 ((lpszTemplate == MAKEINTRESOURCE(IDD_INPUT)) ||
			 (lpszTemplate == MAKEINTRESOURCE(IDD_INPUT_OPT)) ||
			 (lpszTemplate == MAKEINTRESOURCE(IDD_INPUTREF)))) ){

		int DlgWidth = *(WORD *)(dialog + sizeof(DWORD) * 2 + sizeof(WORD) * 3);
		int DlgHeight = *(WORD *)(dialog + sizeof(DWORD) * 2 + sizeof(WORD) * 4);
		int heightPt;
		RECT deskbox;
		int minHeight = (PPX_FONT_MIN_PT * monitordpi + (DEFAULT_WIN_DPI / 2)) / DEFAULT_WIN_DPI; // 約8pt

		DlgHeight += 8; // ダイアログのタイトルバー部分を追加
		if ( ((LONG_PTR)lpszTemplate >= 0x7400) &&
			 ((LONG_PTR)lpszTemplate <= 0x77ff) ){
			DlgHeight += 24; // プロパティシートのボタン部分を追加
		}

		heightPt = BoxFont.font.lfHeight >= 0 ? BoxFont.font.lfHeight : -BoxFont.font.lfHeight;
		GetDesktopRect(hParentWnd, &deskbox);

#define DLGBASE_W 7 // 少し小さめにしている
#define DLGBASE_H 8
		gdidpi = GetGDIdpi(NULL);
		// Pt 変換
		deskbox.right = ((deskbox.right - deskbox.left) * DEFAULT_DTP_DPI) / DEFAULT_WIN_DPI;
		deskbox.bottom = ((deskbox.bottom - deskbox.top) * DEFAULT_DTP_DPI) / DEFAULT_WIN_DPI;

		if ( (OSver.dwBuildNumber >= WINBUILD_10_RS2) &&
			((monitordpi == gdidpi) && (gdidpi != DEFAULT_WIN_DPI)) ){ // Win10 RS2 以降は、既に補正がかかっているので、其の分調整
			deskbox.right = (deskbox.right * DEFAULT_WIN_DPI) / gdidpi;
			deskbox.bottom = (deskbox.bottom * DEFAULT_WIN_DPI) / gdidpi;
		}
		// ダイアログの横幅がはみ出すなら、縮める
		if ( ((DlgWidth * heightPt) / DLGBASE_W) > deskbox.right ){
			heightPt = (deskbox.right * DLGBASE_W) / DlgWidth;
			if ( heightPt < minHeight ) heightPt = minHeight;
			BoxFont.font.lfHeight = BoxFont.font.lfHeight >= 0 ? heightPt : -heightPt;
		}

		if ( ((DlgHeight * heightPt) / DLGBASE_H) > deskbox.bottom ){
			heightPt = (deskbox.bottom * DLGBASE_H) / DlgHeight;
			if ( heightPt < minHeight ) heightPt = minHeight;
			BoxFont.font.lfHeight = BoxFont.font.lfHeight >= 0 ? heightPt : -heightPt;
		}
	}
	// フォントサイズ(pixelでなく、Pt)
	fontdata[0] = (WCHAR)BoxFont.font.lfHeight;

									// メニュー、クラス名、タイトル名を飛ばす
	for ( i = 0; i < 3; i++ ){
		WORD id;

		id = *(WORD *)dp;
		if ( id == 0 ){ // なし
			dp += sizeof(WORD);
		} else if ( id == 0xffff ){ // WORD値
			dp += sizeof(WORD) * 2;
		} else { // UCS-2 - z
			dp += sizeof(WORD);
			while ( *dp != 0 ) dp += sizeof(WORD);
			dp += sizeof(WORD);
		}
	}
	dpitem = dp;
									// 既定のフォントがあれば飛ばす
	if ( ((DLGTEMPLATE *)dialog)->style & DS_SETFONT ){
		if ( forcust != 0 ){
			fontdata[0] = *(WORD *)dpitem;
			if ( forcust == LCID_JAPANESE ){ // フォントサイズ補正
				switch ( (LONG_PTR)lpszTemplate ){
					case IDD_INFO:
					case IDD_COLOR:
					case IDD_ADDON:
					case IDD_ETCTREE:
						fontdata[0] += (WCHAR)2;
						break;

					case IDD_GENERALE:
					case IDD_EXT:
					case IDD_KEYD:
					case IDD_MOUSED:
					case IDD_MENUD:
					case IDD_BARD:
						fontdata[0]++;
						break;
				}
			}
		}
		dpitem += sizeof(WORD); // font point
		while ( *dpitem != '\0' ) dpitem += sizeof(WORD);
		dpitem += sizeof(WORD); // NIL
	}
								// dword 補正1
	if ( ALIGNMENT_BITS(dpitem) & 3 ) dpitem += sizeof(WORD);
								// dword 補正2
	if ( (ALIGNMENT_BITS(dp) + fontsize) & 3 ) fontsize += sizeof(WORD);
									// テンプレートの修正
	setflag(((DLGTEMPLATE *)dialog)->style, DS_SETFONT);
	memmove(dp + fontsize, dpitem, dialogsize - (dpitem - dialog));
	memcpy(dp, fontdata, fontsize);

	if ( IsUseRichEdit(-1) > 0 ){ // Class EDIT を RichEdit に変換する
		BYTE *dpmax, *newdialog;
		int left;

		dialogsize += fontsize;
		dpmax = dialog + dialogsize;
		dpitem += fontsize - 2;
		left = dialog_res->cdit;
		while ( left-- && (dpitem < dpmax) ){
			dpitem += sizeof(DLGITEMTEMPLATE);
								// クラス名、タイトル名、追加データを飛ばす
			for ( i = 0; i < 3; i++ ){
				WORD id;

				id = *(WORD *)dpitem;
				if ( id == 0 ){ // なし
//					XMessage(NULL, NULL, XM_DbgLOG, T("%d: none"),i);
					dpitem += sizeof(WORD);
				} else if ( id == 0xffff ){ // WORD値
//					XMessage(NULL, NULL, XM_DbgLOG, T("%d: atom %s"),i,ClassAtomString[*(WORD *)(dpitem + 2)-0x80] );
					if ( (i == 0) &&
						 (*(WORD *)(dpitem + sizeof(WORD)) == 0x81) ){ // EDIT

						#ifdef UNICODE
							const WCHAR *RTClassName;

							if ( X_uxt_rich == UXTRICH_DW ){
								RTClassName = RichTextClassD2D;
							}else{
								if ( X_uxt_rich == UXTRICH_V4 ){
									RTClassName = RichTextClass5W;
								}else{ // UXTRICH_V3
									RTClassName = RichTextClass2W;
								}
							}
						#else
							#define RTClassName RichTextClass2W
						#endif

						/* 桁折りできなくなる？
						if ( ((DLGITEMTEMPLATE *)(dpitem - sizeof(DLGITEMTEMPLATE)))->style & WS_VSCROLL ){
	#define ES_DISABLENOSCROLL 0x00002000
							((DLGITEMTEMPLATE *)(dpitem - sizeof(DLGITEMTEMPLATE)))->style |= ES_DISABLENOSCROLL;
						}
						*/

					// ※ sizeof(RichTextClassW) で sizeof(RichTextClassD2D) を
					//    済ませているので文字数が変わった時は再実装(文字列
					//    サイズ取得と、DWORDアライメント調整) が必要
						#define RTClassSize sizeof(RichTextClass2W)
						newdialog = HeapReAlloc(ProcHeap, 0, dialog, dialogsize + RTClassSize + sizeof(WORD));
						if ( newdialog == NULL ) break;
						dpmax = newdialog + dialogsize;
						dpitem = newdialog + (dpitem - dialog);
						dialog = newdialog;
						memmove(dpitem + RTClassSize,
						 		dpitem + sizeof(WORD) * 2,
						 		dpmax - (dpitem + sizeof(WORD) * 2));
						memcpy(dpitem, RTClassName, RTClassSize);
						dialogsize += RTClassSize;
						dpitem += RTClassSize;
						dpmax += RTClassSize;
					}else{
						dpitem += sizeof(WORD) * 2;
					}
				} else { // UCS-2 - z
//					XMessage(NULL, NULL, XM_DbgLOG, T("%d: %Ls"),i,dpitem);
					dpitem += sizeof(WORD);
					while ( *dpitem != 0 ) dpitem += sizeof(WORD);
					dpitem += sizeof(WORD);
				}
			}
			if ( ALIGNMENT_BITS(dpitem) & 3 ) dpitem += sizeof(WORD);
		}
	}
#endif
	return (DLGTEMPLATE *)dialog;
}

// メインスレッドのウィンドウがメッセージループに入る前に
// サブスレッドでダイアログを表示させようとすると、
// ATOK のスレッド内でデッドロックに陥るのを回避するコード
void FixBeforeDialog(HWND hParentWnd)
{
	DWORD WndThreadID;

	if ( hParentWnd == NULL ) return;

	WndThreadID = GetWindowThreadProcessId(hParentWnd, NULL);

	if ( WndThreadID != GetCurrentThreadId() ){ // サブスレッド?
		DWORD_PTR sendresult;

		// メッセージポンプが動いているとすぐ戻ってくる。
		// そうでなければ TIMEOUT まで待機する
		SendMessageTimeout(hParentWnd, WM_NULL, 0, 0, 0, 5000, &sendresult);
	}
}

// 書いてみたけど、ForegroundWindow / Focus が NULL なので、意味なし？
HWND hPrevFGwnd;
HWND hNowFGwnd, hNowFGwnd2;

#pragma argsused
static void CALLBACK ConsoleAssistProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	HWND hFG;
	TCHAR buf[CMDLINESIZE];
	DWORD size;
	INPUT_RECORD cin;
	UnUsedParam(hWnd);UnUsedParam(uMsg);UnUsedParam(idEvent);UnUsedParam(dwTime);

	hFG = GetForegroundWindow();
	if ( hFG != hNowFGwnd ){
		hNowFGwnd2 = hNowFGwnd;
		hNowFGwnd = hFG;
		GetWindowText(hFG, buf, TSIZEOF(buf));
		XMessage(NULL, NULL, XM_DbgCon, T("Focus: %s\r\n"), buf);
	}

	if ( (DPeekConsoleInput(hConsoleStdin, &cin, 1, &size) == FALSE) || (size == 0) ){
		return;
	}
	if ( cin.EventType == KEY_EVENT ){
		if ( cin.Event.KeyEvent.bKeyDown == FALSE ){
			DReadConsoleInput(hConsoleStdin, &cin, 1, &size);
			return;
		}
	}
	DReadConsoleInput(hConsoleStdin, &cin, 1, &size);
/*
	XMessage(NULL, NULL, XM_DbgCon, T("size:%d %d %d %d"),size,input.EventType,
	input.Event.KeyEvent.wVirtualKeyCode,
	input.Event.KeyEvent.wVirtualScanCode );
*/
	if ( hFG != hPrevFGwnd ){
		PostMessage(hFG, WM_KEYDOWN, cin.Event.KeyEvent.wVirtualKeyCode, 0);
		PostMessage(hFG, WM_KEYUP, cin.Event.KeyEvent.wVirtualKeyCode, 0);
	}else if ( hNowFGwnd2 != hPrevFGwnd ){
		PostMessage(hNowFGwnd2, WM_KEYDOWN, cin.Event.KeyEvent.wVirtualKeyCode, 0);
		PostMessage(hNowFGwnd2, WM_KEYUP, cin.Event.KeyEvent.wVirtualKeyCode, 0);
	}
}

PPXDLL INT_PTR PPXAPI PPxDialogBoxParam(HANDLE hinst, const TCHAR *lpszTemplate, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit)
{
#if 1
	DLGTEMPLATE *dialog;
	INT_PTR result;
	DWORD old;

	InitSysColors();
	dialog = GetDialogTemplate(hwndOwner, hinst, lpszTemplate);
	if ( dialog == NULL ) return -1;
	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		hPrevFGwnd = hNowFGwnd = hNowFGwnd2 = GetForegroundWindow();
		old = UseConsoleKey();
		SetTimer(NULL, TIMERID_CONSOLE_ASSIST, TIME_CONSOLE_ASSIST, ConsoleAssistProc);
	}
	result = DialogBoxIndirectParam(hinst, dialog, hwndOwner, dlgprc, lParamInit);
#ifdef WINEGCC
	if ( (result == -1) && (GetLastError() == ERROR_OPEN_FAILED) ){
		result = DialogBoxParam(hinst, lpszTemplate, hwndOwner, dlgprc, lParamInit);
	}
#endif
	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		FreeConsoleKey(old);
		KillTimer(NULL, TIMERID_CONSOLE_ASSIST);
	}
	HeapFree(ProcHeap, 0, dialog);
	return result;
#else

	return DialogBoxParam(hinst, lpszTemplate, hwndOwner, dlgprc, lParamInit);
#endif
}

#ifdef UNICODE
static const TCHAR *GetDialogTemplateText(const BYTE **dpitem)
#else
static const TCHAR *GetDialogTemplateText(const BYTE **dpitem, char *text)
#endif
{
	if ( *((WORD *)*dpitem) == 0xffff ){ // atom
		WORD atom;

		atom = *(WORD *)((BYTE *)(*dpitem + sizeof(WORD)));
		*dpitem += sizeof(WORD) * 2;

		if ( atom == 0x81 ){ // EDIT
			return RichEditClassname(-1);
		}

		if ( (atom >= 0x80) && (atom <= 0x85) ){
			return ClassAtomString[atom - 0x80];
		}
		return (const TCHAR *)(DWORD_PTR)atom;
	} else{
#ifdef UNICODE
		WCHAR *text;
		DWORD size;

		text = (WCHAR *)*dpitem;
		size = strlenW(text) + 1;
		*dpitem += sizeof(WORD) * size;
#else
		WCHAR *resulttextW;
		DWORD size;

		resulttextW = (WCHAR *)*dpitem;
		size = strlenW(resulttextW) + 1;
		*dpitem += sizeof(WORD) * size;
		if ( text != NULL ) UnicodeToAnsi(resulttextW, text, MAX_PATH);
#endif
		return text;
	}
}

static const TCHAR * USEFASTCALL GetCaptionText(int id)
{
	TCHAR name[8];

	if ( id < 0 ) return NULL;
	thprintf(name, TSIZEOF(name), T("%04X"), id);
	return SearchMessageText(name);
}

// hParentWnd で指定したウィンドウに lpszTemplate のコントロール群を貼り付ける
HWND *CreateDialogControls(HANDLE hinst, LPCTSTR lpszTemplate, HWND hParentWnd)
{
	HRSRC hrDialog;
	DWORD controls;
	const BYTE *dialog;
	const BYTE *dp, *dpitem;
	DLGITEMTEMPLATE *dtp;
	RECT box;
	int i;
	HFONT hFont;
	HWND *hCtrlWnds, *hCtrlWndDst;

//	InitSysColors();
	hrDialog = FindResource(hinst, lpszTemplate, RT_DIALOG);
	if (hrDialog == NULL) return NULL;
	dialog = LockResource(LoadResource(hinst, hrDialog));
	controls = ((DLGTEMPLATE *)dialog)->cdit;
	hCtrlWnds = hCtrlWndDst = HeapAlloc(DLLheap, 0, (controls + 1) * sizeof(HWND));
	if ( hCtrlWnds == NULL ) return NULL;

	hFont = (HFONT)SendMessage(hParentWnd, WM_GETFONT, 0, 0);
	dp = dialog + (sizeof(DWORD) * 2 + sizeof(WORD) * 5); // DLGTEMPLATE 相当
									// メニュー、クラス名、タイトル名を飛ばす
	for ( i = 0; i < 3; i++ ){
		WORD id;

		id = *(WORD *)dp;
		if ( id == 0 ){ // なし
			dp += sizeof(WORD);
		} else if ( id == 0xffff ){ // WORD値
			dp += sizeof(WORD) * 2;
		} else { // UCS-2 - z
			dp += sizeof(WORD);
			while ( *dp != 0 ) dp += sizeof(WORD);
			dp += sizeof(WORD);
		}
	}
	dpitem = dp;
									// 既定のフォントがあれば飛ばす
	if ( ((DLGTEMPLATE *)dialog)->style & DS_SETFONT ){
		dpitem += sizeof(WORD);
		while( *dpitem ) dpitem += sizeof(WORD);
		dpitem += sizeof(WORD);
	}
	for ( ; controls ; controls-- ){
		const TCHAR *ClassName, *CaptionName;
#ifndef UNICODE
		char ClassNameA[MAX_PATH];
		char CaptionNameA[MAX_PATH];
#endif
		WORD extrasize;
		HWND hCtrlWnd;

		if ( ALIGNMENT_BITS(dpitem) & 3 ) dpitem += sizeof(WORD); // アライメント補正

		dtp = (DLGITEMTEMPLATE *)dpitem;
		box.left	= dtp->x;
		box.top		= dtp->y;
		box.right	= dtp->cx;
		box.bottom	= dtp->cy;
		MapDialogRect(hParentWnd, &box);

		dpitem += (sizeof(DWORD) * 2 + sizeof(WORD) * 5); //DLGITEMTEMPLATE相当
#ifdef UNICODE
		ClassName = GetDialogTemplateText(&dpitem);
		CaptionName = GetCaptionText(dtp->id);
		if ( CaptionName != NULL ){
			GetDialogTemplateText(&dpitem);
		}else{
			CaptionName = GetDialogTemplateText(&dpitem);
		}
#else
		ClassName = GetDialogTemplateText(&dpitem, ClassNameA);
		CaptionName = GetCaptionText(dtp->id);
		if ( CaptionName != NULL ){
			GetDialogTemplateText(&dpitem, NULL);
		}else{
			CaptionName = GetDialogTemplateText(&dpitem, CaptionNameA);
		}
#endif
		extrasize = *(WORD *)dpitem;

		hCtrlWnd = CreateWindowEx(dtp->dwExtendedStyle | WS_EX_NOPARENTNOTIFY,
				ClassName, CaptionName, dtp->style | WS_CHILD | WS_VISIBLE,
				box.left, box.top, box.right, box.bottom, hParentWnd,
				CHILDWNDID(dtp->id), hinst, extrasize ? (void *)dpitem : NULL);
		if ( hCtrlWnd == NULL ){
			PPErrorBox(hParentWnd, NULL, PPERROR_GETLASTERROR);
			break;
		}
		SendMessage(hCtrlWnd, WM_SETFONT, (WPARAM)hFont, TMAKELPARAM(TRUE,0));
		FixUxTheme(hCtrlWnd, ClassName);
		*hCtrlWndDst++ = hCtrlWnd;
		dpitem += extrasize ? extrasize : sizeof(WORD);
	}
	*hCtrlWndDst = NULL;
	return hCtrlWnds;
}

//===================================== 簡易整形表示するウィンドウクラス
static void PaintPPxStatic(HWND hWnd)
{
	PAINTSTRUCT ps;
	TCHAR buf[0x800], *maxptr;
	RECT rect;

	BeginPaint(hWnd, &ps);
	maxptr = buf + GetWindowText(hWnd, buf, TSIZEOF(buf));

	if ( maxptr != buf ){
		TCHAR *first, *format;
		POINT Draw = {1, 2};
		int align = -1; // はみ出したときの揃え方
		HGDIOBJ hOldFont;
		TEXTMETRIC tm;
		int baseW, baseH;

		#if 0
		{	// 呼び出し間隔計測用
			DWORD oldtick,nowtick = GetTickCount();

			oldtick = (DWORD)GetProp(hWnd,T("IntervalCheck"));
			if ( (nowtick - oldtick) < 100 ){
				XMessage(NULL, NULL, XM_DbgLOG, T("Static %x %3dm %s"), (DWORD)hWnd & 0xff, (nowtick - oldtick), buf);
			}
			SetProp(hWnd,T("IntervalCheck"),(HANDLE)nowtick);
		}
		#endif

		rect.top = 0;
		rect.bottom = 0;
		rect.left = 0;
		rect.right = 1;
		MapDialogRect(GetParent(hWnd), &rect);
		baseW = rect.right;
		GetClientRect(hWnd, &rect);
		InitSysColors();

		// WS_BORDER の代わりに自前で枠を描画。
		// ※ WS_BORDER がダイアログ作成時に WS_EX_CLIENTEDGE に置き換わり、
		//    しかも、後で変更できないため
		rect.right--;
		rect.bottom--;
		FrameRect(ps.hdc, &rect, GetEdgeLineBrush());
		rect.left += 2;
		rect.top++;
		rect.right--;
		rect.bottom--;

		SetTextColor(ps.hdc, C_DialogText);
		SetBkColor(ps.hdc, C_DialogBack);
		hOldFont = SelectObject(ps.hdc,
				(HFONT)GetWindowLongPtr(hWnd, GWLP_USERDATA));

		GetTextMetrics(ps.hdc, &tm);
		baseH = tm.tmHeight;

		format = buf;
		while ( format < maxptr ){
			SIZE ssize;
			size_t strlength;

			first = format;
			while ( format < maxptr ){
				if ( (UTCHAR)*format < ' ' ) break;
				format++;
			}
			strlength = format - first;
			if ( strlength ){
				RECT box;

				GetTextExtentPoint32(ps.hdc, first, strlength, &ssize);
				if ( ssize.cx < (rect.right - Draw.x) ){ // はみ出ない
					if ( align != SS_RIGHT ){ // 左揃え
						ssize.cx = Draw.x;
					}else{ // 右揃え
						ssize.cx = rect.right - ssize.cx;
					}
				}else{ // はみ出る
					if ( align < 0 ){
						align = GetWindowLongPtr(hWnd, GWL_STYLE) & 7;
					}
					switch ( align ){
						case SS_CENTER: // (中央)最終エントリ表示
							first = VFSFindLastEntry(first);
							strlength = tstrlen(first);
							format = first + strlength;
							ssize.cx = Draw.x;
							break;
						case SS_RIGHT: // 右揃え
							ssize.cx = Draw.x;
							break;
						case 3: // (SS_RIGHT + SS_CENTER)dir部分のみ右揃え
							format = VFSFindLastEntry(first);
							if ( *format == '\\' ) format++;
							strlength = format - first;
							format += tstrlen(format);
							GetTextExtentPoint32(ps.hdc, first, strlength, &ssize);
							ssize.cx = rect.right - ssize.cx;
							break;
						default: // 左揃え ( SS_LEFT )
							ssize.cx = rect.right - ssize.cx;
					}
				}
				box.left = Draw.x;
				box.top = Draw.y;
				box.right = rect.right;
				box.bottom = min(rect.bottom, Draw.y + baseH);

				ExtTextOut(ps.hdc, ssize.cx, Draw.y, ETO_CLIPPED | ETO_OPAQUE,
						&box, first, strlength, NULL);
				if ( format >= maxptr ) break;
			}
			switch ( *format ){
				case PXSC_NORMAL: {				// 通常
					SetTextColor(ps.hdc, C_DialogText);
					SetBkColor(ps.hdc, C_DialogBack);
					break;
				}
				case PXSC_HILIGHT: {
					SetTextColor(ps.hdc, C_HighlightText);
					SetBkColor(ps.hdc, C_HighlightBack);
					break;
				}
				case '\n':			// 改行
					Draw.x = 1;
					Draw.y += baseH;
					break;
				case PXSC_PAR: {	// プログレス表示
/*
----------------------+
3                     |
  +----+  +--+5 +--+  |
x1| x10|x1|…|x2|…|x1|
  +----+  +--+  +--+  |
2                     |
----------------------+
*/
					// PAR_ALLBLOCKS * PAR_BLOCKRANGE = 100
					#define PAR_ALLBLOCKS 10 // 全ブロック数
					#define PAR_BLOCKRANGE 10 // ブロックの単位
					#define PAR_BLOCKSIZE 10 // 1ブロックの描画幅
					#define PAR_BLOCKSBLANK 1 // ブロック間の空白
					#define PAR_BLOCKSPACING (PAR_BLOCKSIZE + PAR_BLOCKSBLANK)
					#define PAR_WIDTH (PAR_BLOCKSBLANK + PAR_BLOCKSPACING * PAR_ALLBLOCKS + PAR_BLOCKSBLANK + PAR_BLOCKSBLANK ) // プログレス表示幅
					int i;
					RECT box;
					HBRUSH hB;
					int count, par, drawbar;

					par = (DWORD)*((BYTE *)(format + 1)) - 1;	// 1-100(-1)
					count = *(BYTE *)(format + 2) - 1;			// 1-11(-1)
					format += 2;

					box.top = rect.top + (baseW + (baseW / 2));
					box.bottom = min(rect.bottom, Draw.y + baseH) - baseW;

					hB = (count <= 10) ? GetHighlightBackBrush() : GetEdgeLineBrush();
														// 最後直前まで
					box.left  = Draw.x + baseW;
					box.right = Draw.x + baseW * PAR_BLOCKSPACING;
					for ( i = 0 ; (i + PAR_BLOCKRANGE) < par ; i += PAR_BLOCKRANGE ){
						FillBox(ps.hdc, &box, hB);
						box.left = box.right + baseW;
						// 50% 境界は空白幅が倍
						if ( i == (PAR_BLOCKRANGE * (5 - 1)) ){
							box.left += baseW; // * PAR_BLOCKSBLANK
						}
						box.right = box.left + baseW * PAR_BLOCKSIZE;
					}
														// 最後
					box.right = box.left + ((par - i) * baseW);
					FillBox(ps.hdc, &box, hB);
													// 空白 -------------------
					box.top = rect.top;
					box.bottom += baseW;
					box.left = box.right;
					box.right = Draw.x + baseW * PAR_WIDTH;
					hB = GetDialogBackBrush();
					FillBox(ps.hdc, &box, hB);

					drawbar = (int)GetWindowLongPtr(hWnd, 0);
					if ( (drawbar > 0) && (drawbar < box.left) ){
						// 直前の処理中バーを消去
						box.left = drawbar;
						box.right = drawbar + baseW;
						FillBox(ps.hdc, &box, hB);
					}
											// 処理中バー -----------
					if ( (count >= 0) && (count <= 10) ){
						hB = CreateSolidBrush(C_WindowText);
						box.left = Draw.x + ( count * PAR_BLOCKSPACING + PAR_BLOCKSPACING ) * baseW;
						if ( count >= 4 ) box.left += baseW;
						box.right = box.left + baseW;
						FillBox(ps.hdc, &box, hB);
						DeleteObject(hB);
						SetWindowLongPtr(hWnd, 0, (LONG_PTR)box.left);
					}

					Draw.x += baseW * PAR_WIDTH;
					break;
				}
				case PXSC_LEFT:
					align = SS_LEFT;
					break;
				case PXSC_RIGHT:
					align = SS_RIGHT;
					break;
				default:	// 未定義
					format = maxptr;
			}
			format++;
		}
		SelectObject(ps.hdc, hOldFont);
	}
	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK PPxStaticProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch ( message ){
		case WM_PAINT:
			PaintPPxStatic(hWnd);
			return 0;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			PostMessage(GetParent(hWnd), WM_COMMAND, GetDlgCtrlID(hWnd) | (WM_CONTEXTMENU << 16), (LPARAM)hWnd);
			return 0;

		case WM_SETTEXT:
			InvalidateRect(hWnd, NULL, FALSE);
			break;

		case WM_SETFONT:
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)wParam);
			if ( LOWORD(lParam) ) InvalidateRect(hWnd, NULL, FALSE);
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void ShowDlgWindow(const HWND hDlg, const UINT id, BOOL show)
{
	HWND hControlWnd;

	hControlWnd = GetDlgItem(hDlg, id);
	if ( hControlWnd == NULL ) return;
	ShowWindow(hControlWnd, show ? SW_SHOW : SW_HIDE);
}

void EnableDlgWindow(HWND hDlg, int id, BOOL state)
{
	HWND hControlWnd;

	hControlWnd = GetDlgItem(hDlg, id);
	if ( hControlWnd == NULL ) return;
	EnableWindow(hControlWnd, state ? TRUE : FALSE);
}

struct {
	HWND hCheckWnd;
	HWND hParentWnd;
	BOOL result;
} DialogKeyProcCache = {NULL, NULL, FALSE}; // ダイアログ判定の以前の結果

// msg がダイアログコントロールかどうかを判定し、合っていれば IsDialogMessage を実行する
BOOL DialogKeyProc(MSG *msg)
{
	DWORD style;

	if ( (msg->message < WM_KEYFIRST) || (msg->message >= 0x109) ) return FALSE;
	if ( msg->hwnd == DialogKeyProcCache.hCheckWnd ){
//		XMessage(NULL, NULL, XM_DbgLOG, T("cache %x %d"),msg->hwnd,DialogKeyProcCache.result);
		if ( DialogKeyProcCache.result == FALSE ) return FALSE;
	}else{
		HWND hParent;

		DialogKeyProcCache.hCheckWnd = msg->hwnd;

//		XMessage(NULL, NULL, XM_DbgLOG, T("check %x"),msg->hwnd);

		hParent = GetParent(msg->hwnd); // 親がダイアログかを判断する
		if ( hParent == DialogKeyProcCache.hParentWnd ){
			if ( DialogKeyProcCache.result == FALSE ) return FALSE;
		}else{
			DialogKeyProcCache.hParentWnd = hParent;
			DialogKeyProcCache.result = FALSE;
			if ( hParent == NULL ) return FALSE;
			style = GetWindowLongPtr(hParent, GWL_STYLE);
			if ( style & WS_CHILD ){
				DialogKeyProcCache.hParentWnd = hParent = GetParent(hParent);
				if ( hParent == NULL ) return FALSE;
				style = GetWindowLongPtr(hParent, GWL_STYLE);
			}
			// ダイアログは WS_POPUP が有効なのでそれで判断
			if ( !(style & WS_POPUP) ) return FALSE;
			DialogKeyProcCache.result = TRUE;
		}
	}
//		XMessage(NULL, NULL, XM_DbgLOG, T("check %x -> dialog %x"),msg->hwnd,DialogKeyProcCache.hParentWnd);
	return IsDialogMessage(DialogKeyProcCache.hParentWnd, msg);
}

