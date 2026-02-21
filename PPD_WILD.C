/*-----------------------------------------------------------------------------
	Paper Plane xUI				ワイルドカード処理
-----------------------------------------------------------------------------*/
#define ONPPXDLL // PPCOMMON.H の DLL 定義指定
#define PXREGEXPS_DEF
#include "WINAPI.H"
#include <ole2.h>
#include <objbase.h>
#include "WINOLE.H"

#ifndef WINEGCC
#define UbstrVal bstrVal
#else
#include <wchar.h>
#define UbstrVal n1.n2.n3.bstrVal
#endif

#include "PPX.H"
#include "VFS.H"
#include "PPX_64.H"
#include "PPD_DEF.H"
#include "FATTIME.H"
#pragma hdrstop

#define PPXREGEXP void

#define EX_NONE		0		//				拡張子無し
#define EX_BOTTOM	0xffe8	//				拡張指定の末端(EX_MEM)
#define EX_MEM		0xffe8	//				追加メモリ
#define EX_NOT		0xffe9	// !			否定prefix
#define EX_AND		0xffea	// &			必須条件prefix
#define EX_ATTR		0xffeb	// attributes:	属性限定
#define EX_WRITEDATEREL 0xffec	// date:(w)	時刻限定(日数) ※以下、順番依存
#define EX_WRITEDATEABS 0xffed	// date:(w)	時刻限定(日付)
#define EX_CREATEDATEREL 0xffee	// date:c	時刻限定(日数)
#define EX_CREATEDATEABS 0xffef	// date:c	時刻限定(日付)
#define EX_ACCESSDATEREL 0xfff0	// date:a	時刻限定(日数)
#define EX_ACCESSDATEABS 0xfff1	// date:a	時刻限定(日付) ここまで依存
#define EX_SIZE		0xfff2	// size:		ファイルサイズ限定
#define EX_BREGEXP	0xfff3	// /.../		正規表現 bregexp(s/.../)
#define EX_BREGONIG	0xfff4	// /.../		正規表現 bregonig(s/.../)
#define EX_IREGEXP	0xfff5	// /.../		正規表現 IRegExp(s/.../)
#define EX_REGICU	0xfff6	// /.../		正規表現 ICU(s/.../)
#define EX_ROMA		0xfff7	// roma:		roma
#define EX_PATH		0xfff8	// path:		ディレクトリ指定prefix
#define EX_JOINEXT	0xfff9	// i: ,o:x		拡張子を分離しないで比較(_NOEXT)
#define EX_STRING	0xfffa	// o:s			パス区切りを意識しない(ディレクトリも対象に)
//#define EX_GROUP	0xfffx	// (...)		グループ化prefix
#define EX_REGEXP_LOAD_ERROR	1	// 正規表現ライブラリの読み込み失敗

#define TYPE_EQUAL	B0	// 指定値と同じ
#define TYPE_SMALL	B1	// 指定値より小さいもの
#define TYPE_LARGE	B2	// 指定値より大きいもの
#define TYPE_INVERT(type) ((type & TYPE_EQUAL) | ((type & TYPE_SMALL) << 1) | ((type & TYPE_LARGE) >> 1))

#define EXSMEMALLOCSIZE 0x40	// メモリ確保単位

int RegExpType = EX_NONE; // EX_NONE / EX_REGEXP_LOAD_ERROR / EX_BREGEXP / EX_BREGONIG / EX_IREGEXP / EX_REGICU
DWORD X_rscah = 2;
HMODULE hOleaut32DLL = NULL;

const TCHAR AttrLabelString[] = T("rhsldaxmtfpconeivbgkuqw"); // ファイル属性ラベル FILE_ATTRIBUTE_～  残:jyz

const TCHAR inclabel[] = T("dxewXXXXsl");		// o: 指定用ラベル
const TCHAR defRepOpt[] = T("i"); // Replace 用初期設定
const TCHAR preope_search[] = T("s");
const TCHAR preope_match[] = T("m");
const TCHAR preope_trans[] = T("tr");

const TCHAR StrIREGEXPPATTENERROR[] = T("IRegExp: Pattern error");

typedef struct {
	TCHAR *param;		// name内の':'の位置
	DWORD flags; 		// 拡張 REGEXPF_xxx
	BOOL useext;	//
	TCHAR name[VFPS];	// パラメータ一時抽出先
} MAKEFNSTRUCT;

//----------------------------------------------------------------- migemo 定義
typedef struct _migemo{int dummy;} migemo;
#if !defined(_WIN64) && USETASM32 // 旧版・新版共用
	extern migemo *(__stdcall * migemo_open)(const char *dict);
	extern void (__stdcall * migemo_close)(migemo *object);
	extern char *(__stdcall * migemo_query)(migemo *object, const char *query);
	extern void (__stdcall * migemo_release)(migemo *object, char *string);

	extern migemo * migemo_open_fix(const char *dict);
#else // 新版
	migemo *(__stdcall * migemo_open)(const char *dict) = NULL;
	void (__stdcall * migemo_close)(migemo *object);
	char *(__stdcall * migemo_query)(migemo *object, const char *query);
	void (__stdcall * migemo_release)(migemo *object, char *string);

	#define migemo_open_fix migemo_open
#endif

migemo *migemodic = NULL;

LOADWINAPISTRUCT MIGEMODLL[] = {
	LOADWINAPI1ND(migemo_open),
	LOADWINAPI1ND(migemo_close),
	LOADWINAPI1ND(migemo_query),
	LOADWINAPI1ND(migemo_release),
	{NULL, NULL}
};

#ifdef UNICODE
UINT migemocodepage = CP_ACP;
UINT migemocodeflags = MB_PRECOMPOSED;
const TCHAR migemodicnameUTF8[] = T("dict\\utf-8\\migemo-dict");
#endif
const TCHAR migemodicnameSJIS[] = T("dict\\cp932\\migemo-dict");
// BRegExp 関連 ---------------------------------------------------------------
typedef struct tag_bregexp {
	const char *outp;		/* result string start ptr   */
	const char *outendp;	/* result string end ptr     */
	/*const*/ int splitctr;	/* split result counter      */
	const char **splitp;	/* split result pointer ptr  */
	int rsv1;				/* reserved for external use */
	char *parap;
	char *paraendp;
	char *transtblp;
	char **startp;
	char **endp;
	int nparens;
} BREGEXP;

#define BREGEXP_MAX_ERROR_MESSAGE_LEN 80

int (USECDECL *BMatch)(const char *str, const char *target, const char *targetendp, BREGEXP **rxp, char *msg) = NULL;
int (USECDECL *BSubst)(char *str, char *target, char *targetendp, BREGEXP **rxp, char *msg);
int (USECDECL *BTrans)(char *str, char *target, char *targetendp, BREGEXP **rxp, char *msg);
void (USECDECL *BRegfree)(BREGEXP *rx);
//int (USECDECL *BSPLIT)(char *str, char *target, char *targetendp, int limit, BREGEXP **rxp, char *msg);
//char* (USECDECL *BRegexpVersion)(void);

LOADWINAPISTRUCT BREGDLL[] = {
	LOADWINAPI1ND(BMatch),
	LOADWINAPI1ND(BSubst),
	LOADWINAPI1ND(BTrans),
	LOADWINAPI1ND(BRegfree),
	{NULL, NULL}
};

// bregonig 関連 --------------------------------------------------------------
typedef struct tag_bregonig {
	const TCHAR *outp;
	const TCHAR *outendp;
	const int   splitctr; // アラインメント有り
	const TCHAR **splitp;
	INT_PTR rsv1;
	TCHAR *parap;
	TCHAR *paraendp;
	TCHAR *transtblp;
	TCHAR **startp;
	TCHAR **endp;
	int nparens;
} BREGONIG;

int (USECDECL * DBMatch)(TCHAR *str, TCHAR *target, TCHAR *targetendp, BREGONIG **rxp, TCHAR *msg) = NULL;
int (USECDECL * DBSubst)(TCHAR *str, TCHAR *target, TCHAR *targetendp, BREGONIG **rxp, TCHAR *msg);
int (USECDECL * DBTrans)(TCHAR *str, TCHAR *target, TCHAR *targetendp, BREGONIG **rxp, TCHAR *msg);
void (USECDECL * DBRegfree)(BREGONIG *rx);

#ifdef UNICODE
	#define LOADWINAPI1Tx LOADWINAPI1T
#else
	#define LOADWINAPI1Tx LOADWINAPI1
#endif

LOADWINAPISTRUCT BREGONIGDLL[] = {
	LOADWINAPI1Tx(BMatch),
	LOADWINAPI1Tx(BSubst),
	LOADWINAPI1Tx(BTrans),
	LOADWINAPI1Tx(BRegfree),
	{NULL, NULL}
};

// ICU 関連 --------------------------------------------------------------
typedef void * URegularExpression;
typedef WCHAR UChar;
typedef int UErrorCode;
#define U_PARSE_CONTEXT_LEN 16
#define U_EXPORT2 USECDECL
#define INT8_T char
#define INT32_T LONG
#define UINT32_T ULONG
typedef INT8_T UBool;
typedef struct UParseError {
	INT32_T line;
	INT32_T offset;
	UChar preContext[U_PARSE_CONTEXT_LEN];
	UChar postContext[U_PARSE_CONTEXT_LEN];
} UParseError;

typedef void * UText;

#define UREGEX_CASE_INSENSITIVE 2
#if _INTEGRAL_MAX_BITS >= 64
  #define ICU_USE_UTEXT 1
#else
  #define ICU_USE_UTEXT 0
#endif

URegularExpression * (U_EXPORT2 *uregex_open)(const UChar *pattern, INT32_T patternLength, UINT32_T flags, UParseError *pe, UErrorCode *status);

void (U_EXPORT2 *uregex_setText)(URegularExpression *regexp, const UChar *text, LONG textLength, UErrorCode *status);
UBool (U_EXPORT2 *uregex_find)(URegularExpression *regexp, INT32_T startIndex, UErrorCode *status);
INT32_T (U_EXPORT2 *uregex_group)(URegularExpression *regexp, INT32_T groupNum, UChar *dest, INT32_T destCapacity, UErrorCode *status);
void (U_EXPORT2 *uregex_close)(URegularExpression *regexp);

#if ICU_USE_UTEXT
typedef __int64 INT64_T;

UText * (U_EXPORT2 *uregex_replaceFirstUText)(URegularExpression *regexp, UText *replacement, UText *dest, UErrorCode *status);
UText * (U_EXPORT2 *uregex_replaceAllUText)(URegularExpression *regexp, UText *replacement, UText *dest, UErrorCode *status);

UText * (U_EXPORT2 *utext_openUChars)(UText *ut, const UChar *s, INT64_T length, UErrorCode *status);
UText * (U_EXPORT2 *utext_close)(UText *ut);
INT64_T (U_EXPORT2 *utext_nativeLength)(UText *ut);
INT32_T (U_EXPORT2 *utext_extract)(UText *ut, INT64_T nativeStart, INT64_T nativeLimit, UChar *dest, INT32_T destCapacity, UErrorCode *status);
#else
 INT32_T (U_EXPORT2 *uregex_replaceFirst)(URegularExpression *regexp, const UChar *replacementText, INT32_T replacementLength, UChar *destBuf, INT32_T destCapacity, UErrorCode *status);
 INT32_T (U_EXPORT2 *uregex_replaceAll)(URegularExpression *regexp, const UChar *replacementText, INT32_T replacementLength, UChar *destBuf, INT32_T destCapacity, UErrorCode *status);
#endif

LOADWINAPISTRUCT ICUDLL[] = {
	LOADWINAPI1ND(uregex_open),
	LOADWINAPI1ND(uregex_setText),
	LOADWINAPI1ND(uregex_find),
	LOADWINAPI1ND(uregex_group),
	LOADWINAPI1ND(uregex_close),
#if ICU_USE_UTEXT
	LOADWINAPI1ND(uregex_replaceAllUText),
	LOADWINAPI1ND(uregex_replaceFirstUText),
	LOADWINAPI1ND(utext_openUChars),
	LOADWINAPI1ND(utext_close),
	LOADWINAPI1ND(utext_nativeLength),
	LOADWINAPI1ND(utext_extract),
#else
	LOADWINAPI1ND(uregex_replaceAll),
	LOADWINAPI1ND(uregex_replaceFirst),
#endif
	{NULL, NULL}
};

// Oleaut32 関連定義 ----------------------------------------------------------
BSTR (STDAPICALLTYPE *DSysAllocString)(const OLECHAR *) = NULL;
BSTR (STDAPICALLTYPE *DSysAllocStringLen)(const OLECHAR *, UINT);
typedef INT (STDAPICALLTYPE *DSYSREALLOCSTRING)(BSTR *, const OLECHAR *);
typedef INT (STDAPICALLTYPE *DSYSREALLOCSTRINGLEN)(BSTR *, const OLECHAR *, UINT);
void (STDAPICALLTYPE *DSysFreeString)(BSTR);
typedef UINT (STDAPICALLTYPE *DSYSSTRINGLEN)(BSTR);

HRESULT (STDAPICALLTYPE *DGetActiveObject)(REFCLSID rclsid, void * pvReserved, IUnknown ** ppunk);
HRESULT (STDAPICALLTYPE *DRegisterActiveObject)(IUnknown * punk, REFCLSID rclsid, DWORD dwFlags, DWORD *pdwRegister);
HRESULT (STDAPICALLTYPE *DRevokeActiveObject)(DWORD dwRegister, void *pvReserved);

void (STDAPICALLTYPE *DVariantInit)(_Out_ VARIANTARG * pvarg);
HRESULT (STDAPICALLTYPE *DVariantClear)(_Inout_ VARIANTARG * pvarg);
HRESULT (STDAPICALLTYPE *DVariantChangeType)(_Inout_ VARIANTARG * pvargDest, _In_ VARIANTARG *pvarSrc, _In_ USHORT wFlags, _In_ VARTYPE vt);

LOADWINAPISTRUCT OLEAUT32_SysStr[] = {
	LOADWINAPI1(SysAllocString),
	LOADWINAPI1(SysAllocStringLen),
//	LOADWINAPI1(SysReAllocString),
//	LOADWINAPI1(SysReAllocStringLen),
	LOADWINAPI1(SysFreeString),
//	LOADWINAPI1(SysStringLen),
	LOADWINAPI1(GetActiveObject),
	LOADWINAPI1(RegisterActiveObject),
	LOADWINAPI1(RevokeActiveObject),
	{NULL, NULL}
};

LOADWINAPISTRUCT OLEAUT32_Variant[] = {
	LOADWINAPI1(VariantInit),
	LOADWINAPI1(VariantClear),
	LOADWINAPI1(VariantChangeType),
	{NULL, NULL}
};


//---------------------------------------------------- 解析時のメモリ管理の定義
typedef struct {
	BYTE *alloced;		// 確保したメモリへのポインタ NULL:使用せず
	BYTE *dest;			// 保存先
	BYTE **fixptr;		// EXS_MEM の補修位置
	DWORD size, left;	// 確保メモリと残量
} EXSMEM;

#define CEM_ERROR 0
#define CEM_COMP 1
#define CEM_ANALYZE -1
int CheckExtMode(EXSMEM *exm, MAKEFNSTRUCT *mfs);

//------------------------------------------------------------- Exs_regexp 定義
typedef struct {	// ファイル名 or *ファイル名
	WORD ext;	// 拡張子のオフセット/EX_????
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	TCHAR data[]; // 処理内容
} EXS_DATA;

typedef struct {	// 追加メモリへのポインタ
	WORD ext;	// EX_MEM
	WORD next;	// 1(解析を簡単にするため)
	BYTE *nextptr;	// 次の解析内容へのポインタ
} EXS_MEM;

typedef struct {	// ! 否定
	WORD ext;	// EX_NOT
	WORD next;	// 次の解析内容へのオフセット(0:最終)
} EXS_NOT;

typedef struct {	// & 論理積
	WORD ext;	// EX_AND
	WORD next;	// 次の解析内容へのオフセット(0:最終)
} EXS_AND;

typedef struct {	// option
	WORD ext;	// EX_NONE, EX_JOINEXT, EX_STRING 等
	WORD next;	// 次の解析内容へのオフセット(0:最終)
} EXS_OPTION;

typedef struct {	// /.../ 正規表現
	WORD ext;	// EX_BREGEXP
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	BREGONIG *rxp;
	TCHAR data[]; // 処理内容
} EXS_BONIREGEXP;

typedef struct {	// /.../ 正規表現
	WORD ext;	// EX_BREGEXP
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	BREGEXP *rxp;
	char data[]; // 処理内容
} EXS_BREGEXP;

typedef struct {	// /.../ 正規表現
	WORD ext;	// EX_IREGEXP
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	IRegExp *iregexp;
} EXS_IREGEXP;

typedef struct {	// /.../ 正規表現
	WORD ext;	// EX_REGICU
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	URegularExpression *regexp;
} EXS_REGICU;

typedef struct {	// roma: ローマ字検索
	WORD ext;	// EX_ROMA
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	DWORD_PTR handle;
	TCHAR data[]; // 処理内容
} EXS_ROMA;

typedef struct {	// attribute:.... ファイル属性
	WORD ext;	// EX_ATTR
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	DWORD mask;	// 属性マスク
	DWORD value;	// 比較値
} EXS_ATTR;

typedef struct {	// date:...days
	WORD ext;	// EX_WRITEDATEREL / EX_CREATEDATEREL / EX_ACCESSDATEREL
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	FILETIME days;	// 基準時刻
	DWORD type;
} EXS_DATE_REL;

typedef struct {	// date:...y/m/d
	WORD ext;	// EX_WRITEDATEABS / EX_CREATEDATEABS / EX_ACCESSDATEABS
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	SYSTEMTIME date;	// 基準時刻
	DWORD type;
} EXS_DATE_ABS;

typedef struct {	// size:<>...K/M/G/T
	WORD ext;	// EX_SIZE
	WORD next;	// 次の解析内容へのオフセット(0:最終)
	INTHL size;	// 基準サイズ
	DWORD type;
} EXS_SIZE;

typedef struct {	// path:
	WORD ext;	// EX_PATH
	WORD next;	// 次の解析内容へのオフセット(0:最終)
} EXS_DIR;

TCHAR *SplitRegularExpression(TCHAR *dst, const TCHAR *src);
BOOL SimpleRegularExpression(const TCHAR *src, const TCHAR *ss);
BOOL FilenameRegularExpression2(const TCHAR *fname, const TCHAR *fext, EXS_DATA *exs);

// 正規表現ライブラリ読み込み -------------------------------------------------
BOOL LoadRegexpDll(void)
{
	if ( (hBregonigDLL != NULL) || (hBregexpDLL != NULL) ) return TRUE;
	hBregonigDLL = LoadWinAPI("BREGONIG.DLL", NULL, BREGONIGDLL, LOADWINAPI_LOAD);
	if ( hBregonigDLL != NULL ) return TRUE;

	hBregexpDLL = LoadWinAPI("BREGEXP.DLL", NULL, BREGDLL, LOADWINAPI_LOAD);
	if ( hBregexpDLL != NULL ) return TRUE;
	return FALSE;
}

void FreeRegexpDll(void)
{
	if ( hBregexpDLL != NULL ){
		FreeLibrary(hBregexpDLL);
		hBregexpDLL = NULL;
	}
	if ( hBregonigDLL != NULL ){
		FreeLibrary(hBregonigDLL);
		hBregonigDLL = NULL;
		DBSubst = NULL;
	}
	if ( hIcuDLL != NULL ){
		FreeLibrary(hIcuDLL);
		hIcuDLL = NULL;
	}
	RegExpType = EX_NONE;
}

BOOL LoadRegExp(void)
{
	IRegExp *regexp;
	int X_retyp;
	HRESULT ComInitResult;

	if ( RegExpType > EX_REGEXP_LOAD_ERROR ) return TRUE;
	if ( RegExpType == EX_REGEXP_LOAD_ERROR ) return FALSE;

	X_retyp = GetCustDword(T("X_retyp"), 0);
	if ( X_retyp < 2 ){		// 0:auto / 1:bregexp or bregonig
		if ( IsTrue(LoadRegexpDll()) ){
			RegExpType = (DBSubst != NULL) ? EX_BREGONIG : EX_BREGEXP;
			return TRUE;
		}
		if ( X_retyp == 1 ) goto loaderror;
	}
	if ( X_retyp != 3 ){
							// 0 or 2 IRegExp
		if ( hOleaut32DLL == NULL ){
			hOleaut32DLL = LoadSystemWinAPI(SYSTEMDLL_OLEAUT32, OLEAUT32_SysStr);
			if ( hOleaut32DLL == NULL ) goto loaderror;

			LoadWinAPI(NULL, hOleaut32DLL, OLEAUT32_Variant, LOADWINAPI_HANDLE);
		}

		ComInitResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if ( SUCCEEDED(CoCreateInstance(&XCLSID_RegExp, NULL,
				CLSCTX_INPROC_SERVER, &XIID_IRegExp, (void **)&regexp)) ){
			IRegExp_Release(regexp);
			if ( SUCCEEDED(ComInitResult) ) CoUninitialize();
			RegExpType = EX_IREGEXP;
			return TRUE;
		}else{
			if ( SUCCEEDED(ComInitResult) ) CoUninitialize();
		}
	}
	// 0 or 3 ICU
	hIcuDLL = LoadWinAPI("icu.dll", NULL, ICUDLL, LOADWINAPI_LOAD);
	if ( hIcuDLL != NULL ){
		if ( hOleaut32DLL == NULL ) hOleaut32DLL = LoadSystemWinAPI(SYSTEMDLL_OLEAUT32, OLEAUT32_SysStr);

		RegExpType = EX_REGICU;
		return TRUE;
	}

loaderror:
	RegExpType = EX_REGEXP_LOAD_ERROR;
	PMessageBox(NULL, MES_BREX, NULL, MB_APPLMODAL | MB_OK);
	return FALSE;
}
//-----------------------------------------------------------------------------
#define MIGEMO_FREE 0
#define MIGEMO_LOADING 1
#define MIGEMO_READY 2
#define MIGEMO_LOAD_ERROR 3
volatile LONG LoadMigemo = MIGEMO_FREE;
volatile LONG Migemo_Query_count;

#define IEA_VALTYPE (LONG *)
#ifdef UNICODE
//	#ifdef __BORLANDC__
		#define SafeReadLong(value) InterlockedExchangeAdd(IEA_VALTYPE value, 0)
//	#else
//		#define SafeReadLong(value) InterlockedOr(IEA_VALTYPE value, 0)
//	#endif
	#define SafeWriteLong(value, data) InterlockedExchange(IEA_VALTYPE value, data)
#else
	#define SafeReadLong(value) (*(value))
	#define SafeWriteLong(value, data) (*(value) = (data))
#endif

void WaitMigemoLoad(void)
{
	int waitcount = 1000 / 50;

	while ( SafeReadLong(&LoadMigemo) == (LONG)MIGEMO_LOADING ){
		Sleep(50);
		if ( --waitcount == 0 ) break; // 強制待機終了
	}
}

void FreeMigemo(void)
{
	if ( SafeReadLong(&LoadMigemo) == MIGEMO_LOADING ){
		WaitMigemoLoad();
	}
	if ( hMigemoDLL == NULL ) return;
	if ( SafeReadLong(&Migemo_Query_count) != 0 ){
		int waitcount = 1000 / 50;

		while ( SafeReadLong(&Migemo_Query_count) != 0 ){
			Sleep(50);
			if ( --waitcount == 0 ) break; // 強制待機終了
		}
	}
	SafeWriteLong(&LoadMigemo, MIGEMO_LOADING);
	if ( migemodic != NULL ){
		migemo_close(migemodic);
		migemodic = NULL;
	}
	FreeLibrary(hMigemoDLL);
	hMigemoDLL = NULL;
	SafeWriteLong(&LoadMigemo, MIGEMO_FREE);
}

BOOL InitMigemo(int preload)
{
	TCHAR migemodicpath[MAX_PATH];
	LONG load_migemo;

	if ( LoadRegExp() == FALSE ) return FALSE; // 正規表現ライブラリ読み込み失敗

	load_migemo = SafeReadLong(&LoadMigemo);
	if ( load_migemo == MIGEMO_READY ) return TRUE;
	if ( load_migemo == MIGEMO_LOAD_ERROR ) return FALSE;
	WaitMigemoLoad();
	if ( SafeReadLong(&LoadMigemo) == MIGEMO_READY ) return TRUE;
	SafeWriteLong(&LoadMigemo, MIGEMO_LOADING);
	Migemo_Query_count = 0;

	hMigemoDLL = LoadWinAPI("MIGEMO.DLL", NULL, MIGEMODLL, LOADWINAPI_LOAD);
	#ifdef _WIN64
	if ( hMigemoDLL == NULL ){
		hMigemoDLL = LoadWinAPI("MIGEMO64.DLL", NULL, MIGEMODLL, LOADWINAPI_LOAD);
	}
	#endif
	#if defined(_M_ARM) || defined(_M_ARM64)
	if ( hMigemoDLL == NULL ){
		hMigemoDLL = LoadWinAPI("UNBYPASS.DLL", NULL, MIGEMODLL, LOADWINAPI_LOAD);
	}
	#endif
	if ( hMigemoDLL != NULL ){
		TCHAR *ptr;
		#ifdef UNICODE
		char migemodicpathA[MAX_PATH];
		#else
		#define migemodicpathA migemodicpath
		#endif

		GetModuleFileName(hMigemoDLL, migemodicpath, TSIZEOF(migemodicpath));
		ptr = tstrrchr(migemodicpath, '\\') + 1;

#ifdef UNICODE
		tstrcpy(ptr, migemodicnameUTF8);
		if ( GetFileAttributesL(migemodicpath) != BADATTR ){
			migemocodepage = CP_UTF8;
			migemocodeflags = 0;
		}else
#endif
		{
			tstrcpy(ptr, migemodicnameSJIS);
			if ( GetFileAttributesL(migemodicpath) == BADATTR ){
				tstrcpy(ptr + 5, migemodicnameSJIS + 11); // migemo-dict のみ
				if ( GetFileAttributesL(migemodicpath) == BADATTR ){
					migemodicpath[0] = '\0';
				}
			}
		}

		if ( migemodicpath[0] != '\0' ){
			#ifdef UNICODE
				UnicodeToAnsi(migemodicpath, migemodicpathA, MAX_PATH);
			#endif
			migemodic = migemo_open_fix(migemodicpathA);
			if ( migemodic != NULL ){
				GetCustData(T("X_rscah"), &X_rscah, sizeof X_rscah);
				SafeWriteLong(&LoadMigemo, MIGEMO_READY);
				return TRUE;
			}
		}
		PMessageBox(NULL, T("migemo-dict open error"), NULL, MB_APPLMODAL | MB_OK);

		SafeWriteLong(&LoadMigemo, MIGEMO_LOAD_ERROR);
		FreeMigemo();
	}
	if ( preload == 0 ){
		SafeWriteLong(&LoadMigemo, MIGEMO_LOAD_ERROR);
		PMessageBox(NULL, MES_MIGE, NULL, MB_APPLMODAL | MB_OK);
	}else{
		SafeWriteLong(&LoadMigemo, MIGEMO_FREE);
	}
	return FALSE;
}

typedef struct {
	BREGEXP *bregwork;
	char migemotext[];
} SRS_BREG;

typedef struct {
	BREGONIG *boniregwork;
	TCHAR migemotext[];
} SRS_BONIREG;

typedef union { // 各種ポインタ(共用体)
	void *ptr;
	IRegExp *ireg;
	URegularExpression *regicu;
	SRS_BREG *bm;
	SRS_BONIREG *bo;
} SRS_UNION;

PPXDLL BOOL PPXAPI SearchRomaString(const TCHAR *text, const TCHAR *searchstr, DWORD mode, DWORD_PTR *handles)
{
	SRS_UNION srs;
	char mesA[256];
	#ifdef UNICODE
		char textA[VFPS];
		#define TXTSTR textA
		WCHAR mes[256];
	#else
		#define TXTSTR text
		#define mes mesA
	#endif
	if ( handles == NULL ){
		return migemodic != NULL;
	}
								// 領域解放
	srs = *(SRS_UNION *)handles;
	if ( text == NULL ){
		if ( srs.ptr != NULL ){
			if ( RegExpType < EX_IREGEXP ){
				if ( RegExpType == EX_BREGONIG ){
					DBRegfree( srs.bo->boniregwork );
				}else{ // EX_BREGEXP
					BRegfree( srs.bm->bregwork );
				}
				HeapFree(DLLheap, 0, (char *)srs.bm);
			}else if ( RegExpType == EX_IREGEXP ){
				IRegExp_Release( srs.ireg );
			}else if ( RegExpType == EX_REGICU ){
				uregex_close(srs.regicu);
			}
			*handles = 0;
		}
		return TRUE;
	}
								// 新規検索
	if ( srs.ptr == NULL ){
		char *query;
		BOOL usehist = FALSE;
		BOOL usesave = FALSE;
		#ifdef UNICODE
			char searchstrA[VFPS];
			#define SSTR searchstrA
		#else
			#define SSTR searchstr
		#endif

		if ( InitMigemo(0) == FALSE ) return TRUE;

		if ( RegExpType == EX_IREGEXP ){ // IRegExp
			if ( FAILED(CoCreateInstance(&XCLSID_RegExp, NULL,
					CLSCTX_INPROC_SERVER, &XIID_IRegExp, (void **)&srs.ireg)) ){
				return TRUE;
			}
		}
		if ( tstrlen(searchstr) <= X_rscah ){ // キャッシュ検索
			usesave = TRUE;
			UsePPx();
			query = (char *)SearchHistory(PPXH_ROMASTR, searchstr);
			if ( query != NULL ){
				query = (char *)GetHistoryData(query);
				usehist = TRUE;
			}
		}
		if ( usehist == FALSE ){	// Migemo で正規表現を取得する
			DWORD querysize;
			int chance = 0;
			#ifdef UNICODE
				WideCharToMultiByte(migemocodepage, 0, searchstr, -1, searchstrA, VFPS, NULL, NULL);
			#endif
			for (;;){
				if ( InterlockedIncrement((LPLONG)&Migemo_Query_count) < 2 ){
					query = migemo_query(migemodic, SSTR);
					InterlockedDecrement((LPLONG)&Migemo_Query_count);
					break;
				}
				InterlockedDecrement((LPLONG)&Migemo_Query_count);
				Sleep(20);
				if ( ++chance < 30 ) continue;
				query = NULL;
				break;
			}
			if ( query == NULL ){
				if ( IsTrue(usesave) ) FreePPx();
				return FALSE;
			}
			querysize = strlen32(query);
			if ( IsTrue(usesave) && (querysize > 0) && (querysize < 0xff00) ){
				if ( LimitHistory(PPXH_ROMASTR, querysize) ){
					WriteHistory(PPXH_ROMASTR, searchstr, (WORD)(querysize + 1), query);
				}
			}
		}
		if ( IsTrue(usesave) ) FreePPx();
													// RegExp を準備
		if ( RegExpType < EX_IREGEXP ){ // BRegExp
			if ( RegExpType == EX_BREGONIG ){
				size_t querylen;
				TCHAR *p;

				#ifdef UNICODE
					querylen = MultiByteToWideChar(migemocodepage, migemocodeflags, query, -1, NULL, 0);
				#else
					querylen = strlen(query);
				#endif

				srs.bo = (SRS_BONIREG *)HeapAlloc(DLLheap, 0,
						sizeof(SRS_BONIREG) + (querylen + 12) * sizeof(TCHAR));
				if ( srs.bo == NULL ) return FALSE;
				*handles = (DWORD_PTR)srs.bo;
				srs.bo->boniregwork = NULL;
				p = srs.bo->migemotext;
				*p++ = '/';
				if ( !(mode & ISEA_FLOAT) ) *p++ = '^';
				#ifdef UNICODE
					strcpyW(p + MultiByteToWideChar(migemocodepage, migemocodeflags, query, -1, p, (int)querylen) - 1, L"/i");
				#else
					strcpy(p, query);
					strcpy(p + querylen, "/ki");
				#endif
			}else{ // EX_BREGEXP
				size_t querylen;
				char *p;

				querylen = strlen(query);
				srs.bm = (SRS_BREG *)HeapAlloc(DLLheap, 0,
						sizeof(SRS_BREG) + querylen + 12);
				if ( srs.bm == NULL ) return FALSE;
				*handles = (DWORD_PTR)srs.bm;
				srs.bm->bregwork = NULL;
				p = (char *)srs.bm->migemotext;
				*p++ = '/';
				if ( !(mode & ISEA_FLOAT) ) *p++ = '^';
				strcpy(p, query);
				#ifdef UNICODE
					strcpy(p + querylen, (migemocodepage == CP_UTF8) ? "/i" : "/ki");
				#else
					strcpy(p + querylen, "/ki");
				#endif
			}
		}else if ( RegExpType == EX_IREGEXP ){
			BSTR pattern;

			IRegExp_put_IgnoreCase(srs.ireg, VARIANT_TRUE);
			#ifdef UNICODE
			{
				size_t querylen;
				WCHAR *widep;

				querylen = MultiByteToWideChar(migemocodepage, migemocodeflags, query, -1, NULL, 0);
				widep = (WCHAR *)HeapAlloc(DLLheap, 0, (querylen + 2) * sizeof(WCHAR));
				if ( widep == NULL ) return FALSE;
				MultiByteToWideChar(migemocodepage, migemocodeflags, query, -1, widep, (int)querylen);
				pattern = CreateBstring(widep);
				HeapFree(DLLheap, 0, widep);
			}
			#else
				pattern = CreateBstringA(query);
			#endif
			if ( FAILED(IRegExp_put_Pattern(srs.ireg, pattern)) ){
				XMessage(NULL, NULL, XM_GrERRld, StrIREGEXPPATTENERROR);
			}
			DSysFreeString(pattern);
		}else if ( RegExpType == EX_REGICU ){
			BSTR pattern;
			UParseError pe      = { 0 };
			UErrorCode ecode  = 0;

			#ifdef UNICODE
			{
				size_t querylen;
				WCHAR *widep;

				querylen = MultiByteToWideChar(migemocodepage, migemocodeflags, query, -1, NULL, 0);
				widep = (WCHAR *)HeapAlloc(DLLheap, 0, (querylen + 2) * sizeof(WCHAR));
				if ( widep == NULL ) return FALSE;
				MultiByteToWideChar(migemocodepage, migemocodeflags, query, -1, widep, (int)querylen);
				pattern = CreateBstring(widep);
				HeapFree(DLLheap, 0, widep);
			}
			#else
				pattern = CreateBstringA(query);
			#endif

			srs.regicu = uregex_open(pattern, -1, UREGEX_CASE_INSENSITIVE, &pe, &ecode);
			DSysFreeString(pattern);


		}
		if ( usehist == FALSE ) migemo_release(migemodic, query);
	}
													// 検索
	if ( *text == '\0' ) return FALSE; // 空文字列
	if ( RegExpType < EX_IREGEXP ){
		if ( RegExpType == EX_BREGONIG ){
			return DBMatch( srs.bo->migemotext, (TCHAR *)text,
					(TCHAR *)text + tstrlen(text), &srs.bo->boniregwork, mes);
		}else{ // EX_BREGEXP
			#ifdef UNICODE
				WideCharToMultiByte(migemocodepage, 0, text, -1, textA, VFPS, NULL, NULL);
			#endif
			return BMatch( srs.bm->migemotext, TXTSTR,
					TXTSTR + strlen(TXTSTR), &srs.bm->bregwork, mesA);
		}
	}else if ( RegExpType == EX_IREGEXP ){
		VARIANT_BOOL result;
		BSTR target;

		result = 0;
		target = CreateBstring(text);
		IRegExp_Test(srs.ireg, target, &result);
		DSysFreeString(target);
		return (BOOL)result;
	}else{ // if ( RegExpType == EX_REGICU )
		BSTR target;
		UErrorCode ecode  = 0;
		BOOL result;

		target = CreateBstring(text);
		result = uregex_find( srs.regicu, -1, &ecode);
		DSysFreeString(target);
		return result;
	}
}
//-----------------------------------------------------------------------------
BSTR CreateBstringLenA(const char *string, int size)
{
	BSTR bstring;
	UINT bsize;

	if ( size == -1 ) size = strlen32(string);
	bsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, string, size, NULL, 0);
	if ( size == -1 ) bsize--;
	bstring = DSysAllocStringLen(NULL, bsize);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, string, size, bstring, bsize + 1);
	return bstring;
}

#define ONEDAYH    0xC9
#define ONEDAYL    0x2A69C000
#define ONEHOURH   0x08
#define ONEHOURL   0x61c46800
#define ONEMINUTEL 0x23c34600
#define ONESECONDL 0x00989680
#define DAY_NONE 0
#define DAY_RELATIVE 1
#define DAY_ABSOLUTELY 2

int GetWildDate(const TCHAR **string, FILETIME *rel_time, SYSTEMTIME *abs_time)
{
	DWORD num1, num2, num3;
	const TCHAR *nowp, *oldp;
	TCHAR c;

	oldp = nowp = *string;
	num1 = (DWORD)GetDigitNumber(&nowp);

	c = SkipSpace(&nowp);
	if ( (c == '-') || (c == '/') ){ // Date 2007-1-31
		SYSTEMTIME stmptime;

		GetLocalTime(&stmptime);

		nowp++;
		num2 = (DWORD)GetDigitNumber(&nowp);

		c = SkipSpace(&nowp);
		if ( (c == '-') || (c == '/') ){ // パラメータが3つ 年-月-日
			nowp++;
			num3 = (DWORD)GetDigitNumber(&nowp);

			if ( num1 != 0 ){
				if ( num1 < 100 ) num1 = num1 < 80 ? num1 + 2000 : num1 + 1900;
				stmptime.wYear = (WORD)num1;
			}
			if ( num2 != 0 ) stmptime.wMonth = (WORD)num2;
			if ( num3 != 0 ) stmptime.wDay = (WORD)num3;
		}else{	// パラメータが2つ 月-日
			if ( num1 != 0 ) stmptime.wMonth = (WORD)num1;
			if ( num2 != 0 ) stmptime.wDay = (WORD)num2;
		}
		stmptime.wHour = 0;
		stmptime.wMinute = 0;
		stmptime.wSecond = 0;
		stmptime.wMilliseconds = 0;
		*abs_time = stmptime;

		*string = nowp;
		return DAY_ABSOLUTELY;
	}else{	// ひにち相対
		DWORD dateL, dateH, tmpH, tmpHH;

		if ( oldp == nowp ){ // 当日
			GetLocalTime(abs_time);
			abs_time->wHour = 0;
			abs_time->wMinute = 0;
			abs_time->wSecond = 0;
			abs_time->wMilliseconds = 0;
			return DAY_ABSOLUTELY;
		}

		if ( c == 'h' ){ // 時間指定
			nowp++;
			DDmul(num1, ONEHOURL, &dateL, &dateH);
			DDmul(num1, ONEHOURH, &tmpH, &tmpHH);
			dateH += tmpH;
		}else if ( c == 'm' ){ // 分指定
			nowp++;
			DDmul(num1, ONEMINUTEL, &dateL, &dateH);
		}else if ( c == 's' ){ // 秒指定
			nowp++;
			DDmul(num1, ONESECONDL, &dateL, &dateH);
		}else{ // 日数指定
			DDmul(num1, ONEDAYL, &dateL, &dateH);
			DDmul(num1, ONEDAYH, &tmpH, &tmpHH);
			dateH += tmpH;
		}

		GetSystemTimeAsFileTime(rel_time);
		SubDD(rel_time->dwLowDateTime, rel_time->dwHighDateTime, dateL, dateH);

		*string = nowp;
		return DAY_RELATIVE;
	}
}

// ワイルドカード処理メイン ---------------------------------------------------
/*-----------------------------------------------------------------------------
	１項目について判別

	ss:	command(srearchstring) NULL command(srearchstring) NULL ... NULL NULL
-----------------------------------------------------------------------------*/
BOOL SimpleRegularExpression(const TCHAR *src, const TCHAR *ss)
{
	if ( *ss == '\0' ) return FALSE;		//検索文字無し→無条件に不成立
	while( *ss != '\0' ){
		switch( *ss ){
			case '*': {				// n 文字の任意の文字
				size_t len;

				ss++;
				if ( *ss == '\0' ){			// 検索内容が付随せず→持ち越し
					int wild = 0;

					ss++;
					while( *ss != '\0' ){
						switch (*ss){
							case '?':
								wild++;
							case '*':
								ss++;
								break;
						}
						if ( *ss != '\0' ) break;
						ss++;
					}
					while( wild != 0 ){			// ? の数だけ文字を確保
						if ( Ismulti(*src) ) src++;
						if ( *src == '\0' ) return FALSE;
						src++;
						wild--;
					}
					if ( *ss == '\0' ) return TRUE; // 末尾の'*'だった
				}
										// 文字列の検索
				len = tstrlen32(ss);	// i > 0 のはず
				while ( *src ){
					src = tstrchr(src, *ss);
					if ( src == NULL ) return FALSE;
					if ( memcmp(src, ss, TSTROFF(len)) == 0 ){
						const TCHAR *src2, *ss2;

						src2 = src + len;
						ss2  = ss + len + 1;
						if ( *ss2 == '\0' ){
							if ( *src2 == '\0') return TRUE;
						}else{
							if ( SimpleRegularExpression(src2, ss2) ){
								return TRUE;
							}
						}
					}
					src++;
				}
				return FALSE;
			}
/*
			case '\\':				// 各種文字
				ss++;
				switch ( *ss ){
					case 'd':
						if ( !*src ) return FALSE;	// 対象文字が無い
						if ( !Isdigit(*src) ) return FALSE;
						if ( Ismulti(*src) ) src++;
						src++;
						break;
					case 'D':
						if ( !*src ) return FALSE;	// 対象文字が無い
						if ( Isdigit(*src) ) return FALSE;
						if ( Ismulti(*src) ) src++;
						src++;
						break;
					case 's':
						if ( !*src ) return FALSE;	// 対象文字が無い
						if ( *src != ' ' ) return FALSE;
						if ( Ismulti(*src) ) src++;
						src++;
						break;
					case 'S':
						if ( !*src ) return FALSE;	// 対象文字が無い
						if ( *src == ' ' ) return FALSE;
						if ( Ismulti(*src) ) src++;
						src++;
						break;
				}
				if ( *ss ) continue;
				break;
*/
			case '?':				// 1 文字の任意の文字
				ss++;
				if ( Ismulti(*src) ) src++;
				if ( *src == '\0' ) return FALSE;
				src++;
				if ( *ss == '\0' ){
					ss++;
					break;
				}
				// default へ
			default: {				// 通常の文字列
				size_t len;

				len = tstrlen(ss);
				if ( memcmp(src, ss, TSTROFF(len)) ) return FALSE;
				src += len;
				ss += len + 1;
			}
		}
	}
	return *src ? FALSE : TRUE;
}
//-----------------------------------------------------------------------------
BOOL FilenameRegularExpression2(const TCHAR *fname, const TCHAR *fext, EXS_DATA *exs)
{
	const TCHAR *ext;

	ext = (TCHAR *)(BYTE *)((BYTE *)exs->data + exs->ext);
	if ( *ext != '\0' ){	// 拡張子指定あり -------------------------
								// (ext[0] は 0 or '.' の二択しかない)
		if ( !*(ext + 1) ? !*fext :					// 空欄拡張子指定
				SimpleRegularExpression(fext, ext + 1) ){
			if ( exs->data[0] == '\0' ){	// ファイル名は何でもよい→完全一致
				return TRUE;
			}else{						// ファイル名で一致を調べる
				return SimpleRegularExpression(fname, exs->data);
			}
		}else{							// 拡張子が違う→一致しない
			return FALSE;
		}
	}else{						// 拡張子指定なし -------------------------
		if ( exs->data[0] == '\0' ){		// ファイル名は何でもよい→完全一致
			return TRUE;
		}else{						// ファイル名で一致を調べる
			return SimpleRegularExpression(fname, exs->data);
		}
	}
}
//-----------------------------------------------------------------------------
PPXDLL int PPXAPI FilenameRegularExpression(const TCHAR *src, FN_REGEXP *fn)
{
	TCHAR fname[VFPS];	// 比較対象のファイル名
	const TCHAR *fext;	// 比較対象の拡張子
	BYTE *b;
	BOOL result;
	BOOL usenot = FALSE;
	BOOL useand = FALSE;
	char mesbufA[256];
	#ifdef UNICODE
		char fbuf[VFPS];
		BOOL conved = FALSE;
		#define NAME fbuf
		WCHAR mesbuf[256];
	#else
		#define NAME src
		#define mesbuf mesbufA
	#endif

	b = fn->b;
	if ( ((EXS_DATA *)b)->ext == EX_NONE ) return 1; // 条件なし→すべてヒット
									// 比較対象の準備
	if ( ((EXS_DATA *)b)->ext == EX_JOINEXT ){ // 拡張子分離を必ずしない
		if ( tstrlen(src) >= VFPS ){ // OVER_VFPS_MSG
			fname[0] = '\0';
		}else{
			tstrcpy(fname, src);
			CharLower(fname);
		}
		fext = NilStr;
		b += ((EXS_DATA *)b)->next;
		if ( ((EXS_DATA *)b)->ext == EX_NONE ) return 1;
	}else{
		int extOffset;

		tstplimcpy(fname, src, VFPS);
		CharLower(fname);
		extOffset = FindExtSeparator(fname);
		if ( fname[extOffset] != '\0' ) fname[extOffset++] = '\0';
		fext = fname + extOffset;
	}

	for ( ; ; ){
		WORD ext;

		ext = ((EXS_DATA *)b)->ext;
		switch ( ext ){
			case EX_NONE:
				return 1;

			case EX_MEM:
				b = ((EXS_MEM *)b)->nextptr;
				continue;

			case EX_NOT:
				if ( ((EXS_DATA *)b)->next == 0 ) return 0;
				b += ((EXS_DATA *)b)->next;
				usenot = TRUE;
				continue;

			case EX_AND:
				if ( ((EXS_DATA *)b)->next == 0 ) return 0;
				b += ((EXS_DATA *)b)->next;
				useand = TRUE;
				continue;

			case EX_BREGEXP:
				if ( *src == '\0' ){ // 文字列が空の時
					result = 0;
					break;
				}
				#ifdef UNICODE
				if ( conved == FALSE ){
					conved = TRUE;
					UnicodeToAnsi(src, fbuf, VFPS);
				}
				#endif
				result = BMatch(((EXS_BREGEXP *)b)->data,
						NAME,
						NAME + strlen(NAME),
						&((EXS_BREGEXP *)b)->rxp, mesbufA);
				break;

			case EX_BREGONIG:
				if ( *src == '\0' ){ // 文字列が空の時
					result = 0;
					break;
				}
				result = DBMatch(((EXS_BONIREGEXP *)b)->data,
						(TCHAR *)src,
						(TCHAR *)src + tstrlen(src),
						&((EXS_BONIREGEXP *)b)->rxp, mesbuf);
				break;

			case EX_IREGEXP: {
				BSTR target;

				if ( *src == '\0' ){ // 文字列が空の時
					result = 0;
					break;
				}
				result = FALSE;
				target = CreateBstring(src);
				IRegExp_Test(((EXS_IREGEXP *)b)->iregexp, target,
						(VARIANT_BOOL *)&result);
				DSysFreeString(target);
				break;
			}

			case EX_REGICU: {
				BSTR target;
				UErrorCode ecode  = 0;

				if ( *src == '\0' ){ // 文字列が空の時
					result = 0;
					break;
				}
				result = FALSE;
				target = CreateBstring(src);
				result = uregex_find( ((EXS_REGICU *)b)->regexp, 0, &ecode);
				DSysFreeString(target);
				break;
			}

			case EX_ROMA:
				if ( ((EXS_ROMA *)b)->data[0] ){
					result = SearchRomaString(src,
							((EXS_ROMA *)b)->data,
							ISEA_FLOAT,
							&((EXS_ROMA *)b)->handle);
				}else{
					result = TRUE;
				}
				break;

			case EX_PATH:
			case EX_ATTR:
			case EX_WRITEDATEABS:
			case EX_WRITEDATEREL:
			case EX_CREATEDATEABS:
			case EX_CREATEDATEREL:
			case EX_ACCESSDATEABS:
			case EX_ACCESSDATEREL:
			case EX_SIZE:
				XMessage(NULL, NULL, XM_GrERRld, MES_EWUS);
				result = FALSE;
				break;

			default:
				if ( ext >= EX_BOTTOM ){ // 不明コード…無視する
					result = FALSE;
					break;
				}
				result = FilenameRegularExpression2(fname, fext, (EXS_DATA *)b);
				break;
		}
		{
			WORD nextsize;

			nextsize = ((EXS_DATA *)b)->next;

			if ( IsTrue(usenot) ){
				usenot = FALSE;
				useand = FALSE;
				if ( IsTrue(result) ) return 0;
				if ( nextsize == 0 ) return 1;
			}else{
				if ( IsTrue(useand) ){
					useand = FALSE;
					if ( result == FALSE ) return 0;
					if ( nextsize == 0 ) return 1;
				}else{
					if ( IsTrue(result) ) return 1;
					if ( nextsize == 0 ) return 0;
				}
			}
			b += nextsize;
		}
	}
}

BOOL CompareValue(int cp, int type)
{
	switch ( type ){
		case TYPE_EQUAL: // ==
			return (cp == 0);
		case TYPE_SMALL: // <
			return (cp < 0);
		case TYPE_LARGE: // >
			return (cp > 0);
		case TYPE_EQUAL | TYPE_SMALL: // <=
			return (cp <= 0);
		case TYPE_EQUAL | TYPE_LARGE: // >=
			return (cp >= 0);
		case TYPE_SMALL | TYPE_LARGE: // <> , ><
		case TYPE_EQUAL | TYPE_SMALL | TYPE_LARGE: // !=
			return cp;
		default:
			return 0;
	}
}

BOOL USEFASTCALL RelDateCompareValue(EXS_DATE_REL *dr, const FILETIME *datetime)
{
	return CompareValue(FuzzyCompareFileTime(&dr->days, datetime), dr->type);
}

#define ABSDATEVALUE(ftime) ((ftime.wYear * 512) + (ftime.wMonth * 32) + ftime.wDay)
BOOL USEFASTCALL AbsDateCompareValue(EXS_DATE_ABS *da, const FILETIME *datetime)
{
	SYSTEMTIME stime;
	FILETIME lfime;

	FileTimeToLocalFileTime(datetime, &lfime);
	FileTimeToSystemTime(&lfime, &stime);
	return CompareValue(ABSDATEVALUE(da->date) - ABSDATEVALUE(stime), da->type);
}

//-----------------------------------------------------------------------------
PPXDLL int PPXAPI FinddataRegularExpression(const WIN32_FIND_DATA *ff, FN_REGEXP *fn)
{
	TCHAR fname[VFPS];	// 比較対象のファイル名
	const TCHAR *fext;	// 比較対象の拡張子
	BYTE *b;
	BOOL result;
	BOOL usenot = FALSE;
	BOOL useand = FALSE;
	BOOL stringmode = FALSE;
	char mesbufA[256];
	#ifdef UNICODE
		char fbuf[VFPS];
		BOOL conved = FALSE;
		#define NAMED fbuf
		WCHAR mesbuf[256];
	#else
		#define NAMED ff->cFileName
		#define mesbuf mesbufA
	#endif

	b = fn->b;
	if ( ((EXS_DATA *)b)->ext == EX_NONE ) return 1; // 条件なし→すべてヒット

	if ( ((EXS_DATA *)b)->ext == EX_STRING ){
		stringmode = TRUE;
		b += ((EXS_DATA *)b)->next;
		if ( ((EXS_DATA *)b)->ext == EX_NONE ) return 1;
	}

									// 比較対象の準備
	{
		const TCHAR *src;
		TCHAR *dest;

		src = ff->cFileName;
		dest = fname;
		for (;;){
			#ifdef UNICODE
				WCHAR chr;

				chr = *src++;
				if ( chr == '\0' ) break;
				*dest++ = chr;
				if ( ((chr == '/') || (chr == '\\')) && !stringmode ){
					dest = fname;
				}
			#else
				char chr, chrtype;

				chr = *src++;
				if ( chr == '\0' ) break;
				chrtype = T_CHRTYPE[chr];
				*dest++ = chr;
				if ( chrtype & T_IS_KNJ ) *dest++ = *src++;
				if ( ((chr == '/') || (chr == '\\')) && !stringmode ){
					dest = fname;
				}
			#endif
		}
		*dest = '\0';
		CharLower(fname);
	}

	if ( ((EXS_DATA *)b)->ext == EX_JOINEXT ){ // 拡張子分離を必ずしない
		fext = NilStr;
		b += ((EXS_DATA *)b)->next;
		if ( ((EXS_DATA *)b)->ext == EX_NONE ) return 1;
	}else{	// ファイルの時だけ拡張子分離
		if ( !(ff->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ){
			int extOffset;

			extOffset = FindExtSeparator(fname);
			if ( fname[extOffset] != '\0' ) fname[extOffset++] = '\0';
			fext = fname + extOffset;
		}else{ // ディレクトリは分離しない
			fext = NilStr;
		}
	}
	for ( ; ; ){
		WORD ext;

		ext = ((EXS_DATA *)b)->ext;
		switch ( ext ){
			case EX_NONE:
//				return useand ? 1 : 0; // and の時はヒット扱い、or の時はなし
				return 1;

			case EX_MEM:
				b = ((EXS_MEM *)b)->nextptr;
				continue;

			case EX_PATH:
				if ( !(ff->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ){
					// ファイルは次の判定を無視する
					for ( ; ; ){
						if ( ((EXS_DATA *)b)->next == 0 ) return 0;
						b += ((EXS_DATA *)b)->next;
						if ( ((EXS_DATA *)b)->ext == EX_MEM ){
							b = ((EXS_MEM *)b)->nextptr;
							continue;
						}
						break;
					}
					// b は次の判定処理をポイントしている→次のでスキップ
				}
				// ディレクトリ→ディレクトリ判定になる
				if ( ((EXS_DATA *)b)->next == 0 ) return 0;
				b += ((EXS_DATA *)b)->next;
				if ( ((EXS_DATA *)b)->ext == EX_MEM ){
					b = ((EXS_MEM *)b)->nextptr;
				}
				continue;

			case EX_NOT:
				if ( ((EXS_DATA *)b)->next == 0 ) return 0;
				b += ((EXS_DATA *)b)->next;
				usenot = TRUE;
				continue;

			case EX_AND:
				if ( ((EXS_DATA *)b)->next == 0 ) return 0;
				b += ((EXS_DATA *)b)->next;
				useand = TRUE;
				continue;

			case EX_BREGEXP:
				if ( ff->cFileName[0] == '\0' ){ // 文字列が空の時
					result = 0;
					break;
				}
				#ifdef UNICODE
				if ( conved == FALSE ){
					conved = TRUE;
					UnicodeToAnsi(ff->cFileName, fbuf, VFPS);
				}
				#endif
				result = BMatch(((EXS_BREGEXP *)b)->data,
						NAMED,
						NAMED + strlen(NAMED),
						&((EXS_BREGEXP *)b)->rxp, mesbufA);
				break;

			case EX_BREGONIG:
				if ( ff->cFileName[0] == '\0' ){ // 文字列が空の時
					result = 0;
					break;
				}
				result = DBMatch(((EXS_BONIREGEXP *)b)->data,
						(TCHAR *)ff->cFileName,
						(TCHAR *)ff->cFileName + tstrlen(ff->cFileName),
						&((EXS_BONIREGEXP *)b)->rxp, mesbuf);
				break;

			case EX_IREGEXP: {
				BSTR target;

				if ( ff->cFileName[0] == '\0' ){ // 文字列が空の時
					result = 0;
					break;
				}
				result = FALSE;
				target = CreateBstring(ff->cFileName);
				IRegExp_Test(((EXS_IREGEXP *)b)->iregexp, target,
						(VARIANT_BOOL *)&result);
				DSysFreeString(target);
				break;
			}

			case EX_REGICU: {
				BSTR target;
				UErrorCode ecode  = 0;

				if ( ff->cFileName[0] == '\0' ){ // 文字列が空の時
					result = 0;
					break;
				}
				result = FALSE;
				target = CreateBstring(ff->cFileName);
				uregex_setText(((EXS_REGICU *)b)->regexp, (UChar *)target, -1, &ecode);
				result = uregex_find( ((EXS_REGICU *)b)->regexp, -1,  &ecode);
				DSysFreeString(target);
				break;
			}

			case EX_ROMA:
				if ( ((EXS_ROMA *)b)->data[0] ){
					result = SearchRomaString(ff->cFileName,
							((EXS_ROMA *)b)->data,
							ISEA_FLOAT,
							&((EXS_ROMA *)b)->handle);
				}else{
					result = TRUE;
				}
				break;

			case EX_ATTR:
				result = (ff->dwFileAttributes &
						((EXS_ATTR *)b)->mask) == ((EXS_ATTR *)b)->value;
				break;

			case EX_WRITEDATEREL:
				result = RelDateCompareValue(
						(EXS_DATE_REL *)b, &ff->ftLastWriteTime);
				break;
			case EX_CREATEDATEREL:
				result = RelDateCompareValue(
						(EXS_DATE_REL *)b, &ff->ftCreationTime);
				break;
			case EX_ACCESSDATEREL:
				result = RelDateCompareValue(
						(EXS_DATE_REL *)b, &ff->ftLastAccessTime);
				break;

			case EX_WRITEDATEABS:
				result = AbsDateCompareValue(
						(EXS_DATE_ABS *)b, &ff->ftLastWriteTime);
				break;
			case EX_CREATEDATEABS:
				result = AbsDateCompareValue(
						(EXS_DATE_ABS *)b, &ff->ftCreationTime);
				break;
			case EX_ACCESSDATEABS:
				result = AbsDateCompareValue(
						(EXS_DATE_ABS *)b, &ff->ftLastAccessTime);
				break;

			case EX_SIZE: {
				int cp;

				cp = CmpDD(ff->nFileSizeLow, ff->nFileSizeHigh,
						((EXS_SIZE *)b)->size.s.L, ((EXS_SIZE *)b)->size.s.H);
				result = CompareValue(cp, ((EXS_SIZE *)b)->type);
				break;
			}

			default:
				if ( ext >= EX_BOTTOM ){ // 不明コード…無視する
					result = FALSE;
					break;
				}
				result = FilenameRegularExpression2(fname, fext, (EXS_DATA *)b);
				break;
		}
		{
			WORD nextsize;

			nextsize = ((EXS_DATA *)b)->next;

			if ( IsTrue(usenot) ){
				usenot = FALSE;
				useand = FALSE;
				if ( IsTrue(result) ) return 0;
				if ( nextsize == 0 ) return 1;
			}else{
				if ( IsTrue(useand) ){
					useand = FALSE;
					if ( result == FALSE ) return 0;
					if ( nextsize == 0 ) return 1;
				}else{
					if ( IsTrue(result) ) return 1;
					if ( nextsize == 0 ) return 0;
				}
			}
			b += nextsize;
		}
	}
}

// ワイルドカードの保存内容を破棄 ---------------------------------------------
PPXDLL void PPXAPI FreeFN_REGEXP(FN_REGEXP *fn)
{
	BYTE *p, *allocptr = NULL;

	p = fn->b;
	for ( ; ; ){
		switch( ((EXS_DATA *)p)->ext ){
			case EX_MEM:
				p = allocptr = ((EXS_MEM *)p)->nextptr;
				continue;

			case EX_BREGEXP:
				BRegfree( ((EXS_BREGEXP *)p)->rxp );
				break;

			case EX_BREGONIG:
				DBRegfree( ((EXS_BONIREGEXP *)p)->rxp );
				break;

			case EX_IREGEXP:
				IRegExp_Release( ((EXS_IREGEXP *)p)->iregexp );
				break;

			case EX_REGICU:
				uregex_close( ((EXS_REGICU *)p)->regexp );
				break;

			case EX_ROMA:
				SearchRomaString(NULL, NULL, 0, &((EXS_ROMA *)p)->handle);
				break;
		}

		if ( ((EXS_DATA *)p)->next == 0 ) break;
		p += ((EXS_DATA *)p)->next;
	}
	if ( allocptr != NULL ) HeapFree(DLLheap, 0, allocptr);
}

#define DUMP_REGCODE 0
#if DUMP_REGCODE
void DumpRegCode(FN_REGEXP *fn)
{
	BYTE *b;

	b = fn->b;
	if ( ((EXS_DATA *)b)->ext == EX_NONE ){
		XMessage(NULL, NULL, XM_DbgLOG, T("no item, all hit"));
		return; // 条件なし→すべてヒット
	}

	if ( ((EXS_DATA *)b)->ext == EX_STRING ){
		b += ((EXS_DATA *)b)->next;
		if ( ((EXS_DATA *)b)->ext == EX_NONE ){
			XMessage(NULL, NULL, XM_DbgLOG, T("no string, all hit"));
			return; // 条件なし→すべてヒット
		}
	}

	if ( ((EXS_DATA *)b)->ext == EX_JOINEXT ){ // 拡張子分離を必ずしない
		XMessage(NULL, NULL, XM_DbgLOG, T("joint ext"));
		b += ((EXS_DATA *)b)->next;
		if ( ((EXS_DATA *)b)->ext == EX_NONE ){
			XMessage(NULL, NULL, XM_DbgLOG, T("joint ext ... all hit"));
			return;
		}
	}

	for ( ; ; ){
		WORD ext;

		ext = ((EXS_DATA *)b)->ext;
		switch ( ext ){
			case EX_NONE:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_NONE"));
				return;

			case EX_MEM:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_MEM"));
				b = ((EXS_MEM *)b)->nextptr;
				continue;

			case EX_PATH:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_PATH"));
				break;

			case EX_NOT:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_NOT"));
				break;

			case EX_AND:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_AND"));
				break;

			case EX_BREGEXP:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_BREGEXP %hs"), ((EXS_BREGEXP *)b)->data);
				break;

			case EX_BREGONIG:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_BREGONIG %s"), ((EXS_BONIREGEXP *)b)->data);
				break;

			case EX_IREGEXP:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_IREGEXP"));
				break;

			case EX_REGICU:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_REGICU"));
				break;

			case EX_ROMA:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_ROMA %s"), ((EXS_ROMA *)b)->data);
				break;

			case EX_ATTR:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_ATTR mask:%x value:%x"), ((EXS_ATTR *)b)->mask, ((EXS_ATTR *)b)->value);
				break;

			case EX_WRITEDATEREL:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_WRITE DATE REL"));
				break;
			case EX_CREATEDATEREL:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_CREATE DATE REL"));
				break;
			case EX_ACCESSDATEREL:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_ACCESS DATE REL"));
				break;
			case EX_WRITEDATEABS:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_WRITE DATE ABS"));
				break;
			case EX_CREATEDATEABS:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_CREATE DATE ABS"));
				break;
			case EX_ACCESSDATEABS:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_ACCESS DATE ABS"));
				break;

			case EX_SIZE:
				XMessage(NULL, NULL, XM_DbgLOG, T("EX_SIZE"));
				break;

			default:
				if ( ext >= EX_BOTTOM ){ // 不明コード…無視する
					XMessage(NULL, NULL, XM_DbgLOG, T("Unknown code %d"), ext);
					return;
				}
				{
					const TCHAR *ptr;

					XMessage(NULL, NULL, XM_DbgLOG, T("DumpFilename"));
					ptr = ((EXS_DATA *)b)->data;
					for (;;){
						if ( *ptr == '\0' ) break;
						XMessage(NULL, NULL, XM_DbgLOG, T("name = %s"), ptr);
						ptr += tstrlen(ptr) + 1;
					}

					ptr = (TCHAR *)(BYTE *)((BYTE *)((EXS_DATA *)b)->data + ((EXS_DATA *)b)->ext);
					for (;;){
						if ( *ptr == '\0' ) break;
						XMessage(NULL, NULL, XM_DbgLOG, T("ext = %s"), ptr);
						ptr += tstrlen(ptr) + 1;
					}
				}
				break;
		}
		{
			WORD nextsize;

			nextsize = ((EXS_DATA *)b)->next;
			if ( nextsize == 0 ) break;
			b += nextsize;
		}
	}
}
#endif

TCHAR *SearchSeparater(const TCHAR *text, TCHAR separater)
{
	const TCHAR *ptr = text;
	TCHAR sep = separater;

	for (;;){
		TCHAR chr;

		chr = *ptr;
		if ( chr == '\0' ) return NULL;
		if ( chr == sep ) return (TCHAR *)ptr;
		if ( chr != '\\' ){
			#ifdef UNICODE
				ptr++;
			#else
				ptr += Chrlen(chr);
			#endif
			continue;
		}
		if ( *(ptr + 1) == '\0' ) return NULL;
		ptr += 2;
	}
}

// ワイルドカード内容保存ができるかチェック、必要なら拡張。--------------------
BOOL CheckExsmem(_In_ EXSMEM *exm, DWORD size)
{
									// EXS_MEM が確保できるようにする
	while ( exm->left < (size + sizeof(EXS_MEM) ) ){
		if ( exm->alloced == NULL ){ // 新規
			exm->alloced = HeapAlloc(DLLheap, 0, EXSMEMALLOCSIZE);
			if ( exm->alloced == NULL ) return FALSE;
			exm->fixptr = &((EXS_MEM *)exm->dest)->nextptr;
			((EXS_MEM *)exm->dest)->ext = EX_MEM;
			((EXS_MEM *)exm->dest)->next = 1;
			exm->dest = exm->alloced;
			exm->size = exm->left = EXSMEMALLOCSIZE;
		}else{
			BYTE *newptr;

			newptr = HeapReAlloc(DLLheap, 0,
					exm->alloced, (size_t)exm->size + EXSMEMALLOCSIZE);
			if ( newptr == NULL ) return FALSE;
			#pragma warning(suppress:6001) // HeapReAlloc成功時のポインタ再計算
			exm->dest = newptr + (exm->dest - exm->alloced);
			exm->alloced = newptr;
			exm->size += EXSMEMALLOCSIZE;
			exm->left += EXSMEMALLOCSIZE;
		}
		*exm->fixptr = exm->alloced;
	}
	return TRUE;
}

// 正規表現を取得 -------------------------------------------------------------
BOOL RegExpWildCard(EXSMEM *exm, const TCHAR **src)
{
	char *termp;
	const TCHAR *lastsep;
	DWORD size, structsize;
	BOOL IgnoreCase = TRUE;

	if ( LoadRegExp() == FALSE ) return FALSE;	// DLL 読み込み失敗

	lastsep = SearchSeparater(*src + 1, '/');	// 区切りがあるか？
	if ( lastsep != NULL ){
		if ( (size = ToSIZE32_T(lastsep - *src)) == 0 ) return FALSE;	// '/' しか無かった
		lastsep++;
		if ( *lastsep == 'I' ){
			IgnoreCase = FALSE;
			lastsep++;
		}
	}else{
		if ( (size = tstrlen32(*src)) == 0 ) return FALSE;	// '/' しか無かった
		lastsep = *src + size; // 最後を示すように
	}

	if ( RegExpType < EX_IREGEXP ){
		if ( RegExpType == EX_BREGONIG ){
			// 4 = ("/ki" + '\0')TCHAR
			structsize = sizeof(EXS_BONIREGEXP) + (size * sizeof(TCHAR)) + 4 * sizeof(TCHAR);
			if ( CheckExsmem(exm, structsize) == FALSE ) return FALSE;

			((EXS_BONIREGEXP *)(exm->dest))->ext = EX_BREGONIG;
			((EXS_BONIREGEXP *)(exm->dest))->next = (WORD)structsize;
			((EXS_BONIREGEXP *)(exm->dest))->rxp = NULL;
			memcpy(((EXS_BONIREGEXP *)(exm->dest))->data, *src, size * sizeof(TCHAR));
			tstrcpy( ((EXS_BONIREGEXP *)(exm->dest))->data + size ,
					IgnoreCase ? T("/ki") : T("/k") );
		}else{ // EX_BREGEXP
			// 4 = ("/ki" + '\0')ANSI
			// UNICODE の時は size は概算(多め)の文字列の長さとする
			structsize = sizeof(EXS_BREGEXP) + (size * sizeof(char)) + 4;
			if ( CheckExsmem(exm, structsize) == FALSE ) return FALSE;

			((EXS_BREGEXP *)(exm->dest))->ext = EX_BREGEXP;
			((EXS_BREGEXP *)(exm->dest))->next = (WORD)structsize;
			((EXS_BREGEXP *)(exm->dest))->rxp = NULL;
			#ifdef UNICODE
				termp = ((EXS_BREGEXP *)(exm->dest))->data +
							WideCharToMultiByte(CP_ACP, 0, *src, size,
						 ((EXS_BREGEXP *)(exm->dest))->data,
						 size * sizeof(TCHAR), NULL, NULL);
			#else
				memcpy(((EXS_BREGEXP *)(exm->dest))->data, *src, size);
				termp = ((EXS_BREGEXP *)(exm->dest))->data + size;
			#endif
			strcpy(termp, IgnoreCase ? "/ki" : "/k" );
		}
	}else if ( RegExpType == EX_IREGEXP ){
		BSTR pattern;
		IRegExp *regexp;

		if ( CheckExsmem(exm, sizeof(EXS_IREGEXP)) == FALSE ) return FALSE;

		if ( FAILED(CoCreateInstance(&XCLSID_RegExp, NULL,
				CLSCTX_INPROC_SERVER, &XIID_IRegExp, (void **)&regexp)) ){
			return FALSE;
		}
		((EXS_IREGEXP *)(exm->dest))->ext = EX_IREGEXP;
		((EXS_IREGEXP *)(exm->dest))->next = (WORD)sizeof(EXS_IREGEXP);
		((EXS_IREGEXP *)(exm->dest))->iregexp = regexp;

		if ( IgnoreCase ) IRegExp_put_IgnoreCase(regexp, VARIANT_TRUE);
		pattern = CreateBstringLen(*src + 1, size - 1);
		if ( FAILED(IRegExp_put_Pattern(regexp, pattern)) ){
			XMessage(NULL, NULL, XM_GrERRld, StrIREGEXPPATTENERROR);
			IRegExp_Release(regexp);
			DSysFreeString(pattern);
			return FALSE;
		}
		DSysFreeString(pattern);
	}else if ( RegExpType == EX_REGICU ){
		BSTR pattern;
		URegularExpression *regexp;
		UParseError pe      = { 0 };
		UErrorCode ecode  = 0;

		if ( CheckExsmem(exm, sizeof(EXS_REGICU)) == FALSE ) return FALSE;

		((EXS_REGICU *)(exm->dest))->ext = EX_REGICU;
		((EXS_REGICU *)(exm->dest))->next = (WORD)sizeof(EXS_REGICU);

		pattern = CreateBstringLen(*src + 1, size - 1);
		regexp = uregex_open(pattern, -1,
				IgnoreCase ? UREGEX_CASE_INSENSITIVE : 0, &pe, &ecode);
		DSysFreeString(pattern);
		if ( regexp == NULL ){
			XMessage(NULL, NULL, XM_GrERRld, StrIREGEXPPATTENERROR);
			return FALSE;
		}else{
			((EXS_REGICU *)(exm->dest))->regexp = regexp;
		}
	}
	*src = lastsep;
	return TRUE;
}

int GetCompareType(const TCHAR **p)
{
	int type = 0;

	SkipSpace(p);
	for ( ; ; (*p)++ ){
		switch (**p){
			case '=':
				setflag(type, TYPE_EQUAL);
				continue;
			case '!':
				setflag(type, TYPE_EQUAL | TYPE_SMALL | TYPE_LARGE);
				continue;
			case '<':
				setflag(type, TYPE_SMALL);
				continue;
			case '>':
				setflag(type, TYPE_LARGE);
				continue;
		}
		break;
	}
	return type;
}
//------------------------------------------- スペシャルキャラクタを抽出する
// abc def*ght? -> *abc* *def* *ght? に分割
TCHAR *SplitWordExpression(TCHAR *dst, const TCHAR *src)
{
	const TCHAR *s;
	TCHAR c;
	SIZE32_T ms;

	c = *src;
	if ( c == '\0' ){
		*dst++ = '\0';
	}else{
		if ( c != '*' ){
			*dst++ = '*';
			if ( c == '?' ) *dst++ = '\0';
		}
		for(;;){
			s = src + 1;
			for (;;){
				c = *s;
				if ( (c == '\0') || (c == '*') || (c == '?') ) break;
				s++;
			}
			ms = ToSIZE32_T(s - src);
			memcpy(dst, src, TSTROFF(ms));
			dst += ms;
			*dst++ = '\0';

			if ( c != '?' ) *dst++ = '*';
			src = s;
			if ( c != '\0' ) continue;
			*dst++ = '\0';
			break;
		}
	}
	*dst++ = '\0';
	return dst;
}

// abcdef*ght? -> abcdef *ght ? に分割
TCHAR *SplitRegularExpression(TCHAR *dst, const TCHAR *src)
{
	const TCHAR *s;
	TCHAR c;
	SIZE32_T ms;

	if ( *src == '\0' ){
		*dst++ = '\0';
	}else{
		for(;;){
			s = src + 1;
			for (;;){
				c = *s;
				if ( (c == '\0') || (c == '*') || (c == '?') ) break;
				s++;
			}
			ms = ToSIZE32_T(s - src);
			memcpy(dst, src, TSTROFF(ms));
			dst += ms;
			*dst++ = '\0';

			src = s;
			if ( c != '\0' ) continue;
			break;
		}
	}
	*dst++ = '\0';
	return dst;
}

//---------------------------------------- ファイル判別用に中間コードを生成する
PPXDLL DWORD PPXAPI MakeFN_REGEXP(FN_REGEXP *fn, const TCHAR *src)
{
	EXSMEM exm;
	MAKEFNSTRUCT mfs;
	TCHAR sepchar;
									// 指定無し→無条件で該当する
	if ( *src == '\0' ){
		((EXS_DATA *)fn->b)->ext = EX_NONE;
		((EXS_DATA *)fn->b)->next = 0;
		return REGEXPF_BLANK;
	}
#if DUMP_REGCODE
	XMessage(NULL, NULL, XM_DbgLOG, T("MakeFN_REGEXP %s"), src);
#endif
									// 初期化
	exm.dest = fn->b;
	exm.alloced = NULL;
	exm.size = exm.left = sizeof(FN_REGEXP);

	mfs.flags = 0;
	mfs.useext = TRUE;
	sepchar = ';';
									// 検索開始
	for ( ; ; ){
		switch ( SkipSpace(&src) ){
			case '!':	// not 識別子
				src++;
				if ( *src == '\0' ) goto error;
				if ( CheckExsmem(&exm, sizeof(EXS_NOT)) == FALSE ) goto error;
				((EXS_NOT *)exm.dest)->ext = EX_NOT;
				((EXS_NOT *)exm.dest)->next = sizeof(EXS_NOT);
				exm.dest += sizeof(EXS_NOT);
				exm.left -= sizeof(EXS_NOT);
				continue;

			case '&':	// and 識別子
				src++;
				if ( *src == '\0' ) goto error;
				if ( CheckExsmem(&exm, sizeof(EXS_AND)) == FALSE ) goto error;
				((EXS_AND *)exm.dest)->ext = EX_AND;
				((EXS_AND *)exm.dest)->next = sizeof(EXS_AND);
				exm.dest += sizeof(EXS_AND);
				exm.left -= sizeof(EXS_AND);
				continue;

			case '/': // 正規表現
				if ( RegExpWildCard(&exm, &src) == FALSE ) goto error;
				break;

			default: {
				const TCHAR *nextsrc;
				DWORD extoffset, extrasize = 0;
				TCHAR *namebufdest;

				mfs.param = NULL;
										// １回分の解析文字列を準備 -----------
				namebufdest = mfs.name;
				nextsrc = src;
				if ( *nextsrc != '\"' ){
					for ( ;; ){
						TCHAR c;

						c = *nextsrc;
						if ( (c == '\0') || (c == ',') || (c == sepchar) ){
							break;
						}else if ( (c == '?') || (c == '*') ){
							extrasize += (mfs.flags & REGEXPF_WORDMATCH) ?
									sizeof(TCHAR) * 2 : sizeof(TCHAR);
						}else if ( (c == ':') &&
								(mfs.param == NULL) &&
								(!(mfs.flags & REGEXPF_COMPLIST) ||
								 (mfs.name[0] == 'R')) ){ // option:
							mfs.param = namebufdest + 1;
						}else if ( c == '\\' ){
							TCHAR cnext;

							cnext = *(nextsrc + 1);
							if ( (cnext == ',') || (cnext == ';') ){
								c = cnext;
								nextsrc++;
							}
						}
						*namebufdest++ = c;
						nextsrc++;
						if ( namebufdest < (mfs.name + VFPS) ) continue;
						if ( !(mfs.flags & REGEXPF_SILENTERROR) ){
							XMessage(NULL, NULL, XM_FaERRd, MES_EWLG);
						}
						goto error;
					}
				}else{
					nextsrc++;
					for ( ;; ){
						TCHAR c;

						c = *nextsrc;
						if ( c == '\0' ) break;
						if ( c == '\"' ){
							c = *(++nextsrc);
							if ( c != '\"' ) break;
						}else if ( (c == '?') || (c == '*') ){
							extrasize += (mfs.flags & REGEXPF_WORDMATCH) ?
									sizeof(TCHAR) * 2 : sizeof(TCHAR);
						}
						*namebufdest++ = c;
						nextsrc++;
						if ( namebufdest < (mfs.name + VFPS) ) continue;
						if ( !(mfs.flags & REGEXPF_SILENTERROR) ){
							XMessage(NULL, NULL, XM_FaERRd, MES_EWLG);
						}
						goto error;
					}
				}
				*namebufdest = '\0';

				if ( mfs.param != NULL ){ // オプションを取得
					int extresult;

					//XMessage(NULL, NULL, XM_DbgLOG, T("[%s] [%s]"),mfs.param,nextsrc);
					src = nextsrc;
					extresult = CheckExtMode(&exm, &mfs);
					if ( extresult == CEM_ERROR ) goto error;
					if ( mfs.flags & REGEXPF_WORDMATCH ) sepchar = ' ';
					if ( extresult > 0 /* CEM_COMP */ ) break;
				}

				CharLower(mfs.name);

				if ( mfs.flags & REGEXPF_WORDMATCH ){
					extrasize +=
						TSTROFF(1 + 2) + // ファイル名部 部分一致時に使う*の分
						TSTROFF(1 + 2);  // 拡張子部分一致時に使う「*」の分

					if ( *nextsrc == ' ' ){
						const TCHAR *tmpsrc;

						tmpsrc = nextsrc + 1;
						while ( *tmpsrc == ' ' ) tmpsrc++;
						if ( *tmpsrc != '\0' ){
							if ( (*tmpsrc == 'O') && (*(tmpsrc + 1) == 'R') && (*(tmpsrc + 2) == ' ') ){
								nextsrc = tmpsrc + 2;
							}else{
								if ( CheckExsmem(&exm, sizeof(EXS_AND)) == FALSE ) goto error;
								((EXS_AND *)exm.dest)->ext = EX_AND;
								((EXS_AND *)exm.dest)->next = sizeof(EXS_AND);
								exm.dest += sizeof(EXS_AND);
								exm.left -= sizeof(EXS_AND);
							}
						}
					}
				}

				// ※ファイル名部分と拡張子部分をまとめて算出し、'\0'*2分を付加
				if ( CheckExsmem(&exm,
						sizeof(EXS_DATA) + // ヘッダサイズ
						TSTROFF32(namebufdest - mfs.name) + extrasize + // 文字列長
									// ファイル名部
						TSTROFF32(1 + 1) + // 文字列Nil + 文字列群の末尾
									// 拡張子部
						TSTROFF32(1 + 1) // 文字列Nil + 文字列群の末尾
					 ) == FALSE ){
					goto error;
				}
										// 解析 -------------------------------
										// ファイル名と拡張子を分離
				if ( IsTrue(mfs.useext) ){
					const TCHAR *extp;
					extp = tstrrchr(mfs.name, '.');
					extoffset = (extp != NULL) ? ToSIZE32_T(extp - mfs.name) : tstrlen32(mfs.name);
					mfs.name[extoffset] = '\0';
				}else{
					extoffset = tstrlen32(mfs.name);
				}
									// ファイル名部分を解析
				if ( mfs.flags & REGEXPF_WORDMATCH ){
					mfs.param = SplitWordExpression(((EXS_DATA *)exm.dest)->data, mfs.name);
				}else{
					mfs.param = SplitRegularExpression(((EXS_DATA *)exm.dest)->data, mfs.name);
				}

				((EXS_DATA *)exm.dest)->ext =
						(WORD)TSTROFF(mfs.param - ((EXS_DATA *)exm.dest)->data);
				if ( *(src + extoffset) == '.' ){
					*mfs.param++ = '.';
					extoffset++;
				}
									// 拡張子部分を解析
				if ( mfs.flags & REGEXPF_WORDMATCH ){
					mfs.param = SplitWordExpression(mfs.param, mfs.name + extoffset);
				}else{
					mfs.param = SplitRegularExpression(mfs.param, mfs.name + extoffset);
				}
#if DUMP_REGCODE
				XMessage(NULL, NULL, XM_DbgLOG, T("split name %s(%d).%s(%d)"),
						mfs.name, tstrlen(mfs.name),
						mfs.name + extoffset, tstrlen(mfs.name + extoffset));
#endif
				((EXS_DATA *)exm.dest)->next = (WORD)((BYTE *)mfs.param - exm.dest);
				src = nextsrc;
			}
		}
		if ( (*src == '\0') || ((*src != ',') && (*src != sepchar)) ) break;
		while ( (*src == ',') || (*src == sepchar) ) src++;
		if ( *src == '\0' ) break;

		exm.left -= ((EXS_DATA *)exm.dest)->next;
		exm.dest += ((EXS_DATA *)exm.dest)->next;
	}							// 末端処理
	((EXS_DATA *)exm.dest)->next = 0;
#if DUMP_REGCODE
	DumpRegCode(fn);
#endif
	return mfs.flags;
error:
	((EXS_DATA *)exm.dest)->ext = EX_NONE;
	((EXS_DATA *)exm.dest)->next = 0;
	return REGEXPF_ERROR;
}
//-----------------------------------------------------------------------------
typedef struct {
	BREGEXP *bregp;
	BREGONIG *bonip;
	IRegExp *iregexp;
	URegularExpression *regicu;
//	union {
		// bregexp / bonigexp
			TCHAR *string_rb; // breg/boni 用の x/pattern/replace/ 保存場所

		// IRegExp
			BSTR string_ri; // iregexp 用の replace 保存場所
			BSTR string_result; // iregexp 用の result 保存場所

		// ICU
#if ICU_USE_UTEXT
			UText *utext_ri;
#else
			BSTR itext_ri;
#endif
			int global; // g 指定
//	} var;

	// match
	TCHAR *string_rh;

	HRESULT ComInitResult;

// bregexp / bonigexp
#define RXMODE_BREGEXP_S 0	// s/.../
#define RXMODE_BREGEXP_TR 1	// tr/.../
#define RXMODE_BREGEXP_H 2	// f/.../
#define RXMODE_BREGEXP_MAX 2
// iregexp
#define RXMODE_IREGEXP_MIN 3
#define RXMODE_IREGEXP_S 3		// s/.../
//#define RXMODE_IREGEXP_TR 4	// tr/.../ 未対応
#define RXMODE_IREGEXP_H 5		// f/.../
#define RXMODE_IREGEXP_MAX 5
// icu
#define RXMODE_ICU_MIN 6
#define RXMODE_ICU_S 6		// s/.../
//#define RXMODE_ICU_TR 7	// tr/.../ 未対応
#define RXMODE_ICU_H 8		// f/.../

	int mode;
} PXREGEXPS;

#define Issymbol(chr) ( Isgraph(chr) && !(T_CHRTYPE[(unsigned char)(chr)] & (T_IS_DIG | T_IS_UPP | T_IS_LOW)) )

size_t RegularExpressionMatch_GetItem(PXREGEXPS *rxps, IMatchCollection *MatchCollection, TCHAR *dest, size_t dest_buflen, int itemno)
{
	if ( rxps->mode <= RXMODE_BREGEXP_MAX ){	// BRegExp
		if ( DBMatch != NULL ){ // bregonig
			if ( rxps->bonip->nparens < itemno ) return 0;
			tstplimcpy(dest, rxps->bonip->startp[itemno], dest_buflen);
			tstrcpy(dest, rxps->bonip->startp[itemno]);
			return rxps->bonip->endp[itemno] - rxps->bonip->startp[itemno];

		}else{
#ifdef UNICODE
			char renameA[VFPS];

			if ( rxps->bregp->nparens < itemno ) return 0;
			strcpy(renameA, rxps->bregp->startp[itemno]);
			renameA[rxps->bregp->endp[itemno] - rxps->bregp->startp[itemno]] = '\0';
			return AnsiToUnicode(renameA, dest, dest_buflen);
#else
			if ( rxps->bregp->nparens < itemno ) return 0;
			tstplimcpy(dest, rxps->bregp->startp[itemno], dest_buflen);
			return rxps->bregp->endp[itemno] - rxps->bregp->startp[itemno];
#endif
		}
	}else if ( rxps->mode <= RXMODE_IREGEXP_MAX ){	// IRegExp
		IDispatch *dispatch;
		int len = 0;

		if ( FAILED(MatchCollection->lpVtbl->get_Item(MatchCollection, 0/*group no*/, &dispatch)) ){
			return 0;
		}

		if ( itemno == 0 ){
			IMatch *match;
			BSTR result;

			if ( FAILED(dispatch->lpVtbl->QueryInterface(dispatch, &XIID_IMatch, (void**)&match)) ){
				return 0;
			}
			dispatch->lpVtbl->Release(dispatch);
			if ( SUCCEEDED(match->lpVtbl->get_Value(match, &result)) ){
				#ifdef UNICODE
					tstrcpy(dest, result);
					len = tstrlen32(dest);
				#else
					UnicodeToAnsi(result, dest, VFPS);
					len = tstrlen(dest);
				#endif
				DSysFreeString(result);
			}
			match->lpVtbl->Release(match);
		}else{
			IMatch2 *match2;
			ISubMatches *submatches;
			VARIANT value;

			if ( FAILED(dispatch->lpVtbl->QueryInterface(dispatch, &XIID_IMatch2, (void**)&match2)) ){
				return 0;
			}
			dispatch->lpVtbl->Release(dispatch);

			match2->lpVtbl->get_SubMatches(match2, &dispatch);
			dispatch->lpVtbl->QueryInterface(dispatch, &XIID_ISubMatches, (void**)&submatches);
			dispatch->lpVtbl->Release(dispatch);

			DVariantInit(&value);
			if ( SUCCEEDED(submatches->lpVtbl->get_Item(submatches, itemno - 1, &value)) ){
				#ifdef UNICODE
					tstrcpy(dest, V_BSTR(&value));
					len = tstrlen32(dest);
				#else
					UnicodeToAnsi(V_BSTR(&value), dest, VFPS);
					len = tstrlen(dest);
				#endif
				DVariantClear(&value);
			}
			submatches->lpVtbl->Release(submatches);
			match2->lpVtbl->Release(match2);
		}
		return len;
	}else{
		return 0;
	}
}

void RegularExpressionMatch_Result(PXREGEXPS *rxps, IMatchCollection *Matchs, TCHAR *target, size_t dest_buflen)
{
	const TCHAR *fmtptr;
	TCHAR *dest;

	dest = target;
	fmtptr = rxps->string_rh;
	if ( fmtptr[0] == '\0' ){
		dest += RegularExpressionMatch_GetItem(rxps, Matchs, dest, dest_buflen, 0);
	}else{
		TCHAR *destmax;

		destmax = dest + dest_buflen - 1;
		for ( ; *fmtptr != '\0' ; ){
			if ( (*fmtptr == '$') && Isdigit(*(fmtptr + 1)) ){
				int index;

				fmtptr++;
				index = GetNumber(&fmtptr);
				dest += RegularExpressionMatch_GetItem(rxps, Matchs, dest, dest_buflen, index);
			}else{
				if ( *fmtptr == '\\' ) fmtptr++;
				#ifndef UNICODE
					if ( Iskanji(*fmtptr) ) *dest++ = *fmtptr++;
				#endif
				*dest++ = *fmtptr++;
			}
			if ( dest >= destmax ) break;
		}
	}
	*dest = '\0';
}

const TCHAR *RegExp_CopyReturn(TCHAR *dest, const TCHAR *result, size_t dest_buflen)
{
	TCHAR *last;

	last = tstplimcpy(dest, result, dest_buflen);
	if ( last < (dest + dest_buflen - 1) ) return dest;
	*dest = '\0';
	return result;
}

#ifdef UNICODE
#define RegExp_CopyReturnT(dest, result, dest_buf) RegExp_CopyReturn(dest, result, dest_buf)
#else
const char *RegExp_CopyReturnA(char *dest, const WCHAR *result, size_t dest_buflen)
{
	int len;

	len = UnicodeToAnsi(result, dest, dest_buflen);
	if ( len != 0 ) return dest;
	*(dest + dest_buflen - 1) = '\0'; // len==0...長すぎて失敗
	return dest;
}
#define RegExp_CopyReturnT(dest, result, dest_buf) RegExp_CopyReturnA(dest, result, dest_buf)
#endif

const TCHAR *RegularExpressionMatch_bregexp(PXREGEXPS *rxps, TCHAR *target, TCHAR *dest, size_t dest_buflen)
{
	TCHAR msg[BREGEXP_MAX_ERROR_MESSAGE_LEN];

	if ( DBMatch != NULL ){ // bregonig
		int result;

		msg[0] = '\0';
		result = DBMatch(rxps->string_rb, target, target + tstrlen(target),
				&rxps->bonip, msg); // msg は未利用
		if ( (result > 0) && (rxps->bonip != NULL) ){
			RegularExpressionMatch_Result(rxps, NULL, msg, dest_buflen);
			return RegExp_CopyReturn(dest, msg, dest_buflen);
		}
	}else{ // bregexp
		int result;
#ifdef UNICODE
		char targetA[VFPS];
		char renameA[VFPS];

		UnicodeToAnsi(target, targetA, VFPS);
		targetA[VFPS - 1] = '\0';
		UnicodeToAnsi(rxps->string_rb, renameA, VFPS);
		renameA[VFPS - 1] = '\0';
#else
		#define targetA target
		#define renameA rxps->string_rb
#endif
		msg[0] = '\0';
		result = BMatch(renameA, targetA, targetA + strlen(targetA),
				&rxps->bregp, (char *)msg); // msg は未利用
		if ( (result > 0) && (rxps->bregp != NULL) ){
			RegularExpressionMatch_Result(rxps, NULL, msg, dest_buflen);
			return RegExp_CopyReturn(dest, msg, dest_buflen);
		}
	}
	dest[0] = '\0';
	return dest;
}

const TCHAR *RegularExpressionMatch_iregexp(PXREGEXPS *rxps, const TCHAR *target, TCHAR *dest, size_t dest_buflen)
{
	BSTR btarget;
	IDispatch *RegDispatch = NULL;
	IMatchCollection *Matchs;

	btarget = CreateBstring(target);
	if ( FAILED(IRegExp_Execute(rxps->iregexp, btarget, &RegDispatch)) ){
		XMessage(NULL, NULL, XM_GrERRld, T("IRegExp error"));
		DSysFreeString(btarget);
		return NULL;
	}
	DSysFreeString(btarget);
	dest[0] = '\0';
	if ( SUCCEEDED(RegDispatch->lpVtbl->QueryInterface(RegDispatch, &XIID_IMatchCollection, (void **)&Matchs)) ){
		RegularExpressionMatch_Result(rxps, Matchs, dest, dest_buflen);
		Matchs->lpVtbl->Release(Matchs);
	}
	RegDispatch->lpVtbl->Release(RegDispatch);
	return dest;
}

BOOL InitRegularExpression(PXREGEXPS **rxpsptr, TCHAR *rxstring, BOOL slash)
{
	TCHAR *slist, *rlist, separater = '/';
	const TCHAR *option = defRepOpt, *preope = preope_search;
	TCHAR buf[VFPS];
	PXREGEXPS *rxps;

	if ( LoadRegExp() == FALSE ) return FALSE;
	rxps = HeapAlloc(DLLheap, 0, sizeof(PXREGEXPS));
	if ( rxps == NULL ) return FALSE;
	rxps->mode = RXMODE_BREGEXP_S;
	// 正規表現のコマンド & 左端セパレータをチェックする
	{
		TCHAR *ptr;

		ptr = rxstring;
		if ( Issymbol(*ptr) ){
			if ( (slash == FALSE) || (*ptr == '/') ){
				separater = *ptr++;
			}
		}else if ( (*ptr == 's') && Issymbol(*(ptr + 1)) ){
			if ( (slash == FALSE) || (*(ptr + 1) == '/') ){
				separater = *(ptr + 1);
				ptr += 2;
			}
		}else if ( (*ptr == 'y') && Issymbol(*(ptr + 1)) ){
			if ( (slash == FALSE) || (*(ptr + 1) == '/') ){
				separater = *(ptr + 1);
				ptr += 2;
				rxps->mode = RXMODE_BREGEXP_TR;
				preope = preope_trans;
			}
		}else if ( (*ptr == 'h') && Issymbol(*(ptr + 1)) ){
			if ( (slash == FALSE) || (*(ptr + 1) == '/') ){
				separater = *(ptr + 1);
				ptr += 2;
				rxps->mode = RXMODE_BREGEXP_H;
				rxps->string_rh = NULL;
				preope = preope_match;
			}
		}else if((*ptr == 't') && (*(ptr + 1) == 'r') && Issymbol(*(ptr + 2))){
			if ( (slash == FALSE) || (*(ptr + 2) == '/') ){
				separater = *(ptr + 2);
				ptr += 3;
				rxps->mode = RXMODE_BREGEXP_TR;
				preope = preope_trans;
			}
		}
		slist = ptr;
	}

	rlist = SearchSeparater(slist, separater); // 中央(右端)セパレータ
	if ( rlist == NULL ){
		if ( slash != SLASH_SEARCH ) goto noparamerror;
		rlist = slist + tstrlen(slist);
	}else{
		*rlist++ = '\0';
	}
	{
		TCHAR *rsep = SearchSeparater(rlist, separater); // 右端セパレータ
		if ( rsep != NULL ){
			*rsep = '\0';
			option = rsep + 1;
											// 余分なセパレータがないか確認
			if ( tstrchr(option, separater) != NULL ){
				XMessage(NULL, NULL, XM_GrERRld, T("RegExp: Bad separater"));
				goto error;
			}
		}
	}
	if ( rxps->mode != RXMODE_BREGEXP_S ){ // H or TR
		if ( RegExpType == EX_REGICU ){
			XMessage(NULL, NULL, XM_GrERRld, T("icu.dll: Unsupport 'tr', 'h'"));
			goto error;
		}

		if ( rxps->mode == RXMODE_BREGEXP_H ){
			int rsize;

			rsize = TSTRSIZE32(rlist);
			rxps->string_rh = HeapAlloc(DLLheap, 0, rsize);
			if ( rxps->string_rh == NULL ) goto error;
			memcpy(rxps->string_rh, rlist, rsize);
		}else{ // if ( rxps->mode == RXMODE_BREGEXP_TR )
			if ( (RegExpType == EX_IREGEXP) && (LoadRegexpDll() == FALSE) ){
				XMessage(NULL, NULL, XM_GrERRld, T("IRegExp: Unsupport 'tr'"));
				goto error;
			}
		}
	}

	if ( RegExpType < EX_IREGEXP ){ // EX_BREGEXP / EX_BREGONIG
		int ssize;

		if ( slash != SLASH_SEARCH ){
			ssize = (thprintf(buf, TSIZEOF(buf),
#ifdef UNICODE
					(RegExpType == EX_BREGONIG) ?
						T("%s%c%s%c%s%c%s") : T("%s%c%s%c%s%c%sk"),
#else
					T("%s%c%s%c%s%c%s"),
#endif
					preope, separater, slist, separater, rlist,
					separater, option) - buf + 1) * sizeof(TCHAR);
		}else{
			ssize = (thprintf(buf, TSIZEOF(buf),
#ifdef UNICODE
					(RegExpType == EX_BREGONIG) ? T("%c%s%c%s") : T("%c%s%c%sk"),
#else
					T("%c%s%c%s"),
#endif
					separater, slist,
					separater, option) - buf + 1) * sizeof(TCHAR);
		}
		rxps->string_rb = HeapAlloc(DLLheap, 0, ssize);
		if ( rxps->string_rb == NULL ) goto error;
		memcpy(rxps->string_rb, buf, ssize);
		rxps->bregp = NULL;
		rxps->bonip = NULL;
	}else if ( RegExpType == EX_IREGEXP ) {
		BSTR pattern;
		IRegExp *regexp;

		rxps->ComInitResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if ( FAILED(CoCreateInstance(&XCLSID_RegExp, NULL,
				CLSCTX_INPROC_SERVER, &XIID_IRegExp, (void **)&regexp)) ){
			if ( SUCCEEDED(rxps->ComInitResult) ) CoUninitialize();
			goto error;
		}
		while( *option ){
			switch (*option++){
				case 'i':
					IRegExp_put_IgnoreCase(regexp, VARIANT_TRUE);
					break;
				case 'g':
					IRegExp_put_Global(regexp, VARIANT_TRUE);
					break;
				default:
					XMessage(NULL, NULL, XM_GrERRld, T("IRegExp: Option error"));
					break;
			}
		}

		pattern = CreateBstring(slist);
		if ( FAILED(IRegExp_put_Pattern(regexp, pattern)) ){
			XMessage(NULL, NULL, XM_GrERRld, StrIREGEXPPATTENERROR);
			IRegExp_Release(regexp);
			DSysFreeString(pattern);
			if ( SUCCEEDED(rxps->ComInitResult) ) CoUninitialize();
			goto error;
		}
		DSysFreeString(pattern);
		rxps->mode += RXMODE_IREGEXP_MIN;
		rxps->string_ri = CreateBstring(rlist);
		rxps->string_result = NULL;
		rxps->iregexp = regexp;
	}else if ( RegExpType == EX_REGICU ) {
		BSTR wtext;
		UParseError pe = { 0 };
		UErrorCode ecode = 0;
		int flags = 0;

		rxps->global = 0;
		while( *option ){
			switch (*option++){
				case 'i':
					flags = UREGEX_CASE_INSENSITIVE;
					break;
				case 'g':
					rxps->global = 1;
					break;
				default:
					XMessage(NULL, NULL, XM_GrERRld, T("ICU: Option error"));
					break;
			}
		}

		wtext = CreateBstring(slist);
		rxps->regicu = uregex_open(wtext, -1, flags, &pe, &ecode);
		DSysFreeString(wtext);

#if ICU_USE_UTEXT
		wtext = CreateBstring(rlist);
		rxps->utext_ri = utext_openUChars(NULL, wtext, -1, &ecode);
		DSysFreeString(wtext);
#else
		rxps->itext_ri = CreateBstring(rlist);
#endif
		rxps->mode += RXMODE_ICU_MIN;

	}else{
		goto error;
	}
	*rxpsptr = rxps;
	return TRUE;

noparamerror:
	XMessage(NULL, NULL, XM_GrERRld,
			T("RegExp: Bad command or Not found src\'/\'replace") );
	// error へ
error:
	HeapFree(DLLheap, 0, rxps);
	return FALSE;
}

void FreeRegularExpression(PXREGEXPS *rxps)
{
	if ( rxps->mode <= RXMODE_BREGEXP_MAX ){
		if ( rxps->mode == RXMODE_BREGEXP_H ) HeapFree(DLLheap, 0, rxps->string_rh);
		if ( rxps->bregp != NULL ) BRegfree(rxps->bregp);
		if ( rxps->bonip != NULL ) DBRegfree(rxps->bonip);
		HeapFree(DLLheap, 0, rxps->string_rb);
	}else if ( rxps->mode <= RXMODE_IREGEXP_MAX ){
		if ( rxps->mode == RXMODE_IREGEXP_H ) HeapFree(DLLheap, 0, rxps->string_rh);
		IRegExp_Release(rxps->iregexp);
		DSysFreeString(rxps->string_ri);
		if ( rxps->string_result != NULL ) DSysFreeString(rxps->string_result);
		if ( SUCCEEDED(rxps->ComInitResult) ) CoUninitialize();
	}else{
		if ( rxps->mode == RXMODE_ICU_H ) HeapFree(DLLheap, 0, rxps->string_rh);
		uregex_close( rxps->regicu );
#if ICU_USE_UTEXT
		utext_close( rxps->utext_ri);
#else
		DSysFreeString(rxps->itext_ri);
#endif
	}
	HeapFree(DLLheap, 0, rxps);
}

const TCHAR *RegularExpressionReplace(PXREGEXPS *rxps, TCHAR *target, TCHAR *dest, size_t dest_buflen)
{
	if ( rxps->mode <= RXMODE_BREGEXP_MAX ){	// BRegExp(s/tr/h)
		TCHAR msg[BREGEXP_MAX_ERROR_MESSAGE_LEN];

		if ( rxps->mode == RXMODE_BREGEXP_H ){ // h
			return RegularExpressionMatch_bregexp(rxps, target, dest, dest_buflen);
		}
		msg[0] = '\0';
		if ( DBSubst != NULL ){ // bregonig
			int count; // 変換した文字数

			if ( rxps->mode != RXMODE_BREGEXP_TR ){
				count = DBSubst(rxps->string_rb, target,
						target + tstrlen(target), &rxps->bonip, msg);
			}else{
				count = DBTrans(rxps->string_rb, target,
						target + tstrlen(target), &rxps->bonip, msg);
			}
			if ( count <= 0 ){
				if ( count == 0 ){ // no match ... 元の文字列
					return RegExp_CopyReturn(dest, target, dest_buflen);
				}else{ // error
					*dest = '*';
					tstplimcpy(dest + 1, msg, dest_buflen);
					return dest;
				}
				// 空文字列
			}else if ( (rxps->bonip == NULL) || (rxps->bonip->outp == NULL) ){
				*dest = '\0';
				return dest;
				// 結果有り
			}else{
				return RegExp_CopyReturn(dest, rxps->bonip->outp, dest_buflen);
			}
		}else{ // bregexp
			int count; // 変換した文字数
#ifdef UNICODE
			char targetA[VFPS];
			char renameA[VFPS];

			UnicodeToAnsi(target, targetA, VFPS);
			targetA[VFPS - 1] = '\0';
			UnicodeToAnsi(rxps->string_rb, renameA, VFPS);
			renameA[VFPS - 1] = '\0';
#else
			#define targetA target
			#define renameA rxps->string_rb
#endif
			if ( rxps->mode != RXMODE_BREGEXP_TR ){
				count = BSubst(renameA, targetA, targetA + strlen(targetA),
						&rxps->bregp, (char *)msg);
			}else{
				count = BTrans(renameA, targetA, targetA + strlen(targetA),
						&rxps->bregp, (char *)msg);
			}
			if ( count <= 0 ){
				if ( count == 0 ){ // no match ... 元の文字列
					return RegExp_CopyReturn(dest, target, dest_buflen);
				}else{ // error
#ifdef UNICODE
					tstrcpy(dest, L"*bregexp error");
#else
					*dest = '*';
					stplimcpy(dest + 1, msg, dest_buflen);
#endif
					return dest;
				}
				// 空文字列
			}else if ( (rxps->bregp == NULL) || (rxps->bregp->outp == NULL) ){
				*dest = '\0';
				return dest;
				// 結果有り
			}else{
#ifdef UNICODE
				int len;
				len = AnsiToUnicode(rxps->bregp->outp, dest, (int)dest_buflen);
				if ( len == 0 ){
					*(dest + dest_buflen - 1) = '\0'; // len==0...長すぎて失敗
				}
				return dest;
#else
				return RegExp_CopyReturn(dest, rxps->bregp->outp, dest_buflen);
#endif
			}
		}
	}else if ( rxps->mode <= RXMODE_IREGEXP_MAX ){	// IRegExp(s/h)
		BSTR btarget, result = NULL;

		if ( rxps->mode == RXMODE_IREGEXP_H ){
			return RegularExpressionMatch_iregexp(rxps, target, dest, dest_buflen);
		}
		btarget = CreateBstring(target);
		if ( FAILED(IRegExp_Replace(rxps->iregexp,
				btarget, rxps->string_ri, &result)) ){
			XMessage(NULL, NULL, XM_GrERRld, T("IRegExp error"));
			DSysFreeString(btarget);
			return NULL;
		}
		DSysFreeString(btarget);
		if ( rxps->string_result != NULL ){ // 古い内容を破棄
			DSysFreeString(rxps->string_result);
		}
		rxps->string_result = result;
		return RegExp_CopyReturnT(dest, result, dest_buflen);
	}else {	// ICU(s)
#if ICU_USE_UTEXT
		UErrorCode ecode = 0;
		const TCHAR *res;
		UText *uresult;
		BSTR btarget;

		btarget = CreateBstring(target);
		uregex_setText(rxps->regicu, (UChar *)btarget, -1, &ecode);

		if ( rxps->global ){
			uresult = uregex_replaceAllUText(rxps->regicu, rxps->utext_ri, NULL, &ecode);
		}else{
			uresult = uregex_replaceFirstUText(rxps->regicu, rxps->utext_ri, NULL, &ecode);
		}
		DSysFreeString(btarget);

		if ( uresult != NULL ){
			WCHAR *wresult;

			INT64_T length64 = utext_nativeLength(uresult);
			wresult = (WCHAR *)HeapAlloc(DLLheap, 0, (SIZE_T)(length64 + 2) * sizeof(WCHAR));
			utext_extract(uresult, 0, length64, wresult, (INT32_T)length64, &ecode);
			wresult[length64] = '\0';
			utext_close(uresult);

			res = RegExp_CopyReturnT(dest, wresult, dest_buflen);
			HeapFree(DLLheap, 0, wresult);
			return res;
		}else{
			return NULL;
		}
#else
		UErrorCode ecode = 0;
		WCHAR wresult[CMDLINESIZE];
		BSTR btarget;
		int len;

		btarget = CreateBstring(target);
		uregex_setText(rxps->regicu, (UChar *)btarget, -1, &ecode);

		if ( rxps->global ){
			len = uregex_replaceAll(rxps->regicu, rxps->itext_ri, -1, wresult, CMDLINESIZE - 1, &ecode);
		}else{
			len = uregex_replaceFirst(rxps->regicu, rxps->itext_ri, -1, wresult, CMDLINESIZE - 1, &ecode);
		}

		wresult[len] = '\0';
		DSysFreeString(btarget);

		return RegExp_CopyReturnT(dest, wresult, dest_buflen);
#endif
	}
}

TCHAR *RegularExpressionSearch(PXREGEXPS *rxps, TCHAR *target, size_t target_len, size_t *hitlen)
{
	if ( rxps->mode <= RXMODE_BREGEXP_MAX ){	// BRegExp(s/tr/h)
		TCHAR msg[BREGEXP_MAX_ERROR_MESSAGE_LEN];

		if ( DBMatch != NULL ){ // bregonig
			int result;

			msg[0] = '\0';
			result = DBMatch(rxps->string_rb, target, target + target_len,
					&rxps->bonip, msg); // msg は未利用
			if ( (result > 0) && (rxps->bonip != NULL) ){
				if ( rxps->bonip->nparens < 0 ) return NULL;
				if ( (rxps->bonip->startp[0] < target) ||
					 (rxps->bonip->endp[0] > (target + target_len)) ){
					return NULL;
				}
				*hitlen = rxps->bonip->endp[0] - rxps->bonip->startp[0];
				return rxps->bonip->startp[0];
			}
		}else{ // bregexp
			int result;
#ifdef UNICODE
			#define REGBUFSIZE 0x2000
			char targetA[REGBUFSIZE];
			char renameA[REGBUFSIZE];
			size_t target_lenA;

			UnicodeToAnsi(target, targetA, REGBUFSIZE);
			targetA[REGBUFSIZE - 1] = '\0';
			target_lenA = strlen(targetA);
			UnicodeToAnsi(rxps->string_rb, renameA, REGBUFSIZE);
			renameA[REGBUFSIZE - 1] = '\0';
#else
			#define targetA target
			#define target_lenA target_len
			#define renameA rxps->string_rb
#endif
			msg[0] = '\0';
			result = BMatch(renameA, targetA, targetA + target_lenA,
					&rxps->bregp, (char *)msg); // msg は未利用
			if ( (result > 0) && (rxps->bregp != NULL) ){
				if ( rxps->bregp->nparens < 0 ) return NULL;
				if ( (rxps->bregp->startp[0] < targetA) ||
					 (rxps->bregp->endp[0] > (targetA + target_len)) ){
					return NULL;
				}
#ifdef UNICODE
				{
					size_t start_len;
					start_len = MultiByteToWideChar(CP_ACP, 0, targetA, rxps->bregp->startp[0] - targetA, NULL, 0);
					*hitlen = MultiByteToWideChar(CP_ACP, 0, rxps->bregp->startp[0], rxps->bregp->endp[0] - rxps->bregp->startp[0], NULL, 0);
					return target + start_len;
				}
#else
				*hitlen = rxps->bregp->endp[0] - rxps->bregp->startp[0];
				return rxps->bregp->startp[0];
#endif
			}
		}
		return NULL;
	}else if ( rxps->mode <= RXMODE_IREGEXP_MAX ){ // IRegExp(s/h)
		BSTR btarget;
		IDispatch *RegDispatch = NULL;
		IMatchCollection *Matchs;
		IMatch *match;
		TCHAR *result = NULL;

		btarget = CreateBstring(target);
		if ( FAILED(IRegExp_Execute(rxps->iregexp, btarget, &RegDispatch)) ){
			XMessage(NULL, NULL, XM_GrERRld, T("IRegExp error"));
			DSysFreeString(btarget);
			return NULL;
		}

		if ( SUCCEEDED(RegDispatch->lpVtbl->QueryInterface(RegDispatch, &XIID_IMatchCollection, (void **)&Matchs)) ){
			IDispatch *dispatch;
			// 始めのグループを取得
			if ( SUCCEEDED(Matchs->lpVtbl->get_Item(Matchs, 0, &dispatch)) ){
				// その該当位置と長さを取得
				if ( SUCCEEDED(dispatch->lpVtbl->QueryInterface(dispatch, &XIID_IMatch, (void**)&match)) ){
					long index = 0, length = 0;

					match->lpVtbl->get_FirstIndex(match, &index);
					match->lpVtbl->get_Length(match, &length);
					match->lpVtbl->Release(match);
#ifndef UNICODE
					result = target + WideCharToMultiByte(CP_ACP, 0, btarget, index, NULL, 0, NULL, NULL);
					*hitlen = WideCharToMultiByte(CP_ACP, 0, btarget + index, length, NULL, 0, NULL, NULL);
#else
					result = target + index;
					*hitlen = length;
#endif
				}
				dispatch->lpVtbl->Release(dispatch);
			}
			Matchs->lpVtbl->Release(Matchs);
		}
		RegDispatch->lpVtbl->Release(RegDispatch);
		DSysFreeString(btarget);
		return result;
	}else{
		return NULL;
	}
}

void GetParamFlags(GETPARAMFLAGSSTRUCT *gpfs, const TCHAR **param, const TCHAR *flagnames)
{
	DWORD flags;
	const TCHAR *lp, *nowp;

	gpfs->mask = 0;
	gpfs->value = 0;
	nowp = *param;
	while( *nowp != '\0' ){
		TCHAR c;

		c = TinyCharLower(SkipSpace(&nowp));
		for ( flags = LSBIT, lp = flagnames ; *lp ; flags <<= 1, lp++){
			if ( c != *lp ) continue;
			nowp++;
			setflag(gpfs->mask, flags);
			switch( *nowp ){
				case '-':
					nowp++;
					break;
				case '+':
					nowp++;
				// default へ
				default:
					setflag(gpfs->value, flags);
			}
			break;
		}
		if ( *lp == '\0' ) break;
	}
	*param = nowp;
	return;
}

int CheckExtMode(EXSMEM *exm, MAKEFNSTRUCT *mfs)
{
	switch ( TinyCharLower(mfs->name[0]) ){
		case 'a': {	// Attribute: 属性指定
			GETPARAMFLAGSSTRUCT gpfs;

			setflag(mfs->flags, REGEXPF_REQ_ATTR);
			if ( CheckExsmem(exm, sizeof(EXS_ATTR)) == FALSE ) return CEM_ERROR;
			GetParamFlags(&gpfs, (const TCHAR **)&mfs->param, AttrLabelString);
			if ( gpfs.mask & FILE_ATTRIBUTE_DIRECTORY ){
				setflag(mfs->flags, REGEXPF_PATHMASK);
			}
			((EXS_ATTR *)exm->dest)->ext = EX_ATTR;
			((EXS_ATTR *)exm->dest)->next = sizeof(EXS_ATTR);
			((EXS_ATTR *)exm->dest)->mask = gpfs.mask;
			((EXS_ATTR *)exm->dest)->value = gpfs.value;
			break;
		}

		case 'p': {	// Path: ディレクトリ指定
			setflag(mfs->flags, REGEXPF_PATHMASK | REGEXPF_REQ_ATTR);
										// ワイルドカード指定がない
			if ( SkipSpace((const TCHAR **)&mfs->param) == '\0' ){
				((EXS_DATA *)exm->dest)->ext = EX_NONE;
				((EXS_OPTION *)exm->dest)->next = 0;
				break;
			}

			if ( CheckExsmem(exm, sizeof(EXS_DIR)) == FALSE ) return CEM_ERROR;
			((EXS_DIR *)exm->dest)->ext = EX_PATH;
			((EXS_DIR *)exm->dest)->next = sizeof(EXS_DIR);
			exm->left -= sizeof(EXS_DIR);
			exm->dest += sizeof(EXS_DIR);
			if ( *mfs->param != '/' ){
				memmove(mfs->name, mfs->param, TSTRSIZE(mfs->param));
				return CEM_ANALYZE; //解析を続行させて名前/正規表現の処理を行う
			}
			// 正規表現
			if ( RegExpWildCard(exm, (const TCHAR **)&mfs->param) == FALSE ){
				return CEM_ERROR; // error;
			}
			break;
		}

		case 'o': // Option: オプション指定
		case 'i': {	// 旧オプション指定
			if ( SkipSpace((const TCHAR **)&mfs->param) != '\0' ){
				GETPARAMFLAGSSTRUCT gpfs;

				GetParamFlags(&gpfs, (const TCHAR **)&mfs->param, inclabel);
				if ( gpfs.value & REGEXPF_PATHMASK ){
					setflag(gpfs.value, REGEXPF_REQ_ATTR);
				}
				mfs->flags = (mfs->flags & ~(REGEXPF_PATHMASK | REGEXPF_NOEXT | REGEXPF_SILENTERROR | REGEXPF_WORDMATCH)) | gpfs.value;

				if ( gpfs.value & REGEXPF_STRING ){
					if ( CheckExsmem(exm, sizeof(EXS_OPTION)) == FALSE ){
						return CEM_ERROR;
					}
					((EXS_OPTION *)exm->dest)->ext = EX_STRING;
					((EXS_OPTION *)exm->dest)->next = sizeof(EXS_OPTION);
					exm->dest += sizeof(EXS_OPTION);
					exm->left -= sizeof(EXS_OPTION);
				}

				// REGEXPF_NOEXT フラグがなければ、保存不要
				if ( !(gpfs.value & REGEXPF_NOEXT) ){
					((EXS_OPTION *)exm->dest)->ext = EX_NONE;
					((EXS_OPTION *)exm->dest)->next = 0;
					break;
				}
			}	// 拡張子非分離指定(REGEXPF_NOEXT)
			if ( CheckExsmem(exm, sizeof(EXS_OPTION)) == FALSE ){
				return CEM_ERROR;
			}
			((EXS_OPTION *)exm->dest)->ext = EX_JOINEXT;
			((EXS_OPTION *)exm->dest)->next = sizeof(EXS_OPTION);
			mfs->useext = FALSE;
			break;
		}

		case 'd': { // Date: 日付指定
			DWORD type, type2;
			FILETIME rel_time; // DAY_RELATIVE
			SYSTEMTIME abs_time; // DAY_ABSOLUTELY
			int daytype;
			DWORD offset = 0;
			TCHAR typechar;

			setflag(mfs->flags, REGEXPF_REQ_TIME);
			typechar = TinyCharLower(SkipSpace((const TCHAR **)&mfs->param));
			if ( typechar == 'c' ){
				mfs->param++;
				offset = EX_CREATEDATEREL - EX_WRITEDATEREL;
			}else if ( typechar == 'a' ){
				mfs->param++;
				offset = EX_ACCESSDATEREL - EX_WRITEDATEREL;
			}
			type = GetCompareType((const TCHAR **)&mfs->param);
			daytype = GetWildDate((const TCHAR **)&mfs->param, &rel_time, &abs_time);
//			if ( daytype == DAY_NONE ) return CEM_ERROR;
			type2 = GetCompareType((const TCHAR **)&mfs->param);
			if ( type2 != 0 ){ // type2 は符号反転を行う
				type = TYPE_INVERT(type2);
			}
			if ( daytype == DAY_ABSOLUTELY ){
				if ( CheckExsmem(exm, sizeof(EXS_DATE_ABS)) == FALSE ){
					return CEM_ERROR;
				}
				((EXS_DATE_ABS *)exm->dest)->ext = (WORD)(offset + EX_WRITEDATEABS);
				((EXS_DATE_ABS *)exm->dest)->next = sizeof(EXS_DATE_ABS);
				((EXS_DATE_ABS *)exm->dest)->date = abs_time;

				if ( type == 0 ) type = TYPE_EQUAL; // 指定無し…当日
				((EXS_DATE_ABS *)exm->dest)->type = TYPE_INVERT(type);
			}else{ // DAY_RELATIVE
				if ( CheckExsmem(exm, sizeof(EXS_DATE_REL)) == FALSE){
					return CEM_ERROR;
				}
				((EXS_DATE_REL *)exm->dest)->ext = (WORD)(offset + EX_WRITEDATEREL);
				((EXS_DATE_REL *)exm->dest)->next = sizeof(EXS_DATE_REL);
				((EXS_DATE_REL *)exm->dest)->days = rel_time;
				if ( type == 0 ) type = TYPE_SMALL; // 指定無し…日数以内
				((EXS_DATE_REL *)exm->dest)->type = type;
			}
			break;
		}

		case 's': { // Size: ファイルサイズ指定
			DWORD type, type2;

			setflag(mfs->flags, REGEXPF_REQ_SIZE);
			if ( CheckExsmem(exm, sizeof(EXS_SIZE)) == FALSE ) return CEM_ERROR;

			type = GetCompareType((const TCHAR **)&mfs->param);

			if ( FALSE == GetSizeNumberHL((const TCHAR **)&mfs->param,
					&((EXS_SIZE *)exm->dest)->size) ){
				return CEM_ERROR;
			}

			type2 = GetCompareType((const TCHAR **)&mfs->param);
			if ( type2 != 0 ){ // type2 は符号反転を行う
				type = TYPE_INVERT(type2);
			}
			if ( type == 0 ) type = TYPE_SMALL | TYPE_EQUAL;

			((EXS_SIZE *)exm->dest)->ext = EX_SIZE;
			((EXS_SIZE *)exm->dest)->next = sizeof(EXS_SIZE);
			((EXS_SIZE *)exm->dest)->type = type;
			break;
		}

		case 'r': { // Roma: ローマ字指定
			size_t size, structsize;

			if ( InitMigemo(0) == FALSE ) return CEM_ERROR;

			size = tstrlen(mfs->param);
			structsize = sizeof(EXS_ROMA) + size * sizeof(TCHAR) + sizeof(TCHAR);
			if ( CheckExsmem(exm, (DWORD)structsize) == FALSE) return CEM_ERROR;

			((EXS_ROMA *)(exm->dest))->ext = EX_ROMA;
			((EXS_ROMA *)(exm->dest))->next = (WORD)structsize;
			((EXS_ROMA *)(exm->dest))->handle = 0;
			memcpy(((EXS_ROMA *)(exm->dest))->data, mfs->param, size * sizeof(TCHAR));
			((EXS_ROMA *)(exm->dest))->data[size] = '\0';
			break;
		}

		default:
			if ( !(mfs->flags & REGEXPF_SILENTERROR) ){
				XMessage(NULL, NULL, XM_FaERRd, MES_EBWA);
			}
			return CEM_ERROR;
	}
	return CEM_COMP; // 処理成功 & 解釈終了
}

TCHAR *GetRegularExpressionName(TCHAR *str)
{
	const TCHAR *name;

	LoadRegExp();
	switch ( RegExpType ){
		case EX_BREGEXP:
			name = T("BREGEXP");
			break;

		case EX_BREGONIG:
			name = T("bregonig");
			break;

		case EX_IREGEXP:
			name = T("IRegExp");
			break;

		case EX_REGICU:
			name = T("ICU");
			break;

		default:
			name = T("none");
			break;
	}
	return tstpcpy(str, name);
}
