/*-----------------------------------------------------------------------------
	Paper Plane bUI								～ コマンドライン編集処理 ～
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <wincon.h>
#include <string.h>
#include "PPX.H"
#include "VFS.H"
#include "PPB.H"
#include "TCONSOLE.H"
#pragma hdrstop

#if !NODLL
#define DEFINE_tstplimcpy
#define DEFINE_tstristr
#define DEFINE_bchrlen
#include "tstrings.c"
#endif

int X_csyh_c = -1;

#define SYNTAXCOLOR_SELECT_MASK B7
#define SYNTAXCOLOR_edit SYNTAXCOLOR_count

typedef struct {
	EDITCOLORS color[SYNTAXCOLOR_count];

	EDITCOLORS edit;
	EDITCOLORS select;
} SYNTAXCOLORSC;

SYNTAXCOLORSC SynColorC = {
	{DEFAULT_EDITCOLORS},
	{C_S_EDIT_TEXT, C_S_EDIT_BACK}, // edit
	{C_AUTO, C_S_SELECT_BACK}, // select
};

int eflag_first = TCI_TI_FIRST;
const TCHAR *first_command = NULL;
int statuslen = 0;

TCHAR *EditText;		// 表示内容
WORD *RevBuf;	// スクロークモードの反転状態を保存するバッファ。
				// NULL なら反転状態でない。
DWORD OldMouseButton = 0; // 以前のマウスボタンの状態
int WheelOffset = 0; // ホイールの分解能を低下させるための移動量記憶
DWORD OldScrollTick = 0; // マウス範囲選択の連続描画の抑制

TCHAR *ScreenCapText = NULL; // 画面からの補完一覧作成用のキャプチャバッファ
DWORD ScreenCapSize = 0;

TCINPUTSTRUCT tis;
#define FORMLINEWIDTH 60

const TCHAR CmodeStr[CMODE_MAX] = {'E', 'L', 'R', 'S'};
const TCHAR *extmapname[CMODE_MAX] = {
	NULL,
	T("KB_list"),
	T("KB_ref"),
	T("KB_ref"),
};

const TMENU editmenu[] = {
	{1, MES_EDBL},
	{K_c | 'C', MES_EDCP},
	{K_c | 'X', MES_EDCU},
	{K_c | 'V', MES_EDPA},
	{K_c | 'A', MES_EDAL},
	{0, MES_EDCA},
	{0, NULL}
};

const TMENU selectmenu[] = {
	{K_c | 'C', MES_EDCP},
	{K_c | 'A', MES_EDAL},
	{0, MES_EDCA},
	{0, NULL}
};

const TMENU optionmenu[] = {
	{1, T("interactive search list(&I)")},
	{2, T("syntax highlight(&G)")},
	{3, T("terminal mode(no GUI mode, &T)")},
	{4, T("desktop mode(GUI mode, &D)")},
	{0, MES_EDCA},
	{0, NULL}
};

const TMENU executemenu[] = {
	{K_c | 'C', MES_EDCP},
	{K_c | 'X', MES_EDCU},
	{K_c | 'V', MES_EDPA},
	{K_c | 'A', MES_EDAL},
	{1, T("show image")},
	{0, MES_EDCA},
	{0, NULL}
};

const TMENU keylistmenu[] = {
	{K_raw | K_c | 'G',		T("execute menu   Ctrl+G")},
	{K_raw | K_c | 'Q',		T("edit menu      Ctrl+Q")},
	{K_raw | K_c | 'T',		T("option menu    Ctrl+T")},
	{K_raw | K_c | 'R',		T("complete list  Ctrl+R")},

	{K_raw | K_ri,			T("Right  Ctrl+F")},
	{K_raw | K_lf,			T("Left   Ctrl+B")},
	{K_raw | K_c | 'N',		T("Up     Ctrl+N")},
	{K_raw | K_c | 'P',		T("Down   Ctrl+P")},
	{K_raw | K_home,		T("Home   Home")},
	{K_raw | K_c | 'E',		T("End    Ctrl+E")},

	{K_raw | K_s | K_up,	T("Log reference  Shift+UP")},

	{K_raw | K_c | 'A',		T("Select all  Ctrl+A")},

	{K_raw | K_c | 'C',		T("Clip   Ctrl+C")},
	{K_raw | K_c | 'V',		T("Paste  Ctrl+V")},
	{K_raw | K_c | 'X',		T("Cut    Ctrl+X")},
	{K_raw | K_c | 'Z',		T("Undo   Ctrl+Z")},

	{K_raw | K_c | 'D',		T("Delete letter  Ctrl+D")},
	{K_raw | K_c | 'D',		T("Delete right   Ctrl+K")},
	{K_raw | K_c | 'W',		T("word BS        Ctrl+W")},
	{K_raw | K_c | 'Y',		T("pop form cut buffer  Ctrl+Y")},
	{K_raw | K_ins,			T("Insert mode    Insert")},

	{K_raw | K_c | 'I',		T("complete entry  Ctrl+I")},
	{K_raw | K_a | K_home,  T("restore Term windw position  Alt+home")},
	{K_raw | K_c | K_s | K_v | VK_ADD,    T("Opaque  Ctrl+Shift+'+'")},
	{K_raw | K_c | K_s | K_v | VK_SUBTRACT,    T("Transparent  Ctrl+Shift+'-'")},
	{K_raw | K_c | K_v | VK_ADD,		T("Zoom in   Ctrl+Shift+'+'")},
	{K_raw | K_c | K_v | VK_SUBTRACT,	T("Zoom out  Ctrl+Shift+'-'")},
	{K_raw | K_c | K_v | VK_NUMPAD0,	T("Reset font size  Ctrl+Num 0")},
	{K_raw | K_c | 'L',		T("reset terminal  Ctrl+L")},
	{K_raw | K_a | K_F4,	T("Exit  Alt+F4")},
	{0, MES_EDCA},
	{0, NULL}
};

const TMENU f1menu[] = {
	{1, T("help(&H)")},
	{2, T("context menu...(&C)")},
	{3, T("interactive search list(&I)")},
	{4, T("option menu...(&O)")},
	{5, T("key list...(&K)")},
	{6, T("Exit PPb(&Q)")},
	{0, MES_EDCA},
	{0, NULL}
};

const TMENU esc_menu[] = {
	{'q', T("Exit PPb(&Q)")},
	{1, T("context menu...(&C)")},
	{2, T("option menu...(&O)")},
	{3, T("interactive search list(&I)")},
	{4, T("terminal mode(no GUI mode, &T)")},
	{5, T("key list...(&K)")},
	{0, MES_EDCA},
	{0, NULL}
};

int MouseCommand(TCINPUTSTRUCT *tis, MOUSE_EVENT_RECORD *me);
void EditModeCommand(TCINPUTSTRUCT *tis, int key);
void LogModeCommand(TCINPUTSTRUCT *tis, int key);
void MoveCursorS(TCINPUTSTRUCT *tis, int offX, int offY, int key);
int PPbContextMenu(TCINPUTSTRUCT *tis);
void JumpWordCursor(TCINPUTSTRUCT *tis, BOOL x, int mode, int key);

#ifdef UNICODE
#define TChrlen(c) CCharWide(c)
#else
#define TChrlen(c) Chrlen(c)
#endif

void InitSyntaxColor(TCINPUTSTRUCT *tis, int csyh)
{
	if ( csyh < 0 ){
		if ( X_csyh_c < 0 ){
			DWORD X_csyh_full[2];

			X_csyh_full[0] = 0;
			X_csyh_full[1] = 0;
			GetCustData(T("X_csyh"), &X_csyh_full, sizeof(X_csyh_full));
			X_csyh_c = X_csyh_full[1];
		}
		csyh = X_csyh_c;
	}
	if ( csyh == 0 ){
		tis->useattr = 0;
		return;
	}

	tis->useattr = 1;
	X_cone = ConsoleColor_MAX;

	if ( SynColorC.color[0].back == (C_S_AUTO - 1) ){
		COLORREF *col = &SynColorC.color[0].text;
		int i;

		col[1] = C_AUTO;
		GetCustData(T("C_csyh"), col, (sizeof(SynColorC) - sizeof(EDITCOLORS) * 2));
		for ( i = 0; i < (SYNTAXCOLOR_count * 2); i++, col++){
			*col = GetSchemeColor(*col, C_S_AUTO);
		}
	}
}

void ToggleSyntaxColor(TCINPUTSTRUCT *tis)
{
	tis->useattr = tis->useattr ? 0 : 1;
	if ( tis->useattr ) InitSyntaxColor(tis, 1);
}

void SelectTextCommand(TCINPUTSTRUCT *tis, int id)
{
	TCHAR buf[0x800];
	int braket;
	ECURSOR cursor;

	tstrcpy(buf, EditText);
	if ( tis->sel.start == tis->sel.end ){
		cursor.start = tis->EdX;
		cursor.end	 = tis->EdX;
	}else{
		cursor.start = tis->sel.start;
		cursor.end	 = tis->sel.end;
	}

	braket = GetWordStrings(buf, &cursor, 0);
	if ( id == -5 ){
		if ( braket >= GWS_BRAKET_LEFT ) cursor.start--;
		if ( braket >= GWS_BRAKET_LEFTRIGHT ) cursor.end++;
	}
	tis->sel.start = cursor.start;
	tis->sel.end = cursor.end;
	setflag(tis->eflag, TCI_TI_DRAW);
}

static void ThListInit(ThList *list)
{
	ThInit(&list->Index);
	ThInit(&list->Data);
}

static void ThListFree(ThList *list)
{
	ThFree(&list->Index);
	ThFree(&list->Data);
}

static void CloseFileList(TCINPUTSTRUCT *tis)
{
	if ( tis->linelist.show != FALSE ){
		tFillBox(0 , tis->linelist.show_min_Y, screen.info.dwSize.X - 1, tis->linelist.show_min_Y + tis->list_height, TC_default);
		tCsrPos(0, tis->linelist.show_min_Y - 1);
		tis->linelist.show = FALSE;
	}
	ThListFree(&tis->linelist.list);
}

static void PPbOptionMenu(TCINPUTSTRUCT *tis)
{
	switch ( Select(optionmenu) ){
		case 1:
			EditModeCommand(tis, K_F4);
			break;

		case 2:
			ToggleSyntaxColor(tis);
			break;

		case 3:
			UseGUI = FALSE;
			UseMouse = ConsoleMouse_DISABLE;
			PPxCommonExtCommand(K_ConsoleMode, ConsoleMode_ConsoleOnly);
			tConsoleInit();
			break;

		case 4:
			UseGUI = TRUE;
			UseMouse = ConsoleMouse_FULL;
			PPxCommonExtCommand(K_ConsoleMode, ConsoleMode_GUI);
			tConsoleInit();
			break;
	}
}

static void PPbExecuteMenu(TCINPUTSTRUCT *tis)
{
	TCHAR path[VFPS];

	int key = Select(executemenu);
	switch ( key ){
		case 1:
			GetWordText(tis, path, GWSF_SPLIT_PARAM);
			VFSFullPath(NULL, path, EditPath);
			if ( GetFileAttributesL(path) != BADATTR ){
				ShowImageToConsole(path);
			}
			break;
		default:
			if ( key > 0 ) TconCommonCommand(tis, key);
	}
}

static void ShowStatusText(const TCHAR *text)
{
	statuslen = tstrlen32(text);
	tputposstr(0, screen.info.dwCursorPosition.Y + 1, text);
	if ( screen.info.dwCursorPosition.Y >= screen.info.srWindow.Bottom ){
		screen.info.dwCursorPosition.Y--;
	}
	tCsrPos(0, screen.info.dwCursorPosition.Y);
}

void ClearStatusLine(void)
{
	tFillSpace(0, screen.info.dwCursorPosition.Y + 1, FORMLINEWIDTH, screen.info.dwCursorPosition.Y + 1);
}

static void PPbInteractiveSeachMode(TCINPUTSTRUCT *tis)
{
	if ( tis->linelist.show == FALSE ){
		tis->linelist.mode = listmode_entry;
		ShowStatusText(T("entry search"));
	}else{
		CloseFileList(tis);
		tis->cmode = CMODE_EDIT;
		tis->linelist.mode = listmode_off;
		ShowStatusText(T("search off"));
	}
}

static void PPbKeylistMenu(TCINPUTSTRUCT *tis)
{
	int key = Select(keylistmenu);
	if ( key > 0 ) TconCommonCommand(tis, key);
}

void PPbF1Menu(TCINPUTSTRUCT *tis) // *menu
{
	switch ( Select(UseGUI ? f1menu : f1menu + 1) ){
		case 1:
			PPxHelp(hHostWnd, HELP_CONTEXT, IDH_PPB);
			break;
		case 2:
			PPbExecuteMenu(tis);
			break;
		case 3:
			PPbInteractiveSeachMode(tis);
			break;
		case 4:
			PPbOptionMenu(tis);
			break;
		case 5:
			PPbKeylistMenu(tis);
			break;
		case 6:
			tis->eflag = TCI_QUIT;
			break;
	}
}

static BOOL ThListAddStr(ThList *list, const TCHAR *str)
{
	if ( ThAppend(&list->Index, &list->Data.top, sizeof(list->Data.top)) == FALSE ){
		return FALSE;
	}
	if ( IsTrue(ThAddString(&list->Data, str)) ) return TRUE;
	list->Index.top -= sizeof(list->Data.top);
	return FALSE;
}

static DWORD ThListGetCount(ThList *list)
{
	return list->Index.top / sizeof(list->Data.top);
}

static const TCHAR *ThListGetStr(ThList *list, DWORD index)
{
	if ( index >= (list->Index.top / sizeof(list->Data.top)) ) return NULL;
	return ThPointerT(&list->Data, *(DWORD *)ThPointer(&list->Index, index * sizeof(list->Data.top)));
}

const TCHAR *cmodelist[] = {
	T("edit(Shift+F4)"),
	T("list(Shift+F4)"),
	T("folder(Shift+F4)"),
	T("file(Shift+F4)")
};
#ifndef WINEGCCx
void DeControl(TCHAR *str)
{
	for(;;){
		TCHAR chr;

		chr = *str;
		if ( chr == '\0' ) return;
		if ( (chr == '\t') || (chr == '\r') || (chr == '\n') ) *str = '.';
		str++;
	}
}
#else
#endif

void tputlimatrstr(int x, int y, int color, const TCHAR *str, DWORD maxsize)
{
	DWORD size;
	#ifndef WINEGCCx
		COORD xy;
		DWORD dummy;
		TCHAR strT[CMDLINESIZE];

		size = tstrlen32(str);
		if ( maxsize > CMDLINESIZE ) maxsize = CMDLINESIZE;
		if ( size > maxsize ) size = maxsize;
		tstplimcpy(strT, str, maxsize);
		DeControl(strT);
		xy.X = (SHORT)x;
		xy.Y = (SHORT)y;
		SetConsoleCursorPosition(hStdout, xy);
		tSetColor(color);
		WriteConsole(hStdout, strT, size, &dummy, NULL);
	#else
		move(y, x);
		size = tstrlen32(str);
		if ( maxsize > CMDLINESIZE ) maxsize = CMDLINESIZE;
		if ( size > maxsize ) size = maxsize;
		{
			char strA[CMDLINESIZE];
		#ifdef UNICODE
			size_t length;

			length = WideCharToMultiByte(CP_ACP, 0, str, size, staA, CMDLINESIZE - 1, NULL, NULL);
			if ( length == 0 ){
				char *extbufA;

				length = WideCharToMultiByte(CP_ACP, 0, str, size, NULL, 0, NULL, NULL) + 1;
				if ( length > 1 ){
					extbufA = HeapAlloc(hProcHeap, 0, length);
					if ( extbufA != NULL ){
						length = WideCharToMultiByte(CP_ACP, 0, str, size, extbufA, length, NULL, NULL);
						extbufA[length] = '\0';
						DeControl(extbufA);
						addstr(extbufA);
						HeapFree(hProcHeap, 0, extbufA);
						refresh();
						return;
					}
				}
				return;
			}
			strA[length] = '\0';
			DeControl(extbufA);
			addstr(strA);
			refresh();
		#else
			tstplimcpy(strA, str, maxsize);
			DeControl(extbufA);
			addstr(strA);
			refresh();
		#endif
		}
	#endif
}

BOOL IsSDir(const TCHAR *ptr)
{
	size_t len;
	TCHAR chr;

	len = tstrlen(ptr);
	if ( len < 1 ) return FALSE;
	chr = *(ptr + len - 1);
	if ( (chr == '\\') || (chr == '/') ) return TRUE;
	if ( (len >= 2) && (chr == '\"') ){
		chr = *(ptr + len - 2);
		if ( (chr == '\\') || (chr == '/') ) return TRUE;
	}
	return FALSE;
}


static void ShowLineList(TCINPUTSTRUCT *tis)
{
	LINEINFO *linelist = &tis->linelist;
	int index;

	tFrameBox(0 , linelist->show_min_Y, screen.info.dwSize.X - 1, linelist->show_min_Y + tis->list_height, 1);

	for ( index = 0; index < tis->list_height; index++ ){
		const TCHAR *ptr;
		BOOL dir;

		ptr = ThListGetStr(&linelist->list, index + linelist->item_min);
		if ( ptr == NULL ) break;
		dir = ((*ptr != '\0') && IsSDir(ptr));
		if ( (index + linelist->item_min) == linelist->item_index ){
			tputlimatrstr(1, linelist->show_min_Y + index,
					(WORD)(dir ? TC_popup_select_dir : TC_popup_select_item),
					ptr, screen.info.dwSize.X - 2);
		}else{
			tputlimatrstr(1, linelist->show_min_Y + index,
					(WORD)(dir ? TC_popup_dir : TC_popup_item),
					ptr, screen.info.dwSize.X - 2);
		}
	}
	if ( tis->cmode < CMODE_LOG_VIEW ){
		tSetColor(TC_popup_item);
		tputposstr(screen.info.dwSize.X - 20, linelist->show_min_Y + tis->list_height, cmodelist[tis->cmode]);
	}
	{
		int items = ThListGetCount(&linelist->list);
		if ( tis->list_height < items ){
			int NewScrollPos;

			NewScrollPos = ((linelist->item_min * tis->list_height) - 1) / (items - tis->list_height);
			tSetColor(TC_popup_item);
			tputposstr(screen.info.srWindow.Right - 1, linelist->show_min_Y + NewScrollPos, T("*"));
		}
	}
}

static void SelectLineList(TCINPUTSTRUCT *tis, int new_index)
{
	LINEINFO *linelist = &tis->linelist;

	if ( new_index < linelist->item_min ){
		if ( new_index < 0 ) new_index = 0;
		if ( linelist->item_index == new_index ) return;

		linelist->item_index = new_index;
		linelist->item_min = (new_index > 0) ? new_index : 1;
		tCsrMode(-1);
		ShowLineList(tis);
		tCsrMode(0);
		tSetDefaultColor();
	}else{
		int items = ThListGetCount(&linelist->list);

		if ( new_index >= items ) new_index = items - 1;
		if ( linelist->item_index == new_index ) return;

		if ( new_index >= (linelist->item_min + tis->list_height) ){
			linelist->item_index = new_index;
			linelist->item_min = new_index - tis->list_height + 1;
			tCsrMode(-1);
			ShowLineList(tis);
			tCsrMode(0);
			tSetDefaultColor();
		}else{
			const TCHAR *ptr;
			BOOL dir;

			if ( linelist->item_index >= 1 ){ // 前のを消去
				ptr = ThListGetStr(&linelist->list, linelist->item_index);
				dir = ((*ptr != '\0') && IsSDir(ptr));
				tputlimatrstr(1, linelist->show_min_Y + (linelist->item_index - linelist->item_min),
						(WORD)(dir ? TC_popup_dir : TC_popup_item),
						ptr, screen.info.dwSize.X - 2);
			}
			linelist->item_index = new_index;
			if ( linelist->item_index >= 1 ){ // 新しいのを表示
				ptr = ThListGetStr(&linelist->list, new_index);
				dir = ((*ptr != '\0') && IsSDir(ptr));

				tputlimatrstr(1, linelist->show_min_Y + (new_index - linelist->item_min),
						(WORD)(dir ? TC_popup_select_dir : TC_popup_select_item),
						ptr, screen.info.dwSize.X - 2);
			}
			tSetDefaultColor();
		}
	}

	// Scroll position 表示
	tis->sel = tis->linelist.cursor;
	Replace(tis, ThListGetStr(&tis->linelist.list, tis->linelist.item_index), REPLACE_FROMLIST);
	resetflag(tis->eflag, TCI_TI_LIST_CHANGE_TARGET);
	tis->linelist.cursor.end = tis->linelist.cursor.start + tstrlen(ThListGetStr(&tis->linelist.list, tis->linelist.item_index));
}

static void PPbEscMenu(TCINPUTSTRUCT *tis)
{
	int key;
	INPUT_RECORD cin;

	if ( tis->linelist.show != FALSE ){
		CloseFileList(tis);
		tis->cmode = CMODE_EDIT;
		return;
	}

	key = tgetin(&cin, 500);

	if ( (key != 'q') && ((key <= 0) || (UseGUI == FALSE)) ){
		int oldGUI = UseGUI;

		UseGUI = FALSE; // 常にテキストメニューにする
		key = Select(esc_menu);
		UseGUI = oldGUI;
	}
	switch ( key ){
		case K_c | '[':
		case K_c | K_esc:
		case K_esc:
		case 'q':
			tis->eflag = TCI_QUIT;
			break;
		case 1:
			PPbExecuteMenu(tis);
			break;
		case 2:
			PPbOptionMenu(tis);
			break;
		case 3:
			PPbInteractiveSeachMode(tis);
			break;
		case 4:
			UseGUI = FALSE;
			UseMouse = ConsoleMouse_DISABLE;
			PPxCommonExtCommand(K_ConsoleMode, ConsoleMode_ConsoleOnly);
			break;
		case 5:
			PPbKeylistMenu(tis);
			break;
	}
}

void GetWordText(TCINPUTSTRUCT *tis, TCHAR *dst, DWORD flags)
{
	TCHAR buf[CMDLINESIZE];
	ECURSOR cursor;

	tstrcpy(buf, EditText);
	if ( tis->sel.start == tis->sel.end ){
		cursor.start = tis->EdX;
		cursor.end	 = tis->EdX;
	}else{
		cursor.start = tis->sel.start;
		cursor.end	 = tis->sel.end;
	}
	GetWordStrings(buf, &cursor, flags);
	tstrcpy(dst, buf + cursor.start);
}

static void FreeScreenCapText(void)
{
	if ( ScreenCapText != NULL ){
		HeapFree(hProcHeap, 0, ScreenCapText);
		ScreenCapText = NULL;
	}
}

static void ScrollArea(TCINPUTSTRUCT *tis, int offY)
{
	SMALL_RECT box;
	CONSOLE_SCREEN_BUFFER_INFO cinfo;

	GetConsoleWindowInfo(hStdout, &cinfo);

	box = cinfo.srWindow;
	if ( (box.Top + offY) < 0 ){
		offY = -box.Top;
	}
	if ( (box.Bottom + offY) >= cinfo.dwSize.Y ){
		offY = cinfo.dwSize.Y - box.Bottom - 1;
	}
	box.Top += (short)offY;
	box.Bottom += (short)offY;
	SetConsoleWindowInfo(hStdout, TRUE, &box);
	setflag(tis->eflag, TCI_TI_SCROLL);
}

static int ForwardEditWord(TCINPUTSTRUCT *tis, int x)
{
	if ( (size_t)x < tis->maxsize ){
		while (*(EditText + x) && ((UTCHAR)*(EditText + x) > ' ') ){
			if ( (size_t)x >= tis->maxsize ) break;
			x++;
		}
	}
	return x;
}

static int BackEditWord(LINEINFO *linelist, int x)
{
	int oldx = x;
	while ( x && (*(EditText + (x - 1)) == ' ') ) x--;
	while ( x && ((UTCHAR)*(EditText + (x - 1)) > ' ') ) x--;
	if ( linelist->mode == listmode_entry ){
		TCHAR *ptr = VFSFindLastEntry(EditText + x);
		int newx;
		newx = ptr - EditText;
		if ( newx < oldx ) x = newx;
	}
	return x;
}

static TCHAR *ClipText_FixPtr(TCHAR *src, int x)
{
	int charlen;
	while ( x > 0 ){
		charlen = TChrlen(*src);
		if ( charlen == 0 ) break;
		x -= charlen;
		#ifdef UNICODE
			src++;
		#else
			src += charlen;
		#endif
	}
	return src;
}

static BOOL OpenClipboardT(HWND hWnd)
{
	int trycount = 6;

	for (;;){
		if ( IsTrue(OpenClipboard(hWnd)) ) return TRUE;
		if ( --trycount == 0 ) return FALSE;
		Sleep(20);
	}
}

static void ClipText(TCINPUTSTRUCT *tis)
{
	COORD pos;
	SIZE range;
	int i;
	TCHAR *src, *srcO, *dst, *spc;
	DWORD readsize, col;
	HGLOBAL hG;

	if ( (RevBuf == NULL) || !IsTrue(OpenClipboardT(NULL)) ) return;

	if ( tis->sinfo.sx > tis->sinfo.cx ){
		pos.X = (SHORT)tis->sinfo.cx;
		range.cx = tis->sinfo.sx - tis->sinfo.cx + 1;
	}else{
		pos.X = (SHORT)tis->sinfo.sx;
		range.cx = tis->sinfo.cx - tis->sinfo.sx + 1;
	}

	if ( tis->sinfo.sy > tis->sinfo.cy){
		pos.Y = (SHORT)tis->sinfo.cy;
		range.cy = tis->sinfo.sy - tis->sinfo.cy + 1;
	}else{
		pos.Y = (SHORT)tis->sinfo.sy;
		range.cy = tis->sinfo.cy - tis->sinfo.sy + 1;
	}
									// 読み込み
	if ( tis->sinfo.LineMode ){
		DWORD allocsize;
		int x;

		if ( range.cy <= 1 ){ // 1行
			allocsize = range.cx + 2;
		}else{ // 複数行
			if ( tis->sinfo.sy > tis->sinfo.cy ){
				pos.X = (SHORT)tis->sinfo.cx;
				range.cx = tis->sinfo.sx;
			}else{
				pos.X = (SHORT)tis->sinfo.sx;
				range.cx = tis->sinfo.cx;
			}
			allocsize = screen.info.dwSize.X - pos.X + 2; // 先頭行
			 // 中間行
			if ( range.cy >= 3 ) allocsize += (range.cy - 2) * (screen.info.dwSize.X + 2);
			allocsize += range.cx + 2; // 末尾行
		}
		srcO = HeapAlloc(hProcHeap, 0, TSTROFF(screen.info.dwSize.X));
		hG = GlobalAlloc(GMEM_MOVEABLE, TSTROFF(allocsize + 2));
		if ( hG == NULL ) return;
		dst = GlobalLock(hG);
		x = pos.X;
		pos.X = 0;
		for ( i = 0 ; i < range.cy ; i++ ){
			#ifndef WINEGCCx
			ReadConsoleOutputCharacter(hStdout, srcO, screen.info.dwSize.X, pos, &readsize);
			#else
				tCsrPos(0, pos.Y);
				innstr(srcO, screen.info.dwSize.X);
			#endif
			pos.Y++;
			src = srcO;
			if ( range.cy <= 1 ){ // 1行
				src = ClipText_FixPtr(src, x);
				readsize = ClipText_FixPtr(src, range.cx) - src;
			}else{
				if ( i == 0 ){ // 1行
					src = ClipText_FixPtr(src, x);
					readsize = ClipText_FixPtr(src, screen.info.dwSize.X - x) - src;
				}else if ( i == (range.cy - 1) ){ // 末尾行
					readsize = ClipText_FixPtr(src, range.cx) - src;
				} // 中間行
			}
			spc = dst;
			for ( col = 0 ; col < readsize ; src++, col++ ){
				UTCHAR c;

				c = (UTCHAR)*src;
				if ( c < (UTCHAR)' ' ) continue;	// 0
				*dst++ = c;
				if ( c != ' ' ) spc = dst;
			}
			// 最終行でない &&(末尾に空白がない || 最終桁まで選択無し)
			if ( ((i + 1) < range.cy) && (dst != spc) ){
				*spc++ = '\r';
				*spc++ = '\n';
				dst = spc;
			}
		}
	}else{
		srcO = HeapAlloc(hProcHeap, 0, TSTROFF(range.cx));
		hG = GlobalAlloc(GMEM_MOVEABLE, TSTROFF((range.cx + 2) * range.cy + 2));
		if ( hG == NULL ) return;
		dst = GlobalLock(hG);
		for ( i = 0 ; i < range.cy ; i++ ){
			#ifndef WINEGCCx
			ReadConsoleOutputCharacter(hStdout, srcO, range.cx, pos, &readsize);
			#else
				tCsrPos(pos.X, pos.Y);
				innstr(srcO, range.cx);
			#endif
			pos.Y++;
			src = srcO;
			#pragma warning(suppress:6387) // GlobalLock は失敗しないと見なす
			spc = dst;
			for ( col = 0 ; col < readsize ; src++, col++ ){
				UTCHAR c;

				c = (UTCHAR)*src;
				if ( c < (UTCHAR)' ' ) continue;	// 0
				*dst++ = c;
				if ( c != ' ' ) spc = dst;
			}
			// 最終行でない &&(末尾に空白がない || 最終桁まで選択無し)
			if ( ((i + 1) < range.cy) && ((dst != spc) ||
					((pos.X + range.cx) < screen.info.dwSize.X)) ){
				*spc++ = '\r';
				*spc++ = '\n';
			}
			dst = spc;
		}
	}
	*dst = '\0';
	HeapFree(hProcHeap, 0, srcO);
	GlobalUnlock(hG);
	EmptyClipboard();
	SetClipboardData(CF_TTEXT, hG);
	CloseClipboard();
}

static int CallKeyHook(PPXAPPINFO *info, WORD key)
{
	PPXMKEYHOOKSTRUCT keyhookinfo;
	PPXMODULEPARAM pmp;

	keyhookinfo.key = key;
	pmp.keyhook = &keyhookinfo;
	return CallModule(info, PPXMEVENT_KEYHOOK, pmp, KeyHookEntry);
}

void MoveCursor(TCINPUTSTRUCT *tis, int newX, int key)
{
	if ( newX < 0 ) newX = 0;

	if ( key & K_s ){	// 選択
		if ( tis->sel.start == tis->sel.end ) tis->sel.end = tis->sel.start = tis->EdX; // 選択開始
		if ( tis->EdX == tis->sel.start ){ // 新しい範囲を決定
			tis->sel.start = newX;
		}else{
			tis->sel.end = newX;
		}
		if ( tis->sel.start == tis->sel.end ){
			tis->sel.end = tis->sel.start = 0; // 選択解除
		}else if ( tis->sel.start > tis->sel.end ){
			int tempX;

			tempX = tis->sel.start;
			tis->sel.start = tis->sel.end;
			tis->sel.end = tempX;
		}
		setflag(tis->eflag, TCI_TI_DRAW);
	}else{				// 非選択
		if ( tis->sel.start != tis->sel.end ){
			tis->sel.start = tis->sel.end = 0;
			setflag(tis->eflag, TCI_TI_DRAW);
		}
	}
	tis->EdX = newX;
}

// ----------------------------------------------------------------------------
void Replace(TCINPUTSTRUCT *tis, const TCHAR *istr, int flags)
{
	size_t len, sellen;

	len = tstrlen(istr);
	sellen = 0;
	if ( flags & REPLACE_ALL ){							// 全て置き換え
		if ( tis->maxsize < len + 1 ) return;
		sellen = tstrlen(EditText);
		tis->EdX = 0;
	}else if ( tis->sel.start != tis->sel.end ){					// 選択
		if ( tis->maxsize < (tis->sel.start +len+ tstrlen(EditText + tis->sel.end) + 1)) return;
		sellen = tis->sel.end - tis->sel.start;
		tis->EdX = tis->sel.start;
	}else if ( tis->insmode == INS_INS ){						// 挿入
		if ( tis->maxsize < (tstrlen(EditText) + len + 1) ) return;
	}else{												// 上書
		do{
			#ifdef UNICODE
				if ( *(EditText + tis->EdX + sellen) != '\0' ){
					sellen++;
				}else{
					break;
				}
			#else
			{
				int clen;

				clen = Chrlen(*(EditText + tis->EdX + sellen));
				if ( clen == 0 ) break;
				sellen += clen;
			}
			#endif
		}while( sellen < len );
		if ( tis->maxsize < (tis->EdX + len + tstrlen(EditText + tis->EdX + sellen)+ 1) ){
			return;
		}
	}

	// "xxx" に "yyy" を挿入したとき、 "yyy"" を回避
	if ( flags & REPLACE_FROMLIST ){
		if ( (len > 1) &&
			 (istr[len - 1] == '\"') &&
			 (EditText[tis->EdX + sellen] == '\"') ){
			len--;
		}
	}

	memmove(EditText + tis->EdX + len, EditText + tis->EdX + sellen, TSTRSIZE(EditText + tis->EdX + sellen));
	memcpy (EditText + tis->EdX,       istr,                         TSTROFF(len));

	tis->sel.start = tis->sel.end = 0;
	if ( flags & REPLACE_SELECT ){
		tis->sel.start = tis->EdX;
		tis->sel.end = tis->EdX + len;
	}
	if ( tis->istrt == IRT_RIGHT ) tis->EdX += len;
	if ( tis->istrt == IRT_BRAKET ) tis->EdX += len - 1;
	setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
}

static int GetMouseX(int ox, int mousex, const TCHAR *dl)
{
	int dispx, x;

	dispx = 0;
	for ( x = ox ; *(dl + x) != '\0' ; ){
		#ifdef UNICODE
			dispx += CCharWide(*(dl + x));
			if ( dispx > mousex ) break;
			x++;
		#else
			dispx += Chrlen(*(dl + x));
			if ( dispx > mousex ) break;
			x += Chrlen(*(dl + x));
		#endif
	}
	return x;
}

// ----------------------------------------------------------------------------
static void USEFASTCALL ReverseLine(WORD *ptr, int len)
{
	int i;

	for ( i = 0 ; i < len ; i++ ){
		*ptr = (WORD)((*ptr & 0xff00) | (~(*ptr) & 0xff) );
		ptr++;
	}
}

static void ReverseRange(TCINPUTSTRUCT *tis, BOOL revon)
{
	COORD pos;
	SIZE range;
	int i;
	WORD *p;
	DWORD s;

	if ( !revon && (RevBuf == NULL) ) return;
	if ( tis->sinfo.sx > tis->sinfo.cx ){
		pos.X = (SHORT)tis->sinfo.cx;
		range.cx = tis->sinfo.sx - tis->sinfo.cx + 1;
	}else{
		pos.X = (SHORT)tis->sinfo.sx;
		range.cx = tis->sinfo.cx - tis->sinfo.sx + 1;
	}

	if ( tis->sinfo.sy > tis->sinfo.cy ){
		pos.Y = (SHORT)tis->sinfo.cy;
		range.cy = tis->sinfo.sy - tis->sinfo.cy + 1;
	}else{
		pos.Y = (SHORT)tis->sinfo.sy;
		range.cy = tis->sinfo.cy - tis->sinfo.sy + 1;
	}
	if ( revon ){
		if ( tis->sinfo.LineMode ){
			SHORT a;
												// 読み込み
			p = RevBuf = HeapAlloc(hProcHeap, 0, screen.info.dwSize.X * range.cy * sizeof(WORD));
			if ( p == NULL ) return;
			a = pos.X;
			pos.X = 0;
			for ( i = 0 ; i < range.cy ; i++ ){
				ReadConsoleOutputAttribute(hStdout, p, screen.info.dwSize.X, pos, &s);
				p += s;
				pos.Y++;
			}
			pos.X = a;
		}else{
												// 読み込み
			p = RevBuf = HeapAlloc(hProcHeap, 0, range.cx * range.cy * sizeof(WORD));
			if ( p == NULL ) return;
			for ( i = 0 ; i < range.cy ; i++ ){
				ReadConsoleOutputAttribute(hStdout, p, range.cx, pos, &s);
				p += s;
				pos.Y++;
			}
		}
	}
												// 反転
	if ( tis->sinfo.LineMode ){
		if ( range.cy <= 1 ){ // 1行
			ReverseLine(RevBuf + pos.X, range.cx);
		}else{ // 複数行
			if ( tis->sinfo.sy > tis->sinfo.cy ){
				pos.X = (SHORT)tis->sinfo.cx;
				range.cx = tis->sinfo.sx;
			}else{
				pos.X = (SHORT)tis->sinfo.sx;
				range.cx = tis->sinfo.cx;
			}

			ReverseLine(RevBuf + pos.X, screen.info.dwSize.X - pos.X); // 先頭行
			if ( range.cy >= 3 ){ // 中間行
				for ( i = 1 ; i < (range.cy - 1) ; i++ ){
					ReverseLine(RevBuf + screen.info.dwSize.X * i, screen.info.dwSize.X);
				}
			}
			ReverseLine(RevBuf + screen.info.dwSize.X * (range.cy - 1), range.cx); // 末尾行
		}
		range.cx = screen.info.dwSize.X;
		pos.X = 0;
	}else{
		ReverseLine(RevBuf, range.cx * range.cy);
	}
												// 表示
	if ( revon ) pos.Y -= (SHORT)range.cy;

	p = RevBuf;
	for ( i = 0 ; i < range.cy ; i++ ){
		#ifndef WINEGCCx
			WriteConsoleOutputAttribute(hStdout, p, range.cx, pos, &s);
		#else
			tFillAtr(pos.X, pos.Y, pos.X + range.cx, pos.Y, TC_edit_select);
		#endif
		p += s;
		pos.Y++;
	}

	if ( !revon ){								// 解放
		HeapFree(hProcHeap, 0, RevBuf);
		RevBuf = NULL;
	}
}

// ----------------------------------------------------------------------------
static void ReverseText(TCINPUTSTRUCT *tis, int len)
{
	WORD temp[TSIZEOF(tis->incsearch.text)];
	DWORD tempsize;

	ReadConsoleOutputAttribute(hStdout, temp, len, tis->incsearch.pos, &tempsize);
												// 反転
	ReverseLine(temp, len);
	WriteConsoleOutputAttribute(hStdout, temp, len, tis->incsearch.pos, &tempsize);
}

#define INCSEARCH_LETTER	0
#define INCSEARCH_NEXT	1
#define INCSEARCH_FIRST	2
#define INCSEARCH_ONSCREEN	3
#define INCSEARCH_BACK	4
#define INCSEARCH_LAST	5

static BOOL ScrIncSearch(TCINPUTSTRUCT *tis, int next)
{
	COORD pos;
	TCHAR *temptext;
	int oldlen, posX, lastY;
	DWORD getlen;

	pos.X = 0;
	pos.Y = tis->incsearch.pos.Y;
	posX = tis->incsearch.pos.X;
	oldlen = tis->incsearch.len;
	temptext = HeapAlloc(hProcHeap, 0, TSTROFF(screen.info.dwSize.X) + sizeof(tis->incsearch.text));
	if ( temptext == NULL ) return FALSE;
	if ( next >= INCSEARCH_BACK ){
		if ( next == INCSEARCH_LAST ){
			posX = 0;
			pos.Y = screen.info.dwSize.Y;
		}
		posX--;
		if ( posX < 0 ){
			pos.Y--;
			posX = (SHORT)(screen.info.dwSize.X - 1);
		}
		for ( ; pos.Y >= 0 ; pos.Y-- ){
			getlen = screen.info.dwSize.X + TSIZEOF(tis->incsearch.text) - 1;
			if ( ReadConsoleOutputCharacter(
					hStdout, temptext, getlen, pos, &getlen) ){
				temptext[getlen] = '\0';
				#ifdef UNICODE
					if ( posX > 0 ) posX = GetMouseX(0, posX, temptext);
				#endif
				for ( ; posX >= 0 ; posX-- ){
					if ( memicmp(temptext + posX, tis->incsearch.text, TSTROFF(tis->incsearch.len)) == 0 ){
						goto found;
					}
				}
			}
			posX = (SHORT)(screen.info.dwSize.X - 1);

			if ( WaitForSingleObject(hStdin, 0) == WAIT_OBJECT_0 ) break;
		}
		goto fault;
	}

	if ( next != INCSEARCH_LETTER ){ // 次を検索
		posX++;
		lastY = screen.info.dwSize.Y;
		if ( next == INCSEARCH_FIRST ) posX = pos.Y = 0; // 最初から検索
		if ( next == INCSEARCH_ONSCREEN ){ // 画面内で検索
			posX = 0;
			pos.Y = screen.info.srWindow.Top;
			lastY = screen.info.srWindow.Bottom;
		}
	}else{ // INCSEARCH_LETTER
		if ( tis->incsearch.len == 1 ){ // 検索開始
			if ( tis->sinfo.cy == tis->sinfo.BackupY ){ // 編集行なら画面左上
				posX = 0;
				pos.Y = screen.info.srWindow.Top;
			}else{ // カーソル移動済みならその場から
				posX = (SHORT)tis->sinfo.cx;
				pos.Y = (SHORT)tis->sinfo.cy;
			}
		}
		oldlen--; // 反転消すときは1文字減らす
		lastY = screen.info.srWindow.Bottom;
	}

	for ( ; pos.Y < lastY ; pos.Y++ ){
		getlen = screen.info.dwSize.X + TSIZEOF(tis->incsearch.text) - 1;
		if ( ReadConsoleOutputCharacter(
				hStdout, temptext, getlen, pos, &getlen) == FALSE ){
			break;
		}
		temptext[getlen] = '\0';
		if ( getlen >= (DWORD)screen.info.dwSize.X ){
			getlen = (DWORD)screen.info.dwSize.X;
		}
		#ifdef UNICODE
			if ( posX > 0 ) posX = GetMouseX(0, posX, temptext);
		#endif
		for ( ; posX < (SHORT)getlen ; posX++ ){
			if ( memicmp(temptext + posX, tis->incsearch.text, TSTROFF(tis->incsearch.len)) == 0 ){
				goto found;
			}
		}
		posX = 0;

		if ( WaitForSingleObject(hStdin, 0) == WAIT_OBJECT_0 ) break;
	}
fault:
	HeapFree(hProcHeap, 0, temptext);
	return FALSE;
found:
	if ( oldlen ) ReverseText(tis, oldlen);
#ifdef UNICODE
	{
		int x = 0, fixedx = 0;

		for (;;){
			if ( x >= posX ) break;
			fixedx += (temptext[x] >= 0x100) ? 2 : 1;
			x++;
		}
		posX = fixedx;
		tis->incsearch.pos.X = (SHORT)fixedx;
	}
#else
	tis->incsearch.pos.X = (SHORT)posX;
#endif
	tis->incsearch.pos.Y = pos.Y;
	MoveCursorS(tis, posX - tis->sinfo.cx, pos.Y - tis->sinfo.cy, 0);
	ReverseText(tis, tis->incsearch.len);
	HeapFree(hProcHeap, 0, temptext);
	return TRUE;
}

//----------------------------------------------------------------------------
static void USEFASTCALL HilightLine(int colorindex)
{
	tFillAtr(0, screen.info.dwCursorPosition.Y,
			screen.info.dwSize.X - 1, screen.info.dwCursorPosition.Y,
			colorindex);
}

#ifdef UNICODE
static int GetScreenX(TCINPUTSTRUCT *tis, int x)
{
	int i, nx = 0;

	for ( i = 0 ; i < x ; i++ ){
		WCHAR c;

		c = *(EditText + tis->ShowOffset + i);
		if ( c == '\0' ) break;
		nx += CCharWide(c);
	}
	return nx;
}
#else
#define GetScreenX(tis, x) x
#endif

void DisplayColorLine(TCINPUTSTRUCT *tis, TCHAR *dispbuf, int posY)
{
	BYTE *attrptr, *ptr, nowattr;
	int width = screen.info.dwSize.X - 1;
	DWORD dummy;
	TCHAR escbuf[100], *last;
	COLORREF fg, bg;
	TCHAR *p;

	last = thprintf(escbuf, TSIZEOF(escbuf), CSI_T T("%d;1H"), posY + 1);
	WriteConsole(hStdout, escbuf, last - escbuf, &dummy, NULL);
	attrptr = tis->attribute + tis->ShowOffset;
	nowattr = *attrptr;
	ptr = attrptr + 1;
	p = dispbuf;
	fg = ConRefColor(SynColorC.edit.text);
	bg = ConRefColor(SynColorC.edit.back);
	for (;;){
		if ( (width == 0) || (nowattr != *ptr) ){
			if ( attrptr < ptr ){
				EDITCOLORS *ec;
				BYTE mask;
				COLORREF cl;

				mask = (BYTE)(nowattr & SYNTAXCOLOR_SELECT_MASK);
				nowattr &= (BYTE)(~SYNTAXCOLOR_SELECT_MASK);
				ec = SynColorC.color + nowattr;

				if ( mask && (SynColorC.select.text < C_S_AUTO) ){
					cl = ConRefColor(SynColorC.select.text);
				}else{
					cl = ConRefColor(ec->text);
					if ( cl >= C_S_AUTO ){
						cl = fg;
					}else if ( cl != fg ){
						fg = cl;
					}
				}
				if ( cl < C_S_AUTO ){
					last = thprintf(escbuf, 32, CSI_T T("38;2;%d;%d;%d"),
							cl & 0xff, (cl >> 8) & 0xff, (cl >> 16) & 0xff);
				}else{
					escbuf[0] = ESC_B;
					escbuf[1] = '[';
					last = escbuf + 2;
				}

				if ( mask && (SynColorC.select.back < C_S_AUTO) ){
					cl = ConRefColor(SynColorC.select.back);
				}else{
					cl = ConRefColor(ec->back);
					if ( cl >= C_S_AUTO ){
						cl = bg;
					}else if ( cl != bg ){
					//	bg = cl;
					}
				}
				if ( cl < C_S_AUTO ){
					if ( last > (escbuf + 2) ) *last++ = ';';
					last = thprintf(last, 32, T("48;2;%d;%d;%dm"),
							cl & 0xff, (cl >> 8) & 0xff, (cl >> 16) & 0xff);
				}else if ( last == (escbuf + 2) ){
					last = escbuf;
				}else{
					*last++ = 'm';
				}
				if ( last > escbuf ){
					WriteConsole(hStdout, escbuf, last - escbuf, &dummy, NULL);
				}
				WriteConsole(hStdout, p, ptr - attrptr, &dummy, NULL);
			}
			if ( width == 0 ) break;
			p += ptr - attrptr;
			attrptr = ptr;
			nowattr = *ptr;
		}
		ptr++;
		width--;
	}
}

//----------------------------------------------------------------------------
static void DisplayLine(TCINPUTSTRUCT *tis, TCHAR *buf)
{
	#ifdef UNICODE
	WCHAR c;
	#endif
	TCHAR dispbuf[CMDLINESIZE], *p;
	int s = screen.info.dwSize.X, i, j, linecolor;
	DWORD dummy;

	if ( tis->useattr == 1 ){
		memset(&tis->attribute, SYNTAXCOLOR_edit, CMDLINESIZE);
		CommandSyntaxColor(PPXH_COMMAND, buf, tis->attribute);
		if ( tis->sel.start != tis->sel.end ){
			BYTE *ptr, *ptrmax;

			ptr = tis->attribute + tis->sel.start;
			ptrmax = tis->attribute + tis->sel.end;
			for (;;){
				if ( ptr >= ptrmax ) break;
				*ptr++ |= SYNTAXCOLOR_SELECT_MASK;
			}
		}
//		tis->useattr = 2;
	}

	p = dispbuf;
	buf += tis->ShowOffset;
					// 表示用データを作成する
	for ( ; s ; s-- ){
				// 未使用領域
		if ( *buf == '\0' ){
			*p++ = ' ';
			continue;
		}
		#ifdef UNICODE
		c = *buf++;
		if ( CCharWide(c) > 1 ){
			if ( s > 1 ){
				s--;
			}else{
				c = ' ';
			}
		}
		*p++ = c;
		#else
		if ( IskanjiA(*p++ = *buf++) ){
			if ( s > 1 ){
				*p++ = *buf++;
				s--;
			}else{
				*(p - 1) = ' ';
			}
		}
		#endif
	}
	*p = '\0';

	if ( tis->useattr ){
		DisplayColorLine(tis, dispbuf, screen.info.dwCursorPosition.Y);
		return;
	}

	linecolor = (tis->cmode < CMODE_LOG_VIEW) ? TC_edit : TC_ref;
	tSetColor(linecolor);
	tCsrPos(0, screen.info.dwCursorPosition.Y);

								// 範囲選択処理 -------------------------------
	s = tis->ShowOffset + GetScreenX(tis, screen.info.dwSize.X);
	if ( (tis->ShowOffset >= (int)tis->sel.end) || (s <= (int)tis->sel.start)){	// 選択範囲が画面上にない
		WriteConsole(hStdout, dispbuf, screen.info.dwSize.X, &dummy, NULL);
	}else{
		if ( X_cone < ConsoleColor_ESC ){
			WriteConsole(hStdout, dispbuf, screen.info.dwSize.X, &dummy, NULL);
		}
		i = 0;
		j = s - tis->ShowOffset;

		if ( tis->ShowOffset < (int)tis->sel.start ){ // 範囲左端が画面内→左端まで選択解除
			i = GetScreenX(tis, tis->sel.start - tis->ShowOffset);
			if ( X_cone >= ConsoleColor_ESC ){
				WriteConsole(hStdout, dispbuf, i, &dummy, NULL);
			}else{
				tFillAtr( 0,     screen.info.dwCursorPosition.Y,
						  i - 1, screen.info.dwCursorPosition.Y, linecolor);
			}
		}

		if ( s > (int)tis->sel.end ){ // 範囲右端が画面内→右端以降を選択解除
			j = GetScreenX(tis, tis->sel.end - tis->ShowOffset);
		}

											// 範囲を選択
		if ( X_cone >= ConsoleColor_ESC ){
			tSetColor(linecolor + 1);
			WriteConsole(hStdout, dispbuf + i, j - i, &dummy, NULL);
		}else{
			tFillAtr( i    , screen.info.dwCursorPosition.Y,
					  j - 1, screen.info.dwCursorPosition.Y, linecolor + 1);
		}

		if ( s > (int)tis->sel.end ){ // 範囲右端が画面内→右端以降を選択解除
			if ( X_cone >= ConsoleColor_ESC ){
				tSetColor(linecolor);
				WriteConsole(hStdout, dispbuf + j, s - tis->ShowOffset - j, &dummy, NULL);
			}else{
				tFillAtr(j, screen.info.dwCursorPosition.Y,
					 s - tis->ShowOffset - 1, screen.info.dwCursorPosition.Y,
					linecolor);
			}
		}
	}
}
// ----------------------------------------------------------------------------
#define BackupLine(dl, bak) tstrcpy(bak, dl)
// ----------------------------------------------------------------------------
static void SetScrollMode(TCINPUTSTRUCT *tis)
{
	if ( tis->cmode < CMODE_LOG_VIEW ){
		tis->cmode = CMODE_LOG_VIEW;
		tis->incsearch.len = 0;
		tis->sinfo.cx = tis->EdX - tis->ShowOffset;
		tis->sinfo.cy = tis->sinfo.BackupY = screen.info.dwCursorPosition.Y;
		setflag(tis->eflag, TCI_TI_DRAW);
	}
}
static void SetSelectMode(TCINPUTSTRUCT *tis, MOUSE_EVENT_RECORD *me)
{
	tis->sinfo.sx = tis->sinfo.cx = me->dwMousePosition.X;
	tis->sinfo.sy = tis->sinfo.cy = me->dwMousePosition.Y;
	tis->sinfo.LineMode = TRUE;
	tCsrPos(tis->sinfo.cx, tis->sinfo.cy);
	tis->cmode = CMODE_LOG_SELECT;
}

// ----------------------------------------------------------------------------
static void TPushEditStack(TCHAR mode, TCHAR *str, int b, int t)
{
	TCHAR buf[CMDLINESIZE];

	memcpy(buf, str + b, TSTROFF(t - b));
	buf[t - b] = '\0';
	PushTextStack(mode, buf);
}
// ----------------------------------------------------------------------------
static void InitHistorySearch(HISTVAR *hivar, TCHAR *findstr, TCHAR *buflast)
{
	hivar->index = -1;
	hivar->count = 0;
	hivar->mode = -1;
	hivar->findstr = findstr;
	hivar->findbuflast = buflast;
	hivar->findsize = 0;
}
// ----------------------------------------------------------------------------
// ヒストリの検索方法を決定する
static void GetHistorySearchMode(TCINPUTSTRUCT *tis, TCHAR *dl, TCHAR *buflast)
{
	TCHAR *p, *q, *cmd, *s;

	if ( tis->EdX == 0 ){							// カーソルが先頭…全体
		InitHistorySearch(&tis->hvar, dl, buflast);
		tis->hvar.mode = HIST_CMD_ALL;
		return;
	}
												// カーソルが末尾…前回と同じ
	if ( (tis->hvar.mode != -1) && (*(dl + tis->EdX) == '\0') ) return;

										// カーソルがどこを示しているか調べる
	p = dl;
	q = dl + tis->EdX;
	s = NULL;
	while ( (p < q) && (*p == ' ') ) p++;	// 先頭の空白は無視
	cmd = p;
	while ( p < q ){
		if ( *p == ' ' ) s = p + 1;
		p++;
	}
	InitHistorySearch(&tis->hvar, dl, buflast);
	if ( s == NULL ){ // 空白がなかった→コマンド検索
		tis->hvar.mode = (cmd == p) ? HIST_CMD_ALL : HIST_CMD_FIND; // 文字列なし→全体検索
		tis->hvar.findstr = cmd; // 文字列有り→コマンド前方一致検索
		tis->hvar.findbuflast = buflast;
		tis->hvar.findsize = p - cmd;
	}else{					// 空白有り→パラメータ検索
		tis->hvar.mode = (s == p) ? HIST_PARAM_ALL : HIST_PARAM_FIND; // 文字列なし→パラ検索
		tis->hvar.findstr = s; // 文字列有り→パラ前方一致検索
		tis->hvar.findbuflast = buflast;
		tis->hvar.findsize = p - s;
		tis->hvar.count = 1;
	}
	return;
}
// ----------------------------------------------------------------------------
static const TCHAR *NextWord(const TCHAR *p)
{
	while ( *p && ((UTCHAR)*p <= (UTCHAR)' ') ) p++; // 単語前の空白をスキップ
	while ( *p && ((UTCHAR)*p >  (UTCHAR)' ') ) p++; // 単語自体をスキップ
	while ( *p && ((UTCHAR)*p <= (UTCHAR)' ') ) p++; // 単語の後の空白をスキップ
	return p;
}
// ----------------------------------------------------------------------------
static const TCHAR *SearchLineHistory2(HISTVAR *hivar, const TCHAR *p)
{
	switch( hivar->mode ){
		case HIST_CMD_ALL:			// 全体検索
			break;

		case HIST_CMD_FIND:			// コマンド前方一致検索
			while ( *p == ' ' ) p++;
			if ( (tstrlen(p) < hivar->findsize) ||
				 tstrnicmp(p, hivar->findstr, hivar->findsize) ){
				p = NULL;
			}
			break;

		case HIST_PARAM_ALL: {		// パラメータ検索
			int i;

			for ( i = hivar->count ; i ; i-- ){
				p = NextWord(p);
				if ( *p == '\0' ){
					if ( hivar->count == HISCOUNT_MAX ){
						hivar->count -= i;
						p = NULL;
					}
					return p;
				}
			}
			break;
		}
		case HIST_PARAM_FIND:{		// パラメータ前方一致検索
			int i;

			for ( i = hivar->count ; i ; i-- ){
				p = NextWord(p);
				if ( *p == '\0' ){
					if ( hivar->count == HISCOUNT_MAX ){
						hivar->count -= i;
						p = NULL;
					}
					return p;
				}
			}
			if ( (tstrlen(p) < hivar->findsize) ||
				 tstrnicmp(p, hivar->findstr, hivar->findsize) ){
				p = NULL;
			}
			break;
		}
	}
	if ( p ){
		if ( hivar->mode <= HIST_CMD_FIND ){ // HIST_CMD_ALL / HIST_CMD_FIND
			tstrcpy(hivar->findstr, p);
		}else{
			const UTCHAR *src;
			UTCHAR *dst;

			src = (const UTCHAR *)p;
			dst = (UTCHAR *)hivar->findstr;
			while( *src > ' ' ) *dst++ = *src++;
			*dst = '\0';
		}
	}
	return p;
}
// ----------------------------------------------------------------------------
static const TCHAR *SearchLineHistory(HISTVAR *hivar, int offset)
{
	const TCHAR *p;

#ifdef __DEBUG
	TCHAR buf[100];

	thprintf(buf, TSIZEOF(buf), T("Mode:%d Index:%3d Count:%3d"),
			hivar->mode, hivar->index, hivar->count);
	tputposstr(0, screen.info.dwCursorPosition.Y + 1, buf);
#endif
	if ( offset >= 0 ){			// 古いほうへ検索
		if ( hivar->count != 0 ){
			if ( hivar->index == -1 ) hivar->index = 0;
			p = EnumHistory(hivar->type, hivar->index);
			if ( p ) for ( ; ; ){
				for ( ; ; ){
					const TCHAR *q;

					q = SearchLineHistory2(hivar, p);
					if ( q != NULL ){
						if ( *q == '\0' ) break;
						hivar->count++;
						return q;
					}
					hivar->count++;
				}
				p = EnumHistory(hivar->type, hivar->index + 1);
				if ( p == NULL ) break;
				hivar->index++;
				hivar->count = 1;
			}
		}else{
			do{
				p = EnumHistory(hivar->type, hivar->index + 1);
				if ( p == NULL) break;
				hivar->index++;
				p = SearchLineHistory2(hivar, p);
			}while( p == NULL );
		}
	}else{						// 新しいほうへ検索
		p = NULL;
		if ( hivar->index != -1 ){
			if ( hivar->count != 0 ){
				for ( ; ; ){
					p = EnumHistory(hivar->type, hivar->index);
					if ( p != NULL ){
						if ( hivar->count < 1 ) hivar->count = HISCOUNT_MAX + 1;
						hivar->count--;

						while ( hivar->count != 0 ){
							const TCHAR *q;

							q = SearchLineHistory2(hivar, p);
							if ( q != NULL ){
								if ( *q == '\0' ) break;
								return q;
							}
							hivar->count--;
						}
					}
					hivar->index--;
					if ( hivar->index == -1 ) break;
				}
			}else{
				hivar->index--;
				while( hivar->index > -1 ){
					p = EnumHistory(hivar->type, hivar->index);
					if ( p == NULL ) break;
					p = SearchLineHistory2(hivar, p);
					if ( p != NULL ) break;
					hivar->index--;
				}
			}
		}
	}
	return p;
}

static void MakeFileList(TCINPUTSTRUCT *tis, TCHAR *mask, ECURSOR *cursor)
{
	LINEINFO *linelist = &tis->linelist;
		TCHAR tmp[CMDLINESIZE];
		ECURSOR cur;

	linelist->item_min = 1;
	linelist->item_index = 0;

	linelist->cursor = *cursor;

	if ( linelist->mode == listmode_entry ){
		TCHAR *ptr;

		tstrcpy(tmp, mask);
		SearchFileIned(&tis->ComplED, T(""), NULL, 0);
		ptr = SearchFileIned(&tis->ComplED, tmp, &linelist->cursor, CMDSEARCH_MULTI | CMDSEARCH_FLOAT | CMDSEARCH_ROMA | CMDSEARCH_ONE | CMDSEARCH_MAKELIST);
		if ( ptr != NULL ){
			TCHAR backupchr;

			// 編集内容を保存
			backupchr = mask[linelist->cursor.end];
			mask[linelist->cursor.end] = '\0';
			ThListAddStr(&linelist->list, mask + linelist->cursor.start);
			mask[linelist->cursor.end] = backupchr;

			for (;;){ // 検索結果を保存
				if ( (tis->cmode != CMODE_WALKDIR) || IsSDir(ptr) ){
					if ( ThListAddStr(&linelist->list, ptr) == FALSE ) break;
				}

				ptr = SearchFileIned(&tis->ComplED, tmp, &linelist->cursor, CMDSEARCH_MULTI | CMDSEARCH_FLOAT | CMDSEARCH_ROMA | CMDSEARCH_ONE | CMDSEARCH_MAKELIST);
				if ( ptr == NULL ) break;
			}
			SearchFileIned(&tis->ComplED, T(""), NULL, 0);
		}
	}

	tstrcpy(tmp, mask);
	cur = *cursor;
	GetWordStrings(tmp, &cur, 0);
	if ( *(tmp + cur.start) != '\0' ){
		if ( ScreenCapText == NULL ){
			DWORD allocsize;
			COORD pos = {0, 0};

			allocsize = screen.info.dwSize.X * screen.info.dwSize.Y + 2;
			ScreenCapText = HeapAlloc(hProcHeap, 0, TSTROFF(allocsize));
			ReadConsoleOutputCharacter(hStdout, ScreenCapText, allocsize, pos, &ScreenCapSize);
		}

		if ( ScreenCapSize > 0 ){
			TCHAR *search, *top, *end, *last, backup;
			BOOL chkbreak;
//			int cnt = 0;

			search = ScreenCapText;
			last = ScreenCapText + ScreenCapSize;

			for(; search < last;){
				search = tstristr(search, tmp + cur.start);
				if ( search == NULL ) break;
				top = search;
				if ( (UTCHAR)*top > ' ' ){
					for ( top = search; top > ScreenCapText; top--){
						if ( (UTCHAR)*(top - 1) <= ' ' ) break;
					}
				}
				end = search + tstrlen(tmp + cur.start);//cur.end - cur.start;
				for ( ; end < last; end++){
					if ( (UTCHAR)*end <= ' ' ) break;
				}
				// 編集内容を保存
				if ( ThListGetCount(&linelist->list) <= 1 ){
					ThListAddStr(&linelist->list, tmp + cur.start);
				}

//				XMessage(NULL, NULL, XM_DbgDIA, T("%d %d"),);
				//cnt++;
				backup = *end;
				*end = '\0';
//				XMessage(NULL, NULL, XM_DbgDIA, T("x[%d] %s"),cnt,top);
				chkbreak = ThListAddStr(&linelist->list, top);
				*end = backup;
				if ( chkbreak == FALSE ) break;
				search = end;
			}
//			XMessage(NULL, NULL, XM_DbgDIA, T("[%d] %s"),cnt,tmp + cur.start);
		}

		{
			const TCHAR *hisp;
			TCHAR *p;
			HISTVAR hivar;

			p = tmp + cur.start;
			InitHistorySearch(&hivar, p, tmp + TSIZEOF(tmp) - cur.start);
			hivar.mode = HIST_CMD_ALL;
			hivar.type = tis->hvar.type;
			hivar.findstr = p;
			hivar.findsize = tstrlen(p);
			hivar.index = 0;
			hisp = EnumHistory(hivar.type, hivar.index);
			if ( hisp != NULL ){
				for ( ; ; ){
					for ( ; ; ){
						const TCHAR *q;

						q = tstristr(hisp, hivar.findstr); // SearchLineHistory2(&hivar, p);
						if ( q != NULL ){
							if ( *q == '\0' ) break;
							hivar.count++;

			// 編集内容を保存
			if ( ThListGetCount(&linelist->list) <= 1 ){
				ThListAddStr(&linelist->list, tmp + cur.start);
			}
			if ( ThListAddStr(&linelist->list, hisp) == FALSE ) goto endhist;

						}
						hivar.count++;
						break;
					}
					hisp = (TCHAR *)EnumHistory(hivar.type, hivar.index + 1);
					if ( hisp == NULL ) break;
					hivar.index++;
					hivar.count = 1;
				}
			}
			endhist: ;
		}
	}

	if ( ThListGetCount(&linelist->list) <= 1 ){
		ThListFree(&linelist->list);
		tputstr( T("\r\n* no match *") );
		return;
	}
	linelist->show = TRUE;
	if ( (tis->cmode == CMODE_EDIT) || (tis->cmode >= CMODE_LOG_VIEW) ){
		tis->cmode = CMODE_LIST;
	}

	tputstr(
		T("\r\n") T("\r\n") T("\r\n") T("\r\n")
		T("\r\n") T("\r\n") T("\r\n") T("\r\n")
		T("\r\n") T("\r\n") T("\r\n") T("\r\n")
		T("\r\n") T("\r\n") T("\r\n") T("\r\n")
	);
	tConsoleInit();
	linelist->show_min_Y = screen.info.dwCursorPosition.Y - tis->list_height - 1;
	screen.info.dwCursorPosition.Y = (short)(linelist->show_min_Y - 1);
	ShowLineList(tis);

	tSetDefaultColor();
//	tConsoleInit();
}

void ResettCInput(TCINPUTSTRUCT *tis, TCHAR *buf, BOOL first)
{
	if ( first ){
		memset(tis, 0, sizeof(TCINPUTSTRUCT));
		tis->linelist.mode = listmode_off;
		tis->linelist.show = FALSE;
		ThListInit(&tis->linelist.list);
		tis->cmode = CMODE_EDIT;
		tis->list_height = default_list_height;
	}
	tis->insmode = INS_INS;
	tis->ShowOffset = 0;
	buf[0] = '\0';
	tis->baseptr = EditText = buf;
	tis->sel.start = tis->sel.end = 0;
	tis->EdX = 0;
}

// ----------------------------------------------------------------------------
int tCInput(TCHAR *buf, size_t bsize, WORD htype)
{
	INPUT_RECORD cin;
	int X_calc = 1;
	int ScreenWidth;

	statuslen = 0;
	tis.useattr = 0;

	tis.ComplED.info = &ppbappinfo;
	tis.ComplED.hF = NULL;
	tis.ComplED.romahandle = 0;

	tis.mouseSX = -1;
	tis.maxsize = bsize;
	tis.cmode = CMODE_EDIT;
	tis.baseptr = EditText = buf;
	tis.eflag = eflag_first;
	tstplimcpy(tis.backuptext, EditText, CMDLINESIZE);
	RevBuf = NULL;
	SetIMEDefaultStatus(hHostWnd);
	InitHistorySearch(&tis.hvar, EditText, EditText + CMDLINESIZE);
	tis.histype = tis.hvar.type = htype;

	GetConsoleWindowInfo(hStdout, &screen.info);
	*TMC_Default = screen.info.wAttributes;

	if ( screen.info.dwCursorPosition.X != 0 ){ // 改行していないなら改行
		tputstr(T("\r\n"));
		screen.info.dwCursorPosition.Y++;
	}

	if ( tis.eflag != TCI_TI_FIRST ) tConsoleInit();

	InitSyntaxColor(&tis, -1);

	// 現在位置が画面外なら調整
	if ( (screen.info.dwCursorPosition.Y + 1) >= screen.info.dwSize.Y ){
		COORD pos;

		pos.Y = screen.info.dwCursorPosition.Y;
		pos.X = (SHORT)(screen.info.dwSize.X - 1);
		SetConsoleCursorPosition(hStdout, pos);
		if ( ConsoleHost == ConsoleHost_conhost ){ // conhost 用
			tputstr(T(" ")); // 反映
			tConsoleInit();
		}else{ // Windows terminal 用
			tputstr(T("\n\n")); // 反映
			tConsoleInit();
			pos.Y = screen.info.dwCursorPosition.Y;
			pos.X = 0;
			SetConsoleCursorPosition(hStdout, pos);
			screen.info.dwCursorPosition.Y--;
		}
	}
	ScreenWidth = screen.info.dwSize.X;
	GetCustData(T("X_calc"), &X_calc, sizeof(X_calc));
	do{
		int key;			// 現在のキーコード

		tis.istrt = IRT_RIGHT;
											// 表示位置の補正
		if ( tis.ShowOffset > tis.EdX ){
			tis.ShowOffset = tis.EdX;
			setflag(tis.eflag, TCI_TI_DRAW);
		}
		#ifdef UNICODE
		{
			int x, wide = 0;
			for ( x = tis.ShowOffset ; x < tis.EdX ; x++ ){
				wide += CCharWide(*(EditText + x));
			}
			while ( screen.info.dwSize.X <= wide ){
				wide -= CCharWide(*(EditText + tis.ShowOffset));
				tis.ShowOffset++;
				setflag(tis.eflag, TCI_TI_DRAW);
			}
		}
		#else
			while ( screen.info.dwSize.X <= (tis.EdX - tis.ShowOffset) ){
				tis.ShowOffset += Chrlen(*(EditText + tis.ShowOffset));
				setflag(tis.eflag, TCI_TI_DRAW);
			}
		#endif
											// 表示処理
		if ( tis.eflag & TCI_TI_DRAW ){
			TCHAR form[GetCalc_ResultMaxLength];

			DisplayLine(&tis, EditText);
			resetflag(tis.eflag, TCI_TI_DRAW);
			tSetDefaultColor();

			if ( statuslen ){
				statuslen = 0;
				if ( (screen.info.dwCursorPosition.Y + 1) < screen.info.dwSize.Y ){
					ClearStatusLine();
				}
			}
			if ( X_calc && IsTrue(GetCalc(EditText, form, NULL)) ){
				ShowStatusText(form);
			}
		}
		if ( (tis.eflag & TCI_TI_LIST_CHANGE_TARGET) && (tis.linelist.mode != listmode_off) && (tis.cmode < CMODE_LOG_VIEW) ){
			ECURSOR cursor;
			TCHAR tmp[CMDLINESIZE];

			resetflag(tis.eflag, TCI_TI_LIST_CHANGE_TARGET);
			CloseFileList(&tis);
			tstrcpy(tmp, EditText);
			if ( tis.sel.start == tis.sel.end ){
				cursor.start = tis.EdX;
				cursor.end	 = tis.EdX;
			}else{
				cursor.start = tis.sel.start;
				cursor.end	 = tis.sel.end;
			}
			tis.linelist.mode = listmode_entry;
			MakeFileList(&tis, tmp, &cursor);
		}

		if ( tis.eflag & TCI_TI_EXTRA ){
			if ( tis.eflag & TCI_TI_SCROLL ){
				tis.eflag = 0;
			}else if ( tis.eflag & TCI_TI_FIRST ){
				tis.eflag = eflag_first = TCI_TI_DRAW;
				TconCommonCommand(&tis, K_E_FIRST);
				if ( first_command != NULL ){
					PP_ExtractMacro(hHostWnd, &ppbappinfo, NULL, first_command, NULL, (XEO_CONSOLE | XEO_NOUSEPPB) );
					first_command = NULL;
				}
				tConsoleInit();
				continue;
			}
		}else if ( tis.cmode < CMODE_LOG_VIEW ){
			#ifdef UNICODE
				WCHAR *p;
				int i, x;

				tCsrMode(tis.insmode);
				for ( i = tis.EdX - tis.ShowOffset, x = 0, p = EditText + tis.ShowOffset ; i ; i--, p++ ){
					x += TChrlen(*p);
				}
				tCsrPos(x, screen.info.dwCursorPosition.Y);
			#else
				tCsrMode(tis.insmode);
				tCsrPos(tis.EdX - tis.ShowOffset, screen.info.dwCursorPosition.Y);
			#endif
		// カーソルが末尾でないなら、ヒストリの検索方法のキャッシュを解除する。
			if (*(EditText + tis.EdX)) InitHistorySearch(&tis.hvar, EditText, EditText + CMDLINESIZE);
		}else{ //  if ( cmode >= CMODE_LOG_VIEW ){
			tCsrMode(100);
			tCsrPos(tis.sinfo.cx, tis.sinfo.cy);
		}
		do{
			key = tgetin(&cin, INFINITE);
			if ( cin.EventType == MOUSE_EVENT ){
				key = MouseCommand(&tis, &cin.Event.MouseEvent);
			}else if ( cin.EventType == WINDOW_BUFFER_SIZE_EVENT ){
				if ( ScreenWidth != screen.info.dwSize.X ){
					int y;

					key = KEY_DUMMY;
					tis.eflag = TCI_TI_DRAW;

					y = ScreenWidth / screen.info.dwSize.X + 1;
					if ( y > 1 ){ // 幅が狭くなった→２行以上に跨いで表示しているので、余分な行を消去
						tFillBox(0,
								screen.info.dwCursorPosition.Y + 1,
								screen.info.dwSize.X - 1,
								screen.info.dwCursorPosition.Y + y,
								TC_execute);
					}
					ScreenWidth = screen.info.dwSize.X;
				}
			}
		}while( key == 0 );

		if ( key > 0 ){
			if ( KeyHookEntry != NULL ){
				if ( CallKeyHook(&ppbappinfo, (WORD)key) != PPXMRESULT_SKIP ){
					continue;
				}
			}
			TconCommonCommand(&tis, key);
		}else{
			if ( key == KEY_RECV ){			// コマンド受信（PPb用）
				if ( EditText != buf ) tstrcpy(buf, EditText);
				tis.eflag = TCI_RECV;
			}
		}
	}while( tis.eflag >= 0 );
	// 反転を戻す
	ReverseRange(&tis, FALSE);
	if ( tis.incsearch.len ) ReverseText(&tis, tis.incsearch.len);

	if ( tis.cmode >= CMODE_LOG_VIEW ) screen.info.dwCursorPosition.Y = (SHORT)tis.sinfo.BackupY;
	SearchFileIned(&tis.ComplED, T(""), NULL, 0);
	CloseFileList(&tis);
	tis.cmode = CMODE_EDIT;
	FreeScreenCapText();

	{
		COORD pos;

		HilightLine(TC_execute); // 編集位置の強調解除
		if ( statuslen ) ClearStatusLine();
		pos.X = 0;
		pos.Y = screen.info.dwCursorPosition.Y;
		SetConsoleCursorPosition(hStdout, pos);
	}
	return tis.eflag;
}


void TconCommonCommand(TCINPUTSTRUCT *tis, int key)
{
	TCHAR buf[CMDLINESIZE], *bufp;

	if ( !(key & K_raw) ){
		const TCHAR *extmap;

		PutKeyCode(buf, key);
//		XMessage(NULL, NULL, XM_DbgLOG, T("key %s"),buf);
		extmap = extmapname[tis->cmode];
		if ( ( (extmap != NULL) && (NULL != (bufp = GetCustValue(extmap, buf, buf, sizeof(buf)))) ) ||
			 (NULL != (bufp = GetCustValue(T("KB_edit"), buf, buf, sizeof(buf)))) ){
			WORD *p;

			if ( (UTCHAR)buf[0] == EXTCMD_CMD ){
				PP_ExtractMacro(hHostWnd, &ppbappinfo, NULL, buf + 1, NULL, (XEO_CONSOLE | XEO_NOUSEPPB) );
				if ( bufp != buf ) HeapFree(hProcHeap, 0, bufp);
				FreeScreenCapText();
				tConsoleInit();
				return;
			}
			p = (WORD *)(((UTCHAR)buf[0] == EXTCMD_KEY) ? (buf + 1) : buf);
			key = *p;
			if ( key == 0 ) return;
			while( *(++p) ){
				TconCommonCommand(tis, key);
				key = *p;
			}
			if ( bufp != buf ) HeapFree(hProcHeap, 0, bufp);
			if ( !(key & K_raw) ){
				TconCommonCommand(tis, key);
				return;
			}
		}
	}
	resetflag(key, K_raw);	// エイリアスビットを無効にする

	if ( IsTrue(tis->linelist.show) && (tis->cmode < CMODE_LOG_VIEW) ){
		switch( key ){
			case K_c | 'F':
			case K_ri:
				if ( EditText[tis->EdX] == '\0' ){
					CloseFileList(tis);
					setflag(tis->eflag, TCI_TI_LIST_CHANGE_TARGET);
					return;
				}
				break;
/*
			case '\\':
			case '/':
				if ( (tis->EdX > 1) &&
					 (tis->EdX == (int)tis->linelist.cursor.end) &&
					 (EditText[tis->EdX - 1] == key) ){
					CloseFileList(tis);
					setflag(tis->eflag, TCI_TI_LIST_CHANGE_TARGET);
					return;
				}
				break;
*/
			case K_cr:					// CR
				if ( tis->linelist.item_index > 0 ){
					tis->sel = tis->linelist.cursor;
					Replace(tis, ThListGetStr(&tis->linelist.list, tis->linelist.item_index), 0);
				}
				CloseFileList(tis);
				resetflag(tis->eflag, TCI_TI_LIST_CHANGE_TARGET);
				return;

			case K_esc:					// ESC
				PPbEscMenu(tis);
				return;

			case K_c | 'P':
			case K_up:					// ↑
				{
					DWORD tick;
					tick = GetTickCount();

					if ( tis->linelist.item_index == 0 ){
						if ( (tick - tis->RepeatTick) > 300 ){ // ヒストリ参照へ
							break;
						}else{ // リピート中なので停止
							tis->RepeatTick = tick;
							return;
						}
					}
					tis->RepeatTick = tick;
				}

				if ( (tis->EdX < (int)tis->linelist.cursor.start) || (tis->EdX > (int)tis->linelist.cursor.end) ){
					setflag(tis->eflag, TCI_TI_LIST_CHANGE_TARGET);
					return;
				}
				if ( tis->linelist.mode == listmode_history ){
					tis->ShowOffset = 0;
				}
				if ( tis->linelist.item_index > 0 ){
					SelectLineList(tis, tis->linelist.item_index - 1);
				}
				return;

			case K_c | 'N':
			case K_dw:					// ↓
				if ( (tis->EdX < (int)tis->linelist.cursor.start) /*|| (EdX > (int)linelist.cursor.end)*/ ){
					setflag(tis->eflag, TCI_TI_LIST_CHANGE_TARGET);
					return;
				}

				if ( tis->linelist.mode == listmode_history ){
					tis->ShowOffset = 0;
				}
				if ( tis->linelist.item_index < (int)(ThListGetCount(&tis->linelist.list) - 1) ){
					SelectLineList(tis, tis->linelist.item_index + 1);
				}
				return;

			case K_Pup:					// Pup
				SelectLineList(tis,
					((tis->linelist.item_index - tis->list_height) > 1) ?
						tis->linelist.item_index - tis->list_height : 0);
				return;

			case K_Pdw:					// Pdw
				SelectLineList(tis, tis->linelist.item_index + tis->list_height);
				return;
		}
	}

	switch ( key ){
		case K_F1:
			PPbF1Menu(tis);
			break;

		case K_a | K_F4:			// &[F4] 終了
			tis->eflag = TCI_QUIT;
			break;

		case K_s | K_esc:			// \ESC:最小化(PPB用)
			if ( hBackWnd != NULL ){
				DWORD XV_minf = 2;

				GetCustData(T("XV_minf"), &XV_minf, sizeof(XV_minf));
				if ( XV_minf == 1 ){
					SendMessage(hHostWnd, WM_SYSCOMMAND, SC_MINIMIZE, MAX32);
					ForceSetForegroundWindow(hBackWnd);
					SetWindowPos(hHostWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
					hBackWnd = NULL;
					break;
				}else if ( XV_minf == 2 ){
					SetForegroundWindow(hBackWnd);
				}
				hBackWnd = NULL;
			}
			ShowWindow(hHostWnd, SW_MINIMIZE);
			break;

		case K_a | K_up:				// [↑]
			MoveWindowByKey(hHostWnd, 0, -1);
			break;
		case K_a | K_dw:			// [↓]
			MoveWindowByKey(hHostWnd, 0, 1);
			break;
		case K_a | K_lf:			// [←]
			MoveWindowByKey(hHostWnd, -1, 0);
			break;
		case K_a | K_ri:			// [→]
			MoveWindowByKey(hHostWnd, 1, 0);
			break;

		case K_c | K_Pup:				// ^[PgUP]
			ScrollArea(tis, -(screen.info.srWindow.Bottom - screen.info.srWindow.Top));
			break;

		case K_c | K_Pdw:				// ^[PgUP]
			ScrollArea(tis, screen.info.srWindow.Bottom - screen.info.srWindow.Top);
			break;

		case K_c | K_up:				// ^[↑]
			ScrollArea(tis, -1);
			break;
		case K_c | K_dw:			// ^[↓]
			ScrollArea(tis, 1);
			break;
//-----------------------------------------------
		case K_a | K_home: {	// &[home]
			WINPOS WPos;

			thprintf(buf, TSIZEOF(buf), T("%s_"), RegCID);
			if ( NO_ERROR == GetCustTable(T("_WinPos"), buf, &WPos, sizeof(WPos)) ){
				SetWindowPos(hHostWnd, NULL,
						WPos.pos.left, WPos.pos.top,
						0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
			}else{
				CenterWindow(hHostWnd);
			}
			break;
		}
//-----------------------------------------------
		case K_a | K_s | K_home:	// &\[home]
			thprintf(buf, TSIZEOF(buf), T("%s_"), RegCID);
			SaveConsolePos(buf);
			break;

		case K_c | 'G':
			PPbExecuteMenu(tis);
			break;

		case K_c | 'T':
			PPbOptionMenu(tis);
			break;

		case K_c | ']':
		case K_c | 'Q':
		case K_v | K_s | VK_F10:
		case K_v | VK_APPS: {
			int index = PPbContextMenu(tis);
			if ( index > 0 ) TconCommonCommand(tis, index);
			break;
		}

		case K_c | K_s | '_': // Ctrl+Shift US[-/_] JIS[-/=] ※ tgetin の加工による
			PPxCommonCommand(hHostWnd, 0, (WORD)(K_c | K_s | K_v | VK_SUBTRACT));
			break;

		case K_c | K_s | K_v | VK_ADD:
		case K_c | K_s | K_v | VK_OEM_PLUS: // US[=/+] JIS[;/+]
		case K_c | K_s | K_v | VK_SUBTRACT:
		case K_c | K_s | K_v | VK_OEM_MINUS: // US[-/_] JIS[-/=]
			PPxCommonCommand(hHostWnd, 0, (WORD)key);
			break;

		case K_c | K_v | VK_ADD:
		case K_c | K_v | VK_OEM_PLUS: // US[=/+] JIS[;/+]
			ConChangeFontSize(1);
			break;
		case K_c | K_v | VK_SUBTRACT:
		case K_c | K_v | VK_OEM_MINUS: // US[-/_] JIS[-/=]
			ConChangeFontSize(-1);
			break;

		case K_c | K_v | VK_NUMPAD0:
			ConChangeFontSize(-2);
			break;

		default:
			if ( tis->cmode < CMODE_LOG_VIEW ){
				EditModeCommand(tis, key);
			}else{
				LogModeCommand(tis, key);
			}
	}
}

void EditModeCommand(TCINPUTSTRUCT *tis, int key)
{
	TCHAR *istr; // 入力された文字列（0:未入力）
	TCHAR tmp[CMDLINESIZE];	// 一時編集領域

	istr = NULL;

	switch( key ){
		case K_c | K_v | '0':	// ^[0]
		case K_c | '0':			// ^[0]
		case K_a | '0':			// &[0]
			istr = PPxPath;
			break;

		case K_c | K_v | '1':	// ^[1]
		case K_c | '1':			// ^[1]
		case K_a | '1':			// &[1]
			istr = EditPath;
			break;

		case K_c | 'A': {				//	^A:全て選択
			DWORD len;

			len = (DWORD)tstrlen32(EditText);
			if ( (tis->sel.start != 0) || (tis->sel.end != len) ){
				tis->sel.start = 0;
				tis->sel.end = len;
				setflag(tis->eflag, TCI_TI_DRAW);
			}
			break;
		}

		case K_c | K_s | 'A':			//	^\A:選択解除
			if ( tis->sel.start != tis->sel.end ){
				tis->sel.start = tis->sel.end = 0;
				setflag(tis->eflag, TCI_TI_DRAW);
			}
			break;

		case K_c | 'C':				//	^C:クリップ
		case K_c | K_s | 'C':
		case K_c | 'X':				//	^X:カット
		case K_c | K_s | 'X':
			if ( *EditText && IsTrue(OpenClipboardT(NULL)) ){
				HGLOBAL hG;
				int i, j;
				TCHAR *p;

				if ( tis->sel.start != tis->sel.end ){
					i = tis->sel.end - tis->sel.start;
					j = tis->sel.start;
				}else{
					i = tstrlen32(EditText);
					j = 0;
				}
				hG = GlobalAlloc(GMEM_MOVEABLE, TSTROFF(i + 1));
				if ( hG != NULL ){
					p = GlobalLock(hG);
					#pragma warning(suppress:6387) // GlobalLock は失敗しないと見なす
					memcpy(p, (char *)(TCHAR *)(EditText + j), TSTROFF(i));
					*(p + i) = 0;
					GlobalUnlock(hG);
					EmptyClipboard();
					SetClipboardData(CF_TTEXT, hG);
				}
				CloseClipboard();

				if ( (key & 0xff) == 'X' ){
					if ( tis->sel.start != tis->sel.end ){
						tstrcpy(EditText + tis->sel.start, EditText + tis->sel.end);
						tis->EdX = tis->sel.start;
						tis->sel.start = tis->sel.end = 0;
					}else{
						*EditText = 0;
						tis->EdX = tis->sel.start = tis->sel.end = 0;
					}
					setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
				}
			}
			break;

		case K_c | K_s | 'V':
		case K_c | 'V':				//	^V:ペースト
			if ( IsTrue(OpenClipboardT(NULL)) ){
				HGLOBAL hG;

				hG = GetClipboardData(CF_TTEXT);
				if ( hG != NULL ){
					TCHAR *src, *srcmax, *dest;
					size_t len;

					src = GlobalLock(hG);
					#pragma warning(suppress:6387) // GlobalLock は失敗しないと見なす
					len = tstrlen(src);
					if ( len >= (TSIZEOF(tmp) - 1)) len = TSIZEOF(tmp) - 1;
					srcmax = src + len;
					dest = tmp;
					while( src < srcmax ){
						if ( (*src != '\r') && (*src != '\n') ){
							*dest++ = *src;
						}
						src++;
					}
					*dest = '\0';
					GlobalUnlock(hG);
					istr = tmp;
				}
				CloseClipboard();
			}
			break;

		case K_c | 'J':
		case K_c | 'M':
		case K_cr:					// CR:確定
			if ( EditText != tis->baseptr ) tstrcpy(tis->baseptr, EditText);
			tis->eflag = TCI_EXECUTE;
			break;

		case K_c | K_s | 'M':
		case K_s | K_cr: {			// Shift+CR:GUI一行編集
			if ( !ConsoleOnRemote ){
				TINPUT tinput;

				tinput.hOwnerWnd= hHostWnd;
				tinput.hRtype	= tis->histype;
				tinput.hWtype	= PPXH_COMMAND;
				tinput.title	= T("PPb");
				tinput.buff		= EditText;
				tinput.size		= CMDLINESIZE;
				tinput.flag		= TIEX_TOP;
				if ( tInputEx(&tinput) > 0 ){
					tis->EdX = 0;
					tis->sel.start = tis->sel.end = 0;
					setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
				}
			}else{
				ShowStatusText(T("no desktop"));
			}
			break;
		}
		case K_esc:					// ESC:中止
		case K_c | '[':
			PPbEscMenu(tis);
			break;

		case K_s | K_F4:
			if ( tis->linelist.show == FALSE ){
				tis->cmode = CMODE_WALKFILE;
			}else{
				ECURSOR cursor;
				tis->cmode++;
				tstrcpy(tmp, EditText);
				if ( tis->cmode >= CMODE_LOG_VIEW ){
					tis->cmode = CMODE_LIST;
				}
				if ( tis->sel.start == tis->sel.end ){
					cursor.start = tis->EdX;
					cursor.end	 = tis->EdX;
				}else{
					cursor = tis->sel;
				}
				CloseFileList(tis);
				MakeFileList(tis, tmp, &cursor);
				break;
			}
			// case K_F4 へ
		case K_c | 'R': // file / hist / edit
		case K_F4: {
			if ( tis->linelist.show == FALSE ){
				ECURSOR cursor;

				tis->linelist.mode = listmode_entry;
				tstrcpy(tmp, EditText);
				if ( tis->sel.start == tis->sel.end ){
					cursor.start = tis->EdX;
					cursor.end	 = tis->EdX;
				}else{
					cursor = tis->sel;
				}
				MakeFileList(tis, tmp, &cursor);
			}else{
				CloseFileList(tis);
				tis->cmode = CMODE_EDIT;
				tis->linelist.mode = listmode_off;
			}
			break;
		}

		case K_tab:					// \SPACE/TAB:ファイル名補完
		case K_c | 'I': {
			TCHAR *p;
			ECURSOR cursor;

			tstrcpy(tmp, EditText);
			if ( tis->sel.start == tis->sel.end ){
				cursor.start = tis->EdX;
				cursor.end	 = tis->EdX;
			}else{
				cursor.start = tis->sel.start;
				cursor.end	 = tis->sel.end;
			}

			p = SearchFileIned(&tis->ComplED, tmp, &cursor,
				((tis->histype & PPXH_COMMAND) ? CMDSEARCH_CURRENT : CMDSEARCH_OFF) |
				((key != (K_c | 'I')) ? 0 : CMDSEARCH_FLOAT) |
				CMDSEARCH_MULTI );

			if ( p != NULL ){	// 検索成功
				size_t len;

				len = tstrlen(p);
				tis->sel.start = cursor.start;
				tis->EdX = tis->sel.end = cursor.end;

				istr = p;
				tis->istrt = IRT_RIGHT;
				if ( len && (*(p + len - 1) == '\"') ) tis->istrt = IRT_BRAKET;
				tis->ShowOffset = 0;	// 表示開始桁を補正させる
			}
			break;
		}
		case K_c | 'F':
		case       K_ri:			//  →:右へ移動
		case K_s | K_ri: {			// \→:右へ移動+選択
			int newX;

			newX = tis->EdX;
			#ifdef UNICODE
			if ( ((size_t)newX < tis->maxsize) && *(EditText + newX) ) newX++;
			#else
			if ((size_t)newX < tis->maxsize) newX += Chrlen(*(EditText + newX));
			#endif
			MoveCursor(tis, newX, key);
			break;
		}
		case       K_c | K_ri:		//  ^→:右へ単語移動
		case K_s | K_c | K_ri: {	// ^\→:右へ単語移動+選択
			int newX = ForwardEditWord(tis, tis->EdX);

			while ( *(EditText + newX) == ' ' ){
				if ( (size_t)newX >= tis->maxsize ) break;
				newX++;
			}
			MoveCursor(tis, newX, key);
			break;
		}

		case       K_lf:			//  ←:左へ移動
		if ( (tis->cmode == CMODE_WALKDIR) || (tis->cmode == CMODE_WALKFILE) ){
			int newX;
			TCHAR chr;

			newX = tis->EdX;
			if ( newX <= 0 ) break;
			newX--;
			chr = EditText[newX];
			if ( (newX > 2) &&
				 ((chr == '\"') || (chr == '\'')) ){
				newX--;
				chr = EditText[newX];
			}
			if ( (chr == '/') || (chr == '\\') ){
				newX--;
			}
			for (;;){
				if ( newX <= 0 ) break;
				newX--;
				chr = EditText[newX];
				if ( (chr == '/') || (chr == '\\') ){
					ECURSOR cursor;
					newX++;
					EditText[newX] = '\0';
					tis->EdX = newX;
					tis->sel.start = tis->sel.end = 0;
					setflag(tis->eflag, TCI_TI_DRAW);
					newX = -1;

					tstrcpy(tmp, EditText);
					cursor.start = tis->EdX;
					cursor.end	 = tis->EdX;

					CloseFileList(tis);
					MakeFileList(tis, tmp, &cursor);
					break;
				}
			}
			if ( newX < 0 ) break;
		}

		case K_c | 'B':
		case K_s | K_lf: {			// \←:左へ移動+選択
			int newX;

			newX = tis->EdX;
			#ifdef UNICODE
			if ( newX != 0 ) newX--;
			#else
			if ( newX != 0 ) newX -= bchrlen(EditText, newX);
			#endif
			MoveCursor(tis, newX, key);
			break;
		}
		case       K_c | K_lf:		// ^←:左へ単語移動
		case K_s | K_c | K_lf:	// ^\←:左へ単語移動+選択
			MoveCursor(tis, BackEditWord(&tis->linelist, tis->EdX), key);
			break;

		case       K_home:			//  HOME:左端
		case K_s | K_home:			// \HOME:左端+選択
			MoveCursor(tis, 0, key);
			break;

		case K_c | 'E':			//  右端
		case       K_end:			//  END:右端
		case K_s | K_end:			// \END:右端+選択
			MoveCursor(tis, tstrlen32(EditText), key);
			break;

		case K_c | 'N':
			key = K_up;
		case K_c | 'P':
		case K_up:					// ↑:古いヒストリ
		case K_dw: {				// ↓:新しいヒストリ
			const TCHAR *p;

			if ( tis->linelist.mode != listmode_off ){
				CloseFileList(tis);
				tis->linelist.mode = listmode_history;
				key = K_up;
				ThListAddStr(&tis->linelist.list, EditText);
			}

			GetHistorySearchMode(tis, EditText, EditText + CMDLINESIZE);
			UsePPx();
			for (;;){
				p = SearchLineHistory(&tis->hvar, (key == K_up) ? 1 : -1);
				if ( p == NULL ){
					if ( tis->hvar.mode == HIST_CMD_FIND ){
						tis->hvar.mode = HIST_CMD_ALL;
						tis->hvar.index = -1;
						tis->hvar.count = 0;
						p = SearchLineHistory(&tis->hvar, (key == K_up) ? 1 : -1);
					}
					if ( tis->hvar.mode == HIST_PARAM_FIND ){
						tis->hvar.mode = HIST_PARAM_ALL;
						tis->hvar.index = -1;
						tis->hvar.count = 1;
						p = SearchLineHistory(&tis->hvar, (key == K_up) ? 1 : -1);
					}
				}
				if ( p == NULL ) break;

				tis->ShowOffset = 0;
				tis->EdX = tstrlen32(EditText);
				tis->sel.start = tis->sel.end = 0;
				setflag(tis->eflag, TCI_TI_DRAW);

				if ( tis->linelist.mode == listmode_off ){
					break;
				}else{
					if ( ThListAddStr(&tis->linelist.list, EditText) == FALSE ){
						break;
					}
					continue;
				}
			}
			FreePPx();
			if ( tis->linelist.mode == listmode_history ){
				ECURSOR cursor;

				cursor.start = 0;
				cursor.end = tis->EdX;
				MakeFileList(tis, EditText, &cursor);
			}
			break;
		}

		case K_c | K_bs:	// ^BS:左へ単語移動+削除 ※通常IME ONなので使われない
		case K_c | 'W':
			MoveCursor(tis, BackEditWord(&tis->linelist, tis->EdX), K_s | K_c | K_lf);
			// K_bs へ
		case K_c | 'H':
		case K_bs:					//	BS / ^H :左側の１文字を削除
			InitHistorySearch(&tis->hvar, EditText, EditText + CMDLINESIZE);
			if ( tis->sel.start != tis->sel.end ){
				BackupLine(EditText, tis->backuptext);

				TPushEditStack(0, EditText, tis->sel.start, tis->sel.end);
				tstrcpy(EditText + tis->sel.start, EditText + tis->sel.end);
				tis->EdX = tis->sel.start;
				tis->sel.start = tis->sel.end = 0;
				setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
			}else if ( tis->EdX != 0 ){
				int len;

				BackupLine(EditText, tis->backuptext);
				tis->EdX -= len = bchrlen(EditText, tis->EdX);
				TPushEditStack(0, EditText, tis->EdX, tis->EdX + len);
				tstrcpy(EditText + tis->EdX, EditText + tis->EdX + len);
				setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
			}
			break;

		case K_s | K_bs:			//	\BS:左側を全て削除
			if ( tis->EdX != 0 ){
				InitHistorySearch(&tis->hvar, EditText, EditText + CMDLINESIZE);
				BackupLine(EditText, tis->backuptext);
				TPushEditStack(0, EditText, 0, tis->EdX);
				tstrcpy(EditText, EditText + tis->EdX);
				tis->EdX = tis->sel.start = tis->sel.end = 0;
				setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
			}
			break;

		case K_a | K_bs:			//	&BS:元に戻す
		case K_c | '_':				//	^Z
		case K_c | 'Z':				//	^Z
			if ( tstrcmp(tis->backuptext, EditText) != 0 ){
				tstrcpy(tmp, EditText);
				tstrcpy(EditText, tis->backuptext);
				tstrcpy(tis->backuptext, tmp);

				tis->ShowOffset = 0;
				tis->sel.start = tis->sel.end = 0;
				tis->EdX = tstrlen32(EditText);
				setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
			}
			break;

		case K_c | K_del: {
			int newX = ForwardEditWord(tis, tis->EdX);

			while ( *(EditText + newX) == ' ' ){
				if ( (size_t)newX >= tis->maxsize ) break;
				newX++;
			}
			MoveCursor(tis, newX, K_c | K_s | K_ri);
		}
		// K_c | 'd' へ
		case K_c | 'D':
		case K_del:					//	DEL:現在の文字を削除
			if ( tis->sel.start != tis->sel.end ){
				BackupLine(EditText, tis->backuptext);
				TPushEditStack(B0, EditText, tis->sel.start, tis->sel.end);
				tstrcpy(EditText + tis->sel.start, EditText + tis->sel.end);
				tis->EdX = tis->sel.start;
				tis->sel.start = tis->sel.end = 0;
				setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
			}else if ( *(EditText + tis->EdX) != '\0' ){
				int len;

				BackupLine(EditText, tis->backuptext);
				#ifdef UNICODE
				len = *(EditText + tis->EdX) ? 1 : 0;
				#else
				len = Chrlen(*(EditText + tis->EdX));
				#endif
				TPushEditStack(B0, EditText, tis->EdX, tis->EdX + len);
				tstrcpy(EditText + tis->EdX, EditText + tis->EdX + len);
				setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
			}
			break;

		case K_s | K_del:			//	\DEL:現在以降を削除
		case K_c | 'K':
			if ( *(EditText + tis->EdX) != '\0' ){
				BackupLine(EditText, tis->backuptext);
				TPushEditStack(B0, EditText, tis->EdX, tstrlen32(EditText));
				*(EditText + tis->EdX) = '\0';
				tis->sel.start = tis->sel.end = 0;
				setflag(tis->eflag, TCI_TI_DRAW | TCI_TI_LIST_CHANGE_TARGET);
			}
			break;

		case K_ins:					//	INS:挿入状態の変更
			tis->insmode ^= (INS_INS ^ INS_OVER);
			break;

		case K_c | 'L':
			tClearScreen();
			setflag(tis->eflag, TCI_TI_DRAW);
			break;

		case K_c | 'Y':	// ^[Y]
		case K_c | 'U':{	// ^[U]
			TCHAR mode;

			PopTextStack(&mode, tmp);
			if ( tmp[0] ){
				istr = tmp;
				if ( mode ) tis->istrt = IRT_STOP;
			}
			break;
		}

		case K_s | K_up:			// \↑:スクロールモード
		case K_s | K_dw:			// \↓:スクロールモード
		case K_Pup:			// Pup:スクロールモード
		case K_Pdw:			// Pdw:スクロールモード
			SetScrollMode(tis);
			break;

		case K_c | K_s | 'G':
			ToggleSyntaxColor(tis);
			setflag(tis->eflag, TCI_TI_DRAW);
			break;

		default:					//	文字入力
			#ifdef UNICODE
			if ( key & (K_v | K_e | K_c | K_a) ) break;
			key = (key & 0xff) | ((key >> 8) & 0xff00);
			if ( ' ' <= key ){
			#else
			key &= 0xff | K_v | K_e | K_c | K_a;	/* shift 情報を破棄 */
			if ( (' ' <= key) && (key < 0x100) ){
			#endif
				BackupLine(EditText, tis->backuptext);
				InitHistorySearch(&tis->hvar, EditText, EditText + CMDLINESIZE);
				istr = tmp;
				tmp[0] = (TCHAR)key;
				tmp[1] = '\0';
				#ifndef UNICODE
				if ( IskanjiA(key) ){
					INPUT_RECORD cin;
					int key;

					key = tgetin(&cin, 200);
					if ( key == KEY_TIMEOUT ) break;
					if ( (key < 0) || (cin.EventType != KEY_EVENT) ){
						tmp[0] = '\0';
					}else{
						tmp[1] = (TCHAR)key;
						tmp[2] = '\0';
					}
				}
				#endif
			}
	}
	if ( istr ) Replace(tis, istr, 0);
}

void MoveCursorS(TCINPUTSTRUCT *tis, int offX, int offY, int key)
{
	int newX, newY;

	newX = tis->sinfo.cx + offX;
	if ( newX < 0 ){
		newX = 0;
	}else if ( newX >= screen.info.dwSize.X ){
		newX = screen.info.dwSize.X - 1;
	}
	newY = tis->sinfo.cy + offY;
	if ( newY < 0 ){
		newY = 0;
	}else if ( newY >= screen.info.dwSize.Y ){
		newY = screen.info.dwSize.Y - 1;
	}
	if ( (tis->sinfo.cx == newX) && (tis->sinfo.cy == newY) ){
		if ( !(key & K_s) ) ReverseRange(tis, FALSE);
		return;
	}

	if ( key & K_s ){	// 選択
		if ( RevBuf == NULL ){
			tis->sinfo.sx = tis->sinfo.cx;
			tis->sinfo.sy = tis->sinfo.cy;
		}
		ReverseRange(tis, FALSE);
		// 左右移動無しの上下移動 or 左右移動 →行選択
		// 左右移動後の上下移動 →ブロック選択
		tis->sinfo.LineMode = (tis->sinfo.sx == newX) ? TRUE : offX;
		tis->sinfo.cx = newX;
		tis->sinfo.cy = newY;
		ReverseRange(tis, TRUE);
	}else{
		ReverseRange(tis, FALSE);
		tis->sinfo.cx = newX;
		tis->sinfo.cy = newY;
	}
}
// mode = 0:→↑ 1:←↓
void JumpWordCursor(TCINPUTSTRUCT *tis, BOOL x, int mode, int key)
{
	COORD newXY;
	TCHAR c;
	DWORD temp;
	SHORT *cur, maxpos;

	newXY.X = (SHORT)tis->sinfo.cx;
	newXY.Y = (SHORT)tis->sinfo.cy;

	if ( IsTrue(x) ){
		cur = &newXY.X;
		maxpos = (SHORT)(screen.info.dwSize.X - 1);
	}else{
		cur = &newXY.Y;
		maxpos = (SHORT)(screen.info.dwSize.Y - 1);
	}

	for ( ; ; ){
		ReadConsoleOutputCharacter(hStdout, &c, 1, newXY, &temp);
		if ( (mode == 0) || (mode == 3) ){
			if ( (UTCHAR)c <= ' ' ) mode += 2;
		}else{
			if ( (UTCHAR)c != ' ' ) mode += 2;
		}
		if ( mode > 3 ) break;

		if ( mode & 1 ){
			if ( *cur == '\0' ) break;
			(*cur)--;
		}else{
			if ( *cur >= maxpos ) break;
			(*cur)++;
		}
	}
	MoveCursorS(tis, newXY.X - tis->sinfo.cx, newXY.Y - tis->sinfo.cy, key);
}

void LogModeCommand(TCINPUTSTRUCT *tis, int key)
{
	switch(key){
		case K_c | 'A':				//	^A:全て選択
			ReverseRange(tis, FALSE);
			tis->sinfo.cx = screen.info.dwSize.X - 1;
			tis->sinfo.cy = screen.info.dwSize.Y - 1;
			tis->sinfo.sx = 0;
			tis->sinfo.sy = 0;
			tis->sinfo.LineMode = FALSE;
			ReverseRange(tis, TRUE);
			break;
		case K_c | 'C':				// ^C:クリップ
		case K_c | K_s | 'C':
		case K_c | 'M':
		case K_cr:					// CR:確定
			ClipText(tis);
			// K_esc へ
		case K_c | 'R':
		case K_esc:					// ESC:入力モードへ
			ReverseRange(tis, FALSE);
			tis->cmode = CMODE_EDIT;
			screen.info.dwCursorPosition.Y = (SHORT)tis->sinfo.BackupY;
			setflag(tis->eflag, TCI_TI_DRAW);
			break;

		case K_c | 'F':
		case K_ri:					// →
		case K_s | K_ri:			// \→
			MoveCursorS(tis, 1, 0, key);
			break;

		case K_c | 'B':
		case K_lf:					// ←
		case K_s | K_lf:			// \←
			MoveCursorS(tis, -1, 0, key);
			break;

		case K_c | 'N':
		case K_up:					// ↑
		case K_s | K_up:			// \↑
			MoveCursorS(tis, 0, -1, key);
			break;

		case K_c | 'P':
		case K_dw:					// ↓
		case K_s | K_dw:			// \↓
			MoveCursorS(tis, 0, 1, key);
			break;

		case K_Pup:					// Pup
		case K_s | K_Pup:			// \Pup
			MoveCursorS(tis, 0, -(screen.info.srWindow.Bottom - screen.info.srWindow.Top), key);
			break;

		case K_Pdw:					// Pdw
		case K_s | K_Pdw:			// \Pdw
			MoveCursorS(tis, 0, (screen.info.srWindow.Bottom - screen.info.srWindow.Top), key);
			break;

		case K_home:				// home
		case K_s | K_home:			// \home
			MoveCursorS(tis, -tis->sinfo.cx, 0, key);
			break;

		case K_c | 'E':
		case K_end:					// end
		case K_s | K_end:			// \end
			MoveCursorS(tis, screen.info.dwSize.X, 0, key);
			break;

		case K_c | K_ri:			// ^→
		case K_s | K_c | K_ri:		// \^→
			JumpWordCursor(tis, TRUE, 0, key);
			break;

		case K_c | K_lf:			// ^←
		case K_s | K_c | K_lf:		// \^←
			JumpWordCursor(tis, TRUE, 1, key);
			break;

		case K_s | K_c | K_dw:		// \^↓
			JumpWordCursor(tis, FALSE, 0, key);
			break;

		case K_s | K_c | K_up:		// \^↑
			JumpWordCursor(tis, FALSE, 1, key);
			break;

		case K_c | 'I':
		case K_F3: // 検索
		case K_tab:
			if ( tis->incsearch.len == 0 ) break;
			if ( ScrIncSearch(tis, INCSEARCH_NEXT) == FALSE ){
				ScrIncSearch(tis, INCSEARCH_FIRST);
			}
			return;

		case K_s | K_F3: // 検索
		case K_s | K_tab:
			if ( tis->incsearch.len == 0 ) break;
			if ( ScrIncSearch(tis, INCSEARCH_BACK) == FALSE ){
				ScrIncSearch(tis, INCSEARCH_LAST);
			}
			return;

		case K_bs:
			if ( tis->incsearch.len > 0 ){
				ReverseText(tis, tis->incsearch.len);
				tis->incsearch.len--;
				ReverseText(tis, tis->incsearch.len);
			}
			return;

		default:
			#ifdef UNICODE
			if ( key & (K_v | K_e | K_c | K_a) ) return;
			key = (key & 0xff) | ((key >> 8) & 0xff00);
			if ( ' ' <= key ){
			#else
			key &= 0xff | K_v | K_e | K_c | K_a;	/* shift 情報を破棄 */
			if ( (' ' <= key) && (key < 0x100) ){
			#endif
				if ( (size_t)tis->incsearch.len < (TSIZEOF(tis->incsearch.text) - 1) ){
					tis->incsearch.text[tis->incsearch.len++] = (TCHAR)key;
					tis->incsearch.text[tis->incsearch.len] = '\0';
					if ( ScrIncSearch(tis, INCSEARCH_LETTER) == FALSE ){
						if ( ScrIncSearch(tis, INCSEARCH_ONSCREEN) == FALSE ){
							if ( ScrIncSearch(tis, INCSEARCH_FIRST) == FALSE ){
								tis->incsearch.len--;
								tis->incsearch.text[tis->incsearch.len] = '\0';
							}
						}
					}
				}
			}
			return;
	}
	if ( tis->incsearch.len ){
		ReverseText(tis, tis->incsearch.len);
		tis->incsearch.len = 0;
	}
}

int MouseCommand(TCINPUTSTRUCT *tis, MOUSE_EVENT_RECORD *me)
{
	switch ( me->dwEventFlags ){
		case MOUSE_MOVED:{ //範囲
			CONSOLE_SCREEN_BUFFER_INFO cinfo;
			DWORD NewScrollTick;

			// カーソル位置の補正(NT4.0だとはみでる)
			GetConsoleWindowInfo(hStdout, &cinfo);
			if ( me->dwMousePosition.X < 0 ){
				me->dwMousePosition.X = 0;
			}
			if ( me->dwMousePosition.X >= cinfo.dwSize.X ){
				me->dwMousePosition.X = (SHORT)(cinfo.dwSize.X - 1);
			}
			if ( me->dwMousePosition.Y < 0 ){
				me->dwMousePosition.Y = 0;
			}
			if ( me->dwMousePosition.Y >= cinfo.dwSize.Y ){
				me->dwMousePosition.Y = (SHORT)(cinfo.dwSize.Y - 1);
			}

			// 範囲選択
			if ( (tis->cmode == CMODE_LOG_SELECT) &&
				 (me->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) &&
				 ((tis->sinfo.cx != me->dwMousePosition.X) ||
				  (tis->sinfo.cy != me->dwMousePosition.Y)) ){
				ReverseRange(tis, FALSE);
				tis->sinfo.LineMode = (tis->sinfo.cx != me->dwMousePosition.X);
				if ( me->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED) ){
					tis->sinfo.LineMode = !tis->sinfo.LineMode;
				}
				tis->sinfo.cx = me->dwMousePosition.X;
				tis->sinfo.cy = me->dwMousePosition.Y;
				ReverseRange(tis, TRUE);
				tCsrPos(tis->sinfo.cx, tis->sinfo.cy);
				NewScrollTick = GetTickCount();
				// overflow は無視
				if ( (OldScrollTick + 10) < NewScrollTick ){
					BOOL modify = FALSE;

					OldScrollTick = NewScrollTick;
					if ( cinfo.srWindow.Left &&
							(cinfo.srWindow.Left >= me->dwMousePosition.X) ){
						modify = TRUE;
						cinfo.srWindow.Left--;
						cinfo.srWindow.Right--;
					}else if ( (cinfo.srWindow.Right < (cinfo.dwSize.X - 1) )
						&& (cinfo.srWindow.Right <= me->dwMousePosition.X) ){
						modify = TRUE;
						cinfo.srWindow.Left++;
						cinfo.srWindow.Right++;
					}
					if ( cinfo.srWindow.Top &&
							(cinfo.srWindow.Top >= me->dwMousePosition.Y) ){
						modify = TRUE;
						cinfo.srWindow.Top--;
						cinfo.srWindow.Bottom--;
					}else if ( (cinfo.srWindow.Bottom < (cinfo.dwSize.Y - 1) )
						&& (cinfo.srWindow.Bottom <= me->dwMousePosition.Y) ){
						modify = TRUE;
						cinfo.srWindow.Top++;
						cinfo.srWindow.Bottom++;
					}
					if ( IsTrue(modify) ){
						SetConsoleWindowInfo(hStdout, TRUE, &cinfo.srWindow);
					}
				}
			}
			if ( (tis->mouseSX >= 0) &&
				 (me->dwButtonState & (FROM_LEFT_1ST_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED)) ){
				int nx;

				nx = GetMouseX(tis->ShowOffset, me->dwMousePosition.X, EditText);
				if ( tis->mouseSX < nx ){
					tis->sel.start = tis->mouseSX;
					tis->EdX = tis->sel.end = nx;
				}else{
					tis->EdX = tis->sel.start = nx;
					tis->sel.end = tis->mouseSX;
				}
				NewScrollTick = GetTickCount();
				// overflow は無視
				if ( (OldScrollTick + 10) < NewScrollTick){
					OldScrollTick = NewScrollTick;
					if ( tis->ShowOffset && !me->dwMousePosition.X ) tis->ShowOffset--;
					if ( (me->dwMousePosition.X >= (screen.info.dwSize.X - 1)) &&
						((size_t)tis->EdX < tis->maxsize) && *(EditText + tis->EdX) ){
						tis->ShowOffset++;
					}
				}
				setflag(tis->eflag, TCI_TI_DRAW);
				return KEY_DUMMY;
			}
			break;
		}
		case DOUBLE_CLICK:
			if ( tis->cmode == CMODE_EDIT ){
				// 編集行なら、編集行の選択
				if ( me->dwMousePosition.Y == screen.info.dwCursorPosition.Y ){
					int nx;

					nx = GetMouseX(tis->ShowOffset, me->dwMousePosition.X, EditText);
					if ( nx >= 0 ){
						tis->sel.end = ForwardEditWord(tis, nx);
						tis->sel.start = BackEditWord(&tis->linelist, nx);
						setflag(tis->eflag, TCI_TI_DRAW);
						return KEY_DUMMY;
					}
				}else{ // ログ
/*
					if ( tis->sel.start != tis->sel.end ){ // 編集行選択中なら解除
						tis->sel.start = tis->sel.end = 0;
						setflag(eflag, TCI_TI_DRAW);
						return KEY_DUMMY;
					}else{
						SetScrollMode();
						SetSelectMode(me);
						return KEY_DUMMY;
					}
*/
				}
			}else{
			/*
				JumpWordCursor(TRUE, 0, K_s | K_ri);
						setflag(eflag, TCI_TI_DRAW);
						return KEY_DUMMY;
			*/
			}

			break;
		#ifndef MOUSE_HWHEELED
		#define MOUSE_HWHEELED 8
		#endif
		case MOUSE_HWHEELED:
		case MOUSE_WHEELED: {
			int now;

			now = -HISHORTINT(me->dwButtonState);
			if ( (WheelOffset ^ now) & B31 ) WheelOffset = 0; // Overflow
			WheelOffset += now;
			now = WheelOffset / (WHEEL_DELTA / WHEEL_STANDARD_LINES);
			WheelOffset -= now * (WHEEL_DELTA / WHEEL_STANDARD_LINES);

			if ( me->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) ){
				if ( me->dwControlKeyState & SHIFT_PRESSED ){
#if 0 // 現在、正常に機能しない
					PPxCommonCommand(hHostWnd, 0,
						(now < 0) ? (WORD)(K_c | K_s | K_v | VK_ADD) : (WORD)(K_c | K_s | K_v | VK_SUBTRACT));
#endif
				}else{
					ConChangeFontSize((SHORT)-now);
				}
			}else{
				ScrollArea(tis, now);
			}
			break;
		}

		default:{		// マウスボタンの状態が変化した
			DWORD b, c;
			int button = 0;
													// 左右同時押し
			if ( me->dwButtonState == (FROM_LEFT_1ST_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED) ){
				return K_s | K_esc;
			}
													// ボタン変化を抽出
			b = OldMouseButton ^ me->dwButtonState;
			c = me->dwButtonState;
			OldMouseButton = me->dwButtonState;

			while ( button < 30 ){
				if ( !(b & LSBIT) ){	// このボタンは変化していない
					b = b >> 1;
					c = c >> 1;
					button++;
					continue;
				}
				c = c & LSBIT;
				break;
			}
			if ( c ){ // down 状態
				if ( button == 0 ){	// 左down
					if ( tis->cmode == CMODE_EDIT ){
						// 編集行なら、編集行の選択
						if ( me->dwMousePosition.Y ==
								screen.info.dwCursorPosition.Y ){
							int nx;

							nx = GetMouseX(tis->ShowOffset,
								 me->dwMousePosition.X, EditText);
							if ( (nx >= 0) &&
								 ((button == 0) ||
								 	((nx < (int)tis->sel.start) || (nx > (int)tis->sel.end )) ) ){
								tis->mouseSX = tis->sel.start = tis->sel.end = tis->EdX = nx;
								setflag(tis->eflag, TCI_TI_DRAW);
								return KEY_DUMMY;
							}
						}else{ // ログ
							if ( tis->sel.start != tis->sel.end ){ // 編集行選択中なら解除
								tis->sel.start = tis->sel.end = 0;
								setflag(tis->eflag, TCI_TI_DRAW);
								return KEY_DUMMY;
							}else{
								SetScrollMode(tis);
								SetSelectMode(tis, me);
								return KEY_DUMMY;
							}
						}
					}else{
						ReverseRange(tis, FALSE);
						SetSelectMode(tis, me);
					}
					break;
				}
			}
			if ( c ) break;
			tis->mouseSX = -1;
			// up 状態
/*
			if ( button == 0 ){	// 左up
				if ( (tis->cmode >= CMODE_LOG_SELECT) && (sx == cx) && (sy == cy) ){
					ReverseRange(tis, FALSE);
					tis->cmode = CMODE_EDIT;
					screen.info.dwCursorPosition.Y = (SHORT)BackupY;
					setflag(eflag, TCI_TI_DRAW);
					return KEY_DUMMY;
				}
			}
*/
			if ( button == 2 ){		// 中up
				return K_s | K_esc;
			}
			if ( button == 1 ){		// 右up
				return PPbContextMenu(tis);
			}
		}
	}
	return 0;
}

int PPbContextMenu(TCINPUTSTRUCT *tis)
{
	int index;

	if ( tis->cmode >= CMODE_LOG_VIEW ){
		index = Select(selectmenu);
		if ( index <= 0 ){ // キャンセルなら元に戻す
			ReverseRange(tis, FALSE);
			tis->cmode = CMODE_EDIT;
			screen.info.dwCursorPosition.Y = (SHORT)tis->sinfo.BackupY;
			setflag(tis->eflag, TCI_TI_DRAW);
			return KEY_DUMMY;
		}
		return index;
	}else{
		index = Select(editmenu);

		if ( index <= 0 ) return 0;
		if ( index == 1 ){ // 範囲選択モードへ
			SetScrollMode(tis);
			return K_s;
		}
		return index;
	}
}

void SaveConsolePos(const TCHAR *id)
{
	WINPOS WinPos;

	GetWindowRect(hHostWnd, &WinPos.pos);
	WinPos.show = WinPos.reserved = 0;
	SetCustTable(T("_WinPos"), id, &WinPos, sizeof(WinPos));
}
