/*-----------------------------------------------------------------------------
	Paper Plane bUI								～ コンソールライブラリ ～
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <wincon.h>
#include <string.h>
#include "PPX.H"
#include "VFS.H"
#include "PPB.H"
#include "TCONSOLE.H"
#pragma  hdrstop

/*
	ESC c
	CSI y;xH
	CSI 6n

ECMA-48 3rd
	CSI 38/48;r;g;bm

VT330
	CSI ?h DCS ST sixel

VT400
	CSI t;l;b;r$z
*/


#ifndef MOUSE_WHEELED
 #define MOUSE_WHEELED 4
#endif

DLLDEFINED HMODULE hKernel32;
HANDLE hStdin, hStdout;
DLLDEFINED OSVERSIONINFO OSver;
HANDLE hProcHeap;

#define LocateBuffSize 32  // ESC [ 1234567890 ; 1234567890 m
#define SpaceBuffSize 5
#define LocateBuff PrintBuff
#define SpaceBuff (PrintBuff + LocateBuffSize)
TCHAR PrintBuff[LocateBuffSize + SpaceBuffSize];

typedef struct {
	ULONG cbSize;
	DWORD nFont;
	COORD dwFontSize;
	UINT FontFamily;
	UINT FontWeight;
	WCHAR FaceName[LF_FACESIZE];
} xCONSOLE_FONT_INFOEX;

// conhost の色から COLORREF に変換するためのテーブル
COLORREF ConsoleColors[16] = {
	C_BLACK, C_DBLUE, C_DGREEN, C_DCYAN, C_DRED, C_DMAGENTA, C_DYELLOW, C_DWHITE,
	C_DBLACK, C_BLUE, C_GREEN, C_CYAN, C_RED, C_MAGENTA, C_YELLOW, C_WHITE,
};

DefineWinAPI(BOOL, GetConsoleScreenBufferInfoEx, (HANDLE hConsoleOutput, xCONSOLE_SCREEN_BUFFER_INFOEX *lpConsoleScreenBufferInfoEx));
DefineWinAPI(BOOL, SetConsoleScreenBufferInfoEx, (HANDLE hConsoleOutput, xCONSOLE_SCREEN_BUFFER_INFOEX *lpConsoleScreenBufferInfoEx));
DefineWinAPI(BOOL, GetCurrentConsoleFontEx, (HANDLE hConsoleOutput, BOOL bMaximumWindow, xCONSOLE_FONT_INFOEX *lpConsoleCurrentFontEx)) = NULL;
DefineWinAPI(BOOL, SetCurrentConsoleFontEx, (HANDLE hConsoleOutput, BOOL bMaximumWindow, xCONSOLE_FONT_INFOEX *lpConsoleCurrentFontEx));

#ifndef CONSOLE_FULLSCREEN_HARDWARE
#define CONSOLE_FULLSCREEN_HARDWARE B1	// ウィンドウでない
#endif
DefineWinAPI(BOOL, GetConsoleDisplayMode, (LPDWORD lpModeFlags)) = INVALID_VALUE(impGetConsoleDisplayMode);

CONSOLE_CURSOR_INFO OldCsrMod ={0, 0};	// 起動時のカーソルの状態
DWORD OldStdinMode = 0, OldStdoutMode = 0; // 起動時のコンソールの状態
DWORD PPxStdinMode = CONSOLE_IN_FLAGS_NOWINEDIT;
DWORD PPxStdoutMode = CONSOLE_OUT_FLAGS_NT;

HWND hHostWnd = NULL; // conhost.exe 等のコンソールホストウィンドウ

int ConsoleHost = ConsoleHost_unknown;
BOOL UseGUI = TRUE; // GUI(menu や focus)使用可
int UseMouse = ConsoleMouse_FULL; // 0 使用不可、1 ホイール使用不可(WT)、2 完全使用可能(conhost)
BOOL ConsoleOnRemote = FALSE;	// デスクトップ上のコンソール / リモート

TSCREENINFO screen;
CONSOLE_CURSOR_INFO NowCsr;				// 現在のカーソルの状態
CALLBACKMODULEENTRY KeyHookEntry = NULL;
SHORT DefaultFontY = 0; // 元のフォントサイズ(未設定:0)
int X_dwfr = 0; // メニュー等の枠を表示する
int X_cone = ConsoleColor_MIN;

COLORREF palettes[16] = {
	C_AUTO, C_AUTO, C_AUTO, C_AUTO,
	C_AUTO, C_AUTO, C_AUTO, C_AUTO,
	C_AUTO, C_AUTO, C_AUTO, C_AUTO,
	C_AUTO, C_AUTO, C_AUTO, C_AUTO
};

#define CB_edit_total 4
#define CB_com_total 1
#define CB_pop_total 7
#define CB_total (CB_edit_total + CB_com_total + CB_pop_total)
union {
	struct {
		WORD CB_edit[CB_edit_total];
		WORD CB_com[CB_com_total];
		WORD CB_pop[CB_pop_total];
	} cb;
	WORD raw[CB_total];
} TermColor = { {
	{T_BLA | TR_GRY | T_DL,	TR_DCY | T_GRY | COMMON_LVB_UNDERSCORE, // CB_edit
	 T_GRE | TR_DCY | T_DL,	TR_GRE | T_DCY | COMMON_LVB_UNDERSCORE},
	{T_BRO}, // CB_com
	{T_DGR | TR_GRY,	T_DGR | TR_GRY, // CB_pop
	 T_BLA | TR_GRY,	T_GRY | TR_DCY,
	 T_DCY | TR_GRY,	T_GRE | TR_DCY,
	 T_GRY // 元の色(tInput起動時に初期化)
	}
} };

WORD *TMC_Default = &TermColor.raw[TC_default];

union {
	struct {
		COLORREF CB_editE[CB_edit_total * 2];
		COLORREF CB_comE[CB_com_total * 2];
		COLORREF CB_popE[CB_pop_total * 2];
	} cb;
	COLORREF raw[CB_total * 2];
} TermColorE = { {
	{C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO}, // CB_edit
	{C_AUTO, C_AUTO}, // CB_com
	{C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO, C_AUTO}, // CB_pop
} };

#define TCE_textlen (19 + 1) // ESC [ 38 ; 2 ; 255 ; 255 ; 255 m
#define TCE_backlen (19 + 1) // ESC [ 48 ; 2 ; 255 ; 255 ; 255 m
#define TCE_colorlen (TCE_textlen + TCE_backlen)
#define TCE_text(index) TermColorE.raw[(index) * 2]
#define TCE_back(index) TermColorE.raw[(index) * 2 + 1]

void ConvertConTo32(COLORREF *FullColors, /*WORD *ConColors,*/ int colors)
{
	for(;;){
		/*
		if ( *FullColors >= C_S_AUTO ){
			*FullColors = ConsoleColors[*ConColors & 0xf];
		}
		*/
		*FullColors = GetSchemeColor(*FullColors, C_S_AUTO);
		FullColors++;

		/*
		if ( *FullColors >= C_S_AUTO ){
			*FullColors = ConsoleColors[(*ConColors >> 4) & 0xf];
		}
		*/
		*FullColors = GetSchemeColor(*FullColors, C_S_AUTO);
		FullColors++;
		// ConColors++;
		colors--;
		if ( colors == 0 ) break;
	}
}

/*-----------------------------------------------------------------------------
	初期化・終了処理
-----------------------------------------------------------------------------*/
void tInitCustomize(void)
{
	GetCustData(T("CB_edit"), &TermColor.cb.CB_edit, sizeof(TermColor.cb.CB_edit));
	GetCustData(T("CB_editE"), &TermColorE.cb.CB_editE, sizeof(TermColorE.cb.CB_editE));

	GetCustData(T("CB_com"), &TermColor.cb.CB_com, sizeof(TermColor.cb.CB_com));
	GetCustData(T("CB_comE"), &TermColorE.cb.CB_comE, sizeof(TermColorE.cb.CB_comE));

	TermColor.raw[TC_popup_dir] = 0xffff;
	GetCustData(T("CB_pop"), &TermColor.cb.CB_pop, sizeof(TermColor.cb.CB_pop));
	if ( TermColor.raw[TC_popup_item] == 0xffff ){
		TermColor.raw[TC_popup_item] = T_BLA | TR_GRY;
		TermColor.raw[TC_popup_select_item] = T_GRY | TR_DCY;
	}
	if ( TermColor.raw[TC_popup_dir] == 0xffff ){
		TermColor.raw[TC_popup_dir] = (WORD)((TermColor.raw[TC_popup_item] & 0x80f0) | T_DCY);
		TermColor.raw[TC_popup_select_dir] = (WORD)((TermColor.raw[TC_popup_select_item] & 0x80f0) | T_GRE);
	}
	GetCustData(T("CB_popE"), &TermColorE.cb.CB_popE, sizeof(TermColorE.cb.CB_popE));

	ConvertConTo32(TermColorE.raw, /*TermColor.raw,*/ CB_total);


	if ( DSetCurrentConsoleFontEx != NULL ){
		LOGFONT confont;
		xCONSOLE_FONT_INFOEX cfi;

		if ( GetCustData(T("F_con"), &confont, sizeof(confont)) == NO_ERROR ){
			cfi.cbSize = sizeof(cfi);
			cfi.nFont = 0;
			cfi.dwFontSize.X = (SHORT)confont.lfWidth;
			cfi.dwFontSize.Y = (SHORT)confont.lfHeight;
			cfi.FontFamily = (UINT)confont.lfPitchAndFamily;;
			cfi.FontWeight = (UINT)confont.lfWeight;
			strcpyToW(cfi.FaceName, confont.lfFaceName, LF_FACESIZE);
			DSetCurrentConsoleFontEx(hStdout, FALSE, &cfi);
		}
	}
}

#if 0
void conpos(COORD *pos)
{
	char abuf[32];
	DWORD mode, dummy,n;

	GetConsoleMode(hStdin, &mode);
	SetConsoleMode(hStdin, CONSOLE_IN_FLAGS_KEYONLY);


	puts(CSI_A "6n");
	abuf[0] = getchar();
	if ( abuf[0] != 0x1b ) goto end;
	abuf[0] = getchar();
	if ( abuf[0] != '[' ) goto end;

	abuf[0] = getchar();
	if ( !IsdigitA(abuf[0]) ) goto end;
	n = abuf[0] - '0';
	for(;;){
	abuf[0] = getchar();
		if ( !IsdigitA(abuf[0]) ) break;
		n = n * 10 + abuf[0] - '0';
	}
	if ( abuf[0] != ';' ) goto end;
	pos->Y = n - 1;

	abuf[0] = getchar();
	if ( !IsdigitA(abuf[0]) ) goto end;
	n = abuf[0] - '0';
	for(;;){
	abuf[0] = getchar();
		if ( !IsdigitA(abuf[0]) ) break;
		n = n * 10 + abuf[0] - '0';
	}
	if ( abuf[0] != 'R' ) goto end;
	Messagef("X %d",n);
	pos->X = n - 1;
end:
	SetConsoleMode(hStdin, &mode);

}
#endif

BOOL tConsoleInit(void)
{
	DWORD flags = CONSOLE_IN_FLAGS_DEF;

#if 0
	COORD pos;
	conpos(&pos);
#endif

	if ( UseMouse == ConsoleMouse_FULL ){
		setflag( flags, ENABLE_EXTENDED_FLAGS );	// ENABLE_QUICK_EDIT_MODE 指定を有効に
	}else if ( UseMouse == ConsoleMouse_DISABLE ){
		resetflag( flags, ENABLE_MOUSE_INPUT );
	}
	if ( ConsoleOnRemote ) setflag( flags, ENABLE_PROCESSED_INPUT );
										// 標準入力の設定
	SetConsoleMode(hStdin, flags);
	SetConsoleMode(hStdout, PPxStdoutMode);

	GetConsoleWindowInfo(hStdout, &screen.info);

#if 0
	screen.info.dwCursorPosition.X = pos.X;
	screen.info.dwCursorPosition.Y = pos.Y;
#endif
	return TRUE;
}

BOOL tInitConsole(void)
{
	TCHAR buf[0x100], caption[16];
	DefineWinAPI(HWND, GetConsoleWindow, (void));

	hKernel32 = GetModuleHandle(T("Kernel32.DLL"));

	OSver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSver);

	GETDLLPROC(hKernel32, GetConsoleWindow);
	GETDLLPROC(hKernel32, GetConsoleScreenBufferInfoEx);
	GETDLLPROC(hKernel32, SetConsoleScreenBufferInfoEx);
	GETDLLPROC(hKernel32, GetCurrentConsoleFontEx);
	GETDLLPROC(hKernel32, SetCurrentConsoleFontEx);
	hProcHeap = GetProcessHeap();

	{
		TCHAR *dest, *destmax;
		dest = PrintBuff;
		destmax = dest + TSIZEOF(PrintBuff);
		while (dest < destmax) *dest++ = ' ';
	}

	if ( DGetConsoleWindow != NULL ){
		hHostWnd = DGetConsoleWindow();
	}else{
		thprintf(caption, TSIZEOF(caption), T("+%X)"), GetCurrentThreadId());
		SetConsoleTitle(caption);		// 検出用タイトル表示
	}
										// 標準入出力ハンドルの取得
	hStdin	= GetStdHandle(STD_INPUT_HANDLE);
	hStdout	= GetStdHandle(STD_OUTPUT_HANDLE);
										// 標準入出力状態の取得
	GetConsoleMode(hStdin, &OldStdinMode);
	GetConsoleMode(hStdout, &OldStdoutMode);

	#ifndef UNICODE
	if ( OSver.dwPlatformId != VER_PLATFORM_WIN32_NT ){
		PPxStdinMode = CONSOLE_IN_FLAGS_DEF;
	}
	#endif
	if ( OSver.dwBuildNumber >= WINBUILD_10_RS2 ){
		PPxStdoutMode = CONSOLE_OUT_FLAGS_VT;
	}

	tInitCustomize();
									// 標準出力のカーソル状態
	if ( GetConsoleCursorInfo(hStdout, &OldCsrMod) == FALSE ){
		return FALSE; // Wineのとき
	}
	NowCsr.dwSize = OldCsrMod.dwSize;
	NowCsr.bVisible = OldCsrMod.bVisible;

	buf[0] = '\0';
	if ( GetConsoleOutputCP() == 0 ){ // ターミナル上のときは不明
		ConsoleOnRemote = TRUE;
		UseGUI = FALSE;
	}else if ( (ExpandEnvironmentStrings(T("%TERM%"), buf, TSIZEOF(buf)) != 0) &&
		(buf[0] != '%') &&
		(tstricmp(buf, T("dumb")) != 0) ){ // ターミナル上
		ConsoleOnRemote = TRUE;
		UseGUI = FALSE;
	}else{
		// full screen であれば FALSE (XP/2003 以前)
		if ( DGetConsoleDisplayMode == INVALID_HANDLE_VALUE ){
			GETDLLPROC(hKernel32, GetConsoleDisplayMode);
		}
		if ( DGetConsoleDisplayMode != NULL ){
			DWORD displaymode;

			if ( (DGetConsoleDisplayMode(&displaymode) == FALSE) ||
				 (displaymode & CONSOLE_FULLSCREEN_HARDWARE) ){
				ConsoleOnRemote = TRUE;
				UseGUI = FALSE;
			}
		}
	}

	if ( ConsoleOnRemote ){
		PPxCommonExtCommand(K_ConsoleMode, ConsoleMode_ConsoleOnly);
		UseMouse = ConsoleMouse_TINY;
	}else{
		UseMouse = ConsoleMouse_FULL;
	}

	ExpandEnvironmentStrings(T("%WT_SESSION%"), buf, TSIZEOF(buf));
	if ( (buf[0] != '%') && (UseMouse != ConsoleMouse_DISABLE) ){
		UseMouse = ConsoleMouse_TINY;
		// DGetConsoleWindow で取得できる PseudoConsoleWindow は、表示に使われないウィンドウなので使わない
		DGetConsoleWindow = NULL;
		hHostWnd = NULL;
		thprintf(caption, TSIZEOF(caption), T("+%X)"), GetCurrentThreadId());
		SetConsoleTitle(caption);		// 検出用タイトル表示
	}

	if ( OSver.dwBuildNumber >= WINBUILD_11_21H1 ){ // Windows11 は強制 GUI 無効化
		// HKEY_CURRENT_USER\Console\%%Startup\DelegationConsole とかで
		// 一応識別できるかも。
		UseGUI = FALSE;
		UseMouse = ConsoleMouse_DISABLE;
	}

	tConsoleInit(); // 標準入力の設定
	TermColor.raw[TC_default] = screen.info.wAttributes;
	{

	#ifdef UNICODE
		if ( UseGUI && (buf[0] == '%') )
	#else
		if ( UseGUI && (buf[0] == '%') && (OSver.dwPlatformId == VER_PLATFORM_WIN32_NT) )
	#endif
		{ // conhost.exe なら利用可能。wt.exe の場合は現在ウィンドウの変化が無いので無効
			COORD WindowSize[2] = {{80, 200},{80, 30}};
												// ウィンドウの大きさを変更
			GetCustData(T("XB_size"), &WindowSize, sizeof(WindowSize));
			SetConsoleScreenBufferSize(hStdout, WindowSize[0]);

			screen.info.srWindow.Right =  (SHORT)(screen.info.srWindow.Left + WindowSize[1].X - 1);
			screen.info.srWindow.Bottom = (SHORT)(screen.info.srWindow.Top  + WindowSize[1].Y - 1);
			SetConsoleWindowInfo(hStdout, TRUE, &screen.info.srWindow);
		}
	}

	if ( hHostWnd == NULL ){
										// 自分のコンソールの HWND を取得する
		if ( DGetConsoleWindow == NULL ){
			int i;

			for ( i = 0 ; i < 30 ; i++ ){
				hHostWnd = FindWindow(NULL, caption);
				if ( hHostWnd != NULL ) break;
				Sleep(100);
			}
		}
		if ( hHostWnd == NULL ) hHostWnd = BADHWND;
	}

	buf[0] = '\0';
	if ( hHostWnd != BADHWND ){
		GetClassName(hHostWnd, buf, TSIZEOF(buf)); // conhost: ConsoleWindowClass / wt,ssh: PseudoConsoleWindow / wine: 存在しない
	}
	if ( tstrcmp(buf, T("ConsoleWindowClass")) == 0 ){
		xCONSOLE_SCREEN_BUFFER_INFOEX infoex;

		ConsoleHost = ConsoleHost_conhost;

		infoex.cbSize = sizeof(infoex);
		if ( (DGetConsoleScreenBufferInfoEx != NULL) &&
			 IsTrue(DGetConsoleScreenBufferInfoEx(hStdout, &infoex)) ){
			int i;
			BOOL change = FALSE;

			GetCustData(T("CB_pals"), &palettes, sizeof(palettes));
			for ( i = 0 ; i < 16 ; i++ ){
				if ( palettes[i] != C_AUTO ){
					infoex.ColorTable[i] = palettes[i];
					change = TRUE;
				}
			}
			if ( change ){
				// これを実行すると、WindowsTerminalのとき、画面最下行で
				// 複数行を出力してもスクロールしなくなる
				DSetConsoleScreenBufferInfoEx(hStdout, &infoex);
			}
		}
	}else if ( tstrcmp(buf, T("CASCADIA_HOSTING_WINDOW_CLASS")) == 0 ){
		ConsoleHost = ConsoleHost_WindowsTerminal;
		if ( X_cone == ConsoleColor_MIN ) X_cone = ConsoleColor_MAX;
	}else if ( tstrcmp(buf, T("PseudoConsoleWindow")) == 0 ){
		ConsoleHost = ConsoleHost_OpenConsole;
	}
	return TRUE;
}

TCHAR ESC_Fg_ColorConv[16][4] = {
	// 黒,   青,      緑,      水,      赤,      紫,      黄,      暗白,
	T("30"), T("34"), T("32"), T("36"), T("31"), T("35"), T("33"), T("37"),
	// 灰,  明青,     明緑,    明水,    明赤,    明紫,    明黄,    明白
	T("90"), T("94"), T("92"), T("96"), T("91"), T("95"), T("93"), T("97")
};

TCHAR ESC_Bg_ColorConv[16][4] = {
	// 黒,   青,      緑,      水,      赤,      紫,      黄,      暗白,
	T("40"), T("44"), T("42"), T("46"), T("41"), T("45"), T("43"), T("47"),
	// 灰,  明青,     明緑,    明水,    明赤,    明紫,    明黄,    明白
	T("100"),T("104"),T("102"),T("106"),T("101"),T("105"),T("103"),T("107")
};

void tSetAttr(WORD attr)
{
	TCHAR buf[100], *last;
	DWORD dummy;

	#ifndef WINEGCCx
	if ( X_cone >= ConsoleColor_ESC ){
		last = thprintf(buf, TSIZEOF(buf), CSI_T T("%s;%s%sm"),
				ESC_Fg_ColorConv[attr & 0xf],
				ESC_Bg_ColorConv[(attr >> 4) & 0xf],
				(attr & COMMON_LVB_UNDERSCORE) ? T(";4") : T(";24"));
		WriteConsole(hStdout, buf, last - buf, &dummy, NULL);
	}else{
		SetConsoleTextAttribute(hStdout, attr);
	}
	#else
		last = thprintf(buf, TSIZEOF(buf), CSI_T T("%s;%s%sm"),
				ESC_Fg_ColorConv[attr & 0xf],
				ESC_Bg_ColorConv[(attr >> 4) & 0xf],
				(attr & COMMON_LVB_UNDERSCORE) ? T(";4") : T(";24"));
		WriteConsole(hStdout, buf, last - buf, &dummy, NULL);
	#endif
}

void tSetColor(int colorindex)
{
	if ( X_cone >= ConsoleColor_MAX ){
		COLORREF fc, bc;
		TCHAR buf[TCE_colorlen], *last;
		DWORD tmp;

		fc = TCE_text(colorindex);
		bc = TCE_back(colorindex);
		if ( (fc < C_S_AUTO) && (bc < C_S_AUTO) ){
			last = thprintf(buf, TSIZEOF(buf),
					CSI_T T("38;2;%d;%d;%d")  // 前景色
					T(";48;2;%d;%d;%dm"), // 背景色
					fc & 0xff, (fc >> 8) & 0xff, (fc >> 16) & 0xff,
					bc & 0xff, (bc >> 8) & 0xff, (bc >> 16) & 0xff);
		}else{
			WORD attr;

			attr = TermColor.raw[colorindex];
			// 前景色
			if ( fc < C_S_AUTO ){
				last = thprintf(buf, TCE_textlen, CSI_T T("38;2;%d;%d;%d"),
						fc & 0xff, (fc >> 8) & 0xff, (fc >> 16) & 0xff);
			}else{
				last = thprintf(buf, TCE_textlen, CSI_T T("%s"),
						ESC_Fg_ColorConv[attr & 0xf]);
			}
			// 背景色
			if ( fc < C_S_AUTO ){
				last = thprintf(last, TCE_backlen, T(";48;2;%d;%d;%dm"),
						bc & 0xff, (bc >> 8) & 0xff, (bc >> 16) & 0xff);
			}else{
				last = thprintf(last, TCE_backlen, T(";%sm"),
						ESC_Bg_ColorConv[(attr >> 4) & 0xf]);
			}
		}
		WriteConsole(hStdout, buf, last - buf, &tmp, NULL);
		return;
	}
	tSetAttr(TermColor.raw[colorindex]);
}

void tSetDefaultColor(void)
{
	SetConsoleTextAttribute(hStdout, TermColor.raw[TC_default]);
}

#define TC_text(index) ConsoleColors[TermColor.raw[index] & 0xf]
#define TC_back(index) ConsoleColors[(TermColor.raw[index] >> 4) & 0xf]

COLORREF GetTCE_text(int index)
{
	COLORREF color;

	color = TCE_text(index);
	if ( color >= C_S_AUTO ) color = TC_text(index);
	return color;
}

COLORREF GetTCE_back(int index)
{
	COLORREF color;

	color = TCE_back(index);
	if ( color >= C_S_AUTO ) color = TC_back(index);
	return color;
}


COLORREF ConRefColor(COLORREF color)
{
	if ( (color >= C_S_EDIT_TEXT) && (color <= C_S_SELECT_BACK) ){
		switch(color){
			case C_S_EDIT_TEXT:
				color = GetTCE_text(TC_edit);
				break;

			case C_S_EDIT_BACK:
				color = GetTCE_back(TC_edit);
				break;

			case C_S_SELECT_TEXT:
				color = GetTCE_text(TC_edit_select);
				break;

			case C_S_SELECT_BACK:
				color = GetTCE_back(TC_edit_select);
				break;
		}
	}else{
		color = GetSchemeColor(color, C_S_AUTO);
	}
	return color;
}

void tRelease(void)
{
	SetConsoleTextAttribute(hStdout, TermColor.raw[TC_default]);
	SetConsoleCursorPosition(hStdout, screen.info.dwCursorPosition);

	SetConsoleCursorInfo(hStdout, &OldCsrMod);
	SetConsoleMode(hStdin, OldStdinMode);
	SetConsoleMode(hStdout, OldStdoutMode);
	#ifdef WINEGCCx
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
	}
	#endif
}

#ifdef WINEGCCx
void GetConsoleWindowInfo(HANDLE conout, CONSOLE_SCREEN_BUFFER_INFO *scrinfo)
{
	GetConsoleScreenBufferInfo(conout, &screen.info);
	getmaxyx(winewin, screen.info.dwSize.Y, screen.info.dwSize.X);
	getyx(winewin, screen.info.dwCursorPosition.Y, screen.info.dwCursorPosition.X);
	screen.info.srWindow.Bottom = 0;
	screen.info.srWindow.Top = 20;
}
#endif

void tClearScreen(void)
{
	if ( X_cone >= ConsoleColor_ESC ){
//		tputstr(CSI_T T("2J"));
		tputstr(ESC_T T("c"));
	}else{
		DWORD write;
		COORD xy = {0,0};

		FillConsoleOutputCharacter(hStdout, ' ',
				screen.info.dwSize.X * screen.info.dwSize.Y, xy,
				&write);
		FillConsoleOutputAttribute(hStdout, TermColor.raw[TC_default],
				screen.info.dwSize.X * screen.info.dwSize.Y, xy,
				&write);

		screen.info.dwCursorPosition.Y = 1;
	}
}

int ESC_csi(TCHAR *esc_codes, DWORD esc_len)
{
	switch ( esc_codes[1] ){
		case '<': // Tera term 拡張
			break;

		case '=': // DA3
			// ESC [ = 0 c →  DCS ! | xxxxxxxx ST
			break;

		case '>':
			switch ( esc_codes[esc_len - 1] ){
				case 'c': // DA2
				// ← CSI > 32 ; 331 ; 0 c
				case 'J': // 消去
					 // 短形消去 CSI > 3 ; top ; left ; bottom ; right J
				case 'K':
					 // 幅消去 CSI > 3 ; left ; right K
					 // 幅消去 CSI > 5 ; 3...6:線 12:文字色 K
				break;
			}
			break;

		case '?':
			switch ( esc_codes[esc_len - 1] ){
				case 'J': // DECSED
				case 'K': // DECSEL
				case 'h': // DECSET
				case 'i': // DECMC 印刷モード
				case 'l': // DECRST
				case 'n': // DECDSR
					// ← CSI ? 50 n
				case 'q': // DECSCUSR // カーソル形式

				case 'y': // DECRQM // CSI ? ps $ p の応答 CSI ? Ps1 ; Ps2 $ y
					break;
			}
			break;

		case '!':
//			if ( esc_codes[2] == 'p' ) // DECSTR  soft reset
			break;



		default:
			switch ( esc_codes[esc_len - 1] ){
				case '@': // UCH
					return ' ';

				case 'A': // CUU
					return K_up;

				case 'B': // CUD
					return K_dw;

				case 'C': // CUF
					return K_ri;

				case 'D': // CUB
					return K_lf;

//				case 'E': // CNL
//					return K_dw; // dw + home

				case 'F': // CNL
					return K_end;

				case 'G': // CHA
					return K_home;

				case 'H': //
					return K_home;

				case 'S': // SU
					if ( tstrcmp(esc_codes + 2, T("1;3S")) == 0 ) return K_a | K_F4;
					return K_home;


				case 'q':
				case 'y': // DECRQM // CSI ps $ p の応答 CSI Ps1 ; Ps2 $ y
					break;
			}
			break;
	}
	XMessage(NULL, NULL, XM_DbgCon, T("*ESC csi %s*"), esc_codes);
	return 0;
}

/*-----------------------------------------------------------------------------
	キー入力処理
-----------------------------------------------------------------------------*/
/*--------------------------------------
	キー入力

	inf は NULL でもよい
--------------------------------------*/
#define ESC_SIZE 32
int tgetc(INPUT_RECORD *cin)
{
	return tgetin(cin, INFINITE);
}

int tgetin(INPUT_RECORD *inf, DWORD timeout)
{
	INPUT_RECORD con;
	DWORD key = 0;
	TCHAR esc_codes[ESC_SIZE], esc_len = 0;

#ifndef WINEGCCx
	DWORD read;
	HANDLE hWait[2];

	hWait[0] = hStdin;
	hWait[1] = hCommSendEvent;

	for ( ; ; ){
		read = WaitForMultipleObjects(2, hWait, FALSE, timeout);
		if ( read == WAIT_TIMEOUT ){
			return KEY_TIMEOUT;
		}else if ( read == WAIT_OBJECT_0 + 1 ){ // SendPPB [5]
			if ( inf != NULL ) inf->EventType = 0;
			return KEY_RECV;
		}else if ( read == WAIT_OBJECT_0 ){				// 読み込み
			con.EventType = 0;
			ReadConsoleInput(hStdin, &con, 1, &read);
			if ( read == 0 ) continue;
			switch( con.EventType ){
				case KEY_EVENT: // conhost / wt / ssh
				// ssh x apps,全半,無変換,変換,scroll lock,num lock
				// ssh o pause
#ifdef ___DEBUG
			{
				TCHAR buf[300];

				thprintf(buf, TSIZEOF(buf), T("Key: Shift:%x/Vkey:%x/Uni:%x"),
					con.Event.KeyEvent.dwControlKeyState,
					con.Event.KeyEvent.wVirtualKeyCode,
					con.Event.KeyEvent.uChar.UnicodeChar);
				tputposstr(0, screen.info.dwCursorPosition.Y - 1, buf);
				XMessage(NULL, NULL, XM_DbgLOG, T("%s"), buf);
			}
#endif
					if ( con.Event.KeyEvent.bKeyDown == FALSE ){
/* terminal 用
						if ( con.Event.KeyEvent.wVirtualKeyCode == 27 ){
							key = K_esc;
							esc_len = 0;
						}else{
							continue;
						}
*/
						continue;
					}else{
					#ifdef UNICODE
						key = con.Event.KeyEvent.uChar.UnicodeChar;
						if ( key > 0xff ){
							key = (key & 0xff) | ((key & 0xff00) << 8);
						}
					#else
						key = con.Event.KeyEvent.uChar.AsciiChar;
						if ( key >= 0xffff80 ) key &= 0xff;	// S-JIS の処理
					#endif
					}
/*
					if ( key == 0 ){ // NUL 無視する
						continue;
					}
*/
					if ( (con.Event.KeyEvent.dwControlKeyState &
							(RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) &&
							(key > 0) && (key < 32) ){
						key += '@';
					}

					if ( key < 32 ){
/*
						if ( key == 27 ){ // ESC シーケンス開始
							esc_codes[0] = 27;
							esc_len = 1;
							continue;
						}
*/
						key = K_v | con.Event.KeyEvent.wVirtualKeyCode;
					}else {
						if ( key == 0x7f ) key = K_bs;
					}
					if ( con.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED){
						key |= K_s;
					}
					if ( con.Event.KeyEvent.dwControlKeyState &
							(RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED) ){
						if ( (key <= 'z') && ( key >= 'a' ) ) key -= 0x20;
						key |= K_a;
					}
					if ( con.Event.KeyEvent.dwControlKeyState &
							(RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED) ){
						if ( key < 32 ) key += '@';
						key |= K_c;
					}

					if ( esc_len > 0 ){
						if ( (key <= 0x20) || (key >= 0x7f) || (esc_len >= (ESC_SIZE - 1)) ){
							esc_codes[esc_len] = '\0';
							XMessage(NULL, NULL, XM_DbgLOG, T("*ESC error : %s*"), esc_codes);
							esc_len = 0;
							continue;
						}
						if ( esc_len == 1 ){
							switch ( key ){
								case '6': // DECBI ←
									key = K_lf;
									break;

								case '7': // DECSC カーソル位置保存
								case '8': // DECRC カーソル位置復帰
									break;

								case '9': // DECFI →
									key = K_ri;
									break;

								case '=': // DECKPAM テンキーをアプリケーションキーパッド(ESCOx)
								case '>': // DECKPNM テンキーをノーマルキーパッド(123)
									break;

								case 'D': // IND ↓
									key = K_dw;
									break;

								case 'E': // NUL 次の行の先頭
								case 'H': // HTS タブストップ設定
									break;

								case 'M': // RI  ↑
									key = K_up;
									break;

								case 'O':
									esc_codes[esc_len++] = (TCHAR)key;
									continue;

								case 'Z': // DECID 端末ID

								case 'c': // RIS 端末リセット
								case 'g': //     可視ベル

									XMessage(NULL, NULL, XM_DbgCon, T("\r\n*ESC %c*"), esc_codes[1]);
									break;

								case 'P': // DCS ～ST(0x9c or ESC \)
								case 'X': // SOS ～ST(0x9c or ESC \)
								case '[': // CSI
								case ']': // OSC ～ST(0x9c or ESC \)
								case '^': // PM  ～ST(0x9c or ESC \)
								case '_': // APC ～ST(0x9c or ESC \)
									esc_codes[esc_len++] = (TCHAR)key;
									continue;
							}
						}else if ( esc_codes[1] == '[' ){ // CSI
							esc_codes[esc_len++] = (TCHAR)key;
							esc_codes[esc_len] = '\0';
							if ( (key >= 0x21) && (key <= 0x3f) ) continue;
							key = ESC_csi(esc_codes, esc_len);
//							if ( key == -1 ) continue;
//							esc_len = 0;
						}else{ // DCS / SOS / OSC / PM / APC
							esc_codes[esc_len++] = (TCHAR)key;
							esc_codes[esc_len] = '\0';
							if ( esc_len >= 3 ){
								if ( (esc_codes[esc_len - 1] == '\\') && (esc_codes[esc_len - 2] == 27) ){ // ST 終了
									XMessage(NULL, NULL, XM_DbgCon, T("\r\n*Extra %s*"), esc_codes);
									esc_len = 0;
								}
								if ( esc_codes[1] == 'O' ){
									switch(key){
										case 'P':
											key = K_F1;
											break;
										case 'Q':
											key = K_F2;
											break;
										case 'R':
											key = K_F3;
											break;
										case 'S':
											key = K_F4;
											break;
									}
								}
							}
							continue;
						}
					}
					break;
#ifdef DEBUG___
				case MOUSE_EVENT: { // conhost / wt / x ssh / 非推奨
					TCHAR buf[300];

					thprintf(buf, TSIZEOF(buf), T("Mouse: (%d,%d)%x/%x/%x"),
						con.Event.MouseEvent.dwMousePosition.X,
						con.Event.MouseEvent.dwMousePosition.Y,
						con.Event.MouseEvent.dwButtonState,
						con.Event.MouseEvent.dwControlKeyState,
						con.Event.MouseEvent.dwEventFlags);
					if ( con.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED ){
/*
						int wheel;
						wheel = (int)(short)HIWORD(con.Event.MouseEvent.dwButtonState);
						SendMessage(GetParent(hMainWnd), WM_VSCROLL, SB_LINEDOWN, 0);
*/
						tstrcat(buf,T(" wh"));
					}
					tputposstr(0, screen.info.dwCursorPosition.Y - 1, buf);
					XMessage(NULL, NULL, XM_DbgLOG, T("%s"), buf);

					break;
				}
				case WINDOW_BUFFER_SIZE_EVENT: { // conhost / wt / ssh
					TCHAR buf[300];

					thprintf(buf, TSIZEOF(buf), T("WINDOW_BUFFER_SIZE_EVENT: (%d,%d)"),
						con.Event.WindowBufferSizeEvent.dwSize.X,
						con.Event.WindowBufferSizeEvent.dwSize.Y);
					tputposstr(0, screen.info.dwCursorPosition.Y - 1, buf);
					XMessage(NULL, NULL, XM_DbgLOG, T("%s"), buf);
					break;
				}
				case MENU_EVENT: { // conhostのみ 非推奨
					TCHAR buf[300];

					thprintf(buf, TSIZEOF(buf), T("MENU_EVENT: %d"),
						con.Event.MenuEvent.dwCommandId);
					tputposstr(0, screen.info.dwCursorPosition.Y - 1, buf);
					XMessage(NULL, NULL, XM_DbgLOG, T("%s"), buf);
					break;
				}
				case FOCUS_EVENT: { // conhostのみ
					TCHAR buf[300];

					thprintf(buf, TSIZEOF(buf), T("FOCUS_EVENT: %d"),
						con.Event.FocusEvent.bSetFocus);
					tputposstr(0, screen.info.dwCursorPosition.Y - 1, buf);
					XMessage(NULL, NULL, XM_DbgLOG, T("%s"), buf);
					break;
				}
				default: {
					TCHAR buf[300];

					thprintf(buf, TSIZEOF(buf), T("unknown_EVENT: %d"),
						con.EventType);
					tputposstr(0, screen.info.dwCursorPosition.Y - 1, buf);
					XMessage(NULL, NULL, XM_DbgLOG, T("%s"), buf);
				}
#else
				case MOUSE_EVENT:
					break;
				case WINDOW_BUFFER_SIZE_EVENT:
					GetConsoleWindowInfo(hStdout, &screen.info);
					break;
				default:
					continue;
#endif
			}
			break;
		}else{
			XMessage(NULL, NULL, XM_FaERRld, MES_FBDW);
			PostMessage(hHostWnd, WM_CLOSE, 0, 0);
		}
	}
	if ( inf != NULL ) *inf = con;
	#ifndef WINEGCC
		GetConsoleWindowInfo(hStdout, &screen.info);
	#endif
#else
	key = getch();
	if ( inf != NULL ){
		inf->EventType = KEY_EVENT;
	}
	switch ( key ){ // 1-31(^Cを除く) Ctrl+A
		case FFD: // 入力無し
			if ( WaitForSingleObject(hCommSendEvent, 0) == WAIT_OBJECT_0 ){ // SendPPB [5]
				if ( inf != NULL ) inf->EventType = 0;
				return KEY_RECV;
			}
			return 0;

		case 10:
		case KEY_ENTER:
			key = K_cr;
			break;

		case 0x14b: return K_ins;
		case 0x12e: return K_del;
		case 0x17f: return K_s | K_del;
		case 0x153: return K_Pup;
		case 0x152: return K_Pdw;
		case 0x161: return K_s | K_tab;

		case 27: {
			TCHAR extkeys[100], *extmax, *extp;

			extmax = extkeys + 99;
			for ( extp = extkeys ; extp < extmax ; extp++ ){
				key = getch();
				if ( key == FFD ){
					break;
				}
				*extp = key;
				if ( IsupperA(key) ){
					extp++;
					key = 0;
					break;
				}
			}
			*extp = '\0';
			if ( key == FFD ) return K_esc; // ESC 単独
			if ( tstrcmp(extkeys, T("[3;5~")) == 0 ) return K_c | K_del;
			if ( tstrcmp(extkeys, T("[5;5~")) == 0 ) return K_c | K_Pup;
			if ( tstrcmp(extkeys, T("[6;5~")) == 0 ) return K_c | K_Pdw;
			if ( tstrcmp(extkeys, T("[1;2A")) == 0 ) return K_s | K_up;
			if ( tstrcmp(extkeys, T("[1;2B")) == 0 ) return K_s | K_dw;
			if ( tstrcmp(extkeys, T("[1;5A")) == 0 ) return K_c | K_up;
			if ( tstrcmp(extkeys, T("[1;5B")) == 0 ) return K_c | K_dw;
			if ( tstrcmp(extkeys, T("[1;5D")) == 0 ) return K_c | K_lf;
			if ( tstrcmp(extkeys, T("[1;5C")) == 0 ) return K_c | K_ri;
			// 未知 ESC
			getyx(winewin, screen.info.dwCursorPosition.Y, screen.info.dwCursorPosition.X);
			tputposstr(40, 0, extkeys);
			tCsrPos(screen.info.dwCursorPosition.X, screen.info.dwCursorPosition.Y);
			return 0;
		}
//		case 'Q':
//			key = K_esc;
//			break;

		case KEY_DOWN:
			key = K_dw;
			break;

//		case KEY_SDOWN:
//			key = K_s | K_dw;
//			break;

		case KEY_UP:
			key = K_up;
			break;

//		case KEY_SUP:
//			key = K_s | K_up;
//			break;

		case KEY_LEFT:
			key = K_lf;
			break;

		case KEY_SLEFT:
			key = K_s | K_lf;
			break;

		case KEY_RIGHT:
			key = K_ri;
			break;

		case KEY_SRIGHT:
			key = K_s | K_ri;
			break;

		case KEY_HOME:
			key = K_home;
			break;

		case KEY_BACKSPACE:
			key = K_bs;
			break;

		case KEY_DC:
			key = K_del;
			break;

		case KEY_F(1):
			key = K_F1;
			break;

		case KEY_F(4):
			key = K_F4;
			break;

		case KEY_END:
			key = K_end;
			break;

		default:
			if ( (key >= 1) && (key <= 26) ){
				key += K_c | '@';
				break;
			}
	}
#if 0
	{
		TCHAR buf[100];

		getyx(winewin, screen.info.dwCursorPosition.Y, screen.info.dwCursorPosition.X);
		thprintf(buf, TSIZEOF(buf), T("%03d  0x%03x"), key, key);
		tputposstr(0, 0, buf);
		tCsrPos(screen.info.dwCursorPosition.X, screen.info.dwCursorPosition.Y);
	}
#endif
#endif
	return key;
}
/*-----------------------------------------------------------------------------
	カーソル処理
-----------------------------------------------------------------------------*/
/*--------------------------------------
	カーソルの座標を指定する
--------------------------------------*/
void tCsrPos(int x, int y)
{
	DWORD dummy;
#ifndef WINEGCCx
	if ( X_cone >= ConsoleColor_ESC ){
		TCHAR buf[32], *last;

		last = thprintf(buf, TSIZEOF(buf), CSI_T T("%d;%dH"), y + 1, x + 1);
		WriteConsole(hStdout, buf, last - buf, &dummy, NULL);
	}else{
		COORD xy;

		xy.X = (SHORT)x;
		xy.Y = (SHORT)y;
		SetConsoleCursorPosition(hStdout, xy);
	}
#else
	char abuf[32];

	dummy = wsprintfA(abuf, CSI_A "%d;%dH", y, x + 1);
	WriteFile(hStdout, abuf, dummy, &dummy, NULL);
#endif
}
/*--------------------------------------
	カーソルを表示する
	-1		消去する
	 0		以前の大きさで表示
	 1-100	指定した割合で表示
--------------------------------------*/
CONSOLE_CURSOR_INFO clearcur = { 1 , FALSE };
void tCsrMode(int size)
{
	if ( size < 0 ){
		SetConsoleCursorInfo(hStdout, &clearcur);
		return;
	}
	if ( size ) NowCsr.dwSize = size;
	SetConsoleCursorInfo(hStdout, &NowCsr);
}
/*-----------------------------------------------------------------------------
	文字表示処理
-----------------------------------------------------------------------------*/
/*--------------------------------------
	座標を指定して表示
--------------------------------------*/
void tputposstr(int x, int y, const TCHAR *str)
{
	DWORD dummy;

	#ifndef WINEGCC
		tCsrPos(x, y);
		WriteConsole(hStdout, str, tstrlen32(str), &dummy, NULL);
	#else
		char abuf[32];
		dummy = wsprintfA(abuf, CSI_A "%d;%dH", y + 1, x + 1);
		WriteFile(hStdout, abuf, dummy, &dummy, NULL);
		WriteConsole(hStdout, str, tstrlen32(str), &dummy, NULL);
	#endif
}

/*--------------------------------------
	文字列の表示
--------------------------------------*/
void tputstr(const TCHAR *str)
{
	#ifndef WINEGCCx
		DWORD dummy;

		if ( WriteConsole(hStdout, str, tstrlen32(str), &dummy, NULL) == FALSE ){
			WriteFile(hStdout, str, TSTRLENGTH32(str), &dummy, NULL);
		}
	#else
		#ifdef UNICODE
			char strA[CMDLINESIZE];

			if ( UnicodeToAnsi(str, strA, sizeof(strA)) == 0 ){
				size_t length;
				char *extbufA;

				length = UnicodeToAnsi(str, NULL, 0) + 1;
				if ( length > 1 ){
					extbufA = HeapAlloc(hProcHeap, 0, length);
					if ( extbufA != NULL ){
						UnicodeToAnsi(str, extbufA, length);
						addstr(extbufA);
						HeapFree(hProcHeap, 0, extbufA);
						refresh();
						return;
					}
				}
				return;
			}
			addstr(strA);
			refresh();
		#else
			addstr(str);
			refresh();
		#endif
	#endif
}

void tputstr_noinit(const TCHAR *str)
{
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	tputstr(str);
}

/*-----------------------------------------------------------------------------
	ボックス処理
-----------------------------------------------------------------------------*/
/*--------------------------------------
	指定範囲の表示属性を変更する
		(x1, y1)-(x2, y2), atr
--------------------------------------*/
void tFillAtr(int x1, int y1, int x2, int y2, int colorindex)
{
	#ifndef WINEGCCx
		COORD xy;
		DWORD dummy;

		xy.X = (SHORT)x1;
		xy.Y = (SHORT)y1;
		for ( ; xy.Y <= y2 ; xy.Y++ ){
			FillConsoleOutputAttribute(hStdout, TermColor.raw[colorindex],
					(DWORD)(x2 - x1) + 1, xy, &dummy);
		}
	#else
		BOOL negmode;
		DWORD atr;

		atr = TermColor.raw[colorindex];
		negmode = ((atr >> 4) & 0xf) > (atr & 0xf);
		tCsrPos(x1, y1);
		chgat(x2 - x1 + 1, 0, negmode ? NegPair : DefPair, NULL);
		tCsrPos(screen.info.dwCursorPosition.X, screen.info.dwCursorPosition.Y);
	#endif
}

void LongWriteSpace(int len)
{
	DWORD dummy;

	for(;;){
		if ( len < SpaceBuffSize ){
			WriteConsole(hStdout, SpaceBuff, len, &dummy, NULL);
			return;
		}
		WriteConsole(hStdout, SpaceBuff, SpaceBuffSize, &dummy, NULL);
		len -= SpaceBuffSize;
	}
}

void tFillSpace(int left, int top, int right, int bottom)
{
	DWORD dummy;

	if ( X_cone >= ConsoleColor_ESC ){
		if ( X_cone >= ConsoleColor_MAX ){
			TCHAR buf[64], *last;

			last = thprintf(buf, TSIZEOF(buf),
					CSI_T T("%d;%d;%d;%d$z"), // 四角形
					top + 1, left + 1, bottom + 1, right + 1);
			WriteConsole(hStdout, buf, last - buf, &dummy, NULL);
		}else{
			TCHAR *dest, *destmax;
			int fillright = LocateBuffSize, len;

			right = right - left + 1;
			top++;
			left++;
			bottom++;
			while ( top <= bottom ){
				dest = thprintf(LocateBuff, LocateBuffSize, CSI_T T("%d;%dH"), top, left);
				len = dest - LocateBuff;
				destmax = LocateBuff + ((right < fillright) ? right : fillright);
				while (dest < destmax) *dest++ = ' ';
				if ( right <= SpaceBuffSize ){
					WriteConsole(hStdout, LocateBuff, len + right, &dummy, NULL);
				}else{
					WriteConsole(hStdout, LocateBuff, len + SpaceBuffSize, &dummy, NULL);
					LongWriteSpace(right - SpaceBuffSize);
				}
				top++;
			}
		}
	}else{
		COORD xy;

		right = right - left + 1;
		xy.Y = (SHORT)top;
		xy.X = (SHORT)left;
		while ( xy.Y <= bottom ){
			SetConsoleCursorPosition(hStdout, xy);
			if ( right <= SpaceBuffSize ){
				WriteConsole(hStdout, SpaceBuff, right, &dummy, NULL);
			}else{
				LongWriteSpace(right);
			}
			xy.Y++;
		}
	}
}

void tFillBox(int left, int top, int right, int bottom, int colorindex)
{
	if ( X_cone >= ConsoleColor_MAX ){
		TCHAR linebuf[96], *last;
		COLORREF cl = GetTCE_back(colorindex);
		DWORD tmp;

		last = thprintf(linebuf, TSIZEOF(linebuf),
				CSI_T T("48;2;%d;%d;%dm") // 背景色
				CSI_T T("%d;%d;%d;%d$z"), // 四角形
				cl & 0xff, (cl >> 8) & 0xff, (cl >> 16) & 0xff,
				top + 1, left + 1, bottom + 1, right + 1);
		WriteConsole(hStdout, linebuf, last - linebuf, &tmp, NULL);
	}else{
		tSetColor(colorindex);
		tFillSpace(left, top, right, bottom);
	}
}

#ifdef UNICODE
	#define CONBOX_H_BAR  '-'
	#define CONBOX_V_BAR  L"|"
	#define CONBOX_LT  '+'
	#define CONBOX_RT  '+'
	#define CONBOX_LB  '+'
	#define CONBOX_RB  '+'
/*
	#define CONBOX_H_BAR  0x2501
	#define CONBOX_V_BAR  L"|"
	#define CONBOX_LT  0x250f
	#define CONBOX_RT  0x2513
	#define CONBOX_LB  0x2517
	#define CONBOX_RB  0x251b
*/
#else
	#if 1
		#define CONBOX_H_BAR  '-'
		#define CONBOX_V_BAR  "|"
		#define CONBOX_LT  '+'
		#define CONBOX_RT  '+'
		#define CONBOX_LB  '+'
		#define CONBOX_RB  '+'
	#else //	Shift_JIS
		#define CONBOX_H_BAR  '\6'
		#define CONBOX_V_BAR  "\5"
		#define CONBOX_LT  '\1'
		#define CONBOX_RT  '\2'
		#define CONBOX_LB  '\3'
		#define CONBOX_RB  '\4'
	#endif
#endif

/*--------------------------------------
	ボックスを描く
--------------------------------------*/
void tFrameBox(int left, int top, int right, int bottom, int frameopt)
{
	COORD xy;
	DWORD dummy;
	TCHAR linebuf[192], *line = linebuf;
	int i, linesize;

	if ( !X_dwfr ){ // 枠無し描画
		tFillBox(left, top, right, bottom, TC_popup_item);
		return;
	}
	// 枠あり描画

	// 線で囲む
	linesize = right - left + 2;
	if ( linesize > TSIZEOF(linebuf) ){
		line = HeapAlloc(hProcHeap, 0 , TSTROFF(linesize));
		if ( line == NULL ){
			tFillBox(left, top, right, bottom, TC_popup_item);
			return;
		}
	}

	tSetColor(TC_popup_frame);
										// 左右
	for ( i = frameopt ? top : top + 1 ; i < bottom ; i++ ){
		xy.Y = (SHORT)i;
		xy.X = (SHORT)left;
		SetConsoleCursorPosition(hStdout, xy);
		WriteConsole(hStdout, CONBOX_V_BAR, 1, &dummy, NULL);
		xy.X = (SHORT)right;
		SetConsoleCursorPosition(hStdout, xy);
		WriteConsole(hStdout, CONBOX_V_BAR, 1, &dummy, NULL);
	}

	for ( i = 1 ; i < (right - left) ; i++) line[i] = CONBOX_H_BAR;
	line[i++] = '\0';

	xy.X = (SHORT)left;
	if ( !frameopt ){									// 上
		xy.Y = (SHORT)top;
		line[0] = CONBOX_LT;
		line[i - 1] = CONBOX_RT;
		SetConsoleCursorPosition(hStdout, xy);
		WriteConsole(hStdout, line, i, &dummy, NULL);
	}
										// 下角
	xy.Y = (SHORT)bottom;
	line[0] = CONBOX_LB;
	line[i - 1] = CONBOX_RB;
	SetConsoleCursorPosition(hStdout, xy);
	WriteConsole(hStdout, line, i, &dummy, NULL);

	tFillBox(left + 1, frameopt ? top : top + 1, right - 1, bottom - 1, TC_popup_item);
	if ( line != linebuf ) HeapFree(hProcHeap, 0, line);
}
/*--------------------------------------
	指定範囲を保存する
--------------------------------------*/
void tStore(int x1, int y1, int x2, int y2, CHAR_INFO **ptr)
{
	COORD xy0, xy1 = {0, 0};
	SMALL_RECT xy;

	xy0.X = (SHORT)(x2 - x1 + 1);
	xy0.Y = (SHORT)(y2 - y1 + 1);
	xy.Left = (SHORT)x1;
	xy.Right = (SHORT)x2;
	xy.Top = (SHORT)y1;
	xy.Bottom = (SHORT)y2;

	*ptr = HeapAlloc(hProcHeap, 0 , xy0.X * xy0.Y * sizeof(CHAR_INFO));
	if ( *ptr == NULL ) return;
	ReadConsoleOutput(hStdout, *ptr, xy0, xy1, &xy);
}
/*--------------------------------------
	指定範囲を復旧する
--------------------------------------*/
void tRestore(int x1, int y1, int x2, int y2, CHAR_INFO **ptr)
{
	COORD xy0, xy1 = {0, 0};
	SMALL_RECT xy;

	if ( *ptr == NULL ) return;
	xy0.X = (SHORT)(x2 - x1 + 1);
	xy0.Y = (SHORT)(y2 - y1 + 1);
	xy.Left = (SHORT)x1;
	xy.Right = (SHORT)x2;
	xy.Top = (SHORT)y1;
	xy.Bottom = (SHORT)y2;

	WriteConsoleOutput(hStdout, *ptr, xy0, xy1, &xy);
	HeapFree( hProcHeap, 0, *ptr);
}

#define MESIZE 14
void InitTMenuInfo(TMENUINFO *tmi)
{
	GetConsoleWindowInfo(hStdout, &tmi->oldoutinfo);
	GetConsoleCursorInfo(hStdout, &tmi->oldoutcsr);

	tmi->screensize.cx = tmi->oldoutinfo.srWindow.Right - tmi->oldoutinfo.srWindow.Left;
	tmi->screensize.cy = tmi->oldoutinfo.srWindow.Bottom - tmi->oldoutinfo.srWindow.Top;

#if 0 // 中央表示
	tmi->window.x = ((tmi->oldoutinfo.info.srWindow.Right  - tmi->size.cx - tmi->size.cx - 2) >> 1) + tmi->size.cx;
	tmi->window.y = ((tmi->oldoutinfo.info.srWindow.Bottom - tmi->size.cy - tmi->size.cy - 2) >> 1) + tmi->size.cy;
#else
	tmi->window.x = tmi->oldoutinfo.dwCursorPosition.X - 1;
	tmi->window.y = tmi->oldoutinfo.dwCursorPosition.Y + 1;
#endif

	if ( (tmi->size.cy + tmi->window.y) >= tmi->screensize.cy ){
		tmi->size.cy = tmi->screensize.cy - tmi->window.y - 3;
	}
	if ( tmi->size.cy < MESIZE ) tmi->size.cy = MESIZE;
	if ( tmi->size.cy >= tmi->total ) tmi->size.cy = tmi->total;
	if ( tmi->size.cy > (tmi->screensize.cy - 3) ) tmi->size.cy = tmi->screensize.cy - 3;

	if ( (tmi->window.x + tmi->size.cx + 1) >= tmi->oldoutinfo.srWindow.Right ){
		tmi->window.x = tmi->oldoutinfo.srWindow.Right - tmi->size.cx - 1;
	}
	if ( tmi->window.x < tmi->oldoutinfo.srWindow.Left ){
		tmi->window.x = tmi->oldoutinfo.srWindow.Left;
	}

	if ( (tmi->window.y + tmi->size.cy + 1) >= tmi->oldoutinfo.srWindow.Bottom ){
		tmi->window.y = tmi->oldoutinfo.srWindow.Bottom - tmi->size.cy - 3;
	}
	if ( tmi->window.y < tmi->oldoutinfo.srWindow.Top ){
		tmi->window.y = tmi->oldoutinfo.srWindow.Top;
	}

	tmi->index = tmi->showindex = tmi->mousebutton = 0;
	tmi->draw = TMI_DRAW_ALL_REDRAW;
	tmi->oldmousepos.x = -0x7fffffff;
}

void EndTMenuInfo(TMENUINFO *tmi)
{
	SetConsoleCursorInfo(hStdout, &tmi->oldoutcsr);
	tSetAttr(tmi->oldoutinfo.wAttributes);
}

void ClearTMenuInfoArea(TMENUINFO *tmi, BOOL popup)
{
	if ( popup ){
		tFrameBox(tmi->window.x , tmi->window.y ,
				tmi->window.x + tmi->size.cx, tmi->window.y + tmi->size.cy + 1,
				0);
		tSetAttr(tmi->oldoutinfo.wAttributes);
	}else{
		tSetAttr(tmi->oldoutinfo.wAttributes);
		tFillSpace(tmi->window.x, tmi->window.y,
				tmi->window.x + tmi->size.cx, tmi->window.y + tmi->size.cy + 1);
	}
}

int TMenuInfoKey(TMENUINFO *tmi)
{
	INPUT_RECORD cin;
	int key;

	do{
		key = tgetin(&cin, INFINITE);
		if ( cin.EventType == MOUSE_EVENT ){
			if ( cin.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED ){
				if ( cin.Event.MouseEvent.dwButtonState & B31 ){
					if ( (tmi->showindex + tmi->size.cy) < tmi->total ){
						tmi->showindex += 3;
						tmi->draw = TMI_DRAW_ALL_REDRAW;
					}
				}else{
					if ( tmi->showindex > 0 ){
						if ( tmi->showindex > 3 ){
							tmi->showindex -= 3;
						}else{
							tmi->showindex = 0;
						}
						tmi->draw = TMI_DRAW_ALL_REDRAW;
						break;
					}
				}
				break;
			}else if ( ( cin.Event.MouseEvent.dwMousePosition.X > tmi->window.x) &&
				 ( cin.Event.MouseEvent.dwMousePosition.X < (tmi->window.x + tmi->size.cx)) &&
				 ( cin.Event.MouseEvent.dwMousePosition.Y > tmi->window.y) &&
				 ( cin.Event.MouseEvent.dwMousePosition.Y < (tmi->window.y + tmi->size.cy + 1))){
				POINT pos;

				GetCursorPos(&pos);
				if ( /*(pos.x != tmi->oldmousepos.x) ||*/ (pos.y != tmi->oldmousepos.y) ){
					tmi->oldmousepos = pos;
					tmi->index = cin.Event.MouseEvent.dwMousePosition.Y - tmi->window.y - 1 + tmi->showindex;
					tmi->draw = TMI_DRAW_CHANGE_CURSOR;
					key = K_s;
				}

				if ( cin.Event.MouseEvent.dwButtonState & 0x1f ){
					tmi->mousebutton = cin.Event.MouseEvent.dwButtonState & 0x1f;
				}else if ( tmi->mousebutton ){
					if ( tmi->mousebutton & 3 ){
						key = (tmi->mousebutton & 2) ? K_esc : K_cr;
						tmi->mousebutton = 0;
						break;
					}
					tmi->mousebutton = 0;
				}
			}else{
				if ( cin.Event.MouseEvent.dwButtonState & 0x1f ){
					tmi->mousebutton = cin.Event.MouseEvent.dwButtonState & 0x1f;
				}else if ( tmi->mousebutton ){
					key = K_esc;
					break;
				}
			}
		}
	}while( key == 0 );

	switch( key ){
		case K_s:
			return 0;

		case K_cr:					// CR
			return -1;

		case K_esc:					// ESC
			return -2;

		case K_up:					// ↑
			if ( tmi->index > 0 ){
				tmi->index--;
			}else{
				if ( tmi->total <= tmi->size.cy ){
					tmi->index = tmi->total - 1;
				}
			}
			tmi->draw = TMI_DRAW_CHANGE_CURSOR;
			break;

		case K_dw:					// ↓
			if ( tmi->index < (tmi->total - 1) ){
				tmi->index++;
			}else{
				if ( tmi->total <= tmi->size.cy ){
					tmi->index = 0;
				}
			}
			tmi->draw = TMI_DRAW_CHANGE_CURSOR;
			break;

		case K_Pup:					// PgUP
		case K_lf:					// ←
			if ( tmi->index > tmi->size.cy ){
				tmi->index -= tmi->size.cy;
				tmi->draw = TMI_DRAW_CHANGE_CURSOR;
				break;
			}
		// K_home へ
		case K_s | K_up:			// \↑
		case K_home:
			tmi->index = 0;
			tmi->draw = TMI_DRAW_CHANGE_CURSOR;
			break;

		case K_Pdw:					// PgDW
		case K_ri:					// →
			if ( (tmi->index + tmi->size.cy) < tmi->total ){
				tmi->index += tmi->size.cy;
				tmi->draw = TMI_DRAW_CHANGE_CURSOR;
				break;
			}
		// K_end へ
		case K_s | K_dw:			// \↓
		case K_end:
			tmi->index = tmi->total - 1;
			tmi->draw = TMI_DRAW_CHANGE_CURSOR;
			break;

		default:
			return key;
	}

	if ( tmi->index >= (tmi->showindex + tmi->size.cy) ){
		tmi->showindex = tmi->index - (tmi->size.cy - 1);
		tmi->draw = TMI_DRAW_ALL_REDRAW;
	}
	if ( tmi->index < tmi->showindex ){
		tmi->showindex = tmi->index;
		tmi->draw = TMI_DRAW_ALL_REDRAW;
	}
	return 0;
}

const TCHAR ScrollEnableChar[] = T("*");
const TCHAR ScrollDisableChar[] = T(" ");

void PrintMenuItem(int x, int y, int width, int color, const TCHAR *text)
{
	TCHAR buf[256], *last;

	tSetColor(color);

	if ( width > 254 ) width = 254;
	buf[0] = ' ';

	#ifdef UNICODE
	{
		const TCHAR *ptr;
		TCHAR chr;
		int left;

		left = width;
		ptr = MessageText(text);
		last = buf;
		for (;;){
			chr = *ptr++;
			*last++ = chr;
			if ( chr == '\0' ) break;
			left -= CCharWide(chr);
			if ( left <= 0 ) break;
		}
		while( left > 0 ){
			*last++ = ' ';
			left--;
		}
	}
	#else
	{
		int len;

		last = tstplimcpy(buf + 1, MessageText(text), width);
		len = width - (last - buf + 1);

		while( len > 0 ){
			*last++ = ' ';
			len--;
		}
	}
	#endif

	*last = '\0';
	tputposstr(x, y, buf);
}

void ChangeMenuItem(int x, int y, int width, int color, const TCHAR *text)
{
	if ( X_cone >= ConsoleColor_ESC ){
		PrintMenuItem(x + 1, y + 1, width, color, text);
	}else{
		tFillAtr(x + 1, y + 1, x + width - 1, y + 1, color);
	}
}

int Select(const TMENU *tmenu)
{
	TMENUINFO tmi;
	int menu_width = 0;	// 幅
	int key;		/*	キー入力＆フラグ -1:正常 -2:中止 -3:ページャ */
	CHAR_INFO *win;
	const TMENU *t;
	int ScrollPos = 0; // スクロールのつまみ
	DWORD oldin = MAX32;

	if ( UseGUI ){
		HMENU hMenu;
		DWORD tick;

		hMenu = CreatePopupMenu();
		while ( tmenu->mes != NULL ){
			AppendMenu(hMenu, MF_ES, tmenu->id, MessageText(tmenu->mes));
			tmenu++;
		}
		tick = GetTickCount();
		key = (int)PPxCommonExtCommand(K_CPOPMENU, (WPARAM)hMenu);
		DestroyMenu(hMenu);
		if ( (key != 0) || ((GetTickCount() - tick) > 80) ) return key;
		// GUI menu が表示できなくなったようなので、コンソールモードに移行
		UseGUI = FALSE;
		UseMouse = ConsoleMouse_DISABLE;
		PPxCommonExtCommand(K_ConsoleMode, ConsoleMode_ConsoleOnly);
	}
	t = tmenu;
												// 最大文字列長を求める
	for ( tmi.total = 0 ; t[tmi.total].mes != NULL ; tmi.total++ ){
		int len;
		#ifdef UNICODE
			const TCHAR *ptr;
			TCHAR chr;

			len = 0;
			ptr = MessageText(t[tmi.total].mes);
			for (;;){
				chr = *ptr;
				if ( chr == '\0' ) break;
				len += CCharWide(chr);
				ptr++;
			}
		#else
			len = tstrlen32(MessageText(t[tmi.total].mes));
		#endif
		if ( len > menu_width ) menu_width = len;
	}
	if ( tmi.total == 0 ) return 0;	/* 項目なし */
	tmi.size.cy = tmi.total - 1;
	tmi.size.cx = menu_width + 3;

	InitTMenuInfo(&tmi);
	if ( UseMouse < ConsoleMouse_FULL ){ // 一時的にPPbによるマウス操作受付を有効にする
		GetConsoleMode(hStdin, &oldin);
		SetConsoleMode(hStdin, CONSOLE_IN_FLAGS_DEF | ENABLE_EXTENDED_FLAGS);
	}
	tCsrMode(-1);

	tStore( tmi.window.x , tmi.window.y , tmi.window.x + tmi.size.cx, tmi.window.y + tmi.size.cy + 2, &win);

	ClearTMenuInfoArea(&tmi, TRUE);
												/*	全項目の表示  */
	do {
		if ( tmi.draw ){
			if ( tmi.draw == TMI_DRAW_CHANGE_CURSOR ){
				ChangeMenuItem(tmi.window.x,
						tmi.window.y + tmi.oldindex - tmi.showindex,
						tmi.size.cx ,TC_popup_item, t[tmi.oldindex].mes);
			}else if ( tmi.draw >= TMI_DRAW_ALL ){
				int y = 0, index, maxindex;

				if ( tmi.draw == TMI_DRAW_ALL_REDRAW ){
					ClearTMenuInfoArea(&tmi, TRUE);
					ScrollPos = 0;
				}

				tSetColor(TC_popup_item);
				if ( tmi.total > tmi.size.cy ){ // scroll position
					int NewScrollPos;

					NewScrollPos = (tmi.showindex * tmi.size.cy - 1) / (tmi.total - tmi.size.cy) + 1;
					if ( ScrollPos != NewScrollPos ){
						if ( ScrollPos != 0 ){
							tputposstr(tmi.window.x + tmi.size.cx, tmi.window.y + ScrollPos, ScrollDisableChar);
						}
						ScrollPos = NewScrollPos;
						tputposstr(tmi.window.x + tmi.size.cx, tmi.window.y + ScrollPos, ScrollEnableChar);
					}
				}

				index = tmi.showindex;
				maxindex = index + tmi.size.cy;
				for ( ; index < maxindex; y++, index++ ){
					tputposstr(tmi.window.x + 2, tmi.window.y + y + 1, MessageText(t[index].mes));
				}
			}
			ChangeMenuItem(tmi.window.x,
					tmi.window.y + tmi.index - tmi.showindex,
					tmi.size.cx, TC_popup_select_item, t[tmi.index].mes);
			tmi.draw = TMI_NODRAW;
			tmi.oldindex = tmi.index;
			tmi.oldshowindex = tmi.showindex;
		}
		key = TMenuInfoKey(&tmi);
		if ( key > ' ' ){ // ショートカットキー
			int i;

			key = TinyCharUpper(key);
			for ( i = 0 ; i < tmi.total ; i++ ){
				const TCHAR *p;

				p = tstrchr(t[i].mes, '&');
				if ( (p != NULL) && (key == TinyCharUpper(*(p + 1))) ){
					tmi.index = i;
					key = -1;
					break;
				}
			}
			break;
		}
	}while( key >= 0 );
	tRestore( tmi.window.x, tmi.window.y, tmi.window.x + tmi.size.cx, tmi.window.y + tmi.size.cy + 2, &win);
	screen.info.dwCursorPosition = tmi.oldoutinfo.dwCursorPosition;
	tCsrPos(tmi.oldoutinfo.dwCursorPosition.X, tmi.oldoutinfo.dwCursorPosition.Y);
	EndTMenuInfo(&tmi);
	if ( oldin != MAX32 ){
		SetConsoleMode(hStdin, oldin);
	}

	// UseMouse != ConsoleMouse_FULL のときの tConsoleInit は省略。
	return (key == -1) ? t[tmi.index].id : 0;
}

void ConChangeFontSize(SHORT delta)
{
	xCONSOLE_FONT_INFOEX cfi;

	if ( DSetCurrentConsoleFontEx == NULL ) return;
	cfi.cbSize = sizeof(cfi);
	DGetCurrentConsoleFontEx(hStdout, FALSE, &cfi);
	if ( DefaultFontY == 0 ) DefaultFontY = cfi.dwFontSize.Y;
	if ( delta != -2 ){
		if ( (cfi.dwFontSize.Y + delta) < 5 ) return;
		cfi.dwFontSize.Y += delta;
	}else{
		cfi.dwFontSize.Y = DefaultFontY;
	}
	DSetCurrentConsoleFontEx(hStdout, FALSE, &cfi);
}

#ifdef UNICODE
/*-----------------------------------------------------------------------------
	指定された文字の文字幅を求める(Console & UNICODE 用)
	※ 細かい違いは判断していない
-----------------------------------------------------------------------------*/
int CCharWide(WCHAR c)
{
					//もう少し正しくするなら GetStringTypeEx C3_HALFWIDTH で。
	if ( c <= 0x390 ){				// 0000-0390 ASCII からギリシャ文字まで
		return 1;
	}
	if ( c < 0x4e00 ){				// 0391-4dff 漢字より前の記号類
		if ( c >= 0x3000 ){			// 3000-4dff カタカナ・ひらがな類
			return (c < 0x33ff) ? 2 : 1;
		}
		if ( c >= 0x2000 ){				// 2000-2fff 記号類
			if ( c >= 0x2103 ){
				return (c <= 0x27be) ? 2 : 1;	// 2103-27be 記号類2
			}else{
				return (c <= 0x203b) ? 2 : 1;	// 2000-203b 記号類1
			}
		}								// 0391-1fff
		if ( c <= 0x3c9 ) return 2;			// 0391-03c9 ギリシャ文字
		if ( c < 0x410 ) return 1;
		if ( c <= 0x451 ) return 2;			// 0410-0451 ロシア文字
		return 1;
	}else{							// 4e00-ffff 漢字など
		if ( c < 0xff01 ){
			if ( c <= 0x9fa5 ) return 2;	// 4e00-9fa5 漢字1
			if ( c >= 0xe000 ){
				if ( c <= 0xfa2d ) return 2; // e000-fa2d 漢字2
				return 1;
			}
			if ( c < 0xac00 ) return 1;
			if ( c <= 0xd7a3 ) return 2;	// ac00-d7a3 ハングル
			return 1;
		}
		if ( c <= 0xff5e ) return 2;	// ff01-ff5e 全角アルファベット
		if ( c < 0xffe0 ) return 1;		// ff5f-ffe0 半角カタカナ
		if ( c <= 0xffe6 ) return 2;	// ffe0-ffe6 通貨
		return 1;
	}
}
#endif

void PrintOption(void)
{
	TCHAR buf1[200], buf2[200];
	HWND hForegroundWindow = GetForegroundWindow();

	GetClassName(hHostWnd, buf1, TSIZEOF(buf1)); // conhost: ConsoleWindowClass / wt,ssh: PseudoConsoleWindow / wine: 存在しない
	GetClassName(hForegroundWindow, buf2, TSIZEOF(buf2)); // conhost: ConsoleWindowClass / wt,ssh: PseudoConsoleWindow / wine: 存在しない
	XMessage(NULL, NULL, XM_DbgCon,
			T("*Terminal Intomation\r\n")
			T("HostWindow: %x(%s),type %d\r\n")
			T("ForegroundWindow: %x(%s)\r\n"),
			hHostWnd, buf1, ConsoleHost,
			hForegroundWindow, buf2);

	XMessage(NULL, NULL, XM_DbgCon, T("GetConsoleCP: %d\r\nGetConsoleOutCP: %d\r\n"), GetConsoleCP(), GetConsoleOutputCP());
	XMessage(NULL, NULL, XM_DbgCon, T("Stdin: mode:%x type:%d\r\nStdout: mode:%x type:%d\r\n"),
			OldStdinMode, GetFileType(hStdin),
			OldStdoutMode, GetFileType(hStdout));

	XMessage(NULL, NULL, XM_DbgCon, T("Screenbuf: Size[%dx%d] view(%d,%d)[%dx%d] Cursor(%d,%d) Attr:%x\r\n"),
		screen.info.dwSize.X, screen.info.dwSize.Y,
		screen.info.srWindow.Left, screen.info.srWindow.Top,
		screen.info.srWindow.Right - screen.info.srWindow.Left + 1,
		screen.info.srWindow.Bottom - screen.info.srWindow.Top + 1,
		screen.info.dwCursorPosition.X, screen.info.dwCursorPosition.Y,
		screen.info.wAttributes );

	XMessage(NULL, NULL, XM_DbgCon, T("Monitor: %d, Mouse: %d, Remote Session: %d, Remote control: %d\r\n"),
			GetSystemMetrics(80 /*SM_CMONITORS*/ ),
			GetSystemMetrics(19 /*SM_MOUSEPRESENT*/ ),
			GetSystemMetrics(0x1000 /*SM_REMOTESESSION*/ ),
			GetSystemMetrics(0x2001 /*SM_REMOTECONTROL*/ ));
	{	// ウィンドウステーション情報の表示
		HWINSTA hSrcWinStat = GetProcessWindowStation();
		HDESK   hSrcDesktop = GetThreadDesktop(GetCurrentThreadId());
		TCHAR SrcWinStat[MAX_PATH];
		TCHAR SrcDesktop[MAX_PATH];
		TCHAR stationname[MAX_PATH];
		DWORD size, ssID = FFD, ssID2;
		HANDLE hKernel32 = GetModuleHandleA("KERNEL32.DLL");

		DefineWinAPI(DWORD,WTSGetActiveConsoleSessionId,(void)); // XP,kernel32
		DefineWinAPI(BOOL,ProcessIdToSessionId,(DWORD dwProcessId,DWORD *pSessionId)); // NT4,kernel32

		GETDLLPROC(hKernel32, WTSGetActiveConsoleSessionId);
		GETDLLPROC(hKernel32, ProcessIdToSessionId);
		if ( DProcessIdToSessionId != NULL ){
			DProcessIdToSessionId(GetCurrentProcessId(),&ssID2);

			if ( DWTSGetActiveConsoleSessionId != NULL ){
				ssID = DWTSGetActiveConsoleSessionId();
			}

			GetUserObjectInformation(hSrcWinStat,UOI_NAME,
					(LPVOID)SrcWinStat,MAX_PATH * sizeof(char),&size);
			GetUserObjectInformation(hSrcDesktop,UOI_NAME,
					(LPVOID)SrcDesktop,MAX_PATH * sizeof(char),&size);
			thprintf(stationname, TSIZEOF(stationname),
					T("station: %s\\%s, SSID:%d PSSID:%d\r\n"),
					SrcWinStat, SrcDesktop, ssID, ssID2);
			XMessage(NULL, NULL, XM_DbgCon, T("%s"), stationname);
		}
	}

	if ( hHostWnd != BADHWND ){
		HDC hDC;
		hDC = GetDC(hHostWnd);
		if ( hDC == NULL ){
			XMessage(NULL, NULL, XM_DbgCon, T("no gdc\r\n"));
		}else{
			XMessage(NULL, NULL, XM_DbgCon,
					T("GDI Device: Tec:%d  (%dx%d) RASTERCAPS:%x\r\n"),
					GetDeviceCaps(hDC, TECHNOLOGY),
					GetDeviceCaps(hDC, HORZRES),
					GetDeviceCaps(hDC, VERTRES),
					GetDeviceCaps(hDC, RASTERCAPS)
			);
			ReleaseDC(hHostWnd,hDC);
		}
	}

	XMessage(NULL, NULL, XM_DbgCon,T("\r\n*PPb state\r\n")
			T("ConsoleOnRemote: %d\r\n")
			T("GUI:%d\r\n")
			T("Mouse handle:%d\r\n\r\n"),
			ConsoleOnRemote, UseGUI, UseMouse );
}
