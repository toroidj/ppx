/*-----------------------------------------------------------------------------
	Paper Plane xUI	customizer									色シート
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPCUST.H"
#pragma hdrstop

// COMMON_LVB_ は、WindowNT系の多バイト言語 or Windows10以降でサポート
#ifndef COMMON_LVB_UNDERSCORE
	#define COMMON_LVB_UNDERSCORE 0x8000
#endif

#define GC_mask		0xff
#define GC_single	0
#define GC_list		1
#define GC_table	2 // CustTable (拡張子色)
#define GC_entry	3
#define GC_hmenu	4
#define GC_line		5
#define GC_theme	6  // テーマ用
#define GC_nocolor	B9 // 色選択無し(GC_theme用)
#define GC_aoff		B10 // C_AUTO は非表示扱い
#define GC_ppbItem	B11 // GC_list
#define GC_sortItem	B12 // GC_table
#define GC_editExt	B13 // GC_table (C_ext)
#define GC_haveItem	B14 // sublist 有り
#define GC_auto		B15 // 一部のGC_single

#define USER_COLOR_HEADER T("\\color-")

typedef struct {
	const TCHAR *name;
	const TCHAR *item;
	DWORD flags; // GC_
	const TCHAR **sublist;
	int subs;
} LISTS;

#define THEMEINDEX_RESTORE 0
#define THEMEINDEX_MAXRESOURCE 5
#define THEMEINDEX_USER (THEMEINDEX_MAXRESOURCE + 1)
const TCHAR *GCtheme[] = {
	MES_VCTB, MES_VCTC, MES_VCTD, MES_VCTE, MES_VCTF, MES_VCTH, NULL
};

const TCHAR *GCline[] = { MES_VCLM, MES_VCLL, NULL};
const TCHAR *GCCdisp[] = { MES_VCDA, MES_VCDB, NULL};
const TCHAR *GCwin[] = {
	MES_VCWA, MES_VCWB, MES_VCWC, MES_VCWD,
	MES_VCWE, MES_VCWF, MES_VCWG, MES_VCWH,
	MES_VCWI, MES_VCWJ, MES_VCWK, MES_VCWL,
	NULL};
const TCHAR *GCscheme[] = {
	MES_VCSB, MES_VCPE, MES_VCPC, MES_VCPG,
	MES_VCPB, MES_VCPF, MES_VCPD, MES_VCSC,
	MES_VCPI, MES_VCPM, MES_VCPK, MES_VCPO,
	MES_VCPJ, MES_VCPN, MES_VCPL, MES_VCPP,
	MES_VCSD, MES_VCSE, MES_VCSF, MES_VCSH,
	MES_VCSG, MES_VCSJ, MES_VCSI, MES_VCSK,
	MES_VCSL, MES_VCSM, MES_VCSN, MES_VCSO,
	MES_VCSP, MES_VCSQ, MES_VCSR, MES_VCVA,
	NULL};

const TCHAR *GCcsyh[] = {
	MES_VCXC, MES_VCXB, MES_VCXE, MES_VCXB,
	MES_VEAB, MES_VCXB, MES_VCXI, MES_VCXB,
	MES_VCXK, MES_VCXB, MES_VCXM, MES_VCXB,
	MES_VCXO, MES_VCXB, MES_VCXQ, MES_VCXB,
	MES_VCXS, MES_VCXB, MES_VCXU, MES_VCXB,
	MES_VCXW, MES_VCXB, MES_VCYA, MES_VCXB,
	MES_VCYC, MES_VCXB, MES_VCYE, MES_VCXB,
	MES_VCYG, MES_VCXB, MES_VCXY, MES_VCXB,
	NULL};

const TCHAR *GCcapt[] = {
	MES_VCFF, MES_VCFG, MES_VCFH, MES_VCFI,
	MES_VCFJ, MES_VCFK, MES_VCFL, MES_VCFM,
	MES_VCFN, MES_VCFO, MES_VCFP, MES_VCFQ,
	MES_VCFR, MES_VCFS, NULL};
const TCHAR *GCentry[] = {
	MES_VCEM, MES_VCEC, MES_VCEP, MES_VCEA,
	MES_VCED, MES_VCES, MES_VCEH, MES_VCER,
	MES_VCEN, MES_VCEO, MES_VCEL, MES_VCEI,
	MES_VCEE, MES_VCEF, NULL};
const TCHAR *GCeInfo[] = {
	MES_VCJA, MES_VCJB, MES_VCJC, MES_VCJD,
	MES_VCJE, MES_VCJF, MES_VCJG, MES_VCJH,
	MES_VCJI, MES_VCJJ, MES_VCJK,
	MES_VCJL, MES_VCJM,
	MES_VCJN, MES_VCJO, MES_VCJP, MES_VCJQ,
	MES_VCJR, MES_VCJS, MES_VCJT, MES_VCJV, MES_VCJW, NULL};
const TCHAR *GCltag[] = { MES_VCTT, MES_VCTS, NULL};
const TCHAR *GCchar[] = {
	MES_VCPI, MES_VCPM, MES_VCPK, MES_VCPJ,
	MES_VCPO, MES_VCPL, MES_VCPN, MES_VCPP,
	MES_VCPQ, MES_VCPE, MES_VCPC, MES_VCPB,
	MES_VCPG, MES_VCPD, MES_VCPF, MES_VCPR,
	NULL};
const TCHAR *GCb_pals[] = {
	MES_VCPQ, MES_VCPB, MES_VCPC, MES_VCPD,
	MES_VCPE, MES_VCPF, MES_VCPG, MES_VCPR,
	MES_VCPI, MES_VCPJ, MES_VCPK, MES_VCPL,
	MES_VCPM, MES_VCPN, MES_VCPO, MES_VCPP,
	NULL};
const TCHAR *GChili[] = { MES_VCHF,
	MES_VCJN, MES_VCJO, MES_VCJP, MES_VCJQ,
	MES_VCJR, MES_VCJS, MES_VCJT, MES_VCJU,
	NULL};

const TCHAR *GClbak[] = { MES_VCVC, MES_VCVA, MES_VCVD, NULL};
const TCHAR *GClnum[] = { MES_VCLG, MES_VCLH, NULL};

const TCHAR *GCBedit[] = {
	MES_VCBS, MES_VCBT, MES_VCBU, MES_VCBV,
	MES_VCBW, MES_VCBX, MES_VCBY, MES_VCBZ,
	NULL};

const TCHAR *GCBpop[] = {
	MES_VCBF, MES_VCBG, MES_VCBC, MES_VCBD,
	MES_VCBA, MES_VCBB, MES_VCBU, MES_VCBV,
	MES_VCBH, MES_VCBI,
	NULL};

const TCHAR StrC_schN[] = T("C_schN");
const TCHAR StrC_schD[] = T("C_schD");

const LISTS GClist[] = {
	{MES_VCCT, NilStr,		GC_theme | GC_nocolor | GC_haveItem, GCtheme, 5},
	{MES_VCBK, T("C_back"),	GC_single | GC_auto, NULL, 0},
	{MES_VCME, T("C_mes"),	GC_single | GC_auto, NULL, 0},
	{MES_VCIN, T("C_info"),	GC_single | GC_auto, NULL, 0},
	{MES_VCRE, T("C_res"),	GC_list | GC_haveItem, GCCdisp, 2},
	{MES_VCLI, T("C_line"),	GC_list | GC_haveItem, GCline, 2},
	{MES_VCTR, T("CC_tree"),GC_list | GC_haveItem, GCCdisp, 2},
	{MES_VCTI, T("C_tip"),	GC_list | GC_haveItem, GCCdisp, 2},
	{MES_VCFD, T("C_win"),	GC_list | GC_haveItem, GCwin, 12},
	{MES_VCSA, StrC_schN,	GC_list | GC_haveItem, GCscheme, C_Scheme2_TOTAL},
	{MES_VCSZ, StrC_schD,	GC_list | GC_haveItem, GCscheme, C_Scheme2_TOTAL},
	{MES_VCXA, T("C_csyh"),	GC_list | GC_haveItem, GCcsyh, 32},
	{MES_VCFE, T("C_ext"),	GC_table | GC_haveItem | GC_editExt | GC_sortItem, NULL, 0},
	{MES_VCET, T("C_entry"), GC_list | GC_haveItem, GCentry, 14},
	{MES_VCEB, T("C_eInfo"), GC_list | GC_haveItem, GCeInfo, 22},
	{MES_VCIA, T("XC_inf1"), GC_list | GC_haveItem, GCCdisp, 2},
	{MES_VCIB, T("XC_inf2"), GC_list | GC_haveItem, GCCdisp, 2},
	{MES_VCCA, T("C_capt"),	GC_list | GC_haveItem, GCcapt, 14},
	{MES_VCLA, T("CC_log"), GC_list | GC_haveItem, GCCdisp, 2},
	{MES_VCVT, T("CV_char"), GC_list | GC_haveItem, GCchar, 16},
	{MES_VCVE, T("CV_lbak"), GC_list | GC_haveItem | GC_aoff, GClbak, 3},
	{MES_VCVB, T("CV_boun"), GC_single,	NULL, 0},
	{MES_VCCL, T("CV_lcsr"), GC_single,	NULL, 0},
	{MES_VCLN, T("CV_lnum"), GC_list | GC_haveItem, GClnum, 2},
	{MES_VCCC, T("CV_ctrl"), GC_single,	NULL, 0},
	{MES_VCLF, T("CV_lf"),	GC_single,	NULL, 0},
	{MES_VCTA, T("CV_tab"),	GC_single,	NULL, 0},
	{MES_VCWS, T("CV_spc"),	GC_single,	NULL, 0},
	{MES_VCLK, T("CV_link"), GC_single,	NULL, 0},
	{MES_VCTG, T("CV_syn"),	GC_list | GC_haveItem, GCltag, 3},
	{MES_VCHL, T("CV_hili"), GC_list | GC_haveItem, GChili, 9},
	{MES_VCBE, T("CB_edit"), GC_list | GC_haveItem | GC_ppbItem, GCBedit, 4},
	{MES_VCBR, T("CB_com"),	GC_list | GC_haveItem | GC_ppbItem, GCCdisp, 2},
	{MES_VCBO, T("CB_pop"),	GC_list | GC_haveItem | GC_ppbItem, GCBpop, 6},
	{MES_VCBE, T("CB_editE"), GC_list | GC_haveItem, GCBedit, 4},
	{MES_VCBR, T("CB_comE"),	GC_list | GC_haveItem, GCCdisp, 2},
	{MES_VCBO, T("CB_popE"),	GC_list | GC_haveItem, GCBpop, 6},
	{MES_VCBP, T("CB_pals"), GC_list | GC_haveItem, GCb_pals, 16},
	{NULL, NULL, 0, NULL, 0}
};

#define COLOR_SPECIAL 0xfe000000
#define COLOR_SUB 0xfffffffe // 下層あり
#define COLOR_NO 0xfefefefe // 色指定無し(theme用)

#define COLORLIST_WIDTH 9
#define COLORLIST_HEIGHT 12
#define COLORLIST_SAMPLES (5 * COLORLIST_HEIGHT) // 既定サンプル色
#define COLORLIST_SETTINGS 12 // 設定例(CL_back ～ CL_auto)
#define COLORLIST_Schemes1 3 // スキーム1(C_S_BACK, C_S_MES, C_S_INFO)
#define COLORLIST_Schemes2 (C_Scheme2_TOTAL) // スキーム2(C_Scheme2)
#define COLORLIST_Schemes (COLORLIST_Schemes1 + COLORLIST_Schemes2)
#define COLORLIST_COLORS (COLORLIST_SAMPLES + COLORLIST_SETTINGS + COLORLIST_Schemes)

const TCHAR Schemes2_1[] = T("fg");
const TCHAR Schemes2_2[] = T("BR");
const TCHAR Schemes2_3[] = T("bg");
const TCHAR *IndexColorName[] = {
	MES_VCOT, // CL_user
	MES_VCAU, // CL_auto MES_VCAU / MES_VCDI
	MES_VCLB, // CL_Schemes1  C_S_BACK
	MES_VCLC, // C_S_MES
	MES_VCLD, // C_S_INFO
	Schemes2_1, // CL_Schemes2
	Schemes2_1,
	Schemes2_1,
	Schemes2_1,
	Schemes2_1,
	Schemes2_1,
	Schemes2_1,
	Schemes2_1,

	Schemes2_2,
	Schemes2_2,
	Schemes2_2,
	Schemes2_2,
	Schemes2_2,
	Schemes2_2,
	Schemes2_2,
	Schemes2_2,

	Schemes2_3,
	Schemes2_3,
	Schemes2_3,
	Schemes2_3,
	Schemes2_3,
	Schemes2_3,
	Schemes2_3,
	Schemes2_3,

	T("BG"),
	T("TXT"),
	T("sBG"),
	T("sFG"),
	T("LBL"),
	T("CSR"),
	T("FRM"),
	T("-"),
};
const TCHAR stringaoff[] = MES_VCDI;
const TCHAR AutoString[] = T("a)");
const TCHAR SubMenuString[] = T("> ");
const WCHAR stringExtInfo[] = BANNERTEXT(MES_VBEI, L"拡張子/ワイルドカード\0extention/wildcard");

enum {
//PPcの定義色
	CL_back = COLORLIST_SAMPLES, CL_mes, CL_info,
//Windowsの定義色
	CL_windowtext, CL_window, CL_hilighttext, CL_hilight,
	CL_btnface, CL_inactive,
//カスタマイザ関連
	CL_prev, // 直前で使用した色
	CL_user, // ユーザ作成色
	CL_auto, // 動的自動取得色
	CL_Schemes1 = (COLORLIST_SAMPLES + COLORLIST_SETTINGS),
	CL_Schemes2 = (COLORLIST_SAMPLES + COLORLIST_SETTINGS + COLORLIST_Schemes1)
};

#define CL_first CL_back
#define CL_withtext CL_user
#define CL_last CL_auto

COLORREF G_Colors[COLORLIST_COLORS] = {
// sample
	C_BLACK, C_RED, C_GREEN, C_BLUE,
	C_YELLOW, C_CYAN, C_MAGENTA, C_SBLUE,
	C_MGREEN, 0x80ff80, C_CREAM, 0x202020,

	C_DBLACK, 0x0000c0, 0x40c000, 0xc00000,
	0x00c0c0, 0xff8000, 0x8000ff, 0xffff80,
	0x00c0ff, 0x80ff00, 0x80ffff, 0x404040,

	C_GRAY, 0x404080, 0x408000, 0xa00000,
	0x408080, 0xc08000, 0xff0080, 0x808040,
	0x0080ff, 0x80ff00, 0x40c0c0, 0x606060,

	C_DWHITE, C_DRED,   C_DGREEN, C_DBLUE,
	C_DYELLOW, C_DCYAN, C_DMAGENTA, 0x804000,
	0xc08080, 0x00ff80, 0x8080ff, 0xa0a0a0,

	C_WHITE,  0x000040, 0x004000, 0x400000,
	0x004080, 0x404000, 0xc00040, 0x4000c0,
	0xff8080, 0x808040, 0xff80ff, 0xe0e0e0,
// settings  CL_back ～ CL_AUTO
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C_AUTO,
// CL_Schemes1
	0, 0, 0,
// CL_Schemes2
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0
};


#define CONSOLECOLOR 16

COLORREF ConsoleColors[CONSOLECOLOR] = {
	C_BLACK, C_DBLUE, C_DGREEN, C_DCYAN, C_DRED, C_DMAGENTA, C_DYELLOW, C_DWHITE,
	C_DBLACK, C_BLUE, C_GREEN, C_CYAN, C_RED, C_MAGENTA, C_YELLOW, C_WHITE,
};

#define USERUSEPPXCOLOR 3
// 色詳細ダイアログのユーザ定義色
#define userColor_SEL 15 // 現在選択している色
COLORREF userColor[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define GC_THEME 0
#define GC_INFO 3
int edit_gct = GC_THEME;	// IDV_GCTYPE の選択中インデックス
const LISTS *GcType = &GClist[GC_THEME];

int edit_sitem = 0;			// IDV_GCITEM の選択中インデックス

#define WINDOWSCOLOR 6
int GDIColors[WINDOWSCOLOR] = {
	COLOR_WINDOWTEXT, COLOR_WINDOW,
	COLOR_HIGHLIGHTTEXT, COLOR_HIGHLIGHT,
	COLOR_BTNFACE, COLOR_INACTIVECAPTION
};

const TCHAR OldColorWildcard[] = T("&C*,&!CV_hkey");
TCHAR *OldColorTheme = NULL;
int OldX_uxt[X_uxt_items] = {X_uxt_defvalue};

typedef struct {
	TCHAR text[VFPS], *textptr;
	BOOL cb_top, cb_bottom;
} HighlightItem;

int DrawHeight = -1;

int InitDrawHeight(HWND hDlg)
{
	HDC hDC;
	HGDIOBJ hOldFont;
	TEXTMETRIC tm;

	hDC = GetDC(hDlg);
	hOldFont = SelectObject(hDC, (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0));
	GetTextMetrics(hDC, &tm);
	SelectObject(hDC, hOldFont);
	ReleaseDC(hDlg, hDC);
	return tm.tmHeight + 1;
}

BOOL ListView__Select(HWND hListView, int index)
{
	LV_ITEM lvi;

	lvi.mask = LVIF_STATE;
	lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
	lvi.stateMask = 0xf;
	lvi.iItem = index;
	lvi.iSubItem = 0;

	SendMessage(hListView, LVM_SETITEMSTATE, (WPARAM)index, (LPARAM)&lvi);
	SendMessage(hListView, LVM_ENSUREVISIBLE, (WPARAM)index, (LPARAM)FALSE);
	return TRUE;
}

void ListView__FixColorColumn(HWND hListView)
{
	int width;
	RECT box;

	if ( DrawHeight < 0 ) DrawHeight = InitDrawHeight(hListView);
	SendMessage(hListView, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE);
	width = ListView_GetColumnWidth(hListView, 0) + DrawHeight;
	GetClientRect(hListView, &box);
	if ( width < box.right ) width = box.right;
	SendMessage(hListView, LVM_SETCOLUMNWIDTH, 0, width);
}

BOOL ListView__GetText(HWND hListView, int index, TCHAR *text, int textlen)
{
	LV_ITEM lvi;

	lvi.iItem = (index >= 0) ? index : TListView_GetFocusedItem(hListView);
	if ( lvi.iItem < 0 ){
		text[0] = '\0';
		return FALSE;
	}
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = 0;
	lvi.pszText = text;
	lvi.cchTextMax = textlen;
	ListView_GetItem(hListView, &lvi);
	return TRUE;
}
/*
BOOL ListView__DeleteSelectedItem(HWND hListView)
{
	int index = TListView_GetFocusedItem(hListView);

	if ( index < 0 ) return FALSE;
	return ListView_DeleteItem(hListView, index);
}
*/
LPARAM ListView__GetItemData(HWND hListView, int index)
{
	LV_ITEM lvi;

	lvi.iItem = (index >= 0) ? index : TListView_GetFocusedItem(hListView);
	if ( lvi.iItem < 0 ) return 0;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;
	ListView_GetItem(hListView, &lvi);
	return lvi.lParam;
}

BOOL ListView__SetItemData(HWND hListView, int index, LPARAM lParam)
{
	LV_ITEM lvi;

	lvi.iItem = (index >= 0) ? index : TListView_GetFocusedItem(hListView);
	if ( lvi.iItem < 0 ) return FALSE;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.lParam = lParam;
	ListView_SetItem(hListView, &lvi);
	return TRUE;
}

TCHAR *GetUserTheme(HWND hDlg)
{
	PPXDBINFOSTRUCT dbinfo;
	TCHAR path[VFPS], buf[VFPS];
	TCHAR *mem, *text, *maxptr, *last;
	ERRORCODE err;

	dbinfo.structsize = sizeof dbinfo;
	dbinfo.custpath = path;
	GetPPxDBinfo(&dbinfo);

	if ( ListView__GetText(GetDlgItem(hDlg, IDV_GCITEM), edit_sitem, buf, TSIZEOF(buf)) == FALSE ) return NULL;
	last = VFSFindLastEntry(path);
	thprintf(last, VFPS - (last - path), USER_COLOR_HEADER T("%s"), buf);
	err = LoadTextImage(path, &mem, &text, &maxptr);
	if ( err != NO_ERROR ){
		ErrorPathBox(hDlg, NULL, path, err);
		return NULL;
	}
	text = mem; // 色とフォント以外の設定があれば拒否する。（セキュリティ目的）
	for (;;){
		TCHAR c, d;

		c = *text++;
		if ( c == '\0' ) break;
		if ( (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') ) continue;

		d = *text;
		if ( d == '\0' ) break;
		if ( ((d == '_') &&
				((c == 'C') || (c == 'F') || // C_, F_
				 ((c == 'X') && (text[1] == 'u') && (text[2] == 'x')) || // X_uxt
				 ((c == 'A') && (text[1] == 'c') && (text[2] == 'o')))) || // A_color
			  ((c == 'C') && Isalpha(d) && (text[1] == '_')) ){ // Cx_
			// 許可する形式
		}else if ( ((c == '_') && Isalpha(d)) || // _xxx
			 ((d == '_') && Isalpha(c)) || // x_xxx
			 (Isalpha(c) && Isalpha(d) && (text[1] == '_')) || // xx_xxx
			 ((c == 'M') && (d == 'e') && (text[1] == 's')) ){ // Mesxxxx
			XMessage(NULL, NULL, XM_GrERRld, T("%s"), MessageText(MES_ECCF));
			HeapFree(GetProcessHeap(), 0, mem);
			return NULL;
		}
		for(;;){
			if ( d == '\0' ) return mem;
			text++;
			if ( (d == '\r') || (d == '\n') ) break;
			d = *text;
		}
	}
	return mem;
}

#define CUSTOFFSETDATA(cust) (const BYTE *)(cust + sizeof(WORD))
int WINAPI SortColorFunc(const BYTE *cust1, const BYTE *cust2)
{
	return *((DWORD *)(CUSTOFFSETDATA(cust1) + TSTRSIZE((const TCHAR *)CUSTOFFSETDATA(cust1)))) -
		   *((DWORD *)(CUSTOFFSETDATA(cust2) + TSTRSIZE((const TCHAR *)CUSTOFFSETDATA(cust2))));
}

// 使いそうな色を一覧に用意する
void FixColorList(void)
{
	int i;

	// PPx の色から３色用意
	for ( i = 0 ; i < USERUSEPPXCOLOR ; i++ ){
		GetCustData(GClist[i + 1].item, &userColor[i], sizeof(COLORREF));
		G_Colors[CL_first + i] = userColor[i];
	}
	// Windows の色から6色用意
	for ( i = 0 ; i < WINDOWSCOLOR ; i++ ){
		G_Colors[i + CL_windowtext] =
			userColor[i + USERUSEPPXCOLOR] = GetSysColor(GDIColors[i]);
	}

	for ( i = 0 ; i < COLORLIST_Schemes1 ; i++ ){
		G_Colors[i + CL_Schemes1] = GetSchemeColor(i + C_Scheme1_MIN, C_AUTO);
	}
	for ( i = 0 ; i < COLORLIST_Schemes2 ; i++ ){
		G_Colors[i + CL_Schemes2] = GetSchemeColor(i + C_Scheme2_MIN, C_AUTO);
	}
}

COLORREF GetGindexCOLOR(HWND hListWnd)
{
	DWORD index;

	index = (DWORD)SendMessage(hListWnd, LB_GETCURSEL, 0, 0);

	if ( index >= COLORLIST_COLORS ) return COLOR_NO;
	if ( index >= CL_Schemes2 ) return index - CL_Schemes2 + C_Scheme2_MIN;
	if ( index >= CL_Schemes1 ) return index - CL_Schemes1 + C_Scheme1_MIN;
	if ( index == CL_auto ) return C_AUTO;
	return G_Colors[index];
}

void SetColorList(HWND hDlg, COLORREF c)
{
	int i;
	COLORREF selcolor;
	HWND hListWnd;

	hListWnd = GetDlgItem(hDlg, IDL_GCOLOR);
	selcolor = GetGindexCOLOR(hListWnd);
	if ( selcolor != COLOR_NO ){
		G_Colors[CL_prev] = selcolor;
		InvalidateRect(hListWnd, NULL, FALSE);
	}

	if ( c >= C_S_AUTO ){
		i = CL_auto;
		G_Colors[CL_user] = 0xc0c0c0;
	}else if ( (c >= C_Scheme1_MIN) && (c <= C_Scheme1_MAX) ){
		i = (DWORD)(c - C_Scheme1_MIN + CL_Schemes1);
		G_Colors[CL_user] = G_Colors[i];
	}else if ( (c >= C_Scheme2_MIN) && (c <= C_Scheme2_MAX) ){
		i = (DWORD)(c - C_Scheme2_MIN + CL_Schemes2);
		G_Colors[CL_user] = G_Colors[i];
	}else{
		for ( i = 0 ; i < CL_user ; i++ ){
			if ( G_Colors[i] == c ) break;
		}
		G_Colors[i] = c; // 見つからなかったとき、CL_user
		G_Colors[CL_user] = c;
	}
	SendMessage(hListWnd, LB_SETCURSEL, (WPARAM)i, 0);
}

void ThemeCust(TCHAR *mem, DWORD size)
{
	TCHAR *Have_C_ext;

	Have_C_ext = tstrstr(mem, T("\nC_ext"));
	PPcustCStore(mem, mem + size, PPXCUSTMODE_STORE, NULL, NULL);
	HeapFree(GetProcessHeap(), 0, mem);

	if ( Have_C_ext == NULL ){ // C_ext がない場合は簡易明度調整を行う
		COLORREF C_back, color;
		DWORD blight, tmpblight;
		TCHAR ext[CUST_NAME_LENGTH];
		int i;

		C_back = GetSchemeColor(C_S_BACK, 0);
		// C_back の明るさを求める
		blight = GetRValue(C_back);
		blight = blight * blight * 2;
		tmpblight = GetGValue(C_back);
		blight += tmpblight * tmpblight * 4;
		tmpblight = GetBValue(C_back);
		blight += tmpblight * tmpblight * 3;
		// 基準値を決定
		blight = (blight >= (0xff * 0xff * 5)) ? 0x808080 : 0;

		for ( i = 0; ;i++ ){
			if ( 0 > EnumCustTable(i, T("C_ext"), ext, &color, sizeof(color)) ){
				break;
			}
			if ( (color & 0xff808080) != blight ) continue;
			// 基準値に該当するなら調整
			color ^= 0x808080; // 明度を 0.5 あげるか下げる
			SetCustTable(T("C_ext"), ext, &color, sizeof(color));
		}
	}
	Test();
}

void CSelectItem(HWND hDlg)
{
	const LISTS *gcl;
	TCHAR buf[0x4000];

	gcl = GcType;
	switch( gcl->flags & GC_mask ){
		case GC_list:
			memset(buf, 0xff, sizeof buf);
			GetCustData(gcl->item, buf, sizeof(buf));
			if ( gcl->flags & GC_ppbItem ){ // console の色に変換
				WORD conc;

				conc = ((WORD *)buf)[edit_sitem / 2];
				if ( conc == 0xffff ){
					conc = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
				}
				((COLORREF *)buf)[0] = ConsoleColors[conc & 0xf];
				((COLORREF *)buf)[1] = ConsoleColors[(conc >> 4) & 0xf];
				CheckDlgButton(hDlg, IDX_GCULINE, conc & COMMON_LVB_UNDERSCORE);
				SetColorList(hDlg, ((COLORREF *)buf)[edit_sitem & 1]);
			}else{
				SetColorList(hDlg, ((COLORREF *)buf)[edit_sitem]);
			}
			break;

		case GC_table:
			if ( ListView__GetText(GetDlgItem(hDlg, IDV_GCITEM), edit_sitem, buf, TSIZEOF(buf)) ){
				COLORREF c;

				GetCustTable(gcl->item, buf, (TCHAR *)&c, sizeof(c));
				SetColorList(hDlg, c);
			}
			break;

		case GC_theme: {
			TCHAR *mem;
			int BeforeX_uxt[X_uxt_items];
			int AfterX_uxt[X_uxt_items];
			HWND hPropWnd;

			GetCustData(T("X_uxt"), &BeforeX_uxt, sizeof(BeforeX_uxt));
			if ( edit_sitem == THEMEINDEX_RESTORE ){ // 元に戻す
				if ( OldColorTheme == NULL ) break;
				mem = HeapAlloc(GetProcessHeap(), 0, TSTRSIZE(OldColorTheme));
				if ( mem == NULL ) break;
				tstrcpy(mem, OldColorTheme);
			}else{
				if ( OldColorTheme == NULL ){ // バックアップ
					OldColorTheme = PPcust(PPXCUSTMODE_DUMP_PART_NOBOM, OldColorWildcard);
					memcpy(OldX_uxt, BeforeX_uxt, sizeof(OldX_uxt));
				}
				if ( edit_sitem <= THEMEINDEX_MAXRESOURCE ){
					mem = LoadTextResourceData(hInst,
						MAKEINTRESOURCE((edit_sitem - 1) + COLOR_THEME_1));
				}else{
					mem = GetUserTheme(hDlg);
				}
				if ( mem == NULL ) break;
			}
			SetCustData(T("X_uxt"), &OldX_uxt, sizeof(OldX_uxt));
			ThemeCust(mem, tstrlen(mem));

			GetCustData(T("X_uxt"), &AfterX_uxt, sizeof(AfterX_uxt));
			// テーマ指定を UXT_OFF にするときは、一旦 UXT_AUTO を反映しないと
			// 前回の指定が残る
			if ( (AfterX_uxt[0] < UXT_MINMODIFY) &&
				 (BeforeX_uxt[0] >= UXT_MINMODIFY) ){
				BeforeX_uxt[0] = UXT_AUTO;
				SetCustData(T("X_uxt"), &BeforeX_uxt, sizeof(BeforeX_uxt));
				Changed(hDlg); // UXT_AUTO で反映
				Sleep(300);
				SetCustData(T("X_uxt"), &AfterX_uxt, sizeof(AfterX_uxt));
				// AfterX_uxt で再反映させる
			}
			PPxCommonCommand(hDlg, 0, K_Lcust);
			X_uxt[0] = PPxCommonExtCommand(K_UxTheme, KUT_LOADCUST);

			Changed(hDlg); // 内部で GUILoadCust()
			hPropWnd = GetParent(hDlg);
			SetTabTitles(hPropWnd);
			LocalizeDialogText(hPropWnd, 0);
			InitPropSheetsUxtheme(hDlg);
			if ( X_uxt[0] == UXT_OFF ){
				SendDlgItemMessage(hDlg, IDV_GCTYPE, LVM_SETBKCOLOR, 0, (LPARAM)WinColors.c.DialogBack);
				SendDlgItemMessage(hDlg, IDV_GCTYPE, LVM_SETTEXTCOLOR, 0, (LPARAM)WinColors.c.DialogText);
				SendDlgItemMessage(hDlg, IDV_GCITEM, LVM_SETBKCOLOR, 0, (LPARAM)WinColors.c.DialogBack);
				SendDlgItemMessage(hDlg, IDV_GCITEM, LVM_SETTEXTCOLOR, 0, (LPARAM)WinColors.c.DialogText);
			}
			InvalidateRect(hPropWnd, NULL, TRUE);
			break;
		}
	}
	SetDlgItemText(hDlg, IDE_GCEDIT, NilStr);
}

void CSelectType(HWND hDlg)
{
	const LISTS *gcl;
	HWND hListView;
	BYTE buf[0x400];
	COLORREF color;
	DWORD flags;
	BOOL showflag;
	LV_ITEM lvi;

	hListView = GetDlgItem(hDlg, IDV_GCITEM);
	gcl = GcType;

	flags = gcl->flags;
	showflag = flags & GC_haveItem;	// アイテム有り(IDV_GCITEM有り)
	ShowDlgWindow(hDlg, IDS_GCITEM, showflag);
	ShowDlgWindow(hDlg, IDV_GCITEM, showflag);

	showflag = flags & GC_editExt;	// アイテム追加・登録・ソート(拡張子色用)
	ShowDlgWindow(hDlg, IDE_GCEDIT, showflag);
	ShowDlgWindow(hDlg, IDB_GCADD, showflag);
	ShowDlgWindow(hDlg, IDB_GCDEL, showflag);
	ShowDlgWindow(hDlg, IDB_GCSORTN, showflag);
	ShowDlgWindow(hDlg, IDB_GCSORTC, showflag);
	ShowDlgWindow(hDlg, IDB_GCGROUP, showflag);
	ShowDlgWindow(hDlg, IDX_GCULINE, flags & GC_ppbItem); // コンソール色
	SetWindowLongPtr(hListView, GWL_STYLE, showflag ?
			WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED :
			WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_OWNERDRAWFIXED);

	showflag = !(flags & GC_nocolor); // 色選択有り(GC_theme以外)
	ShowDlgWindow(hDlg, IDL_GCOLOR, showflag);
	ShowDlgWindow(hDlg, IDB_GCSEL, showflag);

	switch( flags & GC_mask ){
		case GC_single:{
			COLORREF c = C_AUTO;

			GetCustData(gcl->item, &c, sizeof(c));
			SetColorList(hDlg, c);
			break;
		}
		case GC_theme:
		case GC_list:{
			const TCHAR **list;
			int i;

			if ( (flags & GC_mask) == GC_theme ){
				memset(buf, (char)(BYTE)(COLOR_NO >> 24) , 32 * sizeof(COLORREF));
			}else{
				memset(buf, 0, 32 * sizeof(COLORREF));
				GetCustData(gcl->item, buf, sizeof(buf));
			}

			SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
			ListView_DeleteAllItems(hListView);
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = 0;
			lvi.iSubItem = 0;

			for ( list = gcl->sublist, i = 0 ; *list ; list++, i++ ){
				if ( gcl->flags & GC_ppbItem ){
					WORD bc;

					bc = ((WORD *)buf)[i / 2];
					if ( i & 1 ){
						color = ConsoleColors[(bc >> 4) & 0xf];
					}else{
						color = ConsoleColors[bc & 0xf];
					}
					lvi.lParam = color;
				}else{
					lvi.lParam = *((COLORREF *)buf + i);
				}
				lvi.pszText = (TCHAR *)MessageText(*list);
				ListView_InsertItem(hListView, &lvi);
				lvi.iItem++;
			}
			edit_sitem = (gcl->sublist == GCchar) ? 15 : 0;

			if ( (flags & GC_mask) == GC_theme ){
				PPXDBINFOSTRUCT dbinfo;
				TCHAR path[VFPS];
				HANDLE hFF;
				WIN32_FIND_DATA ff;

				dbinfo.structsize = sizeof dbinfo;
				dbinfo.custpath = path;
				GetPPxDBinfo(&dbinfo);
				tstrcpy( VFSFindLastEntry(path), USER_COLOR_HEADER T("*.cfg") );
				hFF = FindFirstFileL(path, &ff);
				if ( INVALID_HANDLE_VALUE != hFF ){
					do{
						if ( IsRelativeDir(ff.cFileName) ) continue;
						lvi.pszText = ff.cFileName + 6;
						lvi.lParam = COLOR_NO;
						ListView_InsertItem(hListView, &lvi);
					}while( FindNextFile(hFF, &ff) );
					FindClose(hFF);
				}
			}

			ListView__FixColorColumn(hListView);
			SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
			ListView__Select(hListView, edit_sitem);
			CSelectItem(hDlg);
			break;
		}

		case GC_table:{
			int size;
			TCHAR label[CUST_NAME_LENGTH];

			SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
			ListView_DeleteAllItems(hListView);

			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = 0;
			lvi.iSubItem = 0;
			lvi.pszText = label;

			for ( ;; ){
				size = EnumCustTable(lvi.iItem, gcl->item, label, &color, sizeof(color));
				if ( 0 > size ) break;

				lvi.lParam = color;
				ListView_InsertItem(hListView, &lvi);
				lvi.iItem++;
			}
			edit_sitem = 0;
			ListView__FixColorColumn(hListView);
			SendMessage(hListView, WM_SETREDRAW, TRUE, 0);

			CSelectItem(hDlg);
			break;
		}
	}
}

COLORREF USEFASTCALL ConColorConvertSub(COLORREF color)
{
	color &= 0xff;
	if ( color < 0x60 ) return 0;
	if ( color < 0xe0 ) return 0x80;
	return 0xff;
}

COLORREF USEFASTCALL ConColorConvert(COLORREF color)
{
	if ( color == C_DWHITE ) return color;
	color = ConColorConvertSub(color) |
		(ConColorConvertSub(color >> 8) << 8) |
		(ConColorConvertSub(color >> 16) << 16);
	if ( color & 0x010101 ){ // 0xff の要素を見つけたら、0x80→0xff変換
		if ( color & 0x000080 ) color |= 0x0000ff;
		if ( color & 0x008000 ) color |= 0x00ff00;
		if ( color & 0x800000 ) color |= 0xff0000;
	}
	return color;
}

void SelectColor(HWND hDlg, COLORREF selc)
{
	TCHAR buf[0x4000];

	if ( selc == COLOR_NO ) return;

	switch( GcType->flags & GC_mask ){
		case GC_single: {
			LV_ITEM lvi;

			SetCustData(GcType->item, (TCHAR *)&selc, sizeof(selc));
			lvi.mask = LVIF_PARAM;
			lvi.iItem = edit_gct;
			lvi.iSubItem = 0;
			lvi.lParam = selc;
			SendDlgItemMessage(hDlg, IDV_GCTYPE, LVM_SETITEM, 0, (LPARAM)&lvi);
			InvalidateRect(GetDlgItem(hDlg, IDV_GCTYPE), NULL, TRUE);
			if ( edit_gct <= GC_INFO ){
				PPxCommonCommand(hDlg, 0, K_Lcust);
				InvalidateRect(GetDlgItem(hDlg, IDL_GCOLOR) , NULL, FALSE);
			}
			break;
}
		case GC_list: {
			DWORD size, minsize;
			HWND hListView = GetDlgItem(hDlg, IDV_GCITEM);

			if ( GcType->flags & GC_ppbItem ){	// コンソール
				int i;
				WORD *dest;

				minsize = GcType->subs * sizeof(WORD);
				size = GetCustDataSize(GcType->item);
				if ( (size < minsize) || (size > 0x800000) ) size = minsize;
				dest = &((WORD *)buf)[edit_sitem / 2];
				*dest = 0;

				GetCustData(GcType->item, buf, sizeof(buf));
				selc = ConColorConvert(selc);
				for ( i = 0 ; i < CONSOLECOLOR ; i++ ){
					if ( ConsoleColors[i] == selc ) break;
				}
				if ( i < CONSOLECOLOR ){
					if ( edit_sitem & 1 ){ // B4-7
						*dest = (WORD)((*dest & 0x000f) | (i << 4));
					}else{				// B0-3
						*dest = (WORD)((*dest & 0x00f0) | i);
					}
					if ( IsDlgButtonChecked(hDlg, IDX_GCULINE) ){
						setflag(*dest, COMMON_LVB_UNDERSCORE);
					}
					SetCustData(GcType->item, buf, size);
				}else{
					selc = (COLORREF)ListView__GetItemData(hListView, edit_sitem);
				}
				CSelectItem(hDlg);
			}else{									// GUI
				if ( GcType->sublist == GCscheme ){
					selc = GetSchemeColor(selc, G_Colors[CL_auto]);
				}
				minsize = GcType->subs * sizeof(COLORREF);
				size = GetCustDataSize(GcType->item);
				if ( (size < minsize) || (size > 0x800000) ) size = minsize;

				memset(buf, 0xff, sizeof buf);
				GetCustData(GcType->item, buf, sizeof(buf));
				((COLORREF *)buf)[edit_sitem] = selc;
				SetCustData(GcType->item, buf, size);
			}
			ListView__SetItemData(hListView, edit_sitem, selc);

			if ( GcType->item ==
					((X_uxt[0] == UXT_DARK) ? StrC_schD : StrC_schN) ){
				PPxCommonCommand(hDlg, 0, K_Lcust);
				InvalidateRect(GetDlgItem(hDlg, IDL_GCOLOR) , NULL, FALSE);
			}

			InvalidateRect(hListView, NULL, FALSE);
			break;
		}
		case GC_table: { // 拡張子色
			HWND hListView = GetDlgItem(hDlg, IDV_GCITEM);
			LONG_PTR index = -1;

			for (;;){
				index = SendMessage(hListView, LVM_GETNEXTITEM, index, TMAKELPARAM(LVNI_SELECTED, 0));
				if ( index < 0 ) break;

				if ( ListView__GetText(hListView, index, buf, TSIZEOF(buf)) ){
					SetCustTable(GcType->item, buf, (TCHAR *)&selc, sizeof(selc));
					ListView__SetItemData(hListView, index, selc);
				}
			}
			InvalidateRect(hListView, NULL, FALSE);
			break;
		}
	}
	FixColorList();
}

void ChangeUnderline(HWND hDlg)
{
	DWORD size, minsize;
	WORD *dest;
	BYTE buf[0x100];

	minsize = GcType->subs * sizeof(WORD);
	size = GetCustDataSize(GcType->item);
	if ( (size < minsize) || (size > 0x800000) ) size = minsize;
	dest = &((WORD *)buf)[edit_sitem / 2];
	*dest = 0;

	GetCustData(GcType->item, buf, sizeof(buf));
	*dest = (WORD)(*dest & ~COMMON_LVB_UNDERSCORE);
	if ( IsDlgButtonChecked(hDlg, IDX_GCULINE) ){
		setflag(*dest, COMMON_LVB_UNDERSCORE);
	}
	SetCustData(GcType->item, buf, size);
}

void InitColorControls(HWND hDlg)
{
	HWND hCListWnd;
	RECT box;
	int i;

	hCListWnd = GetDlgItem(hDlg, IDL_GCOLOR);

		/* はじめは、値に COLORREF を直接入れていたが、SP 無し
		   WinXP で、0 等を使用しても登録されないため、ダミーの値を
		   入れるように変更した									*/
	SendMessage(hCListWnd, LB_RESETCONTENT, 0, 0);
	for ( i = 0 ; i < COLORLIST_COLORS ; i++ ){
		SendMessage(hCListWnd, LB_ADDSTRING, 0, (LPARAM)1);
	}

	GetClientRect(hCListWnd, &box);
	SendMessage(hCListWnd, LB_SETCOLUMNWIDTH,
			(WPARAM)((box.right - box.left) / COLORLIST_WIDTH), 0);
	SendMessage(hCListWnd, LB_SETITEMHEIGHT,
			0, (LPARAM)((box.bottom - box.top) / COLORLIST_HEIGHT));
	DrawHeight = InitDrawHeight(hDlg);
}

void InitColorPage(HWND hDlg)
{
	int i;
	HWND hControlWnd;
	COLORREF color;
	LV_ITEM lvi;
	LV_COLUMN lvc;

	InitColorControls(hDlg);

	lvc.mask = LVCF_WIDTH;
	lvc.cx = 200;

	hControlWnd = GetDlgItem(hDlg, IDV_GCTYPE);
	if ( hControlWnd != NULL ){
		SendMessage(hControlWnd, WM_SETREDRAW, FALSE, 0);
		SendMessage(hControlWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

		ListView_InsertColumn(hControlWnd, 0, &lvc);

		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.iItem = 0;
		lvi.iSubItem = 0;

		for ( i = 0 ; GClist[i].name ; i++ ){
			lvi.pszText = (TCHAR *)MessageText(GClist[i].name);
			if ( (GClist[i].flags & GC_mask) == GC_single ){
				GetCustData(GClist[i].item, &color, sizeof(color));
				lvi.lParam = color;
			}else{
				lvi.lParam = COLOR_SUB;
			}
			ListView_InsertItem(hControlWnd, &lvi);
			lvi.iItem++;
		}
		ListView__FixColorColumn(hControlWnd);
		SendMessage(hControlWnd, WM_SETREDRAW, TRUE, 0);
	}

	hControlWnd = GetDlgItem(hDlg, IDV_GCITEM);
	if ( hControlWnd != NULL ){
		SendMessage(hControlWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
		ListView_InsertColumn(hControlWnd, 0, &lvc);
	}

	SendDlgItemMessage(hDlg, IDE_GCEDIT, EM_SETCUEBANNER, 0, (LPARAM)GetBannerText(stringExtInfo));

	FixColorList();
}

// 選択可能な色の一覧の表示
void ColorSampleDraw(DRAWITEMSTRUCT *lpdis)
{
	if ( lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT) ){
		int OldBkMode;
		HBRUSH hB;
		COLORREF showcolor;

		OldBkMode = SetBkMode(lpdis->hDC, OPAQUE);
		showcolor = (lpdis->itemID == CL_auto) ?
				C_WindowBack : G_Colors[lpdis->itemID];

		if ( GcType->flags & GC_ppbItem ){
			showcolor = ConColorConvert(showcolor);
		}

		hB = CreateSolidBrush(showcolor);
		FillRect(lpdis->hDC, &lpdis->rcItem, hB);
		DeleteObject(hB);

		if ( lpdis->itemID >= CL_withtext ){
			const TCHAR *text;

			text = IndexColorName[lpdis->itemID - CL_withtext];
			if ( (lpdis->itemID == CL_auto) && (GcType->flags & GC_aoff) ){
				text = stringaoff;
			}
			text = MessageText(text);
			TextOut(lpdis->hDC, lpdis->rcItem.left + 2,
					lpdis->rcItem.top + 2, text, tstrlen32(text));
		}

		SetBkMode(lpdis->hDC, OldBkMode);
		if ( lpdis->itemState & ODS_SELECTED ){
			RECT rect;

			FrameRect(lpdis->hDC, &lpdis->rcItem, GetStockObject(BLACK_BRUSH));
			rect.left = lpdis->rcItem.left + 1;
			rect.top = lpdis->rcItem.top + 1;
			rect.right = lpdis->rcItem.right - 1;
			rect.bottom = lpdis->rcItem.bottom - 1;
			FrameRect(lpdis->hDC, &rect, GetStockObject(WHITE_BRUSH));
		}
	}
	if ( lpdis->itemAction & ODA_FOCUS ){
		int i;
		RECT rect;
		for ( i = 0 ; i < 2 ; i++ ){
			rect.left = lpdis->rcItem.left + i;
			rect.top = lpdis->rcItem.top + i;
			rect.right = lpdis->rcItem.right - i;
			rect.bottom = lpdis->rcItem.bottom - i;
			DrawFocusRect(lpdis->hDC, &rect);
		}
	}
}

// 項目一覧 + 現在の色の表示
void ColorItemDraw(DRAWITEMSTRUCT *lpdis)
{
	TCHAR buf[CMDLINESIZE];
	int OldBkMode;
	COLORREF OldText, OldBack;
	RECT box;
	HBRUSH hB;
	LV_ITEM lvi;

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = lpdis->itemID;
	lvi.iSubItem = 0;
	lvi.pszText = buf;
	lvi.cchTextMax = TSIZEOF(buf);

	ListView_GetItem(lpdis->hwndItem, &lvi);

	OldBkMode = SetBkMode(lpdis->hDC, OPAQUE);
	box = lpdis->rcItem;
	if ( (lpdis->itemAction & ODA_FOCUS) && !(lpdis->itemState & ODS_FOCUS) ){
		hB = CreateSolidBrush(C_WindowBack);
		FillRect(lpdis->hDC, &lpdis->rcItem, hB);
		DeleteObject(hB);
	}

	if ( lpdis->itemState & ODS_SELECTED ){
		OldText = SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
		OldBack = SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
	}else{
		OldText = SetTextColor(lpdis->hDC, C_WindowText);
		OldBack = SetBkColor(lpdis->hDC, C_WindowBack);
	}

	box.right = box.left + (box.bottom - box.top);
	if ( lvi.lParam >= COLOR_SPECIAL ){
		if ( lvi.lParam == COLOR_SUB ){ // 下層アリ
			TextOut(lpdis->hDC, box.left, box.top, SubMenuString, 2);
		}else if ( lvi.lParam == C_AUTO ){ // 自動
			TextOut(lpdis->hDC, box.left, box.top, AutoString, 2);
		}else{
			box.right = box.left;
		}
	}else{
		hB = CreateSolidBrush(GetSchemeColor((COLORREF)lvi.lParam, 0));
		FillRect(lpdis->hDC, &box, hB);
		DeleteObject(hB);
	}
	TextOut(lpdis->hDC, box.right, box.top, buf, tstrlen32(buf));
	SetTextColor(lpdis->hDC, OldText);
	SetBkColor(lpdis->hDC, OldBack);
	if ( lpdis->itemState & ODS_FOCUS ){
		DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
	}
	SetBkMode(lpdis->hDC, OldBkMode);
}

BOOL SelectUserColor(HWND hDlg)
{
	CHOOSECOLOR cc;
	COLORREF color;
	HWND hListWnd;

	hListWnd = GetDlgItem(hDlg, IDL_GCOLOR);
	color = GetGindexCOLOR(hListWnd);
	if ( color == COLOR_NO ) color = G_Colors[CL_prev];
	color = GetSchemeColor(color, G_Colors[CL_auto]);
	cc.rgbResult = color;

	userColor[userColor_SEL] = GetSchemeColor(G_Colors[CL_user], G_Colors[CL_auto]);
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hDlg;
	cc.lpCustColors = userColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	if ( ChooseColor(&cc) ){
		G_Colors[CL_user] = cc.rgbResult;
		SendMessage(hListWnd, LB_SETCURSEL, (WPARAM)CL_user, 0);
		return TRUE;
	}
	return FALSE;
}

#pragma argsused
INT_PTR CALLBACK ColorPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR buf[MAX_PATH];

	switch (msg){
		case WM_SETTINGCHANGE:
			DrawHeight = -1;
			InitColorControls(hDlg);
			break;

		case WM_INITDIALOG:
			InitPropSheetsUxtheme(hDlg);
			InitColorPage(hDlg);
			CSelectType(hDlg);
			return FALSE;

		case WM_MEASUREITEM:
			if ( (wParam == IDV_GCTYPE) || (wParam == IDV_GCITEM) ){
				((MEASUREITEMSTRUCT *)lParam)->itemHeight =
						InitDrawHeight(GetDlgItem(hDlg, IDV_GCITEM));
			}
			return TRUE;

		case WM_DRAWITEM:
			if ( wParam == IDL_GCOLOR ){
				ColorSampleDraw((DRAWITEMSTRUCT *)lParam);
			}else if ( (wParam == IDV_GCTYPE) || (wParam == IDV_GCITEM) ){
				ColorItemDraw((DRAWITEMSTRUCT *)lParam);
			}
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDB_TEST:
					Test();
					break;

				case IDB_GCADD:
					GetControlText(hDlg, IDE_GCEDIT, buf, TSIZEOF(buf));
					if ( buf[0] != '\0' ){
						int index;
						LV_ITEM lvi;
						LV_FINDINFO lvf;
						HWND hListView = GetDlgItem(hDlg, IDV_GCITEM);

						FixWildItemName(buf);
						if ( !IsExistCustTable(GcType->item, buf) ){
							COLORREF color;

							lvi.mask = LVIF_TEXT;
							lvi.iItem = INT32__MAX;
							lvi.iSubItem = 0;
							lvi.pszText = buf;

							color = GetGindexCOLOR(GetDlgItem(hDlg, IDL_GCOLOR));
							if ( color == COLOR_NO ) color = G_Colors[CL_prev];
							edit_sitem = ListView_InsertItem(hListView, &lvi);
							SelectColor(hDlg, color);
							Changed(hDlg);
						}
						lvf.flags = LVFI_STRING;
						lvf.psz = buf;
						index = ListView_FindItem(hListView, 0, &lvf);
						if ( index >= 0 ){
							ListView__Select(hListView, index);
							edit_sitem = index;
						}
						SetDlgItemText(hDlg, IDE_GCEDIT, NilStr);
					}
					break;

				case IDB_GCDEL:
					if ( edit_sitem >= 0 ){
						HWND hListView = GetDlgItem(hDlg, IDV_GCITEM);
						int old_edit_sitem;
						LONG_PTR index;

						old_edit_sitem = edit_sitem;
						for (;;){
							index = SendMessage(hListView, LVM_GETNEXTITEM,
									-1, TMAKELPARAM(LVNI_SELECTED, 0));
							if ( index < 0 ) break;

							if ( ListView__GetText(hListView, index, buf, TSIZEOF(buf)) ){
								DeleteCustTable(GcType->item, buf, 0);
								ListView_DeleteItem(hListView, index);
							}
						}
						ListView__Select(hListView, old_edit_sitem);
						Changed(hDlg);
					}
					break;

				case IDL_GCOLOR:
					if ( HIWORD(wParam) == LBN_SELCHANGE ){
						SelectColor(hDlg, GetGindexCOLOR((HWND)lParam));
						Changed(hDlg);
					}else if ( HIWORD(wParam) == IDL_GCOLOR ){
						InitColorControls(hDlg);
					}
					break;

				case IDB_GCSEL:
					if ( IsTrue(SelectUserColor(hDlg)) ){
						SelectColor(hDlg, G_Colors[CL_user]);
						Changed(hDlg);
					}
					InvalidateRect(GetDlgItem(hDlg, IDL_GCOLOR), NULL, FALSE);
					break;

				case IDB_GCSORTN:
					SortCustTable(T("C_ext"), NULL);
					CSelectType(hDlg);
					break;

				case IDB_GCSORTC:
					SortCustTable(T("C_ext"), SortColorFunc);
					CSelectType(hDlg);
					break;

				case IDX_GCULINE:
					ChangeUnderline(hDlg);
					Changed(hDlg);
					break;

				case IDB_GCGROUP: {
					HWND hListView = GetDlgItem(hDlg, IDV_GCITEM);
					COLORREF oldcolor = (COLORREF)ListView__GetItemData(hListView, edit_sitem);
					LV_ITEM lvi;
					LONG_PTR index = 0;

					for (;;){
						lvi.mask = LVIF_PARAM;
						lvi.iItem = (int)index;
						lvi.iSubItem = 0;
						if ( ListView_GetItem(hListView, &lvi) == FALSE ){
							break;
						}
						if ( (COLORREF)lvi.lParam == oldcolor ){
							lvi.mask = LVIF_STATE;
							lvi.state = lvi.stateMask = LVIS_SELECTED;
							SendMessage(hListView, LVM_SETITEMSTATE, index, (LPARAM)&lvi);
						}
						index++;
					}
					InvalidateRect(hListView, NULL, FALSE);
					break;
				}
			}
			break;

		case WM_NOTIFY:
			#define NHPTR ((NMHDR *)lParam)
			#define PNM ((NM_LISTVIEW *)lParam)
			if ( NHPTR->code == PSN_SETACTIVE ){
				InitWndIcon(hDlg, IDB_TEST);
			}else if ( wParam == IDV_GCTYPE ){
				if ( NHPTR->code == LVN_ITEMCHANGED ){
					if ( edit_gct != PNM->iItem ){
						edit_gct = PNM->iItem;
						GcType = &GClist[edit_gct];
						CSelectType(hDlg);
					}
				}
				return TRUE;
			}else if ( wParam == IDV_GCITEM ){
				if ( NHPTR->code == LVN_ITEMCHANGED ){
					if ( edit_sitem != PNM->iItem ){
						edit_sitem = PNM->iItem;
						CSelectItem(hDlg);
					}
				}
				return TRUE;
			}
			#undef PNM
			#undef NHPTR
		// default へ
		default:
			return DlgSheetProc(hDlg, msg, wParam, lParam, IDD_COLOR);
	}
	return TRUE;
}

//=============================================================================
INT_PTR CALLBACK HideMenuColorDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg){
		case WM_INITDIALOG:
			LocalizeDialogText(hDlg, IDD_SCOLOR);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
			InitColorPage(hDlg);
			SetColorList(hDlg, *(COLORREF *)lParam);
			return TRUE;

		case WM_DRAWITEM:
			if ( wParam == IDL_GCOLOR ){
				ColorSampleDraw((DRAWITEMSTRUCT *)lParam);
			}
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDB_GCSEL:
					SelectUserColor(hDlg);
					break;

				case IDL_GCOLOR:
					if ( HIWORD(wParam) == LBN_SELCHANGE ){
						COLORREF color;

						color = GetGindexCOLOR((HWND)lParam);
						if ( color != COLOR_NO ){
							*(COLORREF *)GetWindowLongPtr(hDlg, DWLP_USER) =
									color;
						}
					}
					break;

				case IDOK:
					EndDialog(hDlg, 1);
					break;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;
			}
			break;

		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}

//=============================================================================
// ハイライト(CV_hkey)

void InitHighlightDialogBox(HWND hDlg, TCHAR *subkey)
{
	TCHAR text[VFPS], data[0x10000], *line, *next;
	HWND hListView;
	LV_ITEM lvi;

	LocalizeDialogText(hDlg, 0);
	thprintf(text, TSIZEOF(text), MessageText(MES_TEHC), subkey);
	SetWindowText(hDlg, text);

	FixWildItemName(subkey);
	SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)subkey);
	InitColorPage(hDlg);

	hListView = GetDlgItem(hDlg, IDV_GCITEM);
	SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
	ListView_DeleteAllItems(hListView);

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = 0;
	lvi.iSubItem = 0;

	data[0] = '\0';
	if ( (NO_ERROR == GetCustTable(T("CV_hkey"), subkey, data, sizeof(data))) && data[0] ){
		line = data;
		for (;;){
			COLORREF color;
			TCHAR *dst;

			dst = text;
			next = tstrchr(line, '\n');
			if ( next != NULL ) *next = '\0';
			for(;;){
				TCHAR c = *line;
				if ( (c == '<') || (c == '>') ){
					*dst++ = c;
					line++;
					continue;
				}
				break;
			}
			*dst++ = ',';
			color = GetColor((const TCHAR **)&line, TRUE);
			if ( SkipSpace((const TCHAR **)&line) == ',' ) line++;
			SkipSpace((const TCHAR **)&line);
			tstrcpy(dst, line);

			if ( dst[0] != '\0' ){
				lvi.pszText = text;
				lvi.lParam = color;
				ListView_InsertItem(hListView, &lvi);
				lvi.iItem++;
			}

			if ( next != NULL ){
				line = next + 1;
				continue;
			}
			break;
		}
	}
	ListView__FixColorColumn(hListView);
	SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
}

void GetHighlightItem(HWND hListView, int index, HighlightItem *hi)
{
	TCHAR *wordptr;

	if ( hListView != NULL ){
		ListView__GetText(hListView, index, hi->text, TSIZEOF(hi->text));
	}

	hi->cb_top = hi->cb_bottom = FALSE;
	wordptr = hi->text;

	for(;;){
		TCHAR c = *wordptr;

		if ( c == '<' ){
			hi->cb_top = TRUE;
		}else if ( c == '>' ){
			hi->cb_bottom = TRUE;
		}else{
			if ( c == ',' ) wordptr++;
			hi->textptr = wordptr;
			return;
		}
		wordptr++;
	}
}

void SetHighlightItem(HWND hListView, int index, HighlightItem *hi, COLORREF color)
{
	TCHAR text[VFPS], *dst = text;
	LV_ITEM lvi;

	if ( hi->cb_top ) *dst++ = '<';
	if ( hi->cb_bottom ) *dst++ = '>';
	if ( hListView == NULL ) dst = thprintf(dst, 16, T("H%06X"), color);
	*dst++ = ',';
	tstrcpy(dst, hi->textptr);
	if ( hListView == NULL ){
		tstrcpy(hi->text, text);
		return;
	}

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.lParam = color;
	lvi.pszText = text;

	if ( index >= 0 ){
		lvi.iItem = index;
		ListView_SetItem(hListView, &lvi);
	}else{
		lvi.iItem = INT32__MAX;
		index = ListView_InsertItem(hListView, &lvi);
		ListView__Select(hListView, index);
	}
}

void ChangeHighlightItem(HWND hDlg, int id)
{
	HighlightItem hi;
	HWND hListView;
	int index;
	COLORREF color;
	TCHAR text[VFPS];

	hListView = GetDlgItem(hDlg, IDV_GCITEM);
	index = (int)TListView_GetFocusedItem(hListView);
	text[0] = '\0';
	GetControlText(hDlg, IDE_GCEDIT, text, TSIZEOF(text));

	if ( index >= 0 ){ // 選択有り
		GetHighlightItem(hListView, index, &hi);
		color = (COLORREF)ListView__GetItemData(hListView, index);
		if ( tstrcmp(hi.textptr, text) != 0 ){
			if ( id != IDB_GCADD ) return;
			index = -1;
		}
	}
	if ( index < 0 ){ // 選択無し…新規(addボタンの時のみ)
		if ( (id != IDB_GCADD) || (text[0] == '\0') ) return;
		color = GetGindexCOLOR(GetDlgItem(hDlg, IDL_GCOLOR));
		if ( color == COLOR_NO ) return;
		tstrcpy(hi.text, text);
		hi.textptr = hi.text;
	}
	hi.cb_top = IsDlgButtonChecked(hDlg, IDX_HL_TOP);
	hi.cb_bottom = IsDlgButtonChecked(hDlg, IDX_HL_BOTTOM);

	SetHighlightItem(hListView, index, &hi, color);
}

void SelectHighlightItem(HWND hDlg, int index)
{
	HighlightItem hi;
	HWND hListView;

	hListView = GetDlgItem(hDlg, IDV_GCITEM);
	GetHighlightItem(hListView, index, &hi);
	if ( hi.textptr == hi.text ) return;

	SetDlgItemText(hDlg, IDE_GCEDIT, hi.textptr);
	SetColorList(hDlg, (COLORREF)ListView__GetItemData(hListView, index));
	CheckDlgButton(hDlg, IDX_HL_TOP, hi.cb_top);
	CheckDlgButton(hDlg, IDX_HL_BOTTOM, hi.cb_bottom);
}

void SelectHighlightColor(HWND hDlg, COLORREF color)
{
	if ( color == COLOR_NO ) return;
	ListView__SetItemData(GetDlgItem(hDlg, IDV_GCITEM), -1, color);
}

void SaveHighlightDialogBox(HWND hDlg)
{
	HighlightItem hi;
	HWND hListView;
	int count, index = 0;
	TCHAR text[0x2000], *dst = text, *subkey;

	subkey = (TCHAR *)GetWindowLongPtr(hDlg, DWLP_USER);
	hListView = GetDlgItem(hDlg, IDV_GCITEM);

	count = ListView_GetItemCount(hListView);
	if ( count == 0 ){
		DeleteCustTable(T("CV_hkey"), subkey, 0);
		return;
	}
	while ( index < count ){
		COLORREF color;

		GetHighlightItem(hListView, index, &hi);
		color = (COLORREF)ListView__GetItemData(hListView, index);
		SetHighlightItem(NULL, 0, &hi, color);
		dst = thprintf(dst, CMDLINESIZE, T("%s\n"), hi.text);
		index++;
	}
	*dst = '\0';
	SetCustStringTable(T("CV_hkey"), subkey, text, 0);
}

INT_PTR CALLBACK HighlightDialogBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg){
		case WM_INITDIALOG:
			InitHighlightDialogBox(hDlg, (TCHAR *)lParam);
			return FALSE;

		case WM_MEASUREITEM:
			if ( wParam == IDV_GCITEM ){
				((MEASUREITEMSTRUCT *)lParam)->itemHeight =
						InitDrawHeight(GetDlgItem(hDlg, wParam));
			}
			return TRUE;

		case WM_DRAWITEM:
			if ( wParam == IDL_GCOLOR ){
				ColorSampleDraw((DRAWITEMSTRUCT *)lParam);
			}else if ( wParam == IDV_GCITEM ){
				ColorItemDraw((DRAWITEMSTRUCT *)lParam);
			}
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDOK:
					SaveHighlightDialogBox(hDlg);
					EndDialog(hDlg, 1);
					break;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;

				case IDL_GCOLOR:
					if ( HIWORD(wParam) == LBN_SELCHANGE ){
						SelectHighlightColor(hDlg,
								GetGindexCOLOR(GetDlgItem(hDlg, IDL_GCOLOR)));
					}
					break;

				case IDX_HL_TOP:
				case IDX_HL_BOTTOM:
				case IDB_GCADD:
					ChangeHighlightItem(hDlg, LOWORD(wParam));
					break;

				case IDB_GCDEL: {
					int index;
					HWND hListView = GetDlgItem(hDlg, IDV_GCITEM);

					index = TListView_GetFocusedItem(hListView);
					if ( index < 0 ) break;
					ListView_DeleteItem(hListView, index);
					ListView__Select(hListView, index);
					break;
				}

				case IDB_GCSEL:
					if ( IsTrue(SelectUserColor(hDlg)) ){
						SelectHighlightColor(hDlg, CL_user);
					}
					InvalidateRect(GetDlgItem(hDlg, IDL_GCOLOR), NULL, FALSE);
					break;
			}
			break;

		case WM_NOTIFY:
			#define NHPTR ((NMHDR *)lParam)
			#define PNM ((NM_LISTVIEW *)lParam)
			if ( wParam == IDV_GCITEM ){
				if ( NHPTR->code == LVN_ITEMCHANGED ){
					if ( PNM->iItem >= 0 ){
						SelectHighlightItem(hDlg, PNM->iItem);
						return TRUE;
					}
				}
			}
			#undef PNM
			#undef NHPTR
		// default へ

		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}
