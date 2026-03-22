/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library			拡張エディット/tInput
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "PPX.H"
#include "VFS.H"
#include "PPD_DEF.H"
#include "PPD_EDL.H"
#include "VFS_STRU.H"
#pragma hdrstop

int X_calc = -1;
DWORD X_esel[4] = {1, 1, 1, 1};
int X_shortedit = 0;
const TCHAR StrContinuousMenu[] = MES_TREC;
const TCHAR StrRefline[] = MES_TRER;
const TCHAR StrRenConfig[] = MES_MTST;

// return 21B5 21B2 2936 2ba0
// reg 2750
// ok 2713 2714
// cancel 274c 2715 2716
// folder 1f4c1 1f4c2 1f5bf 1f5c0 15fc1
const WCHAR StrEnter[] = {0x21b5, '\0'};
const WCHAR StrCancel[] = {0xd7, '\0'};
const WCHAR StrRef[] = {' ', 0x2750, '&', 'F', '\0'};

typedef struct {
	HWND hEdWnd;
	ThSTRUCT ThMenu;
	TINPUT *tinput;
} TINPUTSTRUCT;

void ChangeRefLine(HWND hDlg)
{
	RECT box;
	HWND hTextWnd = GetDlgItem(hDlg, IDE_INPUT_LINE);

	GetClientRect(hTextWnd, &box);
	if ( X_esel[3] == 0 ) box.bottom = (box.bottom - box.top) / 2;
	SetWindowPos(hTextWnd, NULL, 0, box.bottom, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER);
	ShowDlgWindow(hDlg, IDE_INPUT_REF, X_esel[3]);
}

void ExecPreCommand(TINPUTSTRUCT *ts)
{
	TCHAR buf[CMDLINESIZE], *ptr;

	if ( !(ts->tinput->flag & TIEX_EXECPRECMD) ) return;
	buf[0] = '\0';
	ptr = ThGetLongString(ts->tinput->StringVariable, T("Input_FirstCmd"), buf, CMDLINESIZE);
	if ( ptr != NULL ){
		if ( ptr[0] != '\0' ){
			EditExtractMacro((PPxEDSTRUCT *)GetProp(ts->hEdWnd, StrPPxED), ptr, NULL, 0);
		}
		if ( ptr != buf ) HeapFree(ProcHeap, 0, ptr);
	}
}

void TinputExtractMacro(TINPUTSTRUCT *ts, const TCHAR *param)
{
	EditExtractMacro((PPxEDSTRUCT *)GetProp(ts->hEdWnd, StrPPxED), param, NULL, 0);
}

void ExtPosFix(const TCHAR *filename, int *pos)
{
	TCHAR *entryptr, *ptr;
	int extoffset;

	entryptr = FindLastEntryPoint(filename);
	extoffset = FindExtSeparator(entryptr);
	ptr = entryptr + extoffset;
	if ( (*pos == -3) && (*ptr == '.') ) ptr++;
	*pos = ptr - filename;
}

void FixInputbox(HWND hDlg, HWND hEdWnd, TINPUT *tinput)
{
	HWND hRefWnd, hTreeWnd = NULL;
	RECT ncbox, clientbox, refbox;
	int editwidth, editheight, dpi, ok_width, cancel_width, ref_width;
	PPxEDSTRUCT *PES;

	PES = (PPxEDSTRUCT *)GetProp(hEdWnd, StrPPxED);

	if ( (PES != NULL) && (PES->hTreeWnd != NULL) && (PES->flags & PPXEDIT_JOINTTREE) ){
		hTreeWnd = PES->hTreeWnd;
	}

	GetWindowRect(hEdWnd, &ncbox);
	GetClientRect(hEdWnd, &clientbox);
	dpi = GetMonitorDPI(hDlg);

	hRefWnd = GetDlgItem(hDlg, IDB_REF);
	GetWindowRect(hRefWnd, &refbox);
	refbox.bottom -= refbox.top;
	refbox.top -= ncbox.top;
	refbox.right -= refbox.left;
	ncbox.bottom -= ncbox.top;
	editheight = ncbox.bottom;

	GetClientRect(hDlg, &clientbox);
	editwidth = clientbox.right;

	if ( !(tinput->flag & (TIEX_USEREFLINE | TIEX_USEOPTBTN)) ){
		// ボタンは右
		if ( (TouchMode & TOUCH_LARGEWIDTH) || X_shortedit ){
			refbox.right = CalcMinDpiSize(dpi, X_tous[tous_BUTTONSIZE]);
			if ( X_shortedit >= 2 ){
				ok_width = (X_shortedit == 3) ? 0 : refbox.bottom;
				cancel_width = 0;
				ref_width = (X_shortedit == 4) ? refbox.bottom : 0;
			}else{ // 0, 1
				ok_width = refbox.right;
				cancel_width = refbox.bottom;
				ref_width = 0;
			}
			editwidth -= ok_width + cancel_width + ref_width;
		}else{
			editwidth -= refbox.right * 3;
		}
	}else{
		// ボタンは下
		if ( TouchMode & TOUCH_LARGEWIDTH ){
			int nw = CalcMinDpiSize(dpi, X_tous[tous_BUTTONSIZE]);
			if ( refbox.right < nw ) refbox.right = nw;
		}
	}
	if ( editwidth < 64 ) editwidth = 64;

		// 大きさ変更不要 ?
	if ( (hTreeWnd == NULL) && (editwidth == (ncbox.right - ncbox.left)) ){
		return;
	}
	if ( ((TouchMode & TOUCH_LARGEWIDTH) || X_shortedit ) &&
		!(tinput->flag & (TIEX_USEREFLINE | TIEX_USEOPTBTN)) ){
		HWND hButton;
		// タッチ用ボタン(ボタンは右)
		// Ok
		hButton = GetDlgItem(hDlg, IDOK);
		SetWindowTextW(hButton, StrEnter);
		SetWindowPos(hButton, NULL,
				editwidth, refbox.top,
				ok_width, refbox.bottom,
				SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOZORDER);
		// Cancel
		hButton = GetDlgItem(hDlg, IDCANCEL);
		if ( cancel_width ) SetWindowTextW(hButton, StrCancel);
		SetWindowPos(hButton, NULL,
				editwidth + ok_width, refbox.top,
				cancel_width, refbox.bottom,
				SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOZORDER);
		// Reference
		if ( ref_width ){
//			SetWindowLongPtr(hRefWnd, GWL_STYLE, // 効果が無いので無効化
//					GetWindowLongPtr(hRefWnd, GWL_STYLE) | BS_RIGHT);
			SetWindowTextW(hRefWnd, StrRef);
		}
		SetWindowPos(hRefWnd, NULL,
				editwidth + ok_width + cancel_width, refbox.top,
				ref_width, refbox.bottom,
				SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOZORDER);
	}else{
		int buttonleft;

		if ( tinput->flag & (TIEX_LINE_MULTI | TIEX_LINE_MULTILINE) ){ // 行数が変化することあるので再取得
			GetWindowRect(hRefWnd, &refbox);
			refbox.bottom -= refbox.top;
			refbox.top -= ncbox.top;
			refbox.right -= refbox.left;
		}
		buttonleft = editwidth;
		if ( tinput->flag & (TIEX_USEREFLINE | TIEX_USEOPTBTN) ){ // ボタンは下
			if ( tinput->flag & TIEX_USEREFLINE ){
				RECT reflinebox;
				HWND hEref = GetDlgItem(hDlg, IDE_INPUT_REF);

				GetWindowRect(hEref, &reflinebox);
				reflinebox.bottom -= reflinebox.top;
				SetWindowPos(hEref, NULL, 0, 0, buttonleft, reflinebox.bottom,
					SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
				refbox.top += reflinebox.bottom;
			}
			buttonleft -= refbox.right * 3;
			ncbox.bottom = refbox.top + refbox.bottom + 4; // ツリーY 修正
		}
									// ボタンの位置修正
		SetWindowPos(GetDlgItem(hDlg, IDOK), NULL,
				buttonleft, refbox.top, 0, 0,
				SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDCANCEL), NULL,
				buttonleft + refbox.right, refbox.top, 0, 0,
				SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER);
		SetWindowPos(hRefWnd, NULL,
				buttonleft + refbox.right * 2, refbox.top, 0, 0,
				SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER);
	}

	if ( hTreeWnd != NULL ){
		int treeheight;

		treeheight = (clientbox.bottom - clientbox.top) - ncbox.bottom;
		if ( treeheight < 8 ) treeheight = 8;
		SetWindowPos(hTreeWnd, NULL, 0, 0,
				clientbox.right - clientbox.left, treeheight,
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
	}
	// EditBox の大きさ修正(複数行の時は、ここでDlgの変更が起きることがあり、
	// コントロールの再配置の考慮を不要にするため、最後に修正が必要)
	SetWindowPos(hEdWnd, NULL, 0, 0, editwidth, editheight,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
	InvalidateRect(hDlg, NULL, TRUE);
}

int tInputSeparateExt(HWND hDlg, TCHAR *text)
{
	TCHAR *entryptr, *ptr;
	int extoffset;

	entryptr = FindLastEntryPoint(text);
	extoffset = FindExtSeparator(entryptr);

	ptr = entryptr + extoffset;
	if ( *ptr == '.' ){
		SetDlgItemText(hDlg, IDE_INPUT_EXT, ptr + 1);
		*ptr = '\0';
		SetDlgItemText(hDlg, IDE_INPUT_LINE, text);
	}
	return extoffset;
}

void tInputConnectExt(HWND hDlg, TCHAR *text, int size, BOOL savehist)
{
	TCHAR *ptr;
	DWORD nsize = tstrlen32(text);

	ptr = text + nsize;
	*(ptr + 1) = '\0';
	GetDlgItemText(hDlg, IDE_INPUT_EXT, ptr + 1, size - nsize - 1);
	text[size - 1] = '\0';
	if ( *(ptr + 1) != '\0' ){
		*ptr = '.';
		if ( savehist ) WriteHistory(PPXH_GENERAL, ptr + 1, 0, NULL);
	}
}

void InitCancelLeave(HWND hDlg)
{
	HWND hParentWnd = GetParent(hDlg);

	if ( hParentWnd != NULL ) EnableWindow(hParentWnd, TRUE);
}

void USEFASTCALL tInputInitDialog(HWND hDlg, TINPUTSTRUCT *ts)
{
	TINPUT *tinput;
	RECT ncbox, box, refbox, deskbox;

	SIZE size;
	int width;
	HGDIOBJ hOldFont;
	BOOL fix_size = FALSE;
	TCHAR buf[CMDLINESIZE];

	// ※ ts はもう少し後で DWLP_USER に登録する
	tinput = ts->tinput;
	if ( !(tinput->flag & TIEX_USEINFO) ) tinput->info = NULL;

	ThInit(&ts->ThMenu);

	if ( (tinput->flag & (TIEX_USEPNBTN | TIEX_USEREFLINE)) == TIEX_USEREFLINE ){
		DestroyWindow(GetDlgItem(hDlg, IDB_PREV));
		DestroyWindow(GetDlgItem(hDlg, IDB_NEXT));
	}

	if ( tinput->flag & TIEX_LEAVECANCEL ) InitCancelLeave(hDlg);

	LocalizeDialogText(hDlg, 0);

	ts->hEdWnd = PPxRegistExEdit(tinput->info,
			GetDlgItem(hDlg, IDE_INPUT_LINE),
			tinput->size, tinput->buff, tinput->hRtype, tinput->hWtype,
			PPXEDIT_USEALT | PPXEDIT_JOINTTREE | PPXEDIT_LINEEDIT |
			(tinput->flag & (PPXEDIT_REFTREE | PPXEDIT_SINGLEREF | PPXEDIT_INSTRSEL | PPXEDIT_LINE_MULTI)) |
			((tinput->flag & TIEX_LINE_MULTILINE) ? (PPXEDIT_LINE_MULTILINE | PPXEDIT_WANTALLKEY) : 0) |
			((tinput->flag & TIEX_USEREFLINE) ? 0 : PPXEDIT_TABCOMP) );
	SendMessage(ts->hEdWnd, WM_PPXCOMMAND, KE_setkeya, (LPARAM)StrK_lied);
	EnableTextChangeNotify(ts->hEdWnd);
											// メニュー追加
	if ( IsExistCustData(T("M_edit")) ){
		HMENU hMenu;
		DWORD id;

		id = IDW_MENU;
		hMenu = CreateMenu();
		PP_AddMenu(tinput->info, hDlg, hMenu, &id, T("M_edit"), &ts->ThMenu);
		SetMenu(hDlg, hMenu);
		GetWindowRect(hDlg, &box);
		SetWindowPos(hDlg, NULL, 0, 0,
				box.right - box.left,
				box.bottom - box.top + GetSystemMetrics(SM_CYMENU),
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
		fix_size = TRUE;
	}
									// 文字列の大きさを取得する
	{
		HDC hDC;
		int len;
		#define LINE_MULTI_WIDTH 120

		hDC = GetDC(ts->hEdWnd);
		hOldFont = SelectObject(hDC, (HFONT)SendMessage(ts->hEdWnd, WM_GETFONT, 0, 0));
		len = tstrlen32(tinput->buff);
		if ( (tinput->flag & (TIEX_LINE_MULTI | TIEX_LINE_MULTILINE) ) && (len > LINE_MULTI_WIDTH) ){
			len = LINE_MULTI_WIDTH;
		}
		GetTextExtentPoint32(hDC, tinput->buff, len, &size);
		SelectObject(hDC, hOldFont);
		ReleaseDC(ts->hEdWnd, hDC);
	}
							// edit controlのクライアントサイズを算出
	GetWindowRect(ts->hEdWnd, &ncbox);
	GetClientRect(ts->hEdWnd, &box);
	box.right -= box.left;
	size.cx += 32; // 編集用余白

	width = size.cx + ((ncbox.right - ncbox.left) - box.right); // 文字列幅 + エディットコントロールの枠
								// ダイアログの大きさ修正
	GetWindowRect(hDlg, &ncbox);
	GetDesktopRect(hDlg, &deskbox);
	deskbox.right -= deskbox.left + 16;

	if ( (TouchMode & TOUCH_LARGEWIDTH) || (size.cx > box.right) || ((ncbox.right - ncbox.left) > deskbox.right) ){ // 大きさが足りない・大きすぎなら調整する
		WPARAM wParam;
		LPARAM lParam;

		if ( size.cx > box.right ){
			GetClientRect(hDlg, &box);
			GetWindowRect(GetDlgItem(hDlg, IDB_REF), &refbox);

			// window サイズを算出
			ncbox.right = (width + ((refbox.right - refbox.left) * 3)) +
					(ncbox.right - ncbox.left) - (box.right - box.left);
			if ( ncbox.right > deskbox.right ) ncbox.right = deskbox.right;
		}else if ((ncbox.right - ncbox.left) > deskbox.right){
			ncbox.right = deskbox.right;
		}else{
			ncbox.right -= ncbox.left - 1;
		}

		SetWindowPos(hDlg, NULL, 0, 0, ncbox.right, ncbox.bottom - ncbox.top,
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);

		SendMessage(ts->hEdWnd, EM_GETSEL, (WPARAM)&wParam, (LPARAM)&lParam);
		SendMessage(ts->hEdWnd, EM_SETSEL, 0, 0); // 先頭が表示されるように
		if ( (wParam != 0) || (lParam != 0) ){ // 選択し直し
			SendMessage(ts->hEdWnd, EM_SETSEL, wParam, lParam);
		}
		fix_size = TRUE;
	}
											// Rename 用処理
	if ( tinput->flag & TIEX_USEREFLINE ){
		TCHAR *entryptr;
		int extoffset;


		GetCustData(T("X_esel"), &X_esel, sizeof(X_esel));
		CheckDlgButton(hDlg, IDB_INPUT_EXT, X_esel[2]);
		ShowDlgWindow(hDlg, IDE_INPUT_EXT, !X_esel[2]);
		if ( X_esel[3] == 0 ) ChangeRefLine(hDlg);

		SetDlgItemText(hDlg, IDE_INPUT_REF, tinput->buff);
		PPxRegistExEdit(tinput->info,
				GetDlgItem(hDlg, IDE_INPUT_EXT),
				MAX_PATH, NULL, PPXH_GENERAL, PPXH_GENERAL | PPXH_FILENAME,
				PPXEDIT_USEALT | PPXEDIT_JOINTTREE | PPXEDIT_SINGLEREF);

		entryptr = FindLastEntryPoint(tinput->buff);
		if ( !(tinput->flag & TIEX_REFEXT) ){ // 拡張子無し
			ShowDlgWindow(hDlg, IDB_INPUT_EXT, FALSE);
			ShowDlgWindow(hDlg, IDE_INPUT_EXT, FALSE);
			extoffset = tstrlen32(entryptr);
		}else if ( X_esel[2] == 0 ){ // 拡張子分離
			extoffset = tInputSeparateExt(hDlg, tinput->buff);
		}else{ // 拡張子も編集
			extoffset = FindExtSeparator(entryptr);
		}

		if ( !(tinput->flag & TIEX_USESELECT) ){
			setflag(tinput->flag, TIEX_USESELECT);

			tinput->firstC = entryptr - tinput->buff; // エントリ名先頭
			tinput->lastC = tinput->firstC + extoffset; // 拡張子前
			switch ( X_esel[0] ){ // カーソル位置を決定
				case 0: // 先頭
					if ( X_esel[1] == 0 ) tinput->lastC = tinput->firstC;
					break;

				case 2: // 末尾
					tinput->firstC = X_esel[1] ? tinput->lastC + 1 : EC_LAST;
					tinput->lastC = EC_LAST;
					break;

//				case 1: // 拡張子前
				default:
					if ( X_esel[1] == 0 ) tinput->firstC = tinput->lastC;
			}
		}
	}

	if ( tinput->flag & TIEX_USEOPTBTN ){ // ボタンは下
		ThSTRUCT *thStringValue;
		const TCHAR *var = NULL;

		thStringValue = (ThSTRUCT *)PPxInfoFunc(tinput->info, PPXCMDID_GETWNDVARIABLESTRUCT, NULL);
		if ( thStringValue != NULL ) var = ThGetString(thStringValue, T("Edit_OptionTitle"), buf, CMDLINESIZE);
		if ( var == NULL ) var = ThGetString(NULL, T("Edit_OptionTitle"), buf, CMDLINESIZE);
		if ( var != NULL ) SetDlgItemText(hDlg, IDB_INPUT_OPT, MessageText(buf));
	}

	if ( fix_size || X_shortedit ){ // ダイアログ大きさ調整／ボタンを小さく／隠す
		FixInputbox(hDlg, ts->hEdWnd, ts->tinput);
	}
	// ダイアログ表示位置を調整
	if ( (tinput->info != NULL) &&
		 (tinput->info->RegID != NULL) &&
		 (tinput->info->RegID[0] == 'C') ){
		if ( X_combosD[1] < 0 ){
			GetCustData(T("X_combos"), &X_combosD, sizeof(X_combosD));
		}
		if ( X_combosD[1] & CMBS1_DIALOGNOPANE ){
			CenterWindow(hDlg);
		}else{
			MoveCenterWindow(hDlg, tinput->hOwnerWnd);
		}
	}else{
		CenterWindow(hDlg);
	}

	if ( (tinput->flag & (TIEX_USEREFLINE | TIEX_USEPNBTN)) == (TIEX_USEREFLINE | TIEX_USEPNBTN) ){ // アシストリスト
		PPxEDSTRUCT *PES;
		int itemcount = 0;
		int index = 0;
		TCHAR keyword[CUST_NAME_LENGTH], param[CMDLINESIZE];

		PES = (PPxEDSTRUCT *)GetProp(ts->hEdWnd, StrPPxED);
		while ( EnumCustTable(index++, T("M_path"), keyword, param, sizeof(param)) >= 0 ){
			if ( keyword[0] == '\0' ) continue;
		//	PP_ExtractMacro(NULL, NULL, NULL, keyword, keyword, 0);
			if ( tstrstr(tinput->buff, keyword) != NULL ){
				if ( itemcount == 0 ){
					BackupText(PES->hWnd, PES);
					PES->list.select.start = 0;
					PES->list.select.end = EC_LAST;
					PES->list.mode = LIST_FILL;
					CreateMainListWindow(PES, 1);
				}
				tstrcpy(buf, tinput->buff);
				tstrreplace(buf, keyword, param);
				SendMessage(PES->list.hWnd, LB_ADDSTRING, (WPARAM)0, (LPARAM)buf);
				itemcount++;
			}
		}
		if ( itemcount > 0 ) ShowWindow(PES->list.hWnd, SW_SHOWNA);
#if 0
		{
		/*
			int mov;
			FN_REGEXP fn;

			MakeFN_REGEXP(&fn, param);
			mov = FilenameRegularExpression(line, &fn);
			FreeFN_REGEXP(&fn);
			if ( mov != FRRESULT_NO ){
				PXREGEXPS *rexps;
				if ( IsTrue(InitRegularExpression(&rexps, keyword, SLASH_REQUIRE)) ){
					RegularExpressionReplace(rexps, line;
					FreeRegularExpression(rexps);
				}
			}
		*/
		/*
			PXREGEXPS *rexps;
			if ( IsTrue(InitRegularExpression(&rexps, param, SLASH_REQUIRE)) ){
				RegularExpressionReplace(rexps, line);
				FreeRegularExpression(rexps);
				SendMessage(mainlist.hWnd, mainlist.msg, (WPARAM)mainlist.wParam, (LPARAM)line);
				mainlist.items++;
			}
		*/
		}
#endif
	}

	SetWindowText(hDlg, MessageText(tinput->title));
			// Callback を使用開始(サイズ変更でFixInputbox呼び出さないように)
	SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)ts);
	SetFocus(ts->hEdWnd);
											// 範囲選択
	if ( (tinput->flag & TIEX_USESELECT) &&
		 ( !(tinput->flag & TIEX_INSTRSEL) || (SearchVLINE(tinput->buff) == NULL) ) ){

		if ( tinput->firstC < -1 ) ExtPosFix(tinput->buff, &tinput->firstC);
		if ( tinput->lastC < -1 ) ExtPosFix(tinput->buff, &tinput->lastC);

#ifndef UNICODE
		if ( xpbug < 0 ){ // PPxRegistExEdit 内で初期化済み
			CaretFixToW(tinput->buff, (DWORD *)&tinput->firstC);
			CaretFixToW(tinput->buff, (DWORD *)&tinput->lastC);
		}
#endif
		SendMessage(ts->hEdWnd, EM_SETSEL,
				(WPARAM)tinput->firstC, (LPARAM)tinput->lastC);
	}
											// 参照用ツリー使用の判定
	if ( (tinput->flag & (TIEX_REFTREE | TIEX_SINGLEREF)) ==
				(TIEX_REFTREE | TIEX_SINGLEREF) ){
		DWORD X_rtree;

		X_rtree = GetCustDword(T("X_rtree"), 0);
		if ( X_rtree == 1) X_rtree = !tinput->buff[0];
		if ( X_rtree ) setflag(tinput->flag, TIEX_REFMODE);

		if ( (tinput->buff[0] != '\0') && (GetCustDword(T("X_rclst"), 0) > 0) ){
			PostMessage(ts->hEdWnd, WM_PPXCOMMAND, K_raw | K_s | K_c | '3', 0);
		}
	}

	if ( tinput->flag & TIEX_REFMODE ){
		PostMessage(ts->hEdWnd, WM_PPXCOMMAND, K_raw | K_s | K_c | 'I', 0);
	}
	if ( tinput->flag & TIEX_TOP ) ForceSetForegroundWindow(hDlg);

	SendMessage(ts->hEdWnd, WM_PPXCOMMAND, KE_firstevent_enable, 0);

	if ( tinput->flag & TIEX_EXECPRECMD ){
		PostMessage(hDlg, WM_COMMAND, TMAKEWPARAM(KE_execprecmd, 1), 0);
	}
}

void tInputPathFix(TCHAR *buff)
{
	const TCHAR *src;
	TCHAR *dst;

	src = dst = buff;

	#ifdef UNICODE
		while ( (*src == L'\t') || (*src == L' ') || (*src == L'　') ) src++;
	#else
		while ( (*src == '\t') || (*src == ' ') ) src++;
	#endif
	for ( ;; ){
		TCHAR c;

		c = *src;
		switch ( c ){
			case '\0':
				*dst = '\0';
				goto tailfix;
			case '\t':
				*dst++ = ' ';
				break;
			case '\n':
			case '\r':
				break;
	#ifdef UNICODE
			case '<':
				*dst++ = L'＜';
				break;
			case '>':
				*dst++ = L'＞';
				break;
			case '|':
				*dst++ = L'|';
				break;
			case '\"':
				*dst++ = L'”';
				break;
	#else
			case '<':
			case '>':
			case '|':
			case '\"':
				break;
	#endif
			default:
				*dst++ = c;
				#ifndef UNICODE
					if ( IskanjiA(c) ){
						c = *(src + 1);
						*dst = c;
						if ( c == '\0' ) goto tailfix;
						dst++;
						src += 2;
						continue;
					}
				#endif
		}
		src++;
	}
	tailfix: ;
	for ( ;; ){
		dst--;
		if ( dst < buff ) return;
	#ifdef UNICODE
		if ( (*dst == L' ') || (*dst == L'\t') || (*dst == L'　') ){
			*dst = '\0';
			continue;
		}
	#else
		if ( (*dst == ' ') || (*dst == '\t') ){
			*dst = '\0';
			continue;
		}
	#endif
		if ( *dst == '.' ){
			TCHAR *pdst;

			pdst = dst;
			while ( (pdst > buff) && (*(pdst - 1) == '.' ) ) pdst--;
			if ( pdst > buff ) *pdst = '\0';
			continue;
		}
		break;
	}
}

void tInputContinuousMenu(HWND hDlg, LPARAM lParam)
{
	HMENU hPopupMenu = CreatePopupMenu();
	POINT pos;
	RECT box;
	int index;

	if ( lParam == (LPARAM)0xffffffff ){ // keyによる操作
		GetWindowRect(hDlg, &box);
		pos.x = box.left;
		pos.y = box.bottom;
	}else{ // マウス
		LPARAMtoPOINT(pos, lParam);
	}
	AppendMenuString(hPopupMenu, 1, StrContinuousMenu);
	AppendMenuString(hPopupMenu, 2, StrRefline);
	AppendMenuString(hPopupMenu, 3, StrRenConfig);
	index = TrackPopupMenu(hPopupMenu, TPM_TDEFAULT, pos.x, pos.y, 0, hDlg, NULL);
	DestroyMenu(hPopupMenu);
	switch ( index ){
		case 1:
			EndDialog(hDlg, -(K_c | 'R'));
			break;
		case 2:
			X_esel[3] = !X_esel[3];
			SetCustData(T("X_esel"), &X_esel, sizeof(X_esel));
			ChangeRefLine(hDlg);
			InvalidateRect(hDlg,NULL,TRUE);
			break;
		case 3:
			PP_ExtractMacro(hDlg, NULL, NULL, T("*ppcust -:X_esel"), NULL, 0);
			break;
	}
}

void USEFASTCALL SaveInputText(HWND hDlg, TINPUTSTRUCT *ts, INT_PTR result)
{
	TINPUT *tinput = ts->tinput;

	tinput->buff[0] = '\0';
	GetWindowText(ts->hEdWnd, tinput->buff, tinput->size);
	tinput->buff[tinput->size - 1] = '\0';
	if ( tinput->hWtype ) WriteHistory(tinput->hWtype, tinput->buff, 0, NULL);
	if ( tinput->flag & TIEX_USEREFLINE ){
		tInputConnectExt(hDlg, tinput->buff, tinput->size, TRUE);
	}
	if ( tinput->flag & TIEX_FIXFORPATH ) tInputPathFix(tinput->buff);
	if ( tinput->flag & TIEX_FIXFORDIGIT ) Strds(tinput->buff, tinput->buff);
	EndDialog(hDlg, result);
}

void tInputCalc(HWND hDlg, TINPUTSTRUCT *ts)
{
	TCHAR buf1[VFPS], buf2[GetCalc_ResultMaxLength], *ptr;
	int result;

	buf1[0] = '\0';
	GetWindowText(ts->hEdWnd, buf1, TSIZEOF(buf1));
	buf1[VFPS - 1] = '\0';

	ptr = NULL;
	if ( ts->tinput->hWtype == PPXH_FILENAME ){ // 長さの警告表示
		int lenName = tstrlen32(buf1), lenPath = 0;

		if ( ts->tinput->info != NULL ){
			PPXCMDENUMSTRUCT work;

			PPxEnumInfoFunc(ts->tinput->info, '1', buf1, &work);
			lenPath = tstrlen32(buf1) + lenName + 1;
		}
		if ( (lenName >= 140) || (lenPath >= MAX_PATH - 56) ){
			ptr = buf2;
			thprintf(buf2, TSIZEOF(buf2), T("length:%c%d, path length:%c%d"),
					(lenName >= 255) ? '#' : ' ',
					lenName,
					(lenPath >= MAX_PATH) ? '#' : ' ',
					lenPath);
		}
	}
	if ( (ptr == NULL) && IsTrue(GetCalc(buf1, buf2, &result)) ) ptr = buf2;
	SetMessageOnCaption(hDlg, ptr);
}

void ChangeExtEditMode(HWND hDlg, HWND hBtnWnd)
{
	TCHAR buf[CMDLINESIZE];
	int len = GetWindowTextLength(GetDlgItem(hDlg, IDE_INPUT_EXT));

	if ( len >= CMDLINESIZE ) return;
	GetDlgItemText(hDlg, IDE_INPUT_LINE, buf, CMDLINESIZE);
	if ( SendMessage(hBtnWnd, BM_GETCHECK, 0, 0) ){ // 拡張子編集有効
		if ( len == 0 ) return;
		tInputConnectExt(hDlg, buf, CMDLINESIZE, FALSE);
		SetDlgItemText(hDlg, IDE_INPUT_LINE, buf);
		SetDlgItemText(hDlg, IDE_INPUT_EXT, NilStr);
		X_esel[2] = 1;
	}else{ // 無効
		if ( len != 0 ) return;
		tInputSeparateExt(hDlg, buf);
		X_esel[2] = 0;
	}
	SetCustData(T("X_esel"), &X_esel, sizeof(X_esel));
	ShowDlgWindow(hDlg, IDE_INPUT_EXT, !X_esel[2]);
}

INT_PTR tInputPPxCommand(HWND hDlg, TINPUTSTRUCT *ts, WPARAM wParam, LPARAM lParam)
{
	switch (wParam){
		case KE_setLeaveCancel:
			setflag(ts->tinput->flag, TIEX_LEAVECANCEL);
			InitCancelLeave(hDlg);
			return NO_ERROR;

		case KE_setWType:
			ts->tinput->hWtype = (WORD)lParam;
			return NO_ERROR;
	}
	return ERROR_INVALID_FUNCTION;
}

/*-----------------------------------------------------------------------------
	一行入力用ダイアログボックス/コールバック
-----------------------------------------------------------------------------*/
INT_PTR CALLBACK tInputMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TINPUTSTRUCT *ts;

	ts = (TINPUTSTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER);
	if ( ts == NULL ){
		if ( message == WM_INITDIALOG ){
			tInputInitDialog(hDlg, (TINPUTSTRUCT *)lParam);
		}
		return FALSE;
	}
	switch (message){
		case WM_ACTIVATE:
			if ( (ts->tinput->flag & TIEX_LEAVECANCEL) &&
				 (LOWORD(wParam) == WA_INACTIVE) &&
				 (GetParent((HWND)lParam) != hDlg) ) // list を除外
			{
				PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
			}
			return FALSE;

		case WM_DESTROY:
			ThFree(&ts->ThMenu);
			break;

		case WM_SIZE:
			if ( wParam != SIZE_MINIMIZED ){
				FixInputbox(hDlg, ts->hEdWnd, ts->tinput);
			}
			return FALSE;

		case WM_CONTEXTMENU:
			if ( (GetDlgCtrlID((HWND)wParam) == IDOK) || (GetDlgCtrlID((HWND)wParam) == IDCANCEL) ){
				SendMessage(ts->hEdWnd, WM_PPXCOMMAND, KE_RefButton, 0);
			}else if ( ts->tinput->flag & TIEX_USEPNBTN ){
				tInputContinuousMenu(hDlg, lParam);
			}
			return FALSE;

		case WM_NCHITTEST: { // サイズ変更を左右のみに制限
			LRESULT result = DefWindowProc(hDlg, WM_NCHITTEST, wParam, lParam);

			switch ( result ){
				case HTTOP:
				case HTBOTTOM: {
					PPxEDSTRUCT *PES;

					PES = (PPxEDSTRUCT *)GetProp(ts->hEdWnd, StrPPxED);
					if ( (PES == NULL) || (PES->hTreeWnd == NULL) || !(PES->flags & PPXEDIT_JOINTTREE) ){
						result = HTBORDER;
					}
					break;
				}

				case HTTOPLEFT:
				case HTBOTTOMLEFT:
					result = HTLEFT;
					break;

				case HTTOPRIGHT:
				case HTBOTTOMRIGHT:
					result = HTRIGHT;
					break;
			}
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
			break;
		}

		case WM_COMMAND:
			if ( HIWORD(wParam) == 0 ){	// メニュー
				if ( LOWORD(wParam) >= IDW_MENU ){
					const TCHAR *ptr;

					ptr = (TCHAR *)ts->ThMenu.bottom;
					if ( ptr != NULL ){
						ptr = GetMenuDataString(&ts->ThMenu,
								LOWORD(wParam) - IDW_MENU);
						if ( *ptr != '\0' ){
							TinputExtractMacro(ts, ptr/*, NULL, 0*/);
						}
					}
				}
			}
			switch ( LOWORD(wParam) ){
				case KE_execprecmd:
					ExecPreCommand(ts);
					break;

				case IDOK:
					SaveInputText(hDlg, ts, 1);
					break;

				case IDB_PREV:
				case IDB_NEXT:
					SaveInputText( hDlg, ts, LOWORD(wParam) );
					break;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;

				case IDB_REF:
					SendMessage(ts->hEdWnd, WM_PPXCOMMAND,
							KE_RefButton, 0);
					break;

				case IDB_INPUT_EXT:
					ChangeExtEditMode(hDlg, (HWND)lParam);
					break;

				case IDE_INPUT_LINE:
					if ( HIWORD(wParam) != EN_CHANGE ) break;
					if ( X_calc < 0 ){
						GetCustData(T("X_calc"), &X_calc, sizeof X_calc);
					}
					if ( X_calc ) tInputCalc(hDlg, ts);
					if ( ts->tinput->flag & TIEX_NCHANGE ){
						SendMessage(ts->tinput->hOwnerWnd, WM_PPXCOMMAND, K_EDITCHANGE, lParam);
					}
					break;

				case IDB_INPUT_OPT:
					TinputExtractMacro(ts,
							T("*execute %si,\"Edit_OptionCmd\"")/*, NULL, 0*/);
					break;
			}
			break;

/*
		case WM_CLOSE:
			EndDialog(hDlg, 0);
			break;
*/
		case WM_SYSCOMMAND:
			if ( wParam == SC_KEYMENU ){
				SendMessage(ts->hEdWnd, WM_PPXCOMMAND,
						(WPARAM)(upper((TCHAR)lParam) | GetShiftKey()), 0);
				break;
			}
			// default:
		default:
			if ( message == WM_PPXCOMMAND ){
				return tInputPPxCommand(hDlg, ts, wParam, lParam);
			}
			return PPxDialogHelper(hDlg, message, wParam, lParam);
	}
	return TRUE;
}

/*-----------------------------------------------------------------------------
	一行入力用ダイアログボックス
-----------------------------------------------------------------------------*/
PPXDLL int PPXAPI tInput(HWND hWnd, const TCHAR *title, TCHAR *string, int maxlen, WORD readhist, WORD writehist)
{
	TINPUT tinput;

	tinput.hOwnerWnd= hWnd;
	tinput.hRtype	= readhist;
	tinput.hWtype	= writehist;
	tinput.title	= title;
	tinput.buff		= string;
	tinput.size		= maxlen;
	tinput.flag		= 0;
	if ( writehist == PPXH_DIR ){
		setflag(tinput.flag, TIEX_REFTREE | TIEX_SINGLEREF);
	}else if ( writehist == PPXH_NUMBER ){
		setflag(tinput.flag, TIEX_FIXFORDIGIT);
	}else if ( writehist & (PPXH_NUMBER | PPXH_FILENAME | PPXH_PATH | PPXH_SEARCH | PPXH_MASK) ){
		setflag(tinput.flag, TIEX_SINGLEREF);
	}
	return tInputEx(&tinput);
}

BOOL tInputConsoleHandler(DWORD type)
{
	if ( type == CTRL_C_EVENT ){
		PrintToConsole(T("**cancel**"));
		return TRUE;
	}
	return FALSE;
}

#ifndef ENABLE_INSERT_MODE
	#define ENABLE_INSERT_MODE 0x0020
	#define ENABLE_QUICK_EDIT_MODE 0x0040
	#define ENABLE_EXTENDED_FLAGS 0x0080
#endif
#ifndef ENABLE_AUTO_POSITION
	#define ENABLE_AUTO_POSITION 0x0100
#endif


int tInputConsole(TINPUT *tinput)
{
	DWORD size, oldin;
	int result;

	DSetConsoleCtrlHandler((PHANDLER_ROUTINE)tInputConsoleHandler, TRUE);
	DGetConsoleMode(hConsoleStdin, &oldin);
	DSetConsoleMode(hConsoleStdin, ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS | ENABLE_AUTO_POSITION);

	PrintToConsole(tinput->title);
	PrintToConsole(T(">"));
	if ( FALSE == DReadConsole(hConsoleStdin, tinput->buff, (tinput->size - 1) * sizeof(TCHAR), &size, NULL) ){
		result = -1;
		tinput->buff[0] = '\0';
	}else{
		TCHAR *ptr;

		tinput->buff[size] = '\0';
		ptr = tstrchr(tinput->buff, '\3');
		if ( ptr != NULL ){
			result = -1;
		}else{
			ptr = tstrchr(tinput->buff, '\r');
			if ( ptr != NULL ) *ptr = '\0';
			result = 1;
		}
	}
//	XMessage(NULL, NULL, XM_DbgDIA, T("%s (%d,%d)"), tinput->buff, result, tstrlen(tinput->buff));
	DSetConsoleMode(hConsoleStdin, oldin);
	DSetConsoleCtrlHandler((PHANDLER_ROUTINE)tInputConsoleHandler, FALSE);
	return result;
}

/*-----------------------------------------------------------------------------
	一行入力用ダイアログボックス(拡張版)
-----------------------------------------------------------------------------*/
PPXDLL int PPXAPI tInputEx(TINPUT *tinput)
{
	int result;
	UINT dialogid;
	TINPUTSTRUCT ts;

	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		return tInputConsole(tinput);
	}

	InitSysColors();
	ts.tinput = tinput;
	if ( tinput->flag & (TIEX_USEREFLINE | TIEX_USEOPTBTN) ){
		dialogid = (tinput->flag & TIEX_USEREFLINE) ? IDD_INPUTREF : IDD_INPUT_OPT;
	}else{
		dialogid = IDD_INPUT;
	}
	GetCustTable(StrCustOthers, T("shortedit"), &X_shortedit, sizeof(X_shortedit));
	X_shortedit = Isdigit(X_shortedit) ? ((X_shortedit & 0xff) - '0') : 0;

	if ( (GetCustDword(T("X_mled"), 0) != 0) || (tinput->flag & TIEX_LINE_MULTILINE) ){ // 複数行の時は、IDE_INPUT_LINE の style に ES_MULTILINE を追加、ES_AUTOHSCROLL を削除
		DLGTEMPLATE *dialog;
		WORD *dlgptr, *dlgmax;

		dialog = GetDialogTemplate(tinput->hOwnerWnd, DLLhInst, MAKEINTRESOURCE(dialogid));
		if ( dialog == NULL ) return -1;

		if ( !(tinput->flag & TIEX_LINE_MULTILINE) ){
			setflag(tinput->flag, TIEX_LINE_MULTI);
		}

		dlgptr = (WORD *)dialog;
		dlgmax = dlgptr + ((HeapSize(ProcHeap, 0, dlgptr) - sizeof(DLGTEMPLATE) - sizeof(DLGITEMTEMPLATE)) / sizeof(WORD));
		for ( ; dlgptr < dlgmax ; dlgptr++ ){ // IDE_INPUT_LINE を検索、修正
			if ( (((DLGITEMTEMPLATE *)dlgptr)->style == IDE_INPUT_LINE_STYLE) &&
				 (((DLGITEMTEMPLATE *)dlgptr)->id == IDE_INPUT_LINE) ){
				((DLGITEMTEMPLATE *)dlgptr)->style = IDE_INPUT_LINE_MULTI_STYLE;
				break;
			}
		}
		result = DialogBoxIndirectParam(DLLhInst, dialog,
				tinput->hOwnerWnd, tInputMain, (LPARAM)&ts);
		HeapFree(ProcHeap, 0, dialog);
	}else{
		result = PPxDialogBoxParam(DLLhInst, MAKEINTRESOURCE(dialogid),
				tinput->hOwnerWnd, tInputMain, (LPARAM)&ts);
	}
	SetIMEDefaultStatus(tinput->hOwnerWnd);
	return result;
}

void ClosePPeTreeWindow(PPxEDSTRUCT *PES)
{
	if ( PES->hTreeWnd == NULL ) return;

	if ( PES->flags & PPXEDIT_JOINTTREE ){ // 親と一体
		HWND hParent;
		RECT parentbox, treebox;

		SetFocus(PES->hWnd);
		hParent = GetParentCaptionWindow(PES->hWnd);
		GetWindowRect(hParent, &parentbox);
		GetWindowRect(PES->hTreeWnd, &treebox);
		DestroyWindow(PES->hTreeWnd);
		SetWindowPos(hParent, NULL, 0, 0,
				parentbox.right - parentbox.left,
				parentbox.bottom - parentbox.top - (treebox.bottom - treebox.top),
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}else{
		DestroyWindow(PES->hTreeWnd);
	}
	PES->hTreeWnd = NULL;
}

// Tree 処理用 ----------------------------------------------------------------
void PPeTreeWindow(PPxEDSTRUCT *PES)
{
	HWND hParent;
	RECT box, deskbox;
	PPXCMDENUMSTRUCT work;
	TCHAR path[VFPS];
	HWND hWnd = PES->hWnd;
	DWORD X_tree[3];
	#define expand_height X_tree[2]

	if ( PES->hTreeWnd != NULL ){
		ClosePPeTreeWindow(PES);
		return;
	}
	InitVFSTree();

	hParent = GetParentCaptionWindow(hWnd);
	GetWindowRect(hParent, &box);
	GetDesktopRect(hParent, &deskbox);

	X_tree[1] = X_tree[2] = (TREEHEIGHT * GetMonitorDPI(hWnd)) / DEFAULT_WIN_DPI;
	GetCustData(StrX_tree, X_tree, sizeof(X_tree));
	if ( PES->flags & PPXEDIT_JOINTTREE ) X_tree[2] = X_tree[1];  // 親と一体

	if ( expand_height < 64 ){
		expand_height = (expand_height < 32) ? TREEHEIGHT : 64;
	}

	if ( (box.bottom + (int)expand_height) > deskbox.bottom ) {
		// 上にずらして画面内に納める
		int diff = box.bottom + (int)expand_height - deskbox.bottom;

		if ( (box.top >= deskbox.top) && ((box.top - diff) < deskbox.top) ){
			diff = box.top - deskbox.top;
		}
		if ( diff > 0 ){
			box.top -= diff;
			box.bottom -= diff;

			SetWindowPos(hParent, NULL, box.left, box.top, 0, 0,
					SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
		}
		// 高さを減らして画面内に納める
		if ( (box.bottom + (int)expand_height) > deskbox.bottom ) {
			expand_height = deskbox.bottom - box.bottom;
			if ( expand_height < 100 ) expand_height = 100;
		}
	}

	if ( PES->flags & PPXEDIT_JOINTTREE ){
		SetWindowPos(hParent, NULL, 0, 0,
				box.right - box.left, box.bottom - box.top + expand_height,
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

		GetClientRect(hParent, &box);

		PES->hTreeWnd = CreateWindow(TreeClassStr, MessageText(MES_TSDR),
				WS_CHILD | WS_TABSTOP,
				0, box.bottom - (int)expand_height,
				box.right - box.left, (int)expand_height,
				hParent, CHILDWNDID(IDT_INPUT_TREE), NULL, 0);
	}else{
		PES->hTreeWnd = CreateWindow(TreeClassStr, MessageText(MES_TSDR),
				WS_OVERLAPPEDWINDOW,
				box.left, box.bottom,
				box.right - box.left, (int)expand_height, hParent, NULL, NULL, 0);
	}
	SendMessage(PES->hTreeWnd, VTM_SETFLAG,
			(WPARAM)hWnd, (LPARAM)(VFSTREE_SELECT | VFSTREE_PATHNOTIFY));

	if ( PES->flags & PPXEDIT_SINGLEREF ){
		GetWindowText(hWnd, path, TSIZEOF(path));
		path[TSIZEOF(path) - 1] = '\0';
	}else{
		path[0] = '\0';
	}
	if ( path[0] == '\0' ) PPxEnumInfoFunc(PES->info, '1', path, &work);
	SendMessage(PES->hTreeWnd, VTM_INITTREE, 0, (LPARAM)path);

	ShowWindow(PES->hTreeWnd, SW_SHOWNORMAL);
	SetFocus(PES->hTreeWnd);
}
