/*-----------------------------------------------------------------------------
	Paper Plane cUI											Combo Window 定義

	_Path の記憶情報
		現在窓ID 反対窓ID 左端ペインの１番目タブID … 最終ペインの最終タブID
		※ペイン毎にタブ一覧がある場合、境界に「-」を記載

		窓ID	一文字目大文字、二文字目以降小文字 (A,Zabc)
		タブID	窓ID [$:ロック中] [n:ペイン番号] [.n:タブグループ番号]

-----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#define Combo_Max_Base 200 // 一体化窓に登録可能な最大数(最大は TCM_FIRST (0x1300) )
#define Combo_Max_Show 32 // 一体化窓の最大ペイン数(最大は Combo_Max_Base)

#define ComboFrameID 0x4000
#define ComboCommandIDFirst 0x4001 // コントロールIDの割当て先頭

#define PPCOMBOWinClass T("PaperPlaneCombo")
#define PPCOMBOWinFrameClass T("PPxComboFrame")

#define AREAMINSIZE 4 // 表示領域を最低限確保する大きさ
#define AREAMINSAVESIZE 10 // 設定を記憶する最低限の大きさ（小さいと次回起動時に戻る）
#define BOTTOM_AREAMINSIZE 100

#define MTASKLEN 14 // _Pathに保存する情報量 [GRP:??(2)][ID:xxxx (4)][lock:$ (1)][num:0000 (4)][sep:-(1)]
// ID: Aabc
#define Panelistsize(count) ((count + 2) * MTASKLEN + 16)

/*
	BaseXXX	COMBOITEMSTRUCT に記録されているindex
	ShowXXX	表示位置
*/
typedef struct {
	HWND hWnd;
	PPC_APPINFO *cinfo;
	DWORD ActiveID; // アクティブ順を表す数値
	#define CAPTURE_NONE	0
	#define CAPTURE_WINDOW	1 // 一般window
	#define CAPTURE_WINDOWEX	2 // PPx対応のcaptured window
	int capture;
	COLORREF tabtextcolor, tabbackcolor;
} COMBOITEMSTRUCT;

typedef struct {
	HWND hWnd; // tab window
	HWND hCurWnd; // 選択された窓(複数ペイン共通タブのときに使用する)
	int height; // 表示高さ
} TABGROUP;

typedef struct {
	HWND hWnd; // tab window(グループの場合は tablist[n].hWnd のいずれかと同じ)
	HWND hTipWnd; // tab text tip window
	HWND hSelecterWnd; // tab for group selector
	TABGROUP *group;
	int groupcount; // TABLIST の登録数
	BOOL show_all; // 全てのlistを表示するか
	int height, selector_height; // 表示高さ
} COMBOTABINFO;

typedef struct {
	RECT box;	// 表示位置
	int baseNo;	// 表示中のitem
	int resizepos; // リサイズ時に使用するサイズ比率
	COMBOTABINFO tab; // !CMBS_TABEACHITEM && !CMBS_TABSEPARATE のとき
					// （タブがペイン共通）、tablist を除いて値が共通する
					// tablist は選択したタブを記憶するために使用する
} COMBOPANES;

typedef struct {
	TCHAR ID[REGEXTIDSIZE];
} CLOSEDPPC;

typedef struct tagCOMBOSTRUCT {
	HWND hWnd;	// Combo 親ウィンドウ(INVALID_HANDLE_VALUE:終了中)
	HWND hAddressWnd; // Address bar
	HWND hTreeWnd; // Common Tree
	int BaseCount, ShowCount; // 管理している数、表示／ペインの数
	int Tabs; // タブコントロールの数(タブ共用なら1、非共用ならShowCount)
	int MainPane; // メインペイン(左側ペイン)に該当するペイン

	COMBOITEMSTRUCT *base;
	COMBOPANES *show;

	WINPOS WinPos;
	SIZE WndSize;	// Combo window の大きさ

	DIRECTXDEFINE(DXDRAWSTRUCT *DxDraw)

	PPXDOCKS Docks; // 上下dock

	ComboDropTarget *pDropTarget; // Drop 処理用

	struct {
		HFONT handle;
		struct {
			int cy;
		} size;
	} Font;

	struct {	// ActiveID の最小値、最大値
	// over flow は 0.1sにつき2カウントで1200日なので、考慮しない
		int low, high;
	} Active;

	struct {
		HWND hWnd;
		int Height;
	} ToolBar; // 標準ツールバー

	struct {
		// ペインより上部の各高さ
			int TopDock;	// 上側 Dock Window
			// Combo.ToolBar.Height 標準ツールバー
			int InfoLine;	// 標準情報行
			int AddrBar;	// 標準アドレスバー
		int TopArea; // 上記の総計
		// ペイン内の要素
			int TabBar; // タブ(上)
			//	ペイン
			// TabBar タブ(下)のとき
		// ペインより下部の各高さ
			int ReportJob; // 以下の２つの総計
				// SplitPix 境界線
				// レポート(ログ)・ジョブ一覧
			int BottomDock;	// 下側Dock Window の高さ
	} Height;

	#define InfoTop (Combo.Height.TopDock + Combo.ToolBar.Height)
	#define InfoBottom (InfoTop + Combo.Height.InfoLine)
	#define AddrTop (Combo.Height.TopArea - Combo.Height.AddrBar)
	#define AddrBottom Combo.Height.TopArea

	int LeftAreaWidth; //ツリーの右端 兼 幅
	DWORD FontDPI;
	ControlFontStruct cfs; // タブ用フォント(InitComboでNULL化済み)

	struct tagPanes {
		HWND hFrameWnd; // カラム表示用ウィンドウ(!CMBS_TABFRAME のときは、Combo.hWnd と同じ値)
		RECT box; // 表示領域
		RECT clientbox; // 表示領域(CMBS_TABFRAME時は、boxより小さくなる)
		POINT delta; // 表示開始位置
		int resizewidth; // リサイズ時に使用するサイズ比率 -1:未計算、-2:未計算+SORTWIN_LAYOUTALL を行う
	} Panes;

	struct {
		HWND hWnd;
		int JobAreaWidth;
	} Joblist;

	struct {
		HWND hWnd;
		HBRUSH hBrush;
		AREA box;
		BOOL change_color;
	} Report;

	struct {
		CLOSEDPPC *list; // (ring buffer)
		int Items, LastIndex;
	} Closed; // 閉じたタブの一覧

	struct {
		int type;
		TCHAR *text;
	} TabCaption; // タブのタイトルの表示方法

	LPARAM CustTick; // K_Lcust の再帰防止用

	LPCTSTR MouseCsrName; // 現在のマウスカーソルの形状
	HANDLE hMouseCsr; // 現在のマウスカーソルハンドル

	#define DMS_PANE 0	// ペインNo.最小値
	#define DMS_NONE -1	// 該当無し
	#define DMS_TOP  -2	// 情報行等
	#define DMS_LEFT -3	// 左エリアの右
	#define DMS_REPORT -4	// report上
	#define DMS_JOB -5	// job上
	int SizingPane; // サイズ変更中(枠ドラッグ中)のペイン

	BOOL UniqueX_Combos; // X_combos が X_combom を利用

	HANDLE hMenuTheme;
} COMBOSTRUCT;

extern COMBOSTRUCT Combo;

//=============================================================================
#ifndef CGLOBALDEFINE
	#define GVAR extern
	#define GPARAM(x)
#else
	#undef GVAR
	#undef GPARAM
	#define GVAR
	#define GPARAM(x) = x
#endif

GVAR DWORD ComboTreeMode GPARAM(PPCTREE_OFF);
extern HWND hComboFocusWnd;
extern HWND hComboSubPaneFocus;

extern int ComboInit;
extern ThSTRUCT Combo_thGuiWork;	// リバー関連の情報保存に使用する

//=============================================================================
GVAR int SplitPix GPARAM(4); // ウィンドウの仕切の幅

GVAR struct { // 最大, 起動時表示ペイン数
	int limit, first;
} X_mpane
#ifdef CGLOBALDEFINE
= {Combo_Max_Base, 1};
#endif
;

// Report
GVAR COLORREF CC_log[2]
#ifdef CGLOBALDEFINE
= {C_AUTO, C_AUTO}
#endif
;

//=============================================================================
// PPCOMBO.C
extern LRESULT CALLBACK ComboProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LRESULT ComboLMouseUp(LPARAM lParam);
extern LRESULT ComboMouseMove(HWND hWnd, LPARAM lParam);

typedef struct {
	PAINTSTRUCT ps;
	HBRUSH hBr;
} DRAWCOMBOSTRUCT;
extern void DrawPaneArea(DRAWCOMBOSTRUCT *dcs);

#define SORTWIN_LAYOUTALL -1	// 窓内の配置が変更
#define SORTWIN_LAYOUTPAIN -2	// ペインの並びや割合が変更
#define SORTWIN_RESIZE -3	// 窓の大きさが変更
#define SORTWIN_FIXINNER -4	// SortWindows内での再帰調整用
#define SORTWIN_FIX_NORESIZE B28
#define SORTWIN_NEWPANE      B27
extern void SortComboWindows(int fixshowindex);

extern void KCW_FocusFix(HWND hWnd, HWND hTargetWnd);

// PPCOMBOS.C
extern void ResetSubFocus(TCHAR *buf);
extern void CheckComboTable(const TCHAR *p1);
extern void WmComboDestroy(BOOL EndSession);
extern ERRORCODE PaneCommand(const TCHAR *param, int targetbaseindex);
extern void SetComboAddresBar(const TCHAR *path);
extern void EnterAddressBar(void);
extern void SetComboReportText(const TCHAR *text);
extern void ComboCust(BOOL reload);
extern void CloseLeftArea(void);
extern void CreateLeftArea(DWORD mode, const TCHAR *initpath, BOOL sync);
extern void CreateJobArea(void);
extern void InitComboGUI(BOOL reload);
extern HWND CreateComboTabBar(const TCHAR *groupname);
#define NewTabGroup_First -1 // 新規ペイン+グループ作成のとき
extern ERRORCODE NewTabGroup(int targetpane, const TCHAR *groupname);
extern const TCHAR * NewTabGroupName(TCHAR *name);
extern void CreateTabGroupSelector(COMBOTABINFO *tabs);
extern void LoadX_Combos(void);
extern void SaveX_Combos(void);

// PPCOMBOB.C	ツールバー
extern void ComboCreateToolBar(HWND hWnd);
extern void ComboToolbarCommand(int id, int orcode);

// PPCOMBOP.C	タブバー & ペイン等管理
// タブ関連
#define CREATETAB_FIRST 0 // 起動時
#define CREATETAB_APPEND 1 // 実行時
extern void CreateTabBar(int mode, const TCHAR *groupname, BOOL NoTabAdd);
extern void SetTabInfoData(int setinfo, int showindex, HWND hItemWnd);
#define SETTABINFO_SET 0
#define SETTABINFO_ADD 1
#define SETTABINFO_ADDENTRY 2
#define SetTabInfo(showindex, hItemWnd) SetTabInfoData(SETTABINFO_SET, showindex, hItemWnd)
#define AddTabInfo(showindex, hItemWnd) SetTabInfoData(SETTABINFO_ADD, showindex, hItemWnd)

extern void SetTabColor(int baseindex);
extern BOOL TabMenu(HWND hWnd, int baseindex, int targetpane, POINT *pos);
extern int GetTabItemIndex(HWND hWnd, int tabwndindex);

extern void SelectChangeTab(NMHDR *nmh);
extern BOOL SetTabTipText(NMHDR *nmh);
extern void SelectHidePane(int showindex, HWND hWnd);
extern void ComboSwap(void);
extern void CreatePane(int baseindex);
extern void CreateAndInitPane(int baseindex);
extern void EjectPane(int baseindex);
extern void HidePane(int showindex);

extern void DestroyedPaneWindow(HWND hWnd, int baseindex);
extern void SelectComboWindow(int showindex, HWND hTargetWnd, BOOL focus);
extern void SelectTabByWindow(HWND hTargetWnd, int selectpane);

extern int GetComboBaseIndex(HWND hWnd);
extern int GetComboShowIndex(HWND hBaseWnd);
extern int GetComboShowIndexDefault(HWND hBaseWnd);
extern int GetComboShowIndexAll(HWND hBaseWnd);
extern int GetComboShowIndexFromPos(POINT *pos);
extern int GetPairPaneComboShowIndex(HWND hTargetWnd);
extern int GetPairPaneComboBaseIndex(HWND hTargetWnd);
extern HWND GetTabWndForGetPane(HWND hTagetWnd, int showindex);
extern void RenameGroupNameMain(COMBOTABINFO *tabs, HWND hTabWnd, TCHAR *name);
extern ERRORCODE SelectGroup(int showindex, int groupindex);

extern int SearchTabParam(HWND hTabWnd, LPARAM param);

extern void SwapPane(int targetpane);
extern void NewPane(int baseindex, const TCHAR *param);
extern void ClosePanes(HWND hTabWnd, int baseindex, int mode, BOOL closelocked);
extern void CreateNewTabParam(int showindex, int newbaseindex, const TCHAR *param);
extern HWND GetHwndFromIDCombo(const TCHAR *regid);
extern void DrawTab(DRAWITEMSTRUCT *dis);
extern void Tab_MovePosition(HWND hSrcTabWnd, int SrcTabIndex, int DestShowIndex, int DestTabIndex, int DestGroupIndex);
extern ERRORCODE NewTabGroupMain(int targetpane, const TCHAR *groupname);
extern void EraseTabBkgnd(HWND hWnd, WPARAM wParam);


typedef struct {
	HWND hWnd;		// 自分自身の hWnd
	MOUSESTATE ms;
	WNDPROC hOldProc;	// 親のウィンドウプロージャ
	struct {
#define THS_SHOW_NODRAG -2
#define THS_SHOW_CANCELDRAG -1
		int showindex;
		int tabindex, width;
	} DDinfo;
} TABHOOKSTRUCT;

extern int PPxDownMouseButtonX(TABHOOKSTRUCT *THS, WPARAM wParam, LPARAM lParam);
extern VOID CALLBACK HideCloseButtonProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

#define SIZEBAND 5 // 幅調整の認識幅

// for debug state
#define NestedMsgs 16
extern int ComboProcNested;
extern UINT ComboProcMsg[NestedMsgs];
GVAR int LastTabInsType GPARAM(0);
GVAR int UseTabInsType GPARAM(0);
extern const TCHAR *ChangeReason;
GVAR BOOL UseCapture GPARAM(FALSE);

#ifdef __cplusplus
}
#endif
