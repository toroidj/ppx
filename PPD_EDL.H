/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library						～ 拡張エディット ～
-----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

// CMDSEARCH_ 内部用
#define CMDSEARCHI_FINDFIRST B16 // SearchFileInedMain 内でFindFirstを１度行った
#define CMDSEARCHI_SAVEWORD  B17 // 分離単語をFnamePに書き込む(始めの１回目専用)
#define CMDSEARCHI_FINDBRAKET B18 // 括弧有り(通知用)
#define CMDSEARCHI_SELECTPART B19 // 補完時に不一致部分を選択
#define CMDSEARCHI_BRAKET B20 // キーワードが " で括られていた

#define TREEHEIGHT 240
#define SELBUFSIZE 0x600

struct PPeFindReplace {
	TCHAR findtext[VFPS];
	TCHAR replacetext[VFPS];
};

// 補完用テキストファイル
typedef struct {
	const TCHAR *filename;	// ベースとなるファイル名
	TCHAR *mem;		// イメージの先頭
	TCHAR *text;	// イメージ内のテキストの先頭
	volatile BOOL loading;
	volatile int ref;
//	FILETIME modifytime;
} FILLTEXTFILE;

// ComboList.mode の値
typedef enum {
	LIST_SEARCH,	// 前方一致検索(リスト表示無し)
	LIST_WRITE,		// 書込み用ヒストリで検索(リスト表示無し)
	LIST_READ,		// 読込み用ヒストリで検索(リスト表示無し)
	LIST_MANUAL,	// リストを手動で開いた
	LIST_FILL,		// エントリ自動補完用リスト
	LIST_FILLEXT	// エントリ自動補完用リスト(拡張子考慮有り)
} COMBOLISTMODE;

#define LISTU_NOLIST 0	// リストウィンドウは使用していない
#define LISTU_FOCUSMAIN 1	// 第１リスト／エディットボックスにフォーカス
#define LISTU_FOCUSSUB 2	// 第２リストにフォーカス

#define LISTFLAG_NOMODULE	B0	// Search module を使用しない
#define LISTFLAG_USER_COMB	B1	// ユーザ定義列挙
#define LISTFLAG_LISTMODE	B2	// リスト形式(検索無し)
#define LISTFLAG_DISABLE	B7	// 補完処理を無効にする。（終了中）

struct ComboList {	// コンボボックス処理用リストボックス
	HWND hWnd, hSubWnd;		// 第１リスト用 hWnd と 第２リスト用 hWnd
	WNDPROC OrgProc, OrgSubProc;	// 元 WndProc
	TCHAR *OldString;		// リストボックス起動前のエディットボックスの内容
	DWORD OldStart, OldEnd;	// ↑の選択範囲
	int ListFocus;			// LISTU_
	int index;				// リスト非表示時に使用するヒストリ参照index
	ECURSOR select;			// 書き換える範囲
	COMBOLISTMODE mode;		// 参照方法(現在) LIST_
	COMBOLISTMODE startmode;	// 参照方法(開始時) LIST_
	int direction;			// 第１リストの表示方向 >0:下側 =<0:上側
	int itemheight;			// リスト項目の高さ
	int upperbottom;		// 上側リストの下辺の位置
	WORD WhistID;			// 書込み用ヒストリID
	WORD RhistID;			// 読込み用ヒストリID
	volatile DWORD flags;	// LISTFLAG_
	FILLTEXTFILE filltext_user;	// ユーザ指定用
};

// 内部で使用する PPXEDIT_ 。B20-B30 まで使用可能
#define PPXEDIT_SYNTAXCOLOR	B24	// シンタックス着色
#define PPXEDIT_SUGGEST		B25	// キー操作によるヒストリ参照や補完リスト使用
#define PPXEDIT_RICHEDIT	B26	// RichEdit 上で使用 RichEdit 向け制御を行う
#define PPXEDIT_LINEEDIT	B27	// 一行編集ダイアログ上で使用。フォント変更時の自動窓サイズ変更、キーによる窓位置変更
#define PPXEDIT_TEXTEDIT	B28	// 改行有りテキスト編集(MULTILINE)
#define PPXEDIT_LISTCOMP	B29	// list 形式補完
#define PPXEDIT_LINECSR		B30	// Line cursor

#define LIST_THREAD_MAX 20


#ifndef __ITextDocument_INTERFACE_DEFINED__
#define __ITextDocument_INTERFACE_DEFINED__
#undef INTERFACE
#define INTERFACE ITextDocument
DECLARE_INTERFACE_(ITextDocument, IDispatch)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID, void **) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	STDMETHOD(GetTypeInfoCount)(THIS_ UINT *) PURE;
	STDMETHOD(GetTypeInfo)(THIS_ UINT, LCID, ITypeInfo **) PURE;
	STDMETHOD(GetIDsOfNames)(THIS_ REFIID, LPOLESTR *, UINT, LCID, DISPID *) PURE;
	STDMETHOD(Invoke)(THIS_ DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *) PURE;

	STDMETHOD(GetName)(THIS_ BSTR *pName);
	STDMETHOD(GetSelection)(THIS_ /*ITextSelection*/void **ppSel);
	STDMETHOD(GetStoryCount)(THIS_ long *pCount);
	STDMETHOD(GetStoryRanges)(THIS_ /*ITextStoryRanges*/void **ppStories);

	STDMETHOD(GetSaved)(THIS_ long *pValue);
	STDMETHOD(SetSaved)(THIS_ long Value);
	STDMETHOD(GetDefaultTabStop)(THIS_ float *pValue);
	STDMETHOD(SetDefaultTabStop)(THIS_ float Value);
	STDMETHOD(New)(THIS);
	STDMETHOD(Open)(THIS_ VARIANT *pVar, long Flags, long CodePage);
	STDMETHOD(Save)(THIS_ VARIANT *pVar, long Flags, long CodePage);
	STDMETHOD(Freeze)(THIS_ long *pCount);
	STDMETHOD(Unfreeze)(THIS_ long *pCount);
	STDMETHOD(BeginEditCollection)(THIS);
	STDMETHOD(EndEditCollection)(THIS);
	STDMETHOD(Undo)(THIS_ long Count, long *pCount);
	STDMETHOD(Redo)(THIS_ long Count, long *pCount);
	STDMETHOD(Range)(THIS_ long cpActive, long cpAnchor, /*ITextRange*/void **ppRange);
	STDMETHOD(RangeFromPoint)(THIS_ long x, long y, /*ITextRange*/void **ppRange);
};
#endif
#define RichEdit_Suspend_Undo(RichEdit) RichEdit->lpVtbl->Undo(RichEdit, /*tomSuspend*/ -9999995, 0);
#define RichEdit_Resume_Undo(RichEdit) RichEdit->lpVtbl->Undo(RichEdit, /*tomResume*/ -9999994, 0);


typedef struct tagPPxEDSTRUCT {
	PPXAPPINFO *info;	// 親のcallback ※必ず構造体の先頭に配置すること！
	HWND hWnd;		// 自分自身の hWnd
	HWND hTreeWnd;	// ツリー表示用ウィンドウ
	struct {
		HWND hWnd, hToolBarWnd;	// 浮遊ツールバー用ウィンドウ
		ThSTRUCT Cmd;
	} FloatBar;
	WNDPROC hOldED;		// 元のウィンドウプロージャ
	const TCHAR *AKey;	// 追加キー割当て
	CALLBACKMODULEENTRY KeyHookEntry;		// キー入力拡張モジュール
	int ref;		// PPxEDSTRUCT 参照カウンタ
	int flags;		// PPxRegistExEdit の PPXEDIT_ と同じ
						// GUI情報
	DWORD style;	// Window style
	DWORD exstyle;	// Window exstyle
	DWORD fontX, fontY, OrgFontY;	// 文字高さ,初期文字高さ
	int caretY;		// カレットのY
	DWORD caretLY;	// カレットの行数Y
	UINT tab;		// TAB 幅(ダイアログ単位)
					// テキスト情報
	int CrCode;		// 保存時に使用する改行コード
	UINT CharCP;	// 保存時に使用する文字コード
					// 補完
	ESTRUCT ED;			// GUI/CUI 共通構造体
					// サーチ中の判断に使用
	WORD oldkey, oldkey2;	// 直前の入力キー

	ThSTRUCT LocalStringValue;	// Edit内限定特殊環境変数
		#define LSV_filename T("filename") // text filename
		#define LSV_user_comb T("ListComb") // user combination
	struct PPeFindReplace *findrep;
	BOOL mousepos;	// メニューの表示位置 FALSE:キー操作 TRUE:マウス操作
	DWORD SyntaxType; // シンタックスハイライトの種類(書込み用ヒストリID)
						// 一覧表示関係
	struct ComboList list;
	DWORD fontYext;
	HFONT hEditFont; // WM_SETFONT で指定されたフォント

	ITextDocument *RichEdit;
	volatile DWORD ActiveListThreadID; // 現在有効なスレッドのID
	volatile int ListThreadCount; // 生きているスレッド数
} PPxEDSTRUCT;

typedef struct {
	WORD key;		// 使用するキー 0なら末尾
						// 実行する関数	0:スルー処理、!0:戻り値
	int (USEFASTCALL *func)(PPxEDSTRUCT *PES);
} KEYCOMMANDS;

typedef struct {
	WORD key;		// 押されたキー・機能.
	int repeat;		// リピート数
} PPECOMMANDPARAM;
typedef BOOL (APIENTRY *impGetOpenFileName)(LPOPENFILENAME lpofn);

typedef struct {
	TCHAR textbuf[SELBUFSIZE], *text;	// 作業領域
	TCHAR *word;				// 取得した文字列へのポインタ
	ECURSOR cursor, cursororg;	// 選択範囲
	size_t textlength;	// text内の文字数
} TEXTSEL;

enum { TEXTSEL_ALL = 0, TEXTSEL_CHAR = 1, TEXTSEL_BACK, TEXTSEL_WORD, TEXTSEL_BEFORE, TEXTSEL_AFTER, TEXTSEL_BEFOREWORD };

#define Edit_SendMessageMsg(PES, msg) CallWindowProc(PES->hOldED, PES->hWnd, msg, 0, 0)

#define Edit_SendMessage(PES, msg, wparam, lparam) CallWindowProc(PES->hOldED, PES->hWnd, msg, wparam, lparam)
#define Edit_GetWindowTextLength(PES) CallWindowProc(PES->hOldED, PES->hWnd, WM_GETTEXTLENGTH, 0, 0)
#define Edit_GetWindowText(PES, text, length) CallWindowProc(PES->hOldED, PES->hWnd, WM_GETTEXT, (WPARAM)length, (LPARAM)text)
#define Edit_SetWindowText(PES, text) CallWindowProc(PES->hOldED, PES->hWnd, WM_SETTEXT, 0, (LPARAM)text)

//--------------------------------------------------------------- PPD_EDL.C
extern int xpbug;
extern int X_esrx;

LRESULT CALLBACK EDsHell(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
extern void InitExEdit(PPxEDSTRUCT *PES);
extern const TCHAR StrPPxED[];

extern ERRORCODE EditExtractMacro(PPxEDSTRUCT *PES, const TCHAR *param, TCHAR *extract, int flags);

typedef struct {
	UINT codepage;
	int crcode;
	int tabwidth;
} EDITMODESTRUCT;
#define EDITMODESTRUCT_DEFAULT 0, -1, -1

extern void FreeEdStruct(PPxEDSTRUCT *PES);
extern void PPeSetTab(PPxEDSTRUCT *PES, int usetab);
extern BOOL EditModeParam(EDITMODESTRUCT *ems, const TCHAR *param, const TCHAR *more);
extern void XEditSetModify(PPxEDSTRUCT *PES);
extern void XEditClearModify(PPxEDSTRUCT *PES);

#define charmenu_items 13
#define charmenu_other 10
#define charmenu_sjis 2
extern PPXINMENU returnmenu[];
extern PPXINMENU charmenu[charmenu_items];
extern int CloseLineList(PPxEDSTRUCT *PES);

typedef struct {
	TCHAR *textbuf;
	TCHAR *textptr;
	int mode;
	int x, y;
	int *inums;
} EditLPos;

#define EditLPos_RELCURSOR B0
#define EditLPos_GET_L_CURSORPOS B1
#define EditLPos_GET_CURSORPOS (EditLPos_GET_L_CURSORPOS | EditLPos_RELCURSOR)
#define EditLPos_GET_TEXT B2
#define EditLPos_COUNT_POS B3
#define EditLPos_SET B4
#define EditLPos_SET_RANGE B5
#define EditLPos_SET_POS (EditLPos_COUNT_POS | EditLPos_SET)
#define EditLPos_CURSOR_ENDSEL B6

extern BOOL USEFASTCALL PPeLogicalLinePos(PPxEDSTRUCT *PES, EditLPos *elp);
extern void RichSyntaxColor(PPxEDSTRUCT *PES);

//--------------------------------------------------------------- PPD_EDLC.C
typedef int EDITDIST;
#define EDITDIST_BACK ((EDITDIST)-1)
#define EDITDIST_DIALOG ((EDITDIST)0)
#define EDITDIST_NEXT ((EDITDIST)1)
#define EDITDIST_NEXT_FILL ((EDITDIST)2) // FloatListで使用、LIST_FILL有効
#define EDITDIST_NEXT_WITHNOW ((EDITDIST)3) // SearchStrで使用、現在位置も検索
#define EDITDIST_REPLACE_TEST ((EDITDIST)4) // SearchStrで使用、置換対象か確認

extern void FreeBackupText(PPxEDSTRUCT *PES);
extern void FloatList(PPxEDSTRUCT *PES, EDITDIST dist);
extern BOOL ListUpDown(HWND hWnd, PPxEDSTRUCT *PES, int offset, int repeat);
extern ERRORCODE ListPageUpDown(PPxEDSTRUCT *PES, EDITDIST dist, BOOL search);
extern void ListSearch(HWND hWnd, PPxEDSTRUCT *PES, int len);
extern TCHAR KeyStepFill_listmode[];
extern void KeyStepFill(PPxEDSTRUCT *PES, TCHAR *option);
extern void FixListPosition(PPxEDSTRUCT *PES);
extern void ListSelect(PPxEDSTRUCT *PES, HWND hListWnd);
extern HWND PPxRegistExEditCombo(HWND hED, int maxlen, const TCHAR *defstr, WORD rHist, WORD wHist, int flags);

extern void BackupText(HWND hWnd, PPxEDSTRUCT *PES);
extern void CreateMainListWindow(PPxEDSTRUCT *PES, int direction);
extern void CreateSubListWindow(PPxEDSTRUCT *PES);
extern void CancelListThread(PPxEDSTRUCT *PES);
extern BOOL ExtraEntrySearchString(const TCHAR *text, ESTRUCT *ED);
extern void FreeFillTextFile(FILLTEXTFILE *filltext);

//--------------------------------------------------------------- PPD_EDLL.C
extern void ClosePPeTreeWindow(PPxEDSTRUCT *PES);
extern void PPeTreeWindow(PPxEDSTRUCT *PES);

//--------------------------------------------------------------- PPD_EDLM.C
#define PPE_OPEN_MODE_OPEN	0
#define PPE_OPEN_MODE_INSERT	1
#define PPE_OPEN_MODE_OPENNEW	-1
#define PPE_OPEN_MODE_CMDOPEN	2
#define PPE_OPEN_MODE_CMDINSERT	3
#define PPE_OPEN_MODE_EXCMDOPEN	PPE_OPEN_MODE_EXCMDOPENnoSIZE
#define PPE_OPEN_MODE_EXCMDOPENnoSIZE	4
#define PPE_OPEN_MODE_EXCMDOPENandSIZE	5
extern BOOL OpenFromFile(PPxEDSTRUCT *PES, int openmode, const TCHAR *fname);
extern void OpenMainFromMem(PPxEDSTRUCT *PES, int openmode, const TCHAR *filename, const TCHAR *textimage, DWORD memsize, UINT usecp);

#define EDL_FILEMODE_APPEND		B0
#define EDL_FILEMODE_NODIALOG	B1
#define EDL_FILEMODE_DIALOG		B2

extern BOOL FileSave(PPxEDSTRUCT *PES, int mode);
extern void FileOpen(PPxEDSTRUCT *PES, int mode);
extern void JumptoLine(HWND hWnd, int line);
extern void InitEditCharCode(PPxEDSTRUCT *PES);

//--------------------------------------------------------------- PPD_EDLS.C
#ifndef UNICODE
extern void USEFASTCALL CaretFixToW(const char *str, DWORD *of);
extern void USEFASTCALL CaretFixToA(const char *str, DWORD *of);
#else
#define CaretFixToW(str, of)
#define CaretFixToA(str, of)
#endif
extern BOOL InitPPeFindReplace(PPxEDSTRUCT *PES);
extern BOOL SelectEditStrings(PPxEDSTRUCT *PES, TEXTSEL *ts, int mode);
extern void FreeTextselStruct(TEXTSEL *ts);
extern TCHAR *SearchFileInedMain(ESTRUCT *ED, TCHAR *str, int flags);

extern BOOL SearchStr(PPxEDSTRUCT *PES, EDITDIST dist);
extern int CheckXPbug(void);
extern void GetHeight(PPxEDSTRUCT *PES, HFONT hFont);
extern void LineCursor(PPxEDSTRUCT *PES, DWORD mes);
extern void USEFASTCALL SetMessageForEdit(HWND hWnd, const TCHAR *message);
extern void FixEditWindowRect(HWND hWnd);

#ifndef UNICODE
extern void GetEditSel(HWND hWnd, const TCHAR *buf, ECURSOR *cursor);
extern void SetEditSel(HWND hWnd, const TCHAR *buf, DWORD start, DWORD end);
#else
#define GetEditSel(hWnd, buf, cursor) SendMessage(hWnd, EM_GETSEL, (WPARAM)cursor.start, (LPARAM)cursor.end)
#define SetEditSel(hWnd, buf, start, end) SendMessage(hWnd, EM_SETSEL, start, end)
#endif

#ifdef __cplusplus
}
#endif
