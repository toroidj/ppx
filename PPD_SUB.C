/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library			Sub
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include <richedit.h>
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#include "CALC.H"
#include "PPD_EDL.H"
#pragma hdrstop

#ifndef REBARCLASSNAME
#define REBARCLASSNAME T("ReBarWindow32")
#endif

#ifdef UNICODE
	#define NilStrW NilStr
#else
	const WCHAR NilStrW[1] = L"";
#endif

BOOL LoadedCharlengthTable = FALSE; // 文字列長テーブルを初期化を行ったかどうか

int ExecKeyStack = 0; // MAXEXECKEYSTACK まで
#define MAXEXECKEYSTACK 50

const TCHAR StrTempExtractPath[] = T("EXTRACTTEMP");
const TCHAR StrDummyTempPath[] = T("C:\\PPXTEMP");
const TCHAR RegAppData[] =T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
const TCHAR RegAppDataName[] =T("AppData");
const TCHAR EnvAppDataName[] =T("%APPDATA%");

#ifndef IMF_DUALFONT
  #define IMF_DUALFONT  0x0080
#endif
#ifndef SPF_SETDEFAULT
  #define SPF_SETDEFAULT  0x0004
  #define EM_SETEDITSTYLE  (WM_USER + 204)
#endif
#ifndef SES_EXTENDBACKCOLOR
  #define SES_EXTENDBACKCOLOR  4
#endif

const TCHAR RichEditClassHead[] = T("RICHEDIT");
#define RichEditClassHeadSize (sizeof(TCHAR) * 8)

IID XCLSID_RegExp = {0x3F4DACA4, 0x160D, 0x11D2, {0xA8, 0xE9, 0x00, 0x10, 0x4B, 0x36, 0x5C, 0x9F}};
//IID XCLSID_Match = {0x3F4DACA5, 0x160D, 0x11D2, {0xA8, 0xE9, 0x00, 0x10, 0x4B, 0x36, 0x5C, 0x9F}};
//IID XCLSID_MatchCollection = {0x3F4DACA6, 0x160D, 0x11D2, {0xA8, 0xE9, 0x00, 0x10, 0x4B, 0x36, 0x5C, 0x9F}};
IID XIID_IRegExp =	{0x3F4DACA0, 0x160D, 0x11D2, {0xA8, 0xE9, 0x00, 0x10, 0x4B, 0x36, 0x5C, 0x9F}};
IID XIID_IMatch =	{0x3F4DACA1, 0x160D, 0x11D2, {0xA8, 0xE9, 0x00, 0x10, 0x4B, 0x36, 0x5C, 0x9F}};
IID XIID_IMatch2 =	{0x3F4DACB1, 0x160D, 0x11D2, {0xA8, 0xE9, 0x00, 0x10, 0x4B, 0x36, 0x5C, 0x9F}};
IID XIID_IMatchCollection = {0x3F4DACA2, 0x160D, 0x11D2, {0xA8, 0xE9, 0x00, 0x10, 0x4B, 0x36, 0x5C, 0x9F}};
IID XIID_ISubMatches = {0x3F4DACB3, 0x160D, 0x11D2, {0xA8, 0xE9, 0x00, 0x10, 0x4B, 0x36, 0x5C, 0x9F}};

// Shell Extension
IID XIID_IContextMenu3 = {0xbcfce0a0, 0xec17, 0x11d0, {0x8d, 0x10, 0x0, 0xa0, 0xc9, 0xf, 0x27, 0x19}};
IID XIID_IColumnProvider = {0xe8025004, 0x1c42, 0x11d2, {0xbe, 0x2c, 0x00, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1}};
IID XIID_IPropertySetStorage = {0x0000013A, 0x0000, 0x0000, {0xc0, 0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46}};

#ifdef WINEGCC
IID XIID_IClassFactory ={1, 0, 0, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
IID XIID_IUnknown = {0, 0, 0, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
IID XIID_IDataObject = {0x0000010e, 0, 0, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
IID XIID_IDropSource = {0x00000121, 0, 0, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

IID XIID_IStorage = {0x0000000b, 0x0000, 0x0000, {0xc0, 0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46}};
IID XIID_IPersistFile =	{0x0000010b, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

// Shell Extension
IID XCLSID_ShellLink =	{0x00021401, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
IID XIID_IContextMenu =	{0x000214E4, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
IID XIID_IShellFolder =	{0x000214E6, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
IID XIID_IShellLink =	{0x000214EE, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
IID XIID_IContextMenu2 ={0x000214F4, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
#endif

#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x1a
#endif

DWORD AjiEnterCount = 0;

const TCHAR ThemesPersonalize[] = T("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
const TCHAR ThemesAppsUseLightTheme[] = T("AppsUseLightTheme");
HANDLE hDwmapi = NULL;

#define GETDLLPROCN(handle, name, id) D ## name = (imp ## name)GetProcAddress(handle, MAKEINTRESOURCEA(id));

DefineWinAPI(HRESULT, DwmSetWindowAttribute, (HWND, DWORD, LPCVOID, DWORD)) = NULL;
DefineWinAPI(HRESULT, SetWindowTheme, (HWND, LPCWSTR name, LPCWSTR idlist)) = NULL;
//DefineWinAPI(BOOL, ShouldAppsUseDarkMode, (void)); // 132
DefineWinAPI(BOOL, AllowDarkModeForWindow, (HWND, BOOL)); // 133
DefineWinAPI(BOOL, AllowDarkModeForApp, (DWORD)); // 135 メニューdark化
//DefineWinAPI(void, FlushMenuTheme, (void)); // 136
DefineWinAPI(void, RefreshImmersiveColorPolicyState , (void)); // 104

// C:\Windows\Resources\Themes\aero\aero.msstyles を参照
const WCHAR TN_GENERAL[] = L"DarkMode_Explorer"; // Button / Edit / ScrollBar
const WCHAR TN_COMBOBOX[] = L"DarkMode_CFD"; // Combobox / Edit?
const WCHAR TN_LISTVIEW[] = L"DarkMode_ItemsView"; // Header / ListView
const WCHAR TN_TOOLBAR[] = L"DarkMode_InfoPaneToolbar"; // Toolbar
const WCHAR TN_REBAR[] = L"DarkModeNavbar"; // Rebar

typedef struct {
	const WCHAR *general;
	const WCHAR *ComboBox;
	const WCHAR *ListView;
	const WCHAR *toolbar;
	const WCHAR *rebar;
} THEME_LIST;

THEME_LIST Theme_Dark = {TN_GENERAL, TN_COMBOBOX, TN_LISTVIEW, TN_TOOLBAR, TN_REBAR};
THEME_LIST Theme_Light = {TN_GENERAL + 9, TN_COMBOBOX + 9, TN_LISTVIEW + 9, TN_TOOLBAR + 9, TN_REBAR + 8};

#define IATHOOK 0
#if IATHOOK
const WCHAR ChangeBarTheme[] = L"Explorer::ScrollBar";
HANDLE (WINAPI *OrgOpenNcThemeData)(HWND hWnd, LPCWSTR ClassList) = NULL;

HANDLE WINAPI HookOpenNcThemeData(HWND hWnd, LPCWSTR ClassList)
{
	if ( strcmpW(ClassList, L"ScrollBar") == 0 ){
		hWnd = NULL;
		ClassList = ChangeBarTheme;
	}
	return OrgOpenNcThemeData(hWnd, ClassList);
}

// COMCTL32.DLL 中の OpenNcThemeData(uxtheme.dll:49) をフックし、
// ListViewのスクロールバーをダークモード対応にする
void HookUxtheme(void)
{
	ULONG_PTR Comctl32;
	DWORD oldProtect;
	PIMAGE_THUNK_DATA OpenNcThemeData = NULL;
	PIMAGE_NT_HEADERS PEbase;
	PIMAGE_DELAYLOAD_DESCRIPTOR imports;

	if ( OrgOpenNcThemeData != NULL ) return; // フック済み

	Comctl32 = (ULONG_PTR)GetModuleHandle(T("COMCTL32.DLL"));
	if ( Comctl32 == 0 ) return;

	PEbase = (PIMAGE_NT_HEADERS)(ULONG_PTR)(Comctl32 + ((PIMAGE_DOS_HEADER)Comctl32)->e_lfanew);
	imports = (PIMAGE_DELAYLOAD_DESCRIPTOR)(ULONG_PTR)(Comctl32 + PEbase->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress);

	for ( ; imports->DllNameRVA != 0 ; ++imports ){
		PIMAGE_THUNK_DATA NameThunk, AddrThunk;

		if ( stricmp((char *)((char *)Comctl32 + imports->DllNameRVA), "uxtheme.dll") != 0 ){
			continue;
		}
		NameThunk = (PIMAGE_THUNK_DATA)(ULONG_PTR)(Comctl32 + imports->ImportNameTableRVA);
		AddrThunk = (PIMAGE_THUNK_DATA)(ULONG_PTR)(Comctl32 + imports->ImportAddressTableRVA);
		for (; NameThunk->u1.Ordinal; ++NameThunk, ++AddrThunk){
			if ( IMAGE_SNAP_BY_ORDINAL(NameThunk->u1.Ordinal) &&
				 (IMAGE_ORDINAL(NameThunk->u1.Ordinal) == 49) ){
				OpenNcThemeData = (PIMAGE_THUNK_DATA)AddrThunk;
				break;
			}
		}
	}
	if ( OpenNcThemeData == NULL ) return;

	if ( VirtualProtect(OpenNcThemeData, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect) == FALSE ){
		return;
	}
	OrgOpenNcThemeData = (void *)OpenNcThemeData->u1.Function;
	OpenNcThemeData->u1.Function = (ULONG_PTR)(void *)HookOpenNcThemeData;
	VirtualProtect(OpenNcThemeData, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
}
#endif

#pragma argsused
DWORD_PTR USECDECL DummyPPxInfoFunc(PPXAPPINFO *info, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	UnUsedParam(info);

	if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
	return PPXA_INVALID_FUNCTION;
}

// レジストリから文字列を得る -------------------------------------------------
// ※ path と dest が同じ変数でも問題無い
_Success_(return)
PPXDLL BOOL PPXAPI GetRegString(HKEY hKey, const TCHAR *path, const TCHAR *name, _Out_ TCHAR *dest, SIZE32_T destlen)
{
	HKEY HK;
	DWORD typ, siz;
	TCHAR buf[VFPS];

	if ( RegOpenKeyEx(hKey, path, 0, KEY_READ, &HK) == ERROR_SUCCESS ){
		siz = destlen * sizeof(TCHAR);
		if ( RegQueryValueEx(HK, name, NULL, &typ, (LPBYTE)dest, &siz) == ERROR_SUCCESS ){
			if ( (typ == REG_EXPAND_SZ) && (siz < sizeof(buf)) ){

				tstrcpy(buf, dest);
				if ( destlen <= ExpandEnvironmentStrings(buf, dest, destlen) ){
					tstrcpy(dest, buf);
				}
			}
			RegCloseKey(HK);
			return TRUE;
		}
		RegCloseKey(HK);
	}
	return FALSE;
}

BOOL GetRegDword(HKEY hKey, const TCHAR *path, const TCHAR *name, DWORD *value)
{
	HKEY HK;
	DWORD typ, siz;

	if ( RegOpenKeyEx(hKey, path, 0, KEY_READ, &HK) == ERROR_SUCCESS ){
		siz = sizeof(DWORD);
		if ( RegQueryValueEx(HK, name, NULL, &typ, (LPBYTE)value, &siz) == ERROR_SUCCESS ){
			RegCloseKey(HK);
			return TRUE;
		}
		RegCloseKey(HK);
	}
	return FALSE;
}

//======================================================================== 同期
/*-----------------------------------------------------------------------------
 共有メモリ使用の排他処理

戻り値:0:正常終了 -1:20秒以上使用できず、ユーザーが無視した
-----------------------------------------------------------------------------*/
#define USE_SPINTIMEOUT B0
#define USE_SPINTIMEOUT_DIE B1
#define USE_SPINTIMEOUT_DIALOG B2
#define USE_SPINTIMEOUT_SAMETHREAD B3
#define USE_NOWSHUTDOWN B4
#define USE_CHECK_DIE B8
#define USE_CHECK_DIALOG B9
// #define USE_CHECK_DIALOG_SAMETHREAD B10
#define USE_CHECK_DIALOG_ERROR B11
#define USE_CHECK_SAMETHREAD B14
/*
0x200	通常待機、ダイアログ表示有り
0x205	通常待機ダイアログ表示有り & SPINTIMEOUT ダイアログ表示有り


*/
HANDLE SpinThreadAliveCheck(void)
{
	HANDLE hThread;

	// 利用中スレッドの生存確認
	if ( DOpenThread == INVALID_HANDLE_VALUE ){
		GETDLLPROC(hKernel32, OpenThread);
	}
	if ( DOpenThread != NULL ){
		hThread = DOpenThread(THREAD_QUERY_INFORMATION,
				FALSE, Sm->UsePPxSync.ThreadID);
		if ( hThread != NULL ) CloseHandle(hThread); // スレッドは有効
#pragma warning(suppress:6001) // 真偽の判断のみに使用
		return hThread;
	}else{
		return INVALID_HANDLE_VALUE;
	}
}

#if PPXSYNCDEBUG
PPXDLL void PPXAPI UsePPxDebug(const char *filename, int fileline)
#else
PPXDLL void PPXAPI UsePPx(void)
#endif
{
	DWORD OldThreadID = Sm->UsePPxSync.ThreadID;
	DWORD ThreadID = GetCurrentThreadId();
	DWORD Tick = GetTickCount();
	DWORD SpinCheckWait;
	DWORD checkflag = 0;

	for ( ; ; ){
		// 構造体へのアクセス権を確保
		SpinCheckWait = 0;
		while ( InterlockedExchange(&Sm->UsePPxSync.SpinLock, 1) != 0 ){
			DWORD TickDelta;

			Sleep(SpinCheckWait);
			if ( (TickDelta = (GetTickCount() - Tick)) != 0 ){
				SpinCheckWait = 10;
				if ( IsTrue(Sm->NowShutdown) && (TickDelta >= 100) ){
					TickDelta = UsePPxSpinMaxTime;
				}
			}
			// オーバーフローしたときはすぐにダイアログがでる→ok
			if ( TickDelta >= UsePPxSpinMaxTime ){
				setflag(checkflag, USE_SPINTIMEOUT);
				if ( Sm->UsePPxSync.ThreadID == ThreadID ){ // 再入
					setflag(checkflag, USE_SPINTIMEOUT_SAMETHREAD);
					break;
				}

				if ( SpinThreadAliveCheck() == NULL ){ // 利用中スレッドが死亡→奪う
					setflag(checkflag, USE_SPINTIMEOUT_DIE);
					Sm->UsePPxSync.ThreadID = 0; // 強制解放
					break;
				}

				if ( IsTrue(Sm->NowShutdown) ){
					setflag(checkflag, USE_SPINTIMEOUT_DIALOG | USE_NOWSHUTDOWN);
					Sm->UsePPxSync.ThreadID = 0; // 強制解放
					break;
				}

				setflag(checkflag, USE_SPINTIMEOUT_DIALOG);
				Sm->UsePPxSync.ThreadID = 0; // 強制解放
				break;
			}
		}
		// 構造体へのアクセス権を確保完了

		if ( Sm->UsePPxSync.ThreadID == ThreadID ){ // 再入
			#if PPXSYNCDEBUG
				wsprintfA(Sm->UsePPxLast, "R:%s:%d(%d)+%d", filename, fileline, ThreadID, Sm->UsePPxSync.SpinCount);
			#endif
			if ( Sm->UsePPxSync.SpinCount < 0x10000 ){
				Sm->UsePPxSync.SpinCount++;
			}
			break; // 許可
		}

		if ( Sm->UsePPxSync.ThreadID != 0 ){ // 既に利用中
			HANDLE hThread;
			if ( (GetTickCount() - Tick) < (IsTrue(Sm->NowShutdown) ?
					(DWORD)(UsePPxMaxTime / 3) : UsePPxMaxTime) ){ // 待機時間内
				InterlockedExchange(&Sm->UsePPxSync.SpinLock, 0); //spin解放
				Sleep(10);
				continue;
			}

			hThread = SpinThreadAliveCheck(); // 利用中スレッドの生存確認
			// 利用中スレッドが死亡→奪う
			if ( hThread == NULL ) setflag(checkflag, USE_CHECK_DIE);

			setflag(checkflag, USE_CHECK_DIALOG_ERROR);
			InterlockedExchange(&Sm->UsePPxSync.SpinLock, 0); //spin解放
			Sleep(10);
		}
		// 新規利用
		Sm->UsePPxSync.ThreadID = ThreadID;
//		Sm->UsePPxSync.TickCount = GetTickCount();
		Sm->UsePPxSync.SpinCount = 1;
#if PPXSYNCDEBUG
		wsprintfA(Sm->UsePPxFirst, "Use>%s:%d", filename, fileline);
		Sm->UsePPxLast[0] = '\0';
#endif
		break;
	}
	InterlockedExchange(&Sm->UsePPxSync.SpinLock, 0);
	if ( checkflag ){
#if PPXSYNCDEBUG
		XMessage(NULL, NULL, XM_DbgLOG, T("UsePPx %hs:%d(%s-%d): %x,%d, %s-%d,%hs,%hs"),
				filename, fileline, GetThreadName(ThreadID), ThreadID,
				checkflag, Sm->UsePPxSync.SpinCount,
				GetThreadName(OldThreadID), OldThreadID, Sm->UsePPxFirst, Sm->UsePPxLast);
#else
		XMessage(NULL, NULL, XM_DbgLOG, T("UsePPx : %x,%d,%s-%d,%s-%d"),
				checkflag, Sm->UsePPxSync.SpinCount, GetThreadName(ThreadID), ThreadID, GetThreadName(OldThreadID), OldThreadID);
#endif
	}
	return;
}
/*-----------------------------------------------------------------------------
 共有メモリ使用の排他処理の解除
-----------------------------------------------------------------------------*/
#if PPXSYNCDEBUG
int FreePPxDialog(const TCHAR *text, const char *filename, int fileline, UINT flags)
{
	TCHAR titlebuf[200];

	thprintf(titlebuf, TSIZEOF(titlebuf), T("PPx synchronize error(FreePPx)%hs(%d)"), filename, fileline);
	return CriticalMessageBox(text, titlebuf, flags);
}

PPXDLL void PPXAPI FreePPxDebug(const char *filename, int fileline)
#else

const TCHAR FreePPxTitle[] = T("PPx synchronize error(FreePPx)");
#define FreePPxDialog(text, filename, fileline, flags) CriticalMessageBox(text, FreePPxTitle, flags)

PPXDLL void PPXAPI FreePPx(void)
#endif
{
	DWORD Tick = GetTickCount();
	DWORD SpinCheckWait = 0;

	// 構造体へのアクセス権を確保
	while ( InterlockedExchange(&Sm->UsePPxSync.SpinLock, 1) != 0 ){
		DWORD TickDelta;

		Sleep(SpinCheckWait);
		if ( (TickDelta = (GetTickCount() - Tick)) != 0 ){
			SpinCheckWait = 10;
			if ( IsTrue(Sm->NowShutdown) && (TickDelta >= 100) ){
				TickDelta = UsePPxSpinMaxTime;
			}
		}
		// オーバーフローしたときはすぐにダイアログがでる→ok
		if ( TickDelta >= UsePPxSpinMaxTime ){

			if ( Sm->UsePPxSync.ThreadID != GetCurrentThreadId() ){
				// 他のスレッドによって奪われていた
				if ( SpinThreadAliveCheck() == NULL ){ // 利用中スレッドが死亡→解放
					Sm->UsePPxSync.ThreadID = 0;
					break;
				}
				return;
			}
			break;
		}
	}

	// 構造体へのアクセス権を確保完了

	if ( Sm->UsePPxSync.ThreadID != GetCurrentThreadId() ){
		// 他のスレッドによって奪われていた
		Sm->UsePPxSync.ThreadID = 0; // 解放
	}else{
		Sm->UsePPxSync.SpinCount--;
		if ( Sm->UsePPxSync.SpinCount <= 0 ){
			Sm->UsePPxSync.ThreadID = 0; // 解放
		}
	}
	InterlockedExchange(&Sm->UsePPxSync.SpinLock, 0);
	return;
}
//=========================================================== Stack Heap 関連
PPXDLL void PPXAPI ThInit(ThSTRUCT *TH)
{
	TH->bottom = NULL;
	TH->top = 0;
	TH->size = 0;
}

PPXDLL BOOL PPXAPI ThFree(ThSTRUCT *TH)
{
	char *bottom;

	bottom = TH->bottom;
	if ( (bottom == NULL) || (bottom == (char *)((char *)TH + sizeof(ThSTRUCT))) ){
		return TRUE;
	}
	TH->bottom = NULL;
	TH->top = 0;
	TH->size = 0;
	return HeapFree(ProcHeap, 0, bottom);
}

BOOL ThSizeCheckMain(ThSTRUCT *TH, DWORD check_size)
{
	char *newptr;
	DWORD nextsize;

	nextsize = (TH->size + ((TH->size < (256 * KB)) ? (TH->size / 2) : TH->size) + check_size + ThSTEP) & ~(ThSTEP - 1);
	if ( nextsize < TH->size ) return FALSE;

	if ( TH->bottom == (char *)((char *)TH + sizeof(ThSTRUCT)) ){
		newptr = HeapAlloc(ProcHeap, 0, nextsize);
		if ( newptr == NULL ) return FALSE;
		memcpy(newptr, (char *)TH->bottom, TH->top);
	}else{
		newptr = HeapReAlloc(ProcHeap, 0, TH->bottom, nextsize);
		if ( newptr == NULL ) return FALSE;
	}
	TH->bottom = (void *)newptr;
	TH->size = nextsize;
	return TRUE;
}

#define ThSizecheck(TH, CHECKSIZE)						\
	while ( (TH->top + (CHECKSIZE)) > TH->size ){		\
		if ( ThSizeCheckMain(TH, CHECKSIZE) ) continue;	\
		return FALSE;			\
	}

PPXDLL BOOL PPXAPI ThSize(ThSTRUCT *TH, DWORD size)
{
	ThAllocCheck(TH);
	ThSizecheck(TH, size);
	return TRUE;
}

PPXDLL BOOL PPXAPI ThAppend(ThSTRUCT *TH, const void *data, DWORD size)
{
	ThAllocCheck(TH);
	ThSizecheck(TH, size);
	memcpy(ThLast(TH), data, size);
	TH->top += size;
	return TRUE;
}

PPXDLL BOOL PPXAPI ThAddString(ThSTRUCT *TH, const TCHAR *data)
{
	DWORD size;

	ThAllocCheck(TH);
	size = TSTRSIZE32(data);
	ThSizecheck(TH, size + 1);
	memcpy(ThLast(TH), data, size);
	TH->top += size;
	*ThStrLastT(TH) = '\0'; //文字列の\0とは別の保護用
	return TRUE;
}

PPXDLL BOOL PPXAPI ThCatString(ThSTRUCT *TH, const TCHAR *data)
{
	DWORD size;

	ThAllocCheck(TH);
	size = TSTRSIZE32(data);
	ThSizecheck(TH, size);
	memcpy(ThLast(TH), data, size);
	TH->top += size - TSTROFF(1);
	return TRUE;
}
#ifdef UNICODE
PPXDLL BOOL PPXAPI ThCatStringA(ThSTRUCT *TH, const char *data)
{
	DWORD size;

	ThAllocCheck(TH);
	size = strlen32(data) + 1;
	ThSizecheck(TH, size);
	memcpy(ThLast(TH), data, size);
	TH->top += size - 1;
	return TRUE;
}
#endif

/*
	((DWORD 全体のサイズ=8+namesize+strsize+'\0')(WORD nameのサイズ)name str '\0')...
*/

#pragma pack(push, 1)
typedef struct {
	DWORD varsize;
	WORD namesize;
	char name[1];
} THVARS;
#pragma pack(pop)

PPXDLL BOOL PPXAPI ThSetString(ThSTRUCT *thStringValue, const TCHAR *name, const TCHAR *str)
{
	DWORD namesize = TSTRLENGTH32(name), strsize = TSTRSIZE32(str);
	DWORD varsize;
	THVARS *nulltvs = NULL; // 途中で見つけた空きブロック
	THVARS *tvs, *maxtvs;

	varsize = (sizeof(DWORD) * 2) + namesize + strsize;
	if ( thStringValue == NULL ) thStringValue = &ProcessStringValue;
	tvs = (THVARS *)thStringValue->bottom;
	maxtvs = (THVARS *)(char *)(thStringValue->bottom + thStringValue->top);
	while ( tvs < maxtvs ){
		DWORD tvsnamesize;

		tvsnamesize = tvs->namesize;
		if ( (tvsnamesize == namesize) &&
			 (memcmp(tvs->name, name, namesize) == 0) ){
			if ( tvs->varsize >= varsize ){ // 再利用が可能
				if ( strsize <= sizeof(TCHAR) ){ // 削除
					tvs->namesize = 0;
				}else{
					memcpy( tvs->name + tvs->namesize, str, strsize );
				}
				return TRUE;
			}
			// 小さいので削除する
			tvs->namesize = 0;
		}else if ( (tvsnamesize == 0) && (tvs->varsize >= varsize) ){ // 再利用が可能
			nulltvs = tvs;
		}
		tvs = (THVARS *)(char *)( (char *)tvs + tvs->varsize );
	}
	if ( strsize <= sizeof(TCHAR) ) return TRUE; // 削除...保存してなかった

	if ( nulltvs != NULL ){ // 再利用可能
		nulltvs->namesize = (WORD)namesize;
		memcpy(nulltvs->name, name, namesize);
		memcpy(nulltvs->name + namesize, str, strsize);
		return TRUE;
	}
	// 新規確保
	if ( ThSize(thStringValue, varsize) == FALSE ) return FALSE;
	tvs = (THVARS *)(char *)(thStringValue->bottom + thStringValue->top);
	tvs->varsize = varsize;
	tvs->namesize = (WORD)namesize;
	memcpy(tvs->name, name, namesize);
	memcpy(tvs->name + namesize, str, strsize);
	thStringValue->top += varsize;
	return TRUE;
}

PPXDLL TCHAR * PPXAPI ThAllocString(ThSTRUCT *thStringValue, const TCHAR *name, DWORD strsize)
{
	DWORD namesize = TSTRLENGTH32(name);
	DWORD varsize;
	THVARS *nulltvs = NULL; // 途中で見つけた空きブロック
	THVARS *tvs, *maxtvs;

	varsize = (sizeof(DWORD) * 2) + namesize + strsize;

	if ( thStringValue == NULL ) thStringValue = &ProcessStringValue;
	tvs = (THVARS *)thStringValue->bottom;
	maxtvs = (THVARS *)(char *)(thStringValue->bottom + thStringValue->top);
	while ( tvs < maxtvs ){
		DWORD tvsnamesize;

		tvsnamesize = tvs->namesize;
		if ( (tvsnamesize == namesize) &&
			 (memcmp(tvs->name, name, namesize) == 0) ){
			if ( tvs->varsize >= varsize ){ // 再利用が可能
				return (TCHAR *)(char *)(tvs->name + tvs->namesize);
			}
			// 小さいので削除する
			tvs->namesize = 0;
		}else if ( (tvsnamesize == 0) && (tvs->varsize >= varsize) ){ // 再利用が可能
			nulltvs = tvs;
		}
		tvs = (THVARS *)(char *)( (char *)tvs + tvs->varsize );
	}
	if ( nulltvs != NULL ){ // 再利用可能
		nulltvs->namesize = (WORD)namesize;
		memcpy(nulltvs->name, name, namesize);
		return (TCHAR *)(char *)(nulltvs->name + namesize);
	}
	// 新規確保
	if ( ThSize(thStringValue, varsize) == FALSE ) return FALSE;
	tvs = (THVARS *)(char *)(thStringValue->bottom + thStringValue->top);
	tvs->varsize = varsize;
	tvs->namesize = (WORD)namesize;
	memcpy(tvs->name, name, namesize);
	thStringValue->top += varsize;
	return (TCHAR *)(char *)(tvs->name + namesize);
}

PPXDLL const TCHAR * PPXAPI ThGetString(ThSTRUCT *thStringValue, const TCHAR *name, TCHAR *str, SIZE32_T strlength)
{
	DWORD namesize = TSTRLENGTH32(name);
	THVARS *tvs, *maxtvs;

	if ( thStringValue == NULL ) thStringValue = &ProcessStringValue;
	tvs = (THVARS *)thStringValue->bottom;
	maxtvs = (THVARS *)(char *)(thStringValue->bottom + thStringValue->top);
	while ( tvs < maxtvs ){
		if ( (tvs->namesize == (WORD)namesize) &&
			 (memcmp(tvs->name, name, namesize) == 0) ){
			if ( str != NULL ){
				tstplimcpy(str, (TCHAR *)(char *)(tvs->name + namesize), strlength);
			}
			return (TCHAR *)(char *)(tvs->name + namesize);
		}
		tvs = (THVARS *)(char *)( (char *)tvs + tvs->varsize );
	}
	// 保存してなかった
	if ( str != NULL ) *str = '\0';
	return NULL;
}

PPXDLL TCHAR * PPXAPI ThGetLongString(ThSTRUCT *thStringValue, const TCHAR *name, TCHAR *str, SIZE32_T strlength)
{
	DWORD namesize = TSTRLENGTH32(name);
	THVARS *tvs, *maxtvs;

	if ( thStringValue == NULL ) thStringValue = &ProcessStringValue;
	tvs = (THVARS *)thStringValue->bottom;
	if ( tvs != NULL ){
		maxtvs = (THVARS *)(char *)(thStringValue->bottom + thStringValue->top);
		while ( tvs < maxtvs ){
			if ( (tvs->namesize == (WORD)namesize) &&
				 (memcmp(tvs->name, name, namesize) == 0) ){
				size_t strsize;
				TCHAR *longstr;

				str[strlength - 2] = '\0';
				tstplimcpy(str, (TCHAR *)(char *)(tvs->name + namesize), strlength);
				if ( str[strlength - 2] == '\0' ) return str;
				strsize = (tstrlen((TCHAR *)(char *)(tvs->name + namesize)) + 1) * sizeof(WCHAR);
				longstr = HeapAlloc(ProcHeap, 0, strsize);
				if ( longstr == NULL ) return str;
				memcpy(longstr , (TCHAR *)(char *)(tvs->name + namesize), strsize);
				return longstr;
			}
			tvs = (THVARS *)(char *)( (char *)tvs + tvs->varsize );
		}
	}
	// 保存してなかった
	*str = '\0';
	return NULL;
}

PPXDLL BOOL PPXAPI ThEnumString(ThSTRUCT *thStringValue, int index, TCHAR *name, TCHAR *str, DWORD strlength)
{
	THVARS *tvs, *maxtvs;

	if ( thStringValue == NULL ) thStringValue = &ProcessStringValue;
	tvs = (THVARS *)thStringValue->bottom;
	maxtvs = (THVARS *)(char *)(thStringValue->bottom + thStringValue->top);
	while ( tvs < maxtvs ){
		if ( tvs->namesize && (index-- == 0) ){
			memcpy(name, tvs->name, tvs->namesize);
			name[tvs->namesize / sizeof(TCHAR)] = '\0';
			tstplimcpy(str, (TCHAR *)(char *)(tvs->name + tvs->namesize), strlength - 1);
			return TRUE;
		}
		tvs = (THVARS *)(char *)( (char *)tvs + tvs->varsize );
	}
	return FALSE;
}

//================================================================== エラー関係
/*-----------------------------------------------------------------------------
	エラーメッセージの取得

	str		格納バッファ(VFPS)
	number	エラーの内容、PPERROR_GETLASTERROR なら GetLastError() の値を用いる
-----------------------------------------------------------------------------*/
PPXDLL ERRORCODE PPXAPI PPErrorMsg(TCHAR *str, ERRORCODE code)
{
	TCHAR *p;

	*str = '\0';
	if ( code == PPERROR_GETLASTERROR ){
		code = GetLastError();
		if ( code == NO_ERROR ) return NO_ERROR;
	}
	FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
			(LPTSTR)str, VFPS, NULL);
	for ( p = str ; *p ; p++ ) if ( (UTCHAR)*p < ' ' ) *p = ' ';
	while ( (p > str) && (*(p - 1) == ' ') ) p--;
	if ( p < (str + 4) ){
		thprintf(str, VFPS, T("Unknown error : %d"), code);
	}else if ( (p - str) < (VFPS - 12) ){
		thprintf(p, 13, T("(%d)"), code);
	}else{
		*p = '\0';
	}
	return code;
}
/*-----------------------------------------------------------------------------
	エラーメッセージの表示

	hWnd	エラーを起こしたウィンドウ。NULL でもよい。
	code	エラーの内容、PPERROR_GETLASTERROR なら GetLastError() の値を用いる
	戻り値	エラー番号、成功なら 0
-----------------------------------------------------------------------------*/
const TCHAR ErrorMsg_NoPath[] = T("%Mm");
const TCHAR ErrorMsg_WithPath[] = T("%Mm\n(%s)");
PPXDLL ERRORCODE PPXAPI ErrorPathBox(HWND hWnd, const TCHAR *title, const TCHAR *path, ERRORCODE code)
{
	if ( code == PPERROR_GETLASTERROR ) code = GetLastError();
	XMessage(hWnd, title, XM_GrERRld,
			(path == NULL) ? ErrorMsg_NoPath : ErrorMsg_WithPath, code, path);
	return code;
}

PPXDLL ERRORCODE PPXAPI PPErrorBox(HWND hWnd, const TCHAR *title, ERRORCODE code)
{
	return ErrorPathBox(hWnd, title, NULL, code);
}

//====================================================================== その他
/*-----------------------------------------------------------------------------
 カスタマイズ領域／ヒストリ領域用の名前を生成する
	src = 4バイトの英数字（名前に使用）
	idname = 使用するレジストリの位置
-----------------------------------------------------------------------------*/
void MakeUserfilename(TCHAR *dst, const TCHAR *src, const TCHAR *idname)
{
	TCHAR path[MAX_PATH];
	TCHAR name[MAX_PATH];
										// 固定ファイルがあるならそれを使用 ===
	thprintf(dst, MAX_PATH, T("%s%sDEF.DAT"), DLLpath, src);
	if ( GetFileAttributes(dst) != BADATTR ) return;

	if ( !GetRegString(HKEY_CURRENT_USER, idname, src, dst, MAX_PATH) ){
		TCHAR *p;
		DWORD t;
		// %APPDATA%…Windows2000 以降で使用
		// レジストリ…将来的になくなると思われる
		// SHGetFolderPath…DllMainで使用禁止
		if ( ((ExpandEnvironmentStrings(EnvAppDataName, path, TSIZEOF(path)) > 0) &&
			  (path[0] != '\0') && (GetFileAttributesL(path) != BADATTR) ) ||
			 (GetRegString(HKEY_CURRENT_USER,
					RegAppData, RegAppDataName, path, TSIZEOF(path)) &&
			  (GetFileAttributesL(path) != BADATTR)) ){
			CatPath(NULL, path, T(PPxSettingsAppdataPath));
		}else{
			tstrcpy(path, DLLpath);
			if ( path[0] == '\\' ) (void)GetWindowsDirectory(path, MAX_PATH);
		}

		t = 0xa55a;
		p = UserName;
		while ( *p ) t = (t << 1) + (DWORD)*p++;
		thprintf(name, TSIZEOF(name), T("%s%04X.DAT"), src, t & 0xffff);
		CatPath(dst, path, name);
#if USETEMPCONFIGFILE // 一発ネタ(きちんと設定を保存しない/仮の設定を作成)の時
		if ( GetFileAttributes(dst) == BADATTR ){
			thprintf(dst, MAX_PATH, T("%s%sDEF.DAT"), DLLpath, src);
		}
#else	// 通常の時
		if ( GetFileAttributes(path) == BADATTR ) MakeDirectories(path, NULL);
#endif
	}
}
/*-----------------------------------------------------------------------------
	シフトキーの状態を入手する
-----------------------------------------------------------------------------*/
PPXDLL DWORD PPXAPI GetShiftKey(void)
{
	BYTE pbKeyState[256];

	(void)GetKeyboardState((LPBYTE)&pbKeyState);
	return	(pbKeyState[VK_SHIFT]	& B7 ? K_s : 0) |
			(pbKeyState[VK_CONTROL]	& B7 ? K_c : 0) |
			(pbKeyState[VK_MENU]	& B7 ? K_a : 0) |
			(pbKeyState[X_es]		& B7 ? K_e : 0);
}

/*-----------------------------------------------------------------------------
	キーカスタマイズの解釈をする
-----------------------------------------------------------------------------*/
PPXDLL ERRORCODE PPXAPI ExecKeyCommand(const EXECKEYCOMMANDSTRUCT *ekcs, PPXAPPINFO *info, WORD key)
{
	TCHAR buf[CMDLINESIZE], *bufp;
	ERRORCODE result;
	const TCHAR *ekname;

	if ( ExecKeyStack >= MAXEXECKEYSTACK ){
		XMessage(NULL, NULL, XM_FaERRd, MES_RKAL);
		return ERROR_OUT_OF_STRUCTURES;
	}
	ExecKeyStack++;

	PutKeyCode(buf, key);
	ekname = ekcs->CustName1;
	if ( NULL == (bufp = GetCustValue(ekname, buf, buf, sizeof(buf))) ){
		ekname = ekcs->CustName2;
		if ( (ekname == NULL) ||
			 (NULL == (bufp = GetCustValue(ekname, buf, buf, sizeof(buf)))) ){
			ERRORCODE cmdresult;

			cmdresult = ekcs->Command(info, key | (WORD)K_raw); // 該当無し
			ExecKeyStack--;
			return cmdresult;
		}
	}

	if ( (UTCHAR)bufp[0] == EXTCMD_CMD ){ // コマンド実行
		result = PP_ExtractMacro(info->hWnd, info, NULL, bufp + 1, NULL, 0);
	}else{ // キー
		WORD *keyp, readkey;

		if ( X_Keyra == 1 ){
			X_Keyra = 0;
			GetCustData(T("X_Keyra"), &X_Keyra, sizeof(X_Keyra));
			X_Keyra = (X_Keyra == 0) ? K_raw : 0;
		}
		keyp = (WORD *)(((UTCHAR)bufp[0] == EXTCMD_KEY) ? (bufp + 1) : bufp);
		for ( ;; ){
			readkey = *keyp;
			if ( readkey == 0 ){
				result = NO_ERROR;
				break;
			}
			result = ekcs->Command(info, readkey | (WORD)X_Keyra);
			if ( result != NO_ERROR ) break;
			keyp++;
		}
	}
	if ( bufp != buf ) HeapFree(ProcHeap, 0, bufp);
	ExecKeyStack--;
	return result;
}

/*-----------------------------------------------------------------------------
	IME の状態を設定する
	status		0:IME off	1:IME on	2:IME onなら固定入力に切り換え(予定)
-----------------------------------------------------------------------------*/
#define xIS_DEFAULT 0
#define xIS_DIGITS 28
#define xIS_NUMBER 29 // カンマ付き数値
#define xIS_ONECHAR 30 // ANSI character, codepage 1252
PPXDLL void PPXAPI SetIMEStatus(HWND hWnd, int status)
{
#if 0
	HMODULE hMSCTF;
	DefineWinAPI(HRESULT, SetInputScope, (HWND, int));

	hMSCTF = GetModuleHandle(T("msctf.dll"));
	if ( hMSCTF != NULL ){
		GETDLLPROC(hMSCTF, SetInputScope);
		if ( DSetInputScope != NULL ){
			DSetInputScope(hWnd, status ? xIS_DEFAULT : xIS_ONECHAR);
			XMessage(NULL, NULL, XM_DbgLOG, T("SetInputScope"));
			return;
		}
	}
#endif
#if !defined(WINEGCC)
{
	HIMC hIMC;
															// IME を強制解除
	hIMC = ImmGetContext(hWnd);
	if ( hIMC != 0 ){
/*
		if ( status == 2 ){
			if ( IsTrue(ImmGetOpenStatus(hIMC)) ){
				ImmSetConversionStatus(hIMC,
						IME_CMODE_ALPHANUMERIC, IME_SMODE_NONE);
			}
		}else{
*/
			// 希望状態でなければ変更
			if ( ImmGetOpenStatus(hIMC) ? !status : status ){
				ImmSetOpenStatus(hIMC, status);
			}
//		}
		ImmReleaseContext(hWnd, hIMC);
	}
}
#endif
}

/*-----------------------------------------------------------------------------
	IME の状態を初期設定に変更
-----------------------------------------------------------------------------*/
PPXDLL void PPXAPI SetIMEDefaultStatus(HWND hWnd)
{
	if ( (hWnd != NULL) && GetCustDword(T("X_IME"), 0) ) SetIMEStatus(hWnd, 0);
	// ↓押されっぱなしになった場合に備える
	if ( GetShiftKey() & K_e ) keybd_event((BYTE)X_es, 0, KEYEVENTF_KEYUP, 0);
}

BOOL CheckLoadSize(HWND hWnd, DWORD *sizeL)
{
	DWORD X_wsiz = IMAGESIZELIMIT;
	int result;

	GetCustData(T("X_wsiz"), &X_wsiz, sizeof X_wsiz);
	if ( *sizeL <= X_wsiz ) return TRUE;

	if ( hWnd == (HWND)LFI_ALWAYSLIMIT ){
		result = IDYES;
	}else if ( hWnd == (HWND)LFI_ALWAYSLIMITLESS ){
		return TRUE;
	}else{
		HWND hOldFocusWnd C4701CHECK;

		if ( hWnd != NULL ){
			hOldFocusWnd = GetForegroundWindow();
			ForceSetForegroundWindow(hWnd);
		}
		result = PMessageBox(hWnd, MES_QOSL, T("Warning"),
				MB_ICONEXCLAMATION | MB_YESNOCANCEL);
		if ( (hWnd != NULL) && (hOldFocusWnd != hWnd) ){
			ForceSetForegroundWindow(hOldFocusWnd); // C4701ok
		}
	}
	if ( result == IDYES ){
		*sizeL = X_wsiz;
	}else if ( result != IDNO ){
		return FALSE;
	}
	return TRUE;
}
/*-----------------------------------------------------------------------------
	filename で指定されたファイルをメモリに読み込む
-----------------------------------------------------------------------------*/
PPXDLL ERRORCODE PPXAPI LoadFileImage(const TCHAR *filename, DWORD margin, char **image, DWORD *imagesize, DWORD *filesize)
{
	BOOL useheap; // ヒープを確保したなら true
	HANDLE hFile;
	DWORD sizeL, sizeH;		// ファイルの大きさ
	DWORD readsize, readleft;
	ERRORCODE result;
	HWND hWnd = NULL;
	BOOL device = FALSE;
										// ファイルを開く ---------------------
	hFile = CreateFileL(filename, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ){
		if ( tstrcmp(filename, T("\\\\.\\stdin")) == 0 ){
			hFile = GetStdHandle(STD_INPUT_HANDLE);
			if ( hFile == INVALID_HANDLE_VALUE ) return ERROR_INVALID_FUNCTION;
			device = TRUE;
		}else{
			result = GetLastError();;
			return result;
		}
	}
										// ファイルサイズの確認 ---------------
	if ( (filesize == LFI_ALWAYSLIMIT) ||
		 (filesize == LFI_ALWAYSLIMITLESS) ){
		hWnd = (HWND)filesize;
		filesize = NULL;
	}
	sizeL = GetFileSize(hFile, &sizeH);
	if ( (sizeL == MAX32) && ((result = GetLastError()) != NO_ERROR) ){
		if ( result == ERROR_INVALID_FUNCTION ){ // デバイスと思われる
			sizeL = 64 * KB;
			sizeH = 0;
			device = TRUE;
		}else{
			if ( !device ) CloseHandle(hFile);
			return result;
		}
	}

	if ( sizeH != 0 ) sizeL = MAX32;
	if ( filesize != NULL ) *filesize = sizeL;
										// 読み込み準備 -----------------------
	if ( *image != NULL ){	// メモリは確保済み
		DWORD imgsize;

		useheap = FALSE;
		imgsize = *imagesize;
		if ( imgsize < sizeL ) sizeL = imgsize;
		if ( imgsize < (sizeL + margin) ) margin = imgsize - sizeL;
	}else{						// 内部確保
		if ( CheckLoadSize(hWnd, &sizeL) == FALSE ){
			if ( !device ) CloseHandle(hFile);
			return ERROR_CANCELLED;
		}

		useheap = TRUE;
		if ( (*image = HeapAlloc(ProcHeap, 0, sizeL + margin)) == NULL ){
			result = GetLastError();
			if ( !device ) CloseHandle(hFile);
			return result;
		}
	}
										// 読み込み & 0 padding ---------------
	result = NO_ERROR;
	readsize = 0;
	readleft = sizeL;
	for (;;){
		if ( ReadFile(hFile, *image + readsize, readleft, &sizeH, NULL) != FALSE ){
			if ( sizeH == 0 ) break; // 残りなし
			readsize += sizeH;
			readleft -= sizeH;
			if ( readleft == 0 ){
				char *newimage;

				if ( !device || !useheap ) break;
				if ( sizeL >= GB ) break; // サイズオーバ
				newimage = HeapReAlloc(ProcHeap, 0, *image, (sizeL * 2) + margin);
				if ( newimage == NULL ) break;
				*image = newimage;
				readleft = sizeL;
				sizeL *= 2;
			}
		}else{
			result = GetLastError();
			break;
		}
	}
	if ( !device ){
		CloseHandle(hFile);
	}else{
		if ( filesize != NULL ) *filesize = readsize;
	}
	if ( (result != NO_ERROR) && (result != ERROR_BROKEN_PIPE) ){
		if ( IsTrue(useheap) ) HeapFree(ProcHeap, 0, *image);
		return result;
	}
	if ( margin != 0 ) memset(*image + readsize, 0, margin);
	if ( imagesize != NULL ) *imagesize = readsize;
	return NO_ERROR;
}

struct CHARSETLISTSTRUCT {
	const char *name;
	int size;
	UINT type;
} charsetlist[] = {
	{"SHIFT_JIS", 9,	CP__SJIS},
	{"X-SJIS", 6,		CP__SJIS},
	{"EUC", 3,			VTYPE_EUCJP},
	{"X-EUC", 5,		VTYPE_EUCJP},
	{"UTF-7", 5,		VTYPE_UTF7},
	{"UTF-8", 5,		CP_UTF8},
	{"ISO-8859-1", 10,	28591},
	{"ISO-2022-JP", 11,	VTYPE_SYSTEMCP}, // ESC処理が必要なので無視されるように記載
	{NULL, 0, 0}
};

// Win95/NT4 eng RTM は VTYPE_IBM / VTYPE_ANSI / VTYPE_SYSTEMCP のみ使用可能
UINT VTypeToCPlist[VTypeToCPlist_max] = {
	CP__US,		// VTYPE_IBM
	CP__LATIN1,	// VTYPE_ANSI
	CP__JIS,	// VTYPE_JIS
	CP_ACP,		// VTYPE_SYSTEMCP
	CP__SJIS,	// VTYPE_EUCJP // ※ 殆ど CP__SJIS で処理するため
	CP__UTF16L,	// VTYPE_UNICODE
	CP__UTF16B,	// VTYPE_UNICODEB
	CP__SJIS,	// VTYPE_SJISNEC
	CP__SJIS,	// VTYPE_SJISB
	CP__SJIS,	// VTYPE_KANA
	CP_UTF8,	// VTYPE_UTF8
	CP_UTF7,	// VTYPE_UTF7
};

const TCHAR CodePageListPath[] = T("Software\\Classes\\MIME\\Database\\Charset");
const TCHAR CodePageName[] = T("InternetEncoding");
const TCHAR CodePageAliasName[] = T("AliasForCharset");
#define CPBUFSIZE 20

void SkipSepA(char **ptr)
{
	for ( ; ; ){
		char c;

		c = **ptr;
		if ( (c != ' ') && (c != '\t') && (c != '\r') && (c != '\n') ) break;
		(*ptr)++;
	}
}

UINT GetCPfromCharset(const BYTE *ptr)
{
	struct CHARSETLISTSTRUCT *cl;
	const BYTE *textsrc;
	TCHAR textbuf[CPBUFSIZE], *textdest;
	UINT cp = 0;
	HKEY HKroot, HKitem;
	BOOL loop = FALSE;

	for ( cl = charsetlist ; cl->name ; cl++ ){
		if ( memicmp(ptr, cl->name, cl->size) == 0 ){
			if ( cl->type == GetACP() ){
				return VTYPE_SYSTEMCP;
			}
			return cl->type;
		}
	}

	if ( (TinyCharUpper(ptr[0]) == 'C') &&
		 (TinyCharUpper(ptr[1]) == 'P') &&
		Isdigit(ptr[2]) ){
		UINT codepage;

		ptr += 2;
		codepage = (UINT)GetNumberA((const char **)&ptr);
		if ( codepage == GetACP() ) codepage = VTYPE_SYSTEMCP;
		return codepage;
	}

	// レジストリの MIME から判別する(Win2k/Win98以降)
	textsrc = ptr;
	textdest = textbuf;
	for (;;){
		BYTE chr;

		chr = *textsrc;
		if ( (chr <= '\"') || (chr == '?') || (chr == ';') ) break;
		if ( textdest >= (textbuf + CPBUFSIZE - 2) ) return 0;
		*textdest++ = (TCHAR)chr;
		textsrc++;
	}
	*textdest = '\0';
	if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, CodePageListPath, 0, KEY_READ,
			&HKroot) != ERROR_SUCCESS ){
		return 0;
	}
	for (;;){
		DWORD size;

		if ( RegOpenKeyEx(HKroot, textbuf, 0, KEY_READ, &HKitem) != ERROR_SUCCESS ){
			break;
		}

		size = sizeof cp;
		if ( ERROR_SUCCESS != RegQueryValueEx(HKitem, CodePageName, NULL, NULL, (LPBYTE)&cp, &size) ){
			size = sizeof textbuf;
			if ( (loop == FALSE) && (ERROR_SUCCESS == RegQueryValueEx(HKitem, CodePageAliasName, NULL, NULL, (LPBYTE)&textbuf, &size)) ){
				loop = TRUE;
				RegCloseKey(HKitem);
				continue;
			}
		}
		RegCloseKey(HKitem);
		break;
	}
	RegCloseKey(HKroot);
	return cp;
}

#define DEFAULTCODEPAGE	VTYPE_SYSTEMCP // wine のときは CP_UTF8 の方が良さそう
#define CPMAXCHECKSIZE 0x4000
#define CPCHECKSIZE 1024
#define UTF8CHECKSIZE 1024
#define CODECHECKSIZE (6 * KB)

// 戻り値 : VTypeToCPlist 又は codepage
// SJIS の場合、GetACP == CP__SJIS なら VTYPE_SYSTEMCP。そうでなければ CP__SJIS
// UNICODE の場合、BOM有りなら VTYPE_UNICODE/VTYPE_UNICODEB/VTYPE_UTF8
//                 BOM無しなら CP__UTF16L/CP__UTF16B/CP_UTF8

PPXDLL int PPXAPI GetTextCodeType(const BYTE *image, DWORD memsize)
{
	return GetFileCodeType(NULL, image, memsize);
}

PPXDLL int PPXAPI GetFileCodeType(const TCHAR *filename, const BYTE *image, DWORD memsize)
{
	const BYTE *bottom, *ptr, *maximage;
	int cnt; // utf-8の２バイト目以降のバイト数カウント & 7bit限定チェッカ

	if ( memsize != 0 ){
		if ( *image >= 0xef ){ // UNICODE ヘッダの可能性
			if ( memcmp(image, UTF8HEADER, UTF8HEADERSIZE) == 0 ){
				return VTYPE_UTF8;
			}
			if ( memcmp(image, UCF2HEADER, UCF2HEADERSIZE) == 0 ){
				return VTYPE_UNICODE;
			}
			if ( memcmp(image, UCF2BEHEADER, UCF2HEADERSIZE) == 0 ){
				return VTYPE_UNICODEB;
			}
		}

		if ( memsize > CPMAXCHECKSIZE ) memsize = CPMAXCHECKSIZE;
										// charset / encoding 指定があるか
		maximage = image + min(CPCHECKSIZE, memsize); // このチェックは控えめ
		for ( ptr = image ; ptr < maximage ; ptr++ ){
			BYTE c;
			UINT codepage;

			c = *ptr;
			// バイナリ相当か、8bit文字が検出されたら検索を終了する
			if ( c < '\t' ) goto checkucs;
			if ( c >= 0x80 ) goto checkutf8;

			if ( !((c == 'c') && !memcmp(ptr + 1, "harset", 6)) &&
				 !((c == 'e') && !memcmp(ptr + 1, "ncoding", 7)) ){
				continue;
			}
			ptr += (c == 'c') ? 7 : 8;
			SkipSepA((char **)&ptr);
			if ( *ptr != '=' ) continue;

			ptr++;
			SkipSepA((char **)&ptr);
			if ( (*ptr == '\"') || (*ptr == '\'') ){
				ptr++;
				SkipSepA((char **)&ptr);
			}
			codepage = GetCPfromCharset(ptr);
			if ( codepage != 0 ) return (int)codepage;
			continue;
		}
										// 8bit code があるか
		maximage = image + min(CODECHECKSIZE, memsize); // このチェックは控えめ
		for ( ; ptr < maximage ; ptr++ ){
			BYTE c;

			c = *ptr;
			// バイナリ相当か、8bit文字が検出されたら検索を終了する
			if ( c < '\t' ) break;
			if ( c >= 0x80 ) goto checkutf8;
		}

checkucs:;	// UNICODE/B 検出
		if ( (ptr < maximage) && (*ptr == '\0') ){ // 上位バイトっぽいのを発見
			size_t len;
			const BYTE *up;
			BYTE c;

			len = ptr - image;
			if ( len < (memsize - 4) && !(IsalnumA(*image) && Isalnum(*(image + 1))) ){
				up = image + (len & 1);
				c = 8; // tab
				while ( up < maximage ){	// 上位バイトのみ調査
					c = *up;
					up += 2;
					// 通常 UNICODEに存在しない文字を見つけたら中止
					// 2b-2dは一部(4.0↑)あり d8-dfはサロゲートだが、ここでは×
					if ( (c == 8) || (c == 0x1c) ||
						 ((c >= 0x2b) && (c <= 0x2d)) ||
						 ((c >= 0xd8) && (c <= 0xf8)) ){
						c = 8;
						break;
					}
				}
				if ( c != 8 ) return (len & 1) ? CP__UTF16L : CP__UTF16B;
			}
		}
checkutf8:
		maximage = min(ptr + UTF8CHECKSIZE, maximage);
										// UTF-8 チェック
		bottom = ptr; // 少なくとも tabcode～0x80 の範囲内であったところは省略
		// maximage = image + memsize;
		cnt = -1;
		for ( ptr = bottom ; ptr < maximage ; ){
			BYTE c;

			c = *ptr++;
			if ( c < 0x80 ) continue;		// 00-7f
			if ( c < 0xc0 ) goto noutf8;	// 80-bf 範囲外
			if ( c < 0xe0 ){ cnt = 1;		// C0     80-     7ff
			}else if ( c < 0xf0 ){ cnt = 2;	// E0    800-    ffff
			}else if ( c < 0xf8 ){ cnt = 3;	// F0  10000-  1fffff
//			}else if ( c < 0xfc ){ cnt = 4;	// F8 200000- 3ffffff  *現在規格で使
//			}else if ( c < 0xfe ){ cnt = 5;	// FC 4000000-7fffffff *用されてない
			}else goto noutf8;				// fe-ff 範囲外
											// ２バイト以降のチェック
			while ( cnt ){
				if ( ptr >= maximage ) goto breaksub8;
				if ( (*ptr < 0x80) || (*ptr >= 0xc0 ) ) goto noutf8; // 範囲外
				ptr++;
				cnt--;
			}
		}
breaksub8:

		// テキスト最後まで、cnt が変更されていない→7bit コードのみだった
		if ( cnt >= 0 ) return CP_UTF8; // 調べた範囲では utf-8 だった
		// 7bit コードのみだった
		if ( memsize > 0x10 ){
			if ( (*image == '{') || // json ?
				 (memcmp(image, "<?xml", 5) == 0) ){ // XML
				return CP_UTF8;
			}
		}
		// 拡張子で判断
		goto extcheck;

noutf8: // utf-8 以外の 8bit 文字コードのチェック
		for ( ptr = bottom ;; ){				// EUC-JP チェック
			BYTE c;

			if ( ptr >= maximage ) return VTYPE_EUCJP;
			c = *ptr++;
			if ( c < 0x80 ) continue;		// 00-7f
			if ( (c != 0x8e) && (c < 0xa1) ) break;	// SS2, 1bytes KANA 以外
										// ２バイト目チェック
			c = *ptr;
			if ( ((c < 0xa1) && c) || (c >= 0xff) ) break; // 範囲外
			ptr++;
		}

		if ( GetACP() != CP__SJIS ){		// Shift_JIS チェック(日本以外の時)
			for ( ptr = bottom ;; ){
				BYTE c;

				if ( ptr >= maximage ) return CP__SJIS;
				c = *ptr++;
				if ( c <= 0x80 ) continue;		// 00-80

				if ( (c >= 0xa0) && (c < 0xe0) ) continue; // a0-df 1byte(KANA)
										// ２バイト目チェック
				c = *ptr;
				if ( (c < 0x40) || (c == 0x7f) || (c >= 0xfe) ) break; // 範囲外
				ptr++;
			}
		}
		return DEFAULTCODEPAGE; // 不明→system codepage
	}
extcheck:; // 拡張子から推測
	if ( filename == NULL ) return DEFAULTCODEPAGE;
	filename += FindExtSeparator(filename);
	if ( *filename == '.' ){
		filename++;
		if ( (tstricmp(filename, T("xml")) == 0) ||
			 (tstricmp(filename, T("json")) == 0) ){
			return CP_UTF8;
		}
	}
	return DEFAULTCODEPAGE; // 不明→system codepage
}

PPXDLL int PPXAPI FixTextImage(const char *src, DWORD memsize, TCHAR **dest, UINT usecp)
{
	return FixTextFileImage(NULL, src, memsize, dest, usecp);
}

// 新しいイメージを作成したときは、負の値
PPXDLL int PPXAPI FixTextFileImage(const TCHAR *filename, const char *src, DWORD memsize, TCHAR **dest, UINT usecp)
{
	int charcode;
	DWORD size;
	BYTE *tmpimage = NULL;
	UINT cp;
	DWORD flags;

	if ( memsize == 0 ){
		*dest = (TCHAR *)src;
		return (usecp > 0) ? usecp : DEFAULTCODEPAGE;
	}
	if ( usecp > 0 ){
		charcode = usecp;
	}else{
		charcode = GetFileCodeType(filename, (const BYTE *)src, memsize);
	}
	switch (charcode){
		case VTYPE_UNICODE: // BOM あり
		case VTYPE_UNICODEB:
		case CP__UTF16L: // BOM なし
		case CP__UTF16B:
			if ( (charcode == VTYPE_UNICODE) || (charcode == CP__UTF16L) ){
				if ( memcmp(src, UCF2HEADER, UCF2HEADERSIZE) == 0 ){
					src += UCF2HEADERSIZE;
					#ifndef UNICODE
						memsize -= UCF2HEADERSIZE;
					#endif
				}
			}else{ // VTYPE_UNICODEB / CP__UTF16B
				WCHAR *udest;
				if ( memcmp(src, UCF2BEHEADER, UCF2HEADERSIZE) == 0 ){
					src += UCF2HEADERSIZE;
					#ifndef UNICODE
						memsize -= UCF2HEADERSIZE;
					#endif
				}
				size = strlenW((WCHAR *)src) + 1;
				tmpimage = HeapAlloc(ProcHeap, 0, size * sizeof(WCHAR));
				if ( tmpimage != NULL ){ // バイトオーダ変換
					udest = (WCHAR *)tmpimage;
					while ( size-- ){
						WCHAR lc;

						lc = *((WCHAR *)src);
						src += sizeof(WCHAR) / sizeof(char);
						*udest++ = (WCHAR)((lc >> 8) | (lc << 8));
					}
					#ifdef UNICODE
						*dest = (WCHAR *)tmpimage;
						return -charcode;
					#else
						src = (char *)tmpimage;
						memsize = (char *)udest - (char *)tmpimage - sizeof(WCHAR);
					#endif
				}
			}
		#ifdef UNICODE
			*dest = (TCHAR *)src;
			return charcode;
		#else
			break;
		#endif

		case VTYPE_UTF8: // BOM あり
		case CP_UTF8: // BOM なし
			if ( memcmp(src, UTF8HEADER, UTF8HEADERSIZE) == 0 ){
				src += UTF8HEADERSIZE;
			}
			break;

		case VTYPE_EUCJP: {
			BYTE *srcp, *dstp;
			int c, d;

			size = strlen32(src) + 1;
			tmpimage = HeapAlloc(ProcHeap, 0, size);
			if ( tmpimage == NULL ) break;
			memcpy(tmpimage, src, size);
			dstp = srcp = (BYTE *)tmpimage;
			while ( *srcp != '\0' ){
				c = *srcp++;
				if ( c < 0x80 ){
					*dstp++ = (BYTE)c;
					continue;
				}
				if ( c == 0x8e ){		//SS2, 1bytes KANA
					*dstp++ = *srcp++;
					continue;
				}
				d = c - 0x80;
				c = *srcp++;
				if ( c == '\0' ) break;
				c -= 0x80;

				if ( d & 1 ){
					if ( c < 0x60 ){
						c += 0x1F;
					}else{
						c += 0x20;
					}
				}else{
					c += 0x7E;
				}
				if ( d < 0x5F ){
					d = (d + 0xE1) >> 1;
				}else{
					d = (d + 0x161) >> 1;
				}

				*dstp++ = (unsigned char)d;
				*dstp++ = (unsigned char)c;
			}
			*dstp = '\0';
			#ifdef UNICODE
				src = (char *)tmpimage;
				memsize = dstp - tmpimage;
				break;
			#else
				*dest = (char *)tmpimage;
				return -charcode;
			#endif
		}
#if 0
		case VTYPE_JIS: {
			BYTE *srcp, *dstp;
			int c, d, jismode = 0;

			size = strlen32(src) + 1;
			tmpimage = HeapAlloc(ProcHeap, 0, size);
			if ( tmpimage == NULL ) break;
			memcpy(tmpimage, src, size);
			dstp = srcp = (BYTE *)tmpimage;
			while ( *srcp != '\0' ){
				if ( jismode == 0 ){ // 非JIS
					while ( *srcp != '\0' ){
						c = *srcp++;
						if ( (c == '\x1b') && (*srcp == '$') && (*(srcp + 1) == 'B') ){
							jismode = 1;
							srcp += 2;
							break;
						}
						*dstp++ = (BYTE)c;
						continue;
					}
					continue;
				}
				while ( *srcp != '\0' ){ // JIS
					d = *srcp;
					c = *(srcp + 1);
					if ( (d < 0x21) || (d >= 0x7f) || (c < 0x21) || (c >= 0x7f) ){
						if ( (d == '\x1b') && (c == '(') && (*(srcp + 2) == 'B') ){
							jismode = 0;
							srcp += 3;
							break;
						}
						*dstp++ = (unsigned char)d;
						srcp += 1;
						break;
					}
					srcp += 2;

					if ( d & 1 ){
						if ( c < 0x60 ){
							c += 0x1F;
						}else{
							c += 0x20;
						}
					}else{
						c += 0x7E;
					}
					if ( d < 0x5F ){
						d = (d + 0xE1) >> 1;
					}else{
						d = (d + 0x161) >> 1;
					}
					*dstp++ = (unsigned char)d;
					*dstp++ = (unsigned char)c;
				}
			}
			*dstp = '\0';
			charcode = CP__SJIS;
			#ifdef UNICODE
				src = (char *)tmpimage;
				memsize = dstp - tmpimage;
				break;
			#else
				*dest = (char *)tmpimage;
				return -charcode;
			#endif
		}
#endif
	}
						// S-JIS 等の MultiByte
	cp = (charcode < VTypeToCPlist_max) ? VTypeToCPlist[charcode] : (UINT)charcode;
	flags = (cp >= CP__NOPREC) ? 0 : MB_PRECOMPOSED;
#ifdef UNICODE
	if ( (cp != CP_UTF8) && !IsValidCodePage(cp) ){
		cp = VTYPE_SYSTEMCP;
	}
	size = MultiByteToWideCharU8(cp, flags, src, memsize, NULL, 0) + 1;
	*dest = HeapAlloc(ProcHeap, 0, TSTROFF(size));
	if ( *dest == NULL ){
		if ( tmpimage != NULL ){
			*dest = (TCHAR *)tmpimage;
			return -charcode;
		}else{
			*dest = (TCHAR *)src;
			return charcode;
		}
	}
	MultiByteToWideCharU8(cp, flags, src, memsize, *dest, size);
	*(*dest + size - 1) = '\0';
	if ( tmpimage != NULL ) HeapFree(ProcHeap, 0, tmpimage);
	return -charcode;

#else // Multibyte
	if ( (charcode == VTYPE_SYSTEMCP) || // 変換不要
		 ((cp != CP_UTF8) && (cp != CP__UTF16L) && (cp != CP__UTF16B) && !IsValidCodePage(cp)) ){ // 変換不可
		*dest = (char *)src;
		return charcode;
	}else{
		WCHAR *srcW;
		BOOL reqfree;

		if ( (cp != CP__UTF16L) && (cp != CP__UTF16B) ){ // 一旦UNICODEに変換
			size = MultiByteToWideCharU8(cp, flags, src, memsize, NULL, 0) + 1;
			srcW = HeapAlloc(ProcHeap, 0, size * sizeof(WCHAR));
			if ( srcW == NULL ){
				*dest = (TCHAR *)src;
				return charcode;
			}
			MultiByteToWideCharU8(cp, flags, src, memsize, srcW, size);
			*(srcW + size - 1) = '\0';
			reqfree = TRUE;
		}else{
			srcW = (WCHAR *)src;
			reqfree = FALSE;
		}
		// system cp に変換
		size = WideCharToMultiByte(CP_ACP, 0, srcW, -1, NULL, 0, NULL, NULL) + 1;
		*dest = HeapAlloc(ProcHeap, 0, size);
		if ( *dest == NULL ){
			HeapFree(ProcHeap, 0, srcW);
			*dest = (TCHAR *)src;
			if ( tmpimage != NULL ) HeapFree(ProcHeap, 0, tmpimage);
			return charcode;
		}
		WideCharToMultiByte(CP_ACP, 0, srcW, -1, *dest, size, NULL, NULL);
		*(*dest + size - 1) = '\0';
		if ( IsTrue(reqfree) ) HeapFree(ProcHeap, 0, srcW);
		if ( tmpimage != NULL ) HeapFree(ProcHeap, 0, tmpimage);
		return -charcode;
	}
#endif
}

ERRORCODE LoadTextDataEx(const TCHAR *filename, TCHAR **image, TCHAR **readpoint, TCHAR **maxptr, const TCHAR *separator)
{
	TCHAR newname[CMDLINESIZE], *ptr;
	TCHAR buf[CMDLINESIZE];
	EDITMODESTRUCT ems = {EDITMODESTRUCT_DEFAULT};
	TCHAR *more;
	const TCHAR *param = separator + 2;
	UTCHAR code;

	tstrcpy(newname, filename);
	newname[separator - filename] = '\0';

	// ※現在「,」区切りに対応していない
	while( '\0' != (code = GetOptionParameter(&param, buf, &more)) ){
		if ( code == '-' ){
			ptr = buf + 1;
		}else{
			ptr = buf;
			if ( (more = tstrchr(ptr, ':')) != NULL ){
				*more++ = '\0';
			}
			tstrupr(buf);
		}
		EditModeParam(&ems, ptr, more);
		continue;
	}
	return LoadTextData(newname, image, readpoint, maxptr, ems.codepage);
}

PPXDLL ERRORCODE PPXAPI LoadTextData(const TCHAR *filename, TCHAR **image, TCHAR **readpoint, TCHAR **maxptr, UINT codepage)
{
	ERRORCODE result;
	TCHAR *newimage;
	DWORD size;

	if ( filename != NULL ){
		*image = NULL;
		result = LoadFileImage(filename, 0x40, (char **)image, &size, (maxptr != NULL) ? LFI_ALWAYSLIMITLESS : NULL);
		if ( result != NO_ERROR ){
			const TCHAR *separator = tstrstr(filename, T("::"));

			if ( separator == NULL ) return result;
			return LoadTextDataEx(filename, image, readpoint, maxptr, separator);
		}
	}else{
		size = TSTROFF32(*maxptr - *image);
	}

	if ( FixTextFileImage(filename, (char *)*image, size, &newimage, codepage) < 0 ){
		HeapFree(ProcHeap, 0, *image);
		*image = newimage;
	}
	*readpoint = newimage;
	if ( maxptr != NULL ) *maxptr = newimage + tstrlen(newimage);
	return NO_ERROR;
}

PPXDLL ERRORCODE PPXAPI LoadTextImage(const TCHAR *filename, TCHAR **image, TCHAR **readpoint, TCHAR **maxptr)
{
	return LoadTextData(filename, image, readpoint, maxptr, 0);
}

BOOL MakeTempEntrySub(TCHAR *tempath, DWORD attribute)
{
	TCHAR buf[VFPS], *p;
	int count = 16;

	p = tempath;
	if ( *p == '\\' ) p++;
	tstrcpy(buf, p);
					// 長さ制限を行う
	p = buf;
	while( *p != '\0' ){
		if ( count-- <= 0 ){
			*p = '\0';
			break;
		}
		p += Ismulti(*p) ? 2 : 1;
	}

	CatPath(tempath, ProcTempPath, buf);
	GetUniqueEntryName(tempath);
	if ( attribute & FILE_ATTRIBUTE_DIRECTORY ){
		return CreateDirectory(tempath, NULL);
	}
	if ( attribute & FILE_ATTRIBUTE_NORMAL ){
		HANDLE hFile;

		hFile = CreateFileL(tempath, GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_NEW,
				FILE_ATTRIBUTE_NORMAL, NULL);
		if ( hFile != INVALID_HANDLE_VALUE ){
			CloseHandle(hFile);
			return TRUE;
		}else{
			return FALSE;
		}
	}
	return TRUE;
}

PPXDLL BOOL PPXAPI MakeTempEntry(DWORD bufsize, TCHAR *tempath, DWORD attribute)
{
	DWORD seed;
	int pathlen, i;

	if ( ProcTempPath[0] == '\0' ){
		GetTempPath(MAX_PATH, ProcTempPath);
		if ( ProcTempPath[0] == '\0' ) tstrcpy(ProcTempPath, DLLpath);

		if ( MakeTempEntry(MAX_PATH, ProcTempPath, FILE_ATTRIBUTE_DIRECTORY) == FALSE ){
			tstrcpy(ProcTempPath, StrDummyTempPath); // ProcTempPath が生成できなかったので、決め打ちでディレクトリを決める
		}
		if ( tempath == NULL ) return TRUE; // ProcTempPath の作成のみ
	}
	// 2: "\\" と "\n" 8:filename 4:ext
	if ( (tstrlen(ProcTempPath) + ( 2 + 8 + 4 )) >= (size_t)bufsize ){
		if ( bufsize >= TSIZEOF(StrDummyTempPath) ){
			tstrcpy(tempath, StrDummyTempPath);
		}
		return FALSE;
	}

	if ( attribute & FILE_ATTRIBUTE_LABEL ){
		return MakeTempEntrySub(tempath, attribute);
	}

	CatPath(tempath, ProcTempPath, NilStr);

	if ( attribute & FILE_ATTRIBUTE_COMPRESSED ){
		resetflag(attribute, FILE_ATTRIBUTE_COMPRESSED);
		if ( (NO_ERROR == GetCustTable(StrCustOthers, T("ExtractTemp"), tempath, TSTROFF(MAX_PATH))) ||
			(GetEnvironmentVariable(StrTempExtractPath, tempath, MAX_PATH) != 0) ){
			CatPath(NULL, tempath, NilStr);
		}
	}
	if ( attribute == 0 ) return TRUE;

	pathlen = tstrlen32(tempath);
	seed = GetTickCount();
	for ( i = 0 ; i < 0x10000 ; i++ ){
		ERRORCODE result;

		thprintf(tempath + pathlen, 14, T("PPX%X.TMP"), LOWORD(seed));
		if ( attribute & FILE_ATTRIBUTE_DIRECTORY ){
			if ( IsTrue(CreateDirectory(tempath, NULL)) ) return TRUE;
		}else{
			HANDLE hFile;

			hFile = CreateFileL(tempath, GENERIC_WRITE,
					FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_NEW,
					FILE_ATTRIBUTE_NORMAL, NULL);
			if ( hFile != INVALID_HANDLE_VALUE ){
				CloseHandle(hFile);
				return TRUE;
			}
		}
		result = GetLastError();

		if ( result == ERROR_PATH_NOT_FOUND ){
			if ( CreateDirectory(ProcTempPath, NULL) == FALSE ) break;
			continue;
		}
		if ( result != ERROR_FILE_EXISTS ){
			if ( GetFileAttributesL(tempath) == BADATTR ) break;
		}
		seed++;
	}
	return FALSE;
}

BOOL DeleteDirectories(const TCHAR *path, BOOL notify)
{
	TCHAR buf[VFPS];
	WIN32_FIND_DATA	ff;
	HANDLE hFF;

	if ( !(GetFileAttributesL(path) & FILE_ATTRIBUTE_REPARSE_POINT) ){
		CatPath(buf, (TCHAR *)path, WildCard_All);
		hFF = FindFirstFileL(buf, &ff);
		if ( INVALID_HANDLE_VALUE != hFF ){
			do{
				if ( IsRelativeDirectory(ff.cFileName) ) continue;
				CatPath(buf, (TCHAR *)path, ff.cFileName);

				if ( ff.dwFileAttributes & FILE_ATTRIBUTE_READONLY ){
					SetFileAttributesL(buf, FILE_ATTRIBUTE_NORMAL);
				}
				if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
					DeleteDirectories(buf, FALSE);
				}else{
					DeleteFileL(buf);
				}
			}while(FindNextFile(hFF, &ff));
			FindClose(hFF);
		}
	}
	if ( IsTrue(RemoveDirectoryL(path)) ){
		if ( IsTrue(notify) ) FolderNotifyToShell(SHCNE_RMDIR, path, NULL);
		return TRUE;
	}
	return FALSE;
}

PPXDLL const TCHAR * PPXAPI PPxGetSyncTag(void)
{
	return SyncTag;
}

#if 0
struct GETCHILD {
	RECT box;
	HWND hWnd;
	POINT pos;
};

BOOL CALLBACK GetWindowFromPointProc(HWND hWnd, LPARAM gc)
{
	RECT box;

	if ( !IsWindowVisible(hWnd) ) return TRUE; // HIDE状態
	GetWindowRect(hWnd, &box);
	if ( PtInRect(&box, ((struct GETCHILD *)gc)->pos) &&
			(((struct GETCHILD *)gc)->box.left   <= box.left  ) &&
			(((struct GETCHILD *)gc)->box.top    <= box.top   ) &&
			(((struct GETCHILD *)gc)->box.right  >= box.right ) &&
			(((struct GETCHILD *)gc)->box.bottom >= box.bottom) ){
		((struct GETCHILD *)gc)->hWnd = hWnd;
	}
	return TRUE;
}

/* 指定座標のウィンドウを厳密に抽出する sPos:screen*/
HWND GetChildWindowFromPoint(HWND hWnd, POINT *pos)
{
	struct GETCHILD gc;
	HWND hPWnd;
										// まず、親を検索
	gc.hWnd = hWnd;
	if ( gc.hWnd == NULL ) return gc.hWnd;
							// 子を検索、隠れているのを探すため、
							// ChildWindowFromPoint をあえて使わない
	GetWindowRect(gc.hWnd, &gc.box);
	gc.pos = *pos;
	if ( !(GetWindowLong(gc.hWnd, GWL_STYLE) & ((WS_CAPTION | WS_SYSMENU) & ~WS_BORDER)) ){
		hPWnd = GetParent(gc.hWnd);
		if ( hPWnd != NULL ) gc.hWnd = hPWnd;
	}
	EnumChildWindows(gc.hWnd, GetWindowFromPointProc, (LPARAM)&gc);
	return gc.hWnd;
}
#endif


// 自か親のタイトル付きウィンドウを取得する -----------------------------------
HWND GetCaptionWindow(HWND hWnd)
{
	// タイトルバーを持っているかを確認
	while ( !(GetWindowLong(hWnd, GWL_STYLE) & (WS_CAPTION & ~WS_BORDER)) ){
		HWND htWnd;

		htWnd = GetParent(hWnd);
		if ( htWnd == NULL ) break;
		hWnd = htWnd;
	}
	return hWnd;
}

// 親ウィンドウ(タイトル付き)を推測する ---------------------------------------
HWND GetParentCaptionWindow(HWND hWnd)
{
	HWND nhWnd;

	nhWnd = hWnd;
	while ( (nhWnd = GetParent(nhWnd)) != NULL ){
		if ( GetWindowLong(nhWnd, GWL_STYLE) & (WS_CAPTION & ~WS_BORDER) ){
			return nhWnd;
		}
	}
	nhWnd = GetParent(hWnd);
	return nhWnd;
}

// ウィンドウを親ウィンドウのまん中に移動させる -------------------------------
PPXDLL void PPXAPI CenterWindow(HWND hWnd)
{
	MoveCenterWindow(hWnd, GetParentCaptionWindow(hWnd));
}

PPXDLL void PPXAPI MoveCenterWindow(HWND hWnd, HWND hParentWnd)
{
	RECT box, pbox, desk;
	int parentheight;

	GetDesktopRect( (hParentWnd != NULL) ? hParentWnd : hWnd, &desk);
	GetWindowRect(hWnd, &box);
	if ( hParentWnd != NULL ){
		GetWindowRect(hParentWnd, &pbox);
	}else{
		pbox = desk;
	}
								// 幅と高さに変換
	box.right -= box.left;
	box.bottom -= box.top;

								// 左右を中央に移動
	box.left = pbox.left + (((pbox.right  - pbox.left) - box.right) / 2);

	parentheight = pbox.bottom - pbox.top;
	if ( parentheight > box.bottom ){ // 親の方が背が高い…上下を中央に移動
		box.top  = pbox.top  + ((parentheight - box.bottom) / 2);
	}else{ // 親の方が背が低い…親の下に移動
		if ( (pbox.right - pbox.left) != box.right ){
			box.top = pbox.top + (parentheight / 5);
		}else{
			box.top = pbox.bottom - (parentheight / 3);
		}
	}
								// 左右がはみ出しているか？
	if ( (box.left + box.right) > desk.right ){
		box.left = desk.right - box.right;
	}
	if ( box.left < desk.left ) box.left = desk.left;
								// 上下がはみ出しているか？
	if ( (box.top + box.bottom) > desk.bottom ){
		box.top = desk.bottom - box.bottom;
	}
	if ( box.top < desk.top ) box.top = desk.top;

	SetWindowPos(hWnd, NULL, box.left, box.top, 0, 0,
			SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

typedef struct {
	DWORD dwSize;
	DWORD dwICC;
} INITCOMMONCONTROLSEXSTRUCT;
INITCOMMONCONTROLSEXSTRUCT UseCommonControls = {
	sizeof(INITCOMMONCONTROLSEXSTRUCT), 0
};

//  -------------------------------
PPXDLL HANDLE PPXAPI LoadCommonControls(DWORD usecontrol)
{
	DefineWinAPI(void, InitCommonControls, (void));
	DefineWinAPI(void, InitCommonControlsEx, (INITCOMMONCONTROLSEXSTRUCT *));

	if ( (X_dss == DSS_NOLOAD) && (WinType >= WINTYPE_VISTA) ){
		GetCustData(T("X_dss"), &X_dss, sizeof(X_dss));
	}

	if ( hComctl32 != NULL ){	// 実行済み
		if ( UseCommonControls.dwICC == 0 ) return hComctl32; // no Ex
	}else{
		hComctl32 = LoadSystemDLL(SYSTEMDLL_COMCTL32);
		if ( hComctl32 == NULL ) return NULL;
		GETDLLPROC(hComctl32, InitCommonControlsEx);
		if ( DInitCommonControlsEx != NULL ){
			UseCommonControls.dwICC = 0;
		}else{
			GETDLLPROC(hComctl32, InitCommonControls);
			if ( DInitCommonControls != NULL ) DInitCommonControls();
			return hComctl32;
		}
	}
	if ( (UseCommonControls.dwICC & usecontrol) != usecontrol ){	// まだ未ロードがある
		setflag(UseCommonControls.dwICC, usecontrol);
		GETDLLPROC(hComctl32, InitCommonControlsEx);
		DInitCommonControlsEx(&UseCommonControls);
	}
	return hComctl32;
}

void SetWindowRound(HWND hWnd)
{
	if ( OSver.dwBuildNumber < WINBUILD_11_21H1 ) return;
	if ( X_uxt_window_corner > 0 ){
		if ( hDwmapi == NULL ){
			hDwmapi = LoadSystemDLL(SYSTEMDLL_DWMAPI);
			if ( hDwmapi != NULL ) GETDLLPROC(hDwmapi, DwmSetWindowAttribute);
		}
		if ( DDwmSetWindowAttribute != NULL ){
			#define DWMWA_WINDOW_CORNER_PREFERENCE 33
			DDwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE,
					&X_uxt_window_corner, sizeof(X_uxt_window_corner));
		}
	}
}

#pragma argsused
BOOL CALLBACK EnumChild_ControlFixProc(HWND hWnd, LPARAM lParam)
{
	TCHAR name[64];
	UnUsedParam(lParam);

	if ( ThemeColors.ExtraDrawFlags & EDF_DIALOG_TEXT ){
		if ( 0 < GetClassName(hWnd, name, TSIZEOF(name)) ){
			FixUxTheme(hWnd, name);
		}
	}
	return TRUE;
}

#pragma argsused
BOOL CALLBACK EnumChild_TextFixProc(HWND hWnd, LPARAM lParam)
{
	TCHAR name[64];
	const TCHAR *findtext;
	DWORD ctrlID;
	UnUsedParam(lParam);

	if ( ThemeColors.ExtraDrawFlags & EDF_DIALOG_TEXT ){
		if ( 0 < GetClassName(hWnd, name, TSIZEOF(name)) ){
			FixUxTheme(hWnd, name);
		}
	}
	ctrlID = (DWORD)GetWindowLongPtr(hWnd, GWLP_ID);
	if ( (ctrlID != 0) && (ctrlID < 0xffff) ){
		thprintf(name, TSIZEOF(name), T("%04X"), ctrlID);
		findtext = SearchMessageText(name);
		if ( findtext != NULL ) SetWindowText(hWnd, findtext);
	}
	return TRUE;
}

#pragma argsused
BOOL CALLBACK EnumChild_RichControlFixProc(HWND hWnd, LPARAM lParam)
{
	TCHAR name[64];

	if ( 0 < GetClassName(hWnd, name, TSIZEOF(name)) ){
		if ( memicmp(name, RichEditClassHead, RichEditClassHeadSize) == 0 ){
		// ASCII フォントと日本語フォントを共通にする
		// ※ FixRichEditTheme でも行っている
			SendMessage(hWnd, EM_SETLANGOPTIONS, 0,
				SendMessage(hWnd, EM_GETLANGOPTIONS, 0, 0) & ~IMF_DUALFONT );
		}
	}
	return TRUE;
}

PPXDLL void PPXAPI LocalizeDialogText(HWND hDlg, DWORD titleID)
{
	if ( MessageTextTable == NULL ) LoadMessageTextTable();
	if ( MessageTextTable != NOMESSAGETEXT ){
		if ( titleID != 0 ){
			TCHAR name[8];
			const TCHAR *findtext;

			thprintf(name, TSIZEOF(name), T("%04X"), titleID);
			findtext = SearchMessageText(name);
			if ( findtext != NULL ) SetWindowText(hDlg, findtext);
		}
		if ( X_uxt_color >= UXT_MINMODIFY ) FixUxTheme(hDlg, NULL);
		EnumChildWindows(hDlg, EnumChild_TextFixProc, 0);
	}else{
		if ( X_uxt_color >= UXT_MINMODIFY ) FixUxTheme(hDlg, NULL);
		EnumChildWindows(hDlg, EnumChild_ControlFixProc, 0);
	}
	if ( (hRichEdit != NULL) &&
		 !(ThemeColors.ExtraDrawFlags & EDF_DIALOG_TEXT) ){
		EnumChildWindows(hDlg, EnumChild_RichControlFixProc, 0);
	}
	SetWindowRound(hDlg);
}

#if 0 // ● 1.46+3 未使用みたいなので廃止
PPXDLL void PPXAPI MessageTextDialog(HWND hDlg)
{
	LocalizeDialogText(hDlg, 0);
}
#endif

ERRORCODE ExecuteCust(PPXAPPINFO *info, const TCHAR *str, const TCHAR *sub, int option)
{
	TCHAR buf[CMDLINESIZE], *cmd;
	ERRORCODE result;

	cmd = GetCustValue(str, sub, buf, sizeof(buf));
	if ( cmd == NULL ) return ERROR_ENVVAR_NOT_FOUND;

	if ( option & B31 ){
		if ( (UTCHAR)cmd[0] == EXTCMD_KEY ){
			WORD *keyp, readkey;

			if ( X_Keyra == 1 ){
				X_Keyra = 0;
				GetCustData(T("X_Keyra"), &X_Keyra, sizeof(X_Keyra));
				X_Keyra = (X_Keyra == 0) ? K_raw : 0;
			}
			keyp = (WORD *)(cmd + 1);
			for ( ;; ){
				readkey = *keyp;
				if ( readkey == 0 ){
					result = NO_ERROR;
					break;
				}
				result = (ERRORCODE)SendMessage(info->hWnd, WM_PPXCOMMAND, readkey, 0);
				if ( result != NO_ERROR ) break;
				keyp++;
			}
		}else{
			result = PP_ExtractMacro(info->hWnd, info, NULL,
					((UTCHAR)cmd[0] == EXTCMD_CMD) ? cmd + 1 : cmd, NULL, 0);
		}
	}else{
		result = PP_ExtractMacro(info->hWnd, info, NULL, cmd, NULL, 0);
	}

	if ( cmd != buf ) HeapFree(ProcHeap, 0, cmd);
	return result;
}

const TCHAR *AjiModeStr[] = { T("show"), T("comp"), NilStr, T("start"), NilStr};

BOOL CheckAjiParam(DWORD mode, const TCHAR *type, const TCHAR *name, TCHAR *param)
{
	TCHAR keyword[64];

	thprintf(keyword, TSIZEOF(keyword), T("%s%s%s"), name, AjiModeStr[mode], type);
	param[0] = '\0';
	GetCustTable(T("X_jinfc"), keyword, param, TSTROFF(CMDLINESIZE));
	if ( param[0] != '\0' ) return TRUE;
	if ( mode <= AJI_SELECT ){
		if ( mode == AJI_SELECT ){
			tstrcpy(keyword, AjiModeStr[AJI_SHOW]);
			name = NilStr;
		}
		GetCustTable(T("X_jinfc"), keyword + tstrlen(name), param, TSTROFF(CMDLINESIZE));
		if ( param[0] != '\0' ) return TRUE;
	}
	return FALSE;
}

// ●legacy
PPXDLL void PPXAPI ActionJobInfo(HWND hWnd, DWORD mode, const TCHAR *name)
{
	ActionInfo(hWnd, NULL, mode, name);
}

PPXDLL void PPXAPI ActionInfo(HWND hWnd, PPXAPPINFO *info, DWORD mode, const TCHAR *name)
{
	HWND hFWnd;
	DWORD flash, play;
	TCHAR param[CMDLINESIZE];

	if ( AjiEnterCount > 5 ) return;
	AjiEnterCount++;

	if ( mode <= AJI_SELECT ){
		int jinfomode;
		if ( X_jinfo[0] == MAX32 ){
			X_jinfo[0] = 1;
			GetCustData(T("X_jinfo"), X_jinfo, sizeof(X_jinfo));
		}
		hFWnd = GetForegroundWindow();
		jinfomode = (mode == AJI_COMPLETE) ? 2 : 0;
		flash = X_jinfo[jinfomode];
		if ( (flash > 1) || ((flash == 1) && (hWnd != hFWnd) && (GetParent(hWnd) != hFWnd)) ){
			PPxFlashWindow(hWnd, PPXFLASH_NOFOCUS);
		}
		play = X_jinfo[jinfomode + 1];
		if ( (play > 1) || ((play == 1) && (hWnd != hFWnd)) ){
			if ( IsTrue(CheckAjiParam(mode, T("wav"), name, param)) ){
				PlayWave(param);
			}
		}
	}

	if ( IsTrue(CheckAjiParam(mode, T("cmd"), name, param)) ){
		PP_ExtractMacro(hWnd, info, NULL, param, NULL, 0);
	}
	AjiEnterCount--;
}

PPXDLL BOOL PPXAPI GetCalc(const TCHAR *param, TCHAR *resultstr, int *resultnum)
{
	int num, i, j;
	const TCHAR *ptr;

	ptr = param;
	if ( CalcString(&ptr, &num) != CALC_NOERROR ) return FALSE;

	if ( resultnum != NULL ) *resultnum = num;
	if ( resultstr != NULL ){
		char str[6], *bptr;

		bptr = str;
		j = num;
		for ( i = 0 ; i < 4 ; i++ ){
			*bptr = (char)j;
			if ( (BYTE)*bptr < (BYTE)' ' ) *bptr = '.';
			bptr++;
			j >>= 8;
		}
		str[4] = '\0';
		str[5] = '\0';
		// D:-1234567890 X:499602d2 U:1234567890(1.20G) xxxxxx ...53文字
		thprintf(resultstr, GetCalc_ResultMaxLength, T("D:%d X:%x U:%u(%Mu) %hs"), num, num, num, num, str);
	}
	return TRUE;
}

void InitUxThemeMain(void)
{
	if ( OSver.dwBuildNumber < WINBUILD_10_19H1 ){
		X_uxt_color = UXT_OFF;
		InitSysColors();
		return;
	}
	if ( X_uxt_color == UXT_AUTO ){
		GetRegDword(HKEY_CURRENT_USER, ThemesPersonalize, ThemesAppsUseLightTheme, (DWORD *)&X_uxt_color);
	}
	InitSysColors();

	if ( hDwmapi == NULL ) hDwmapi = LoadSystemDLL(SYSTEMDLL_DWMAPI);
	if ( hDwmapi != NULL ) GETDLLPROC(hDwmapi, DwmSetWindowAttribute);
//	GETDLLPROCN(hUxtheme, ShouldAppsUseDarkMode, 132);
	GETDLLPROCN(hUxtheme, AllowDarkModeForWindow, 133);
	GETDLLPROCN(hUxtheme, AllowDarkModeForApp, 135);
//	GETDLLPROCN(hUxtheme, FlushMenuTheme, 136);
	GETDLLPROCN(hUxtheme, RefreshImmersiveColorPolicyState, 104);

//	DShouldAppsUseDarkMode();
	if ( X_uxt_color == UXT_LIGHT ){ // Dark から切り替えようとすると失敗する対策
		DAllowDarkModeForApp(1);
		DRefreshImmersiveColorPolicyState();
	}
	DAllowDarkModeForApp( (X_uxt_color == UXT_DARK) ? 2 : 3);
	DRefreshImmersiveColorPolicyState();
}

BOOL InitUnthemeDLL(void)
{
	if ( hUxtheme != NULL ) return TRUE;
	hUxtheme = LoadSystemDLL(SYSTEMDLL_UXTHEME);
	if ( hUxtheme != NULL ) return TRUE;
	return FALSE;
}

void InitUnthemeCmd(void)
{
	X_uxt_color = UXT_OFF;
	if ( IsTrue(InitUnthemeDLL()) ){
		GETDLLPROC(hUxtheme, SetWindowTheme);
		if ( DSetWindowTheme != NULL ){
			GetCustData(T("X_uxt"), &X_uxt, sizeof(X_uxt));
			if ( X_uxt_color >= UXT_MINMODIFY ) InitUxThemeMain();
		}
	}
	InitSysColors();
}

void ReloadUnthemeCmd(void)
{
	int old_uxt;

	if ( X_uxt_color == UXT_NA ) return; // 未初期化…InitUnthemeCmdで初期化
	old_uxt = X_uxt_color;

	FreeSysColors();
	memset(&ThemeColors, 0xff, sizeof(ThemeColors)); // ThemeColors を C_auto に戻す & ExtraDrawFlags に EDF_NOT_INIT をセット
	C_SchemeColor1[0] = C_AUTO;
	C_SchemeColor2[0] = C_AUTO;
	X_uxt_color = UXT_OFF;
	GetCustData(T("X_uxt"), &X_uxt, sizeof(X_uxt));
	if ( X_uxt_color >= UXT_MINMODIFY ){
		InitUxThemeMain();
	}else{
		InitSysColors_main();
	}
	if ( old_uxt >= UXT_MINMODIFY ){
		ThemeColors.ExtraDrawFlags = EDF_WINDOW_TEXT | EDF_WINDOW_BACK | EDF_DIALOG_TEXT | EDF_DIALOG_BACK;
	}
	if ( (old_uxt >= UXT_MINMODIFY) && (X_uxt_color == UXT_OFF) ){
		DAllowDarkModeForApp(3);
		DRefreshImmersiveColorPolicyState();
	}
}

int GetUxtMode(void)
{
	int uxt = UXT_OFF;

	GetCustData(T("X_uxt"), &uxt, sizeof(uxt));
	if ( uxt == UXT_AUTO ){
		GetRegDword(HKEY_CURRENT_USER, ThemesPersonalize, ThemesAppsUseLightTheme, (DWORD *)&uxt);
	}
	return uxt;
}

#ifndef BS_TYPEMASK
#define BS_TYPEMASK 0xf
#endif

const TCHAR Button_prop_name[] = T("xUxTx");

#define AllButtonImage ((3 * 2) + (2 * 2))
#define Index_NonCheck 0
#define Index_Check 2
#define Index_Indeterminate 4
#define Index_Radio 6
#define Check_Buttons (Index_Indeterminate + 2)
#define ButtonImageCharCount 2
const TCHAR ButtonImageChars[ButtonImageCharCount + 1] = {
//	0xa8, 0xa8, 0xa8,	// Check buttons
	0xa1, 0xa4, 0x6c	// Radio buttons
};

const TCHAR CheckImageChars[2] = {
	0xfc, 0xa7 // Check Char, Indeterminate-Char
};

volatile int ButtonBMPheight;
LOGFONT ButtonSymbolLog = {
/*Width, Height*/					0, 0,
/*Escapement, Orientation, Weight*/	0, 0, FW_NORMAL,
/*Italic, Underline, StrikeOut*/	FALSE, FALSE, FALSE,
/*CharSet*/							SYMBOL_CHARSET,
/*OutPrecision*/					OUT_DEFAULT_PRECIS,
/*ClipPrecision*/					CLIP_DEFAULT_PRECIS,
/*Quality*/							DEFAULT_QUALITY,
/*PitchAndFamily*/					FIXED_PITCH | FF_DONTCARE,
/*FaceName*/						T("Wingdings")
};

HBITMAP MakeButtonImages(HWND hWnd, HDC hDC)
{
	HBITMAP hBMP;
	HDC hMemDC;
	RECT box;
	HGDIOBJ hOldBmp, hOldFont;
	int i;
	int linewidth, lw;
	HFONT hControlFont;
	TEXTMETRIC tm;
	SIZE chrsize;
	BITMAPINFO bmi;
	LPVOID lpBits;
	HBRUSH hBr;
	RECT ImageSize = {0, 0, 8, 8};

	MapDialogRect(GetParent(hWnd), &ImageSize);
	if ( (hButtonBMP != NULL) &&
		 ( ImageSize.bottom == ButtonBMPheight) ){
		return hButtonBMP;
	}
	EnterCriticalSection(&ThreadSection);
	if ( (hButtonBMP != NULL) &&
		 (ImageSize.bottom == ButtonBMPheight) ){ // 待機中に再作成された
		LeaveCriticalSection(&ThreadSection);
		return hButtonBMP;
	}
	ButtonBMPheight = 0; // 一時的に無効にする

	if ( hButtonBMP != NULL ){
		DeleteObject(hButtonBMP);
		hButtonBMP = NULL;
	}

	ButtonSymbolLog.lfHeight = -ImageSize.bottom;
	hControlFont = CreateFontIndirect(&ButtonSymbolLog);

	hMemDC = CreateCompatibleDC(hDC);
	hOldFont = SelectObject(hMemDC, hControlFont);
	GetTextMetrics(hMemDC, &tm);
	if ( tm.tmHeight == 0 ){
		tm.tmHeight = GetSystemMetrics(SM_CYMENU) - 5;
	}
	linewidth = tm.tmHeight / 15;
	if ( linewidth < 1 ) linewidth = 1;

	memset(&bmi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biHeight = tm.tmHeight - tm.tmInternalLeading;
	bmi.bmiHeader.biWidth = bmi.bmiHeader.biHeight * AllButtonImage * 2;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = (WORD)((OSver.dwMajorVersion < 6) ? 24 : 32);
	hButtonBMP = hBMP = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &lpBits, NULL, 0);
	hOldBmp = SelectObject(hMemDC, hBMP);

	box.left = box.top = 0;
	box.right = box.bottom = bmi.bmiHeader.biWidth;
	FillBox(hMemDC, &box, hDialogBackBrush);

	// チェックボックスの枠を作成
	GetEdgeLineBrush(); // 戻り値は未使用
	ControlWindowColor((WPARAM)hMemDC);
	for ( lw = 0 ; lw < linewidth ; lw++ ){
		box.left = box.top = lw;
		box.right = box.bottom = bmi.bmiHeader.biHeight - lw;
		for ( i = 0 ; i < Check_Buttons ; i++ ){
			if ( lw == 0 ) FillBox(hMemDC, &box, hWindowBackBrush);
			FrameRect(hMemDC, &box, hEdgeLine);
			box.left += bmi.bmiHeader.biHeight;
			box.right += bmi.bmiHeader.biHeight;
		}
	}
	// チェックボックスのチェックを作成
	GetTextExtentPoint32(hMemDC, CheckImageChars, 1, &chrsize);
	box.left = bmi.bmiHeader.biHeight * Index_Check + (tm.tmHeight - chrsize.cx) / 2 - linewidth;
	SetBkMode(hMemDC, TRANSPARENT);
	SetBkColor(hMemDC, C_DialogBack);
//	SetTextColor(hMemDC, C_WindowText);
	TextOut(hMemDC, box.left, -tm.tmInternalLeading + linewidth, CheckImageChars, 1);
	SetTextColor(hMemDC, C_GrayText);
	TextOut(hMemDC, box.left + bmi.bmiHeader.biHeight, -tm.tmInternalLeading + linewidth, CheckImageChars, 1);

	// チェックボックスの不定印を作成
	box.top = linewidth * 4;
	box.left = bmi.bmiHeader.biHeight * Index_Indeterminate + box.top;
	box.right = bmi.bmiHeader.biHeight * (Index_Indeterminate + 1) - box.top;
	box.bottom = bmi.bmiHeader.biHeight - box.top;
	hBr = CreateSolidBrush(C_WindowText);
	FillBox(hMemDC, &box, hBr);
	DeleteObject(hBr);
	box.left += bmi.bmiHeader.biHeight;
	box.right += bmi.bmiHeader.biHeight;
	FillBox(hMemDC, &box, hEdgeLine);

	// ラジオボタンを作成
	box.left = Index_Radio * bmi.bmiHeader.biHeight;
	for ( i = 0 ; i < ButtonImageCharCount * 2 ; i++ ){
		SetTextColor(hMemDC, (i & 1) ? C_DialogBack : C_WindowBack);
		GetTextExtentPoint32(hMemDC, ButtonImageChars + 2, 1, &chrsize);
		TextOut(hMemDC,
				box.left + (tm.tmHeight - chrsize.cx) / 2,
				-tm.tmInternalLeading + linewidth,
				ButtonImageChars + 2, 1);

		SetTextColor(hMemDC, (i & 1) ? C_GrayText : C_WindowText);
		GetTextExtentPoint32(hMemDC, ButtonImageChars + i / 2, 1, &chrsize);
		TextOut(hMemDC,
				box.left + (tm.tmHeight - chrsize.cx) / 2,
				-tm.tmInternalLeading + linewidth,
				ButtonImageChars + i / 2, 1);
		box.left += bmi.bmiHeader.biHeight;
	}
	SelectObject(hMemDC, hOldBmp);
	SelectObject(hMemDC, hOldFont);
	DeleteObject(hControlFont);
	DeleteDC(hMemDC);
	ButtonBMPheight = ImageSize.bottom; // 有効にする
	LeaveCriticalSection(&ThreadSection);
	return hBMP;
}

void PaintEtcHed(HWND hWnd)
{
	PAINTSTRUCT ps;
	RECT box;

	BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &box);
	box.bottom = (box.bottom - box.top) / 2;
	FillBox(ps.hdc, &box, GetEdgeLineBrush());
	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK DarkEtcHedProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ( uMsg == WM_PAINT ){
		PaintEtcHed(hWnd);
		return 0;
	}
	return CallWindowProc((WNDPROC)GetProp(hWnd, Button_prop_name), hWnd, uMsg, wParam, lParam);
}

void PaintGrayStatic(HWND hWnd)
{
	PAINTSTRUCT ps;
	TCHAR buf[0x80];
	int length;
	COLORREF oldcolor;
	RECT box;
	HGDIOBJ  hOldFont;

	GetClientRect(hWnd, &box);
	BeginPaint(hWnd, &ps);
	length = GetWindowText(hWnd, buf, TSIZEOF(buf));
	oldcolor = SetTextColor(ps.hdc, C_GrayText);
	SetBkMode(ps.hdc, TRANSPARENT);
	hOldFont = SelectObject(ps.hdc, (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0));
	DrawText(ps.hdc, buf, length, &box, DT_CENTER | DT_VCENTER);
	SelectObject(ps.hdc, hOldFont);
	SetBkMode(ps.hdc, OPAQUE);
	SetTextColor(ps.hdc, oldcolor);
	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK DarkGrayStaticProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ( uMsg == WM_PAINT ){
		PaintGrayStatic(hWnd);
		return 0;
	}
	return CallWindowProc((WNDPROC)GetProp(hWnd, Button_prop_name), hWnd, uMsg, wParam, lParam);
}

void PaintGroupButton(HWND hWnd)
{
	PAINTSTRUCT ps;
	TCHAR buf[0x80];
	int length;
	COLORREF oldcolor, oldback;
	RECT textbox, framebox = {0, 0, 1, 3};
	HGDIOBJ hOldFont;

	MapDialogRect(GetParent(hWnd), &framebox);
	BeginPaint(hWnd, &ps);
	length = GetWindowText(hWnd, buf, TSIZEOF(buf) - 2);
	GetClientRect(hWnd, &textbox);

	framebox.left = textbox.left + framebox.right;
	framebox.right = textbox.right - framebox.right;
	framebox.top = textbox.top + framebox.bottom;
	framebox.bottom = textbox.bottom - 2;
	FrameRect(ps.hdc, &framebox, GetEdgeLineBrush());

	textbox.left = framebox.left * 8 - textbox.left;
	oldcolor = SetTextColor(ps.hdc, C_DialogText);
	oldback = SetBkColor(ps.hdc, C_DialogBack);
	hOldFont = SelectObject(ps.hdc, (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0));
	DrawText(ps.hdc, buf, length, &textbox, 0);
	SelectObject(ps.hdc, hOldFont);
	SetBkColor(ps.hdc, oldback);
	SetTextColor(ps.hdc, oldcolor);
	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK DarkGroupProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ( uMsg == WM_PAINT ){
		PaintGroupButton(hWnd);
		return 0;
	}
	return CallWindowProc((WNDPROC)GetProp(hWnd, Button_prop_name), hWnd, uMsg, wParam, lParam);
}

void PaintStateButton(HWND hWnd)
{
	PAINTSTRUCT ps;
	TCHAR buf[0x80];
	int length, height;
	COLORREF oldcolor, oldback;
	RECT box;
	HGDIOBJ hOldBmp, hOldFont;
	DWORD w_style, b_state;
	HDC hMemDC;
	int buttonindex;

	w_style = GetWindowLongPtr(hWnd, GWL_STYLE);
	b_state = (DWORD)SendMessage(hWnd, BM_GETSTATE, 0, 0);
	BeginPaint(hWnd, &ps);

	GetClientRect(hWnd, &box);
	FillBox(ps.hdc, &box, hDialogBackBrush);

	// ボタンを描画
	buttonindex = ((w_style & BS_TYPEMASK) == BS_AUTORADIOBUTTON) ? 6 : 0;
	buttonindex += (b_state & (BST_CHECKED | BST_INDETERMINATE)) * 2;
	if ( w_style & WS_DISABLED ) buttonindex++;
	hMemDC = CreateCompatibleDC(ps.hdc);
	hOldBmp = SelectObject(hMemDC, MakeButtonImages(hWnd, ps.hdc));

	height = ButtonBMPheight;
	if ( w_style & BS_LEFTTEXT ){
		BitBlt(ps.hdc, box.right - height, (box.bottom - height) / 2, height, height, hMemDC, height * buttonindex , 0, SRCCOPY);
		box.right -= height + 4;
	}else{
		BitBlt(ps.hdc, 0, (box.bottom - height) / 2, height, height, hMemDC, height * buttonindex , 0, SRCCOPY);
		box.left += height + 4;
	}
	SelectObject(hMemDC, hOldBmp);
	DeleteDC(hMemDC);

	// 文字列を描画
	length = GetWindowText(hWnd, buf, TSIZEOF(buf));
	oldcolor = SetTextColor(ps.hdc, (w_style & WS_DISABLED) ? C_GrayText : C_DialogText);
	oldback = SetBkColor(ps.hdc, (b_state & BST_PUSHED) ? C_GrayText : C_DialogBack);
	hOldFont = SelectObject(ps.hdc, (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0));
	DrawText(ps.hdc, buf, length, &box, (w_style & BS_CENTER) ? (DT_SINGLELINE	 | DT_CENTER | DT_VCENTER) : (DT_SINGLELINE | DT_VCENTER));
	SelectObject(ps.hdc, hOldFont);
	SetBkColor(ps.hdc, oldback);
	SetTextColor(ps.hdc, oldcolor);

	// フォーカス表示
	if ( b_state & BST_FOCUS ){
		box.right--;
		box.bottom--;
		DrawFocusRect(ps.hdc, &box);
	}

	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK DarkButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ( uMsg == WM_PAINT ){
		PaintStateButton(hWnd);
		return 0;
	}
	return CallWindowProc((WNDPROC)GetProp(hWnd, Button_prop_name), hWnd, uMsg, wParam, lParam);
}

void FixRichEditTheme(HWND hWnd /*, THEME_LIST *tl*/)
{
	CHARFORMAT2 chfmt;

	// ASCII フォントと日本語フォントを共通にする
	// ※ EnumChild_RichControlFixProc でも行っている
	SendMessage(hWnd, EM_SETLANGOPTIONS, 0,
			SendMessage(hWnd, EM_GETLANGOPTIONS, 0, 0) & ~IMF_DUALFONT );

	// 背景色設定
	SendMessage(hWnd, EM_SETBKGNDCOLOR, 0, (LPARAM)C_WindowBack);

	// 文字色設定
	chfmt.cbSize = sizeof(chfmt);
	chfmt.dwMask = CFM_COLOR;
	chfmt.dwEffects = 0;
	chfmt.crTextColor = C_WindowText;
	// SCF_DEFAULT は 0 なので別個に指定
	SendMessage(hWnd, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&chfmt);
	SendMessage(hWnd, EM_SETCHARFORMAT, SPF_SETDEFAULT | SCF_SELECTION | SCF_ALL, (LPARAM)&chfmt);
}

void FixStaticTheme(HWND hWnd, THEME_LIST *tl)
{
	DWORD style;

	style = GetWindowLongPtr(hWnd, GWL_STYLE);
	if ( (style & SS_TYPEMASK) == SS_ETCHEDFRAME ){
		DSetWindowTheme(hWnd, NilStrW, NilStrW); // Clasic style
		if ( GetProp(hWnd, Button_prop_name) == 0 ){
			LONG_PTR oldprop;

			oldprop = SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)DarkEtcHedProc);
			SetProp(hWnd, Button_prop_name, (HANDLE)oldprop);
		}
	}else if ( (style & (SS_TYPEMASK | WS_DISABLED)) == (SS_CENTER | WS_DISABLED) ){
		DSetWindowTheme(hWnd, NilStrW, NilStrW); // Clasic style
		if ( GetProp(hWnd, Button_prop_name) == 0 ){
			LONG_PTR oldprop;

			oldprop = SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)DarkGrayStaticProc);
			SetProp(hWnd, Button_prop_name, (HANDLE)oldprop);
		}
	}else{
		if ( X_uxt_color >= UXT_MINMODIFY ){
			DSetWindowTheme(hWnd, tl->general, NULL);
		}
	}
}

void FixButtonTheme(HWND hWnd, THEME_LIST *tl)
{
	DWORD style;

	style = GetWindowLongPtr(hWnd, GWL_STYLE);
	// CHECKBOX(0x2) / RADIOBUTTON(0x4) / GROUPBOX(0x7) / AUTORADIOBUTTON(0x9)
	// の新しいスタイルは、ダークモード＆文字色変更に対応していない
	if ( style & (BS_CHECKBOX | BS_RADIOBUTTON | 0x8) ){
		DSetWindowTheme(hWnd, NilStrW, NilStrW); // Clasic style
		if ( GetProp(hWnd, Button_prop_name) == 0 ){
			LONG_PTR oldprop;
			WNDPROC newprop;

			if ( (style & BS_TYPEMASK) == BS_GROUPBOX ){
				newprop = DarkGroupProc;
			}else{
				newprop = DarkButtonProc;
			}
			oldprop = SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)newprop);
			SetProp(hWnd, Button_prop_name, (HANDLE)oldprop);
		}
	}else{ // Push Button / Def Push Button
		if ( X_uxt_color >= UXT_MINMODIFY ){
			DSetWindowTheme(hWnd, tl->general, NULL);
		}
	}
}

PPXDLL int PPXAPI FixUxTheme(HWND hWnd, const TCHAR *classname)
{
	BOOL mode;
#if 0
	TCHAR title[0x400];
	GetWindowText(hWnd, title, 0x400);
	XMessage(NULL, NULL, XM_DbgLOG, T("%s %s"), classname, title);
#endif
	if ( (classname == NULL) || (classname[0] == '\0') ) SetWindowRound(hWnd);

	if ( X_uxt_color < UXT_MINMODIFY ){
		if ( (ThemeColors.ExtraDrawFlags & EDF_DIALOG_TEXT) && (classname != NULL) && (DSetWindowTheme != NULL)){
			if ( tstricmp(classname, ButtonClassName) == 0 ){
				FixButtonTheme(hWnd, NULL);
			}else if ( tstricmp(classname, T("Static")) == 0 ){
				FixStaticTheme(hWnd, &Theme_Light);
			}
		}
		return UXT_OFF;
	}
	mode = (X_uxt_color == UXT_DARK);
	DAllowDarkModeForWindow(hWnd, mode);
	if ( X_uxt_color >= UXT_MINMODIFY ){
		THEME_LIST *tl;

		tl = (X_uxt_color == UXT_DARK) ? &Theme_Dark : &Theme_Light;
		if ( classname != NULL ){
			if ( tstrcmp(classname, REBARCLASSNAME) == 0 ){
				DSetWindowTheme(hWnd, tl->rebar, NULL);
			}else if ( tstrcmp(classname, TOOLBARCLASSNAME) == 0 ){
				DSetWindowTheme(hWnd, tl->toolbar, NULL);
			}else if ( tstricmp(classname, T("ComboBox")) == 0 ){
				DSetWindowTheme(hWnd, tl->ComboBox, NULL);
			}else if ( tstrcmp(classname, WC_HEADER) == 0 ){
				DSetWindowTheme(hWnd, tl->ListView, NULL);
			}else if ( tstrcmp(classname, WC_LISTVIEW) == 0 ){
#if IATHOOK
				HookUxtheme();
#endif
				DSetWindowTheme(hWnd, (OSver.dwBuildNumber < WINBUILD_11_21H1) ?
						tl->ListView : tl->general, NULL);
				ListView_SetBkColor(hWnd, C_WindowBack);
				ListView_SetTextBkColor(hWnd, C_WindowBack);
				ListView_SetTextColor(hWnd, C_WindowText);
			}else if ( tstricmp(classname, ButtonClassName) == 0 ){
				FixButtonTheme(hWnd, tl);
			}else if ( tstricmp(classname, T("Static")) == 0 ){
				FixStaticTheme(hWnd, tl);
			}else if ( memicmp(classname, RichEditClassHead, RichEditClassHeadSize) == 0 ){
				FixRichEditTheme(hWnd /*, tl*/);
				DSetWindowTheme(hWnd, tl->general, NULL);
			}else{
				if ( tstricmp(classname, WC_TREEVIEW) == 0 ){
					SetTreeColor(hWnd);
				}else if ( tstricmp(classname, WC_EDIT) == 0 ){
					DWORD exstyle;

					exstyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
					if ( exstyle & WS_EX_CLIENTEDGE ){
						SetWindowLongPtr(hWnd, GWL_EXSTYLE,
							exstyle & ~WS_EX_CLIENTEDGE);
						SetWindowLongPtr(hWnd, GWL_STYLE,
							GetWindowLongPtr(hWnd, GWL_STYLE) | WS_BORDER);
					}
				}
				DSetWindowTheme(hWnd, tl->general, NULL);
			}
		}else{
			DSetWindowTheme(hWnd, tl->general, NULL);
		}
	}
	// DWMWA_USE_IMMERSIVE_DARK_MODE 10.0.20348.0 で SDK に登録 (20)
	DDwmSetWindowAttribute(hWnd,
			(OSver.dwBuildNumber < WINBUILD_10_20H1) ? 19 : 20,
			&mode, sizeof(mode));
	return X_uxt_color;
}

PPXDLL void PPXAPI FixCharlengthTable(char *table)
{
	if ( LoadedCharlengthTable == FALSE ){
		TCHAR localebuf[16];
		LCID userlcid;

		LoadedCharlengthTable = TRUE;
		if ( table != NULL ) PPxProcess = TRUE;

		if ( GetACP() != CP__SJIS ){
			char chrcode[] = "\x01\x41";
			BYTE *p;
			int i;

			p = (BYTE *)&T_CHRTYPE[1];
			for ( i = 1 ; i < sizeof(T_CHRTYPE) ; i++, p++ ){
				chrcode[0] = (char)i;
				*p = (BYTE)((*p & ~3) | (CharNextA(chrcode) - chrcode));
			}
		}
		userlcid = GetUserDefaultLCID();
		GetLocaleInfo(userlcid, LOCALE_STHOUSAND, localebuf, TSIZEOF(localebuf));
		NumberUnitSeparator = localebuf[0];
		GetLocaleInfo(userlcid, LOCALE_SDECIMAL, localebuf, TSIZEOF(localebuf));
		NumberDecimalSeparator = localebuf[0];

		if ( (LOWORD(userlcid) != LCID_JAPANESE) &&
			 (LOWORD(userlcid) != LCID_ENGLISH) ){
			thprintf(localebuf, TSIZEOF(localebuf), T("LC%04X"), LOWORD(userlcid));
			if ( !IsExistCustTable(StrCustOthers, localebuf) ){
				ReportLCID(userlcid);
				SetCustTable(StrCustOthers, localebuf, T("0"), TSTROFF(2));
			}
		}
	}
	if ( table != NULL ) memcpy(table, T_CHRTYPE, sizeof(T_CHRTYPE));
}
#ifndef TokenElevationType
#define TokenElevationType 18
#endif
typedef enum  {
  xTokenElevationTypeDefault = 1,
  xTokenElevationTypeFull,
  xTokenElevationTypeLimited
} xTOKEN_ELEVATION_TYPE;

const TCHAR ElevationString[] = T("Elevate");
const TCHAR LimitString[] = T("Limit");

PPXDLL const TCHAR * PPXAPI CheckRunAs(void)
{
	if ( RunAsMode == RUNAS_NOCHECK ){
		// 別ユーザか確認
		if ( RunAsMode == RUNAS_NOCHECK ){
			HANDLE hProcess = NULL;
			HANDLE hToken = NULL;

			RunAsMode = RUNAS_NORMAL;
			#ifndef UNICODE
			if ( WinType != WINTYPE_9x )
			#endif
			for ( ;; ) {
				TCHAR tinfo[0x40], user[UNLEN + 1], domain[0x80];
				DWORD pid, size, dsize;
				SID_NAME_USE snu;
				HWND hWnd;

				hWnd = FindWindow(T("Progman"), NULL);
				if ( hWnd == NULL ) break;
				GetWindowThreadProcessId(hWnd, &pid);

				RunAsMode = RUNAS_RUNAS;
				hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
				if ( hProcess == NULL ) break;

				if ( OpenProcessToken(hProcess, TOKEN_QUERY, &hToken) == FALSE){
					break;
				}
				if ( GetTokenInformation(hToken, TokenUser, tinfo,
							sizeof tinfo, &size) == FALSE ){
					break;
				}
				user[0] = '\0';
				size = TSIZEOF(user);
				dsize = TSIZEOF(domain);
				if ( LookupAccountSid(NULL, ((PTOKEN_USER)tinfo)->User.Sid,
						user, &size, domain, &dsize, &snu) == FALSE ){
					break;
				}
				if ( tstricmp(user, UserName) == 0 ) RunAsMode = RUNAS_NORMAL;
				break;
			}
			if ( hToken != NULL ) CloseHandle(hToken);
			if ( hProcess != NULL ) CloseHandle(hProcess);
		}
	}

	// UAC 状態を取得
	if ( WinType >= WINTYPE_VISTA ){
		HANDLE hCurToken;

		if ( OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hCurToken) ){
			DWORD tetsize = 0;
			xTOKEN_ELEVATION_TYPE tet = xTokenElevationTypeDefault;

			GetTokenInformation(hCurToken, TokenElevationType, &tet, sizeof(tet), &tetsize);
			CloseHandle(hCurToken);
			if ( tet == xTokenElevationTypeFull ){ // UAC 昇格状態
				return ElevationString;
			}else if ( tet == xTokenElevationTypeLimited ){
				// UAC 制限状態
				if ( GetCustDword(T("X_dlim"), 1) ){
					return LimitString;
				}
			}
		}
	}

	if ( RunAsMode <= RUNAS_NORMAL ) return NULL;
	return UserName;
}

HMODULE LoadLibraryTry(const TCHAR *filepath)
{
	HMODULE hDLL;
	int challenge = 10;
	ERRORCODE result;

	for ( ;; ){
		hDLL = LoadLibrary(filepath);
		if ( hDLL != NULL ) return hDLL;
		if ( challenge-- == 0 ) return NULL;
		result = GetLastError();
		if ( (result != ERROR_ACCESS_DENIED) &&
			 (result != ERROR_SHARING_VIOLATION) ){
			SetLastError(result);
			return NULL;
		}
		Sleep(200);
	}
}
#ifdef UNICODE
HMODULE LoadLibraryTryA(const char *filepath)
{
	HMODULE hDLL;
	int challenge = 10;
	ERRORCODE result;

	for ( ;; ){
		hDLL = LoadLibraryA(filepath);
		if ( hDLL != NULL ) return hDLL;
		if ( challenge-- == 0 ) return NULL;
		result = GetLastError();
		if ( (result != ERROR_ACCESS_DENIED) &&
			 (result != ERROR_SHARING_VIOLATION) ){
			SetLastError(result);
			return NULL;
		}
		Sleep(200);
	}
}
#endif


PPXDLL HMODULE PPXAPI LoadWinAPI(const char *DLLname, HMODULE hDLL, LOADWINAPISTRUCT *apis, int mode)
{
	LOADWINAPISTRUCT *list;

	if ( mode & LOADWINAPI_LOAD ){
		hDLL = LoadLibraryTryA(DLLname);
		if ( hDLL == NULL ){
			if ( mode == LOADWINAPI_LOAD_ERRMSG ){
				XMessage(NULL, NULL, XM_GrERRld, T("%hs loaderror"), DLLname);
			}
			return NULL;
		}
	}else if ( mode == LOADWINAPI_GETMODULE ){
		hDLL = GetModuleHandleA(DLLname);
		if ( hDLL == NULL ) return NULL;
	}
	list = apis;
	while ( list->APIname != NULL ){
		*list->APIptr = (void (WINAPI *)())GetProcAddress(hDLL, list->APIname);
		if ( *list->APIptr == NULL ){
			if ( mode & LOADWINAPI_LOAD ){
				while ( apis != list ){
					*apis->APIptr = NULL;
					apis++;
				}
				FreeLibrary(hDLL);
			}
			return NULL;
		}
		list++;
	}
	return hDLL;
}

const TCHAR *SystemDLLlist[] = {
	T("COMDLG32.DLL"),
	T("COMCTL32.DLL"),
	T("OLEAUT32.DLL"),
	T("MPR.DLL"),

	T("WINMM.DLL"),
	T("IMAGEHLP.DLL"),
	T("PSAPI.DLL"),
	T("shcore.dll"),

	T("SHLWAPI.DLL"),
	T("RSTRTMGR.DLL"),
	T("ATL.DLL"),
	T("ATL110.DLL"),

	T("uxtheme.dll"),
	T("dwmapi.dll"),
	T("mlang.dll"),

//	OLEPRO32
	// susie - fullpath - LOAD_WITH_ALTERED_SEARCH_PATH が必要
	// ppxmodule
//	d2d1
//	dwrite
//	zip
};
#define xLOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
int LoadDLLflags = 1;
PPXDLL HMODULE PPXAPI LoadSystemDLL(DWORD dllID)
{
	if ( LoadDLLflags == 1 ){
		if ( (WinType >= WINTYPE_8) || (GetProcAddress(hKernel32, "SetDefaultDllDirectories") != NULL) ){
			LoadDLLflags = xLOAD_LIBRARY_SEARCH_SYSTEM32; // SetDefaultDllDirectories がない Win7/Vista では例外が発生する
		}else{
			LoadDLLflags = 0;
		}
	}
	return LoadLibraryEx(SystemDLLlist[dllID], NULL, LoadDLLflags);
}

HMODULE LoadSystemWinAPI(DWORD dllID, LOADWINAPISTRUCT *apis)
{
	HMODULE hDLL, hDLLresult;

	hDLL = LoadSystemDLL(dllID);
	if ( hDLL == NULL ) return NULL;
	hDLLresult = LoadWinAPI(NULL, hDLL, apis, LOADWINAPI_HANDLE);
	if ( hDLLresult == NULL ){
		FreeLibrary(hDLL);
		hDLL = NULL;
	}
	return hDLL;
}

void USEFASTCALL SetDlgFocus(HWND hDlg, int id)
{
	SetFocus(GetDlgItem(hDlg, id));
}

//------------------------------------- 文字列をバイナリに変換
DWORD ReadEString(TCHAR **string, void *destptr, DWORD destsize)
{
	TCHAR *r;
	BYTE *w;
	DWORD size = 0;

	r = *string;
	w = destptr;
	while( destsize ){
		DWORD seed;
		int i, l;

		seed = 0;
		for ( l = 0 ; l < 4 ; l++ ){
			BYTE c;

			c = (BYTE)*r;
			if ( (c < '0') || (c > ('z' + 1)) ) break;
			r++;
			if ( c >= 'a' ){
				c -= (BYTE)('a' - 10);
			}else if ( c >= 'A' ){
				c -= (BYTE)('A' -(10 + 27));
			}else{
				c -= (BYTE)'0';
			}
			seed = seed | ((DWORD)c << (l * 6));
		}
		l--;
		for ( i = 0 ; i < l ; i++ ){
			*w++ = (BYTE)seed;
			seed >>= 8;
			size++;
			destsize--;
			if ( destsize == 0 ) break;
		}
		if ( l < 3 ) break;
	}
	for ( ; destsize ; destsize-- ) *w++ = 0;

	*string = r;
	return size;
}
//------------------------------------- バイナリを文字列に変換
void WriteEString(TCHAR *dest, BYTE *src, DWORD srcsize)
{
	while( srcsize ){
		DWORD seed;
		int i, l;

		seed = 0;
		for ( l = 0 ; l < 3 ; l++ ){
			seed = seed | (*src << (8 * l));
			src++;
			srcsize--;
			if ( srcsize == 0 ){
				l++;
				break;
			}
		}
		l++;
		for ( i = 0 ; i < l ; i++ ){
			TCHAR r;

			r = (TCHAR)(seed & 0x3f) ;
			seed >>= 6;
			if ( r < 10 ){
				r += (TCHAR)'0';
			}else if ( r < (10 + 27) ){
				r += (TCHAR)('a' - 10);			// 意図的
			}else{
				r += (TCHAR)('A' -(10 + 27));	// 意図的
			}
			*dest++ = r;
		}
	}
	*dest = '\0';
}

WORD FixCharKeycode(WORD key)
{
	if ( IslowerA(key) ) key -= (WORD)0x20;	// 小文字
	if ( IsalnumA(key) || (key <= 0x20) ){
		key |= (WORD)GetShiftKey();
	}else{
		key |= (WORD)(GetShiftKey() & ~K_s);
	}
																// ctrl + char
	if ( (key & K_c) && ((key & 0xff) < 0x20) ) key += (WORD)0x40;
	return key;
}

const LONG crc32_half[16] = {
	0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
	0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
	0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
	0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};

DWORD crc32(const BYTE *bin, DWORD size, DWORD r)
{
	DWORD crc = ~r;
	const BYTE *ptr = bin;
	BYTE chr;

	if ( size == MAX32 ) size = TSTRLENGTH32((TCHAR *)bin);
	while ( size-- ){
		chr = *ptr++;
		crc = crc32_half[(crc ^  chr      ) & 0x0F] ^ (crc >> 4);
		crc = crc32_half[(crc ^ (chr >> 4)) & 0x0F] ^ (crc >> 4);
	}
	return ~crc;
}

#ifdef UNICODE
BOOL WriteFileZT(HANDLE fileh, const WCHAR *str, DWORD *wrote)
{
	char bufA[WriteFileZTSIZE];

	UnicodeToAnsi(str, bufA, WriteFileZTSIZE);
	return WriteFile(fileh, bufA, strlen32(bufA), wrote, NULL);
}

// アラインメントがあっていないテキストをリストやコンボボックスに登録
LRESULT SendUTextMessage_U(HWND hWnd, UINT uMsg, WPARAM wParam, const TCHAR *text)
{
	TCHAR buf[0x1000];

	if ( !((DWORD)(DWORD_PTR)text & 1) ){
		return SendMessage(hWnd, uMsg, wParam, (LPARAM)text);
	}else {  // アライメントがずれている
		tstplimcpy(buf, text, 0x1000);
		return SendMessage(hWnd, uMsg, wParam, (LPARAM)buf);
	}
}
#endif

void GetPopMenuPos(HWND hWnd, POINT *pos, WORD key)
{
	RECT box;

	GetMessagePosPoint(*pos);
	if ( key & K_mouse ) return;
	GetWindowRect(hWnd, &box);
	if ( (box.left > pos->x) || (box.top > pos->y) ||
		 (box.right < pos->x) || (box.bottom < pos->y) ){
		pos->x = (box.left + box.right) / 2;
		pos->y = (box.top + box.bottom) / 2;
	}
}

int GetNowTime(TCHAR *text, int mode)
{
	SYSTEMTIME nowTime;

	GetLocalTime(&nowTime);
	return thprintf(text, 64,
			(mode == 0) ?	T("%04d-%02d-%02d %02d:%02d:%02d") :
							T("%04d-%02d-%02d"),
			nowTime.wYear, nowTime.wMonth, nowTime.wDay,
			nowTime.wHour, nowTime.wMinute, nowTime.wSecond) - text;
}

BOOL PPRecvExecuteByWMCopyData(PPXAPPINFO *info, COPYDATASTRUCT *copydata)
{
	TCHAR cmd[CMDLINESIZE], *cmdbuf;

	if ( copydata->cbData >= 0x7fffffff ) return FALSE;
	if ( copydata->cbData > TSTROFF(CMDLINESIZE) ){
		cmdbuf = HeapAlloc(GetProcessHeap(), 0, copydata->cbData);
		if ( cmdbuf == NULL ) return FALSE;
	}else{
		cmdbuf = cmd;
	}
	memcpy(cmdbuf, copydata->lpData, copydata->cbData);
	if ( LOWORD(copydata->dwData) == 'H' ) ReplyMessage(TRUE);
	PP_ExtractMacro(info->hWnd, info, NULL, cmdbuf, NULL, 0);
	if ( cmdbuf != cmd ) HeapFree(GetProcessHeap(), 0, cmdbuf);
	return TRUE;
}

BOOL PPWmCopyData(PPXAPPINFO *info, COPYDATASTRUCT *copydata)
{
	if ( LOWORD(copydata->dwData) == 'H' ){ // コマンド実行(非同期)
		return PPRecvExecuteByWMCopyData(info, copydata);
	}
	return FALSE;
}

void RemoveCharKey(HWND hWnd)
{
	MSG msgdata;

	PeekMessage(&msgdata, hWnd, WM_CHAR, WM_CHAR, PM_REMOVE);
}

BOOL PeekMessageLoop(HWND hWnd)
{
	MSG msgdata;

	while ( PeekMessage(&msgdata, hWnd, 0, 0, PM_REMOVE) ){
		if ( msgdata.message == WM_QUIT ) return FALSE;
		if ( DialogKeyProc(&msgdata) ) continue;
		TranslateMessage(&msgdata);
		DispatchMessage(&msgdata);
		if ( (msgdata.message == WM_KEYDOWN) && ((int)msgdata.wParam == VK_PAUSE) ){
			return FALSE; // [Pause] で強制中断
		}
	}
	return TRUE;
}

BOOL PeekDialogMessageLoop(HWND hWnd, HWND hDlg)
{
	MSG msgdata;

	while ( PeekMessage(&msgdata, hWnd, 0, 0, PM_REMOVE) ){
		if ( msgdata.message == WM_QUIT ) return FALSE;
		if ( (hDlg != NULL) && IsDialogMessage(hDlg, &msgdata) ) continue;
		if ( DialogKeyProc(&msgdata) ) continue;
		TranslateMessage(&msgdata);
		DispatchMessage(&msgdata);
		if ( (msgdata.message == WM_KEYDOWN) && ((int)msgdata.wParam == VK_PAUSE) ){
			return FALSE; // [Pause] で強制中断
		}
	}
	return TRUE;
}

const TCHAR DummyClassName[] = T("UnarcTempClass");
HWND CreateDummyWindow(HWND hDisplayWnd, const TCHAR *title)
{
	HWND hWnd;
	RECT box;

	if ( DLLWndClass.item.Dummy == 0 ){
		WNDCLASS UnarcTempClass = {
			0, NULL, 0, 0,		// style～cbWndExtra
			NULL, NULL, NULL, NULL, NULL,	// hInstance～lpszMenuName
			DummyClassName			// lpszClassName
		};
		UnarcTempClass.lpfnWndProc = DefWindowProc;
		UnarcTempClass.hInstance = DLLhInst;
		UnarcTempClass.hIcon = LoadIcon(DLLhInst, MAKEINTRESOURCE(Ic_FOP));
		DLLWndClass.item.Dummy = RegisterClass(&UnarcTempClass);
	}

	box.left = box.top = 0;
	if ( hDisplayWnd != NULL ) GetWindowRect(hDisplayWnd, &box);

	hWnd = CreateWindowEx(0, (LPCTSTR)DLLWndClass.item.Dummy, title,
			WS_POPUP, box.left, box.top, 0, 0, NULL, NULL, DLLhInst, NULL);
	if ( hWnd != NULL ){
		ShowWindow(hWnd, (hDisplayWnd == NULL) ? SW_SHOW : SW_SHOWMINNOACTIVE);
		PeekMessageLoop(hWnd);
	}
	return hWnd;
}

#pragma argsused
PPXDLL int WINAPI run(HWND hWnd, HINSTANCE hInstance, char *lpCmdLine, int nShowCmd)
{
	#ifdef UNICODE
		WCHAR CmdLineW[0x8000];
		UnUsedParam(hInstance); UnUsedParam(nShowCmd);

		AnsiToUnicode(lpCmdLine, CmdLineW, 0x8000);
		return (int)PP_ExtractMacro(hWnd, NULL, NULL, CmdLineW, NULL, 0);
	#else
		UnUsedParam(hInstance); UnUsedParam(nShowCmd);

		return (int)PP_ExtractMacro(hWnd, NULL, NULL, lpCmdLine, NULL, 0);
	#endif
}

#if 0
#if _WIN32_WINNT < 0x0403
 #include <winable.h>
#endif
DefineWinAPI(UINT, SendInput, (UINT, INPUT *, int)) = NULL;
DefineWinAPI(VOID, keybd_event, (BYTE, BYTE, DWORD, DWORD)) = NULL;
DefineWinAPI(VOID, mouse_event, (DWORD, DWORD, DWORD, DWORD, DWORD)) = NULL;
#define xKEYEVENTF_KEYUPDOWN B8 // 押す・離すを生成させる

UINT SendKeyboard(DWORD typeflags, WORD VirtualKey)
{
	if ( OSver.dwMajorVersion > 4 ){
		INPUT keyinput[2];
		int keys;

		if ( DSendInput == NULL ){
			HMODULE hUser32;

			hUser32 = GetModuleHandleA(User32DLL);
			GETDLLPROC(hUser32, SendInput);
			if ( DSendInput == NULL ) return 0;
		}
		keyinput[0].type = INPUT_KEYBOARD;
		keyinput[0].ki.wVk = VirtualKey;
		keyinput[0].ki.wScan = 0;
		keyinput[0].ki.dwFlags = typeflags & ~xKEYEVENTF_KEYUPDOWN;
		keyinput[0].ki.time = 0;
		keyinput[0].ki.dwExtraInfo = 0;
		if ( typeflags & xKEYEVENTF_KEYUPDOWN ){
			keys = 2;
			keyinput[1] = keyinput[0];
			setflag(keyinput[1].ki.dwFlags, KEYEVENTF_KEYUP);
		}else{
			keys = 1;
		}
		return DSendInput(keys, keyinput, sizeof(INPUT));
	}
	// Win95 には SendInput がない
	if ( Dkeybd_event == NULL ){
		HMODULE hUser32;

		hUser32 = GetModuleHandleA(User32DLL);
		GETDLLPROC(hUser32, keybd_event);
		if ( Dkeybd_event == NULL ) return 0;
	}
	Dkeybd_event((BYTE)VirtualKey, 0, typeflags & ~xKEYEVENTF_KEYUPDOWN, 0);
	if ( typeflags & xKEYEVENTF_KEYUPDOWN ){
		Dkeybd_event((BYTE)VirtualKey, 0, (typeflags & ~xKEYEVENTF_KEYUPDOWN) | KEYEVENTF_KEYUP, 0);
	}
	return 1;
}
#endif
#if 0
DefineWinAPI(BOOL, SetWindowSubclass, (HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR)) = NULL;
DefineWinAPI(BOOL, RemoveWindowSubclass, (HWND, SUBCLASSPROC, UINT_PTR)) = NULL;
DefineWinAPI(LRESULT, DefSubclassProc, (HWND, UINT, SUBCLASSPROC, UINT_PTR)) = NULL;

typedef struct {
	WNDPROC OldProc;
	SUBCLASSPROC Subclass;
	DWORD_PTR RefData;
	TCHAR propname[16];
} OldSubclassRefData;

LRESULT CALLBACK *OldSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 OldProc = LRESULT OldSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
}

LRESULT OldSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	OldSubclassRefData *osrd;
	GetProp(hWnd, name, (HANDLE)osrd);
}

BOOL SetSubclass(HWND hWnd, SUBCLASSPROC Subclass, DWORD_PTR RefData)
{
	if ( DRemoveWindowSubclass == NULL ){
		if ( LoadCommonControls(0) == NULL ){
			DRemoveWindowSubclass = (impRemoveWindowSubclass)-1;
		}else {
			GETDLLPROC(hComctl32, SetWindowSubclass);
			GETDLLPROC(hComctl32, RemoveWindowSubclass);
			GETDLLPROC(hComctl32, DefSubclassProc);
			if ( DSetWindowSubclass == NULL ){
				DRemoveWindowSubclass = (impRemoveWindowSubclass)-1;
			}
		}
	}
	if ( DSetWindowSubclass != NULL ){ // SetWindowSubclass 使用
		return DSetWindowSubclass(hWnd, Subclass, (UINT_PTR)SubclassID, RefData);
	}else{ // SetWindowLongPtr 使用 2000以前
		OldSubclassRefData *osrd;

		osrd = HeapAlloc(ProcHeap, 0, sizeof(OldSubclassRefData));
		if ( osrd == FALSE ) return FALSE;
		thprintf(osrd->propname, TSIZEOF(osrd->propname), T("S%d"), Subclass);
		SetProp(hWnd, osrd->propname, (HANDLE)osrd);
		osrd->RefData = RefData;
		osrd->Subclass = Subclass;
		osrd->OldProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)OldSubclassProc);
		return TRUE;
	}
//		HeapFree(ProcHeap, 0, srcW);
}
#endif
