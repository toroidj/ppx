/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library			外部プロセス起動/PPx間通信
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#include "VFS_STRU.H"
#ifdef WINEGCC
#include <unistd.h>
#endif
#pragma hdrstop

int GetExecTypeFileOnly(const TCHAR *name);
BOOL CreateProcessDesktopLevel(TCHAR *lpCommandLine, DWORD dwCreationFlags, const TCHAR *lpCurrentDirectory, STARTUPINFO *lpStartupInfo, PROCESS_INFORMATION *lpProcessInformation);

#define DEFAULTWAITPPBTIME WAIT_NO_DIALOG_TIME

const TCHAR ComSpecStr[] = T("ComSpec");

struct PPBWAITSTRUCT {
	HWND hDlg;
	int user;
	int PPbID;
	DWORD starttick;
	DWORD waittick;
	DWORD flags;
};

#ifdef __GNUC__
#undef  SEE_MASK_UNICODE
#define SEE_MASK_UNICODE 0x4000 // 定義ミスがある
#endif

#ifndef ERROR_CANT_ACCESS_FILE
#define ERROR_CANT_ACCESS_FILE 1920
#endif

#define ET_DRIVE B0	// ドライブ指定あり
#define ET_DIRECTORY B1	// ディレクトリ指定あり
#define ET_PATHMASK (ET_DRIVE | ET_DIRECTORY)	// パス指定あり
#define ET_EXT  B2	// 拡張子指定有り

const TCHAR PathextStr[] = T("PATHEXT");
const TCHAR AppPathKey[] =
		T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%s");
#define MAXSTORE 8 // 実行ファイルCRCの保存数

typedef struct {
	PPXAPPINFO info;
	const TCHAR *name;
} CHECKEXEBININFO;
const TCHAR is1[] = T("Execute check");
const TCHAR is2[] = T("%s is new Executive file.\n\n%s\n\nExecute this file?");

void FixWaitBoxPos(HWND hDlg, HWND hFGwnd)
{
	RECT box, fgbox, deskbox;

	GetWindowRect(hDlg, &box);
	box.right -= box.left;
	box.bottom -= box.top;
	GetWindowRect(hFGwnd, &fgbox);
	GetDesktopRect(hFGwnd, &deskbox);

	box.left = fgbox.left;
	if ( (box.left + box.right) > deskbox.right ){
		box.left = fgbox.right - box.right;
	}
	box.top = fgbox.bottom;
	if ( (box.top + box.bottom) > deskbox.bottom ){
		box.top = fgbox.top - box.bottom;
		if ( box.top < deskbox.top ){
			box.left = deskbox.right - box.right;
			box.top = deskbox.bottom - box.bottom;
		}
	}
	SetWindowPos(hDlg, NULL, box.left, box.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void DialogBoxSignalMenu(HWND hDlg, int PPbID)
{
	HMENU hPopup;
	RECT box;
	int index;

	hPopup = CreatePopupMenu();

	AppendMenuString(hPopup, 1, T("Ctrl + C to PPb(&C)"));
	AppendMenuString(hPopup, 2, T("Ctrl + Break to PPb(&B)"));
	AppendMenuString(hPopup, 3, T("Kill process on PPb(&K)"));
	AppendMenuString(hPopup, 4, T("focus PPb(&F, test)"));

	GetWindowRect(GetDlgItem(hDlg, IDB_DIALOG_OPTION), &box);
	index = TrackPopupMenu(hPopup, TPM_TDEFAULT, box.left, box.bottom, 0, hDlg, NULL);
	DestroyMenu(hPopup);

	hDlg = Sm->P[PPbID].UsehWnd;
	switch ( index ){
		case 1:
			PPbSpecialMessage(hDlg, PPbID, T(">Gc"));
			break;

		case 2:
			PPbSpecialMessage(hDlg, PPbID, T(">Gb"));
			break;

		case 3:
			PPbSpecialMessage(hDlg, PPbID, T(">Gk"));
			break;

		case 4:
			PPbSpecialMessage(hDlg, PPbID, T(">Gf"));
//			SetForegroundWindow(Sm->P[PPbID].hWnd);
			break;
	}
}

// 指定時間以上経過すると表示するボックス -------------------------------------
INT_PTR CALLBACK WaitPPBDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg){
		case WM_INITDIALOG:
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
			LocalizeDialogText(hDlg, IDD_WAITPPB);
			FixWaitBoxPos(hDlg, GetForegroundWindow());
			return FALSE;

		case WM_CLOSE:
			((struct PPBWAITSTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER))->user = IDCANCEL;
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDB_WAITPPB_NOPPB:
				case IDB_WAITPPB_NEWPPB:
					((struct PPBWAITSTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER))->user = LOWORD(wParam);
					break;

				case IDCANCEL:
					WaitPPBDlgBox(hDlg, WM_CLOSE, 0, 0);
					break;

				case IDB_DIALOG_OPTION:
					DialogBoxSignalMenu(hDlg, ((struct PPBWAITSTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER))->PPbID);
					break;
			}
			break;

		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}

/*-----------------------------------------------------------------------------
	外部プロセスを起動する
-----------------------------------------------------------------------------*/
PPXDLL BOOL PPXAPI ComExec(HWND hOwner, const TCHAR *line, const TCHAR *path)
{
	return ComExecEx(hOwner, line, path, NULL, 0, NULL);
}

int AllocPPb(HWND hOwner, const TCHAR *path, int flags)
{
	HANDLE hE;
	TCHAR buf[200];
	int ID;

	thprintf(buf, TSIZEOF(buf), T(PPBBOOTSYNC) T("%x"), (DWORD)(DWORD_PTR)hOwner);
	hE = CreateEvent(NULL, TRUE, FALSE, buf);
	thprintf(buf, TSIZEOF(buf), T("-IR%u"), (DWORD)(DWORD_PTR)hOwner);
	if ( BootPPB(path, buf, flags) ){	// PPb の起動に失敗
		CloseHandle(hE);
		return -1;
	}else{							// PPb を割り当てるまで待つ
		int loopcnt = (15*1000) / 500;

		while ( WaitForSingleObject(hE, 500) != WAIT_OBJECT_0 ){
			PeekMessageLoop(NULL);
			loopcnt--;
			if ( loopcnt ) continue;

			CloseHandle(hE);
			XMessage(hOwner, NULL, XM_GrERRld, MES_EPBY, T("boot timeout"));
			return -1;
		}
		CloseHandle(hE);
		ID = UsePPb(hOwner);
		if ( ID == -1 ) XMessage(hOwner, NULL, XM_GrERRld, MES_EPBY, T("share fault"));
		return ID;
	}
}
/*-----------------------------------------------------------------------------
	外部プロセスを起動する
-----------------------------------------------------------------------------*/
PPXDLL ERRORCODE PPXAPI GetExecuteErrorReason(const TCHAR *filename, TCHAR *reason)
{
	ERRORCODE errcode;

	errcode = GetLastError();
	if ( errcode == ERROR_BAD_EXE_FORMAT ){
		VFSFILETYPE type;

		type.flags = VFSFT_TYPETEXT;
		errcode = VFSGetFileType(filename, NULL, 0, &type);
		if ( errcode != NO_ERROR ){
			PPErrorMsg(reason, errcode);
		}else{
			thprintf(reason, MAX_PATH, MessageText(MES_EEFT), type.typetext);
		}
	}else{
		PPErrorMsg(reason, errcode);
	}
	SetLastError(errcode);
	return errcode;
}

void PPbLoadWaitTime(HWND hOwner, struct PPBWAITSTRUCT *pws)
{
	if ( pws->user == IDOK ){
		GetCustData(T("X_wppb"), &pws->waittick, sizeof(DWORD));
		if ( pws->waittick == (DWORD)-1 ){
			pws->user = IDB_WAITPPB_NOPPB;
		}else if ( pws->waittick == (DWORD)-2 ){
			pws->user = IDB_WAITPPB_NEWPPB;
		}else{
			pws->waittick *= 1000; // ms→sec
			pws->user = 0;
		}
		if ( (pws->flags & XEO_SEQUENTIAL) && (pws->user != 0) ){
			pws->waittick = DEFAULTWAITPPBTIME;
			pws->user = 0;
		}
	}
	if ( pws->user != 0 ) return;

	PeekDialogMessageLoop(NULL, pws->hDlg);

				// GetTickCountオーバーフロー時は、即座に反応
	if ( (GetTickCount() - pws->starttick) > pws->waittick ){
		if ( (pws->hDlg == NULL) && !(pws->flags & XEO_WAITQUIET) ){
			pws->hDlg = CreateDialogParam(DLLhInst,
					MAKEINTRESOURCE(IDD_WAITPPB),
					hOwner, WaitPPBDlgBox, (LPARAM)pws);
			ShowWindow(pws->hDlg, SW_SHOWNOACTIVATE);
			if ( GetParent(hOwner) != NULL ) SetFocus(pws->hDlg);
		}
	}
	return;
}

BOOL PPbCheck(int ID)
{
	ShareX *P;
	TCHAR syncname[32];
	DWORD result = WAIT_FAILED;

	tstrcpy(syncname, SyncTag);
	P = &Sm->P[ID];
	UsePPx();
	if ( (P->ID[0] == 'B') && (P->ID[1] == '_') ){
		HANDLE hCommEvent2;

		tstrcat(syncname, P->ID);			// PPb との通信手段を確保 ---------
		tstrcat(syncname, T("R"));
		hCommEvent2 = OpenEvent(EVENT_ALL_ACCESS, FALSE, syncname);
		if ( hCommEvent2 != NULL ){
			result = WaitForSingleObject(hCommEvent2, 50);
			CloseHandle(hCommEvent2);
		}
	}
	FreePPx();
	return (result == WAIT_OBJECT_0);
}

BOOL ComExecPPb(HWND hOwner, const TCHAR *line, const TCHAR *path, int *useppb, int flags, int PPbID, EXEC_EXTRA *extra)
{
	TCHAR buf[RECEIVESTRINGSLENGTH];
	struct PPBWAITSTRUCT pws = { NULL, IDOK, 0, -1, DEFAULTWAITPPBTIME / 1000, 0 };
	HWND hOldFocus;

	if ( hOwner != NULL ) hOldFocus = GetFocus();
	if ( (useppb != NULL) && (*useppb >= 0) ){	// 指定PPbが空くまで待機
		ShareX *P;
		TCHAR syncname[32];

		tstrcpy(syncname, SyncTag);
		P = &Sm->P[PPbID];
		UsePPx();
		if ( (P->ID[0] == 'B') && (P->ID[1] == '_') && (P->UsehWnd == hOwner)){
			HANDLE hCommEvent2;

			tstrcat(syncname, P->ID);		// PPB との通信手段を確保 ---------
			tstrcat(syncname, T("R"));
			FreePPx();
			hCommEvent2 = OpenEvent(EVENT_ALL_ACCESS, FALSE, syncname);
			if ( hCommEvent2 == NULL ){
				XMessage(hOwner, NULL, XM_FaERRd, MES_EPBY, T("Event destroyed"));
				FreePPb(hOwner, PPbID);
				goto error;
			}
			if ( hOwner != NULL ) EnableWindow(hOwner, FALSE);
			pws.PPbID = PPbID;
			pws.flags = flags;
			pws.starttick = GetTickCount();
			for ( ; ; ){
				DWORD result;

				result = WaitForSingleObject(hCommEvent2, 50);
				if ( result == WAIT_OBJECT_0 ) break;
				if ( result == WAIT_FAILED ){
					XMessage(hOwner, NULL, XM_FaERRd, MES_EPBY, T("Event destroyed"));
					CloseHandle(hCommEvent2);
					FreePPb(hOwner, PPbID);
					return FALSE;
				}
				PPbLoadWaitTime(hOwner, &pws);
				if ( pws.user ) break;
			}
			CloseHandle(hCommEvent2);
		}else{	// 確保していた PPb が無くなった
			FreePPx();
			PPbID = -1;
		}
	}else if ( PPbID == -1 ){	// PPb は全く起動していないのですぐに用意
		PPbID = AllocPPb(hOwner, path, flags);
	}else{	// 任意PPbが空くまで待機
		if ( hOwner != NULL ) EnableWindow(hOwner, FALSE);
		pws.flags = flags;
		pws.starttick = GetTickCount();
		for ( ; ; ){
			FreePPb(hOwner, PPbID);
			PPbID = UsePPb(hOwner);
			if ( PPbID >= 0 ){
				if ( IsTrue(PPbCheck(PPbID)) ) break; // 確保成功
			}
			if ( PPbID == -1 ){	// PPb が存在しないので新規に用意
				PPbID = AllocPPb(hOwner, path, flags);
				break;
			}

			pws.PPbID = PPbID;
			PPbLoadWaitTime(hOwner, &pws);
			if ( pws.user ) break;
			Sleep(50);
		}
	}
	if ( pws.hDlg != NULL ) DestroyWindow(pws.hDlg);
	if ( hOwner != NULL ){
		EnableWindow(hOwner, TRUE);
		SetFocus(hOldFocus);
	}
	switch( pws.user ){
		case IDCANCEL:
			FreePPb(hOwner, PPbID);
			SetLastError(ERROR_CANCELLED);
			return FALSE;

		case IDB_WAITPPB_NEWPPB: {
			int newID = AllocPPb(hOwner, path, flags);
			FreePPb(hOwner, PPbID);
			PPbID = newID;
			break;
		}

		case IDB_WAITPPB_NOPPB:
			FreePPb(hOwner, PPbID);
			PPbID = -1;
			break;
	}
					// PPb の起動に失敗 → 自前で
	if ( PPbID < 0 ) return ComExecSelf(hOwner, line, path, flags, extra);

										// PPb が確保できているなら PPb に送信
	if ( path != NULL ){				// Path送信
		if ( LongSendPPB(hOwner, path, 'L', PPbID) < 0 ) goto senderror;
	}
	if ( line != NULL ){				// パラメータ送信
		TCHAR *p1;
		const TCHAR *opt = XEO_OptionString;
		int bits;
		size_t linelen;

		buf[0] = '>';
		buf[1] = 'H';

		p1 = buf + 2;
		for ( bits = flags ; *opt ; bits >>= 1, opt++ ){
			if ( bits & 1 ) *p1++ = *opt;
		}
		if ( hOwner != NULL ){
			p1 = thprintf(p1, 20, T(";%u"), (DWORD)(DWORD_PTR)hOwner);
		}
		*p1++ = ',';

		linelen = tstrlen(line);
		if ( linelen > PPbMaxSendSize ){ // CMDLINESIZE 以上の長いときは分割送信
			buf[1] = 'p';
			do {
				memcpy(p1, line, PPbMaxSendSize * sizeof(TCHAR));
				*(p1 + PPbMaxSendSize) = '\0';
				if ( SendPPB(hOwner, buf, PPbID) < 0 ) goto senderror;

				line += PPbMaxSendSize;
				linelen -= PPbMaxSendSize;
				p1 = buf + 2;
			}while ( linelen > PPbMaxSendSize );
			buf[1] = 'P';
		}

		tstrcpy(p1, line);
		if ( SendPPB(hOwner, buf, PPbID) < 0 ) goto senderror;

		if ( flags & (XEO_WAITIDLE | XEO_SEQUENTIAL) ){
			if ( (extra == NULL) || (extra->ExitCode == NULL) ){
				if ( SendPPB(hOwner, T(">"), PPbID) < 0 ) goto senderror;
			}else{
				*extra->ExitCode = RecvPPBExitCode(hOwner, PPbID);
			}
		}
	}
	if ( useppb != NULL ){
		*useppb = PPbID;
	}else{
		FreePPb(hOwner, PPbID);
	}
	return TRUE;
error:
	if ( pws.hDlg != NULL ) DestroyWindow(pws.hDlg);
	if ( hOwner != NULL ) EnableWindow(hOwner, TRUE);
	return FALSE;
senderror:
	if ( useppb != NULL ){
		*useppb = PPbID;
	}else{
		FreePPb(hOwner, PPbID);
	}
	return FALSE;
}

void ExecErrorMessageOnConsole(const TCHAR *message)
{
	TCHAR text[0x1000], *ptr;

	ptr = tstplimcpy(text, message, 0xff0);
	tstrcpy(ptr, T("\r\n"));
	PrintToConsole(text);
}

void ExecErrorMessage(HWND hWnd, const TCHAR *title, const TCHAR *message)
{
	if ( ConsoleMode < ConsoleMode_GUI ){
		PopupErrorMessage(hWnd, title, message);
	}else{
		ExecErrorMessageOnConsole(message);
	}
}

void PopupErrorCodeMessage(HWND hWnd, const TCHAR *title, ERRORCODE code)
{
	TCHAR buf[0x400];

	PPErrorMsg(buf, code);
	PopupErrorMessage(hWnd, title, buf);
	SetLastError(code);
}

BOOL IsRedirect(const TCHAR *str) // SearchVLINE 関連
{
	TCHAR type;

	for ( ;; ){
		type = *str;
		if ( type == '\0' ) return FALSE;
		if ( (type == '|') || (type == '<') || (type == '>') ) return TRUE;
		#ifdef UNICODE
			str++;
		#else
			type = (char)Chrlen(type);
			str += type;
		#endif
	}
}


BOOL ComExecEx(HWND hOwner, const TCHAR *line, const TCHAR *path, int *useppb, int flags, EXEC_EXTRA *extra)
{
	int PPbID = -1;

		// 自前 or 自分がコンソール なら自前
	if ( (flags & (XEO_NOUSEPPB | XEO_CONSOLE)) || (ConsoleMode >= ConsoleMode_GUI) ){
		if ( !(flags & XEO_NOPIPERDIR) && IsRedirect(line) ){
			setflag(flags, XEO_USECMD);
		}
		return ComExecSelf(hOwner, line, path, flags, extra);
	}
	FixTask();
	if ( useppb != NULL ) PPbID = *useppb;

	if ( PPbID < 0 ){
		if ( flags & XEO_USEPPB ){
			PPbID = UsePPb(hOwner); // もし、空いている PPb があれば確保
		}else{
			int type;
			const TCHAR *p;

			p = line;
			type = GetExecType(&p, NULL, path);
			if ( (type == GTYPE_ERROR) &&
				 (GetLastError() == ERROR_PATH_NOT_FOUND ) ){
				PopupErrorCodeMessage(hOwner, T("Comexec"), ERROR_PATH_NOT_FOUND);
				return FALSE;
			}

			if ( (type == GTYPE_ERROR) && (flags & XEO_NOCMDCMD) ){
				return FALSE;
			}

			// Console/CMD 不要？→自前実行できるかも
			if ( ((type == GTYPE_GUI) || (type == GTYPE_SHELLEXEC)) &&
				  ( (flags & XEO_NOPIPERDIR) || !IsRedirect(line)) ){
				// 順次実行不要か空きPPb無しなら、自前実行
				if ( !(flags & XEO_SEQUENTIAL) || ((PPbID = UsePPb(hOwner)) < 0)){
					return ComExecSelf(hOwner, line, path, flags, extra);
				}
			}else{
				PPbID = UsePPb(hOwner); // もし、空いている PPb があれば確保
			}
		}
	}
	// PPb 経由で実行する
	return ComExecPPb(hOwner, line, path, useppb, flags, PPbID, extra);
}

// 指定時間以上経過すると表示するボックス -------------------------------------
INT_PTR CALLBACK WaitDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg){
		case WM_INITDIALOG: {
			WAITDLGSTRUCT *wds = (WAITDLGSTRUCT *)lParam;
			HWND hFGwnd = GetForegroundWindow();

			wds->md.text = MessageText(STR_WAITOPERATION);
			wds->md.style = MB_PPX_STARTCANCEL | MB_DEFBUTTON1 | MB_PPX_NOCENTER;
			if ( wds->md.title == strSendPPb ){
				wds->md.style = MB_PPX_ABORTOPTION | MB_DEFBUTTON1 | MB_PPX_NOCENTER;
			}
			MessageBoxInitDialog(hDlg, &wds->md);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)wds);
			FixWaitBoxPos(hDlg, hFGwnd);
			return FALSE;
		}

		case WM_CLOSE:
			((WAITDLGSTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER))->choose = WDS_CANCEL;
			break;

		case WM_DESTROY:
			DeleteObject(((WAITDLGSTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER))->md.hDlgFont);
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDYES: // 順番待ちを止めて続行
					((WAITDLGSTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER))->choose = WDS_SKIP;
					break;

				case IDCANCEL: // 実行中止
				case IDABORT:
					WaitDlgBox(hDlg, WM_CLOSE, 0, 0);
					break;

				case IDM_CONTINUE:
					WaitDlgBox(hDlg, WM_COMMAND, IDOK, 0);
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)IDM_CONTINUE);
					PostMessage(hDlg, WM_NULL, 0, 0); // メッセージループを回す
					break;

				case IDB_DIALOG_OPTION:
					DialogBoxSignalMenu(hDlg, ((WAITDLGSTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER))->PPbID);
					break;
			}
			break;

		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}
// 起動したプロセスの終了を待つ -----------------------------------------------
// TRUE:正常に終了/強制的に次へ FALSE:中断

#if WAIT_NO_DIALOG_TIME < 200
	#error WAIT_NO_DIALOG_TIME < 200 を想定していない
#endif
BOOL WaitJobDialog(HWND hWnd, HANDLE handle, const TCHAR *title, DWORD flags)
{
	DWORD result, timeout = WAIT_NO_DIALOG_TIME, tick;
	HWND hDlg = NULL;
	WAITDLGSTRUCT wds;

	tick = GetTickCount() + WAIT_NO_DIALOG_TIME;
	wds.choose = WDS_UNCHOOSE;
	for ( ; ; ){
		if ( handle != NULL ){
			if ( flags == XEO_WAITIDLE ){
				result = WaitForInputIdle(handle, 50);
				if ( result == WAIT_TIMEOUT ){
					result = MsgWaitForMultipleObjects(1, &handle, FALSE,
							50, QS_ALLEVENTS | QS_SENDMESSAGE);
					if ( result == WAIT_TIMEOUT ){
						timeout -= 50 * 2;
						if ( timeout >= (50 * 2 * 2) ) continue;
					}
				}
			}else{
				result = MsgWaitForMultipleObjects(1, &handle, FALSE,
						timeout, QS_ALLEVENTS | QS_SENDMESSAGE);
			}
		}else{
			if ( CheckJobWait() == FALSE ) break;
			Sleep(50);
//			result = WAIT_OBJECT_0 + 1;
			timeout -= 50;
			if ( timeout >= (50 * 2) ) continue;
			result = WAIT_TIMEOUT;
		}
		if ( IsChooseWDS(wds.choose) ) break;
		// GetTickCountオーバーフロー時は、MsgWaitForMultipleObjectsのみで判断
		if ( (result == WAIT_TIMEOUT) || (GetTickCount() > tick) ){
			tick = MAX32;
			if ( (hDlg == NULL) && !(flags & XEO_WAITQUIET) ){
				BOOL hide;

				if ( X_uxt_color == UXT_NA ) InitUnthemeCmd();
				wds.md.title = (title != NULL) ? title : PPXJOBMUTEX;
				hDlg = CreateDialogParam(DLLhInst, MAKEINTRESOURCE(IDD_NULLMIN), NULL, WaitDlgBox, (LPARAM)&wds);

				if ( hWnd != NULL ) MoveCenterWindow(hDlg, hWnd);

				hide = SetJobTask(hDlg,
						flags ? JOBSTATE_WAITJOB : JOBSTATE_WAITEXEC);
				ShowWindow(hDlg,
						(hide &&
						 (Sm->JobList.hWnd != NULL) &&
						 (Sm->JobList.hidemode == JOBLIST_HIDE)) ?
						SW_HIDE : SW_SHOWNOACTIVATE);
			}
			timeout = INFINITE;
		}else if ( result == WAIT_OBJECT_0 + 1 ){ // Message
			MSG msg;

			while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ){
				if ( msg.message == WM_QUIT ) break;
				// 自ウィンドウのキー入力・マウス入力を無効化する
				// ※ EnableWindow を使うと、フォーカス操作まで無効化されて、
				//    別のウィンドウにフォーカスが移動してしまうことがあるため
				// ※ WM_IME_KEYLAST ≒ WM_KEYLAST
				// ※ (WM_PARENTNOTIFY - 1) ≒ WM_MOUSELAST
				if ( (msg.hwnd == hWnd) &&
					( ((msg.message >= WM_KEYFIRST) && (msg.message <= WM_IME_KEYLAST)) ||
					  ((msg.message >= WM_MOUSEFIRST) && (msg.message < WM_PARENTNOTIFY))) ){
					continue;
				}
				if ( DialogKeyProc(&msg) ) continue;

				if ( (hDlg == NULL) || (IsDialogMessage(hDlg, &msg) == FALSE) ){
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}else{
			break;
		}
	}
	if ( hDlg ){
		DestroyWindow(hDlg);
		SetJobTask(hDlg, flags ? JOBSTATE_FINWJOB : JOBSTATE_FINWEXEC);
	}
	if ( !IsChooseWDS(wds.choose) ) wds.choose = TRUE;
	return wds.choose;
}

BOOL WaitProcessDialog(HWND hOwner, HANDLE hProcess, const TCHAR *title, DWORD flags, EXEC_EXTRA *extra)
{
	BOOL result = TRUE;

	if ( flags & XEO_WAITIDLE ){
		result = WaitJobDialog(hOwner, hProcess, title, XEO_WAITIDLE);
		if ( (extra != NULL) && (extra->ExitCode != NULL) ){
			*extra->ExitCode = (DWORD)-1;
			GetExitCodeProcess(hProcess, extra->ExitCode);
		}
	}

	if ( flags & XEO_SEQUENTIAL ){
		result = WaitJobDialog(hOwner, hProcess, title, flags & XEO_WAITQUIET);
		if ( (extra != NULL) && (extra->ExitCode != NULL) ){
			*extra->ExitCode = (DWORD)-1;
			GetExitCodeProcess(hProcess, extra->ExitCode);
		}
	}
	return result;
}

DefineWinAPI(DWORD, GetProcessId, (HANDLE Process));

#pragma argsused
void DummyGetProcessID(HANDLE hProcess, DWORD *PID)
{
	UnUsedParam(hProcess);

	if ( PID != NULL ) *PID = (DWORD)-1;
}

void GetProcessIDMain(HANDLE hProcess, DWORD *PID)
{
	if ( PID != NULL ) *PID = DGetProcessId(hProcess);
}
void InitGetProcessID(HANDLE hProcess, DWORD *PID);
void (* GetProcessIDx)(HANDLE hProcess, DWORD *PID) = InitGetProcessID;

/*
BOOL CALLBACK FindProcessWindow(HWND hWnd, LPARAM lParam)
{
	DWORD processID;

	if ( 0 != GetWindowThreadProcessId(hWnd, &processID) ){
		if ( processID == (DWORD)lParam ){
			Message("!");
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			return FALSE;
		}
	}
	return TRUE;
}
*/
void InitGetProcessID(HANDLE hProcess, DWORD *PID)
{
	DGetProcessId = GETDLLPROC(hKernel32, GetProcessId);
	if ( DGetProcessId == NULL ){
		GetProcessIDx = DummyGetProcessID;
	}else{
		GetProcessIDx = GetProcessIDMain;
	}
	GetProcessIDx(hProcess, PID);
/*
	if ( (ExitCode != NULL) && (*ExitCode != 0xffffffff) ){
		int i;
		for ( i = 0 ; i < 10000 ; i+= 100 ){
			if ( FALSE == EnumWindows(FindProcessWindow, (LPARAM)*ExitCode) ){
				break;
			}
			Sleep(100);
		}
	}
*/
}

BOOL ComExecSelfShell(HWND hOwner, const TCHAR *title, const TCHAR *exename, const TCHAR *param, const TCHAR *path, int flags, EXEC_EXTRA *extra)
{
	HANDLE hProcess;
	TCHAR reason[VFPS];
	DWORD addflag;

	addflag = ((extra != NULL) && (extra->mask & EEM_GETID)) ? XEO_WAITIDLE : 0;
	if ( NULL != (hProcess = PPxShellExecute(hOwner, NULL, exename, param, path, (flags | addflag), reason)) ){
		if ( (extra != NULL) && (extra->mask & EEM_GETID) ){
			extra->ProcessID = 0;
			extra->ThreadID = 0;
			GetProcessIDx(hProcess, &extra->ProcessID);
			if ( !(flags & (XEO_WAITIDLE | XEO_SEQUENTIAL)) ){
				CloseHandle(hProcess);
			}
		}
		if ( flags & (XEO_WAITIDLE | XEO_SEQUENTIAL) ){
			BOOL result;

			result = WaitProcessDialog(hOwner, hProcess, title, flags, extra);
			CloseHandle(hProcess);
			return result;
		}
		return TRUE;
	}
	ExecErrorMessage(hOwner, T("ComExecSelf"), reason);
	return FALSE;
}

#ifdef WINEGCC // system で実行
int ComExecSystem(TCHAR *commandline, const TCHAR *CurrentDir)
{
	char bufA[CMDLINESIZE];

	TCHAR *src, *dst, OldCurrentDir[VFPS];
	char *chpath;
	BOOL fixsep = FALSE;
	int result;

							// Z: ドライブのパスをネイティブに変換する
	for ( dst = src = commandline ; *src ; src++ ){
		if ( (*src == ':') && (src > commandline) &&
				((*(src - 1) == 'Z') || (*(src - 1) == 'z')) ){
			fixsep = TRUE;
			dst--;
			continue;
		}else if ( IsTrue(fixsep) ){
			if ( (*src == ' ') || (*src == '\t') ){
				fixsep = FALSE;
			}else if ( *src == '\\' ){
				*dst++ = '/';
				continue;
			}
		}
		*dst++ = *src;
	}
	*dst = '\0';

	GetCurrentDirectory(TSIZEOF(OldCurrentDir), OldCurrentDir);
#ifdef UNICODE
	UnicodeToUtf8(CurrentDir, bufA, CMDLINESIZE);

	while ( (chpath = strchr(bufA, '\\')) != NULL ) *chpath = '/';
	chpath = bufA;
	if ( (*(chpath + 1) == ':') &&
		( (*chpath == 'z') || (*chpath == 'Z') ) ){
		chpath += 2;
	}
	(void)chdir(chpath);
	SetCurrentDirectory(CurrentDir);
	if ( (UnicodeToUtf8(commandline, bufA, CMDLINESIZE) == 0) &&
		 (GetLastError() == ERROR_INSUFFICIENT_BUFFER) ){ // バッファが小さい
		int len;
		char *bufptr;

		len = UnicodeToUtf8(commandline, NULL, 0);
		bufptr = malloc(len);
		UnicodeToUtf8(commandline, bufptr, len);
		result = system(bufptr);
		free(bufptr);
	}else{
		result = system(bufA);
	}
#else
	strcpy(bufA, CurrentDir);
	while ( (chpath = strchr(bufA, '\\')) != NULL ) *chpath = '/';
	chpath = bufA;
	if ( (*(chpath + 1) == ':') &&
		( (*chpath == 'z') || (*chpath == 'Z') ) ){
		chpath += 2;
	}
	chdir(chpath);
	SetCurrentDirectory(CurrentDir);
	result = system(commandline);
#endif
	SetCurrentDirectory(OldCurrentDir);
	return result;
}
#endif

const TCHAR *FixCurrentDirPath(const TCHAR *path)
{
	if ( (path != NULL) && (
			(path[0] == '?') || !(
				(Isalpha(path[0]) && (path[1] == ':')) ||
				((path[0] == '\\') && (path[1] == '\\') && (tstrchr(path + 2, '\\') != NULL)) ||
				(path[0] == '/')
			)
		  ) ){
		if ( ProcTempPath[0] == '\0' ){
			MakeTempEntry(MAX_PATH, NULL, 0); // ProcTempPath を作成
		}
		path = ProcTempPath;
	}
	return path;
}

void PipeOut(HANDLE hInputPipe, EXEC_EXTRA *extra)
{
	DWORD sendlen;
	#ifdef UNICODE
		char SENDBUF[0x900];

		if ( extra->stdio.codepage == CP__UTF16L ){
			sendlen = strlenW((WCHAR *)extra->stdio.thReceive.bottom) * sizeof(WCHAR);
			if ( sendlen > (0x900 / sizeof(WCHAR)) ) sendlen = 0x900;
			memcpy(SENDBUF, extra->stdio.thReceive.bottom, sendlen);
		}else{
			sendlen = WideCharToMultiByteU8(extra->stdio.codepage, 0, (WCHAR *)extra->stdio.thReceive.bottom, -1, SENDBUF, 0x900, NULL, NULL);
			if ( sendlen > 0 ) sendlen--;
		}
	#else
		#define SENDBUF extra->stdio.thReceive.bottom
		sendlen = strlen(SENDBUF);
	#endif
	WriteFile(hInputPipe, SENDBUF, sendlen, &sendlen, NULL);
	CloseHandle(hInputPipe);
	extra->stdio.thReceive.top = 0;
	*(TCHAR *)extra->stdio.thReceive.bottom = '\0';
}

void ExecLog(HANDLE hProcess, HANDLE hOutputPipe, EXEC_EXTRA *extra)
{
	char buf[0x200], *last = buf;
	DWORD rsize;
	#define HANDLE_PROCESS (WAIT_OBJECT_0 + 0)
	#define HANDLE_OUTPIPE (WAIT_OBJECT_0 + 1)
	#define HANDLE_MSG (WAIT_OBJECT_0 + 2)
	HANDLE hList[2];
	BOOL autocp;

	#ifdef UNICODE
		WCHAR LOGBUF[0x900];
	#else
		#define LOGBUF buf
	#endif

	autocp = extra->stdio.codepage == CP_ACP;

	hList[0] = hProcess;
	hList[1] = hOutputPipe;
	for ( ;; ){ // リダイレクトしてログ表示する
		DWORD waitresult;
		char *ptr, backup_c;

		waitresult = MsgWaitForMultipleObjects(
				2, hList, FALSE, 500, QS_ALLEVENTS);
		if ( waitresult == HANDLE_MSG ){ // Message
			PeekMessageLoop(NULL);
		}

		if ( PeekNamedPipe(hOutputPipe, NULL, 0, NULL, &rsize, NULL) == FALSE ){
			break;
		}

		if ( rsize == 0 ){
			if ( waitresult != HANDLE_OUTPIPE ){
				if ( waitresult == HANDLE_MSG ) continue;
				if ( waitresult == WAIT_TIMEOUT ) continue; // 待機継続
				break; // 実行完了 & 結果無し
			}
		}

		if ( ReadFile(hOutputPipe, last, sizeof(buf) - (last - buf) - 2, &rsize, NULL) == FALSE ){
			break;
		}
		if ( rsize == 0 ) break;

		rsize += last - buf;
		buf[rsize] = '\0';

		// Multibyte の境界となる末尾を探す
		ptr = buf;

		if ( autocp ){
			autocp = FALSE;
			if ( (rsize > 2) && (buf[1] == 0) ){ // utf16 の簡易判別
				extra->stdio.codepage = CP__UTF16L;
			}
		}

		if ( extra->stdio.codepage == CP_UTF8 ){
			for (;;){
				char *skipptr;
				unsigned char code;
				int len;

				code = (unsigned char)*ptr;
				if ( code == '\0' ) break;
				if ( (code < 0xc0) || (code >= 0xf5) ){ // 1byte code & 異常の2byte以降
					ptr++;
					continue;
				}
				// mutibyte
				skipptr = ptr + 1;
				len = (code < 0xe0) ? 1 : ( (code < 0xf0) ? 2 : 3);
				for (;;){
					code = (unsigned char)*skipptr++;
					if ( code == '\0' ) goto breaktext; // 中断している
					len--;
					if ( len == 0 ) break;
				}
				ptr = skipptr;
			}
			breaktext: ;
		}else if ( extra->stdio.codepage == CP_ACP ){
			for (;;){
				if ( *ptr == '\0' ) break;
				if ( !IskanjiA(*ptr) ){ // 1byte code
					ptr++;
					continue;
				}
				// mutibyte
				if ( *(ptr + 1) == '\0' ){
					break;
				}
				ptr += 2;
			}
		}else{
			ptr = buf + rsize;
		}
		backup_c = *ptr;
		*ptr = '\0';

		#ifdef UNICODE
		if ( extra->stdio.codepage == CP__UTF16L ){
			memcpy(LOGBUF, buf, ptr - buf);
			LOGBUF[(ptr - buf) / sizeof(WCHAR)] = '\0';
		}else{
			MultiByteToWideCharU8(extra->stdio.codepage, 0, buf, -1, LOGBUF, TSIZEOFW(LOGBUF));
		}
		#else
		if ( extra->stdio.codepage != CP_ACP ){
			WCHAR bufW[sizeof(buf) * 2];
			DWORD csize;

			if ( extra->stdio.codepage == CP__UTF16L ){
				memcpy(bufW, buf, rsize);
				csize = rsize / sizeof(WCHAR);
			}else{
				csize = MultiByteToWideCharU8(extra->stdio.codepage, 0, buf, rsize, bufW, TSIZEOFW(bufW));
			}
			csize = WideCharToMultiByte(CP_ACP, 0, bufW, csize, LOGBUF, sizeof(LOGBUF) - 1, NULL, NULL);
			LOGBUF[csize] = '\0';
		}
		#endif

		if ( extra->mask & (EEM_STDOUT_value | EEM_STDOUT_receive) ){
			ThCatString(&extra->stdio.thReceive, LOGBUF);
		}

		if ( ((extra->mask & EEM_STDOUT_log) &&
			  (PPxInfoFunc(extra->info, PPXCMDID_REPORTTEXT, LOGBUF) != PPXCMDID_EXTREPORTTEXT_LOG)) ||
			 (extra->mask & EEM_STDOUT_PPe) ){
			if ( extra->stdio.hPPeWnd == NULL ){
				extra->stdio.hPPeWnd = PPEui(NULL, T("Log"), LOGBUF);
				if ( extra->stdio.hPPeWnd == NULL ){
					extra->stdio.hPPeWnd = BADHWND;
				}
			}else{
				HWND hEdit;

				hEdit = (HWND)SendMessage(extra->stdio.hPPeWnd, WM_PPXCOMMAND, KE_getHWND, 0);
				SendMessage(hEdit, EM_SETSEL, EC_LAST, EC_LAST);
				SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)LOGBUF);
			}
		}


		*ptr = backup_c;
		last = buf;
		if ( ptr >= (buf + rsize) ) continue;
		// 未処理の部分を次回に回す
		for (;;){
			char *oldlast;

			oldlast = last;
			last = stpcpyA(last, ptr);
			ptr += (last - oldlast);
			if ( ptr >= (buf + rsize) ) break;
			ptr++; // skip \0
		}
	}
}

typedef struct {
	HANDLE read;
	HANDLE write;
} PIPES;


//CREATE_PROTECTED_PROCESS		0x00040000			Vista以降
//CREATE_PRESERVE_CODE_AUTHZ_LEVEL	0x02000000		XP以降
/*-----------------------------------------------------------------------------
	自前で外部プロセスを起動する
-----------------------------------------------------------------------------*/
BOOL ComExecSelf(HWND hOwner, const TCHAR *execarg, const TCHAR *path, int flags, EXEC_EXTRA *extra)
{
	TCHAR LineStaticBuf[CMDLINESIZE + MAX_PATH], *LineBuf = LineStaticBuf;
	TCHAR *exeterm, *paramdest;
	size_t linelen;

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	const TCHAR *param;
	int exetype;
	int result;
	DWORD createflags = CREATE_DEFAULT_ERROR_MODE;
	PIPES hInputPipe, hOutputPipe;

	param = execarg;
	while( (*param == ' ') || (*param == '\t') ) param++;
	exetype = GetExecType(&param, LineStaticBuf, path);

	if ( exetype == GTYPE_SHELLEXEC ){	// ShellExecute で起動
		return ComExecSelfShell(hOwner, execarg, LineStaticBuf, param, path, flags, extra);
	}
	if ( (exetype == GTYPE_ERROR) && (flags & XEO_NOCMDCMD) ){
		return FALSE;
	}
	if ( !(flags & XEO_NOSCANEXE) && (CheckExebin(LineStaticBuf, exetype) == FALSE) ){
		return FALSE;
	}
	// LineStaticBuf で足りないならメモリ確保
	// LineStaticBuf(コマンド名) + param(パラメータ) + %COMSPEC% + " /C " + 「"」で括る分
	linelen = tstrlen(LineStaticBuf) + tstrlen(param) + MAX_PATH + 6;
	if ( linelen > TSIZEOF(LineStaticBuf) ){
		LineBuf = HeapAlloc(DLLheap, 0, TSTROFF(linelen));
		if ( LineBuf == NULL ) return FALSE;
		tstrcpy(LineBuf, LineStaticBuf);
	}
	if ( (flags & XEO_CONSOLE) && (exetype == GTYPE_CONSOLE) ){
		setflag(flags, XEO_SEQUENTIAL);
	}

	// unknown / data / cmd
	if ( (exetype == GTYPE_ERROR) || (exetype == GTYPE_DATA) || (flags & XEO_USECMD) ){
	#ifndef WINEGCC // unknown / data ... cmd 経由
		BOOL sepfix = FALSE;

		// CMD 関連
		if ( (SkipSpace(&execarg) == '\"') && (tstrchr(param, '\"') != NULL) ){
			sepfix = TRUE;
		}

		if ( tstrlen(param) > 8000 ){ // CMD の長さ制限
			PPErrorMsg(LineStaticBuf, RPC_S_STRING_TOO_LONG);
			ExecErrorMessage(hOwner, T("ComExecSelf"), LineStaticBuf);
			SetLastError(RPC_S_STRING_TOO_LONG);
			result = FALSE;
			goto fin;
		}
		// %COMSPEC% 取得
		if ( GetEnvironmentVariable(ComSpecStr, LineBuf, MAX_PATH) == 0 ){
			#ifdef UNICODE
				tstrcpy(LineBuf, T("CMD.EXE"));
			#else
				tstrcpy(LineBuf, T("COMMAND.COM"));
			#endif // UNICODE
		}
		exeterm = paramdest = LineBuf + tstrlen(LineBuf);
		tstrcpy(paramdest, T(" /C "));
		paramdest += 4;
		// cmd は「"file name.bat" param」は成功し、「"file name.bat" "param"」
		// で失敗するので、「""file name.bat" "param"」に加工する
		if ( IsTrue(sepfix) ) *paramdest++ = '\"';
		param = execarg;
	#else // WINEGCC // system で実行する
		tstrcpy(LineBuf, execarg);
		result = ComExecSystem(LineBuf, path);
		if ( (extra != NULL) && (extra->ExitCode != NULL) ){
			*extra->ExitCode = (DWORD)result;
		}
		result = (result == -1) ? FALSE : TRUE;
		goto fin;
	#endif // WINEGCC
	}else{
		exeterm = paramdest = LineBuf + tstrlen(LineBuf);
		if ( *param != ' ' ) *paramdest++ = ' ';
	}

	tstrcpy(paramdest, param);
											// 実行条件の指定
	memset(&si, 0, sizeof(si));
	si.cb			= sizeof(si);
//	si.lpReserved	= NULL;
//	si.lpDesktop	= NULL;
//	si.lpTitle		= NULL;
//	si.dwFlags		= 0;
//	si.cbReserved2	= 0;
//	si.lpReserved2	= NULL;
	si.wShowWindow	= SW_SHOWDEFAULT;

	if ( flags & (XEO_MAX | XEO_MIN | XEO_NOACTIVE | XEO_HIDE) ){
		si.dwFlags = STARTF_USESHOWWINDOW;
		if ( flags & XEO_MAX ) si.wShowWindow = SW_SHOWMAXIMIZED;
		if ( flags & XEO_MIN ) si.wShowWindow = SW_SHOWMINNOACTIVE;
		if ( flags & XEO_NOACTIVE ) si.wShowWindow = SW_SHOWNOACTIVATE;
		if ( flags & XEO_HIDE ){
			si.wShowWindow = SW_HIDE;
			if ( !(flags & XEO_USECMD) ){ // リダイレクトするときは DETACHED 不可
				setflag(createflags, DETACHED_PROCESS);
			}
		}
	}
	if ( flags & XEO_LOW ) setflag(createflags, IDLE_PRIORITY_CLASS);
	result = TRUE;

	if ( (extra != NULL) && (hOwner != NULL) && ((extra == NULL) || !(extra->mask & EEM_NoStartMsg)) ){
		const TCHAR *exef;

		exef = MessageText(MES_EXEF);
		if ( *exef != '\0' ){
			SendMessage(hOwner, WM_PPXCOMMAND, K_SETPOPLINENOLOG, (LPARAM)exef);
			UpdateWindow(hOwner);
		}
	}
	if ( extra != NULL ){
		if ( extra->mask & EEM_UseECF ){
			setflag(createflags, extra->ExtraCreateFlags);
		}
		if ( extra->mask & EEM_UseESF ){
			setflag(si.dwFlags, extra->ExtraStartup.flags);
			if ( si.dwFlags & STARTF_MONITOR ){
				si.hStdOutput = extra->ExtraStartup.hStdOut;
			}
			if ( si.dwFlags & STARTF_USEPOSITION ){
				si.dwX = extra->ExtraStartup.x;
				si.dwY = extra->ExtraStartup.y;
			}
			if ( si.dwFlags & STARTF_USESIZE ){
				si.dwXSize = extra->ExtraStartup.width;
				si.dwYSize = extra->ExtraStartup.height;
			}
		}
		if ( extra->mask & EEM_UseSTDIO ){
			SECURITY_ATTRIBUTES sa;

			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = TRUE;

									// STDIN パイプ
			if ( CreatePipe(&hInputPipe.read, &hInputPipe.write, &sa, 0) ){
				SetHandleInformation(hInputPipe.write, HANDLE_FLAG_INHERIT, 0);
			}else{
				hInputPipe.read = NULL;
			}
			si.hStdInput = hInputPipe.read;

									// STDOUT/STDERR パイプ
			if ( CreatePipe(&hOutputPipe.read, &hOutputPipe.write, &sa, 0) ){
				SetHandleInformation(hOutputPipe.read, HANDLE_FLAG_INHERIT, 0);
			}else{
				hOutputPipe.write = NULL;
			}
			si.hStdError = si.hStdOutput = hOutputPipe.write;
		}
	}

	path = FixCurrentDirPath(path);

	if ( ((extra != NULL) && (extra->mask & EEM_DesktopLevel)) ?
		 IsTrue(CreateProcessDesktopLevel(LineBuf,
				createflags, path, &si, &pi)) :
		 IsTrue(CreateProcess(NULL, LineBuf, NULL, NULL,
				(si.dwFlags & STARTF_USESTDHANDLES) ? TRUE : FALSE,
				createflags, NULL, path, &si, &pi)) ){
		CloseHandle(pi.hThread);
		if ( extra != NULL ){
			if ( extra->mask & EEM_GETID ){
				extra->ProcessID = pi.dwProcessId;
				extra->ThreadID = pi.dwThreadId;
			}
			if ( extra->mask & EEM_UseSTDIO ){
				// 子プロセスに継承済みのハンドルを閉じる
				if ( hInputPipe.read != NULL ){
					CloseHandle(hInputPipe.read);
				}
				if ( hOutputPipe.write != NULL ){
					CloseHandle(hOutputPipe.write);
				}

				if ( hInputPipe.read != NULL ){
					if ( extra->mask & EEM_STDIN_value ){
						PipeOut(hInputPipe.write, extra);
					}
					CloseHandle(hInputPipe.write); // 標準入力リダイレクト終了
				}

				if ( hOutputPipe.write != NULL ){
					if ( extra->mask & (EEM_STDOUT_receive | EEM_STDOUT_value | EEM_STDOUT_log | EEM_STDOUT_PPe) ){
						ExecLog(pi.hProcess, hOutputPipe.read, extra);
					}
					CloseHandle(hOutputPipe.read); // 標準出力リダイレクト終了
				}
			}
		}
		if ( flags & (XEO_WAITIDLE | XEO_SEQUENTIAL) ){
			result = WaitProcessDialog(hOwner, pi.hProcess, execarg, flags, extra);
		}
		CloseHandle(pi.hProcess);
	}else{
		ERRORCODE errorcode;
		TCHAR *exefilename;

		if ( (extra != NULL) && (extra->mask & EEM_UseSTDIO) ){
			if ( hInputPipe.read != NULL ){
				CloseHandle(hInputPipe.read);
				CloseHandle(hInputPipe.write);
			}
			if ( hOutputPipe.write != NULL ){
				CloseHandle(hOutputPipe.read);
				CloseHandle(hOutputPipe.write);
			}
		}

		exefilename = LineBuf;
		if ( *exefilename == '\"' ){ // 「"」 括りを除去
			exefilename++;
			exeterm--;
		}
		*exeterm = '\0';

		errorcode = GetLastError();
		if ( errorcode == ERROR_ELEVATION_REQUIRED ){ // UAC が必要
			// ※但し、Vistaは一度このエラーが来ると互換性アシスタントがでて、
			//   それ以降はエラーが出なくなる
			result = ComExecSelfShell(hOwner, execarg, exefilename, param, path, flags, extra);
		}else{
			errorcode = GetExecuteErrorReason(exefilename, LineStaticBuf);
			ExecErrorMessage(hOwner, T("ComExecSelf"), LineStaticBuf);
			SetLastError(errorcode);
			result = FALSE;
		}
	}
	if ( (hOwner != NULL) && IsTrue(result) && ((extra == NULL) || !(extra->mask & EEM_NoStartMsg)) ){
		SendMessage(hOwner, WM_PPXCOMMAND, K_SETPOPLINENOLOG, 0);
	}

fin:
	if ( LineBuf != LineStaticBuf ) HeapFree(DLLheap, 0, LineBuf);
	return result;
}
/*-----------------------------------------------------------------------------
	ShellExecute を実行し、エラーがでたら文字列を取得する。
	失敗したら NULL を返す
	※HANDLE はXEO_WAITIDLE | XEO_SEQUENTIAL でなければ開放不要である
-----------------------------------------------------------------------------*/
PPXDLL HANDLE PPXAPI PPxShellExecute(HWND hwnd, LPCTSTR lpOperation, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, int flag, TCHAR *ErrMsg)
{
	SHELLEXECUTEINFO ExeInfo;

	if ( !(flag & XEO_NOSCANEXE) && (CheckExebin(lpFile, -1) == FALSE) ){
		SetLastError(PPErrorMsg(ErrMsg, ERROR_CANCELLED));
		return NULL;
	}

	ExeInfo.cbSize = sizeof(ExeInfo);
	ExeInfo.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE;
	ExeInfo.hwnd = hwnd;
	ExeInfo.lpVerb = lpOperation;
	ExeInfo.lpFile = lpFile;
	ExeInfo.lpParameters = lpParameters;
	ExeInfo.lpDirectory = FixCurrentDirPath(lpDirectory);
	ExeInfo.nShow = SW_SHOWNORMAL;
	if ( flag & XEO_MAX ) ExeInfo.nShow = SW_SHOWMAXIMIZED;
	if ( flag & XEO_MIN ) ExeInfo.nShow = SW_SHOWMINNOACTIVE;
	if ( flag & XEO_NOACTIVE ) ExeInfo.nShow = SW_SHOWNOACTIVATE;
	if ( flag & XEO_HIDE ){
		ExeInfo.nShow = SW_HIDE;
//		if ( !(flag & XEO_USECMD) ){ // リダイレクトするときは DETACHED 不可
//			setflag(createflag, DETACHED_PROCESS);
//		}
//		setflag(createflag, DETACHED_PROCESS);
	}
//	if ( flag & XEO_LOW ) setflag(createflag, IDLE_PRIORITY_CLASS);

	if ( flag & (XEO_WAITIDLE | XEO_SEQUENTIAL) ){
		setflag(ExeInfo.fMask, SEE_MASK_NOCLOSEPROCESS);
	}
	if ( IsTrue(ShellExecuteEx(&ExeInfo)) ){
		if ( (flag & (XEO_WAITIDLE | XEO_SEQUENTIAL)) &&
				(ExeInfo.hProcess != NULL) ){
			return ExeInfo.hProcess;
		}
		return INVALID_HANDLE_VALUE;
	}else{
		GetExecuteErrorReason(lpFile, ErrMsg);
		return NULL;
	}
}

//====================================================== 起動するプロセスの判別
// ファイルの拡張子からファイルの種類を判別
int GetExecTypeByExt(const TCHAR *fname)
{
	DWORD Bsize;				// バッファサイズ指定用

	TCHAR appN[MAX_PATH];		// アプリケーションのキー
	TCHAR defN[MAX_PATH];		// デフォルトのアクション
	HKEY hExt, hAP;
	TCHAR *ext;
	int type = GTYPE_SHELLEXEC; //GTYPE_DATA;

	ext = VFSFindLastEntry(fname);
	ext += FindExtSeparator(ext);
										// 拡張子からキーを求める -------------
	if ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, ext, 0, KEY_READ, &hExt) ){
		if ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, WildCard_All, 0, KEY_READ, &hExt) ){
			return type; // 調べることができないので OS に任せる
		}
	}
	Bsize = sizeof appN - TSTROFF(7);			// 拡張子の識別子
	if ( ERROR_SUCCESS ==
			RegQueryValueEx(hExt, NilStr, NULL, NULL, (LPBYTE)appN, &Bsize) ){
		tstrcat(appN, T("\\shell"));
										// アプリケーションのシェル -----------
		if ( ERROR_SUCCESS ==
				RegOpenKeyEx(HKEY_CLASSES_ROOT, appN, 0, KEY_READ, &hAP)){
			Bsize = sizeof defN - TSTROFF(9);
			defN[0] = '\0';
			RegQueryValueEx(hAP, NilStr, NULL, NULL, (LPBYTE)defN, &Bsize);
			if ( defN[0] == '\0' ) tstrcpy(defN, ShellVerb_open); // の指定が無い

			tstrcat(defN, T("\\command"));
			if ( GetRegString(hAP, defN, NilStr, appN, TSIZEOF(appN)) ){
				TCHAR *ptr, *q = NULL;

				ptr = appN;
				while( (*ptr == ' ') || (*ptr == '\t') ) ptr++;
				if ( *ptr != '\0' ){
					if ( *ptr == '\"' ){
						ptr++;
						q = tstrchr(ptr, '\"');
					}else{
						q = tstrchr(ptr, ' ');
						if ( q == NULL ) q = tstrchr(ptr, '\t');
					}
				}
				if ( q == NULL ) q = ptr + tstrlen(ptr);
				*q = '\0';
				if ( tstrcmp(ptr, T("%1")) ){
					type = GetExecTypeFileOnly(ptr);

					if ( (type == GTYPE_SHELLEXEC) || (type == GTYPE_GUI) ){
						type = GTYPE_SHELLEXEC;	// GUI型
					}else{
						type = GTYPE_DATA;		// CUI型
					}
				}else{	// 自分自身で実行可能→ COM, EXT, BAT, CMD の可能性
					type = GTYPE_DATA;
				}
			}
			RegCloseKey(hAP);
		}
	}
	RegCloseKey(hExt);
	return type;
}

// %LOCALAPPDATA%\Microsoft\WindowsApps 内のシンボリックリンクは開けないので
// AppPathKey 内のファイルと同じと推測して開いてみる
HANDLE OpenWinappFile(const TCHAR *fname)
{
	TCHAR envpath[1024], exepath[VFPS];
	const TCHAR *ptr;

	ptr = VFSFindLastEntry(fname);
	if ( *ptr == '\\' ) ptr++;
	thprintf(envpath, TSIZEOF(envpath), AppPathKey, ptr);
	if ( (GetRegString(HKEY_CURRENT_USER, envpath, NilStr, exepath, TSIZEOF(exepath)) == FALSE) &&
		 (GetRegString(HKEY_LOCAL_MACHINE, envpath, NilStr, exepath, TSIZEOF(exepath)) == FALSE) ){
		return INVALID_HANDLE_VALUE;
	}

	return CreateFileL(exepath, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

/*-----------------------------------------------------------------------------
	指定位置のみでプロセスを調べる
-----------------------------------------------------------------------------*/
int GetExecTypeFileOnly(const TCHAR *name)
{
	HANDLE hFile;
	DWORD attr;
	int type = GTYPE_DATA;
										// ファイルの先頭を取得する -----------
	hFile = CreateFileL(name, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ){
		if ( GetLastError() == ERROR_CANT_ACCESS_FILE ){
			hFile = OpenWinappFile(name);
		}
		if ( hFile == INVALID_HANDLE_VALUE ){
			// 存在しない(BADATTR),ファイル,ラベルならエラー
			attr = GetFileAttributesL(name);
			if ( attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_LABEL) ){
				return GTYPE_ERROR;
			}
			if ( attr & FILE_ATTRIBUTE_REPARSE_POINT ){ // ファイルが開けないリンク→WindowsApp と思われる ※コンソールアプリかの判断方法は不明
				return GTYPE_GUI;
			}
			return GTYPE_SHELLEXEC; //GTYPE_DATA
		}
	}

	for ( ; ; ){
		WORD wp;
		DWORD tmp;
		const TCHAR *ptr;
		BYTE header[max(sizeof(IMAGE_DOS_HEADER), 0x60)];

		if ( FALSE == ReadFile(hFile, header, sizeof(IMAGE_DOS_HEADER), &tmp, NULL) ){
			type = GTYPE_SHELLEXEC; //GTYPE_ERROR;
			break;	// 読み込み失敗
		}
						// MS EXE header を確認
		if ( (tmp < sizeof(IMAGE_DOS_HEADER)) ||
			 (((IMAGE_DOS_HEADER *)header)->e_magic != IMAGE_DOS_SIGNATURE) ){
			type = GetExecTypeByExt(name); // データ、batch、com
			break;
		}
						// 拡張子の確認
		ptr = tstrrchr(name, '.');
		if ( ptr == NULL ) break;
		if ( tstricmp(ptr + 1, T("COM")) && tstricmp(ptr + 1, T("EXE")) && tstricmp(ptr + 1, T("SCR")) ){ // com/exe/scr 以外は DATA 扱い
			break;
		}

		type = GTYPE_CONSOLE;
		tmp = ((IMAGE_DOS_HEADER *)header)->e_lfanew;	// 拡張ヘッダoffset
													// DOS EXE
		if ( tmp == 0 ) break;
		if ( SetFilePointer(hFile, tmp, NULL, FILE_BEGIN) != tmp ) break;
		if ( ReadFile(hFile, header, 0x60, &tmp, NULL) == FALSE ) break;
		if ( tmp < 0x60 ) break;
						// NE header を確認				WIN16, OS/2
		if ( ((IMAGE_OS2_HEADER *)header)->ne_magic == IMAGE_OS2_SIGNATURE ){
			type = GTYPE_GUI;
			break;
		}
						// PE header を確認				DOS EXE
		if ( ((IMAGE_NT_HEADERS *)header)->Signature != IMAGE_NT_SIGNATURE ){
			break;
		}
		wp = ((IMAGE_NT_HEADERS *)header)->OptionalHeader.Subsystem;
		if ( (wp == IMAGE_SUBSYSTEM_UNKNOWN) ||
			 (wp >= IMAGE_SUBSYSTEM_WINDOWS_CUI) ){ // ネイティブ・GUI 以外
			break;
		}
		type = GTYPE_GUI;
		break;
	};
	CloseHandle( hFile );
	return type;
}
/*-----------------------------------------------------------------------------
	指定ディレクトリで拡張子を補完してプロセスを調べる
-----------------------------------------------------------------------------*/
int GetExecTypeDirOnly(TCHAR *name, int flag)
{
	int type;
	TCHAR *ptr, *q, *namelast;
	TCHAR ext[2048];
										// 素のままで調べる ===================
	type = GetExecTypeFileOnly(name);
	if ( type != GTYPE_ERROR ) return type;
	if ( flag & ET_EXT ) return GTYPE_ERROR;	// 拡張子があるのでここで終了

										// 拡張子を付加して検索 ===============
	ptr = ext;
	if ( GetEnvironmentVariable(PathextStr, ext, TSIZEOF(ext)) == 0 ){
		ptr = T(".COM;.EXE;.BAT");
	}
	namelast = name + tstrlen(name);
	while( *ptr != '\0' ){
		q = tstrchr(ptr, ';');
		if ( q == NULL ) q = ptr + tstrlen(ptr);
		memcpy(namelast, ptr, TSTROFF(q - ptr));
		*(namelast + (q - ptr)) = '\0';
		type = GetExecTypeFileOnly(name);
		if ( type != GTYPE_ERROR ) return type;
		ptr = q;
		if ( *ptr != '\0' ) ptr++;
	}
	return GTYPE_ERROR;
}

void FileNameCopy(TCHAR *dest, const TCHAR *src)
{
	if ( dest == NULL ) return;
	if ( tstrchr(src, ' ') != NULL ){
		*dest++ = '\"';
		dest = tstpcpy(dest, src);
		*(dest + 0) = '\"';
		*(dest + 1) = '\0';
	}else{
		tstrcpy(dest, src);
	}
}

const TCHAR URLprotocol[] = T("URL protocol");
BOOL IsSchemeName(TCHAR *drive, int len)
{
	TCHAR buf[2];
	BOOL result;

	if ( len <= 2 ) return FALSE; // A: AB: とかは無視する
	drive[len] = '\0';
	result = GetRegString(HKEY_CLASSES_ROOT, drive, URLprotocol, buf, sizeof(buf));
	drive[len] = ':';
	return result;
}

/*-----------------------------------------------------------------------------
	ファイルの場所と種類を調べる & 実行名とパラメータを分離する
	※fpath は「"」付で出力されることあり
-----------------------------------------------------------------------------*/
PPXDLL int PPXAPI GetExecType(LPCTSTR *name, TCHAR *fpath, const TCHAR *path)
{
	TCHAR fname[VFPS], envpath[1024], exepath[VFPS];
	const TCHAR *p;
	int flag = 0;
	int type;
									// 実行ファイルを exepath に抽出 ========
	{
		const TCHAR *lastp;

		p = *name;
		while( (*p == ' ') || (*p == '\t') ) p++;
		if ( *p == '\0' ) goto generror;
		if ( *p == '\"' ){
			p++;
			lastp = tstrchr(p, '\"');
		}else{
			lastp = p;
			// \0 \t \r \n ' ' 以外
			while( ((UTCHAR)*lastp > '\xd') && (*lastp != ' ') ){
				lastp++;
			}
		}
		if ( lastp == NULL ) lastp = p + tstrlen(p);
		if ( (lastp - p) >= VFPS ) goto generror; // コマンドが長すぎ OVER_VFPS_MSG
		memcpy(exepath, p, TSTROFF(lastp - p));
		*(exepath + (lastp - p)) = '\0';

		*name = lastp;
		if ( *lastp != '\0' ) (*name)++;
	}
										// 実行ファイルパスの形式を解析 =======
	p = exepath;
	for ( ; ; p++ ){
		TCHAR chr;

		chr = *p;
		if ( chr == '\0' ) break;

		if ( chr == ':' ){
			if ( !(flag & ET_PATHMASK) ){
				int len;

				len = p - exepath;
				if ( ((len == 4) && !memcmp(exepath, T("http"), TSTROFF(4))) ||
					 ((len == 5) && !memcmp(exepath, T("https"), TSTROFF(5))) ||
					 IsSchemeName(exepath, len) ){
					if ( fpath != NULL ) tstrcpy(fpath, exepath);
					return GTYPE_SHELLEXEC;
				}
				flag = (flag & ~ET_EXT) | ET_DRIVE;
			}
			continue;
		}

		if ( (chr == '/') || (chr == '\\') ){
			flag = (flag & ~ET_EXT) | ET_DIRECTORY;
			continue;
		}

		if ( chr == '.'){
			setflag(flag, ET_EXT);
			continue;
		}
#ifndef UNICODE
		if ( IskanjiA(chr) && (*(p + 1) != '\0') ) p++;
#endif
	}
							// カレントディレクトリ／指定パスで検索 =========
	if ( flag & ET_PATHMASK ){
		VFSFixPath(fname, exepath, path, VFSFIX_SEPARATOR | VFSFIX_FULLPATH |
				VFSFIX_REALPATH | VFSFIX_NOFIXEDGE);
	}else{
		VFSFullPath(fname, exepath, path);
	}
#if 0
	// ディレクトリ指定が無いときはカレントディレクトリなので検索をしない
	// ※ドライブ指定＋カレントディレクトリも禁止する
	// ●現在はここで無視しても cmd.exe で実行してしまうので効果が無い
	if ( !skipcurrent || (flag & ET_DIRECTORY) ){
		type = GetExecTypeDirOnly(fname, flag);
		if ( type != GTYPE_ERROR ){
			FileNameCopy(fpath, fname);
			return type;
		}
	}
#else
	type = GetExecTypeDirOnly(fname, flag);
	if ( type != GTYPE_ERROR ){
		FileNameCopy(fpath, fname);
		return type;
	}
#endif
	if ( flag & ET_PATHMASK ){
		SetLastError(ERROR_PATH_NOT_FOUND);
		goto error; // path 指定ありならここで終わり
	}
										// PPxディレクトリで検索 =========
	CatPath(fname, DLLpath, exepath);
	type = GetExecTypeDirOnly(fname, flag);
	if ( type != GTYPE_ERROR ){
		FileNameCopy(fpath, fname);
		return type;
	}
	{									// 環境変数 path で検索 ===============
		const TCHAR *envptr, *sep;
		TCHAR *envbuf = NULL;
		DWORD envlen;

		envptr = envpath;
		envlen = GetEnvironmentVariable(StrPath, envpath, TSIZEOF(envpath));
		if ( envlen == 0 ) goto endenv;
		if ( envlen >= (TSIZEOF(envpath) - 16) ){
			envbuf = HeapAlloc(DLLheap, 0, TSTROFF(envlen + 16));
			if ( envbuf == NULL ) goto endenv;
			GetEnvironmentVariable(StrPath, envbuf, envlen + 16);
			envptr = envbuf;
		}

		while ( *envptr != '\0' ){
			sep = tstrchr(envptr, ';');
			if ( sep == NULL ) sep = envptr + tstrlen(envptr);
			memcpy(fname, envptr, TSTROFF(sep - envptr));
			*(fname + (sep - envptr)) = '\0';
			CatPath(NULL, fname, exepath);
			type = GetExecTypeDirOnly(fname, flag);
			if ( type != GTYPE_ERROR ){
				FileNameCopy(fpath, fname);
				if ( envbuf != NULL ) HeapFree(DLLheap, 0, envbuf);
				return type;
			}
			envptr = sep;
			if ( *envptr != '\0' ) envptr++;
		}
		if ( envbuf != NULL ) HeapFree(DLLheap, 0, envbuf);
	}
endenv: ;
										// レジストリ App Paths で検索 ========
							// ※本当は%path%よりも前だが、あえてこの位置で検索
	thprintf(envpath, TSIZEOF(envpath), AppPathKey, exepath);
	fname[0] = '\0';
	if ( (GetRegString(HKEY_CURRENT_USER, envpath, NilStr, fname, TSIZEOF(fname)) == FALSE) &&
		 (GetRegString(HKEY_LOCAL_MACHINE, envpath, NilStr, fname, TSIZEOF(fname)) == FALSE)

	 ){
		if ( !(flag & ET_EXT) ){
			tstrcat(envpath, T(".exe"));
			if ( GetRegString(HKEY_CURRENT_USER, envpath, NilStr, fname, TSIZEOF(fname)) == FALSE ){
				GetRegString(HKEY_LOCAL_MACHINE, envpath, NilStr, fname, TSIZEOF(fname));
			}
		}
	}
	if ( fname[0] != '\0' ){
		type = GetExecTypeFileOnly(fname);
		if ( type != GTYPE_ERROR ){
			FileNameCopy(fpath, fname);
			return type;
		}
	}

	if ( flag & ET_EXT ){ // 拡張子有りなら、cmdで実行するコマンドでない
		if ( fpath != NULL ) tstrcpy(fpath, exepath);
		return GTYPE_SHELLEXEC;
	}

	{
		TCHAR *CmdList;
		ERRORCODE errcode = NO_ERROR;

		envpath[0] = '\0';
		CmdList = GetCustValue(T("X_cmdls"), NULL, envpath, sizeof(envpath));
		if ( CmdList != NULL ){
			if ( *CmdList != '\0' ){
				TCHAR *ptr, *nextptr;

				errcode = ERROR_PATH_NOT_FOUND;
				ptr = CmdList;
				for (;;){
					nextptr = tstrchr(ptr, ';');
					if ( nextptr != NULL ) *nextptr++ = '\0';
					if ( tstricmp(ptr, exepath) == 0 ){
						errcode = NO_ERROR;
						break;
					}
					if ( nextptr == NULL ) break;
					ptr = nextptr;
				}
			}
			if ( CmdList != envpath ) HeapFree(ProcHeap, 0, CmdList);
			SetLastError(errcode);
			goto error;
		}
	}
generror:
	SetLastError(NO_ERROR);
error:
	if ( fpath != NULL ) *fpath = '\0';
	return GTYPE_ERROR;
}

#pragma warning(suppress:6262) // スタックを使いすぎだが、現在は無視
DWORD GetExeCrc(const TCHAR *path)
{
	HANDLE hFile;
	DWORD crc;
	BYTE bin[0x8000];
	DWORD fsize;

	crc = 0;

	hFile = CreateFileL(path, GENERIC_READ, FILE_SHARE_READ, NULL,
							OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) return 0;

	if ( IsTrue(ReadFile(hFile, bin, sizeof bin, &fsize, NULL)) ){
		crc = crc32(bin, fsize, 0);
	}
	CloseHandle(hFile);
	return crc;
}

BOOL CheckExecs(const TCHAR *file, DWORD *crcs, DWORD crc)
{
	DWORD i;

	memset(crcs, 0, (MAXSTORE + 1) * sizeof(DWORD));
	GetCustTable( T("_Execs"), file, crcs, (MAXSTORE + 1) * sizeof(DWORD));
	if ( crcs[0] == 0 ) return FALSE;
	if ( crcs[0] >= MAXSTORE ) crcs[0] = MAXSTORE;
	i = crcs[0];
	while ( i ){
		crcs++;
		if ( *crcs == crc ) return TRUE;
		i--;
	}
	return FALSE;
}

#define EnableScan 0

#if EnableScan
DWORD_PTR USECDECL CebInfo(CHECKEXEBININFO *CIS, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	switch(cmdID){
		case 'C':
			tstrcpy(uptr->enums.buffer, CIS->name);
			break;
		default:
			if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
			return PPXA_INVALID_FUNCTION;
	}
	return PPXA_NO_ERROR;
}
#endif

PPXDLL BOOL PPXAPI CheckExebin(const TCHAR *path, int type)
{
	TCHAR file[VFPS], buf[VFPS * 2], info[0x600], *inf;
	const TCHAR *p;
	DWORD crc, crcs[MAXSTORE + 1];

	if ( X_execs < 0 ){
		X_execs = 0;
		GetCustData( T("X_execs"), &X_execs, sizeof(X_execs));
	}
	if ( X_execs == 0 ) return TRUE;

	VFSFixPath(buf, (TCHAR *)path, NULL, (*path == '\"') ?
		(VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_REALPATH) :
		(VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_REALPATH | VFSFIX_NOFIXEDGE) );
	if ( type < 0 ){
		p = buf;
		type = GetExecType(&p, NULL, NULL);
	}
	if ( (type != GTYPE_GUI) && (type != GTYPE_CONSOLE) ) return TRUE;

	tstrcpy(file, FindLastEntryPoint(buf));

	crc = GetExeCrc(buf);
	if ( FALSE == CheckExecs(file, crcs, crc) ){
		VFSFILETYPE vft;
		ERRORCODE err;
#if EnableScan
		TCHAR cmd[CMDLINESIZE];

		cmd[0] = '\0';
		GetCustData( T("X_execx"), &cmd, sizeof(cmd));
		if ( cmd[0] ){
			CHECKEXEBININFO cebinfo;

			cebinfo.info.Function = (PPXAPPINFOFUNCTION)CebInfo;
			cebinfo.info.Name = T("check");
			cebinfo.info.RegID = NilStr;
			cebinfo.name = buf;
			PP_ExtractMacro(NULL, (PPXAPPINFO *)&cebinfo, NULL, cmd, NULL, XEO_NOSCANEXE);
		}
#endif
		vft.flags = VFSFT_TYPE | VFSFT_TYPETEXT | VFSFT_EXT | VFSFT_INFO;
		err = VFSGetFileType(buf, NULL, 0, &vft);
		if ( err != NO_ERROR ){
			inf = T("Unknown File Type");
		}else{
			TCHAR *pp;

			pp = thprintf(info, TSIZEOF(info), T("%s:\n"), vft.typetext);
			if ( vft.info != NULL ){
				if ( tstrlen(vft.info) > 0x400 ){
					tstrcpy(pp, T("*info too large*"));
				}else{
					tstrcpy(pp, vft.info);
				}
				HeapFree(ProcHeap, 0, vft.info);
			}
			inf = info;
		}
		thprintf(buf, TSIZEOF(buf), is2, path, inf);
		if ( PMessageBox(NULL, buf, is1,
				MB_ICONQUESTION | MB_DEFBUTTON2 | MB_OKCANCEL) == IDOK ){
			if ( crcs[0] == MAXSTORE ){
				memmove(&crcs[1], &crcs[2], (MAXSTORE - 1) * sizeof(DWORD));
			}else{
				crcs[0]++;
			}
			crcs[crcs[0]] = crc;
			SetCustTable( T("_Execs"), file, crcs, (crcs[0] + 1) * sizeof(DWORD));
		}else{
			return FALSE;
		}
	}
	return TRUE;
}

#define TOKEN__Impersonation (TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID)

#ifndef TOKEN_ADJUST_SESSIONID
	#define TOKEN_ADJUST_SESSIONID  0x0100
#endif
#ifndef TokenElevationType
#define TokenElevationType 18
#endif
typedef enum  {
  xTokenElevationTypeDefault = 1,
  xTokenElevationTypeFull,
  xTokenElevationTypeLimited
} xTOKEN_ELEVATION_TYPE;

DefineWinAPI(BOOL, CreateProcessWithTokenW, (HANDLE, DWORD, LPCWSTR, LPWSTR, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION));
DefineWinAPI(HWND, GetShellWindow, (void));

#ifdef UNICODE
#define DDuplicateTokenEx DuplicateTokenEx
#else
// Windows95 RTM にはない
DefineWinAPI(BOOL, DuplicateTokenEx, (HANDLE, DWORD, LPSECURITY_ATTRIBUTES, SECURITY_IMPERSONATION_LEVEL, TOKEN_TYPE, PHANDLE));
#endif

BOOL CreateProcessDesktopLevel(TCHAR *CommandLine, DWORD CreationFlags, const TCHAR *CurrentDirectory, STARTUPINFO *si, PROCESS_INFORMATION *pi)
{
	HANDLE hDesktopProcess, hDesktopToken, hCurToken, hAdvapi32;
	DWORD DesktopPID = 0;
	BOOL result = FALSE;

	hAdvapi32 = GetModuleHandle(T("advapi32.dll"));
	GETDLLPROC(hAdvapi32, CreateProcessWithTokenW);
	GETDLLPROC(GetModuleHandle(T("user32.dll")), GetShellWindow);
	#ifndef UNICODE
	GETDLLPROC(hAdvapi32, DuplicateTokenEx);
	#endif
	if ( (DCreateProcessWithTokenW == NULL) || (DGetShellWindow == NULL) ){
		return FALSE;
	}

	if ( OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hCurToken) ){
		TOKEN_PRIVILEGES tp;

		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_INCREASE_QUOTA_NAME, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hCurToken, FALSE, &tp, 0, NULL, NULL);
		CloseHandle(hCurToken);
	}

	GetWindowThreadProcessId(DGetShellWindow(), &DesktopPID);
	hDesktopProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, DesktopPID);
	if ( hDesktopProcess != NULL ){
		if ( OpenProcessToken(hDesktopProcess, TOKEN_DUPLICATE, &hDesktopToken) ){
			HANDLE hImpDesktopToken;

			if ( DDuplicateTokenEx(hDesktopToken, TOKEN__Impersonation, NULL,
					SecurityImpersonation, TokenPrimary, &hImpDesktopToken) ){
			#ifdef UNICODE
				#define PARAMW CommandLine
				#define CURDIRW CurrentDirectory
				#define SIW si
			#else
				#define SIW &siw
				STARTUPINFOW siw;
				WCHAR CURDIRW[MAX_PATH];
				WCHAR PARAMW[CMDLINESIZE];

				AnsiToUnicode(CommandLine, PARAMW, TSIZEOFW(PARAMW));
				AnsiToUnicode(CurrentDirectory, CURDIRW, TSIZEOFW(CURDIRW));

				memcpy(&siw, si, (sizeof(siw) < si->cb) ? sizeof(siw) : si->cb);
				siw.cb = sizeof(siw);
				siw.lpReserved = NULL;
				siw.lpDesktop = NULL;
				siw.lpTitle = NULL;
			#endif
				if ( DCreateProcessWithTokenW(hImpDesktopToken, 0, NULL,
						PARAMW, CreationFlags, NULL, CURDIRW, SIW, pi) ){
					result = TRUE;
				}
				CloseHandle(hImpDesktopToken);
			}
			CloseHandle(hDesktopToken);
		}
		CloseHandle(hDesktopProcess);
	}
	return result;
}
