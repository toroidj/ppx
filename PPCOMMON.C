/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library		Main
-----------------------------------------------------------------------------*/
#define ONPPXDLL // PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "TMEM.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#pragma hdrstop
#include "PPD_GVAR.C" // グローバルの実体定義

FM_H SM_tmp;	// 作業用共有メモリ
FM_H SM_hist;	// ヒストリ用共有メモリ
BOOL InitProcessDLL(void);
void EndProcessDLL(void);

/*-----------------------------------------------------------------------------
						pplib.dll の初期化／終了処理
-----------------------------------------------------------------------------*/
const TCHAR usernamestr[] = T("%USERNAME%");
const TCHAR MSG_SHAREBLOCKERROR[] = T("Share block create error");
const TCHAR MSG_DLLVERSIONERROR[] = T("Already running other PPLIBxxx.DLL.\n別バージョンのDLLが使用中のため起動不可");
const TCHAR MSG_HITORYAREAERROR[] = T("History file open error");
const TCHAR MSG_HISTORYIDCOLLAPSED[] = T("History ID collapsed, initialize now.\nヒストリ領域破損の為、初期化");
const TCHAR MSG_HISTORYCOLLAPSED[] = T("History structure collapsed, fix now.\nヒストリ領域破損の為、初期化");
const TCHAR MSG_CUSTOMIZETAREAERROR[] = T("Customize file open error");
const TCHAR MSG_CUSTOMIZEIDCOLLAPSED[] = T("Customize ID collapsed, initialize now.\nカスタマイズ領域破損の為、初期化");
const TCHAR MSG_CUSTOMIZESTRUCTUREBROKEN[] = T("Customize structure broken, fix now.\nカスタマイズ領域破損の為、一部設定を廃棄");
#if !defined(_WIN64) && defined(UNICODE)
const TCHAR MSG_NONTERROR[] = T("Can't run on Windows9x.\nUNICODE版です。Windows9xでは動作しません。");
#endif

#pragma argsused
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
	UnUsedParam(reserved);

	switch (reason){
		// DLL 初期化 ---------------------------------------------------------
		case DLL_PROCESS_ATTACH:
			DLLhInst = hInst;
			return InitProcessDLL();

		// DLL の終了処理 -----------------------------------------------------
		case DLL_PROCESS_DETACH:
			EndProcessDLL();
			break;
/*
		case DLL_THREAD_ATTACH:
			GetThreadInfo();
			break;
*/
	}
	return TRUE;
}

// DllMainの実行に支障が出そうな内容
// ● 異常が起きたときは、MessageBox(user32.dll)を使用する
// ● レジストリ関数を呼び出す(MakeUserfilename)
// ● メモリマップ等に使う名前付きオブジェクトは、Win2000 の terminal serviceの
//   DLL が初期化されていないとクラッシュする恐れがある
// ※ User32.dll, Gdi32.dll は他の DLL を読み込む必要があるのが多いので禁止
//    RegisterWindowMessage は問題なさそう
// ※ LoadLibrary 使用禁止(パスが長すぎるとき、～L 系を使わせないように)
// ※ malloc 等のランタイムのメモリ管理の類は使用禁止
// ※ CreateThread, ShGetFolderPath は使用禁止
BOOL InitProcessDLL(void)
{
	TCHAR buf[VFPS], *last;
	TCHAR buf2[VFPS];
	DWORD tempsize;

	GetModuleFileName(DLLhInst, DLLpath, TSIZEOF(DLLpath));
	#pragma warning(suppress:28125) // XP/2003 以前はメモリ不足による例外が発生することがある
	InitializeCriticalSection(&ThreadSection);

	StartTick = GetTickCount();
								// OS 情報の取得 ======================
	OSver.dwOSVersionInfoSize = sizeof(OSver);
	GetVersionEx(&OSver);

	If_WinNT_Block {
		WinType = MAKE_WINTYPE(OSver);
	}
	#if !defined(_WIN64) && defined(UNICODE)
		if ( OSver.dwPlatformId != VER_PLATFORM_WIN32_NT ){
			CriticalMessageBoxOk(MSG_NONTERROR);
			return FALSE;
		}
	#endif
	if ( WinType != WINTYPE_VISTA ){
		tempsize = TSIZEOF(UserName);
		if ( GetUserName(UserName, &tempsize) == FALSE ){
			UserName[0] = '\0';
		}
	}else{ // Vistaではこの段階でGetUserNameを使うと、runasによる実行時+
		// その後の GetUserName 実行時にアクセス違反が発生する。
		// そのため、環境変数経由で取得してごまかし中…
		UserName[0] = '\0';
		ExpandEnvironmentStrings(usernamestr, UserName, TSIZEOF(UserName));
	}
								// DLL 情報の取得 =====================
	last = tstrrchr(DLLpath, '\\');
	if ( last != NULL ) *(last + 1) = '\0';
	ProcHeap = GetProcessHeap();
	WM_PPXCOMMAND = RegisterWindowMessage(T(PPXCOMMAND_WM)); // User32.dll
	ProcTempPath[0] = '\0';
	{
		TCHAR *p;
		DWORD i = 0;

		for ( p = UserName ; *p ; p++ ) i += (UTCHAR)*p;
		thprintf(SyncTag, TSIZEOF(SyncTag), XNAME T("%05X"), i);
	}

	hKernel32 = GetModuleHandle(T("KERNEL32.DLL"));
	hShell32 = GetModuleHandle(T("SHELL32.DLL"));
								// 例外処理の組み込み =================
	*(DWORD *)FR_igpn.b = 0; // 無視フィルタを初期化
	#ifndef _WIN64
		if ( WinType >= WINTYPE_VISTA ){
			GETDLLPROC(hKernel32, AddVectoredExceptionHandler);
			GETDLLPROC(hKernel32, RemoveVectoredExceptionHandler);
		}
		if ( DAddVectoredExceptionHandler == NULL ){
			UEFvec = FUNCCAST(PVOID, SetUnhandledExceptionFilter(PPxUnhandledExceptionFilter));
		}else
	#endif
		{ // 最後に参照されるハンドラとして登録
			UEFvec = DAddVectoredExceptionHandler(0, PPxUnhandledExceptionFilter);
		}
								// 作業用共有メモリの用意 =============
	{
		TCHAR *lastp;
		int mapresult;

		GetTempPath(TSIZEOF(buf), buf);
		lastp = buf + tstrlen(buf);
		thprintf(lastp, 24, T("%s.TMP"), SyncTag);
		mapresult = FileMappingOn(&SM_tmp, buf, lastp, sizeof(ShareM),
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE);
		if ( mapresult < 0 ){
			CriticalMessageBoxOk(MSG_SHAREBLOCKERROR);
			return FALSE;
		}

		Sm = (ShareM *)SM_tmp.ptr;
		if ( mapresult != 0 ){	// 作業領域の初期化 ===================
			memset(Sm, 0, sizeof(ShareM));
			UsePPx(); // 問題発生時に XMessage XM_DbgLOG
			Sm->Version = ShareM_VersionValue;
			// Sm->CustomizeWrite = 0; // 初期化済み
			// Sm->EnumNerServerThreadID = 0; // 初期化済み
			// Sm->CustomizeNameID = 0; // 初期化済み
			// Sm->NowShutdown = FALSE; // 初期化済み
			// Sm->hCommonLogWnd = NULL; // 初期化済み
			FreePPx();
		}else{ // 作業領域のバージョンチェック
			if ( Sm->Version != ShareM_VersionValue ){
				CriticalMessageBoxOk(MSG_DLLVERSIONERROR);
				return FALSE;
			}
		}
	}
								// ヒストリのオープン =================
	{
		int mapresult;

		MakeUserfilename(buf, HISTNAME, PPxRegPath);
		UsePPx();
		mapresult = FileMappingOn(&SM_hist, buf, HISTMEMNAME,
				X_Hsize, FILE_ATTRIBUTE_NORMAL);
		if ( mapresult < 0 ){
			//他プロセスにより作成されているマップがX_Hsizeより小さいときはエラーになるので小さくして再試行
			if ( mapresult == -2 ){
				mapresult = FileMappingOn(&SM_hist, buf, HISTMEMNAME,
						4096, FILE_ATTRIBUTE_NORMAL);
				if ( mapresult >= 1 ) X_Hsize = 4096;
			}
			if ( mapresult < 0 ){
				CriticalMessageBoxOk(MSG_HITORYAREAERROR);
				FreePPx();
				return FALSE;
			}
		}
											// 正当性チェック
		HisP = (BYTE *)SM_hist.ptr + HIST_HEADER_SIZE;
		if ( mapresult & (FMAP_CREATEFILE | FMAP_FILEOPENERROR) ){
			InitHistory(); // 初期化
		}
	}
	if ( strcmp(HistHeaderFromHisP->HeaderID, HistoryID) ){
		if ( strcmp(HistHeaderFromHisP->HeaderID, "RST") ){
			CriticalMessageBoxOk(MSG_HISTORYIDCOLLAPSED);
		}
		InitHistory();
	}
	if ( X_Hsize != HistSizeFromHisP ){
		X_Hsize = HistSizeFromHisP;
		FileMappingOff(&SM_hist);
		FileMappingOn(&SM_hist, buf, HISTMEMNAME,
				X_Hsize, FILE_ATTRIBUTE_NORMAL);
		HisP = (BYTE *)SM_hist.ptr + HIST_HEADER_SIZE;
	}
	{ // ヒストリ領域の破損チェック
		BYTE *p, *histmax;

		p = HisP;
		histmax = HisP + X_Hsize - HIST_HEADER_FOOTER_SIZE;
		for( ; ; ){
			WORD size;

			size = *(WORD *)p;
			if ( size == 0 ) break;
			if ( (p + size) > histmax ){
				CriticalMessageBoxOk(MSG_HISTORYCOLLAPSED);
				*(WORD *)p = 0; // 強制末尾設定
				break;
			}
			p += size;
		}
	}
	FreePPx();
								// カスタマイズ領域のオープン =========
	{
		int mapresult;

		UsePPx();
		MakeUserfilename(buf, CUSTNAME, PPxRegPath);
		MakeCustMemSharename(buf2, Sm->CustomizeNameID);
		mapresult = FileMappingOn(&SM_cust, buf, buf2, X_Csize, FILE_ATTRIBUTE_NORMAL);
		if ( mapresult < 0 ){
			//他プロセスにより作成されているマップがX_Csizeより小さいときはエラーになるので小さくして再試行
			if ( mapresult == -2 ){
				mapresult = FileMappingOn(&SM_cust, buf, buf2,
						4096, FILE_ATTRIBUTE_NORMAL);
				if ( mapresult >= 1 ) X_Csize = 4096;
			}
			if ( mapresult < 0 ){
				CriticalMessageBoxOk(MSG_CUSTOMIZETAREAERROR);
				FreePPx();
				return FALSE;
			}
		}
											// 正当性チェック
		CustP = (BYTE *)SM_cust.ptr + CUST_HEADER_SIZE;
		if ( mapresult & (FMAP_CREATEFILE | FMAP_FILEOPENERROR) ){
			InitCust(); // 新規作成／メモリのみの初期化
		}
	}
	if ( strcmp(CustHeaderFromCustP->HeaderID, CustID) ){
		if ( strcmp(CustHeaderFromCustP->HeaderID, "RST") ){;
			CriticalMessageBoxOk(MSG_CUSTOMIZEIDCOLLAPSED);
		}
		InitCust(); // ヘッダ破損／初期化指示による初期化
	}
	if ( X_Csize != CustSizeFromCustP ){
		X_Csize = CustSizeFromCustP;
		FileMappingOff(&SM_cust);
		FileMappingOn(&SM_cust, buf, CUSTMEMNAME, X_Csize, FILE_ATTRIBUTE_NORMAL);
		CustP = (BYTE *)SM_cust.ptr + CUST_HEADER_SIZE;
	}
	{ // カスタマイズ領域の破損チェック
		BYTE *p, *custmax;

		p = CustP;
		custmax = CustP + X_Csize - CUST_HEADER_FOOTER_SIZE;
		for ( ; ; ){
			DWORD size;

			size = *(DWORD *)p;
			if ( size == 0 ) break;
			if ( (p + size) > custmax ){
				CriticalMessageBoxOk(MSG_CUSTOMIZESTRUCTUREBROKEN);
				*(DWORD *)p = 0; // 強制末尾設定
				break;
			}
			p += size;
		}
	}
	FreePPx();
								// カスタマイズ内容の取り出し =========
	GetCustData(T("X_nshf"), &X_nshf, sizeof(X_nshf));
	GetCustData(T("X_mwid"), &X_mwid, sizeof(X_mwid));
	GetCustData(T("X_es"), &X_es, sizeof(X_es));
	X_es = X_es & 0xff;
								// マルチバイト文字長さテーブルの補正
	FixCharlengthTable(NULL); // ReportLCID を使用

	GetCustData(T("X_log"), &X_log, sizeof(X_log));
	{
		DWORD igpn_size;
		TCHAR *X_igpn = GetCustValue(T("X_igpn"), NULL, &igpn_size, 0);
		if ( X_igpn != NULL ){
			MakeFN_REGEXP(&FR_igpn, X_igpn);
			HeapFree(ProcHeap, 0, X_igpn);
		}
	}
	return TRUE;
}

void EndProcessDLL(void)
{
	int i;

	if ( NowExit == FALSE ){ // K_CLEANUP を使わずに DLL を解放 … PPx 以外
		PPxCommonCommand(NULL, 0, K_CLEANUP);

		for ( i = 0; i < DLLWndClassCount; i++){
			if ( DLLWndClass.list[i] != 0 ){
				UnregisterClass((LPCTSTR)DLLWndClass.list[i], DLLhInst);
			}
		}
	}

	Sm = NULL;
	FileMappingOff(&SM_cust);
	FileMappingOff(&SM_hist);
	FileMappingOff(&SM_tmp);

	#ifndef _WIN64
	if ( DRemoveVectoredExceptionHandler == NULL ){
		SetUnhandledExceptionFilter(FUNCCAST(LPTOP_LEVEL_EXCEPTION_FILTER, UEFvec));
	}else
	#endif
	{
		DRemoveVectoredExceptionHandler(UEFvec);
	}
	DeleteCriticalSection(&ThreadSection);
}
#if NODLL
extern void InitCommonDll(HINSTANCE hInst)
{
	DllMain(hInst, DLL_PROCESS_ATTACH, NULL);
}
#endif
