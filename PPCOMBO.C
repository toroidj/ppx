/*-----------------------------------------------------------------------------
	Paper Plane cUI											Combo Window
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <commctrl.h>
#include <dbt.h>
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPC_DD.H"
#include "PPCUI.RH"
#pragma hdrstop

#define CGLOBALDEFINE
#include "PPCOMBO.H" // グローバル変数定義
#define PaneMinSize (SplitPix * 2) // 各ペインの最小幅／丈
#define PaneMinHeight (SplitPix * 6)
#define TabHeight_fix 1 // タブコントロールの高さ調整用
#define MinSplitWidth (SplitPix * 2) // 仕切り線移動時の最低限確保するペイン幅

//------------------------------------- debug
const TCHAR *ChangeReason = T("?");
int AddDelCount = 0; // 再入チェック用
BOOL EnablePaneSizeChange = FALSE;

//------------------------------------- Combo window 自体の情報
COMBOSTRUCT Combo; // InitPPcGlobal で Combo.hWnd, Combo.Report.hWnd、InitCombo で残り全てが NULL 初期化される
TCHAR ComboID[] = T("CBA");

POINT ClientPos;
ThSTRUCT Combo_thGuiWork = ThSTRUCT_InitData; // リバー関連の情報保存に使用する

int ComboInit = 1;		// 1:combo window 作成中、-1:終了中

//------------------------------------- Combo window に登録されたpaneの情報
HWND hComboFocusWnd = NULL; // 現在のフォーカスがある窓(Main/Subのどちらもあり得る)
HWND hComboSubPaneFocus = NULL; // 現在の右側窓フォーカス(1ペインの時は、隠れている反対窓)

//-------------------------------------
LRESULT WmComboCommand(HWND hComboWnd, WPARAM wParam, LPARAM lParam);
void WmComboPosChanged(HWND hComboWnd);

LRESULT ComboGetIDWnd(LPARAM lParam)
{
	TCHAR regid[16];

	regid[0] = 'C';
	regid[1] = 'Z';
	regid[2] = ComboID[2];
	regid[3] = (TCHAR)(BYTE)lParam;
	regid[4] = (TCHAR)(BYTE)(lParam >> 8);
	regid[5] = (TCHAR)(BYTE)(lParam >> 16);
	regid[6] = (TCHAR)(BYTE)(lParam >> 24);
	regid[7] = '\0';
	return (LRESULT)GetHwndFromIDCombo(regid);
}

/* KCW_ppclist 本体
?Cxxxx\0path\0 がタブだけある
\t\0\t\0 がペインの区切り
? = space / *(focus) / ~(pair) / +(show)
*/
LRESULT ComboGetPPcList(DWORD mode)
{
	int showindex;
	ThSTRUCT list;

	ThInit(&list);
	if ( Combo.Tabs <= 0 ){ // タブ無し
		for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
			int baseindex;
			PPC_APPINFO *cinfo;

			baseindex = Combo.show[showindex].baseNo;
			if ( baseindex < 0 ) continue;

			cinfo = Combo.base[baseindex].cinfo;
			if ( (cinfo != NULL) && (cinfo->path[0] != '\0') ){
				if ( cinfo->info.hWnd == hComboFocusWnd ){
					ThCatString(&list, T("*"));
				}else if ( (showindex == Combo.MainPane) || (cinfo->info.hWnd == hComboSubPaneFocus) ){
					ThCatString(&list, T("~"));
				}else {
					ThCatString(&list, T(" "));
				}
				ThAddString(&list, cinfo->RegSubCID);
				ThAddString(&list, cinfo->path);
			}
		}
	}else{
		int showmax;

		showmax = (X_combos[0] & CMBS_TABEACHITEM) ? Combo.ShowCount : 1;
		for ( showindex = 0 ; showindex < showmax ; showindex++ ){
			HWND hTabWnd;
			int tabcount, tabindex, tabshow, nextgroup;

			hTabWnd = Combo.show[showindex].tab.hWnd;
			nextgroup = 0;
			for (;;){
				tabcount = TabCtrl_GetItemCount(hTabWnd);
				tabshow = TabCtrl_GetCurSel(hTabWnd);
				for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
					TC_ITEM tie;
					int baseindex;
					PPC_APPINFO *cinfo;

					tie.mask = TCIF_PARAM;
					if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ){
						continue;
					}

					baseindex = GetComboBaseIndex((HWND)tie.lParam);
					if ( baseindex < 0 ) continue;

					cinfo = Combo.base[baseindex].cinfo;
					if ( (cinfo != NULL) && (cinfo->path[0] != '\0') ){
						if ( cinfo->info.hWnd == hComboFocusWnd ){
							ThCatString(&list, T("*"));
						}else if ( cinfo->info.hWnd == hComboSubPaneFocus ){
							ThCatString(&list, T("~"));
						}else if ( tabindex == tabshow ){
							ThCatString(&list, (showindex == Combo.MainPane) ? T("~") : T("+"));
						}else {
							ThCatString(&list, T(" "));
						}
						ThAddString(&list, cinfo->RegSubCID);
						ThAddString(&list, cinfo->path);
					}
				}
				{
					COMBOTABINFO *tab;

					tab = &Combo.show[showindex].tab;
					if ( tab->groupcount <= 0 ) break;
					if ( tab->group[nextgroup].hWnd == tab->hWnd ){
						nextgroup++;
					}
					if ( nextgroup >= tab->groupcount ) break;
					hTabWnd = tab->group[nextgroup].hWnd;
					ThAddString(&list, T("\t"));
					ThAddString(&list, T("\t")); // 区切り
					nextgroup++;
				}
			}
			if ( showindex < (showmax - 1) ){
				ThAddString(&list, T("\t"));
				ThAddString(&list, T("\t")); // 区切り
			}
		}
	}
	ThAddString(&list, T(""));

	if ( mode == 0 ){
		return (LRESULT)list.bottom;
	}else{
		TCHAR path[VFPS], path2[VFPS];
		DWORD tick = GetTickCount() | 3, work;
		HANDLE hFile;

		thprintf(path2, TSIZEOF(path2), T("%%temp%%\\ppxl%d"), tick);
		ExpandEnvironmentStrings(path2, path, TSIZEOF(path));
		hFile = CreateFileL(path, GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if ( hFile == INVALID_HANDLE_VALUE ){
			ThFree(&list);
			return 0;
		}
		WriteFile(hFile, list.bottom, list.top, &work, NULL);
		CloseHandle(hFile);
		ThFree(&list);
		return tick;
	}
}

LRESULT WmComboSettingChange(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if ( (wParam == 0) && (lParam != 0) && (*(char *)lParam == 'I') ){ // "ImmersiveColorSet" の簡易判定
		//※ Dark modeになっているとき、「I」になっていることがあるため
		if ( PPxCommonExtCommand(K_UxTheme, KUT_SETTINGCHANGE) ){
			ReloadThemeSettings(hWnd);
			InvalidateRect(hWnd, NULL, TRUE);
#if 0
			int index;

			for ( index = 0 ; index < Combo.BaseCount ; index++ ){
				SendMessage(Combo.base[index].hWnd, WM_SETTINGCHANGE, wParam, lParam);
			}
#endif
		}
	}
	return 1;
}

void WmComboDpiChanged(HWND hWnd, WPARAM wParam, RECT *newpos)
{
	int i;
	HFONT hNewControlFont;

	if ( newpos != NULL ){
		DWORD newDPI = HIWORD(wParam);

		if ( !(X_dss & DSS_ACTIVESCALE) ) return;
		if ( Combo.FontDPI == newDPI ) return; // 変更無し(起動時等)
		Combo.FontDPI = HIWORD(wParam);
	}

	for ( i = 0 ; i < Combo.BaseCount ; i++ ){
		PPC_APPINFO *cinfo;

		cinfo = Combo.base[i].cinfo;
		if ( cinfo != NULL ){
			SendMessage(cinfo->info.hWnd, WM_PPXCOMMAND, K_CHENGEDDISPDPI, wParam);
		}
	}

	if ( newpos != NULL ){
		SetWindowPos(hWnd, NULL, newpos->left, newpos->top,
				newpos->right - newpos->left, newpos->bottom - newpos->top,
				SWP_NOACTIVATE | SWP_NOZORDER);
	}
	InitComboGUI(TRUE);
	if ( Combo.hAddressWnd != NULL ){
		Combo.Height.AddrBar = Combo.Font.size.cy + 6;
		SendMessage(Combo.hAddressWnd, WM_SETFONT, (WPARAM)Combo.Font.handle, 0);
	}
	if ( Combo.Report.hWnd != NULL ){
		SendMessage(Combo.Report.hWnd, WM_SETFONT, (WPARAM)Combo.Font.handle, 0);
	}
	if ( Combo.Tabs || (Combo.hTreeWnd != NULL) ){
		hNewControlFont = GetControlFont(Combo.FontDPI, &Combo.cfs);
		for ( i = 0 ; i < Combo.Tabs ; i++ ){
			COMBOTABINFO *tab;

			tab = &Combo.show[i].tab;
			if ( tab->hSelecterWnd != NULL ){
				SendMessage(tab->hSelecterWnd, WM_SETFONT, (WPARAM)hNewControlFont, TMAKELPARAM(TRUE, 0));
			}
			if ( tab->groupcount <= 1 ){
				SendMessage(tab->hWnd, WM_SETFONT, (WPARAM)hNewControlFont, TMAKELPARAM(TRUE, 0));
			}else{
				int list;
				for ( list = 0; list < tab->groupcount; list++ ){
					SendMessage(tab->group[list].hWnd, WM_SETFONT, (WPARAM)hNewControlFont, TMAKELPARAM(TRUE, 0));
				}
			}
		}
		if ( Combo.hTreeWnd != NULL ){
			SendMessage(Combo.hTreeWnd, VTM_CHANGEDDISPDPI, wParam, 0);
		}
	}

	Combo.WndSize.cx = 0;
	WmComboPosChanged(hWnd);
}

BOOL RecvExecuteByWMComboCopyData(COPYDATASTRUCT *copydata)
{
	TCHAR *cmdbuf;
	COPYDATASTRUCT newcopydata;

	// copydata をそのままSendMessageに渡すと失敗するので、一旦内容をコピー
	if ( copydata->cbData >= 0x3fffffff ) return FALSE;
	cmdbuf = PPcHeapAlloc(copydata->cbData);
	if ( cmdbuf == NULL ) return FALSE;
	memcpy(cmdbuf, copydata->lpData, copydata->cbData);
	newcopydata.dwData = copydata->dwData;
	newcopydata.cbData = copydata->cbData;
	/* if ( LOWORD(copydata->dwData) == 'H' ) */ ReplyMessage(TRUE);
	newcopydata.lpData = cmdbuf;
	SendMessage(hComboFocusWnd, WM_COPYDATA, 0, (LPARAM)&newcopydata);
	PPcHeapFree(cmdbuf);
	return TRUE;
}

BOOL WmComboCopyData(COPYDATASTRUCT *copydata)
{
	switch ( LOWORD(copydata->dwData) ){
		case 'H':
			return RecvExecuteByWMComboCopyData(copydata);

		case KC_MOREPPC:
			SendCallPPc(copydata);
			return TRUE;

		case K_WINDDOWLOG:
			WmComboCommand(NULL, TMAKEWPARAM(K_WINDDOWLOG, PPLOG_REPORT), (LPARAM)copydata->lpData);
			return TRUE;
	}
	return FALSE;
}

// フォーカスをアクティブペインに設定する
void WmComboSetFocus(void)
{
	HWND hFocusWnd;
	int showindex;

	if ( Combo.BaseCount == 0 ) return;
	hFocusWnd = hComboFocusWnd;
	showindex = GetComboShowIndex(hFocusWnd);	// hComboFocusWnd が実在するか確認
	if ( showindex < 0 ){			// 表示されていないときは再設定
		if ( Combo.MainPane >= Combo.ShowCount ){ // MainPane の存在確認
			Combo.MainPane = ((Combo.ShowCount < 2) || (GetComboShowIndex(hComboSubPaneFocus) > 0)) ? 0 : 1; // ないので再設定
		}
		if ( GetComboBaseIndex(hFocusWnd) < 0 ){ // 実在しないときは再設定
			hFocusWnd = Combo.base[Combo.show[Combo.MainPane].baseNo].hWnd;
		}
	}else{
		if ( GetFocus() == hFocusWnd ) return; // フォーカス移動済み
	}
	ChangeReason = T("WmComboSetFocus");
	InvalidateRect(Combo.hWnd, NULL, TRUE); // pane , status, info, tab
	SetFocus(hFocusWnd);
}

#define MAX_TABWND_SIZE 0x7000
int CalcTabBarHeight(COMBOTABINFO *tabs)
{
	RECT temprect = { 0, 0, MAX_TABWND_SIZE, MAX_TABWND_SIZE };
	int oldheight;

	oldheight = tabs->height;
	if ( tabs->groupcount == 0 ){
		TabCtrl_AdjustRect(tabs->hWnd, FALSE, &temprect);
		if ( temprect.top < 8 ) temprect.top = 8;
		tabs->height = temprect.top + TabHeight_fix;
	}else{
		int i, totalheight = 0;

		if ( tabs->hSelecterWnd != NULL ){
			temprect.right = MAX_TABWND_SIZE;
			temprect.top = temprect.left = 0;
			TabCtrl_AdjustRect(tabs->hSelecterWnd, FALSE, &temprect);
			if ( temprect.top < 8 ) temprect.top = 8;
			tabs->selector_height = temprect.top;
			totalheight += temprect.top;
		}
		for ( i = 0 ; i < tabs->groupcount; i++ ){
			if ( tabs->show_all || (tabs->hWnd == tabs->group[i].hWnd) ){
				temprect.right = MAX_TABWND_SIZE;
				temprect.top = temprect.left = 0;
				TabCtrl_AdjustRect(tabs->group[i].hWnd, FALSE, &temprect);
				if ( temprect.top < 8 ) temprect.top = 8;
				tabs->group[i].height = temprect.top;
				totalheight += temprect.top;
			}
		}
		tabs->height = totalheight + TabHeight_fix;
	}
	return oldheight;
}

HDWP DeferTabBar(HDWP hdWins, COMBOTABINFO *tabs, int x, int y, int w, int h)
{
	if ( tabs->groupcount == 0 ){
		return DeferWindowPos(hdWins, tabs->hWnd, NULL,
				x, y, w, h, SWP_NOACTIVATE | SWP_NOZORDER);
	}else{
		int i;

		if ( tabs->hSelecterWnd != NULL ){
			hdWins = DeferWindowPos(hdWins, tabs->hSelecterWnd, NULL,
					x, y, w, tabs->selector_height,
					SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
			y += tabs->selector_height;
		}

		for ( i = 0 ; i < tabs->groupcount; i++ ){
			if ( tabs->show_all || (tabs->hWnd == tabs->group[i].hWnd) ){
				int single_height;

				single_height = tabs->group[i].height;
				if ( i == (tabs->groupcount - 1) ) single_height += TabHeight_fix;
				hdWins = DeferWindowPos(hdWins, tabs->group[i].hWnd, NULL,
					x, y, w, single_height, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
				y += single_height;
			}else{
				hdWins = DeferWindowPos(hdWins, tabs->group[i].hWnd, NULL,
					0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
			}
		}
		return hdWins;
	}
}

#define SortPane_Fix(RANGEMIN_, RANGEMAX_, posmin, posmax, minsize, fixsize) {\
	int I_, RANGE_, addr C4701CHECK, X_ = Combo.Panes.clientbox.posmin;\
\
	RANGE_ = Combo.Panes.clientbox.posmax;\
	for ( I_ = RANGEMIN_ ; I_ < RANGEMAX_ ; I_++ ){\
		int panerange;\
\
		cps = &Combo.show[I_];\
		panerange = cps->box.posmax - cps->box.posmin;\
		if ( (X_ + panerange + fixsize) > RANGE_ ){\
			if ( !(X_combos[0] & CMBS_TABFRAME) ){\
				panerange = RANGE_ - X_ - fixsize;\
				if ( panerange < minsize ) panerange = minsize;\
			}\
		}\
		cps->box.posmin = X_ + fixsize;\
		X_ = cps->box.posmin + panerange;\
		cps->box.posmax = addr = X_;\
		X_ += SplitPix;\
		if ( (I_ < (RANGEMAX_ - 1)) && (X_ >= RANGE_) && (panerange > 100)){\
			if ( !(X_combos[0] & CMBS_TABFRAME) ){\
				X_ = addr = cps->box.posmax = cps->box.posmin + panerange - (SplitPix + fixsize) * 2;\
				X_ += SplitPix;\
			}\
		}\
	}\
	if ( Combo.show[I_ - 1].box.posmax < RANGE_ ){\
		Combo.show[I_ - 1].box.posmax = addr = RANGE_;\
	}\
	Combo.Panes.clientbox.posmax = addr;\
}

#define SortPane_FixB(RANGEMIN_, RANGEMAX_, posmin, posmax, minsize, fixsize) {\
	int I_, RANGE_, addr C4701CHECK, X_ = Combo.Panes.clientbox.posmin;\
\
	RANGE_ = Combo.Panes.clientbox.posmax;\
	for ( I_ = RANGEMIN_ ; I_ < RANGEMAX_ ; I_++ ){\
		int panerange;\
\
		cps = &Combo.show[I_];\
		panerange = cps->box.posmax - cps->box.posmin;\
		if ( (X_ + panerange + fixsize) > RANGE_ ){\
			if ( !(X_combos[0] & CMBS_TABFRAME) ){\
				panerange = RANGE_ - X_ - fixsize;\
				if ( panerange < minsize ) panerange = minsize;\
			}\
		}\
		cps->box.posmin = X_;\
		X_ += panerange;\
		cps->box.posmax = X_;\
		addr = X_ + fixsize;\
		X_ = addr + SplitPix;\
		if ( (I_ < (RANGEMAX_ - 1)) && (X_ >= RANGE_) && (panerange > 100)){\
			if ( !(X_combos[0] & CMBS_TABFRAME) ){\
				X_ = addr = cps->box.posmax = cps->box.posmin + panerange - (SplitPix + fixsize) * 2;\
				X_ += SplitPix;\
			}\
		}\
	}\
	if ( addr < RANGE_ ){\
		Combo.show[I_ - 1].box.posmax = RANGE_ - (addr - Combo.show[I_ - 1].box.posmax);\
		addr = RANGE_;\
	}\
	Combo.Panes.clientbox.posmax = addr;\
}


#define SortPane_ResizeSize(posmin, posmax, pos, fixsize) {\
	int I_, RANGE_, addr C4701CHECK;\
\
	RANGE_ = Combo.Panes.clientbox.posmin;\
	for ( I_ = 0 ; I_ < Combo.ShowCount ; I_++ ){\
		int panerange;\
\
		cps = &Combo.show[I_];\
		panerange = cps->box.posmax - cps->box.posmin;\
		cps->box.posmin = RANGE_ + fixsize;\
		RANGE_ = cps->box.posmin + panerange;\
		cps->box.posmax = addr = RANGE_;\
		RANGE_ += SplitPix;\
	}\
	pos = addr - Combo.Panes.clientbox.posmin;\
}

#define SortPane_ResizeSizeB(posmin, posmax, pos, fixsize) {\
	int I_, RANGE_, addr C4701CHECK;\
\
	RANGE_ = Combo.Panes.clientbox.posmin;\
	for ( I_ = 0 ; I_ < Combo.ShowCount ; I_++ ){\
		int panerange;\
\
		cps = &Combo.show[I_];\
		panerange = cps->box.posmax - cps->box.posmin;\
		cps->box.posmin = RANGE_;\
		RANGE_ = cps->box.posmin + panerange;\
		cps->box.posmax = RANGE_;\
		addr = RANGE_ + fixsize;\
		RANGE_ = addr + SplitPix;\
	}\
	pos = addr - Combo.Panes.clientbox.posmin;\
}

#define SortPane_Setpos(posmin, posmax) {\
	int I_;\
\
	for ( I_ = 0 ; I_ < Combo.ShowCount ; I_++ ){\
		cps = &Combo.show[I_];\
		cps->box.posmin = Combo.Panes.clientbox.posmin;\
		cps->box.posmax = Combo.Panes.clientbox.posmax;\
\
		hdWins = DeferWindowPos(hdWins, Combo.base[cps->baseNo].hWnd, NULL, \
				cps->box.left, cps->box.top, \
				cps->box.right - cps->box.left, \
				cps->box.bottom - cps->box.top, \
				SWP_NOACTIVATE | SWP_NOZORDER);\
	}\
}

#define SortPane_MakeResizeRates(posmin, posmax) {\
	int I_, base = Combo.Panes.clientbox.posmin;\
\
	Combo.Panes.resizewidth = Combo.Panes.clientbox.posmax - base;\
	for ( I_ = 0 ; I_ < Combo.ShowCount ; I_++ ){\
		Combo.show[I_].resizepos = Combo.show[I_].box.posmax - base;\
	}\
}

#define SortPane_ResizeRates(RANGEMIN_, RANGEMAX_, posmin, posmax) {\
	int base, x, I_, nw, resizewidth, newb;\
\
	x = base = Combo.Panes.clientbox.posmin;\
	nw = Combo.Panes.clientbox.posmax - base;\
	resizewidth = Combo.Panes.resizewidth;\
\
	for ( I_ = RANGEMIN_ ; I_ < RANGEMAX_ ; I_++ ){\
		Combo.show[I_].box.posmin = x;\
		newb = base + ( (Combo.show[I_].resizepos * nw) / resizewidth);\
		if ( newb < x ) newb = x;\
		Combo.show[I_].box.posmax = newb;\
		x = newb + SplitPix;\
	}\
}

#define SortPane_ResizeRatesT(RANGEMIN_, RANGEMAX_, posmin, posmax) {\
	int base, x, I_, nw, resizewidth, newb;\
\
	x = base = Combo.Panes.clientbox.posmin;\
	nw = Combo.Panes.clientbox.posmax - base;\
	resizewidth = Combo.Panes.resizewidth;\
\
	for ( I_ = RANGEMIN_ ; I_ < RANGEMAX_ ; I_++ ){\
		Combo.show[I_].box.posmin = x + Combo.show[I_].tab.height;\
		newb = base + ( ((Combo.show[I_].resizepos - Combo.show[I_].tab.height) * nw) / resizewidth) + Combo.show[I_].tab.height;\
		if ( newb < x ) newb = x;\
		Combo.show[I_].box.posmax = newb;\
		x = newb + SplitPix;\
	}\
}

// ウィンドウの表示を並べ直す
// fixshowindex < 0
//		ComboWindow に収まるように調整
//
// fixshowindex >= 0  and  fixshowindex < SORTWIN_FIX_NORESIZE
//		fixshowindex で窓を基準に大きさも調整
//		KCW_size の大きさ変更時、
//		DestroyedPaneWindow のペイン減少時( 0 固定 ) が該当
//
// fixshowindex >= SORTWIN_NEWPANE
//		KCW_EntryCombo のペイン追加時
//
// fixshowindex >= SORTWIN_FIX_NORESIZ
//		RequestPairRate 用。大きさ変えずに変更
//
void SortComboWindows(int fixshowindex)
{
	int PaneRight/* = Combo.Panes.box.right*/, PaneBottom = Combo.Panes.box.bottom;
	RECT pbox, cbox;
	COMBOPANES *cps;
	HDWP hdWins;

	AREA ToolBox;
	AREA InfoBox;
	AREA AddrBox;
	AREA TreeBox;
	AREA TabBox;
	AREA JobBox;

	BOOL OverTabArea;

	if ( (Combo.ShowCount == 0) || (Combo.BaseCount == 0) ) return;

	DEBUGLOGF("SortWindows - %d", fixshowindex);
	Combo.Height.TopDock = Combo.Docks.t.client.bottom;
	Combo.Height.BottomDock = Combo.Docks.b.client.bottom;

	OverTabArea =
			((X_combos[0] & (CMBS_VPANE | CMBS_TABSEPARATE | CMBS_TABMULTILINE)) == (CMBS_TABSEPARATE | CMBS_TABMULTILINE) ) &&
			((X_combos[1] & (CMBS1_TABFIXLAYOUT | CMBS1_OVERTABAREA)) == CMBS1_OVERTABAREA );

	// InfoHeight を再計算する
	if ( X_combos[0] & CMBS_COMMONINFO ){
		int showindex;

		if ( Combo.Docks.t.hInfoWnd || Combo.Docks.b.hInfoWnd ){
			Combo.Height.InfoLine = 0;
		}else{
			Combo.Height.InfoLine = Combo.Font.size.cy * 2;
			for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
				PPC_APPINFO *cinfo;

				cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
				if ( cinfo == NULL ) continue;
				Combo.Height.InfoLine = cinfo->fontY *
						(cinfo->inf1.height + cinfo->inf2.height);
				break;
			}
			if ( XC_ifix && (Combo.Height.InfoLine < XC_ifix) ){
				Combo.Height.InfoLine = XC_ifix;
			}
		}
	}

	if ( fixshowindex == SORTWIN_RESIZE ){
		if ( Combo.Panes.resizewidth < 0 ){
			// 大きさ変更時の比率を生成
			if ( !(X_combos[0] & CMBS_VPANE) ){	// 横整列
				SortPane_MakeResizeRates(left, right);
			}else{ // 縦配列
				SortPane_MakeResizeRates(top, bottom);
			}
		}
		// div 0 防止
		if ( Combo.Panes.resizewidth == 0 ) Combo.Panes.resizewidth = 1;
	}else{
		if ( fixshowindex != SORTWIN_FIXINNER ) Combo.Panes.resizewidth = -1;
	}

	if ( X_combos[0] & CMBS_VPANE ){ // 縦配列は、タブ高さを初期化
		int showindex;

		for ( showindex = 1 ; showindex < Combo.ShowCount ; showindex++ ){
			Combo.show[showindex].tab.height = 0;
		}
	}

	if ( Combo.Tabs ){ // タブ高さを算出
		int tabindex = 1, tabshow = 0;

		if ( Combo.Tabs < Combo.ShowCount ){
			tabshow = GetComboShowIndex(hComboFocusWnd);
			if ( tabshow < 0 ) tabshow = 0;
		}
		CalcTabBarHeight(&Combo.show[tabshow].tab);
		Combo.Height.TabBar = Combo.show[tabshow].tab.height;
		if ( Combo.show[tabshow].tab.groupcount > 0 ){
			OverTabArea = TRUE;
			if ( Combo.Tabs < Combo.ShowCount ){
				Combo.show[0].tab.height = Combo.Height.TabBar;
			}
		}

		for ( ; tabindex < Combo.Tabs ; tabindex++ ){
			COMBOTABINFO *tabs;

			tabs = &Combo.show[tabindex].tab;
			if ( (tabs->groupcount == 0) && !(X_combos[0] & CMBS_TABMULTILINE) ){ // グループ無し and １行表示なら行の高さを再利用可能
				tabs->height = Combo.show[0].tab.height;
			}else{
				CalcTabBarHeight(tabs);

				if ( tabs->groupcount > 0 ) OverTabArea = TRUE;
				if ( !(X_combos[0] & CMBS_VPANE) ){ // 横
					if ( Combo.Height.TabBar <= tabs->height ){
						Combo.Height.TabBar = tabs->height;
					}
				}
			}
		}
		if ( OverTabArea ) Combo.Height.TabBar = Combo.show[0].tab.height;
	}

	Combo.Height.TopArea = Combo.Height.TopDock + Combo.ToolBar.Height + Combo.Height.InfoLine + Combo.Height.AddrBar;

	Combo.Panes.box.left	= Combo.LeftAreaWidth;
	Combo.Panes.box.right	= Combo.WndSize.cx;

	if ( !(X_combos[0] & CMBS_VPANE) ){ // 横…タブ高さを除外する
		if ( X_combos[1] & CMBS1_TABBOTTOM ){
			Combo.Panes.box.top		= Combo.Height.TopArea;
			Combo.Panes.box.bottom	= Combo.WndSize.cy - Combo.Height.ReportJob - Combo.Height.BottomDock - Combo.Height.TabBar;
		}else{
			Combo.Panes.box.top		= Combo.Height.TopArea + Combo.Height.TabBar;
			Combo.Panes.box.bottom	= Combo.WndSize.cy - Combo.Height.ReportJob - Combo.Height.BottomDock;
		}
	}else{ // 縦…タブ高さを含める
		Combo.Panes.box.top		= Combo.Height.TopArea;
		Combo.Panes.box.bottom	= Combo.WndSize.cy - Combo.Height.ReportJob - Combo.Height.BottomDock;
	}

	if ( (Combo.Panes.box.top + PaneMinSize) > Combo.Panes.box.bottom ){ // 最小高さ
		Combo.Panes.box.bottom = Combo.Panes.box.top + PaneMinSize;
	}

	if ( X_combos[0] & CMBS_TABFRAME ){
		Combo.Panes.clientbox.left = 0;
		Combo.Panes.clientbox.right = Combo.Panes.box.right - Combo.Panes.box.left;
		Combo.Panes.clientbox.top = 0;
		Combo.Panes.clientbox.bottom = Combo.Panes.box.bottom - Combo.Panes.box.top - GetSystemMetrics(SM_CYHSCROLL);

		Combo.Panes.clientbox.left -= Combo.Panes.delta.x;
		Combo.Panes.clientbox.right -= Combo.Panes.delta.x;
		Combo.Panes.clientbox.top -= Combo.Panes.delta.y;
		Combo.Panes.clientbox.bottom -= Combo.Panes.delta.y;
	}else{
		Combo.Panes.clientbox = Combo.Panes.box;
	}

	hdWins = BeginDeferWindowPos(Combo.ShowCount + 7);
	if ( !(X_combos[0] & CMBS_VPANE) ){	// 横整列
		if ( fixshowindex < 0 ){	// 基準indexなしのとき
			if ( (X_combos[0] & CMBS_QPANE) && (Combo.ShowCount > 2) ){ // 複数行
				int line, maxlines = (Combo.ShowCount + 1) / 2;
				int w, wm, hdelta, i;

				OverTabArea = FALSE;
				hdelta = Combo.Panes.clientbox.bottom - Combo.Panes.clientbox.top;
				for ( line = 0 ; line < maxlines ; line++ ){
					w = line * 2;
					wm = w + 2;
					if ( wm > Combo.ShowCount ) wm--;

					if ( fixshowindex == SORTWIN_RESIZE ){
						SortPane_ResizeRates(w, wm, left, right);
					}

					if ( (w + 1) == Combo.ShowCount ){
						Combo.show[w].box.left = Combo.Panes.clientbox.left;
						Combo.show[w].box.top = Combo.Panes.clientbox.top
							+ ((hdelta * line) / maxlines);
						Combo.show[w].box.right = Combo.Panes.clientbox.right;
						Combo.show[w].box.bottom = Combo.Panes.clientbox.bottom;
					}else{
						SortPane_Fix(w, wm, left, right, PaneMinSize, 0);

						Combo.show[w].box.top = Combo.show[w + 1].box.top = Combo.Panes.clientbox.top
							+ ((hdelta * line) / maxlines);
						Combo.show[w].box.bottom = Combo.show[w + 1].box.bottom = Combo.Panes.clientbox.top
							+ ((hdelta * (line + 1)) / maxlines);
					}

					if ( (line > 0) && (X_combos[0] & CMBS_TABSEPARATE) &&
						 ((Combo.show[w].box.bottom - Combo.show[w].box.top) >
							Combo.show[w].tab.height) ){
						Combo.show[w].box.top += Combo.show[w].tab.height;
						if ( (w + 1) != Combo.ShowCount ){
							Combo.show[w + 1].box.top += Combo.show[w + 1].tab.height;
						}
					}
				}
				for ( i = 0 ; i < Combo.ShowCount ; i++ ){
					cps = &Combo.show[i];
					hdWins = DeferWindowPos(hdWins,
							Combo.base[cps->baseNo].hWnd, NULL,
							cps->box.left, cps->box.top,
							cps->box.right - cps->box.left,
							cps->box.bottom - cps->box.top,
							SWP_NOACTIVATE | SWP_NOZORDER);
				}
			}else{ // 通常
				if ( fixshowindex == SORTWIN_RESIZE ){
					SortPane_ResizeRates(0, Combo.ShowCount, left, right);
				}
				if ( !OverTabArea ){
					SortPane_Fix(0, Combo.ShowCount, left, right, PaneMinSize, 0);
				}else{
					int showI, Xmax, addr C4701CHECK, paneX = Combo.Panes.clientbox.left;
					int totalheight;

					totalheight = (Combo.Panes.box.bottom - Combo.Panes.box.top) + Combo.Height.TabBar;

					Xmax = Combo.Panes.clientbox.right;
					for ( showI = 0 ; showI < Combo.ShowCount ; showI++ ){
						int panerange;
						int panewidth, paneheight, panetop, tabtop, tabheight;

						cps = &Combo.show[showI];

						panerange = cps->box.right - cps->box.left;
						if ( (paneX + panerange) > Xmax ){
							if ( !(X_combos[0] & CMBS_TABFRAME) ){
								panerange = Xmax - paneX;
								if ( panerange < PaneMinSize ){
									panerange = PaneMinSize;
								}
							}
						}
						cps->box.left = paneX;
						paneX += panerange;
						cps->box.right = addr = paneX;
						paneX += SplitPix;
						if ( (showI < (Combo.ShowCount - 1)) &&
							 (paneX >= Xmax) &&
							 (panerange > 100) ){
							if ( !(X_combos[0] & CMBS_TABFRAME) ){
								paneX = cps->box.right = Xmax - SplitPix * 2;
								paneX += SplitPix;
							}
						}

						if ( (X_combos[0] & CMBS_TABEACHITEM) ||
							 (Combo.Tabs > 1) ){
							tabheight = cps->tab.height;
						}else{
							tabheight = Combo.show[0].tab.height;
						}

						if ( (totalheight - 32) < tabheight ){
							tabheight = totalheight - 32;
							if ( tabheight < PaneMinSize ){
								tabheight = PaneMinSize;
							}
						}
						paneheight = totalheight - tabheight;
						if ( X_combos[1] & CMBS1_TABBOTTOM ){
							cps->box.top = panetop = Combo.Height.TopArea;
							cps->box.bottom = tabtop = panetop + paneheight;
						}else{
							tabtop = Combo.Height.TopArea;
							cps->box.top = panetop = tabtop + tabheight;
							cps->box.bottom = panetop + paneheight;
						}

						if ( showI == (Combo.ShowCount - 1) ){
							if ( cps->box.right < Xmax ){
								cps->box.right = addr = Xmax;
							}
						}
						panewidth = cps->box.right - cps->box.left;
						hdWins = DeferWindowPos(hdWins,
								Combo.base[cps->baseNo].hWnd, NULL,
								cps->box.left, panetop,
								panewidth, paneheight,
								SWP_NOACTIVATE | SWP_NOZORDER);

						if ( showI < (Combo.ShowCount - 1) ){
							panewidth += SplitPix;
						}

						// タブ描画１（ここで描画しないときはタブ描画２で行う）
						if ( (Combo.Tabs > 1) /*&&
							 (X_combos[0] & CMBS_TABSEPARATE)*/ ){
							hdWins = DeferTabBar(hdWins, &cps->tab,
									cps->box.left, tabtop,
									panewidth, tabheight);
						}
					}
					Combo.Panes.clientbox.right = addr;\
				}
			}
			PaneRight = Combo.Panes.box.right;
			PaneBottom = Combo.Panes.box.bottom;
		}else{	// 基準indexありのとき
			int expand = 0;
			OverTabArea = FALSE;
			if ( (X_combos[0] & CMBS_QPANE) && (Combo.ShowCount > 2) ){
				PaneBottom = Combo.Panes.box.bottom;
				PaneRight = Combo.Panes.box.right;
			}else{
				SortPane_ResizeSize(left, right, PaneRight, 0);
				if ( fixshowindex >= SORTWIN_FIX_NORESIZE ){
					fixshowindex = SORTWIN_LAYOUTPAIN;
					PaneBottom = Combo.Panes.box.bottom;
				}else{
					// ↓ SORTWIN_NEWPANE 検出も兼用。pane 0 を大きさの基準に
					if ( fixshowindex >= Combo.ShowCount ){
						if ( fixshowindex >= SORTWIN_NEWPANE ){
							expand = Combo.show[0].box.right -
						 			Combo.show[0].box.left;
						}
						fixshowindex = 0;
					}
					PaneBottom = Combo.show[fixshowindex].box.bottom -
						 Combo.show[fixshowindex].box.top + Combo.Panes.box.top;
				}
			}

			if ( X_combos[0] & CMBS_TABFRAME ){
				PaneRight = Combo.Panes.box.right + expand;
				PaneBottom += GetSystemMetrics(SM_CYHSCROLL);
			}else{
				PaneRight += Combo.Panes.box.left + expand;
			}
		}
		if ( !OverTabArea &&
			 ( !(X_combos[0] & CMBS_QPANE) || (Combo.ShowCount <= 2)) ){
			SortPane_Setpos(top, bottom);
		}
	}else{ // 縦配列
		if ( fixshowindex < 0 ){	// 基準indexなしのとき
			if ( X_combos[1] & CMBS1_TABBOTTOM ){
				if ( fixshowindex == SORTWIN_RESIZE ){
					SortPane_ResizeRates(0, Combo.ShowCount, top, bottom);
				}
				SortPane_FixB(0, Combo.ShowCount, top, bottom, PaneMinHeight, cps->tab.height);
			}else{
				if ( fixshowindex == SORTWIN_RESIZE ){
					SortPane_ResizeRatesT(0, Combo.ShowCount, top, bottom);
				}
				SortPane_Fix(0, Combo.ShowCount, top, bottom, PaneMinHeight, cps->tab.height);
			}
			PaneRight = Combo.Panes.box.right;
			PaneBottom = Combo.Panes.box.bottom;

		}else{	// 基準indexありのとき
			if ( X_combos[1] & CMBS1_TABBOTTOM ){
				SortPane_ResizeSizeB(top, bottom, PaneBottom, cps->tab.height);
			}else{
				SortPane_ResizeSize(top, bottom, PaneBottom, cps->tab.height);
			}
			if ( X_combos[0] & CMBS_TABFRAME ){
				PaneBottom = Combo.Panes.box.bottom;
			}else{
				PaneBottom += Combo.Panes.box.top;
			}

			if ( fixshowindex >= SORTWIN_FIX_NORESIZE ){
				fixshowindex = SORTWIN_LAYOUTPAIN;
				PaneRight = Combo.Panes.box.right;
			}else{
				// ↓ SORTWIN_NEWPANE 検出も兼用。pane 0 を大きさの基準に
				if ( fixshowindex >= Combo.ShowCount ){
					if ( fixshowindex >= SORTWIN_NEWPANE ){
						PaneBottom += Combo.show[0].box.bottom -
								Combo.show[0].box.top;
					}
					fixshowindex = 0;
				}

				PaneRight = Combo.show[fixshowindex].box.right -
					Combo.show[fixshowindex].box.left + Combo.LeftAreaWidth;
			}
		}
		SortPane_Setpos(left, right);
	}
	PaneBottom += Combo.Height.ReportJob + Combo.Height.BottomDock;

	if ( X_combos[0] & CMBS_TABFRAME ){
		SCROLLINFO sinfo;
		int showpane;

//		EndDeferWindowPos(hdWins); // ←ここでSendMessageの受付が起きる
//		hdWins = BeginDeferWindowPos(7+1);

							// frame の幅を算出
		sinfo.nMax = 0;
		for ( showpane = 0 ; showpane < Combo.ShowCount ; showpane++ ){
			sinfo.nMax += Combo.show[showpane].box.right - Combo.show[showpane].box.left + SplitPix;
		}

		sinfo.cbSize = sizeof(sinfo);
		sinfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		sinfo.nMin = 0;
		sinfo.nPage = Combo.Panes.clientbox.right - Combo.Panes.clientbox.left;
		sinfo.nPos = Combo.Panes.delta.x;
		SetScrollInfo(Combo.Panes.hFrameWnd, SB_HORZ, &sinfo, TRUE);

		hdWins = DeferWindowPos(hdWins, Combo.Panes.hFrameWnd, NULL,
				Combo.Panes.box.left, Combo.Panes.box.top,
				Combo.Panes.box.right - Combo.Panes.box.left,
				Combo.Panes.box.bottom - Combo.Panes.box.top,
				SWP_NOZORDER | SWP_NOACTIVATE );
	}
	// 上部コントロールの寸法決定
	ToolBox.x	= 0;
	ToolBox.y		= Combo.Height.TopDock;
	ToolBox.width	= PaneRight;
	ToolBox.height	= Combo.ToolBar.Height;

//	InfoBox.x	= 0;
	InfoBox.y		= ToolBox.y + ToolBox.height;
//	InfoBox.width	= PaneRight;
	InfoBox.height	= Combo.Height.InfoLine;

	AddrBox.x	= 0;
	AddrBox.y		= InfoBox.y + InfoBox.height;
	AddrBox.width	= PaneRight;
	AddrBox.height	= Combo.Height.AddrBar;

	// 下部コントロールの寸法決定
	Combo.Report.box.y	= Combo.Panes.box.bottom + SplitPix;

	if ( !(X_combos[0] & CMBS_VPANE) && // 横整列
		 (X_combos[1] & CMBS1_TABBOTTOM) ){
		Combo.Report.box.y += Combo.Height.TabBar;
	}
	Combo.Report.box.width	= PaneRight;
	Combo.Report.box.height	= Combo.Height.ReportJob - SplitPix;

	// 中央コントロールの寸法決定(上下コントロールを決めてから算出)
	TreeBox.x	= 0;
	TreeBox.y		= Combo.Height.TopArea;
	TreeBox.width	= Combo.LeftAreaWidth;
	TreeBox.height	= Combo.Panes.box.bottom - TreeBox.y;
	if ( !(X_combos[0] & CMBS_VPANE) && (X_combos[1] & CMBS1_TABBOTTOM) ){
		TreeBox.height += Combo.Height.TabBar;
	}

	TabBox.x		= Combo.LeftAreaWidth;
	TabBox.y		= (X_combos[1] & CMBS1_TABBOTTOM) ? Combo.Panes.box.bottom : TreeBox.y;
	TabBox.width	= PaneRight - Combo.LeftAreaWidth;
	TabBox.height	= Combo.Height.TabBar;

	if ( Combo.Joblist.hWnd != NULL ){ // **1
		if ( Combo.Report.hWnd != NULL ){ // ログの右に表示
			JobBox.width = Combo.Joblist.JobAreaWidth;
			Combo.Report.box.width -= JobBox.width + SplitPix;
			if ( Combo.Report.box.width < MinSplitWidth ){
				Combo.Report.box.width = MinSplitWidth;
			}
		}else if ( Combo.hTreeWnd != NULL ){ // ツリー下に表示
			JobBox.height = TreeBox.height / 4;
			TreeBox.height -= JobBox.height;
		}
	}

	if ( (fixshowindex != SORTWIN_LAYOUTPAIN) &&
		 (fixshowindex != SORTWIN_FIXINNER) ){
		if ( Combo.Docks.t.hWnd != NULL ){
			hdWins = DeferWindowPos(hdWins, Combo.Docks.t.hWnd, NULL,
					0, 0,
					PaneRight + REBARFIXWIDTH, Combo.Height.TopDock,
					SWP_NOACTIVATE | SWP_NOZORDER);
		}
		if ( Combo.Docks.b.hWnd != NULL ){
			hdWins = DeferWindowPos(hdWins, Combo.Docks.b.hWnd, NULL,
					0, Combo.WndSize.cy - Combo.Height.BottomDock,
					PaneRight + REBARFIXWIDTH, Combo.Height.BottomDock,
					SWP_NOACTIVATE | SWP_NOZORDER);
		}

		if ( Combo.Joblist.hWnd != NULL ){
			if ( Combo.Report.hWnd != NULL ){ // ログの右に表示
				JobBox.x = Combo.Report.box.x + Combo.Report.box.width + SplitPix;
				JobBox.y = Combo.Report.box.y;
				JobBox.height = Combo.Report.box.height;
			}else if ( Combo.hTreeWnd != NULL ){ // ツリー下に表示
				JobBox.x = TreeBox.x;
				JobBox.y = TreeBox.y + TreeBox.height;
				JobBox.width = TreeBox.width - SplitPix;
			}else{	// 下段に表示
				JobBox.x = 0;
				JobBox.width = PaneRight;
				JobBox.height = Combo.Height.ReportJob - SplitPix;
				JobBox.y = Combo.WndSize.cy - Combo.Height.BottomDock - JobBox.height;
			}
			#pragma warning(suppress: 4701) // C4701ok **1 で初期化済み
			hdWins = DeferWindowPos(hdWins, Combo.Joblist.hWnd, NULL,
					JobBox.x, JobBox.y, JobBox.width, JobBox.height,
					SWP_NOACTIVATE | SWP_NOZORDER);
		}

		if ( Combo.hTreeWnd != NULL ){
			hdWins = DeferWindowPos(hdWins, Combo.hTreeWnd, NULL,
					TreeBox.x, TreeBox.y,
					TreeBox.width - SplitPix, TreeBox.height,
					SWP_NOACTIVATE | SWP_NOZORDER);
		}

		if ( Combo.Report.hWnd != NULL ){
			hdWins = DeferWindowPos(hdWins, Combo.Report.hWnd, NULL,
					Combo.Report.box.x, Combo.Report.box.y,
					Combo.Report.box.width, Combo.Report.box.height,
					SWP_NOACTIVATE | SWP_NOZORDER);
		}

		if ( Combo.hAddressWnd != NULL ){
			hdWins = DeferWindowPos(hdWins, Combo.hAddressWnd, NULL,
					AddrBox.x, AddrBox.y,
					AddrBox.width - Combo.Height.AddrBar, AddrBox.height,
					SWP_NOACTIVATE | SWP_NOZORDER);
		}

		if ( Combo.ToolBar.hWnd != NULL ){
			hdWins = DeferWindowPos(hdWins, Combo.ToolBar.hWnd, NULL,
					ToolBox.x, ToolBox.y,
					ToolBox.width, ToolBox.height,
					SWP_NOACTIVATE | SWP_NOZORDER);
		}
	}
	if ( Combo.Tabs ){ // タブ描画２（１で描画していないとき）
		if ( Combo.Tabs == 1 ){
			int showindex = (Combo.ShowCount > 1) ? GetComboShowIndexDefault(hComboFocusWnd) : 0;
			hdWins = DeferTabBar(hdWins, &Combo.show[(showindex >= 0) ? showindex : 0].tab,
					TabBox.x, TabBox.y, TabBox.width, TabBox.height);
		}else{
			int tabpane;
			#define TabTail(pane) ((pane < Combo.Tabs - 1) ? SplitPix : 0)

			if ( X_combos[0] & CMBS_VPANE ){ // 縦整列
				for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
					RECT *box;
					int taby;

					box = &Combo.show[tabpane].box;
					taby = (X_combos[1] & CMBS1_TABBOTTOM) ? box->bottom :
							box->top - Combo.show[tabpane].tab.height;
					hdWins = DeferTabBar(hdWins, &Combo.show[tabpane].tab,
							box->left, taby,
							box->right - box->left,
							Combo.show[tabpane].tab.height);
				}
						// 横整列
			}else if ( ((X_combos[0] & (CMBS_QPANE | CMBS_TABSEPARATE)) == (CMBS_QPANE | CMBS_TABSEPARATE)) && (Combo.ShowCount > 2) ){
				// 田形
				for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
					RECT *box;

					box = &Combo.show[tabpane].box;
					hdWins = DeferTabBar(hdWins, &Combo.show[tabpane].tab,
							box->left,
							box->top - Combo.show[tabpane].tab.height,
							box->right - box->left + TabTail(tabpane),
							OverTabArea ? Combo.show[tabpane].tab.height : TabBox.height);
				}
			}else if ( X_combos[1] & CMBS1_TABFIXLAYOUT ){ // 比率固定
				int x = Combo.Panes.clientbox.left, nx;
				int w = Combo.Panes.clientbox.right - Combo.Panes.clientbox.left;
				for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
					nx = Combo.Panes.clientbox.left + ((tabpane + 1) * w) / Combo.Tabs;
					hdWins = DeferTabBar(hdWins, &Combo.show[tabpane].tab,
							x, TabBox.y,  nx - x, TabBox.height);
					x = nx;
				}
			}else if ( !OverTabArea ){ // 通常
				for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
					RECT *box;

					box = &Combo.show[tabpane].box;
					hdWins = DeferTabBar(hdWins, &Combo.show[tabpane].tab,
							box->left, TabBox.y,
							box->right - box->left + TabTail(tabpane),
							TabBox.height);
				}
			}
		}
	}
	if ( hdWins != NULL ) EndDeferWindowPos(hdWins); // ←ここでSendMessageの受付が起きる

	// 整列後のコントロールの再調整
	if ( (fixshowindex != SORTWIN_FIXINNER) && // タブ行数が変化していたら再整列
		 (X_combos[0] & CMBS_TABMULTILINE) &&
		 Combo.Tabs ){
		int tabs;

		for ( tabs = 0 ; tabs < Combo.Tabs ; tabs++ ){
			int oldheight;

			oldheight = CalcTabBarHeight(&Combo.show[tabs].tab);
			if ( Combo.show[tabs].tab.height != oldheight ){
				SortComboWindows(SORTWIN_FIXINNER); // タブ高さが変化した
				return;
			}
		}
	}

	if ( Combo.ToolBar.hWnd != NULL ){ //下にずれるので再設定する
		GetWindowRect(Combo.ToolBar.hWnd, &pbox);
		if ( pbox.top != ToolBox.x ){
			SetWindowPos(Combo.ToolBar.hWnd, NULL,
				ToolBox.x, ToolBox.y, 0, 0,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
		}
	}
	if ( (fixshowindex >= 0) && (fixshowindex < Combo.ShowCount) &&
		 ((X_combos[0] & CMBS_VALWINSIZE) || EnablePaneSizeChange) ){
		int nw;

		EnablePaneSizeChange = FALSE;
							// Combo window の大きさを調節する
		GetWindowRect(Combo.hWnd, &pbox);
		GetClientRect(Combo.hWnd, &cbox);

		nw = PaneRight + ((pbox.right - pbox.left) - (cbox.right - cbox.left));

		if ( nw >= 16 ){
			// タブが下に配置しているときは、まだタブ高さが含まれていない
			if ( (X_combos[1] & CMBS1_TABBOTTOM) && Combo.Tabs ){
				PaneBottom += Combo.Height.TabBar;
			}

			Combo.Panes.resizewidth = -2;
			// SortComboWindows の再入がここで発生
			SetWindowPos(Combo.hWnd, NULL, 0, 0,
				nw,
				PaneBottom + ((pbox.bottom - pbox.top) - (cbox.bottom - cbox.top)),
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
		}
	}
	DEBUGLOGF("SortWindows - %d end", fixshowindex);
}

LRESULT WmComboNotify(NMHDR *nmh)
{
	if ( nmh->hwndFrom == NULL ) return 0;

	if ( IsTrue(DocksNotify(&Combo.Docks, nmh)) ){
		if ( (nmh->code == NM_CUSTOMDRAW) && (UseCCDrawBack > 1) ){
			if ( (nmh->hwndFrom != Combo.Docks.t.hWnd) && (nmh->hwndFrom != Combo.Docks.b.hWnd) ){ // toolbarに適用
				return PPxCommonExtCommand(K_DRAWCCBACK, (WPARAM)nmh);
			}else{
				return PPxCommonExtCommand(K_DRAWCCWNDBACK, (WPARAM)nmh);
			}
		}
		if ( nmh->code == RBN_HEIGHTCHANGE ){
			SortComboWindows(SORTWIN_LAYOUTALL);
		}
		return 0;
	}
	if ( nmh->code == TTN_NEEDTEXT ){
		if ( SetTabTipText(nmh) ) return 0;
		if ( SetToolBarTipText(Combo.ToolBar.hWnd, &Combo_thGuiWork, nmh) ){
			return 0;
		}
		if ( DocksNeedTextNotify(&Combo.Docks, nmh) ) return 0;
		return 0;
	}
	if ( nmh->hwndFrom == Combo.ToolBar.hWnd ){
		if ( nmh->code == NM_RCLICK ){
			if ( IsTrue(ToolBarDirectoryButtonRClick(Combo.hWnd, nmh, &Combo_thGuiWork)) ){
				return 0;
			}
			PostMessage(hComboFocusWnd, WM_PPXCOMMAND,
					TMAKELPARAM(K_POPOPS, PPT_MOUSE), 0);
			PostMessage(hComboFocusWnd, WM_PPXCOMMAND, K_layout, 0);
		}
		if ( nmh->code == TBN_DROPDOWN ){
			ComboToolbarCommand(((LPNMTOOLBAR)nmh)->iItem, K_s);
		}
		if ( (nmh->code == NM_CUSTOMDRAW) && (UseCCDrawBack > 1) ){
			return PPxCommonExtCommand(K_DRAWCCBACK, (WPARAM)nmh);
		}
		return 0;
	}
	if ( nmh->code == TCN_SELCHANGE ){
		SelectChangeTab(nmh);
	}
	return 0;
}

int SplitHitTest(POINT *pos)
{
	if ( pos->y < Combo.Panes.box.top ) return DMS_TOP;
	if ( pos->x < Combo.LeftAreaWidth ){
		if ( pos->x >= (Combo.LeftAreaWidth - SplitPix) ) return DMS_LEFT;
	}else{
		if ( pos->y < (Combo.Report.box.y - SplitPix) ){ // ペイン
			return DMS_PANE;
		}
	}

	if ( pos->y > Combo.Report.box.y ){
		return DMS_JOB;
	}
	return DMS_REPORT; // 水平線
}

LRESULT ComboLMouseDown(HWND hComboWnd, LPARAM lParam)
{
	POINT pos;

	LPARAMtoPOINT(pos, lParam);
	if ( (pos.y < InfoTop) || (pos.x < 0) ) return 0;
	SetCapture(hComboWnd);
	Combo.SizingPane = SplitHitTest(&pos);
	if ( Combo.SizingPane == DMS_TOP ){							// 情報行
		if ( X_combos[0] & CMBS_COMMONINFO ){
			int showindex, newHMpos, r;

			showindex = GetComboShowIndexDefault(hComboFocusWnd);
			if ( showindex >= 0 ){
				PPC_APPINFO *cinfo;

				cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
				if ( cinfo == NULL ) return 0;
				r = GetHiddenMenuItemTypeFromPoint(cinfo, InfoTop, Combo.Height.InfoLine, &pos, &newHMpos);
				if ( r != PPCR_HIDMENU ) newHMpos = -1;
				if ( newHMpos >= 0 ) cinfo->DownHMpos = newHMpos;
			}
		}
		return 0;
	}
	if ( Combo.SizingPane == DMS_PANE ){
		int showindex;

		showindex = GetComboShowIndexFromPos(&pos);
		if ( showindex >= 0 ) Combo.SizingPane = DMS_PANE + showindex;
	}
	return 0;
}

LRESULT ComboMouseButton(LPARAM lParam, WORD type)
{
	POINT pos;

	LPARAMtoPOINT(pos, lParam);

	if ( (pos.y >= AddrTop) && (pos.y < AddrBottom) ){
		SendMessage(Combo.hAddressWnd, WM_PPXCOMMAND, K_cr, 0);
		EnterAddressBar();
		return 0;
	}

	if ( (pos.y < InfoBottom) && (X_combos[0] & CMBS_COMMONINFO) ){
		int showindex, n, r;

		showindex = GetComboShowIndexDefault(hComboFocusWnd);
		if ( showindex >= 0 ){
			PPC_APPINFO *cinfo;

			cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
			if ( cinfo != NULL ){
				r = GetHiddenMenuItemTypeFromPoint(cinfo, InfoTop, Combo.Height.InfoLine, &pos, &n);
				PostMessage(cinfo->info.hWnd, WM_PPXCOMMAND,
						KC_MOUSECMD + (type << 16), r + (n << 16));
			}
		}
	}
	return 0;
}

LRESULT ComboLMouseUp(LPARAM lParam)
{
	// ※既に WM_CAPTURECHANGED が済んでいる→ Combo.SizingPane = DMS_NONE 状態
	ReleaseCapture();
	if ( Combo.SizingPane == DMS_NONE ) ComboMouseButton(lParam, 'L');
	Combo.SizingPane = DMS_NONE;
	return 0;
}

LRESULT ComboLMouseDbl(HWND hComboWnd, LPARAM lParam)
{
	POINT pos;
	int showindex;
	HWND hTargetWnd;

	LPARAMtoPOINT(pos, lParam);
	if ( pos.y < InfoBottom ) return ComboMouseButton(lParam, 'L' + ('D'<<8));

	showindex = GetComboShowIndexFromPos(&pos);
	if ( showindex >= 0 ){
		hTargetWnd = Combo.base[Combo.show[showindex].baseNo].hWnd;
		ClientToScreen(hComboWnd, &pos);
		PostMessage(hTargetWnd, WM_NCLBUTTONDBLCLK,
				HTRIGHT, TMAKELPARAM(pos.x, pos.y));
	}
	return 0;
}

#pragma argsused
VOID CALLBACK HideMenuTimerProc(HWND hFrameWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	POINT pos;
	int showindex, HMpos = -1, r;
	PPC_APPINFO *cinfo;
	UnUsedParam(uMsg);UnUsedParam(dwTime);

	GetCursorPos(&pos);
	ScreenToClient(hFrameWnd, &pos);

	showindex = GetComboShowIndexDefault(hComboFocusWnd);
	if ( showindex < 0 ) return;

	cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
	if ( cinfo == NULL ) return;
	if ( pos.y < InfoBottom ){
		r = GetHiddenMenuItemTypeFromPoint(cinfo, InfoTop, Combo.Height.InfoLine, &pos, &HMpos);
		if ( r != PPCR_HIDMENU ) HMpos = -1;
	}
	if ( HMpos < 0 ){
		RECT rect;

		KillTimer(hFrameWnd, idEvent);
		cinfo->HMpos = cinfo->DownHMpos = -1;

		rect.left	= 0;
		rect.top	= InfoTop;
		rect.right	= Combo.WndSize.cx;
		rect.bottom	= InfoBottom;
		InvalidateRect(hFrameWnd, &rect, FALSE);
	}
	return;
}

LRESULT ComboMouseMove(HWND hFrameWnd, LPARAM lParam)
{
	POINT pos;
	RECT box;
	LPCTSTR cr;

	LPARAMtoPOINT(pos, lParam);
	switch ( Combo.SizingPane ){
		case DMS_LEFT:	// 左エリアの右境界線ドラッグ中
			if ( pos.x < MinSplitWidth ) pos.x = MinSplitWidth;
			if ( Combo.LeftAreaWidth != pos.x ){
				Combo.LeftAreaWidth = pos.x;
				SortComboWindows(SORTWIN_LAYOUTALL);	// combo 内調整
				box.left = 0;
				box.top = Combo.Panes.box.top;
				box.right = Combo.WndSize.cx;
				box.bottom = Combo.Report.box.y + Combo.Height.ReportJob;
				InvalidateRect(hFrameWnd, &box, TRUE);
			}
			return 0;

		case DMS_REPORT:	// ログエリアの上境界線ドラッグ中
			if ( pos.y < (Combo.Panes.box.top + MinSplitWidth) ){
				pos.y = Combo.Panes.box.top + MinSplitWidth;
			}
			if ( pos.y > (Combo.Report.box.y + Combo.Height.ReportJob - MinSplitWidth) ){
				pos.y = (Combo.Report.box.y + Combo.Height.ReportJob - MinSplitWidth);
			}

			if ( Combo.Report.box.y != pos.y ){
				Combo.Height.ReportJob = Combo.Report.box.y + Combo.Height.ReportJob - pos.y;
				SortComboWindows(SORTWIN_LAYOUTALL);	// combo 内調整
				box.left = 0;
				box.top = Combo.Panes.box.top;
				box.right = Combo.WndSize.cx;
				box.bottom = Combo.Report.box.y + Combo.Height.ReportJob;
				InvalidateRect(hFrameWnd, &box, TRUE);
			}
			return 0;

		case DMS_JOB:	// jobエリアの左境界線ドラッグ中
			if ( pos.x < (Combo.Report.box.x + MinSplitWidth) ){
				pos.x = Combo.Report.box.x + MinSplitWidth;
			}
			if ( pos.x > (Combo.WndSize.cx - MinSplitWidth) ){
				pos.x = (Combo.WndSize.cx - MinSplitWidth);
			}

			if ( (Combo.WndSize.cx - Combo.Joblist.JobAreaWidth) != pos.x ){
				Combo.Joblist.JobAreaWidth = Combo.WndSize.cx - pos.x;
				SortComboWindows(SORTWIN_LAYOUTALL);	// combo 内調整

				box.left = 0;
				box.top = Combo.Report.box.y;
				box.right = Combo.WndSize.cx;
				box.bottom = Combo.Report.box.y + Combo.Height.ReportJob;
				InvalidateRect(hFrameWnd, &box, TRUE);
			}
			return 0;
	}
	if ( Combo.SizingPane >= DMS_PANE ){	// 窓枠ドラッグ中
		if ( !(X_combos[0] & CMBS_VPANE) ){	// 横整列
			int x;

			x = Combo.show[Combo.SizingPane].box.left + (SplitPix / 2 + 1);
			if ( x > pos.x ) pos.x = x;
			if ( Combo.show[Combo.SizingPane].box.right != pos.x ){
				Combo.show[Combo.SizingPane].box.right = pos.x;
				SortComboWindows(SORTWIN_LAYOUTPAIN);	// combo 内調整
			}
		}else{ // 縦配列
			int y;

			if ( X_combos[1] & CMBS1_TABBOTTOM ){
				pos.y -= Combo.show[Combo.SizingPane].tab.height;
			}
			y = Combo.show[Combo.SizingPane].box.top + (SplitPix / 2 + 1);
			if ( y > pos.y ) pos.y = y;
			if ( Combo.show[Combo.SizingPane].box.bottom != pos.y ){
				Combo.show[Combo.SizingPane].box.bottom = pos.y;
				SortComboWindows(SORTWIN_LAYOUTPAIN);	// combo 内調整
			}
		}
		// pane , tab
		InvalidateRect(hFrameWnd, &Combo.Panes.box, TRUE);
		return 0;
	}

	// 情報行等 or 非ドラッグ
	if ( hFrameWnd == Combo.hWnd ){
		if ( (pos.y < Combo.Panes.box.top) && (X_combos[0] & CMBS_COMMONINFO) ){ // 情報行(隠しメニュー)
			int showindex, HMpos, r;

			showindex = GetComboShowIndexDefault(hComboFocusWnd);
			if ( showindex >= 0 ){
				PPC_APPINFO *cinfo;

				cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
				if ( cinfo != NULL ){
					r = GetHiddenMenuItemTypeFromPoint(cinfo, InfoTop, Combo.Height.InfoLine, &pos, &HMpos);
					if ( r != PPCR_HIDMENU ) HMpos = -1;
					if ( HMpos != cinfo->HMpos ){
						RECT rect;

						cinfo->HMpos = HMpos;
						rect.left	= cinfo->iconR;
						rect.top	= InfoTop;
						rect.right	= ((cinfo->HiddenMenu.item + 1) >> 1) *
								cinfo->HiddenMenu.width * cinfo->fontX + rect.left;
						rect.bottom	= InfoBottom;
						InvalidateRect(hFrameWnd, &rect, FALSE);

						if ( HMpos >= 0 ){
							SetTimer(hFrameWnd, TIMERID_COMBOHIDEMENU,
									TIME_COMBOHIDEMENU, HideMenuTimerProc);
						}
					}
				}
			}
		}
		switch ( SplitHitTest(&pos) ){	// カーソル形状を決定・設定
			case DMS_LEFT:
			case DMS_JOB:
				cr = IDC_SIZEWE;
				break;
			case DMS_REPORT:
				cr = IDC_SIZENS;
				break;
			case DMS_PANE: // ペイン
				if ( !(X_combos[0] & CMBS_VPANE) ){
					cr = IDC_SIZEWE;
				}else{
					cr = IDC_SIZENS;
				}
				break;
			default: // DMS_NONE / DMS_TOP
				cr = IDC_ARROW;
		}
	}else{ // フレーム上
		if ( !(X_combos[0] & CMBS_VPANE) ){
			cr = IDC_SIZEWE;
		}else{
			cr = IDC_SIZENS;
		}
	}

	if ( Combo.MouseCsrName != cr ){
		Combo.MouseCsrName = cr;
		Combo.hMouseCsr = LoadCursor(NULL, cr);
	}
	SetCursor(Combo.hMouseCsr);
	return 0;
}

// 非クライアント領域の判別 ---------------------------------------------------
LRESULT ComboNCMouseCommand(HWND hComboWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	int showindex;

	LPARAMtoPOINT(pos, lParam);
	ScreenToClient(hComboWnd, &pos);
	showindex = GetComboShowIndexFromPos(&pos);
	if ( showindex >= 0 ){
		PostMessage(Combo.base[Combo.show[showindex].baseNo].hWnd, message, wParam, lParam);
	}
	return 0;
}

// ペインを削除したときの窓調整
#define SortPane_ResizeEq(posmin, posmax) {\
	int possize, base, pos, I_;\
\
	pos = base = Combo.Panes.clientbox.posmin;\
	possize = Combo.Panes.clientbox.posmax - Combo.Panes.clientbox.posmin;\
\
	for ( I_ = 0 ; I_ < Combo.ShowCount ; I_++ ){\
		Combo.show[I_].box.posmin = pos;\
		pos = base + ( (possize * (I_ + 1)) / Combo.ShowCount);\
		Combo.show[I_].box.posmax = pos;\
	}\
}

void RequestPairRate(SCW_REQSIZE *rs)
{
	int showindex, baseindex;
	RECT box, *oldbox, *pairbox;
	PPC_APPINFO *cinfo;

	if ( Combo.ShowCount < 2 ) return;	// 反対窓はない

	if ( rs->mode == FPS_ALLSAMERATE ){	// 全て等間隔
		if ( X_combos[0] & CMBS_VPANE ){
			SortPane_ResizeEq(top, bottom);
		}else{
			SortPane_ResizeEq(left, right);
		}
		SortComboWindows(SORTWIN_LAYOUTALL);
		return;
	}

	showindex = GetComboShowIndex(rs->hWnd);
	if ( showindex < 0 ) return;
	baseindex = Combo.show[showindex].baseNo;

	if ( (cinfo = Combo.base[baseindex].cinfo) == NULL ) return;
	oldbox = &Combo.show[showindex].box;
	box = *oldbox;
	pairbox = &Combo.show[GetPairPaneComboShowIndex(rs->hWnd)].box;

	if ( rs->mode == FPS_KEYBOARD ){	// キーボード操作
			// 上下で左右調整は不可
		if ( X_combos[0] & CMBS_VPANE ){
			if ( rs->offsetx != 0 ) return;
		}else{
			if ( rs->offsety != 0 ) return;
		}
		if ( showindex ){	// 反対窓なら反転
			rs->offsetx = -rs->offsetx;
			rs->offsety = -rs->offsety;
		}
	}else if ( rs->mode == FPS_RATE ){
		int thissize, pairsize;

		if ( X_combos[0] & CMBS_VPANE ){
			thissize = box.bottom - box.top;
			pairsize = pairbox->bottom - pairbox->top;
			box.bottom += (thissize + pairsize) * rs->offsety / 100 - thissize;
		}else{
			thissize = box.right - box.left;
			pairsize = pairbox->right - pairbox->left;
			box.right += (thissize + pairsize) * rs->offsetx / 100 - thissize;
		}
	}else{
		if ( X_combos[0] & CMBS_VPANE ){
			rs->offsety = rs->offsetx;
			rs->offsetx = 0;
		}
	}
	if ( rs->mode != FPS_RATE ){
		box.right += rs->offsetx * cinfo->fontX;
		box.bottom += rs->offsety * cinfo->fontY;
	}
	if ( (box.right - box.left) < cinfo->fontX ){
		box.right = box.left + cinfo->fontX;
	}
	if ( (box.bottom - box.top) < cinfo->fontY ){
		box.bottom = box.top + cinfo->fontY;
	}
	if ( EqualRect(oldbox, &box) == FALSE ){
		int delta, pairsize, fixdelta;

		if ( X_combos[0] & CMBS_VPANE ){ // 上下
			delta = (box.bottom - box.top) - (oldbox->bottom - oldbox->top);
			pairsize = pairbox->bottom - pairbox->top;
			fixdelta = (pairsize - delta) - cinfo->fontY;
			if ( fixdelta < 0 ){
				delta += fixdelta;
				if ( delta <= 0 ) return;	// これ以上は無理なので中止
				box.bottom += fixdelta;
			}
			pairbox->top += delta;
		}else{ // 左右
			delta = (box.right - box.left) - (oldbox->right - oldbox->left);
			pairsize = pairbox->right - pairbox->left;
			fixdelta = (pairsize - delta) - SplitPix;
			if ( fixdelta < 0 ){
				delta += fixdelta;
				if ( delta <= 0 ) return;	// これ以上は無理なので中止
				box.right += fixdelta;
			}
			pairbox->left += delta;
		}
		*oldbox = box;
		SortComboWindows(showindex + SORTWIN_FIX_NORESIZE);
		InvalidateRect(Combo.hWnd, &Combo.Panes.box, TRUE);
	}
}

void KCW_treeCombo(int mode, const TCHAR *path)
{
	TCHAR treepath[VFPS];

	if ( mode == PPCTREECOMMAND ){
		if ( Combo.hTreeWnd == NULL ){	// 無い→作成
			if ( tstrcmp(path, T("off")) == 0 ) return; // close 済み
			CreateLeftArea(PPCTREE_SYNC, NilStr, TRUE);
			InvalidateRect(Combo.hWnd, NULL, TRUE);
			SortComboWindows(SORTWIN_LAYOUTALL);
			if ( *path == '\0' ) return;
		}
		if ( Combo.hTreeWnd != NULL ){
			SendMessage(Combo.hTreeWnd, VTM_TREECOMMAND, 0, (LPARAM)path);
		}
		return;
	}

	if ( Combo.hTreeWnd == NULL ){	// 無い→作成
		tstrcpy(treepath, path);
		CreateLeftArea(mode, treepath, FALSE);
	}else{
		if ( mode == PPCTREE_SYNC ){
			CloseLeftArea();
			SetFocus(hComboFocusWnd);
		}else{
			return;
		}
	}
	ReplyMessage(SENDCOMBO_OK); // 呼び出し元を続行させる
	InvalidateRect(Combo.hWnd, NULL, TRUE);
	SortComboWindows(SORTWIN_LAYOUTALL);
}

void USEFASTCALL CheckRightFocus(void)
{
	if ( Combo.BaseCount <= 1 ){ // 窓が１つのみ
		if ( hComboSubPaneFocus == NULL ) return;
	}else if ( hComboSubPaneFocus != NULL ){ // 窓が複数…必ずhComboSubPaneFocus有効
		int showindex = GetComboShowIndex(hComboSubPaneFocus);

		if ( Combo.ShowCount <= 1 ){ // ペインが１つのみ
			if ( showindex < 0 ) return;
		}else if ( showindex >= 1 ){
			return;
		}
	}
	ResetSubFocus(NULL);
}

int GetPspPane(int paneid)
{
	if ( paneid < PSPONE_PANE_SETPANE ){
		switch (paneid){
			case PSPONE_PANE_PAIR:
				paneid = GetPairPaneComboShowIndex(hComboFocusWnd);
				break;

			case PSPONE_PANE_NEWPANE:
				paneid = -2;
				break;

			case PSPONE_PANE_RIGHTPANE:
				paneid = GetComboShowIndex(hComboSubPaneFocus);
				if ( Combo.MainPane > paneid ) paneid = Combo.MainPane;
				break;

			case PSPONE_PANE_LEFTPANE:
				paneid = GetComboShowIndex(hComboSubPaneFocus);
				if ( Combo.MainPane < paneid ) paneid = Combo.MainPane;
				break;

			case PSPONE_PANE_MAINPANE:
				paneid = Combo.MainPane;
				break;

			case PSPONE_PANE_SUBPANE:
				paneid = GetComboShowIndex(hComboSubPaneFocus);
				break;

			default: // PSPONE_PANE_DEFAULT と未定義
				paneid = -1;
		}
	}else{ // PSPONE_PANE_SETPANE
		paneid -= PSPONE_PANE_SETPANE;
	}
	return paneid;
}

LRESULT USEFASTCALL KCW_EntryCombo(HWND hEntryWnd, DWORD type)
{
	COMBOITEMSTRUCT *cws;
	WINPOS entrybox;
	int showpane = -1; // 追加した窓を表示させるためのペイン
	TCHAR id[16], value[32];

	if ( AddDelCount ){
		XMessage(Combo.hWnd, NULL, XM_DbgLOG, T("combo entry - nested %x"), hEntryWnd);
		PostMessage(Combo.hWnd, WM_PPCOMBO_NESTED_ENTRY, (WPARAM)type, (LPARAM)hEntryWnd);
		return SENDCOMBO_OK;
	}
	AddDelCount++;

	if ( (Combo.BaseCount >= Combo_Max_Base) ||
		 ((cws = HeapReAlloc( hProcessHeap, 0, Combo.base, sizeof(COMBOITEMSTRUCT) * (Combo.BaseCount + 2) )) == NULL) ){
		PostMessage(hEntryWnd, WM_CLOSE, 0, 0);
		AddDelCount--;
		ReplyMessage(SENDCOMBO_OK);
		XMessage(hEntryWnd, NULL, XM_GrERRld, MES_EOEC);
		return SENDCOMBO_OK;
	}
	Combo.base = cws;

	cws = &Combo.base[Combo.BaseCount];
	cws->hWnd = hEntryWnd;
	cws->capture = CAPTURE_NONE;
	cws->ActiveID = Combo.Active.high++;
	cws->tabbackcolor = C_AUTO;
	cws->tabtextcolor = C_AUTO;
	Combo.BaseCount++;

	cws->cinfo = NULL;
	if ( LOWORD(type) == KCW_capture ){
		if ( TMAKELPARAM(1, KCW_captureEx) ==
				SendMessage(hEntryWnd, WM_PPXCOMMAND, KCW_captureEx, 0) ){
			cws->capture = CAPTURE_WINDOWEX;
		}
	}else{
		DWORD PanePID;

		GetWindowThreadProcessId(hEntryWnd, &PanePID);
		if ( PanePID == GetCurrentProcessId() ){
			cws->cinfo = (PPC_APPINFO *)GetWindowLongPtr(hEntryWnd, GWLP_USERDATA);
			if ( (cws->cinfo != NULL) && (Combo.Closed.list != NULL) ){
				const TCHAR *ptr;
				int index;
				int baseindex, maxindex;

				// 同じIDが既に登録していたら削除する
				maxindex = Combo.BaseCount - 1;
				for ( baseindex = 0 ; baseindex < maxindex ; baseindex++ ){
					PPC_APPINFO *combo_cinfo;

					combo_cinfo = Combo.base[baseindex].cinfo;
					if ( combo_cinfo == NULL ) continue;
					if ( tstricmp(cws->cinfo->RegSubCID, combo_cinfo->RegSubCID) == 0 ){
						if ( (tstricmp(cws->cinfo->RegSubCID, T("CZ")) == 0) &&
							(hEntryWnd != combo_cinfo->info.hWnd) ){
							break; // CZ は重複有り
						}

						{ // 全てのタブからこの窓を削除
							int tabindex, tabpane;

							for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
								tabindex = GetTabItemIndex(combo_cinfo->info.hWnd, tabpane);
								if ( tabindex >= 0 ){
									TabCtrl_DeleteItem(Combo.show[tabpane].tab.hWnd, tabindex);
								}
							}
						}
						Combo.BaseCount -= 1;
						cws = &Combo.base[baseindex];
						cws->hWnd = hEntryWnd;

						thprintf(value, TSIZEOF(value), T("%s:%s,%d"),
								(hEntryWnd == combo_cinfo->info.hWnd) ?
								T("同ID再設定") : T("重複ID設定"),
								combo_cinfo->RegSubCID,
								IsWindow(combo_cinfo->info.hWnd) );
						PPxCommonExtCommand(K_SENDREPORT, (WPARAM)value);
						break;
					}
				}
				thprintf(id, TSIZEOF(id), T("%s_tabcolor"), (cws->cinfo->RegSubIDNo < 0) ? cws->cinfo->RegID : cws->cinfo->RegSubCID);
				value[0] = '\0';
				GetCustTable(T("_Path"), id, value, sizeof(value));
				ptr = value;
				if ( value[0] != '\0' ){
					cws->tabtextcolor = GetIntNumber(&ptr);
					if ( *ptr == ',' ) ptr++;
					cws->tabbackcolor = GetIntNumber(&ptr);
				}

				for ( index = 0; index < Combo.Closed.Items; index++ ){
					if ( !tstrcmp(Combo.Closed.list[index].ID, cws->cinfo->RegSubCID) ){
						Combo.Closed.list[index].ID[0] = '\0'; // 閉じたIDを再利用
						break;
					}
				}
			}
		}
	}

	if ( type >= KCW_entry_DEFPANE ){
		showpane = GetPspPane( (type / KCW_entry_DEFPANE) - 1 );
	}
				// ペインを追加して登録
	if ( (Combo.ShowCount < Combo_Max_Show) &&
		 ( (showpane == -2) ||
		   ((Combo.ShowCount < X_mpane.limit) &&
			( (showpane < 0) || (showpane >= Combo.ShowCount) ))) ){
		thprintf(value, TSIZEOF(value), T("%s%d"), ComboID, Combo.ShowCount);
		if ( NO_ERROR != GetCustTable(Str_WinPos, value, &entrybox, sizeof(entrybox)) ){
			GetWindowRect(hEntryWnd, &entrybox.pos);
		}

		// ペインを追加するための隙間を準備する(窓サイズ固定時 or 起動時)
		if ( (!(X_combos[0] & CMBS_VALWINSIZE) || (ComboInit > 0)) && Combo.ShowCount ){
			RECT *tbox;

			tbox = &Combo.show[0].box;
			if ( !(X_combos[0] & CMBS_VPANE) ){	// 横整列
				int addwidth, i;

				if ( (X_combos[0] & CMBS_QPANE) && (Combo.ShowCount >= 2) ){
					if ( (Combo.ShowCount & 1) ){
						tbox = &Combo.show[Combo.ShowCount - 1].box;
						tbox->right = (tbox->right + tbox->left) / 2;
					}
				}else{
					addwidth = entrybox.pos.right - entrybox.pos.left + SplitPix;

					if ( (tbox->right - tbox->left) >
							(addwidth + (addwidth >> 1)) ){
						tbox->right = tbox->left +
						 max(tbox->right - tbox->left - addwidth, PaneMinSize);
					}else{
						addwidth = ((Combo.WndSize.cx - Combo.LeftAreaWidth)
							 - (SplitPix * Combo.ShowCount)) /
							 (Combo.ShowCount + 1);

						for ( i = 0 ; i < Combo.ShowCount ; i++ ){
							tbox = &Combo.show[i].box;
							tbox->right = tbox->left + max(addwidth, PaneMinSize);
						}
					}
				}
			}else{ // 縦配列
				int addwidth, i;

				addwidth = entrybox.pos.bottom - entrybox.pos.top + SplitPix;

				if ( (tbox->bottom - tbox->top) >
						(addwidth + (addwidth >> 1)) ){
					tbox->bottom = tbox->top +
					 max(tbox->bottom - tbox->top - addwidth, PaneMinSize);
				}else{
					addwidth = ((Combo.Panes.box.bottom - Combo.Panes.box.top) -
							(SplitPix * Combo.ShowCount)) /
							(Combo.ShowCount + 1);
					for ( i = 0 ; i < Combo.ShowCount ; i++ ){
						tbox = &Combo.show[i].box;
						tbox->bottom = tbox->top + max(addwidth, PaneMinSize);
					}
				}
			}
		}
		Combo.show[Combo.ShowCount].box = entrybox.pos;
															// タブに登録する
		CreatePane(Combo.BaseCount - 1);
		CheckRightFocus();
		ShowWindow(hEntryWnd, SW_SHOWNORMAL);

		SortComboWindows((!(X_combos[0] & CMBS_VALWINSIZE) || (ComboInit > 0)) ?
				// 窓幅固定
				((Combo.ShowCount == 1) ?
					SORTWIN_LAYOUTALL : SORTWIN_LAYOUTPAIN) :
				// 窓幅可変
				((Combo.ShowCount - 1) | SORTWIN_NEWPANE));

		if ( showpane < 0 ) showpane = Combo.ShowCount - 1;

		if ( (X_combos[1] & CMBS1_TABSHOWMULTI) && (Combo.Tabs == 0) && (Combo.ShowCount > 1) ){
			CreateTabBar(CREATETAB_APPEND, NULL, FALSE);
			SortComboWindows(SORTWIN_LAYOUTPAIN);
		}
	}else{ // X_mpane.max を越えたので、ペインを追加しないで登録
		int addtabtext = Combo.Tabs;

		if ( Combo.Tabs == 0 ){
			addtabtext = (X_combos[0] & CMBS_TABEACHITEM) ? 1 : 0;
			CreateTabBar(CREATETAB_APPEND, NULL, FALSE);
		}
		CheckRightFocus();
		ShowWindow(hEntryWnd, SW_HIDE);
		SortComboWindows(SORTWIN_LAYOUTPAIN);

		if ( showpane < 0 ){
			showpane = GetComboShowIndex(hComboFocusWnd);
			if ( showpane < 0 ) showpane = 0;
		}
															// タブに登録する
		if ( addtabtext ){
			SetTabInfoData(SETTABINFO_ADDENTRY,
					((X_combos[0] & (CMBS_TABEACHITEM | CMBS_TABSEPARATE)) == CMBS_TABSEPARATE) ? -1 : showpane, hEntryWnd);
		}
	}

	if ( Combo.Docks.t.cinfo == NULL ){
		Combo.Docks.t.cinfo = cws->cinfo;
		Combo.Docks.b.cinfo = cws->cinfo;
		if ( Combo.Docks.b.hWnd != NULL ) DockFixPPcBarSize(&Combo.Docks.b);
		if ( Combo.Docks.t.hWnd != NULL ) DockFixPPcBarSize(&Combo.Docks.t);
	}
	if ( ComboInit == 0 ){
		if ( type & KCW_entry_SELECTNA ){
			SelectComboWindow(showpane, hEntryWnd, FALSE);
		}else if ( !(type & KCW_entry_NOACTIVE) ){
			SelectComboWindow(showpane, hEntryWnd, TRUE);
			CheckComboTable(T("KCW_EntryCombo3"));
		}
	}
	AddDelCount--;
	return SENDCOMBO_OK;
}

// hTargetWnd がフォーカスになったことを反映させる
void KCW_FocusFix(HWND hComboWnd, HWND hTargetWnd)
{
	int baseindex;

	if ( hComboFocusWnd == hTargetWnd ) return;
	baseindex = GetComboBaseIndex(hTargetWnd);
	if ( baseindex < 0 ) return;

	PostMessage(hComboFocusWnd, WM_PPXCOMMAND, KC_UNFOCUS, 0); // 解除通知
	Combo.base[baseindex].ActiveID = Combo.Active.high++;

	if ( GetComboShowIndex(hTargetWnd) < 0 ){ // 対象がペインのアクティブでない
		int si;
		HWND hHideWnd;

		if ( X_combos[0] & CMBS_TABEACHITEM ){ // 該当ペインを検索する
			int showindex;

			si = -1;
			for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
				if ( GetTabItemIndex(hTargetWnd, showindex) >= 0 ){
					si = showindex;
					break;
				}
			}
		}else{ // カレントペイン
			si = GetComboShowIndex(hComboFocusWnd);
		}

		if ( si < 0 ) si = 0;
		hHideWnd = Combo.base[Combo.show[si].baseNo].hWnd;

		if ( Combo.ShowCount == 1 ){
			hComboSubPaneFocus = hHideWnd;
		}else{
			if ( hHideWnd == hComboSubPaneFocus ){
				hComboSubPaneFocus = hTargetWnd;
			}
		}

		Combo.show[si].baseNo = baseindex;
		SendMessage(hComboWnd, WM_SETREDRAW, FALSE, 0);
		ShowWindow(hHideWnd, SW_HIDE);
		SortComboWindows(SORTWIN_LAYOUTPAIN);
		SendMessage(hComboWnd, WM_SETREDRAW, TRUE, 0);
		ShowWindow(hTargetWnd, SW_SHOWNORMAL);
	}else{
		// 反対窓の情報設定
		if ( Combo.BaseCount > 1 ){
			if ( Combo.ShowCount == 1 ){ // １ペインのみ…現在表示が反対窓に
				hComboSubPaneFocus = hComboFocusWnd;
				if ( (hComboSubPaneFocus == NULL) ||
					 (GetComboBaseIndex(hComboSubPaneFocus) < 0) ){
					hComboSubPaneFocus = Combo.base[baseindex ? 0 : 1].hWnd;
				}
			}else{ // ２ペイン以上
				int targetshow;

				targetshow = GetComboShowIndex(hTargetWnd);
				if ( targetshow != Combo.MainPane ){ // サブペイン変更
					hComboSubPaneFocus = hTargetWnd;
				}else if ( (GetComboShowIndex(hComboSubPaneFocus) < 0) ||
						(GetComboBaseIndex(hComboSubPaneFocus) < 0) ){
					// hComboSubPaneFocus が存在しない
					hComboSubPaneFocus = NULL;
				}
			}
		}else{ // 反対窓が存在しない
			hComboSubPaneFocus = NULL;
		}
	}
	ChangeReason = T("FocusFix");

	// フォーカスに関連する情報を保存しなおし
	Combo.Docks.t.cinfo = Combo.Docks.b.cinfo = Combo.base[baseindex].cinfo;

	if ( Combo.base[baseindex].cinfo != NULL ){
		SetComboAddresBar(Combo.base[baseindex].cinfo->path);
		hComboFocusWnd = hTargetWnd;
	}

	if ( ((hComboSubPaneFocus == NULL) && (Combo.BaseCount >= 2)) ||
		 (hComboFocusWnd == hComboSubPaneFocus) ){
		ResetSubFocus(NULL);
	}

	// フォーカスが変化した窓と、タブを再描画
	InvalidateRect(hComboWnd, NULL, TRUE); // pane, tab, info, status
	if ( Combo.Tabs ){
		int selectpane;

		selectpane = GetComboShowIndex(hTargetWnd);
		if ( selectpane < 0 ) selectpane = 0;
		SelectTabByWindow(hTargetWnd, selectpane);
	}
	CheckComboTable(T("KCW_FocusFix1"));
}

LRESULT KCW_combonextppc(HWND hWnd, HWND hTargetWnd, int mode)
{
	int baseindex, si;

	if ( Combo.BaseCount < 2 ){
		PPCui(hWnd, NULL);
		return SENDCOMBO_OK;
	}
	if ( mode == 0 ){	// 反対窓へ
		baseindex = GetPairPaneComboBaseIndex(hTargetWnd);
		if ( baseindex < 0 ) return 0;
	}else{		// 基準指定有り
		baseindex = GetComboBaseIndex(hTargetWnd);
		if ( baseindex < 0 ) return 0;
		baseindex = baseindex + mode;
		if ( baseindex < 0 ) baseindex = Combo.BaseCount - 1;
		if ( baseindex >= Combo.BaseCount ) baseindex = 0;
	}

	if ( Combo.Tabs ){
		for ( si = 0 ; si < Combo.ShowCount ; si++ ){ // 表示中タブにある？
			if ( Combo.show[si].baseNo == baseindex ){
				SetFocus(Combo.base[Combo.show[si].baseNo].hWnd);
				return SENDCOMBO_OK;
			}
		}
		// 隠れているので、ペインのタブを切換
		if ( X_combos[0] & CMBS_TABEACHITEM ){ // 該当ペインを検索する
			int showindex;

			si = -1;
			for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
				if ( GetTabItemIndex(Combo.base[baseindex].hWnd, showindex) >= 0 ){
					si = showindex;
					break;
				}
			}
		}else{ // カレントペイン
			si = GetComboShowIndex(hComboFocusWnd);
		}

		if ( si < 0 ) si = 0;

		SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
		ShowWindow(Combo.base[Combo.show[si].baseNo].hWnd, SW_HIDE);
		if ( Combo.ShowCount == 1 ){
			hComboSubPaneFocus = Combo.base[Combo.show[si].baseNo].hWnd;
		}else{
			if ( Combo.base[Combo.show[si].baseNo].hWnd == hComboSubPaneFocus ){
				hComboSubPaneFocus = Combo.base[baseindex].hWnd;
			}
		}
		Combo.show[si].baseNo = baseindex;
		// 明後日の位置に表示させて、SortComboWindows による位置移動のときに子ウィンドウ(ツリー)を正しく描画させる
//		ShowWindow(Combo.base[baseindex].hWnd, SW_SHOWNORMAL);
		SetWindowPos(Combo.base[baseindex].hWnd, NULL, -10, -10, 0, 0, SWP_SHOWWINDOW);
		SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
		SortComboWindows(SORTWIN_LAYOUTPAIN);
	}
	SetFocus(Combo.base[baseindex].hWnd);
	return SENDCOMBO_OK;
}

void KCW_dockCommand(WORD id, LPARAM lParam)
{
	TCHAR param[CMDLINESIZE];
	POINT pos;
	PPXDOCK *dock;

	if ( id < 0x100 ){ // dock_menu
		pos = *(POINT *)lParam;
	}else{
		tstrcpy(param, (TCHAR *)lParam);
	}
	ReplyMessage(0);
	dock = ((id & 0xff) == 'B') ? &Combo.Docks.b : &Combo.Docks.t;
	if ( id < 0x100 ){ // dock_menu
		DockModifyMenu(Combo.hWnd, dock, &pos);
	}else{
		int showindex;
		PPC_APPINFO *cinfo = NULL;

		showindex = GetComboShowIndexDefault(hComboFocusWnd);
		if ( showindex >= 0 ){
			cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
		}
		dock->cinfo = cinfo;
		if ( (id >> 8) == dock_drop ){
			if ( (cinfo != NULL) && !TinyCheckCellEdit(cinfo) ){
				DockDropBar(cinfo, dock, param);
			}
		}else{
			DockCommands(Combo.hWnd, dock, id >> 8, param);
		}
	}
	SortComboWindows(SORTWIN_LAYOUTALL);
}

void KCW_ReadyCombo(HWND hFocusTargetWnd)
{
	#ifndef RELEASE
	if ( AddDelCount ){
		ReplyMessage(0);
		XMessage(hFocusTargetWnd, NULL, XM_DbgDIA, T("KCW_ReadyCombo - nested !"));
	}
	#endif

	if ( ComboPaneLayout != NULL ){ // ペインの並びを再現する
		const TCHAR *panes;
		TCHAR ID[REGEXTIDSIZE];
		COMBOPANES *OrgShowItem;

		OrgShowItem = PPcHeapAlloc(sizeof(COMBOPANES) * Combo.ShowCount);
		if ( OrgShowItem != NULL ){
			int i, j;

			memcpy(OrgShowItem, Combo.show, sizeof(COMBOPANES) * Combo.ShowCount);
			ID[0] = 'C'; // Foucs ID
			panes = ComboPaneLayout + GetPPcSubID(ComboPaneLayout, ID + 1); // 右 ID

			if ( hFocusTargetWnd == NULL ){
				HWND hTargetWnd = GetHwndFromIDCombo(ID);
				if ( hTargetWnd != NULL ){
					hFocusTargetWnd = hTargetWnd;
				}
			}
			panes += GetPPcSubID(panes, ID + 1); //右
			// 右窓を取得
			if ( ID[1] != '?' ){
				HWND hRWnd = GetHwndFromIDCombo(ID);
				if ( hRWnd != NULL ){
					hComboSubPaneFocus = hRWnd;
					ChangeReason = T("ReadyCombo");
				}
			}

			// 新しい並びを初期化
			for ( i = 0 ; i < Combo.ShowCount ; i++ ) Combo.show[i].baseNo = -1;
			while ( *panes != '\0' ){
				const TCHAR *panesparam;

				while ( (*panes != '\0') && !Isupper(*panes) ) panes++;
				panesparam = panes + GetPPcSubID(panes, ID + 1);
				while ( (*panesparam != '\0') && !Isalnum(*panesparam) ){
					panesparam++;
				}

				if ( !Isdigit(*panesparam) ){ // 表示位置指定がなければ無視
					panes = panesparam;
				}else{
					int showindex;

					showindex = 0;
					panes = panesparam;
					while ( Isdigit(*panes) ){
						showindex = showindex * 10 + (*panes - '0');
						panes++;
					}
					if ( showindex < Combo.ShowCount ){ // 該当IDを検索し、表示設定する
						for ( i = 0 ; i < Combo.BaseCount ; i++ ){
							PPC_APPINFO *cinfo;

							cinfo = Combo.base[i].cinfo;
							if ( cinfo == NULL ) continue;
							if ( tstricmp(cinfo->RegSubCID, ID) == 0 ){
								Combo.show[showindex].baseNo = i;
								ShowWindow(cinfo->info.hWnd, SW_SHOWNA);

								if ( Combo.show[(showindex < Combo.Tabs) ? showindex : 0].tab.groupcount > 0 ){
									int tabindex;
									int li;
									COMBOTABINFO *tabs;

									tabs = &Combo.show[(showindex < Combo.Tabs) ? showindex : 0].tab;
									tabindex = GetTabItemIndex(cinfo->info.hWnd, showindex);
									if ( tabindex < 0 ){
										for ( li = 0 ; li < tabs->groupcount; li++ ){
											tabs->hWnd = tabs->group[li].hWnd;
											tabindex = GetTabItemIndex(cinfo->info.hWnd, showindex);
											if ( tabindex >= 0 ){
												Combo.show[showindex].tab.group[li].hCurWnd = cinfo->info.hWnd;

												if ( tabs->hSelecterWnd != NULL ){
													TabCtrl_SetCurSel(tabs->hSelecterWnd, li);
												}
												break;
											}
										}
									}else if ( tabs->hSelecterWnd != NULL ){
										for ( li = 0 ; li < tabs->groupcount; li++ ){
											if ( tabs->hWnd == tabs->group[li].hWnd ){
												Combo.show[showindex].tab.group[li].hCurWnd = cinfo->info.hWnd;

												TabCtrl_SetCurSel(tabs->hSelecterWnd, li);
												break;
											}
										}
									}
									if ( tabindex >= 0 ){
										TabCtrl_SetCurSel(tabs->hWnd, tabindex);
									}
								}else if ( Combo.Tabs >= 2 ){
									SelectTabByWindow(cinfo->info.hWnd, showindex);
								}

								// 表示したので、元の並びから消去
								for ( j = 0 ; j < Combo.ShowCount ; j++ ){
									if ( OrgShowItem[j].baseNo == i ){
										OrgShowItem[j].baseNo = -1;
										// break; // OrgShowItem[x].baseNo が同じ事があるので、break しない
									}
								}
								break;
							}
						}
					}
				}
			}

			// 割り当てなかったペインの調整
			for ( i = 0 ; i < Combo.ShowCount ; i++ ){
				if ( Combo.show[i].baseNo >= 0 ) continue;
				if ( Combo.Tabs > 1 ){
					HWND hTabWnd;
					TC_ITEM tie;
					int tabindex;
					int tabcount;

					hTabWnd = Combo.show[(i < Combo.Tabs) ? i : 0].tab.hWnd;
					tabcount = TabCtrl_GetItemCount(hTabWnd);
					for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
						tie.mask = TCIF_PARAM;
						if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) != FALSE ){
							int bno = GetComboBaseIndex((HWND)tie.lParam);

							if ( bno >= 0 ){
								if ( GetComboShowIndex((HWND)tie.lParam) < 0 ){
									Combo.show[i].baseNo = bno;
									TabCtrl_SetCurSel(hTabWnd, tabindex);
									ShowWindow((HWND)tie.lParam, SW_SHOWNA);

									// 表示したので、元の並びから消去
									for ( j = 0 ; j < Combo.ShowCount ; j++ ){
										if ( OrgShowItem[j].baseNo == bno ){
											OrgShowItem[j].baseNo = -1;
											break;
										}
									}
									break;
								}
							}else{ // 不明タブを削除
								TabCtrl_DeleteItem(hTabWnd, tabindex);
							}
						}
					}
					if ( Combo.show[i].baseNo >= 0 ) continue;
				}

				for ( j = 0 ; j < Combo.ShowCount ; j++ ){
					if ( OrgShowItem[j].baseNo != -1 ){

						Combo.show[i].baseNo = OrgShowItem[j].baseNo;
						OrgShowItem[j].baseNo = -1;
						break;
					}
				}
			}

			for ( i = 0 ; i < Combo.ShowCount ; i++ ){
				if ( OrgShowItem[i].baseNo != -1 ){
					ShowWindow(Combo.base[OrgShowItem[i].baseNo].hWnd, SW_HIDE);
					break;
				}
			}

			// 欠番があったら詰める
			j = 0;
			for ( i = 0 ; i < Combo.ShowCount ; i++ ){
				if ( Combo.show[i].baseNo == -1 ) continue;
				Combo.show[j++].baseNo = Combo.show[i].baseNo;
			}
			Combo.ShowCount = j;
			// 窓枠大きさの再取得
			for ( i = 0 ; i < Combo.ShowCount ; i++ ){
				TCHAR buf[10];

				thprintf(buf, TSIZEOF(buf), T("%s%d"), ComboID, i + 1);
				GetCustTable(Str_WinPos, buf, &Combo.show[i].box, sizeof(RECT));
			}

			PPcHeapFree(OrgShowItem);
		}

		CheckRightFocus();
	//	SortComboWindows(SORTWIN_LAYOUTPAIN);

		ProcHeapFree(ComboPaneLayout);
		ComboPaneLayout = NULL;
	}

	if ( hFocusTargetWnd == NULL ) hFocusTargetWnd = hComboFocusWnd;
						// hFocusTargetWnd が ComboWindow 内にない場合
	if ( GetComboBaseIndex(hFocusTargetWnd) < 0 ){
		hFocusTargetWnd = Combo.base[Combo.show[0].baseNo].hWnd;
	}

	if ( Combo.Tabs ){ // hFocusTargetWnd が非表示のときの処理
		HWND hTmpRightWnd = hComboSubPaneFocus;

		if ( GetComboShowIndex(hFocusTargetWnd) < 0 ){
			SelectHidePane(GetComboShowIndex(hComboFocusWnd), hFocusTargetWnd);
		}

		KCW_FocusFix(Combo.hWnd, hFocusTargetWnd);
		if ( hComboSubPaneFocus != hTmpRightWnd ){
			hComboSubPaneFocus = hTmpRightWnd;
		}
	}

	{ // 非表示窓を hide
		int i;

		for ( i = 0 ; i < Combo.BaseCount ; i++ ){
			if ( GetComboShowIndex(Combo.base[i].hWnd) < 0 ){
				ShowWindow(Combo.base[i].hWnd, SW_HIDE);
			}
		}
	}

	SetFocus(hFocusTargetWnd);

	SortComboWindows(SORTWIN_LAYOUTALL);

	// ↓フォーカス設定に失敗したとき、強制設定
	if ( hFocusTargetWnd != hComboFocusWnd ){
		KCW_FocusFix(hFocusTargetWnd, hFocusTargetWnd);
		InvalidateRect(Combo.hWnd, NULL, FALSE);
	}

	CheckRightFocus();

	// 共用タブの時、グループタブの位置をアクティブペインの位置に反映
	if ( Combo.Tabs && !(X_combos[0] & CMBS_TABSEPARATE) && (Combo.ShowCount > 1) ){
		int showindex = GetComboShowIndex(hComboFocusWnd);
		if ( showindex >= 0 ){
			COMBOTABINFO *tabinfo;
			tabinfo = &Combo.show[showindex].tab;
		 	if ( (tabinfo->groupcount > 0) && (tabinfo->hSelecterWnd != NULL) ){
				int i;
				for ( i = 0 ; i < tabinfo->groupcount; i++ ){
					int tabindex;
					tabindex = SearchTabParam(tabinfo->group[i].hWnd, (LPARAM)hComboFocusWnd);
					if ( tabindex >= 0 ){
						tabinfo->group[i].hCurWnd = hComboFocusWnd;
						i = SearchTabParam(tabinfo->hSelecterWnd, (LPARAM)tabinfo->group[i].hWnd);
						if ( i >= 0 ) TabCtrl_SetCurSel(tabinfo->hSelecterWnd, i);
						break;
					}
				}
			}
		}
	}

	CheckComboTable(T("KCW_ReadyCombo3"));
	ComboInit = 0;
}

void WmComboPosChanged(HWND hWnd)
{
	WINDOWPLACEMENT wp;

	wp.length = sizeof(wp);
	GetWindowPlacement(hWnd, &wp); //※wp.rcNormalPosition の座標はタスクバーの考慮無
	if ( wp.showCmd == SW_SHOWNORMAL ){
		PPC_APPINFO *cinfo;

		if ( Combo.ShowCount > 0 ){
			cinfo = Combo.base[Combo.show[0].baseNo].cinfo;
		}else{
			cinfo = NULL;
		}
		if ( (cinfo == NULL) || (cinfo->bg.X_WallpaperType < 10) ){
			GetWindowRect(hWnd, &Combo.WinPos.pos);
		}else{ // 背景画像の再描画チェック
			GetWindowRect(hWnd, &wp.rcNormalPosition);
			if ( (wp.rcNormalPosition.left != Combo.WinPos.pos.left) ||
				 (wp.rcNormalPosition.top != Combo.WinPos.pos.top) ){
				int showindex;

				for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){

					cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
					if ( (cinfo != NULL) && (cinfo->bg.X_WallpaperType >= 10) ){
						InvalidateRect(cinfo->info.hWnd, NULL, TRUE);
					}
				}
			}
			Combo.WinPos.pos = wp.rcNormalPosition;
		}
	}else if ( (wp.showCmd == SW_SHOWMINIMIZED) && (X_tray & X_tray_Combo) ){
		// ※最小化のときと、次の隠したときと２回続けて呼ばれる
		PostMessage(hWnd, WM_PPXCOMMAND, K_HIDE, 0);
		return;
	}else if ( wp.showCmd != SW_SHOWMAXIMIZED ){
		return; // SW_SHOWMINIMIZED / SW_HIDE ならここで終了
	}
	ClientPos.x = ClientPos.y = 0;
	ClientToScreen(hWnd, &ClientPos);
	GetClientRect(hWnd, &wp.rcNormalPosition);

	// 自動縦・横配置変更
	if ( (TouchMode & TOUCH_AUTOPAIRDIST) &&
		 (wp.showCmd == SW_SHOWMAXIMIZED) &&
		 (Combo.ShowCount >= 2) ){
		if ( wp.rcNormalPosition.right > wp.rcNormalPosition.bottom ){ // 横
			if ( X_combos[0] & CMBS_VPANE ){
				resetflag(X_combos[0], CMBS_VPANE);
				Combo.WndSize.cx = -1;
			}
		}else{ // 縦
			if ( !(X_combos[0] & CMBS_VPANE) ){
				setflag(X_combos[0], CMBS_VPANE);
				InvalidateRect(hWnd, NULL, TRUE);
				Combo.WndSize.cx = -1;
			}
		}
	}

	if ( (Combo.WndSize.cx == wp.rcNormalPosition.right) &&
		 (Combo.WndSize.cy == wp.rcNormalPosition.bottom) ){
		if ( Combo.WinPos.show != (BYTE)wp.showCmd ){
			Combo.WinPos.show = (BYTE)wp.showCmd;
			InvalidateRect(hWnd, NULL, TRUE); // ウィンドウ状態が変化したので全更新
		}
		return;
	}
	Combo.WinPos.show = (BYTE)wp.showCmd;

	// サイズ変更されたので調整する
	Combo.WndSize.cx = wp.rcNormalPosition.right;
	Combo.WndSize.cy = wp.rcNormalPosition.bottom;

	if ( Combo.Panes.resizewidth == -2 ){
		Combo.Panes.resizewidth = -1;
		SortComboWindows(SORTWIN_LAYOUTALL);
	}else{
		SortComboWindows(SORTWIN_RESIZE);
	}
	InvalidateRect(hWnd, NULL, TRUE); // 大きさが変化したので全更新
}

void PostZxxPPc(WPARAM wParam, LPARAM lParam)
{
	int i;

	for ( i = 0 ; i < Combo.BaseCount ; i++ ){
		if ( (Combo.base[i].cinfo == NULL) || (Combo.base[i].cinfo->RegSubIDNo < 0) ){
			continue; // Zxx でない
		}
		PostMessage(Combo.base[i].hWnd, WM_PPXCOMMAND, wParam, lParam);
	}
}

LRESULT WmComboCommand(HWND hComboWnd, WPARAM wParam, LPARAM lParam)
{
	DEBUGLOGF("WmComboCommand %4x", wParam);

	switch ( LOWORD(wParam) ){
		// Zxx に中継
		case K_Lvfs:
		case K_Fvfs:
		case K_Scust:
		case K_FREEDRIVEUSE:
		case K_ENDCOPY:
			PostZxxPPc(wParam, lParam);
			break;

		case K_Lcust: // lParam は 0 / hWnd(hComboFocusWndからの通知) / Tick(PPcust)
			if ( (lParam == 0) || ((HWND)lParam == hComboFocusWnd) ){
				HWND hNowWnd = hComboFocusWnd;

				ComboCust(TRUE);
				LoadWallpaper(NULL, hComboWnd, ComboID);

				hComboFocusWnd = NULL;
				KCW_FocusFix(hComboWnd, hNowWnd);

				SortComboWindows(SORTWIN_LAYOUTALL);
			}else{ // PPcust からの送信
				if ( lParam != Combo.CustTick ){
					Combo.CustTick = lParam;
					PostZxxPPc(wParam, lParam);
				}
			}
			break;

		// Combo
		case KCW_capture:
		case KCW_entry:
			return KCW_EntryCombo((HWND)lParam, (DWORD)wParam);

		case KCW_ready:
			KCW_ReadyCombo((HWND)lParam);
			break;

		case KCW_focus:
			DEBUGLOGF("WmComboCommand ↑ KCW_focus", 0);
			KCW_FocusFix(hComboWnd, (HWND)lParam);
			DEBUGLOGF("WmComboCommand KCW_focus end", 0);
			break;

		case KCW_size: {
			int showindex;
			RECT box, *savedbox;

			showindex = GetComboShowIndex((HWND)lParam);
			if ( showindex < 0 ) break;
			DEBUGLOGF("WmComboCommand ↑ KCW_size", 0);
			GetWindowRect((HWND)lParam, &box);
			box.left -= ClientPos.x;
			box.right -= ClientPos.x;
			box.top -= ClientPos.y;
			box.bottom -= ClientPos.y;
			savedbox = &Combo.show[showindex].box;
			if ( EqualRect(savedbox, &box) == FALSE ){
				DEBUGLOGF("WmComboCommand KCW_size - SortComboWindows", 0);
				*savedbox = box;
				if ( ComboInit == 0 ) SortComboWindows(showindex);
			}
			DEBUGLOGF("WmComboCommand KCW_size end", 0);
			break;
		}

		case KCW_reqsize:
			if ( lParam == 0 ){
				EnablePaneSizeChange = HIWORD(wParam);
				break;
			}
			RequestPairRate((SCW_REQSIZE *)lParam);
			break;

		case KCW_setpath:
			SetTabInfo(-1, (HWND)lParam);
			break;

		case KCW_setforeground:
			SetFocus((HWND)lParam);
			if ( IsWindowVisible(hComboWnd) == FALSE ){
				ForceSetForegroundWindow(hComboWnd);
				if ( IsWindowVisible(hComboWnd) == FALSE ) ShowWindow(hComboWnd, SW_SHOW);
			}else{
				ShowWindow(hComboWnd, SW_SHOW);
				ForceSetForegroundWindow(hComboWnd);
			}
			PPxCommonCommand(hComboWnd, 0, K_WTOP);
			break;

		case KCW_drawinfo:
			if ( Combo.Height.InfoLine != 0 ){
				RECT box;

				box.left	= 0;
				box.top		= InfoTop;
				box.right	= Combo.WndSize.cx;
				box.bottom	= InfoBottom;
				InvalidateRect(hComboWnd, &box, FALSE);
			}
			DocksInfoRepaint(&Combo.Docks);
			break;

		case KCW_drawstatus:
			DocksStatusRepaint(&Combo.Docks);
			break;

		case KCW_nextppc:
			return KCW_combonextppc(hComboWnd, (HWND)lParam, HISHORTINT(wParam));

		case KCW_getpath: {
			int baseindex;
			PPC_APPINFO *cinfo;

			if ( Combo.BaseCount < 2 ) return 0;
			baseindex = GetPairPaneComboBaseIndex(*(HWND *)lParam);
			if ( baseindex < 0 ) return 0;
			cinfo = Combo.base[baseindex].cinfo;
			if ( cinfo == NULL ) return 0;
			if ( cinfo->e.Dtype.mode != VFSDT_LFILE ){
				tstrcpy((TCHAR *)lParam, cinfo->RealPath);
			}else if ( cinfo->UseArcPathMask == ARCPATHMASK_OFF ){
				tstrcpy((TCHAR *)lParam, cinfo->path);
			}else{
				VFSFullPath((TCHAR *)lParam, cinfo->ArcPathMask, cinfo->path);
			}
			return SENDCOMBO_OK;
		}

		case KCW_getpairwnd: {
			int baseindex;

			if ( Combo.BaseCount < 2 ) return (LRESULT)NULL;
			baseindex = GetPairPaneComboBaseIndex((HWND)lParam);
			if ( baseindex < 0 ) return (LRESULT)NULL;
			return (LRESULT)Combo.base[baseindex].hWnd;
		}

		case KCW_swapwnd:
			ComboSwap();
			break;

		case KCW_tree:
			KCW_treeCombo(HIWORD(wParam), (TCHAR *)lParam);
			break;

		case KCW_tabrotate:
			if ( Combo.Tabs ){
				int tabwndindex;

				tabwndindex = GetComboBaseIndex((HWND)lParam);
				if ( tabwndindex < 0 ){
					tabwndindex = GetComboBaseIndex(hComboFocusWnd);
				}
				PaneCommand(HIWORD(wParam) < 0x7fff ?
						T("change h-1") : T("change h+1"), tabwndindex);
			}
			break;

		case KCW_panecommand:
			return (LRESULT)PaneCommand((const TCHAR *)lParam, HISHORTINT(wParam));

		case KCW_eject:
			EjectPane(lParam);
			break;

		// Tree
		case KTN_close:
			KCW_treeCombo(PPCTREE_SYNC, NilStr);
			break;

		case KTN_escape:
			// KTN_focus へ
		case KTN_focus:
			ChangeReason = T("WmComboSetFocus@KTN_focus");
			WmComboSetFocus();
			break;

		case KTN_selected:
			SendMessage(hComboFocusWnd, WM_PPXCOMMAND, KTN_select, lParam);
			if ( ComboTreeMode == PPCTREE_SELECT ){
				CloseLeftArea();
			}
			ChangeReason = T("KTN_selected@1");
			SetFocus(hComboFocusWnd);
			break;

		case KTN_size:
			if ( ((int)lParam + SplitPix) != Combo.LeftAreaWidth ){
				Combo.LeftAreaWidth = (int)lParam + SplitPix;
				SortComboWindows(SORTWIN_LAYOUTALL);
			}
			break;

		// 中継
		case K_EXTRACT:
		case KTN_select:
		case KCW_enteraddress:
		case K_SETPOPMSG:
		case K_SETPOPLINENOLOG: {
			int baseindex;

			baseindex = GetComboBaseIndex(hComboFocusWnd);
			if ( (baseindex >= 0) && (Combo.base[baseindex].cinfo != NULL) ){
				return SendMessage(hComboFocusWnd, WM_PPXCOMMAND, wParam, lParam);
			}
			return 0;
		}

		case K_a | '-': {
			int baseindex;

			baseindex = GetComboBaseIndex((HWND)lParam);
			if ( baseindex >= 0 ){
				ReplyMessage(ERROR_CANCELLED);
				TabMenu(NULL, baseindex, -1, NULL);
			}
			break;
		}

		case K_WINDDOWLOG:
			if ( lParam != 0 ) SetComboReportText((const TCHAR *)lParam);
			// 遅延表示関連の処理
			if ( (HIWORD(wParam) != PPLOG_REPORT) &&
				 (Combo.Report.hWnd != NULL) &&
				!(X_combos[0] & CMBS_DELAYLOGSHOW) ){
				if ( HIWORD(wParam) == PPLOG_SHOWLOG ){ // 強制表示を行う
					DelayLogShowProc(Combo.Report.hWnd,
							WM_TIMER, TIMERID_DELAYLOGSHOW, 0);
					break;
				}
				if ( GetWindowLongPtr(Combo.Report.hWnd, GWL_STYLE) &
						WS_VISIBLE ){
					// ログの遅延表示を開始／画面描画を止める
					SendMessage(Combo.Report.hWnd, WM_SETREDRAW, FALSE, 0);
					SetTimer(Combo.Report.hWnd, TIMERID_DELAYLOGSHOW,
							TIME_DELAYLOGSHOW, DelayLogShowProc);
				}
			}
			break;

		case KCW_addressbar:
			if ( FocusAddressBars(Combo.hAddressWnd, &Combo.Docks) ){
				return SENDCOMBO_OK;
			}
			return 0;

		case KCW_dock:
			KCW_dockCommand(HIWORD(wParam), lParam);
			break;

		case KCW_setmenu:
			SetMenu(Combo.hWnd,
					!(lParam & CMBS_NOMENU) ? ComboDMenu.hMenuBarMenu : NULL);
			break;

		case KCW_getsite: {
			int showindex;

			showindex = GetComboShowIndex((HWND)lParam);
			if ( showindex < 0 ) return 0;
			return showindex ? PPCSITE_RIGHT : PPCSITE_LEFT;
		}

		case KCW_layout:
			SortComboWindows(SORTWIN_LAYOUTALL);
			InvalidateRect(hComboWnd, NULL, TRUE);
			break;

		case KCW_pathfocus: {
			int baseindex;

			for ( baseindex = 0 ; baseindex < Combo.BaseCount ; baseindex++ ){
				PPC_APPINFO *cinfo;

				cinfo = Combo.base[baseindex].cinfo;
				if ( (cinfo != NULL) &&
					 (!tstricmp((TCHAR *)lParam, cinfo->path)) ){
					SetFocus(Combo.base[baseindex].hWnd);
					return SENDCOMBO_OK + 1;
				}
			}
			break;
		}

		case KCW_closealltabs:
		case KCW_closetabs: {
			int first = LOSHORTINT(lParam);
			int last = HISHORTINT(lParam);
			int pane = HIWORD(wParam);
			HWND hTabWnd;

			if ( pane >= Combo.ShowCount ) break;
			hTabWnd = Combo.show[(pane < Combo.Tabs) ? pane : 0].tab.hWnd;

			for ( ; last >= first ; last-- ){
				TC_ITEM tie;

				if ( Combo.hWnd == BADHWND ) break;
				tie.mask = TCIF_PARAM;
				if ( TabCtrl_GetItem(hTabWnd, last, &tie) == FALSE ) break;
				if ( LOWORD(wParam) == KCW_closetabs ){ // 未ロックPPCに限定
					int baseindex = GetComboBaseIndex((HWND)tie.lParam);
					if ( baseindex < 0 ) continue;
					if ( Combo.base[baseindex].cinfo == NULL ) continue;
					if ( IsTrue(Combo.base[baseindex].cinfo->ChdirLock) ){
						continue;
					}
				}
				PostMessage((HWND)tie.lParam, WM_CLOSE, 0, 0);
			}
			break;
		}

		case KCW_showjoblist: {
			DWORD X_jlst[2];

			GetCustData(T("X_jlst"), &X_jlst, sizeof(X_jlst));
			if ( Combo.Joblist.hWnd == NULL ){
				CreateJobArea();
				X_jlst[0] = 3;
			}else{
				PostMessage(Combo.Joblist.hWnd, WM_CLOSE, 0, 0);
				Combo.Joblist.hWnd = NULL;
				if ( Combo.Report.hWnd == NULL ){
					if ( Combo.Height.ReportJob != 0 ){
						Combo.Height.ReportJob = 0;
					}else if ( Combo.hTreeWnd == NULL ){
						Combo.LeftAreaWidth = 0;
					}
				}
				X_jlst[0] = 0;
			}
			SetCustData(T("X_jlst"), &X_jlst, sizeof(X_jlst));
			SortComboWindows(SORTWIN_LAYOUTALL);
			break;
		}

		case KCW_ActivateWnd:
		case KCW_SelectWnd:
			SelectComboWindow(HIWORD(wParam), (HWND)lParam,
					(LOWORD(wParam) == KCW_ActivateWnd));
			break;

		case KC_GETSITEHWND:
			if ( (int)lParam < 0 ){ // KC_GETSITEHWND_BASEWND
				return (LRESULT)Combo.hWnd;
			}
			if ( (int)lParam == KC_GETSITEHWND_CURRENT ){
				return (LRESULT)hComboFocusWnd;
			}
			if ( (int)lParam == KC_GETSITEHWND_LEFT ){
				return (LRESULT)Combo.base[Combo.show[0].baseNo].hWnd;
			}
			if ( (int)lParam == KC_GETSITEHWND_RIGHT ){
				return (LRESULT)hComboSubPaneFocus;
			}
			if ( (int)lParam == KC_GETSITEHWND_PAIR ){
				int baseindex;
				baseindex = GetPairPaneComboBaseIndex(hComboFocusWnd);
				if ( baseindex < 0 ) return (LRESULT)NULL;
				return (LRESULT)Combo.base[baseindex].hWnd;
			}
			if ( (int)lParam >= KC_GETSITEHWND_LEFTENUM ){
				int showindex = (int)lParam - KC_GETSITEHWND_LEFTENUM;
				if ( showindex >= Combo.ShowCount ) return (LRESULT)NULL;
				return (LRESULT)Combo.base[Combo.show[showindex].baseNo].hWnd;
			}
			return (LRESULT)NULL;

		case K_E_TABLET:
			WmComboDpiChanged(hComboWnd, 0, NULL);
			if ( Combo.hTreeWnd != NULL ){
				SendMessage(Combo.hTreeWnd, VTM_CHANGEDDISPDPI, TMAKEWPARAM(0, Combo.FontDPI), 0);
			}
			if ( ShowCloseButton == SCB_DISABLE ){
				ShowCloseButton = SCB_SHOW;
			}
			break;

		case KCW_ppclist:
			return ComboGetPPcList(HIWORD(wParam));

		case KCW_GetIDWnd:
			return ComboGetIDWnd(lParam);

		case KCW_NewGroup: { // 初期化時新規グループ作成
			int pane;

			// 対象ペインを決定
			pane = GetPspPane(HIWORD(wParam)); // pspo->combo.pane
			if ( pane < 0 ){
				pane = (pane == -2) ? Combo.ShowCount : Combo.MainPane;
			}
			if ( pane >= Combo.ShowCount ){ // 新規ペイン
				if ( pane > Combo.ShowCount ) break; // 存在しないペイン
				CreatePane(-1);
				pane = NewTabGroup_First - pane;
			}
			NewTabGroup(pane, (const TCHAR *)lParam);
			SortComboWindows(SORTWIN_RESIZE);
			break;
		}
		default:
			PPxCommonCommand(hComboWnd, 0, LOWORD(wParam));
			break;
	}
	return SENDCOMBO_OK;
}

void DrawPaneArea(DRAWCOMBOSTRUCT *dcs)
{
	if ( Combo.ShowCount == 0 ){ // ペイン無しの時の空間表示
		if ( dcs->hBr == NULL ) dcs->hBr = CreateSolidBrush(C_back);
		FillBox(dcs->ps.hdc, &Combo.Panes.box, dcs->hBr);
		return;
	}
	// ペイン間の仕切り線描画
	if ( !(X_combos[0] & CMBS_VPANE) ){	// 横整列
		int showindex, showmax;

		showmax = (X_combos[0] & CMBS_TABFRAME) ? Combo.ShowCount : Combo.ShowCount - 1;
		for ( showindex = 0 ; showindex < showmax ; showindex++ ){
			RECT drawbox, *box;

			box = &Combo.show[showindex].box;
			drawbox.left = box->right;
			drawbox.right = drawbox.left + SplitPix;
			drawbox.top = box->top;
			drawbox.bottom = box->bottom;
			DrawSeparateLine(dcs->ps.hdc, &drawbox, BF_LEFT | BF_RIGHT | BF_MIDDLE);
		}
	}else{						// 縦整列
		int showindex;

		for ( showindex = 0 ; showindex < Combo.ShowCount - 1 ; showindex++ ){
			RECT drawbox, *box;

			box = &Combo.show[showindex].box;
			drawbox.left = box->left;
			drawbox.right = box->right;
			drawbox.top = box->bottom;
			if ( X_combos[1] & CMBS1_TABBOTTOM ){
				drawbox.top += Combo.show[showindex].tab.height;
			}
			drawbox.bottom = drawbox.top + SplitPix;
			DrawSeparateLine(dcs->ps.hdc, &drawbox, BF_TOP | BF_BOTTOM | BF_MIDDLE);
		}
	}
}

void WmComboPaint(HWND hWnd)
{
	HGDIOBJ hOldFont;
	RECT box;
	PPC_APPINFO *cinfo = NULL;
	DRAWCOMBOSTRUCT dcs;
	RECT drawbox;

	BeginPaint(hWnd, &dcs.ps);
	dcs.hBr = NULL;

	// 標準アドレスバーボタン描画
	if ( X_combos[0] & CMBS_COMMONADDR ){
		box.right = Combo.WndSize.cx;
//		box.left = box.right - Combo.Height.AddrBar; // 未使用
		box.top = AddrTop;
		box.bottom = box.top + Combo.Height.AddrBar;
		DrawAddressButton(dcs.ps.hdc, &box);
	}

	// 情報行描画
	if ( (Combo.Height.InfoLine != 0)  && (dcs.ps.rcPaint.top < InfoBottom) ){
		int showindex;

		box.left = 0;
		box.top = InfoTop;
		box.right = Combo.Panes.box.right;
		box.bottom = InfoBottom;

		showindex = GetComboShowIndexDefault(hComboFocusWnd);
		if ( showindex >= 0 ){
			cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
			if ( (cinfo != NULL) && !TinyCheckCellEdit(cinfo) ){
#ifdef USEDIRECTX
				HDC hBackupDC;

				hBackupDC = dcs.ps.hdc;
				if ( !UseCapture && (StartDcDraw(Combo.DxDraw, &dcs.ps) == DXSTART_DX) ){
					ChangeSizeDxDraw(Combo.DxDraw, C_back);
					PaintInfoLine(cinfo, Combo.DxDraw, &dcs.ps, &box, DISPENTRY_NO_OUTPANE);
					EndDxDraw(Combo.DxDraw);
					dcs.ps.hdc = hBackupDC;
				}else
#endif
				{
#ifndef USEDELAYCURSOR
					if ( cinfo->bg.X_WallpaperType )
#endif
					{
						dcs.hBr = CreateSolidBrush(cinfo->BackColor);
						FillBox(dcs.ps.hdc, &box, dcs.hBr);
					}

					hOldFont = SelectObject(dcs.ps.hdc, Combo.Font.handle);
					SetTextAlign(dcs.ps.hdc, TA_LEFT | TA_TOP | TA_UPDATECP);
					PaintInfoLine(cinfo, DIRECTXARG(NULL) &dcs.ps, &box, DISPENTRY_NO_OUTPANE);
					SetTextAlign(dcs.ps.hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);
					SelectObject(dcs.ps.hdc, hOldFont);
				}
			}
		}
		if ( (showindex < 0) || (cinfo == NULL) ){
			if ( dcs.hBr == NULL ) dcs.hBr = CreateSolidBrush(C_back);
			FillBox(dcs.ps.hdc, &box, dcs.hBr);
		}
	}
	if ( Combo.LeftAreaWidth ){ // ツリーとペインとの間の仕切り線
		drawbox.left = Combo.LeftAreaWidth - SplitPix;
		drawbox.right = Combo.LeftAreaWidth;
		drawbox.top = Combo.Height.TopArea;
		drawbox.bottom = Combo.Report.box.y - SplitPix;
		DrawSeparateLine(dcs.ps.hdc, &drawbox, BF_LEFT | BF_RIGHT | BF_MIDDLE);
	}
	if ( ((Combo.Report.hWnd != NULL) || (Combo.Joblist.hWnd != NULL)) &&
		 (dcs.ps.rcPaint.bottom > (Combo.Report.box.y - SplitPix)) ){ // ペインより下側の仕切り線
		drawbox.top = Combo.Report.box.y - SplitPix;
		drawbox.bottom = Combo.Report.box.y;
		drawbox.left = dcs.ps.rcPaint.left;
		drawbox.right = dcs.ps.rcPaint.right;
		DrawSeparateLine(dcs.ps.hdc, &drawbox, BF_TOP | BF_BOTTOM | BF_MIDDLE);
		if ( Combo.Joblist.hWnd != NULL ){ // ログ・ジョブリスト間の仕切り線
			drawbox.left = Combo.Report.box.x + Combo.Report.box.width;
			drawbox.right = drawbox.left + SplitPix;
			drawbox.top = drawbox.bottom;
			drawbox.bottom = drawbox.top + Combo.Report.box.height;
			DrawSeparateLine(dcs.ps.hdc, &drawbox, BF_LEFT | BF_RIGHT | BF_MIDDLE);
		}
	}

	if ( !(X_combos[0] & CMBS_TABFRAME) ) DrawPaneArea(&dcs);
	if ( dcs.hBr != NULL ) DeleteObject(dcs.hBr);
	EndPaint(hWnd, &dcs.ps);
}

LRESULT CALLBACK ComboProcMain(HWND hComboWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message){
		case WM_NCCREATE:
			if ( (X_dss & DSS_COMCTRL) && (OSver.dwMajorVersion >= 10) ){
				PPxCommonCommand(hComboWnd, 0, K_ENABLE_NC_SCALE);
			}
			return 1;

		case WM_PARENTNOTIFY:
			if ( Combo.hWnd == BADHWND ) break; // 終了動作中
			if ( LOWORD(wParam) == WM_DESTROY ){
				int baseindex = GetComboBaseIndex((HWND)lParam);

				if ( baseindex < 0 ) break;
				if ( AddDelCount ){
					PPC_APPINFO *cinfo;

					XMessage(hComboWnd, NULL, XM_DbgLOG, T("combo destroy - nested %x"), (HWND)lParam);
					cinfo = Combo.base[baseindex].cinfo;
					if ( cinfo != NULL ){
						if ( Combo.Docks.t.cinfo == cinfo ){
							Combo.Docks.t.cinfo = NULL;
							Combo.Docks.b.cinfo = NULL;
						}
						Combo.base[baseindex].cinfo = NULL;
					}

					PostMessage(Combo.hWnd, WM_PPCOMBO_NESTED_DESTROY, wParam, lParam);
					break;
				}

				AddDelCount++;
				DEBUGLOGF("WM_PARENTNOTIFY - WM_DESTROY", 0);
				DestroyedPaneWindow(hComboWnd, baseindex);
				DEBUGLOGF("WM_PARENTNOTIFY - WM_DESTROY end", 0);
				AddDelCount--;
			}
			break;

		case WM_NCLBUTTONUP:
		case WM_NCRBUTTONUP:
		case WM_NCMBUTTONUP:
		case WM_NCXBUTTONUP:

		case WM_NCLBUTTONDBLCLK:
		case WM_NCRBUTTONDBLCLK:
		case WM_NCMBUTTONDBLCLK:
		case WM_NCXBUTTONDBLCLK:
			return ComboNCMouseCommand(hComboWnd, message, wParam, lParam);

		case WM_NCRBUTTONDOWN:
			if ( wParam == HTCAPTION ) ComboDMenu.Sysmenu = TRUE;
			return DefWindowProc(hComboWnd, message, wParam, lParam);

		case WM_LBUTTONDOWN:	return ComboLMouseDown(hComboWnd, lParam);
		case WM_LBUTTONUP:		return ComboLMouseUp(lParam);
		case WM_LBUTTONDBLCLK:	return ComboLMouseDbl(hComboWnd, lParam);

		case WM_RBUTTONUP:		return ComboMouseButton(lParam, 'R');
		case WM_RBUTTONDBLCLK:	return ComboMouseButton(lParam, 'R'+('D'<<8));
		case WM_MBUTTONUP:		return ComboMouseButton(lParam, 'M');
		case WM_MBUTTONDBLCLK:	return ComboMouseButton(lParam, 'M'+('D'<<8));
		case WM_XBUTTONUP:		return ComboMouseButton(lParam, 'X');
		case WM_XBUTTONDBLCLK:	return ComboMouseButton(lParam, 'X'+('D'<<8));

		case WM_CAPTURECHANGED:
			Combo.SizingPane = DMS_NONE;
			break;

		case WM_MOUSEMOVE:		return ComboMouseMove(hComboWnd, lParam);

		case WM_SETFOCUS:
			WmComboSetFocus();
			break;
		case WM_PAINT:				WmComboPaint(hComboWnd); break;
		case WM_WINDOWPOSCHANGED:	WmComboPosChanged(hComboWnd); break;
		case WM_NOTIFY:				return WmComboNotify((NMHDR *)lParam);

		case WM_COPYDATA:	return WmComboCopyData((COPYDATASTRUCT *)lParam);

		case WM_DESTROY:			WmComboDestroy(FALSE);	break;

		case WM_ENDSESSION:
			if ( wParam ){	// TRUE:セッションの終了(WM_DESTROY は通知されない)
				WmComboDestroy(TRUE);
				break;
			}
			break;

		case WM_SYSKEYDOWN:	// フォーカスが指定できなかったときの対策
			if ( lParam & B29 ){
				return DefWindowProc(hComboWnd, message, wParam, lParam);
			}
			WmComboSetFocus();
			break;

		case WM_SYSCOMMAND:
			if ( wParam >= 0xf000 ){
				WORD cmdID;

				cmdID = (WORD)(wParam & 0xfff0);
				if ( (cmdID == SC_KEYMENU) || (cmdID == SC_MOUSEMENU) ){
					if ( cmdID == SC_KEYMENU ){
						ReplyMessage(0); // デッドロック防止
					}
					ComboDMenu.Sysmenu = TRUE;
				}
				return DefWindowProc(hComboWnd, message, wParam, lParam);
			}
			// WM_COMMAND
			// Fall through
		case WM_COMMAND:
			if ( lParam != 0 ){
				if ( (HWND)lParam == Combo.hAddressWnd ){
					if ( HIWORD(wParam) == 13 ) EnterAddressBar(); // Enter
					if ( HIWORD(wParam) == 27 ) SetFocus(hComboFocusWnd); // ESC
					break;
				}
				if ( (HWND)lParam == Combo.ToolBar.hWnd ){
					ComboToolbarCommand(LOWORD(wParam), 0);
					break;
				}
				if ( (HWND)lParam == Combo.Report.hWnd ){
					if ( HIWORD(wParam) == 27 ) SetFocus(hComboFocusWnd); // ESC
					break;
				}
				if ( DocksWmCommand(&Combo.Docks, wParam, lParam) ){
					break;
				}
			}
			if ( HIWORD(wParam) == 0 ){ // メニュー
				return PostMessage(hComboFocusWnd, WM_COMMAND, wParam, lParam);
			}
			break;

		case WM_DEVICECHANGE:
			if ( ( ((UINT)wParam == DBT_DEVICEARRIVAL) ||
				   ((UINT)wParam == DBT_DEVICEREMOVECOMPLETE) ) &&
				 (((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype
						== DBT_DEVTYP_VOLUME) ){
				int baseindex;

				if ( Combo.hTreeWnd != NULL ){
					SendMessage(Combo.hTreeWnd, WM_DEVICECHANGE, wParam, lParam);
				}
				DocksWmDevicechange(&Combo.Docks, wParam, lParam);

				for ( baseindex = 0 ; baseindex < Combo.BaseCount ; baseindex++ ){
					SendMessage(Combo.base[baseindex].hWnd, WM_DEVICECHANGE, wParam, lParam);
				}
			}
			return DefWindowProc(hComboWnd, message, wParam, lParam);

		case WM_CTLCOLOREDIT:
			if ( Combo.Report.change_color ){
				if ( ((HWND)lParam == Combo.Report.hWnd) ||
					 ((HWND)lParam == Combo.hAddressWnd) ||
					 ((HWND)lParam == Combo.Docks.t.hAddrWnd) ||
					 ((HWND)lParam == Combo.Docks.b.hAddrWnd) ){
					SetTextColor((HDC)wParam, CC_log[0]);
					SetBkColor((HDC)wParam, CC_log[1]);
					return (LRESULT)Combo.Report.hBrush;
				}
			}
			return DefWindowProc(hComboWnd, message, wParam, lParam);

		case WM_DRAWITEM:
			if ( wParam == IDW_TABCONTROL ) DrawTab((DRAWITEMSTRUCT *)lParam);
			return 1;

		case WM_INITMENU:
			DynamicMenu_InitMenu(&ComboDMenu, (HMENU)wParam,
					!(X_combos[0] & CMBS_NOMENU));
			break;

		case WM_INITMENUPOPUP: {
			int baseindex;

			baseindex = GetComboBaseIndex(hComboFocusWnd);
			if ( baseindex >= 0 ){
				PPC_APPINFO *cinfo = Combo.base[baseindex].cinfo;

				if ( cinfo != NULL ){
					DynamicMenu_InitPopupMenu(&ComboDMenu, (HMENU)wParam, &cinfo->info);
				}
			}
			break;
		}

		// ● ComboProcNested > 1 のときでも来ることがあるので抜本対策が必要
		case WM_PPCOMBO_NESTED_ENTRY: // KCW_entry で再入になってしまった窓を再登録させる
			XMessage(hComboWnd, NULL, XM_DbgLOG, T("COMBO_NESTED_ENTRY %x %d"), lParam, AddDelCount);
			if ( AddDelCount < 0 ) AddDelCount = 0;
			return KCW_EntryCombo((HWND)lParam, (DWORD)wParam);


		case WM_PPCOMBO_NESTED_DESTROY: { // WM_PARENTNOTIFY で再入になってしまった窓を再登録解除させる
			int baseindex;
			// ●…既に存在しない窓なので特別な処理が必要
			if ( AddDelCount ){
				XMessage(hComboWnd, NULL, XM_DbgLOG, T("COMBO_NESTED_DESTROY - dbl nested %d"), AddDelCount);
				if ( AddDelCount < 0 ) AddDelCount = 0;
			}
			baseindex = GetComboBaseIndex((HWND)lParam);
			if ( baseindex >= 0 ){
				AddDelCount++;
				DestroyedPaneWindow(hComboWnd, baseindex);
				AddDelCount--;
			}
			break;
		}

		case WM_PPCOMBO_PRECLOSE:
			ComboProcMain(hComboWnd, WM_PARENTNOTIFY, WM_DESTROY, lParam);
			break;

		case WM_DPICHANGED:
			WmComboDpiChanged(hComboWnd, wParam, (RECT *)lParam);
			break;

		case WM_SETTINGCHANGE:
			return WmComboSettingChange(hComboWnd, wParam, lParam);

		default:
			if ( message == WM_PPXCOMMAND ){
				return WmComboCommand(hComboWnd, wParam, lParam);
			}else if ( message == WM_TaskbarButtonCreated ){
				PPxCommonExtCommand(K_TBB_INIT, 0);
			}
			if ( X_uxt[0] == UXT_DARK ){
				if ( ((message <= 0x94) && (message >= 0x91)) ||
				//	 (message == WM_THEMECHANGED) ||
					 (message == WM_NCPAINT) || (message == WM_NCACTIVATE) ){
					return DarkMenuWndProc(hComboWnd, &Combo.hMenuTheme, message, wParam, lParam);
				}
			}
			return DefWindowProc(hComboWnd, message, wParam, lParam);
	}
	return 0;
}

// 再入状況を確認するためのデバッグコード
int ComboProcNested = 0;
UINT ComboProcMsg[NestedMsgs];

LRESULT CALLBACK ComboProc(HWND hComboWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;

	if ( ComboProcNested < NestedMsgs ){
		ComboProcMsg[ComboProcNested] = message + (wParam << 16);
	}
	ComboProcNested++;
	result = ComboProcMain(hComboWnd, message, wParam, lParam);
	ComboProcNested--;
	return result;
}
