/*-----------------------------------------------------------------------------
	Paper Plane bUI
-----------------------------------------------------------------------------*/
#define WNDTITLESIZE 0x80
extern HANDLE hCommSendEvent;	// 外部からのコマンド受け付け用イベント
extern HANDLE hCommIdleEvent;	// 外部からのコマンド受け付け用イベント
extern HWND hBackWnd;		// 最小化時に戻る Window
extern WINPOS WinPos;
extern TCHAR RegID[REGIDSIZE];
extern TCHAR RegCID[REGIDSIZE - 1];
extern TCHAR PPxPath[VFPS];	// PPx 本体のあるパス
extern TCHAR EditPath[VFPS];	// PPb command line のパス
extern TCHAR *CurrentPath;		// 現在の実行パス(編集用/外部編集用を切り替え)
extern TCHAR ExtPathBuf[VFPS];
extern TCHAR *ExtPath;	// 外部実行時に使用するパス
extern TCHAR WndTitle[WNDTITLESIZE];
extern PPXAPPINFO ppbappinfo;
extern int RegNo;			// PPx Register ID
extern ThSTRUCT LongParam;	// 長いコマンドライン用
extern ThSTRUCT ResultText;	// %*extract用

extern TCHAR *EditText;		// 表示内容
extern DWORD ExitCode;
extern const TCHAR NilStr[1];

//------------------------------------- PPB_SUB.C
extern void PPbExecuteRecv(TCHAR *param);
extern int PPbExecuteInput(TCHAR *param, size_t size);
extern DWORD_PTR USECDECL PPbInfoFunc(PPXAPPINFO *ppb, DWORD cmdID, PPXAPPINFOUNION *uptr);
extern DWORD_PTR USECDECL PPbExecInfoFunc(PPXAPPINFO *ppb, DWORD cmdID, PPXAPPINFOUNION *uptr);
extern void ReleasePPB(void);
extern void TitleDisp(const TCHAR *addmes);

//------------------------------------- TCINPUT.C
typedef enum {
	CMODE_EDIT = 0, CMODE_LIST, CMODE_WALKDIR, CMODE_WALKFILE, CMODE_LOG_VIEW, CMODE_LOG_SELECT, CMODE_MAX
} CMODEINFO;

typedef struct {
	WORD type;		// 検索するヒストリの種類
	int index;		// 参照している古さ。-1:編集中
	int count;		// 検索中のパラメータ 0:検索していない
		#define HISCOUNT_MAX 9999
		#define HIST_CMD_ALL 0
		#define HIST_CMD_FIND 1
		#define HIST_PARAM_ALL 2
		#define HIST_PARAM_FIND 3
	int mode;		// (前回の)検索方法 0:全体 1:コマンド前方一致
						//                2:パラメータ 3:パラメータ前方一致
	TCHAR *findstr;	// 検索文字列の開始位置
	TCHAR *findbuflast; // 保存時の最大位置
	DWORD findsize;	// 検索文字列の長さ
} HISTVAR;

#define listmode_off 0
#define listmode_entry 1
#define listmode_history 2
#define listmode_log 3

typedef struct {
	ThSTRUCT Index;
	ThSTRUCT Data;
} ThList;

typedef struct {
	ThList list;
	int mode;
	BOOL show;
	int item_min, item_index;
	int show_min_Y;
	ECURSOR cursor;
} LINEINFO;

#define TCI_TI_DRAW B0 // 再描画
// 以降は特殊
#define TCI_TI_LIST_CHANGE_TARGET B1 // List の対象が変化した
#define TCI_TI_LIST_SEL_DRAW B2 // List の選択カーソルを描画
#define TCI_TI_LIST_DRAW B3 // List を 描画
#define TCI_TI_LISTFLAGS (TCI_TI_LIST_SEL_DRAW | TCI_TI_LIST_DRAW)

#define TCI_TI_SCROLL B4
#define TCI_TI_FIRST B5
#define TCI_TI_EXTRA (TCI_TI_SCROLL | TCI_TI_FIRST)

typedef struct {
	size_t maxsize; // 最大入力可能文字数
	WORD histype;

	int useattr;
	CMODEINFO cmode;	// 動作モード CMODE_
	ECURSOR sel; // 選択範囲

	struct tagSCROLLMODEINFO {
		int cx, cy;	// スクロールモード時のカーソル座標
		int sx, sy;	// スクロールモード時の選択座標
		int BackupY;	// スクロールモード開始時の編集行
		int LineMode;	// FALSE:ブロック選択 TRUE:行選択
	} sinfo;

	#define INS_INS		50
	#define INS_OVER	100
	int insmode;	// カーソルの高さ+挿入モード INS_INS / INS_OVER

	int ShowOffset;		// 相対表示開始位置
	int EdX;		// 編集位置

	int mouseSX;	// マウスで範囲選択を開始したX座標

	// Replace の挙動指定
	#define IRT_RIGHT 0 // 書き込んだ文字列の右にカーソル
	#define IRT_STOP 1  // カーソル移動なし
	#define IRT_BRAKET 2 // 右端の一つ左（括り付き用）
	int istrt;

	TCHAR *baseptr;
	TCHAR backuptext[CMDLINESIZE];	// バックアップ
	BYTE attribute[CMDLINESIZE];
	int eflag;		// 正:各種フラグ	負:TCI_QUIT など

	LINEINFO linelist;
	HISTVAR hvar;
	ESTRUCT ComplED;
	struct tagIncSearchInfo {
		TCHAR text[64]; // インクリメンタルサーチテキスト
		int len;	// searchtext の長さ
		COORD pos;	// 最後に検索した位置
	} incsearch;

#define default_list_height 14
	int list_height;

	DWORD RepeatTick;
} TCINPUTSTRUCT;

extern TCINPUTSTRUCT tis;
extern const TCHAR CmodeStr[CMODE_MAX];

#define TCI_RECV	-3	// コマンド受信
#define TCI_EXECUTE	-2	// 実行
#define TCI_QUIT	-1	// 終了
extern int tCInput(TCHAR *buf, size_t bsize, WORD htype);
extern void ResettCInput(TCINPUTSTRUCT *tis, TCHAR *buf, BOOL first);
extern void GetWordText(TCINPUTSTRUCT *tis, TCHAR *dst, DWORD flags);
extern void ShowImageToConsole(const TCHAR *param);
extern void SelectTextCommand(TCINPUTSTRUCT *tis, int id);
extern void MoveCursor(TCINPUTSTRUCT *tis, int newX, int key);
extern void TconCommonCommand(TCINPUTSTRUCT *tis, int key);
extern void PPbF1Menu(TCINPUTSTRUCT *tis);

#define REPLACE_ALL	B0
#define REPLACE_SELECT	B1
#define REPLACE_FROMLIST	B2
extern void Replace(TCINPUTSTRUCT *tis, const TCHAR *istr, int flags);
extern void CommonCommand(int key);
