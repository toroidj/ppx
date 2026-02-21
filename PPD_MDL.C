/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library						PPx Module
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include <shlobj.h>
#include "PPXVER.H"
#include "PPX.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#include "CALC.H"
#pragma hdrstop

const WCHAR PPLIBPATH_Name[] = L"Lib";
const WCHAR PPLIBPATH_RegID[] = L"___";
ThSTRUCT Thmodule = ThSTRUCT_InitData, Thmodule_str = ThSTRUCT_InitData;
int ppxmodule_count = -1;

typedef struct {
	PPXAPPINFOW info;
	PPXAPPINFO  *parent;
	EXECSTRUCT *Z;
	const TCHAR *rawparam;
} COMMANDMODULEINFOSTRUCT;

typedef int (PPXAPI *tagCommandModuleEntry)(PPXAPPINFOW *ppxa, DWORD cmdID, PPXMODULEPARAM pxs);
typedef int (USECDECL *tagOldCommandModuleEntry)(PPXAPPINFOW *ppxa, DWORD cmdID, PPXMODULEPARAM pxs);

DWORD_PTR InfoFunc_NewAppInfo(COMMANDMODULEINFOSTRUCT *ppxa, PPXCMDID_NEWAPPINFO_STRUCT *pns);

#define MODULE_NOLOAD 0
typedef struct {
	HMODULE hDLL;	// module handle
	DWORD types;	// 内容(読み込み失敗しているときはMODULE_NOLOAD)
	DWORD time;		// module のタイムスタンプ
	int DllNameOffset; // DLL名
	tagCommandModuleEntry ModuleEntry;
#ifndef _WIN64
	tagOldCommandModuleEntry OldModuleEntry;
#endif
} MODULESTRUCT;

MODULESTRUCT *ppxmodule_list = NULL;
const TCHAR P_moduleStr[] = T("P_module");

DWORD_PTR InfoFunc_GetFileinfo(PPXAPPINFOUNIONW *uptr)
{
	VFSFILETYPE vft;
	WCHAR *path;
	TCHAR *result;
//	BOOL eqfree = FALSE;

	switch ( uptr->nums[0] ){
		case 1:
			vft.flags = VFSFT_TYPETEXT;
			vft.typetext[0] = '\0';
			result = vft.typetext;
			break;

		case 2:
			vft.flags = VFSFT_EXT;
			vft.ext[0] = '\0';
			result = vft.ext;
			break;
/*
		case 3:
			vft.flags = VFSFT_INFO;
			vft.info = NULL;
			result = vft.info;
			eqfree = TRUE;
			break;

		case 4:
			vft.flags = VFSFT_COMMENT;
			vft.comment = NULL;
			result = vft.comment;
			eqfree = TRUE;
			break;
*/
		default:
			vft.flags = VFSFT_TYPE;
			vft.type[0] = '\0';
			result = vft.type;
	}
	path = (WCHAR *)&uptr->nums[1];

	#ifdef UNICODE
		VFSGetFileType(path, NULL, 0, &vft);
		tstplimcpy((WCHAR *)&uptr->nums[1], result, CMDLINESIZE);
	#else
	{
		char bufA[CMDLINESIZE];

		UnicodeToAnsi(path, bufA, sizeof(bufA));
		VFSGetFileType(bufA, NULL, 0, &vft);
		AnsiToUnicode(result, (WCHAR *)&uptr->nums[1], CMDLINESIZE);
	}
	#endif
//	if ( eqfree ) HeapFree(DLLheap, 0, (void *)result);
	return PPXA_NO_ERROR;
}

DWORD_PTR InfoFunc_GetVariableData(ThSTRUCT *thSV, PPXAPPINFOUNIONW *uptr)
{
#ifdef UNICODE
	if ( uptr->dptrs[1] != 0 ){
		uptr->dptrs[1] = (DWORD_PTR)ThGetLongString(thSV, (WCHAR *)uptr->dptrs[0], (WCHAR *)uptr->dptrs[1], CMDLINESIZE);
	}
#else
	if ( uptr->dptrs[1] != 0 ){
		const TCHAR *str;
		TCHAR *bufptr;
		char bufA[CMDLINESIZE];
		int len;

		bufA[0] = '\0';
		UnicodeToAnsi((WCHAR *)uptr->dptrs[0], bufA, sizeof(bufA));
		str = ThGetString(thSV, bufA, bufA, CMDLINESIZE);
		if ( str == NULL ){
			*((WCHAR *)uptr->dptrs[1]) = '\0';
		}else{
			len = AnsiToUnicode(bufA, (WCHAR *)uptr->dptrs[1], CMDLINESIZE);
			if ( (len >= (CMDLINESIZE - 16)) || ((len == 0) && (*str != '\0')) ){
				size_t length;

				length = AnsiToUnicode(str, NULL, 0) + 1;
				bufptr = HeapAlloc(ProcHeap, 0, length * sizeof(WCHAR));
				if ( bufptr != NULL ){
					AnsiToUnicode(str, (WCHAR *)bufptr, length);
					uptr->dptrs[1] = (DWORD_PTR)bufptr;
				}
			}
		}
	}
#endif
	return PPXA_NO_ERROR;
}

void InfoFunc_SetVariableData(ThSTRUCT *thSV, PPXAPPINFOUNIONW *uptr)
{
#ifdef UNICODE
	ThSetString(thSV, (WCHAR *)uptr->dptrs[0], (WCHAR *)uptr->dptrs[1]);
#else
	char bufA[CMDLINESIZE];

	UnicodeToAnsi((WCHAR *)uptr->dptrs[0], bufA, sizeof(bufA));
	UnicodeToAnsi((WCHAR *)uptr->dptrs[1], bufA + 32, sizeof(bufA) - 32);
	ThSetString(thSV, bufA, bufA + 32);
#endif
}

#ifndef UNICODE
char *UnicodeToAnsiLong(const WCHAR *source, char *destbuf, int destlength)
{
	int length;
	char *extbuf;

	if ( UnicodeToAnsi(source, destbuf, destlength) > 0 ) return destbuf;
	if ( (source == NULL) || (*source == '\0') ){
		*destbuf = '\0';
		return destbuf;
	}

	length = UnicodeToAnsi(source, NULL, 0) + 1;
	extbuf = HeapAlloc(DLLheap, 0, length);
	if ( extbuf != NULL ){
		UnicodeToAnsi(source, extbuf, length);
		return extbuf;
	}else{
		destbuf[destlength - 1] = '\0';
		return destbuf;
	}
}
#endif

DWORD_PTR USECDECL CommandModuleInfoFunc(COMMANDMODULEINFOSTRUCT *ppxa, DWORD cmdID, PPXAPPINFOUNIONW *uptr)
{
	TCHAR buf[CMDLINESIZE];
#ifndef UNICODE
	char bufA[CMDLINESIZE];
#endif

	if ( cmdID < PPXCMDID_FILL ){
		if ( cmdID < 0x100 ){
#ifndef UNICODE
			PPXCMDENUMSTRUCT work;

			work.enumID = uptr->enums.enumID;
			work.buffer = bufA;
			PPxInfoFunc(ppxa->parent, cmdID, &work);
			AnsiToUnicode(bufA, uptr->enums.buffer, CMDLINESIZE);
#else
			PPxInfoFunc(ppxa->parent, cmdID, uptr);
#endif
		}
		return PPXA_NO_ERROR;
	}

	switch (cmdID){
		case PPXCMDID_EXECUTE: {
			ERRORCODE result;

			#ifdef UNICODE
			result = PP_ExtractMacro(ppxa->info.hWnd, ppxa->parent, NULL, uptr->str, NULL, 0);
			#else
			char *extbuf;

			extbuf = UnicodeToAnsiLong(uptr->str, bufA, sizeof(bufA));
			result = PP_ExtractMacro(ppxa->info.hWnd, ppxa->parent, NULL, extbuf, NULL, 0);
			if ( extbuf != bufA ) HeapFree(DLLheap, 0, extbuf);
			#endif
			if ( result <= 1 ) result ^= 1;
			return result;
		}

		case PPXCMDID_EXTRACT:
			#ifdef UNICODE
			if ( NO_ERROR == PP_ExtractMacro(ppxa->info.hWnd, ppxa->parent, NULL, uptr->str, buf, XEO_EXTRACTEXEC) ){
				strcpyW(uptr->str, buf);
			}else{
				*uptr->str = '\0';
			}
			#else
			UnicodeToAnsi(uptr->str, bufA, sizeof(bufA));
			if ( NO_ERROR == PP_ExtractMacro(ppxa->info.hWnd, ppxa->parent, NULL, bufA, buf, XEO_EXTRACTEXEC) ){
				AnsiToUnicode(buf, uptr->str, CMDLINESIZE);
			}else{
				*uptr->str = '\0';
			}
			#endif
			return PPXA_NO_ERROR;

		case PPXCMDID_LONG_EXTRACT_E:
		case PPXCMDID_LONG_EXTRACT: {
			ERRORCODE result;
			BSTR sysstr;

			if ( hOleaut32DLL == NULL ){
				hOleaut32DLL = LoadSystemWinAPI(SYSTEMDLL_OLEAUT32, OLEAUT32_SysStr);
				if ( hOleaut32DLL == NULL ) return PPXA_INVALID_FUNCTION;
			}
			PP_InitLongParam(buf);

			#ifdef UNICODE
			result = PP_ExtractMacro(ppxa->info.hWnd, ppxa->parent, NULL, uptr->str, buf, XEO_EXTRACTEXEC | XEO_EXTRACTLONG );
			if ( (result != NO_ERROR) && (result != ERROR_PARTIAL_COPY) &&
				 (cmdID == PPXCMDID_LONG_EXTRACT_E) ){
				return (DWORD_PTR)(result & 0xffff);
			}
			sysstr = CreateBstring(PP_GetLongParam(buf, result));
			#else
			{
				char *extbuf;

				extbuf = UnicodeToAnsiLong(uptr->str, bufA, sizeof(bufA));
				result = PP_ExtractMacro(ppxa->info.hWnd, ppxa->parent, NULL, extbuf, buf, XEO_EXTRACTEXEC | XEO_EXTRACTLONG );
				if ( extbuf != bufA ) HeapFree(DLLheap, 0, extbuf);
			}
			if ( (result != NO_ERROR) && (result != ERROR_PARTIAL_COPY) &&
				 (cmdID == PPXCMDID_LONG_EXTRACT_E) ){
				return (DWORD_PTR)(result & 0xffff);
			}
			sysstr = CreateBstringA(PP_GetLongParam(buf, result));
			#endif

			PP_FreeLongParam(buf, result);
			return (DWORD_PTR)sysstr;
		}

		case PPXCMDID_GETRAWPARAM:
			if ( hOleaut32DLL == NULL ){
				hOleaut32DLL = LoadSystemWinAPI(SYSTEMDLL_OLEAUT32, OLEAUT32_SysStr);
				if ( hOleaut32DLL == NULL ) return PPXA_INVALID_FUNCTION;
			}
			#ifdef UNICODE
				return (DWORD_PTR)CreateBstring((ppxa->rawparam != NULL) ? ppxa->rawparam : NilStr);
			#else
				return (DWORD_PTR)CreateBstringA((ppxa->rawparam != NULL) ? ppxa->rawparam : NilStr);
			#endif

		case PPXCMDID_GETFILEINFO:
			return InfoFunc_GetFileinfo(uptr);

#ifndef UNICODE
		case PPXCMDID_COMBOTABLONGID:
		case PPXCMDID_COMBOTABIDNAME:
		case PPXCMDID_COMBOTABNAME: {
			DWORD_PTR result;

			memcpy(bufA, uptr, sizeof(DWORD) * 2);
			result = PPxInfoFunc(ppxa->parent, cmdID, bufA);
			AnsiToUnicode(bufA, uptr->str, CMDLINESIZE);
			return result;
		}

		case PPXCMDID_COMBOGROUPNAME: {
			DWORD_PTR result;
			WCHAR *strW = uptr->tabidxstr.str;

			uptr->tabidxstr.str = (WCHAR *)bufA;
			result = PPxInfoFunc(ppxa->parent, cmdID, uptr);
			uptr->tabidxstr.str = strW;
			AnsiToUnicode(bufA, strW, CMDLINESIZE);
			return result;
		}

		case PPXCMDID_SETCOMBOGROUPNAME:
		case PPXCMDID_COMBOGETTAB:
		case PPXCMDID_COMBOTABEXECUTE:
		case PPXCMDID_COMBOTABEXTRACT: {
			DWORD_PTR result;
			WCHAR *strW = uptr->tabidxstr.str;

			UnicodeToAnsi(strW, bufA, sizeof(bufA));
			uptr->tabidxstr.str = (WCHAR *)bufA;
			result = PPxInfoFunc(ppxa->parent, cmdID, uptr);
			if ( cmdID == PPXCMDID_COMBOTABEXTRACT ){
				uptr->tabidxstr.str = strW;
				AnsiToUnicode(bufA, strW, CMDLINESIZE);
			}
			return result;
		}

		case PPXCMDID_COMBOTABEXTRACTLONG: {
			WCHAR *strW;
			const TCHAR *result;
			BSTR sysstr;

			HWND hTargetWnd = (HWND)PPxInfoFunc(ppxa->parent, PPXCMDID_COMBOTABHWND, uptr);
			if ( hTargetWnd == NULL ){
				return PPXA_INVALID_FUNCTION;
			}

			if ( hOleaut32DLL == NULL ){
				hOleaut32DLL = LoadSystemWinAPI(SYSTEMDLL_OLEAUT32, OLEAUT32_SysStr);
				if ( hOleaut32DLL == NULL ) return PPXA_INVALID_FUNCTION;
			}
			strW = uptr->tabidxstr.str;
			UnicodeToAnsi(strW, bufA, sizeof(bufA));
			result = SendExtractCall(hTargetWnd, bufA);
			if ( result == NULL ) return PPXA_NO_ERROR;
			sysstr = CreateBstring(result);
			if ( sysstr == NULL ) return PPXA_NO_ERROR;
			HeapFree(DLLheap, 0, (void *)result);
			return (DWORD_PTR)sysstr;
		}

		case PPXCMDID_ENTRYNAME:
		case PPXCMDID_ENTRYANAME:
		case PPXCMDID_ENTRYCOMMENT:
			*((long *)bufA) = uptr->nums[0];
			// PPXCMDID_DRIVELABEL 以下へ
		case PPXCMDID_DRIVELABEL:
		case PPXCMDID_CSRCOMMENT:
		case PPXCMDID_COMBOIDNAME:
		case PPXCMDID_GETSUBID:
			PPxInfoFunc(ppxa->parent, cmdID, bufA);
			AnsiToUnicode(bufA, uptr->str, CMDLINESIZE);
			return PPXA_NO_ERROR;

		case PPXCMDID_REPORTSEARCH_FDATA:
			memcpy(bufA, (WIN32_FIND_DATAW *)uptr, 4 + 8 * 3 + 8);
			UnicodeToAnsi( ((WIN32_FIND_DATAW *)uptr)->cFileName,
					((WIN32_FIND_DATAA *)bufA)->cFileName, MAX_PATH);
			UnicodeToAnsi( ((WIN32_FIND_DATAW *)uptr)->cAlternateFileName,
					((WIN32_FIND_DATAA *)bufA)->cAlternateFileName, 13);
			return PPxInfoFunc(ppxa->parent, PPXCMDID_REPORTSEARCH_FDATA, bufA);

		case PPXCMDID_COMBOGETPANE:
		case PPXCMDID_SETPOPLINE:
		case PPXCMDID_EXTREPORTTEXT:
		case PPXCMDID_CSRSETCOMMENT:
		case PPXCMDID_REPORTPPCCOLUMN:
		case PPXCMDID_REPORTSEARCH:
		case PPXCMDID_REPORTSEARCH_FILE:
		case PPXCMDID_REPORTSEARCH_DIRECTORY:
			UnicodeToAnsi(uptr->str, bufA, sizeof(bufA));
			return PPxInfoFunc(ppxa->parent, cmdID, bufA);

		case PPXCMDID_ENTRYSETCOMMENT:
			UnicodeToAnsi((WCHAR *)&uptr->nums[1], bufA + 4, sizeof(bufA) - 4);
			*((long *)bufA) = uptr->nums[0];
			return PPxInfoFunc(ppxa->parent, cmdID, bufA);

		case PPXCMDID_ENTRYFROMNAME: {
			DWORD_PTR result;
			((PPXUPTR_ENTRYINFOA *)bufA)->result = bufA + 16;
			UnicodeToAnsi(uptr->entryinfo.result, bufA + 16, VFPS);
			result = PPxInfoFunc(ppxa->parent, cmdID, bufA);
			uptr->entryinfo.index = ((PPXUPTR_ENTRYINFOA *)bufA)->index;
			return result;
		}

		case PPXCMDID_ENTRYEXTDATA_SETDATA: {
			WCHAR *wptr;
			DWORD_PTR result;
			DWORD id = ((ENTRYEXTDATASTRUCT *)uptr)->id;

			if ( (id < DFC_COMMENTEX) || (id > DFC_COMMENTEX_MAX) ){
				return PPxInfoFunc(ppxa->parent, cmdID, uptr);
			}

			wptr = (WCHAR *)((ENTRYEXTDATASTRUCT *)uptr)->data;
			UnicodeToAnsi(wptr, bufA, sizeof(bufA));
			((ENTRYEXTDATASTRUCT *)uptr)->data = (BYTE *)bufA;
			result = PPxInfoFunc(ppxa->parent, cmdID, uptr);
			((ENTRYEXTDATASTRUCT *)uptr)->data = (BYTE *)wptr;
			return result;
		}

		case PPXCMDID_ENTRYEXTDATA_GETDATA: {
			WCHAR *wptr;
			DWORD_PTR result;
			DWORD id = ((ENTRYEXTDATASTRUCT *)uptr)->id;

			if ( (id < DFC_COMMENTEX) || (id > DFC_COMMENTEX_MAX) ){
				return PPxInfoFunc(ppxa->parent, cmdID, uptr);
			}

			wptr = (WCHAR *)((ENTRYEXTDATASTRUCT *)uptr)->data;
			((ENTRYEXTDATASTRUCT *)uptr)->data = (BYTE *)bufA;
			result = PPxInfoFunc(ppxa->parent, cmdID, uptr);
			((ENTRYEXTDATASTRUCT *)uptr)->data = (BYTE *)wptr;
			if ( result == PPXA_INVALID_FUNCTION ) return result;

			AnsiToUnicode(bufA, wptr, ((ENTRYEXTDATASTRUCT *)uptr)->size / sizeof(WCHAR));
			return result;
		}

		case PPXCMDID_ENTRYINSERTMSG:
		case PPXCMDID_ENTRYINSERT:
			UnicodeToAnsi((WCHAR *)uptr->dptrs[1], bufA + sizeof(void *) * 2, sizeof(bufA) - sizeof(void *) * 2);
			*((LONG_PTR *)bufA) = uptr->dptrs[0];
			*((LONG_PTR *)bufA + 1) = (LONG_PTR)(bufA + sizeof(void *) * 2);
			return PPxInfoFunc(ppxa->parent, cmdID, bufA);

		case PPXCMDID_ENTRYINFO: {
			size_t length;
			WCHAR *extbufW;

			((PPXUPTR_ENTRYINFOA *)bufA)->index = uptr->entryinfo.index;
			PPxInfoFunc(ppxa->parent, cmdID, bufA);

			length = AnsiToUnicode(((PPXUPTR_ENTRYINFOA *)bufA)->result, NULL, 0) + 1;
			extbufW = HeapAlloc(DLLheap, 0, length * sizeof(WCHAR));
			uptr->entryinfo.result = extbufW;
			if ( extbufW != NULL ){
				AnsiToUnicode(((PPXUPTR_ENTRYINFOA *)bufA)->result, extbufW, length);
			}
			HeapFree(ProcHeap, 0, ((PPXUPTR_ENTRYINFOA *)bufA)->result);
			return PPXA_NO_ERROR;
		}

		case PPXCMDID_MESSAGE: {
			char *extbuf;

			extbuf = UnicodeToAnsiLong(uptr->str, bufA, sizeof(bufA));
			PMessageBox(ppxa->info.hWnd, extbuf, NULL, MB_OK | MB_PPX_PLAIN_TEXT);
			if ( extbuf != bufA ) HeapFree(DLLheap, 0, extbuf);
			return PPXA_NO_ERROR;
		}

		case PPXCMDID_DEBUGLOG: {
			char *extbuf;

			extbuf = UnicodeToAnsiLong(uptr->str, bufA, sizeof(bufA));
			XMessage(NULL, NULL, XM_DbgLOG, T("%s"), extbuf);
			if ( extbuf != bufA ) HeapFree(DLLheap, 0, extbuf);
			return PPXA_NO_ERROR;
		}

		case PPXCMDID_REPORTTEXT: {
			char *extbuf;
			DWORD_PTR result;

			extbuf = UnicodeToAnsiLong(uptr->str, bufA, sizeof(bufA));

			if ( PPxInfoFunc(ppxa->parent, cmdID, extbuf) == PPXCMDID_EXTREPORTTEXT_LOG ){
				result = PPXCMDID_EXTREPORTTEXT_LOG;
			}else{
				PMessageBox(ppxa->info.hWnd, extbuf, NULL, MB_OK | MB_PPX_PLAIN_TEXT);
				result = PPXCMDID_EXTREPORTTEXT_CLOSE;
			}
			if ( extbuf != bufA ) HeapFree(DLLheap, 0, extbuf);
			return result;
		}
#else
		case PPXCMDID_MESSAGE:
			PMessageBox(ppxa->info.hWnd, uptr->strW, NULL, MB_OK | MB_PPX_PLAIN_TEXT);
			return PPXA_NO_ERROR;

		case PPXCMDID_DEBUGLOG:
			XMessage(NULL, NULL, XM_DbgLOG, T("%s"), uptr->strW);
			return PPXA_NO_ERROR;

		case PPXCMDID_REPORTTEXT:
			if ( PPxInfoFunc(ppxa->parent, cmdID, uptr->strW) == PPXCMDID_EXTREPORTTEXT_LOG ){
				return PPXCMDID_EXTREPORTTEXT_LOG;
			}else{
				PMessageBox(ppxa->info.hWnd, uptr->strW, NULL, MB_OK | MB_PPX_PLAIN_TEXT);
				return PPXCMDID_EXTREPORTTEXT_CLOSE;
			}
#endif

		case PPXCMDID_CHARTYPE:
#ifdef UNICODE
			return 1;
#else
			return 0;
#endif

		case PPXCMDID_VERSION:
			return (VersionH * 10000 + VersionM * 1000 + VersionL * 100 + VersionP);

		case PPXCMDID_GETKEYNAME:
#ifdef UNICODE
			PutKeyCode(uptr->str, uptr->key);
#else
			PutKeyCode(bufA, uptr->key);
			AnsiToUnicode(bufA, (WCHAR *)uptr, 64);
#endif
			return PPXA_NO_ERROR;

		case PPXCMDID_GETCONTROLVARIABLEDATA: {
			ThSTRUCT *thSV = (ThSTRUCT *)PPxInfoFunc(ppxa->parent, PPXCMDID_GETCONTROLVARIABLESTRUCT, NULL);
			if ( thSV == NULL ) return PPXA_INVALID_FUNCTION;
			return InfoFunc_GetVariableData(thSV, uptr);
		}

		case PPXCMDID_SETCONTROLVARIABLEDATA: {
			ThSTRUCT *thSV = (ThSTRUCT *)PPxInfoFunc(ppxa->parent, PPXCMDID_GETCONTROLVARIABLESTRUCT, NULL);
			if ( thSV == NULL ) return PPXA_INVALID_FUNCTION;
			InfoFunc_SetVariableData(thSV, uptr);
			return PPXA_NO_ERROR;
		}

		case PPXCMDID_GETWNDVARIABLEDATA: {
			ThSTRUCT *thWndSV = (ThSTRUCT *)PPxInfoFunc(ppxa->parent, PPXCMDID_GETWNDVARIABLESTRUCT, NULL);
			// if ( thWndSV == NULL ) thWndSV = &ProcessStringValue; 不要
			return InfoFunc_GetVariableData(thWndSV, uptr);
		}

		case PPXCMDID_SETWNDVARIABLEDATA: {
			ThSTRUCT *thWndSV = (ThSTRUCT *)PPxInfoFunc(ppxa->parent, PPXCMDID_GETWNDVARIABLESTRUCT, NULL);
			// if ( thWndSV == NULL ) thWndSV = &ProcessStringValue; 不要
			InfoFunc_SetVariableData(thWndSV, uptr);
			return PPXA_NO_ERROR;
		}

		case PPXCMDID_GETVARIABLEDATA:
			return InfoFunc_GetVariableData(&ppxa->Z->StringVariable, uptr);

		case PPXCMDID_SETVARIABLEDATA:
			InfoFunc_SetVariableData(&ppxa->Z->StringVariable, uptr);
			return PPXA_NO_ERROR;

		case PPXCMDID_GETPROCVARIABLEDATA:
			return InfoFunc_GetVariableData(NULL, uptr);

		case PPXCMDID_SETPROCVARIABLEDATA:
			InfoFunc_SetVariableData(&ProcessStringValue, uptr);
			return PPXA_NO_ERROR;

		case PPXCMDID_LONG_RESULT: {
			SIZE32_T textsize;

			if ( ppxa->Z == NULL ) return PPXA_INVALID_FUNCTION;
#ifdef UNICODE
			textsize = strlenW(uptr->str) + 1;
			if ( StoreLongParam(ppxa->Z, textsize) == FALSE ) return PPXA_INVALID_FUNCTION;
			textsize *= sizeof(WCHAR);
			if ( ThSize(&ppxa->Z->ExtendDst, textsize) == FALSE ){
				return PPXA_INVALID_FUNCTION;
			}
			memcpy(ppxa->Z->ExtendDst.bottom + ppxa->Z->ExtendDst.top, uptr->str, textsize);
			ZQuotationDoubler_Ext(ppxa->Z);
#else
			textsize = UnicodeToAnsi(uptr->strW, NULL, 0);
			if ( StoreLongParam(ppxa->Z, textsize) == FALSE ) return PPXA_INVALID_FUNCTION;
			if ( ThSize(&ppxa->Z->ExtendDst, textsize) == FALSE ){
				return PPXA_INVALID_FUNCTION;
			}
			textsize = UnicodeToAnsi(uptr->strW, ppxa->Z->ExtendDst.bottom + ppxa->Z->ExtendDst.top, textsize);
			if ( textsize > 0 ) ZQuotationDoubler_Ext(ppxa->Z);
#endif
			return PPXA_NO_ERROR;
		}
		case PPXCMDID_PPLIBHANDLE:
			return (DWORD_PTR)DLLhInst;
		case PPXCMDID_PPLIBPATH:
			strcpyToW(uptr->strW, DLLpath, MAX_PATH);
			return PPXA_NO_ERROR;
		case PPXCMDID_NEWAPPINFO:
			return InfoFunc_NewAppInfo(ppxa, (PPXCMDID_NEWAPPINFO_STRUCT *)uptr);
		case PPXCMDID_REQUIRE_CLOSETHREAD: {
			THREADSTRUCT *threadinfo = GetCurrentThreadInfo();
			if ( threadinfo != NULL ){
				setflag(threadinfo->flag, XTHREAD_REPORTTHREAD);
				return PPXA_NO_ERROR;
			}
			return ERROR_CANTWRITE;
		}

		default:
			return PPxInfoFunc(ppxa->parent, cmdID, uptr);
	}
}

void FreePPxModule(void)
{
	MODULESTRUCT *mdll;
	int i;
	PPXAPPINFOW info = {NULL, L"", L"", NULL};
	PPXMODULEPARAM module;

	mdll = ppxmodule_list;
	ppxmodule_list = NULL;
	if ( mdll == NULL ) return;

	module.info = NULL;
	i = ppxmodule_count;
	ppxmodule_count = 0;
	for ( ; i > 0 ; i--, mdll++ ){
		if ( mdll->types == MODULE_NOLOAD ) continue;
		if ( mdll->hDLL != NULL ){
			if ( (mdll->ModuleEntry != NULL) &&
				 (mdll->types & PPMTYPEFLAGS(PPXMEVENT_CLEANUP)) ){
				mdll->ModuleEntry(&info, PPXMEVENT_CLEANUP, module);
			}
			mdll->types = MODULE_NOLOAD;
			FreeLibrary(mdll->hDLL);
		}
	}
	ThFree(&Thmodule);
	ThFree(&Thmodule_str);
	ppxmodule_count = -1;
	UseCompListModule = -1;
	return;
}

void LoadModuleList(void)
{
	MODULESTRUCT mdll;
	HANDLE hFF; 		// FindFile 用ハンドル
	WIN32_FIND_DATA ff;	// ファイル情報
	TCHAR dir[MAX_PATH];
	int modulecount = 0;

	EnterCriticalSection(&ThreadSection);
	if ( ppxmodule_count < 0 ){ // 別スレッドで処理されていない
		ThInit(&Thmodule);
		ThInit(&Thmodule_str);
		CatPath(dir, DLLpath, T("PPX*.DLL"));
										// 検索 -------------------------------
		hFF = FindFirstFileL(dir, &ff);
		if ( hFF != INVALID_HANDLE_VALUE ){
			do{
//				0.42 以前の DLL 名を除外する設定
//				if ( !tstricmp(ff.cFileName, T("PPXLIB32.DLL")) ) continue;
				mdll.hDLL = NULL;
				mdll.types = MAX32; // 仮に全機能を読み込み可能に
				mdll.time = 0;
				GetCustTable(P_moduleStr, ff.cFileName, &mdll.types, sizeof(DWORD) * 2);
				if ( mdll.time != ff.ftLastWriteTime.dwHighDateTime ){
					mdll.types = MAX32; // 再取得
				}
				mdll.time = ff.ftLastWriteTime.dwHighDateTime;
				mdll.DllNameOffset = Thmodule_str.top;
				ThAddString(&Thmodule_str, ff.cFileName);
				ThAppend(&Thmodule, &mdll, sizeof mdll);
				modulecount++;
			} while ( IsTrue(FindNextFile(hFF, &ff)) );
			FindClose(hFF);
		}
		if ( Thmodule.bottom == NULL ) modulecount = 0; // モジュールがない
		ppxmodule_list = (MODULESTRUCT *)Thmodule.bottom;
		ppxmodule_count = modulecount;
	}
	LeaveCriticalSection(&ThreadSection);
}

BOOL LoadModuleFile(HWND hWnd, MODULESTRUCT *mdll, DWORD types)
{
	ERRORCODE result;
	TCHAR buf[VFPS];

	if ( !(mdll->types & types) ) return FALSE;
	EnterCriticalSection(&ThreadSection);
	if ( mdll->hDLL != NULL ){
		LeaveCriticalSection(&ThreadSection);
		return TRUE;
	}

	CatPath(buf, DLLpath, (TCHAR *)(Thmodule_str.bottom + mdll->DllNameOffset));
	mdll->hDLL = LoadLibraryTry(buf);
	if ( mdll->hDLL == NULL ) goto loaderror;

	if ( tstricmp((TCHAR *)(Thmodule_str.bottom + mdll->DllNameOffset),
			ValueX3264(T("PPXETS32.DLL"), T("PPXETS64.DLL"))) == 0 ){
		// ETC module は旧版だとマルチスレッド対応していないので使用しない
		if ( FindResource(mdll->hDLL, MAKEINTRESOURCE(PPXRCDATA_CUSTOMIZE_LIST), RT_RCDATA) == NULL ){
			FreeLibrary(mdll->hDLL);
			mdll->hDLL = NULL;
			mdll->types = MODULE_NOLOAD;
			LeaveCriticalSection(&ThreadSection);
			return FALSE;
		}
	}

	mdll->ModuleEntry = (tagCommandModuleEntry)
			GetProcAddress(mdll->hDLL, "ModuleEntry");
	if ( mdll->ModuleEntry != NULL ){
		PPXMODULEPARAM module;
		PPXMINFOSTRUCT pis;

		pis.infotype = 0;
		pis.copyright = (WCHAR *)buf; // 渡される大きさは、VFPS(A),VFPS/2(U)。どちらもMAX_PATH以上
		pis.typeflags = MAX32;
		module.info = &pis;
		if ( mdll->ModuleEntry(NULL, PPXM_INFORMATION, module) != PPXMRESULT_SKIP ){
			if ( mdll->types != pis.typeflags ){
				mdll->types = pis.typeflags;
				SetCustTable(P_moduleStr, (TCHAR *)(Thmodule_str.bottom + mdll->DllNameOffset), &mdll->types, sizeof(DWORD) * 2);
			}
		}
		LeaveCriticalSection(&ThreadSection);
		return TRUE;
	}
#ifndef _WIN64
	mdll->OldModuleEntry = (tagOldCommandModuleEntry)
			GetProcAddress(mdll->hDLL, "_ModuleEntry");
	if ( mdll->OldModuleEntry != NULL ){
		LeaveCriticalSection(&ThreadSection);
		return TRUE;
	}
#endif
	FreeLibrary(mdll->hDLL);
	mdll->hDLL = NULL;
loaderror:
	result = GetLastError();
	mdll->types = MODULE_NOLOAD;
	LeaveCriticalSection(&ThreadSection);

	if ( (result != ERROR_BAD_EXE_FORMAT) && (result != ERROR_GEN_FAILURE) ){
		XMessage(hWnd, NULL, XM_NsERRd, T("%s load error(%d)"),
				(TCHAR *)(Thmodule_str.bottom + mdll->DllNameOffset),
				result);
	}
	return FALSE;
}

UTCHAR GetParameter(LPCTSTR *commandline, TCHAR *param, size_t paramlen)
{
	const TCHAR *src;
	TCHAR *dest, *destmax;
	UTCHAR firstcode, code;

	firstcode = SkipSpace(commandline);
	if ( (firstcode == '\0') || (firstcode == ',') ){ // パラメータ無し
		*param = '\0';
		return firstcode;
	}
	dest = param;
	destmax = dest + paramlen - 1;
	if ( firstcode == '\"' ){
		const TCHAR *ptr, *ptrfirst, *ptrlast;
		TCHAR *dest;

		dest = param;
		ptrfirst = ptr = *commandline + 1;
		for ( ; ; ){
			TCHAR code;

			code = *ptr;
			if ( (code == '\0') /* || (code == '\r') || (code == '\n')*/ ){
				ptrlast = ptr;
				break;
			}
			if ( code != '\"' ){
				ptr++;
				continue;
			}
			// " を見つけた場合の処理
			if ( *(ptr + 1) != '\"' ){	// "" エスケープ?
				ptrlast = ptr++; // 単独 " … ここで終わり
				break;
			}
			// エスケープ処理
			{
				size_t copysize;

				copysize = ptr - ptrfirst + 1;
				if ( (dest + copysize) >= destmax ){ // buffer overflow?
					// " が 1文字エスケープされるので ">=" でok
					ptrlast = ptr;
					break;
				}
				memcpy(dest, ptrfirst, TSTROFF(copysize));
				dest += copysize;
				ptrfirst = (ptr += 2); // " x 2
				continue;
			}
		}
		*commandline = ptr;
		{
			size_t ptrsize;

			ptrsize = ptrlast - ptrfirst;
			if ( (dest + ptrsize) > destmax ){ // buffer overflow?
				tstrcpy(param, T("<flow!!>"));
			}else{
				memcpy(dest, ptrfirst, TSTROFF(ptrsize));
				*(dest + ptrsize) = '\0';
			}
		}
		return *param;
	}
	src = *commandline + 1;
	code = firstcode;
	for ( ;; ){
		*dest++ = code;
		code = *src;
		if ( (dest >= destmax) || (code == ',') || // (code == ' ') ||
			 ((code < ' ') && ((code == '\0') /*|| (code == '\t') ||
							   (code == '\r') || (code == '\n')*/)) ){
			break;
		}
		src++;
	}
	while ( (dest > param) && (*(dest - 1) == ' ') ) dest--;
	*dest = '\0';
	*commandline = src;
	return firstcode;
}

struct libinfo {
	COMMANDMODULEINFOSTRUCT info;
	WCHAR Name[MAX_PATH], RegID[REGIDSIZE];
	EXECSTRUCT dummyZ;
};

void initnewinfo(COMMANDMODULEINFOSTRUCT *ppxa, struct libinfo *newinfo)
{
	// PPXAPPINFOFUNCTIONW
	newinfo->info.info.Function = (PPXAPPINFOFUNCTIONW)CommandModuleInfoFunc;
	newinfo->info.info.Name = newinfo->Name;
	newinfo->info.info.RegID = newinfo->RegID;
	newinfo->info.info.hWnd = ppxa->info.hWnd;
	strcpyW(newinfo->Name, ppxa->info.Name);
	strcpyW(newinfo->RegID, ppxa->info.RegID);

	newinfo->info.parent = &PPxDefInfo;

	newinfo->info.Z = NULL;
	/*
	&newinfo->dummyZ;
	newinfo->info.rawparam = NULL;

	newinfo->dummyZ.hWnd = NULL;
	newinfo->dummyZ.Info = &PPxDefInfo;
	newinfo->dummyZ.src = NilStr;
	newinfo->dummyZ.dst = newinfo->dummyZ.DstBuf;
	*/
}
/*
{
	struct libinfo newinfo;
	DWORD result;

	initnewinfo(ppxa, &newinfo);

	result = pns->ThreadEntry(&newinfo->info.info, pns->value);
	return result;
}
*/

DWORD_PTR InfoFunc_NewAppInfo(COMMANDMODULEINFOSTRUCT *ppxa, PPXCMDID_NEWAPPINFO_STRUCT *pns)
{
	struct libinfo *newinfo;

	newinfo = (struct libinfo *)HeapAlloc(ProcHeap, HEAP_ZERO_MEMORY, sizeof(struct libinfo));
	initnewinfo(ppxa, newinfo);

	if ( pns->flags & NEWAPPINFO_THREAD ){
		return 0;
	}else{
		return (DWORD_PTR)newinfo;
	}
}

int CommandModule(EXECSTRUCT *Z, const TCHAR *cmdparam)
{
	COMMANDMODULEINFOSTRUCT ppxa;
	PPXMCOMMANDSTRUCT command;
	PPXMODULEPARAM module;
	MODULESTRUCT *mdll;
	int result;

	const WCHAR *param;
	DWORD paramcount = 0;
	int i;
	WCHAR cmdmainbuf[CMDLINESIZE], *next;
	WCHAR *cmdbuf, *cmdaltbuf = NULL;
	size_t cmdlenzW;
	DWORD bufleft;

#ifndef UNICODE
	WCHAR regidW[REGIDSIZE], nameW[MAX_PATH];
	char paramtmpbuf[CMDLINESIZE], *parambuf = paramtmpbuf;
	DWORD parambuflen = CMDLINESIZE;

	AnsiToUnicode(Z->Info->Name, nameW, MAX_PATH);
	AnsiToUnicode(Z->Info->RegID, regidW, REGIDSIZE);
	#define PPXAINFONAME nameW
	#define PPXAINFOREGID regidW
#else
	#define PPXAINFONAME Z->Info->Name
	#define PPXAINFOREGID Z->Info->RegID
#endif
	if ( ppxmodule_count < 0 ) LoadModuleList();
	if ( ppxmodule_count <= 0 ) return PPXMRESULT_SKIP; // モジュールがない

	strcpyToW(cmdmainbuf, cmdparam, CMDLINESIZE); // arg(0) コマンド名
	cmdlenzW = strlenW(cmdmainbuf) + 1;
	cmdparam += tstrlen(cmdparam) + 1;
	if ( (cmdparam >= Z->DstBuf) &&
		 (cmdparam < (Z->DstBuf + CMDLINESIZE)) ){
		bufleft = CMDLINESIZE - cmdlenzW;
		cmdbuf = cmdmainbuf;
	}else{ // cmdparam が大きいときは、メモリ確保が必要
		bufleft = cmdlenzW + tstrlen(cmdparam) + 1; // MultiByte のときは多めにメモリ確保することになる
		#ifdef UNICODE
			cmdaltbuf = (WCHAR *)HeapAlloc(DLLheap, 0, bufleft * sizeof(WCHAR));
			if ( cmdaltbuf == NULL ) return PPXMRESULT_SKIP;
		#else
			cmdaltbuf = (WCHAR *)HeapAlloc(DLLheap, 0, bufleft * (sizeof(WCHAR) + sizeof(char)) );
			if ( cmdaltbuf == NULL ) return PPXMRESULT_SKIP;
			parambuf = (char *)(WCHAR *)(cmdaltbuf + bufleft);
			parambuflen = bufleft;
		#endif
		cmdbuf = cmdaltbuf;
		strcpyW(cmdaltbuf, cmdmainbuf); // arg(0) コマンド名
	}
	ppxa.rawparam = cmdparam;
	param = next = cmdbuf + cmdlenzW;
									// arg(1...) パラメータの切り出し
	if ( *cmdparam != '\0' ) for(;;){
		size_t len;

#ifdef UNICODE
		*next = '\0';
		GetParameter(&cmdparam, next, bufleft);
#else
		*parambuf = '\0';
		GetParameter(&cmdparam, parambuf, parambuflen);
		AnsiToUnicode(parambuf, next, bufleft);
#endif
		paramcount++;
		len = strlenW(next) + 1;
		next = next + len;
		bufleft -= len;
		if ( NextParameter(&cmdparam) == FALSE ) break;
	}
										// 検索と実行
	mdll = ppxmodule_list;
	module.command = &command;

	command.commandname = cmdbuf;
	command.commandhash = Z->command;
	command.param = param;
	command.paramcount = paramcount;
	command.resultstring = NULL;

	ppxa.info.Name = PPXAINFONAME;
	ppxa.info.RegID = PPXAINFOREGID;
	ppxa.info.Function = (PPXAPPINFOFUNCTIONW)CommandModuleInfoFunc;
	ppxa.info.hWnd = Z->hWnd;
	ppxa.parent = Z->Info;
	ppxa.Z = Z;

	for ( i = ppxmodule_count ; i ; i--, mdll++ ){
		if ( mdll->hDLL == NULL ){
			if ( LoadModuleFile(ppxa.info.hWnd, mdll, PPMTYPEFLAGS(PPXMEVENT_COMMAND)) == FALSE ){
				continue;
			}
		}
#ifdef _WIN64
		if ( mdll->ModuleEntry == NULL ) continue;
		result = mdll->ModuleEntry(&ppxa.info, PPXMEVENT_COMMAND, module);
#else
		if ( mdll->ModuleEntry != NULL ){
			result = mdll->ModuleEntry(&ppxa.info, PPXMEVENT_COMMAND, module);
		}else{
			result = mdll->OldModuleEntry(&ppxa.info, PPXMEVENT_COMMAND, module);
		}
#endif
		if ( result != PPXMRESULT_SKIP ) goto end;
	}
	result = PPXMRESULT_SKIP;
end:
	if ( cmdaltbuf != 0 ) HeapFree(DLLheap, 0, cmdaltbuf);
	return result;
}
#undef PPXAINFONAME
#undef PPXAINFOREGID

/*
void ToUtf8Function(EXECSTRUCT *Z, const char *param)
{
	WCHAR bufW[CMDLINESIZE];

	AnsiToUnicode(param, bufW, CMDLINESIZE);
	WideCharToMultiByteU8(CP_UTF8, 0, bufW, -1, Z->dst, VFPS, NULL, NULL);
	Z->dst += tstrlen(Z->dst);
}
*/

void TempFunction(EXECSTRUCT *Z, const TCHAR *param) // %*temp
{
	TCHAR name[VFPS];
	DWORD attr = FILE_ATTRIBUTE_LABEL;

	*Z->dst = '\0';
	GetCommandParameter(&param, name, TSIZEOF(name));
	if ( name[0] == '\0' ){
		if ( ProcTempPath[0] == '\0' ){
			MakeTempEntry(MAX_PATH, Z->dst, 0); // ProcTempPath を作成
		}else{
			CatPath(Z->dst, ProcTempPath, NilStr);
		}
	}else{
		if ( SkipSpace(&param) == ',' ){
			param++;
			if ( SkipSpace(&param) == 'd' ){
				attr = FILE_ATTRIBUTE_LABEL | FILE_ATTRIBUTE_DIRECTORY;
			}else if ( *param == 'f' ){
				attr = FILE_ATTRIBUTE_LABEL | FILE_ATTRIBUTE_NORMAL;
			}
		}
		tstrcpy(Z->dst, name);
		MakeTempEntry(MAX_PATH, Z->dst, attr); // ProcTempPath を作成
	}
	Z->dst += tstrlen(Z->dst);
}

void TreeFunction(EXECSTRUCT *Z, const TCHAR *param) // %*tree
{
	HWND hTreeWnd;
	HWND hParentWnd;
	POINT pos;
	MSG msg;
	DWORD X_tree[5];
	RECT deskbox;
	int dpi;

	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		PrintToConsole(T("** tree not support on console **\r\n"));
		Z->result = ERROR_CANCELLED;
		return;
	}

	*Z->dst = '\0';
	Z->result = ERROR_BUSY;
	if ( Z->hWnd == NULL ){
		hParentWnd = NULL;
	}else{
		hParentWnd = GetParentCaptionWindow(Z->hWnd);
	}
	if ( SkipSpace(&param) == '\0' ) param = T("1"); // 空欄なら M_pjump に

	InitVFSTree();
	GetPopupPoint(Z, &pos);

	// 初期大きさ調整
	dpi = GetMonitorDPI(hParentWnd);
	X_tree[3] = (300 * dpi) / DEFAULT_WIN_DPI;
	X_tree[4] = (400 * dpi) / DEFAULT_WIN_DPI;
	GetCustData(StrX_tree, X_tree, sizeof(X_tree));
//	if ( X_tree[3] < 100 ) X_tree[3] = 100; // Windows側で処理される
	if ( X_tree[4] < 100 ) X_tree[4] = 100;

	GetDesktopRect(hParentWnd, &deskbox);
	if ( (int)X_tree[3] >= (deskbox.right - deskbox.left) ){
		X_tree[3] = deskbox.right - deskbox.left;
	}
	if ( (pos.x + (int)X_tree[3]) > deskbox.right ){
		pos.x = deskbox.right - X_tree[3];
	}
	if ( pos.x < deskbox.left ){
		pos.x = deskbox.left;
	}

	if ( (int)X_tree[4] >= (deskbox.bottom - deskbox.top) ){
		X_tree[4] = deskbox.bottom - deskbox.top;
	}
	if ( (pos.y + (int)X_tree[4]) > deskbox.bottom ){
		pos.y = deskbox.bottom - X_tree[4];
	}
	if ( pos.y < deskbox.top ){
		pos.y = deskbox.top;
	}

	// 作成
	hTreeWnd = CreateWindow(TreeClassStr, ZGetTitleName(Z),
			WS_OVERLAPPEDWINDOW,
			pos.x, pos.y, X_tree[3], X_tree[4], NULL, NULL, NULL, 0);

	SendMessage(hTreeWnd, VTM_SETFLAG, (WPARAM)hParentWnd, (LPARAM)VFSTREE_MENU);
	SendMessage(hTreeWnd, VTM_TREECOMMAND, 0, (LPARAM)param);
	SendMessage(hTreeWnd, VTM_SETRESULT, (WPARAM)&Z->result, (LPARAM)Z->dst);
	ShowWindow(hTreeWnd, SW_SHOWNORMAL);

	for ( ; ; ){
		if ( (int)GetMessage(&msg, NULL, 0, 0) <= 0 ){
			DestroyWindow(hTreeWnd);
			break;
		}
		if ( DialogKeyProc(&msg) ) continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if ( Z->result != ERROR_BUSY ) break;
	}

	Z->dst += tstrlen(Z->dst);
}

void UseAppsFunction(EXECSTRUCT *Z, const TCHAR *param) // %*useapps
{
	TCHAR filepath[VFPS + 1];

	GetCommandParameter(&param, filepath + 1, VFPS);
	VFSFixPath(NULL, filepath + 1, GetZCurDir(Z), VFSFIX_FULLPATH | VFSFIX_REALPATH | VFSFIX_NOFIXEDGE);
	filepath[0] = '\1';
	GetAccessApplications(filepath, Z->dst);
	Z->dst += tstrlen(Z->dst);
}

void ScreenDpiFunction(EXECSTRUCT *Z, const TCHAR *param) // %*screendpi
{
	HWND hWnd;

	hWnd = GetPPxhWndFromID(Z->Info, &param, NULL);
	if ( (hWnd == NULL) || (hWnd == BADHWND) ) hWnd = Z->Info->hWnd;
	Z->dst = Numer32ToString(Z->dst, GetMonitorDPI(hWnd));
}

void SelectTextFunction(EXECSTRUCT *Z, const TCHAR *param) // %*selecttext
{
	TCHAR opt;
	LRESULT long_result;

	opt = SkipSpace(&param);
	*Z->dst = '\0';
	if ( Z->flags & XEO_CONSOLE ){
		Z->result = PPxInfoFunc32u(Z->Info, PPXCMDID_SELECTTEXT, Z->dst);
		long_result = 0;
	}else{
		long_result = SendMessage(Z->hWnd, WM_PPXCOMMAND, KE_seltext, (LPARAM)Z->dst);
	}
	if ( long_result >= 0x10000 ){
		*Z->dst = '\0';

		if ( IsTrue(StoreLongParam(Z, 0)) ){
			DWORD oldtop;
			oldtop = Z->ExtendDst.top;
			ThCatString(&Z->ExtendDst, (const TCHAR *)long_result);
			Z->ExtendDst.top = oldtop;;
			ZQuotationDoubler_Ext(Z);
		}
		HeapFree(ProcHeap, 0, (void *)long_result);
	}

	if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);

	if ( opt == 'u' ){
		char bufA[CMDLINESIZE], *srcA;
		TCHAR *dest, *maxptr;

#ifdef UNICODE
		WideCharToMultiByteU8(CP_UTF8, 0, Z->dst, -1, bufA, CMDLINESIZE, NULL, NULL);
#else
		WCHAR bufW[CMDLINESIZE];

		AnsiToUnicode(Z->dst, bufW, CMDLINESIZE);
		WideCharToMultiByteU8(CP_UTF8, 0, bufW, -1, bufA, CMDLINESIZE, NULL, NULL);
#endif
		srcA = bufA;
		dest = Z->dst;
		maxptr = dest + CMDLINESIZE - 300;
		while ( dest < maxptr ){
			BYTE c;

			c = (BYTE)*srcA++;
			if ( c == '\0' ) break;
			if ( IsalnumA(c) ){
				*dest++ = c;
			}else if ( c == ' ' ){
				*dest++ = '+';
			}else{
				dest = thprintf(dest, maxptr - dest, T("%%%02X"), c);
			}
		}
		*dest = '\0';
	}
	Z->dst += tstrlen(Z->dst);
}

void CountJobFunction(EXECSTRUCT *Z)
{
	int i, count = 0;
	JobX *job;

	job = Sm->Job;
	for ( i = 0 ; i < X_MaxJob ; i++, job++ ){
		if ( job->ThreadID != 0 ) count++;
	}
	Z->dst = Numer32ToString(Z->dst, count);
	return;
}

// %*maxlength
void SetMaxLengthFunction(EXECSTRUCT *Z, const TCHAR *param)
{
	Z->LongResultLen = GetDigitNumber32(&param);
	setflag(Z->status, ST_LONGRESULT);
}

// %*linkedpath
void GetLinkedPathFunction(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR buf[CMDLINESIZE], name[VFPS];

	GetCommandParameter(&param, buf, TSIZEOF(buf));

	if ( FAILED(GetLink(NULL, buf, name)) ){
		if ( GetReparsePath(buf, name) == 0 ) return;
	}
	Z->dst = tstpcpy(Z->dst, name);
	return;
}

// %*ppxlist
void GetPPxListFunction(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR buf[CMDLINESIZE];
	ThSTRUCT th;
	ShareX *sx;
#define LISTMODE_normal 0
#define LISTMODE_combolist 1
#define LISTMODE_combofilter 2
#define NUMMODE_num 0
#define NUMMODE_numandlist 1
#define NUMMODE_list 2
	int items = 0, listmode = LISTMODE_normal, nummode = NUMMODE_numandlist;
	DWORD useppclist = 0; // 現プロセスで使用しているPPcのID一覧
	TCHAR *pptr, filter = '\0', combofilter = '\0';

	ThInit(&th);

	// 個数表示モードを決定
	GetCommandParameter(&param, buf, TSIZEOF(buf));
	pptr = buf;
	if ( *pptr == '+' ){
		pptr++;
		nummode = NUMMODE_num;
	}else if ( *pptr == '-' ){
		pptr++;
		nummode = NUMMODE_list;
	}

	// フィルタを決定
	if ( (*pptr == '#') || ((*pptr == 'C') && ((*(pptr + 1) == '#'))) ){
		if ( *pptr == 'C' ) pptr++;
		combofilter = *(pptr + 1);
		if ( combofilter <= ' ' ){
			if ( PPxInfoFunc(Z->Info, PPXCMDID_COMBOIDNAME, buf) != 0 ){
				combofilter = (buf[0] != '\0') ? buf[2] : (TCHAR)'@';
			}else{
				filter = '\1'; // 列挙させない
			}
			listmode = LISTMODE_combofilter;
		}else if ( combofilter == '#' ){
			listmode = LISTMODE_combolist;
		}else{
			listmode = LISTMODE_combofilter;
		}
	}else if ( Isalpha(*pptr) ){
		filter = *pptr;
	}

	// 一体化窓列挙
	if ( ((filter == '\0') || (filter == 'C')) ){
		int i = 0, combomax = X_MaxComboID - 1;

		if ( listmode == LISTMODE_combofilter ){
			i = combomax = (int)(BYTE)(combofilter - 'A');
		}
		for ( ; i <= combomax ; i++ ){
			HWND hComboWnd;

			hComboWnd = Sm->ppc.hComboWnd[i];
			if ( (hComboWnd == NULL) || (hComboWnd == BADHWND) ){
				continue;
			}

			if ( listmode == LISTMODE_combolist ){
				thprintf(&th, 0, T("CB%c,"), i + 'A');
				items++;
			}else{ // LISTMODE_normal / LISTMODE_combofilter
				if ( hComboWnd == hProcessComboWnd ){
					items += GetPPxListFromCurrentCombo(NULL, GetPPxList_IdList, &th, &useppclist, NULL);
					continue;
				}
				items += GetPPxListFromProcessCombo(NULL, GetPPxList_IdList, &th, &useppclist, NULL, hComboWnd);
			}
		}
	}

	// 独立窓列挙
	if ( listmode == LISTMODE_normal ){
		int i;

		UsePPx();
		sx = Sm->P;
		for ( i = 0 ; i < X_Mtask ; i++, sx++ ){
			TCHAR ID;

			ID = sx->ID[0];
			if ( ID == '\0' ) continue;
			if ( (filter != '\0') && (ID != filter) ) continue;
			if ( (ID == 'C') && (useppclist & (1 << (sx->ID[2] - 'A'))) ){
				continue;
			}

			thprintf(&th, 0, T("%s,"), sx->ID);
			items++;
		}
		FreePPx();
	}

	// 結果出力
	if ( nummode == NUMMODE_num ){
		Z->dst = Numer32ToString(Z->dst, items);
	}else{
		if ( nummode == NUMMODE_numandlist ){
			Z->dst = Numer32ToString(Z->dst, items);
			*Z->dst++ = ',';
		}
		if ( th.bottom != NULL ){
			if ( th.top < 1 ){
				Z->dst = tstpcpy(Z->dst, (TCHAR *)th.bottom);
			}else if ( IsTrue(StoreLongParam(Z, th.top / sizeof(TCHAR))) ){
				ThCatString(&Z->ExtendDst, (TCHAR *)th.bottom);
			}
		}
	}
	ThFree(&th);
}

// %*menu
void MenuFunction(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR menuname[0x100], def[0x100];
	int menuflags = 0;

	GetCommandParameter(&param, menuname, TSIZEOF(menuname));
	if ( SkipSpace(&param) != ',' ){
		def[0] = '\0';
	}else{
		param++;
		if ( SkipSpace(&param) == '!' ){
			param++;
			menuflags = MENUFLAG_SELECT;
		}
		GetCommandParameter(&param, def, TSIZEOF(def));
	}
	MenuCommand(Z, menuname, def, menuflags);
}

// %*name
void GetNameFunction(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR buf[CMDLINESIZE], name[VFPS+2], code;
	const TCHAR *p;
	DWORD flag;

	GetCommandParameter(&param, buf, TSIZEOF(buf));
	p = buf;
	flag = GetFmacroOption(&p);
	if ( *p != '\0' ) goto error;
	if ( SkipSpace(&param) != ',' ) goto error;
	param++;
	code = SkipSpace(&param);
	if ( (code == '\"') || ((code != '\0') && (code != ',')) ){
		GetCommandParameter(&param, buf, TSIZEOF(buf));
	}else{
		ZGetName(Z, buf, 'C');
	}

	p = GetZCurDir(Z);
	if ( SkipSpace(&param) == ',' ){	// 基準ディレクトリ指定
		param++;
		GetCommandParameter(&param, name, TSIZEOF(name));
		VFSFullPath(NULL, name, p);
		p = name;
	}

	if ( flag & FMOPT_REALPATH ){
		VFSFixPath(NULL, buf, p, VFSFIX_FULLPATH | VFSFIX_REALPATH | VFSFIX_NOFIXEDGE);
	}else{
		VFSFullPath(NULL, buf, p);
	}
	if ( flag & FMOPT_UNIQUE ) GetUniqueEntryName(buf);
	GetFmacroString(flag, buf, name);
	if ( flag & (FMOPT_SEP_SLASH | FMOPT_SEP_BACKSLASH) ){
		if ( flag & FMOPT_SEP_SLASH ){
		#ifdef UNICODE
			tstrreplace(name, T("\\"), T("/"));
		#else
			TCHAR *ptr;

			for ( ptr = name; *ptr != '\0'; ){
				if ( *ptr == '\\' ) *ptr = '/';
				if ( Iskanji(*ptr++) ) ptr++;
			}
		#endif
		}else{
			tstrreplace(name, T("/"), T("\\"));
		}
	}
	Z->dst = tstpcpy(Z->dst, name);
	return;

error:
	XMessage(NULL, NULL, XM_GrERRld, T("%*name(options,filename)"));
	Z->result = ERROR_INVALID_PARAMETER;
	return;
}

// %*now
void GetNowFunction(EXECSTRUCT *Z, const TCHAR *param)
{
	Z->dst += GetNowTime(Z->dst, (*param == 'd') );
}

// %*InsertSource / %*InsertValue
void InsertFileFunction(EXECSTRUCT *Z, const TCHAR *param, int value)
{
	TCHAR filename[VFPS];
	TCHAR *image, *text, *maxptr;
	TCHAR buf[CMDLINESIZE], *more, code;
	BOOL trim = TRUE;

	while ( '\0' != (code = GetOptionParameterML(&param, buf, CONSTCAST(TCHAR **, &more), CMDLINESIZE)) ){
		if ( code != '-' ){
			tstplimcpy(filename, buf, VFPS);
			continue;
		}
		if ( !tstrcmp(buf + 1, T("NOTRIM")) ){
			trim = FALSE;
			continue;
		}
	}

	VFSFixPath(NULL, filename, GetZCurDir(Z), VFSFIX_FULLPATH | VFSFIX_REALPATH );
	Z->result = LoadTextData(filename, &image, &text, &maxptr, 0);
	if ( Z->result != NO_ERROR ){
		ErrorPathBox(Z->hWnd, NULL, filename, Z->result);
		return;
	}

	if ( trim && (text < maxptr) ){
		if ( *(maxptr - 1) == '\n' ) maxptr--;
		if ( *(maxptr - 1) == '\r' ) maxptr--;
		*maxptr = '\0';
	}

	if ( value ){
		SetLongDest(Z, text, -1);
	}else{
		InsertSrc(Z, text);
	}
	HeapFree(ProcHeap, 0, image);
}

void InputFunctionOption(EXECSTRUCT *Z, TINPUT *tinput, TCHAR *param, TCHAR *titlebuf)
{
	TCHAR shortparam[CMDLINESIZE]; // param の待避場所
	TCHAR shortone[CMDLINESIZE]; // param から抽出した１項目
	TCHAR *paramptr, *oneptr;
	TINPUT_EDIT_OPTIONS options;
	const TCHAR *srcptr, *more;
	size_t paramlen;
	UTCHAR code;

	// param を paramptr へ待避 (param は tinput->buff と被るため)
	paramlen = tstrlen(param) + 1;
	if ( paramlen >= TSIZEOF(shortparam) ){
		paramptr = HeapAlloc(DLLheap, 0, TSTROFF(paramlen) * 2);
		if ( paramptr == NULL ){
			paramlen = TSIZEOF(shortparam);
			param[paramlen - 1] = '\0';
			paramptr = shortparam;
			oneptr = shortone;
		}else{
			oneptr = paramptr + paramlen;
		}
	}else{
		paramptr = shortparam;
		oneptr = shortone;
	}
	tstrcpy(paramptr, param);

	*tinput->buff = '\0';
	srcptr = paramptr;
	while ( '\0' != (code = GetOptionParameterML(&srcptr, oneptr, CONSTCAST(TCHAR **, &more), paramlen)) ){
		if ( code != '-' ){
			tstplimcpy(tinput->buff, oneptr, tinput->size);
			continue;
		}
		if ( !tstrcmp(oneptr + 1, T("TEXT")) ){
			tstplimcpy(tinput->buff, more, tinput->size);
			continue;
		}
		if ( !tstrcmp(oneptr + 1, T("K")) ){
			tinput->StringVariable = &Z->StringVariable;
			setflag(tinput->flag, TIEX_EXECPRECMD);
			if ( *more != '\0' ){
				ThSetString(tinput->StringVariable, T("Input_FirstCmd"), more);
				continue;
			}else{
				ThSetString(tinput->StringVariable, T("Input_FirstCmd"), srcptr);
				break;
			}
		}
		if ( !tstrcmp(oneptr + 1, T("ID")) ){
			tinput->StringVariable = &Z->StringVariable;
			setflag(tinput->flag, TIEX_ID);
			ThSetString(tinput->StringVariable, T("Input_ID"), more);
			continue;
		}
		if ( !tstrcmp(oneptr + 1, T("LEAVECANCEL")) ){
			setflag(tinput->flag, TIEX_LEAVECANCEL);
			continue;
		}
		if ( !tstrcmp(oneptr + 1, T("FORPATH")) ){
			setflag(tinput->flag, TIEX_FIXFORPATH);
			continue;
		}
		if ( !tstrcmp(oneptr + 1, T("FORDIGIT")) ){
			setflag(tinput->flag, TIEX_FIXFORDIGIT);
			continue;
		}
		if ( !tstrcmp(oneptr + 1, T("TITLE")) ){
			tinput->title = titlebuf;
			tstplimcpy(titlebuf, more, CMDLINESIZE);
			continue;
		}
		if ( !tstrcmp(oneptr + 1, T("MODE")) ){
			if ( FALSE == GetEditMode((const TCHAR **)&more, &options) ){
				break;
			}
			tinput->hRtype = options.hist_readflags;
			tinput->hWtype = HistWriteTypeflag[options.hist_writetype];
			tinput->flag =
					(tinput->flag & ~(TIEX_REFTREE | TIEX_SINGLEREF)) |
					TinputTypeflags[options.hist_writetype] |
					TIEX_Z_HIST_SETTINGED;
			SetTInputOptionFlags(tinput, &options);
			continue;
		}
		if ( !tstrcmp(oneptr + 1, T("MULTI")) ){
			setflag(tinput->flag, TIEX_LINE_MULTILINE);
		}
		if ( !tstrcmp(oneptr + 1, T("SELECT")) ){
			if ( *more == 'i' ){
				setflag(tinput->flag, TIEX_INSTRSEL);
			}else{
				setflag(tinput->flag, TIEX_USESELECT);
				if ( (*more == '\0') || (*more == 'a') ){
					tinput->firstC = 0;
					tinput->lastC = EC_LAST;
				}else if ( *more == 'f' ){
					tinput->firstC = tinput->lastC = -2;
					if ( *(more + 1) == 's' ) tinput->firstC = 0;
				}else if ( *more == 'e' ){
					tinput->firstC = tinput->lastC = -3;
					if ( *(more + 1) == 's' ) tinput->lastC = EC_LAST;
				}else if ( *more == 'l' ){
					tinput->firstC = tinput->lastC = EC_LAST;
				}else if ( *more == 't' ){
					tinput->firstC = tinput->lastC = 0;
				}else {
					tinput->firstC = tinput->lastC = GetDigitNumber32((const TCHAR **)&more);
					if ( SkipSpace((const TCHAR **)&more) == ',' ){
						more++;
						tinput->lastC = GetDigitNumber32((const TCHAR **)&more);
					}
				}
			}
		}
		#if 0
		if ( !tstrcmp(oneptr + 1, T("BANNER")) ){ // 複数行表示の時は使用できない
			SendDlgItemMessage(hDlg, IDE_INPUT_LINE, EM_SETCUEBANNER, 1, (LPARAM)more); // MultiByte 版の時は、UNICODE化が必要
			continue;
		}
		#endif
	}
	if ( paramptr != shortparam ) HeapFree(DLLheap, 0, paramptr);
}

// %*input
void InputFunction(EXECSTRUCT *Z, TCHAR *param)
{
	TINPUT tinput;
	TCHAR titlebuf[CMDLINESIZE];
	DWORD hash C4701CHECK;

	if ( Z->flags & (XEO_DISPONLY | XEO_NOEDIT) ) return;

	tinput.title = ZGetTitleName(Z);
	tinput.flag = TIEX_USEINFO;

	if ( !(Z->status & ST_LONGRESULT) ){
		tinput.buff = Z->dst;
		tinput.size = CMDLINESIZE - ToSIZE32_T(Z->dst - Z->DstBuf) - 1;
	}else{
		if ( StoreLongParam(Z, 0) == FALSE ){
			Z->result = RPC_S_STRING_TOO_LONG;
			return;
		}
		tinput.size = GetLongParamMaxLen(Z) - (Z->ExtendDst.top / sizeof(TCHAR));
		if ( tinput.size >= 0x8000 ) tinput.size = 0x7fff;
		if ( ThSize(&Z->ExtendDst, tinput.size * sizeof(TCHAR) ) == FALSE ){
			Z->result = RPC_S_STRING_TOO_LONG;
			return;
		}
		tinput.buff = (TCHAR *)(Z->ExtendDst.bottom + Z->ExtendDst.top);
//		param = tinput.buff + tstrlen(tinput.buff) + 1;
	}

	InputFunctionOption(Z, &tinput, param, titlebuf);
	if ( (Z->status & ST_USECACHE) &&
		 (Z->edit.cache.hash == (hash = GetCacheHash(Z))) ){
		// キャッシュが使用できる
		ThGetString(&Z->StringVariable, EditCache_ValueName, tinput.buff, tinput.size);
	}else{
		if ( ZTinput(Z, &tinput) != FALSE ){
			if ( Z->status & ST_USECACHE ){
				Z->edit.cache.hash = hash; // C4701ok
				ThSetString(&Z->StringVariable, EditCache_ValueName, tinput.buff);
			}
		}
	}
	if ( !(Z->status & ST_LONGRESULT) ){
		if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
		Z->dst = tinput.buff + tstrlen(tinput.buff);
	}else{
		ZQuotationDoubler_Ext(Z);
	}
	if ( Z->status & ST_USECACHE ){
		resetflag(Z->status, ST_USECACHE);
		Z->edit.EdOffset = -1;
	}
}

// %*noq / %*noquotation
void NoQuotationFunction(EXECSTRUCT *Z, TCHAR *param)
{
	TCHAR *dst;
	const TCHAR *paramptr;
	size_t dstsizelen;

	if ( Z->status & ST_EXTENDDST ){
		dst = (TCHAR *)(Z->ExtendDst.bottom + Z->ExtendDst.top);
		dstsizelen = (Z->ExtendDst.size - Z->ExtendDst.top - sizeof(TCHAR)) * sizeof(TCHAR);
	}else{
		dst = Z->dst;
		dstsizelen = CMDLINESIZE;
	}

	paramptr = param;
	if ( SkipSpace(&paramptr) != '\"' ){
		GetCommandParameter((const TCHAR **)&param, dst, dstsizelen);
	}else{ // param 全体が " で括られているかを調査
		TCHAR chr;

		paramptr++;
		for (;;){
			chr = *paramptr++;
			if ( chr == '\0' ){ // 括られていない状態で終了
				chr = '\1';
				break;
			}
			if ( chr == '\"' ){
				if ( *paramptr == '\"' ){ // "" エスケープ
					paramptr++;
					continue;
				}
				// 括りが終わったので、後続がないか確認
				chr = SkipSpace(&paramptr);
				if ( chr == '\0' ){ // param 全体が " で括られている
					GetCommandParameter((const TCHAR **)&param, dst, dstsizelen);
				}
				break;
			}
		}
		if ( chr != '\0' ){ // param 全体が " で括られていない→そのまま出力
			paramptr = param;
			SkipSpace(&paramptr);
			for (;;){
				chr = *paramptr++;
				*dst++ = chr;
				if ( chr == '\0' ) break;
				if ( --dstsizelen <= 0 ){
					*dst = '\0';
					break;
				}
			}
		}
	}

	if ( Z->status & ST_EXTENDDST ){
		Z->ExtendDst.top += tstrlen((TCHAR *)(Z->ExtendDst.bottom + Z->ExtendDst.top)) * sizeof(TCHAR);
	}else{
		Z->dst += tstrlen(Z->dst);
	}
}

TCHAR *PPcustCDumpTextItemPtr(TCHAR *param)
{
	TCHAR *fp = param;
	if ( *fp == '\t' ) fp++;
	if ( (*fp == '=') || (*fp == ',') ) fp++;
	if ( *fp == ' ' ) fp++;
	return fp;
}

void GetCustFunction(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR buf[CMDLINESIZE], str[VFPS+2], *sub, *bufptr, *src, *src_q;

	GetCommandParameter(&param, str, TSIZEOF(str));
	sub = tstrchr(str, ':');
	if ( sub != NULL ) *sub++ = '\0';

	bufptr = buf;
	PPcustCDumpText(str, sub, &bufptr);

	src = PPcustCDumpTextItemPtr(bufptr);
	if ( !Z->func.quotation || ((src_q = tstrchr(src, '\"')) == NULL) ){
		if ( bufptr == buf ){
			Z->dst = tstpcpy(Z->dst, src);
		}else{
			if ( IsTrue(StoreLongParam(Z, 0)) ){
				ThCatString(&Z->ExtendDst, src);
				HeapFree(ProcHeap, 0, bufptr);
			}
		}
	}else{ // Z->func.quotation
		DWORD top, count;

		if ( bufptr == buf ){ // Z->DstBuf に納まる場合
			for (;;){
				TCHAR code;

				code = *src;
				if ( code == '\0' ) return;
				if ( Z->dst >= (Z->DstBuf + CMDLINESIZE - 2) ) break;
				if ( code == '\"' ) *Z->dst++ = code;
				*Z->dst++ = code;
				src++;
			}
		}

		if ( StoreLongParam(Z, 0) == FALSE ) return;

		top = Z->ExtendDst.top;
		ThCatString(&Z->ExtendDst, src);
		if ( bufptr != buf ){
			HeapFree(ProcHeap, 0, bufptr);
		}
		// " による保存領域の増分を算出
		count = 1 * sizeof(TCHAR);
		for (;;){
			src_q = tstrchr(src_q + 1, '\"');
			if ( src_q == NULL ) break;
			count += sizeof(TCHAR);
		}
		if ( ThSize(&Z->ExtendDst, count * sizeof(TCHAR) ) == FALSE ){
			return;
		}
		tstrreplace(ThPointerT(&Z->ExtendDst, top), T("\""), T("\"\""));
		Z->ExtendDst.top += count;
	}
	return;
}

UTCHAR ZFixParameter(TCHAR **commandline)
{
	TCHAR *src;
	TCHAR *dest, *destfirst;
	UTCHAR code;

	code = SkipSpace((const TCHAR **)commandline);
	if ( (code == '\0') || (code == ',') ){ // パラメータ無し
		**commandline = '\0';
		return code;
	}
	src = dest = *commandline;
	if ( code == '\"' ){
		src++;
		for ( ; ; ){
			code = *src;
			if ( code == '\0' ){
				break;
			}
			if ( code != '\"' ){
				*dest++ = code;
				src++;
				continue;
			}
			// " を見つけた場合の処理
			if ( *(src + 1) != '\"' ){	// "" エスケープ?
				src++; // 単独 " … ここで終わり
				code = *src;
				break;
			}
			// エスケープ処理
			*dest++ = code;
			src += 2;
			continue;
		}
	}else{
		src++;
		destfirst = dest;
		for ( ;; ){
			*dest++ = code;
			code = *src;
			if ( (code == ',') || // (code == ' ') ||
				 ((code < ' ') && (code == '\0') ) ){
				break;
			}
			src++;
		}
		while ( (dest > destfirst) && (*(dest - 1) == ' ') ) dest--;
	}
	*dest = '\0';
	*commandline = src;
	return code;
}

void RegExpFunction(EXECSTRUCT *Z, TCHAR *param)
{
	const TCHAR *long_result;
	TCHAR *src, *pattern;
	PXREGEXPS *rexps;

	src = param;
	if ( (ZFixParameter(&param) != ',') && (SkipSpace((const TCHAR **)&param) != ',') ) goto error;
	param++;

	pattern = param;
	ZFixParameter(&param);

	if ( FALSE == InitRegularExpression(&rexps, pattern, SLASH_REQUIRE) ) return;
	long_result = RegularExpressionReplace(rexps, src, Z->dst, CMDLINESIZE);
	if ( long_result != NULL ){
		if ( long_result == Z->dst ){
			if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
			Z->dst += tstrlen(Z->dst);
		}else{
			SetLongDestMain(Z, long_result, 0);
		}
	}
	FreeRegularExpression(rexps);
	return;

error:
	if ( *src != '?' ){
		XMessage(NULL, NULL, XM_GrERRld, T("%*regexp(src,regexp)"));
		Z->result = ERROR_INVALID_PARAMETER;
		return;
	}
	Z->dst = GetRegularExpressionName(Z->dst);
}

void ExtractPPbFunction(EXECSTRUCT *Z, TCHAR *param) // (PPbで実行)
{
	int useID;
	TCHAR *src;

	*Z->dst = '\0';
	useID = GetPPxRegID((const TCHAR **)&param);
	if ( SkipSpace((const TCHAR **)&param) == ',' ) param++;
	src = param;
	ZFixParameter(&param);
	if ( (useID >= 0) && (param != NULL) ){
		if ( (Sm->P[useID].ID[0] == 'B') &&
			 (Sm->P[useID].ID[1] == '_') &&
			 ((Sm->P[useID].UsehWnd == Z->hWnd) ||
			  (Sm->P[useID].UsehWnd == BADHWND)) ){
			Sm->P[useID].UsehWnd = Z->hWnd;

			if ( LongSendPPB(Z->hWnd, GetZCurDir(Z), 'L', useID) >= 0 ){ // >L

				TCHAR buf[32];
				HANDLE hCommSendEvent;
				int count = 2000;
				DWORD result;
				BOOL next;

				LongSendPPB(Z->hWnd, src, 'R', useID); // >R

				for ( ; ; ){
					next = FALSE;
					if ( SendPPB(Z->hWnd, T("=R"), useID) == 0 ){
						tstrcpy(buf, SyncTag);
						tstrcat(buf, Sm->P[useID].ID);
						hCommSendEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, buf);

						if ( hCommSendEvent != NULL ){
							for ( ; ; ){
								if ( GetAsyncKeyState(VK_PAUSE) & KEYSTATE_PUSH ) break;
								result = WaitForSingleObject(hCommSendEvent, 0);
								if ( result != WAIT_OBJECT_0 ) break;
								Sleep(20);
								if ( count-- == 0 ) break;
							}

							if ( (Sm->Param[0] == '=') && (Sm->Param[1] == 'r') ){
								Z->dst = tstpcpy(Z->dst, Sm->Param + 3);
								if ( *(Sm->Param + 2) == '+' ){
									if ( IsTrue(StoreLongParam(Z, 0)) ){
										next = TRUE;
									}
								}
							}
							CloseHandle(hCommSendEvent);
						}
					}
					Sm->ParamID[0] = '\0';
					if ( next == FALSE ) break;
				}
				FreePPb(Z->hWnd, useID);
				return;
			}
			FreePPb(Z->hWnd, useID);
		}
	}
	XMessage(NULL, NULL, XM_GrERRld, T("no PPb or busy"));
	Z->result = ERROR_BAD_UNIT;
}

void ExtractFunction(EXECSTRUCT *Z, TCHAR *param) // *extract
{
	HWND hWnd;
	TCHAR dest[CMDLINESIZE], *src;

	if ( TinyCharLower(SkipSpace((const TCHAR **)&param)) == 'b' ){
		ExtractPPbFunction(Z, param);
		return;
	}
	hWnd = GetPPxhWndFromID(Z->Info, (const TCHAR **)&param, NULL);
	if ( SkipSpace((const TCHAR **)&param) == ',' ) param++;
	src = param;
	ZFixParameter(&param);
	if ( hWnd == NULL ){ // 指定無し
		ERRORCODE result;

		PP_InitLongParam(dest);
		result = PP_ExtractMacro(Z->hWnd, Z->Info, NULL, src, dest, XEO_EXTRACTEXEC | XEO_EXTRACTLONG);
		if ( result == ERROR_PARTIAL_COPY ){
			*Z->dst = '\0';
			SetLongDestMain(Z, PP_GetLongParamRAW(Z->dst), 0);
			PP_FreeLongParamRAW(dest);
		}else{
			tstrcpy(Z->dst, dest);
			if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
			Z->dst += tstrlen(Z->dst);
		}
		return;
	}else if ( hWnd == BADHWND ){ // 該当無し…何もしない
		return;
	}
	ExtractPPxCall(hWnd, Z, src);
}

void CalculationFunction(EXECSTRUCT *Z, TCHAR *param) // %*calc
{
	int result;
	const TCHAR *src;

	src = param;
	ZFixParameter(&param);
	if ( CalcString(&src, &result) == CALC_NOERROR ){
		Z->dst = Int32ToString(Z->dst, result);
	}else{
		XMessage(NULL, NULL, XM_GrERRld, MES_ECAL, param);
		Z->result = ERROR_INVALID_PARAMETER;
	}
	return;
}

#define ChiceResultMax 10
int ChiceResult[ChiceResultMax] = {
	0, // error
	1, // IDOK
	0, // IDCANCEL
	3, // IDABORT
	4, // IDRETRY
	5, // IDIGNORE
	1, // IDYES
	2, // IDNO
	6, // IDCLOSE
	7 // IDHELP
};

void ChoiceFunction(EXECSTRUCT *Z, TCHAR *param)
{
	TCHAR *option, *option_max, option_buf[CMDLINESIZE], *ext_buf = NULL;
	size_t paramlen;
	TCHAR *text = NilStrNC, *title = NULL;

	const TCHAR *src;
	TCHAR *more;
	UTCHAR code;

	DWORD style = MB_ICONQUESTION | MB_OKCANCEL;
	int result;
	BOOL cacelstop = FALSE;

	if ( Z->flags & (XEO_DISPONLY | XEO_NOEDIT) ){
		Z->dst += '0';
		Z->dst = '\0';
		return;
	}

	paramlen = tstrlen(param) + 1;
	if ( paramlen > CMDLINESIZE ){
		option = ext_buf = HeapAlloc(DLLheap, 0, TSTROFF(paramlen));
	}
	if ( ext_buf != NULL ){
		option_max = ext_buf + paramlen;
	}else{
		option = option_buf;
		option_max = option_buf + CMDLINESIZE;
	}

	src = param;
	while ( '\0' != (code = GetOptionParameterML(&src, option, &more, option_max - option)) ){
		if ( code != '-' ){
			text = option;
			option = option + tstrlen(option) + 1;
			continue;
		}
		if ( !tstrcmp(option + 1, T("CANCELSTOP")) ){
			cacelstop = TRUE;
			continue;
		}
		if ( !tstrcmp(option + 1, T("TEXT")) ){
			text = more;
			option = more + tstrlen(more) + 1;
			continue;
		}
		if ( !tstrcmp(option + 1, T("TITLE")) ){
			title = more;
			option = more + tstrlen(more) + 1;
			continue;
		}
		if ( !tstrcmp(option + 1, T("TYPE")) ){
			if ( *(more + 1) == '\0') { // １文字オプション(%Q互換)
				switch (TinyCharUpper(*more)){
					case 'C': // cancel
					case 'N': // no
						style = MB_ICONQUESTION | MB_OKCANCEL | MB_DEFBUTTON2;
						break;

					case 'Y': // cancel
					case 'O': // ok
						style = MB_ICONQUESTION | MB_OKCANCEL | MB_DEFBUTTON1;
						break;
				}
				cacelstop = TRUE;
				continue;
			}
			if ( !tstricmp(more, T("ok")) ){
				style = MB_ICONQUESTION | MB_OK;
				cacelstop = TRUE;
			}
			if ( !tstricmp(more, T("oc")) ){
				style = MB_ICONQUESTION | MB_OKCANCEL;
			}
			if ( !tstricmp(more, T("yn")) ){
				style = MB_ICONQUESTION | MB_YESNO;
			}
			if ( !tstricmp(more, T("ync")) ){
				style = MB_ICONQUESTION | MB_YESNOCANCEL;
			}
			if ( !tstricmp(more, T("ari")) ){
				style = MB_ICONQUESTION | MB_ABORTRETRYIGNORE;
			}
			if ( !tstricmp(more, T("rc")) ){
				style = MB_ICONQUESTION | MB_RETRYCANCEL;
			}
			if ( Isupper(more[1]) ) setflag(style, MB_DEFBUTTON2);
			if ( Isupper(more[2]) ) setflag(style, MB_DEFBUTTON3);
			continue;
		}
		XMessage(Z->hWnd, T("%*choice"), XM_GrERRld, MES_EPRM);
		Z->result = ERROR_INVALID_PARAMETER;
		if ( ext_buf != NULL ) HeapFree(DLLheap, 0, ext_buf);
		return;
	}

	result = PMessageBox(Z->hWnd, text,
			(title != NULL) ? title : ZGetTitleName(Z),
			style | MB_PPX_PLAIN_TEXT);
	if ( result >= ChiceResultMax ) result = 0;
	result = ChiceResult[result];
	if ( cacelstop && (result == 0) ) Z->result = ERROR_CANCELLED;
	Z->dst = Int32ToString(Z->dst, result);
	if ( ext_buf != NULL ) HeapFree(DLLheap, 0, ext_buf);
}

void CallFunction(EXECSTRUCT *Z, TCHAR *cmdname, DWORD namehash, const TCHAR *funcparam)
{
	PPXMDLFUNCSTRUCT mdlparam;

	BOOL result;
	MODULESTRUCT *mdll;
	WCHAR argbuffer[CMDLINESIZE + MAX_PATH], *next, *argbuf = argbuffer;
	const TCHAR *pptr;
	COMMANDMODULEINFOSTRUCT ppxa;
	PPXMCOMMANDSTRUCT function;
	PPXMODULEPARAM module;
	TCHAR olddir[VFPS];

// 各 PPx 内蔵関数
	mdlparam.param = cmdname;
	mdlparam.dest = Z->dst;
	mdlparam.dest[0] = '\0';
	mdlparam.optparam = funcparam;

	if ( ERROR_INVALID_FUNCTION != (Z->result = (PPxInfoFunc32u(Z->Info, PPXCMDID_FUNCTION, &mdlparam) ^ 1)) ){ // PPXARESULT を戻す
		if ( mdlparam.dest == Z->dst ){
			if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
			Z->dst += tstrlen(mdlparam.dest);
		}else{
			SetLongDestMain(Z, mdlparam.dest, 0);
			HeapFree(ProcHeap, 0, mdlparam.dest);
		}
		return;
	}

// Module
	{
		DWORD paramcount = 0;
		int i;
		size_t arglength, cmdlength;
		TCHAR *userfunc;
#ifndef UNICODE
		WCHAR regidW[REGIDSIZE], nameW[MAX_PATH], destW[CMDLINESIZE];

		AnsiToUnicode(Z->Info->Name, nameW, MAX_PATH);
		AnsiToUnicode(Z->Info->RegID, regidW, REGIDSIZE);
		#define PPXAINFONAME nameW
		#define PPXAINFOREGID regidW
		#define DESTBUF destW
#else
		#define PPXAINFONAME Z->Info->Name
		#define PPXAINFOREGID Z->Info->RegID
		#define DESTBUF Z->dst
#endif
		if ( ppxmodule_count < 0 ) LoadModuleList();

		arglength = tstrlen(funcparam) + MAX_PATH;
		if ( arglength >= TSIZEOFW(argbuffer) ){
			argbuf = HeapAlloc(DLLheap, 0, arglength * sizeof(WCHAR));
			if ( argbuf == NULL ){
				Z->result = ERROR_NOT_ENOUGH_MEMORY;
				return;
			}
		}else{
			arglength = TSIZEOFW(argbuffer);
		}
		// arg の用意
		strcpyToW(argbuf, cmdname, MAX_PATH); // arg(0) コマンド名保存
		cmdlength = strlenW(argbuf) + 1;
		function.param = next = argbuf + cmdlength;
		arglength -= cmdlength;
		ppxa.rawparam = pptr = funcparam;
									// arg(1...) パラメータの切り出し
		if ( *pptr != '\0' ) for(;;){
			size_t len;
#ifdef UNICODE
			*next = '\0';
			GetParameter(&pptr, next, arglength);
#else
			char tmp[CMDLINESIZE];

			tmp[0] = '\0';
			GetParameter(&pptr, tmp, TSIZEOF(tmp));
			AnsiToUnicode(tmp, next, arglength);
#endif
			paramcount++;
			len = strlenW(next) + 1;
			next = next + len;
			arglength -= len;
			if ( NextParameter(&pptr) == FALSE ) break;
		}
		GetCurrentDirectory(TSIZEOF(olddir), olddir);
		SetCurrentDirectory(GetZCurDir(Z));
										// function module実行
		mdll = ppxmodule_list;
		module.command = &function;
		function.commandname = argbuf;
		function.commandhash = namehash;
		function.paramcount = paramcount;
		function.resultstring = DESTBUF;
		function.resultstring[0] = '\0';

		ppxa.info.Name = PPXAINFONAME;
		ppxa.info.RegID = PPXAINFOREGID;
		ppxa.info.Function = (PPXAPPINFOFUNCTIONW)CommandModuleInfoFunc;
		ppxa.info.hWnd = Z->hWnd;
		ppxa.parent = Z->Info;
		ppxa.Z = Z;

		for ( i = ppxmodule_count ; i > 0 ; i--, mdll++ ){
			if ( mdll->hDLL == NULL ){
				if ( LoadModuleFile(ppxa.info.hWnd, mdll, PPMTYPEFLAGS(PPXMEVENT_FUNCTION)) == FALSE ){
					continue;
				}
			}
#ifdef _WIN64
			if ( mdll->ModuleEntry == NULL ) continue;
			result = mdll->ModuleEntry(&ppxa.info, PPXMEVENT_FUNCTION, module);
#else
			if ( mdll->ModuleEntry != NULL ){
				result = mdll->ModuleEntry(&ppxa.info, PPXMEVENT_FUNCTION, module);
			}else{
				result = mdll->OldModuleEntry(&ppxa.info, PPXMEVENT_FUNCTION, module);
			}
#endif
			if ( result == PPXMRESULT_SKIP ) continue;
			Z->result = (result == PPXMRESULT_STOP) ? ERROR_CANCELLED : NO_ERROR;
#ifndef UNICODE
			UnicodeToAnsi(DESTBUF, Z->dst, CMDLINESIZE);
#endif
			if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
			Z->dst += tstrlen(Z->dst);
			goto endfunc;
		}

// user関数
		userfunc = GetCustValue(StrUserCommand, cmdname, argbuf, TSTROFF(CMDLINESIZE));
		if ( userfunc == NULL ){
			XMessage(NULL, NULL, XM_GrERRld, MES_ENFU, cmdname);
			Z->result = ERROR_INVALID_FUNCTION;
		}else{
			PP_InitLongParam(Z->dst);
			UserCommand(Z, cmdname, funcparam, userfunc, Z->dst);
			if ( userfunc != (TCHAR *)argbuf ) HeapFree(ProcHeap, 0, userfunc);
			if ( Z->result == ERROR_PARTIAL_COPY ){
				Z->result = NO_ERROR;
				*Z->dst = '\0';
				SetLongDestMain(Z, PP_GetLongParamRAW(Z->dst), 0);
				PP_FreeLongParamRAW(Z->dst);
			}else{
				if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
				Z->dst += tstrlen(Z->dst);
			}
		}
endfunc:
		SetCurrentDirectory(olddir);
		if ( argbuf != argbuffer ) HeapFree(DLLheap, 0, argbuf);
		return;
	}
}

// ※ funcptr, funcparam は、Z->DstBuf 上なので、パラメータを読む前に Z->dst を使うとパラメータが破損する
void ExecuteFunction(EXECSTRUCT *Z)
{
	TCHAR cmdname[CMDLINESIZE];
	TCHAR *funcparam;
	DWORD namehash;
	TCHAR *funcptr;

	*Z->dst = '\0';
	if ( !(Z->status & ST_EXTENDDST) ){
		funcptr = Z->dst = Z->DstBuf + (Z->func.off - 1) / sizeof(TCHAR);
	}else{
		if ( StoreLongParam(Z, 0) == FALSE ){
			Z->result = RPC_S_STRING_TOO_LONG;
			return;
		}
		Z->ExtendDst.top = Z->func.off - 1;
		funcptr = (TCHAR *)(Z->ExtendDst.bottom + Z->ExtendDst.top);
	}
	Z->func.off = 0;

// 関数名抽出
	namehash = GetModuleNameHash(funcptr, cmdname);
	funcparam = funcptr + tstrlen(funcptr) + 1;
// PPx common 内蔵関数
	if ( cmdname[0] <= 'I' ){ //------------------------------------------- A-I
		if ( !tstrcmp(cmdname, T("ADDCHAR")) ){
			TCHAR code = *funcparam;

			if ( code != '\0' ){
				if ( (Z->DstBuf < Z->dst) && (*(Z->dst - 1) != code) ){
					*Z->dst++ = code;
				}
			}
			return;
		}

		if ( !tstrcmp(cmdname, T("CALC")) ){
			CalculationFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("CHOICE")) ){
			ChoiceFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("ERRORMSG")) ){
			PPErrorMsg(Z->dst, GetNumber((const TCHAR **)&funcparam));
			if ( Z->func.quotation ) ZQuotationDoubler_Buf(Z);
			Z->dst += tstrlen(Z->dst);
			return;
		}

		if ( !tstrcmp(cmdname, T("EXITCODE")) || !tstrcmp(cmdname, T("ERRORLEVEL")) ){ // *exitcode
			Z->dst = Int32ToString(Z->dst, Z->ExitCode);
			return;
		}

		if ( !tstrcmp(cmdname, T("EXTRACT")) ){
			ExtractFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("F")) ){
			GetNameFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("GETCUST")) ){
			GetCustFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("INPUT")) ){
			InputFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("INSERTSOURCE")) ){
			InsertFileFunction(Z, funcparam, 0);
			return;
		}

		if ( !tstrcmp(cmdname, T("INSERTVALUE")) ){
			InsertFileFunction(Z, funcparam, 1);
			return;
		}
	}else{ //-------------------------------------------------------------- J-Z
		if ( !tstrcmp(cmdname, T("JOB")) ){
			CountJobFunction(Z);
			return;
		}

		if ( !tstrcmp(cmdname, T("LINKEDPATH")) ){
			GetLinkedPathFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("MAXLENGTH")) ){
			SetMaxLengthFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("MENU")) ){
			MenuFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("NAME")) ){
			GetNameFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("NOQ")) ){
			NoQuotationFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("NOW")) ){
			GetNowFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("PPXLIST")) ){
			GetPPxListFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("REGEXP")) ){
			RegExpFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("RUN")) ){
			CmdRunFunction(Z, funcparam, Run_FUNCTION);
			return;
		}

		if ( !tstrcmp(cmdname, T("SCREENDPI")) ){
			ScreenDpiFunction(Z, funcparam);
			return;
		}

		if ( !tstrcmp(cmdname, T("SELECTTEXT")) ){
			SelectTextFunction(Z, funcparam);
			return;
		}
/*
		if ( !tstrcmp(cmdname, T("TOUTF8")) ){
			ToUtf8Function(Z, funcparam);
			return;
		}
*/
		if ( !tstrcmp(cmdname, T("TEMP")) ){
			TempFunction(Z, funcparam);
			return;
		}
		if ( !tstrcmp(cmdname, T("TREE")) ){
			TreeFunction(Z, funcparam);
			return;
		}
		if ( !tstrcmp(cmdname, T("USEAPPS")) ){
			UseAppsFunction(Z, funcparam);
			return;
		}
	}
	CallFunction(Z, cmdname, namehash, funcparam);
}
#undef PPXAINFONAME
#undef PPXAINFOREGID

PPXDLL int PPXAPI CallModule(PPXAPPINFO *info, DWORD func, PPXMODULEPARAM ModuleParam, CALLBACKMODULEENTRY CallBackModule)
{
	MODULESTRUCT *mdll;
	int i;
	COMMANDMODULEINFOSTRUCT ppxa;

#ifndef UNICODE
		WCHAR regidW[REGIDSIZE], nameW[MAX_PATH];

		AnsiToUnicode(info->Name, nameW, MAX_PATH);
		AnsiToUnicode(info->RegID, regidW, REGIDSIZE);
		#define PPXAINFONAME nameW
		#define PPXAINFOREGID regidW
#else
		#define PPXAINFONAME info->Name
		#define PPXAINFOREGID info->RegID
#endif
										// 検索と実行
	ppxa.info.Name = PPXAINFONAME;
	ppxa.info.RegID = PPXAINFOREGID;
	ppxa.info.Function = (PPXAPPINFOFUNCTIONW)CommandModuleInfoFunc;
	ppxa.info.hWnd = info->hWnd;
	ppxa.parent = info;
	ppxa.Z = NULL;
	ppxa.rawparam = NULL;

	if ( CallBackModule != NULL ){
		return CallBackModule(&ppxa.info, func, ModuleParam);
	}

	if ( ppxmodule_count < 0 ) LoadModuleList();
	mdll = ppxmodule_list;
	for ( i = ppxmodule_count ; i > 0 ; i--, mdll++ ){
		int result;

		if ( mdll->hDLL == NULL ){
			if ( LoadModuleFile(ppxa.info.hWnd, mdll, PPMTYPEFLAGS(func)) == FALSE ){
				continue;
			}
		}
		if ( mdll->ModuleEntry == NULL ) continue;
		result = mdll->ModuleEntry(&ppxa.info, func, ModuleParam);
		if ( result == PPXMRESULT_SKIP ) continue;
		return result;
	}
	return PPXMRESULT_SKIP;
}
