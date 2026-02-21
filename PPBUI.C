/*-----------------------------------------------------------------------------
	Paper Plane basicUI			Main
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <wincon.h>
#include <string.h>
#include "PPXVER.H"
#include "PPX.H"
#include "VFS.H"
#include "TCONSOLE.H"
#include "PPB.H"
#pragma hdrstop

HWND hBackWnd = NULL;	// 最小化時に戻る Window
int RegNo = -1;			// PPx Register ID
DWORD ExitCode = EXIT_SUCCESS; // 実行したプロセスの終了コード
TCHAR RegID[REGIDSIZE] = T("B_");	// PPx Register ID
TCHAR *CurrentPath;		// 現在の実行パス(EditPath or ExtPath)
TCHAR PPxPath[VFPS];	// PPx 本体のあるパス
TCHAR EditPath[VFPS];	// PPb command line が保持するパス
TCHAR ExtPathBuf[VFPS], *ExtPath;	// 外部実行時に使用するパス
HANDLE hCommSendEvent = NULL; // 外部からのコマンド受け付け用イベント
							// signal:受付可能
HANDLE hCommIdleEvent = NULL; // 外部からのコマンド受け付け用イベント
							// signal:作業中
ThSTRUCT LongParam = ThSTRUCT_InitData;	// 長いコマンドライン用
ThSTRUCT ResultText = ThSTRUCT_InitData;	// %*extract用
DWORD BreakTick = 0, BreakCount = 0;

TCHAR WndTitle[WNDTITLESIZE] = T("PPB[a]");
TCHAR RegCID[REGIDSIZE - 1] = T("BA");

const TCHAR PPbMainThreadName[] = T("PPb main");
const TCHAR PPbOptionError[] = T("Bad option\r\n");
const TCHAR PPbMainTitle[] = T("PPbui V") T(FileProp_Version)
		T("(") RUNENVSTRINGS T(") ") TCopyright T("\n");

THREADSTRUCT threadstruct = {PPbMainThreadName, XTHREAD_ROOT, NULL, 0, 0};
PPXAPPINFO ppbappinfo = {(PPXAPPINFOFUNCTION)PPbInfoFunc, T("PPb"), RegID, NULL};

/*-----------------------------------------------------------------------------
	SetConsoleCtrlHandler で登録されるハンドラ
-----------------------------------------------------------------------------*/
BOOL SysHotKey(DWORD dwCtrlType)
{
	switch (dwCtrlType){
		case CTRL_CLOSE_EVENT:				// 終了処理
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			ReleasePPB();
			return FALSE; // 次のハンドラへ…強制終了
		case CTRL_BREAK_EVENT:				// ^BREAK
		case CTRL_C_EVENT: {				// ^C
			DWORD tick, delta;

			// 連続5回来たら、中止確認を行う
			tick = GetTickCount();
			delta = tick - BreakTick;
			BreakTick = tick;
			if ( BreakCount < (5 - 1) ){
				if ( delta > 300 ){
					BreakCount = 0;
				}else{
					BreakCount++;
				}
			}else for (;;){
				DWORD oldinmode;
				int key;
				INPUT_RECORD cin;

				tputstr(T("** Quit PPb?[Y] **"));
				GetConsoleMode(hStdin, &oldinmode);
				SetConsoleMode(hStdin, CONSOLE_IN_FLAGS_KEYONLY | ENABLE_ECHO_INPUT);
				key = tgetc(&cin);
				SetConsoleMode(hStdin, oldinmode);
				if ( (key >= 0) && (cin.EventType == KEY_EVENT) ){
					if ( TinyCharUpper(key) == 'Y' ){
						ReleasePPB();
						return FALSE; // 次のハンドラへ…強制終了
					}
					if ( key & (K_s | K_c | K_a) ) continue;
					BreakCount = 0;
					break;
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}

void CommandMacro(const TCHAR *param)
{
	ppbappinfo.Function = (PPXAPPINFOFUNCTION)PPbExecInfoFunc;
	hStdin	= GetStdHandle(STD_INPUT_HANDLE);
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	PP_ExtractMacro(NULL, &ppbappinfo, NULL, param, NULL,
			XEO_CONSOLE | XEO_NOUSEPPB);
}

const TCHAR *MoreParam(const TCHAR *param)
{
	TCHAR code;
	for (;;){
		code = *param;
		if ( Isalpha(code) ){
			param++;
			continue;
		}
		if ( (code == ' ') || (code == ':') ){
			param++;
			SkipSpace(&param);
			return param;
		}
		return NilStr;
	}
}

BOOL InitPPb(void)
{
	TCHAR buf[CMDLINESIZE * 2];
	const TCHAR *ptr;
	HWND hRwnd = BADHWND; // 起動時同期の対象ウィンドウ
	BOOL quiet = FALSE;
	int MultiBootMode = PPXREGIST_NORMAL;
	WINPOS WinPos;

	(void)CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#if NODLL
	InitCommonDll(GetModuleHandle(NULL));
#endif
	PPxRegisterThread(&threadstruct);
	FixCharlengthTable(T_CHRTYPE);
	PPxCommonExtCommand(K_ConsoleMode, ConsoleMode_Console);
//------------------------------------- カレントディレクトリ設定
	ptr = GetCommandLine();
	GetLineParamS(&ptr, PPxPath, TSIZEOF(PPxPath));
										//---------------------------- PPxPath
	GetModuleFileName(NULL, PPxPath, TSIZEOF(PPxPath));
	*VFSFindLastEntry(PPxPath) = '\0';	// ファイル名部分を除去する
										//---------------------------- EditPath
	GetCurrentDirectory(TSIZEOF(EditPath), EditPath);
										//---------------------------- ExtPath
	ExtPath = ExtPathBuf;
	tstrcpy(ExtPath, EditPath);
										//-------- プロセスのカレントを変更する
	SetCurrentDirectory(PPxPath);

	CurrentPath = EditPath;
	EditText = buf; // 仮設定
	buf[0] = '\0';
	PPxRegGetIInfo(&ppbappinfo);

	for (;;){
		if ( *ptr == '\0' ) break;
		if ( *ptr <= ' ' ){
			ptr++;
			continue;
		}
										// オプションチェック -----
		if ( (*ptr == '-') || (*ptr == '/') ){
			ptr++;
			switch ( TinyCharUpper(*ptr) ){
				case 'B': {
					const TCHAR *more;

					more = MoreParam(ptr);
					if ( Isalpha(*more) ){
						if ( (more > (ptr + 5)) &&
							 (TinyCharUpper(*(ptr + 4)) != 'M') ){ //"BOOTID:A-Z"
							MultiBootMode = PPXREGIST_IDASSIGN;
							if ( Isalpha(*(more + 1)) ){
								RegID[1] = TinyCharUpper(*more);
								RegID[2] = TinyCharUpper(*(more + 1));
							}else{
								RegID[2] = TinyCharUpper(*more);
							}
						}else{	//	"BOOTMAX:A-Z"
							MultiBootMode = PPXREGIST_MAX;
							RegID[2] = TinyCharUpper(*more);
						}
					}
					while ( (*more != '\0') && ((UTCHAR)*more > ' ') ) more++;
					ptr = more;
					break;
				}

				case 'K':
					ptr++;
					SkipSpace(&ptr);
					first_command = ptr;
					ptr = NilStr;
					// Q へ
				case 'Q':
					quiet = TRUE;
					break;

				case 'F': {
					TCHAR *top = NULL, *text;

					ptr++;
					GetLineParamS(&ptr, buf, TSIZEOF(buf));
					if ( LoadTextImage(buf, &top, &text, NULL) == NO_ERROR ){
						CommandMacro(text);
						HeapFree(GetProcessHeap(), 0, top);
					}
					goto goexit; // EXIT_SUCCESS
				}
				case 'C':
					ptr++;
					SkipSpace(&ptr);	// 空白削除
					CommandMacro(ptr);
					goto goexit; // EXIT_SUCCESS

				case 'P':
					ptr++;
					GetLineParamS(&ptr, buf, TSIZEOF(buf));
					VFSFixPath(EditPath, buf, EditPath, VFSFIX_FULLPATH | VFSFIX_REALPATH | VFSFIX_NOFIXEDGE);
					break;

				case 'I':
					ptr++;
					if ( *ptr == 'R' ){
						ptr++;
						hRwnd = (HWND)GetNumber(&ptr);
						break;
					}
					// default へ
				default:
					tputstr_noinit(PPbOptionError);
					ExitCode = EXIT_FAILURE;
					goto goexit;
			}
			continue;
		}
		GetLineParamS(&ptr, buf, TSIZEOF(buf));
	}
	//------------------------------------- 初期化
	RegNo = PPxRegist(PPXREGIST_DUMMYHWND, RegID, MultiBootMode);
	if ( RegNo < 0 ){
		ExitCode = EXIT_FAILURE;
		goto goexit;
	}

	SetErrorMode(SEM_FAILCRITICALERRORS);	// 致命的エラーを取得可能にする

	if ( tInitConsole() == FALSE ){
		tputstr_noinit(T("Can't execute PPb on this OS.\r\n"));
		ExitCode = EXIT_FAILURE;
		goto goexit;
	}
									// システム関連のイベント管理ハンドラの設定
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)SysHotKey, TRUE);
	if ( quiet == FALSE ) tputstr(PPbMainTitle);

	ppbappinfo.hWnd = hHostWnd;
										// PPCOMMON に登録
	PPxRegist(hHostWnd, RegID, PPXREGIST_SETHWND);
	RegCID[1] = RegID[2];
	WndTitle[4] = RegID[2];
	{
		const TCHAR *runasp;

		runasp = CheckRunAs();
		if ( runasp != NULL ){
			thprintf(WndTitle + 6, TSIZEOF(WndTitle) - 6, T("(%s)"), runasp);
		}
	}
										// ↓ SendPPb記載のフロー番号
										// [1] プロセス間通信用イベントの定義
	thprintf(buf, TSIZEOF(buf), T("%s%s"), PPxGetSyncTag(), RegID);

	hCommSendEvent = CreateEvent(NULL, TRUE, FALSE, buf); // nosignal
	tstrcat(buf, T("R"));
	hCommIdleEvent = CreateEvent(NULL, TRUE, FALSE, buf); // nosignal

	if ( hCommSendEvent == NULL ) tputstr(T("Event create error\n"));

											// ウィンドウ位置を再設定
	if ( UseGUI &&
		(NO_ERROR == GetCustTable(T("_WinPos"), RegCID, &WinPos, sizeof(WinPos))) ){
		SetWindowPos(hHostWnd, NULL, WinPos.pos.left, WinPos.pos.top,
				WinPos.pos.right - WinPos.pos.left,
				WinPos.pos.bottom - WinPos.pos.top,
				SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	}
//------------------------------------- 起動時同期
	if ( hRwnd != BADHWND ){
		HANDLE hRevent;

		thprintf(buf, TSIZEOF(buf), T(PPBBOOTSYNC) T("%x"), (DWORD)(DWORD_PTR)hRwnd);
		hRevent = OpenEvent(EVENT_ALL_ACCESS, FALSE, buf);
		if ( hRevent != NULL ){
			SetEvent(hRevent);
			CloseHandle(hRevent);
		}
	}
	return TRUE;
goexit:
	PPxUnRegisterThread();
	CoUninitialize();
	return FALSE;
}

/*-----------------------------------------------------------------------------
	Main Routine
-----------------------------------------------------------------------------*/
// スタートアップコードを簡略にするため、void 指定
int USECDECL main(void)
{
	int breakflag = 0;		// 0:loop
	TCHAR Cparam[CMDLINESIZE]; // 入力されたコマンドライン
	TCHAR RecvParam[RECEIVESTRINGSLENGTH]; // 受信したコマンドライン

	if ( InitPPb() == FALSE ) return ExitCode;
	PPxCommonExtCommand(K_UxTheme, KUT_INIT);
	ResettCInput(&tis, Cparam, TRUE);
	while ( breakflag >= 0 ){
											// カレントディレクトリの解放
		CurrentPath = EditPath;
		SetCurrentDirectory(PPxPath);
		TitleDisp(NULL);
		SetEvent(hCommIdleEvent); // [2]受付可能
		switch( tCInput(Cparam, TSIZEOF(Cparam), PPXH_GENERAL_R) ){
			case TCI_RECV: {	// 外部からのコマンド実行、[4][5] 内容設定通知
				int result;

				*(DWORD *)RecvParam = ExitCode;
				result = ReceiveStrings(RegID, RecvParam); // [6]内容受領
				if ( result == 0 ){
					ResetEvent(hCommSendEvent);
					tFillSpace(0, screen.info.dwCursorPosition.Y,
							screen.info.dwSize.X - 1,
							screen.info.dwCursorPosition.Y);
					CurrentPath = ExtPath;
					PPbExecuteRecv(RecvParam); // [7]実行
				}else{ // 失敗か受信
					if ( result == 2 ){ // こちらからデータ送信が必要
						if ( ResultText.size != 0 ){
							RecvParam[1] = 'r';
							if ( (ResultText.size - ResultText.top) < TSTROFF(CMDLINESIZE) ){
								RecvParam[2] = ' ';
								tstrcpy(RecvParam + 3, ThStrLastT(&ResultText) );
								ThFree(&ResultText);
							}else{
								RecvParam[2] = '+';
								tstrcpypart(RecvParam + 3, ThStrLastT(&ResultText), CMDLINESIZE - 1);
								ResultText.top += TSTROFF(CMDLINESIZE - 1);
							}
							ReceiveStrings(NULL, RecvParam);
						}
					}
					ResetEvent(hCommSendEvent); // [8] 返信
				}
				break; // [9] 再受付へ
			}
			case TCI_EXECUTE: {	// CR 実行
				if ( Cparam[0] != '\0' ){
					ResetEvent(hCommIdleEvent); // 受け付け無しに変更 [1]
					tputstr(T("\n"));
					WriteHistory(PPXH_COMMAND, Cparam, 0, NULL);
					breakflag = PPbExecuteInput(Cparam, tstrlen(Cparam));
					ResettCInput(&tis, Cparam, FALSE);
				}else{
					tstrcpy(Cparam, T("*menu")); // help処理
					TconCommonCommand(&tis, K_raw | K_c | 'A');
				}
				break;
			}
			case TCI_QUIT:		// ESC 終了
				tputstr(T("\n"));
				breakflag = -1;
				break;
		}
	}
	setflag(threadstruct.flag, XTHREAD_ROOT | XTHREAD_EXITENABLE);
	ReleasePPB();
	return EXIT_SUCCESS;
}
