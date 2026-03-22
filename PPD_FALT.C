/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library						内蔵デバッガ？
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include <imagehlp.h>
#include <tlhelp32.h>
#include "PPXVER.H"
#include "PPX.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#include "PPX_64.H"
#pragma hdrstop

#define FAULTTEST 0

volatile int EnableExitState = 0; // スレッドが残っているときの強制終了カウンタ

#ifdef UNICODE
	#define IsSjisLocale() (LOWORD(GetUserDefaultLCID()) == LCID_JAPANESE)
#else
	#define IsSjisLocale() ((LOWORD(GetUserDefaultLCID()) == LCID_JAPANESE) && (GetACP() == CP__SJIS))
#endif

#define AppName T("PPx")
#define AppExeName T("PP")

#ifdef _M_ARM
	#define MSGBIT T("ARM")
	#define REGPREFIX(reg) reg
	#define CHECK_HOOK "hook"
	#define CHECK_LoadDLL "LoadDLL"

	#define CPUTYPE IMAGE_FILE_MACHINE_ARM
	#define LOADWINAPI164(name) {(void (WINAPI **)())&(D ## name), #name}
	#define TAIL64(name) name
	#ifdef UNICODE
		#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "W"}
		#define TAIL64T(name) name ## W
	#else
		#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name}
		#define TAIL64T(name) name
	#endif
#endif

#ifdef _M_ARM64
	#define MSGBIT T("ARM64")
	#define REGPREFIX(reg) reg
	#define CHECK_HOOK "hook64"
	#define CHECK_LoadDLL "LoadDLL64"

	#define CPUTYPE IMAGE_FILE_MACHINE_ARM64
	#define LOADWINAPI164(name) {(void (WINAPI **)())&(D ## name), #name}
	#define TAIL64(name) name
	#ifdef UNICODE
		#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "W"}
		#define TAIL64T(name) name ## W
	#else
		#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name}
		#define TAIL64T(name) name
	#endif
#endif

#ifndef CPUTYPE
	#ifdef _WIN64
		#define MSGBIT T("x64")
		#define REGPREFIX(reg) R ## reg
		#define CHECK_HOOK "hookx64"
		#define CHECK_LoadDLL "LoadDLL64"

		#define CPUTYPE IMAGE_FILE_MACHINE_AMD64
		#define LOADWINAPI164(name) {(void (WINAPI **)())&(D ## name), #name "64"}
		#define TAIL64(name) name ## 64
		#ifdef UNICODE
			#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "W64"}
			#define TAIL64T(name) name ## W64
		#else
			#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "64"}
			#define TAIL64T(name) name ## 64
		#endif
	#else
		#define MSGBIT
		#define REGPREFIX(reg) E ## reg
		#define CHECK_HOOK "hook"
		#define CHECK_LoadDLL "LoadDLL"

		#define CPUTYPE IMAGE_FILE_MACHINE_I386
		#define LOADWINAPI164(name) {(void (WINAPI **)())&(D ## name), #name}
		#define TAIL64(name) name
		#ifdef UNICODE
			#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "W"}
			#define TAIL64T(name) name ## W
		#else
			#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name}
			#define TAIL64T(name) name
		#endif
	#endif
#endif

#ifdef UNICODE
	#define GETDLLPROCTA(handle, name) D ## name = (imp ## name)GetProcAddress(handle, #name "W");

	typedef struct {
		DWORD dwSize;
		DWORD th32ModuleID;
		DWORD th32ProcessID;
		DWORD GlblcntUsage;
		DWORD ProccntUsage;
		BYTE *modBaseAddr;
		DWORD modBaseSize;
		HMODULE hModule;
		WCHAR szModule[MAX_MODULE_NAME32 + 1];
		WCHAR szExePath[MAX_PATH];
	} MODULEENTRY32X;
#else
	#define GETDLLPROCTA(handle, name) D ## name = (imp ## name)GetProcAddress(handle, #name);
	#define MODULEENTRY32X MODULEENTRY32
#endif

#ifdef WINEGCC
	#define EXTTYPE T("Wine")
#else
	#ifdef USEDIRECTX
		#ifdef USEDIRECTWRITE
			#define EXTTYPE T("Dw")
		#else
			#define EXTTYPE T("D9")
		#endif
	#else
		#define EXTTYPE
	#endif
#endif

#ifdef UNICODE
	#define MSGVER	T(FileProp_Version) EXTTYPE L" UNICODE " MSGBIT L"(" T(__DATE__) L")"
	#define MSGCTYPE T("U") MSGBIT EXTTYPE
#else
	#define MSGVER	FileProp_Version EXTTYPE " Multibyte(" T(__DATE__) ")"
	#define MSGCTYPE "M" EXTTYPE
#endif

#ifndef EXCEPTION_IN_PAGE_ERROR
	#define EXCEPTION_IN_PAGE_ERROR 0xC0000006
#endif

#define STACKWALKSIZE 0x3000

const TCHAR msgtitle[] = T("PPx Internal Error");
const TCHAR msgexfthread[] = T("ExceptionFilter");
const TCHAR NoItem[] = T("-");
const TCHAR StrJScr[] = T("usejs9");
const TCHAR StrJScrFix[] = T("1");

//------------------------------------------------------------ 例外表示テーブル
#define msgstr_fault 0
#define msgstr_except 1
#define msgstr_unknownthread 2
#define msgstr_memaddress 3
#define msgstr_faultread 4
#define msgstr_faultwrite 5
#define msgstr_div0 6
#define msgstr_stackflow 7
#define msgstr_brokencust 8
#define msgstr_sendreport 9
#define msgstr_foundprom 10
#define msgstr_deepprom 11
#define msgstr_faultsend 12
#define msgstr_faultDEP 13
#define msgstr_FPU 14

const TCHAR *msgstringsJ[] = {
	T("%s で、%s異常の対処ができなかったため、終了します。\n")
	T("  詳細\tWin%s,") MSGVER T("\n%s\n%s%s"), // 0 msgstr_fault

	T("例外 %XH"), // 1 msgstr_except
	T("不明"), // 2 msgstr_unknownthread
	T("メモリ(%1pH)の%s失敗"), // 3 msgstr_memaddress
	T("読込"), // 4 msgstr_faultread
	T("書込"), // 5 msgstr_faultwrite
	T("0除算"), // 6 msgstr_div0
	T("スタック"), // 7 msgstr_stackflow
	T("\n\n※カスタマイズファイルが破損したかもしれません。\nPPcustを起動すると破損チェックが行われます。"), // 8 msgstr_brokencust
	T("「はい」で、ブラウザを開いて前記内容のみを作者に送信します。\n")
	T("「キャンセル」で、続行します（通常は終了します）。\n")
	T("対策できるかもしれませんので、できれば送信をお願いします。"), // 9 msgstr_sendreport
	T("次の問題を検出しました。継続して使用できます。\n")
	T("  詳細\tWin%s,") MSGVER T("\n\t%s\n\n%s"), // 10 msgstr_foundprom
	T("\n\n※異常報告中の異常再発生、メモリ不足、IMAGEHLP.DLLが使えない等の深刻な問題が起きていると思われます。"), // 11 msgstr_deepprom
	T("送信に失敗しました。"), // 12 msgstr_faultsend
	T("DEP"), // 13 msgstr_faultDEP
	T("浮動小数点演算"), // 14 msgstr_FPU
};

const TCHAR *msgstringsE[] = {
	T("Detected error in %s thread.\n")
	T("Terminating PPx by %s error.\n")
	T(" detail\tWin%s,") MSGVER T("\n%s\n%s%s"), // 0

	T("Exception %XH"), // 1
	T("Unknown"), // 2
	T("%1pH %s memory access"), // 3
	T("Read"), // 4
	T("Write"), // 5
	T("Divide by zero"), // 6
	T("Stack"), // 7
	T("\n\nSeem Customize data collapse."), // 8
	T("When you select 'yes',this report is sent to TORO."), // 9
	T("Detected probrem. It can be used continuously.\n")
	T(" detail\tWin%s,") MSGVER T("\n\t%s\n\n%s"), // 10
	T("\n\n*It seems that a serious problem such as memory shortage and IMAGEHLP.DLL has occurred. "), // 11 msgstr_deepprom
	T("send fault."), // 12
	T("DEP"), // 13
	T("Floating point operation"), // 14 msgstr_FPU
};


#if defined(UNICODE) && !defined(CBA_READ_MEMORY) && !defined(WINEGCC)
typedef struct {
	DWORD SizeOfStruct, BaseOfImage, ImageSize, TimeDateStamp, CheckSum, NumSyms;
	SYM_TYPE SymType;
	WCHAR ModuleName[32], ImageName[256], LoadedImageName[256];
} IMAGEHLP_MODULEW, *PIMAGEHLP_MODULEW;
#endif
DefineWinAPI(BOOL, StackWalk, (DWORD, HANDLE, HANDLE, TAIL64(LPSTACKFRAME),
	PVOID, TAIL64(PREAD_PROCESS_MEMORY_ROUTINE),
	TAIL64(PFUNCTION_TABLE_ACCESS_ROUTINE), TAIL64(PGET_MODULE_BASE_ROUTINE),
	TAIL64(PTRANSLATE_ADDRESS_ROUTINE))
) = NULL;

DefineWinAPI(BOOL, SymInitialize, (HANDLE, PCSTR, BOOL)) = NULL;
DefineWinAPI(PVOID, SymFunctionTableAccess, (HANDLE, DWORD_PTR));
DefineWinAPI(BOOL, SymGetSymFromAddr,
	(HANDLE, DWORD_PTR, DWORD_PTR *, TAIL64(IMAGEHLP_SYMBOL) *));
DefineWinAPI(BOOL, SymCleanup, (HANDLE));

DefineWinAPI(DWORD_PTR, SymGetModuleBase, (HANDLE, DWORD_PTR));
DefineWinAPI(BOOL, SymGetModuleInfo, (HANDLE, DWORD_PTR, TAIL64T(IMAGEHLP_MODULE) *)) = NULL;

LOADWINAPISTRUCT IMAGEHLPDLL[] = {
	LOADWINAPI164(StackWalk),
	LOADWINAPI1(SymInitialize),
	LOADWINAPI164(SymFunctionTableAccess),
	LOADWINAPI164(SymGetModuleBase),
	LOADWINAPI164(SymGetSymFromAddr),
	LOADWINAPI1(SymCleanup),
	LOADWINAPI164TW(SymGetModuleInfo),
	{NULL, NULL}
};

#define defThreadQuerySetWin32StartAddress 9

int ShowErrorDialog(const TCHAR **Msg, int msgtype, const TCHAR *threadtext, const TCHAR *infotext, const TCHAR *addrtext, const TCHAR *comment);


typedef struct {
	char *heap;
	char *last;
	DWORD heapsize, blocksize;
} HeapBlock;

#define HeapBlockInitValue {NULL, NULL, 0, 0}

char *InitHeapBlock(HeapBlock *hblock, size_t block_size, int first_blocks)
{
	if ( hblock->heap == NULL ){
		hblock->blocksize = (DWORD)block_size;
		hblock->heapsize = first_blocks * hblock->blocksize;
		hblock->heap = hblock->last = HeapAlloc(DLLheap, 0, hblock->heapsize);
	}
	return hblock->heap;
}

char *HeapNewBlock(HeapBlock *hblock, DWORD add_blocks)
{
	char *newheap, *newblock;
	DWORD nowsize, requestsize, allocsize;

	allocsize = nowsize = hblock->last - hblock->heap;
	requestsize = nowsize + hblock->blocksize * add_blocks;
	if ( requestsize > hblock->heapsize ){
		for (;;){
			nowsize *= 2;
			if ( requestsize <= nowsize ) break;
		}
		newheap = HeapReAlloc( DLLheap, 0, hblock->heap, nowsize );
		if ( newheap == NULL ) return NULL;
		hblock->heapsize = nowsize;
		hblock->last = newheap + allocsize;
		hblock->heap = newheap;
	}
	newblock = hblock->last;
	hblock->last += hblock->blocksize * add_blocks;
	return newblock;
}
#define FreeHeapBlock(hblock) HeapFree(DLLheap, 0, (hblock)->heap)

typedef struct {
	THREADSTRUCT *thread;
	DWORD id;
} ThreadListStruct;

HeapBlock ThreadList = HeapBlockInitValue;

// 指定スレッドを検索 ---------------------------------------------------------
THREADSTRUCT *GetThreadInfoFromID(DWORD ThreadID)
{
	ThreadListStruct *list;

	list = (ThreadListStruct *)ThreadList.heap;
	if ( list == NULL ) return NULL;
	for (;;){
		if ( list >= (ThreadListStruct *)ThreadList.last ) return NULL;
		if ( list->id == ThreadID ){
			if ( IsTrue(IsBadReadPtr(list->thread, 1)) ){
				list->id = 0; // スレッドが消滅していた
				list->thread = NULL;
			}
			return list->thread;
		}
		list++;
	}
}

THREADSTRUCT *GetCurrentThreadInfo(void)
{
	THREADSTRUCT *result;

	EnterCriticalSection(&ThreadSection);
	result = GetThreadInfoFromID(GetCurrentThreadId());
	LeaveCriticalSection(&ThreadSection);
	return result;
}

PPXDLL BOOL PPXAPI PPxWaitExitThread(void)
{
	DWORD CurrentThreadID, HungThreadID = 0;
	#define ExitWaitInterval 200

	CurrentThreadID = GetCurrentThreadId();
	for (;;){
		ThreadListStruct *list;
		DWORD CheckThreadID;

		EnterCriticalSection(&ThreadSection);
		list = (ThreadListStruct *)ThreadList.heap;
		if ( list == NULL ) break; // 登録無し→終了可能
//		XMessage(NULL, NULL, XM_DbgLOG, T("PPxWaitExitThread %x %x %d"),ThreadList.heap,ThreadList.last, (ThreadList.last-ThreadList.heap)/ThreadList.blocksize);

		for ( ;; ){
//			XMessage(NULL, NULL, XM_DbgLOG, T("PPxWaitExitThread[1] %d %d/%d"), list,((char*)list-ThreadList.heap) / ThreadList.blocksize,ThreadList.heapsize / ThreadList.blocksize);

			if ( list >= (ThreadListStruct *)ThreadList.last ){
				// 最後まで到達→終了可能
				LeaveCriticalSection(&ThreadSection);
				return TRUE;
			}

			CheckThreadID = list->id;
			if ( CheckThreadID != 0 ){
				// 異常スレッドか
				if ( CheckThreadID == HungThreadID ){
					list->id = 0; // 監視から外す
				}else{ // 通常スレッド
					// 強制終了を許可していないスレッドがあるか
					if ( (CheckThreadID != CurrentThreadID) &&
						 !(list->thread->flag & XTHREAD_EXITENABLE) ){
						break;
					}
				}
			}
			list++;
		}
		LeaveCriticalSection(&ThreadSection);

		// CheckThreadID が終了するのを待つ
		if ( DOpenThread == INVALID_HANDLE_VALUE ){
			GETDLLPROC(hKernel32, OpenThread);
		}

		PeekMessageLoop(NULL);

		if ( DOpenThread == NULL ){
			Sleep(ExitWaitInterval);
		}else{
			HANDLE hThread;

			hThread = DOpenThread(SYNCHRONIZE, FALSE, CheckThreadID);
			if ( hThread != NULL ){
				DWORD result = MsgWaitForMultipleObjects(1, &hThread, FALSE, ExitWaitInterval, QS_ALLEVENTS);

				CloseHandle(hThread);
				if ( result == (WAIT_OBJECT_0 + 1) ) continue; // Message
				if ( result != WAIT_TIMEOUT ) hThread = NULL;
			}	// スレッドを開けない→強制終了と見なす
			#pragma warning(suppress:6001)
			if ( hThread == NULL ){
				list->id = 0;
				HungThreadID = CheckThreadID;
				if ( IsBadReadPtr(list->thread, sizeof(THREADSTRUCT)) == FALSE ){
					setflag(list->thread->flag, XTHREAD_EXITENABLE);
				}
			}
		}
		// 終了可能(K_ENABLEEXIT)なとき、一定期間終了後に強制終了
		if ( EnableExitState > 0 ){
			if ( EnableExitState == 1 ){
				XMessage(NULL, NULL, XM_DbgLOG, T("Count EnableExit"));
			}
			EnableExitState++;
			if ( EnableExitState > (5000 / ExitWaitInterval) ){
				XMessage(NULL, NULL, XM_DbgLOG, T("Force EnableExit %d"), EnableExitState);
				return TRUE;
			}
		}
	}
	LeaveCriticalSection(&ThreadSection);
	return TRUE;
}

// 処理中のスレッドを登録 -----------------------------------------------------
// ●PPxWaitExitThread の直前に位置すること
PPXDLL BOOL PPXAPI PPxRegisterThread(THREADSTRUCT *threadstruct)
{
	ThreadListStruct *list;

	if ( threadstruct == NULL ){ // NULL...スレッド登録済みかを確認する
		return (GetCurrentThreadInfo() != NULL);
	}

#if !defined(_WIN64) && !defined(_M_ARM) && USETASM32
	if ( OSver.dwMajorVersion >= 5 ) InitFloat();
#endif

	threadstruct->next = NULL;
	threadstruct->ThreadID = GetCurrentThreadId();
	threadstruct->PPxNo = -1;

	EnterCriticalSection(&ThreadSection);
	InitHeapBlock(&ThreadList, sizeof(ThreadListStruct), 8); // PPc 2pane(2x2) + async 2 = 6threads
	list = (ThreadListStruct *)ThreadList.heap;
	if ( list != NULL ) for (;;){ // 空きがあれば登録する
		if ( list >= (ThreadListStruct *)ThreadList.last ){
			list = (ThreadListStruct *)HeapNewBlock(&ThreadList, 1);
			if ( list != NULL ){ // 追加
//				XMessage(NULL, NULL, XM_DbgLOG, T("RegThread add %x  %d/%d %s"),list,((char*)list-ThreadList.heap)/ThreadList.blocksize,ThreadList.heapsize/ThreadList.blocksize,threadstruct->ThreadName);
				list->thread = threadstruct;
				list->id = threadstruct->ThreadID;
			}
			break;
		}
		if ( list->id == 0 ){
//				XMessage(NULL, NULL, XM_DbgLOG, T("RegThread reuse %d/%d %s"),((char*)list-ThreadList.heap)/ThreadList.blocksize,ThreadList.heapsize/ThreadList.blocksize,threadstruct->ThreadName);
			list->thread = threadstruct;
			list->id = threadstruct->ThreadID;
			break;
		}
		list++;
	}
	LeaveCriticalSection(&ThreadSection);
	return TRUE;
}

void ReportExitThread(void)
{
	PPXMODULEPARAM pmp = {NULL};

	CallModule(&DummyPPxAppInfo, PPXMEVENT_CLOSETHREAD, pmp, NULL);
}

// 処理中のスレッドを登録を抹消する -------------------------------------------
DWORD UnRegisterThread(DWORD ThreadID)
{
	ThreadListStruct *list;
	DWORD flag = 0;

	EnterCriticalSection(&ThreadSection);
	list = (ThreadListStruct *)ThreadList.heap;
	if ( list != NULL ) for (;;){
		if ( list >= (ThreadListStruct *)ThreadList.last ) break;
		if ( list->id == ThreadID ){
			if ( !IsBadReadPtr(list->thread, 16) ) flag = list->thread->flag;
			list->id = 0;
			break;
		}
		list++;
	}
	LeaveCriticalSection(&ThreadSection);
	return flag;
}

PPXDLL BOOL PPXAPI PPxUnRegisterThread(void)
{
	DWORD ThreadID;

	ThreadID = GetCurrentThreadId();
	if ( UnRegisterThread(ThreadID) & XTHREAD_REPORTTHREAD ){
		ReportExitThread();
	}

//	UsePPx(); // 自スレッドによる自スレッド情報の変更なので省略
	// Job が登録されていたら削除する
	{
		int i;

		for ( i = 0 ; i < X_MaxJob ; i++ ){
			if ( Sm->Job[i].ThreadID == ThreadID ) Sm->Job[i].ThreadID = 0;
		}
	}
//	FreePPx();
	return TRUE;
}

const TCHAR *GetThreadName(DWORD id)
{
	THREADSTRUCT *pT;

	pT = GetThreadInfoFromID(id);
	if ( pT != NULL ) return pT->ThreadName;
	return T("???");
}

PPXDLL void * PPXAPI PPxThreadInfo(DWORD id, DWORD mode)
{
	THREADSTRUCT *pT;

	if ( id == 0 ) id = GetCurrentThreadId();

	EnterCriticalSection(&ThreadSection);
	pT = GetThreadInfoFromID(id);
	if ( (pT == NULL) || (mode == XTHREADINFO_GETSTRUCT) ){
		LeaveCriticalSection(&ThreadSection);
		return (void *)pT;
	}

	if ( mode == XTHREADINFO_GETNAME ){
		LeaveCriticalSection(&ThreadSection);
		return (void *)pT->ThreadName;
	}

	if ( mode >= XTHREADINFO_GETFLAGS ){
		void *result;

		if ( mode >= XTHREADINFO_SETFLAGS ){
			setflag(pT->flag, 1 << (mode - XTHREADINFO_SETFLAGS));
			LeaveCriticalSection(&ThreadSection);
			return (void *)pT;
		} // XTHREADINFO_GETFLAGS
		result = (pT->flag & (1 << (mode - XTHREADINFO_GETFLAGS))) ? (void *)2 : (void *)1;
		LeaveCriticalSection(&ThreadSection);
		return result;
	}

	LeaveCriticalSection(&ThreadSection);
	return NULL;
}

#define WSSIZE 1024
void QueryEncode(TCHAR *dst)
{
	char bufA[CMDLINESIZE], *srcA;
	TCHAR *dest, *maxptr;

#ifdef UNICODE
	WideCharToMultiByteU8(CP_UTF8, 0, dst, -1, bufA, CMDLINESIZE, NULL, NULL);
#else
	WCHAR bufW[CMDLINESIZE];

	AnsiToUnicode(dst, bufW, CMDLINESIZE);
	WideCharToMultiByteU8(CP_UTF8, 0, bufW, -1, bufA, CMDLINESIZE, NULL, NULL);
#endif
	srcA = bufA;
	dest = dst;
	maxptr = dest + WSSIZE - 100;
	while ( dest < maxptr ){
		BYTE c;

		c = (BYTE)*srcA++;
		if ( c == '\0' ) break;
		if ( IsalnumA(c) || (c == '=') ){
			*dest++ = c;
		}else if ( c == '\1' ){
			*dest++ = '&';
		}else if ( c == ' ' ){
			*dest++ = '+';
		}else{
			dest = thprintf(dest, 8, T("%%%02X"), c);
		}
	}
	*dest = '\0';
}

// EnumCustTable のとき、enumcustcache を使って正当性を確認
const TCHAR *FixCustTable(void)
{
	BYTE *ptr, *maxptr;
	DWORD DataHeaderSize;
	WORD TableDataSize;
	TCHAR *DataName, *DataNamePtr;

	if ( enumcustcache.Counter != Sm->CustomizeWrite ) return NilStr;
	enumcustcache.Counter = Sm->CustomizeWrite - 1; // キャッシュを無効に

	ptr = enumcustcache.DataPtr;
	maxptr = CustP + X_Csize - CUST_FOOTER_SIZE;
	// Data ヘッダのチェック
	if ( (ptr <= CustP) || (ptr >= maxptr) ) return T("Hdr");
	DataHeaderSize = *(DWORD *)ptr;
	if ( DataHeaderSize < (sizeof(DWORD) + sizeof(TCHAR) + CUST_TABLE_FOOTER_SIZE) ){
		*(CUST_NEXTDATA_OF *)ptr = 0;
		 return T("Hdr");
	}
	if ( ((ptr + DataHeaderSize) > maxptr) ){
		DataHeaderSize = (DWORD)(maxptr - ptr);
		if ( DataHeaderSize < (sizeof(DWORD) + sizeof(TCHAR) + CUST_TABLE_FOOTER_SIZE) ){
			*(CUST_NEXTDATA_OF *)ptr = 0;
			 return T("Hdr");
		}
		*(CUST_NEXTDATA_OF *)ptr = DataHeaderSize;
		*(CUST_NEXTDATA_OF *)maxptr = 0;
	}

	DataNamePtr = DataName = (TCHAR *)(ptr + CUST_NEXTDATA_SIZEOF);
	// Data name のチェック
	for ( ;; ){
		if ( (BYTE *)DataNamePtr >= maxptr ) return T("HdrName");
		if ( *DataNamePtr == '\0' ) break;
		DataNamePtr++;
	}
	// Table のチェック
	maxptr = ptr + DataHeaderSize - CUST_TABLE_FOOTER_SIZE;
	ptr = (BYTE *)(DataNamePtr + 1);
	for ( ;; ){
		TCHAR *TableNamePtr;
		// Table ヘッダのチェック
		TableDataSize = *(CUST_NEXTTABLE_OF *)ptr;
		if ( TableDataSize == 0 ) break;
		if ( (ptr + TableDataSize) > maxptr ){
			*(CUST_NEXTTABLE_OF *)ptr = 0;
			return DataName;
		}
		// Table name のチェック
		TableNamePtr = (TCHAR *)(ptr + CUST_NEXTTABLE_SIZEOF);
		for ( ;; ){
			if ( (BYTE *)TableNamePtr >= maxptr ){
				*(CUST_NEXTTABLE_OF *)ptr = 0;
				return DataName;
			}
			if ( *TableNamePtr == '\0' ) break;
			TableNamePtr++;
		}
		ptr += TableDataSize;
	}
	return T("no error");
}

// address ( modulename [basename].funcname + offset 形式を出力する
void GetFailedModuleAddr(HANDLE hProcess, TAIL64(IMAGEHLP_SYMBOL) *syminfo, TCHAR *dest, void *addr)
{
	TAIL64T(IMAGEHLP_MODULE) mdlinfo;
	DWORD_PTR funcoffset;

	// address
	dest = thprintf(dest, 64, T("%1p"), addr);
	mdlinfo.SizeOfStruct = sizeof(mdlinfo);
	// modulename, basename
	if ( (DSymGetModuleInfo != NULL) && IsTrue(DSymGetModuleInfo(
			GetCurrentProcess(), (DWORD_PTR)addr, &mdlinfo)) ){
		dest = thprintf(dest, 256, T("(%s:%1p"),
				mdlinfo.ModuleName, mdlinfo.BaseOfImage);
	}else{
		if ( ((DWORD_PTR)addr >= (DWORD_PTR)DLLhInst) &&
			 ((DWORD_PTR)addr < ((DWORD_PTR)DLLhInst + 0xb0000)) ){
			dest = thprintf(dest, 256, T("(PPLIB:%1p"), DLLhInst);
		}
	}
	// funcname, offset
	if ( (DSymGetSymFromAddr != NULL) && IsTrue(DSymGetSymFromAddr(
			hProcess, (DWORD_PTR)addr, &funcoffset, syminfo)) ){
		dest = thprintf(dest, 256, T(".%hs+%1p"), syminfo->Name, funcoffset);
	}
	*dest++ = ')';
	*dest = '\0';
}

//-------------------------------------------------------------------- 例外処理
#define SHOWSTACKS 3 // 標準のスタック表示数
#define SHOWSTACKSBUFSIZE 0x200

typedef struct {
	EXCEPTION_POINTERS *ExceptionInfo;
	THREADSTRUCT *FaultThread;
	HANDLE hThread;
	DWORD ThreadID;
	volatile DWORD Result;
} ExceptionData;

volatile DWORD PPxUnhandledExceptionFilterThreadID = 0; // 例外処理中はそのスレッドID
volatile DWORD PPxUnhandledExceptionFilterOld_Code;
volatile PVOID PPxUnhandledExceptionFilterOld_Address;
volatile ULONG_PTR PPxUnhandledExceptionFilterOld_Info1;

void GetExceptionCodeText(EXCEPTION_RECORD *ExceptionRecord, const TCHAR **Msg, TCHAR *exceptionbuf, const TCHAR **Comment)
{
	if ( (ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) ||
		 (ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) ){
		DWORD rwmode = msgstr_faultread;
		DWORD rwinfo = (DWORD)ExceptionRecord->ExceptionInformation[0];

		if ( rwinfo ){
			rwmode = (rwinfo == 8) ? msgstr_faultDEP : msgstr_faultwrite;
		}
		thprintf(exceptionbuf, CMDLINESIZE, Msg[msgstr_memaddress],
				ExceptionRecord->ExceptionInformation[1], Msg[rwmode]);
		if ( (Comment != NULL) &&
			 (ExceptionRecord->ExceptionAddress >= FUNCCAST(void *, DefCust)) &&
			 (ExceptionRecord->ExceptionAddress <= FUNCCAST(void *, GetCustDword)) ){
			*Comment = Msg[msgstr_brokencust];
		}
	}else if ( ExceptionRecord->ExceptionCode == EXCEPTION_FLT_INVALID_OPERATION){
		tstrcpy(exceptionbuf, Msg[msgstr_FPU]);
	}else if ( ExceptionRecord->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO){
		tstrcpy(exceptionbuf, Msg[msgstr_div0]);
	}else if ( ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW ){
		tstrcpy(exceptionbuf, Msg[msgstr_stackflow]);
	}else{
		thprintf(exceptionbuf, CMDLINESIZE, Msg[msgstr_except], ExceptionRecord->ExceptionCode);
	}
}

void DeepExceptionInfo(const TCHAR **Msg, TCHAR *exceptionbuf, TCHAR *stackinfo, EXCEPTION_RECORD *ExceptionRecord)
{
	TCHAR *eb;

	EXCEPTION_RECORD OldExceptionRecord;

	thprintf(stackinfo, SHOWSTACKSBUFSIZE, T("n:%1p(PPLIB:%1p)\np:%1p"),
			ExceptionRecord->ExceptionAddress, DLLhInst,
			PPxUnhandledExceptionFilterOld_Address);

	eb = exceptionbuf + tstrlen(exceptionbuf);
	*eb++ = '/';
	*eb++ = 'p';
	*eb++ = ':';

	OldExceptionRecord.ExceptionCode = PPxUnhandledExceptionFilterOld_Code;
	OldExceptionRecord.ExceptionAddress = PPxUnhandledExceptionFilterOld_Address;
	OldExceptionRecord.ExceptionInformation[0] = 0;
	OldExceptionRecord.ExceptionInformation[1] = PPxUnhandledExceptionFilterOld_Info1;
	GetExceptionCodeText(&OldExceptionRecord, Msg, eb, NULL);
}

#define ACTION_REPORT IDYES
#define ACTION_TERMINATE_PROCESS IDNO
#define ACTION_NEXTHANDLER IDCANCEL
#define ACTION_TERMINATE_THREAD IDIGNORE // ダイアログには選択肢としてでない

#pragma argsused
void CALLBACK ForceCloseProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	HWND hFocusWnd;

	UnUsedParam(uMsg);UnUsedParam(dwTime);

	KillTimer(hWnd, idEvent);
	hFocusWnd = GetActiveWindow();
	if ( hFocusWnd != NULL ){
		PostMessage(GetCaptionWindow(hFocusWnd), WM_COMMAND, TMAKEWPARAM(ACTION_TERMINATE_PROCESS, BN_CLICKED), 0);
	}
}

THREADEXRET THREADEXCALL PPxUnhandledExceptionFilterMain(ExceptionData *EP)
{
	EXCEPTION_RECORD *ExceptionRecord;
	HANDLE hProcess;
	char symblebuffer[STACKWALKSIZE];
	TAIL64(IMAGEHLP_SYMBOL) *syminfo;
	EXCEPTION_POINTERS *ExceptionInfo;
	DWORD ThreadID;

	TCHAR threadnamebuf[0x100];
	const TCHAR *threadname;

	THREADSTRUCT *tstruct;

	TCHAR exceptionbuf[256], stackinfo[SHOWSTACKSBUFSIZE + 0x100], *stackptr;
	const TCHAR **Msg = msgstringsE;
	const TCHAR *Comment = NilStr;
	int result = 0;
	TAIL64(STACKFRAME) sframe;

	if ( PPxUnhandledExceptionFilterThreadID != 0 ){ // 前の作業が終わるまで少し待つ
		int count = 1000 / 100;

		for(;;) {
			Sleep(100);
			if ( PPxUnhandledExceptionFilterThreadID == 0 ) break;
			if ( --count <= 0 ) break;
		}
//		XMessage(NULL, NULL, XM_DbgLOG, T("PPxUnhandledExceptionFilterMain wait end %d %d"), PPxUnhandledExceptionFilterThreadID, count);
	}
	if ( IsSjisLocale() ) Msg = msgstringsJ;

	ExceptionInfo = EP->ExceptionInfo;
	ExceptionRecord = ExceptionInfo->ExceptionRecord;

	// 例外詳細 exception
	GetExceptionCodeText(ExceptionRecord, Msg, exceptionbuf, &Comment);

	ThreadID = GetCurrentThreadId();
	// PPxUnhandledExceptionFilterMain 内で落ちたかもしれないので前回のを表示
	if ( PPxUnhandledExceptionFilterThreadID == ThreadID ){
		DeepExceptionInfo(Msg, exceptionbuf, stackinfo, ExceptionRecord);
		ShowErrorDialog(Msg, msgstr_fault, msgexfthread, exceptionbuf, stackinfo, Msg[msgstr_deepprom]);
		ExitProcess(ExceptionRecord->ExceptionCode);
		#pragma warning(suppress: 4702) // 直前のExitProcessで終わるけど、念のため
		t_endthreadex(EXCEPTION_EXECUTE_HANDLER);
	}
	PPxUnhandledExceptionFilterOld_Code = ExceptionRecord->ExceptionCode;
	PPxUnhandledExceptionFilterOld_Address = ExceptionRecord->ExceptionAddress;
	PPxUnhandledExceptionFilterOld_Info1 = ExceptionRecord->ExceptionInformation[1];
	PPxUnhandledExceptionFilterThreadID = ThreadID;

	// デバッグ情報、シンボル情報取得の準備
	hProcess = GetCurrentProcess();

	if ( DSymInitialize == NULL ){
		LoadSystemWinAPI(SYSTEMDLL_IMAGEHLP, IMAGEHLPDLL);
	}
	if ( DSymInitialize != NULL ){
		DSymInitialize(hProcess, NULL, TRUE);
		syminfo = (TAIL64(IMAGEHLP_SYMBOL) *)symblebuffer;
		syminfo->SizeOfStruct = STACKWALKSIZE;
		syminfo->MaxNameLength = STACKWALKSIZE - sizeof(TAIL64(IMAGEHLP_SYMBOL));
	}

	// 登録 PPx をテーブルから排除、Thread name取得
	tstruct = EP->FaultThread;
	if ( tstruct != NULL ){
		if ( tstruct->PPxNo >= 0 ){
			Sm->P[tstruct->PPxNo].ID[0] = '\0';
		}
		threadname = tstruct->ThreadName;
		if ( IsTrue(IsBadReadPtr(threadname, 8)) ){
			threadname = T("broken PPx");
		}
	}else{ // PPx の登録スレッド以外
		TCHAR *namelast;
		DefineWinAPI(LONG, NtQueryInformationThread, (HANDLE, int, PVOID, ULONG, PULONG));

		GetModuleFileName(NULL, stackinfo, SHOWSTACKSBUFSIZE);
		tstrcpy(threadnamebuf, FindLastEntryPoint(stackinfo));
		threadname = threadnamebuf;
		namelast = threadnamebuf + tstrlen(threadnamebuf);
		*namelast++ = ' ';

		GETDLLPROC(GetModuleHandle(T("ntdll.dll")), NtQueryInformationThread);
		if ( DNtQueryInformationThread == NULL ){
			tstrcpy(namelast, Msg[msgstr_unknownthread]);
		}else{
			ULONG ulLength;
			PVOID pBeginAddress = NULL;

			DNtQueryInformationThread(EP->hThread,
					defThreadQuerySetWin32StartAddress, &pBeginAddress,
					sizeof(pBeginAddress), &ulLength);

			if ( pBeginAddress == FUNCCAST(PVOID, PPxUnhandledExceptionFilterMain) ){
				thprintf(namelast, 256, T("%s(PPLIB:%1p)"), msgexfthread, DLLhInst);
			}else{
				GetFailedModuleAddr(hProcess, syminfo, namelast, pBeginAddress);
			}
		}
	}
	stackptr = stackinfo;
	stackinfo[0] = '\0';

	if ( UEFvalue != 0 ){
		stackptr = thprintf(stackptr, 256, T("Opt:%1p\n"), UEFvalue);
	}

	// スタック/例外アドレス stackinfo
	if ( DStackWalk != NULL ){
		BOOL findapp = FALSE;
		CONTEXT targetThc;
		int stacks = SHOWSTACKS;
		int count = 0;

		targetThc = *ExceptionInfo->ContextRecord;
		targetThc.ContextFlags = CONTEXT_FULL;
		memset(&sframe, 0, sizeof(sframe));

#if defined(_M_ARM) || defined(_M_ARM64)
		sframe.AddrPC.Offset = ExceptionInfo->ContextRecord->REGPREFIX(Pc);
		sframe.AddrStack.Offset = ExceptionInfo->ContextRecord->REGPREFIX(Sp);
		sframe.AddrFrame.Offset = 0;
#else
		sframe.AddrPC.Offset = ExceptionInfo->ContextRecord->REGPREFIX(ip);
		sframe.AddrStack.Offset = ExceptionInfo->ContextRecord->REGPREFIX(sp);
		sframe.AddrFrame.Offset = ExceptionInfo->ContextRecord->REGPREFIX(bp);
#endif
		sframe.AddrPC.Mode = AddrModeFlat;
		sframe.AddrStack.Mode = AddrModeFlat;
		sframe.AddrFrame.Mode = AddrModeFlat;

		for (;;){
			TCHAR addrstr[512];
			BOOL sresult;

			count++;
			sresult = DStackWalk(CPUTYPE, hProcess, EP->hThread, &sframe,
					&targetThc, NULL, DSymFunctionTableAccess,
					DSymGetModuleBase, NULL);
			if ( (sresult == 0) ||
				 (sframe.AddrPC.Offset == sframe.AddrReturn.Offset) ){
				break;
			}
			// エラーアドレスが記載されていないときは追加
			if ( (count == 1) &&
				((void *)sframe.AddrPC.Offset !=
					 (void *)ExceptionRecord->ExceptionAddress) ){
				GetFailedModuleAddr(hProcess, syminfo, addrstr,
						ExceptionRecord->ExceptionAddress);
				XMessage(NULL, NULL, XM_DbgLOG, T("Addr:%s"), addrstr);
				stackptr = thprintf(stackptr, 512, T("\t*%s\n"), addrstr);
			}
			// 指示アドレスの情報を記載
			GetFailedModuleAddr(hProcess, syminfo, addrstr,
					(void *)sframe.AddrPC.Offset);
			XMessage(NULL, NULL, XM_DbgLOG, T("Stack %d:%s"), count, addrstr);

			if ( stacks || !findapp ){ // 残り行数有りか、Appが見つかっていない
				if ( !findapp && (tstrstr(addrstr, T("(") AppExeName) != NULL)){
					findapp = TRUE;
					if ( stacks <= 1 ) stacks = 2;
				}

				// カスタマイズ領域破損の詳細
				if ( (tstrstr(addrstr, T("SortCustTable")) != NULL) ||
					 (tstrstr(addrstr, T("EnumCustTable")) != NULL) ||
					 (tstrstr(addrstr, T("GetCustTable")) != NULL) ){
					thprintf(addrstr + tstrlen(addrstr), 256, T(" %s:%1-%1"),
							FixCustTable(),
							CustHeaderFromCustP,
							(BYTE *)CustHeaderFromCustP + X_Csize);
				}

				if ( NowExit ){
					if ( tstristr(addrstr, T("jscript")) != NULL ){
						// jscript9.dll を使用するように設定する
						if ( (WinType >= WINTYPE_8) && !IsExistCustTable(StrCustOthers, StrJScr) ){
							SetCustTable(StrCustOthers, StrJScr, StrJScrFix, sizeof(StrJScrFix));
						}
						result = ACTION_TERMINATE_PROCESS;
					}
				}
#if 0
#define CMBS_THREAD			B7	// メインスレッド共通化
				// PPc と思われるスレッド(PPxUnRegisterThread後)
				if ( (tstruct == NULL) &&
					 (tstrstr(threadname, T("(PPC")) != NULL) ){
					if ( tstrstr(addrstr, T("CriticalSection")) != NULL ){
						if ( NowExit ) result = ACTION_TERMINATE_PROCESS;
						stacks += 3;
						// スレッド共通化
						GetCustData(T("X_combos"), &X_combosD, sizeof(X_combosD));
						setflag(X_combosD[0], CMBS_THREAD);
						SetCustData(T("X_combos"), &X_combosD, sizeof(X_combosD));
					}
				}
#endif
				// 送信用情報の用意
				if ( (stacks || findapp) &&
					 (((stackptr - stackinfo) + tstrlen(addrstr)) < (SHOWSTACKSBUFSIZE - 8)) ){
					stackptr = thprintf(stackptr, 512, T("\t%d:%s\n"), count, addrstr);
					if ( stacks ) stacks--;
				}
			}
		}
		if ( ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW ){
			stackptr = thprintf(stackptr, 512, T("\tstacks:%d:%d#%x\n"),
					count, CrmenuCheck.enter, CrmenuCheck.exectype);
		}
	}
	if ( stackinfo == stackptr ){
		GetFailedModuleAddr(hProcess, syminfo, stackinfo,
				ExceptionRecord->ExceptionAddress);
	}
	{
		DWORD tick = GetTickCount();
		int off = 0;

		if ( (tick - StartTick) < 3000 ) stackinfo[off++] = 'B';
		if ( Sm->NowShutdown ) stackinfo[off++] = 'S';
		if ( NowExit ) stackinfo[off++] = 'E';
		if ( (tick - CustTick) < 3000 ) stackinfo[off++] = 'C';
		if ( off > 0 ) stackinfo[off] = '>';
	}
/*
	// WM_GETOBJECT で異常終了する場合は、機能を無効にする(1.95+2)
	if ( tstrstr(stackinfo, T("LresultFromObject")) != NULL ){
		DWORD X_iacc = 0;
		GetCustData(T("X_iacc"), &X_iacc, sizeof(X_iacc));
	}
*/
	// PPxスレッド以外であり、PPx 以外の実行ファイルで、PPxが関与していないならエラー処理をしない
	if ( (tstrchr(threadname, '(') != NULL) &&
		 (tstristr(threadname, T("PP")) == NULL) &&
		 (tstristr(stackinfo, T("(") AppExeName) == NULL) ){
		result = ACTION_NEXTHANDLER;
	// X_igpn に該当する場合はエラー処理をしない
	}else if ( FilenameRegularExpression(stackinfo, &FR_igpn) == FRRESULT_MATCH ){
		result = ACTION_NEXTHANDLER;
	// 管理外スレッドで、無視しても良さそうなのは、スレッド強制終了
	}else if ( (tstruct == NULL) && (
		  (tstrstr(stackinfo, T("SS1H001")) != 0) ) ){
		result = ACTION_TERMINATE_THREAD;
	// スレッド再起動可能スレッドで、一部の異常が起きやすいDLLなら、Thr再起動
	}else if ( ((tstruct != NULL) && (tstruct->flag & XTHREAD_RESTARTREQUEST))
		&&
		( (WinType == WINTYPE_VISTA) || // ← SHGetFileInfo が DOS EXE で落ちる対策、XP, 7, 10 では問題なし
		  (tstrstr(stackinfo, T("SugarSyncShellExt")) != 0) ||
		  (tstrstr(stackinfo, T("Resource")) != 0) || // LdrResSearchResource, FindResourceEx, BaseDllMapResourceId
		  (tstrstr(stackinfo, T("TortoiseSVN")) != 0) ||
		  (tstrstr(stackinfo, T("libapr_tsvn")) != 0) ) ){
		result = ACTION_TERMINATE_THREAD;
	// 例外そのものか、例外を握りつぶして動作する物なら続行させて、任せる
	}else if (
			(tstrstr(stackinfo, T("IsBad")) != 0) || // IsBadRead など
			(tstrstr(stackinfo, T(CHECK_HOOK)) != 0) || // hook.dll
			(tstrstr(stackinfo, ValueX3264(T("astMC32:"), T("astMC:"))) != 0) || // AssetView
			(tstrstr(stackinfo, T("ctiuser:")) != 0) || // Cb Defense Sensor
			(tstrstr(stackinfo, T("SS1H001")) != 0) ||
			(tstrstr(threadname, T(CHECK_LoadDLL)) != 0) || // Malware判定？
			(tstrstr(stackinfo, T("RaiseException")) != 0) ){
		result = ACTION_NEXTHANDLER;
	// シャットダウン時、PPx の関与が分からない問題なら、ダイアログなしにする
	}else if ( Sm->NowShutdown &&
		 (tstristr(stackinfo, T("(") AppExeName) == NULL) ){
		result = ACTION_TERMINATE_PROCESS;
	}else if ( result == 0 ){
		if ( Sm->NowShutdown ){ // シャットダウン時は 20秒後、自動NO選択
			SetTimer(NULL, TIMERID_SHUTDOWN_FAULT, TIME_SHUTDOWN_FAULT, ForceCloseProc);
		}
		result = ShowErrorDialog(Msg, msgstr_fault, threadname, exceptionbuf, stackinfo, Comment);
	}

	if ( result == ACTION_NEXTHANDLER ){ // 他のエラーハンドラへ
		if ( DSymCleanup != NULL ) DSymCleanup(hProcess);
		PPxUnhandledExceptionFilterThreadID = 0;
		EP->Result = EXCEPTION_CONTINUE_SEARCH;
		t_endthreadex(EXCEPTION_CONTINUE_SEARCH);
	}

	if ( (tstruct != NULL) &&
		 (tstruct->flag & (XTHREAD_RESTARTREQUEST | XTHREAD_TERMENABLE)) ){
		if ( tstruct->flag & XTHREAD_RESTARTREQUEST ){
			SendMessage( ((RESTARTTHREADSTRUCT *)tstruct)->hParentWnd,
					WM_PPXCOMMAND,
					((RESTARTTHREADSTRUCT *)tstruct)->wParam,
					((RESTARTTHREADSTRUCT *)tstruct)->lParam);
		}else{
			XMessage(NULL, NULL, XM_DbgLOG, T("Terminate Thread"));
		}
		result = ACTION_TERMINATE_THREAD;
	}

	if ( result == ACTION_TERMINATE_THREAD ){ // 無視してスレッドを強制終了
		HANDLE hCloseThread;

		if ( DSymCleanup != NULL ) DSymCleanup(hProcess);

		UnRegisterThread(EP->ThreadID);
		PPxUnhandledExceptionFilterThreadID = 0;
		hCloseThread = EP->hThread;
#pragma warning(suppress:6258) // 異常スレッドの強制終了
		TerminateThread(hCloseThread, 0);
		CloseHandle(hCloseThread);
		t_endthreadex(0);
	}

	// ACTION_TERMINATE_PROCESS / ACTION_REPORT
	ExitProcess(ExceptionRecord->ExceptionCode);
	#pragma warning(suppress: 4702) // 直前のExitProcessで終わるけど、念のため
	t_endthreadex(EXCEPTION_EXECUTE_HANDLER);
}

LONG WINAPI PPxUnhandledExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
	ExceptionData EP;
	DWORD tid;
	HANDLE hThread, hProcess;

	EXCEPTION_RECORD *ExceptionRecord = ExceptionInfo->ExceptionRecord;
	DWORD_PTR checkaddr = (DWORD_PTR)ExceptionRecord->ExceptionAddress;
	DWORD ExceptionCode = ExceptionRecord->ExceptionCode;

	if ( ExceptionCode == EXCEPTION_FLT_INVALID_OPERATION ){
#if !defined(_WIN64) && USETASM32
		if ( InitFloat() == EXCEPTION_CONTINUE_EXECUTION ){
			return EXCEPTION_CONTINUE_EXECUTION;
		}
#endif
		// PPx Process でない場合は、実行元に処理させる
		if ( PPxProcess == FALSE ) return EXCEPTION_CONTINUE_SEARCH;
	}
	if ( ExceptionCode & B29 ){ // ユーザ定義
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if ( ExceptionCode == EXCEPTION_ACCESS_VIOLATION ){
		DWORD_PTR targetaddr;

		// IsBadWritePtr/IsBadReadPtr 内は無視
		targetaddr = (DWORD_PTR)GetProcAddress(hKernel32, "IsBadWritePtr");
		if ( targetaddr != 0 ){
			if ( (checkaddr >= (targetaddr -  0x60)) &&
				 (checkaddr <  (targetaddr + 0x140)) ){
				return EXCEPTION_CONTINUE_SEARCH;
			}
			targetaddr = (DWORD_PTR)GetProcAddress(hKernel32, "IsBadReadPtr");
			if ( (checkaddr >= (targetaddr -  0x60)) &&
				 (checkaddr <  (targetaddr + 0x140)) ){
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}

		// アクセス違反を握りつぶしている/ごまかしているソフト対策
		if ( ExceptionRecord->ExceptionInformation[1] == (DWORD_PTR)-1 ){
			// Kaspersky Anti-Virus
			DWORD_PTR hDLL = (DWORD_PTR)GetModuleHandle(T("prremote.dll"));
			if ( (hDLL != 0) && (hDLL < checkaddr) ){
				return EXCEPTION_CONTINUE_SEARCH;
			}
			// Kaspersky Anti-Virus
			hDLL = (DWORD_PTR)GetModuleHandle(T("scrchpg.dll"));
			if ( (hDLL != 0) && (hDLL < checkaddr) ){
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}
		if ( ExceptionRecord->ExceptionInformation[1] == (DWORD_PTR)0 ){
			// ATOK 2022 (Win10 RS1/RS2、１文字目入力時に落ちる)
			DWORD_PTR hDLL = (DWORD_PTR)GetModuleHandle(T("ATOK32TIP.DLL"));
			if ( (hDLL != 0) && (hDLL < checkaddr) && ((hDLL + 0x800000) > checkaddr) ){
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}
	}else if ( (ExceptionCode != EXCEPTION_FLT_INVALID_OPERATION) &&
//		(ExceptionCode != EXCEPTION_DATATYPE_MISALIGNMENT) && // 境界アクセス違反
		(ExceptionCode != EXCEPTION_IN_PAGE_ERROR) &&
		(ExceptionCode != EXCEPTION_STACK_OVERFLOW) &&
		(ExceptionCode != EXCEPTION_INT_DIVIDE_BY_ZERO) ){
//		XMessage(NULL, NULL, XM_DbgLOG, T("Exception! code:%xH"), ExceptionRecord->ExceptionCode);
		return EXCEPTION_CONTINUE_SEARCH;
	}
/*
	if ( (checkaddr >= FUNCCAST(DWORD_PTR, PPxWaitExitThread)) &&
		 (checkaddr < FUNCCAST(DWORD_PTR, PPxRegisterThread)) ){
		*ExceptionInfo->ContextRecord = WaitThreadContext;
		ExceptionRecord->ExceptionFlags = 0;
		ExceptionRecord->ExceptionAddress = (PVOID)WaitThreadContext.REGPREFIX(ip);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
*/
	// 報告ダイアログを表示するためのスレッドを作成する
	EP.Result = EXCEPTION_EXECUTE_HANDLER;
	EP.ExceptionInfo = ExceptionInfo;
	EP.ThreadID = GetCurrentThreadId();
	EP.FaultThread = GetThreadInfoFromID(EP.ThreadID);

	hProcess = GetCurrentProcess();
	DuplicateHandle(hProcess, GetCurrentThread(),
			hProcess, &EP.hThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
	hThread = t_beginthreadex(NULL, 0,
			(THREADEXFUNC)PPxUnhandledExceptionFilterMain, &EP, 0, &tid);
	if ( hThread == NULL ) return EXCEPTION_CONTINUE_SEARCH;
	CloseHandle(hThread);
	while ( EP.Result == EXCEPTION_EXECUTE_HANDLER ){
		Sleep(100);
	}
	CloseHandle(EP.hThread);
	return EP.Result;
}

THREADEXRET THREADEXCALL PPxSendReportThread(TCHAR *text)
{
	const TCHAR **Msg = msgstringsE;

	if ( IsSjisLocale() ) Msg = msgstringsJ;
	ShowErrorDialog(Msg, msgstr_foundprom, NoItem, text + 1, NoItem, NULL);
	text[0] = '\1';
	t_endthreadex(0);
}

void PPxSendReport(const TCHAR *text)
{
	TCHAR *textbuf;
	DWORD tid;

	if ( text == NULL) text = NilStr;
	textbuf = HeapAlloc(DLLheap, 0, TSTRSIZE(text) + sizeof(TCHAR));
	if ( textbuf == NULL ) return;
	*textbuf = '\0';
	tstrcpy(textbuf + 1, text);
	CloseHandle(t_beginthreadex(NULL, 0,
			(THREADEXFUNC)PPxSendReportThread, textbuf, 0, &tid));
	while( textbuf[0] == '\0' ) Sleep(100);
}

const TCHAR OSverinfo[] = T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
const TCHAR ReleaseId[] = T("ReleaseId"); // Windows10 TH1 より後(RS1位から?)
const TCHAR DisplayVersion[] = T("DisplayVersion"); // Windows10 20H2 から

void GetOSverStr(TCHAR *osverstr)
{
	if ( WinType < WINTYPE_10 ){
		thprintf(osverstr, 32, T("%d.%d"),
				OSver.dwMajorVersion, OSver.dwMinorVersion);
	}else{
		osverstr = thprintf(osverstr, 16, T("%d."),
				(OSver.dwBuildNumber >= WINBUILD_11_21H1) ? 11 : 10);

		GetRegString(HKEY_LOCAL_MACHINE, OSverinfo, DisplayVersion, osverstr, TSTROFF(10));
		if ( osverstr[0] == '\0' ){
			GetRegString(HKEY_LOCAL_MACHINE, OSverinfo, ReleaseId, osverstr, TSTROFF(10));
		}
	}
}

int ShowErrorDialog(const TCHAR **Msg, int msgtype, const TCHAR *threadtext, const TCHAR *infotext, const TCHAR *addrtext, const TCHAR *comment)
{
	TCHAR msgbuf[WSSIZE], OSverstr[16], *typename;
	int result;

	GetOSverStr(OSverstr);

	if ( msgtype == msgstr_foundprom ){
		thprintf(msgbuf, TSIZEOF(msgbuf), Msg[msgstr_foundprom],
				OSverstr, infotext, Msg[msgstr_sendreport]);
		typename = T("report");
	}else{
		thprintf(msgbuf, TSIZEOF(msgbuf), Msg[msgstr_fault],
				threadtext, infotext, OSverstr, addrtext,
				Msg[msgstr_sendreport], comment);
		typename = T("fault");
	}
	msgbuf[TSIZEOF(msgbuf) - 8] = '\0';
	XMessage(NULL, NULL, XM_DbgLOG, T("%s"), msgbuf);
	result = CriticalMessageBox(msgbuf, msgtitle,
			MB_YESNOCANCEL | MB_DEFBUTTON2 | MB_ICONSTOP);
	if ( result == ACTION_REPORT ){
		thprintf(msgbuf, TSIZEOF(msgbuf), REPORTURL T("?exe=") AppName
			T("\1ver=") T(FileProp_Version) MSGCTYPE
			T("\1type=%s\1thread=%s\1info=%s\1os=%s\1addr=%s"),
			typename, threadtext, infotext, OSverstr, addrtext);
		msgbuf[TSIZEOF(msgbuf) - 8] = '\0';
		QueryEncode(tstrchr(msgbuf, '?') + 1);
		if ( PPxShellExecute(NULL, NULL, msgbuf, NULL, NULL,
				XEO_NOSCANEXE, msgbuf) == NULL ){
			CriticalMessageBox(Msg[msgstr_faultsend], msgbuf, MB_OK | MB_ICONSTOP);
		}
	}
	return result;
}

void ReportLCID(LCID userlcid)
{
	ThSTRUCT threcv;
	TCHAR msgbuf[WSSIZE], OSverstr[16];

	GetOSverStr(OSverstr);

	thprintf(msgbuf, TSIZEOF(msgbuf), REPORTURL T("?exe=") AppName
			T("&ver=") T(FileProp_Version) MSGCTYPE
			T("&type=report&thread=-&info=LCID%04x&os=%s&addr=-"),
			LOWORD(userlcid), OSverstr);
	QueryEncode(tstrchr(msgbuf, '?') + 1);
	GetImageByHttp(msgbuf, &threcv);
	ThFree(&threcv);
}

#pragma warning(suppress:6262) // スタックを使いすぎだが、現在は無視
void ProcessInfo(void)
{
	HANDLE hSnapshot;
	THREADENTRY32 ThreadData;
	MODULEENTRY32X ModuleData;
	DWORD pid = GetCurrentProcessId();
	HWND hEWnd;
	TCHAR buf[0x400];
	MSG msg;
	HANDLE hProcess;
	char symblebuffer[STACKWALKSIZE];
	TAIL64(IMAGEHLP_SYMBOL) *syminfo;
	ThSTRUCT thText;

	DefineWinAPI(HANDLE, CreateToolhelp32Snapshot, (DWORD, DWORD));
	DefineWinAPI(BOOL, Thread32First, (HANDLE, LPTHREADENTRY32));
	DefineWinAPI(BOOL, Thread32Next, (HANDLE, LPTHREADENTRY32));
	DefineWinAPI(BOOL, Module32First, (HANDLE, MODULEENTRY32X *));
	DefineWinAPI(BOOL, Module32Next, (HANDLE, MODULEENTRY32X *));
	DefineWinAPI(BOOL, GetThreadTimes, (HANDLE, LPFILETIME, LPFILETIME, LPFILETIME, LPFILETIME));
	DefineWinAPI(LONG, NtQueryInformationThread, (HANDLE, int, PVOID, ULONG, PULONG));

	GETDLLPROC(hKernel32, CreateToolhelp32Snapshot);
	GETDLLPROC(hKernel32, Thread32First);
	GETDLLPROC(hKernel32, Thread32Next);
	GETDLLPROCTA(hKernel32, Module32First);
	GETDLLPROCTA(hKernel32, Module32Next);
	GETDLLPROC(hKernel32, OpenThread);
	GETDLLPROC(hKernel32, GetThreadTimes);
	GETDLLPROC(GetModuleHandleA("ntdll.dll"), NtQueryInformationThread);

	ThInit(&thText);

	hProcess = GetCurrentProcess();
	if ( DSymInitialize == NULL ){
		LoadSystemWinAPI(SYSTEMDLL_IMAGEHLP, IMAGEHLPDLL);
	}
	if ( DSymInitialize != NULL ){
		DSymInitialize(hProcess, NULL, TRUE);
		syminfo = (TAIL64(IMAGEHLP_SYMBOL) *)symblebuffer;
		syminfo->SizeOfStruct = STACKWALKSIZE;
		syminfo->MaxNameLength = STACKWALKSIZE - sizeof(TAIL64(IMAGEHLP_SYMBOL));
	}
	GetOSverStr(buf);
	thprintf(&thText, 0, T(FileProp_Version) T(" WinVer:%s.%d\r\n[Thread]\r\n"),
			buf, OSver.dwBuildNumber);
	if ( DCreateToolhelp32Snapshot != NULL ){
		hSnapshot = DCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD | TH32CS_SNAPMODULE, 0);
	}else{
		hSnapshot = INVALID_HANDLE_VALUE;
	}
	if ( hSnapshot != INVALID_HANDLE_VALUE ){
		ThreadData.dwSize = sizeof(THREADENTRY32);
		if ( DThread32First(hSnapshot, &ThreadData) ) do {
			if ( ThreadData.th32OwnerProcessID == pid ){
				const TCHAR *tstr;
				HANDLE hThread;
				FILETIME CreateTime, ExitTime, KernelTime, UserTime;

				ThreadListStruct *list;
				DWORD a, b, c;

				tstr = T("?");
				a = b = c = 0;

				list = (ThreadListStruct *)ThreadList.heap;
				if ( list != NULL ) for (;;){
					if ( list >= (ThreadListStruct *)ThreadList.last ) break;
					if ( list->id == ThreadData.th32ThreadID ){
						tstr = list->thread->ThreadName;
						a = list->thread->flag & 0x7f;
						b = (list->thread->flag >> 7) & 0x1ff;
						c = (list->thread->flag >> 16) & 0xffff;
					}
					list++;
				}
				if ( DOpenThread != NULL ){
					hThread = DOpenThread(THREAD_QUERY_INFORMATION,
							FALSE, ThreadData.th32ThreadID);
					if ( hThread != NULL ){
						ULONG ulLength;
						PVOID pBeginAddress = NULL;

#define defThreadQuerySetWin32StartAddress 9
						if ( tstr[0] == '?' ){
							if ( DSymInitialize != NULL ){
								DNtQueryInformationThread(hThread,
									defThreadQuerySetWin32StartAddress,
									&pBeginAddress, sizeof(pBeginAddress),
									&ulLength);
								thprintf(buf, 24, T("%p"), pBeginAddress);
								GetFailedModuleAddr(hProcess, syminfo, buf,
									pBeginAddress);
								tstr = buf;
							}
						}
						DGetThreadTimes(hThread, &CreateTime, &ExitTime, &KernelTime, &UserTime);
						CloseHandle(hThread);
					}
				}
				#pragma warning(suppress:6001) // メモなので不定値でも問題ない
				thprintf(&thText, 0, T("%d(%s) Tflag:%x Ewait:%x Eflag:%x %u\r\n"),
					ThreadData.th32ThreadID,
					tstr,
					a, b, c,
					KernelTime.dwLowDateTime + UserTime.dwLowDateTime
				);
			}
		} while ( DThread32Next(hSnapshot, &ThreadData) );

		ThCatString(&thText, T("\r\n[Modules]\r\n"));
		ModuleData.dwSize = sizeof(MODULEENTRY32X);
		if ( DModule32First(hSnapshot, &ModuleData) ) do {
			thprintf(&thText, 0, T("%p %7x %s\r\n"),
					ModuleData.modBaseAddr,
					ModuleData.modBaseSize,
					ModuleData.szExePath
			);
		} while ( DModule32Next(hSnapshot, &ModuleData) );
		CloseHandle(hSnapshot);
	}
	hEWnd = PPEui(NULL, T("Process info."), (TCHAR *)thText.bottom);
	ThFree(&thText);

	while ( IsWindow(hEWnd) ){
		if ( (int)GetMessage(&msg, NULL, 0, 0) <= 0 ) break;
		if ( DialogKeyProc(&msg) ) continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

#if 0
DefineWinAPI(LONG, NtQueryInformationThread, (HANDLE, int, PVOID, ULONG, PULONG)) = NULL;

void GetThreadInfo(void)
{
	TCHAR buf[VFPS];
	ULONG ulLength;
	PVOID pBeginAddress = NULL;
	TAIL64T(IMAGEHLP_MODULE) mdlinfo;

	if ( DNtQueryInformationThread == NULL ){
		GETDLLPROC(GetModuleHandle(T("ntdll.dll")), NtQueryInformationThread);
		if ( DNtQueryInformationThread == NULL ) return;
	}
	if ( DSymInitialize == NULL ){
		if ( LoadSystemWinAPI(SYSTEMDLL_IMAGEHLP, IMAGEHLPDLL) == NULL ){
			return;
		}
		DSymInitialize(GetCurrentProcess(), NULL, TRUE);
	}

	DNtQueryInformationThread(GetCurrentThread(),
			defThreadQuerySetWin32StartAddress, &pBeginAddress,
			sizeof(pBeginAddress), &ulLength);

	if ( pBeginAddress != (PVOID)PPxUnhandledExceptionFilterMain ){
		mdlinfo.SizeOfStruct = sizeof(mdlinfo);
		if ( IsTrue(DSymGetModuleInfo(GetCurrentProcess(), (DWORD_PTR)pBeginAddress, &mdlinfo)) ){
			XMessage(NULL, NULL, XM_DbgLOG, T("New thread: ") T(PTRPRINTFORMAT) T(":") T(PTRPRINTFORMAT) T(" %s"), pBeginAddress, mdlinfo.BaseOfImage, mdlinfo.ModuleName);
		}
	}
}
#endif
