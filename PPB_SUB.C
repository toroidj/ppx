/*-----------------------------------------------------------------------------
	Paper Plane bUI													Sub
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <wincon.h>
#include <string.h>
#ifdef WINEGCC // unknown / data ... cmd 経由
#include <unistd.h>
#endif
#include "PPX.H"
#include "VFS.H"
#include "TCONSOLE.H"
#include "PPB.H"
#pragma hdrstop


DWORD_PTR USECDECL PPbExecInfoFunc(PPXAPPINFO *ppb, DWORD cmdID, PPXAPPINFOUNION *uptr);
void ExeProcess(const TCHAR *name, const TCHAR *path, int flag);
int ExecCommand(const TCHAR *ptr);
void PauseCommand(const TCHAR *message);

DLLDEFINED const TCHAR NilStr[1] DLLPARAM(T(""));
DLLDEFINED const TCHAR ComSpecStr[] DLLPARAM(T("ComSpec"));

const TCHAR ShellOptions[] = T(XEO_STRINGS);

#define PPbTempWindowID 0x106ffff
HWND hTempWindow = NULL;

//---------------------------------- PPCOMMON との通信
PPXAPPINFO ppbexecinfo = {(PPXAPPINFOFUNCTION)PPbExecInfoFunc, T("PPb"), RegID, NULL};

int GetAbsX(TCINPUTSTRUCT *tis, int pos)
{
	int newX = 0;

	while ( pos > 0 ){
		#ifdef UNICODE
			if ( ((size_t)newX < tis->maxsize) && *(EditText + newX) ) newX++;
		#else
			if ((size_t)newX < tis->maxsize) newX += Chrlen(*(EditText + newX));
		#endif
		pos--;
	}
	return newX;
}
// *cursor
// -1:相対(x,y,t)	-2:相対+選択(x,y,t)
// -3:絶対(x,y)		-4:絶対(x,y,x2,y2)
// -5:パラメータ選択 (「"」含む） -6:パラメータ選択 (「"」除く）
DWORD_PTR PPbCursorCommand(PPXAPPINFOUNION *uptr)
{
	switch ( uptr->inums[0] ){
		case -1:
		case -2: {
			int key = (uptr->inums[0] == -1) ? 0 : K_s;
			if ( uptr->inums[1] >= 0 ){
				key |= K_raw | K_ri;
			}else{
				uptr->inums[1] = -uptr->inums[1];
				key |= K_raw | K_lf;
			}
			while ( uptr->inums[1] > 0 ){
				TconCommonCommand(&tis, key);
				uptr->inums[1]--;
			}
			break;
		}

		case -3:
			MoveCursor(&tis, GetAbsX(&tis, uptr->inums[1]), 0);
			break;

		case -4:
			MoveCursor(&tis, GetAbsX(&tis, uptr->inums[1]), 0);
			MoveCursor(&tis, GetAbsX(&tis, uptr->inums[(uptr->inums[4] != 0) ? 4 : 3]), K_s);
			break;

		default:
			SelectTextCommand(&tis, uptr->inums[0]);
			break;
	}
	return PPXA_NO_ERROR;
}

// %*editprop
void PPbGetPropFunction(PPXMDLFUNCSTRUCT *fparam)
{
	fparam->dest[0] = '\0';
	switch ( *fparam->optparam ){
		case 'm': // mode
			if ( tstrcmp(fparam->optparam, T("mode")) == 0 ){
				fparam->dest[0] = 'B';
				fparam->dest[1] = CmodeStr[tis.cmode];
				fparam->dest[2] = ' ';
				fparam->dest[3] = '\0';
			}
			break;

		default:
			tstrcpy(fparam->dest, T("?"));
	}
}

int ConsoleMenu(HMENU hPopupMenu)
{
	TMENUINFO tmi;

	TCHAR buf[CMDLINESIZE];
	DWORD oldin;

	GetConsoleMode(hStdin, &oldin);
	SetConsoleMode(hStdin, ENABLE_PROCESSED_INPUT);
	GetConsoleWindowInfo(hStdout, &screen.info);

	tmi.total = GetMenuItemCount(hPopupMenu);
	tmi.size.cy = tmi.total - 1;
	tmi.size.cx = screen.info.srWindow.Right - screen.info.srWindow.Left;

	InitTMenuInfo(&tmi);
	if ( UseMouse < ConsoleMouse_FULL ){ // 一時的にPPbによるマウス操作受付を有効にする
		SetConsoleMode(hStdin, CONSOLE_IN_FLAGS_NOWINEDIT);
	}
	tCsrMode(-1);

	for (;;){
		int key;

		if ( tmi.draw ){
			int index, maxindex;

			if ( tmi.draw == TMI_DRAW_ALL_REDRAW ){
				ClearTMenuInfoArea(&tmi, FALSE);
			}
			tCsrPos(tmi.window.x, tmi.window.y);

			index = tmi.showindex;
			maxindex = index + tmi.size.cy;
			for (;;){
				if ( GetMenuString(hPopupMenu, index, buf + 1, TSIZEOF(buf) - 1, MF_BYPOSITION) == 0 ){
					if ( index >= tmi.total ) break;
					tputstr(T(" ---\r\n"));
				}else{
					if ( index == tmi.index ){
						buf[0] = '[';
						tstrcat(buf, T("]\r\n"));
					}else{
						buf[0] = ' ';
						tstrcat(buf, T(" \r\n"));
					}
					tputstr(buf);
				}
				index++;
				if ( index >= maxindex ) break;
			}

			tmi.draw = TMI_NODRAW;
			tmi.oldindex = tmi.index;
			tmi.oldshowindex = tmi.showindex;
		}
		key = TMenuInfoKey(&tmi);
		if ( key < 0 ){
			ClearTMenuInfoArea(&tmi, FALSE);
			EndTMenuInfo(&tmi);
			if ( key == -1 ){
				HMENU hSubMenu;

				hSubMenu = GetSubMenu(hPopupMenu, tmi.index);
				if ( hSubMenu == NULL ){
					SetConsoleMode(hStdin, oldin);
					return GetMenuItemID(hPopupMenu, tmi.index) + 2;
				}else{
					int subindex;
					subindex = ConsoleMenu(hSubMenu);
					if ( subindex >= 3 ) return subindex;
					tmi.draw = TMI_DRAW_ALL_REDRAW;
					continue;
				}
			}
			SetConsoleMode(hStdin, oldin);
			return -1 + 2;
		}
	}
}

typedef int SIXELSTATUS;
struct sixel_dither {int dummy;};
typedef struct sixel_dither sixel_dither_t;
struct sixel_output {int dummy;};
typedef struct sixel_output sixel_output_t;
typedef int (USECDECL *sixel_write_function)(char *data, int size, void *priv);
struct sixel_allocator {int dummy;};
typedef struct sixel_allocator sixel_allocator_t;

#define SIXEL_SUCCEEDED(status) (((status) & 0x1000) == 0)
#define SIXEL_FAILED(status)    (((status) & 0x1000) != 0)
#define SIXEL_BUILTIN_XTERM256 0x3 // 8bit colors
#define SIXEL_PIXELFORMAT_BGR888   0x06
#define SIXEL_PIXELFORMAT_RGBA8888 0x11
#define SIXEL_PIXELFORMAT_BGRA8888 0x13

SIXELSTATUS (USECDECL *Dsixel_encode)(unsigned char *pixels,
	int width, int height, int depth,
	sixel_dither_t *dither, sixel_output_t *context);
SIXELSTATUS (USECDECL *Dsixel_output_new)(sixel_output_t **output,
	sixel_write_function fn_write, void *priv, sixel_allocator_t *allocator);
void (USECDECL *Dsixel_output_destroy)(sixel_output_t *output);
sixel_dither_t *(USECDECL *Dsixel_dither_get)(int builtin_dither);
void (USECDECL *Dsixel_dither_set_pixelformat)(sixel_dither_t *dither, int pixelformat);
void (USECDECL *Dsixel_dither_destroy)(sixel_dither_t *dither);

LOADWINAPISTRUCT SIXELDLL[] = {
	LOADWINAPI1(sixel_encode),
	LOADWINAPI1(sixel_output_new),
	LOADWINAPI1(sixel_output_destroy),
	LOADWINAPI1(sixel_dither_get),
	LOADWINAPI1(sixel_dither_set_pixelformat),
	LOADWINAPI1(sixel_dither_destroy),
	{NULL, NULL}
};

static int USECDECL sixel_callback(char *data, int size, void *priv)
{
	DWORD tmp;

	return WriteFile((HANDLE)priv, data, size, &tmp, NULL);
}

#ifdef UNICODE
WCHAR PixSeqence[] = CSI_W L"38;2;%d;%d;%dm" CSI_W L"48;2;%d;%d;%dm\u2584";
#else
char PixSeqence[] = CSI_A "48;2;%d;%d;%dm ";
#endif
#define ASPACTX 1000

void ShowImageToConsole(const TCHAR *param) // *image
{
	HTBMP hTbmp;
	HBITMAP hDestBMP;
	HGDIOBJ hOldDstBMP;
	HDC hDstDC;
	LPVOID lpBits;
	TCHAR buf[VFPS];
	BITMAPINFO bmpinfo;
	DWORD oldmode;
	int show_width, show_height;
	int bmp_width, bmp_height;
	int AspectRate;
	HANDLE hLibsixel;

	if ( param[0] == '\0' ) param = NULL;

	if ( param != NULL ){
		if ( LoadBMP(&hTbmp, param, 100) == FALSE ){
			tputstr(T("\r\nLoad error.\r\n"));
			return;
		}
		bmp_width = hTbmp.size.cx;
		bmp_height = hTbmp.size.cy;
#ifdef UNICODE
		AspectRate = (hTbmp.DIB->biYPelsPerMeter == 0) ? 0 : ((hTbmp.DIB->biXPelsPerMeter * ASPACTX) / hTbmp.DIB->biYPelsPerMeter);
#else
		AspectRate = (hTbmp.DIB->biYPelsPerMeter == 0) ? (ASPACTX / 2) : ((hTbmp.DIB->biXPelsPerMeter * ASPACTX) / hTbmp.DIB->biYPelsPerMeter) / 2;
#endif
	}else{ // Screen capture
		bmp_width = hTbmp.size.cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		bmp_height = hTbmp.size.cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#ifdef UNICODE
		AspectRate = 0;
#else
		AspectRate = ASPACTX / 2;
#endif
	}

	hLibsixel = LoadWinAPI("libsixel-1.dll", NULL, SIXELDLL, LOADWINAPI_LOAD);

	if ( (AspectRate > (ASPACTX * 3)) ||
		 (AspectRate < (ASPACTX / 3)) ||
		 ((AspectRate > (ASPACTX - (ASPACTX / 10))) &&
		  (AspectRate < (ASPACTX + (ASPACTX / 10)))) ){
		AspectRate = 0;
	}
	if ( AspectRate != 0 ){
#ifndef UNICODE
		if ( hLibsixel != NULL ) AspectRate *= 2;
#endif
		if ( AspectRate >= ASPACTX ){
			bmp_height = (bmp_height * AspectRate) / ASPACTX;
		}else{
			bmp_width = (bmp_width * ASPACTX) / AspectRate;
		}
	}

	show_width = screen.info.srWindow.Right - screen.info.srWindow.Left - 1;
	if ( show_width > bmp_width ) show_width = bmp_width;

#ifdef UNICODE
	show_height = (screen.info.srWindow.Bottom - screen.info.srWindow.Top - 1) * 2;
#else
	show_height = screen.info.srWindow.Bottom - screen.info.srWindow.Top - 1;
#endif
	if ( hLibsixel != NULL ){ // １文字は規定値で 10x10扱い
		show_width *= 10;
		show_height *= 10;
	}
	if ( show_height > bmp_height ) show_height = bmp_height;

	if ( ((show_width << 15)  / bmp_width) >
		 ((show_height << 15) / bmp_height) ){
		show_width = (bmp_width * show_height) / bmp_height;
	}else{
		show_height = (bmp_height * show_width) / bmp_width;
	}
	if ( show_width == 0 ) show_width = 1;
	if ( show_height == 0 ) show_height = 1;

	hDstDC = CreateCompatibleDC(NULL);
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biWidth = show_width;
	bmpinfo.bmiHeader.biHeight = show_height;
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = 32;
	bmpinfo.bmiHeader.biCompression = BI_RGB;
	bmpinfo.bmiHeader.biSizeImage = 0;
	bmpinfo.bmiHeader.biClrUsed = 0;
	bmpinfo.bmiHeader.biClrImportant = 0;

	hDestBMP = CreateDIBSection(NULL, &bmpinfo, DIB_RGB_COLORS, &lpBits, NULL, 0);
	if ( hDestBMP != NULL ){
		int destY, destHeight;
		hOldDstBMP = SelectObject(hDstDC, hDestBMP);
		SetStretchBltMode(hDstDC, HALFTONE);

		destY = 0;
		destHeight = bmpinfo.bmiHeader.biHeight;
		if ( hLibsixel != NULL ){
			destY = destHeight - 1;
			destHeight = -destHeight;
		}
		if ( param != NULL ){
			StretchDIBits(hDstDC,
					0, destY, bmpinfo.bmiHeader.biWidth, destHeight,
					0, 0, hTbmp.size.cx, hTbmp.size.cy,
					hTbmp.bits, (BITMAPINFO *)hTbmp.DIB,
					DIB_RGB_COLORS, SRCCOPY);
		}else{
			HDC hscreen = GetDC(NULL);

			StretchBlt(hDstDC,
					0, destY, bmpinfo.bmiHeader.biWidth, destHeight,
					hscreen, GetSystemMetrics(SM_XVIRTUALSCREEN),
					GetSystemMetrics(SM_YVIRTUALSCREEN),
					hTbmp.size.cx, hTbmp.size.cy, SRCCOPY);
			ReleaseDC(NULL, hscreen);
		}

		SelectObject(hDstDC, hOldDstBMP);

										// 標準出力の設定
		GetConsoleMode(hStdout, &oldmode);
		SetConsoleMode(hStdout, PPxStdoutMode);

		if ( hLibsixel != NULL ){
			sixel_output_t *output = NULL;

			if ( SIXEL_SUCCEEDED(Dsixel_output_new(&output, sixel_callback, (void *)hStdout, NULL)) ){
				sixel_dither_t *dither;

				dither = Dsixel_dither_get(SIXEL_BUILTIN_XTERM256);
				if ( dither != NULL ){
					Dsixel_dither_set_pixelformat(dither, SIXEL_PIXELFORMAT_BGRA8888);
					Dsixel_encode(lpBits, bmpinfo.bmiHeader.biWidth,
							bmpinfo.bmiHeader.biHeight, 0, dither, output);
					Dsixel_dither_destroy(dither);
				}
				Dsixel_output_destroy(output);
			}
		}else{
			int x, y;

			#ifdef UNICODE
				COLORREF pix1, pix2;
				#ifndef _WIN64 // BCC5 対策(\u が使用できない)
					PixSeqence[(sizeof(PixSeqence) / 2) - 6] = 0x2584;
					PixSeqence[(sizeof(PixSeqence) / 2) - 5] = 0;
				#endif

				y = bmpinfo.bmiHeader.biHeight - 2;
				if ( y <= 0 ){
					for ( x = 0 ; x < bmpinfo.bmiHeader.biWidth ; x++ ){
						pix1 = *(COLORREF *)(BYTE *)((BYTE *)lpBits + x * sizeof(COLORREF));
						thprintf(buf, TSIZEOF(buf), PixSeqence,
								GetBValue(pix1), GetGValue(pix1),
								GetRValue(pix1), 0, 0, 0);
						tputstr(buf);
					}
					tputstr(CSI_T T("m\r\n"));
				}else for ( ; y >= 0 ; y -= 2)
			#else
				COLORREF pix;

				for ( y = bmpinfo.bmiHeader.biHeight - 1 ; y >= 0 ; y--)
			#endif
				{
					for ( x = 0 ; x < bmpinfo.bmiHeader.biWidth ; x++ ){
					#ifdef UNICODE
						if ( y > 0 ){
							pix1 = *(COLORREF *)(BYTE *)((BYTE *)lpBits + (x + y * bmpinfo.bmiHeader.biWidth) * sizeof(COLORREF));
						}else{
							pix1 = 0;
						}
						pix2 = *(COLORREF *)(BYTE *)((BYTE *)lpBits + (x + (y + 1) * bmpinfo.bmiHeader.biWidth) * sizeof(COLORREF));
						thprintf(buf, TSIZEOF(buf), PixSeqence,
							GetBValue(pix1), GetGValue(pix1), GetRValue(pix1),
							GetBValue(pix2), GetGValue(pix2), GetRValue(pix2) );
					#else
						pix = *(COLORREF *)(BYTE *)((BYTE *)lpBits + (x + y * bmpinfo.bmiHeader.biWidth) * sizeof(COLORREF));
						thprintf(buf, TSIZEOF(buf), PixSeqence, GetBValue(pix), GetGValue(pix), GetRValue(pix));
					#endif
						tputstr(buf);
					}
					tputstr(CSI_T  T("m\r\n"));
				}
		}
		DeleteObject(hDestBMP);
	}
	DeleteDC(hDstDC);
	if ( hLibsixel != NULL ) FreeLibrary(hLibsixel);
	if ( param != NULL ) FreeBMP(&hTbmp);
	SetConsoleMode(hStdout, oldmode);
}

void PPbEditModeCommand(TCINPUTSTRUCT *tis, const TCHAR *param) // *editmode
{
	TCHAR buf[CMDLINESIZE], code, *more;

	for ( ;; ){
		code = GetOptionParameter(&param, buf, &more);
		if ( code == '\0' ) break;
		if ( code == '-' ){
			if ( tstrcmp(buf + 1, T("SYNTAX")) == 0 ){
					if ( *more == '\0' ){
						tis->useattr = tis->useattr ? 0 : 1;
					}else if ( (tstrcmp(more, T("clear")) == 0) || (tstrcmp(more, T("off")) == 0) ){
						tis->useattr = 0;
						resetflag(tis->eflag, TCI_TI_DRAW);
					}else if ( tstrcmp(more, T("on")) == 0 ){
						tis->useattr = 0;
						setflag(tis->eflag, TCI_TI_DRAW);
					}
			}else{
				tputstr(T("option error\n"));
			}
			continue;
		}
	}
}

DWORD_PTR PPbCommand(PPXAPPINFOUNION *uptr)
{
	TCHAR buf[CMDLINESIZE], *more;
	TCHAR *param;

	param = uptr->str + tstrlen(uptr->str) + 1;
	if ( tstrcmp(uptr->str, T("IMAGE")) == 0 ){
		ShowImageToConsole(param);
		return PPXA_NO_ERROR;
	}

	if ( tstrcmp(uptr->str, T("OPTION")) == 0 ){ // *option
		GetOptionParameter((const TCHAR **)&param, buf, &more);
		if ( buf[0] == 'c' ){ // common command
			ExtPath = EditPath;
			return PPXA_NO_ERROR;
		}else if ( buf[0] == 's' ){ // sepate command
			ExtPath = ExtPathBuf;
			return PPXA_NO_ERROR;
		}else if ( buf[0] == 'd' ){ // desktop
			UseGUI = TRUE;
			UseMouse = ConsoleMouse_FULL;
			PPxCommonExtCommand(K_ConsoleMode, ConsoleMode_GUI);
			tConsoleInit();
			return PPXA_NO_ERROR;
		}else if ( buf[0] == 't' ){ // terminal
			UseGUI = FALSE;
			UseMouse = ConsoleMouse_DISABLE;
			PPxCommonExtCommand(K_ConsoleMode, ConsoleMode_ConsoleOnly);
			tConsoleInit();
			return PPXA_NO_ERROR;
		}else if ( tstrcmp(buf, T("esc")) == 0 ){ // esc
			GetOptionParameter((const TCHAR **)&param, buf, &more);
			if ( Isdigit(buf[0]) ){
				X_cone = buf[0] - '0';
			}
			return PPXA_NO_ERROR;
		}else if ( buf[0] == '\0' ){ // オプション表示
			PrintOption();
			return PPXA_NO_ERROR;
		}
	}

	if ( tstrcmp(uptr->str, T("MENU")) == 0 ){ // *menu
		PPbF1Menu(&tis);
		return PPXA_NO_ERROR;
	}

	if ( tstrcmp(uptr->str, T("PAUSE")) == 0 ){
		PauseCommand(param);
		return PPXA_NO_ERROR;
	}

	if ( tstrcmp(uptr->str, T("EDITMODE")) == 0 ){
		PPbEditModeCommand(&tis, param);
		return PPXA_NO_ERROR;
	}

	// ダミーコマンド
	if ( (tstrcmp(uptr->str, T("COMPLETELIST")) == 0) ||
		 (tstrcmp(uptr->str, T("DEFAULTMENU")) == 0) ||
		 (tstrcmp(uptr->str, T("EDITMODE")) == 0) ){
		return PPXA_NO_ERROR;
	}
	return PPXA_INVALID_FUNCTION;
}

DWORD_PTR PPbFunction(PPXAPPINFOUNION *uptr)
{
	if ( tstrcmp(uptr->funcparam.param, T("EDITPROP")) == 0 ){
		PPbGetPropFunction(&uptr->funcparam);
		return PPXA_NO_ERROR;
	}
	if ( tstrcmp(uptr->funcparam.param, T("CURSORTEXT")) == 0 ){
		GetWordText(&tis, uptr->funcparam.dest, (uptr->funcparam.optparam[0] == 'o') ? GWSF_SPLIT_PARAM : 0);
		return PPXA_NO_ERROR;
	}
	if ( tstrcmp(uptr->funcparam.param, T("EDITTEXT")) == 0 ){
		CmdFunctionLongResult(&uptr->funcparam, EditText, -1);
		return PPXA_NO_ERROR;
	}
	return PPXA_INVALID_FUNCTION;
}

HWND GetTempWindow(void)
{
	if ( hTempWindow == NULL ){
		hTempWindow = CreateWindowEx(0, T("STATIC"), T("PPb"), WS_CHILD,
				0, 0, 0, 0, hHostWnd, (HMENU)PPbTempWindowID,
				GetModuleHandle(NULL), 0);
	}
	SetForegroundWindow(hHostWnd);
	ForceSetForegroundWindow(hHostWnd);
	ForceSetForegroundWindow(hTempWindow);
	return hTempWindow;
}

void GetPopupPosition(POINT *pos)
{
	RECT box;

	GetWindowRect(hHostWnd, &box);
	if ( GetCursorPos(pos) &&
		 (pos->x >= box.left) &&
		 (pos->x < box.right) &&
		 (pos->y >= box.top) &&
		 (pos->y < box.bottom) ){
		return;
	}
	pos->x = box.left + 20;
	pos->y = (box.top + box.bottom) / 2 + 20;
}

// 一行編集時に使用する
#pragma argsused
DWORD_PTR USECDECL PPbInfoFunc(PPXAPPINFO *ppb, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	UnUsedParam(ppb);

	switch(cmdID){
		case '1':
			tstrcpy(uptr->enums.buffer, EditPath);
			break;

		case PPXCMDID_STARTENUM:	// 検索開始(マーク無しもあり)
		case PPXCMDID_STARTNOENUM:	// 検索開始(マーク無しはなし)
			uptr->enums.enumID = 0;
			break;

		case PPXCMDID_NEXTENUM:		// 次へ
		case PPXCMDID_ENDENUM:		//列挙終了
			uptr->enums.enumID = -1;
			return 0;

		case 'C': // %C
			if ( uptr->enums.enumID == -1 ){
				*uptr->enums.buffer = '\0';
				break;
			}
		// Fall through
		case 'R': // %R
			GetWordText(&tis, uptr->enums.buffer, GWSF_SPLIT_PARAM);
			break;

		case PPXCMDID_CHDIR: // %j
			VFSFullPath(EditPath, uptr->str, EditPath);
			break;

		case PPXCMDID_EXEC:
			ExeProcess(
					((PPXCMD_EXEC *)uptr)->name,
					((PPXCMD_EXEC *)uptr)->path,
					((PPXCMD_EXEC *)uptr)->flag);
			if ( ((PPXCMD_EXEC *)uptr)->flag & XEO_WAITKEY ){
				PauseCommand(NilStr);
			}
			return ExitCode;

		case PPXCMDID_INSERTSEL:
			Replace(&tis, uptr->str, REPLACE_SELECT);
			return NO_ERROR;

		case PPXCMDID_INSERT:
			Replace(&tis, uptr->str, 0);
			return NO_ERROR;

		case PPXCMDID_REPLACE:
			Replace(&tis, uptr->str, REPLACE_ALL);
			return NO_ERROR;

		case PPXCMDID_SELECTTEXT:
			tstrcpy(uptr->str, EditText + tis.sel.start);
			*(uptr->str + (tis.sel.end - tis.sel.start)) = '\0';
			return NO_ERROR;

		case PPXCMDID_PPXCOMMAD:
			TconCommonCommand(&tis, uptr->key);
			break;

		case PPXCMDID_COMMAND:
			return PPbCommand(uptr);

		case PPXCMDID_FUNCTION:
			return PPbFunction(uptr);

		case PPXCMDID_REQUIREKEYHOOK:
			KeyHookEntry = FUNCCAST(CALLBACKMODULEENTRY, uptr);
			break;

		case PPXCMDID_REPORTTEXT:
			if ( uptr->str != NULL ){
				tputstr(uptr->str);
			}
			return PPXCMDID_EXTREPORTTEXT_LOG;

		case PPXCMDID_SETPOPLINE:
			ClearStatusLine();
			tputstr(T("\r"));
			if ( uptr->str != NULL ) tputstr(MessageText(uptr->str));
			break;

		case PPXCMDID_MOVECSR:
			return PPbCursorCommand(uptr);

		case PPXCMDID_HOOK_POPUPMENU:
			return ConsoleMenu((HMENU)uptr);
/*
		case PPXCMDID_TEMP_WINDOW:
			return GetTempWindow();
*/
		case PPXCMDID_POPUPPOS:
			GetPopupPosition((POINT *)uptr);
			break;

		default:
			if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
			return PPXA_INVALID_FUNCTION;
	}
	return PPXA_NO_ERROR;
}

// ファイル実行時に使用する
#pragma argsused
DWORD_PTR USECDECL PPbExecInfoFunc(PPXAPPINFO *ppb, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	UnUsedParam(ppb);
	switch(cmdID){
		case '0':	// 自分自身へのパス
			CatPath(uptr->enums.buffer, PPxPath, NilStr);
			break;

		case '1':
			tstrcpy(uptr->enums.buffer, CurrentPath);
			break;

		case PPXCMDID_EXEC:
			ExeProcess(
					((PPXCMD_EXEC *)uptr)->name,
					((PPXCMD_EXEC *)uptr)->path,
					((PPXCMD_EXEC *)uptr)->flag);
			if ( ((PPXCMD_EXEC *)uptr)->flag & XEO_WAITKEY ){
				PauseCommand(NilStr);
			}
			return ExitCode;

		case PPXCMDID_COMMAND:
			return PPbCommand(uptr);

		case PPXCMDID_FUNCTION:
			return PPbFunction(uptr);

		case PPXCMDID_REQUIREKEYHOOK:
			KeyHookEntry = FUNCCAST(CALLBACKMODULEENTRY, uptr);
			break;

		case PPXCMDID_REPORTTEXT:
			if ( uptr->str != NULL ){
				tputstr(uptr->str);
			}
			return PPXCMDID_EXTREPORTTEXT_LOG;

		case PPXCMDID_SETPOPLINE:
			if ( uptr->str != NULL ){
				tputstr(MessageText(uptr->str));
			}
			break;

//		case PPXCMDID_POPUPPOS:
//			break;

		default:
			if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
			return PPXA_INVALID_FUNCTION;
	}
	return PPXA_NO_ERROR;
}

/*-----------------------------------------------------------------------------
	PPb の終了時に呼ぶ
-----------------------------------------------------------------------------*/
void ReleasePPB(void)
{
	if ( RegNo >= 0 ){
		PPXMODULEPARAM pmp = {NULL};

		TconCommonCommand(&tis, K_E_CLOSE);
		CallModule(&ppbexecinfo, PPXMEVENT_DESTROY, pmp, NULL);

		PPxRegist(hHostWnd, RegID, PPXREGIST_FREE);
		if ( !IsIconic(hHostWnd) && IsWindowVisible(hHostWnd) ){
			SaveConsolePos(RegCID);
		}
		CloseHandle(hCommSendEvent);	// 通信用イベントの解放
		CloseHandle(hCommIdleEvent);	// 通信用イベントの解放
	}
	tRelease(); // コンソールライブラリの解除

	PPxCommonCommand(NULL, 0, K_CLEANUP);
	PPxUnRegisterThread();
	CoUninitialize();
}

/*-----------------------------------------------------------------------------
	ウィンドウタイトルを登録する
-----------------------------------------------------------------------------*/
void TitleDisp(const TCHAR *addmes)
{
	TCHAR title[WNDTITLESIZE + VFPS + CMDLINESIZE + 4];

	if ( addmes == NULL ){
		thprintf(title, TSIZEOF(title), T("%s%s"), WndTitle, CurrentPath);
	}else{
		thprintf(title, TSIZEOF(title), T("%s%s - %s"), WndTitle, CurrentPath, addmes);
	}
										// Title を設定
	SetConsoleTitle(title);
}

// SearchVLINE 関連
BOOL IsRedirectB(const TCHAR *str) // SearchVLINE 関連
{
	TCHAR type;

	for ( ;; ){
		type = *str;
		if ( type == '\0' ) return FALSE;
		if ( (type == '|') || (type == '<') || (type == '>')) return TRUE;
		#ifdef UNICODE
			str++;
		#else
			type = (char)Chrlen(type);
			str += type;
		#endif
	}
}

void SetWindowsTerminalFocus(void)
{
	HWND hWnd = GetTempWindow();

	DestroyWindow(hWnd);
	ForceSetForegroundWindow(hHostWnd);
	hTempWindow = NULL;
#if 0
	TCHAR buf[MAX_PATH];

	ShowWindow(hHostWnd, SW_SHOW);
	ForceSetForegroundWindow(hHostWnd); // ●現在機能していない
	thprintf(buf, TSIZEOF(buf), T("*pptray -c *focus #%d"), hHostWnd); // 開始直後プロセスにはフォーカス操作権があるので、利用して切り替え
	PP_ExtractMacro(hHostWnd, NULL, NULL, buf, NULL, XEO_CONSOLE | XEO_NOUSEPPB);
#endif
}

/*-----------------------------------------------------------------------------
	コマンドライン実行処理
-----------------------------------------------------------------------------*/
BOOL ConsoleSignalRecieve(HANDLE hProcess)
{
	TCHAR RecvParam[RECEIVESTRINGSLENGTH]; // 受信したコマンドライン

	if ( ReceiveStrings(RegID, RecvParam) == 0 ){ // [6]内容受領
		ResetEvent(hCommSendEvent);
		if ( (RecvParam[0] == '>') && (RecvParam[1] == 'G') ){
			if ( TinyCharUpper(RecvParam[2]) == 'K' ){ // Gk/GK : 実行強制終了
				if ( RecvParam[2] == 'k' ){ // Gk は Ctrl+C も行う
					GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
					Sleep(300);
				}
				TerminateProcess(hProcess, -1);
				tputstr(T("kill\n"));
				return TRUE;
			}
			if ( RecvParam[2] == 'x' ){ // Gx : 実行完了待ち解除
				tputstr(T("no wait\n"));
				return FALSE;
			}
		}
		PPbExecuteRecv(RecvParam); // [7]実行
	}else{
		ResetEvent(hCommSendEvent); // [8] 返信
	}
	return TRUE;
}

#define hd_Process 0
#define hd_Send 1
void ConsoleWaitProcess(HANDLE hProcess, int flags)
{
	if ( flags & XEO_WAITIDLE ){
		if ( hCommSendEvent == NULL ){
			WaitForInputIdle(hProcess, INFINITE);
		}else{
			for (;;){
				if ( WaitForInputIdle(hProcess, 100) != WAIT_TIMEOUT ) break;
				if ( WaitForSingleObject(hCommSendEvent, 0) != WAIT_OBJECT_0 ){
					continue;
				}
				if ( ConsoleSignalRecieve(hProcess) == FALSE ) break;
			}
		}
	}

	if ( flags & XEO_SEQUENTIAL ){
		if ( hCommSendEvent == NULL ){
			WaitForSingleObject(hProcess, INFINITE);
		}else{
			HANDLE handles[2];

			handles[hd_Process] = hProcess;
			handles[hd_Send] = hCommSendEvent;
			for (;;){
				DWORD read;

				read = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
				if ( read != (WAIT_OBJECT_0 + hd_Send) ) break;
				if ( ConsoleSignalRecieve(hProcess) == FALSE ) break;
			}
		}
		GetExitCodeProcess(hProcess, &ExitCode);
	}
}

BOOL ExeProcessShell(HWND hOwner, const TCHAR *exename, const TCHAR *param, const TCHAR *path, int flags)
{
	HANDLE hProcess;
	TCHAR reason[VFPS];

	if ( NULL != (hProcess = PPxShellExecute(hOwner, NULL, exename, param, path, flags, reason)) ){
		if ( flags & (XEO_WAITIDLE | XEO_SEQUENTIAL) ){
			ConsoleWaitProcess(hProcess, flags);
			CloseHandle(hProcess);
		}
		return TRUE;
	}
	tputstr(reason);
	tputstr(T("\n"));
	return FALSE;
}

void tputerror(ERRORCODE result)
{
	TCHAR buf[CMDLINESIZE];

	PPErrorMsg(buf, result);
	tstrcat(buf, T("\n"));
	tputstr(buf);
}

void ExeProcess(const TCHAR *execarg, const TCHAR *path, int flags)
{
	TCHAR LineStaticBuf[CMDLINESIZE * 2], *LineBuf = LineStaticBuf;
	TCHAR *exeterm;
	size_t linelen;

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	const TCHAR *param;
	int exetype;
	DWORD createflags = CREATE_DEFAULT_ERROR_MODE;

	while ( (*execarg == ' ') || (*execarg == '\t') ) execarg++;
									// 種別に応じて実行する
	param = execarg;
	exetype = GetExecType(&param, LineStaticBuf, path);

	ExitCode = EXIT_SUCCESS;
	if ( exetype == GTYPE_SHELLEXEC ){	// ShellExecute で起動
		ExeProcessShell(NULL, LineStaticBuf, param, path, flags);
		return;
	}
	if ( (exetype == GTYPE_ERROR) && (flags & XEO_NOCMDCMD) ){
		tputerror(ERROR_FILE_NOT_FOUND);
		return;
	}
	if ( !(flags & XEO_NOSCANEXE) && (CheckExebin(LineStaticBuf, exetype) == FALSE) ){
		tputerror(ERROR_CANCELLED);
		return;
	}
	// LineStaticBuf で足りないならメモリ確保
	// LineStaticBuf(コマンド名) + param(パラメータ) + %COMSPEC% + " /C " + 「"」で括る分
	linelen = tstrlen(LineStaticBuf) + tstrlen(param) + MAX_PATH + 6;
	if ( linelen > (CMDLINESIZE + VFPS) ){
		LineBuf = HeapAlloc(GetProcessHeap(), 0, TSTROFF(linelen));
		if ( LineBuf == NULL ) return;
		tstrcpy(LineBuf, LineStaticBuf);
	}

	if ( !(flags & XEO_NOPIPERDIR) ){
		if ( IsRedirectB(param) ) exetype = GTYPE_ERROR;
	}

	if ( exetype != GTYPE_GUI ) setflag(flags, XEO_SEQUENTIAL);

	// unknown / data / cmd
	if ( (exetype == GTYPE_ERROR) || (exetype == GTYPE_DATA) ){
	#ifndef WINEGCC // unknown / data ... cmd 経由
	{
		BOOL sepfix = FALSE;

		if ( (CurrentPath == EditPath) && (LineBuf[0] == '\0') ){ // 組み込みコマンド処理
			size_t len;
			len = param - execarg;
			while ( (len > 0) && (execarg[len - 1] == ' ') ) len--;
			tstrcpypart(LineBuf, execarg, len);
			if ( tstricmp(LineBuf, T("cd")) == 0 ){
				ERRORCODE result;

				GetLineParamS(&param, LineBuf, CMDLINESIZE);
				VFSFixPath(NULL, LineBuf, CurrentPath, VFSFIX_FULLPATH | VFSFIX_REALPATH | VFSFIX_NOFIXEDGE);
				if ( (result = VFSChangeDirectory(NULL, LineBuf)) == NO_ERROR ){
					GetCurrentDirectory(VFPS - 1, EditPath);
				}else{
					tputerror(result);
				}
				ExitCode = 0;
				return;
			}
			LineBuf[0] = '\0';
		}

		if ( (SkipSpace(&execarg) == '\"') && (tstrchr(param, '\"') != NULL) ){
			sepfix = TRUE;
		}

		if ( tstrlen(param) > 8000 ){ // CMD の長さ制限
			tputerror(RPC_S_STRING_TOO_LONG);
			goto fin;
		}
		// %COMSPEC% 取得
		if ( GetEnvironmentVariable(ComSpecStr, LineBuf, VFPS) == 0 ){
			#ifdef UNICODE
				tstrcpy(LineBuf, T("CMD.EXE"));
			#else
				tstrcpy(LineBuf, T("COMMAND.COM"));
			#endif // UNICODE
		}
		exeterm = LineBuf + tstrlen(LineBuf);
		tstrcpy(exeterm, T(" /C "));
		// cmd は「"file name.bat" param」は成功し、「"file name.bat" "param"」
		// で失敗するので、「""file name.bat" "param""」に加工する
		if ( IsTrue(sepfix) ) tstrcat(LineBuf, T("\""));
		tstrcat(exeterm, execarg);
		if ( IsTrue(sepfix) ) tstrcat(exeterm, T("\""));
	}
	#else // WINEGCC // system で実行する
		int result;

		tstrcpy(LineBuf, execarg);

		#ifdef UNICODE
		{
			char bufA[CMDLINESIZE * 4];

			UnicodeToUtf8(path, bufA, CMDLINESIZE * 4);
			(void)chdir(bufA); // ● c: とか z: とかの変換をまだしていない
			SetCurrentDirectory(path);

			UnicodeToUtf8(LineBuf, bufA, CMDLINESIZE * 4);
			result = system(bufA);
		}
		#else
			(void)chdir(path); // ● c: とか z: とかの変換をまだしていない
			SetCurrentDirectory(path);

			result = system(bufA);
		#endif
		if ( result < 0 ) tputstr(T("execute error.\n"));

		SetCurrentDirectory(PPxPath);
		goto fin;
		#endif
	}else{
		exeterm = LineBuf + tstrlen(LineBuf);
		if ( *param == ' ' ) param++;
		*exeterm = ' ';
		tstrcpy(exeterm + 1, param);
	}
											// 実行条件の指定
	si.cb			= sizeof(si);
	si.lpReserved	= NULL;
	si.lpDesktop	= NULL;
	si.lpTitle		= NULL;
	si.dwFlags		= 0;
	si.cbReserved2	= 0;
	si.lpReserved2	= NULL;
	si.wShowWindow	= SW_SHOWDEFAULT;

	if ( flags & (XEO_MAX | XEO_MIN | XEO_NOACTIVE | XEO_HIDE) ){
		si.dwFlags = STARTF_USESHOWWINDOW;
		if ( flags & XEO_MAX ) si.wShowWindow = SW_SHOWMAXIMIZED;
		if ( flags & XEO_MIN ) si.wShowWindow = SW_SHOWMINNOACTIVE;
		if ( flags & XEO_NOACTIVE ) si.wShowWindow = SW_SHOWNOACTIVATE;
		if ( flags & XEO_HIDE ){
			si.wShowWindow = SW_HIDE;
//			if ( !(flags & XEO_USECMD) ){ // リダイレクトするときは DETACHED 不可
//				setflag(createflags, DETACHED_PROCESS);
//			}
			setflag(createflags, DETACHED_PROCESS);
		}
	}
	if ( flags & XEO_LOW ) setflag(createflags, IDLE_PRIORITY_CLASS);

	if ( IsTrue(CreateProcess(NULL, LineBuf, NULL, NULL, FALSE,
			createflags, NULL, path, &si, &pi)) ){
		CloseHandle(pi.hThread);

		if ( flags & (XEO_WAITIDLE | XEO_SEQUENTIAL) ){
			ConsoleWaitProcess(pi.hProcess, flags);
		}
		CloseHandle(pi.hProcess);
	}else{
		ERRORCODE errorcode;

		param = LineBuf;
		if ( *param == '\"' ){ // 「"」 括りを除去
			param++;
			*(exeterm - 1) = '\0';
		}
		*exeterm = '\0';

		errorcode = GetLastError();
		if ( errorcode == ERROR_ELEVATION_REQUIRED ){ // UAC が必要
			ExeProcessShell(NULL, param, exeterm + 1, path, flags);
		}else{
			GetExecuteErrorReason(param, LineStaticBuf);
			tputstr(LineStaticBuf);
			tputstr(T("\n"));
		}
	}
fin:
	if ( LineBuf != LineStaticBuf ) HeapFree(GetProcessHeap(), 0, LineBuf);
}

void PauseCommand(const TCHAR *message) // *pause
{
	int press = 0;

	tConsoleInit();
	if ( (message != NULL) && (*message != '\0') ){
		tputstr(message);
		tputstr(T("\n"));
	}else{
		tputstr(MessageText(MES_KWAT));
	}
	for ( ; ; ){
		INPUT_RECORD cin;
		DWORD read;

		ReadConsoleInput(hStdin, &cin, 1, &read);
		if ( read == 0 ) continue;
		switch (cin.EventType){
			case KEY_EVENT:
				if ( cin.Event.KeyEvent.bKeyDown == FALSE ) continue;
				break;

			case MOUSE_EVENT:
				if ( cin.Event.MouseEvent.dwButtonState != 0 ){
					press = 1;
					continue;
				}
				if ( !press ) continue;
				break;

			default:
				continue;
		}
		break;
	}
	tRelease();
}

void RemoteExtractFunction(const TCHAR *param)
{
	ERRORCODE result;
	TCHAR ExtractText[CMDLINESIZE];

	ThFree(&ResultText);
	PP_InitLongParam(ExtractText);
	CurrentPath = EditPath;
	result = PP_ExtractMacro(hHostWnd, &ppbappinfo, NULL, param, ExtractText, XEO_NOUSEPPB | XEO_CONSOLE | XEO_EXTRACTEXEC | XEO_EXTRACTLONG);
	ThAddString(&ResultText, PP_GetLongParam(ExtractText, result));
	PP_FreeLongParam(ExtractText, result);
	ResultText.size = (DWORD)ResultText.top;
	ResultText.top = 0;
}

/*-----------------------------------------------------------------------------
	コマンドラインを解析する
-----------------------------------------------------------------------------*/
int PPbExecuteInput(TCHAR *param, size_t size)
{
	TCHAR *ptr, *next;
	int i;
	TCHAR buf[CMDLINESIZE * 2];

									// 計算式か？ ----------------------------
	if ( IsTrue(GetCalc(param, buf, &i)) ){
		ptr = param;
		if ( GTYPE_ERROR == GetExecType((const TCHAR **)&ptr, NULL, CurrentPath) ){
			tputstr(buf);
			tputstr(T("\n"));
			thprintf(buf, TSIZEOF(buf), T("%d"), i);
			WriteHistory(PPXH_NUMBER, buf, 0, NULL);
			return 0;
		}
	}

	ptr = param;
	while ( (DWORD)(ptr - param) < size ){
		TCHAR *orgptr;

		SkipSpace((const TCHAR **)&ptr); // ---------------------- 先頭空白削除
		next = ptr;
									// --------------------------- 区切りの検索
		while ( !( (*next == 0) || (*next == 0xd) || (*next == 0xa) )) next++;
		if ( (*next == 0xd) || (*next == 0xa) ){
			*next = '\0';
			next++;
		}
		if ( *ptr == '\0' ) break;
									// --------------------------- コマンド解析
		orgptr = ptr;
		GetLineParamS((const TCHAR **)&ptr, buf, TSIZEOF(buf));
		tstrupr(buf);
									// 内部コマンド ---------------------------
		if ( tstrcmp(buf, T("%:")) == 0 ) continue;

		if ( tstrcmp(buf, T("CD")) == 0 ){
			ERRORCODE result;

			GetLineParamS((const TCHAR **)&ptr, buf, TSIZEOF(buf));
			PP_ExtractMacro(hHostWnd, &ppbexecinfo, NULL, buf, buf, XEO_CONSOLE | XEO_NOUSEPPB);

			VFSFixPath(NULL, buf, CurrentPath, VFSFIX_FULLPATH | VFSFIX_REALPATH | VFSFIX_NOFIXEDGE);
			if ( (result = VFSChangeDirectory(NULL, buf)) == NO_ERROR ){
				GetCurrentDirectory(VFPS - 1, CurrentPath);
				continue;
			}else{
				tputerror(result);
				break;
			}
		}

		if ( (tstrcmp(buf, T("EXIT")) == 0) || (tstrcmp(buf, T("*EXIT")) == 0) ){
			return -1;
		}

		if ( tstrcmp(buf, T("PAUSE")) == 0 ){
			PauseCommand(ptr);
			break;
		}
									// --------------------------- 外部コマンド
		tRelease();
		tputstr(T("\n"));
		VFSChangeDirectory(NULL, CurrentPath);
		TitleDisp(orgptr);
		ppbexecinfo.hWnd = hHostWnd;
		PP_ExtractMacro(hHostWnd, &ppbexecinfo, NULL, orgptr, NULL, XEO_CONSOLE | XEO_NOUSEPPB);
		GetCurrentDirectory(VFPS - 1, CurrentPath);
		ptr = next;
		tputstr(T("\n"));
		if ( tis.eflag == TCI_QUIT ) return -1;
	}
	return 0;
}

/*-----------------------------------------------------------------------------
	受信コマンドを解析する
-----------------------------------------------------------------------------*/
void PPbExecuteRecv(TCHAR *param)
{
	switch( *param ){
		case '<':				// ログ出力 -------------------------------
			tputstr( param + 1 );
			tputstr(T("\n"));
			return;
		case '>':				// 受信コマンド ---------------------------
			if ( ExecCommand(param + 1) ) tputstr(T(" \n"));
			return;
		default:
			return;
	}
}

void PPbCommonCommand(const TCHAR *ptr)
{
	DWORD key;

	key = GetNumber(&ptr);
	switch ( key ){
		case K_Lcust:
			tInitCustomize();
		// default へ
		default:
			PPxCommonCommand(hHostWnd, 0, (WORD)key);
			break;
	}
}

int ExecCommand(const TCHAR *ptr)
{
	switch( *ptr++ ){
		case 'L':		// >Lpath	Chdir -----------------------------
			VFSChangeDirectory(NULL, CurrentPath);
			VFSChangeDirectory(NULL, ptr);
			GetCurrentDirectory(VFPS - 1, CurrentPath);
			// '\0' へ
		case '\0':		// >	sync / NULL ---------------------------
			return 0; // 改行無し

		case 'M':		// >Mn	WM_PPXCOMMAND -----------------------------
			PPbCommonCommand(ptr);
			return 0;

		case 'G':		// >Gn	signal
			if ( *ptr == 'b' ){ // Gb : ^break
				GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);
				tputstr(T("^break\n"));
			}else if ( *ptr == 'f' ){ // Focus
				if ( !ConsoleOnRemote ) SetWindowsTerminalFocus();
			}else if ( (*ptr == 'k') || (*ptr == 'x') ){
				// Gk : 実行強制終了
				// Gx : 実行完了待ち解除
				// 何もしない(ConsoleSignalRecieve内で処理)
			}else{ // Gc : ^C
				GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
				tputstr(T("^C\n"));
			}
			// Gz : 実行待ちをバックグラウンドに（未実装。ConsoleSignalRecieveで処理することになりそう）
			return 0;

		case 'p':		// >pcmd line	sHell(part), 最後は'P'を使用 ----------
			if ( LongParam.top == 0 ) ThCatString(&LongParam, T("H"));
			// 'B' へ続く
		case 'B':		// >BXcmd line	Long command(part), 最後は'P'を使用  --
			ThCatString(&LongParam, ptr);
			return 0;
		case 'P':		// >Pcmd line	sHell(last part)
			ThCatString(&LongParam, ptr);
			ExecCommand((TCHAR *)LongParam.bottom);
			ThFree(&LongParam);
			break;

		case 'R':
			RemoteExtractFunction(ptr);
			return 0;

		case 'K':
			CurrentPath = EditPath;
			PP_ExtractMacro(hHostWnd, &ppbappinfo, NULL, ptr, NULL, XEO_NOUSEPPB | XEO_CONSOLE);
			break;

		case 'H':{		// >H[option char][;hBackWnd], [>*]cmdline	sHell -----
			int flags = 0;
			int i;
			TCHAR c;

			while ( (c = *ptr) != '\0' ){
				ptr++;
				if ( c == ';' ){
					hBackWnd = (HWND)GetNumber(&ptr);
				}
				if ( c == ',' ) break;
				for ( i = 0 ; ShellOptions[i] ; i++ ){
					if ( c == ShellOptions[i] ){
						setflag(flags, 1 << i);
						break;
					}
				}
			}
			if ( (*ptr == '>') && ((*(ptr + 1) == '*') || (*(ptr + 1) == '%')) ){ // PPbのExtractMacro
				ppbexecinfo.hWnd = hHostWnd;
				PP_ExtractMacro(hHostWnd, &ppbexecinfo, NULL, ptr + 1, NULL, XEO_NOUSEPPB | XEO_CONSOLE);
				return 0;
			}

			tRelease();
			tputstr(T("\n"));

			if ( *ptr == '@' ){ // echo なし
				ptr++;
			}else{
				tputstr(CurrentPath);
				tputstr(T(">"));
				tSetColor(TC_execute);
				tputstr(ptr);
				SetConsoleTextAttribute(hStdout, screen.info.wAttributes);
				tputstr(T("\n"));
			}

			SetCurrentDirectory(CurrentPath);
			TitleDisp(ptr);
			ExeProcess(ptr, CurrentPath, flags);
			if ( flags & XEO_WAITKEY ){
				PauseCommand(NilStr);
				tputstr(T("\n"));
			}
			GetCurrentDirectory(VFPS - 1, CurrentPath);
			break;
		}
		default:
			tputstr(ptr - 1);
			tputstr(MessageText(MES_UCMD));
	}
	return 1; // 改行あり
}
