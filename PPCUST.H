/*-----------------------------------------------------------------------------
	Paper Plane xUI		customizer
-----------------------------------------------------------------------------*/
#define PPXCUSTMODE_temp_RESTORE -1

#define LCID_PPXDEF LCID_JAPANESE


#ifndef LVM_SETEXTENDEDLISTVIEWSTYLE // IE3
	#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#endif
#ifndef LVS_EX_FULLROWSELECT // IE3
#define LVS_EX_FULLROWSELECT 0x0020
#endif
#ifndef LVS_EX_LABELTIP // IE5
#define LVS_EX_LABELTIP 0x4000
#endif
#define TListView_GetFocusedItem(hwnd) (int)SendMessage(hwnd, LVM_GETNEXTITEM, (WPARAM)-1, TMAKELPARAM(LVNI_FOCUSED, 0))
#define INT32__MAX 0x7fffffff

extern const TCHAR StrCustTitle[];
extern const TCHAR GetFileExtsStr[];
extern TCHAR PPcustRegID[REGIDSIZE];
extern const TCHAR NilStr[1];
extern TCHAR NilStrNC[1];
extern HINSTANCE hInst;
extern UINT WM_PPXCOMMAND;
extern int X_uxt[X_uxt_items];

extern void InfoDb(TCHAR *buf);
// console 関連 ---------------------------------------------------------------
extern void Print(const TCHAR *str);
extern int CustDump(const TCHAR *cmdptr, const TCHAR *filename);
extern int CustStore(TCHAR *fname, int appendmode, HWND hDlg);
extern int CustInit(void);
extern int HistDump(const TCHAR *cmdptr, const TCHAR *filename);
extern int HistInit(void);

// GUI 関連 -------------------------------------------------------------------
#define LONG_CMDSIZE (CMDLINESIZE * 10) // コマンドラインエディタの対応文字数

extern BOOL X_chidc; // コンソールを隠すか
extern HWND hConsoleWnd; // コンソールウィンドウ
extern LCID UseLcid;		// 表示言語
extern OSVERSIONINFO OSver;
extern DWORD X_dss;

extern TCHAR *FirstTypeName;
extern TCHAR *FirstItemName;

extern COLORREF userColor[16];
extern void FixColorList(void);
extern int GetListCursorIndex(WPARAM wParam, LPARAM lParam, int *indexptr);
extern void CustDialog(int Template, DLGPROC dlgprc);
extern void InitWndIcon(HWND hDlg, UINT baseControlID);
extern void MakeKeyDetailText(int key, TCHAR *dest, BOOL extype);
extern void GetControlText(HWND hDlg, UINT ID, TCHAR *text, DWORD len);
extern void GUILoadCust(void);
extern void ChangeCustEnv(HWND hDlg);
extern void SetTabTitles(HWND hDlg);
extern void USEFASTCALL AppendMenuString(HMENU hMenu, UINT id, const TCHAR *string);
extern void RemoveControlKeydown(HWND hWnd);
extern int CustTrackPopupMenu(HWND hDlg, HMENU hMenu, POINT *pos);
extern int ButtonTrackPopupMenu(HWND hDlg, HMENU hMenu, HWND hTargetButton);
extern TCHAR *LoadTextResourceData(HINSTANCE hModule, LPCTSTR rname);

extern THEME_COLORS WinColors;

#define C_WindowBack (WinColors.c.WindowBack) // ウィンドウ窓内(Edit,ListBox)
#define C_DialogBack (WinColors.c.DialogBack) // ダイアログ表面(元は3dFace)

#define C_WindowText (WinColors.c.WindowText) // ウィンドウ文字
#define C_DialogText (WinColors.c.DialogText) // ダイアログ文字

#define C_HighlightText (WinColors.c.HighlightText) // 選択文字
#define C_HighlightBack (WinColors.c.HighlightBack) // 選択背景

#define C_GrayState (WinColors.c.GrayState) // 灰色文字・背景
#define C_FocusBack (WinColors.c.FocusBack)
#define C_EdgeLine (WinColors.c.EdgeLine)


#ifdef UNICODE
	#define BANNERTEXT(mestext, utext) mestext
	#define GetBannerText(mestext) MessageText(mestext)
#else
	#define BANNERTEXT(mestext, utext) utext
	extern const WCHAR *GetBannerText(const WCHAR *utext);
#endif


extern void tstrreplace(TCHAR *text, const TCHAR *targetword, const TCHAR *replaceword);
extern void FixCutReturnCode(TCHAR *text);

// ダイアログ
extern void GUIcustomizer(int startpage, const TCHAR *param);

extern INT_PTR CALLBACK FilePage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK GeneralPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK ExtPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK KeyPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK MousePage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK MenuPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK BarPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK ColorPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK AddonPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK EtcPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

extern INT_PTR CALLBACK TextCustomizeDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK CommandTreeDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

extern INT_PTR CALLBACK DlgSheetProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, UINT id);
extern void InitPropSheetsUxtheme(HWND hDlg);


extern void FixWildItemName(TCHAR *item);

// ppcst_et.c その他
extern void Changed(HWND hWnd);
extern int KeyInput(HWND hWnd, TCHAR *string);
extern void Test(void);
extern TCHAR *LoadTextResource(HINSTANCE hModule, LPCTSTR rname);
extern void USEFASTCALL SetDlgFocus(HWND hDlg, int id);
extern void EnableDlgWindow(HWND hDlg, int id, BOOL state);
extern void ShowDlgWindow(const HWND hDlg, const UINT id, BOOL show);
extern void HideDlgWindow(const HWND hDlg, const UINT id);

extern INT_PTR CALLBACK HideMenuColorDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK HighlightDialogBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

extern int EtcEditFormat;
	// EtcEditFormat の種類
enum {
						// 編集ボタン					削除
	ETC_TEXT = 0,		// 追加(そのまま書き込み)		有り
	ETC_EXTEXT,			// 追加(そのまま書き込み、複数行)	有り
	ETC__EDITBUTTON,	//
	ETC_CELLDISP,		// 編集(書式編集ダイアログ)		有り
	ETC_INFODISP,		// 編集(書式編集ダイアログ)		なし
	ETC_HLKEY,			// 編集(ハイライトダイアログ)	有り
	ETC_DSET,			// 編集(PPc呼び出し)			有り
	ETC__VIEW,			//
	ETC_WINPOS,			// なし							有り
	ETC_EXECS,			// なし							有り
	ETC_NOPARAM			// なし							有り
};

struct EtcLabelsStruct {
	const TCHAR *name, *key, *comment;
	int form;
};

extern const struct EtcLabelsStruct EtcLabels[];

// ppcst_ed.c その他-PPc表示書式
extern void FormatCellDispSample(TCHAR *data, const TCHAR *label, int offset);
extern INT_PTR CALLBACK DispFormatDialogBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

extern BOOL ListView__Select(HWND hListView, int index);
extern BOOL ListView__GetText(HWND hListView, int index, TCHAR *text, int textlen);

