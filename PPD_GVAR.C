/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library						Global Value
-----------------------------------------------------------------------------*/
#ifdef GLOBALEXTERN
	#define GVAR extern
	#define GPARAM(x)
	#define GPARAM2(x, y)
#else
	#undef GVAR
	#undef GPARAM
	#undef GPARAM2
	#define GVAR
	#define GPARAM(x) = x
	#define GPARAM2(x, y) = {x, y}
#endif

GVAR BOOL PPxProcess GPARAM(FALSE);
GVAR int X_pmc[4] GPARAM({X_pmc_defvalue});
GVAR int X_tous[X_tousItems] GPARAM({X_tous_defvalueC});

GVAR int TouchMode GPARAM(0);
GVAR int ConsoleMode GPARAM(ConsoleMode_GUIOnly);

GVAR const TCHAR NilStr[1] GPARAM(T(""));
GVAR TCHAR NilStrNC[1] GPARAM(T(""));
GVAR const TCHAR PPXJOBMUTEX[] GPARAM(T("PPxJOB"));	// 実行シリアライズ用Mutex
GVAR const TCHAR StrListFileID[] GPARAM(T("::") VFS_TYPEID_LISTFILE);
GVAR const TCHAR StrPath[] GPARAM(T("PATH"));
GVAR const TCHAR StrCustOthers[] GPARAM(T("_others"));
GVAR const TCHAR StrCustSetup[] GPARAM(T("_Setup"));
GVAR const TCHAR PPxRegPath[] GPARAM(T(PPxSettingsRegPath));
GVAR const TCHAR IdlCacheName[] GPARAM(T("#IdlC"));
GVAR const TCHAR StrShortcutExt[] GPARAM(T(ShortcutExt));
GVAR const TCHAR StrX_dlf[] GPARAM(T("X_dlf"));
GVAR const TCHAR StrX_tree[] GPARAM(T("X_tree"));
GVAR const TCHAR StrK_lied[] GPARAM(T("K_lied"));
GVAR const TCHAR StrK_edit[] GPARAM(T("K_edit"));
GVAR const TCHAR StrFIRSTEVENT[] GPARAM(T("FIRSTEVENT"));
GVAR const TCHAR StrCLOSEEVENT[] GPARAM(T("CLOSEEVENT"));

GVAR const TCHAR PathJumpNameEx[] GPARAM(T("<%j>M_pjump"));
#define PathJumpName (PathJumpNameEx + 4)
#define PathJumpNameSize ((12 - 4) * sizeof(TCHAR))
GVAR const TCHAR StrMDrives[]	GPARAM(T("M_drives"));

GVAR const TCHAR ButtonClassName[]  GPARAM(WC_BUTTON);
GVAR const TCHAR EditClassName[]    GPARAM(WC_EDIT);
GVAR const TCHAR StaticClassName[]  GPARAM(WC_STATIC);
GVAR const TCHAR ListBoxClassName[] GPARAM(WC_LISTBOX);
GVAR const TCHAR ScrollBarClassName[] GPARAM(WC_SCROLLBAR);
GVAR const TCHAR ComboBoxClassName[] GPARAM(WC_COMBOBOX);
GVAR const TCHAR FullPathMacroStr[] GPARAM(T("NDC"));

GVAR const TCHAR TreeClassStr[] GPARAM(T(VFSTREECLASS));

GVAR const TCHAR STR_WAITOPERATION[] GPARAM(MES_MWOP);

GVAR const char StrUDF[]		GPARAM(UDFHEADER);
GVAR const char StrISO9660[]	GPARAM(ISO9660HEADER);

#ifdef UNICODE
	#define XNAME		T("PPW")
	#define HISTNAME	T("PPWH")
	#define CUSTNAME_	T("PPWC")
	#define HISTMEMNAME	T("PPWHIST.MEM")
	#define CUSTMEMNAME	T("PPWCUST.MEM")
#else
	#define XNAME		T("PPX")
	#define HISTNAME	T("PPXH")
	#define CUSTNAME_	T("PPXC")
	#define HISTMEMNAME	T("PPHIST.MEM")
	#define CUSTMEMNAME	T("PPCUST.MEM")
#endif
GVAR const TCHAR CUSTNAME[] GPARAM(CUSTNAME_);

GVAR const TCHAR *JobTypeNames[JOBSTATE_MAX] // JOBSTATE_xxx のテキスト
#ifndef GLOBALEXTERN
= {
	MES_UNKN,  MES_JMAT, MES_JMCO, MES_JMCS, MES_JMRE,
	T("Thread"),
	MES_JMAP, MES_JMAE,
	MES_JMMO, MES_JMCP, MES_JMMR, MES_JMSH,
	MES_JMLI, MES_JMDE, MES_JMUN, MES_JMSL,
	MES_JMJO, MES_COMT
}
#endif
;
/*
GVAR const TCHAR MenuCacheItem_ValueName[]	GPARAM(T(StringVariable_Command_MenuCacheItem));
GVAR const TCHAR MenuCacheText_ValueName[]	GPARAM(T(StringVariable_Command_MenuCacheText));
*/

GVAR const TCHAR PPxName[] GPARAM(T("PPx"));
GVAR const TCHAR PPcExeName[] GPARAM(T(PPCEXE));
GVAR const TCHAR PPvExeName[] GPARAM(T(PPVEXE));
GVAR const TCHAR XEO_OptionString[] GPARAM(T(XEO_STRINGS));
GVAR const TCHAR WildCard_All[] GPARAM(T("*"));
GVAR const TCHAR ShellVerb_open[] GPARAM(T("open"));
GVAR const TCHAR StrFtp[] GPARAM(T("ftp://"));
#define StrFtpSize TSTROFF(6)
GVAR const TCHAR StrAux[] GPARAM(T("aux:"));
#define StrAuxSize TSTROFF(4)

GVAR const TCHAR StrFopActionCopy[] GPARAM(T("Copy"));

#define HIST_TYPE_COUNT 15
GVAR const TCHAR HistTypes[HIST_TYPE_COUNT + 1] GPARAM(T("gnhdcfsmpveuUxX"));	// ヒストリ種類を表わす文字 ※ R, O は別の用途で使用中

GVAR const WORD HistReadTypeflag[HIST_TYPE_COUNT]
#ifndef GLOBALEXTERN
 = {
	PPXH_GENERAL_R,	PPXH_NUMBER_R,	PPXH_COMMAND_R,	PPXH_DIR_R,	// gnhd
	PPXH_FILENAME_R,PPXH_PATH_R,	PPXH_SEARCH_R,	PPXH_MASK_R, // cfsm
	PPXH_PPCPATH_R,	PPXH_PPVNAME_R,	0,							// pve
	PPXH_USER1,		PPXH_USER1,		PPXH_USER2,		PPXH_USER2	// uUxX
}
#endif
;
GVAR const WORD HistWriteTypeflag[HIST_TYPE_COUNT]
#ifndef GLOBALEXTERN
 = {
	PPXH_GENERAL,	PPXH_NUMBER,	PPXH_COMMAND,	PPXH_DIR,	// gnhd
	PPXH_FILENAME,	PPXH_PATH,		PPXH_SEARCH,	PPXH_MASK,	// cfsm
	PPXH_PPCPATH,	PPXH_PPVNAME,	0,							// pve
	PPXH_USER1,		PPXH_USER1,		PPXH_USER2,		PPXH_USER2	// uUxX
}
#endif
;
GVAR const DWORD TinputTypeflags[HIST_TYPE_COUNT]
#ifndef GLOBALEXTERN
 = {
	0,	TIEX_SINGLEREF,	0,	TIEX_REFTREE | TIEX_SINGLEREF,		// gnhd
	TIEX_SINGLEREF,	TIEX_SINGLEREF,	TIEX_SINGLEREF,	0,			// cfsm
	TIEX_SINGLEREF,	TIEX_SINGLEREF,	0,							// pve
	TIEX_SINGLEREF,	0,		TIEX_REFTREE | TIEX_SINGLEREF, 0		// uUxX
}
#endif
;
GVAR const TCHAR StrPackZipFolderTitle[] GPARAM(T("zip - zipfldr"));
GVAR const TCHAR StrPackZipFolderCommand[] GPARAM(T("%uzipfldr.dll,A \"%2\""));
GVAR const TCHAR EditCache_ValueName[] GPARAM(T(StringVariable_Command_EditCache));

GVAR const TCHAR StrOptionError[] GPARAM(T("Option error: %s"));

GVAR const TCHAR StrUserCommand[] GPARAM(T("_Command"));

GVAR const char User32DLL[] GPARAM("USER32.DLL");

GVAR DLLWndClassUnion DLLWndClass
#ifndef GLOBALEXTERN
 = { {0, 0, 0, 0, 0, 0, 0, 0, 0} }
#endif
;

//----------------------------------------------------- PPLIBxxx.DLL 固有の情報
GVAR HINSTANCE DLLhInst;		// DLL のインスタンス
GVAR TCHAR DLLpath[MAX_PATH];	// DLL の所在地

GVAR HANDLE ProcHeap;			// DLL の GetProcessHeap 値
#define DLLheap ProcHeap		// DLL内のHeap(現在はGetProcessHeap)
GVAR UINT  WM_PPXCOMMAND;		// WM_PPXCOMMAND の登録値
GVAR TCHAR ProcTempPath[MAX_PATH];	// このプロセス専用のテンポラリパス
GVAR TCHAR SyncTag[12];			// 同期オブジェクト名に使用する "PPXxxxxx"-"PPXxxxxxxxx"
GVAR int RunAsMode GPARAM(RUNAS_NOCHECK); // RunAs で起動したかどうかの判定結果

GVAR HWND hTaskProgressWnd GPARAM(NULL); // プログレス表示用ウィンドウ

//-------------------------------------------------------------------- 共有資源
GVAR ShareM *Sm GPARAM(NULL);		// 共有メモリへのポインタ
GVAR BYTE *HisP;					// ヒストリ
GVAR BYTE *CustP;					// カスタマイズ領域
GVAR DWORD X_Hsize GPARAM(DEF_HIST_SIZE);	// History size
GVAR DWORD X_Csize GPARAM(DEF_CUST_SIZE);	// Customize size

#ifdef TMEM_H
GVAR FM_H SM_cust;					// カスタマイズ領域のファイルマップ
#endif
GVAR ThSTRUCT ProcessStringValue GPARAM(ThSTRUCT_InitData);	// プロセス内限定特殊環境変数
GVAR CRITICAL_SECTION ThreadSection; // 汎用スレッドセーフ用

//---------------------------------------------------------------------- OS情報
GVAR OSVERSIONINFO OSver;	// OS 情報
#define WINTYPE_9x		0

#ifdef UNICODE
	#define If_WinNT_Block
#else
	#define If_WinNT_Block if (OSver.dwPlatformId == VER_PLATFORM_WIN32_NT)
#endif

GVAR DWORD WinType GPARAM(WINTYPE_9x);	// Windows の Version 情報

#ifndef UNLEN
#define UNLEN 256
#endif
GVAR TCHAR UserName[UNLEN + 1];	// ログオンユーザ名

GVAR TCHAR NumberUnitSeparator GPARAM(','); // 桁区切り記号
GVAR TCHAR NumberDecimalSeparator GPARAM('.'); // 小数点記号

//-------------------------------------------------------------------- 例外情報
GVAR DWORD_PTR UEFvalue GPARAM(0); // 例外発生時の追加情報
GVAR DWORD StartTick; // PPx 開始時のTick
GVAR BOOL NowExit GPARAM(FALSE); // K_CLEANUP を実行したか
// GVAR void **FaultOptionInfo GPARAM(NULL); // 異常終了時の追加情報ポインタへのポインタ
GVAR PVOID UEFvec; // 例外処理ハンドラ情報

GVAR CRMENUSTACKCHECK CrmenuCheck // スタック異常等の検出用
#ifndef GLOBALEXTERN
= {0, 0}
#endif
;
GVAR TCHAR AuthHostCache[0x100];
GVAR TCHAR AuthUserCache[100];
GVAR TCHAR AuthPassCache[100];

//---- info API
GVAR PPXAPPINFO DummyPPxAppInfo
#ifndef GLOBALEXTERN
= {(PPXAPPINFOFUNCTION)DummyPPxInfoFunc, PPxName, NilStr, NULL}
#endif
;

GVAR PPXAPPINFO PPxDefInfo
#ifndef GLOBALEXTERN
= {(PPXAPPINFOFUNCTION)PPxDefInfoFunc, PPxName, NilStr, NULL}
#endif
;


GVAR PPXAPPINFO *PPxDefAppInfo GPARAM(NULL);

GVAR int UseCompListModule GPARAM(-1); // -1 未確認、0 使用しない、1 使用する


//---------------------------------------------------------------------- DLL
GVAR HMODULE hBregexpDLL GPARAM(NULL);
GVAR HMODULE hBregonigDLL GPARAM(NULL);
GVAR HMODULE hIcuDLL GPARAM(NULL);
GVAR HMODULE hMigemoDLL GPARAM(NULL);

GVAR HMODULE hComctl32 GPARAM(NULL);	// COMCTL32.DLL のハンドル
GVAR HMODULE hShell32 GPARAM(NULL);
GVAR HMODULE hKernel32 GPARAM(NULL);
GVAR HMODULE hUxtheme GPARAM(NULL);
GVAR ValueWinAPI(OpenThread) GPARAM(INVALID_VALUE(impOpenThread));
#ifdef _WIN64
#define DAddVectoredExceptionHandler AddVectoredExceptionHandler
#define DRemoveVectoredExceptionHandler RemoveVectoredExceptionHandler
#else
GVAR ValueWinAPI(AddVectoredExceptionHandler) GPARAM(NULL);
GVAR ValueWinAPI(RemoveVectoredExceptionHandler) GPARAM(NULL);
#endif

//------------------------------------------------------------------- Customize
GVAR HWND hUpdateResultWnd GPARAM(NULL); // カスタマイズログ(終了時に閉じる為)
GVAR int X_es	GPARAM(0x1d);	// 拡張シフトのコード (VK_NOCONVERT)
GVAR int X_mwid	GPARAM(60);		// メニューの最大桁数(要PPx再起動)
GVAR int X_dss	GPARAM(DSS_NOLOAD);	// 画面自動スケーリング
GVAR DWORD X_flst[5]
#ifndef GLOBALEXTERN
	= { 0, 1, 0, 14, 0 };
#endif
;
#define X_flst_mode X_flst[0]
	#define X_fmode_off 0		// 一覧表示をしない
	#define X_fmode_manual 1	// [INS], ^[I], [TAB] で一覧表示をする
	#define X_fmode_api 2		// API 使用
	#define X_fmode_auto1 3		// 1枚表示
	#define X_fmode_auto2 4		// 2枚表示
	#define X_fmode_auto2ex 5	// 2枚表示+
#define X_flst_part X_flst[1]	// 部分一致検索するか(X_ltab[1] == 0 のとき)
#define X_flst_width X_flst[2]	// 一覧表示幅
#define X_flst_height X_flst[3]	// 一覧表示高
#define X_flst_uppos X_flst[4]	// 一覧表示が上側の時、キャプション部分をまたぐかどうか

#define NOMESSAGETEXT (BYTE *)BADPTR	// 読み込みできない場合の値
GVAR BYTE *MessageTextTable GPARAM(NULL);	// 表示メッセージ用テーブル
GVAR DWORD X_jinfo[4]
#ifndef GLOBALEXTERN
= {MAX32, 1, 1, 1}
#endif
;
//--------
GVAR DWORD X_beep GPARAM(0xfff);	/* Beep の出力フラグ 1010 1111
									 B0:致命的エラー表示
									 B1:一般エラー表示
									 B2:重要でないエラー表示
									 B3:重要な警告
									 B4:重要でない警告
									 B5:結果を表示する情報
									 B6:好意で出力する情報/作業完了
									 B7:その他
								*/
GVAR DWORD X_log GPARAM(0);	// 各種ログを出力
GVAR int X_execs GPARAM(-1);
GVAR DWORD X_Keyra GPARAM(1);
GVAR int X_jlst[2] GPARAM2(-1, 1);
GVAR int X_prtg GPARAM(-1);
GVAR int X_uxt[X_uxt_items] GPARAM({X_uxt_defvalue});
GVAR int X_csyh GPARAM(-1); // コマンドラインシンタックスハイライト

GVAR HWND hTipWnd GPARAM(NULL);
GVAR HWND hProcessComboWnd GPARAM(NULL);

GVAR DWORD CustTick GPARAM(0); // K_Lcust を呼び出したときの時間(重複再カスタマイズ回避)

GVAR int X_combosD[2] GPARAM2(-1, -1);
#define CMBS1_DIALOGNOPANE	B4	// ダイアログを一体化窓の中心に

GVAR int X_nshf GPARAM(1);

//-------- 色
#define EDF_NOT_INIT B10
GVAR THEME_COLORS ThemeColors
#ifndef GLOBALEXTERN
 = { EDF_NOT_INIT, {C_AUTO, C_AUTO, C_AUTO,  C_AUTO, C_AUTO,  C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO} }
#endif
;
GVAR const C_WIN_COLORS C_Win_Dark
#ifndef GLOBALEXTERN
 = { COLOR_DARK_FRAME_HIGH, COLOR_DARK_FRAME_FACE, COLOR_DARK_FRAME_SHADOW, //枠
	 COLOR_DARK_BACK, COLOR_DARK_DIALOGBACK, // 背景
	 COLOR_DARK_TEXT, // 文字
	 C_AUTO, C_AUTO, // 選択時
	 COLOR_DARK_GRAYTEXT, // 灰色
	 COLOR_DARK_TEXT, // 文字
	 COLOR_DARK_EDGE_LINE,
	 COLOR_DARK_FOCUSBACK}
#endif
;

GVAR const C_WIN_COLORS C_Win_Light
#ifndef GLOBALEXTERN
 = { COLOR_LIGHT_FRAME_HIGH, COLOR_LIGHT_FRAME_FACE, COLOR_LIGHT_FRAME_SHADOW, //枠
	 COLOR_LIGHT_BACK, COLOR_LIGHT_DIALOGBACK, // 背景
	 COLOR_LIGHT_TEXT, // 文字
	 C_AUTO, C_AUTO, // 選択時
	 COLOR_LIGHT_GRAYTEXT, // 灰色
	 COLOR_LIGHT_TEXT, // 文字
	 COLOR_LIGHT_EDGE_LINE,
	 COLOR_LIGHT_FOCUSBACK}
#endif
;

#define C_FrameHighlight (ThemeColors.c.FrameHighlight) // オブジェクト明
#define C_FrameFace (ThemeColors.c.FrameFace) // オブジェクト表面（本来のダイアログ表面）
#define C_FrameShadow (ThemeColors.c.FrameShadow) // オブジェクト影
#define C_EdgeLine (ThemeColors.c.EdgeLine) // 境界線

#define C_WindowBack (ThemeColors.c.WindowBack) // ウィンドウ窓内(Edit,ListBox)
#define C_DialogBack (ThemeColors.c.DialogBack) // ダイアログ表面(元は3dFace)
#define C_FocusBack (ThemeColors.c.FocusBack) // フォーカス時の背景(TAB用)

#define C_WindowText (ThemeColors.c.WindowText) // ウィンドウ文字
#define C_DialogText (ThemeColors.c.DialogText) // ダイアログ文字

#define C_HighlightText (ThemeColors.c.HighlightText) // 選択文字
#define C_HighlightBack (ThemeColors.c.HighlightBack) // 選択背景

#define C_GrayText (ThemeColors.c.GrayText) // 灰色文字

GVAR COLORREF C_SchemeColor1[C_Scheme1_TOTAL]
#ifndef GLOBALEXTERN
 = {	C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO,
		C_AUTO, C_AUTO, C_AUTO}
#endif
;

#define C_SchemeColor2_def0 0x603C00
GVAR COLORREF C_SchemeColor2[C_Scheme2_TOTAL]
#ifndef GLOBALEXTERN
 = {	C_AUTO, 0x2F32DC, 0x009985, 0xD28B26,
	0x0089B5, 0x98A12A, 0x8236D3, 0xD5E8EE,
	0x735B00, 0x5E61E3, 0x00C4AB, 0xEAC18A,
	0x00AFEA, 0xC9D251, 0xA26ADF, 0xEEF7F9,
	0x423607, 0x171B8A, 0x006054, 0x855718,
	0x006080, 0x6F751E, 0x4B1C80, 0xA5A5A5,
	0x311F00, 0xE0E0E0, 0x6F751E, 0xE0E0E0,
	0x2F32DC, 0x00C4AB, 0x98A12A, 0xFFAC00 }
#endif
;

GVAR HBRUSH hFrameHighlight GPARAM(NULL); // 左・上
GVAR HBRUSH hFrameFace GPARAM(NULL); // 中央
GVAR HBRUSH hFrameShadow GPARAM(NULL); // 右
GVAR HBRUSH hHighlightBack GPARAM(NULL);
GVAR HBRUSH hEdgeLine GPARAM(NULL);

GVAR HBRUSH hDialogBackBrush GPARAM(NULL);
GVAR HBRUSH hWindowBackBrush GPARAM(NULL);

GVAR volatile HBITMAP hButtonBMP GPARAM(NULL);

//--------------------------------------------------------------------- COM定義
// RegExp
extern IID XCLSID_RegExp;
//extern IID XCLSID_Match;
//extern IID XCLSID_MatchCollection;
extern IID XIID_IRegExp;
extern IID XIID_IMatch;
extern IID XIID_IMatch2;
extern IID XIID_IMatchCollection;
extern IID XIID_ISubMatches;

// Shell Extension
extern IID XIID_IContextMenu3;
extern IID XIID_IColumnProvider;
extern IID XIID_IPropertySetStorage;

// Etc
#ifdef WINEGCC
extern IID XIID_IClassFactory;
extern IID XIID_IUnknown;
extern IID XIID_IDataObject;
extern IID XIID_IDropSource;

extern IID XIID_IStorage;
extern IID XIID_IPersistFile;

// Shell Extension
extern IID XCLSID_ShellLink;
extern IID XIID_IContextMenu;
extern IID XIID_IShellFolder;
extern IID XIID_IShellLink;
extern IID XIID_IContextMenu2;
#else
#define XCLSID_ShellLink CLSID_ShellLink
#define XIID_IClassFactory IID_IClassFactory
#define XIID_IContextMenu IID_IContextMenu
#define XIID_IContextMenu2 IID_IContextMenu2
#define XIID_IDataObject IID_IDataObject
#define XIID_IDropSource IID_IDropSource
#define XIID_IPersistFile IID_IPersistFile
#define XIID_IShellFolder IID_IShellFolder
#define XIID_IShellLink IID_IShellLink
#define XIID_IStorage IID_IStorage
#define XIID_IUnknown IID_IUnknown
#define XIID_IPicture IID_IPicture
#endif
