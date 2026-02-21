/*-----------------------------------------------------------------------------
	Paper Plane cUI											自動 D&D 処理
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPC_DD.H"
#include "PPCUI.RH"
#pragma hdrstop

typedef struct {
	PPC_APPINFO *cinfo;
	HWND hTargetWnd;
	DWORD droptype;
} DDTHREADSTRUCT;

typedef struct {
	struct tagPPC_APPINFO *cinfo;
	HWND hTargetWnd;
	TCHAR src[VFPS];
	FN_REGEXP mask_title;
	FN_REGEXP mask_filename;
	DWORD droptype;
} AUTODDLIST;

typedef struct {
	HTREEITEM hParent; // 親
	TV_ITEM tvi; // 親登録用
	AUTODDLIST *adl;
} SETLISTSTRUCT;

const TCHAR ADDThreadName[] = T("Auto D&D");
const TCHAR ADDPROP[] = T("PPXADD");
const TCHAR OLDDnDPropName[] = T("OleDropTargetInterface");
const TCHAR WndTreeName[] = T("PPxWindowsTree");
const TCHAR FrameClassName[] = T("ADDFrame");

HMODULE hPSAPI = INVALID_HANDLE_VALUE;
DefineWinAPI(DWORD, GetModuleFileNameEx, (HANDLE, HMODULE, LPTSTR, DWORD)) = NULL;

BOOL CALLBACK ListChildWindows(HWND hWnd, LPARAM lParam);
void ChangeDDtype(AUTODDLIST *adl, DWORD type, DWORD toggle);

#define WLP_SOURCE 0 // sizeof(DWORD_PTR)

#define ADDMENU_POST 1
#define ADDMENU_COM_HOOK 2
#define ADDMENU_COM_EMULATE 3
#define ADDMENU_RIGHT 4
#define ADDMENU_COPY 5
#define ADDMENU_MOVE 6

// マウス操作エミュレート関連 -------------------------------------------------
void USEFASTCALL SetMouseCursor(int x, int y)
{
	LONG_PTR dispX, dispY;

	dispX = GetSystemMetrics(SM_CXSCREEN);
	dispY = GetSystemMetrics(SM_CYSCREEN);

	mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
				(x * 65536 + dispX - 1) / dispX,
				(y * 65536 + dispY - 1) / dispY, 0, 0);
}

BOOL CheckChildWndHandles(HWND hPointedWnd, HWND hTargetWnd)
{
	for (;;){
		if ( hPointedWnd == hTargetWnd ) return TRUE;
		hPointedWnd = GetParent(hPointedWnd);
		if ( hPointedWnd == NULL ) return FALSE;
	}
}

THREADEXRET THREADEXCALL AutoDDThread(DDTHREADSTRUCT *dts)
{
	POINT pos, cpos = {0, 0};
	RECT box;
	int wait, pushbutton = 0;
	int deltaNo;
	THREADSTRUCT threadstruct = {ADDThreadName, XTHREAD_TERMENABLE, NULL, 0, 0};
	DWORD droptype, ButtonDown, ButtonUp;
	PPC_APPINFO *cinfo;
	HWND hTargetWnd;

	cinfo = dts->cinfo;
	hTargetWnd = dts->hTargetWnd;
	droptype = dts->droptype;
	PPcHeapFree(dts);

	PPxRegisterThread(&threadstruct);

	if ( droptype & DROPTYPE_RIGHT ){
		ButtonDown = MOUSEEVENTF_RIGHTDOWN;
		ButtonUp = MOUSEEVENTF_RIGHTUP;
	}else{
		ButtonDown = MOUSEEVENTF_LEFTDOWN;
		ButtonUp = MOUSEEVENTF_LEFTUP;
	}

	deltaNo = cinfo->e.cellN - cinfo->cellWMin;
	pos.x = CalcCellX(cinfo, deltaNo) + cinfo->fontX * 3;
	pos.y = CalcCellY(cinfo, deltaNo) + 2;
	ClientToScreen(cinfo->info.hWnd, &pos);

	wait = ( WindowFromPoint(pos) == cinfo->info.hWnd ) ? 1 : 0;

	if ( wait != 0 ){
											// クリック
		mouse_event(ButtonDown, 0, 0, 0, 0);
		for ( wait = 20 ; wait ; wait-- ){
			if ( cinfo->MouseStat.PushButton > MOUSEBUTTON_CANCEL ) break;
			Sleep(100);
		}
		pushbutton = 1;
											// ドラッグ開始
		pos.x += 10;
		SetMouseCursor(pos.x, pos.y);
		Sleep(100);
											// 対象アプリケーションオープン
		for ( wait = 20 ; wait ; wait-- ){
			if ( IsIconic(hTargetWnd) ) ShowWindow(hTargetWnd, SW_RESTORE);
			SetForegroundWindow(hTargetWnd);
			Sleep(100);
			if ( GetForegroundWindow() == hTargetWnd ) break;
		}
	}
									// 対象アプリケーションのタイトルバーへ
	if ( droptype & DROPTYPE_FOTYPE_MASK ){
		keybd_event((BYTE)((droptype & DROPTYPE_MOVE) ? VK_SHIFT : VK_CONTROL), 0, 0, 0);
	}

	GetWindowRect(hTargetWnd, &box);
	ClientToScreen(hTargetWnd, &cpos);
	pos.x = (box.left + box.right) / 2 - 1;
	pos.y = box.top + (((cpos.y - box.top) * 2) / 5) + 5;
	if ( wait ) for ( wait = 20 ; wait ; wait-- ){
		HWND hPointWnd;

		SetMouseCursor(pos.x, pos.y);
		Sleep(100);
		hPointWnd = WindowFromPoint(pos);
		if ( CheckChildWndHandles(hPointWnd, hTargetWnd) ) break;
		pos.x++;
		pos.y++;
	}
									// タイトルバー上でクリック解除
	if ( wait ){
		mouse_event(ButtonUp, 0, 0, 0, 0); // ボタン↑
	}else{ // タイムアウト
		if ( pushbutton ){
			keybd_event(VK_ESCAPE, 0, 0, 0);
			keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0);
			Sleep(100);
			mouse_event(ButtonUp, 0, 0, 0, 0); // ボタン↑
		}
		SetPopMsg(cinfo, POPMSG_MSG, MES_EDAD);
	}
	if ( droptype & DROPTYPE_FOTYPE_MASK ){
		keybd_event((BYTE)((droptype & DROPTYPE_MOVE) ? VK_SHIFT : VK_CONTROL), 0, KEYEVENTF_KEYUP, 0);
	}

	PPcAppInfo_Release(cinfo);
	PPxUnRegisterThread();
	t_endthreadex(TRUE);
}

HGLOBAL FileToHDrop(const TCHAR *filename, const TCHAR *path)
{
	TCHAR buf[VFPS];
	TMS_struct files = {{NULL, 0, NULL}, 0};
	DROPFILES *dp;

	if ( path[0] == '?' ) return NULL;

	TMS_reset(&files);
	if ( TM_check(&files.tm, sizeof(DROPFILES)) == FALSE ) goto memerror;
	#pragma warning(suppress: 6011) // TM_check でチェック済み
	dp = (DROPFILES *)((BYTE *)files.tm.p + files.p);
	dp->pFiles = sizeof(DROPFILES);
	dp->pt.x = 0;
	dp->pt.y = 0;
	dp->fNC = FALSE;
	files.p = sizeof(DROPFILES);

#ifndef UNICODE
	if ( OSver.dwPlatformId != VER_PLATFORM_WIN32_NT ){
		dp->fWide = FALSE;	// OEM charset
		VFSFullPath(buf, (TCHAR *)filename, path);
		TMS_set(&files, buf);
		TMS_set(&files, "");
	}else
#endif
	{
		dp->fWide = TRUE;	// UNICODE
		{
			int l;
#ifndef UNICODE
			OLECHAR olePath[VFPS];

			VFSFullPath(buf, (TCHAR *)filename, path);
			l = MultiByteToWideChar(CP_ACP, 0, buf, -1,
					olePath, VFPS) * sizeof(WCHAR);
			if ( TM_check(&files.tm, files.p + l + sizeof(WCHAR)) == FALSE ){
				goto memerror;
			}
			memcpy((char *)files.tm.p + files.p, olePath, l);
#else
			VFSFullPath(buf, (TCHAR *)filename, path);
			l = TSTRSIZE32(buf);
			if ( TM_check(&files.tm, files.p + l + sizeof(WCHAR)) == FALSE ){
				goto memerror;
			}
			memcpy((char *)files.tm.p + files.p, buf, l);
#endif
			files.p += l;
		}
		*(WCHAR *)((char *)files.tm.p + files.p) = '\0';
		files.p += 2;
	}

	TMS_off(&files);
	#pragma warning(suppress: 6387) // 既に使用している
	files.tm.h = GlobalReAlloc(files.tm.h, files.p, GMEM_MOVEABLE);
	return files.tm.h;
memerror:
	TMS_kill(&files);
	return NULL;
}

const TCHAR WC_RICHEDITHEAD1[] = T("RICHEDIT");
const TCHAR WC_RICHEDITHEAD2[] = T("RichEdit");
BOOL IsEditableClass(const TCHAR *classname)
{
	return ( (tstricmp(classname, WC_EDIT) == 0) ||
		 (memcmp(classname, WC_RICHEDITHEAD1, 8 * sizeof(TCHAR)) == 0) ||
		 (memcmp(classname, WC_RICHEDITHEAD2, 8 * sizeof(TCHAR)) == 0) );
}

// D&D の操作を行う -----------------------------------------------------------
ERRORCODE StartAutoDD(PPC_APPINFO *cinfo, HWND hTargetWnd, const TCHAR *src, DWORD droptype)
{
	TCHAR buf[VFPS];

	if ( droptype & DROPTYPE_DIALOG ){
		MSG msgs;

		if ( cinfo->dds.hTargetWnd != NULL ){
			ShowWindow(cinfo->dds.hTargetWnd, SW_HIDE);
		}
		ShowWindow(cinfo->dds.hTreeWnd, SW_HIDE);

		// キーボードバッファをクリア
		while ( PeekMessage(&msgs, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE) );
	}

	if ( (src != NULL) && (src[0] != '\0') ){
		// File Manager(Win3.1) 形式 D&D
		if ( !(droptype & DROPTYPE_COM) && (GetWindowLongPtr(hTargetWnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES) ){
			HGLOBAL hG;

			SetForegroundWindow(hTargetWnd);
			hG = FileToHDrop(src, cinfo->RealPath);
			if ( hG == NULL ) return ERROR_INVALID_DATA;
			PostMessage(hTargetWnd, WM_DROPFILES, (WPARAM)hG, 0);
//			GlobalFree(hG);	←不要なので入れる必要なし
			return NO_ERROR;
		}
		return AutoDD_UseDLL(cinfo, hTargetWnd, src, droptype) ?
				NO_ERROR : ERROR_CAN_NOT_COMPLETE;
	}

	if ( !cinfo->e.markC && (CEL(cinfo->e.cellN).type <= ECT_NOFILEDIRMAX) ){
		return ERROR_FILE_NOT_FOUND;
	}

	buf[0] = '\0';
	GetClassName(hTargetWnd, buf, TSIZEOF(buf));

	// エディットボックスにテキスト貼り付け
	if ( IsEditableClass(buf) ){
		VFSFullPath(buf,
			CellFileNameIndex(cinfo, cinfo->e.markC ? cinfo->e.markTop : cinfo->e.cellN),
			cinfo->RealPath);
		SendMessage(hTargetWnd, WM_SETTEXT, 0, (LPARAM)buf);
		SetForegroundWindow(hTargetWnd);
		return NO_ERROR;
	}
	// File Manager(Win3.1) 形式 D&D
	if ( !(droptype & (DROPTYPE_COM | DROPTYPE_MOVE)) && (GetWindowLongPtr(hTargetWnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES) ){
		HGLOBAL hG;

		SetForegroundWindow(hTargetWnd);
		hG = CreateHDrop(cinfo);
		if ( hG == NULL ) return ERROR_INVALID_DATA;
		PostMessage(hTargetWnd, WM_DROPFILES, (WPARAM)hG, 0);
//		GlobalFree(hG);	←不要なので入れる必要なし
		return NO_ERROR;
	}
	// OLE D&D
	{
		HANDLE hThread;
		DWORD tmp;
		POINT pos;
		int wait;
		DDTHREADSTRUCT *dts;

		if ( droptype & DROPTYPE_HOOK ){
			if ( IsTrue(AutoDD_UseDLL(cinfo, hTargetWnd, src, droptype)) ){
				return NO_ERROR; // 直接 D&D に成功
			}
		}
		// マウス操作エミュレートで D&D を開始
		pos.x = CalcCellX(cinfo, cinfo->e.cellN - cinfo->cellWMin) + cinfo->fontX * 3;
		pos.y = CalcCellY(cinfo, cinfo->e.cellN - cinfo->cellWMin) + 2;
		ClientToScreen(cinfo->info.hWnd, &pos);

		for ( wait = 20 ; wait ; wait-- ){
			SetForegroundWindow(cinfo->info.hWnd);
			SetMouseCursor(pos.x, pos.y);
			PeekLoop();
			Sleep(100);
			if ( WindowFromPoint(pos) != cinfo->info.hWnd ) continue;
			Sleep(100);
			if ( WindowFromPoint(pos) == cinfo->info.hWnd ) break;
		}

		dts = PPcHeapAlloc(sizeof(DDTHREADSTRUCT));
		dts->cinfo = cinfo;
		dts->hTargetWnd = hTargetWnd;
		dts->droptype = droptype;
		PPcAppInfo_AddRef(cinfo);
		hThread = t_beginthreadex(NULL, 0, (THREADEXFUNC)AutoDDThread, dts, 0, &tmp);
		if ( hThread != NULL ){
			CloseHandle(hThread);
		}else{
			PPcAppInfo_Release(cinfo);
		}
	}
	return NO_ERROR;
}

// D&D 先一覧選択 -------------------------------------------------------------
BOOL ListWindowItem(HWND hWnd, SETLISTSTRUCT *parent, HTREEITEM hParentitem)
{
	int index;
	HICON hWndIcon;
	TV_INSERTSTRUCT tvins;

	if ( hParentitem == TVI_ROOT ){
		// ファイル名を取得
		if ( DGetModuleFileNameEx != NULL ){
			HANDLE hProcess;
			DWORD ProcessID = 0xffffffff;
			TCHAR text[0x1000], *name, *p;

			text[0] = '\0';
			GetWindowThreadProcessId(hWnd, &ProcessID);
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					FALSE, ProcessID);
			if ( hProcess != NULL ){
				if ( DGetModuleFileNameEx(hProcess, NULL, text, VFPS) == 0 ){
					text[0] = '\0';
				}
				CloseHandle(hProcess);
			}
			if ( text[0] != '\0' ){
				name = FindLastEntryPoint(text);
				p = name + FindExtSeparator(name);

				*p = '\0';
				if ( FilenameRegularExpression(name, &parent->adl->mask_filename) == FALSE ){
					return FALSE;
				}

				*p++ = ':';
				*p++ = ' ';

				tstrcpy(p, parent->tvi.pszText);
				parent->tvi.pszText = name;
			}
		}

		// アイコンを取得
		if ( parent->tvi.iImage == 0 ){
			hWndIcon = (HICON)SendMessage(hWnd, WM_GETICON, 0, 0);
			if ( hWndIcon == NULL ){
				hWndIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
			}
			index = DImageList_AddIcon(parent->adl->cinfo->dds.hTreeImage, hWndIcon);
			if ( index != -1 ){
				parent->tvi.iImage = parent->tvi.iSelectedImage = index;
			}
		}
	}else{ // 子ウィンドウ
		DWORD id;

		id = GetWindowLongPtr(hWnd, GWLP_ID);
		if ( id != 0 ){
			TCHAR *p;

			p = parent->tvi.pszText + tstrlen(parent->tvi.pszText);
			thprintf(p, 16, T("(%d)"), (int)GetWindowLongPtr(hWnd, GWLP_ID));
		}
	}
	parent->tvi.cchTextMax = tstrlen32(parent->tvi.pszText);
	tvins.hParent = hParentitem;
	tvins.hInsertAfter = TVI_SORT;
	TreeInsertItemValue(tvins) = parent->tvi;
	parent->hParent = (HTREEITEM)SendMessage(parent->adl->cinfo->dds.hTreeViewWnd, TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT)&tvins);
	return TRUE;
}

void SetListItem(HWND hWnd, AUTODDLIST *adl, SETLISTSTRUCT *parent)
{
	TCHAR title[VFPS + 256], classname[VFPS];
	SETLISTSTRUCT thisw;
	DWORD style;

	title[0] = '\0';
	GetWindowText(hWnd, title, VFPS);
	classname[0] = '\0';
	GetClassName(hWnd, classname, TSIZEOF(classname));
	if ( title[0] == '\0' ){
		tstrcpy(title, classname);
	}else if ( tstrcmp(title, T("Program Manager")) == 0 ){
		tstrcpy(title, MessageText(MES_PJDE));
	}
	if ( parent == NULL ){
		if ( FilenameRegularExpression(title, &adl->mask_title) == FALSE ){
			return;
		}
	}

	thisw.hParent = NULL;
	thisw.adl = adl;
	thisw.tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	thisw.tvi.pszText = title;
	thisw.tvi.lParam = (LPARAM)hWnd;

	style = GetWindowLongPtr(hWnd, GWL_STYLE);
	if ( (style & WS_VISIBLE) &&
		((GetProp(hWnd, OLDDnDPropName) != NULL) ||
		 (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES) ||
//		 (tstricmp(classname, T("ApplicationFrameWindow")) == 0) || // UWP用。しかし、その後の DLL注入 / mouse_event / SendInput が使用できない。
		 (!(style & ES_READONLY) && IsEditableClass(classname)) ) ){
		if ( (parent != NULL) && (parent->hParent == NULL) ){ // 親未作成
			parent->tvi.lParam = (LPARAM)hWnd; // ウィンドウハンドル無し→子使用
			parent->tvi.iImage = parent->tvi.iSelectedImage = 1;
			tstrcat(parent->tvi.pszText, T(")"));
			tstrinsert(parent->tvi.pszText, T("("));
			if ( ListWindowItem(hWnd, parent, TVI_ROOT) == FALSE ) return;
		}
		thisw.tvi.iImage = thisw.tvi.iSelectedImage = 0;
		if ( ListWindowItem(hWnd, &thisw, parent == NULL ? TVI_ROOT : parent->hParent) == FALSE ){
			return;
		}
	}
	if ( parent == NULL ){
		EnumChildWindows(hWnd, ListChildWindows, (LPARAM)&thisw);
	}
}

BOOL CALLBACK ListChildWindows(HWND hWnd, LPARAM lParam)
{
	SetListItem(hWnd, ((SETLISTSTRUCT *)lParam)->adl, (SETLISTSTRUCT *)lParam);
	return TRUE;
}

BOOL CALLBACK ListWindows(HWND hWnd, LPARAM lParam)
{
	SetListItem(hWnd, (AUTODDLIST *)lParam, NULL);
	return TRUE;
}

void ResizeAddListWindow(struct ddwndstruct *dds, int sizeX, int sizeY)
{
	RECT box;
	HWND hWnd;

	hWnd = dds->hTreeWnd;

	GetWindowRect(hWnd, &box);

	box.right = box.right - box.left + sizeX;
	box.bottom = box.bottom - box.top + sizeY;
	if ( box.right < 8 ) box.right = 8;
	if ( box.bottom < 8 ) box.bottom = 8;
	SetWindowPos(hWnd, NULL, 0, 0, box.right, box.bottom, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
	return;
}

ERRORCODE ADDlistCommand(struct ddwndstruct *dds, WORD key)
{
	TCHAR buf[CMDLINESIZE];

	if ( !(key & K_raw) ){
		PutKeyCode(buf, key);
		if ( NO_ERROR == GetCustTable(T("K_list"), buf, buf, sizeof(buf)) ){
			WORD *ptr;

			if ( (UTCHAR)buf[0] == EXTCMD_CMD ){
				PP_ExtractMacro(dds->hTreeWnd, &dds->cinfo->info, NULL, buf + 1, NULL, 0);
				return NO_ERROR;
			}
			ptr = (WORD *)(((UTCHAR)buf[0] == EXTCMD_KEY) ? (buf + 1) : buf);
			while( *ptr != 0 ){
				if ( ADDlistCommand(dds, (WORD)(*ptr | K_raw)) != NO_ERROR ){
					CallWindowProc(dds->OldTreeWndProc, dds->hTreeViewWnd, WM_KEYDOWN, *ptr & 0xff, 0);
					CallWindowProc(dds->OldTreeWndProc, dds->hTreeViewWnd, WM_KEYUP, *ptr & 0xff, 0);
				}
				ptr++;
			}
			return NO_ERROR;
		}
	}

	switch ( key & ~(K_v | K_raw) ){ // エイリアスビットを無効にする
		case VK_ESCAPE:
			PostMessage(dds->hTreeWnd, WM_CLOSE, 0, 0);
			break;

		case VK_F5:
			SendMessage(dds->hTreeViewWnd, TVM_DELETEITEM, 0, 0);
			EnumWindows(ListWindows, (LPARAM)GetWindowLongPtr(dds->hTreeWnd, WLP_SOURCE));
			break;

		case K_a | K_s | VK_LEFT:
		case K_c | K_s | VK_LEFT:
			ResizeAddListWindow(dds, -8, 0);
			break;

		case K_a | K_s | VK_RIGHT:
		case K_c | K_s | VK_RIGHT:
			ResizeAddListWindow(dds, +8, 0);
			break;

		case K_a | K_s | VK_UP:
		case K_c | K_s | VK_UP:
			ResizeAddListWindow(dds, 0, -8);
			break;

		case K_a | K_s | VK_DOWN:
		case K_c | K_s | VK_DOWN:
			ResizeAddListWindow(dds, 0, +8);
			break;

		case K_a | VK_UP:
		case K_a | VK_DOWN:
		case K_a | VK_LEFT:
		case K_a | VK_RIGHT:
			PPxCommonCommand(dds->hTreeWnd, 0, key);
			break;

		default:
			return ERROR_INVALID_FUNCTION; // 該当無し
	}
	return NO_ERROR;
}

// TreeView のキー入力を拡張する
LRESULT CALLBACK WindowTreeWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	struct ddwndstruct *dds;

	dds = (struct ddwndstruct *)GetProp(hWnd, ADDPROP);
	switch(iMsg){
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			if ( ADDlistCommand(dds, (WORD)(wParam | GetShiftKey() | K_v))
					!= ERROR_INVALID_FUNCTION){
				return 0;
			}
			break;

		case WM_SYSCHAR:
		case WM_CHAR:
			if ( ((WORD)wParam < 0x80) ){
				if ( ADDlistCommand(dds, FixCharKeycode((WORD)wParam))
						!= ERROR_INVALID_FUNCTION){
					return 0;
				}
			}
			break;
	}
	return CallWindowProc(dds->OldTreeWndProc, hWnd, iMsg, wParam, lParam);
}

// TreeView からのメッセージを受け取るウィンドウ

LRESULT WmWndTreeCreate(HWND hWnd, PPC_APPINFO *cinfo)
{
	COLORREF bkcolor;

	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)cinfo);

	if ( DImageList_Draw == NULL ){			// 初めての呼び出しなら関数準備
		LoadWinAPI(NULL, LoadCommonControls(0), ImageCtlDLL, LOADWINAPI_HANDLE);
	}

	// TreeView を作成
	cinfo->dds.hTreeViewWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW,
		NilStr, WS_VISIBLE | WS_CHILD |
		TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_LINESATROOT,
		0, 0, 400, 200, hWnd, CHILDWNDID(IDW_ADDTREE), hInst, NULL);
	if ( X_dss & DSS_COMCTRL ){
		SendMessage(cinfo->dds.hTreeViewWnd, CCM_DPISCALE, TRUE, 0);
	}
	FixUxTheme(cinfo->dds.hTreeViewWnd, WC_TREEVIEW);
	cinfo->dds.hTreeImage = DImageList_Create(16, 16, 24/* | ILC_MASK*/, 32, 32);

	bkcolor = (OSver.dwMajorVersion >= 5) ? TreeView_GetBkColor(cinfo->dds.hTreeViewWnd) : C_AUTO;
	if ( bkcolor == C_AUTO ) bkcolor = C_WindowBack;
	DImageList_SetBkColor(cinfo->dds.hTreeImage, bkcolor);
	DImageList_AddIcon(cinfo->dds.hTreeImage, LoadIcon(NULL, IDI_APPLICATION));
	DImageList_AddIcon(cinfo->dds.hTreeImage, LoadIcon(NULL, IDI_EXCLAMATION));
	SendMessage(cinfo->dds.hTreeViewWnd, TVM_SETIMAGELIST, (WPARAM)TVSIL_NORMAL, (LPARAM)cinfo->dds.hTreeImage);

	SetProp(cinfo->dds.hTreeViewWnd, ADDPROP, (HANDLE)&cinfo->dds);
	cinfo->dds.OldTreeWndProc = (WNDPROC)SetWindowLongPtr(
			cinfo->dds.hTreeViewWnd, GWLP_WNDPROC, (LONG_PTR)WindowTreeWndProc);
	return 0;
}

// ターゲットを枠で囲う
void WndTreeShowBlock(PPC_APPINFO *cinfo, TV_ITEM *titem)
{
	RECT box;
	HRGN hRgn1, hRgn2, hRgn;

	if ( (titem->mask & TVIF_PARAM) == 0 ) return;
	if ( (HWND)titem->lParam == NULL ){
		if ( cinfo->dds.hTargetWnd != NULL ){
			ShowWindow(cinfo->dds.hTargetWnd, SW_HIDE);
		}
		return;
	}
	if ( cinfo->dds.hTargetWnd == NULL ){
		WNDCLASS FrameClass;
#ifndef _WIN64
		DWORD StyleEx = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;

		if ( OSver.dwMajorVersion >= 5 ) StyleEx |= WS_EX_NOACTIVATE;
#else
		#define StyleEx WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE
#endif
		FrameClass.style		= 0;
		FrameClass.lpfnWndProc	= DefWindowProc;
		FrameClass.cbClsExtra	= 0;
		FrameClass.cbWndExtra	= 0;
		FrameClass.hInstance	= hInst;
		FrameClass.hIcon		= NULL;
		FrameClass.hCursor		= NULL;
		FrameClass.hbrBackground = WNDCLASSBRUSH(COLOR_HIGHLIGHT + 1);
		FrameClass.lpszMenuName = NULL;
		FrameClass.lpszClassName = FrameClassName;
		RegisterClass(&FrameClass);

		cinfo->dds.hTargetWnd = CreateWindowEx(StyleEx, FrameClassName, NilStr,
				WS_POPUP | WS_BORDER, 0, 0, 0, 0, NULL, NULL, hInst, NULL);
	}
	GetWindowRect((HWND)titem->lParam, &box);

	box.right -= box.left;
	box.bottom -= box.top;

	hRgn = CreateRectRgn(0, 0, 1, 1);
	hRgn1 = CreateRectRgn(0, 0, box.right, box.bottom);
	hRgn2 = CreateRectRgn(3, 3, box.right - 3, box.bottom - 3);
	CombineRgn(hRgn, hRgn1, hRgn2, RGN_DIFF);
	DeleteObject(hRgn1);
	DeleteObject(hRgn2);
	SetWindowRgn(cinfo->dds.hTargetWnd, hRgn, TRUE);
	SetWindowPos(cinfo->dds.hTargetWnd, HWND_TOP, box.left, box.top,
			box.right, box.bottom, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void WndTreeKey(PPC_APPINFO *cinfo, WORD key)
{
	TV_ITEM tvi;
	HTREEITEM hTreeitem;

	if ( ADDlistCommand(&cinfo->dds, key) != ERROR_INVALID_FUNCTION ) return;
	switch ( key & 0x1ff ){
		case K_cr:
			hTreeitem = TreeView_GetSelection(cinfo->dds.hTreeViewWnd);
			if ( hTreeitem == NULL ) return;

			tvi.mask = TVIF_PARAM | TVIF_IMAGE;
			tvi.hItem = hTreeitem;
			if ( IsTrue(TreeView_GetItem(cinfo->dds.hTreeViewWnd, &tvi)) ){
				HWND hTargetWnd = (HWND)tvi.lParam;
				DWORD type;
				AUTODDLIST *adl;

				if ( hTargetWnd == NULL ) return; // 実行対象？

				adl = (AUTODDLIST *)GetWindowLongPtr(cinfo->dds.hTreeWnd, WLP_SOURCE);
				type = adl->droptype;

				if ( GetAsyncKeyState(VK_CONTROL) & KEYSTATE_PUSH ){
					resetflag(type, DROPTYPE_HOOK);
				}

				if ( (GetAsyncKeyState(VK_SHIFT) & KEYSTATE_PUSH) ||
					 (key & K_s) ){
					setflag(type, DROPTYPE_RIGHT);
				}

				StartAutoDD(cinfo, hTargetWnd, adl->src, type | DROPTYPE_DIALOG);
				PostMessage(cinfo->dds.hTreeWnd, WM_CLOSE, 0, 0);
			}
			return;

		case K_F10:
		case K_apps: {
			HMENU hPopMenu;

			hPopMenu = CreatePopupMenu();

			hTreeitem = TreeView_GetSelection(cinfo->dds.hTreeViewWnd);
			if ( hTreeitem == NULL ) return;

			tvi.mask = TVIF_PARAM | TVIF_IMAGE;
			tvi.hItem = hTreeitem;
			if ( IsTrue(TreeView_GetItem(cinfo->dds.hTreeViewWnd, &tvi)) ){
				HWND hTargetWnd = (HWND)tvi.lParam;
				POINT pos;
				int index;
				DWORD type;
				AUTODDLIST *adl;

				if ( hTargetWnd == NULL ) return; // 実行対象？
				adl = (AUTODDLIST *)GetWindowLongPtr(cinfo->dds.hTreeWnd, WLP_SOURCE);
				type = adl->droptype;

				if ( GetWindowLongPtr(hTargetWnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES ){
					AppendMenuString(hPopMenu, ADDMENU_POST, MES_ADDP);
				}
				AppendMenuString(hPopMenu, ADDMENU_COM_HOOK, MES_ADDH);
				AppendMenuString(hPopMenu, ADDMENU_COM_EMULATE, MES_ADDE);
				AppendMenu(hPopMenu, MF_SEPARATOR, 0, NULL);
				AppendMenuCheckString(hPopMenu, ADDMENU_COPY, MES_JMCP, type & DROPTYPE_COPY);
				AppendMenuCheckString(hPopMenu, ADDMENU_MOVE, MES_JMMO, type & DROPTYPE_MOVE);
				AppendMenuCheckString(hPopMenu, ADDMENU_RIGHT, MES_ADDR, type & DROPTYPE_RIGHT);

				TinyGetMenuPopPos(cinfo->dds.hTreeWnd, &pos);
				index = TrackPopupMenu(hPopMenu, TPM_TDEFAULT, pos.x, pos.y, 0, cinfo->dds.hTreeWnd, NULL);
				DestroyMenu(hPopMenu);
				switch ( index ){
					case ADDMENU_POST:
						type = (type & ~DROPTYPE_OPTYPE_MASK) | DROPTYPE_POST;
						break;
					case ADDMENU_COM_HOOK:
						type = (type & ~DROPTYPE_OPTYPE_MASK) | DROPTYPE_COM | DROPTYPE_HOOK;
						break;
					case ADDMENU_COM_EMULATE:
						type = (type & ~DROPTYPE_OPTYPE_MASK) | DROPTYPE_COM;
						break;
					case ADDMENU_COPY:
						ChangeDDtype(adl, type, DROPTYPE_COPY);
						return;
					case ADDMENU_MOVE:
						ChangeDDtype(adl, type, DROPTYPE_MOVE);
						return;
					case ADDMENU_RIGHT:
						ChangeDDtype(adl, type ^ DROPTYPE_RIGHT, 0);
						return;
					default:
						return;
				}

				StartAutoDD(cinfo, hTargetWnd, adl->src, type | DROPTYPE_DIALOG);
				PostMessage(cinfo->dds.hTreeWnd, WM_CLOSE, 0, 0);
				return;
			}
		}
	}
}

void ChangeDDtype(AUTODDLIST *adl, DWORD type, DWORD toggle)
{
	if ( toggle ){
		DWORD tvalue;

		tvalue = type & DROPTYPE_FOTYPE_MASK;
		type = type & ~DROPTYPE_FOTYPE_MASK;
		if ( tvalue != toggle ) setflag(type, toggle);
	}

	adl->droptype = type;
	WndTreeKey(adl->cinfo, K_apps);
}

void FreeAinfo(PPC_APPINFO *cinfo)
{
	AUTODDLIST *adl;

	adl = (AUTODDLIST *)GetWindowLongPtr(cinfo->dds.hTreeWnd, WLP_SOURCE);
	if ( adl == NULL ) return;

	FreeFN_REGEXP(&adl->mask_title);
	FreeFN_REGEXP(&adl->mask_filename);
	PPcHeapFree(adl);
}

void WmAddDestroy(HWND hWnd)
{
	HFONT hTVfont;
	WINPOS WPos;
	WINDOWPLACEMENT wp;
	AUTODDLIST *adl;
	struct ddwndstruct *dds;

	adl = (AUTODDLIST *)GetWindowLongPtr(hWnd, WLP_SOURCE);
	if ( adl == NULL ) return;

	dds = &adl->cinfo->dds;

	wp.length = sizeof(wp);
	GetWindowPlacement(hWnd, &wp);
	if ( wp.showCmd == SW_SHOWNORMAL ){
		WPos.show = (BYTE)wp.showCmd;
		WPos.reserved = 0;
		GetWindowRect(hWnd, &WPos.pos);
		SetCustTable(T("_WinPos"), T("WndLst"), &WPos, sizeof(WPos));
	}

	dds->hTreeWnd = NULL;
							// Tree view の廃棄と関連オブジェクトの削除
	hTVfont = (HFONT)SendMessage(dds->hTreeViewWnd, WM_GETFONT, 0, 0);
	DestroyWindow(dds->hTreeViewWnd);
	// ImageList / Font は使用していた窓が廃棄されたので そのまま削除可
	DImageList_Destroy(dds->hTreeImage);
	if ( hTVfont != NULL ) DeleteObject(hTVfont);
							// D&D先枠の廃棄
	if ( dds->hTargetWnd != NULL ){
		PostMessage(dds->hTargetWnd, WM_CLOSE, 0, 0);
		dds->hTargetWnd = NULL;
	}

	FreeFN_REGEXP(&adl->mask_title);
	FreeFN_REGEXP(&adl->mask_filename);
	PPcHeapFree(adl);
}

LRESULT CALLBACK WndTreeProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PPC_APPINFO *cinfo;

	cinfo = (PPC_APPINFO *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch (message){
		case WM_CREATE:
			return WmWndTreeCreate(hWnd, (PPC_APPINFO *)(((CREATESTRUCT *)lParam)->lpCreateParams));

		case WM_DESTROY:
			WmAddDestroy(hWnd);
			break;

		case WM_SETFOCUS:
			SetFocus(cinfo->dds.hTreeViewWnd);
			break;

		case WM_SIZE:
			SetWindowPos(cinfo->dds.hTreeViewWnd, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOMOVE | SWP_NOZORDER);
			break;

		case WM_NOTIFY:
			#define NHPTR ((NMHDR *)lParam)
			#define NTVPTR ((NM_TREEVIEW *)lParam)
			switch ( NHPTR->code ){
				case TVN_KEYDOWN:
					WndTreeKey(cinfo, (WORD)K_v |
						((TV_KEYDOWN *)lParam)->wVKey | (WORD)GetShiftKey());
					break;

				case NM_DBLCLK:
					WndTreeKey(cinfo, K_cr);
					return 0;

				case NM_RCLICK:
					WndTreeKey(cinfo, K_apps);
					return 0;

				case NM_RDBLCLK:
					WndTreeKey(cinfo, K_cr | K_s);
					return 0;

				case TVN_SELCHANGED:
					if ( NTVPTR->itemNew.hItem != NULL ){
						WndTreeShowBlock(cinfo, &NTVPTR->itemNew);
					}
					return 0;
//				default:
			}
			#undef NTVPTR
			#undef NHPTR
			// default へ

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void AutoDDDialog(PPC_APPINFO *cinfo, AUTODDINFO *ainfo, DWORD droptype)
{
	AUTODDLIST *adl;

	adl = (AUTODDLIST *)PPcHeapAlloc(sizeof(AUTODDLIST));
	if ( adl == NULL ){
		XMessage(NULL, NULL, XM_GrERRld, T("memory error"));
		return;
	}

	if ( ainfo != NULL ){
		tstrcpy(adl->src, (ainfo->src != NULL) ? ainfo->src : NilStr);
		MakeFN_REGEXP(&adl->mask_title,
				(ainfo->mask_title != NULL) ? ainfo->mask_title : NilStr);
		MakeFN_REGEXP(&adl->mask_filename,
				(ainfo->mask_filename != NULL) ? ainfo->mask_filename : NilStr);
		adl->hTargetWnd = ainfo->hTargetWnd;
		adl->droptype = ainfo->droptype;
	}else{
		adl->src[0] = '\0';
		MakeFN_REGEXP(&adl->mask_title, NilStr);
		MakeFN_REGEXP(&adl->mask_filename, NilStr);
		adl->hTargetWnd = NULL;
		adl->droptype = droptype;
	}
	adl->cinfo = cinfo;

	if ( cinfo->dds.hTreeWnd == NULL ){
		WNDCLASS TreeClass;
		WINPOS WPos = {{0, 0, 400, 400}, 0, 0};

		if ( hPSAPI == INVALID_HANDLE_VALUE ){
			hPSAPI = LoadSystemDLL(SYSTEMDLL_PSAPI);
			if ( hPSAPI != NULL ) GETDLLPROCT(hPSAPI, GetModuleFileNameEx);
		}

		cinfo->dds.cinfo = cinfo;

		LoadCommonControls(ICC_TREEVIEW_CLASSES);
		TreeClass.style			= 0;
		TreeClass.lpfnWndProc	= WndTreeProc;
		TreeClass.cbClsExtra	= 0;
		TreeClass.cbWndExtra	= sizeof(DWORD_PTR);
		TreeClass.hInstance		= hInst;
		TreeClass.hIcon			= LoadIcon(hInst, MAKEINTRESOURCE(Ic_PPC));
		TreeClass.hCursor		= LoadCursor(NULL, IDC_ARROW);
		TreeClass.hbrBackground	= NULL;
		TreeClass.lpszMenuName	= NULL;
		TreeClass.lpszClassName	= WndTreeName;
											// クラスを登録する
		RegisterClass(&TreeClass);

		GetCustTable(T("_WinPos"), T("WndLst"), &WPos, sizeof(WPos));
		WPos.pos.right -= WPos.pos.left;
		if ( WPos.pos.right < 32 ) WPos.pos.right = 400;
		WPos.pos.bottom -= WPos.pos.top;
		if ( WPos.pos.bottom < 32 ) WPos.pos.bottom = 400;

		cinfo->dds.hTreeWnd = CreateWindowEx(0, WndTreeName,
				MessageText(MES_TDAD), WS_OVERLAPPEDWINDOW,
				0, 0, WPos.pos.right, WPos.pos.bottom, NULL, NULL, hInst, cinfo);
		FixUxTheme(cinfo->dds.hTreeWnd, NULL);
		CenterPPcDialog(cinfo->dds.hTreeWnd, cinfo);

		{
									// フォント情報の作成
			LOGFONTWITHDPI lBoxFont;

			GetPPxFont(PPXFONT_F_tree, cinfo->FontDPI, &lBoxFont);
			if ( lBoxFont.font.lfFaceName[0] != '\0' ){
				HFONT hFont;
				hFont = CreateFontIndirect(&lBoxFont.font);
				SendMessage(cinfo->dds.hTreeViewWnd, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE);
			}
		}
	}

	if ( cinfo->dds.hTreeWnd != NULL ){
		FreeAinfo(cinfo);

		SetWindowLongPtr(cinfo->dds.hTreeWnd, WLP_SOURCE, (DWORD_PTR)adl);
		SendMessage(cinfo->dds.hTreeViewWnd, TVM_DELETEITEM, 0, 0);

		EnumWindows(ListWindows, (LPARAM)adl);

		ShowWindow(cinfo->dds.hTreeWnd, SW_SHOWNORMAL);
		SetForegroundWindow(cinfo->dds.hTreeWnd);
	}
}
