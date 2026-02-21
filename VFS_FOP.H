/*-----------------------------------------------------------------------------
	Paper Plane xUI	 Virtual File System			ファイル操作/ *fileコマンド
-----------------------------------------------------------------------------*/
#define SECTORSIZE 2048
#define MAXACTIONNAME 32

#define WM_FOP_COMMAND (WM_APP + 1062) // WPARAM: WM_FOP_CMD_xxx
#define WM_FOP_CMD_CREATE_LOGWINDOW 1 // ログ窓を用意する
#define WM_FOP_CMD_END_OPERATION 2 // 別スレッドで作業完了 LPARAM: result
#define WM_FOP_CMD_PROGRESS 3 // 進捗更新
#define WM_FOP_CMD_INIT 4 // initcmd 実行
#define WM_FOP_CMD_START_SELECT_SAME 5 // 同名時の選択し動作を開始・やり直し

// 削除するとき問題になる属性
#define OPENERRORATTRIBUTES (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)

#define FOPERROR_GETLASTERROR (ERRORCODE)-1
#define FOPERROR_DIV (ERRORCODE)-2

#ifndef COPY_FILE_ALLOW_DECRYPTED_DESTINATION
#define COPY_FILE_ALLOW_DECRYPTED_DESTINATION 0x8
#endif

enum fop_states {
	FOP_UNKNOWN,	// 未使用
	FOP_READY,		// 開始前の設定
	FOP_BUSY,		// 処理中
	FOP_TOPAUSE,	// 中断希望
	FOP_PAUSE,		// 中断中
	FOP_TOBREAK,	// 中止希望
	FOP_COUNT,		// ファイル数計算中
	FOP_SKIP,		// 計算省略/待機省略希望
	FOP_HOTREST,	// 高温による停止中
	FOP_WAIT,		// 処理完了後の待機中
	FOP_END			// 終了
};

#define FOPR_UNKNOWN 0	// リパース選択肢未選択

struct tagFOPSTRUCT;

#define SECURITY_FLAG_NONE 0
#define SECURITY_FLAG_SCOPY (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION)
#define SECURITY_FLAG_SACL (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)

struct ProgressInfo {
	DWORD mark, markall;	// 処理済マーク数、全マーク数
	DWORD donefiles;		// 処理済ファイル数。MAX32: SHN や Image
	DWORD filesall;			// 全ファイル数。 0: 総数未計算
	INTHL donesize, allsize;	// 処理済バイト数、全バイト数
	DWORD LEskips, EXskips, errors; // 軽微エラー無視によるスキップ数、上書き無しスキップ数、エラー数
	DWORD busybar;			// ワーキングカウンタ
	DWORD count_exists; // 数算出時に検出した同名ファイル数
	DWORD exists[3]; // 比較結果 new date, same date, old date
};

#define PROGRESS_INTERVAL_TICK 100
#define CAPTION_INTERVAL_TICK 700
struct _ProgressWndInfo {
	HWND hWnd; // FS->hDlg 相当
	HWND hSrcNameWnd;  // IDS_FOP_SRCNAME の hWnd
	HWND hProgressWnd; // IDS_FOP_PROGRESS の hWnd
	const TCHAR *srcpath; // Source 欄に表示する文字列。NULL:表示不要
	struct tagFOPSTRUCT *FS;

	INTHL FileTransSize, FileSize; // 1file copy の進捗状況

	DWORD StartTick; // 経過時間計算用Tick
	DWORD ProgTick; // プログレス表示計算用Tick(Source 欄表示兼用)
	DWORD CapTick;  // キャプション更新用Tick
	struct ProgressInfo info;	// 状況表示関係(OperationStart開始時に0 fill)
	int nowper;
};

struct HotCheckInfo {
	HANDLE hSrcDisk;
	HANDLE hDestDisk;
	DWORD LastTick; // 経過時間計算用Tick
};

#define ProgressShowType_MAX 4 // 0:累積コピーサイズ 1:経過,残り時間 2:転送レート 3:なし
#define SrcShowType_MAX 4 // 0:左揃え(はみ出し時は右揃え) 1:左(最終エントリ左) 2:右(左) 3:左(ディレクトリ部のみ右)

#define RENAME_NUMBERING '\\'
#define RENAME_EXTRACTNAME ':'
#define RENAME_REGEXP '/'

struct FopOption {
	TCHAR source[VFPS];
	const TCHAR *files;
	HWND hReturnWnd;
	BYTE *CopyBuf;	// コピー用バッファ
	TCHAR *initcmd; // 初期化時のコマンド
	TCHAR *compcmd; // 正常完了時のコマンド
	PXREGEXPS *rexps; // 名前変更用正規表現

	BOOL AllocFiles; // files を内部で生成した
	#define NameFilter_Use B0
	#define NameFilter_Rename B1
	#define NameFilter_ExtractName B2
	int UseNameFilter; // 名前加工の有無
	int SrcDtype; // files １つ目のドライブ種別 VFSDT_
	DWORD fopflags; // VFSFILEOPERATION の VFSFOP_

	VFSFOP_OPTIONS fop; // 設定原本
	TCHAR action[MAXACTIONNAME];	// Action Name
	TCHAR rename[MAX_PATH];	// Rename
	TCHAR renum[12];	// S.num
	int OnDriveMove;	// 同名ドライブで移動指定なら !0
	int burst;		// バーストモードなら真
	DWORD erroraction;		// エラー時の対処方法
	DWORD errorretrycount;	// エラー時の再試行回数
	struct { // 全て K 単位
		DWORD CopyBufSize;
		DWORD EnablePPxBurstSize;
		DWORD DisableApiCacheSize;
	} X_cbsz;
	SECURITY_INFORMATION security;
};

enum foptab { // 現在タブ FOPTAB_ ※-1は未設定
	FOPTAB_GENERAL = 0, FOPTAB_RENAME, FOPTAB_OPTION, FOPTAB_NONE = -1
};
struct pageinfo {
	HWND *Controls; // ページ内のコントロールのウィンドウハンドル
	enum foptab showID;
};

typedef struct tagFOPSTRUCT{
	HWND hDlg;	// メインダイアログ
	HWND hDstED;	// コピー先のテキストボックス

	UINT Command;	// 同名処理時のボタン操作に使用
	int errorcount;
	BOOL Cancel;	// キャンセルの指定をするために使用
	BOOL testmode; // テスト処理かどうか
	BOOL flat;		// ディレクトリ作成をなくして平坦化(VFSFOP_OPTFLAG_FLATMODE)
	BOOL renamemode;
	BOOL BackupDirCreated; // deleted$ が作成済みか
	BOOL FixDest;	// 処理先が固定 (VFSFOP_SPECIALDEST)
	BOOL NoAutoClose; // ログ確認のため、終了後もダイアログを閉じない
	BOOL hide; // 普段は隠れているか
	BOOL DestroyWait; // WM_DESTROY で待機をするか(Fop_ShellNameSpaceで、メディアプレイヤーへ処理を行うとき、メッセージポンプをしばらく動かさないとうまく動作しないための対策)
	DWORD UI_ThreadID;

	int reparsemode; // リパースポイントの扱い

	volatile enum fop_states state;	// 動作状況

	struct FopOption opt;
	struct _ProgressWndInfo progs;	// 状況表示関係
	struct HotCheckInfo hotc;
	DELETESTATUS DelStat;
	PPXAPPINFO DelInfo;

	struct { // 結果ログ
		HWND hWnd;
		ThSTRUCT cache;
		DWORD tick;
	} log;

	HANDLE hUndoLogFile; // UNDOログ
	HANDLE hStartMutex; // コピー開始待機用のMutex
	HANDLE hOperationThread; // 実際の操作を行うスレッド
	HANDLE hStateChangeEvent; // OperationThread に状態変化を伝える
	PPXAPPINFO *info;
	struct pageinfo page;
	TCHAR DestDir[VFPS];
	int UiEnableSetting; // グレーにするコントロールの状態
	FN_REGEXP maskFn; // ファイル名マスク
	DWORD maskFnFlags;
	DWORD RunID;
	xIFileOperation *ifo;
} FOPSTRUCT;

typedef struct {
	VFSFILEOPERATION *fileop;
	FOPSTRUCT *FS;
} FILEOPERATIONDLGBOXINITPARAMS;

#define CAPTION "File Operation Options"

enum foplogtypes {
	LOG_COPY = 0, LOG_COPYOVERWRITE, LOG_MOVE, LOG_MOVEOVERWRITE,
	LOG_APPEND, LOG_BACKUP, LOG_LINK, LOG_MOVEDIR, LOG_SKIP,
	LOG_DIR, LOG_DIR_DELETE, LOG_DELETE, LOG_STRING,
	LOG_MAKEDIR, LOG_COMPLETED
};

#define CreateFile_OpenSource(SRC, FILEMODE) CreateFileL(SRC, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILEMODE, NULL)

void FopLog(FOPSTRUCT *FS, const TCHAR *src, const TCHAR *dst, enum foplogtypes type);

//----------------------------------------------------------------- vfs_fop.c
extern DWORD X_fopi, X_fopg, X_fost;

#define Operation_END_ERROR FALSE
#define Operation_END_SUCCESS TRUE
#define Operation_WORKING 2
extern int OperationStart(FOPSTRUCT *FS);
extern void FWriteLog(FOPSTRUCT *FS, const TCHAR *message);
extern void CreateFWriteLogWindow(FOPSTRUCT *FS);
extern void CreateFWriteLogWindowMain(FOPSTRUCT *FS);
extern void FWriteLogMsg(FOPSTRUCT *FS, const TCHAR *mes);
extern void FShowLog(FOPSTRUCT *FS);
extern void FWriteErrorLogs(FOPSTRUCT *FS, const TCHAR *mes, const TCHAR *type, ERRORCODE error);
extern void DisplaySrcNameNow(FOPSTRUCT *FS);
extern void TinyDisplayProgress(FOPSTRUCT *FS);
extern void SetTinyProgress(FOPSTRUCT *FS);
extern void FullDisplayProgress(struct _ProgressWndInfo *Progs);
extern void SetFullProgress(struct _ProgressWndInfo *Progs);

#define ACTION_CREATE NO_ERROR
#define ACTION_SKIP		MAX32
#define ACTION_RETRY	(MAX32 - 1)
#define ACTION_APPEND	(MAX32 - 2)
#define ACTION_OVERWRITE (MAX32 - 3)
#define ACTION_ERROR (MAX32 - 4)
ERRORCODE SameNameAction(FOPSTRUCT *FS, HANDLE dstH, BY_HANDLE_FILE_INFORMATION *srcfinfo, BY_HANDLE_FILE_INFORMATION *dstfinfo, const TCHAR *src, TCHAR *dst);

DWORD CALLBACK CopyProgress(LARGE_INTEGER TotalSize, LARGE_INTEGER TotalTransSize, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamTransSize, DWORD StreamNumber, DWORD reason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData);

extern ERRORCODE TestDest(FOPSTRUCT *FS, const TCHAR *src, TCHAR *dst);
extern void EndingOperation(FOPSTRUCT *FS);
extern HWND FopGetMsgParentWnd(FOPSTRUCT *FS);

extern const TCHAR STR_FOP[];
extern const TCHAR STR_FOPWARN[];
extern const TCHAR STR_FOPUNDOLOG[];

extern const TCHAR STR_FOPOK[];
extern const TCHAR STR_FOPSTART[];
extern const TCHAR STR_FOPCONTINUE[];

extern const TCHAR STR_FOPCANCEL[];
extern const TCHAR STR_FOPCLOSE[];

extern const TCHAR STR_FOPPAUSE[];

extern const TCHAR StrLogBackup[];
extern const TCHAR StrLogReplace[];
extern const TCHAR StrLogMove[];
extern const TCHAR StrLogOverwrite[];
extern const TCHAR StrLogCopy[];
extern const TCHAR StrLogTest[];
extern const TCHAR StrLogExist[];
extern const TCHAR StrLogAppend[];
extern const TCHAR StrLogLink[];
extern const TCHAR StrLogMoveDir[];

extern BOOL USEFASTCALL CheckSaveDriveMove(struct FopOption *opt, _In_ const TCHAR *src, _In_ const TCHAR *dst);
extern BOOL Fop_ShellNameSpace(FOPSTRUCT *FS, const TCHAR *srcDIR, const TCHAR *dstDIR);
extern ERRORCODE RenameDestFile(FOPSTRUCT *FS, const TCHAR *dst, BOOL addOldStr);

TypedefWinAPI(HRESULT, SHCreateItemFromParsingName, (PCWSTR, IBindCtx *, REFIID, void **));
extern ValueWinAPI(SHCreateItemFromParsingName);

//----------------------------------------------------------------- vfs_ocd.c
extern ERRORCODE DlgCopyDir(FOPSTRUCT *FS, const TCHAR *src, TCHAR *dst, DWORD srcatr);

//----------------------------------------------------------------- vfs_ocf.c
extern ERRORCODE DlgCopyFile(FOPSTRUCT *FS, const TCHAR *src, TCHAR *dst, FILE_STAT_DATA *srcStat);
extern ERRORCODE CheckEntryMask(FOPSTRUCT *FS, const TCHAR *src, DWORD srcatr);

//----------------------------------------------------------------- vfs_odel.c
extern DWORD_PTR USECDECL FopDeleteInfoFunc(PPXAPPINFO *ppxa, DWORD cmdID, PPXAPPINFOUNION *uptr);

//----------------------------------------------------------------- vfs_odlg.c
#define FUE_DISABLE_ALL	0
#define FUE_ENABLE_PAUSE	1	// 同名関係だけ enable
#define FUE_ENABLE_SAMEFILE	2	// 同ファイル用 enable
#define FUE_ENABLE_SAMEACTION	3	// 同名関係 + 試行 enable
#define FUE_ENABLE_ALL	4
extern void FopUiEnable(FOPSTRUCT *FS, int setting);

extern void FopChangeStateAndNotify(FOPSTRUCT *FS, int state);
extern void FopMessageLoopOrPause(FOPSTRUCT *FS);
extern void USEFASTCALL FopPeekMessageLoop(void);
extern void SetWindowY(FOPSTRUCT *FS, int y);
extern void DispAttr(HWND hDlg, UINT cID, BY_HANDLE_FILE_INFORMATION *src, BY_HANDLE_FILE_INFORMATION *dst);
extern void SaveControlData(FOPSTRUCT *FS);
extern INT_PTR CALLBACK FileOperationDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
extern void SetFopTab(FOPSTRUCT *FS, int id);
extern void SetStateOnCaption(FOPSTRUCT *FS, const TCHAR *text);

//----------------------------------------------------------------- vfs_oflt.c
extern ERRORCODE LFNfilter(struct FopOption *opt, TCHAR *src, DWORD attributes);

//----------------------------------------------------------------- vfs_oimg.c
extern int OperationStartListFile(FOPSTRUCT *FS, const TCHAR *srcDIR, TCHAR *dstDIR);
extern int OperationStartToFile(FOPSTRUCT *FS, const TCHAR *srcDIR, TCHAR *dstDIR);
extern void ImgExtract(FOPSTRUCT *FS, const TCHAR *srcDIR, const TCHAR *dstDIR);

//----------------------------------------------------------------- vfs_olnk.c
#ifndef SYMBOLIC_LINK_FLAG_DIRECTORY
#define SYMBOLIC_LINK_FLAG_DIRECTORY (0x1)
#endif
TypedefWinAPI(BOOL, CreateHardLink,
		(LPCTSTR lpFileName, LPCTSTR lpExistingFileName,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes));

extern ERRORCODE FOPMakeShortCut(const TCHAR *src, TCHAR *dst, BOOL directory, BOOL test);
extern ERRORCODE FopCreateSymlink(const TCHAR *FileName, const TCHAR *ExistingFileName, DWORD flags);
extern ValueWinAPI(CreateHardLink);

//----------------------------------------------------------------- vfs_osub.c
extern ERRORCODE CopySecurity(FOPSTRUCT *FS, const TCHAR *src, const TCHAR *dst);
extern ERRORCODE BackupFile(FOPSTRUCT *FS, const TCHAR *src);
extern void SetFopLowPriority(FOPSTRUCT *FS);
extern void FopCompareFile(HWND hWnd, const TCHAR *src, const TCHAR *dest);
extern int UndoCommand(FOPSTRUCT *FS);
extern void GetUndoLogFileName(TCHAR *name);
extern ERRORCODE FileOperationReparse(FOPSTRUCT *FS, const TCHAR *src, TCHAR *dst, DWORD attr);
extern int FopOperationMsgBox(FOPSTRUCT *FS, const TCHAR *msg, const TCHAR *title, UINT flags);

typedef struct {
	PPXAPPINFO info;
	const TCHAR *filename;
} FILENAMEINFOSTRUCT;
extern DWORD_PTR USECDECL FilenameInfoFunc(FILENAMEINFOSTRUCT *linfo, DWORD cmdID, PPXAPPINFOUNION *uptr);
extern ERRORCODE FileOperationAux(FOPSTRUCT *FS, const TCHAR *id, const TCHAR *AuxPath, const TCHAR *src, const TCHAR *dst);

//---------------------------------------------------------------- vfs_shnp.cpp
#ifdef __cplusplus
extern "C" {
#endif
extern void CheckAndInitIfo(FOPSTRUCT *FS);
extern BOOL CopyFileWithIfo(FOPSTRUCT *FS, const TCHAR *src, TCHAR *dst);
extern void FreeIfo(FOPSTRUCT *FS);
#ifdef __cplusplus
}
#endif
