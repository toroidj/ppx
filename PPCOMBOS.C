/*-----------------------------------------------------------------------------
	Paper Plane cUI			Combo Window Sub
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <commctrl.h>
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPC_DD.H"
#include "PPCOMBO.H"
#include "PPCUI.RH"
#include "PPXVER.H"
#pragma hdrstop

TypedefWinAPI(HRESULT, DwmGetColorizationColor, (DWORD*, BOOL*));

void CreateAddressBar(void);
void CreateReportArea(void);
void USEFASTCALL ComboTableCheck(void);

typedef struct {
	TCHAR *str;
	TCHAR *dest;
	DWORD size;
} HeapStr;

#define CheckWorkExt 3 // チェック用に使う memo 領域の大きさ(倍率)

#if VersionP
int oldpaneerr = 0;
#endif

int Group_id = 1; // 仮グループ名に付ける通し番号

TCHAR *InitHeapStr(HeapStr *hstr, DWORD first_len)
{
	hstr->size = first_len * sizeof(TCHAR);
	hstr->str = hstr->dest = PPcHeapAlloc(hstr->size);
	return hstr->dest;
}

BOOL CheckHeapStrSize(HeapStr *hstr, DWORD eq_len)
{
	TCHAR *newbuf;
	DWORD size;

	size = ((hstr->dest - hstr->str) + eq_len + 1) * sizeof(TCHAR);
	if ( size < hstr->size ) return TRUE;
	size += 0x20 * sizeof(TCHAR); // 余裕分
	newbuf = HeapReAlloc( hProcessHeap, 0, hstr->str, size );
	if ( newbuf != NULL ){
		hstr->size = size;
		hstr->dest = newbuf + (hstr->dest - hstr->str);
		hstr->str = newbuf;
		return TRUE;
	}else{
		return FALSE;
	}
}
#define FreeHeapStr(hstr) PPcHeapFree((hstr)->str);

void SavePane_Base(TCHAR *dest, TCHAR *memo, int save);
void SavePane_Tab(HeapStr *hstr, TCHAR *memo);

// ペインフレーム =============================================================
void USEFASTCALL ComboFrameHscroll(HWND hWnd, WORD scrollcode)
{
	switch ( scrollcode ){
								// 一番上 .............................
		case SB_TOP:
			Combo.Panes.delta.x = 0;
			break;
								// 一番下 .............................
		case SB_BOTTOM:
			Combo.Panes.delta.x = 100;
			break;
								// 一行上 .............................
		case SB_LINEUP:
			Combo.Panes.delta.x += 100;
			break;
								// 一行下 .............................
		case SB_LINEDOWN:
			Combo.Panes.delta.x -= 100;
			break;
								// 一頁上 .............................
		case SB_PAGEUP:
			Combo.Panes.delta.x += 500;
			break;
								// 一頁下 .............................
		case SB_PAGEDOWN:
			Combo.Panes.delta.x -= 500;
			break;
								// 特定位置まで移動中 .................
		case SB_THUMBTRACK:
			//SB_THUMBPOSITION へ続く
								// 特定位置まで移動 ...................
		case SB_THUMBPOSITION:{
			SCROLLINFO scri;

			scri.cbSize = sizeof(scri);
			scri.fMask = SIF_TRACKPOS;
			GetScrollInfo(hWnd, SB_HORZ, &scri);
			Combo.Panes.delta.x = scri.nTrackPos;
			break;
		}
	}
	SortComboWindows(SORTWIN_LAYOUTPAIN);
	InvalidateRect(hWnd, NULL, TRUE);
}

void PaintComboFrame(HWND hFrameWnd)
{
	DRAWCOMBOSTRUCT dcs;

	BeginPaint(hFrameWnd, &dcs.ps);
	dcs.hBr = NULL;

	DrawPaneArea(&dcs);
	if ( dcs.hBr != NULL ) DeleteObject(dcs.hBr);
	EndPaint(hFrameWnd, &dcs.ps);
}

LRESULT ComboFrameLMouseDown(HWND hFrameWnd, LPARAM lParam)
{
	POINT pos;
	int showindex;

	LPARAMtoPOINT(pos, lParam);
	if ( (pos.y < 0) || (pos.x < 0) ) return 0;
	SetCapture(hFrameWnd);

	showindex = GetComboShowIndexFromPos(&pos);
	if ( showindex >= 0 ) Combo.SizingPane = showindex;
	return 0;
}

LRESULT CALLBACK ComboFrameProc(HWND hFrameWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message){
							// 垂直スクロール
		case WM_VSCROLL:
							// 水平スクロール
		case WM_HSCROLL:
			ComboFrameHscroll(hFrameWnd, LOWORD(wParam));
			break;

		case WM_PARENTNOTIFY:
			if ( Combo.hWnd == BADHWND ) break; // 終了動作中
			if ( LOWORD(wParam) == WM_DESTROY ){
				ComboProc(Combo.hWnd, message, wParam, lParam);
			}
			break;

		case WM_PAINT:
			PaintComboFrame(hFrameWnd);
			break;

		case WM_LBUTTONDOWN:	return ComboFrameLMouseDown(hFrameWnd, lParam);
		case WM_LBUTTONUP:		return ComboLMouseUp(lParam);
		case WM_MOUSEMOVE:		return ComboMouseMove(hFrameWnd, lParam);

		default:
			return DefWindowProc(hFrameWnd, message, wParam, lParam);
	}
	return 0;
}

// アドレスバー ==============================================================
void EnterAddressBar(void)
{
	TCHAR cmdline[CMDLINESIZE];

	GetWindowText(Combo.hAddressWnd, cmdline, CMDLINESIZE);
	SendMessage(hComboFocusWnd, WM_PPXCOMMAND, KCW_enteraddress, (LPARAM)cmdline);
	SetFocus(hComboFocusWnd);
}

void SetComboAddresBar(const TCHAR *path)
{
	if ( Combo.hAddressWnd != NULL ) SetWindowText(Combo.hAddressWnd, path);
	if ( Combo.Docks.t.hAddrWnd != NULL ) SetWindowText(Combo.Docks.t.hAddrWnd, path);
	if ( Combo.Docks.b.hAddrWnd != NULL ) SetWindowText(Combo.Docks.b.hAddrWnd, path);
}

// ログ窓 ====================================================================
void SetComboReportText(const TCHAR *text)
{
	if ( text == REPORTTEXT_TOGGLE ){
		text = (Combo.Report.hWnd == NULL) ? REPORTTEXT_OPEN : REPORTTEXT_CLOSE;
	}
	if ( text == REPORTTEXT_CLOSE ){
		if ( Combo.Report.hWnd != NULL ){
			PPxCommonExtCommand(K_SETLOGWINDOW, 0);
			PostMessage(Combo.Report.hWnd, WM_CLOSE, 0, 0);
			Combo.Report.hWnd = NULL;
			Combo.Height.ReportJob = 0;
		}
		resetflag(X_combos[0], CMBS_COMMONREPORT);
		SortComboWindows(SORTWIN_LAYOUTALL);
		return;
	}
	if ( text == REPORTTEXT_FOCUS ){
		if ( Combo.Report.hWnd != NULL ) SetFocus(Combo.Report.hWnd);
		return;
	}

	if ( (Combo.Report.hWnd == NULL) &&
		 ((text == REPORTTEXT_OPEN) || (X_Logging == 0)) ){
		setflag(X_combos[0], CMBS_COMMONREPORT);
		CreateReportArea();
		SortComboWindows(SORTWIN_LAYOUTALL);
		if ( text == REPORTTEXT_OPEN ) text = NULL;
	}
	if ( (text != NULL) && (text != REPORTTEXT_OPEN) ){
		SetReportTextMain(Combo.Report.hWnd, text, FALSE);
	}
}

// 初期化関連 =================================================================
// フォント生成 -------------------------
void ComboInitFont(void)
{
	TEXTMETRIC tm;
	HDC hDC;
	HGDIOBJ hOldFont;	//一時保存用
	int X_lspc = 0;

	GetCustData(T("X_lspc"), &X_lspc, sizeof(X_lspc));
	Combo.Font.handle = CreateMesFont(0, Combo.FontDPI);

	hDC = GetDC(Combo.hWnd);

	SplitPix = (GetSystemMetrics(SM_CXSIZEFRAME) * Combo.FontDPI) / GetDeviceCaps(hDC, LOGPIXELSX);

	hOldFont = SelectObject(hDC, Combo.Font.handle);
										// フォント情報を入手
	GetAndFixTextMetrics(hDC, &tm);
//	Combo.Font.size.cx = tm.tmAveCharWidth;
	Combo.Font.size.cy = tm.tmHeight + X_lspc;
	SelectObject(hDC, hOldFont);
	ReleaseDC(Combo.hWnd, hDC);

	if ( X_combos[0] & CMBS_COMMONINFO ){
		Combo.Height.InfoLine = Combo.Font.size.cy * 2;
		if ( XC_ifix && (Combo.Height.InfoLine < XC_ifix) ){
			Combo.Height.InfoLine = XC_ifix;
		}
	}else{
		Combo.Height.InfoLine = 0;
	}
}

void LoadX_Combos(void)
{
	if ( NO_ERROR == GetCustTable(T("X_combou"), ComboID, &X_combos, sizeof(X_combos)) ){
		Combo.UniqueX_Combos = TRUE;
	}else{
		Combo.UniqueX_Combos = FALSE;
		GetCustData(T("X_combos"), &X_combos, sizeof(X_combos));
	}
}

void SaveX_Combos(void)
{
	if ( Combo.UniqueX_Combos == FALSE ){
		SetCustData(T("X_combos"), &X_combos, sizeof(X_combos));
	}else{
		SetCustTable(T("X_combou"), ComboID, &X_combos, sizeof(X_combos));
	}
}

void LoadCombos(void)
{
	if ( (X_combos[0] & (CMBS_TABSEPARATE | CMBS_TABEACHITEM)) != (CMBS_TABSEPARATE | CMBS_TABEACHITEM) ){
		resetflag(X_combos[0], CMBS_TABEACHITEM); // CMBS_TABSEPARATE がない場合は、値が不整合
	}

	if ( (X_combos[0] & (CMBS_VPANE | CMBS_QPANE)) == (CMBS_VPANE | CMBS_QPANE) ){
		resetflag(X_combos[0], CMBS_VPANE);
	}
}

void InitComboGUI(BOOL reload)
{
	PPC_APPINFO *cinfo = NULL;
	int bindex = GetComboBaseIndex(hComboFocusWnd);

	if ( bindex >= 0 ) cinfo = Combo.base[bindex].cinfo;

	if ( reload && !UseOnceDock ){	// 使っていたのを閉じる
		if ( Combo.ToolBar.hWnd != NULL ){
			Combo.ToolBar.Height = 0;
			CloseToolBar(Combo.ToolBar.hWnd);
			Combo.ToolBar.hWnd = NULL;
		}
//		BackupLog(); // ReportArea を再作成しないので無効
		if ( Combo.Docks.t.hWnd != NULL ) DestroyWindow(Combo.Docks.t.hWnd);
		if ( Combo.Docks.b.hWnd != NULL ) DestroyWindow(Combo.Docks.b.hWnd);

		ThFree(&Combo_thGuiWork);
	}
	// 初期化開始
	ComboInitFont();
	SetFontDxDraw(Combo.DxDraw, Combo.Font.handle ,0);

	if ( !reload || !UseOnceDock ){
		UINT ID;

		if ( X_combos[0] & CMBS_TOOLBAR ) ComboCreateToolBar(Combo.hWnd);
		ID = ComboCommandIDFirst;
		DocksInit(&Combo.Docks, Combo.hWnd, cinfo, ComboID, Combo.Font.handle, Combo.Font.size.cy, &Combo_thGuiWork, &ID);
	}

	if ( X_combos[0] & CMBS_COMMONADDR ){
		if ( Combo.hAddressWnd == NULL ) CreateAddressBar();
	}else{
		if ( Combo.hAddressWnd != NULL ){
			DestroyWindow(Combo.hAddressWnd);
			Combo.hAddressWnd = NULL;
			Combo.Height.AddrBar = 0;
		}
	}
//	RestoreLog(); // ReportArea を再作成しないので無効
	SortComboWindows(SORTWIN_LAYOUTALL);
}

COLORREF USEFASTCALL GlassFix(COLORREF color)
{
	if ( (OSver.dwMajorVersion != 6) || (OSver.dwMinorVersion > 1) ){
		return color;
	}
	color = color + 0x50;
	if ( color >= 0x100 ) color = 0xff;
	return color;
}

void ComboCust(BOOL reload)
{
	DWORD oldcombos;
	TCHAR buf[CMDLINESIZE];
	int i;

	oldcombos = X_combos[0];

	LoadX_Combos();
	LoadCombos();

	if ( (oldcombos ^ X_combos[0]) & CMBS_TABEACHITEM ){
		X_combos[0] = oldcombos;
		ReplyMessage(0);
		XMessage(Combo.hWnd, NULL, XM_FaERRld, MES_RTPC);
	}

	GetCustData(T("XC_ifix"), &XC_ifix, sizeof(XC_ifix));
	if ( X_combo == COMBO_TAB ) setflag(X_combos[0], CMBS_TABALWAYS);

	GetCustData(T("C_capt"), &C_capt, sizeof(C_capt));
	for ( i = 0; i < (sizeof(C_capt) / sizeof(COLORREF)); i++){
		if ( C_capt[i] >= C_Scheme1_MIN ){
			C_capt[i] = GetSchemeColor(C_capt[i], C_AUTO);
		}
	}
	if ( C_ActiveBack == C_AUTO ){
		C_ActiveBack = GetSysColor(COLOR_ACTIVECAPTION);

		if ( hDwmapi != NULL ){
			ValueWinAPI(DwmGetColorizationColor);
			ValueWinAPI(DwmIsCompositionEnabled);

			GETDLLPROC(hDwmapi, DwmGetColorizationColor);
			GETDLLPROC(hDwmapi, DwmIsCompositionEnabled);
			if ( DDwmGetColorizationColor != NULL ){
				BOOL tmp = FALSE;

				DDwmIsCompositionEnabled(&tmp);
				if ( tmp && SUCCEEDED(DDwmGetColorizationColor(&C_ActiveBack, &tmp)) ){
					C_ActiveBack = (GlassFix(C_ActiveBack & 0xff) << 16) | (GlassFix((C_ActiveBack & 0xff00) >> 8) << 8) | GlassFix((C_ActiveBack & 0xff0000) >> 16);
					if ( C_ActiveText == C_AUTO ){
						C_ActiveText = (GetColorBright(C_ActiveBack) > 0x500) ? C_BLACK : C_WHITE;
					}
				}
			}
		}
	}
	if ( C_ActiveText == C_AUTO ) C_ActiveText = GetSysColor(COLOR_CAPTIONTEXT);
	if ( C_PairText == C_AUTO ) C_PairText = GetSysColor(COLOR_INACTIVECAPTIONTEXT);
	if ( C_InActiveText == C_AUTO ) C_InActiveText = GetSysColor(COLOR_CAPTIONTEXT);
	if ( C_PairBack == C_AUTO ) C_PairBack = GetSysColor(COLOR_INACTIVECAPTION);
	if ( C_InActiveBack == C_AUTO ) C_InActiveBack = GetSysColor(COLOR_INACTIVECAPTION);

	if ( Combo.TabCaption.text != NULL ){
		PPcHeapFree(Combo.TabCaption.text);
		Combo.TabCaption.text = NULL;
	}
	buf[0] = '\0';
	GetCustData(T("X_tcap"), &buf, sizeof(buf));
	if ( buf[0] == '\0' ){
		Combo.TabCaption.type = 0;
	}else if ( Isdigit(buf[0]) && (buf[1] == '\0') ){
		Combo.TabCaption.type = buf[0] - '0';
	}else{
		Combo.TabCaption.type = 0;
		Combo.TabCaption.text = PPcHeapAllocT(CMDLINESIZE);
		if ( Combo.TabCaption.text != NULL ) tstrcpy(Combo.TabCaption.text, buf);
	}

	if ( (Combo.hWnd != NULL) && (Combo.hWnd != BADHWND) ){
		InitComboGUI(reload);
	}
}

// Tree =======================================================================
void CreateBottomArea(void)
{
	TCHAR buf[8];
	DWORD tmp[2];

	if ( Combo.Height.ReportJob >= AREAMINSIZE ) return;
	thprintf(buf, TSIZEOF(buf), T("%sL"), ComboID);
	tmp[1] = BOTTOM_AREAMINSIZE;
	GetCustTable(T("XC_tree"), buf, &tmp, sizeof(tmp));
	Combo.Height.ReportJob = tmp[1];
	if ( Combo.Height.ReportJob < AREAMINSIZE ) Combo.Height.ReportJob = AREAMINSIZE;
}

void CreateReportArea(void)
{
	GetCustData(T("X_askp"), &X_askp, sizeof(X_askp));
	CreateBottomArea();
	InitEditColor();
	Combo.Report.hWnd = CreateWindowEx(0, WC_EDIT, NilStr,
			WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL |
			ES_LEFT | ES_MULTILINE | ES_WANTRETURN,
			0, Combo.Report.box.y,
			Combo.WndSize.cx, Combo.Height.ReportJob,
			Combo.hWnd, CHILDWNDID(IDW_REPORTLOG), hInst, 0);
												// EditBox の拡張 -------------
	SendMessage(Combo.Report.hWnd, WM_SETFONT, (WPARAM)Combo.Font.handle, 0);
	PPxRegistExEdit(NULL, Combo.Report.hWnd, 0x100000, NULL, 0, 0,
			X_askp ?
				PPXEDIT_USEALT | PPXEDIT_WANTENTER | PPXEDIT_WANTEVENT |
				PPXEDIT_TABCOMP | PPXEDIT_NOWORDBREAK | PPXEDIT_PANEMODE:
				PPXEDIT_USEALT | PPXEDIT_WANTENTER | PPXEDIT_WANTEVENT |
				PPXEDIT_TABCOMP | PPXEDIT_NOWORDBREAK);
	ShowWindow(Combo.Report.hWnd, SW_SHOWNORMAL);
	PPxCommonExtCommand(K_SETLOGWINDOW, (WPARAM)Combo.hWnd);
}

void CreateJobArea(void)
{
	TCHAR buf[8];
	DWORD tmp[2];

	// 開いてあるか確認
	if ( PPxCommonCommand(Combo.hWnd, 0, K_GETJOBWINDOW) != FALSE ) return;

	if ( Combo.hTreeWnd == NULL ) CreateBottomArea();

	thprintf(buf, TSIZEOF(buf), T("%sJ"), ComboID);
	tmp[1] = 200;
	GetCustTable(T("XC_tree"), buf, &tmp, sizeof(tmp));
	Combo.Joblist.JobAreaWidth = tmp[1];
	PPxCommonCommand(Combo.hWnd, (LPARAM)&Combo.Joblist.hWnd, K_GETJOBWINDOW);
/*
	if ( Combo.Joblist.hWnd == NULL ){
		PPxCommonCommand(Combo.hWnd, 1, K_GETJOBWINDOW);
	}
*/
}

void CreateLeftArea(DWORD mode, const TCHAR *initpath, BOOL sync)
{
	PPCTREESETTINGS pts;

	pts.mode = PPCTREE_SELECT;
	pts.width = 200;
	pts.name[0] = '\0';

	GetCustTable(T("XC_tree"), ComboID, &pts, sizeof(pts));
	if ( !mode && (pts.mode == PPCTREE_OFF) ) return;	// 表示しない

	Combo.LeftAreaWidth = pts.width;
	if ( Combo.LeftAreaWidth < 20 ) Combo.LeftAreaWidth = 20;
	ComboTreeMode = mode ? mode : pts.mode;

	InitVFSTree();
	Combo.hTreeWnd = CreateWindow(Str_TreeClass, Str_TreeClass,
			/*WS_VISIBLE |*/ WS_CHILD, 0, InfoBottom, Combo.LeftAreaWidth,
			Combo.Panes.box.bottom - InfoBottom - SplitPix,
			Combo.hWnd, CHILDWNDID(IDW_COMMONTREE), hInst, 0);
	PPc_SetTreeFlags(Combo.hWnd, Combo.hTreeWnd);
	SendMessage(Combo.hTreeWnd, VTM_INITTREE, (WPARAM)sync,
			(LPARAM)((pts.name[0] != '\0') ? pts.name : initpath) );
	ShowWindow(Combo.hTreeWnd, SW_SHOWNORMAL);
}

void CloseLeftArea(void)
{
	SaveTreeSettings(Combo.hTreeWnd, ComboID, PPCTREE_OFF, Combo.LeftAreaWidth);
	SendMessage(Combo.hTreeWnd, WM_CLOSE, 0, 0);
	Combo.hTreeWnd = 0;
	Combo.LeftAreaWidth = 0;
	SortComboWindows(SORTWIN_LAYOUTALL);
}

void CreateAddressBar(void)
{
	Combo.Height.AddrBar = Combo.Font.size.cy + 6;
	InitEditColor();
	Combo.hAddressWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, NilStr,
			WS_CHILD | WS_VSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | ES_LEFT,
			-10, -10, 10, 10, Combo.hWnd, CHILDWNDID(IDW_ADDRESS), hInst, 0);
												// EditBox の拡張 -------------
	SendMessage(Combo.hAddressWnd, WM_SETFONT, (WPARAM)Combo.Font.handle, 0);
	PPxRegistExEdit(NULL, Combo.hAddressWnd, CMDLINESIZE,
			NULL, PPXH_DIR_R | PPXH_COMMAND, PPXH_DIR_R,
			PPXEDIT_USEALT | PPXEDIT_WANTENTER | PPXEDIT_WANTEVENT | PPXEDIT_TABCOMP);
	ShowWindow(Combo.hAddressWnd, SW_SHOWNORMAL);
}

HWND InitCombo(PPCSTARTPARAM *psp)
{
	WNDCLASS wcClass;
	UINT ID;
	HMENU hMenuBar = NULL;
	DWORD tmp[2];
	int index;

	if ( Combo.hWnd != NULL ){
		if ( Combo.hWnd != BADHWND ) return Combo.hWnd;
	}
	memset(&Combo, 0, sizeof(COMBOSTRUCT));
	if ( (psp == NULL) || (psp->ComboID <= 'A') ){
		Combo.hWnd = PPxCombo(NULL);
		if ( Combo.hWnd != BADHWND ){ // 作成済み
			if ( (psp != NULL) && psp->usealone) Combo.hWnd = NULL; // alone 禁止
			return Combo.hWnd;
		}
	}else{
		ComboID[2] = psp->ComboID;
		PPxRegist(BADHWND, ComboID, PPXREGIST_COMBO_IDASSIGN);
	}
	ComboInit = 1;
	ComboThreadID = GetCurrentThreadId();

	Combo.base = PPcHeapAlloc(sizeof(COMBOITEMSTRUCT) * 2);
	Combo.show = PPcHeapAlloc(sizeof(COMBOPANES) * 2);
	memset(Combo.show, 0, sizeof(COMBOPANES));
	Combo.SizingPane = DMS_NONE;

	Combo.Closed.Items = 24;
	Combo.Closed.list = PPcHeapAlloc(sizeof(CLOSEDPPC) * Combo.Closed.Items);
	for ( index = 0; index < Combo.Closed.Items; index++ ){
		Combo.Closed.list[index].ID[0] = '\0';
	}

	ComboCust(FALSE);
									// ウィンドウ生成 -------------------------
	if ( NO_ERROR == GetCustTable(Str_WinPos, ComboID, &Combo.WinPos, sizeof(Combo.WinPos)) ){
		Combo.WndSize.cx = Combo.WinPos.pos.right - Combo.WinPos.pos.left;
		Combo.WndSize.cy = Combo.WinPos.pos.bottom - Combo.WinPos.pos.top;
	}else{
		Combo.WinPos.pos.top = Combo.WinPos.pos.left = CW_USEDEFAULT;
		Combo.WndSize.cx = 640;
		Combo.WndSize.cy = 400;
		Combo.WinPos.show = SW_SHOWDEFAULT;
	}
	if ( (psp != NULL) && (psp->show != SW_SHOWDEFAULT) ){
		Combo.WinPos.show = (BYTE)psp->show;
	}

	wcClass.style			= CS_DBLCLKS;
	wcClass.lpfnWndProc		= ComboProc;
	wcClass.cbClsExtra		= 0;
	wcClass.cbWndExtra		= 0;
	wcClass.hInstance		= hInst;
	wcClass.hIcon			= LoadIcon(hInst, MAKEINTRESOURCE(Ic_PPC));
	wcClass.hCursor			= NULL; // カーソルを動的変更する
	wcClass.hbrBackground	= NULL;
	wcClass.lpszMenuName	= NULL;
	wcClass.lpszClassName	= PPCOMBOWinClass;
	RegisterClass(&wcClass);

	InitDynamicMenu(&ComboDMenu, T("MC_menu"), ppcbar);
	if ( !(X_combos[0] & CMBS_NOMENU) ){
		hMenuBar = ComboDMenu.hMenuBarMenu;
	}
	Combo.hWnd = Combo.Panes.hFrameWnd = CreateWindowEx(0,
		PPCOMBOWinClass, PPCOMBOWinClass,
		X_combos[0] & CMBS_NOTITLE ? WS_NOTITLEOVERLAPPED : WS_OVERLAPPEDWINDOW,
		Combo.WinPos.pos.left, Combo.WinPos.pos.top,
		Combo.WndSize.cx, Combo.WndSize.cy, NULL, hMenuBar, hInst, NULL);
	if ( Combo.hWnd == NULL ) return NULL;
	FixUxTheme(Combo.hWnd, NULL);
	Combo.FontDPI = PPxCommonExtCommand(K_GETDISPDPI, (WPARAM)Combo.hWnd);
	InitSystemDynamicMenu(&ComboDMenu, Combo.hWnd);
	ComboInitFont();
	ThInit(&Combo_thGuiWork);

	#if DRAWMODE == DRAWMODE_DW
	CreateDxDraw(&Combo.DxDraw, Combo.hWnd, DxRENDER_DC);
	SetFontDxDraw(Combo.DxDraw, Combo.Font.handle ,0);
	#endif

									// GUIパーツ生成 -------------------------
// 現在、左右の幅を拡げたりすると、ログオフする必要がある問題がある
//	setflag(X_combos[0], CMBS_TABFRAME );

	if ( X_combos[0] & CMBS_TABFRAME ){
		Combo.Panes.delta.x = 0;
		Combo.Panes.delta.y = 0;
		wcClass.lpfnWndProc		= ComboFrameProc;
		wcClass.lpszClassName	= PPCOMBOWinFrameClass;
		RegisterClass(&wcClass);

		Combo.Panes.hFrameWnd = CreateWindowEx(0, PPCOMBOWinFrameClass, NilStr,
				WS_CHILD | WS_CLIPSIBLINGS | WS_HSCROLL | WS_VISIBLE,
				0, 0, 300, 300, Combo.hWnd, CHILDWNDID(ComboFrameID), hInst, NULL);
	}

	ID = ComboCommandIDFirst;
	DocksInit(&Combo.Docks, Combo.hWnd, NULL, ComboID, Combo.Font.handle, Combo.Font.size.cy, &Combo_thGuiWork, &ID);
	if ( X_combos[0] & CMBS_COMMONREPORT )	CreateReportArea();
	if ( X_combos[0] & CMBS_TOOLBAR )		ComboCreateToolBar(Combo.hWnd);
	if ( X_combos[0] & CMBS_COMMONADDR )	CreateAddressBar();
	if ( X_combos[0] & CMBS_COMMONTREE )	CreateLeftArea(0, NilStr, FALSE);
	if ( X_combos[0] & CMBS_TABALWAYS ){
		X_mpane.limit = 2;
		CreateTabBar(CREATETAB_FIRST, NULL, FALSE);
	}
	tmp[0] = 0;
//	tmp[2] = 200;
	GetCustData(T("X_jlst"), tmp, sizeof(tmp));
	if ( tmp[0] >= 3 ) CreateJobArea();

	GetCustData(T("X_mpane"), &X_mpane, sizeof(X_mpane));
	if ( X_mpane.limit < 1 ) X_mpane.limit = 50;

	ShowWindow(Combo.hWnd, Combo.WinPos.show);
	if ( ComboID[2] == 'A' ){
		if ( Combo.hWnd != PPxCombo(Combo.hWnd) ){	// DLL に登録する
			PostMessage(Combo.hWnd, WM_CLOSE, 0, 0);
		}
	}else{
		PPxRegist(Combo.hWnd, ComboID, PPXREGIST_COMBO_IDASSIGN);
	}
	dd_combo_init();
	LoadWallpaper(NULL, Combo.hWnd, ComboID);
	return Combo.hWnd;
}

const TCHAR TGHSPROP[] = T("PcmbTabGH");

BOOL TabGroupLDownMouse(TABHOOKSTRUCT *THS, /*WPARAM wParam,*/ LPARAM lParam)
{
	TC_HITTESTINFO th;
	TC_ITEM tie;
	int index;

	LPARAMtoPOINT(th.pt, lParam);
	index = TabCtrl_HitTest(THS->hWnd, &th);

	// Tabが指すIndexを求める
	tie.mask = TCIF_PARAM;
	if ( IsTrue(TabCtrl_GetItem(THS->hWnd, index, &tie)) ){

		if ( TabCtrl_GetCurSel(THS->hWnd) == index ){
			// 選択済みのタブであるが、現在窓でなければ、
			// 改めて選択処理をさせて表示ペインに割当てを行う
			if ( (HWND)tie.lParam != hComboFocusWnd ){
				NMHDR nmh;

				nmh.hwndFrom = THS->hWnd;
				SelectChangeTab(&nmh);
			}
		}
	}
	return FALSE;
}

int GetTabGroupFromPos(TABHOOKSTRUCT *THS, POINT *spos, POINT *cpos, int *TargetTab)
{
	HWND hTargetWnd = WindowFromPoint(*spos);
	int showindex = -1, tabpane;
	TC_HITTESTINFO th;

	if ( THS->hWnd == hTargetWnd ){
		for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
			if ( Combo.show[tabpane].tab.hSelecterWnd == hTargetWnd ){
				showindex = tabpane;
				break;
			}
		}
	}

	*TargetTab = showindex;
	if ( showindex < 0 ) return -1;
	th.pt = *spos;
	ScreenToClient(hTargetWnd, &th.pt);
	if ( cpos != NULL ) *cpos = th.pt;
	return TabCtrl_HitTest(hTargetWnd, &th);
}

void TabGroupMoveMouse(TABHOOKSTRUCT *THS)
{
	int tabindex;
	int showindex;

//	if ( THS->ms.PushButton <= MOUSEBUTTON_CANCEL ) return; // キャンセル状態

	if ( THS->ms.PushButton == MOUSEBUTTON_W ){
		THS->DDinfo.showindex = THS_SHOW_CANCELDRAG;
		SetCursor( LoadCursor(NULL, IDC_ARROW) );
	}

	if ( THS->ms.mode != MOUSEMODE_DRAG ) return;

	if ( THS->DDinfo.showindex == THS_SHOW_NODRAG ){ // D&D 開始
		POINT cpos;

		THS->DDinfo.width = -1;
		if ( !(X_combos[1] & CMBS1_TABMODIFYALT) || (GetKeyState(VK_MENU) & B15) ){
			THS->DDinfo.tabindex = GetTabGroupFromPos(THS, &THS->ms.PushScreenPoint, &cpos, &THS->DDinfo.showindex);
			if ( THS->DDinfo.tabindex < 0 ){
				THS->DDinfo.showindex = THS_SHOW_CANCELDRAG;
			}else{
				RECT box;

				TabCtrl_GetItemRect(Combo.show[THS->DDinfo.showindex].tab.hWnd, THS->DDinfo.tabindex, &box);
				if ( (box.right - SIZEBAND) < cpos.x ){
					THS->DDinfo.width = box.left;
				}
				if ( ShowCloseButton != SCB_DISABLE ){
					ShowCloseButton = SCB_SHOW;
					InvalidateRect(THS->hWnd, NULL, TRUE);
				}
			}
		}
	}
	tabindex = GetTabGroupFromPos(THS, &THS->ms.MovedScreenPoint, NULL, &showindex);
	if ( (((X_combos[0] & (CMBS_TABSEPARATE | CMBS_TABEACHITEM)) == CMBS_TABSEPARATE) && (showindex != THS->DDinfo.showindex)) ||
			((showindex == THS->DDinfo.showindex) && (tabindex == THS->DDinfo.tabindex)) ){
		showindex = -1;
	}
	SetCursor(LoadCursor(NULL, showindex < 0 ? IDC_NO : IDC_UPARROW) );
}

void TabGroup_MovePosition(HWND hSrcTabWnd, int SrcTabIndex, int DestShowIndex, int DestTabIndex)
{
	TC_ITEM tie;
	TCHAR cap[VFPS + 16];
	HWND hDestTabWnd = Combo.show[DestShowIndex].tab.hSelecterWnd;
	int srcselect;

	srcselect = TabCtrl_GetCurSel(hSrcTabWnd);
	if ( SrcTabIndex < 0 ) SrcTabIndex = srcselect;
	if ( DestTabIndex < 0 ) DestTabIndex = TabCtrl_GetItemCount(hDestTabWnd);
	if ( (hDestTabWnd == hSrcTabWnd) && (DestTabIndex == (SrcTabIndex + 1)) ){
		DestTabIndex++;
	}

	tie.mask = TCIF_TEXT | TCIF_PARAM;
	tie.cchTextMax = TSIZEOF(cap);
	tie.pszText = cap;
	if ( IsTrue(TabCtrl_GetItem(hSrcTabWnd, SrcTabIndex, &tie)) ){
		TABGROUP movegroup, *newgroup;
		int srcpane,dstpane;

		for ( srcpane = 0 ; srcpane < Combo.Tabs ; srcpane++ ){
			if ( Combo.show[srcpane].tab.hSelecterWnd == hSrcTabWnd ){
				movegroup = Combo.show[srcpane].tab.group[SrcTabIndex];
				break;
			}
		}

		for ( dstpane = 0 ; dstpane < Combo.Tabs ; dstpane++ ){
			if ( Combo.show[dstpane].tab.hSelecterWnd == hSrcTabWnd ){
				break;
			}
		}

		SendMessage(hDestTabWnd, TCM_INSERTITEM, (WPARAM)DestTabIndex, (LPARAM)&tie);
		newgroup = HeapReAlloc( hProcessHeap, 0, Combo.show[dstpane].tab.group, sizeof(TABGROUP) * (Combo.show[dstpane].tab.groupcount + 1) );
		if ( newgroup != NULL ){
			if ( DestTabIndex > Combo.show[dstpane].tab.groupcount ){
				DestTabIndex = Combo.show[dstpane].tab.groupcount;
			}

			memmove(&newgroup[DestTabIndex + 1],
					&newgroup[DestTabIndex],
					(Combo.show[dstpane].tab.groupcount - (DestTabIndex)) * sizeof(TABGROUP));

			newgroup[DestTabIndex] = movegroup;
			Combo.show[dstpane].tab.group = newgroup;
			Combo.show[dstpane].tab.groupcount++;

			if ( (hDestTabWnd == hSrcTabWnd) && (DestTabIndex <= SrcTabIndex) ){
				SrcTabIndex++;
			}


			TabCtrl_DeleteItem(hSrcTabWnd, SrcTabIndex);
			memmove(&Combo.show[srcpane].tab.group[SrcTabIndex],
					&Combo.show[srcpane].tab.group[SrcTabIndex + 1],
					(Combo.show[srcpane].tab.groupcount - (SrcTabIndex + 1)) * sizeof(TABGROUP));
			Combo.show[srcpane].tab.groupcount--;
			SortComboWindows(SORTWIN_LAYOUTPAIN);
		}
	}
}

void DrawGroupTab(DRAWITEMSTRUCT *dis)
{
	TC_ITEM tie;
	TCHAR buf[CMDLINESIZE];
	int select_index;
	RECT box;
	COLORREF oldfc = C_AUTO, oldbc = C_AUTO;

	// ※ DrawTab の後に、Windowsがタブの枠を描画している

	tie.mask = TCIF_TEXT | TCIF_PARAM;
	tie.pszText = buf;
	tie.cchTextMax = CMDLINESIZE;
	if ( TabCtrl_GetItem(dis->hwndItem, dis->itemID, &tie) == FALSE ) return;

	if ( dis->itemState & ODS_SELECTED ){
		select_index = CI_TabSelected;
	}else{
		select_index = CI_TabNoSelected;
	}

	// 文字色
	if ( C_capt[select_index] != C_AUTO ) {
		oldfc = SetTextColor(dis->hDC, C_capt[select_index]);
	}else if ( X_uxt_color >= UXT_MINPRESET ){
		oldfc = SetTextColor(dis->hDC, C_DialogText);
	}

	// 背景色
	{
		HBRUSH hBack;
		COLORREF col = C_AUTO;

		if ( C_capt[select_index + CI_TabBackOffset] != C_AUTO ) {
			col = C_capt[select_index + CI_TabBackOffset];
		}else if ( X_uxt_color >= UXT_MINPRESET ){
			col = (select_index == CI_TabSelected) ? C_FocusBack : C_DialogBack;
//		}else if ( select_index == CI_TabFocus ){
//			col = GetSysColor(COLOR_3DHIGHLIGHT);
		}else if ( select_index == CI_TabSelected ){
			col = GetSysColor(COLOR_3DLIGHT);
		}
		if ( col != C_AUTO ){
			hBack = CreateSolidBrush(col);
			FillBox(dis->hDC, &dis->rcItem, hBack);
			DeleteObject(hBack);
			oldbc = SetBkColor(dis->hDC, col);
		}
	}

	box = dis->rcItem; // TCM_GETITEMRECT で得られるRECTより左右上下2pixずつ小さい
	box.left += 6;
	box.top += 4;
	#pragma warning(suppress: 6054) // TabCtrl_GetItem で取得
	DrawText(dis->hDC, buf, tstrlen32(buf), &box, DT_LEFT | DT_NOPREFIX);
	//DT_END_ELLIPSIS DT_PATH_ELLIPSIS

	if ( oldfc != C_AUTO ) SetTextColor(dis->hDC, oldfc);
	if ( oldbc != C_AUTO ) SetBkColor(dis->hDC, oldbc);
}

void PaintGroupTab(HWND hWnd)
{
	PAINTSTRUCT ps;
	DRAWITEMSTRUCT dis;
	UINT cursor, focus;
	DWORD maxindex;
	HFONT hOldFont;
	HBRUSH hFrameBrush;

	BeginPaint(hWnd, &ps);

	dis.hwndItem = hWnd;
	dis.hDC = ps.hdc;

	hOldFont = SelectObject(ps.hdc, (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0));
	hFrameBrush = CreateSolidBrush(C_EdgeLine);
	SetBkColor(ps.hdc, C_DialogBack);
	cursor = (UINT)TabCtrl_GetCurSel(hWnd);
	focus = (UINT)TabCtrl_GetCurFocus(hWnd);
	maxindex = TabCtrl_GetItemCount(hWnd);
	for ( dis.itemID = 0 ; dis.itemID < maxindex ; dis.itemID++ ){
		TabCtrl_GetItemRect(hWnd, dis.itemID, &dis.rcItem);
		if ( (cursor != focus) && (dis.itemID == focus) ){
			FrameRect(ps.hdc, &dis.rcItem, hControlBackBrush);
		}else{
			FrameRect(ps.hdc, &dis.rcItem, hFrameBrush);
		}
		dis.rcItem.left++;
		dis.rcItem.top++;
		dis.rcItem.right--;
		dis.rcItem.bottom--;
		dis.itemState = (dis.itemID == cursor) ? ODS_SELECTED : 0;
		DrawGroupTab(&dis);
	}
	DeleteObject(hFrameBrush);
	SelectObject(ps.hdc, hOldFont);
	EndPaint(hWnd, &ps);
}

// タブの DnD 完了処理
void TabGroupUp_DnD_Mouse(TABHOOKSTRUCT *THS)
{
	int targetshow, targettabindex;
	TC_HITTESTINFO th;

	GetMessagePosPoint(th.pt);

	targettabindex = GetTabGroupFromPos(THS, &th.pt, NULL, &targetshow);
	if ( (THS->DDinfo.showindex >= 0) && (targetshow >= 0) ){ // tab入替
		TabGroup_MovePosition(THS->hWnd, THS->DDinfo.tabindex, targetshow, targettabindex);
	}
}


void TabGroupUpMouse(HWND hWnd, TABHOOKSTRUCT *THS, LPARAM lParam, int button)
{
	TC_HITTESTINFO th;

	if ( button <= MOUSEBUTTON_CANCEL ) return;

	LPARAMtoPOINT(th.pt, lParam);

	if ( THS->DDinfo.showindex != THS_SHOW_NODRAG ){ // タブ D&D
		TabGroupUp_DnD_Mouse(THS);
		return;
	}

	if ( button == MOUSEBUTTON_R ){
		int showindex;
		int groupindex = -1;
		COMBOTABINFO *tabs;

		for ( showindex = 0 ; showindex < Combo.Tabs ; showindex++ ){
			if ( hWnd == Combo.show[showindex].tab.hSelecterWnd ){
				groupindex = TabCtrl_HitTest(hWnd, &th);
				if ( groupindex < 0 ) groupindex = TabCtrl_GetCurSel(hWnd);
				break;
			}
		}

		if ( showindex < Combo.Tabs ){
			POINT pos;
			GetMessagePosPoint(pos);

			tabs = &Combo.show[showindex].tab;
			if ( groupindex < 0 ){
				TabMenu(hWnd, -1, showindex, &pos);
				return;
			}

			if ( groupindex < tabs->groupcount ){
				HWND hOldTabWnd, hNewTabWnd;

				hNewTabWnd = tabs->group[groupindex].hWnd;
				if ( tabs->hWnd == hNewTabWnd ){
					TabMenu(hWnd, -1, showindex, &pos);
					return;
				}
				hOldTabWnd = tabs->hWnd;
				tabs->hWnd = hNewTabWnd;
				TabMenu(hWnd, -1, showindex, &pos);
				if ( tabs->hWnd == hNewTabWnd ) tabs->hWnd = hOldTabWnd;
			}
		}
		return;
	}
}

LRESULT CALLBACK TabGroupHookProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	TABHOOKSTRUCT *THS;

	THS = (TABHOOKSTRUCT *)GetProp(hWnd, TGHSPROP);
	if ( THS == NULL ) return DefWindowProc(hWnd, iMsg, wParam, lParam);
	switch(iMsg){
		case WM_DESTROY: {
			WNDPROC hOldProc;

			hOldProc = THS->hOldProc;
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)hOldProc);
			RemoveProp(hWnd, TGHSPROP);
			ProcHeapFree(THS);
			return CallWindowProc(hOldProc, hWnd, iMsg, wParam, lParam);
		}
		case WM_NCHITTEST:
			return HTCLIENT;	// 非タブ領域上のマウス操作も取得可能にする

		case WM_PAINT:
			if ( X_uxt_color < UXT_MINMODIFY ) break;
			PaintGroupTab(hWnd);
			return 0;

		case WM_ERASEBKGND:
			if ( UseCCDrawBack != 2 ) break;
			EraseTabBkgnd(hWnd, wParam);
			return 1;
							// マウスのクライアント領域押下 ------
		case WM_LBUTTONDOWN:
			if ( TabGroupLDownMouse(THS, /*wParam,*/ lParam) ) return 0;
		// WM_RBUTTONDOWN へ
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
			if ( THS->DDinfo.showindex == THS_SHOW_NODRAG ){
				CallWindowProc(THS->hOldProc, hWnd, iMsg, wParam, lParam);
			}
			PPxDownMouseButtonX(THS, wParam, lParam);
//			SetTabGroupCursor(hWnd);
			return 0;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_XBUTTONUP:
			if ( THS->DDinfo.showindex == THS_SHOW_NODRAG ){
				 CallWindowProc(THS->hOldProc, hWnd, iMsg, wParam, lParam);
			}
			TabGroupUpMouse(hWnd, THS, lParam, PPxUpMouseButton(&THS->ms, wParam));
			THS->DDinfo.showindex = THS_SHOW_NODRAG;
			return 0;

		case WM_MOUSEMOVE:
			PPxMoveMouse(&THS->ms, hWnd, lParam);
			TabGroupMoveMouse(THS);
			break;

		case WM_MOUSEWHEEL:
			if ( !(X_combos[1] & CMBS1_TABMODIFYALT) || (GetKeyState(VK_MENU) & B15) ){
				int i;

				i = PPxWheelMouse(&THS->ms, hWnd, wParam, lParam);
				if ( i != 0 ){
					int key = i > 0 ? VK_LEFT : VK_RIGHT;
					PostMessage(hWnd, WM_KEYDOWN, key, 0);
					PostMessage(hWnd, WM_KEYUP, key, 0);
					PostMessage(hWnd, WM_KEYDOWN, VK_SPACE, 0);
					PostMessage(hWnd, WM_KEYUP, VK_SPACE, 0);
				}
			}
			return 1;
	}
	return CallWindowProc(THS->hOldProc, hWnd, iMsg, wParam, lParam);
}

void CreateTabGroupSelector(COMBOTABINFO *tabs)
{
	HWND hTabWnd;
	TABHOOKSTRUCT *THS;
	DWORD style = WS_CHILD | WS_VISIBLE | TCS_TOOLTIPS |
			TCS_HOTTRACK | TCS_FOCUSNEVER | CCS_NODIVIDER;

	THS = HeapAlloc(hProcessHeap, 0, sizeof(TABHOOKSTRUCT));
	if ( THS == NULL ) return;

//	if ( WinColors.ExtraDrawFlags & EDF_DIALOG_BACK ) LoadCCDrawBack();

//	if ( X_combos[0] & CMBS_TABFIXEDWIDTH ) setflag(style, TCS_FIXEDWIDTH);
	if ( X_combos[0] & CMBS_TABMULTILINE )  setflag(style, TCS_MULTILINE);
	if ( X_combos[0] & CMBS_TABBUTTON ){ // タブの行位置が変化しないようにする時の設定。但し、見た目が変わる
		setflag(style, TCS_BUTTONS | TCS_FLATBUTTONS);
	}
	if ( WinColors.ExtraDrawFlags & EDF_DIALOG_BACK ){
		setflag(style, TCS_OWNERDRAWFIXED);
	}

	hTabWnd = CreateWindowEx(0, WC_TABCONTROL, NilStr, style,
			-10, -10, 10, 10, Combo.hWnd /* Combo.Panes.hFrameWnd */,
			CHILDWNDID(IDW_TABCONTROL), hInst, NULL);
	if ( hTabWnd == NULL ){
		ProcHeapFree(THS);
		return;
	}else{
		TCHAR name[VFPS];
		int i;

		THS->hWnd = hTabWnd;
		THS->DDinfo.showindex = THS_SHOW_NODRAG;
		PPxInitMouseButton(&THS->ms);
		SetProp(THS->hWnd, TGHSPROP, (HANDLE)THS);
		THS->hOldProc = (WNDPROC)
				SetWindowLongPtr(THS->hWnd, GWLP_WNDPROC, (LONG_PTR)TabGroupHookProc);

		// ↓TCS_EX_REGISTERDROP は親でDrop処理するため不要
		SendMessage(hTabWnd, TCM_SETEXTENDEDSTYLE, 0, TCS_EX_FLATSEPARATORS);
		SendMessage(hTabWnd, WM_SETFONT, (WPARAM)GetControlFont(Combo.FontDPI, &Combo.cfs), TMAKELPARAM(TRUE, 0));

		tabs->hSelecterWnd = hTabWnd;
		for ( i = 0; i < tabs->groupcount; i++ ){
			TC_ITEM tie;

			GetWindowText(tabs->group[i].hWnd, name, VFPS);
			tie.mask = TCIF_TEXT | TCIF_PARAM;
			tie.pszText = name;
			tie.lParam = (LPARAM)tabs->group[i].hWnd;
			SendMessage(hTabWnd, TCM_INSERTITEM, (WPARAM)i, (LPARAM)&tie);
		}
	}
}

ERRORCODE NewTabGroupMain(int targetpane, const TCHAR *groupname)
{
	COMBOTABINFO *tabs;
	BOOL first = FALSE;

	if ( targetpane < 0 ){ // NewTabGroup_First
		targetpane = -targetpane - 1;
		first = TRUE;
	}
	tabs = &Combo.show[targetpane].tab;
#if 0 // 1.97 常に全ペインがタブ情報を持つようになったので↑に変更
	if ( (X_combos[0] & (CMBS_TABSEPARATE | CMBS_TABEACHITEM)) == (CMBS_TABSEPARATE | CMBS_TABEACHITEM) ){ // ペイン別独立タブ
		tabs = &Combo.show[targetpane].tab;
	}else{ // 共通タブ内容
		tabs = &Combo.show[(targetpane < Combo.Tabs) ? targetpane : 0].tab;
	}
#endif
	if ( tabs->hWnd == NULL ){ // タブが 0 の時は、グループ無し
		CreateTabBar(CREATETAB_APPEND, groupname, first);
		SortComboWindows(SORTWIN_LAYOUTPAIN);
		InvalidateRect(Combo.hWnd, NULL, TRUE);
	}else{ // グループ新規・追加
		TABGROUP *newgroup;

		if ( tabs->groupcount == 0 ){
			newgroup = PPcHeapAlloc(sizeof(TABGROUP) * 2);
			if ( newgroup != NULL ){
				newgroup->hWnd = tabs->hWnd;
				if ( !first ) tabs->groupcount++;
			}
			tabs->show_all = (X_combos[1] & CMBS1_SELECTEDGROUP) ? 0 : 1;
		}else{
			newgroup = HeapReAlloc( hProcessHeap, 0, tabs->group, sizeof(TABGROUP) * (tabs->groupcount + 1) );
		}
		if ( newgroup != NULL ){
			HWND hTabWnd;

			hTabWnd = CreateComboTabBar(groupname);
			if ( hTabWnd == NULL ){
				tabs->group = newgroup;
				return ERROR_INVALID_DATA;
			}
			if ( tabs->hSelecterWnd != NULL ){ //グループセレクタにグループ追加
				TC_ITEM tie;
				TCHAR name[VFPS];

				GetWindowText(hTabWnd, name, VFPS);
				tie.mask = TCIF_TEXT | TCIF_PARAM;
				tie.pszText = name;
				tie.lParam = (LPARAM)hTabWnd;
				SendMessage(tabs->hSelecterWnd, TCM_INSERTITEM, (WPARAM)tabs->groupcount, (LPARAM)&tie);
			}

			tabs->group = newgroup;
			tabs->group[tabs->groupcount].hCurWnd = NULL;
			tabs->group[tabs->groupcount++].hWnd = tabs->hWnd = hTabWnd;

			if ( !(X_combos[1] & CMBS1_HIDESELECTOR) &&
				 (tabs->hSelecterWnd == NULL) ){
				CreateTabGroupSelector(tabs);
			}

			SortComboWindows(SORTWIN_LAYOUTPAIN);
			InvalidateRect(Combo.hWnd, NULL, TRUE);
		}
	}
	return NO_ERROR;
}

const TCHAR *NewTabGroupName(TCHAR *name)
{
	TCHAR *tail, usename[VFPS];
	int showindex, groupindex;

	tail = name;
	name[0] = '\0';
	GetCustTable(Str_others, T("NewTabGroup"), name, TSTROFF(MAX_PATH));
	if ( name[0] == '\0' ) tail = tstpcpy(name, T("group"));

	for (;;){
		thprintf(tail, 20, T("%d"), Group_id++);
		for ( showindex = 0; showindex < Combo.Tabs; showindex++){
			COMBOTABINFO *tabs;

			tabs = &Combo.show[showindex].tab;
			if ( tabs->group != NULL ){
				for ( groupindex = 0; groupindex < tabs->groupcount; groupindex++){
					if ( GetWindowText(tabs->group[groupindex].hWnd, usename, VFPS) == FALSE ){
						continue;
					}
					if ( tstrcmp(name, usename) != 0 ) continue;
					goto used;
				}
			}else{
				if ( tabs->hWnd != NULL ){
					if ( GetWindowText(tabs->hWnd, usename, VFPS) == FALSE ){
						continue;
					}
					if ( tstrcmp(name, usename) != 0 ) continue;
					goto used;
				}
			}
		}
		return name; // 重複無し
		used: ;
		continue;
	}
}

ERRORCODE NewTabGroup(int targetpane, const TCHAR *groupname)
{
	TCHAR groupnamebuf[VFPS];

	if ( Combo.Tabs == 0 ){
		CreateTabBar(CREATETAB_APPEND, NULL, FALSE);
	}

	if ( (groupname == NULL) || (*groupname == '\0') ){
		groupname = NewTabGroupName(groupnamebuf);
	}

	if ( (X_combos[0] & CMBS_TABEACHITEM) || (Combo.ShowCount <= 1) ){ // タブ内容がペイン毎に異なる
		return NewTabGroupMain(targetpane, groupname);
	}else if ( X_combos[0] & CMBS_TABSEPARATE ){ // タブ内容共通、ペイン別タブ
		int pane;
		ERRORCODE result = NO_ERROR;

		if ( targetpane < 0 ){ // NewTabGroup_First
			result = NewTabGroupMain(targetpane, groupname);
			targetpane = -targetpane - 1;
		}else{
			targetpane = -1;
		}
		for (pane = 0; pane < Combo.ShowCount; pane++){
			if ( pane == targetpane ) continue;
			result = NewTabGroupMain(pane, groupname);
		}
		return result;
	}else{  // タブ内容共通、ペイン共通タブ
		int pane;
		COMBOTABINFO *newinfo, *baseinfo;

		NewTabGroupMain(targetpane, groupname);

		if ( targetpane < 0 ){ // NewTabGroup_First
			targetpane = -targetpane - 1;
		}

		// 他にペインに追加したグループをコピーする
		baseinfo = &Combo.show[targetpane].tab;
		for ( pane = 0; pane < Combo.ShowCount; pane++ ){
			int gpmax;

			if ( pane == targetpane ) continue;
			newinfo = &Combo.show[pane].tab;
			gpmax = baseinfo->groupcount;
			if ( newinfo->groupcount == 0 ){
				*newinfo = *baseinfo;
				newinfo->groupcount = 0;
				newinfo->group = PPcHeapAlloc(sizeof(TABGROUP) * gpmax);
			}else{
				newinfo->group = HeapReAlloc(hProcessHeap, 0, newinfo->group, sizeof(TABGROUP) * gpmax );
			}
			for( ; newinfo->groupcount < gpmax; newinfo->groupcount++ ){
				newinfo->group[newinfo->groupcount].hWnd = baseinfo->group[newinfo->groupcount].hWnd;
				newinfo->group[newinfo->groupcount].hCurWnd = NULL;
			}
		}
		return NO_ERROR;
	}
}

void NewTabGroupCare(int showindex)
{
	if ( !(X_combos[0] & CMBS_TABSEPARATE) && (Combo.ShowCount > 1) ){
		int index;

		for ( index = 0; index < Combo.ShowCount; index++){
			if ( index != showindex ){
				TCHAR param[32];

				thprintf(param, TSIZEOF(param), T("-pane:%d"), index);
				CallPPcParam(Combo.hWnd, param);
			}
		}
	}
}

void PaneExecuteParam(int baseindex, const TCHAR *param)
{
	COPYDATASTRUCT copydata;

	SkipSpace(&param);

	copydata.dwData = 'H' + 0x100;
	copydata.cbData = TSTRSIZE32(param);
	copydata.lpData = (PVOID)param;
	SendMessage(Combo.base[baseindex].hWnd, WM_COPYDATA, 0, (LPARAM)&copydata);
}

// 共用タブでも該当ペインのタブを見つける
HWND GetTabWndForGetPane(HWND hTagetWnd, int showindex)
{
	if ( (Combo.Tabs <= showindex)  ){
		COMBOTABINFO *tabinfo;

		tabinfo = &Combo.show[0].tab; // ペイン共用タブなので必ず１番目
		if ( tabinfo->groupcount ){
			int group;

			for ( group = 0; group < tabinfo->groupcount; group++ ){
				HWND hTabWnd;
				TC_ITEM tie;
				int tabindex;
				int tabcount;

				hTabWnd = tabinfo->group[group].hWnd;

				tabcount = TabCtrl_GetItemCount(hTabWnd);
				for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
					tie.mask = TCIF_PARAM;
					if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ){
						continue;
					}

					if ( (HWND)tie.lParam == hTagetWnd ) return hTabWnd;
				}
			}
		}
	}
	return Combo.show[showindex].tab.hWnd;
}

int GetPaneBaseIndexParam(const TCHAR **paramptr, int *targetpane, int DefaultBaseIndex)
{
	int baseindex = -1;
	int index;
	BOOL hideindex = FALSE; // "h" 非表示タブ
	BOOL usetabindex = FALSE;	// "t" FALSEならshowindex;
	BOOL relative = FALSE;	// "+" / "-" FALSEなら左端からの位置
	BOOL useactive = FALSE; // "a"
	HWND hWnd = hComboFocusWnd;
	TCHAR paramu;
	const TCHAR *param;
	const TCHAR *p2;

	param = *paramptr;
	paramu = upper(*param);
	if ( paramu == 'H' ){
		param++;
		if ( Combo.Tabs ) hideindex = TRUE;
	}else if ( paramu == 'T' ){
		param++;
		if ( Combo.Tabs ) usetabindex = TRUE;
	}
	if ( *param == 'a' ){
		param++;
		useactive = TRUE;
	}

	paramu = upper(*param);
	if ( paramu == 'C' ){ // PPc ID ?
		TCHAR cc;
		const TCHAR *cptr;

		cptr = param;
		cc = *(cptr + 1);
		if ( cc == '_' ){
			cc = *(cptr + 2);
			cptr++;
		}
		if ( Isalpha(cc) ){
			if ( Islower(*(cptr + 2)) ){ // CZxyy
				int bindex = GetComboBaseIndex((DefaultBaseIndex >= 0) ?
						Combo.base[DefaultBaseIndex].hWnd :
						hComboFocusWnd);
				if ( bindex >= 0 ){
					PPC_APPINFO *cinfo = Combo.base[bindex].cinfo;
					if ( cinfo != NULL ){
						hWnd = GetPPxhWndFromID(&cinfo->info, &param, NULL);
					}
				}
			}else{
				TCHAR cid[REGEXTIDSIZE] = T("C_A");

				cid[2] = upper(cc);
				param = cptr + 2;
				hWnd = PPxGetHWND(cid);
			}
		}
		if ( (X_combos[0] & CMBS_TABEACHITEM) && (hWnd != NULL) ){
			*targetpane = GetComboShowIndexAll(hWnd);
		}
	}else if ( paramu == '~' ){
		param++;

		baseindex = GetPairPaneComboBaseIndex( (DefaultBaseIndex >= 0) ?
				Combo.base[DefaultBaseIndex].hWnd :
				hComboFocusWnd);
		if ( baseindex < 0 ) return -1;
		hWnd = Combo.base[baseindex].hWnd;
		if ( (X_combos[0] & CMBS_TABEACHITEM) && (hWnd != NULL) ){
			*targetpane = GetComboShowIndexAll(hWnd);
		}
	}else if ( paramu == 'L' ){
		param++;
		hWnd = Combo.base[Combo.show[0].baseNo].hWnd;
		if ( (X_combos[0] & CMBS_TABEACHITEM) && (hWnd != NULL) ){
			*targetpane = GetComboShowIndexAll(hWnd);
		}
	}else if ( paramu == 'R' ){
		param++;
		hWnd = hComboSubPaneFocus;
		if ( hWnd == NULL ) return -1;
		if ( (X_combos[0] & CMBS_TABEACHITEM) && (hWnd != NULL) ){
			*targetpane = GetComboShowIndexAll(hWnd);
		}
	}else if ( upper(*param) == 'E' ){
		param++;
		if ( IsTrue(usetabindex) ){
			TC_ITEM tie;
			int tabindex = TabCtrl_GetItemCount(Combo.show[*targetpane].tab.hWnd);
			if ( tabindex < 0 ) return -1;
			tie.mask = TCIF_PARAM;
			if ( FALSE == TabCtrl_GetItem(Combo.show[*targetpane].tab.hWnd, tabindex - 1, &tie) ){
				return -1;
			}
			hWnd = (HWND)tie.lParam;
		}else{
			hWnd = Combo.base[Combo.show[Combo.ShowCount - 1].baseNo].hWnd;
		}
		if ( (X_combos[0] & CMBS_TABEACHITEM) && (hWnd != NULL) ){
			*targetpane = GetComboShowIndexAll(hWnd);
		}
	}

	if ( (*param == '+') || (*param == '-') ) relative = TRUE;
	p2 = param;
	index = GetIntNumber(&param);
	if ( p2 == param ){ // 数値指定無し…現在窓
		if ( usetabindex ){
			baseindex = DefaultBaseIndex;
		}else{
			baseindex = GetComboBaseIndex(hWnd);
		}
	}else{
		if ( IsTrue(useactive) ){ // ax アクティブ基準
			int bindex, activeid, activeid2, baseindex2 = -1, showindex = GetComboShowIndex(hWnd);
			#define ActShwCheck(bindex, showindex) ((X_combos[0] & CMBS_TABEACHITEM) ? (GetTabItemIndex(Combo.base[bindex].hWnd, showindex) >= 0) : (GetComboShowIndex(Combo.base[bindex].hWnd) < 0) )

			if ( showindex < 0 ) return -1;
			baseindex = -1;
			if ( index < 0 ){ // 一つ前のアクティブを探す
				activeid = activeid2 = Combo.Active.low - 1;
				// 最大値を求める
				for ( bindex = 0 ; bindex < Combo.BaseCount ; bindex++ ){
					int aid;

					aid = Combo.base[bindex].ActiveID;
					if ( (aid > activeid) && ActShwCheck(bindex, showindex) ){
						activeid = aid;
						baseindex2 = bindex;
					}
				}
				if ( baseindex2 < 0 ) return -1;

				// ２番目を求める
				for ( bindex = 0 ; bindex < Combo.BaseCount ; bindex++ ){
					int aid;

					aid = Combo.base[bindex].ActiveID;
					if ( (aid > activeid2) && (aid < activeid) && ActShwCheck(bindex, showindex) ){
						activeid2 = aid;
						baseindex = bindex;
					}
				}
				if ( baseindex < 0 ) return -1;

				Combo.base[baseindex2].ActiveID = --Combo.Active.low;
			}else{ // 一番古いアクティブを探す
				// 最小値を求める
				activeid = Combo.Active.high;

				for ( bindex = 0 ; bindex < Combo.BaseCount ; bindex++ ){
					int aid;

					aid = Combo.base[bindex].ActiveID;
					if ( (aid < activeid) && ActShwCheck(bindex, showindex) ){
						activeid = aid;
						baseindex = bindex;
					}
				}
				if ( baseindex < 0 ) return -1;
			}
		}else if ( IsTrue(hideindex) ){ // hx 非表示タブ
			HWND hTargetTabWnd = GetTabWndForGetPane(hWnd, *targetpane);
			int count, offset = 1;

			if ( IsTrue(relative) ){
				if ( baseindex < 0 ){
					int tabindex;
					TC_ITEM tie;

					tabindex = TabCtrl_GetCurSel(hTargetTabWnd);
					tie.mask = TCIF_PARAM;
					TabCtrl_GetItem(hTargetTabWnd, tabindex, &tie);
					baseindex = GetComboBaseIndex((HWND)tie.lParam);
				}
				if ( index < 0 ){
					offset = -1;
					index = -index;
				}
			}
			if ( baseindex < 0 ) baseindex = 0;

			count = Combo.BaseCount;
			if ( index ){
				int tabcount = TabCtrl_GetItemCount(hTargetTabWnd);
				int tabindex = GetTabItemIndex(Combo.base[baseindex].hWnd, *targetpane);
				for (;;){
					TC_ITEM tie;

					tabindex += offset;
					if ( tabindex >= tabcount ){
						tabindex = 0;
					}else if ( tabindex < 0 ){
						tabindex = tabcount - 1;
					}

					tie.mask = TCIF_PARAM;
					TabCtrl_GetItem(hTargetTabWnd, tabindex, &tie);
					if ( GetComboShowIndex((HWND)tie.lParam) < 0 ){
						int bi = GetComboBaseIndex((HWND)tie.lParam);

						if ( bi >= 0 ){
							index--;
							if ( index == 0 ){
								baseindex = bi;
								break;
							}
						}
					}
					if ( !(--count) ) return -1; // ない
				}
			}
		}else if ( IsTrue(usetabindex) ){	// tx タブ基準
			int tabindex;
			TC_ITEM tie;

			if ( relative == FALSE ){
				tabindex = index;
			}else{
				tabindex = GetTabItemIndex(hWnd, *targetpane);
				if ( tabindex < 0 ) return -1;
				tabindex += index;
			}

			if ( tabindex < 0 ) return -1;

			tie.mask = TCIF_PARAM;
			if ( FALSE == TabCtrl_GetItem(GetTabWndForGetPane(hWnd, *targetpane), tabindex, &tie) ){
				return -1;
			}
			baseindex = GetComboBaseIndex((HWND)tie.lParam);
		}else{	// show pane 指定
			int showindex;

			if ( relative == FALSE ){
				showindex = index;
			}else{
				showindex = GetComboShowIndex(hWnd);
				if ( showindex < 0 ) return -1;
				showindex += index;
			}
			if ( (showindex < 0) || (showindex >= Combo.ShowCount) ) return -1;
			baseindex = Combo.show[showindex].baseNo;
			*targetpane = showindex;
		}
	}
	*paramptr = param;
	return baseindex;
}

void PaneColorCommand(int baseindex, const TCHAR *param)
{
	if ( Combo.base[baseindex].cinfo == NULL ) return;
	if ( SkipSpace(&param) != ',' ){
		Combo.base[baseindex].tabtextcolor = GetColor(&param, TRUE);
	}
	if ( SkipSpace(&param) == ',' ){
		param++;
		Combo.base[baseindex].tabbackcolor = GetColor(&param, TRUE);
	}
	SetTabColor(baseindex);
}

TCHAR PaneNextParam(const TCHAR **param)
{
	TCHAR c;

	c = SkipSpace(param);
	if ( (c != ':') && (c != ',') ) return c;
	(*param)++;
	return SkipSpace(param);
}

void PaneCloseCommand(int targetpane, int baseindex, int mode, const TCHAR *param)
{
	BOOL locked;

	locked = (PaneNextParam(&param) == 'a') ? TRUE : FALSE;
	ClosePanes(Combo.show[targetpane].tab.hWnd, baseindex, mode, locked);
}

void ReopenTab(int targetpane, const TCHAR **param)
{
	int index, id, count;
	TCHAR buf[CMDLINESIZE];
	BOOL delitem = FALSE;

	PaneNextParam(param);
	count = GetNumber(param);
	if ( count <= 0 ) count = 1;
	if ( ((*param)[0] == '-') && ((*param)[1] == '\0') ){ // 削除指定
		(*param)++;
		delitem = TRUE;
	}

	id = Combo.Closed.LastIndex - 1;
	for ( index = 0; index < Combo.Closed.Items; index++, id-- ){
		if ( id < 0 ) id = Combo.Closed.Items - 1;
		if ( Combo.Closed.list[id].ID[0] != '\0' ){
			if ( count > 1 ){
				count--;
				continue;
			}
			if ( delitem ){
				Combo.Closed.list[id].ID[0] = '\0';
			}else{
				thprintf(buf, TSIZEOF(buf), T("-bootid:%s %s"), Combo.Closed.list[id].ID + 1, *param);
				CreateNewTabParam(targetpane, -1, buf);
			}
			return;
		}
	}
}

const TCHAR *PaneStatus_RegID(COMBOITEMSTRUCT *cmb)
{
	if ( cmb->cinfo == NULL ) return NilStr;
	return cmb->cinfo->RegSubCID;
}

void PaneStatus(void)
{
	HeapStr hstr;
	TCHAR *memo, buf[1024];
	int scount = max(Combo.BaseCount, Combo.ShowCount);
	int baseindex;

	ComboTableCheck(); // 破損チェック

	for ( baseindex = 0 ; baseindex < Combo.BaseCount ; baseindex++ ){
		COMBOITEMSTRUCT *base;

		base = &Combo.base[baseindex];
		thprintf(buf, TSIZEOF(buf), T("Base #%3d: HWND:%s(%x) AcitveIndex:%d Capture:%d color[FG:%x BG:%x]"),
				baseindex, PaneStatus_RegID(base), (DWORD)(DWORD_PTR)base->hWnd,
				base->ActiveID, base->capture,
				base->tabtextcolor, base->tabbackcolor);
		SetComboReportText(buf);
	}

	thprintf(buf, TSIZEOF(buf), T("Tabs: %d"), Combo.Tabs);
	SetComboReportText(buf);

	// 並びチェック
	memo = PPcHeapAllocT(Panelistsize(scount) * CheckWorkExt);
	if ( InitHeapStr(&hstr, Panelistsize(scount) + 100) != NULL ){
		memo[0] = '\0';
		*hstr.dest++ = '?';
		*hstr.dest++ = '?';
		if ( Combo.Tabs == 0 ){	// タブ無し
			SavePane_Base(hstr.dest, memo, 0);
		}else{	// タブ有り
			SavePane_Base(hstr.dest, memo, 0);
			SavePane_Tab(&hstr, memo);
		}
		SetComboReportText(hstr.str);
		SetComboReportText(memo);
		FreeHeapStr(&hstr);
	}
	PPcHeapFree(memo);

	{
		int showindex, grpindex;

		COMBOITEMSTRUCT *base;
		for ( showindex = 0; showindex < Combo.ShowCount; showindex++ ){
			COMBOTABINFO *tabs;
			COMBOPANES *show;

			show = &Combo.show[showindex];
			base = &Combo.base[show->baseNo];
			thprintf(buf, TSIZEOF(buf), T("Showpane #%d base:%d[%s(%x)], select:%x tab wnd:%x group wnd:%x"),
					showindex, show->baseNo, PaneStatus_RegID(base), base->hWnd,
					show->tab.hWnd, show->tab.hSelecterWnd);
			SetComboReportText(buf);

			tabs = &show->tab;
			if ( tabs->groupcount <= 0 ){
				thprintf(buf, TSIZEOF(buf), T("\tno group"));
				SetComboReportText(buf);
			}else{
				for ( grpindex = 0; grpindex < tabs->groupcount; grpindex++ ){
					TCHAR buf2[1000];

					buf2[0] = '\0';
					GetWindowText(tabs->group[grpindex].hWnd, buf2, 1000);
					thprintf(buf, TSIZEOF(buf), T("\tgroup#%d(%s) hCurWnd:%x Tab:%x(%d items) valid:%d"),
						grpindex, buf2, tabs->group[grpindex].hCurWnd,
						tabs->group[grpindex].hWnd, TabCtrl_GetItemCount(tabs->group[grpindex].hWnd),
						IsWindow(tabs->group[grpindex].hWnd) );
					SetComboReportText(buf);
				}
			}
		}
	}
}

int GetComboShowIndexGroup(HWND hBaseWnd)
{
	int showindex;

	for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
		if ( Combo.base[Combo.show[showindex].baseNo].hWnd == hBaseWnd ){
			return showindex;
		}
		if ( Combo.show[showindex].tab.groupcount > 0 ){
			int group, groupmax;
			TABGROUP *list;
			list = Combo.show[showindex].tab.group;
			groupmax = Combo.show[showindex].tab.groupcount;
			for ( group = 0 ; group < groupmax ; group++, list++ ){
				if ( list->hCurWnd == hBaseWnd ) return showindex;
			}
		}
	}
	return -1;
}

int GetTabItemIndex_R(HWND hWnd, int tabwndindex)
{
	TC_ITEM tie;
	int tabindex;
	int tabcount;
	HWND hTabWnd;

	hTabWnd = Combo.show[tabwndindex].tab.hWnd;
	tabcount = TabCtrl_GetItemCount(hTabWnd);
	for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
		tie.mask = TCIF_PARAM;
		if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ) continue;
		if ( tie.lParam == (LPARAM)hWnd ) return tabindex;
	}
	return -1;
}

int SearchTabData(HWND hTabWnd, HWND data)
{
	TC_ITEM tie;
	int tabindex;
	int tabcount;

	tabcount = TabCtrl_GetItemCount(hTabWnd);
	for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
		tie.mask = TCIF_PARAM;
		if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ) continue;
		if ( tie.lParam == (LPARAM)data ) return tabindex;
	}
	return -1;
}

ERRORCODE SelectGroup(int showindex, int groupindex)
{
	COMBOTABINFO *tabs;
	TC_ITEM tie;
	int tabindex;

	tabs = &Combo.show[showindex].tab;

	if ( tabs->groupcount <= 0 ) return ERROR_INVALID_PARAMETER;
	if ( groupindex < 0 ) return ERROR_INVALID_PARAMETER;
	if ( groupindex >= tabs->groupcount ) return ERROR_INVALID_PARAMETER;

	tabs->hWnd = tabs->group[groupindex].hWnd;

	if ( X_combos[1] & CMBS1_SELECTEDGROUP ){
		tabs->show_all = 0;
	}

	if ( tabs->hSelecterWnd != NULL ){
		TabCtrl_SetCurSel(tabs->hSelecterWnd, groupindex);
	}

	if ( (X_combos[0] & CMBS_TABEACHITEM) || (Combo.ShowCount <= 1) ){ // 個別タブなら、選択タブを利用
	XMessage(NULL, NULL, XM_DbgDIA, T("1"));
		tabindex = TabCtrl_GetCurSel(tabs->hWnd);
	// 共用タブの場合、hCurWnd が使えたらそのタブ、使えない場合は空きタブを捜す
	}else{
		COMBOTABINFO *a_tabs;
		int a_tabindex, show_i;
		HWND hChgWnd;

	XMessage(NULL, NULL, XM_DbgDIA, T("2"));
		tabindex = GetTabItemIndex_R(tabs->group[groupindex].hCurWnd, showindex);
		if ( tabindex < 0 ){
			tabindex = TabCtrl_GetCurSel(tabs->hWnd);
			tie.mask = TCIF_PARAM;
			TabCtrl_GetItem(tabs->hWnd, tabindex, &tie);
			if ( GetComboShowIndex((HWND)tie.lParam) != showindex ){
				tabindex = 0;

				for (;;){
					if ( TabCtrl_GetItem(tabs->hWnd, tabindex, &tie) == FALSE ){
						tabindex = 0;
						break;
					}
					if ( GetComboShowIndexGroup((HWND)tie.lParam) < 0 ) break;
					tabindex++;
				}
			}

			if ( tabindex < 0 ) tabindex = 0;
		}

		tie.mask = TCIF_PARAM;
		if ( IsTrue(TabCtrl_GetItem(tabs->hWnd, tabindex, &tie)) ){
			hChgWnd = (HWND)tie.lParam;
			SelectComboWindow(showindex, hChgWnd, TRUE);
		}else{
			SortComboWindows(SORTWIN_LAYOUTPAIN);
			InvalidateRect(Combo.hWnd, NULL, TRUE);
		}

		for ( show_i = 0; show_i < Combo.ShowCount; show_i++){
			if ( show_i == showindex ) continue;
			// XMessage(Combo.hWnd, NULL, XM_MesLogWnd, T("%d check%d"),showindex,show_i);
			a_tabs = &Combo.show[show_i].tab;
			if ( ((a_tabindex = SearchTabData(a_tabs->group[groupindex].hWnd, a_tabs->group[groupindex].hCurWnd)) >= 0) ){
				// XMessage(Combo.hWnd, NULL, XM_MesLogWnd, T("hit %d"),show_i);
				SelectComboWindow(show_i, a_tabs->group[groupindex].hCurWnd, FALSE);
			}else{
				a_tabindex = TabCtrl_GetCurSel(tabs->hWnd);
				tie.mask = TCIF_PARAM;
				TabCtrl_GetItem(tabs->hWnd, a_tabindex, &tie);
				if ( GetComboShowIndex((HWND)tie.lParam) == show_i ){
					// XMessage(Combo.hWnd, NULL, XM_MesLogWnd, T("use csr %d"),show_i);
					SelectComboWindow(show_i, (HWND)tie.lParam, FALSE);
				}else{
					a_tabindex = 0;
					// XMessage(Combo.hWnd, NULL, XM_MesLogWnd, T("search %d"),show_i);

					for (;;){
						if ( TabCtrl_GetItem(tabs->hWnd, a_tabindex, &tie) == FALSE ){
							break;
						}
						if ( GetComboShowIndexGroup((HWND)tie.lParam) < 0 ){
							// XMessage(Combo.hWnd, NULL, XM_MesLogWnd, T("search hit %d"),show_i);
							SelectComboWindow(show_i, (HWND)tie.lParam, FALSE);
							break;
						}
						a_tabindex++;
					}
				}
			}
		}
		TabCtrl_SetCurSel(tabs->hWnd, tabindex);
		return NO_ERROR;
	}
	if ( tabindex < 0 ) tabindex = 0;
	tie.mask = TCIF_PARAM;
	if ( IsTrue(TabCtrl_GetItem(tabs->hWnd, tabindex, &tie)) ){
		SelectComboWindow(showindex, (HWND)tie.lParam, TRUE);
	}else{
		SortComboWindows(SORTWIN_LAYOUTPAIN);
		InvalidateRect(Combo.hWnd, NULL, TRUE);
	}
	return NO_ERROR;
}

// *pane コマンド =============================================================
// WmComboCommand から呼び出される target;呼び出し元PPc
ERRORCODE PaneCommand(const TCHAR *paramptr, int DefaultBaseIndex)
{
	TCHAR cmdname[MAX_PATH], *dst;
	UTCHAR id_use;
	const TCHAR *param;
	int baseindex, targetpane = -1;

	if ( DefaultBaseIndex >= 0 ){
		targetpane = GetComboShowIndexAll(Combo.base[DefaultBaseIndex].hWnd);
	}

	if ( targetpane < 0 ){
		targetpane = GetComboShowIndex(hComboFocusWnd);
		if ( targetpane < 0 ) targetpane = 0;
	}
	SkipSpace(&paramptr);
	dst = cmdname;
	while ( Isalpha(*paramptr) ){
		*dst++ = upper(*paramptr++);
	}
	*dst = '\0';
	id_use = (UTCHAR)SkipSpace(&paramptr);
	param = paramptr;
	baseindex = GetPaneBaseIndexParam(&param, &targetpane, DefaultBaseIndex);
	if ( baseindex < 0 ) return ERROR_INVALID_DATA;

	if ( tstrcmp(cmdname, T("FOCUS")) == 0 ){
		if ( GetComboShowIndex(Combo.base[baseindex].hWnd) < 0 ){
			CreateAndInitPane(baseindex);
		}
		SetFocus(Combo.base[baseindex].hWnd);
	}else if ( tstrcmp(cmdname, T("FOCUSTAB")) == 0 ){
		if ( Combo.Tabs ){
			int showindex = GetComboShowIndex(Combo.base[baseindex].hWnd);

			if ( showindex >= 0 ) SetFocus(Combo.show[showindex].tab.hWnd);
		}
	}else if ( tstrcmp(cmdname, T("COLOR")) == 0 ){
		PaneNextParam(&param);
		PaneColorCommand(baseindex, param);
	}else if ( (tstrcmp(cmdname, T("SELECT")) == 0) ||
			   (tstrcmp(cmdname, T("CHANGE")) == 0) ){
		if ( PaneNextParam(&param) != '\0' ){
			int bi, tp = targetpane;

			bi = GetPaneBaseIndexParam(&param, &tp, DefaultBaseIndex);
			if ( bi < 0 ) return ERROR_INVALID_DATA;
			targetpane = GetComboShowIndex(Combo.base[bi].hWnd);
			if ( targetpane < 0 ) return ERROR_INVALID_DATA;
		}
		SelectComboWindow(targetpane, Combo.base[baseindex].hWnd, cmdname[0] == 'S');
	}else if ( tstrcmp(cmdname, T("HIDE")) == 0 ){
		int showindex;

		showindex = GetComboShowIndex(Combo.base[baseindex].hWnd);
		if ( showindex >= 0 ) HidePane(showindex);
	}else if ( (tstrcmp(cmdname, T("TABSHIFT")) == 0) ||
			   (tstrcmp(cmdname, T("SHIFT")) == 0) ){
		int offset;

		PaneNextParam(&param);
		offset = GetIntNumber(&param);

		if ( (Combo.Tabs == 0) || (tstrcmp(cmdname, T("SHIFT")) == 0) ){ // 実体のみシフト
			int showindex, newshowindex;

			showindex = GetComboShowIndex(Combo.base[baseindex].hWnd);
			newshowindex = showindex + offset;
			if ( !offset || (showindex < 0) ||
				(newshowindex < 0) || (newshowindex >= Combo.ShowCount) ){
				return ERROR_INVALID_DATA;
			}
			if ( offset < 0 ){	// 左に移動
				memmove(&Combo.show[newshowindex + 1], &Combo.show[newshowindex],
				(BYTE *)&Combo.show[showindex] - (BYTE *)&Combo.show[newshowindex]);
			}else{ // 右に移動
				memmove(&Combo.show[showindex], &Combo.show[showindex + 1],
				(BYTE *)&Combo.show[newshowindex] - (BYTE *)&Combo.show[showindex]);
			}
			Combo.show[newshowindex].baseNo = baseindex;
//			CheckComboTable(T("PaneCommand-shift1"));
			SortComboWindows(SORTWIN_LAYOUTPAIN);
		}else{ // タブシフト
			int tabindex, newtabindex;
			TC_ITEM tie;
			TCHAR text[VFPS];

			tabindex = GetTabItemIndex(Combo.base[baseindex].hWnd, targetpane);
			newtabindex = tabindex + offset;
			if ( !offset || (tabindex < 0) ||
				(newtabindex < 0) || (newtabindex > Combo.BaseCount) ){
				return ERROR_INVALID_DATA;
			}
			tie.mask = TCIF_TEXT | TCIF_PARAM;
			tie.pszText = text;
			tie.cchTextMax = VFPS;
			TabCtrl_GetItem(Combo.show[targetpane].tab.hWnd, tabindex, &tie);
			TabCtrl_DeleteItem(Combo.show[targetpane].tab.hWnd, tabindex);
			SendMessage(Combo.show[targetpane].tab.hWnd, TCM_INSERTITEM, (WPARAM)newtabindex, (LPARAM)&tie);
			LastTabInsType = 7000;
			UseTabInsType |= B1;
		}
	}else if ( tstrcmp(cmdname, T("EJECT")) == 0 ){
		PostMessage(Combo.hWnd, WM_PPXCOMMAND, KCW_eject, (LPARAM)baseindex);
	}else if ( (tstrcmp(cmdname, T("CLOSE")) == 0) ||
			   (tstrcmp(cmdname, T("CLOSETAB")) == 0) ){
		PostMessage(Combo.base[baseindex].hWnd, WM_CLOSE, 0, 0);
	}else if ( tstrcmp(cmdname, T("LOCK")) == 0 ){
		int mode;

		PaneNextParam(&param);
		mode = GetStringCommand(&param, T("OFF\0") T("ON\0"));
		if ( Combo.base[baseindex].cinfo != NULL ){
			if ( mode < 0 ) mode = !Combo.base[baseindex].cinfo->ChdirLock;
			Combo.base[baseindex].cinfo->ChdirLock = mode;
			SetTabInfo(-1, Combo.base[baseindex].hWnd); // ロック状態を文字で表現している間、用意
		}
	}else if ( tstrcmp(cmdname, T("MENU")) == 0 ){
		ReplyMessage(ERROR_CANCELLED);
		TabMenu(NULL, baseindex, targetpane, NULL);
	}else if ( tstrcmp(cmdname, T("MOVE")) == 0 ){
		int DestBaseIndex, DestShowIndex = targetpane;

		targetpane = GetComboShowIndexAll(Combo.base[baseindex].hWnd);
		if ( targetpane < 0 ) return ERROR_INVALID_DATA;

		if ( PaneNextParam(&param) == '\0' ){ // 移動先指定無し→反対ペイン
			DestBaseIndex = GetPairPaneComboBaseIndex(hComboFocusWnd);
		}else{ // 指定あり
			DestBaseIndex = GetPaneBaseIndexParam(&param, &DestShowIndex, DefaultBaseIndex);
		}
		if ( DestBaseIndex < 0 ) return ERROR_INVALID_DATA;

		DestShowIndex = GetComboShowIndex(Combo.base[DestBaseIndex].hWnd);
		if ( DestShowIndex < 0 ) return ERROR_INVALID_DATA;

		Tab_MovePosition(Combo.show[targetpane].tab.hWnd,
				GetTabItemIndex(Combo.base[baseindex].hWnd, targetpane),
				DestShowIndex, -1, -1);
	}else if ( tstrcmp(cmdname, T("REOPEN")) == 0 ){
		ReopenTab(targetpane, &param);
	}else if ( tstrcmp(cmdname, T("STATUS")) == 0 ){
		PaneStatus();
	}else if ( tstrcmp(cmdname, T("SWAPPANE")) == 0 ){
		SwapPane(targetpane);
	}else if ( tstrcmp(cmdname, T("SWAPTAB")) == 0 ){
		PostMessage(Combo.base[baseindex].hWnd, WM_PPXCOMMAND, K_raw | 'G', 0);
	}else if ( tstrcmp(cmdname, T("CLOSELEFT")) == 0 ){
		PaneCloseCommand(targetpane, baseindex, -1, param);
	}else if ( tstrcmp(cmdname, T("CLOSEPANE")) == 0 ){
		PaneCloseCommand(targetpane, baseindex, 0, param);
	}else if ( tstrcmp(cmdname, T("CLOSERIGHT")) == 0 ){
		PaneCloseCommand(targetpane, baseindex, 1, param);
	}else if ( tstrcmp(cmdname, T("CLOSEOTHER")) == 0 ){
		PaneCloseCommand(targetpane, baseindex, 1, param);
		PaneCloseCommand(targetpane, baseindex, -1, param);
	}else if ( tstrcmp(cmdname, T("MAIN")) == 0 ){
		if ( Combo.ShowCount > 1 ){
			int showindex;

			showindex = GetComboShowIndex(Combo.base[baseindex].hWnd);
			if ( (showindex >= 0) && (showindex != Combo.MainPane) ){
				// サブペインをメインにするときは入れ替える
				if ( GetComboShowIndex(hComboSubPaneFocus) == showindex ){
					hComboSubPaneFocus = Combo.base[Combo.show[Combo.MainPane].baseNo].hWnd;
				}
				Combo.MainPane = showindex;
				InvalidateRect(Combo.hWnd, NULL, FALSE);
			}
		}else{ // とりあえず focus と同じ動作
			if ( GetComboShowIndex(Combo.base[baseindex].hWnd) < 0 ){
				CreateAndInitPane(baseindex);
			}
			SetFocus(Combo.base[baseindex].hWnd);
		}
	}else if ( tstrcmp(cmdname, T("SUB")) == 0 ){
		if ( (hComboSubPaneFocus != NULL) &&
			 (Combo.base[baseindex].hWnd != hComboSubPaneFocus) ){
			// メインをサブにするときは入れ替える
			if ( Combo.base[baseindex].hWnd == hComboFocusWnd ){
				SetFocus(hComboSubPaneFocus);
			}else{
				hComboSubPaneFocus = Combo.base[baseindex].hWnd;
				InvalidateRect(Combo.hWnd, NULL, FALSE);
			}
		}
	}else if ( tstrcmp(cmdname, T("NEWGROUP")) == 0 ){
		ERRORCODE result;
		if ( SkipSpace(&param) == '\"' ){
			GetCommandParameter(&param, cmdname, TSIZEOF(cmdname));
		}else{
			cmdname[0] = '\0';
		}
		result = NewTabGroup(targetpane, cmdname);
		if ( result != NO_ERROR ) return result;
		ReplyMessage(NO_ERROR); // ここで返事をしておかないと、CreateNewTabParam 内実行中(新しい窓登録時？)に、呼び出し元の SendMessage がエラー終了してしまうことがある
		SelectGroup(targetpane, Combo.show[targetpane].tab.groupcount - 1);
		SortComboWindows(SORTWIN_LAYOUTPAIN);
		CreateNewTabParam(targetpane, -1, param);
		NewTabGroupCare(targetpane);
//	}else if ( tstrcmp(cmdname, T("TABTEXT")) == 0 ){
	}else if ( tstrcmp(cmdname, T("NEWPANE")) == 0 ){
		ReplyMessage(NO_ERROR); // ここで返事をしておかないと、CreateNewTabParam 内実行中(新しい窓登録時？)に、呼び出し元の SendMessage がエラー終了してしまうことがある
		SkipSpace(&param);
		NewPane((id_use >= ' ') ? baseindex : -1, param);
	}else if ( tstrcmp(cmdname, T("NEWTAB")) == 0 ){
		ReplyMessage(NO_ERROR); // ここで返事をしておかないと、CreateNewTabParam 内実行中(新しい窓登録時？)に、呼び出し元の SendMessage がエラー終了してしまうことがある
		CreateNewTabParam(targetpane, baseindex, param);
	}else if ( tstrcmp(cmdname, T("SELECTGROUP")) == 0 ){
		TCHAR name[VFPS];
		int groupindex;
		COMBOTABINFO *tabs;

		tabs = &Combo.show[targetpane].tab;
		if ( tabs->groupcount <= 0 ) return ERROR_INVALID_PARAMETER;

		if ( SkipSpace(&param) == '\0' ){
			groupindex = -1;
		}else{
			if ( *param == '\"' ){ // 名前指定
				GetCommandParameter(&param, cmdname, TSIZEOF(cmdname));

				for ( groupindex = 0 ;; groupindex++ ){
					if ( groupindex >= tabs->groupcount ){
						return ERROR_INVALID_PARAMETER;
					}
					GetWindowText(tabs->group[groupindex].hWnd, name, VFPS);
					if ( tstrcmp(name, cmdname) == 0 ) break;
				}
			}else{
				if ( *param == 'g' ){ // 数値指定
					param++;

					if ( (*param == '+') || (*param == '-') ){
						if ( tabs->hSelecterWnd != NULL ){
							groupindex = TabCtrl_GetCurSel(tabs->hSelecterWnd);
						}else{
							for ( groupindex = 0 ;; groupindex++ ){
								if ( groupindex >= tabs->groupcount ){
									groupindex = 0;
									break;
								}
								if ( tabs->group[groupindex].hWnd == tabs->hWnd ){
									break;
								}
							}
						}
						groupindex += GetNumber(&param);
						if ( groupindex < 0 ){
							groupindex = tabs->groupcount - 1;
						}else if ( groupindex >= tabs->groupcount ){
							groupindex = 0;
						}
					}else{
						groupindex = GetNumber(&param);
					}
				}else{
					groupindex = -1;
				}
			}
		}
		if ( groupindex < 0 ){
			ReplyMessage(ERROR_CANCELLED);
			TabMenu(NULL, baseindex, targetpane, NULL);
			return NO_ERROR;
		}
		return SelectGroup(targetpane, groupindex);

	}else if ( tstrcmp(cmdname, T("SHOWTABBAR")) == 0 ){
		if ( Combo.Tabs == 0 ){
			CreateTabBar(CREATETAB_APPEND, NULL, FALSE);
			SortComboWindows(SORTWIN_LAYOUTPAIN);
			InvalidateRect(Combo.hWnd, NULL, TRUE);
		}
	}else if ( tstrcmp(cmdname, T("EXECUTE")) == 0 ){
		PaneExecuteParam(baseindex, param);
	}else{
		ReplyMessage(ERROR_INVALID_DATA);
		XMessage(Combo.hWnd, NULL, XM_GrERRld, MES_EBPC, cmdname);
		return ERROR_INVALID_DATA;
	}
	return NO_ERROR;
}

// pane の状態チェック ========================================================

// 同じIDが存在していないか調べる
BOOL CheckPaneList(TCHAR *panelist)
{
	TCHAR *chkpane;
	TCHAR pane;

	for ( chkpane = panelist + 2 ; (pane = *chkpane) != '\0' ; chkpane++ ){
		if ( !Isupper(pane) || (pane == 'Z') ) continue;
		if ( tstrchr(chkpane + 1, pane) != NULL ) return FALSE;
	}
	return TRUE;
}

void SavePane_Base(TCHAR *dest, TCHAR *memo, int save) //save=2 - 区切り追加
{
	TCHAR *panedst = dest;
	int showindex, baseindex;
	WINPOS WPos;
	TCHAR kbuf[16];

	WPos.show = 0;
	WPos.reserved = 0;
	// 表示分を列挙 + 窓枠の大きさ保存
	for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
		PPC_APPINFO *cinfo;

		cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
		if ( cinfo != NULL ){
			if ( IsBadReadPtr( (BYTE *)cinfo + (sizeof(PPC_APPINFO) / 2), 1) ){
				Combo.base[Combo.show[showindex].baseNo].cinfo = NULL;
				Combo.base[Combo.show[showindex].baseNo].hWnd = BADHWND;
				continue;
			}else{
				if ( save == 2 ) *panedst++ = '-'; // CMBS_TABEACHITEM 用区切り
				panedst = tstpcpy(panedst, cinfo->RegSubCID + 1);
				if ( IsTrue(cinfo->ChdirLock) ) *panedst++ = '$';
				panedst = thprintf(panedst, 12, T("%d"), showindex);
			}
		}
		if ( save != 0 ){
			WPos.pos = Combo.show[showindex].box;
			thprintf(kbuf, TSIZEOF(kbuf), T("%s%d"), ComboID, showindex);
			SetCustTable(Str_WinPos, kbuf, &WPos, sizeof(WPos));
		}
	}
	*panedst = '\0';
	// 非表示を列挙
	for ( baseindex = 0 ; baseindex < Combo.BaseCount ; baseindex++ ){
		PPC_APPINFO *cinfo;

		cinfo = Combo.base[baseindex].cinfo;
		if ( cinfo != NULL ){
/* IDが重複することもあるときに使用するコード
			TCHAR *ptr;

			ptr = tstrchr(dest, cinfo->RegCID[1]);
			if ( ptr != NULL ){
				ptr++;
				if ( *ptr == '$' ) ptr++;
				if ( !Isdigit(*ptr) ) ptr = NULL; // 非表示
			}
			if ( ptr == NULL ){ // 表示していない
*/
			if ( IsBadReadPtr( (BYTE *)cinfo + (sizeof(PPC_APPINFO) / 2), 1) ){
				Combo.base[baseindex].cinfo = NULL;
				Combo.base[baseindex].hWnd = BADHWND;
				continue;
			}else{ // 表示していない(既に登録されていない)なら登録
				if ( cinfo->RegSubIDNo < 0 ){
					if ( tstrchr(dest, cinfo->RegCID[1]) == NULL ){
						*panedst++ = cinfo->RegCID[1];
						if ( IsTrue(cinfo->ChdirLock) ) *panedst++ = '$';
						*panedst = '\0';
					}
				}else{
					if ( tstrstr(dest, cinfo->RegSubCID + 1) == NULL ){
						panedst = tstpcpy(panedst, cinfo->RegSubCID + 1);
						if ( IsTrue(cinfo->ChdirLock) ){
							*panedst++ = '$';
							*panedst = '\0';
						}
					}
				}
			}
		}
	}
	*panedst = '\0';
	if ( memo != NULL ){
		tstrcat(memo, T("(B)"));
		tstrcat(memo, dest);
	}
}

void SavePane_GroupTab(HeapStr *hstr, HWND hTabWnd)
{
	TCHAR name[VFPS], *src;
	int len;

	len = GetWindowText(hTabWnd, name, VFPS);
	name[VFPS - 1] = '\0';

	if ( CheckHeapStrSize(hstr, len * 3 + 2) == FALSE ) return;

	*hstr->dest++ = '?';
	*hstr->dest = '\0';

	// %/?/A -> %%/%&/%a  (Multibyte)、2ndがA-Z -> %nnnA
	src = name;
	for (;;){ // ID や 末端? と誤認識しないように除去
		TCHAR chr;

		chr = *src++;
		if ( chr == '\0' ) break;
		if ( chr == '?' ){
			*hstr->dest++ = '%';
			*hstr->dest++ = '&';
		}else if ( chr == '%' ){
			*hstr->dest++ = chr;
			*hstr->dest++ = chr;
		}else if ( Isupper(chr) ){
			*hstr->dest++ = '%';
			*hstr->dest++ = TinyCharLower(chr);
		}
		#ifndef UNICODE
		else if ( Iskanji(chr) && Isupper(*src) ){
			hstr->dest = thprintf(hstr->dest, 8, T("%%%d%c"), (int)(BYTE)chr ,TinyCharLower(*src++));
		}
		#endif
		else{
			*hstr->dest++ = chr;
		}
	}
	*hstr->dest++ = '?';
}

void SavePane_Tab(HeapStr *hstr, TCHAR *memo) // memo有り→check用
{
	int tabid, tabid_max;

	tabid_max = (X_combos[0] & CMBS_TABEACHITEM) ? Combo.ShowCount : 1;
	for ( tabid = 0 ; tabid < tabid_max ; tabid++ ){ // ペイン別
		COMBOTABINFO *tabs;
		HWND hTabWnd;
		int tablist, groupcount;

		if ( X_combos[0] & CMBS_TABEACHITEM ) *hstr->dest++ = '-'; // 識別子

		tabs = &Combo.show[tabid].tab;
		hTabWnd = tabs->hWnd;
		groupcount = tabs->groupcount;
		tablist = 0;
		for (;;){ // グループ別
			int tabcount, tabindex;

			if ( groupcount > 0 ){
				hTabWnd = tabs->group[tablist].hWnd;
				if ( memo == NULL ){ // グループ名(保存時のみ)
					SavePane_GroupTab(hstr, hTabWnd);
				}
			}

			tabcount = TabCtrl_GetItemCount(hTabWnd);
			for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){ // タブ別
				TC_ITEM tie;
				int baseindex;

				tie.mask = TCIF_PARAM;
				if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ){
					continue;
				}

				baseindex = GetComboBaseIndex((HWND)tie.lParam);
				if ( baseindex >= 0 ){
					PPC_APPINFO *cinfo;

					cinfo = Combo.base[baseindex].cinfo;
					if ( cinfo != NULL ){
						TCHAR *panedst;

						if ( CheckHeapStrSize(hstr, Panelistsize(2)) == FALSE ){
							break;
						}

						panedst = tstpcpy(hstr->dest, cinfo->RegSubCID + 1);
						if ( IsTrue(cinfo->ChdirLock) ) *panedst++ = '$';

						if ( X_combos[0] & CMBS_TABEACHITEM ){ // ペイン別タブ
							if ( (HWND)tie.lParam == Combo.base[Combo.show[tabid].baseNo].hWnd ){
								panedst = thprintf(panedst, 12, T("%d"), tabid);
							}
						}else{ // 共通
							int showindex;

							showindex = GetComboShowIndex((HWND)tie.lParam);
							if ( showindex >= 0 ){
								panedst = thprintf(panedst, 12, T("%d"), showindex);
							}
						}
						// グループで選択されたタブ
						if ( groupcount > 0 ){
							if ( X_combos[0] & CMBS_TABEACHITEM ){
								if ( (HWND)tie.lParam == tabs->group[tablist].hCurWnd ){
									panedst = thprintf(panedst, 12, T(".%d"), tabid);
								}
							}else{
								int showindex;

								for ( showindex = 0; showindex < Combo.ShowCount; showindex++ ){
									if ( (HWND)tie.lParam == Combo.show[showindex].tab.group[tablist].hCurWnd ){
										panedst = thprintf(panedst, 12, T(".%d"), showindex);
										break;
									}
								}
							}
						}
						hstr->dest = panedst;
					}
				}else{
					if ( tie.lParam != 0 ){ // 仮タブ
						if ( memo != NULL ){
							TCHAR kbuf[16];

							thprintf(kbuf, TSIZEOF(kbuf), T("[tab%d-?]"), tabindex);
							tstrcat(memo, kbuf);
						}
					}
					TabCtrl_DeleteItem(hTabWnd, tabindex); // 不明タブを削除
				}
			}
			tablist++;
			if ( tablist >= groupcount ) break;
		}
	}
	*hstr->dest = '\0';
	if ( memo != NULL ){
		tstrcat(memo, T("(T)"));
		tstrcat(memo, hstr->str);
	}
}

void USEFASTCALL ComboTableCheck(void)
{
	int baseindex, showindex;

	for ( baseindex = 0 ; baseindex < Combo.BaseCount ; baseindex++ ){
		PPC_APPINFO *cinfo;

		cinfo = Combo.base[baseindex].cinfo;
		if ( cinfo == NULL ) continue;
		if ( IsBadReadPtr(cinfo->RegCID, 4) ){
			Combo.base[baseindex].cinfo = NULL;
			Combo.base[baseindex].hWnd = BADHWND;
		}
	}

	for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
		if ( Combo.show[showindex].baseNo >= Combo.BaseCount ){
			Combo.show[showindex].baseNo = 0;
		}
	}
}

void WmComboDestroy(BOOL EndSession)
{
	HeapStr hstr;
	int scount = max(Combo.BaseCount, Combo.ShowCount);
	int showindex, rightbaseindex;

	if ( Combo.hWnd == BADHWND ) return;
	if ( ComboInit == 0 ) ComboInit = -1;

	dd_combo_close();

	if ( EndSession ){
		PPxCommonExtCommand(K_REPORTSHUTDOWN, 0);
	}else{
		PPxCommonExtCommand(K_SETLOGWINDOW, 0);
	}

	Combo.Report.hWnd = NULL;

	Combo.hWnd = BADHWND; // 再帰を禁止

	SetCustTable(Str_WinPos, ComboID, &Combo.WinPos, sizeof(Combo.WinPos));

	if ( !EndSession ) ComboTableCheck(); // 破損チェック

	// 終了時のペイン構成を保存する
	if ( InitHeapStr(&hstr, Panelistsize(scount) + 100) != NULL ){
		*hstr.dest = '?'; // Focus
		showindex = GetComboShowIndexDefault(hComboFocusWnd);
		if ( showindex >= 0 ){ // Focus を記憶する
			PPC_APPINFO *cinfo;

			cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
			if ( cinfo != NULL ){
				hstr.dest = tstpcpy(hstr.dest, cinfo->RegSubCID + 1) - 1;
			}
		}
		// ※ hstr.dest は末尾の１字前

		*(++hstr.dest) = '?'; // 右
		rightbaseindex = GetComboBaseIndex(hComboSubPaneFocus);
		if ( rightbaseindex >= 0 ){ // 右 を記憶する
			PPC_APPINFO *cinfo;

			cinfo = Combo.base[rightbaseindex].cinfo;
			if ( cinfo != NULL ){
				tstrcpy(hstr.dest, cinfo->RegSubCID + 1);
				hstr.dest += tstrlen(hstr.dest) - 1;
			}
		}
		// ※ hstr.dest は末尾の１字前

		*(++hstr.dest) = '\0';
		if ( Combo.Tabs == 0 ){	// タブ無し
			SavePane_Base(hstr.dest, NULL,
					(X_combos[0] & CMBS_TABEACHITEM) ? 2 : 1);
		}else{	// タブ有り
			SavePane_Base(hstr.dest, NULL, 1); // レイアウト保存
			SavePane_Tab(&hstr, NULL); // タブ保存
		}

		if ( ComboInit <= 0 ){
			SetCustStringTable(T("_Path"), ComboID, hstr.str, MAX_PATH / 2);
		}
		FreeHeapStr(&hstr);
	}

	if ( Combo.LeftAreaWidth > AREAMINSAVESIZE ){
		SaveTreeSettings(Combo.hTreeWnd, ComboID, ComboTreeMode, Combo.LeftAreaWidth);
	}
	if ( Combo.Height.ReportJob > AREAMINSIZE ){
		TCHAR buf[8];

		thprintf(buf, TSIZEOF(buf), T("%sL"), ComboID);
		SaveTreeSettings(NULL, buf, 0, Combo.Height.ReportJob);
		if ( Combo.Joblist.hWnd != NULL ){
			thprintf(buf, TSIZEOF(buf), T("%sJ"), ComboID);
			SaveTreeSettings(NULL, buf, 0, Combo.Joblist.JobAreaWidth);
		}
	}
	{					// 一つづつ確実に終了させていく
		HWND hPaneWnd;
		int i;

		for ( i = 0 ; i < Combo.BaseCount ; i++ ){
			hPaneWnd = Combo.base[i].hWnd;
			if ( (hPaneWnd == NULL) ||
				 (((LONG_PTR)hPaneWnd & (LONG_PTR)HWND_BROADCAST) == (LONG_PTR)HWND_BROADCAST) ){
				continue;
			}
			if ( EndSession ){
				SendMessage(hPaneWnd, WM_ENDSESSION, TRUE, 0);
			}else{
			#ifndef UNICODE
				if ( OSver.dwPlatformId != VER_PLATFORM_WIN32_NT ){
					int cnt;
					// これだと、終了時にUSER.EXEで落ちない on Win9x
					// その代わり終了が遅くなる
					PostMessage(hPaneWnd, WM_CLOSE, 0, 0);
					cnt = 10;
					while ( cnt-- ){
						Sleep(30);
						if ( IsWindow(hPaneWnd) == FALSE ) break;
						PeekLoop();
					}
				}else
			#endif
				{
					SendMessage(hPaneWnd, WM_PPXCOMMAND, KC_PRECLOSE, 0);
				}
			}
		}
	}

	CloseDxDraw(&Combo.DxDraw);
	DarkMenuWndProc(Combo.hWnd, &Combo.hMenuTheme, WM_THEMECHANGED, 0, 0);

	DeleteObject(Combo.Font.handle);
	if ( Combo.cfs.hFont != NULL ) DeleteObject(Combo.cfs.hFont);
	if ( X_combos[0] & CMBS_NOMENU ) DestroyMenu(ComboDMenu.hMenuBarMenu);
	if ( Combo.Closed.list != NULL ){
		PPcHeapFree(Combo.Closed.list);
		Combo.Closed.list = NULL;
	}

	PPxRegist(Combo.hWnd, ComboID, PPXREGIST_COMBO_FREE);
	PostWindowClosed(ComboThreadID);
// 現在の所、終了時の異常終了を避けるため、意図的にリーク。
#if 0
	PPcHeapFree(Combo.show);
	PPcHeapFree(Combo.base);
#endif
}

#pragma argsused
void ResetSubFocus(TCHAR *buf)
{
	int index, mainbase;
	UnUsedParam(buf);

	if ( Combo.BaseCount <= 1 ){ // 窓が１つのみ
		Combo.MainPane = 0;
		hComboSubPaneFocus = NULL;
	}else if ( Combo.ShowCount == 1 ){ // ペインが１つのみ
		Combo.MainPane = 0;
		index = GetComboBaseIndex(hComboSubPaneFocus);
		mainbase = GetComboBaseIndex(hComboFocusWnd);
		if ( (index < 0) || (index == mainbase) ){ // 修正が必要なら修正
			hComboSubPaneFocus = Combo.base[(mainbase == 0) ? 1 : 0].hWnd;
		}
	}else{ // ペインが複数
		index = GetComboShowIndex(hComboSubPaneFocus);
		if ( Combo.MainPane >= Combo.ShowCount ){ // MainPane の存在確認
			Combo.MainPane = ((Combo.ShowCount < 2) || (index > 0)) ? 0 : 1; // ないので再設定
		}
		if ( (index < 0) || (index == Combo.MainPane) ){
			hComboSubPaneFocus = Combo.base[Combo.show[Combo.MainPane ? 0 : 1].baseNo].hWnd;
		}
		#if 0
		if ( GetComboShowIndex(hComboSubPaneFocus) < 0 ){
			if ( X_combos[0] & CMBS_TABEACHITEM ){
				TC_ITEM tie;

				tie.mask = TCIF_PARAM;
				if ( TabCtrl_GetItem(Combo.show[1].tab.hWnd, 0, &tie) ){
					hComboSubPaneFocus = (HWND)tie.lParam;
					Combo.show[1].baseNo = GetComboBaseIndex(hComboSubPaneFocus);
					ShowWindow(hComboSubPaneFocus, SW_SHOWNORMAL);
				}
			}else{
				int i;

				for ( i = 0 ; i < Combo.BaseCount ; i++ ){
					if ( GetComboShowIndex(Combo.base[i].hWnd) < 0 ){
						hComboSubPaneFocus = Combo.base[i].hWnd;
						Combo.show[1].baseNo = i;
						ShowWindow(hComboSubPaneFocus, SW_SHOWNORMAL);
					}
				}
			}
		}
		#endif
	}
	ChangeReason = T("ResetSubFocus");

	#if VersionP
	if ( buf != NULL ){
		PPxCommonExtCommand(K_SENDREPORT, (WPARAM)((*buf == '@') ? buf + 1 : buf));
	}
	#endif

	for ( index = 0 ; index < Combo.Tabs ; index++ ){
		InvalidateRect(Combo.show[index].tab.hWnd, NULL, TRUE);
	}
}

#ifndef RELEASE
TCHAR * nestedinfo(void)
{
	TCHAR *nested = PPcHeapAllocT(512), *dest;
	int i;

	dest = thprintf(nested, 512, T("[%d:"), ComboProcNested);
	for ( i = 0 ; (i < ComboProcNested) && (i < NestedMsgs) ; i++ ){
		if ( LOWORD(ComboProcMsg[i]) == (WORD)WM_PPXCOMMAND ){
			switch ( HIWORD(ComboProcMsg[i]) ){
				case KCW_entry:
					dest = tstpcpy(dest, T("entry:"));
					break;
				case KCW_ready:
					dest = tstpcpy(dest, T("ready:"));
					break;
				default:
					dest = thprintf(dest, 16, T("PPx%X:"), HIWORD(ComboProcMsg[i]));
			}
		}else{
			switch ( LOWORD(ComboProcMsg[i]) ){
				case WM_SETFOCUS:
					dest = thprintf(dest, 16, T("SETFOCUS%x:"), HIWORD(ComboProcMsg[i]));
					break;
				case WM_ACTIVATE:
					dest = thprintf(dest, 16, T("ACTIVATE%x:"), HIWORD(ComboProcMsg[i]));
					break;
				default:
					dest = thprintf(dest, 16, T("%X.%X:"), LOWORD(ComboProcMsg[i]), HIWORD(ComboProcMsg[i]));
			}
		}
	}
	*(dest - 1) = ']';

	thprintf(dest, 200, T("(F B%d.S%d)(M B%d.S%d.P%d)(S B%d.S%d)"),
			GetComboBaseIndex(hComboFocusWnd), GetComboShowIndex(hComboFocusWnd),
			GetComboBaseIndex(Combo.base[Combo.MainPane].hWnd), GetComboShowIndex(Combo.base[Combo.MainPane].hWnd) ,Combo.MainPane,
			GetComboBaseIndex(hComboSubPaneFocus), GetComboShowIndex(hComboSubPaneFocus)
	 );
	return nested;
}
#endif

void CheckComboTable_NowPaneError(const TCHAR *fmt, const TCHAR *p1)
{
#ifdef RELEASE
	XMessage(Combo.hWnd, NULL, XM_DbgLOG, fmt, p1, NilStr);
#else
	TCHAR *nested;

	nested = nestedinfo();
	XMessage(Combo.hWnd, NULL, XM_DbgLOG, fmt, p1, nested);
	PPcHeapFree(nested);
#endif
}

void CheckComboTable_PairPaneError(const TCHAR *fmt, const TCHAR *p1)
{
#ifdef RELEASE
	TCHAR *buf;
	DWORD len;

	len  = Panelistsize(max(Combo.BaseCount, Combo.ShowCount)) * 3 + 512;
	buf = PPcHeapAllocT(len);

	thprintf(buf, len, fmt, p1, ChangeReason, NilStr,
			Combo.ShowCount, Combo.BaseCount, Combo.Tabs);

	ResetSubFocus(buf);
	PPcHeapFree(buf);
#else
	TCHAR *buf, *nested;
	DWORD len;

	len  = Panelistsize(max(Combo.BaseCount, Combo.ShowCount)) * 3 + 512;
	buf = PPcHeapAllocT(len);
	nested = nestedinfo();

	thprintf(buf, len, fmt, p1, ChangeReason, nested,
			Combo.ShowCount, Combo.BaseCount, Combo.Tabs);

	ResetSubFocus(buf);
	PPcHeapFree(nested);
	PPcHeapFree(buf);
#endif
}

void CheckComboTable(const TCHAR *p1)
{
	HeapStr hstr;
	TCHAR *memo;
	int scount = max(Combo.BaseCount, Combo.ShowCount);
	#ifdef RELEASE
		#define IfCheckPaneList(str, errcode) CheckPaneList(str);
	#else
		int paneerr = 0;
		#define IfCheckPaneList(str, errcode) if ( CheckPaneList(str) == FALSE ) paneerr += errcode;
	#endif

	ComboTableCheck(); // 破損チェック

	// 並びチェック
	memo = PPcHeapAllocT(Panelistsize(scount) * CheckWorkExt);
	if ( InitHeapStr(&hstr, Panelistsize(scount) + 100) != NULL ){
		memo[0] = '\0';
		*hstr.dest++ = '?';
		*hstr.dest++ = '?';
		SavePane_Base(hstr.dest, memo, 0);
		if ( Combo.Tabs == 0 ){	// タブ無し
			IfCheckPaneList(hstr.str, 1);
		}else{	// タブ有り
			IfCheckPaneList(hstr.str, 10);

			SavePane_Tab(&hstr, memo);
			#ifndef RELEASE
			if ( CheckPaneList(hstr.str) == FALSE ){
				if ( ComboInit <= 0 ) paneerr += 100; // ●初期化中はエラーにしない 1.92+2 仮(tab group の問題？)
			}
			#else
			CheckPaneList(hstr.str);
			#endif
		}
		FreeHeapStr(&hstr);
	}

	#ifndef RELEASE
	if ( paneerr ){
		if ( paneerr != oldpaneerr ){
			TCHAR *buf, *nested;
			DWORD len;

			len = Panelistsize(scount) * CheckWorkExt;
			buf = PPcHeapAllocT(len);
			oldpaneerr = paneerr;

			nested = nestedinfo();
			thprintf(buf, len, T("並び順破壊%s.%d(%x.%d.%d.%d)%s%s-%s"),
				p1, paneerr + LastTabInsType + (UseTabInsType * 10000),
				X_combos[0], Combo.Tabs, Combo.ShowCount, Combo.BaseCount,
				nested, ChangeReason, memo);
			PPcHeapFree(nested);

			PPxCommonExtCommand(K_SENDREPORT, (WPARAM)buf);
			PPcHeapFree(buf);
		}
	}
	#endif
	PPcHeapFree(memo);

	if ( (p1 != NULL) && (*p1 != '@') && !ComboInit && (*ChangeReason != '@') ){
		// 現在窓チェック
		if ( Combo.BaseCount > 1 ){
			if ( GetComboBaseIndex(hComboFocusWnd) < 0 ){
				CheckComboTable_NowPaneError(T("現在窓base異常%s%s"), p1);

				if ( Combo.MainPane >= Combo.ShowCount ) Combo.MainPane = 0;
				hComboFocusWnd = Combo.base[Combo.MainPane].hWnd;
			}
			if ( GetComboShowIndex(hComboFocusWnd) < 0 ){
				CheckComboTable_NowPaneError(T("現在窓show異常%s%s"), p1);

				if ( Combo.MainPane >= Combo.ShowCount ) Combo.MainPane = 0;
				hComboFocusWnd = Combo.base[Combo.MainPane].hWnd;
			}
		}

		// 反対窓チェック
		if ( Combo.BaseCount > 1 ){
			if ( hComboSubPaneFocus == NULL ){
				ResetSubFocus(NULL);
//				CheckComboTable_PairPaneError(T("反対窓未設定%s,%s%s%d/%d,%d"), p1);
			}else{
				if ( GetComboBaseIndex(hComboSubPaneFocus) >= 0 ){
					int showindex = GetComboShowIndex(hComboSubPaneFocus);

					if ( Combo.ShowCount <= 1 ){
						if ( showindex >= 0 ){
							CheckComboTable_PairPaneError(T("反対窓表示%s,%s%s%d/%d,%d"), p1);
//							ResetSubFocus(NULL);
						}
					}else{
						if ( (showindex < 0) || (showindex == Combo.MainPane) ){
							CheckComboTable_PairPaneError(
								(showindex < 0) ?
									T("反対窓異常%s,%s%s%d/%d,%d") :
									T("反対窓左%s,%s%s%d/%d,%d"), p1);
//							ResetSubFocus(NULL);
						}
					}
				}
			}
		}
	}
}

int GetComboRegZID(PPCSTARTPARAM *psp)
{
	PPC_APPINFO *cinfo;
	int baseindex;
	int regid;
	BOOL retry;

	regid = (RegSubIDcounter + 1) & 0x1ff;
	for (;;){
		retry = FALSE;
		if ( psp != NULL ){
			size_t size;
			PSPONE *pspo;

			pspo = psp->next;
			if ( psp->next != NULL ) while ( pspo->id.RegID[0] != '\0' ){
				size = PSPONE_size(pspo);
				if ( regid == pspo->id.SubID ){
					regid = (regid + 1) & 0x1ff;
					retry = TRUE;
					break;
				}
				pspo = (PSPONE *)(char *)((char *)pspo + size); // 次へ
			}
			if ( retry != FALSE ) continue;
		}
		for ( baseindex = 0 ; baseindex < Combo.BaseCount ; baseindex++ ){
			cinfo = Combo.base[baseindex].cinfo;
			if ( cinfo == NULL ) continue;
			if ( regid == cinfo->RegSubIDNo ){
				regid = (regid + 1) & 0x1ff;
				retry = TRUE;
				break;
			}
		}
		if ( retry == FALSE ) break;
	}
	RegSubIDcounter = regid;
	return regid;
}
