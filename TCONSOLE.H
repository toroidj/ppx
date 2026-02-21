/*-----------------------------------------------------------------------------
	コンソール向け定義集
-----------------------------------------------------------------------------*/
#include <wincon.h>

extern HMODULE hKernel32;
extern HANDLE hProcHeap;
/*-----------------------------------------------------------------------------
		色の定義
-----------------------------------------------------------------------------*/
#define T_BLA	0
#define T_GRY	(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
#define T_DCY	(FOREGROUND_BLUE  | FOREGROUND_GREEN)
#define T_DGR	FOREGROUND_INTENSITY
#define T_BLU	(FOREGROUND_BLUE  | FOREGROUND_INTENSITY)
#define T_RED	(FOREGROUND_RED   | FOREGROUND_INTENSITY)
#define T_MAG	(FOREGROUND_BLUE  | FOREGROUND_RED | FOREGROUND_INTENSITY)
#define T_GRE	(FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define T_CYA	(FOREGROUND_BLUE  | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define T_BRO	(FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define T_WHI	(FOREGROUND_BLUE  | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)

#define TR_BLA	0
#define TR_DGR	BACKGROUND_INTENSITY
#define TR_DCY	(BACKGROUND_BLUE  | BACKGROUND_GREEN)
#define TR_GRY	(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED)
#define TR_BLU	BACKGROUND_BLUE
#define TR_RED	BACKGROUND_RED
#define TR_MAG	(BACKGROUND_BLUE  | BACKGROUND_RED | BACKGROUND_INTENSITY)
#define TR_GRE	(BACKGROUND_GREEN | BACKGROUND_INTENSITY)
#define TR_CYA	(BACKGROUND_BLUE  | BACKGROUND_GREEN | BACKGROUND_INTENSITY)
#define TR_BRO	(BACKGROUND_RED   | BACKGROUND_GREEN | BACKGROUND_INTENSITY)
#define TR_WHI	(BACKGROUND_BLUE  | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)

#define T_DL	COMMON_LVB_UNDERSCORE				// 下線

extern WORD *TMC_Default;

#define TC_edit				0 // CB_edit
#define TC_edit_select		1
#define TC_ref				2
#define TC_ref_select		3
#define TC_execute			4 // CB_com
#define TC_popup_frame		5 // CB_pop
#define TC_popup_caption	6
#define TC_popup_item		7
#define TC_popup_select_item 8
#define TC_popup_dir		9
#define TC_popup_select_dir	10
#define TC_default	11

#define C_S_EDIT_TEXT		((COLORREF)0x1010101)
#define C_S_EDIT_BACK		((COLORREF)0x1010102)
#define C_S_SELECT_TEXT		((COLORREF)0x1010103)
#define C_S_SELECT_BACK		((COLORREF)0x1010104)

extern BOOL tInitConsole(void);
extern BOOL tConsoleInit(void);
extern void tRelease(void);
extern void tClearScreen(void);

#ifndef WINEGCCx
#define GetConsoleWindowInfo(conout, scrinfo) GetConsoleScreenBufferInfo(conout, scrinfo)
#else
extern void GetConsoleWindowInfo(HANDLE conout, CONSOLE_SCREEN_BUFFER_INFO *scrinfo);
#endif

#define KEY_RECV -1
#define KEY_TIMEOUT -2
#define KEY_DUMMY -3
extern int tgetc(INPUT_RECORD *inf);
extern int tgetin(INPUT_RECORD *inf, DWORD timeout);

/*--------------------------------------
	キー入力バッファをクリア
--------------------------------------*/
#define killkey()	FlushConsoleInputBuffer(hStdin)
/*--------------------------------------
	表示属性を変更する
--------------------------------------*/
extern void tSetColor(int colorindex);
extern void tSetDefaultColor(void);
extern void tFillAtr(int x1, int y1, int x2, int y2, int colorindex);
extern void tFillBox(int left, int top, int right, int bottom, int colorindex);
extern COLORREF ConRefColor(COLORREF color);

extern void tCsrPos(int x, int y);
extern void tCsrMode(int size);
extern void tputposstr(int x, int y, const TCHAR *str);
extern void tputstr(const TCHAR *str);
extern void tputstr_noinit(const TCHAR *str);
extern void tFillSpace(int left, int top, int right, int bottom);
extern void tFrameBox(int left, int top, int right, int bottom, int frameopt);

extern int CCharWide(WCHAR c);
extern void ClearStatusLine(void);

typedef struct {
	DWORD id;
	TCHAR *mes;	// 表示内容
} TMENU;

extern int Select(const TMENU *tmenu);
extern void SaveConsolePos(const TCHAR *id);

// Win32 SDK で登録されていない可能性があるもの -------------------------------
// COMMON_LVB_ は、WindowNT系の多バイト言語 or Windows10以降でサポート
#ifndef COMMON_LVB_UNDERSCORE
#define COMMON_LVB_UNDERSCORE 0x8000
#endif
#ifndef COMMON_LVB_REVERSE_VIDEO
#define COMMON_LVB_REVERSE_VIDEO 0x4000
#endif

typedef struct {
	ULONG cbSize;
	COORD dwSize;
	COORD dwCursorPosition;
	WORD wAttributes;
	SMALL_RECT srWindow;
	COORD dwMaximumWindowSize;
	WORD wPopupAttributes;
	BOOL bFullscreenSupported;
	COLORREF ColorTable[16];
} xCONSOLE_SCREEN_BUFFER_INFOEX;

typedef union {
	CONSOLE_SCREEN_BUFFER_INFO info;		// 現在の画面状態
} TSCREENINFO;

extern TSCREENINFO screen;		// 現在の画面状態
extern HANDLE hStdin, hStdout;
extern DWORD OldStdinMode, OldStdoutMode;
extern DWORD PPxStdinMode, PPxStdoutMode;

extern CALLBACKMODULEENTRY KeyHookEntry;		// キー入力拡張モジュール
extern void ConChangeFontSize(SHORT delta);
extern void tInitCustomize(void);
extern const TCHAR *first_command;
extern void PrintOption(void);

extern HWND hHostWnd; // conhost.exe 等のコンソールホストウィンドウ
extern BOOL UseGUI; // TRUE の場合は、メニューやフォーカス制御が可能
extern int UseMouse;
#define ConsoleMouse_DISABLE 0	// マウス使用不可
#define ConsoleMouse_TINY 1	// マウス一部使用（ターミナル側優先）
#define ConsoleMouse_FULL 2	// マウス使用可能
extern int ConsoleHost;
#define ConsoleHost_conhost 0	// conhost.exe を使用
#define ConsoleHost_WindowsTerminal 1	// WindowsTerminal を使用
#define ConsoleHost_OpenConsole 2	// OpenConsole.exe を使用
#define ConsoleHost_unknown 3	// 不明
extern BOOL ConsoleOnRemote; // TRUE のときはリモートなので、ダイアログ等使用禁止
#define ConsoleColor_MIN 0 // 16色
#define ConsoleColor_ESC 1 // ESC 16色
#define ConsoleColor_MAX 2 // ESC 32bit
extern int X_cone;

#define TMI_NODRAW 0
#define TMI_DRAW_CURSOR 1
#define TMI_DRAW_CHANGE_CURSOR 2
#define TMI_DRAW_ALL 3
#define TMI_DRAW_ALL_REDRAW 4

typedef struct {
	POINT window; // 表示位置
	POINT oldmousepos;
	SIZE screensize; // 画面大きさ
	SIZE size; // 表示大きさ
	int index, oldindex; // 現在位置
	int showindex, oldshowindex; // 表示開始位置
	int draw;
	int total; // 総数
	int mousebutton;
	CONSOLE_SCREEN_BUFFER_INFO oldoutinfo;
	CONSOLE_CURSOR_INFO oldoutcsr;
} TMENUINFO;

extern void InitTMenuInfo(TMENUINFO *tmi);
extern void EndTMenuInfo(TMENUINFO *tmi);
extern void ClearTMenuInfoArea(TMENUINFO *tmi, BOOL popup);
extern int TMenuInfoKey(TMENUINFO *tmi);

#ifndef FROM_LEFT_1ST_BUTTON_PRESSED
#define FROM_LEFT_1ST_BUTTON_PRESSED B0
#define RIGHTMOST_BUTTON_PRESSED B1
#endif

#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS 0x80
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
 #define ENABLE_VIRTUAL_TERMINAL_INPUT 0x200
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
 #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 4 // Win10 RS2 以降
#endif

#define CONSOLE_IN_FLAGS_DEF (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT)
#define CONSOLE_IN_FLAGS_NOWINEDIT (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS)
#define CONSOLE_IN_FLAGS_KEYONLY (0)

#define CONSOLE_OUT_FLAGS_NT (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT)
#define CONSOLE_OUT_FLAGS_VT (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)

//           IN/OUT  ENABLE_xxx の初期値
// Win95     17/3
// WinNT4    97/3
// Win2000   f7/3
// WinXP     b7/3
// WinVista-Win10 1b7/3
// Win11	1f7/7
