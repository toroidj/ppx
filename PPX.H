//-----------------------------------------------------------------------------
//	Paper Plane xUI		共通ヘッダ
//-----------------------------------------------------------------------------
#if !defined(__BORLANDC__) && !defined(WINEGCC)
#include <process.h>
#endif
#include "CHRTYPE.H"
#include "TOROWIN.H"
#include "TKEY.H"
#include "PPXID.H"
#include "PPXMES.H"
#include "PPX.RH"
#include "PPCOMMON.H"
#include "BMP.H"
#include "tstrings.h"

#ifdef RELEASE
#undef Message
#undef MessageA
#undef MessageW
#undef Messagef
#endif

#define PPxSettingsRegPath "Software\\TOROid\\PPx"
#define PPxSettingsAppdataPath "TOROID\\PPx"
#define PPxProgramDataPath "PPx"

#define SENDCOMBO_OK	106	// 正常に通信が成功したときの値

// GetMessageExtraInfo で取得できるポイントメッセージの種類
#define POINTTYPE_SIG_MASK 0xFFFFFF00
#define POINTTYPE_TOUCH_MASK 0xFFFFFF80
#define POINTTYPE_TOUCH 0xFF515700
#define POINTTYPE_TOUCH_TOUCH 0xFF515780
#define POINTTYPE_TOUCH_PEN 0xFF515700
//      POINTTYPE_TOUCH_MOUSE				// 0
// 値の例(windows10 2016/08)
// FF515702(touch screen pen top/side)
// FF515704(BAMBOO pen top)
// FF515705(BAMBOO pen bottom)
// FF515780-FF51578F(touch、下位4bitは、タッチ毎に増加)

#define ESC_B	'\x1b'
#define ESC_T	T("\x1b")
#define ESC_A	"\x1b"
#define CSI_T	T("\x1b[")
#define CSI_A	"\x1b["
#define CSI_W	L"\x1b["

// 各種設定 ===================================================================
#define USETEMPCONFIGFILE 0 // AppData に設定を作成しないようにするには 1を指定

#define WINHELPFILE "PPX.HLP"
#define HTMLHELP_TEXT  "ppxhelp.html"
#define HTMLHELP_INDEX "ppxindex.html"
#define HTMLHELP_WORDS "ppxwords.html"
#define HTMLHELP_WEBPAGE "ppxhlp.html"
#define PPxWebPage "/slppx.html"

#if !defined(USEDIRECTX) && !defined(USEDIRECTWRITE)
  #define DRAWTYPESTRING
#else
  #ifdef USEDIRECTWRITE
	#define DRAWTYPESTRING T("DirectWrite,")
  #else
	#define DRAWTYPESTRING T("Direct9,")
  #endif
#endif

#ifdef WINEGCC
	#define USEWINESTRING T("Wine,")
#else
	#define USEWINESTRING
#endif

// DirectWrite,x64,UNICODE
#define RUNENVSTRINGS USEWINESTRING DRAWTYPESTRING T(BITSTRING) T(",") T(CODETYPESTRING)

#define SKIPBUTTONMENUTIME 500 // ボタンメニューを２度押ししたときの無視時間
#define WAIT_NO_DIALOG_TIME 3000	// 待機時に、ダイアログの表示を待つ時間
#define WAIT_DIALOG_INTERVAL 100	// 待機時にダイアログの表示を確認する間隔

#define MESTEXTIDLEN 4
#define MES_IDD_FOP	T("7020|")

// SetPopMsg
#define POPMSG_MSG	0
#define POPMSG_NOLOGFLAG	B29
#define POPMSG_NOLOGMSG		(POPMSG_NOLOGFLAG+0)
#define POPMSG_PROGRESSMSG	(POPMSG_NOLOGFLAG+1)	// 進捗状況
#define POPMSG_PROGRESSBUSYMSG	(POPMSG_NOLOGFLAG+2) // 進捗状況+完了待ち中
#define POPMSG_KEEPMSG	(POPMSG_NOLOGFLAG+3) // 常時表示用文字列を指定
#define POPMSG_GETLASTERROR	(POPMSG_NOLOGFLAG-1)	// エラーコード表示

// メモリ使用制限関連 =========================================================
#ifndef _WIN64 // 32bit版の設定
	// ファイルイメージのデフォルト読み込み限界、X_wsiz で変更可
#define IMAGESIZELIMIT (1024*1024*20)

	// PPcのスレッド間共用のアイコンのキャッシュ数。まだバグ有りなので200程度にして直すこと
#define ICONCACHELISTINDEXMAX 0x7ffff0
	// PPcの窓内のアイコンのキャッシュ数。
	// 32dotICON 7850個分(1600*1200*3画面 or 4000*2000*1画面 を埋め尽くし可能)
#define PPCWINDOWICONCACHEMAX (7850 * 32 * 32)

	// iconを先読みする量。表示画面＋先読みページの数
#define READCACHEPAGES 2


#else // 64bit版の設定
#define IMAGESIZELIMIT (1024*1024*50)
#define ICONCACHELISTINDEXMAX 0x7fffff0
#define PPCWINDOWICONCACHEMAX (7850 * 32 * 32 * 4)
#define READCACHEPAGES 3
#endif

#define LOGLINES_MAX 3000		// 共用ログの最大行数
#define LOGLINES_DELETE 1000	// 削除するときに残す行数
#define DEF_X_svsz (10000000)	// 10M ※lowsp なら 4000000
// ============================================================================
#if 0
#define DEBUGLOGF(message, param) XMessage(NULL, NULL, XM_DbgLOG, T(message), param)
#else
#define DEBUGLOGF(message, param)
#endif

#if defined(_MSC_VER) && !defined(RELEASE)
	#define C4701CHECK = 0 // C4701(未初期化) の検討済み
#endif
#ifndef C4701CHECK
	#define C4701CHECK
#endif

#define PAIRBIT B0 // PPc等のペアID算出に使う

// Key Alias/メニュー登録用
#define K_M  K_internal			// メニューのみ
#define K_Mv (K_internal | K_v)	// VK
#define K_Me (K_internal | K_e)	// 拡張SHIFT
#define K_Ms (K_internal | K_s)	// SHIFT
#define K_Mc (K_internal | K_c)	// CTRL
#define K_Ma (K_internal | K_a)	// ALT/GRPH

// X_dlf 用定数
#define XDLF_ROOTJUMP	B0
#define XDLF_DISPALIAS	B1
#define XDLF_DISPDRIVETITLE	B2
#define XDLF_NODRIVES	B3
#define XDLF_NODISPSHARE	B4
#define XDLF_NOM_PJUMP	B5
#define XDLF_UPDIRJUMP	B6
#define XDLF_USEMDRIVES	B7 // XDLF_NOOVERLAP と重複
#define XDLF_NOOVERLAP	B7
#define XDLF_DISPFREE	B8
#define XDLF_ADMINDRIVE	B9

typedef struct {
	RECT pos;		//MoveWindow
	BYTE show;		//ShowWindow
	BYTE reserved;
} WINPOS;

#define HiddenMenu_MinLen 2
#define HiddenMenu_GetNameLen(base) (*(WORD *)(char *)((char *)base - sizeof(WORD)))
// #define HiddenMenu_fg(base) *(COLORREF *)(base + TSTROFF(HiddenMenu_GetNameLen(base) + 1))
// #define HiddenMenu_bg(base) *(COLORREF *)(base + TSTROFF(HiddenMenu_GetNameLen(base) + 1) + sizeof(COLORREF))
#define HiddenMenu_cmd(base) (TCHAR *)(base + TSTROFF(HiddenMenu_GetNameLen(base) + 1) + sizeof(COLORREF) * 2)

#define CURSOR_PARAM_ITEMS 7


#pragma pack(push, 1)
typedef struct {
	WORD offset;
	TCHAR name[HiddenMenu_MinLen]; // 実際は可変長
//	COLORREF fg;
//	COLORREF bg;
} HIDDENMENUSTRUCT;
#pragma pack(pop)

// タイトルバー無しにするときに制御する必要があるスタイル
#define WS_NOTITLEOVERLAPPED (WS_POPUP | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)

// %F / %*name のオプション
//(出力形式)
#define FMOPT_FILENAME	B0	// C ファイル名+拡張子
#define FMOPT_DIR		B1	// D ディレクトリ
#define FMOPT_DRIVE		B2	// H ドライブ名		(%*nameのみ)
#define FMOPT_FILENOEXT	B3	// X ファイル名のみ
#define FMOPT_FILEEXT	B4	// T 拡張子			(%*nameのみ)
//(「"」の処理)
#define FMOPT_BLANKET	B5	// B ""で必ず括る
#define FMOPT_NOBLANKET	B6	// N ""は必ず無い
//(加工種類)
#define FMOPT_ENABLEVFS	B7	// V PPx独自指定を有効にする
#define FMOPT_REALPATH	B8	// R パスを実体にする(%*nameのみ)
#define FMOPT_USESFN	B9	// S SFNを使用する	(%*nameのみ)
#define FMOPT_UNIQUE	B10	// U 既存と重複しないように加工する (%*nameのみ)
#define FMOPT_MARK		B11	// M マークファイルのみ (%F, PPcのみ)
#define FMOPT_LASTSEPARATOR	B12	// P 末尾に \ を付加する (%*nameのみ)
#define FMOPT_SEP_SLASH	B13		// L パス区切りを「/」にする (%*nameのみ)
#define FMOPT_SEP_BACKSLASH	B14	// K パス区切りを「\」にする (%*nameのみ)
//#define FMOPT_CURSOR	B12	// O カーソルファイルのみ まだ用意していない
//#define FMOPT_IDL		B14	// I IDL表記( ::{...}::{...} ) まだ用意していない

#define Default_X_ltab {0, 2}

// X_pmc の第２項目以降の設定、X_pmcは負論理、touchmodeは正論理
#define TOUCH_LARGEWIDTH		B0	// T   タッチ領域の幅を拡大する
#define TOUCH_LARGEHEIGHT		B1	// T   タッチ領域の高を拡大する
#define TOUCH_DISABLEDRAG		B2	// T   ドラッグ開始を禁止する
#define TOUCH_DISABLEHIDDENMENU	B3	// T   隠しメニューを無効に
#define TOUCH_DBLCLICKTOSINGLE	B4	// T   ダブルクリックをシングルタッチで行う
#define TOUCH_AUTOPAIRDIST		B5	// T   最大化２窓の並びを縦／横自動変更
#define TOUCH_ACTIONWITHACTIVE	B6	// T   アクティブ化時は、タッチ操作を無効に
#define TOUCH_EDIT_COMMAND_BAR	B7	// TMP 一行編集にコマンドバーを表示
#define TOUCH_DISABLE_DRAG_SELECT B8 // TMP ドラッグにより範囲選択をする
// PPc 内部用
#define TOUCH_DETECT_PEN		B28	// ペン操作有り
#define TOUCH_DETECT_TOUCH		B29	// タッチ操作有り
#define TOUCH_TABLET_MODE		B30	// タブレットモード有効中

#define X_tousItems 4
#define X_tous_defvalue GUI_TEXT_MINSIZE, GUI_BUTTON_MINSIZE, GUI_BAR_BUTTON_MINSIZE, GUI_TOUCH_MINSIZE
#define X_tous_defvalueC -1, GUI_BUTTON_MINSIZE, GUI_BAR_BUTTON_MINSIZE, GUI_TOUCH_MINSIZE // 未カスタマイズ検出可能
#define tous_TEXTSIZE 0
#define tous_BUTTONSIZE 1
#define tous_BARSIZE 2
#define tous_ITEMSIZE 3

#define X_pmc_defvalue -8, 0, (B0|B1|B2|B3|B4|B5|B6|B10|B11), 0 // 負論理注意
// ※ mouse の B10, B11 は将来マウスでは使わない機能向けに確保
#define pmc_mode 0
#define pmc_touch 1
#define pmc_mouse 2
#define pmc_pen 3

// SetPopMsg/StopPopMsg
#define PMF_WAITTIMER	B0	// 時間待ちの間表示する
#define PMF_WAITKEY		B1	// キー入力待ちの間表示する
#define PMF_PROGRESS	B2	// 進捗状況表示(指示が来るまで表示を維持する)
#define PMF_FLASH		B3	// タイトルバーのフラッシュ有り
#define PMF_KEEP		B4	// 常時表示用文字列がある。PMF_STOP では消せない
#define PMF_BUSY		B5	// 待機モーションをつける, PPc DX版のみ
#define PMF_DOCMSG		B8	// PPv専用、表示ファイルのみに有効な内容。次のファイルを開くときに消去する
#define PMF_STOP (PMF_WAITTIMER | PMF_WAITKEY | PMF_PROGRESS | PMF_FLASH | PMF_BUSY)
// ステータス行に表示する必要がある指定
#define PMF_DISPLAYMASK (PMF_WAITTIMER | PMF_WAITKEY | PMF_PROGRESS | PMF_KEEP)

#pragma pack(push, 4)
typedef struct {
	int x, y, width, height;
} AREA;
#pragma pack(pop)

#define BADHWND ((HWND)(DWORD_PTR)0xfefffffe)
#define FILEATTRMASK_FILES 0xf
#define FILEATTRMASK_DIR_FILES 0x1f

#define EXCEPTION_PPX_DUMPSTACK 0x6106106f // デバッグ用にスタックダンプをする
#define PpxDumpstack() RaiseException(EXCEPTION_PPX_DUMPSTACK, 0, 0, NULL)

#define X_tray_Combo (B0 | B1 | B4)
#define X_tray_SinglePPc (B0 | B1 | B5)
#define X_tray_PPv (B0 | B1 | B6)

// F_mes, F_dlg, F_fix に使用するフォント
#define Default_F_mes { \
/*Width, Height*/					0, 0, \
/*Escapement, Orientation, Weight*/	0, 0, FW_NORMAL, \
/*Italic, Underline, StrikeOut*/	FALSE, FALSE, FALSE, \
/*CharSet*/							SHIFTJIS_CHARSET, \
/*OutPrecision*/					OUT_DEFAULT_PRECIS, \
/*ClipPrecision*/					CLIP_DEFAULT_PRECIS, \
/*Quality*/							DEFAULT_QUALITY, \
/*PitchAndFamily*/					FIXED_PITCH | FF_DONTCARE, \
/*FaceName*/						T("Courier New")}

#define Default_F_mes_JA T("ＭＳ ゴシック")

// F_unfix に使用するフォント
#define Default_F_unfix { \
/*Width, Height*/					0, 0, \
/*Escapement, Orientation, Weight*/	0, 0, FW_NORMAL, \
/*Italic, Underline, StrikeOut*/	FALSE, FALSE, FALSE, \
/*CharSet*/							SHIFTJIS_CHARSET, \
/*OutPrecision*/					OUT_DEFAULT_PRECIS, \
/*ClipPrecision*/					CLIP_DEFAULT_PRECIS, \
/*Quality*/							DEFAULT_QUALITY, \
/*PitchAndFamily*/					FF_DONTCARE, \
/*FaceName*/						T("MS Shell Dlg")}

#define Default_F_unfix_JA T("ＭＳ Ｐゴシック")

#define PPX_FONT_MIN_PIXEL 10 // 96dpi時の最小フォントサイズ
#define PPX_FONT_MIN_PT 8 // 96dpi時の最小フォントポイント(ダイアログ用)
#define DEFAULT_WIN_DPI 96
#define DEFAULT_DTP_DPI 72

#define ENUMOBJECTSFORFOLDERFLAG_VISTA (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN | SHCONTF_FASTITEMS)
#define ENUMOBJECTSFORFOLDERFLAG_XP    (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN)

#define ShortcutExt ".lnk" // Win2000/98まで:.LNK WinXP以降:.lnk

#define PPC_HIGHLIGHT_COLORS 7
#define PPV_HIGHLIGHT_COLORS 8

// Richedit
#ifndef EM_SETEVENTMASK
  #define EM_SETEVENTMASK (WM_USER + 69)
  #define EM_GETEVENTMASK (WM_USER + 59)
  #define ENM_CHANGE 1
#endif
#define EnableTextChangeNotify(hWnd) SendMessage(hWnd, EM_SETEVENTMASK, 0, SendMessage(hWnd, EM_GETEVENTMASK, 0, 0) | ENM_CHANGE);
#define DisableTextChangeNotify(hWnd) SendMessage(hWnd, EM_SETEVENTMASK, 0, SendMessage(hWnd, EM_GETEVENTMASK, 0, 0) & ~ENM_CHANGE);
#define EnableTextChangeNotifyItem(hWnd, ID) SendDlgItemMessage(hWnd, ID, EM_SETEVENTMASK, 0, SendDlgItemMessage(hWnd, ID, EM_GETEVENTMASK, 0, 0) | ENM_CHANGE);
#define DisableTextChangeNotifyItem(hWnd, ID) SendDlgItemMessage(hWnd, ID, EM_SETEVENTMASK, 0, SendDlgItemMessage(hWnd, ID, EM_GETEVENTMASK, 0, 0) & ~ENM_CHANGE);

// X_uxt_color
#define UXT_NA  -4	// 設定未取得
#define UXT_OFF -3	// 動作無し
#define UXT_GDI -2	// デザインのみ変更
#define UXT_AUTO -1	// レジストリにあわせる
#define UXT_DARK 0	// dark mode
#define UXT_LIGHT 1	// light mode

// X_uxt_rich, X_uxt_ppe_rich
#define UXTRICH_NO 0
#define UXTRICH_V3 1
#define UXTRICH_V4 2
#define UXTRICH_DW 3

#define UXT_MINMODIFY UXT_GDI
#define UXT_MINPRESET UXT_DARK
#define X_uxt_items 4
#define X_uxt_defvalue UXT_NA, 0, 0, 0
#define X_uxt_color X_uxt[0] // UXT_
#define X_uxt_window_corner X_uxt[1]
#define X_uxt_rich X_uxt[2] // UXTRICH_
#define X_uxt_ppe_rich X_uxt[3] // UXTRICH_

#define COLOR_DARK_FRAME_HIGH	0xFF2B2B2B
#define COLOR_DARK_FRAME_FACE	0x404040
#define COLOR_DARK_FRAME_SHADOW	0xFF191919
#define COLOR_DARK_EDGE_LINE	0x404040

#define COLOR_DARK_TEXT	0xd0d0d0
#define COLOR_DARK_BACK	0x202020

#define COLOR_DARK_DIALOGBACK	0x282828
#define COLOR_DARK_GRAYTEXT	0x606060
#define COLOR_DARK_FOCUSBACK	0x404040


#define COLOR_LIGHT_FRAME_HIGH	0xf0f0f0
#define COLOR_LIGHT_FRAME_FACE	0xe0e0e0
#define COLOR_LIGHT_FRAME_SHADOW	0xd0d0d0
#define COLOR_LIGHT_EDGE_LINE	0xa0a0a0

#define COLOR_LIGHT_TEXT	0x101010
#define COLOR_LIGHT_BACK	0xfcfcfc

#define COLOR_LIGHT_DIALOGBACK	0xf8f8f8
#define COLOR_LIGHT_GRAYTEXT	0x606060
#define COLOR_LIGHT_FOCUSBACK	0xffffff


typedef struct {
	COLORREF text, back;
} EDITCOLORS;

#define DEFAULT_EDITCOLORS \
	{C_S_BR_YELLOW, C_S_AUTO - 1},\
	{C_S_YELLOW, C_S_BK_CYAN},\
	{C_S_BR_BLUE, C_AUTO},\
	{C_S_BLUE, C_AUTO},\
	{C_S_BR_BLUE, C_AUTO},\
	{C_S_BR_GREEN, C_AUTO},\
	{C_S_YELLOW, C_AUTO},\
	{C_S_AUTO, C_S_BK_RED},\
	{C_S_YELLOW, C_AUTO},\
	{C_S_BR_RED, C_AUTO},\
	{C_S_CYAN, C_AUTO},\
	{C_S_BR_CYAN, C_AUTO},\
	{C_S_BR_YELLOW, C_AUTO},\
	{C_S_BR_BLUE, C_AUTO},\
	{C_S_BR_GREEN, C_AUTO},\
	{C_AUTO, C_S_BR_YELLOW}

//------------------------------------- *file 関連
#pragma pack(push, 1)
typedef struct {			// カスタマイズ内容(X_fopt)
							// Same Name Action -------------------------------
	int		sameSW;			// (ALL&W)同名ファイルの対処方法が設定済みなら !0
	int		same;			// 同名ファイルの対処方法 FOPSAME_ 関連
							// Name Filter ------------------------------------
	int		chrcase;		// 1:大文字化 2:小文字化
	int		sfn;			// 1:8.3
	int		delspc;			// 1:空白・不可文字削除 2:不可文字のみ削除
	DWORD	filter;			// VFSFOP_FILTER_
							// Attributes -------------------------------------
	DWORD	AtrMask;		// And 指定用
	DWORD	AtrFlag;		// Or 指定用
							// Etc --------------------------------------------
	int		mode;			// FOPMODE_ 関連
	DWORD	flags;			// VFSFOP_OPTFLAG_ 関連
	char	aall;			// !0:auto all check
	BYTE	firstsheet;		// 始めに開くシート FOPTAB_
	char	useburst;		// バーストモード
	BYTE	divide_unit;	// 分割大きさの単位
	DWORD	divide_num;		// 分割大きさ
	TCHAR	str[VFPS];		// ASCIIZs  1st:Dest 2nd:Rename 3rt:S.num
} VFSFOP_OPTIONS;
#pragma pack(pop)

enum { // same の内容
	FOPSAME_NEWDATE = 0,
	FOPSAME_RENAME,
	FOPSAME_OVERWRITE,
	FOPSAME_SKIP,
	FOPSAME_ARCHIVE,
	FOPSAME_ADDNUMBER,
	FOPSAME_APPEND,
	FOPSAME_SIZE,
	FOPSAME_MAX
};
enum { // mode の内容
	FOPMODE_MOVE = 0,
	FOPMODE_COPY,
	FOPMODE_MIRROR,
	FOPMODE_SHORTCUT,
	FOPMODE_LINK,
	FOPMODE_DELETE,
	FOPMODE_UNDO,
	FOPMODE_SYMLINK,
	FOPMODE_MAX
};
#define FOPMODE_ISCOPYMOVE(FOPMODE) (FOPMODE <= FOPMODE_MIRROR)

// flags の内容
#define VFSFOP_OPTFLAG_SKIPERROR	B0	// 軽微エラーの無視
#define VFSFOP_OPTFLAG_FLATMODE		B1	// 平坦化
#define VFSFOP_OPTFLAG_AUTOROFF		B2	// ReadOnlyの自動解除
#define VFSFOP_OPTFLAG_BACKUPOLD	B3	// 削除・上書きファイルの保存

#define VFSFOP_OPTFLAG_UNDOLOG		B4	// Undo用ログを出力
#define VFSFOP_OPTFLAG_LOGWINDOW	B5	// ログウィンドウに経過表示
#define VFSFOP_OPTFLAG_LOGRWAIT		B6	// ログ表示したときに結果表示待機しない
#define VFSFOP_OPTFLAG_QSTART		B7	// 順序待ちしない

#define VFSFOP_OPTFLAG_NOCOUNT		B8	// カウントしない
#define VFSFOP_OPTFLAG_SECURITY		B9	// セキュリティコピー
#define VFSFOP_OPTFLAG_SACL			B10	// 監査コピー
#define VFSFOP_OPTFLAG_LOWPRI		B11	// 優先順位を低くする

#define VFSFOP_OPTFLAG_NOWCREATEDIR	B12	// ディレクトリ作成警告無し
#define VFSFOP_OPTFLAG_PREVENTSLEEP	B13	// スリープ抑制
#define VFSFOP_OPTFLAG_ALLOWDECRYPT	B14	// 暗号化非対応ドライブへの暗号化ファイルのコピーを許可
#define VFSFOP_OPTFLAG_NOWDELEXIST	B15	// サブディレクトリ内の存在警告なし

#define VFSFOP_OPTFLAG_KEEPDIR		B16	// 作成元の階層を再現(srcがListFileの時)
#define VFSFOP_OPTFLAG_JOINBATCH	B17	// 分割時に元に戻す為のバッチを生成
#define VFSFOP_OPTFLAG_ADDNUMDEST	B18	// 上書きファイル元の名前変更
#define VFSFOP_OPTFLAG_NOFIRSTEXIST	B19	// 開始時に処理先存在チェックをしない

#define VFSFOP_OPTFLAG_SYMCOPY_SHIFT	20
#define VFSFOP_OPTFLAG_SYMCOPY_SYM	B20	// シンボリックリンクのリンク コピー
#define VFSFOP_OPTFLAG_SYMCOPY_FILE	B21	// シンボリックリンクのファイル コピー
#define VFSFOP_OPTFLAG_SYMDEL_SHIFT	22
#define VFSFOP_OPTFLAG_SYMDEL_SYM	B22	// シンボリックリンクを解除
#define VFSFOP_OPTFLAG_SYMDEL_FILE	B23	// シンボリックリンク内ファイルを削除

#define VFSFOP_OPTFLAG_WAIT_CLOSE	B24	// 移動時使用終了を待つ
#define VFSFOP_OPTFLAG_IFILEOP		B25	// IFileOperation を使用する
#define VFSFOP_OPTFLAG_NOKEEPTIME_SRC	B26	// 処理元のアクセス日時を保護しない
#define VFSFOP_OPTFLAG_NOKEEPTIME_DEST	B27	// 処理先のアクセス日時を保護しない
#define VFSFOP_OPTFLAG_THERMAL_THROT	B28	// ストレージ高温時は一時停止
#define VFSFOP_OPTFLAG_LOGGING	B31	// 経過表示(ログウィンドウ不要, internal)

//#define VFSFOP_OPTFLAG_DELDELAY		// 未 Delaymode
//#define VFSFOP_OPTFLAG_SUBSTREAM		// 未 サブストリームも必ず処理
//#define VFSFOP_OPTFLAG_NOSUBSTREAM	// 未 サブストリームも必ず処理
// 未 FLATMODE/BACKUPOLD dir名付加

// filter の内容
#define VFSFOP_FILTER_DELNUM		B0
#define VFSFOP_FILTER_NOFILEFILTER	B1
#define VFSFOP_FILTER_NODIRFILTER	B2
#define VFSFOP_FILTER_EXTRACTNAME	B3
#define VFSFOP_FILTER_NOEXTFILTER	B4
#define VFSFOP_FILTER_NOINITEXTRACTNAME	B7 // 内部用

// ログに使用する文字列。undo とかで参照するので変更不可
#define FOPLOGACTION_ATTRIBUTE	T("Attrib")	// 属性変更
#define FOPLOGACTION_COPY		T("Copy")	// コピー(Undo対象)
#define FOPLOGACTION_CREATEFILE	T("Create")	// ファイル作成
#define FOPLOGACTION_DELETE		T("Delete")	// ファイル削除
#define FOPLOGACTION_LINK		T("Link")	// ショートカット/ハードリンク.
#define FOPLOGACTION_MAKEDIR	T("MakeDir") // ディレクトリ作成
#define FOPLOGACTION_MOVE		T("Move")	// 移動(Undo対象)
#define FOPLOGACTION_RENAME		T("Rename")	// 名前変更
#define FOPLOGACTION_TRASH		T("Trash")	// ごみ箱に移動
#define FOPLOGACTION_UNPACK		T("Unpack")

#define FOPLOGACTION_APPEND		T("Append")	// 追加コピー／移動(Undo不可)
#define FOPLOGACTION_BACKUP		T("Backup")	// バックアップ
#define FOPLOGACTION_EXIST		T("Exist")	// Test時に存在有り(Undo不要)
#define FOPLOGACTION_MOVEDIR	T("MoveDir") // ディレクトリ移動
#define FOPLOGACTION_OVERWRITE	T("Overwrt") // 上書きコピー(Undo不可)
#define FOPLOGACTION_REPLACE	T("Replace") // 上書き移動(Undo不可)
#define FOPLOGACTION_SKIP		T("Skip")
#define FOPLOGACTION_SOURCE		T("Source")
#define FOPLOGACTION_FILESTAT	T(" stat") // ファイル情報、Undoのファイル確認
#define FOPLOGACTION_DESTINATION T("Destination")
#define FOPLOGACTION_TEST		T("Test") // Test 成功(Undo不要)
#define FOPLOGACTION_UNMATCH	T("Unmatch") // 同名のファイルとディレクトリ

#define FOPLOGACTION_DESTARROW	T(" ->") // 処理先を示す

//------------------------------------- PPb 関連
#define PPBBOOTSYNC "PPbS" // PPbの起動時同期に使用するイベント名

//	PPx 本体, PPCOMMON, PPx Module 間の情報交換用構造体
//## 0xff000～0xfffff:	内部用
#define PPXCMDID_EXEC	0xff000 // *execute
#define PPXCMDID_INSERT	0xff001 // *insert
#define PPXCMDID_REPLACE	0xff002 // *replace
#define PPXCMDID_SELECTTEXT	0xff003 // %*selecttext
// #define PPXCMDID_EDITTEXT	0xff004 // %*edittext 1.82+5 廃止
#define PPXCMDID_INSERTSEL	0xff005 // *insertsel
typedef struct {
	const TCHAR *name;
	int flag;
	const TCHAR *path;
} PPXCMD_EXEC;

//------------------------------------- PPc 関連
#define CLSID_IDATAOBJECT_AUTODD {0x95D5EDD3, 0x8F9B, 0x4825, {0xA5, 0x70, 0x81, 0x5D, 0x74, 0xA1, 0xEA, 0xA0}} // AutoD&Dに使う相手のDropTarget取得用GUID

// カラム拡張、ユーザカラムの内容を記憶する領域で使用する識別ID
// 0-77ff カラム拡張用 : DFC_COLUMNEX(0) ～ DFC_COLUMNEX_MAX(0x7800 - 1)
// -7f00  拡張コメント用 : DFC_COMMENTEX ～ DFC_COMMENTEX_MAX
// 7f80   長いファイル名 : DFC_LONGNAME
// 7f81   IDL : DFC_SHELLIDLIST
// 8000-  ユーザカラム用 : DFC_USERMIN ～ DFC_USERMAX ※DFC_USERMAXから順に使う
#define DFC_UNDEF 0xffff // ID未割当て
#define DFC_FAULT 0xfffe // カラム拡張読み込み失敗
#define DFC_USERMAX (0xff00 - 1)
#define DFC_USERMIN 0x8000
#define DFC_SHELLIDLIST 0x7f81
#define DFC_LONGNAME 0x7f80
// #define DFC_COMMENTEX_MAX 0x7f00 // PPCOMMON.H で定義
// #define DFC_COMMENTEX (DFC_COMMENTEX_MAX - 10 + 1)
#define DFC_COLUMNEX_MAX (0x7800 - 1)
#define DFC_COLUMNEX 0

#pragma pack(push, 1)
typedef struct { // DE_COLUMN
//	BYTE fmtID = DE_COLUMN
	BYTE width;
	WORD itemindex; // DFC_xxx
	BYTE fmtsize;
	TCHAR name[1];
} DISPFMT_COLUMN;
#pragma pack(pop)

typedef union {
	char dat[4];
	int block;
} XC_SORT_MODE;

typedef struct {
	XC_SORT_MODE mode;
	DWORD atr;
	DWORD option;
} XC_SORT;

#define DSET_CACHEONLY	B0	// キャッシュを使ってディレクトリ読み込みを抑制
#define DSET_NODIRCHECK	B1	// ディレクトリの監視を無効にする
#define DSET_ASYNCREAD	B2	// 非同期/遅延読み込みを有効にする
#define DSET_REFRESH_ACACHE	B3	// 非同期読み込み完了後、常にキャッシュを更新
#define DSET_NOSAVE_ACACHE	B4	// 非同期読み込み完了後のキャッシュ保存をしない
#define DSET_OFF_ACTION_REFRESH	B5	// ファイル操作後の更新を無効にする
#define DSET_DEFAULT	MAX32

#define DSETI_NOSPACE	0	// (INFO)アイコンの場所を用意しない (CELL)info利用
#define DSETI_BLANK		1	// 空欄
#define DSETI_BOX		2	// (INFO)枠のみ (CELL)空欄
#define DSETI_EXTONLY	3	// 拡張子による判断のみ
#define DSETI_NORMAL	4	// 拡張子+α
#define DSETI_OVL		5	// 拡張子+α+オーバレイ
#define DSETI_OVLNOC	6	// 拡張子+α+オーバレイ+キャッシュ無し
#define DSETI_OVLSINGLE	7	// 拡張子+α+オーバレイ+キャッシュ無し+シングルThr
#define DSETI_DEFAULT	MAX16	// 初期値を使用

typedef struct {
	WORD flags, deflags; // DSET_
	WORD infoicon, cellicon; // DSETI_	情報行/セルのアイコン
	XC_SORT sort; // ソートの設定
} XC_DSET;

typedef struct {
	DWORD index;
	TCHAR text[CMDLINESIZE];
} TOOLBARCUSTTABLESTRUCT;

#define LOADSETTINGS_DISPLEN 0x80
#define LOADSETTINGS_MASKLEN MAX_PATH
#define LOADSETTINGS_CMDLEN  CMDLINESIZE
typedef struct {
	XC_DSET dset;
	TCHAR disp[LOADSETTINGS_DISPLEN];	// [;]
	TCHAR mask[LOADSETTINGS_MASKLEN];
	TCHAR cmd[LOADSETTINGS_CMDLEN];
} LOADSETTINGS_TMP;

typedef struct {
	XC_DSET dset;
	TCHAR buf[4];
} LOADSETTINGS_CUST;

typedef struct {
	LOADSETTINGS_CUST *buf;
	LOADSETTINGS_CUST cust;
} LOADSETTINGS_MOD;
#define LOADSETTINGS_DIR_OPT(ls) (ls->longbuf + (sizeof(XC_DSET) / sizeof(TCHAR)))

#define MC_CELS_TEMPNAME T("?Edit?") // 表示書式編集用に使う一時設定名

#define DetectWildcardLetter T("!*,/:;?")
#define DetectPathWildcardLetter T("!*,/;?")

// X_dss の指定
#define DSS_FONT	B0 // フォントサイズの変更を有効にする
#define DSS_MINFONT	B1 // フォントサイズの最小値調整を有効にする
#define DSS_ICON	B2 // アイコンサイズの変更を有効にする
#define DSS_COMCTRL	B3 // その他の部品(Tree, ListView, Header, Button, ToolBar, ImageList)のDSを有効にする
#define DSS_ACTIVESCALE	B4 // 動的変更を有効にする
#define DSS_DIALOGREDUCE	B5 // ダイアログの画面はみ出し抑制

#define DSS_DEFFLAGS	0xff
#define DSS_NOLOAD	(DSS_DEFFLAGS | 0xa00) // 設定未読み込み

typedef struct {
	int enter;
	int exectype;
} CRMENUSTACKCHECK;

#define PSPONE_PANE_DEFAULT 0 // デフォルト
#define PSPONE_PANE_PAIR 1	// 反対窓
#define PSPONE_PANE_NEWPANE 2 // 新規ペイン
#define PSPONE_PANE_RIGHTPANE 3 // 右/上側ペイン
#define PSPONE_PANE_LEFTPANE 5 // 左/下側ペイン
#define PSPONE_PANE_MAINPANE 6 // メインペイン
#define PSPONE_PANE_SUBPANE 7 // サブペイン

#define PSPONE_PANE_SETPANE 32 // 指定ペイン(pane no + PSPONE_PANE_SETPANE)

//------------------------------------- PPv 関連
typedef enum {
	DISPT_NONE = 0, DISPT_HEX, DISPT_TEXT, DISPT_DOCUMENT, DISPT_IMAGE, DISPT_RAWIMAGE, DISPT_MAX, DISPT_UNDEFINED = -1
} DISPT;

//-------------------------------------
//#define XUUB	T("XUUB|%s is not known TORO's site.\nBegin download?") 未使用
//#define QECS	There are few customize area. Extend? 0.50廃止
//#define IECS	Customize area extension was ready. 0.50廃止
//#define NOES	any mach 1.28廃止
//#define ERED	Read error 1.28廃止
//#define 7883	一時設定 1.70 ボタン→ボタンメニューに変更
//#define 78C5	前方一致&F 1.74 廃止
//#define ITFC	file+c 1.74 廃止
