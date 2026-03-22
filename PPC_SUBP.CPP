/*-----------------------------------------------------------------------------
	Paper Plane cUI								sub on cpp
-----------------------------------------------------------------------------*/
#define _PPC_SUBP_

#include "WINAPI.H"
#include <string.h>
#include <shlobj.h>
#include <ole2.h>
#include "WINOLE.H"

#if defined(WINEGCC) && (WINEGCC < 900)
#include <wchar.h>
#define UbstrVal n1.n2.n3.bstrVal
#else
#define UbstrVal bstrVal
#endif

#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#pragma hdrstop

#define UNICODE_LRM 0x200e
#define UNICODE_RLM 0x200f

#if !defined(RELEASE) && 0
	#define MSGMSG(t, s) XMessage(NULL, NULL, XM_DbgLOG, T(t), s);
	#define MSGMSGA(t, s) MSGMSG(t, s)
#else
	#define MSGMSG(t, s)
	#define MSGMSGA(t, s) UnUsedParam(s);
#endif
#ifndef PSM_INDEXTOPAGE
	#define PSM_INDEXTOPAGE (WM_USER + 132)
#endif

typedef struct {
	ThSTRUCT *thEcdata;
	COLUMNEXTDATAINFO *cndi;
	SHCOLUMNDATA scd;
	GETINFOTIPCALLBACK callbackfunc;
	void *data;
} GetColumnDataMainStruct;

PWSTR GetPropertyValue(GetColumnDataMainStruct *gcdms, COLUMNEXTDATAINFO *cndi);
const TCHAR GetColumnDataMainErrorMsg[] = T("error");

const IID XIID_IQueryInfo = {0x00021500, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

INT_PTR (WINAPI *DPropertySheet)(LPCPROPSHEETHEADER) = NULL;
HPROPSHEETPAGE (WINAPI *DCreatePropertySheetPage)(LPCPROPSHEETPAGE) = NULL;

LOADWINAPISTRUCT COMCTL32DLL[] = {
	LOADWINAPI1T(PropertySheet),
	LOADWINAPI1T(CreatePropertySheetPage),
	{NULL, NULL}
};

DLLDEFINED BSTR (STDAPICALLTYPE *DSysAllocString)(const OLECHAR *) DLLPARAM(NULL);
DLLDEFINED void (STDAPICALLTYPE *DVariantInit)(_Out_ VARIANTARG * pvarg);
DLLDEFINED HRESULT (STDAPICALLTYPE *DVariantClear)(VARIANTARG * pvarg);
DLLDEFINED HRESULT (STDAPICALLTYPE *DVariantChangeType)(VARIANTARG * pvargDest, VARIANTARG *pvarSrc, USHORT wFlags, VARTYPE vt);

DLLDEFINED HRESULT (STDAPICALLTYPE *DGetActiveObject)(REFCLSID rclsid, void * pvReserved, IUnknown ** ppunk);
DLLDEFINED HRESULT (STDAPICALLTYPE *DRegisterActiveObject)(IUnknown * punk, REFCLSID rclsid, DWORD dwFlags, DWORD *pdwRegister);
DLLDEFINED HRESULT (STDAPICALLTYPE *DRevokeActiveObject)(DWORD dwRegister, void *pvReserved);

LOADWINAPISTRUCT OLEAUT32APIS[] = {
	LOADWINAPI1(VariantInit),
	LOADWINAPI1(VariantClear),
	LOADWINAPI1(VariantChangeType),

	LOADWINAPI1(GetActiveObject),
	LOADWINAPI1(RegisterActiveObject),
	LOADWINAPI1(RevokeActiveObject),
	{NULL, NULL}
};
HANDLE hOleaut32 = NULL;
HANDLE hPropsys = NULL;

#define PAGEMAX 20
#define SYNCPROPPOSNAME T("SyncPrp")

const TCHAR PHandlersKey[] = T("shellex\\PropertySheetHandlers");
const TCHAR PAllKey[] = T("*");
#if 1
 const TCHAR PDirectoryKey[] = T("Directory");
 const TCHAR PAllFileKey[] = T("AllFilesystemObjects");
#endif
const TCHAR PropThreadThreadName[] = T("PPc Prop");

WNDPROC OldPropProc;
HHOOK OldPropKeyProc;
HWND hPropParentWnd;


#ifndef __IPreviewHandler_INTERFACE_DEFINED__
#define __IPreviewHandler_INTERFACE_DEFINED__
const IID IID_IPreviewHandler = {0x8895b1c6, 0xb41f, 0x4c1c, {0xa5, 0x62, 0x0d, 0x56, 0x42, 0x50, 0x83, 0x6f}};
class IPreviewHandler : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE SetWindow(HWND hwnd, const RECT *prc) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetRect(const RECT *prc) = 0;
		virtual HRESULT STDMETHODCALLTYPE DoPreview(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE Unload(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetFocus(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE QueryFocus(HWND *phwnd) = 0;
		virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(MSG *pmsg) = 0;
};
#endif

// STGM_READ
#ifndef __IInitializeWithFile_INTERFACE_DEFINED__
#define __IInitializeWithFile_INTERFACE_DEFINED__
const IID IID_IInitializeWithFile = {0xb7d14566, 0x0509, 0x4cce, {0xa7, 0x1f, 0x0a, 0x55, 0x42, 0x33, 0xbd, 0x9b}};
class IInitializeWithFile : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE Initialize(LPCWSTR pszFilePath, DWORD grfMode) = 0;
};
#endif

#ifndef __IInitializeWithStream_INTERFACE_DEFINED__
#define __IInitializeWithStream_INTERFACE_DEFINED__
const IID IID_IInitializeWithStream = {0xb824b49d, 0x22ac, 0x4161, {0xac, 0x8a, 0x99, 0x16, 0xe8, 0xfa, 0x3f, 0x7f}};
class IInitializeWithStream : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE Initialize(_In_  IStream *pstream, _In_  DWORD grfMode) = 0;
};
#endif

#ifndef __IFilter_INTERFACE_DEFINED__
#define __IFilter_INTERFACE_DEFINED__
typedef enum tagCHUNK_BREAKTYPE
{
	CHUNK_NO_BREAK	= 0,
	CHUNK_EOW	= 1,
	CHUNK_EOS	= 2,
	CHUNK_EOP	= 3,
	CHUNK_EOC	= 4
} CHUNK_BREAKTYPE;

typedef enum tagCHUNKSTATE
{
	CHUNK_TEXT	= 0x1,
	CHUNK_VALUE	= 0x2,
	CHUNK_FILTER_OWNED_VALUE	= 0x4
} CHUNKSTATE;

typedef struct tagFULLPROPSPEC
{
	GUID guidPropSet;
	PROPSPEC psProperty;
} FULLPROPSPEC;

typedef struct tagFILTERREGION
{
	ULONG idChunk;
	ULONG cwcStart;
	ULONG cwcExtent;
} FILTERREGION;

typedef struct tagSTAT_CHUNK
{
	ULONG idChunk;
	CHUNK_BREAKTYPE breakType;
	CHUNKSTATE flags;
	LCID locale;
	FULLPROPSPEC attribute;
	ULONG idChunkSource;
	ULONG cwcStartSource;
	ULONG cwcLenSource;
} STAT_CHUNK;

EXTERN_C const IID xIID_IFilter = {0x89BCB740,0x6119,0x101A,{0xBC,0xB7,0x00,0xDD,0x01,0x06,0x55,0xAF}};
class IFilter : public IUnknown
{
	public:
		virtual SCODE STDMETHODCALLTYPE Init(ULONG grfFlags, ULONG cAttributes, const FULLPROPSPEC *aAttributes, ULONG *pFlags) = 0;

		virtual SCODE STDMETHODCALLTYPE GetChunk(STAT_CHUNK *pStat) = 0;

		virtual SCODE STDMETHODCALLTYPE GetText(ULONG *pcwcBuffer, WCHAR *awcBuffer) = 0;

		virtual SCODE STDMETHODCALLTYPE GetValue(PROPVARIANT **ppPropValue) = 0;

		virtual SCODE STDMETHODCALLTYPE BindRegion(FILTERREGION origPos,REFIID riid,void **ppunk) = 0;
};

typedef enum tagIFILTER_INIT
{
	IFILTER_INIT_CANON_PARAGRAPHS	= 1,
	IFILTER_INIT_HARD_LINE_BREAKS	= 2,
	IFILTER_INIT_CANON_HYPHENS	= 4,
	IFILTER_INIT_CANON_SPACES	= 8,
	IFILTER_INIT_APPLY_INDEX_ATTRIBUTES	= 16,
	IFILTER_INIT_APPLY_OTHER_ATTRIBUTES	= 32,
	IFILTER_INIT_APPLY_CRAWL_ATTRIBUTES	= 256,
	IFILTER_INIT_INDEXING_ONLY	= 64,
	IFILTER_INIT_SEARCH_LINKS	= 128,
	IFILTER_INIT_FILTER_OWNED_VALUE_OK	= 512,
	IFILTER_INIT_FILTER_AGGRESSIVE_BREAK	= 1024,
	IFILTER_INIT_DISABLE_EMBEDDED	= 2048,
	IFILTER_INIT_EMIT_FORMATTING	= 4096
} IFILTER_INIT;
#endif

// カラム拡張関連 - XPまで ====================================================
void GetColumnExtTextInfo(PPC_APPINFO *cinfo, int cellcolumn, ThSTRUCT *text)
{
	CELLEXTRASTRUCT *cdsptr;
	COLUMNEXTDATAINFO *cndi;

	if ( (cellcolumn < 0) || (cinfo->ColumnExtDlls.bottom == NULL) ) return;

	cdsptr = (CELLEXTRASTRUCT *)(BYTE *)(cinfo->e.CellExtra.bottom + cellcolumn);
	for ( ; ; ){
		if ( cdsptr->textoffset ){
			DWORD index = cdsptr->itemindex, offset;

			offset = *(DWORD *)cinfo->ColumnExtDlls.bottom;
			while ( offset < cinfo->ColumnExtDlls.top ){
				cndi = (COLUMNEXTDATAINFO *)(BYTE *)(cinfo->ColumnExtDlls.bottom + offset);
				if ( index-- == 0 ){
					thprintf(text, THP_CAT, T("\r\n%s\t:"), (TCHAR *)(BYTE *)(cinfo->ColumnExtDlls.bottom + offset + sizeof(COLUMNEXTDATAINFO)) );
					break;
				}
				offset = cndi->next;
			}
			ThCatString(text, GetCellExtraText(cinfo, cdsptr) );
		}
		if ( cdsptr->nextoffset == 0 ) break;
		cdsptr = CellExtraNext(cinfo, cdsptr);
	}
}

WORD GetColumnExtItemIndex(PPC_APPINFO *cinfo, const TCHAR *itemname)
{
	WORD itemindex = DFC_COLUMNEX;
	COLUMNEXTDATAINFO *cndi;
	DWORD offset;

	if ( cinfo->ColumnExtDlls.bottom == NULL ){
		GetColumnExtMenu(&cinfo->ColumnExtDlls, cinfo->RealPath, NULL, 0);
	}

	offset = *(DWORD *)cinfo->ColumnExtDlls.bottom;
	while ( offset < cinfo->ColumnExtDlls.top ){
		cndi = (COLUMNEXTDATAINFO *)(BYTE *)(cinfo->ColumnExtDlls.bottom + offset);
		if ( !tstrcmp(itemname, (TCHAR *)(BYTE *)(cinfo->ColumnExtDlls.bottom + offset + sizeof(COLUMNEXTDATAINFO))) ){
			return itemindex;
		}
		offset = cndi->next;
		itemindex++;
	}
	return DFC_FAULT;
}

COLUMNEXTDATAINFO *GetColumnExtDataInfo(ThSTRUCT *thEcdata, DWORD index)
{
	COLUMNEXTDATAINFO *cndi;
	DWORD offset;

	if ( hOleaut32 == NULL ){
		hOleaut32 = LoadWinAPI("OLEAUT32.DLL", NULL, OLEAUT32APIS, LOADWINAPI_LOAD);
		if ( hOleaut32 == NULL ) return NULL;
	}
	if ( thEcdata->bottom == NULL ) return NULL;
	offset = *(DWORD *)thEcdata->bottom;
	while ( offset < thEcdata->top ){
		cndi = (COLUMNEXTDATAINFO *)(BYTE *)(thEcdata->bottom + offset);
		if ( index-- == 0 ) return cndi;
		offset = cndi->next;
	}
	return NULL;
}

#ifndef UNICODE
void USEFASTCALL FixPropTextA(WCHAR *src, char *dest)
{
	WCHAR *sp, *dp;

	sp = dp = src;
	while ( *sp != '\0' ){
		if ( (*sp != UNICODE_LRM) && (*sp != UNICODE_RLM) ) *dp++ = *sp;
		sp++;
	}
	*dp = '\0';
	UnicodeToAnsi(src, dest, VFPS);
}
#endif

COLUMNEXTDATAINFO *ChainPropertyList(GetColumnDataMainStruct *gcdms, COLUMNEXTDATAINFO *cndi)
{
	DWORD offset;
	const TCHAR *idname;
	COLUMNEXTDATAINFO *newcndi = NULL;
	ThSTRUCT *thEcdata = gcdms->thEcdata;

	// 同名がないか調べる
	offset = cndi->next;
	idname = (const TCHAR *)(BYTE *)((BYTE *)cndi + sizeof(COLUMNEXTDATAINFO));

	while ( offset < thEcdata->top ){
		newcndi = (COLUMNEXTDATAINFO *)(BYTE *)(thEcdata->bottom + offset);
		if ( !tstrcmp(idname, (const TCHAR *)(BYTE *)((BYTE *)newcndi + sizeof(COLUMNEXTDATAINFO))) ){
			// 発見
			cndi->next_pkey = offset;
			break;
		}
		offset = newcndi->next;
	}
	if ( cndi->next_pkey != 0 ) return newcndi;
	cndi->next_pkey = 1; // 検索終了
	return NULL;
}

void GetColumnDataMain(GetColumnDataMainStruct *gcdms)
{
	COLUMNEXTDATAINFO *cndi;
	COLUMNEXTDATAINFO_ICP *icps;
	VARIANT var;

	cndi = gcdms->cndi;
	if ( cndi == NULL ){
		gcdms->callbackfunc(gcdms->data, GetColumnDataMainErrorMsg);
		return;
	}
	icps = &cndi->icps;

	DVariantInit(&var);

	// XP 用
	if ( icps->Icp != NULL ) for (;;){
		if ( icps->Icp->GetItemData(&cndi->id.scid, &gcdms->scd, &var) == S_OK ){
			VARIANT newvar;
			DVariantInit(&newvar);
			DVariantChangeType(&newvar, &var, 0, VT_BSTR);

			#ifndef UNICODE
				TCHAR buf[VFPS];

				UnicodeToAnsi(newvar.UbstrVal, buf, VFPS);

				gcdms->callbackfunc(gcdms->data, buf);
			#else
				gcdms->callbackfunc(gcdms->data, newvar.UbstrVal);
			#endif
			::DVariantClear(&newvar);
			::DVariantClear(&var);
			return;
		}
		// 他に取得が可能か？
		if ( icps->next != 0 ){
			icps = (COLUMNEXTDATAINFO_ICP *)(BYTE *)(gcdms->thEcdata->bottom + icps->next);
			continue;
		}
		// Vista以降用取得が可能か？
		if ( cndi->next_pkey == 1 ) return;
		if ( cndi->next_pkey != 0 ){
			cndi = (COLUMNEXTDATAINFO *)(BYTE *)(gcdms->thEcdata->bottom + cndi->next_pkey);
			break;
		}
		// cndi->next_pkey == 0
		cndi = ChainPropertyList(gcdms, cndi);
		if ( cndi != NULL ) break;
		return;
	}

	// Vista 以降用
	if ( cndi->icps.Icp == NULL ){
		PWSTR ValueText = GetPropertyValue(gcdms, cndi);
		if ( ValueText != NULL ){
		#ifndef UNICODE
			TCHAR buf[VFPS];

			FixPropTextA(ValueText, buf);
			gcdms->callbackfunc(gcdms->data, buf);
		#else
			gcdms->callbackfunc(gcdms->data, ValueText);
		#endif
			CoTaskMemFree(ValueText);
			return;
		}
	}

}

typedef struct {
	PPC_APPINFO *cinfo;
	ENTRYCELL *cell;
	DWORD CommentID;
} ExtExecCallbackStruct;

void WINAPI ExtExecCallback(ExtExecCallbackStruct *eecs, const TCHAR *text)
{
	SetComment(eecs->cinfo, eecs->CommentID, eecs->cell, text);
}

void ExtExec(PPC_APPINFO *cinfo, ThSTRUCT *thEcdata, DWORD index, DWORD CommentID)
{
	GetColumnDataMainStruct gcdms;
	ExtExecCallbackStruct eecs;
	int work;

	gcdms.thEcdata = thEcdata;
	gcdms.cndi = GetColumnExtDataInfo(thEcdata, index);
	gcdms.callbackfunc = (GETINFOTIPCALLBACK)ExtExecCallback;
	gcdms.data = &eecs;
	eecs.cinfo = cinfo;
	eecs.CommentID = CommentID;

	InitEnumMarkCell(cinfo, &work);
	while ( (eecs.cell = EnumMarkCell(cinfo, &work)) != NULL ){
		gcdms.scd.dwFlags = 0;
		gcdms.scd.dwFileAttributes = eecs.cell->f.dwFileAttributes;
		#ifndef UNICODE
			char fname[VFPS];

			VFSFullPath(fname, eecs.cell->f.cFileName, cinfo->RealPath);
			AnsiToUnicode(fname, gcdms.scd.wszFile, MAX_PATH);
			if ( gcdms.scd.wszFile[0] != '\0' ){
				gcdms.scd.pwszExt = strrchrW(gcdms.scd.wszFile + 1, '.');
				if ( gcdms.scd.pwszExt == NULL ){
					gcdms.scd.pwszExt = gcdms.scd.wszFile + strlenW(gcdms.scd.wszFile);
				}else{
					gcdms.scd.pwszExt++;
				}
			}else{
				gcdms.scd.pwszExt = gcdms.scd.wszFile;
			}
		#else
			if ( (tstrlen(eecs.cell->f.cFileName) + tstrlen(cinfo->RealPath)) >= (MAX_PATH - 1) ) break;
			VFSFullPath(gcdms.scd.wszFile, eecs.cell->f.cFileName, cinfo->RealPath);
			if ( *(eecs.cell->f.cFileName + eecs.cell->ext) == '\0' ){
				gcdms.scd.pwszExt = gcdms.scd.wszFile + eecs.cell->ext;
			}else{
				gcdms.scd.pwszExt = gcdms.scd.wszFile + eecs.cell->ext + 1;
			}
		#endif
		GetColumnDataMain(&gcdms);
	}
	::Repaint(cinfo);
}

void ExtGetData(ThSTRUCT *thEcdata, DWORD index, const TCHAR *filename, DWORD attributes, GETINFOTIPCALLBACK callbackfunc, void *data)
{
	GetColumnDataMainStruct gcdms;

	gcdms.thEcdata = thEcdata;
	gcdms.cndi = GetColumnExtDataInfo(thEcdata, index);
	gcdms.callbackfunc = callbackfunc;
	gcdms.data = data;

	gcdms.scd.dwFlags = 0;
	gcdms.scd.dwFileAttributes = attributes;
	#ifndef UNICODE
		AnsiToUnicode(filename, gcdms.scd.wszFile, MAX_PATH);
	#else
		tstrcpy(gcdms.scd.wszFile, filename);
	#endif
	gcdms.scd.pwszExt = strrchrW(gcdms.scd.wszFile, '.');
	if ( gcdms.scd.pwszExt == NULL ){
		gcdms.scd.pwszExt = gcdms.scd.wszFile + strlenW(gcdms.scd.wszFile);
	}else{
		gcdms.scd.pwszExt++;
	}
	GetColumnDataMain(&gcdms);
}

// カラム拡張関連 - Vista以降 =================================================
const IID XIID_IPropertyStore = {0x886d8eeb, 0x8cf2, 0x4446, {0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99}};
const IID XIID_IPropertyDescription = {0x6f79d558, 0x3e96, 0x4549, {0xa1, 0xd1, 0x7d, 0x75, 0xd2, 0x28, 0x88, 0x14}};

#ifndef PSSTDAPI
typedef enum {
	GPS_DEFAULT = 0,
	GPS_BESTEFFORT = 0x40,
} GETPROPERTYSTOREFLAGS;
#endif

// OLE32.dll
DefineWinAPI(HRESULT, PropVariantClear, (PROPVARIANT *pvar));

// Shell32.dll
DefineWinAPI(HRESULT, SHGetPropertyStoreFromParsingName, (PCWSTR pszPath, IBindCtx *pbc, GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv));
DefineWinAPI(HRESULT, SHGetPropertyStoreFromIDList, (LPITEMIDLIST pidl, GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv));

// Propsys.dll
HRESULT (WINAPI *DPSFormatForDisplayAlloc)(REFPROPERTYKEY key, REFPROPVARIANT propvar, PROPDESC_FORMAT_FLAGS pdff, PWSTR *ppszDisplay);
//HRESULT (WINAPI *DPSGetNameFromPropertyKey)(REFPROPERTYKEY propkey, PWSTR *ppszCanonicalName);
HRESULT (WINAPI *DPSGetPropertyKeyFromName)(PCWSTR pszName, PROPERTYKEY *ppropkey);
HRESULT (WINAPI *DPSGetPropertyDescription)(REFPROPERTYKEY propkey, REFIID riid, void **ppv);

LOADWINAPISTRUCT PROPSAPIS[] = {
	LOADWINAPI1(PSFormatForDisplayAlloc),
//	LOADWINAPI1(PSGetNameFromPropertyKey),
	LOADWINAPI1(PSGetPropertyKeyFromName),
	LOADWINAPI1(PSGetPropertyDescription),
	{NULL, NULL}
};

BOOL InitPropSys(void)
{
	HMODULE hShell32 = GetModuleHandle(StrShell32DLL);

	GETDLLPROC(hShell32, SHGetPropertyStoreFromParsingName);
	GETDLLPROC(hShell32, SHGetPropertyStoreFromIDList);
	if ( DSHGetPropertyStoreFromParsingName == NULL ) return FALSE;
	if ( DPSFormatForDisplayAlloc == NULL ){
		hPropsys = LoadWinAPI("PROPSYS.DLL", NULL, PROPSAPIS, LOADWINAPI_LOAD);
		if ( hPropsys == NULL ) return FALSE;
	}
	GETDLLPROC(GetModuleHandle(T("OLE32.DLL")), PropVariantClear);
	return TRUE;
}

void VistaProperties(const TCHAR *filename, ThSTRUCT *text)
{
	#ifdef UNICODE
		#define propfilename filename
	#else
		WCHAR propfilename[VFPS];
		AnsiToUnicode(filename, propfilename, VFPS);
	#endif
	HRESULT hr;
	IPropertyStore *ps;

	if ( DSHGetPropertyStoreFromParsingName == NULL ){
		if ( InitPropSys() == FALSE ) return;
	}

	if ( *filename != '#' ){
#pragma warning(suppress: 6011) // ※１ InitPropSys で DSHGetPropertyStoreFromParsingName が初期化
		hr = DSHGetPropertyStoreFromParsingName(propfilename, NULL, GPS_BESTEFFORT, XIID_IPropertyStore, reinterpret_cast<void**>(&ps));
	}else{
		hr = E_FAIL;
	}
	if ( FAILED(hr) && (DSHGetPropertyStoreFromIDList != NULL) ){
		LPITEMIDLIST pidl = PathToPidl(filename);
		hr = DSHGetPropertyStoreFromIDList(pidl, GPS_BESTEFFORT, XIID_IPropertyStore, reinterpret_cast<void**>(&ps)); // ※２
		FreePIDL(pidl);
	}

	if ( SUCCEEDED(hr) ){
		DWORD propcount;

		#pragma warning(suppress: 4701) // succeed のときは※１か※２が成功の時だけ
		hr = ps->GetCount(&propcount);
		if ( SUCCEEDED(hr) && propcount ){
			ThCatString(text, T("Columns:\r\n"));
			for ( DWORD index = 0; index < propcount; index++ ){
				PROPERTYKEY propertyKey;

				hr = ps->GetAt(index, &propertyKey);
				if ( SUCCEEDED(hr) ){
					PROPVARIANT PropValue;

					hr = ps->GetValue(propertyKey, &PropValue);
					if ( SUCCEEDED(hr) ){
						PWSTR ValueText;

						hr = DPSFormatForDisplayAlloc(propertyKey, PropValue, PDFF_DEFAULT, &ValueText);
						if ( SUCCEEDED(hr) ){
							IPropertyDescription *pd;

							hr = DPSGetPropertyDescription(propertyKey, XIID_IPropertyDescription, reinterpret_cast<void**>(&pd));
							if ( SUCCEEDED(hr) ){
								PWSTR pszPropertyLabel = NULL;

								hr = pd->GetDisplayName(&pszPropertyLabel);
								if ( SUCCEEDED(hr) ){
									thprintf(text, THP_CAT, T("%Ls\t:%Ls\r\n"), pszPropertyLabel, ValueText);
									CoTaskMemFree(pszPropertyLabel);
								}
								pd->Release();
							}
							CoTaskMemFree(ValueText);
						}
						DPropVariantClear(&PropValue);
					}
				}
			}
		}
		ps->Release();
	}
	return;
}

PWSTR GetPropertyValue(GetColumnDataMainStruct *gcdms, COLUMNEXTDATAINFO *cndi)
{
	IPropertyStore *ps = NULL;
	PWSTR ValueText = NULL;
	PROPVARIANT PropValue;

	if ( DSHGetPropertyStoreFromParsingName == NULL ){
		if ( InitPropSys() == FALSE ) return NULL;
	}
#pragma warning(suppress: 6011) // InitPropSys で DSHGetPropertyStoreFromParsingName が初期化
	if ( FAILED(DSHGetPropertyStoreFromParsingName(gcdms->scd.wszFile, NULL, GPS_BESTEFFORT, XIID_IPropertyStore, reinterpret_cast<void**>(&ps))) ){
		return NULL;
	}
	for (;;){
		if ( SUCCEEDED(ps->GetValue(cndi->id.pkey, &PropValue)) ){
			HRESULT hr = DPSFormatForDisplayAlloc(cndi->id.pkey, PropValue, PDFF_DEFAULT, &ValueText);
			DPropVariantClear(&PropValue);
			if ( SUCCEEDED(hr) ) break;
		}
		// 他に取得が可能か？
		if ( cndi->next_pkey == 1 ){
			ValueText = NULL;
			break;
		}
		if ( cndi->next_pkey != 0 ){
			cndi = (COLUMNEXTDATAINFO *)(BYTE *)(gcdms->thEcdata->bottom + cndi->next_pkey);
			continue;
		}
		// cndi->next_pkey == 0
		cndi = ChainPropertyList(gcdms, cndi);
		if ( cndi == NULL ){
			ValueText = NULL;
			break;
		}
	}
	ps->Release();
	return ValueText;
}

const TCHAR InfoTipStr[] = T("InfoTip");
const TCHAR FileOperationPromptStr[] = T("FileOperationPrompt");
const TCHAR AllTypeStr[] = T("*");
void GetVistaFileProps(PPC_APPINFO *cinfo, TCHAR *dest, int len, ENTRYCELL *cell)
{
	TCHAR filename[VFPS];
	TCHAR *ext;
	TCHAR regpath[VFPS], proptypes[VFPS];

	GetCellRealFullName(cinfo, cell, filename);
	dest[0] = '\0';
	proptypes[0] = '\0';

	ext = tstrrchr(filename, '.');
	if ( ext != NULL ){
		regpath[0] = '\0';
		GetRegString(HKEY_CLASSES_ROOT, ext, InfoTipStr, regpath, TSIZEOF(regpath));
		if ( regpath[0] != '\0' ){
			GetRegString(HKEY_CLASSES_ROOT, regpath, InfoTipStr, proptypes, TSIZEOF(proptypes));
		}
		if ( proptypes[0] == '\0' ){
			thprintf(regpath, TSIZEOF(regpath), T("SystemFileAssociations\\%s"), ext);
			GetRegString(HKEY_CLASSES_ROOT, regpath, InfoTipStr, proptypes, TSIZEOF(proptypes));
			if ( proptypes[0] == '\0' ){
				GetRegString(HKEY_CLASSES_ROOT, regpath, FileOperationPromptStr, proptypes, TSIZEOF(proptypes));
			}
		}
	}

	if ( proptypes[0] == '\0' ){
		GetRegString(HKEY_CLASSES_ROOT, AllTypeStr, InfoTipStr, proptypes, TSIZEOF(proptypes));
		if ( proptypes[0] == '\0' ) return;
	}

#ifdef UNICODE
	#define propfilename filename
#else
	WCHAR propfilename[VFPS];
	AnsiToUnicode(filename, propfilename, VFPS);
#endif
	HRESULT hr;
	IPropertyStore *ps;

	if ( DSHGetPropertyStoreFromParsingName == NULL ){
		if ( InitPropSys() == FALSE ) return;
	}

#pragma warning(suppress: 6011) // InitPropSys で DSHGetPropertyStoreFromParsingName が初期化
	hr = DSHGetPropertyStoreFromParsingName(propfilename, NULL, GPS_BESTEFFORT, XIID_IPropertyStore, reinterpret_cast<void**>(&ps));

	if ( FAILED(hr) ) return;

	TCHAR *ptp, *nextptp;
	ptp = tstrchr(proptypes, ':');
	if ( ptp == NULL ){
		ptp = proptypes;
	}else{
		ptp++;
	}
	while( *ptp != '\0' ){
		if ( *ptp == '*' ) ptp++;
		nextptp = tstrchr(ptp, ';');
		if ( nextptp != NULL ) *nextptp = '\0';

	#ifdef UNICODE
		#define TPTP ptp
	#else
		WCHAR TPTP[VFPS];
		AnsiToUnicode(ptp, TPTP, VFPS);
	#endif

		PROPERTYKEY propertyKey;
		if ( SUCCEEDED(DPSGetPropertyKeyFromName(TPTP, &propertyKey)) ){
			PROPVARIANT PropValue;

			hr = ps->GetValue(propertyKey, &PropValue);
			if ( SUCCEEDED(hr) ){
				PWSTR ValueText;

				hr = DPSFormatForDisplayAlloc(propertyKey, PropValue, PDFF_DEFAULT, &ValueText);
				if ( SUCCEEDED(hr) ){
					if ( ValueText[0] != '\0' ){
						IPropertyDescription *pd;

						hr = DPSGetPropertyDescription(propertyKey, XIID_IPropertyDescription, reinterpret_cast<void**>(&pd));
						if ( SUCCEEDED(hr) ){
							PWSTR pszPropertyLabel = NULL;

							hr = pd->GetDisplayName(&pszPropertyLabel);
							if ( SUCCEEDED(hr) ){
								int nlen;

								nlen = thprintf(dest, len, T("\r\n%Ls\t:%Ls"), pszPropertyLabel, ValueText) - dest;

								CoTaskMemFree(pszPropertyLabel);
								dest += nlen;
								len -= nlen;
							}
							pd->Release();
						}
					}
					CoTaskMemFree(ValueText);
				}
				DPropVariantClear(&PropValue);
			}
		}
		if ( nextptp == NULL ) break;
		ptp = nextptp + 1;
	}
	ps->Release();
}

// チップ拡張関連 =========================================================
BOOL GetInfoTipText(const TCHAR *filename, int extlen, GETINFOTIPCALLBACK callbackfunc, void *data)
{
	TCHAR name[VFPS], idname[MAX_PATH];
	LPMALLOC pMA;
	CLSID hid;
	IPersistFile *ppf;
	HRESULT hres;

	if ( FAILED(SHGetMalloc(&pMA)) ) return FALSE;

	thprintf(name, TSIZEOF(name),
			T("%s\\ShellEx\\{00021500-0000-0000-C000-000000000046}"),
			filename + extlen);
	idname[0] = '\0';
	GetRegString(HKEY_CLASSES_ROOT, name, NilStr, idname, TSIZEOF(idname));
	if ( (idname[0] != '\0') && !IsShellExBlocked(idname) ){
		#undef tidname
		#ifndef UNICODE
			WCHAR idnameW[MAX_PATH];
			#define tidname idnameW

			AnsiToUnicode(idname, idnameW, MAX_PATH);
		#else
			#define tidname idname
		#endif
		if ( SUCCEEDED(::CLSIDFromString(tidname, &hid)) ){
			if ( SUCCEEDED( ::CoCreateInstance(hid, NULL, CLSCTX_INPROC_SERVER,
				IID_IPersistFile, (LPVOID *)&ppf)) ){
			#ifdef UNICODE
				hres = ppf->Load(filename, STGM_READ);
			#else
				WCHAR wsz[VFPS];

				::MultiByteToWideChar(CP_ACP, 0, filename, -1, wsz, TSIZEOF(wsz));
				hres = ppf->Load(wsz, STGM_READ);	// link を取得
			#endif
				if ( SUCCEEDED(hres) ){
					IQueryInfo *pqi;

					hres = ppf->QueryInterface(XIID_IQueryInfo, (LPVOID *)&pqi);
					if ( SUCCEEDED(hres) ){
						WCHAR *tiptext;

						if ( SUCCEEDED(pqi->GetInfoTip(0, &tiptext)) && tiptext ){
						#ifdef UNICODE
							callbackfunc(data, tiptext);
						#else
							TCHAR buf[0x1000];
							buf[0] = '\0';

							UnicodeToAnsi(tiptext, buf, 0x1000 - 1);
							buf[0x1000 - 1] = '\0';
							callbackfunc(data, buf);
						#endif
							pMA->Free(tiptext);
						}
						pqi->Release();
					}
				}
				ppf->Release();
			}
		}
	}
	pMA->Release();
	return TRUE;
}

typedef struct {
	PPC_APPINFO *cinfo;
	ENTRYCELL *cell;
} EXTINFOTIPSTRUCT;

void WINAPI ExtInfoTipCallback(EXTINFOTIPSTRUCT *eits, const TCHAR *text)
{
	SetComment(eits->cinfo, 0, eits->cell, text);
}

void ExtInfoTip(PPC_APPINFO *cinfo)
{
	EXTINFOTIPSTRUCT eits;
	int work;
	TCHAR name[VFPS];

	eits.cinfo = cinfo;

	InitEnumMarkCell(cinfo, &work);
	while ( (eits.cell = EnumMarkCell(cinfo, &work)) != NULL ){
		const TCHAR *ls;

		VFSFullPath(name, eits.cell->f.cFileName, cinfo->RealPath);
		ls = VFSFindLastEntry(name);
		GetInfoTipText(name, (ls - name) + FindExtSeparator(ls), (GETINFOTIPCALLBACK)ExtInfoTipCallback, (void *)&eits);
	}
	::Repaint(cinfo);
}

//  ================================================
#if !NODLL
HRESULT GetLink(HWND hWnd, const TCHAR *LinkFile, TCHAR *Original)
{
	HRESULT hres;
	IShellLink *psl;
	WIN32_FIND_DATA ff;
										// IShellLink interface を取得
	hres = ::CoCreateInstance(CLSID_ShellLink, NULL,
			CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);
	if ( SUCCEEDED(hres) ){
		IPersistFile *ppf;
									// IShellLink から IPersistFile interface
		hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
		if ( SUCCEEDED(hres) ){
			#ifdef UNICODE
				hres = ppf->Load(LinkFile, STGM_READ);	// link を取得
			#else
				WCHAR wsz[VFPS];

				::MultiByteToWideChar(CP_ACP, 0, LinkFile, -1, wsz, TSIZEOF(wsz));
				hres = ppf->Load(wsz, STGM_READ);	// link を取得
			#endif
			if ( SUCCEEDED(hres) ){
				hres = psl->Resolve(hWnd, SLR_NO_UI | SLR_NOUPDATE | SLR_NOSEARCH | SLR_NOTRACK | SLR_NOLINKINFO);
				if ( SUCCEEDED(hres) ){
					#pragma warning(suppress: 6001) // ff 未参照
					hres = psl->GetPath(Original, MAX_PATH, &ff, 0);
					if ( FAILED(hres) || (Original[0] == '\0') ){
						LPITEMIDLIST idl;

						if ( SUCCEEDED(psl->GetIDList(&idl)) && (idl != NULL) ){
							LPSHELLFOLDER pSF;

							(void)SHGetDesktopFolder(&pSF);
							GetIDLSub(Original, pSF, idl);
							pSF->Release();

							FreePIDL(idl);
							hres = S_OK;
						}
					}
				}
			}
			ppf->Release();
		}
		psl->Release();
	}
	return hres;
}
#endif
// 同期プロパティ関連 =========================================================

typedef struct {
	HPROPSHEETPAGE sheet[PAGEMAX];
	struct {
		IShellExtInit *pShellExt;
		IShellPropSheetExt *pShellProp;
	} COM[PAGEMAX];

	int pages, FirstPage;
	int use;

	LPITEMIDLIST idlist;
	LPSHELLFOLDER ShellFolder;
	LPDATAOBJECT DataObject;
} PropInfoStruct;

void AddExtProps(PropInfoStruct *propinfo, const TCHAR *tname);

PropInfoStruct *pPropInfo;

BOOL CALLBACK PropPageFunc(HPROPSHEETPAGE hPSP, LPARAM lParam)
{
	*((HPROPSHEETPAGE *)lParam) = hPSP;
	return TRUE;
}

void FreePropCom(PropInfoStruct *po)
{
	int i;

	if ( po->DataObject != NULL )	po->DataObject->Release();
	if ( po->ShellFolder != NULL )	po->ShellFolder->Release();
	if ( po->idlist != NULL ) FreePIDL(po->idlist);
//	po->idlist = NULL;
//	po->ShellFolder = NULL;
//	po->DataObject = NULL;

	for ( i = 1 ; i < po->pages ; i++ ){
		po->COM[i].pShellProp->Release();
		po->COM[i].pShellExt->Release();
	}
}

void SavePropPos(HWND hWnd)
{
	WINPOS wpos = {{0, 0, 0, 0}, 0, 0};

	GetWindowRect(hWnd, &wpos.pos);
	SetCustTable(Str_WinPos, SYNCPROPPOSNAME, &wpos, sizeof(wpos));
}

INT_PTR CALLBACK FirstPageDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg){
		case WM_INITDIALOG: {
			TCHAR buf[VFPS], *p;
			SYNCPROPINFO *info;
			HICON hFileIcon;
			DWORD attr;
			RECT box;

			LocalizeDialogText(hDlg, 0);
			info = (SYNCPROPINFO *)(((PROPSHEETPAGE *)lParam)->lParam);

			tstrcpy(buf, info->filename);
			attr = GetFileAttributesL(buf);
			if ( attr == BADATTR ) attr = 0;
			if ( OSver.dwMajorVersion >= 6 ){
				GetWindowRect(GetDlgItem(hDlg, IDS_ICON), &box);
				box.bottom = (box.bottom - box.top) & 0xfffc;
			}else{
				box.bottom = 32;
			}
			EnterCriticalSection(&SHGetFileInfoSection);
			hFileIcon = LoadFileIcon(info->filename, attr, SHGFI_ICON,
					box.bottom, NULL);
			LeaveCriticalSection(&SHGetFileInfoSection);
			SendDlgItemMessage(hDlg, IDS_ICON, STM_SETICON, (WPARAM)hFileIcon, 0);

			p = VFSFindLastEntry(buf);
			if ( (p == buf) || (*p == '\0') ){
				SetDlgItemText(hDlg, IDE_FILENAME, p);
				SetDlgItemText(hDlg, IDE_PPATH, NilStr);
				SetDlgItemText(hDlg, IDE_FILETYPE, NilStr);
				SetDlgItemText(hDlg, IDE_PASSOC, NilStr);
			}else{
				TCHAR *extp, buf2[0x100];
				SetDlgItemText(hDlg, IDE_FILENAME, (*p == '\\') ? p + 1 : p);
				extp = p + FindExtSeparator(p);

				SetDlgItemText(hDlg, IDE_PASSOC, extp);
				if ( *extp == '.' ){
					buf2[0] = '\0';
					GetRegString(HKEY_CLASSES_ROOT, extp, NilStr, buf2, TSIZEOF(buf2));
					if ( buf2[0] != '\0' ){
						GetRegString(HKEY_CLASSES_ROOT, buf2, NilStr, buf2, TSIZEOF(buf2));
					}
					SetDlgItemText(hDlg, IDE_FILETYPE, buf2);
				}
				*p = '\0';
				SetDlgItemText(hDlg, IDE_PPATH, buf);
			}
			FormatNumber(buf, XFN_SEPARATOR | XFN_MINKILO, 6, info->ff.nFileSizeLow, info->ff.nFileSizeHigh);
			p = buf + tstrlen(buf);
			*p++ = ' ';
			*p++ = '(';
			FormatNumber(p, XFN_SEPARATOR, XFNW_FULL_SEP, info->ff.nFileSizeLow, info->ff.nFileSizeHigh);
			p += tstrlen(p);
			*p++ = ')';
			*p = '\0';
			SetDlgItemText(hDlg, IDE_PSIZE, buf);
			CnvDateTime(buf, NULL, NULL, &info->ff.ftCreationTime);
			SetDlgItemText(hDlg, IDE_PCREATETIME, buf);
			CnvDateTime(buf, NULL, NULL, &info->ff.ftLastWriteTime);
			SetDlgItemText(hDlg, IDE_PMODIFYTIME, buf);
			CnvDateTime(buf, NULL, NULL, &info->ff.ftLastAccessTime);
			SetDlgItemText(hDlg, IDE_PACCESSTIME, buf);
			break;
		}

		case WM_NOTIFY:
			#define NHPTR ((NMHDR *)lParam)
			switch( NHPTR->code ){
				case PSN_APPLY:
					break;

				default:
					return FALSE;
			}
			break;
			#undef NHPTR
		case WM_CLOSE:	// ※PPxDialogHelper では WM_CLOSE で EndDialogするから
			break;

		case WM_DESTROY:
			DestroyIcon((HICON)SendDlgItemMessage(hDlg, IDS_ICON, STM_GETICON, 0, 0));
			break;
/*
		case WM_ERASEBKGND: // Windows 11 (22000) で WM_CTLCOLORDLG が送られてこない対策
			return PPxDialogHelper(hDlg, WM_CTLCOLORDLG, wParam, lParam);
*/
		default:	// 特になし
			return PPxDialogHelper(hDlg, msg, wParam, lParam);
	}
	return TRUE;
}

int MakePropPages(PropInfoStruct *propinfo, SYNCPROPINFO *fileinfo)
{
	const TCHAR *extp;
	TCHAR buf[VFPS];
	PropInfoStruct NewProps;
	PROPSHEETPAGE psp;
	DWORD attr;

	attr = GetFileAttributesL(fileinfo->filename);

// Page 0 の設定
	psp.dwSize = sizeof(psp);
	psp.dwFlags = 0;
	psp.hInstance = hInst;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPGENERAL);
	psp.pfnDlgProc	= FirstPageDlgProc;
	psp.lParam		= (LPARAM)fileinfo;
	NewProps.sheet[0] = DCreatePropertySheetPage(&psp);
	NewProps.use = 0;
	NewProps.pages = 1;
	NewProps.ShellFolder = NULL;
	NewProps.idlist = NULL;
	NewProps.DataObject = NULL;

	if ( VFSMakeIDL(NilStr, &NewProps.ShellFolder, &NewProps.idlist, fileinfo->filename) ){
	// Page 1 以降（拡張）の設定
		if ( SUCCEEDED(NewProps.ShellFolder->GetUIObjectOf(NULL, 1,
				(LPCITEMIDLIST *)&NewProps.idlist,
				IID_IDataObject, NULL, (void **)&NewProps.DataObject) )){
		}

		// 拡張子に該当する拡張を取得
		extp = GetPathExt(fileinfo->filename);
		if ( *extp != '\0' ){
										// 拡張子からキーを求める -------------
			if ( GetRegString(HKEY_CLASSES_ROOT, extp, NilStr, buf, TSIZEOF(buf)) ){
										// アプリケーションのシェル -----------
				AddExtProps(&NewProps, buf);
			}
		}

		// * の拡張を取得
		AddExtProps(&NewProps, PAllKey);

		// ファイル・ディレクトリの拡張を取得
		if ( attr != BADATTR ){
			AddExtProps(&NewProps,
				( attr & FILE_ATTRIBUTE_DIRECTORY ) ? PDirectoryKey : PAllFileKey);
		}
	}

	// Props を更新
	::FreePropCom(propinfo);
	*propinfo = NewProps;
	return NewProps.pages;
}

void InitProcWindow(HWND hWnd)
{
	RECT pbox, bbox;
	HWND hBwnd;
	LPARAM lFont;
	WINPOS wpos;
	TCHAR PropTab[VFPS];

	hPropWnd = hWnd;
	lFont = SendMessage(hWnd, WM_GETFONT, 0, 0);

	if ( NO_ERROR == GetCustTable(Str_WinPos, SYNCPROPPOSNAME, &wpos, sizeof(wpos)) ){
		SetWindowPos(hWnd, NULL, wpos.pos.left, wpos.pos.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

#if 0	// 窓に連結するためのコード。未使用
	SetParent(hPropWnd, hParentWnd);
	SetWindowLongPtr(hPropWnd, GWL_STYLE,
		(GetWindowLongPtr(hPropWnd, GWL_STYLE) & ~(WS_OVERLAPPEDWINDOW | WS_POPUPWINDOW | WS_DLGFRAME)) | WS_CHILD  );

	GetWindowRect(hPropParentWnd, &pbox);
	SetForegroundWindow(hPropParentWnd);
#endif
										// ボタンを追加
	GetClientRect(hWnd, &pbox);
	GetWindowRect(GetDlgItem(hWnd, IDOK), &bbox);
	hBwnd = CreateWindow(WC_BUTTON, T("&<"), BS_CENTER | WS_CHILD | WS_VISIBLE | WS_TABSTOP, pbox.left + 10, pbox.bottom - (bbox.bottom-bbox.top) - 5, (bbox.bottom-bbox.top), (bbox.bottom-bbox.top), hWnd, (HMENU)K_up, hInst, NULL);
	SendMessage(hBwnd, WM_SETFONT, lFont, 0);
	hBwnd = CreateWindow(WC_BUTTON, T("&>"), BS_CENTER | WS_CHILD | WS_VISIBLE | WS_TABSTOP, pbox.left + 15+(bbox.bottom-bbox.top), pbox.bottom - (bbox.bottom-bbox.top) - 5, (bbox.bottom-bbox.top), (bbox.bottom-bbox.top), hWnd, (HMENU)K_dw, hInst, NULL);
	SendMessage(hBwnd, WM_SETFONT, lFont, 0);
	LocalizeDialogText(hWnd, 0);

	PropTab[0] = '\0';
	GetCustTable(Str_others, T("SyncPropTab"), PropTab, sizeof(PropTab));
	if ( PropTab[0] != '\0' ){
		int i, pages;
		TC_ITEM tie;
		TCHAR tabname[VFPS];
		HWND hTabWnd;

		hTabWnd = (HWND)SendMessage(hWnd, PSM_GETTABCONTROL, 0, 0);
		pages = SendMessage(hTabWnd, TCM_GETITEMCOUNT, 0, 0);

		// 表示ページを決定
		for ( i = 0 ; i < pages ; i++ ){
			tie.mask = TCIF_TEXT;
			tie.pszText = tabname;
			tie.cchTextMax = VFPS;
			tabname[0] = '\0';
			TabCtrl_GetItem(hTabWnd, i, &tie);
			if ( !tstrcmp(tabname, PropTab) ){
				pPropInfo->FirstPage = i;
				SendMessage(hWnd, PSM_SETCURSEL, i, 0);
			}
		}
	}
}

void SetFileProcWindow(HWND hWnd, SYNCPROPINFO *info)
{
	int i, oldpages, newpages;
	PropInfoStruct *propinfo;
	TCHAR filename[VFPS], oldtitle[VFPS];
	int OldPage;
	HWND hTabWnd;
	TC_ITEM tie;

	propinfo = pPropInfo;
	if ( propinfo->use > 0 ) return; // 使用中
	propinfo->use = 1;

	hTabWnd = (HWND)SendMessage(hWnd, PSM_GETTABCONTROL, 0, 0);
	OldPage = TabCtrl_GetCurSel(hTabWnd);
	oldtitle[0] = '\0';
	if ( propinfo->FirstPage != OldPage ){ // ページ変更あり
		tie.mask = TCIF_TEXT;
		tie.pszText = oldtitle;
		tie.cchTextMax = VFPS;
		TabCtrl_GetItem(hTabWnd, OldPage, &tie);
		SetCustStringTable(Str_others, T("SyncPropTab"), oldtitle, 0);
	}else{ // ページ変更無し
		GetCustTable(Str_others, T("SyncPropTab"), oldtitle, sizeof(oldtitle));
	}

	tstrcpy(filename, info->filename);
	oldpages = (int)SendMessage(hTabWnd, TCM_GETITEMCOUNT, 0, 0);
	newpages = MakePropPages(propinfo, info);
	if ( newpages > 0 ){
		SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);

		// 新しいページを末尾に追加
		for ( i = 0 ; i < newpages ; i++ ){
			SendMessage(hWnd, PSM_ADDPAGE, 0, (LPARAM)propinfo->sheet[i]);
		}
		// 古いページを削除
		for ( i = oldpages - 1 ; i >= 0 ; i-- ){
			// PropSheet_UnChanged(hWnd, hwndPage);
			SendMessage(hWnd, PSM_REMOVEPAGE, i, SendMessage(hWnd, PSM_INDEXTOPAGE, i, 0));
		}
		SetWindowText(hWnd, filename);
		// 表示ページを決定
		for ( i = 0 ; i < newpages ; i++ ){
			tie.mask = TCIF_TEXT;
			tie.pszText = filename;
			tie.cchTextMax = VFPS;
			filename[0] = '\0';
			TabCtrl_GetItem(hTabWnd, i, &tie);
			if ( !tstrcmp(filename, oldtitle) ) break;
		}
		if ( i >= newpages ) i = 0;
		propinfo->FirstPage = i;
		SendMessage(hWnd, PSM_SETCURSEL, i, 0);
		SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
	}
	propinfo->use = 0;
}

LRESULT CALLBACK ProcHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( uMsg ){
		case WM_CTLCOLORDLG:
			return PPxDialogHelper(hWnd, uMsg, wParam, lParam);

		case WM_DESTROY:
			SavePropPos(hWnd);
			break;

		case WM_COMMAND:
			if ( LOWORD(wParam) == K_up ){
				PostMessage(hPropParentWnd, WM_KEYDOWN, VK_UP, lParam);
			}
			if ( LOWORD(wParam) == K_dw ){
				PostMessage(hPropParentWnd, WM_KEYDOWN, VK_DOWN, lParam);
			}
			break;

		default:
			if ( uMsg == WM_PPXCOMMAND ){
				switch ( LOWORD(wParam) ){
					case KC_SYNCPROP_INIT:	// 初期化
						InitProcWindow(hWnd);
						break;
					case KC_SYNCPROP_SETFILE:	// ファイル指定
						SetFileProcWindow(hWnd, (SYNCPROPINFO *)lParam);
						break;
				}
			}
	}
	return CallWindowProc(OldPropProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ProcKeyHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if ( nCode >= 0 ){
		if ( !(DWORD)(lParam & B31) ){
			if ( (wParam == VK_PRIOR) || (wParam == VK_NEXT) ){
				if ( !(GetAsyncKeyState(VK_CONTROL) & KEYSTATE_PUSH) ){
					PostMessage(hPropParentWnd, WM_KEYDOWN,
							(wParam == VK_PRIOR) ? VK_UP : VK_DOWN, lParam);
					return 1;
				}
			}
		}
	}
	return CallNextHookEx(OldPropKeyProc, nCode, wParam, lParam);
}

int CALLBACK PropSheetProc(HWND hWnd, UINT uMsg, LPARAM lParam)
{
	if ( uMsg == PSCB_PRECREATE ){
		RECT pbox;

		GetWindowRect(hPropParentWnd, &pbox);

		((LPDLGTEMPLATE)lParam)->style &= ~(DS_CONTEXTHELP | DS_CENTER);
		// ↓効いていない？
		((LPDLGTEMPLATE)lParam)->x = (WORD)pbox.right;
		((LPDLGTEMPLATE)lParam)->y = (WORD)pbox.top;
	}else if ( uMsg == PSCB_INITIALIZED ){
		OldPropProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)ProcHookProc);

		OldPropKeyProc = SetWindowsHookEx(WH_KEYBOARD,
				(HOOKPROC)ProcKeyHookProc, NULL, GetCurrentThreadId());
		PostMessage(hWnd, WM_PPXCOMMAND, KC_SYNCPROP_INIT, 0);
	}
	return 0;
}

void AddExtProps(PropInfoStruct *propinfo, const TCHAR *tname)
{
//	IShellExtInit *pShellExt;
//	IShellPropSheetExt *pShellProp;
	HKEY hKey, hFolderKey;
	int enumno = 0, page;

	if ( ERROR_SUCCESS != ::RegOpenKeyEx(
			HKEY_CLASSES_ROOT, tname, 0, KEY_READ, &hFolderKey) ){
		return;
	}
	::RegOpenKeyEx(hFolderKey, PHandlersKey, 0, KEY_READ, &hKey);

	for ( ; ; ){
		TCHAR keyname[MAX_PATH];
		TCHAR idname[MAX_PATH];
		CLSID hid;
		DWORD s;
		FILETIME ft;
		HRESULT hres;

		page = propinfo->pages;
		if ( page >= PAGEMAX ) break;
		// IShellExtInit を取得
		s = MAX_PATH;
		if ( ::RegEnumKeyEx(hKey, enumno++, keyname, &s, NULL, NULL, NULL, &ft) != ERROR_SUCCESS ){
			break;
		}
		if ( keyname[0] == '{' ){
			tstrcpy(idname, keyname);
		}else{
			if ( GetRegString(hKey, keyname, NilStr, idname, TSIZEOF(idname))
					== FALSE ){
				continue;
			}
		}
		if ( IsShellExBlocked(idname) ) continue; // block check

		#ifndef UNICODE
			WCHAR idnameW[MAX_PATH];
			#define tidname idnameW

			AnsiToUnicode(idname, idnameW, MAX_PATH);
		#else
			#define tidname idname
		#endif
		if ( FAILED(::CLSIDFromString(tidname, &hid)) ) continue;

		hres = ::CoCreateInstance(hid, NULL,
				CLSCTX_INPROC_SERVER, IID_IShellExtInit, (LPVOID *)&propinfo->COM[page].pShellExt);
		if ( FAILED(hres) ) continue;
		// IShellPropSheetExt を取得
		if ( SUCCEEDED(propinfo->COM[page].pShellExt->Initialize(propinfo->idlist, propinfo->DataObject, hFolderKey)) ){
			// ●この付近でハンドルが２つリークしているかも 2015-09
			if ( SUCCEEDED(propinfo->COM[page].pShellExt->QueryInterface(
						IID_IShellPropSheetExt, (LPVOID *)&propinfo->COM[page].pShellProp)) ){
				propinfo->sheet[page] = NULL;
				propinfo->COM[page].pShellProp->AddPages(PropPageFunc, (LPARAM)&propinfo->sheet[page]);
				if ( propinfo->sheet[page] != NULL ){
					propinfo->pages++;
				}else{
					propinfo->COM[page].pShellProp->Release();
				}
			}
		}
		if ( propinfo->sheet[page] == NULL ){
			propinfo->COM[page].pShellExt->Release();
		}
	}
	::RegCloseKey(hKey);
	::RegCloseKey(hFolderKey);
}

THREADEXRET THREADEXCALL PropThread(void *infoparam)
{
	THREADSTRUCT threadstruct = {PropThreadThreadName, XTHREAD_EXITENABLE | XTHREAD_TERMENABLE, NULL, 0, 0};
	PropInfoStruct propinfo;
	PROPSHEETHEADER head;
	SYNCPROPINFO info;

	info = *(SYNCPROPINFO *)infoparam;
	::PPcHeapFree((void *)infoparam);
	propinfo.FirstPage = propinfo.pages = 0;
	propinfo.idlist = NULL;
	propinfo.ShellFolder = NULL;
	propinfo.DataObject = NULL;

	if ( FAILED(::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) ){
		t_endthreadex(0);
	}

	PPxRegisterThread(&threadstruct);

	if ( LoadWinAPI(NULL, (HINSTANCE)LoadCommonControls(ICC_TAB_CLASSES), COMCTL32DLL, LOADWINAPI_HANDLE) == NULL ){
		goto end;
	}

	// プロパティシートを表示
	head.dwSize		= sizeof(PROPSHEETHEADER);
	head.dwFlags	= PSH_USECALLBACK;
	head.hwndParent	= NULL;
	head.hInstance	= hInst;
	head.pszCaption = info.filename;
	head.nPages		= MakePropPages(&propinfo, &info);
	head.nStartPage = 0;
	head.phpage = propinfo.sheet;
	head.pfnCallback = PropSheetProc;
	pPropInfo = &propinfo;
	if ( head.nPages > 0 ) DPropertySheet(&head);

	::FreePropCom(&propinfo);
	hPropWnd = NULL;
end:
	::CoUninitialize();
	::PPxUnRegisterThread();
	t_endthreadex(0);
}

void SyncProperties(HWND hWnd, SYNCPROPINFO *info)
{
	hPropParentWnd = hWnd;
	if ( info == NULL ){	// Prop の廃棄
		if ( hPropWnd != NULL ) PostMessage(hPropWnd, WM_CLOSE, 0, 0);
		return;
	}

	if ( hPropWnd != NULL ){	// Prop の変更
		if ( !IsWindowEnabled(hPropWnd) ){
			SendMessage(hWnd, WM_PPXCOMMAND, K_SETPOPLINENOLOG, (LPARAM)T("SyncProp. : Dialog is being used."));
			return;
		}
		SendMessage(hPropWnd, WM_PPXCOMMAND, KC_SYNCPROP_SETFILE, (LPARAM)info);
	}else{						// Prop を作成
		SYNCPROPINFO *infoparam;
		DWORD tmp;

		infoparam = (SYNCPROPINFO *)PPcHeapAlloc(sizeof(SYNCPROPINFO));
		if ( infoparam != NULL ){
			*infoparam = *info;
			CloseHandle(t_beginthreadex(NULL, 0,
					PropThread, (void *)infoparam, 0, &tmp));
		}
	}
}
//================================================== IAccessible
IID xIID_IAccessible = xIID_IAccessibleIID;

HRESULT (STDAPICALLTYPE *DCreateStdAccessibleObject)(HWND, LONG, REFIID, void **);
LRESULT (STDAPICALLTYPE *DLresultFromObject)(REFIID, WPARAM, LPUNKNOWN);

LOADWINAPISTRUCT OLEACCDLL[] = {
	LOADWINAPI1(CreateStdAccessibleObject),
	LOADWINAPI1(LresultFromObject),
	{NULL, NULL}
};

LOADWINAPISTRUCT OLEAUTDLL[] = {
	LOADWINAPI1(SysAllocString),
	{NULL, NULL}
};

TYPEATTR DummyTypeAttr = {
	{0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0,}}, // GUID
	0, // LCID
	0, // DWORD dwReserved;
	MEMBERID_NIL, // MEMBERID memidConstructor;
	MEMBERID_NIL, // MEMBERID memidDestructor;
	L"", // LPOLESTR lpstrSchema;
	0, // ULONG cbSizeInstance;
	TKIND_RECORD, // TYPEKIND typekind;
	0, // WORD     cFuncs;
	0, // WORD     cVars;
	0, // WORD     cImplTypes;
	0, // WORD     cbSizeVft;
	0, // WORD     cbAlignment;
	0, // WORD     wTypeFlags;
	0, // WORD     wMajorVerNum;
	0, // WORD     wMinorVerNum;
	{0, 0}, // TYPEDESC tdescAlias;
	{0, 0}, // IDLDESC  idldescType;
};

class CDummyTypeInfo : public ITypeInfo
{
private:
	long m_cRefCount;

public:
	CDummyTypeInfo()
	{
		m_cRefCount = 1;
	}

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID /*riid*/, void **ppvObj)
	{
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}

	STDMETHOD_(ULONG, AddRef)(void)
	{
		return ++m_cRefCount;
	}

	STDMETHOD_(ULONG, Release)(void)
	{
		if ( --m_cRefCount == 0 ){
			delete this;
			return 0;
		}
		return m_cRefCount;
	}

	// IDispatch
	virtual HRESULT STDMETHODCALLTYPE GetTypeAttr(TYPEATTR **ppTypeAttr)
	{
		*ppTypeAttr = &DummyTypeAttr;
		return E_OUTOFMEMORY;
	}
	virtual HRESULT STDMETHODCALLTYPE GetTypeComp(ITypeComp **ppTComp)
	{
		return E_OUTOFMEMORY;
	}
	virtual HRESULT STDMETHODCALLTYPE GetFuncDesc(UINT index, FUNCDESC **ppFuncDesc)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE GetVarDesc(UINT index, VARDESC **ppVarDesc)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE GetNames(MEMBERID memid, BSTR *rgBstrNames, UINT cMaxNames,  UINT *pcNames)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE GetRefTypeOfImplType(UINT index, HREFTYPE *pRefType)
	{
		return TYPE_E_ELEMENTNOTFOUND;
	}

	virtual HRESULT STDMETHODCALLTYPE GetImplTypeFlags(UINT index, INT *pImplTypeFlags)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(LPOLESTR *rgszNames, UINT cNames, MEMBERID *pMemId)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Invoke(PVOID pvInstance, MEMBERID memid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDocumentation(MEMBERID memid, BSTR *pBstrName, BSTR *pBstrDocString, DWORD *pdwHelpContext, BSTR *pBstrHelpFile)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDllEntry(MEMBERID memid, INVOKEKIND invKind, BSTR *pBstrDllName, BSTR *pBstrName, WORD *pwOrdinal)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE GetRefTypeInfo(HREFTYPE hRefType, ITypeInfo **ppTInfo)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE AddressOfMember(MEMBERID memid, INVOKEKIND invKind, PVOID *ppv)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter, REFIID riid, PVOID *ppvObj)
	{
		return E_NOINTERFACE;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMops(MEMBERID memid, BSTR *pBstrMops)
	{
		return E_INVALIDARG;
	}

	virtual HRESULT STDMETHODCALLTYPE GetContainingTypeLib(ITypeLib **ppTLib, UINT *pIndex)
	{
		return E_NOINTERFACE;
	}

	virtual void STDMETHODCALLTYPE ReleaseTypeAttr(TYPEATTR *pTypeAttr)
	{
	}

	virtual void STDMETHODCALLTYPE ReleaseFuncDesc(FUNCDESC *pFuncDesc)
	{
	}

	virtual void STDMETHODCALLTYPE ReleaseVarDesc(VARDESC *pVarDesc)
	{
	}
};

// IAccessible <- IDispatch <- IUnknown
// IEnumVARIANT <- IUnknown
class CPPcAccServer : public IAccessible, public IEnumVARIANT
{
private:
	long m_cRefCount;
	PPC_APPINFO *m_cinfo;
	ULONG m_enumCount;
	IAccessible *m_AccessibleServer;

public:
	CPPcAccServer(PPC_APPINFO *cinfo);
	~CPPcAccServer();

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	// IDispatch
	STDMETHOD(GetTypeInfoCount)(_Out_ UINT *pctinfo);
	STDMETHOD(GetTypeInfo)(_In_ UINT itinfo, _In_ LCID lcid, _Out_ ITypeInfo **pptinfo);
	STDMETHOD(GetIDsOfNames)(_In_ REFIID riid, _In_ OLECHAR **rgszNames,
			_In_ UINT cNames, _In_ LCID lcid, _Out_ DISPID *rgdispid);
	STDMETHOD(Invoke)(_In_ DISPID dispidMember, _In_ REFIID riid,
			_In_ LCID lcid, _In_ WORD wFlags, DISPPARAMS *pdispparams,
			_Out_ VARIANT *pvarResult, _Out_ EXCEPINFO *pexcepinfo,
			_Out_ UINT *puArgErr);

	// IAccessible
	STDMETHOD(get_accParent)(IDispatch **ppdispParent);
	STDMETHOD(get_accChildCount)(long *pcountChildren);
	STDMETHOD(get_accChild)(VARIANT varChild, IDispatch **ppdispChild);
	STDMETHOD(get_accName)(VARIANT varChild, BSTR *pszName);
	STDMETHOD(get_accValue)(VARIANT varChild, BSTR *pszValue);
	STDMETHOD(get_accDescription)(VARIANT varChild, BSTR *pszDescription);
	STDMETHOD(get_accRole)(VARIANT varChild, VARIANT *pvarRole);
	STDMETHOD(get_accState)(VARIANT varChild, VARIANT *pvarState);
	STDMETHOD(get_accHelp)(VARIANT varChild, BSTR *pszHelp);
	STDMETHOD(get_accHelpTopic)(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic);
	STDMETHOD(get_accKeyboardShortcut)(VARIANT varChild, BSTR *pszKeyboardShortcut);
	STDMETHOD(get_accFocus)(VARIANT *pvarChild);
	STDMETHOD(get_accSelection)(VARIANT *pvarChildren);
	STDMETHOD(get_accDefaultAction)(VARIANT varChild, BSTR *pszDefaultAction);
	STDMETHOD(accSelect)(long flagsSelect, VARIANT varChild);
	STDMETHOD(accLocation)(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild);
	STDMETHOD(accNavigate)(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt);
	STDMETHOD(accHitTest)(long xLeft, long yTop, VARIANT *pvarChild);
	STDMETHOD(accDoDefaultAction)(VARIANT varChild);
	STDMETHOD(put_accName)(VARIANT varChild, BSTR szName);
	STDMETHOD(put_accValue)(VARIANT varChild, BSTR szValue);

	// IEnumVARIANT
	STDMETHOD(Next)(_In_ ULONG celt, _Out_ VARIANT *rgVar, _Out_ ULONG *pCeltFetched);
	STDMETHOD(Skip)(_In_ ULONG celt);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(_Out_ IEnumVARIANT **ppEnum);

	// その他
	void free(void);
	BOOL state(void);
};

CPPcAccServer::CPPcAccServer(PPC_APPINFO *cinfo)
{
	m_cRefCount = 1;
	m_cinfo = cinfo;
	m_enumCount = 0;
	m_AccessibleServer = NULL;

	DCreateStdAccessibleObject(cinfo->info.hWnd, OBJID_CLIENT,
			xIID_IAccessible, (void **)&m_AccessibleServer);
}

CPPcAccServer::~CPPcAccServer()
{
	if ( m_AccessibleServer != NULL ) m_AccessibleServer->Release();
}

//==================================================================== IUnknown
STDMETHODIMP CPPcAccServer::QueryInterface(REFIID iid, void **ppv)
{
#if 0
	LPOLESTR lplpsz;
	char a[2000];
	StringFromCLSID(iid, &lplpsz);
	tstrcpy(a, "CLSID\\");
	UnicodeToAnsi(lplpsz, a + 6, 2000 - 6);
	CoTaskMemFree(lplpsz);
	if ( !strcmp(a + 6, "{00000000-0000-0000-C000-000000000046}") ){
		strcpy(a, "IUnknown");
	}else if ( !strcmp(a + 6, "{00000003-0000-0000-C000-000000000046}") ){
		strcpy(a, "IMarshal");
	}else if ( !strcmp(a + 6, "{00000018-0000-0000-C000-000000000046}") ){
		strcpy(a, "IStdMarshalInfo");
	}else if ( !strcmp(a + 6, "{00000019-0000-0000-C000-000000000046}") ){
		strcpy(a, "IExternalConnection");
	}else if ( !stricmp(a + 6, "{618736e0-3c3d-11cf-810c-00aa00389b71}") ){
		strcpy(a, "IAccessible");
	}else if ( !strcmp(a + 6, "{00020400-0000-0000-C000-000000000046}") ){
		strcpy(a, "IDispatch");
	}else if ( !strcmp(a + 6, "{00020404-0000-0000-C000-000000000046}") ){
		strcpy(a, "IEnumVARIANT");
	}else if ( !strcmp(a + 6, "{6D5140C1-7436-11CE-8034-00AA006009FA}") ){
		strcpy(a, "IServiceProvider");
	}else if ( !stricmp(a + 6, "{7852b78d-1cfd-41c1-a615-9c0c85960b5f}") ){
		strcpy(a, "IAccIdentity");
	}

	GetRegString(HKEY_CLASSES_ROOT, a, NilStr, a, 1000);
	MSGMSG("QueryInterface : %s", a);
#endif
	if ( (iid == IID_IUnknown) || (iid == xIID_IAccessible) ){
		*ppv = static_cast<IAccessible*>(this);
		AddRef();
		return S_OK;
	}
	if ( iid == IID_IEnumVARIANT ){
		*ppv = static_cast<IEnumVARIANT*>(this);
		AddRef();
		return S_OK;
	}
	if ( iid == IID_IDispatch ){
		*ppv = static_cast<IDispatch*>(this);
		AddRef();
		return S_OK;
	}
	// IAccIdentity, IServiceProvider, IExternalConnection, IAgileObject
	*ppv = NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CPPcAccServer::AddRef(void)
{
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CPPcAccServer::Release(void)
{
	long count = m_cRefCount;

	if ( count > 0 ){
#if 0
		m_cRefCount = count - 1;
#else
		if ( (m_cRefCount = count - 1) <= 0 ){
			MSGMSG("CPPcAccServer Delete", 0);
	//		m_cinfo->PPcAccServer = NULL;

			delete this;
			return 0;
		}
#endif
		MSGMSG("CPPcAccServer Release %d", count);
	}
	return count;
}

void CPPcAccServer::free(void)
{
	delete this;
}

BOOL CPPcAccServer::state(void)
{
	return m_AccessibleServer != NULL;
}

//=================================================================== IDispatch
#define ATEST 1

STDMETHODIMP CPPcAccServer::GetTypeInfoCount(_Out_ UINT *pctinfo)
{
	if ( pctinfo == NULL ) return E_INVALIDARG;
	*pctinfo = 0;
	return S_OK;
};

STDMETHODIMP CPPcAccServer::GetTypeInfo(_In_ UINT iTInfo, _In_ LCID, _Out_ ITypeInfo **pptinfo)
{
	if ( iTInfo != 0 ) return DISP_E_BADINDEX;
#if ATEST
	*pptinfo = NULL;
	return E_NOTIMPL;
#else
	*pptinfo = new CDummyTypeInfo;
	return S_OK;
#endif
};

STDMETHODIMP CPPcAccServer::GetIDsOfNames(_In_ REFIID, _In_ OLECHAR **, _In_ UINT cNames, _In_ LCID, _Out_ DISPID *rgDispId)
{
	for (; cNames > 0; cNames--, rgDispId++) {
		*rgDispId = DISPID_UNKNOWN;
	}
	return DISP_E_UNKNOWNNAME;
};

STDMETHODIMP CPPcAccServer::Invoke(_In_ DISPID, _In_ REFIID, _In_ LCID, _In_ WORD, DISPPARAMS *, _Out_ VARIANT *pVarResult, _Out_ EXCEPINFO *, _Out_ UINT *)
{
	if ( pVarResult != NULL ) V_VT(pVarResult) = VT_EMPTY;
	return DISP_E_NONAMEDARGS;
};

//================================================================ IEnumVARIANT
STDMETHODIMP CPPcAccServer::Next(_In_ ULONG celt, _Out_ VARIANT *rgVar, _Out_ ULONG *pCeltFetched)
{
	long childCount;
	HRESULT hr = S_OK;
	long fetched = 0;

	MSGMSG("next", 0);
	get_accChildCount(&childCount);
	if ( (rgVar == NULL) || (celt < 1) ) return E_INVALIDARG;
	for ( ULONG x = 0 ; x < celt ; x++ ){
		if ( ++m_enumCount <= (ULONG)childCount ){
			V_VT(&rgVar[x]) = VT_I4;
			V_I4(&rgVar[x]) = m_enumCount;
			fetched++;
		}else{
			V_VT(&rgVar[x]) = VT_EMPTY;
			hr = S_FALSE;
		}
	}
	if ( pCeltFetched != NULL ) *pCeltFetched = fetched;
	return hr;
}

STDMETHODIMP CPPcAccServer::Skip(_In_ ULONG celt)
{
	long childCount;

	MSGMSG("skip", 0);
	get_accChildCount(&childCount);
	if ( (m_enumCount + celt) >= (ULONG)childCount ){
		return S_FALSE;
	}else{
		m_enumCount += celt;
		return S_OK;
	}
}

STDMETHODIMP CPPcAccServer::Reset()
{
	MSGMSG("reset", 0);
	m_enumCount = 0;
	return S_OK;
}

STDMETHODIMP CPPcAccServer::Clone(_Out_ IEnumVARIANT **ppEnum)
{
	MSGMSG("clone", 0);
	CPPcAccServer *pAcc = new CPPcAccServer(m_cinfo);
	pAcc->m_enumCount = m_enumCount;
	*ppEnum = static_cast<IEnumVARIANT*>(pAcc);
	return S_OK;
}
//================================================================ IAccessible
#define mCEL(No) ((ENTRYCELL *)m_cinfo->e.CELLDATA.p)[((DWORD *)m_cinfo->e.INDEXDATA.p)[No]]

STDMETHODIMP CPPcAccServer::get_accParent(IDispatch **ppdispParent)
{
	MSGMSG("get_accParent", 0);
	return m_AccessibleServer->get_accParent(ppdispParent);
};

STDMETHODIMP CPPcAccServer::get_accChildCount(long *pcountChildren)
{
	*pcountChildren = m_cinfo->e.cellIMax;
	MSGMSG("get_accChildCount %d", *pcountChildren);
	return S_OK;
};

STDMETHODIMP CPPcAccServer::get_accChild(VARIANT varChild, IDispatch **ppdispChild)
{
	MSGMSG("get_accChild %d", V_I4(&varChild));
	*ppdispChild = NULL;
	if ( (V_VT(&varChild) != VT_I4) || (V_I4(&varChild) > m_cinfo->e.cellIMax) ){
		return E_INVALIDARG;
	}
	return S_FALSE;
};

STDMETHODIMP CPPcAccServer::get_accName(VARIANT varChild, BSTR *pszName)
{
	*pszName = NULL;

	if ( V_I4(&varChild) == CHILDID_SELF ){
		MSGMSG("get_accName CHILDID_SELF", 0);
		return m_AccessibleServer->get_accName(varChild, pszName);
	}else{
		if ( (V_VT(&varChild) != VT_I4) || (V_I4(&varChild) > m_cinfo->e.cellIMax) ){
			*pszName = NULL;
			return E_INVALIDARG;
		}

		ENTRYINDEX celln = (ENTRYINDEX)V_I4(&varChild) - 1; // 基底が0
		ENTRYCELL *cell = &mCEL(celln);
		#ifdef UNICODE
			*pszName = DSysAllocString(cell->f.cFileName);
		#else
			WCHAR name[MAX_PATH];

			AnsiToUnicode(cell->f.cFileName, name, MAX_PATH);
			*pszName = DSysAllocString(name);
		#endif
		if ( pszName == NULL ) return E_OUTOFMEMORY;
		MSGMSG("get_accName %s", pszName);
	}
	return S_OK;
};

STDMETHODIMP CPPcAccServer::get_accValue(VARIANT varChild, BSTR *pszValue)
{
	MSGMSG("get_accValue %d", V_I4(&varChild));
	*pszValue = NULL;
	if ( V_VT(&varChild) != VT_I4 ) return E_INVALIDARG;
	return DISP_E_MEMBERNOTFOUND;
};

STDMETHODIMP CPPcAccServer::get_accDescription(VARIANT varChild, BSTR *pszDescription)
{
	MSGMSG("get_accDescription", 0);
	*pszDescription = NULL;
	if ( V_VT(&varChild) != VT_I4 ) return E_INVALIDARG;
	return DISP_E_MEMBERNOTFOUND;
};

// Active Accessibility 1.3 でカスタムROLLがサポート外に
STDMETHODIMP CPPcAccServer::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
	MSGMSG("get_accRole %d", V_I4(&varChild));
	if ( V_VT(&varChild) != VT_I4 ) return E_INVALIDARG;

	V_VT(pvarRole) = VT_I4;
	if ( V_I4(&varChild) == CHILDID_SELF ){
		V_I4(pvarRole) = ROLE_SYSTEM_LIST;
	}else{
		if ( V_I4(&varChild) > m_cinfo->e.cellIMax ){
			V_VT(pvarRole) = VT_EMPTY;
			return E_INVALIDARG;
		}
		V_I4(pvarRole) = ROLE_SYSTEM_LISTITEM;
	}
	return S_OK;
};

// Active Accessibility 1.3 でカスタムROLLがサポート外に
STDMETHODIMP CPPcAccServer::get_accState(VARIANT varChild, VARIANT *pvarState)
{
	MSGMSG("get_accState %d", V_I4(&varChild));
	V_VT(pvarState) = VT_EMPTY;

	if ( V_VT(&varChild) != VT_I4 ) return E_INVALIDARG;
	if ( V_I4(&varChild) == CHILDID_SELF ){
		return m_AccessibleServer->get_accState(varChild, pvarState);
	}else{
		if ( V_I4(&varChild) > m_cinfo->e.cellIMax ) return E_INVALIDARG;

//		DWORD flags = STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_SELECTABLE;
		DWORD flags = STATE_SYSTEM_SELECTABLE;
		ENTRYINDEX celln = (ENTRYINDEX)V_I4(&varChild) - 1; // 基底が0
		ENTRYCELL *cell = &mCEL(celln);

		if ( celln == m_cinfo->e.cellN ) setflag(flags, STATE_SYSTEM_FOCUSED);
		if ( IsCellPtrMarked(cell) )     setflag(flags, STATE_SYSTEM_SELECTED);

		V_VT(pvarState) = VT_I4;
		V_I4(pvarState) = flags;
	}
	return S_OK;
};

STDMETHODIMP CPPcAccServer::get_accHelp(VARIANT, BSTR *pszHelp)
{
	MSGMSG("get_accHelp", 0);
	*pszHelp = NULL;
	return S_FALSE;
};

STDMETHODIMP CPPcAccServer::get_accHelpTopic(BSTR *pszHelpFile, VARIANT, long *)
{
	MSGMSG("get_accHelpTopic", 0);
	*pszHelpFile = NULL;
	return S_FALSE;
};

STDMETHODIMP CPPcAccServer::get_accKeyboardShortcut(VARIANT, BSTR *pszKeyboardShortcut)
{
	MSGMSG("get_accKeyboardShortcut", 0);
	*pszKeyboardShortcut = NULL;
	return S_FALSE;
};

STDMETHODIMP CPPcAccServer::get_accFocus(VARIANT *pvarChild)
{
	HRESULT hr;
	MSGMSG("get_accFocus", 0);

	hr = m_AccessibleServer->get_accFocus(pvarChild);
	if ( FAILED(hr) ) return hr;
	if ( V_VT(pvarChild) == VT_I4 ){
		V_I4(pvarChild) = m_cinfo->e.cellN + 1; // 基底が1
	}
	return S_OK;
};

STDMETHODIMP CPPcAccServer::get_accSelection(VARIANT *pvarChildren)
{
	MSGMSG("get_accSelection", 0);
	V_VT(pvarChildren) = VT_I4;
	V_I4(pvarChildren) = m_cinfo->e.cellN + 1; // 基底が1
	return S_OK;
};

STDMETHODIMP CPPcAccServer::get_accDefaultAction(VARIANT varChild, BSTR *pszDefaultAction)
{
	MSGMSG("get_accDefaultAction", 0);

	if ( V_I4(&varChild) == CHILDID_SELF ){
		*pszDefaultAction = NULL;
		return DISP_E_MEMBERNOTFOUND;
	}

	if ( (V_VT(&varChild) != VT_I4) || (V_I4(&varChild) > m_cinfo->e.cellIMax) ){
		*pszDefaultAction = NULL;
		return E_INVALIDARG;
	}
	*pszDefaultAction = DSysAllocString(L"Enter");
	return S_OK;
};

STDMETHODIMP CPPcAccServer::accSelect(long flagsSelect, VARIANT varChild)
{
	MSGMSG("accSelect", 0);

	if ( flagsSelect & ~(SELFLAG_TAKEFOCUS | SELFLAG_TAKESELECTION) ){
		return E_INVALIDARG;
	}

	if ( (V_VT(&varChild) != VT_I4) || (V_I4(&varChild) > m_cinfo->e.cellIMax) ){
		return E_INVALIDARG;
	}

	SetFocus(m_cinfo->info.hWnd);

	if ( ((flagsSelect & (SELFLAG_TAKESELECTION | SELFLAG_TAKEFOCUS)) != 0) &&
		(V_I4(&varChild) != CHILDID_SELF) ){
		MoveCellCsr(m_cinfo, ((ENTRYINDEX)V_I4(&varChild) - 1) - m_cinfo->e.cellN, NULL);
	}
	return S_OK;
};

STDMETHODIMP CPPcAccServer::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild)
{
	MSGMSG("accLocation", 0);
	POINT pos;
	*pxLeft = 0;
	*pyTop = 0;
	*pcxWidth = 0;
	*pcyHeight = 0;

	if ( V_VT(&varChild) != VT_I4 ) return E_INVALIDARG;
	if ( V_I4(&varChild) == CHILDID_SELF ){
		return m_AccessibleServer->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
	}else{
		if ( V_I4(&varChild) > m_cinfo->e.cellIMax ) return E_INVALIDARG;

		ENTRYINDEX cell = V_I4(&varChild) - 1 - m_cinfo->cellWMin;
		pos.x = CalcCellX(m_cinfo, cell);
		pos.y = CalcCellY(m_cinfo, cell);
		ClientToScreen(m_cinfo->info.hWnd, &pos);

		*pxLeft = pos.x;
		*pyTop  = pos.y;
		*pcxWidth  = m_cinfo->cel.Size.cx - m_cinfo->fontX;
		*pcyHeight = m_cinfo->cel.Size.cy;
		return S_OK;
	}
};

STDMETHODIMP CPPcAccServer::accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt)
{
	MSGMSG("accNavigate", 0);
	V_VT(pvarEndUpAt) = VT_EMPTY;

	if ( (V_VT(&varStart) != VT_I4) || (V_I4(&varStart) > m_cinfo->e.cellIMax) ){
		return E_INVALIDARG;
	}

	switch (navDir){
		case NAVDIR_FIRSTCHILD:
			if ( (V_I4(&varStart) == CHILDID_SELF) && (m_cinfo->e.cellIMax > 0) ){
				V_VT(pvarEndUpAt) = VT_I4;
				V_I4(pvarEndUpAt) = 1;
				break;
			}
			return S_FALSE;

		case NAVDIR_LASTCHILD:
			if ( (V_I4(&varStart) == CHILDID_SELF) && (m_cinfo->e.cellIMax > 0) ){
				V_VT(pvarEndUpAt) = VT_I4;
				V_I4(pvarEndUpAt) = m_cinfo->e.cellIMax;
				break;
			}
			return S_FALSE;

		case NAVDIR_NEXT:
		case NAVDIR_DOWN:
			if ( V_I4(&varStart) != CHILDID_SELF ){
				V_VT(pvarEndUpAt) = VT_I4;
				V_I4(pvarEndUpAt) = V_I4(&varStart) + 1; // 基底が1

				if ( V_I4(pvarEndUpAt) > m_cinfo->e.cellIMax ){
					V_VT(pvarEndUpAt) = VT_EMPTY;
					return S_FALSE;
				}
			}else{
				return m_AccessibleServer->accNavigate(navDir, varStart, pvarEndUpAt);
			}
			break;

		case NAVDIR_PREVIOUS:
		case NAVDIR_UP:
			if ( V_I4(&varStart) != CHILDID_SELF ){
				V_VT(pvarEndUpAt) = VT_I4;
				V_I4(pvarEndUpAt) = V_I4(&varStart) - 1;

				if ( V_I4(pvarEndUpAt) < 1 ){
					V_VT(pvarEndUpAt) = VT_EMPTY;
					return S_FALSE;
				}
			}else{
				return m_AccessibleServer->accNavigate(navDir, varStart, pvarEndUpAt);
			}
			break;

		case NAVDIR_LEFT:
		case NAVDIR_RIGHT:
			if ( V_I4(&varStart) == CHILDID_SELF ){
				return m_AccessibleServer->accNavigate(navDir, varStart, pvarEndUpAt);
			}else{
				V_VT(pvarEndUpAt) = VT_EMPTY;
				return S_FALSE;
			}
		default:
			V_VT(pvarEndUpAt) = VT_EMPTY;
			return S_FALSE;
	}
	return S_OK;
};

STDMETHODIMP CPPcAccServer::accHitTest(long xLeft, long yTop, VARIANT *pvarChild)
{
	MSGMSG("accHitTest", 0);
	int celln = -1;
	POINT pos = {static_cast<LONG>(xLeft), static_cast<LONG>(yTop)};

	V_VT(pvarChild) = VT_I4;
	ScreenToClient(m_cinfo->info.hWnd, &pos);
	GetItemTypeFromPoint(m_cinfo, &pos, &celln);
	if ( celln >= 0 ){
		V_I4(pvarChild) = celln + 1; // 基底が1
	}else{
		V_I4(pvarChild) = CHILDID_SELF;
	}
	return S_OK;
};

STDMETHODIMP CPPcAccServer::accDoDefaultAction(VARIANT varChild)
{
	MSGMSG("accDoDefaultAction", 0);
	if ( (V_VT(&varChild) != VT_I4) || (V_I4(&varChild) > m_cinfo->e.cellIMax) ){
		return E_INVALIDARG;
	}
	if ( V_I4(&varChild) != CHILDID_SELF ){
		if ( SUCCEEDED(accSelect(SELFLAG_TAKESELECTION, varChild)) ){
			PostMessage(m_cinfo->info.hWnd, WM_PPXCOMMAND, K_raw | K_cr, 0);
		}
	}
	return S_OK;
};

// Active Accessibility 1.3 でサポート外に
STDMETHODIMP CPPcAccServer::put_accName(VARIANT, BSTR)
{
	MSGMSG("put_accName", 0);
	return E_NOTIMPL;
};

// Active Accessibility 1.3 でサポート外に
STDMETHODIMP CPPcAccServer::put_accValue(VARIANT, BSTR)
{
	MSGMSG("put_accValue", 0);
	return E_NOTIMPL;
};

HMODULE hOleautDLL = NULL;
HMODULE hOleaccDLL = NULL;

LRESULT WmGetObject(PPC_APPINFO *cinfo, WPARAM wParam, LPARAM lParam)
{
/*
	if ( cinfo->PPcAccServer != NULL ){
		// LresultFromObject 内で AddRef される
		return DLresultFromObject(xIID_IAccessible, wParam,
				static_cast<IAccessible*>(cinfo->PPcAccServer));
	}
*/
	if ( hOleaccDLL == NULL ){
		if ( hOleautDLL == NULL ){
			hOleautDLL = LoadWinAPI("OLEAUT32.DLL", NULL, OLEAUTDLL, LOADWINAPI_LOAD);
			if ( hOleautDLL == NULL ){
				return DefWindowProc(cinfo->info.hWnd, WM_GETOBJECT, wParam, lParam);
			}
		}
		hOleaccDLL = LoadWinAPI("OLEACC.DLL", NULL, OLEACCDLL, LOADWINAPI_LOAD);
		if ( hOleaccDLL == NULL ){
			return DefWindowProc(cinfo->info.hWnd, WM_GETOBJECT, wParam, lParam);
		}
	}

	CPPcAccServer *pAccServer = new CPPcAccServer(cinfo);
	if ( pAccServer != NULL ){
		if ( pAccServer->state() == FALSE ){
			pAccServer->free();
		}else{
			LRESULT lObject;
//			cinfo->PPcAccServer = static_cast<void*>(pAccServer);
			if ( DNotifyWinEvent == DummyNotifyWinEvent ){
				GETDLLPROC(GetModuleHandle(StrUser32DLL), NotifyWinEvent);
				if ( DNotifyWinEvent == NULL ){
					DNotifyWinEvent = DummyNotifyWinEvent;
				}
			}
			// LresultFromObject 内で AddRef される
			lObject = DLresultFromObject(xIID_IAccessible, wParam,
					static_cast<IAccessible*>(pAccServer));
			pAccServer->Release();
			return lObject;
		}
	}
	return DefWindowProc(cinfo->info.hWnd, WM_GETOBJECT, wParam, lParam);
}

void FreeAccServer(PPC_APPINFO *cinfo)
{
	if ( cinfo->PPcAccServer != NULL ){
//		static_cast<IAccessible*>(cinfo->PPcAccServer)->Release();
		static_cast<CPPcAccServer*>(cinfo->PPcAccServer)->free();
//		cinfo->PPcAccServer = NULL;
	}
}

BOOL GetExtentionRegString(const TCHAR *extention, const TCHAR *path, const TCHAR *itemname, TCHAR *text, SIZE32_T textlength)
{
	TCHAR pathbuf[MAX_PATH];
	TCHAR extname[MAX_PATH];

	thprintf(pathbuf, TSIZEOF(pathbuf), T("%s\\%s"), extention, path);
	if ( IsTrue(GetRegString(HKEY_CLASSES_ROOT, pathbuf, itemname, text, textlength)) ){
		return TRUE;
	}
	if ( GetRegString(HKEY_CLASSES_ROOT, extention, NilStr, extname, MAX_PATH) == FALSE ){
		return FALSE;
	}
	thprintf(pathbuf, TSIZEOF(pathbuf), T("%s\\%s"), extname, path);
	return GetRegString(HKEY_CLASSES_ROOT, pathbuf, itemname, text, textlength);
}

const TCHAR Ext_PreviewID[] = T("ShellEx\\{8895b1c6-b41f-4c1c-a562-0d564250836f}");

void ClosePreviewWindow(HWND hWnd)
{
	IPreviewHandler *pph;

	pph = (IPreviewHandler *)GetProp(hWnd, T("IPrevHandle"));
	if ( pph != NULL ){
		pph->Release();
		RemoveProp(hWnd, T("IPrevHandle"));
	}
}

void ResizePreviewWindow(HWND hWnd)
{
	IPreviewHandler *pph;

	pph = (IPreviewHandler *)GetProp(hWnd, T("IPrevHandle"));
	if ( pph != NULL ){
		RECT box;

		GetClientRect(hWnd, &box);
		pph->SetRect(&box);
	}
}

const TCHAR Ext_IFilter[] = T("PersistentHandler");

HRESULT CreateStreamOnFile(const TCHAR *filename, IStream **pStream)
{
	HANDLE hFile;
	DWORD filesize, filesizeH;
	HGLOBAL hMem;
	int mode;

	*pStream = NULL;

	VFSGetDriveType(filename, &mode, NULL);
	if ( (mode <= VFSPT_SHN_DESK) || (mode == VFSPT_SHELLSCHEME) ){
		LPITEMIDLIST pidl;
		LPSHELLFOLDER pSF;

		if ( VFSMakeIDL(NULL, &pSF, &pidl, filename) != FALSE ){
			if ( FAILED(pSF->BindToObject(pidl, NULL, IID_IStream, (void **)pStream)) ){
				*pStream = NULL;
			}
			FreePIDL(pidl);
			pSF->Release();
			if ( *pStream != NULL ) return S_OK;
		}
	}

	hFile = CreateFileL(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) return E_FAIL;

	filesize = GetFileSize(hFile, &filesizeH);
	if ( (filesize > 0) && (filesizeH == 0) &&
		 ((hMem = GlobalAlloc(GPTR, filesize)) != NULL) ){
		BYTE *ptr = (BYTE *)GlobalLock(hMem);

		if ( ptr != NULL ){
			DWORD tmp;
			BOOL isread;

			isread = ReadFile(hFile, ptr, filesize, &tmp, NULL);
			GlobalUnlock(hMem);
			if ( !isread || FAILED(CreateStreamOnHGlobal(hMem, TRUE, pStream)) ){
				*pStream = NULL;
			}
		}
	}
	CloseHandle(hFile);
	return (*pStream == NULL) ? E_FAIL : S_OK;
}

BOOL TextFromIFilter(HWND hWnd, const TCHAR *filename)
{
	IFilter *pFilter;
	ULONG flags;
	STAT_CHUNK chunk;

	#ifdef UNICODE
		#define FILENAME filename
		#define SPW_CLSID buf
	#else
		WCHAR wsz[VFPS];
		#define FILENAME wsz
		#define SPW_CLSID wsz
	#endif

	TCHAR buf[MAX_PATH], buf2[MAX_PATH];
	const TCHAR *extention;
	CLSID hid;

#if 1 // 一部だけ読み込む場合
	extention = GetPathExt(filename);

	if ( FALSE == GetExtentionRegString(extention, Ext_IFilter, NilStr, buf, MAX_PATH) ){
		return FALSE;
	}
	if ( tstrcmp(buf, T("{5e941d80-bf96-11cd-b579-08002b30bfeb}")) == 0 ){
		return FALSE; // text 形式なので省略
	}
	thprintf(buf2, TSIZEOF(buf2), T("CLSID\\%s\\PersistentAddinsRegistered\\{89BCB740-6119-101A-BCB7-00DD010655AF}"), buf);
	GetRegString(HKEY_CLASSES_ROOT, buf2, NilStr, buf, TSIZEOF(buf));

	#ifndef UNICODE
		::MultiByteToWideChar(CP_ACP, 0, buf, -1, wsz, TSIZEOF(wsz));
	#endif
	if ( FAILED(::CLSIDFromString(SPW_CLSID, &hid)) ) return FALSE;

	if ( FAILED( ::CoCreateInstance(hid, NULL,
			CLSCTX_INPROC_SERVER, xIID_IFilter, (LPVOID *)&pFilter)) ){
		XMessage(NULL, NULL, XM_DbgLOG, T("IID_IFilter error %s"), buf);
		return FALSE;
	}

	IPersistFile *ppf;
	IPersistStream *pps;
	HRESULT hres = E_FAIL;

	#ifndef UNICODE
		::MultiByteToWideChar(CP_ACP, 0, filename, -1, wsz, TSIZEOF(wsz));
	#endif

	if ( SUCCEEDED(pFilter->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf)) ){
		hres = ppf->Load(FILENAME, STGM_READ);
		ppf->Release();
	}

	if ( FAILED(hres) && SUCCEEDED(pFilter->QueryInterface(IID_IPersistStream, (LPVOID *)&pps)) ){
		IStream *ps;

		if ( SUCCEEDED(CreateStreamOnFile(filename, &ps)) ){
			hres = pps->Load(ps);
			ps->Release();
		}
		pps->Release();
	}
#else // お手軽に用意する場合
	if ( hQueryDLL == NULL ){
		hQueryDLL = LoadLibrary(T("query.dll")); // LoadSystemDLL 化予定
		if ( hQueryDLL == NULL ) return;
	}
	GETDLLPROC(hQueryDLL, LoadIFilter);

	//ファイルの拡張子に応じて、IFilterをロードする
	if ( FAILED(DLoadIFilter(FILENAME, NULL, reinterpret_cast<void**>(&pFilter))) ){
		return;
	}
#endif
	if ( SUCCEEDED(hres) ){
		hres = pFilter->Init(IFILTER_INIT_APPLY_INDEX_ATTRIBUTES, 0, 0, &flags);

		if ( SUCCEEDED(hres) ){
			#define TEXTBUFLEN 2000
			WCHAR text[TEXTBUFLEN], *lastptr = text;
			ULONG bufSize;
			hres = E_FAIL;

			while ( SUCCEEDED(pFilter->GetChunk(&chunk)) ){
				if ( chunk.flags & CHUNK_TEXT ){ //テキスト
					bufSize = TEXTBUFLEN - (lastptr - text) - 1;
					if ( SUCCEEDED(pFilter->GetText(&bufSize, lastptr)) ){
						if ( bufSize > 0 ){
							if ( lastptr[bufSize - 1] == '\0' ){
								bufSize--;
							}else{
								lastptr[bufSize] = L'\0';
							}
							lastptr += bufSize;
							if ( (lastptr - text + 2) >= TEXTBUFLEN ) break;
						}
					}
				}
			}

			if ( lastptr > text ){
				SendMessageW(hWnd, WM_SETTEXT, 0, (LPARAM)text);
				hres = S_OK;
			}
		}
	}
	pFilter->Release();
	return SUCCEEDED(hres) ? TRUE : FALSE;
}

void *SetPreviewWindow(HWND hWnd, const TCHAR *filename)
{
	TCHAR buf[MAX_PATH];
	const TCHAR *extention;
	CLSID hid;
	#ifdef UNICODE
		#define SPW_CLSID buf
	#else
		WCHAR wsz[VFPS];
		#define SPW_CLSID wsz
	#endif

	extention = GetPathExt(filename);

	if ( FALSE == GetExtentionRegString(extention, Ext_PreviewID, NilStr, buf, MAX_PATH) ){
		return NULL;
	}
	#ifndef UNICODE
		::MultiByteToWideChar(CP_ACP, 0, buf, -1, wsz, TSIZEOF(wsz));
	#endif

	if ( SUCCEEDED(::CLSIDFromString(SPW_CLSID, &hid)) ){
		IPreviewHandler *pph;

		if ( SUCCEEDED( ::CoCreateInstance(hid, NULL,
				CLSCTX_LOCAL_SERVER, IID_IPreviewHandler, (LPVOID *)&pph)) ){
			IInitializeWithFile *piwf;
			IInitializeWithStream *piws;
			HRESULT hres = E_FAIL;

			if ( SUCCEEDED(pph->QueryInterface(IID_IInitializeWithFile, (LPVOID *)&piwf)) ){
			#ifdef UNICODE
				hres = piwf->Initialize(filename, STGM_READ);
			#else
				::MultiByteToWideChar(CP_ACP, 0, filename, -1, wsz, TSIZEOF(wsz));
				hres = piwf->Initialize(wsz, STGM_READ);
			#endif
				piwf->Release();
			}

			if ( FAILED(hres) && SUCCEEDED(pph->QueryInterface(IID_IInitializeWithStream, (LPVOID *)&piws)) ){
				IStream *ps;

				if ( SUCCEEDED(CreateStreamOnFile(filename, &ps)) ){
					hres = piws->Initialize(ps, STGM_READ);
					ps->Release();
				}
				piws->Release();
			}

			if ( SUCCEEDED(hres) ){
				RECT box;

				SetProp(hWnd, T("IPrevHandle"), (HANDLE)pph);
				GetClientRect(hWnd, &box);
				pph->SetWindow(hWnd, &box);
				pph->DoPreview();
				return (void *)pph;
			}
			pph->Release();
		}
	}
	return NULL;
}

#ifndef RELEASE
#ifndef WINEGCC
void Debug_DispIID(const TCHAR *mes, REFIID riid)
{
	WCHAR *iidstring, pathW[MAX_PATH];
	TCHAR path[MAX_PATH], name[MAX_PATH];

	(void)StringFromIID(riid, &iidstring);
	strcpyW(pathW, L"Interface\\");
	strcatW(pathW, iidstring);
	CoTaskMemFree(iidstring);
#ifdef UNICODE
	strcpyW(path, pathW);
#else
	UnicodeToAnsi(pathW, path, MAX_PATH);
#endif
	if ( GetRegString(HKEY_CLASSES_ROOT, path, NilStr, name, TSIZEOF(name)) ){
		XMessage(NULL, NULL, XM_DbgLOG, T("%s %s"), mes, name);
	}else{
		XMessage(NULL, NULL, XM_DbgLOG, T("%s %s"), mes, path);
	}
}
#else
void Debug_DispIID(const TCHAR *mes, REFIID riid)
{
}
#endif
#endif
