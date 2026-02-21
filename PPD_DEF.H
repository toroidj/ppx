/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library		内部定義
-----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

// 共有メモリ等のバージョン。※実際に使うときは ShareM_VersionValue
#define ShareM_Version 72

#define UseTMessageBox 1 // MessageBox は 0:システムのを使う 1:自前
#define UsePPxType 1 // リソース調停に 0:Mutex を使う 1:自前

#define X_Mtask		32			// PPx の管理可能な動作数
#define X_MaxJob	32			// PPx の管理可能なJob動作数
#define X_MaxComboID	26		// ComboID の数(A-Z)
#define X_MaxPPxID	26			// PPxID の数(A-Z)

#ifndef UNICODE
	#define HistoryID	"TH1"	// ヒストリファイルの ID
	#define CustID		"TC1"	// カスタマイズファイルの ID
	#define DEF_CUST_SIZE 0x1c000 // def無し:76292, setup:89161 (1.93)
#else
	#define HistoryID	"TH2"	// ヒストリファイルの ID
	#define CustID		"TC2"	// カスタマイズファイルの ID
	#define DEF_CUST_SIZE 0x14000 // def無し:52382, setup:60501 (1.93)
#endif
#define EXTEND_CUSTOMIZE_DELTA TSTROFF(0x1000) // カスタマイズ領域の拡張単位
#define DEF_HIST_SIZE TSTROFF(0x10000) // 初期ヒストリサイズ

#ifdef _WIN64
#define TextStackSize 0x4000	// テキストスタックの大きさ(64bit)
#else
#define TextStackSize 0x1000	// テキストスタックの大きさ(32bit)
#endif

#define TIMERID_PREVENTSLEEP 0x7401 // スリープ抑制タイマ
#define TIME_PREVENTSLEEP (60 * 1000 * 3) // スリープ抑制タイマの間隔(3分)
#define TIMERID_MSGBOX_AUTORETRY 0x7402 // メッセージボックス表示中の自動再試行
#define TIME_MSGBOX_AUTORETRY 300 // (0.3秒)
#define TIMERID_MSGBOX_TIMER 0x7403 // メッセージボックス表示中のカウント表示
#define TIME_MSGBOX_TIMER 1000
#define TIMERID_CONSOLE_ASSIST 0x7404 // ダイアログをコンソールから操作するための監視
#define TIME_CONSOLE_ASSIST 200
#define TIMERID_SHUTDOWN_FAULT 0x7405 // シャットダウン時に異常終了が発生したときの強制ダイアログ選択
#define TIME_SHUTDOWN_FAULT (20 * 1000)
#define TIMERID_HOT_REST 0x7406 // 高温停止中の温度監視タイマ
#define TIME_HOT_REST (5 * 1000)

#define ERROR_EX_RETURN	ERROR_CONTINUE // *return コマンドによる終了通知

//==================================================== 共有メモリ(PPCOMMON.MEM)
#if defined(UNICODE) && !defined(_WIN64)
	#pragma pack(push, 8) // 64bit版と共用できるように
	#define SVAL3264(type, name) type name; type dummy##name
	#define PAD3264(name, size) char PAD##name[size];
#else
	#define SVAL3264(type, name) type name
	#define PAD3264(name, size)
#endif

#define UsePPxSpinMaxTime 2000 // 専有可能な最大時間
#define UsePPxMaxTime 5000 // 専有可能な最大時間
typedef struct  {
	volatile LONG SpinLock;	// この構造体へのアクセス許可
	DWORD ThreadID;	// 利用中のThreadID
	int SpinCount;		// 再入回数
	DWORD dummy; // TickCount;		// 利用開始時間
} UsePPxSyncStruct;

typedef struct  {	// PPx Main Window 管理構造体
	SVAL3264(HWND, hWnd);		// HWND
	SVAL3264(HWND, UsehWnd);	// 占有している HWND
	DWORD	ProcessID;			// プロセスID
	DWORD	ActiveID;			// アクティブ順を表す数値
	TCHAR	ID[REGEXTIDSIZE];	// +0, 1:Name, +2:R-ID, +3:\0
								// ここが "" なら使用していない
								//		ex) C_A / V_B / V_C
	TCHAR	path[VFPS];
} ShareX;

#define JOBFLAG_NAMEMASK	0x7f // JobX.state B0-6:実行コマンド
#define JOBFLAG_STATEMASK	0xff80 // JobX.state B7-15:ステータス
#define JOBFLAG_CHANGESTATE	B7 // STATE変更のみ、回数確認しない
// flags
#define JOBFLAG_ENABLEWAIT	B8 // Job待機が必要な処理
#define JOBFLAG_ENABLEHIDE	B9 // 隠すことが可能。JobList廃棄時に表示の必要有り
#define JOBFLAG_WAITEXEC	B10
#define JOBFLAG_WAITJOB		B11
#define JOBFLAG_PAUSE		B12
#define JOBFLAG_ERROR		B13
#define JOBFLAG_REG			B14
#define JOBFLAG_UNREG		B15
// 登録系
#define JOBSTATE_REGIST		(JOBFLAG_REG | 0) // 実行スレッドの登録(登録されているだけでは待ち対象にならない)
#define JOBSTATE_STARTJOB	(JOBFLAG_REG | JOBFLAG_ENABLEWAIT) // 1Job(待機が必要な処理)の開始
#define JOBSTATE_WAITEXEC	(JOBFLAG_REG | JOBFLAG_WAITEXEC) // Process終了待機中
#define JOBSTATE_WAITJOB	(JOBFLAG_REG | JOBFLAG_WAITJOB) // Job実行待機中
#define JOBSTATE_PAUSE		(JOBFLAG_REG | JOBFLAG_PAUSE | JOBFLAG_CHANGESTATE) // 一時停止
#define JOBSTATE_ERROR		(JOBFLAG_REG | JOBFLAG_ERROR) // エラー発生中
// 登録解除系
#define JOBSTATE_UNREGIST	(JOBFLAG_UNREG) // 実行スレッドの登録解除
#define JOBSTATE_ENDJOB		(JOBFLAG_UNREG | JOBFLAG_ENABLEWAIT) // 1Jobの終了
#define JOBSTATE_FINWEXEC	(JOBFLAG_UNREG | JOBFLAG_WAITEXEC) // 待機完了
#define JOBSTATE_FINWJOB	(JOBFLAG_UNREG | JOBFLAG_WAITJOB) // 待機完了
#define JOBSTATE_UNPAUSE	(JOBFLAG_UNREG | JOBFLAG_PAUSE | JOBFLAG_CHANGESTATE) // 再開
#define JOBSTATE_DEERROR	(JOBFLAG_UNREG | JOBFLAG_ERROR) // エラー発生中
// 経過
#define JOBSTATE_SETRATE	(JOBFLAG_CHANGESTATE) // (cmd)進捗の通知(B0-B7:%)

typedef union {
	DWORD dw;
	struct {
		BYTE type;	// JOBSTATE_xxx
		BYTE flags;	// JOBFLAG_xxx
		BYTE rate;	// 進捗率
		BYTE count;	// 使用数カウント
	} b;
} JobXstate;

typedef struct	 {	// PPx Job 管理構造体
	SVAL3264(HWND, hWnd);
	DWORD ThreadID;
	JobXstate state;
} JobX;

typedef struct {	// PPv 通信用
	DWORD flags;
	TCHAR param[CMDLINESIZE];	// 表示ファイル名(size=0:なし)
} PPvShareX;

typedef struct {	// PPc 管理用
	SVAL3264(HWND, hComboWnd[X_MaxComboID]);	// Combo window
	SVAL3264(HWND, hLastFocusWnd);
	int PrevFocusID;	// LastFocusIDの一つ前
	int LastFocusID;	// 最後にフォーカス移動したPPc
} PPcShareX;

#define ShareM_VersionValue ((ShareM_Version << 18) | sizeof(ShareM))
#define PPbMaxSendSize (CMDLINESIZE - 1)
typedef struct {
											//-------------------------- COMMON
	int Version;					// 起動中の PPcommon の version
									//  ShareM_VersionValue が入る
// UsePPx / FreePPx
	UsePPxSyncStruct UsePPxSync;
// カスタマイズ重複管理用
	volatile DWORD CustomizeNameID;	// カスタマイズ領域名の付加ID
	volatile DWORD CustomizeWrite;	// カスタマイズ領域を書き換えて、ブロックがずれるときはインクリメントされる
// PPbとの通信
	TCHAR Param[RECEIVESTRINGSLENGTH];	// 通信用 >H (option32) (;HWND10), 計46
	TCHAR ParamID[REGIDSIZE];		// Param の相手
// 一行編集
	TCHAR TextStack[TextStackSize];	// Text Stack
	PAD3264(1, 4)
// PPx管理
	ShareX P[X_Mtask];				// xUI 管理テーブル
	struct {	// ActiveID (最後にアクティブになったPPxの把握)の最小値、最大値
		int /*low, */high;
	} Active;
	PAD3264(2, 4)
// ログ窓
	SVAL3264(HWND, hCommonLogWnd);		// ログウィンドウ
// PPc関係
	PPcShareX ppc;
// PPvとの通信
	PPvShareX view;
	PAD3264(3, 4)
// 一行編集フック
	SVAL3264(HHOOK, hhookCBT); // Edit 拡張用フック
// Job 管理
	JobX Job[X_MaxJob];				// Job 管理テーブル

#define JOBLIST_OFF 0
#define JOBLIST_FIXED 1
#define JOBLIST_FLOAT 2
#define JOBLIST_INNER 3

#define JOBLIST_SHOW 0
#define JOBLIST_HIDE 1
	struct {
		SVAL3264(HWND, hWnd); // Job一覧を表示するウィンドウ
		DWORD showmode, hidemode;
	} JobList;
// その他
	DWORD EnumNerServerThreadID;	// \\ / \\+ のPC一覧取得用スレッドのID

	BOOL NowShutdown;				// シャットダウン中かどうか

#if PPXSYNCDEBUG
	char UsePPxFirst[96]; // UsePPx を使用している場所
	char UsePPxLast[96];  // 同スレッドが再入した場所
#endif
} ShareM;

#if defined(UNICODE) && !defined(_WIN64)
	#pragma pack(pop)
#endif
//================================================================
#define RUNAS_NOCHECK	 0	// 未判定
#define RUNAS_NORMAL	 1	// RunAs 使用せず
#define RUNAS_RUNAS		 2	// 別ユーザで起動中
//#define RUNAS_ELEVATION	 3	// 昇格中
//#define RUNAS_LIMITED	 4	// 制限中

// EnumCustTable 等で使用する最終アクセス位置キャッシュ
typedef struct {
	BYTE *DataPtr;		// 該当 CustData の先頭
	BYTE *SubPtr;		// SubOffset 番目の Table の位置
	DWORD Counter;		// Sm->CustomizeWrite の控え
	int SubOffset;	// offset のメモ
} ENUMCUSTCACHESTRUCT;
extern ENUMCUSTCACHESTRUCT enumcustcache;

//================================================================ コマンド実行
#define CmdFuncMaxLen	64	// *commane / %*function の名称最大値 + 1

// Z->status
#define ST_2ndLOOP		B0	// %Orによる２回目以降のコマンドライン実行中(%Q用)
#define ST_NO_NEXTENUM	B1	// 次のコマンドライン実行判定は既にした
#define ST_LOOPEND		B2	// 列挙対象はもう無い

#define ST_EXTENDDST	B3	// Z->ExtendDst を使用中
#define ST_LONGRESULT	B4	// Z->LongResultLen の値が有効

// 部分編集( %!, %E, %} )の挙動調整、１文のみ有効
#define ST_PATHITEM		B6	// 部分編集はパス関係
#define ST_PATHFIX		B7	// 部分編集はパスなので補完やVFSFixPathが必要
#define ST_USECACHE		B8	// 部分編集でキャッシュを使用(%$x)
#define ST_EDITHIST		B9	// 部分編集のヒストリ指定有り(options.hist_xxx)

// ディレクトリ時展開( %S )の判定用、コマンドライン内で有効、%Or 無効
#define ST_SDIRREF		B10	// サブディレクトリを参照しているので、%S で展開
#define ST_CHKSDIRREF	B11	// ST_SDIRREF の判定を済ませている

#define ST_MASK_FIRSTCMDLINE (ST_2ndLOOP) // コマンドライン解釈開始時に維持するフラグ


typedef struct tagOLDSRCSTRUCT {	// 入れ子解析時に使用する
	const TCHAR *src;
	struct tagOLDSRCSTRUCT *backptr;
} OLDSRCSTRUCT;

#define TINPUT_EDIT_OPTIONS_use_refline B0
#define TINPUT_EDIT_OPTIONS_use_optionbutton B1
#define TINPUT_EDIT_OPTIONS_use_refext B2
#define TINPUT_EDIT_OPTIONS_UI_mask (B0 | B1 | B2)
#define TINPUT_EDIT_OPTIONS_single_param B3
typedef struct {
	WORD hist_readflags;
	BYTE hist_writetype;
	BYTE flags;
} TINPUT_EDIT_OPTIONS;

#define TIEX_Z_HIST_SETTINGED B31 // Tinput のヒストリ設定が済んでいる

typedef struct {
	BOOL quotation;			// 関数モジュールのパラメータが " を使用中
	DWORD off; // 関数の開始位置byte ※ 0:未使用 1-:開始位置 + 1
} EXECFUNC;

typedef struct {
	HWND hWnd;				// オーナーのウィンドウハンドル
	PPXAPPINFO *Info;		// 親からの情報取得関数
	POINT *posptr;			// メニュー表示に使用する座標
	const TCHAR *src;		// 解析先頭
	TCHAR *dst;				// 展開位置
	TCHAR *extract;			// 展開先

	ThSTRUCT ExtendDst;		// 展開内容の拡張保存領域
	ThSTRUCT StringVariable;	// 文字列変数領域

	PPXCMDENUMSTRUCT EnumInfo;
	OLDSRCSTRUCT *oldsrc;
	ThSTRUCT ExpandCache;	// %M 展開内容の繰り返し実行用キャッシュ
							// 1ststr:キャッシュ項目
							// 2ndstr:キャッシュ展開内容
							// %M を複数回使ったときは、複数組みキャッシュされる

	ERRORCODE result;		// 戻り値の保存
	DWORD ExitCode;			// XEO_SEQUENTIAL 指定時の プロセス終了コード
	DWORD command;			// CID_ // ※表示用展開はCID_FILE_EXEC限定
	DWORD LongResultLen;	// dst の最大値 ( ST_LONGRESULT 有効時 )
	int status;				// 状態保存フラグ (上記ST_ を使用)
	int useppb;				// 専有している PPB の ID
	int flags;				// 各種フラグ XEO_????

	EXECFUNC func;

	struct {	// 編集関連
		int EdPartStart;		// %{ の位置
		int EdOffset;			// 部分編集(%{ / %!)開始位置
		int CsrStart, CsrEnd;	// カーソル／選択開始／終了位置(%|)

		struct EXECSTRUCT_Cache {
			const TCHAR *srcptr; // キャッシュ対象テキストのsrcの位置
			DWORD hash;			// キャッシュ対象テキストのハッシュ
		} cache;
		TINPUT_EDIT_OPTIONS options;
	} edit;

	TCHAR curdir[VFPS];			// カレントディレクトリ
	TCHAR DstBuf[CMDLINESIZE * 2];	// 変換後の内容を保存する一時領域
} EXECSTRUCT;

typedef struct {
	const TCHAR *title;
	const TCHAR *text;
	HFONT hDlgFont;
	HWND hOldFocusWnd, hOldForegroundWnd;
	TIMERPROC autoretryfunc;
	DWORD style;
} MESSAGEDATA;

// CD-ROM ********************************
#define ISO9660HEADER	"\1CD001"
#define JOLIETHEADER	"\2CD001"
#define UDFHEADER		"\0BEA01"
#define CDHEADERSIZE	6

//================================================================ Win32API
#if !defined(SEF_DACL_AUTO_INHERIT) || defined(__BORLANDC__)
	typedef LONG (NTAPI *PVECTORED_EXCEPTION_HANDLER)(
		struct _EXCEPTION_POINTERS *ExceptionInfo
	);
#endif

TypedefWinAPI(HRESULT, SHGetFolderPath, (HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath));
TypedefWinAPI(HANDLE, OpenThread, (DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId));
TypedefWinAPI(PVOID, AddVectoredExceptionHandler, (ULONG First, PVECTORED_EXCEPTION_HANDLER Handler));
TypedefWinAPI(ULONG, RemoveVectoredExceptionHandler, (PVOID Handle));

ExternWinAPI(BOOL, GetConsoleMode, (HANDLE, LPDWORD));
ExternWinAPI(BOOL, SetConsoleMode, (HANDLE, DWORD));
ExternWinAPI(BOOL, ReadConsoleInput, (HANDLE, PINPUT_RECORD, DWORD, LPDWORD));
ExternWinAPI(BOOL, PeekConsoleInput, (HANDLE, PINPUT_RECORD, DWORD, LPDWORD));
ExternWinAPI(BOOL, SetConsoleCtrlHandler, (PHANDLER_ROUTINE, BOOL));
ExternWinAPI(BOOL, ReadConsole, (HANDLE, LPVOID , DWORD , LPDWORD, LPVOID));
ExternWinAPI(HANDLE, OpenThemeData, (HWND hwnd, LPCWSTR pszClassList));
ExternWinAPI(HRESULT, CloseThemeData, (HANDLE hTheme));

extern HANDLE hConsoleStdin;
extern LOADWINAPISTRUCT ConsoleAPI[];

#define DLLWndClassCount 9
typedef union {
	ATOM list[DLLWndClassCount];
	struct {
		ATOM ContextMenuRecieve;
		ATOM Dummy;
		ATOM FileOp;
		ATOM FloatMenu;
		ATOM Joblist;
		ATOM PPe;
		ATOM PPxStatic;
		ATOM PopupMenu;
		ATOM Tree;
	} item;
} DLLWndClassUnion;

//================================================== グローバル変数(PPD_GVAR.C)
#define GLOBALEXTERN
#include "PPD_GVAR.C"
#undef GLOBALEXTERN

//==================================================================== 内部関数
//------------------------------------------------------------------ ppd_ccmd.c
#define EWX_EXFLAG B31
#define EWX_EX_SUSPEND (EWX_EXFLAG | B0)
#define EWX_EX_HIBERNATE EWX_EXFLAG
extern void ExitSession(HWND hWnd, UINT options);
extern void LockPC(void);
extern void SendWmSysyemMessage(WPARAM command, LPARAM lParam);
extern void Get_X_save_widthUI(TCHAR *path);
extern void WindowZPosition(HWND hWnd, HWND mode);
extern void ChangeOpaqueWindow(HWND hWnd, int opaque);

//------------------------------------------------------------------ ppd_cmd.c
extern DWORD_PTR USECDECL PPxDefInfoFunc(PPXAPPINFO *ppxa, DWORD cmdID, PPXAPPINFOUNION *uptr);

extern void ExtractPPxCall(HWND hTargetWnd, EXECSTRUCT *Z, const TCHAR *macroparam);
extern void InsertSrc(EXECSTRUCT *Z, const TCHAR *newsrc);
extern void GetPopupPoint(EXECSTRUCT *Z, POINT *pos);
extern DWORD GetFmacroOption(const TCHAR **string);
extern void GetFmacroString(DWORD flag, TCHAR *src, TCHAR *dest);
extern DWORD GetModuleNameHash(const TCHAR *src, TCHAR *dest);
extern TCHAR * USEFASTCALL GetZCurDir(EXECSTRUCT *Z);
extern void Get_F_MacroData(PPXAPPINFO *info, PPXCMD_F *fbuf, PPXCMDENUMSTRUCT *work);
extern BOOL GetEditMode(const TCHAR **param, TINPUT_EDIT_OPTIONS *options);
extern void SetTInputOptionFlags(TINPUT *tinput, TINPUT_EDIT_OPTIONS *options);
extern UTCHAR USEFASTCALL IsEOL(const TCHAR **str);
extern const TCHAR *ZGetTitleName(EXECSTRUCT *Z);
extern BOOL ZTinput(EXECSTRUCT *Z, TINPUT *tinput);
extern void GetValue(EXECSTRUCT *Z, DWORD cmdID, TCHAR *dest);
extern BOOL ZStringVariable(EXECSTRUCT *Z, const TCHAR **src, int mode);
extern BOOL StoreLongParam(EXECSTRUCT *Z, DWORD addlen);
extern DWORD USEFASTCALL GetLongParamMaxLen(EXECSTRUCT *Z);

extern const TCHAR Title_ValueName[];
extern void UserCommand(EXECSTRUCT *Z, const TCHAR *cmdname, const TCHAR *args, const TCHAR *cmdline, TCHAR *dest);
extern DWORD GetCacheHash(EXECSTRUCT *Z);
extern int GetPPxRegID(const TCHAR **src);
extern void ZQuotationDoubler_Buf(EXECSTRUCT *Z);
extern void ZQuotationDoubler_Ext(EXECSTRUCT *Z);
extern void SetLongDest(EXECSTRUCT *Z, const TCHAR *text, int length);
extern void SetLongDestMain(EXECSTRUCT *Z, const TCHAR *text, int length);

#define StringVariable_command	0
#define StringVariable_function	1
#define StringVariable_extract	2

//------------------------------------------------------------------ ppd_cmdc.c
extern void ZExec(EXECSTRUCT *Z);
extern void ZGetName(EXECSTRUCT *Z, TCHAR *dest, TCHAR cmd);
extern void GetWindowHashList(HMENU hMenuDest, DWORD *PopupID);
extern void BreakAction(EXECSTRUCT *Z);

#define Run_LAUNCH	0
#define Run_COMMAND	1
#define Run_FUNCTION	2
extern void CmdRunFunction(EXECSTRUCT *Z, const TCHAR *param, int runmode);

//------------------------------------------------------------------ ppd_cmdl.c
extern UTCHAR GetCommandParameter(LPCTSTR *commandline, TCHAR *param, size_t paramlen);
extern void GetLfGetParam(const TCHAR **param, TCHAR *dest, DWORD destlength);
extern UTCHAR GetCommandParameterDual(LPCTSTR *commandline, TCHAR *param, size_t paramlen);
extern UTCHAR GetOptionParameterML(const TCHAR **commandline, TCHAR *optionname, TCHAR **optionparameter, size_t optionlength);
extern int GetStringListCommand(const TCHAR *param, const TCHAR *commands);

//------------------------------------------------------------------ ppd_code.c
extern INT_PTR GetDigitNumber(const TCHAR **ptr);
#define GetDigitNumber32(ptr) ((int)GetDigitNumber(ptr))
#define GetDigitNumber32u(ptr) ((DWORD)GetDigitNumber(ptr))
extern INT_PTR USEFASTCALL GetHexNumber(const TCHAR **ptr);
extern int USEFASTCALL GetHexChar(TCHAR **p);
extern TCHAR USEFASTCALL upper(TCHAR c);
#ifdef UNICODE
extern INT_PTR GetNumberA(const char **line);
extern int USEFASTCALL GetHexCharA(char **p);
#define upperA(c) (char)upper((TCHAR)c)
#else
#define GetNumberA GetNumber
#define GetHexCharA GetHexChar
#endif
extern TCHAR *Int32ToString(TCHAR *buf, int i);
extern TCHAR *Numer32ToString(TCHAR *buf, DWORD i);
extern TCHAR *Numer64ToString(TCHAR *dest, UINTHL num);

typedef struct { // 色エイリアス
	const TCHAR *str;	//	ラベル名
	COLORREF num;		//	値
} COLORLABEL;

#define ColorNameListTotal (21 + (C_Scheme1_MAX - C_Scheme1_MIN + 1) + (C_Scheme2_MAX - C_Scheme2_MIN + 1))
extern COLORLABEL ColorNameList[ColorNameListTotal];

#ifdef _WIN64
#define MultiByteToWideCharU8(CodePage, dwFlags, lpMultiByteStr, cchMultiByte, lpWideCharStr, cchWideChar) MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cchMultiByte, lpWideCharStr, cchWideChar)
#define WideCharToMultiByteU8(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cchMultiByte, lpDefaultChar, lpfUsedDefaultChar) WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cchMultiByte, lpDefaultChar, lpfUsedDefaultChar)
#else
ExternWinAPI (int, MultiByteToWideCharU8, (UINT CodePage, DWORD dwFlags, const char *lpMultiByteStr, int cchMultiByte, WCHAR *lpWideCharStr, int cchWideChar));
ExternWinAPI (int, WideCharToMultiByteU8, (UINT CodePage, DWORD dwFlags, const WCHAR *lpWideCharStr, int cchWideChar, char *lpMultiByteStr, int cchMultiByte, const char *lpDefaultChar, BOOL *lpfUsedDefaultChar));
#define MultiByteToWideCharU8(CodePage, dwFlags, lpMultiByteStr, cchMultiByte, lpWideCharStr, cchWideChar) DMultiByteToWideCharU8(CodePage, dwFlags, lpMultiByteStr, cchMultiByte, lpWideCharStr, cchWideChar)
#define WideCharToMultiByteU8(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cchMultiByte, lpDefaultChar, lpfUsedDefaultChar) DWideCharToMultiByteU8(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cchMultiByte, lpDefaultChar, lpfUsedDefaultChar)
#endif

//------------------------------------------------------------------ ppd_cust.c
extern TCHAR * PPcustCDumpMain(int ModeFlags);
extern void PPcustCDumpText(const TCHAR *str, const TCHAR *sub, TCHAR **result);

//------------------------------------------------------------------ ppd_dlg.c
// PPXSTATIC で使用可能な制御文字
// ※キャプションを利用するため、\x0 はパラメータに使えない
#define PXSC_PAR		0x1b // 独自プログレスバー parsent(1-100), count(1-10)
#define PXSC_LEFT		0x1c // 左寄せ
#define PXSC_RIGHT		0x1d // 右寄せ
#define PXSC_NORMAL		0x1e // ハイライト解除
#define PXSC_HILIGHT	0x1f // ハイライト

extern LRESULT CALLBACK PPxStaticProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
extern HWND *CreateDialogControls(HANDLE hinst, LPCTSTR lpszTemplate, HWND hParentWnd);
extern void EnableDlgWindow(HWND hDlg, int id, BOOL state);
extern void ShowDlgWindow(const HWND hDlg, const UINT id, BOOL show);
extern void FixBeforeDialog(HWND hParentWnd);
extern BOOL DialogKeyProc(MSG *msg);
extern int IsUseRichEdit(void);
extern const TCHAR *RichEditClassname(void);

//------------------------------------------------------------------ ppd_edl.c
extern void CleanUpEdit(void);
extern BOOL SetWindowTextWithSelect(HWND hED, const TCHAR *defstr);

//------------------------------------------------------------------ ppd_exec.c
#define EEM_GETID			B0	// ProcessID / ThreadID を取得
#define EEM_UseECF			B1	// ExtraCreateFlags を使用
#define EEM_UseESF			B2	// ExtraStartup.flags を使用
#define EEM_NoStartMsg		B3	// 開始表示を無くす
#define EEM_UseSTDIO		(EEM_STDOUT_receive | EEM_STDOUT_value | EEM_STDOUT_pipe | EEM_STDOUT_redir | EEM_STDOUT_log | EEM_STDOUT_PPe | EEM_STDIN_value | EEM_STDIN_pipe | EEM_STDIN_redir) // 標準入出力を変更
#define EEM_STDOUT_receive	B4	// thReceive に保存
#define EEM_STDOUT_value	B5	// 特殊環境変数 stdout に保存
#define EEM_STDOUT_pipe		B6	// パイプ接続する
#define EEM_STDOUT_redir	B7	// ファイルへリダイレクト
#define EEM_STDOUT_log		B8	// ログ窓に送信
#define EEM_STDOUT_PPe		B9	// ログ窓用にPPeを用意
#define EEM_STDIN_value		B10	// テキストをリダイレクト
#define EEM_STDIN_pipe		B11	// パイプ接続する
#define EEM_STDIN_redir		B12	// ファイルからリダイレクト
#define EEM_DesktopLevel	B13	// デスクトップと同じ格で実行(UAC降格)

typedef struct {
	PPXAPPINFO *info;
	DWORD mask; // EEM_xxx
	DWORD *ExitCode;
	DWORD ProcessID;
	DWORD ThreadID;
	DWORD ExtraCreateFlags;
	struct {
		DWORD flags;
		DWORD x, y;
		DWORD width, height;
		HANDLE hStdIn, hStdOut, hStdErr;
	} ExtraStartup;
	struct {
		HWND hPPeWnd;
		UINT codepage;
		ThSTRUCT thReceive;
		HANDLE hPipeOut, hPipeIn;
		HANDLE hRedirOut, hRedirIn;
	} stdio;
} EXEC_EXTRA;

extern BOOL ComExecEx(HWND hOwner, const TCHAR *line, const TCHAR *path, int *useppb, int flag, EXEC_EXTRA *extra);
extern BOOL ComExecSelf(HWND hOwner, const TCHAR *line, const TCHAR *path, int flag, EXEC_EXTRA *extra);
extern BOOL WaitJobDialog(HWND hWnd, HANDLE handle, const TCHAR *title, DWORD flags);

typedef struct {
	MESSAGEDATA md;
	int choose, count, PPbID;
} WAITDLGSTRUCT;
#define WDS_UNCHOOSE -1
#define WDS_SKIP 1
#define WDS_CANCEL 0
#define IsChooseWDS(choose) ((choose) >= 0)
extern const TCHAR strSendPPb[];

extern INT_PTR CALLBACK WaitDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

//------------------------------------------------------------------ ppd_falt.c
extern LONG WINAPI PPxUnhandledExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo);
extern THREADSTRUCT *GetCurrentThreadInfo(void);
extern DWORD UnRegisterThread(DWORD ThreadID);
extern void PPxSendReport(const TCHAR *text);
extern const TCHAR *GetThreadName(DWORD id);
extern void ProcessInfo(void);
extern volatile int EnableExitState;
extern void ReportLCID(LCID userlcid);

//------------------------------------------------------------------ ppd_hist.c
#pragma pack(push, 1)
typedef struct {
	char HeaderID[4]; // HistoryID / CustID が格納
	DWORD FileSize; // ファイルの大きさ
} PPXDATAHEADER;

#define HistHeaderFromHisP ((PPXDATAHEADER *)(HisP - sizeof(PPXDATAHEADER)))
#define HistSizeFromHisP (*(DWORD *)(HisP - sizeof(DWORD)))

#define HIST_HEADER_SIZE (sizeof(PPXDATAHEADER))
#define HIST_HEADER_FOOTER_SIZE (sizeof(PPXDATAHEADER) + sizeof(WORD))

#define CustHeaderFromCustP ((PPXDATAHEADER *)(CustP - sizeof(PPXDATAHEADER)))
#define CustSizeFromCustP (*(DWORD *)(CustP - sizeof(DWORD)))

#define CUST_HEADER_SIZE (sizeof(PPXDATAHEADER))
#define CUST_FOOTER_SIZE (sizeof(DWORD))
#define CUST_TABLE_FOOTER_SIZE (sizeof(WORD))
#define CUST_HEADER_FOOTER_SIZE (sizeof(PPXDATAHEADER) + sizeof(DWORD) + 2) // +2 はオーバラン対策用

typedef DWORD CUST_NEXTDATA_OF;
#define CUST_NEXTDATA_SIZEOF  sizeof(CUST_NEXTDATA_OF)
typedef WORD CUST_NEXTTABLE_OF;
#define CUST_NEXTTABLE_SIZEOF  sizeof(CUST_NEXTTABLE_OF)
#pragma pack(pop)

extern void DefCust(int mode);
extern int CountHistory(WORD type);
extern BOOL LimitHistory(WORD type, DWORD addsize);
extern DWORD USEFASTCALL GetCustDword(const TCHAR *name, DWORD defaultvalue);
extern void LoadMessageTextTable(void);
extern void USEFASTCALL MakeCustMemSharename(TCHAR *custsharename, int id);
extern const TCHAR * USEFASTCALL SearchMessageText(const TCHAR *idstr);
extern void ReloadMessageText(void);
extern void ClearCustomizeArea(void);

//------------------------------------------------------------------ ppd_mdl.c
extern int CommandModule(EXECSTRUCT *Z, const TCHAR *ptr);
extern void ExecuteFunction(EXECSTRUCT *Z);
extern void FreePPxModule(void);
extern UTCHAR ZFixParameter(TCHAR **commandline);

//------------------------------------------------------------------ ppd_menu.c
#define MENUFLAG_SELECT		B0
#define MENUFLAG_NOEXTACT	B1
#define MENUFLAG_SETINDEX	B2
extern BOOL MenuCommand(EXECSTRUCT *Z, TCHAR *menuname, const TCHAR *def, int menuflag);
extern void USEFASTCALL AppendMenuString(HMENU hMenu, UINT id, const TCHAR *string);
extern void USEFASTCALL AppendMenuCheckString(HMENU hMenu, UINT id, const TCHAR *string, BOOL check);

extern int TTrackPopupMenu(EXECSTRUCT *Z, HMENU hMenu, PPXMENUINFO *xminfo);
extern const TCHAR * USEFASTCALL GetMenuDataString(ThSTRUCT *thMenuData, int index);

//------------------------------------------------------------------ ppd_ppe.c
#define PPE_TEXT_OPENNEW ((const TCHAR *)(DWORD_PTR)1)
#define PPE_TEXT_CMDMODE ((const TCHAR *)(DWORD_PTR)2)
typedef struct {
	const TCHAR *dummy;
	const TCHAR *param;
	const TCHAR *curdir;
} PPE_CMDMODEDATA;

//------------------------------------------------------------------ ppd_str.c
extern TCHAR *SkipSpaceAndFix(TCHAR *p);
extern const TCHAR *EscapeMacrochar(const TCHAR *string, TCHAR *buf);

//------------------------------------------------------------------ ppd_sub.c
extern void MakeUserfilename(TCHAR *dst, const TCHAR *src, const TCHAR *idname);
extern HWND GetCaptionWindow(HWND hWnd);
extern HWND GetParentCaptionWindow(HWND hWnd);
extern BOOL DeleteDirectories(const TCHAR *path, BOOL notify);
extern BOOL CheckLoadSize(HWND hWnd, DWORD *sizeL);
extern void USEFASTCALL SetDlgFocus(HWND hDlg, int id);
extern DWORD ReadEString(TCHAR **string, void *ptr, DWORD maxsize);
extern void WriteEString(TCHAR *string, BYTE *ptr, DWORD size);
extern WORD FixCharKeycode(WORD key);
extern DWORD crc32(const BYTE *bin, DWORD size, DWORD r);
extern UINT VTypeToCPlist[VTypeToCPlist_max];
extern HMODULE LoadLibraryTry(const TCHAR *filepath);

#ifdef UNICODE
	#define WriteFileZTSIZE (VFPS * 3) // この長さまでの文字列に対応
	extern BOOL WriteFileZT(HANDLE fileh, const WCHAR *str, DWORD *wrote);
	extern LRESULT SendUTextMessage_U(HWND hWnd, UINT uMsg, WPARAM wParam, const TCHAR *text);
	extern HMODULE LoadLibraryTryA(const char *filepath);
#else
	#define WriteFileZT(fileh, str, wrote) WriteFile(fileh, str, strlen(str), wrote, NULL)
	#define LoadLibraryTryA(filepath) LoadLibraryTry(filepath)
#endif
#define K_mouse K_internal // マウスによる操作(メニュー位置判定用)
extern void GetPopMenuPos(HWND hWnd, POINT *pos, WORD key);
extern HMODULE LoadSystemWinAPI(DWORD dllID, LOADWINAPISTRUCT *apis);
extern int GetNowTime(TCHAR *text, int mode);
extern BOOL PPRecvExecuteByWMCopyData(PPXAPPINFO *info, COPYDATASTRUCT *copydata);
extern BOOL PPWmCopyData(PPXAPPINFO *info, COPYDATASTRUCT *copydata);
extern void RemoveCharKey(HWND hWnd);
extern BOOL PeekMessageLoop(HWND hWnd);
extern BOOL PeekDialogMessageLoop(HWND hWnd, HWND hDlg);
extern void EndPreventSleep(HWND hWnd);
extern void StartPreventSleep(HWND hWnd, BOOL fop);
extern DWORD_PTR USECDECL DummyPPxInfoFunc(PPXAPPINFO *info, DWORD cmdID, PPXAPPINFOUNION *uptr);
extern HWND CreateDummyWindow(HWND hDisplayWnd, const TCHAR *title);

#define ThAllocCheck(TH)							\
	if ( (TH)->bottom == NULL ){					\
		(TH)->bottom = HeapAlloc(ProcHeap, 0, ThSTEP); \
		if ( (TH)->bottom == NULL ) return FALSE;	\
		(TH)->size = ThSTEP;						\
	}

//------------------------------------------------------------------ ppd_task.c
extern void FixTask(void);
extern int SearchPPx(TCHAR *ID);
extern HWND PPxGetWindow(const TCHAR *refid, int d);
extern BOOL USEFASTCALL CheckPPcID(int no);
extern BOOL BootPPB(const TCHAR *path, const TCHAR *line, int flags);
extern int UsePPb(HWND hOwner);
extern int SendPPB(HWND hOwner, const TCHAR *param, int useID);
extern int LongSendPPB(HWND hOwner, const TCHAR *param, TCHAR command, int useID);

#define PPBSPECIAL_EXITCODE	T("=E")
extern DWORD RecvPPBExitCode(HWND hOwner, int useID);
extern DWORD PPbSpecialMessage(HWND hOwner, int useID, const TCHAR *command);
extern void FreePPb(HWND hOwner, int useID);
extern void DeleteJobTask(void);
extern BOOL SetJobTask(HWND hWnd, DWORD state);
extern BOOL JobListMenu(EXECSTRUCT *Z, TCHAR param);
extern void CheckJobList(void);
extern BOOL CheckJobWait(void);
extern void SetAppID(void);
extern void CreateJobListWnd(void);
extern void USEFASTCALL ClosePPxOne(HWND hWnd, BOOL AllClose);

#define GetPPxList_hWnd 0
#define GetPPcList_Path 1	// Pathをitem/thMenuDataから取得
#define GetPPxList_Select 2	// *focusコマンドをthMenuDataから取得
#define GetPPxList_Id 3		// idをthMenuDataから取得
#define GetPPxList_IdList 4	// 「id,」をthMenuDataから取得
extern int GetPPxList(HMENU hPopupMenu, int mode, ThSTRUCT *thMenuData, DWORD *menuid);
extern int GetPPxListFromProcessCombo(HMENU hPopupMenu, int mode, ThSTRUCT *th, DWORD *useppclist, MENUITEMINFO *minfo, HWND hComboWnd);
extern int GetPPxListFromCurrentCombo(HMENU hPopupMenu, int mode, ThSTRUCT *th, DWORD *useppclist, MENUITEMINFO *minfo);
ExternWinAPI(BOOL, SetDllDirectory, (LPCTSTR lpPathName));


//------------------------------------------------------------------ ppd_ui.c
#define PPXFLASH_STOP -1
#define PPXFLASH_FLASH 0
#define PPXFLASH_NOFOCUS 1
extern BOOL PPxFlashWindow(HWND hWnd, int mode);
extern UINT GetMonitorDPI(HWND hWnd);
extern UINT USEFASTCALL GetGDIdpi(HWND hWnd);
#define ID_PPX_CHECKED B7

// B24 以降は PPx 独自設定
// MessageBox は B21(MB_SERVICE_NOTIFICATION) まで使用済み
#define MB_PPX_EXMSGBOX    B31
#define MB_PPX_ALLCHECKBOX B30
#define MB_PPX_AUTORETRY   B29
#define MB_PPX_NOCENTER    B28
// #define MB_PPX_ENABLE_MINIMIZE     B27 未使用
// B24(MB_PPX_PLAIN_TEXT)-B26(MB_PPX_BUTTON_TEXT) 公開設定

#define MB_PPX_ADDABORTRETRYIGNORE 7
#define MB_PPX_STARTCANCEL 8
#define MB_PPX_YESNOIGNORECANCEL   9
#define MB_PPX_USEPPXCHECKOKCANCEL 10
#define MB_PPX_ABORTRETRYRENAMEIGNORE 11
#define MB_PPX_ABORTOPTION 12
#define MB_PPX_MAX 13
extern BOOL MessageBoxInitDialog(HWND hDlg, MESSAGEDATA *md);
extern INT_PTR CALLBACK MessageBoxDxProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
extern void PopupErrorMessage(HWND hWnd, const TCHAR *title, const TCHAR *msg);
extern void PrintToConsole(const TCHAR *text);

extern void ClipTextData(HWND hWnd, const TCHAR *text);
extern int CriticalMessageBox(const TCHAR *text, const TCHAR *title, UINT style);
extern int CriticalMessageBoxOk(const TCHAR *text);
extern BOOL PPxDialogHelp(HWND hDlg);
// FillRect 置き換え
extern void FillBox(HDC hDC, const RECT *box, HBRUSH hbr);
extern void PlayWave(const TCHAR *name);
extern BOOL OpenClipboard2(HWND hWnd);

extern HBRUSH GetFrameShadowBrush(void);
extern HBRUSH GetFrameFaceBrush(void);
extern HBRUSH GetFrameHighlightBrush(void);
extern HBRUSH GetHighlightBackBrush(void);
extern HBRUSH GetEdgeLineBrush(void);
extern HBRUSH GetDialogBackBrush(void);
extern void InitSysColors_main(void);
#define InitSysColors() {if ( ThemeColors.ExtraDrawFlags & EDF_NOT_INIT ) InitSysColors_main();}
extern void FreeSysColors(void);
extern BOOL InitUnthemeDLL(void);
extern void InitUnthemeCmd(void);
extern void ReloadUnthemeCmd(void);
extern int GetUxtMode(void);
extern LRESULT ControlWindowColor(WPARAM wParam);
#define GMH_INWINDOW 0	// hBaseWnd が有るモニタ
extern HMONITOR GetMonitorHandle(HWND hBaseWnd, int mode);
extern int PMessageBoxEx(HWND hWnd, MESSAGEDATA *md);
extern int TrackPopupMenuConsole(HMENU hMenu);

extern DWORD UseConsoleKey(void);
extern void FreeConsoleKey(DWORD oldmode);
extern int ReadConsoleKey(void);
extern HANDLE hConsoleStdIn;

//------------------------------------------------------------------ ppd_wild.c
extern void FreeRegexpDll(void);

#define SLASH_REQUIRE 0
#define SLASH_FREE 1
#define SLASH_SEARCH 2

#ifndef PXREGEXPS_DEF
typedef struct {int dummy;} PXREGEXPS;

extern BOOL InitRegularExpression(PXREGEXPS **rxps, TCHAR *rxstring, int slash);
extern const TCHAR *RegularExpressionReplace(PXREGEXPS *rxps, TCHAR *target, TCHAR *dest, size_t dest_buflen);
extern void FreeRegularExpression(PXREGEXPS *rxps);
extern TCHAR *GetRegularExpressionName(TCHAR *str);
extern TCHAR *RegularExpressionSearch(PXREGEXPS *rxps, TCHAR *target, size_t target_len, size_t *hitlen);
#endif
extern BOOL InitMigemo(int preload);
extern void FreeMigemo(void);

extern LOADWINAPISTRUCT OLEAUT32_SysStr[];
extern HRESULT (STDAPICALLTYPE *DGetActiveObject)(REFCLSID rclsid, void * pvReserved, IUnknown ** ppunk);
extern HRESULT (STDAPICALLTYPE *DRegisterActiveObject)(IUnknown * punk, REFCLSID rclsid, DWORD dwFlags, DWORD *pdwRegister);
extern HRESULT (STDAPICALLTYPE *DRevokeActiveObject)(DWORD dwRegister, void *pvReserved);

extern LOADWINAPISTRUCT OLEAUT32_Variant[];
extern void (STDAPICALLTYPE *DVariantInit)(_Out_ VARIANTARG * pvarg);
extern HRESULT (STDAPICALLTYPE *DVariantClear)(_Inout_ VARIANTARG * pvarg);
extern HRESULT (STDAPICALLTYPE *DVariantChangeType)(_Inout_ VARIANTARG * pvargDest, _In_ VARIANTARG *pvarSrc, _In_ USHORT wFlags, _In_ VARTYPE vt);

extern BSTR (STDAPICALLTYPE *DSysAllocString)(const OLECHAR *);
extern BSTR CreateBstringLenA(const char *string, int size);
#define CreateBstringA(string) CreateBstringLenA(string, -1)
#ifdef UNICODE
#define CreateBstring(string) DSysAllocString(string)
#define CreateBstringLen(string, size) DSysAllocStringLen(string, size)
#else
#define CreateBstring(string) CreateBstringA(string)
#define CreateBstringLen(string, size) CreateBstringLenA(string, size)
#endif

extern HMODULE hOleaut32DLL;

typedef struct {
	DWORD value, mask;
} GETPARAMFLAGSSTRUCT;

extern void GetParamFlags(GETPARAMFLAGSSTRUCT *gpfs, const TCHAR **param, const TCHAR *flagnames);
extern const TCHAR AttrLabelString[];

//---------------------------------------------------------------- ppd_subp.cpp
extern void DragDropPath(HWND hWnd, const TCHAR *path);
extern LRESULT CALLBACK AutoDDDLLProc(int nCode, WPARAM wParam, CWPSTRUCT *lParam);
extern HRESULT GetLink(HWND hWnd, const TCHAR *LinkFile, TCHAR *Original);

extern int CheckParamOnOff(LPCTSTR *str);

extern int PopupButtonMenu(HMENU hPopup, HWND hWnd, HWND hButtonWnd);
extern void EndButtonMenu(void);
extern ERRORCODE IsShowButtonMenu(UINT buttonid);

#ifndef STARTF_UNTRUSTEDSOURCE
	#define STARTF_UNTRUSTEDSOURCE 0x00008000
	#define STARTF_MONITOR 0x400
#endif
#ifndef CREATE_SECURE_PROCESS
	#define CREATE_SECURE_PROCESS  0x00400000
#endif

#ifdef __cplusplus
}
#endif
