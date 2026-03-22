/*-----------------------------------------------------------------------------
	Paper Plane cUI			Combo Window TabBar / pane control
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <commctrl.h>
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPCOMBO.H"
#pragma hdrstop

const TCHAR THSPROP[] = T("PcmbTabH");
const TCHAR CloseButtonChar = 'x';

HWND DeletePane(int showindex);

TCHAR tiptext[VFPS]; // チップテキスト受け渡し用

int PPxDownMouseButtonX(TABHOOKSTRUCT *THS, WPARAM wParam, LPARAM lParam)
{
	DWORD button;
	MOUSESTATE *ms = &THS->ms;

	button = wParam & MOUSEBUTTONMASK;

	// 2つ以上ボタンを押しているか？
	if ( !((button > 0) && ((button & (button - 1)) == 0)) ){ //bitが一つのみ?
		PPxCancelMouseButton(ms);
		return MOUSEBUTTON_CANCEL;
	}else{	// 押しているボタンは１つのみ→新規
		int x = GetSystemMetrics(SM_CXDRAG);
		int y = GetSystemMetrics(SM_CYDRAG);

		ms->mode = MOUSEMODE_PUSH;
		if ( GetCapture() == NULL ) SetCapture(THS->hWnd);
		PPxGetMouseButtonDownInfo(ms, THS->hWnd, button, lParam);
		ms->DDRect.left   = ms->PushScreenPoint.x - x;
		ms->DDRect.right  = ms->PushScreenPoint.x + x;
		ms->DDRect.top    = ms->PushScreenPoint.y - y;
		ms->DDRect.bottom = ms->PushScreenPoint.y + y;
		ms->MovedClientPoint = ms->PushClientPoint;
		ms->MovedScreenPoint = ms->PushScreenPoint;
	}
	ms->PushTick = GetTickCount();
	return ms->PushButton;
}

BOOL TabMouseCommand(HWND hWnd, POINT *pos, const TCHAR *click, int index)
{
	TCHAR buf[CMDLINESIZE], *p;

	p = PutShiftCode(buf, GetShiftKey());
	thprintf(p, 200, T("%s_%s"), click, (index >= 0) ? T("TABB") : T("TABS") );
	if ( NO_ERROR == GetCustTable(T("MC_click"), buf, buf, sizeof(buf)) ){
		TC_ITEM tie;

		if ( index < 0 ) index = TabCtrl_GetCurSel(hWnd);
		tie.mask = TCIF_PARAM;
		if ( IsTrue(TabCtrl_GetItem(hWnd, index, &tie)) ){
			SendMessage((HWND)tie.lParam, WM_PPCEXEC,
					(WPARAM)buf, TMAKELPARAM(pos->x, pos->y));
			return TRUE;
		}
	}
	return FALSE;
}

void CreateNewTab(int showindex)
{
	HWND hWnd;
	TCHAR buf[CMDLINESIZE], path[VFPS];

	if ( showindex < 0 ) showindex = 0;

	hWnd = Combo.base[Combo.show[showindex].baseNo].hWnd;
	SetFocus(hWnd);

	path[0] = '\0';
	GetCustTable(Str_others, T("newtab"), path, VFPS);
	if ( path[0] != '\0' ){
		PPC_APPINFO *cinfo;

		cinfo = Combo.base[Combo.show[showindex].baseNo].cinfo;
		PP_ExtractMacro(NULL, (cinfo != NULL) ? &cinfo->info : NULL,
				NULL, path, path, 0);

		thprintf(buf, TSIZEOF(buf), T("-pane:%d \"%s\""), showindex, path);
		CallPPcParam(Combo.hWnd, buf);
		return;
	}

	if ( (X_combos[0] & CMBS_TABEACHITEM) || (Combo.Tabs < Combo.ShowCount) ){
		thprintf(buf, TSIZEOF(buf), T("-pane:%d"), showindex);
		CallPPcParam(Combo.hWnd, buf);
	}else{
		PostMessage(hWnd, WM_PPXCOMMAND, K_raw | K_F11, 0);
	}
}

void CreateNewTabParam(int showindex, int newbaseindex, const TCHAR *param)
{
	TCHAR buf[CMDLINESIZE];

	int newshowindex;

	if ( newbaseindex >= 0 ){
		for ( newshowindex = 0 ; newshowindex < Combo.ShowCount ; newshowindex++ ){
			if ( Combo.show[newshowindex].baseNo == newbaseindex ){
				showindex = newshowindex;
				break;
			}
		}
	}

	if ( SkipSpace(&param) == '\0' ){
		CreateNewTab(showindex);
		return;
	}
	if ( showindex < 0 ) showindex = 0;
	thprintf(buf, TSIZEOF(buf), T("-pane:%d %s"), showindex, param);

	CallPPcParam(Combo.hWnd, buf);
}

void TabDblClickMouse(TABHOOKSTRUCT *THS)
{
	TC_HITTESTINFO th;
	TCHAR click[3];
	int index;

	if ( THS->ms.PushButton <= MOUSEBUTTON_CANCEL ) return;
	th.pt = THS->ms.PushClientPoint;
	index = TabCtrl_HitTest(THS->hWnd, &th);

	click[0] = PPxMouseButtonChar[THS->ms.PushButton];
	click[1] = 'D';
	click[2] = '\0';
	TabMouseCommand(THS->hWnd, &THS->ms.PushClientPoint, click, index);
}

int GetTabShowIndex(HWND hWnd)
{
	int tabpane;

	if ( Combo.Tabs <= 1 ){
		return GetComboShowIndex(hComboFocusWnd);
	}

	for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
		if ( Combo.show[tabpane].tab.hWnd == hWnd ) return tabpane;
	}
	return -1;
}

int GetTabFromPos(POINT *ScreenPos, POINT *ClientPos, int *TargetTab, int *TargetGroup)
{
	HWND hTargetWnd = WindowFromPoint(*ScreenPos);
	int showindex = -1, tabpane;
	TC_HITTESTINFO th;

	for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
		if ( Combo.show[tabpane].tab.hWnd == hTargetWnd ){
			showindex = tabpane;
			break;
		}
	}

	if ( TargetGroup != NULL ){
		COMBOTABINFO *tabinfo;

		*TargetGroup = -1;
		if ( showindex < 0 ){
			for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
				tabinfo = &Combo.show[tabpane].tab;
				if ( tabinfo->groupcount > 0 ){
					int group;
					if ( tabinfo->hSelecterWnd == hTargetWnd ){
						showindex = tabpane;
						*TargetGroup = -2;
						break;
					}
					for ( group = 0; group < tabinfo->groupcount; group++){
						if ( tabinfo->group[group].hWnd == hTargetWnd ){
							showindex = tabpane;
							*TargetGroup = group;
							goto panebreak;
						}
					}
				}
			}
		}
		panebreak: ;
	}

	*TargetTab = showindex;
	if ( showindex < 0 ) return -1;

	th.pt = *ScreenPos;
	ScreenToClient(hTargetWnd, &th.pt);
	if ( ClientPos != NULL ) *ClientPos = th.pt;
	return TabCtrl_HitTest(hTargetWnd, &th);
}

HWND FindGroupByName(COMBOTABINFO *tabs, const TCHAR *name)
{
	int index;
	HWND hTabWnd;
	TCHAR cap[VFPS + 16];

	for ( index = 0; index < tabs->groupcount; index++ ){
		hTabWnd = tabs->group[index].hWnd;
		GetWindowText(hTabWnd, cap, TSIZEOF(cap));
		if ( tstrcmp(cap, name) == 0 ) return hTabWnd;
	}
	return NULL;
}

void Tab_MovePosition(HWND hSrcTabWnd, int SrcTabIndex, int DestShowIndex, int DestTabIndex, int DestGroupIndex)
{
	TC_ITEM tie;
	TCHAR cap[VFPS + 16];
	HWND hDestTabWnd = Combo.show[DestShowIndex].tab.hWnd;
	int srcselect;
	int reqsort = 0;

	if ( DestGroupIndex >= 0 ){
		hDestTabWnd = Combo.show[DestShowIndex].tab.group[DestGroupIndex].hWnd;
	}

	srcselect = TabCtrl_GetCurSel(hSrcTabWnd);
	if ( SrcTabIndex < 0 ) SrcTabIndex = srcselect;
	if ( DestTabIndex < 0 ) DestTabIndex = TabCtrl_GetItemCount(hDestTabWnd);

	if ( (((X_combos[0] & (CMBS_TABSEPARATE | CMBS_TABEACHITEM)) == CMBS_TABSEPARATE) && (Combo.show[DestShowIndex].tab.hWnd != hSrcTabWnd)) ||
		((hDestTabWnd == hSrcTabWnd) && (DestTabIndex == SrcTabIndex)) ){
		return;
	}

	if ( (hDestTabWnd == hSrcTabWnd) && (DestTabIndex == (SrcTabIndex + 1))){
		DestTabIndex++;
	}

	tie.mask = TCIF_TEXT | TCIF_PARAM;
	tie.cchTextMax = TSIZEOF(cap);
	tie.pszText = cap;
	if ( IsTrue(TabCtrl_GetItem(hSrcTabWnd, SrcTabIndex, &tie)) ){
		int SrcShowIndex = GetTabShowIndex(hSrcTabWnd);

		// タブ共用ペイン別タブの場合は、グループ間移動のとき、別ペインも反映が必要
		if ( ((X_combos[0] & (CMBS_TABSEPARATE | CMBS_TABEACHITEM)) == CMBS_TABSEPARATE) &&
			 (hDestTabWnd != hSrcTabWnd) ){ // グループ間移動
			int showindex;
			TCHAR SrcName[VFPS + 16], DstName[VFPS + 16];
			HWND hTabSrcWnd, hTabDstWnd;

			GetWindowText(hSrcTabWnd, SrcName, TSIZEOF(SrcName));
			GetWindowText(hDestTabWnd, DstName, TSIZEOF(DstName));

			for ( showindex = 0; showindex < Combo.Tabs; showindex++ ){
				if ( showindex == DestShowIndex ) continue;
				hTabSrcWnd = FindGroupByName(&Combo.show[showindex].tab, SrcName);
				hTabDstWnd = FindGroupByName(&Combo.show[showindex].tab, DstName);
				if ( (hTabSrcWnd != NULL) && (hTabSrcWnd != hTabDstWnd) ){
					int tabindex;

					tabindex = SearchTabParam(hTabSrcWnd, tie.lParam);
					if ( tabindex >= 0 ){
						TabCtrl_DeleteItem(hTabSrcWnd, tabindex);
					}
					SendMessage(hTabDstWnd, TCM_INSERTITEM, (WPARAM)EC_LAST, (LPARAM)&tie);
				}
			}
		}

		SendMessage(hDestTabWnd, TCM_INSERTITEM, (WPARAM)DestTabIndex, (LPARAM)&tie);
		LastTabInsType = 5000;
		UseTabInsType |= B0;
		if ( SrcTabIndex == srcselect ){ // 現在窓を移動したときの処理
			if ( hDestTabWnd == hSrcTabWnd ){ // ペイン内タブ移動
				TabCtrl_SetCurSel(hSrcTabWnd, DestTabIndex);
			}else{ // ペイン間タブ移動
				int count = TabCtrl_GetItemCount(hSrcTabWnd);
				HWND hTargetWnd = (HWND)tie.lParam;
				BOOL focus = FALSE;

				if ( (srcselect + 1) >= count ){
					srcselect--;
				}
				// ShowWindow の前に削除しないと、窓の整合性チェックに
				// 引っ掛かる
				TabCtrl_DeleteItem(hSrcTabWnd, SrcTabIndex);
				ShowWindow(hTargetWnd, SW_HIDE);

				if ( srcselect >= 0 ){
					if ( TabCtrl_GetItem(hSrcTabWnd, srcselect, &tie) == FALSE ){
						srcselect = count - 2;
						TabCtrl_GetItem(hSrcTabWnd, srcselect, &tie);
					}
					if ( hTargetWnd == hComboSubPaneFocus ){
						hComboSubPaneFocus = (HWND)tie.lParam;
					}
					if ( hTargetWnd == hComboFocusWnd ){
						hComboFocusWnd = (HWND)tie.lParam;
						focus = TRUE;
					}
					Combo.show[SrcShowIndex].baseNo = GetComboBaseIndex((HWND)tie.lParam);
					TabCtrl_SetCurSel(hSrcTabWnd, srcselect);
					ShowWindow((HWND)tie.lParam, SW_SHOWNORMAL);
					reqsort = 1;
					ChangeReason = T("TabDnD");
					if ( focus ) SetFocus(hComboFocusWnd);
				}else{
					reqsort = -1;
				}
			}
		}
		if ( (hDestTabWnd == hSrcTabWnd) && (DestTabIndex <= SrcTabIndex) ){
			SrcTabIndex++;
		}
		InvalidateRect(hSrcTabWnd, NULL, TRUE);
		if ( reqsort == 0 ){
			TabCtrl_DeleteItem(hSrcTabWnd, SrcTabIndex);
		}else{
			if ( reqsort < 0 ){
				COMBOTABINFO *tabinfo;
				int group = -1, groupindex;

				tabinfo = &Combo.show[SrcShowIndex].tab;
				for ( groupindex = 0; groupindex < tabinfo->groupcount; groupindex++ ){
					if ( (group >= 0) && (tabinfo->hWnd == hSrcTabWnd) ){
						break;
					}
					if ( TabCtrl_GetItemCount(tabinfo->group[groupindex].hWnd) > 0 ){
						group = groupindex;
					}
				}
				if ( group < 0 ){
					DeletePane(SrcShowIndex);
				}else{
					SelectGroup(SrcShowIndex, group);
					return;
				}
			}
			SortComboWindows(SORTWIN_LAYOUTPAIN);
		}
	}
}

// タブの DnD 完了処理
void TabUp_DnD_Mouse(TABHOOKSTRUCT *THS)
{
	int targetshow, targettabindex, targetgroup;
	TC_HITTESTINFO th;

	GetMessagePosPoint(th.pt);
	if ( THS->DDinfo.width >= 0 ){ // 幅変更
		int width;

		ScreenToClient(THS->hWnd, &th.pt);
		width = th.pt.x - THS->DDinfo.width;
		if ( width >= 16 ){
			setflag(X_combos[0], CMBS_TABFIXEDWIDTH);
			SetWindowLongPtr(THS->hWnd, GWL_STYLE,
					GetWindowLongPtr(THS->hWnd, GWL_STYLE) | TCS_FIXEDWIDTH);
			SendMessage(THS->hWnd, TCM_SETITEMSIZE, 0, TMAKELPARAM(width, 0));
		}else{
			resetflag(X_combos[0], CMBS_TABFIXEDWIDTH);
			SetWindowLongPtr(THS->hWnd, GWL_STYLE,
					GetWindowLongPtr(THS->hWnd, GWL_STYLE) & ~TCS_FIXEDWIDTH);
		}
		InvalidateRect(THS->hWnd, NULL, TRUE);
		if ( X_combos[0] & CMBS_TABMULTILINE ){
			SortComboWindows(SORTWIN_LAYOUTPAIN);
		}
		return;
	}
	targettabindex = GetTabFromPos(&th.pt, NULL, &targetshow, &targetgroup);
	if ( (THS->DDinfo.showindex >= 0) && (targetshow >= 0) ){ // tab入替
		if ( targetgroup == -1 ){ // タブにドロップ
			Tab_MovePosition(THS->hWnd, THS->DDinfo.tabindex, targetshow, targettabindex, -1);
		}else if ( targetgroup == -2 ){ // グループタブにドロップ
			int groupcount;

			groupcount = Combo.show[targetshow].tab.groupcount;
			if ( groupcount > 0 ){
				if ( (targettabindex < 0) || (targettabindex >= groupcount) ){
					targettabindex = groupcount - 1;
				}
				Tab_MovePosition(THS->hWnd, THS->DDinfo.tabindex,
						targetshow, -1, targettabindex);
			}
		}else if ( targetgroup >= 0 ){ // 非アクティブグループのタブにドロップ
			Tab_MovePosition(THS->hWnd, THS->DDinfo.tabindex,
					targetshow, targettabindex, targetgroup);
		}
	}
}

void TabUpMouse(HWND hWnd, TABHOOKSTRUCT *THS, LPARAM lParam, int button)
{
	TC_HITTESTINFO th;
	TCHAR click[2];
	int index;
	int tabpane;

	if ( button <= MOUSEBUTTON_CANCEL ) return;

	LPARAMtoPOINT(th.pt, lParam);

	if ( THS->DDinfo.showindex != THS_SHOW_NODRAG ){ // タブ D&D
		TabUp_DnD_Mouse(THS);
		return;
	}

	// アクティブグループの切り替え
	for ( tabpane = 0; tabpane < Combo.Tabs; tabpane++ ){
		COMBOTABINFO *tabinfo;

		tabinfo = &Combo.show[tabpane].tab;
		if ( tabinfo->hWnd == hWnd ) break;
		if ( tabinfo->groupcount > 0 ){
			int i;

			for ( i = 0 ; i < tabinfo->groupcount; i++ ){
				if ( hWnd == tabinfo->group[i].hWnd ){
					tabinfo->hWnd = hWnd;
				}
			}
		}
	}

	index = TabCtrl_HitTest(hWnd, &th);

	click[0] = PPxMouseButtonChar[button];
	click[1] = '\0';
	if ( IsTrue(TabMouseCommand(hWnd, &th.pt, click, index)) ) return;

	if ( button == MOUSEBUTTON_M ){
		if ( index >= 0 ){
			TC_ITEM tie;

			tie.mask = TCIF_PARAM;
			if ( IsTrue(TabCtrl_GetItem(hWnd, index, &tie)) ){
				PostMessage((HWND)tie.lParam, WM_CLOSE, 0, 0);
			}
		}else{
			CreateNewTab(GetTabShowIndex(hWnd));
		}
		return;
	}
	if ( button == MOUSEBUTTON_R ){
		POINT pos;
		TC_ITEM tie;
		int baseindex;

		GetMessagePosPoint(pos);
		// Tabが指すIndexを求める
		tie.mask = TCIF_PARAM;
		if ( IsTrue(TabCtrl_GetItem(hWnd, index, &tie)) ){
			baseindex = GetComboBaseIndex((HWND)tie.lParam);
		}else{
			baseindex = -1;	// 空白を右クリック
		}
		TabMenu(hWnd, baseindex, GetTabShowIndex(hWnd), &pos);
		return;
	}

	if ( button == MOUSEBUTTON_L ){
		TC_ITEM tie;
		int baseindex = -1, showindex;

		// Tabが指すIndexを求める
		tie.mask = TCIF_PARAM;
		if ( IsTrue(TabCtrl_GetItem(hWnd, index, &tie)) ){
			baseindex = GetComboBaseIndex((HWND)tie.lParam);
			if ( baseindex < 0 ){ // 不明タブを削除
				TabCtrl_DeleteItem(hWnd, index);
				return;
			}

			if ( !(X_combos[1] & CMBS1_NOTABCLOSEBTN) ){
				RECT box;

				SendMessage(hWnd, TCM_GETITEMRECT, index, (LPARAM)&box);
				if ( th.pt.x >= (box.right - (box.bottom - (box.top + 4)) - 2)){
					PostMessage(Combo.base[baseindex].hWnd, WM_CLOSE, 0, 0);
					return;
				}
			}
		}
		if ( (baseindex >= 0) &&
			 ( (X_combos[0] & (CMBS_TABMULTILINE | CMBS_TABBUTTON)) != CMBS_TABMULTILINE) ){
			SetFocus(Combo.base[baseindex].hWnd);
		}else{
			showindex = GetTabShowIndex(hWnd);
			if ( showindex >= 0 ){
				SetFocus(Combo.base[Combo.show[showindex].baseNo].hWnd);
			}
		}
		return;
	}
}

BOOL TabLDownMouse(TABHOOKSTRUCT *THS, WPARAM wParam, LPARAM lParam)
{
	TC_HITTESTINFO th;
	TC_ITEM tie;
	int index;

	LPARAMtoPOINT(th.pt, lParam);
	index = TabCtrl_HitTest(THS->hWnd, &th);

	// Tabが指すIndexを求める
	tie.mask = TCIF_PARAM;
	if ( IsTrue(TabCtrl_GetItem(THS->hWnd, index, &tie)) ){
		RECT box;

		if ( TabCtrl_GetCurSel(THS->hWnd) == index ){
			// 選択済みのタブであるが、現在窓でなければ、
			// 改めて選択処理をさせて表示ペインに割当てを行う
			if ( (HWND)tie.lParam != hComboFocusWnd ){
				NMHDR nmh;

				nmh.hwndFrom = THS->hWnd;
				SelectChangeTab(&nmh);
			}
		}

		if ( X_combos[1] & CMBS1_NOTABCLOSEBTN ) return FALSE;
		// 閉じるボタンの判別
		SendMessage(THS->hWnd, TCM_GETITEMRECT, index, (LPARAM)&box);
		if ( th.pt.x >= (box.right - (box.bottom - (box.top + 4)) - 2)){
			PPxDownMouseButtonX(THS, wParam, lParam);
			return TRUE; // 閉じるボタン上なら、タブ切り替えさせない
		}
	}
	return FALSE;
}

BOOL TabKeyDown(HWND hWnd, int key)
{
	switch (key){
		case VK_ESCAPE: {
			int showindex;

			showindex = GetTabShowIndex(hWnd);
			if ( !(X_combos[0] & CMBS_TABBUTTON) ){
				int tabi = GetTabItemIndex(Combo.base[Combo.show[showindex].baseNo].hWnd, showindex);
				SendMessage(hWnd, TCM_SETCURSEL, (WPARAM)tabi, 0);
			}

			if ( showindex >= 0 ){
				SetFocus(Combo.base[Combo.show[showindex].baseNo].hWnd);
			}
			return TRUE;
		}
		case VK_F10:
		case VK_APPS: {
			TC_ITEM tie;
			int cursor;
			int baseindex;
			int showindex;

			cursor = (int)SendMessage(hWnd, TCM_GETCURFOCUS, 0, 0);
			if ( cursor >= 0 ){
				RECT box;
				POINT pos;

				SendMessage(hWnd, TCM_GETITEMRECT, cursor, (LPARAM)&box);
				pos.x = box.left;
				pos.y = box.bottom;
				ClientToScreen(hWnd, &pos);
				// Tabが指すIndexを求める
				tie.mask = TCIF_PARAM;
				TabCtrl_GetItem(hWnd, cursor, &tie);
				baseindex = GetComboBaseIndex((HWND)tie.lParam);
				showindex = GetTabShowIndex(hWnd);
				if ( TabMenu(hWnd, baseindex, showindex, &pos) ){
					if ( showindex >= 0 ){
						SetFocus(Combo.base[Combo.show[showindex].baseNo].hWnd);
					}
				}
			}
			return TRUE;
		}

		default:
			if ( !(X_combos[0] & CMBS_TABBUTTON) ) switch (key){
				case VK_SPACE:
				case VK_RETURN: {
					TC_ITEM tie;
					int cursor;

					cursor = (int)SendMessage(hWnd, TCM_GETCURFOCUS, 0, 0);
					if ( cursor >= 0 ){
						tie.mask = TCIF_PARAM;
						TabCtrl_GetItem(hWnd, cursor, &tie);
						SetFocus((HWND)tie.lParam);
					}
					return TRUE;
				}

				case VK_UP:
				case VK_DOWN:
					return TRUE; // 無効に

				case VK_LEFT: {
					int cursor;
					cursor = (int)SendMessage(hWnd, TCM_GETCURFOCUS, 0, 0);
					if ( cursor >= 1 ){
						SendMessage(hWnd, TCM_SETCURSEL, (WPARAM)(cursor -1 ), 0);
					}
					return TRUE;
				}

				case VK_RIGHT: {
					int cursor;
					cursor = (int)SendMessage(hWnd, TCM_GETCURFOCUS, 0, 0);
					if ( cursor >= 0 ){
						SendMessage(hWnd, TCM_SETCURSEL, (WPARAM)(cursor +1 ), 0);
					}
					return TRUE;
				}
			}
	}
	return FALSE;
}

void SetTabSizeCursor(HWND hWnd)
{
	TC_HITTESTINFO th;
	RECT box;
	int index;

	GetCursorPos(&th.pt);
	ScreenToClient(hWnd, &th.pt);
	index = TabCtrl_HitTest(hWnd, &th);
	if ( index >= 0 ){
		int new_close_show = SCB_SHOW;

		TabCtrl_GetItemRect(hWnd, index, &box);
		if ( (box.right - SIZEBAND) < th.pt.x ){
			if ( !(X_combos[1] & CMBS1_TABMODIFYALT) ){
				SetCursor( LoadCursor(NULL, IDC_SIZEWE) );
			}
		}else if ( (box.right - (box.bottom - box.top - 1)) < th.pt.x ){
			new_close_show = SCB_SHOW_HL;
		}
		if ( (ShowCloseButton != SCB_DISABLE) && (ShowCloseButton != new_close_show) ){
			ShowCloseButton = new_close_show;
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}
}

#pragma argsused
VOID CALLBACK HideCloseButtonProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	POINT pos;
	RECT box;
	UnUsedParam(uMsg);UnUsedParam(dwTime);

	GetCursorPos(&pos);
	GetWindowRect(hWnd, &box);
	if ( PtInRect(&box, pos) == FALSE ){
		ShowCloseButton = SCB_DISABLE;
		KillTimer(hWnd, idEvent);
		InvalidateRect(hWnd, NULL, TRUE);
	}
}

void TabMoveMouse(TABHOOKSTRUCT *THS)
{
	int tabindex;
	int showindex;
	int groupindex;
	RECT box;

	if ( !(X_combos[1] & (CMBS1_NOTABCLOSEBTN | CMBS1_NOAUTOHIDEBTN)) && (ShowCloseButton == SCB_DISABLE) ){ // 閉じるボタンの表示
		ShowCloseButton = SCB_SHOW;
		SetTimer(THS->hWnd, TIMERID_HIDECLOSEBUTTON, TIME_HIDECLOSEBUTTON, HideCloseButtonProc);
		InvalidateRect(THS->hWnd, NULL, TRUE);
	}
//	if ( THS->ms.PushButton <= MOUSEBUTTON_CANCEL ) return; // キャンセル状態

	if ( THS->ms.PushButton == MOUSEBUTTON_W ){
		THS->DDinfo.showindex = THS_SHOW_CANCELDRAG;
		SetCursor( LoadCursor(NULL, IDC_ARROW) );
	}

	if ( THS->ms.mode == MOUSEMODE_NONE ) SetTabSizeCursor(THS->hWnd);

	if ( THS->ms.mode != MOUSEMODE_DRAG ) return;

	if ( THS->DDinfo.showindex == THS_SHOW_NODRAG ){ // D&D 開始
		POINT cpos;

		THS->DDinfo.width = -1;
		if ( !(X_combos[1] & CMBS1_TABMODIFYALT) || (GetKeyState(VK_MENU) & B15) ){
			// ここではグループタブをD&Dの対象にしない。
			THS->DDinfo.tabindex = GetTabFromPos(&THS->ms.PushScreenPoint, &cpos, &THS->DDinfo.showindex, NULL);
			if ( THS->DDinfo.tabindex < 0 ){
				THS->DDinfo.showindex = THS_SHOW_CANCELDRAG;
			}else{
				int new_close_show = SCB_SHOW;
				TabCtrl_GetItemRect(Combo.show[THS->DDinfo.showindex].tab.hWnd, THS->DDinfo.tabindex, &box);
				if ( (box.right - SIZEBAND) < cpos.x ){
					THS->DDinfo.width = box.left;
				}else if ( (box.right - (box.bottom - box.top - 1)) < cpos.x ){
					new_close_show = SCB_SHOW_HL;
				}
				if ( (ShowCloseButton != SCB_DISABLE) && (ShowCloseButton != new_close_show) ){
					ShowCloseButton = new_close_show;
					InvalidateRect(THS->hWnd, NULL, TRUE);
				}
			}
		}else{
			THS->DDinfo.showindex = THS_SHOW_CANCELDRAG;
		}
	}
	if ( THS->DDinfo.showindex < 0 ) return;
	tabindex = GetTabFromPos(&THS->ms.MovedScreenPoint, NULL, &showindex, &groupindex);

	if ( (((X_combos[0] & (CMBS_TABSEPARATE | CMBS_TABEACHITEM)) == CMBS_TABSEPARATE) && (showindex != THS->DDinfo.showindex)  ) ||
			((showindex == THS->DDinfo.showindex) && (tabindex == THS->DDinfo.tabindex) && (groupindex == -1) ) ){
		showindex = -1;
	}
	SetCursor(LoadCursor(NULL, (THS->DDinfo.width >= 0) ? IDC_SIZEWE : (showindex < 0 ? IDC_NO : IDC_UPARROW) ) );
}

void EraseTabBkgnd(HWND hWnd, WPARAM wParam)
{
	RECT box;

	GetClientRect(hWnd, &box);
	FillBox((HDC)wParam, &box, hControlBackBrush);
}

void DrawTab(DRAWITEMSTRUCT *dis)
{
	TC_ITEM tie;
	TCHAR buf[CMDLINESIZE];
	int type, select_index, baseindex;
	RECT box, box2;
	COLORREF oldfc = C_AUTO, oldbc = C_AUTO;

	// ※ DrawTab の後に、Windowsがタブの枠を描画している

	tie.mask = TCIF_TEXT | TCIF_PARAM;
	tie.pszText = buf;
	tie.cchTextMax = CMDLINESIZE;
	if ( TabCtrl_GetItem(dis->hwndItem, dis->itemID, &tie) == FALSE ) return;

	if ( ((HWND)tie.lParam == Combo.base[Combo.show[Combo.MainPane].baseNo].hWnd) || // 左
		 ((HWND)tie.lParam == hComboSubPaneFocus) ){ // 右
		type = ( (HWND)tie.lParam == hComboFocusWnd ) ?
				CI_CaptionFocusBack : CI_CaptionPairBack;
	}else if ( (Combo.ShowCount >= 3) &&
		(GetComboShowIndex((HWND)tie.lParam) >= 0) ){ // 現在・反対以外の表示窓
		type = CI_CaptionNormalBack;
	}else{ // それ以外
		type = -1;
	}

	baseindex = GetComboBaseIndex((HWND)tie.lParam);
	if ( dis->itemState & ODS_SELECTED ){
		select_index = ((HWND)tie.lParam == hComboFocusWnd) ? CI_TabFocus : CI_TabSelected;
	}else{
		select_index = CI_TabNoSelected;
	}

	// 文字色
	if ( (baseindex >= 0) && (Combo.base[baseindex].tabtextcolor != C_AUTO) ){
		oldfc = SetTextColor(dis->hDC, Combo.base[baseindex].tabtextcolor);
	}else if ( C_capt[select_index] != C_AUTO ) {
		oldfc = SetTextColor(dis->hDC, C_capt[select_index]);
	}else if ( X_uxt_color >= UXT_MINPRESET ){
		oldfc = SetTextColor(dis->hDC, C_DialogText);
	}

	// 背景色
	if ( (baseindex >= 0) && (Combo.base[baseindex].tabbackcolor != C_AUTO) ){
		HBRUSH hBack;

		hBack = CreateSolidBrush(Combo.base[baseindex].tabbackcolor);
		FillBox(dis->hDC, &dis->rcItem, hBack);
		DeleteObject(hBack);
		oldbc = SetBkColor(dis->hDC, Combo.base[baseindex].tabbackcolor);
	}else{
		HBRUSH hBack;
		COLORREF col = C_AUTO;

		if ( C_capt[select_index + CI_TabBackOffset] != C_AUTO ) {
			col = C_capt[select_index + CI_TabBackOffset];
		}else if ( X_uxt_color >= UXT_MINPRESET ){
			col = ((HWND)tie.lParam == hComboFocusWnd) ? C_FocusBack : C_DialogBack;
		}else if ( select_index == CI_TabFocus ){
			col = GetSysColor(COLOR_3DHIGHLIGHT);
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

	if ( type >= 0 ){ // フォーカスの印
		HBRUSH hBack;

		box2 = dis->rcItem;
		box2.bottom = box2.top + (5 * Combo.FontDPI) / DEFAULT_WIN_DPI;

		hBack = CreateSolidBrush(C_capt[type]);
		FillBox(dis->hDC, &box2, hBack);
		DeleteObject(hBack);
	}

	if ( oldfc != C_AUTO ) SetTextColor(dis->hDC, oldfc);

	if ( ShowCloseButton != SCB_DISABLE ){ // 閉じるボタン
		COLORREF backcolor;
		HBRUSH hBack;

		box.left = box.right - (box.bottom - box.top);
		backcolor = (C_TabCloseBack == C_AUTO) ? GetBkColor(dis->hDC) : C_TabCloseBack;
		if ( ShowCloseButton == SCB_SHOW_HL ) backcolor = GetGrayColorB(backcolor);
		hBack = CreateSolidBrush(backcolor);
		FillBox(dis->hDC, &box, hBack);
		if ( C_TabCloseText != C_AUTO ){
			oldfc = SetTextColor(dis->hDC, C_TabCloseText);
		}else if ( X_uxt_color >= UXT_MINPRESET ){
			oldfc = SetTextColor(dis->hDC, C_GrayText);
		}
		SetBkMode(dis->hDC, TRANSPARENT);
		DrawText(dis->hDC, &CloseButtonChar, 1, &box, DT_CENTER | DT_NOPREFIX);
		SetBkMode(dis->hDC, OPAQUE);
		if ( C_TabCloseText != C_AUTO ) SetTextColor(dis->hDC, oldfc);
		DeleteObject(hBack);
	}
	if ( oldbc != C_AUTO ) SetBkColor(dis->hDC, oldbc);
}

void PaintTab(HWND hWnd)
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
		DrawTab(&dis);
	}
	DeleteObject(hFrameBrush);
	SelectObject(ps.hdc, hOldFont);
	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK TabHookProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	TABHOOKSTRUCT *THS;

	THS = (TABHOOKSTRUCT *)GetProp(hWnd, THSPROP);
	if ( THS == NULL ) return DefWindowProc(hWnd, iMsg, wParam, lParam);
	switch(iMsg){
		case WM_DESTROY: {
			WNDPROC hOldProc;

			hOldProc = THS->hOldProc;
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)hOldProc);
			RemoveProp(hWnd, THSPROP);
			ProcHeapFree(THS);
			return CallWindowProc(hOldProc, hWnd, iMsg, wParam, lParam);
		}
		case WM_NCHITTEST:
			return HTCLIENT;	// 非タブ領域上のマウス操作も取得可能にする

		case WM_PAINT:
			if ( X_uxt_color < UXT_MINMODIFY ) break;
			PaintTab(hWnd);
			return 0;

		case WM_ERASEBKGND:
			if ( UseCCDrawBack != 2 ) break;
			EraseTabBkgnd(hWnd, wParam);
			return 1;
							// マウスのクライアント領域押下 ------
		case WM_LBUTTONDOWN:
			if ( TabLDownMouse(THS, wParam, lParam) ) return 0;
		// WM_RBUTTONDOWN へ
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
			if ( THS->DDinfo.showindex == THS_SHOW_NODRAG ){
				CallWindowProc(THS->hOldProc, hWnd, iMsg, wParam, lParam);
			}
			PPxDownMouseButtonX(THS, wParam, lParam);
			SetTabSizeCursor(hWnd);
			return 0;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_XBUTTONUP:
			if ( THS->DDinfo.showindex == THS_SHOW_NODRAG ){
				 CallWindowProc(THS->hOldProc, hWnd, iMsg, wParam, lParam);
			}
			TabUpMouse(hWnd, THS, lParam, PPxUpMouseButton(&THS->ms, wParam));
			THS->DDinfo.showindex = THS_SHOW_NODRAG;
			return 0;

		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
			PPxDoubleClickMouseButton(&THS->ms, hWnd, wParam, lParam);
			TabDblClickMouse(THS);
			break;

		case WM_MOUSEMOVE:
			PPxMoveMouse(&THS->ms, hWnd, lParam);
			TabMoveMouse(THS);
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

		case WM_KEYDOWN:
			if ( TabKeyDown(hWnd, (int)wParam) ) return 0;
			break;

	}
	return CallWindowProc(THS->hOldProc, hWnd, iMsg, wParam, lParam);
}

HWND CreateComboTabBar(const TCHAR *groupname)
{
	HWND hTabWnd;
	TABHOOKSTRUCT *THS;
	DWORD style = WS_CHILD | WS_VISIBLE | TCS_TOOLTIPS |
			TCS_HOTTRACK | TCS_FOCUSNEVER | CCS_NODIVIDER;
	TCHAR name[VFPS];

	THS = HeapAlloc(hProcessHeap, 0, sizeof(TABHOOKSTRUCT));
	if ( THS == NULL ) return NULL;

	if ( WinColors.ExtraDrawFlags & EDF_DIALOG_BACK ) LoadCCDrawBack();

	if ( X_combos[0] & CMBS_TABFIXEDWIDTH ) setflag(style, TCS_FIXEDWIDTH);
	if ( X_combos[0] & CMBS_TABMULTILINE )  setflag(style, TCS_MULTILINE);
	if ( X_combos[0] & CMBS_TABBUTTON ){ // タブの行位置が変化しないようにする時の設定。但し、見た目が変わる
		setflag(style, TCS_BUTTONS | TCS_FLATBUTTONS);
	}
	if ( (X_combos[0] & CMBS_TABCOLOR) ||
		 !(X_combos[1] & CMBS1_NOTABCLOSEBTN) ||
		 (WinColors.ExtraDrawFlags & EDF_DIALOG_BACK) ){
		setflag(style, TCS_OWNERDRAWFIXED);
	}

	ShowCloseButton = ((X_combos[1] & CMBS1_NOAUTOHIDEBTN) | (TouchMode & (TOUCH_LARGEWIDTH | TOUCH_LARGEHEIGHT | TOUCH_DETECT_PEN | TOUCH_DETECT_TOUCH))) ? SCB_SHOW : SCB_DISABLE; // 隠さないときは常時表示

	if ( (groupname == NULL) || (*groupname == '\0') ){
		groupname = NewTabGroupName(name);
	}

	hTabWnd = CreateWindowEx(0, WC_TABCONTROL, groupname, style,
			-10, -10, 10, 10, Combo.hWnd /* Combo.Panes.hWnd */,
			CHILDWNDID(IDW_TABCONTROL), hInst, NULL);
	if ( hTabWnd == NULL ){
		ProcHeapFree(THS);
		return NULL;
	}

	// ↓TCS_EX_REGISTERDROP は親でDrop処理するため不要
	SendMessage(hTabWnd, TCM_SETEXTENDEDSTYLE, 0, TCS_EX_FLATSEPARATORS);
	SendMessage(hTabWnd, WM_SETFONT, (WPARAM)GetControlFont(Combo.FontDPI, &Combo.cfs), TMAKELPARAM(TRUE, 0));

	THS->hWnd = hTabWnd;
	THS->DDinfo.showindex = THS_SHOW_NODRAG;
	PPxInitMouseButton(&THS->ms);
	SetProp(THS->hWnd, THSPROP, (HANDLE)THS);
	THS->hOldProc = (WNDPROC)
			SetWindowLongPtr(THS->hWnd, GWLP_WNDPROC, (LONG_PTR)TabHookProc);
	return hTabWnd;
}

// ペインにタブを追加（繰り返し用）
void NewPaneTabBar(const TCHAR *groupname, BOOL NoTabAdd)
{
	int item;
	int tabpane;
	HWND hTabWnd;

	hTabWnd = CreateComboTabBar(groupname);
	if ( hTabWnd == NULL ) return;

	tabpane = Combo.Tabs;
	Combo.show[tabpane].tab.hWnd = hTabWnd;
	Combo.show[tabpane].tab.hTipWnd = TabCtrl_GetToolTips(hTabWnd);

	if ( X_combos[0] & CMBS_TABFIXEDWIDTH ){
		DWORD X_twid[2] = {96, 0};

		GetCustData(T("X_twid"), &X_twid, sizeof(X_twid));
		SendMessage(hTabWnd, TCM_SETITEMSIZE, 0, TMAKELPARAM(X_twid[0], X_twid[1]));
	}

	Combo.Tabs++;

	if ( (Combo.BaseCount > 0) && !NoTabAdd ){
		if ( X_combos[0] & CMBS_TABEACHITEM ){
			AddTabInfo(tabpane, Combo.base[Combo.show[tabpane].baseNo].hWnd);
		}else{
			for ( item = 0 ; item < Combo.BaseCount ; item++ ){
				AddTabInfo(tabpane, Combo.base[item].hWnd);
			}
		}
	}
}

// 全てのペインにタブを追加する。
void CreateTabBar(int mode, const TCHAR *groupname, BOOL NoTabAdd)
{

	LoadCommonControls(ICC_TAB_CLASSES);
	Combo.Height.TabBar = Combo.Font.size.cy + 10;

	if ( mode == CREATETAB_APPEND ){
		TCHAR name[VFPS];

		if ( X_combos[0] & CMBS_TABSEPARATE ){
			if ( (groupname == NULL) || (*groupname == '\0') ){
				groupname = NewTabGroupName(name);
			}
			while ( Combo.Tabs < Combo.ShowCount ){
				NewPaneTabBar(groupname, NoTabAdd);
			}
		}else{
			int showindex;

			NewPaneTabBar(groupname, NoTabAdd);
			for ( showindex = 1; showindex < Combo.ShowCount; showindex++ ){
//				if ( showindex == pane ) continue;
				Combo.show[showindex].tab.hWnd = Combo.show[0].tab.hWnd;
				Combo.show[showindex].tab.hTipWnd = Combo.show[0].tab.hTipWnd;
			}
		}
	}else{
		NewPaneTabBar(groupname, NoTabAdd);
	}
}

void SetTabInfoMain(int setinfo, int showindex, HWND hItemWnd, TCHAR *label)
{
	TC_ITEM tie;
	int tabpane, tabmin, tabmax;
	int msg;

	tie.mask = TCIF_TEXT | TCIF_PARAM;
	tie.pszText = label;
	tie.lParam = (LPARAM)hItemWnd;

	msg = (setinfo == SETTABINFO_SET) ? TCM_SETITEM : TCM_INSERTITEM;
	if ( showindex < 0 ){
		tabmin = 0;
		tabmax = Combo.Tabs - 1;
	}else{
		tabmin = tabmax = showindex;
	}

	for ( tabpane = tabmin ; tabpane <= tabmax ; tabpane++ ){
		HWND hTabWnd;
		int tabid;

		TC_ITEM atie;
		int baseindex, abaseindex;
		PPC_APPINFO *cinfo;

		hTabWnd = Combo.show[tabpane].tab.hWnd;
		tabid = GetTabItemIndex(hItemWnd, tabpane);
		if ( setinfo == SETTABINFO_SET ){ // (SetTabInfo)
			if ( tabid < 0 ) continue; // 登録されていない
			LastTabInsType = 8000;
		}else{ // SETTABINFO_ADD(AddTabInfo) / SETTABINFO_ADDENTRY
			// ● 1.2x タブのペイン増加で表示するとき、タブが余分に追加されることがあるため、暫定でチェックしている。
			// 再現方法…タブをペイン毎表示、タブはペイン間共通、で F11/Q を繰り返して、タブバーを作成／廃棄を繰り返しているときに起きる
			if ( tabid >= 0 ) continue; // 既に登録されている

			tabid = Combo_Max_Base;
			if ( (setinfo == SETTABINFO_ADDENTRY) && (ComboInit == 0) ){
				if ( X_combos[0] & CMBS_TABDIRSORTNEW ){ // ディレクトリ順にする
					int atabid;
					TCHAR *newpath;

					baseindex = GetComboBaseIndex(hItemWnd);
					if ( (baseindex >= 0) &&
						 ((cinfo = Combo.base[baseindex].cinfo) != NULL) ){
						newpath = cinfo->path;

						for ( atabid = 0 ;  ; atabid++ ){
							atie.mask = TCIF_PARAM;
							if ( FALSE == TabCtrl_GetItem(hTabWnd, atabid, &atie) ){
								break;
							}
							abaseindex = GetComboBaseIndex((HWND)atie.lParam);
							if ( (abaseindex >= 0) && (Combo.base[abaseindex].cinfo != NULL) ){
								if ( tstrcmp(newpath, Combo.base[abaseindex].cinfo->path) < 0 ){
									tabid = atabid;
									break;
								}
							}
						}
					}
				}else if ( X_combos[0] & CMBS_TABNEXTTABNEW ){
					int sel = TabCtrl_GetCurSel(hTabWnd) + 1;
					if ( tabid > 0 ) tabid = sel;
				}
			}
			LastTabInsType = 9000;
		}

		if ( tabid != 0 ){
			atie.mask = TCIF_PARAM;
			if ( TabCtrl_GetItem(hTabWnd, 0, &atie) ){
				baseindex = GetComboBaseIndex(hItemWnd);
				if ( (baseindex >= 0) &&
						 ((cinfo = Combo.base[baseindex].cinfo) != NULL) ){
					abaseindex = GetComboBaseIndex((HWND)atie.lParam);
					if ( (abaseindex >= 0) && (Combo.base[abaseindex].cinfo != NULL) ){
						if ( Combo.base[abaseindex].cinfo == cinfo ){
							continue;
						}
						if ( tstrcmp(cinfo->RegSubCID, Combo.base[abaseindex].cinfo->RegSubCID) == 0 ){
							TCHAR buf[32];

							if ( tstricmp(cinfo->RegSubCID, T("CZ")) == 0 ){
								continue; // CZ は重複有り
							}
							thprintf(buf, TSIZEOF(buf), T("%s:%s"),
									((setinfo == SETTABINFO_SET) ?
									 T("同tab設定") : T("同tab追加")),
									cinfo->RegSubCID);
							PPxCommonExtCommand(K_SENDREPORT, (WPARAM)buf);
							continue;
						}
					}
				}
			}
		}
		SendMessage(hTabWnd, msg, (WPARAM)tabid, (LPARAM)&tie);
	}
}

void RenameGroupNameOne(COMBOTABINFO *tabs, HWND hTabWnd, TCHAR *name)
{
	SetWindowText(hTabWnd, name);
	if ( tabs->hSelecterWnd != NULL ){
		int tabindex, tabcount;

		tabcount = TabCtrl_GetItemCount(tabs->hSelecterWnd);
		for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
			TC_ITEM tie;

			tie.mask = TCIF_PARAM;
			if ( IsTrue(TabCtrl_GetItem(tabs->hSelecterWnd, tabindex, &tie)) ){
				if ( (HWND)tie.lParam == hTabWnd ){
					tie.mask = TCIF_TEXT;
					tie.pszText = name;
					TabCtrl_SetItem(tabs->hSelecterWnd, tabindex, &tie);
					break;
				}
			}
		}
	}
}

void RenameGroupNameMain(COMBOTABINFO *tabs, HWND hTabWnd, TCHAR *name)
{
	if ( X_combos[0] & CMBS_TABEACHITEM ){
		RenameGroupNameOne(tabs, hTabWnd, name);
	}else{
		TCHAR orgname[MAX_PATH], checkname[MAX_PATH];
		int tabindex;

		GetWindowText(hTabWnd, orgname, TSIZEOF(orgname));
		for ( tabindex = 0; tabindex < Combo.Tabs; tabindex++ ){
			tabs = &Combo.show[tabindex].tab;

			if ( tabs->groupcount <= 1 ){
				RenameGroupNameOne(tabs, tabs->hWnd, name);
			}else{
				int i;

				for ( i = 0 ; i < tabs->groupcount ; i++ ){
					GetWindowText(tabs->group[i].hWnd, checkname, MAX_PATH);
					if ( tstrcmp(orgname, checkname) == 0 ){
						RenameGroupNameOne(tabs, tabs->group[i].hWnd, name);
						break;
					}
				}
			}
		}
	}
}


void RenameGroupName(COMBOTABINFO *tabs)
{
	TCHAR name[MAX_PATH];

	GetWindowText(tabs->hWnd, name, TSIZEOF(name));
	if ( tInput(tabs->hWnd, MES_TENN, name, TSIZEOF(name), PPXH_GENERAL, PPXH_GENERAL_R) > 0 ){
		RenameGroupNameMain(tabs, tabs->hWnd, name);
	}
}

void SetTabInfoData(int setinfo, int showindex, HWND hItemWnd)
{ // AddTabInfo / SetTabInfo 本体
	TCHAR caption[VFPS + 50], label[VFPS + 50], base[64], *path, *p;
	int baseindex;
	PPC_APPINFO *cinfo;
	BOOL lock = FALSE;

	caption[0] = '\0';
	GetWindowText(hItemWnd, caption, TSIZEOF(caption));
	if ( hItemWnd == hComboFocusWnd ) SetWindowText(Combo.hWnd, caption);

	baseindex = GetComboBaseIndex(hItemWnd);
	if ( (baseindex >= 0) && ((cinfo = Combo.base[baseindex].cinfo) != NULL) ){
		lock = cinfo->ChdirLock;
		thprintf(base, TSIZEOF(base), T("[%s]"), cinfo->RegSubCID + 1);

		if ( (cinfo->e.Dtype.mode == VFSDT_LFILE) &&
			 (cinfo->e.Dtype.BasePath[0] != '\0') ){
			path = cinfo->e.Dtype.BasePath;
		}else{
			path = cinfo->path;
		}

		p = VFSFindLastEntry(path + 3);	// 最終エントリを取り出す
		if ( (*p == '\0') || ((*p == '\\') && (*(p + 1) == '\0')) ){
			p = path;
		}
	}else{
		base[0] = '\0';
		cinfo = NULL;
		path = tstrchr(caption, ']');
		if ( (path == NULL) || ((path - caption) > 10) ){
			path = caption;
		}else{
			path++;
			p = tstrchr(caption, '[');
			if ( (p == NULL) || (p >= path) ){
				path = caption;
			}else{
//				memcpy(base, path, (path - p) * sizeof(TCHAR) );
//				base[(path - p)] = '\0';
				*p = '\0';
				path = caption;
			}
		}
		p = VFSFindLastEntry(path);	// 最終エントリを取り出す
		if ( XC_dpmk ){				// マスクが付いているときは除去
			if ( *p == '\\' ){
				*p = '\0';
				p = VFSFindLastEntry(path);
			}
		}
	}
	if ( hItemWnd == hComboFocusWnd ) SetComboAddresBar(path);
	if ( Combo.Tabs == 0 ) return;

	{
		TCHAR *dst;

		dst = label;
		if ( lock ){
			DWORD symbol;

			symbol = X_mcc[MCC_TAB_LOCK1];
			*dst++ = (TCHAR)symbol;
		#ifdef UNICODE
			if ( symbol >= 0x10000 ) *dst++ = (WCHAR)(DWORD)(symbol >> 16);
		#else
			if ( symbol >= 0x100 ) *dst++ = (char)(DWORD)(symbol >> 8);
		#endif
		}
		if ( (Combo.TabCaption.text != NULL) && (cinfo != NULL) ){
			PP_ExtractMacro(NULL, &cinfo->info, NULL, Combo.TabCaption.text, dst, XEO_DISPONLY);
		}else{
			if ( Combo.TabCaption.type == 1 ){
				dst = tstpcpy(dst, base);
			}
			if ( *p != '\\' ){
				tstrcpy(dst, path);
			}else{
				tstrcpy(dst, p + 1);
			}
		}
	}
	if ( (X_combos[1] & (CMBS1_TABFIXLAYOUT | CMBS1_NOTABCLOSEBTN | CMBS1_NOAUTOHIDEBTN)) == CMBS1_NOAUTOHIDEBTN  ){ // タブ幅が可変長の時、閉じるボタン分の場所を確保
		tstrcat(label, T("    ")); // " [x]"
	}
	SetTabInfoMain(setinfo, showindex, hItemWnd, label);
}

// hWnd に該当する Tab item の index を取得
int GetTabItemIndex(HWND hWnd, int tabwndindex)
{
	TC_ITEM tie;
	int tabindex;
	int tabcount;
	HWND hTabWnd;

	hTabWnd = Combo.show[tabwndindex < Combo.Tabs ? tabwndindex : 0].tab.hWnd;
	tabcount = TabCtrl_GetItemCount(hTabWnd);
	for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
		tie.mask = TCIF_PARAM;
		if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ) continue;
		if ( tie.lParam == (LPARAM)hWnd ) return tabindex;
	}
	return -1;
}

int GetTabItemIndex_AndChangeGroup(HWND hWnd, int tabwndindex)
{
	TC_ITEM tie;
	int tabindex;
	int tabcount;
	HWND hTabWnd;
	COMBOTABINFO *tabinfo;

	tabinfo = &Combo.show[(tabwndindex < Combo.Tabs) ? tabwndindex : 0].tab;

	if ( tabinfo->groupcount <= 0 ){
		hTabWnd = tabinfo->hWnd;
		tabcount = TabCtrl_GetItemCount(hTabWnd);
		for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
			tie.mask = TCIF_PARAM;
			if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ) continue;
			if ( tie.lParam == (LPARAM)hWnd ) return tabindex;
		}
	}else{
		int i;

		for ( i = 0 ; i < tabinfo->groupcount; i++ ){
			HWND hTabWnd;
			int tabindex, tabcount;

			hTabWnd = tabinfo->group[i].hWnd;
			tabcount = TabCtrl_GetItemCount(hTabWnd);
			for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
				tie.mask = TCIF_PARAM;
				if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ){
					continue;
				}
				if ( tie.lParam == (LPARAM)hWnd ){
					Combo.show[tabwndindex].tab.hWnd = hTabWnd;
					Combo.show[tabwndindex].tab.hTipWnd = TabCtrl_GetToolTips(hTabWnd);
					return tabindex;
				}
			}
		}
	}
	return -1;
}

enum {
	CMENU_NEWTAB = 1,
	CMENU_NEWPANE,
	CMENU_HIDEPANE,
	CMENU_EJECTTAB,
	CMENU_CLOSETAB,
	CMENU_LOCKTAB,
	CMENU_COLORTAB,
	CMENU_DELETETABCOLOR,
	CMENU_CLOSELEFT,	// CLOSELEFT/CLOSEPANE/CLOSERIGHT は必ずこの順に
	CMENU_CLOSEPANE,
	CMENU_CLOSERIGHT,
	CMENU_SWAPPANE,
	CMENU_SAVETABWIDTH,
	CMENU_KEYSELECT,
	CMENU_NEWGROUP,
	CMENU_SHOWALLGROUP,
	CMENU_SHOWSELECTOR,
	CMENU_RENAMEGROUP,
	CMENU_CLOSEGROUP,
	CMENU_SWAPTAB,
	CMENU_USER_ITEMS = 100, // user 拡張項目(M_tabc)
	CMENU_GROUPLIST = 28000, // group 一覧
	CMENU_GROUPLIST_MAX = 29999,
	CMENU_CLOSEDLIST = 30000, // 閉じたタブ一覧
	CMENU_CLOSEDLIST_MAX = 31999
};

/* 状況毎の項目
// タブボタン上		空欄
// タブの操作
CMENU_NEWTAB		CMENU_NEWTAB			TABN &N 新規タブ
CMENU_LOCKTAB								TABL &L ロック
CMENU_EJECTTAB								TABE &E 独立窓にする
CMENU_SWAPTAB								TABB &B タブを反対窓と入れ替える
CMENU_COLORTAB								TABR &O 色設定
CMENU_DELETETABCOLOR						TABD &D 色を解除
CMENU_SAVETABWIDTH	CMENU_SAVETABWIDTH		TABI &V	タブ幅記憶
// ペインの操作
CMENU_NEWPANE		CMENU_NEWPANE			TABP &P 新規ペイン
CMENU_SWAPPANE		CMENU_SWAPPANE			TABW &W ペインを反対窓と入れ替える
CMENU_KEYSELECT		CMENU_KEYSELECT			TABK &K タブをキーで選択
CMENU_HIDEPANE		CMENU_HIDEPANE			TABH/TACP &H/&A (ペインを隠す、タブがペイン共通の時) / CMENU_CLOSEPANE / CMENU_NEWPANE(新規ペインで表示) ペインを閉じる
CMENU_CLOSELEFT								TACL &F 左を全て閉じる
CMENU_CLOSERIGHT							TACR &R 右を全て閉じる
CMENU_CLOSEDLIST - CMENU_CLOSEDLIST_MAX		TABS &S (AddClosedList)
// グループの操作
CMENU_NEWGROUP								TACG &G 新規グループ
CMENU_RENAMEGROUP							TACM &M グループ名
CMENU_GROUPLIST - CMENU_GROUPLIST_MAX		グループ一覧
CMENU_SHOWALLGROUP							TACU &U 全グループ表示
CMENU_SHOWSELECTOR							TACT &T グループ選択タブ
// ユーザ
M_tabc
--
CMENU_CLOSETAB								TABC &C	タブを閉じる
*/


void SetTabColor(int baseindex)
{
	TCHAR id[16], value[32];
	PPC_APPINFO *cinfo;

	InvalidateRect(Combo.hWnd, NULL, TRUE);

	if ( (cinfo = Combo.base[baseindex].cinfo) != NULL ){
		thprintf(id, TSIZEOF(id), T("%s_tabcolor"),
				(cinfo->RegSubIDNo < 0) ? cinfo->RegID : cinfo->RegSubCID);
		if ( (Combo.base[baseindex].tabbackcolor == C_AUTO) && (Combo.base[baseindex].tabtextcolor == C_AUTO) ){
			DeleteCustTable(T("_Path"), id, 0);
		}else{
			thprintf(value, TSIZEOF(value), T("H%x, H%x"),
					Combo.base[baseindex].tabtextcolor,
					Combo.base[baseindex].tabbackcolor);
			SetCustStringTable(T("_Path"), id, value, 22);
		}
	}
}

void SetTabColorDialog(int baseindex)
{
	CHOOSECOLOR cc;
	DefineWinAPI(BOOL, ChooseColor, (LPCHOOSECOLOR lpcc));
	COLORREF userColor[16];

	HMODULE hComdlg32 = LoadSystemDLL(SYSTEMDLL_COMDLG32);
	if ( hComdlg32 == NULL ) return;

	GETDLLPROCT(hComdlg32, ChooseColor);

	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = Combo.hWnd;
	cc.rgbResult = Combo.base[baseindex].tabbackcolor;
	cc.lpCustColors = userColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if ( DChooseColor(&cc) ){
		if ( !(X_combos[0] & CMBS_TABCOLOR) ){
			setflag(X_combos[0], CMBS_TABCOLOR);
			SaveX_Combos();
		}
		Combo.base[baseindex].tabtextcolor = C_AUTO;
		Combo.base[baseindex].tabbackcolor = cc.rgbResult;
		SetTabColor(baseindex);
	}

	FreeLibrary(hComdlg32);
}

void ClosePanes(HWND hTabWnd, int baseindex, int mode, BOOL closelocked)
{
	TC_ITEM tie;
	int tabindex;
	int tabcount;
	HWND hSWnd;
	int first = 0, last = TabCtrl_GetItemCount(hTabWnd) - 1;
	WPARAM closedata;

	closedata = TMAKEWPARAM(
			closelocked ? KCW_closealltabs : KCW_closetabs,
			GetTabShowIndex(hTabWnd));

	hSWnd = Combo.base[baseindex].hWnd;
	tabcount = last + 1;
	for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
		tie.mask = TCIF_PARAM;
		if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ) continue;
		if ( tie.lParam != (LPARAM)hSWnd ) continue;

		if ( mode < 0 ) last = tabindex - 1;
		if ( mode > 0 ) first = tabindex + 1;
		PostMessage(Combo.hWnd, WM_PPXCOMMAND, closedata, TMAKELPARAM(first, last));
		return;
	}
}

void NewPane(int baseindex, const TCHAR *param)
{
	HWND hPaneWnd;

	if ( baseindex < 0 ){ // ターゲットなし…新規IDで新規ペイン
		CreateNewPane(param);
		return;
	}
	// 既存ターゲット有り…新規ペインに移動

	// 移動したらそのペインが空になる場合は、とりあえず新規IDにする
	if ( (X_combos[0] & (CMBS_TABSEPARATE | CMBS_TABEACHITEM)) == (CMBS_TABSEPARATE | CMBS_TABEACHITEM)){
		int showindex;
		if ( Combo.Tabs == 0 ){
			CreateNewPane(param);
			return;
		}
		showindex = GetComboShowIndexAll(Combo.base[baseindex].hWnd);
		if ( (showindex < 0) ||
			 (TabCtrl_GetItemCount(Combo.show[showindex].tab.hWnd) <= 1) ){
			CreateNewPane(param);
			return;
		}
	}else{
		if ( Combo.ShowCount >= Combo.BaseCount ){
			CreateNewPane(param);
			return;
		}
	}

	// ※ param 未サポート
	hPaneWnd = Combo.base[baseindex].hWnd;
	if ( GetComboShowIndex(hPaneWnd) >= 0 ){ // 現在表示中の時は対象にできない
		CreateNewPane(param);
		return;
	}

//	ChangeReason = T("CMENU_NEWPANE");
	CreateAndInitPane(baseindex);
	SetFocus(hPaneWnd);
}

void SwapPane(int targetpane)
{
	int swapshow, rightpane = GetComboShowIndex(hComboSubPaneFocus);
	COMBOPANES tmpshow;

	swapshow = GetComboShowIndex(hComboFocusWnd);
	if ( swapshow == targetpane ){
		swapshow = rightpane;
		if ( swapshow == targetpane ){
			swapshow = 0;
		}
	}
	if ( (swapshow < 0) || (targetpane < 0) ) return;

	if ( targetpane == rightpane ){
		hComboSubPaneFocus = Combo.base[Combo.show[swapshow].baseNo].hWnd;
	}else if ( swapshow == rightpane ){
		hComboSubPaneFocus = Combo.base[Combo.show[targetpane].baseNo].hWnd;
	}
	ChangeReason = T("SwapPane");

	tmpshow = Combo.show[targetpane];
	Combo.show[targetpane] = Combo.show[swapshow];
	Combo.show[swapshow] = tmpshow;

	InvalidateRect(Combo.hWnd, NULL, TRUE);
	SortComboWindows(SORTWIN_LAYOUTPAIN);
}

void AddClosedList(HMENU hMenu)
{
	TCHAR buf[VFPS + 16];
	int index, id, count = 0;
	HMENU hSubMenu = CreatePopupMenu();

	id = Combo.Closed.LastIndex - 1;
	for ( index = 0; index < Combo.Closed.Items; index++, id-- ){
		if ( id < 0 ) id = Combo.Closed.Items - 1;
		if ( Combo.Closed.list[id].ID[0] != '\0' ){
			TCHAR *tail;

			tail = thprintf(buf, TSIZEOF(buf), T("%s : "), Combo.Closed.list[id].ID + 1);
			GetCustTable(T("_Path"), Combo.Closed.list[id].ID, tail, TSTROFF(VFPS));
			AppendMenuString(hSubMenu, CMENU_CLOSEDLIST + id, buf);
			count++;
		}
	}
	if ( count > 0 ){
		AppendMenu(hMenu, MF_EPOP, (UINT_PTR)hSubMenu, MessageText(MES_TABS));
	}else{
		DestroyMenu(hSubMenu);
	}
}

void DestroyTabGroupSelecter(COMBOTABINFO *tabs)
{
	DestroyWindow(tabs->hSelecterWnd);

	if ( (X_combos[0] & (CMBS_TABSEPARATE | CMBS_TABEACHITEM)) == (CMBS_TABSEPARATE)){
		int showindex;

		for ( showindex = 0; showindex < Combo.ShowCount; showindex++ ){
			Combo.show[showindex].tab.hSelecterWnd = NULL;
		}
	}else{
		tabs->hSelecterWnd = NULL;
	}
}

BOOL TabMenu(HWND hTabWnd, int baseindex, int targetpane, POINT *pos)
{
	HMENU hMenu;
	int menuindex, showindex = -1;
	HWND hPaneWnd = NULL;
	ThSTRUCT thMenuData;
	POINT temppos;
	TCHAR buf[VFPS + 32];
	DWORD exid = CMENU_USER_ITEMS;
	COMBOTABINFO *tabctrl, *panetab;

	if ( baseindex >= Combo.BaseCount ) return FALSE;
	if ( targetpane < 0 ){
		targetpane = GetComboShowIndexAll(Combo.base[baseindex].hWnd);
	}
	if ( targetpane < 0 ){
		tabctrl = panetab = NULL;
	}else{
		panetab = &Combo.show[targetpane].tab;
		tabctrl = &Combo.show[(targetpane < Combo.Tabs) ? targetpane : 0].tab;
		if ( hTabWnd == NULL ) hTabWnd = panetab->hWnd;
	}

	ThInit(&thMenuData);
	hMenu = CreatePopupMenu();

	AppendMenuString(hMenu, CMENU_NEWTAB, MES_TABN);
	if ( baseindex >= 0 ){ // タブ上メニュー
		if ( Combo.base[baseindex].cinfo != NULL ){
			AppendMenuCheckString(hMenu, CMENU_LOCKTAB, MES_TABL,
					Combo.base[baseindex].cinfo->ChdirLock);
			if ( Combo.BaseCount > 1 ){
				AppendMenuString(hMenu, CMENU_EJECTTAB, MES_TABE);
			}
//			AppendMenuString(hMenu, CMENU_SWAPTAB, MES_TABB); // 非アクティブタブを対象にしたときの動作がまだおかしい
		}

		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenuString(hMenu, CMENU_NEWPANE, MES_TABP);
		hPaneWnd = Combo.base[baseindex].hWnd;
		showindex = GetComboShowIndex(hPaneWnd);
		if ( showindex >= 0 ){
			if ( pos == NULL ){
				temppos.x = Combo.show[showindex].box.left;
				temppos.y = Combo.show[showindex].box.top;
				pos = &temppos;
				ClientToScreen(Combo.hWnd, &temppos);
			}
			if ( Combo.ShowCount > 1 ){
				if ( !(X_combos[0] & CMBS_TABEACHITEM) ){
					AppendMenuString(hMenu, CMENU_HIDEPANE, MES_TABH);
				}else{
					AppendMenuString(hMenu, CMENU_CLOSEPANE, MES_TACP);
				}
			}
		}

		if ( Combo.ShowCount >= 2 ){
			AppendMenuString(hMenu, CMENU_SWAPPANE, MES_TABW);
		}

		if ( hTabWnd != NULL ){
			AppendMenuString(hMenu, CMENU_KEYSELECT, MES_TABK);
			AppendMenuString(hMenu, CMENU_COLORTAB, MES_TABR);
			if ( Combo.base[baseindex].tabbackcolor != C_AUTO ){
				AppendMenuString(hMenu, CMENU_DELETETABCOLOR, MES_TABD);
			}

			AppendMenuString(hMenu, CMENU_CLOSELEFT, MES_TACL);
			AppendMenuString(hMenu, CMENU_CLOSERIGHT, MES_TACR);
		}

		PP_AddMenu( (Combo.base[baseindex].cinfo != NULL) ?
				 &Combo.base[baseindex].cinfo->info : NULL,
				Combo.hWnd, hMenu, &exid, T("M_tabc"), &thMenuData);

	}else{ // 空欄上メニュー
		AppendMenuString(hMenu, CMENU_NEWPANE, MES_TABP);
		if ( X_combos[0] & CMBS_TABFIXEDWIDTH ){
			AppendMenuString(hMenu, CMENU_SAVETABWIDTH, MES_TABI);
		}
		if ( hTabWnd != NULL ){
			AppendMenuString(hMenu, CMENU_KEYSELECT, MES_TABK);
		}
		if ( Combo.ShowCount >= 2 ){
			AppendMenuString(hMenu, CMENU_SWAPPANE, MES_TABW);
			AppendMenuString(hMenu, !(X_combos[0] & CMBS_TABEACHITEM) ?
					CMENU_HIDEPANE : CMENU_CLOSEPANE, MES_TACP);
		}
	}
	AddClosedList(hMenu);

	if ( targetpane >= 0 ){ // グループ関係
		if ( tabctrl->groupcount > 0 ){
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		}
		AppendMenuString(hMenu, CMENU_NEWGROUP, MES_TACG);
		if ( tabctrl->groupcount > 0 ){
			int i;

			AppendMenuString(hMenu, CMENU_RENAMEGROUP, MES_TACM);

			for ( i = 0; i < tabctrl->groupcount; i++ ){
				GetWindowText(tabctrl->group[i].hWnd, buf, VFPS);
				AppendMenuCheckString(hMenu, CMENU_GROUPLIST + i, buf,
						panetab->hWnd == tabctrl->group[i].hWnd);
			}
			AppendMenuCheckString(hMenu, CMENU_SHOWALLGROUP,
					MES_TACU, tabctrl->show_all);

			AppendMenuCheckString(hMenu, CMENU_SHOWSELECTOR,
					MES_TACT, (tabctrl->hSelecterWnd != NULL) );

			if ( tabctrl->groupcount > 1 ){
				AppendMenuString(hMenu, CMENU_CLOSEGROUP, MES_TACV);
			}

		}
	}
	if ( baseindex >= 0 ){
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenuString(hMenu, CMENU_CLOSETAB, MES_TABC);
	}

	if ( pos == NULL ){
		if ( Combo.Tabs && (hTabWnd != NULL) ){	// タブがあればタブに表示
			TC_ITEM tie;
			RECT box;
			int i;

			for ( i = 0 ; i < Combo.BaseCount ; i++ ){	// 検索
				tie.mask = TCIF_PARAM;
				if ( TabCtrl_GetItem(hTabWnd, i, &tie) == FALSE ) continue;
				if ( hPaneWnd != (HWND)tie.lParam ) continue;
				// タブの位置を取得し、変換
				TabCtrl_GetItemRect(hTabWnd, i, &box);
				temppos.x = box.left;
				temppos.y = box.bottom;
				pos = &temppos;
				ClientToScreen(hTabWnd, &temppos);
				break;
			}
		}
		if ( pos == NULL ){	// 該当場所がないので、左上に表示
			temppos.x = 0;
			temppos.y = 0;
			pos = &temppos;
			ClientToScreen(Combo.hWnd, &temppos);
		}
	}
	menuindex = TrackPopupMenu(hMenu, TPM_TDEFAULT, pos->x, pos->y, 0, Combo.hWnd, NULL);
	if ( (menuindex >= CMENU_CLOSEDLIST) && (menuindex <= CMENU_CLOSEDLIST_MAX) ){
		TCHAR *ptr;
		tstrcpy(buf, T("-bootid:"));
		GetMenuString(hMenu, menuindex, buf + 8, VFPS + 8, MF_BYCOMMAND);
		ptr = tstrchr(buf, ' ');
		if ( ptr != NULL ) *ptr = '\0';
		CreateNewTabParam(showindex, -1, buf);
	}
	DestroyMenu(hMenu);
	switch (menuindex){
		case CMENU_NEWTAB: // 現在ペインに新規PPc
			CreateNewTab(targetpane);
			break;

		case CMENU_CLOSETAB:
			PostMessage(hPaneWnd, WM_CLOSE, 0, 0);
			break;

		case CMENU_HIDEPANE:
			HidePane((showindex >= 0) ? showindex : targetpane);
			break;

		case CMENU_NEWPANE: // 新規ペインに新規PPc
			NewPane(-1, NULL);
//			NewPane(baseindex, NULL);	// 現在窓を新規ペインに
			break;

		case CMENU_EJECTTAB:
			if ( baseindex < 0 ) break;
			PostMessage(Combo.hWnd, WM_PPXCOMMAND, KCW_eject, (LPARAM)baseindex	);
			break;

		case CMENU_DELETETABCOLOR:
			if ( baseindex < 0 ) break;
			Combo.base[baseindex].tabtextcolor = C_AUTO;
			Combo.base[baseindex].tabbackcolor = C_AUTO;
			SetTabColor(baseindex);
			break;

		case CMENU_COLORTAB:
			if ( baseindex < 0 ) break;
			SetTabColorDialog(baseindex);
			break;

		case CMENU_LOCKTAB:
			if ( baseindex < 0 ) break;
			if ( Combo.base[baseindex].cinfo != NULL ){
				Combo.base[baseindex].cinfo->ChdirLock =
						!Combo.base[baseindex].cinfo->ChdirLock;
				SetTabInfo(-1, Combo.base[baseindex].hWnd); // ロック状態を文字で表現している間、用意
			}
			break;

		case CMENU_CLOSEPANE:
			if ( baseindex < 0 ) baseindex = Combo.show[targetpane].baseNo;
			// CMENU_CLOSELEFT へ
		case CMENU_CLOSELEFT:
		case CMENU_CLOSERIGHT:
			ClosePanes(hTabWnd, baseindex, menuindex - CMENU_CLOSEPANE,
					GetShiftKey() & K_s);
			break;

		case CMENU_SWAPPANE:
			SwapPane(targetpane);
			break;

		case CMENU_SAVETABWIDTH: {
			RECT box;

			TabCtrl_GetItemRect(hTabWnd, 0, &box);
			thprintf(buf, TSIZEOF(buf), T("%d"), box.right - box.left);
			if ( tInput(hTabWnd, MES_TTAB, buf, TSIZEOF(buf), PPXH_NUMBER, PPXH_NUMBER) > 0 ){
				int X_twid[2];
				const TCHAR *p;

				p = buf;
				X_twid[0] = GetNumber(&p);
				if ( X_twid[0] >= 16 ){
					SetWindowLongPtr(hTabWnd, GWL_STYLE,
							GetWindowLongPtr(hTabWnd, GWL_STYLE) | TCS_FIXEDWIDTH);

					X_twid[1] = 0;
					SetCustData(T("X_twid"), &X_twid, sizeof(X_twid));
					SendMessage(hTabWnd, TCM_SETITEMSIZE, 0, TMAKELPARAM(X_twid[0], X_twid[1]));
				}else{
					resetflag(X_combos[0], CMBS_TABFIXEDWIDTH);
					SetWindowLongPtr(hTabWnd, GWL_STYLE,
						GetWindowLongPtr(hTabWnd, GWL_STYLE) & ~TCS_FIXEDWIDTH);
				}
			}
			break;
		}

		case CMENU_KEYSELECT:
			SetFocus(hTabWnd);
			break;

		case CMENU_NEWGROUP:
			if ( targetpane >= 0 ){
			//	panetab->show_all = 1;
				if ( NewTabGroup(targetpane, NULL) == NO_ERROR ){
					SelectGroup(targetpane, Combo.show[targetpane].tab.groupcount - 1);
					SortComboWindows(SORTWIN_LAYOUTPAIN);
					CreateNewTab(targetpane);
					NewTabGroupCare(showindex);
				}
			}
			break;

		case CMENU_SHOWALLGROUP:
			if ( targetpane >= 0 ){
				tabctrl->show_all = ! tabctrl->show_all;
				SortComboWindows(SORTWIN_LAYOUTPAIN);
				InvalidateRect(Combo.hWnd, NULL, TRUE);
			}
			break;

		case CMENU_SHOWSELECTOR:
			if ( targetpane >= 0 ){
				if ( tabctrl->hSelecterWnd == NULL ){
					CreateTabGroupSelector(tabctrl);
				}else{
					DestroyTabGroupSelecter(tabctrl);
				}
				SortComboWindows(SORTWIN_LAYOUTPAIN);
				InvalidateRect(Combo.hWnd, NULL, TRUE);
			}
			break;

		case CMENU_RENAMEGROUP:
			RenameGroupName(tabctrl);
			break;

		case CMENU_CLOSEGROUP:
			if ( targetpane >= 0 ){
				int tabindex, tabcount;

				tabcount = TabCtrl_GetItemCount(tabctrl->hWnd);
				for ( tabindex = 0 ; tabindex < tabcount; tabindex++ ){
					TC_ITEM tie;

					tie.mask = TCIF_PARAM;
					if ( TabCtrl_GetItem(tabctrl->hWnd, tabindex, &tie) == FALSE ){
						break;
					}
					PostMessage((HWND)tie.lParam, WM_CLOSE, 0, 0);
				}
			}
			break;

		case CMENU_SWAPTAB:
			PostMessage(hPaneWnd, WM_PPXCOMMAND, (WPARAM)(K_raw | 'G'), 0);
			break;

		default:
			if ( menuindex >= CMENU_CLOSEDLIST ) break;
			if ( menuindex >= CMENU_GROUPLIST ){
				int tabindex;
				TC_ITEM tie;
				HWND hTabWnd;

				if ( X_combos[1] & CMBS1_SELECTEDGROUP ){
					tabctrl->show_all = 0;
				}
				hTabWnd = panetab->hWnd = tabctrl->group[menuindex - CMENU_GROUPLIST].hWnd;
				if ( tabctrl->hSelecterWnd != NULL ){
					TabCtrl_SetCurSel(tabctrl->hSelecterWnd, menuindex - CMENU_GROUPLIST);
				}
				tabindex = TabCtrl_GetCurSel(hTabWnd);
				if ( tabindex < 0 ) tabindex = 0;
				tie.mask = TCIF_PARAM;
				if ( IsTrue(TabCtrl_GetItem(hTabWnd, tabindex, &tie)) ){
					SelectComboWindow(targetpane, (HWND)tie.lParam, TRUE);
					break;
				}

				SortComboWindows(SORTWIN_LAYOUTPAIN);
				InvalidateRect(Combo.hWnd, NULL, TRUE);
				break;
			}

			if ( menuindex >= CMENU_USER_ITEMS ){
				const TCHAR *command;

				GetMenuDataMacro2(command, &thMenuData, menuindex - CMENU_USER_ITEMS);
				if ( command != NULL ){
					PP_ExtractMacro(Combo.base[baseindex].hWnd,
							(Combo.base[baseindex].cinfo != NULL) ?
							 &Combo.base[baseindex].cinfo->info : NULL,
							NULL, command, NULL, 0);
				}
				break;
			}
			ThFree(&thMenuData);
			return FALSE;
	}
	ThFree(&thMenuData);
	return TRUE;;
}

//============================================================================
// ペイン操作
//============================================================================
// 現在－反対のペインの中身を交換 --------------------------------------------
void ComboSwap(void)
{
	int baseindexL, baseindexR, focusshowindex;
	int showindexL, showindexR;
	int tabL, tabR;
	int tabpane;
	TC_ITEM tieL, tieR;
	TCHAR strL[VFPS + 16], strR[VFPS + 16];
	HWND hNewFocusR;

	if ( Combo.BaseCount < 2 ) return; // １枚しかない
//	CheckComboTable(T("ComboSwap-pre"));

	focusshowindex = GetComboShowIndexDefault(hComboFocusWnd);
	showindexL = 0;										// 左側
	baseindexL = Combo.show[showindexL].baseNo;
	showindexR = GetComboShowIndex(hComboSubPaneFocus);	// 右側
	if ( showindexR > 0 ){
		baseindexR = Combo.show[showindexR].baseNo;
	}else{ // 右側が非表示・左右が同じになった
		if ( showindexR == 0 ) return;
		baseindexR = GetComboBaseIndex(hComboSubPaneFocus);
		if ( baseindexR < 0 ){
			if ( Combo.ShowCount < 2 ){
				baseindexR = 1;
			}else{
				showindexR = 1;
				baseindexR = Combo.show[showindexR].baseNo;
			}
		}
		if ( baseindexR == baseindexL ) baseindexR = baseindexL ? 0 : 1;
	}
							// 表示入れ替え
	Combo.show[showindexL].baseNo = baseindexR;
	hNewFocusR = Combo.base[(showindexL == 0) ? baseindexL : baseindexR].hWnd;

	if ( showindexR >= 0 ){ // 反対ペイン有り
		Combo.show[showindexR].baseNo = baseindexL;
	}else{ // ペインは１つのみ…表示入れ替え
		HWND hNewLeftWnd;

		hNewLeftWnd = Combo.base[baseindexR].hWnd;

		//●この２行は整合性チェック(WM_SETFOCUS)の誤判定回避のときに必要1.29+5
		//	hComboFocusWnd = hNewLeftWnd;
		//	hComboSubPaneFocus = hNewFocusR;

		ShowWindow(Combo.base[baseindexL].hWnd, SW_HIDE);
		ShowWindow(hNewLeftWnd, SW_SHOWNORMAL);
	}

	if ( (showindexR > 0) && (X_combos[0] & CMBS_TABEACHITEM) ){ // タブ入れ替え(各ペイン間で入替え)
		tabL = GetTabItemIndex(Combo.base[baseindexL].hWnd, showindexL);
		tabR = GetTabItemIndex(Combo.base[baseindexR].hWnd, showindexR);
		if ( (tabL >= 0) && (tabR >= 0) ){
			tieL.mask = tieR.mask = TCIF_TEXT | TCIF_PARAM;
			tieL.cchTextMax = tieR.cchTextMax = VFPS + 16;
			tieL.pszText = strL;
			tieR.pszText = strR;
			TabCtrl_GetItem(Combo.show[showindexL].tab.hWnd, tabL, &tieL);
			TabCtrl_GetItem(Combo.show[showindexR].tab.hWnd, tabR, &tieR);
			TabCtrl_SetItem(Combo.show[showindexL].tab.hWnd, tabL, &tieR);
			TabCtrl_SetItem(Combo.show[showindexR].tab.hWnd, tabR, &tieL);
		}
	}else{ // タブ入れ替え(各ペイン毎に入れ替え)
		for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
			tabL = GetTabItemIndex(Combo.base[baseindexL].hWnd, tabpane);
			tabR = GetTabItemIndex(Combo.base[baseindexR].hWnd, tabpane);
			if ( (tabL >= 0) && (tabR >= 0) ){
				tieL.mask = tieR.mask = TCIF_TEXT | TCIF_PARAM;
				tieL.cchTextMax = tieR.cchTextMax = VFPS;
				tieL.pszText = strL;
				tieR.pszText = strR;
				TabCtrl_GetItem(Combo.show[tabpane].tab.hWnd, tabL, &tieL);
				TabCtrl_GetItem(Combo.show[tabpane].tab.hWnd, tabR, &tieR);
				TabCtrl_SetItem(Combo.show[tabpane].tab.hWnd, tabL, &tieR);
				TabCtrl_SetItem(Combo.show[tabpane].tab.hWnd, tabR, &tieL);
			}
		}
	}
	hComboSubPaneFocus = hNewFocusR;
	ChangeReason = T("ComboSwap");
//	CheckComboTable(T("ComboSwap"));

	SortComboWindows(SORTWIN_LAYOUTPAIN);
	if ( focusshowindex >= 0 ){
		SetFocus(Combo.base[Combo.show[focusshowindex].baseNo].hWnd);
	}
}

int SearchTabParam(HWND hTabWnd, LPARAM param)
{
	TC_ITEM tie;
	int tabindex;
	int tabcount;

	tabcount = TabCtrl_GetItemCount(hTabWnd);
	for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
		tie.mask = TCIF_PARAM;
		if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ) continue;
		if ( tie.lParam == param ) return tabindex;
	}
	return -1;
}

void SelectTabByWindow(HWND hTargetWnd, int selectpane)
{
	int tabindex, foundgroup = 0;
	COMBOTABINFO *tabinfo;

	PostMessage(hTargetWnd, WM_PPXCOMMAND, K_raw | K_c | 'L', 0);

	tabinfo = &Combo.show[selectpane].tab;
	tabindex = SearchTabParam(tabinfo->hWnd, (LPARAM)hTargetWnd);
	if ( tabindex < 0 ){
		if ( tabinfo->groupcount > 1 ){
			int i;

			for ( i = 0 ; i < tabinfo->groupcount; i++ ){
				tabindex = SearchTabParam(tabinfo->group[i].hWnd, (LPARAM)hTargetWnd);
				if ( tabindex >= 0 ){
					HWND hOldTabWnd;

					foundgroup = 1;
					tabinfo->group[i].hCurWnd = hTargetWnd;
					hOldTabWnd = tabinfo->hWnd;
					tabinfo->hWnd = tabinfo->group[i].hWnd;
					if ( tabinfo->hSelecterWnd != NULL ){
						TabCtrl_SetCurSel(tabinfo->hSelecterWnd, i);
						if ( !tabinfo->show_all &&
							 (hOldTabWnd != tabinfo->hWnd) ){
							SortComboWindows(SORTWIN_RESIZE);
							PostMessage(Combo.hWnd, WM_PPXCOMMAND, KCW_layout, 0);
						}
					}
					goto found;
				}
			}
		}
		return;
	}else if ( tabinfo->hSelecterWnd != NULL ){
		int i;

		for ( i = 0 ; i < tabinfo->groupcount; i++ ){
			if ( tabinfo->hWnd == tabinfo->group[i].hWnd ){
				foundgroup = 1;

				tabinfo->group[i].hCurWnd = hTargetWnd;
				TabCtrl_SetCurSel(tabinfo->hSelecterWnd, i);

				if ( !tabinfo->show_all ){
					SortComboWindows(SORTWIN_RESIZE);
					PostMessage(Combo.hWnd, WM_PPXCOMMAND, KCW_layout, 0);
				}
				break;
			}
		}
	}
found: ;
	TabCtrl_SetCurSel(tabinfo->hWnd, tabindex);
	if ( !foundgroup && (tabinfo->groupcount > 0) ){
		int i;

		for ( i = 0 ; i < tabinfo->groupcount; i++ ){
			if ( 0 <= SearchTabParam(tabinfo->group[i].hWnd, (LPARAM)hTargetWnd) ){
				tabinfo->group[i].hCurWnd = hTargetWnd;
				break;
			}
		}
	}
}

void SetFocusWithBackupedRightWindow(HWND hNewFocusWnd)
{
	HWND hTmpRFocus = hComboSubPaneFocus;

	SetFocus(hNewFocusWnd);

	// SetFocus により、hComboSubPaneFocus が変化してしまった場合は元に戻す。
	// 但し、hNewFocusWndが新反対窓の場合を除く。
	if ( hNewFocusWnd != hComboSubPaneFocus ){
		ChangeReason = T("SetFocusWithR");
		hComboSubPaneFocus = hTmpRFocus;
	}
}

BOOL USEFASTCALL IsHideTabForMultiShow(void)
{
	int tabpane;

	if ( X_combos[1] & CMBS1_TABSHOWMULTI ){
		if ( Combo.ShowCount > 1 ) return FALSE; // 2ペイン以上なので表示
	}

	if ( !(X_combos[0] & CMBS_TABEACHITEM) ){ // タブ共通
		if ( Combo.ShowCount > X_mpane.limit ) return FALSE; // ペイン数が最大以上なら表示
		if ( Combo.BaseCount <= X_mpane.limit ){
			 return TRUE; // 全タブが最大以下なら非表示
		}
	}

	// ２タブ以上のペインがあれば表示
	for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
		if ( (Combo.show[tabpane].tab.groupcount > 1) ||
			 (TabCtrl_GetItemCount(Combo.show[tabpane].tab.hWnd) >= 2) ){
			return FALSE; // 2tab 以上のペインがある
		}
	}

	return TRUE; // 全ペイン１タブなので非表示
}


void CloseGroup(COMBOTABINFO *tabs, int closegroup, HWND hGroupTab)
{
	if ( closegroup < tabs->groupcount ){
		DestroyWindow(hGroupTab);
		memmove(&tabs->group[closegroup],
				&tabs->group[closegroup + 1],
				sizeof(COMBOTABINFO) * (tabs->groupcount - closegroup - 1) );
		tabs->groupcount--;
		if ( tabs->groupcount == 0 ){
			PPcHeapFree(tabs->group);
			tabs->group = NULL;
			if ( tabs->hSelecterWnd != NULL ){
				DestroyTabGroupSelecter(tabs);
			}
		}

		if ( Combo.Tabs < Combo.ShowCount ){
			int showindex;
			for ( showindex = 0; showindex < Combo.ShowCount; showindex++ ){
				COMBOTABINFO *tabs2;
				tabs2 = &Combo.show[showindex].tab;
				if ( tabs == tabs2 ) continue;

				if ( closegroup < tabs2->groupcount ){
					memmove(&tabs2->group[closegroup],
						&tabs2->group[closegroup + 1],
						sizeof(COMBOTABINFO) * (tabs2->groupcount - closegroup - 1) );
					tabs2->groupcount--;
					if ( tabs2->groupcount == 0 ){
						PPcHeapFree(tabs2->group);
						tabs2->group = NULL;
						if ( tabs2->hSelecterWnd != NULL ){
							tabs2->hSelecterWnd = NULL;
						}
					}
				}
			}
		}
	}

	if ( tabs->hSelecterWnd != NULL ){
		int index = 0;
		TC_ITEM tie;

		for(;;){
			tie.mask = TCIF_PARAM;
			if ( TabCtrl_GetItem(tabs->hSelecterWnd, index, &tie) == FALSE ){
				break;
			}
			if ( (HWND)tie.lParam == hGroupTab ){
				TabCtrl_DeleteItem(tabs->hSelecterWnd, index);
				if ( TabCtrl_GetItemCount(tabs->hSelecterWnd) <= 0 ){
					DestroyTabGroupSelecter(tabs);
				}
				break;
			}
			index++;
		}
	}
}

int DeleteWndTab(HWND hTabWnd, int showindex, HWND hPaneWnd)
{
	COMBOTABINFO *tabinfo;
	int i;

	tabinfo = &Combo.show[showindex].tab;
	for ( i = 0 ; i < tabinfo->groupcount; i++ ){
		int tabcount;
		int tabindex, tabbase;

		if ( hTabWnd == tabinfo->group[i].hWnd ){
			continue; // 削除予定のグループだった
		}

		hTabWnd = tabinfo->group[i].hWnd;
		tabcount = TabCtrl_GetItemCount(hTabWnd);

		if ( tabcount <= 0 ) continue;
		for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
			TC_ITEM tie;

			tie.mask = TCIF_PARAM;
			if ( IsTrue(TabCtrl_GetItem(hTabWnd, tabindex, &tie)) ){
				if ( (HWND)tie.lParam == hPaneWnd ) continue;
				tabbase = GetComboBaseIndex((HWND)tie.lParam);
				if ( (tabbase >= 0) &&
					 (GetComboShowIndex((HWND)tie.lParam) < 0) ){
					int closegroup;
					HWND hGroupTab = NULL;

					// 現在グループを削除
					for ( closegroup = 0 ; closegroup < tabinfo->groupcount; closegroup++ ){
						if ( tabinfo->hWnd == tabinfo->group[closegroup].hWnd ){
							hGroupTab = tabinfo->group[closegroup].hWnd;
							break;
						}
					}
					if ( hGroupTab != NULL ){
						tabinfo->hWnd = tabinfo->group[i].hWnd;
						CloseGroup(tabinfo, closegroup, hGroupTab);
					}
					return tabbase;
				}
			}
		}
	}
	return -1;
}


#define CutTableItem(table, index, max) memmove(&table[(index)], &table[(index)+1], (BYTE *)&table[max] - (BYTE *)&table[(index)+1]); // table[index] を除去して残りを詰める

// １窓終了時の後処理 --------------------------------------------
void DestroyedPaneWindow(HWND hComboWnd, int baseindex)
{
	int showindex;
	HWND hNewFocusWnd = NULL;
	BOOL DecPane = FALSE;
	TCHAR buf[VFPS];
	HWND hPaneWnd;

	PPC_APPINFO *cinfo;

	hPaneWnd = Combo.base[baseindex].hWnd;
	showindex = GetComboShowIndex(hPaneWnd);
	cinfo = Combo.base[baseindex].cinfo;

	if ( Combo.Docks.t.cinfo == cinfo ){
		Combo.Docks.t.cinfo = NULL;
		Combo.Docks.b.cinfo = NULL;
	}

	if ( (Combo.Closed.list != NULL) && (Combo.Closed.Items > 0) && (cinfo != NULL) ){
		tstrcpy(Combo.Closed.list[Combo.Closed.LastIndex++].ID, cinfo->RegSubCID);
		if ( Combo.Closed.LastIndex >= Combo.Closed.Items ) Combo.Closed.LastIndex = 0;
	}

	if ( showindex >= 0 ){						// 表示ペインから削除 ---------
		int newppc = -1;

		if ( Combo.Tabs > 0 ){ // タブがあるなら、別のタブを表示する
			HWND hTabWnd;
			int tabcount;

			if ( X_combos[0] & CMBS_TABEACHITEM ) newppc = -2; // 未使用割当てを禁止
			hTabWnd = GetTabWndForGetPane(hPaneWnd, showindex);
			tabcount = TabCtrl_GetItemCount(hTabWnd);

			if ( tabcount > 1 ){ // 現在グループのタブ(閉じたタブの左)から選択
				int tabindex, tabbase;

				for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
					TC_ITEM tie;

					tie.mask = TCIF_PARAM;
					if ( IsTrue(TabCtrl_GetItem(hTabWnd, tabindex, &tie)) ){
						if ( (HWND)tie.lParam == hPaneWnd ){
							if ( newppc < 0 ) continue; // まだ未発見
							break; // 現在タブの左に候補があるので確定
						}
						tabbase = GetComboBaseIndex((HWND)tie.lParam);
						if ( (tabbase >= 0) &&
							 (GetComboShowIndex((HWND)tie.lParam) < 0) ){
							newppc = tabbase; // 候補を設定
						}
					}
				}
			}else if ( Combo.show[showindex].tab.groupcount > 1 ){
				// 別グループのタブを選択＆現グループを削除
				if ( (Combo.Tabs >= 2) && !(X_combos[0] & CMBS_TABEACHITEM) ){
					int tabpane, newppc2;

					for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
						newppc2 = DeleteWndTab(Combo.show[tabpane].tab.hWnd, tabpane, hPaneWnd);
						if ( tabpane == showindex ) newppc = newppc2;
					}
				}else{
					newppc = DeleteWndTab(hTabWnd, showindex, hPaneWnd);
				}
			}
		}

		// 未使用窓(別のペインでも可)があるなら割り当てる
		if ( (newppc == -1) &&
			 (Combo.BaseCount > X_mpane.limit) &&
			 (Combo.ShowCount >= X_mpane.limit) ){
			int newbase;

			for ( newbase = 0 ; newbase < Combo.BaseCount ; newbase++ ){
				int showc;

				for ( showc = 0 ; showc < Combo.ShowCount ; showc++ ){
					if ( Combo.show[showc].baseNo == newbase ) break;
				}
				if ( showc == Combo.ShowCount ){
					newppc = newbase;
					break;
				}
			}
		}

		if ( newppc < 0 ){ // 候補がないのでペインを閉じる
			WINPOS WPos;

			if ( (X_combos[0] & CMBS_TABMINCLOSE) &&
				 (Combo.ShowCount > 1) &&
				 (Combo.ShowCount == X_mpane.first) ){
				PostMessage(hComboWnd, WM_CLOSE, 0, 0);
				return; // タブ並びを保存するために、以降の処理をしない
			}
			// ペイン大きさを記憶
			thprintf(buf, TSIZEOF(buf), T("%s%d"), ComboID, showindex);
			WPos.show = 0;
			WPos.reserved = 0;
			WPos.pos = Combo.show[showindex].box;
			SetCustTable(Str_WinPos, buf, &WPos, sizeof(WPos));

			hNewFocusWnd = DeletePane(showindex);
			DecPane = TRUE;
		}else{ // 新しいPPcを設定
			SendMessage(Combo.hWnd, WM_SETREDRAW, FALSE, 0);
			Combo.show[showindex].baseNo = newppc;
			if ( hPaneWnd == hComboSubPaneFocus ){
				hComboSubPaneFocus = Combo.base[newppc].hWnd;
			}
			if ( hPaneWnd == hComboFocusWnd ){
				hNewFocusWnd = Combo.base[newppc].hWnd;
			}else if ( Combo.Tabs >= 2 ){ // 現在窓以外の表示タブが変わった
				// →タブの再選択が必要
				int tabpane, focuspane;

				if ( X_combos[0] & CMBS_TABEACHITEM ){
					focuspane = GetComboShowIndex(hComboFocusWnd);
					for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
						if ( tabpane == focuspane ) continue;
						SelectTabByWindow(Combo.base[newppc].hWnd, tabpane);
					}
				}else{
					SelectTabByWindow(Combo.base[newppc].hWnd, showindex);
				}
			}
			if ( (hComboSubPaneFocus == hNewFocusWnd) && (showindex == 0) ){
				if ( Combo.ShowCount > 1 ){
					hComboSubPaneFocus = Combo.base[Combo.show[1].baseNo].hWnd;
				}else{
					if ( Combo.BaseCount <= 1 ){
						hComboSubPaneFocus = NULL;
					}else{
						hComboSubPaneFocus =
								(Combo.base[0].hWnd == hNewFocusWnd)
								? Combo.base[1].hWnd : Combo.base[0].hWnd;
					}
				}
			}
			ShowWindow(Combo.base[newppc].hWnd, SW_SHOWNOACTIVATE);
			SendMessage(Combo.hWnd, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(Combo.hWnd, NULL, TRUE);
		}
	}

	// COMBOITEMSTRUCT を使用できないようにする
	Combo.base[baseindex].hWnd = (HWND)(DWORD_PTR)(-2);
	Combo.base[baseindex].cinfo = NULL;

	// 親ディレクトリのタブを捜す
	if ( (X_combos[0] & CMBS_TABUPDIRCLOSE) && (cinfo != NULL) ){
		if ( VFSFullPath(buf, T(".."), cinfo->path) != NULL ){
			int bi;

			for ( bi = 0 ; bi < Combo.BaseCount ; bi++ ){
				if ( (Combo.base[bi].cinfo != NULL) &&
					 (tstrcmp(buf, Combo.base[bi].cinfo->path) == 0) ){
					hNewFocusWnd = Combo.base[bi].hWnd;
					break;
				}
			}
		}
	}

	if ( (hNewFocusWnd == NULL) && (hPaneWnd == hComboFocusWnd) ){
		if ( (showindex > 0) && (Combo.ShowCount > 1) && (Combo.base[Combo.show[1].baseNo].hWnd != hNewFocusWnd) ){ // 左
			hNewFocusWnd = Combo.base[Combo.show[1].baseNo].hWnd;
		}else if ( Combo.base[Combo.show[0].baseNo].hWnd != hNewFocusWnd ){
			hNewFocusWnd = Combo.base[Combo.show[0].baseNo].hWnd;
		}
	}

	if ( hPaneWnd == hComboSubPaneFocus ){
		hComboSubPaneFocus = NULL;
	}

	{ // 全てのタブからこの窓を削除
		int tabindex, tabpane;

		for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
			tabindex = GetTabItemIndex(hPaneWnd, tabpane);
			if ( tabindex >= 0 ){
				HWND hTabWnd = Combo.show[tabpane].tab.hWnd;

				// 削除するタブが画面左端の場合に削除すると、より左側のタブが
				// 画面に出てこないことがあるので、一旦左端を選択して、
				// より多く表示させるようにする
				// ※削除タブが最終タブの場合、タブが選択されていないと
				//   操作ができなくなる対策も含む
				if ( tabindex > 0 ){
					RECT box;

					if ( TabCtrl_GetItemRect(hTabWnd, tabindex, &box) ){
						if ( box.left < 8 ){ // 非選択:0 画面左端:2
							TabCtrl_SetCurSel(hTabWnd, 0);
						}
					}
				}
				TabCtrl_DeleteItem(hTabWnd, tabindex);
			}
		}
	}
									// 全体テーブルから削除
	if ( (baseindex + 1) < Combo.BaseCount ){
		CutTableItem(Combo.base, baseindex, Combo.BaseCount);
	}
	Combo.BaseCount--;
	{							// 表示テーブルを補正する
		int i;

		for ( i = 0 ; i < Combo.ShowCount ; i++ ){
			if ( Combo.show[i].baseNo > baseindex ) Combo.show[i].baseNo--;
		}
	}
									// フォーカス補正／終了処理
	if ( Combo.BaseCount ){
		SendMessage(Combo.hWnd, WM_SETREDRAW, FALSE, 0);
		if ( Combo.ShowCount == 0 ){
			CreatePane(0);
			ShowWindow(Combo.base[0].hWnd, SW_SHOWNORMAL);
		}
		// タブが不要になったので、なくす
		if ( !(X_combos[0] & CMBS_TABALWAYS) &&
			 Combo.Tabs &&
			 IsHideTabForMultiShow() ){
			int tabpane;

			for ( tabpane = 0 ; tabpane < Combo.ShowCount ; tabpane++ ){
				COMBOTABINFO *tabs;

				tabs = &Combo.show[tabpane].tab;
				if ( tabpane >= Combo.Tabs ){
					if ( tabs->groupcount > 0 ){
						PPcHeapFree(tabs->group);
						tabs->group = NULL;
						tabs->groupcount = 0;
					}
				}else if ( tabs->groupcount == 0 ){
					DestroyWindow(tabs->hWnd);
				}else{
					int i;

					for ( i = 0 ; i < tabs->groupcount ; i++ ){
						DestroyWindow(tabs->group[i].hWnd);
					}
					PPcHeapFree(tabs->group);
					tabs->group = NULL;
					tabs->groupcount = 0;
					if ( tabs->hSelecterWnd != NULL ){
						DestroyWindow(tabs->hSelecterWnd);
						tabs->hSelecterWnd = NULL;
					}
				}
				tabs->height = 0;
				tabs->hWnd = NULL;
			}

			Combo.Tabs = 0;
			Combo.show[0].box.bottom += Combo.Height.TabBar; // CMBS_VALWINSIZE が有効の時、これがないと高さが縮む
			Combo.Height.TabBar = 0;
		}

		if ( hNewFocusWnd != NULL ){
			SetFocusWithBackupedRightWindow(hNewFocusWnd);
		}

		if ( hComboSubPaneFocus == NULL ) ResetSubFocus(NULL);
		SortComboWindows( (DecPane && (X_combos[0] & CMBS_VALWINSIZE)) ?
				0 : SORTWIN_LAYOUTPAIN);
		SendMessage(Combo.hWnd, WM_SETREDRAW, TRUE, 0);
		{
			int i;

			for ( i = 0 ; i < Combo.ShowCount ; i++ ){
				PPC_APPINFO *cinfo;

				cinfo = Combo.base[Combo.show[i].baseNo].cinfo;
				if ( cinfo != NULL ){
					if ( cinfo->hHeaderWnd != NULL ){
						InvalidateRect(cinfo->hHeaderWnd, NULL, FALSE);
					}
					if ( cinfo->hScrollBarWnd != NULL ){
						InvalidateRect(cinfo->hScrollBarWnd, NULL, FALSE);
					}
				}
			}
		}
		InvalidateRect(Combo.hWnd, NULL, TRUE);
	}else{
		PostMessage(hComboWnd, WM_CLOSE, 0, 0);
	}
}

// 指定のペインに指定の窓を設定 --------------------------------------------
void SelectComboWindow(int showindex, HWND hTargetWnd, BOOL focus)
{
	int targetshowindex, targetbaseindex;

	if ( Combo.ShowCount <= 0 ) return;
	if ( showindex < 0 ){
		showindex = GetComboShowIndex(hComboFocusWnd);
		if ( showindex < 0 ){
			showindex = Combo.MainPane;
			if ( showindex < 0 ) showindex = 0;
		}
	}
	if ( showindex >= Combo.ShowCount ) showindex = Combo.ShowCount - 1;

	targetshowindex = GetComboShowIndex(hTargetWnd);
	if ( targetshowindex >= 0 ){
		targetbaseindex = Combo.show[targetshowindex].baseNo;
	}else{
		targetbaseindex = GetComboBaseIndex(hTargetWnd);
		if ( targetbaseindex < 0 ) return; // 該当無し
	}

	if ( (Combo.Tabs > 0) && (Combo.show[showindex].tab.groupcount > 0) ){
		int i;
		COMBOTABINFO *tabinfo;
		HWND hTabWnd = GetTabWndForGetPane(hTargetWnd, showindex);

		tabinfo = &Combo.show[showindex].tab;
		for ( i = 0 ; i < tabinfo->groupcount; i++ ){
			if ( hTabWnd == tabinfo->group[i].hWnd ){
				tabinfo->hWnd = hTabWnd;
				tabinfo->group[i].hCurWnd = hTargetWnd;
				break;
			}
		}
	}

	// フォーカスがあるペインを変える為、フォーカスを常に設定する必要がある
	if ( (focus == FALSE) && (showindex == GetComboShowIndex(hComboFocusWnd)) ){
		focus = TRUE;
	}

	if ( Combo.base[targetbaseindex].cinfo == NULL ){
		TCHAR path[VFPS];

		path[0] = '\0';
		GetWindowText(hTargetWnd, path, VFPS);
		SetComboAddresBar(path);
	}

	if ( Combo.Tabs <= 1 ){
		if ( targetshowindex >= 0 ){
			ChangeReason = T("SelectComboWindow@1");
			if ( focus ) SetFocus(hTargetWnd);
			return;
		}
		// 表示されていないタブなので切換
		SelectHidePane(showindex, hTargetWnd);
	}else{ // タブの分離表示中
		int showindexBaseindex;

		if ( X_combos[0] & CMBS_TABEACHITEM ){
			// 表示しようとしているペインに該当タブがない…追加する
			if ( GetTabItemIndex_AndChangeGroup(hTargetWnd, showindex) < 0 ){
				// 既存ペインに該当タブがあれば削除
				int tabindex;
				for ( tabindex = 0 ; tabindex < Combo.Tabs ; tabindex++ ){
					int tabii;

					tabii = GetTabItemIndex(hTargetWnd, tabindex);
					if ( tabii >= 0 ){ // 登録されていたので削除
						TabCtrl_DeleteItem(Combo.show[tabindex].tab.hWnd, tabii);
					}
				}
				AddTabInfo(showindex, hTargetWnd);
			}
		}

		if ( targetshowindex >= 0 ){
			if ( targetshowindex != showindex ){ // 別のタブに表示中
				HWND hSwapTargetWnd;

				// 情報入れ替え
				Combo.show[targetshowindex].baseNo = showindexBaseindex = Combo.show[showindex].baseNo;
				Combo.show[showindex].baseNo = targetbaseindex;
				if ( hComboSubPaneFocus == Combo.base[showindexBaseindex].hWnd ){
					hComboSubPaneFocus = Combo.base[targetbaseindex].hWnd;
					ChangeReason = T("SelectComboWindow1");

				}else if ( hComboSubPaneFocus == Combo.base[targetbaseindex].hWnd ){
					hComboSubPaneFocus = Combo.base[showindexBaseindex].hWnd;
					ChangeReason = T("SelectComboWindow2");

				}

				if ( (hComboSubPaneFocus == NULL) || (Combo.ShowCount == 1) ){
					ResetSubFocus(NULL); // １窓になった／反対窓が消えたので反対窓を再設定
					ChangeReason = T("SelectComboWindow2a");
				}

				CheckComboTable(T("SelectComboWindow1"));
				SortComboWindows(SORTWIN_LAYOUTPAIN);
				InvalidateRect(Combo.hWnd, &Combo.Panes.box, TRUE);
				hSwapTargetWnd = Combo.base[showindexBaseindex].hWnd;
				InvalidateRect(hSwapTargetWnd, NULL, TRUE);

				// 入れ替え先のタブを切り替え
				SelectTabByWindow(hSwapTargetWnd, targetshowindex);
			}
		}else{ // 表示していない窓
			SendMessage(Combo.hWnd, WM_SETREDRAW, FALSE, 0);
			showindexBaseindex = Combo.show[showindex].baseNo;
			Combo.show[showindex].baseNo = targetbaseindex;
			if ( Combo.base[showindexBaseindex].hWnd == hComboSubPaneFocus ){
				hComboSubPaneFocus = hTargetWnd;
				ChangeReason = T("SelectComboWindow3");
			}
			// SW_HIDE 先が focus を持っている場合、foucs 変化が生じるので、
			// 先にfocusをいじっておく
			if ( Combo.base[showindexBaseindex].hWnd == GetFocus() ){
//				ShowWindow(hTargetWnd, SW_SHOWNA);
				// 明後日の位置に表示させて、SortComboWindows による位置移動のときに子ウィンドウ(ツリー)を正しく描画させる
				SetWindowPos(hTargetWnd, NULL, -10, -10, 0, 0,
					SWP_SHOWWINDOW | SWP_NOACTIVATE);
				SetFocus(hTargetWnd);
				ShowWindow(Combo.base[showindexBaseindex].hWnd, SW_HIDE);
				CheckComboTable(T("@SelectComboWindow2a"));
			}else{
				ShowWindow(Combo.base[showindexBaseindex].hWnd, SW_HIDE);
				CheckComboTable(T("@SelectComboWindow2b"));
//				ShowWindow(hTargetWnd, SW_SHOWNA);
				// 明後日の位置に表示させて、SortComboWindows による位置移動のときに子ウィンドウを正しく描画させる
				SetWindowPos(hTargetWnd, NULL, -10, -10, 0, 0,
					SWP_SHOWWINDOW | SWP_NOACTIVATE);
			}
			SendMessage(Combo.hWnd, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(Combo.hWnd, NULL, TRUE);
			SortComboWindows(SORTWIN_LAYOUTPAIN);
		}
	}

	if ( focus ){
		ChangeReason = T("SelectComboWindow@2");
		SetFocus(hTargetWnd);
	}

	if ( Combo.Tabs &&
		 ( (focus == FALSE) || (Combo.base[targetbaseindex].capture != CAPTURE_WINDOWEX) ) ){
		int tabindex;
		int tabwndindex;

		tabwndindex = Combo.Tabs > 1 ? GetComboShowIndex(hTargetWnd) : 0;
		tabindex = GetTabItemIndex(hTargetWnd, tabwndindex);
		if ( tabindex < 0 ){
			SetTabInfo(tabwndindex, hTargetWnd);
			tabindex = GetTabItemIndex(hTargetWnd, tabwndindex);
		}
		if ( tabindex >= 0 ){
			TabCtrl_SetCurSel(Combo.show[tabwndindex].tab.hWnd, tabindex);
		}
		CheckComboTable(T("SelectComboWindow3"));
	}
}

// 表示中の窓を隠し、ペインを削除 --------------------------------------------
void HidePane(int showindex)
{
	HWND hNewFocusWnd;

	if ( (showindex < 0) || (Combo.ShowCount <= 1) || (Combo.ShowCount <= showindex) ){
		return;
	}

	if ( X_combos[0] & CMBS_TABEACHITEM ) return;

	if ( (Combo.Tabs == 0) && (Combo.ShowCount > 1) ){
		CreateTabBar(CREATETAB_APPEND, NULL, FALSE);
	}

	if ( Combo.base[Combo.show[showindex].baseNo].hWnd == hComboFocusWnd ){
		if ( Combo.ShowCount > 1 ){
			if ( showindex == 0 ){
//				ChangeReason = T("HidePane@1");
				SetFocus(hComboSubPaneFocus);
			}else{
//				ChangeReason = T("HidePane@2");
				SetFocus(Combo.base[Combo.show[0].baseNo].hWnd);
			}
		}
	}
	ShowWindow(Combo.base[Combo.show[showindex].baseNo].hWnd, SW_HIDE);
									// 表示テーブルから削除
	hNewFocusWnd = DeletePane(showindex);
	if ( hNewFocusWnd != NULL ){
		SetFocusWithBackupedRightWindow(hNewFocusWnd);
	}

	SortComboWindows(SORTWIN_LAYOUTPAIN);
	InvalidateRect(Combo.hWnd, NULL, TRUE);
}

// 隠れていたペインを既存のペインに表示する ----------------------------------
void SelectHidePane(int showindex, HWND hWnd)
{
	int baseindex;

	if ( showindex < 0 ) showindex = 0;

	baseindex = GetComboBaseIndex(hWnd);
	if ( baseindex < 0 ) return;

	if ( Combo.ShowCount == 0 ){ // ペインがない→ペインを追加
		CreatePane(baseindex);
		ShowWindow(hWnd, SW_SHOWNORMAL);
		SortComboWindows(SORTWIN_LAYOUTPAIN);
	}else{
		int tempindex;
		BOOL focus = FALSE;
		HWND hHideWnd;

		tempindex = Combo.show[showindex].baseNo;
		hHideWnd = Combo.base[tempindex].hWnd;

		if ( Combo.ShowCount == 1 ){
			if ( hWnd != hHideWnd ) ShowWindow(hHideWnd, SW_HIDE);
			KCW_FocusFix(Combo.hWnd, hWnd);
			SetFocus(hWnd); // 順番が違うがこれで変更
			return;
		}

		if ( hHideWnd == hComboFocusWnd ) focus = TRUE;

		// 表示ペインにフォーカス→入れ替え後にフォーカス設定
		if ( hWnd == hComboSubPaneFocus ){
			// 表示ペインが右→入れ替え後に右設定
			hComboSubPaneFocus = hHideWnd;
		}else if ( hHideWnd == hComboSubPaneFocus ){
			// これから表示するペインが右→入れ替え後に右設定
			hComboSubPaneFocus = hWnd;
		}

		SendMessage(Combo.hWnd, WM_SETREDRAW, FALSE, 0);
		Combo.show[showindex].baseNo = baseindex;
		ShowWindow(hHideWnd, SW_HIDE);
		// 明後日の位置に表示させて、SortComboWindows による位置移動のときに子ウィンドウ(ツリー)を正しく描画させる
//		ShowWindow(hWnd, SW_SHOWNOACTIVATE);
		SetWindowPos(hWnd, NULL, -10, -10, 0, 0,
			SWP_SHOWWINDOW | SWP_NOACTIVATE);

		ChangeReason = T("SelectHidePane@1");
		if ( IsTrue(focus) ) SetFocus(hWnd);
		SendMessage(Combo.hWnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(Combo.hWnd, NULL, TRUE);
		SortComboWindows(SORTWIN_LAYOUTPAIN);
		ShowWindow(hWnd, SW_SHOWNORMAL);
	}
}

// 表示中の窓を独立化(KCW_eject経由で呼び出さないと、paneのメッセージループ
// 内から呼び出す場合が生じ、トラブルが起きる)
void EjectPane(int baseindex)
{
	PPC_APPINFO *cinfo;
	TCHAR param[MAX_PATH], RegID[10];
	int i;

	if ( (Combo.BaseCount <= 1) || (baseindex >= Combo.BaseCount) ) return;
	cinfo = Combo.base[baseindex].cinfo;
	if ( cinfo == NULL ) return;
	tstrcpy(RegID, cinfo->RegID);
	thprintf(param, TSIZEOF(param), T("/single /show /bootid:%c"), RegID[2]);
	if ( Combo.BaseCount == X_mpane.first ){
		X_mpane.first--;
	}
	PostMessage(Combo.base[baseindex].hWnd, WM_CLOSE, 0, 0);
	for ( i = 0 ; i < 20 ; i++ ){
		PeekLoop();
		Sleep(50);
		if ( PPxGetHWND(RegID) == NULL ) break;
	}
	PPCui(Combo.hWnd, param); // 新規プロセスで生成
	for ( i = 0 ; i < 500 ; i++ ){
		HWND hNewPPcWnd;

		PeekLoop();
		Sleep(50);
		hNewPPcWnd = PPxGetHWND(RegID);
		if ( hNewPPcWnd != NULL ){
			ShowWindow(hNewPPcWnd, SW_SHOW);
			SetForegroundWindow(hNewPPcWnd);
			break;
		}
	}
}

// ペインを作成＋登録 --------------------------------------------
void CreatePane(int baseindex)
{
	COMBOPANES *ncs;
	int PaneIndex;
	COMBOPANES *paneinfo;

	if ( Combo.ShowCount >= Combo_Max_Show ) return;
	if ( (ncs = HeapReAlloc( hProcessHeap, 0, Combo.show, sizeof(COMBOPANES) * (Combo.ShowCount + 2) )) == NULL ){ // +2... 増加する+1 と、予備兼処理簡略化用+1
		return; // ペインを確保する余裕がない
	}
	Combo.show = ncs;
	PaneIndex = Combo.ShowCount++;
	paneinfo = &Combo.show[PaneIndex];

	if ( PaneIndex > 0 ) memset(paneinfo, 0, sizeof(COMBOPANES)); // 1つめは初期化済み(TABを先に用意している)
	if ( baseindex < 0 ) return; // Show の確保のみ

	paneinfo->baseNo = baseindex;
	if ( Combo.ShowCount == 2 ){
		hComboSubPaneFocus = Combo.base[baseindex].hWnd;
	}

	if ( Combo.Tabs == 0 ) return;

	// タブの処理
	if ( (Combo.ShowCount > 1) && (X_combos[0] & CMBS_TABSEPARATE) ){
		int tabpane;

		if ( !(X_combos[0] & CMBS_TABEACHITEM) ){ // 追加したペイン以外にタブを追加
			for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
				int tabii;

				tabii = GetTabItemIndex(Combo.base[baseindex].hWnd, tabpane);
				if ( tabii < 0 ){ // 登録されていないので追加
					AddTabInfo(tabpane, Combo.base[baseindex].hWnd);
				}
			}
		}else{ // 既存ペインに該当タブがあれば削除
			for ( tabpane = 0 ; tabpane < Combo.ShowCount ; tabpane++ ){
				int tabii;

				if ( tabpane >= PaneIndex ) break;
				tabii = GetTabItemIndex(Combo.base[baseindex].hWnd, tabpane);
				if ( tabii >= 0 ){ // 登録されていたので削除
					HWND hTabWnd = Combo.show[tabpane].tab.hWnd;

					if ( TabCtrl_GetItemCount(hTabWnd) <= 1 ){ // ペインも削除
						NewPaneTabBar(NULL, 0);
						TabCtrl_DeleteItem(Combo.show[tabpane].tab.hWnd, tabii);

						DestroyWindow(hTabWnd);
						Combo.Tabs--;
						CutTableItem(Combo.show, tabpane, Combo.ShowCount);
						Combo.ShowCount--;

						// 削除により SubPane が MainPane と一致するようになったときの処理
						if ( (tabpane == Combo.MainPane) &&
							 (tabpane == GetComboShowIndexDefault(hComboSubPaneFocus)) ){
							hComboSubPaneFocus = Combo.base[baseindex].hWnd;
						}
						SelectTabByWindow(Combo.base[baseindex].hWnd, Combo.ShowCount - 1);
						return;
					}else if ( TabCtrl_GetCurSel(hTabWnd) == tabii ){ // 再選択
						TC_ITEM tie;
						int newcur;

						newcur = tabii ? 0 : 1;
						tie.mask = TCIF_PARAM;
						TabCtrl_GetItem(hTabWnd, newcur, &tie);

						NewPaneTabBar(NULL, 0);
						TabCtrl_DeleteItem(Combo.show[tabpane].tab.hWnd, tabii);
						TabCtrl_SetCurSel(hTabWnd, newcur);
						SelectComboWindow(tabpane, (HWND)tie.lParam, FALSE);
						SelectTabByWindow(Combo.base[baseindex].hWnd, Combo.ShowCount - 1);
						return;
					}else{ // 単純削除
						TabCtrl_DeleteItem(Combo.show[tabpane].tab.hWnd, tabii);
					}
				}
			}
		}
		if ( PaneIndex > 0 ){
			TCHAR name[VFPS];

			if ( (Combo.show[0].tab.groupcount > 0) && !(X_combos[0] & CMBS_TABEACHITEM) ){
				int group, group_max;

				group_max = Combo.show[0].tab.groupcount;
				for ( group = 0; group < group_max; group++ ){
					TABGROUP *newgroup;
					COMBOTABINFO *tabs;
					TC_ITEM tie;
					HWND hTabWnd;

					GetWindowText(Combo.show[0].tab.group[group].hWnd, name, VFPS);
					if ( Combo.show[PaneIndex].tab.hWnd == NULL ){
						CreateTabBar(CREATETAB_APPEND, name, TRUE);
					}

					tabs = &Combo.show[PaneIndex].tab;

					if ( tabs->groupcount == 0 ){
						newgroup = PPcHeapAlloc(sizeof(TABGROUP) * 2);
						if ( newgroup != NULL ) newgroup->hWnd = tabs->hWnd;
						tabs->show_all = (X_combos[1] & CMBS1_SELECTEDGROUP) ? 0 : 1;
					}else{
						newgroup = HeapReAlloc( hProcessHeap, 0, tabs->group, sizeof(TABGROUP) * (tabs->groupcount + 1) );
					}
					if ( newgroup == NULL ) break; // これ以上保存できない

					hTabWnd = CreateComboTabBar(name);

					// 仮。ないと高さが計算できない
					if ( tabs->hSelecterWnd != NULL ){
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

					{
						HWND hSrcTab, hDstTab;
						int index, count;
						hSrcTab = Combo.show[0].tab.group[group].hWnd;
						hDstTab = tabs->group[group].hWnd;
						count = TabCtrl_GetItemCount(hSrcTab);
						for (index = 0; index < count; index++ ){
							tie.mask = TCIF_TEXT | TCIF_PARAM;
							tie.pszText = name;
							tie.cchTextMax = VFPS;

							TabCtrl_GetItem(hSrcTab, index, &tie);
							SendMessage(hDstTab, TCM_INSERTITEM, (WPARAM)index, (LPARAM)&tie);
						}
					}
				}
			}else{
				GetWindowText(Combo.show[0].tab.hWnd, name, VFPS);
				NewPaneTabBar(name, 0);
			}
		}else{
			NewPaneTabBar(NULL, 0);
		}
		SelectTabByWindow(Combo.base[baseindex].hWnd, PaneIndex);
	}else{ // ペイン毎タブ無し
		// ペインが２以上なら、タブhwndを設定
		if ( (Combo.ShowCount > 1)  ){
			paneinfo->tab = Combo.show[0].tab;
			paneinfo->tab.group = NULL;
			paneinfo->tab.groupcount = 0;
			if ( (Combo.show[0].tab.groupcount > 0) && !(X_combos[0] & CMBS_TABSEPARATE) ){
				int i;

				paneinfo->tab.group = PPcHeapAlloc(sizeof(TABGROUP) * Combo.show[0].tab.groupcount);
				for ( i = 0; i < Combo.show[0].tab.groupcount; i++ ){
					paneinfo->tab.group[i].hWnd = Combo.show[0].tab.group[i].hWnd;
					paneinfo->tab.group[i].hCurWnd = NULL;
				}
			}
		}
		// タブ未登録なら登録
		if ( (baseindex >= 0) &&
			 (GetTabItemIndex(Combo.base[baseindex].hWnd, 0) < 0) ){
			AddTabInfo(0, Combo.base[baseindex].hWnd);
		}
	}
}

void CreateAndInitPane(int baseindex)
{
	CreatePane(baseindex);
	ShowWindow(Combo.base[baseindex].hWnd, SW_SHOWNORMAL);
	SortComboWindows(SORTWIN_LAYOUTPAIN);
}

#define SortPane_DeleteResize(posmin, posmax, panepossize) {\
	int pos, I_;\
\
	pos = Combo.Panes.clientbox.posmin;\
\
	for ( I_ = 0 ; I_ < Combo.ShowCount ; I_++ ){\
		int rate; \
		rate = (panepossize * (I_ + 1)) / Combo.ShowCount;\
		Combo.show[I_].box.posmax = pos + (Combo.show[I_].box.posmax - Combo.show[I_].box.posmin) + rate;\
		Combo.show[I_].box.posmin = pos;\
		pos = Combo.show[I_].box.posmax; \
	}\
}


// ペインを削除 --------------------------------------------
HWND DeletePane(int showindex)
{
	HWND hNewFocusWnd = NULL;
	int panepossize;

	CheckComboTable(T("DeletePane-pre"));
	if ( Combo.ShowCount <= 1 ){ // 最後の１つ
		hComboFocusWnd = NULL;

		// 削除するペインにフォーカスが当たっている場合
	}else if ( Combo.base[Combo.show[showindex].baseNo].hWnd == hComboFocusWnd ){
		hNewFocusWnd = Combo.base[Combo.show[(showindex == 0) ? 1 : 0].baseNo].hWnd;
		if ( hNewFocusWnd == hComboSubPaneFocus ) hComboSubPaneFocus = NULL;
	}

	if ( Combo.base[Combo.show[showindex].baseNo].hWnd == hComboSubPaneFocus ){
		hComboSubPaneFocus = NULL;
	}else if ( (showindex == 0) &&
		 (Combo.ShowCount > 1) &&
		 (Combo.base[Combo.show[1].baseNo].hWnd == hComboSubPaneFocus) ){
		hComboSubPaneFocus = NULL;
	}
									// 該当タブを消す
	if ( (Combo.Tabs > 1) || (Combo.ShowCount <= 1) ){
		HWND hTabWnd;
		COMBOTABINFO *tabs;

		Combo.Tabs--;
		tabs = &Combo.show[showindex].tab;
		if ( tabs->hSelecterWnd != NULL ){
			DestroyWindow(tabs->hSelecterWnd);
			tabs->hSelecterWnd = NULL;
		}
		if ( tabs->groupcount > 0 ){
			int index;

			for ( index = 0; index < tabs->groupcount; index++ ){
				DestroyWindow(tabs->group[index].hWnd);
			}
			PPcHeapFree(tabs->group);
			tabs->groupcount = 0;
			tabs->group = NULL;
		}else{
			hTabWnd = tabs->hWnd;
			DestroyWindow(hTabWnd);
		}
		tabs->hWnd = NULL;
		InvalidateRect(Combo.hWnd, NULL, TRUE);
	}

									// 表示テーブルから削除
	if ( X_combos[0] & CMBS_VPANE ){
		panepossize = Combo.show[showindex].box.bottom - Combo.show[showindex].box.top;
	}else{
		panepossize = Combo.show[showindex].box.right - Combo.show[showindex].box.left;
	}

	if ( (showindex + 1) < Combo.ShowCount ){
		CutTableItem(Combo.show, showindex, Combo.ShowCount);
	}
	Combo.ShowCount--;

	if ( Combo.ShowCount > 0 ){
		if ( Combo.ShowCount == 1 ){ // １窓になったので反対窓を再設定
			ChangeReason = T("DeletePane@@1");
			if ( Combo.BaseCount <= 1 ){ // 窓が１つのみ
				hComboSubPaneFocus = NULL;
			}else{
				if ( hNewFocusWnd == NULL ){
					hComboSubPaneFocus = Combo.base[Combo.show[0].baseNo ? 0 : 1].hWnd;
				}else{
					hComboSubPaneFocus = Combo.base[0].hWnd;
					if ( hNewFocusWnd == hComboSubPaneFocus ){
						hComboSubPaneFocus = Combo.base[1].hWnd;
					}
				}
			}
		}else{
			if ( (ComboInit == 0) && !(X_combos[0] & CMBS_VALWINSIZE) && (X_combos[1] & CMBS1_KEEPRATEONDELPANE) ){
				if ( X_combos[0] & CMBS_VPANE ){
					SortPane_DeleteResize(top, bottom, panepossize);
				}else{
					SortPane_DeleteResize(left, right, panepossize);
				}
			}

			if ( hComboSubPaneFocus == NULL ){// 反対窓が消えたので反対窓を再設定
				ChangeReason = T("DeletePane@@2");
				hComboSubPaneFocus = Combo.base[Combo.show[1].baseNo].hWnd;
			}
		}
		CheckComboTable(T("@DeletePane"));
	}
	return hNewFocusWnd;
}

//============================================================================
// 現在ペイン・indexなどの情報を求める関数群
//============================================================================
// 指定のタブの座標から PPC_APPINFO を求める(D&D用)
PPC_APPINFO *GetComboTarget(HWND hTargetWnd, POINT *pos)
{
	int baseindex, tabpane;
	HWND hWnd;

	hWnd = hTargetWnd;
	for (;;){
		if ( hWnd == NULL ) break;
		if ( hWnd == Combo.hWnd ) break;

		// Combo でないのに Caption →別用途のダイアログか Capture window
		if ( GetWindowLongPtr(hWnd, GWL_STYLE) & WS_CAPTION ) return NULL;
		hWnd = GetParent(hWnd);
	}

	for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
		if ( hTargetWnd == Combo.show[tabpane].tab.hWnd ){
			TC_HITTESTINFO th;
			int tabindex;

			th.pt = *pos;
			ScreenToClient(Combo.show[tabpane].tab.hWnd, &th.pt);
			tabindex = TabCtrl_HitTest(Combo.show[tabpane].tab.hWnd, &th);
			if ( tabindex >= 0 ){
				TC_ITEM tie;

				tie.mask = TCIF_PARAM;
				if ( IsTrue(TabCtrl_GetItem(Combo.show[tabpane].tab.hWnd, tabindex, &tie)) ){
					baseindex = GetComboBaseIndex((HWND)tie.lParam);
					goto getdata;
				}
				return NULL;
			}
		}
	}
	baseindex = GetComboBaseIndex(hComboFocusWnd);
getdata:
	if ( baseindex < 0 ) return NULL;
	return Combo.base[baseindex].cinfo;
}

// hWnd から ItemNo baseindex を取得
int GetComboBaseIndex(HWND hWnd)
{
	int baseindex;

	for ( baseindex = 0 ; baseindex < Combo.BaseCount ; baseindex++ ){
		if ( Combo.base[baseindex].hWnd == hWnd ) return baseindex;
	}
	return -1;
}

// hBaseWnd から showindex を取得 ※非表示のときは showindex が得られない
int GetComboShowIndex(HWND hBaseWnd)
{
	int showindex;

	for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
		if ( Combo.base[Combo.show[showindex].baseNo].hWnd == hBaseWnd ){
			return showindex;
		}
	}
	return -1;
}

// hWnd から showindex を取得(該当がない場合は 0 を返す)
int GetComboShowIndexDefault(HWND hBaseWnd)
{
	int showindex;

	for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
		if ( Combo.base[Combo.show[showindex].baseNo].hWnd == hBaseWnd ){
			return showindex;
		}
	}
	return (Combo.ShowCount ? 0 : -1);
}

// hBaseWnd から showindex を取得(非表示の時でも所属ペインが分かる)
int GetComboShowIndexAll(HWND hBaseWnd)
{
	COMBOTABINFO *tabinfo;
	int showindex;
	TC_ITEM tie;

	// 各paneのアクティブbaseを探す
	for ( showindex = 0; showindex < Combo.ShowCount; showindex++ ){
		if ( Combo.base[Combo.show[showindex].baseNo].hWnd == hBaseWnd ){
			return showindex;
		}
	}
	// 各paneの非アクティブbaseを探す
	for ( showindex = 0; showindex < Combo.ShowCount; showindex++ ){
		tabinfo = &Combo.show[showindex].tab;
		if ( tabinfo->groupcount == 0 ){ // 非グループ
			HWND hTabWnd;
			int tabindex, tabcount;

			hTabWnd = tabinfo->hWnd;
			tabcount = TabCtrl_GetItemCount(hTabWnd);
			for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
				tie.mask = TCIF_PARAM;
				if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ){
					continue;
				}
				if ( (HWND)tie.lParam == hBaseWnd ) return showindex;
			}
		}else{ // グループ
			int i;

			for ( i = 0 ; i < tabinfo->groupcount; i++ ){
				HWND hTabWnd;
				int tabindex, tabcount;

				hTabWnd = tabinfo->group[i].hWnd;
				tabcount = TabCtrl_GetItemCount(hTabWnd);
				for ( tabindex = 0 ; tabindex < tabcount ; tabindex++ ){
					tie.mask = TCIF_PARAM;
					if ( TabCtrl_GetItem(hTabWnd, tabindex, &tie) == FALSE ){
						continue;
					}
					if ( (HWND)tie.lParam == hBaseWnd ){
						return showindex;
					}
				}
			}
		}
	}
	return -1;
}

// pos から showindex を取得
int GetComboShowIndexFromPos(POINT *pos)
{
	int showindex;

	if ( Combo.ShowCount == 0 ) return -1;
	if ( !(X_combos[0] & CMBS_VPANE) ){	// 横整列
		int x, y;

		x = pos->x;
		y = pos->y;
		for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
			if ( (Combo.show[showindex].box.top <= y) &&
				 (Combo.show[showindex].box.bottom > y) &&
				 ((Combo.show[showindex].box.right + SplitPix) > x) ){
				break;
			}
		}
	}else{ // 縦配列
		int y;

		y = pos->y;
		for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){
			if ( X_combos[1] & CMBS1_TABBOTTOM ){
				if ( (Combo.show[showindex].box.bottom + Combo.show[showindex].tab.height + SplitPix) > y ){
					break;
				}
			}else{
				if ( (Combo.show[showindex].box.bottom + SplitPix) > y ){
					break;
				}
			}

		}
	}
	if ( showindex == Combo.ShowCount ) showindex--;
	return showindex;
}

// hWnd から反対ペインの showindex を取得
int GetPairPaneComboShowIndex(HWND hTargetWnd)
{
	int showindex;

	if ( Combo.ShowCount < 2 ) return 0;

	showindex = GetComboShowIndex(hTargetWnd);
	if ( showindex >= 0 ){
		if ( showindex != Combo.MainPane ){	// メイン窓以外→メイン窓を選択
			return Combo.MainPane;
		}else{
			showindex = GetComboShowIndex(hComboSubPaneFocus);
		}
	}
	if ( (showindex < 0) || (showindex == Combo.MainPane) ){ // 強制補正
		showindex = (Combo.MainPane == 0) ? 1 : 0;
	}
	return showindex;
}

// hWnd から反対ペインの baseindex を取得
int GetPairPaneComboBaseIndex(HWND hTargetWnd)
{
	int baseindex;

	baseindex = GetComboBaseIndex(hTargetWnd);
	if ( baseindex < 0 ) return -1;
	if ( Combo.BaseCount < 2 ) return -1;
	if ( hTargetWnd == hComboFocusWnd ){ // 現在窓→反対窓
		if ( baseindex == Combo.show[Combo.MainPane].baseNo ){
			baseindex = GetComboBaseIndex(hComboSubPaneFocus);
		}else{
			baseindex = Combo.show[Combo.MainPane].baseNo;
		}
	}else{ // 現在窓以外→現在窓
		baseindex = GetComboBaseIndex(hComboFocusWnd);
	}
	return baseindex;
}

BOOL SetTabTipText(NMHDR *nmh)
{
	int tabpane;

	for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
		if ( nmh->hwndFrom == Combo.show[tabpane].tab.hTipWnd ){
			TC_ITEM tie;
			HWND hTabWnd;

			hTabWnd = Combo.show[tabpane].tab.hWnd;
														// 表示中タブにある？
			tie.mask = TCIF_PARAM;
			if ( IsTrue(TabCtrl_GetItem(hTabWnd, nmh->idFrom, &tie)) ){
				tstrcpy(tiptext, T("?"));
				GetWindowText((HWND)tie.lParam, tiptext, TSIZEOF(tiptext));
				((LPTOOLTIPTEXT)nmh)->lpszText = tiptext;
				((LPTOOLTIPTEXT)nmh)->hinst = NULL;
				return TRUE;
			}
		}
	}
	return FALSE;
}

void SelectChangeTab(NMHDR *nmh)
{
	int tabpane;
	HWND hNotifyWnd;
	BOOL sel_sel = FALSE;

	hNotifyWnd = nmh->hwndFrom;
	for ( tabpane = 0 ; tabpane < Combo.Tabs ; tabpane++ ){
		COMBOTABINFO *tabinfo;
		int tabindex;
		TC_ITEM tie;

		tabinfo = &Combo.show[tabpane].tab;
		if ( tabinfo->groupcount <= 0 ){
			if ( hNotifyWnd != tabinfo->hWnd ) continue;
		}else{
			int i;

			if ( hNotifyWnd == tabinfo->hSelecterWnd ){
				int showindex, groupindex;

				if ( Combo.Tabs < Combo.ShowCount ){
					showindex = GetComboShowIndex(hComboFocusWnd);
				}else{
					showindex= tabpane;
				}
				if ( showindex >= 0 ){
					groupindex = TabCtrl_GetCurSel(hNotifyWnd); // 先程選択されたタブ
					SelectGroup(showindex, groupindex);
					SortComboWindows(SORTWIN_LAYOUTALL);
				}
				return;
			}

			for ( i = 0 ; i < tabinfo->groupcount; i++ ){
				if ( hNotifyWnd == tabinfo->group[i].hWnd ){
					goto found;
				}
			}
			continue;
		}
		found: // hNotifyWnd は常にタブ。グループタブはタブに変更済み
		if ( !(X_combos[0] & CMBS_TABSEPARATE) ){
			int si;

			si = GetComboShowIndex(hComboFocusWnd);
			if ( si >= 0 ) tabpane = si;
		}
														// 表示中タブにある？
		if ( !sel_sel ) tabindex = TabCtrl_GetCurSel(hNotifyWnd); // 先程選択されたタブ
		if ( tabindex < 0 ) tabindex = 0; // 未選択だった

		tie.mask = TCIF_PARAM;
		if ( IsTrue(TabCtrl_GetItem(hNotifyWnd, tabindex, &tie)) ){
			if ( tabinfo->hWnd != hNotifyWnd ){ // グループ変更の必要あり
				tabinfo->hWnd = hNotifyWnd;
			}
			SelectComboWindow(tabpane, (HWND)tie.lParam, TRUE);
		}
		return;
	}
}

HWND GetHwndFromIDCombo(const TCHAR *regid)
{
	int baseindex;
	PPC_APPINFO *combo_cinfo;

	if ( Combo.hWnd == NULL ) return NULL;
	for ( baseindex = 0 ; baseindex < Combo.BaseCount ; baseindex++ ){
		combo_cinfo = Combo.base[baseindex].cinfo;
		if ( combo_cinfo == NULL ) continue;
		if ( tstricmp(regid, combo_cinfo->RegSubCID) == 0 ){
			return combo_cinfo->info.hWnd;
		}
	}
	return NULL;
}
