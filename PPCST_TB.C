/*-----------------------------------------------------------------------------
	Paper Plane xUI	customizer	ファイル判別/キー割当て/マウス割当て/メニュー
-----------------------------------------------------------------------------*/
#pragma setlocale("Japanese")
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPCUST.H"
#pragma hdrstop

#define AddDefaultCommandList 0

#ifndef NM_CUSTOMDRAW
#define NM_CUSTOMDRAW           (0-12)
typedef struct tagNMCUSTOMDRAWINFO
{
	NMHDR hdr;
	DWORD dwDrawStage;
	HDC hdc;
	RECT rc;
	DWORD_PTR dwItemSpec;
	UINT  uItemState;
	LPARAM lItemlParam;
} NMCUSTOMDRAW;
#endif
#ifndef CDRF_NOTIFYITEMDRAW
#define CDRF_NOTIFYITEMDRAW     0x00000020
#define CDRF_DODEFAULT          0x00000000
#endif

#define ID_COMMAND_ENUMTYPE IDW_INTERNALMIN

// ListView の lParam に設定する値。PPcust内で共通＆クリアしないが、通常の
// 使用範囲ではオーバーフローしないのでそのまま使う。
LPARAM ListViewCounter = 1;

typedef struct {
	HWND	hLVTypeWnd, hLVAlcWnd;	// ListView
	int		ListViewLastSort;		// IDV_ALCLISTのソート状態(+昇順, -降順)
	TCHAR	key;					// 項目の識別子
	int		helpID;					// [ヘルプ]を押したときに表示するヘルプ
	int		index_type;				// 種類のインデックス
	int		index_alc;				// 項目のインデックス
	TCHAR	name_type[MAX_PATH];	// 選択された種類の内容
} TABLEINFO;

TABLEINFO extinfo;		// key = E
TABLEINFO keyinfo;		// key = K
TABLEINFO mouseinfo;	// key = m
TABLEINFO menuinfo;		// key = M
TABLEINFO barinfo;		// key = B

struct LABELTEXT {
 const TCHAR *name, *text;
} TypeLists[] = {
 {T("E_cr"),	MES_VXCR},
 {T("E_scr"),	MES_VXSC},
 {T("E_unpack2"), MES_VXUP},
 {T("E_TipView"), MES_VXTV},

 {T("K_edit"),	MES_VKED},
 {T("K_lied"),	MES_VKLI},
 {T("K_ppe"),	MES_VKPE},
 {T("K_tray"),	MES_VKTY},
 {T("K_tree"),	MES_VKTR},
 {T("KB_edit"),	MES_VKPB},
 {T("KB_list"),	MES_VKBL},
 {T("KB_ref"),	MES_VKBR},
 {T("KC_main"),	MES_VKCG},
 {T("KC_tree"),	MES_VKCT},
 {T("KC_incs"),	MES_VKCI},
 {T("K_list"),	MES_VKAL},
 {T("KV_main"),	MES_VKPV},
 {T("KV_page"),	MES_VKPT},
 {T("KV_crt"),	MES_VKPC},
 {T("KV_img"),	MES_VKVI},

 {T("M_edit"),	MES_VMLB},
 {T("M_editc"),	MES_VMEC},
 {T("M_pjump"),	MES_VMPJ},
 {T("MC_menu"),	MES_VMCB},
 {T("M_Ccr"),	MES_VMCR},
 {T("M_DirMenu"), MES_VMDI},
 {T("M_mask"),	MES_VMWI},
 {T("MC_sort"),	MES_VMSO},
 {T("M_wsrc"), MES_VMWS},
 {T("MC_mdds"),	MES_VMMD},
 {T("M_xpack"),	MES_VMEP},
 {T("M_bin"),	MES_VMOB},
 {T("M_tabc"),	MES_VMTA},
 {T("MV_menu"),	MES_VMVB},
 {T("M_ppvc"),	MES_VMVC},
 {T("ME_menu"),	MES_VMEB},

 {T("MC_click"), MES_VPPC},
 {T("MV_click"), MES_VPPV},
 {T("MT_icon"),	MES_VPPI},

 {T("B_flm"),	MES_VRFM},
 {T("B_flp"),	MES_VRFP},
 {T("B_flt"),	MES_VRFT},

 {T("B_cdef"),	MES_VRCT},
 {T("B_vdef"),	MES_VRVT},
 {T("B_tree"),	MES_VRTT},

 {T("HM_ppc"),	MES_VHPC},
 {T("HM_ppv"),	MES_VHPV},
 {NULL, NULL}
};

const TCHAR *InsertMacroMenuString[] = {
	T(" %FCD	選択ファイル名(１つのみ)\0 %FCD	one file name"),
	T(" %#FCD	選択ファイル名(列挙)\0 %#FCD	enum. file names"),
	T(" %1	カレントディレクトリ\0 %1	current directory name"),
	T(" %2	反対窓ディレクトリ\0 %2	pair window directory name"),
	T(" %0	PPxディレクトリ\0 %0	PPx directory name"),
	T(" %M	メニュー...\0 %M	menus..."),
	T(" %ME	ファイル判別...\0 %ME	extension lists..."),
	T(" %'	環境変数・エイリアス...\0 %'	env. , alias"),
	T(" %{text%|%}	編集(textが既定値,カーソル末尾)\0 %{text%|%}	edit \'text\'"),
	T("%Ob 	PPbを使わずに実行指定(先頭に記載)\0%Ob	no use PPb(written top)"),
	T("%Or-	マークがあっても１つのみ実行指定(末尾に記載)\0%Or-	no enum.(written bottom)"),
	T(" %:	コマンド区切り\0 %:	command separator"),
	NULL
};

const TCHAR *InsertExtraMenuString[] = {
	T("?favorites"),
	T("?ppclist"),
	T("?ppxidlist"),
	T("?selectppx"),
	T("?eject"),
	T("?drivelist"),
	T("?drivemenu"),
	T("?extdrivelist"),
	T("?extdrivemenu"),
	T("?exjumpmenu"),
	T("?diroptionmenu"),
	T("?layoutmenu"),
	T("?sortmenu"),
	T("?viewmenu"),
	T("?newmenu"),
	NULL
};

const TCHAR *InsertExtraMenuStringText[] = {
	T("お気に入り\0favorites"),
	T("PPc パス一覧\0PPc path list"),
	T("PPx ID 一覧\0PPx ID list"),
	T("指定 PPx へフォーカス移動\0Focus to selected PPx"),
	T("ドライブの取り外し\0Eject drive"),
	T("ドライブ一覧\0drive list\0"),
	T("[PPc]ドライブ選択\0[PPc] Select drive\0"),
	T("非表示ドライブを含むドライブのパス\0drive path with hidden drives"),
	T("[PPc]非表示ドライブを含むドライブの選択\0Select drive with hidden drives"),
	T("[PPc]種類を指定してディレクトリ移動\0[PPc] Select type for folder"),
	T("[PPc]ディレクトリ設定\0[PPc] folder settings"),
	T("[PPc]レイアウト\0[PPc] window layout"),
	T("[PPc]並び替え\0[PPc] sort"),
	T("[PPc]表示形式\0[PPc] view style"),
	T("[PPc]エントリ作成\0[PPc] new entry"),
	NULL
};

const TCHAR InsertMacroExt_LAUNCHstr[] = MES_VIML;
const TCHAR InsertMacroExt_KEYCODEstr[] = MES_VIMY;
const TCHAR InsertMacroExt_FILENAMEstr[] = MES_VIMF;

const TCHAR *InsertMenuSeparatorString[] = {
	MES_VIMS, MES_VIMC, MES_VIMK, MES_VIMI,
};

#define GESTUREID 0
struct MouseButtonListStruct{
	const TCHAR type[4], *name;
} MouseButtonList[] = {
	{T("RG"), MES_VLRG},
	{T("R"),  MES_VLRC}, // 右
	{T("RD"), MES_VLRD},
	{T("RH"), MES_VLRH},
	{T("L"),  MES_VLLC}, // 左
	{T("LD"), MES_VLLD},
	{T("LH"), MES_VLLH},
	{T("M"),  MES_VLMC}, // 中／ホイール
	{T("MD"), MES_VLMD},
	{T("MH"), MES_VLMH},
	{T("W"),  MES_VLWC}, // 左右同時
	{T("WH"), MES_VLWH},
	{T("X"),  MES_VLXC}, // 第4
	{T("XD"), MES_VLXD},
	{T("XH"), MES_VLXH},
	{T("Y"),  MES_VLYC}, // 第5
	{T("YD"), MES_VLYD},
	{T("YH"), MES_VLYH},
	{T("H"),  MES_VLHT}, // チルト
	{T("I"),  MES_VLIT},
	{T("\0"), NULL}
};
#define CE_SHIFT 8
#define CE_RG	B8
#define CE_R	B9
#define CE_RD	B10
#define CE_RH	B11
#define CE_L	B12
#define CE_LD	B13
#define CE_LH	B14
#define CE_M	B15
#define CE_MD	B16
#define CE_MH	B17
#define CE_W	B18
#define CE_WH	B19
#define CE_X	B20
#define CE_XD	B21
#define CE_XH	B22
#define CE_Y	B23
#define CE_YD	B24
#define CE_YH	B25
#define CE_H	B26
#define CE_I	B27

#define CE_ALL	0xfffffff0
#define CE_NC	(CE_R | CE_RD | CE_LD | CE_M | CE_X | CE_XD | CE_Y | CE_YD)
#define CE_CTRL	(CE_L | CE_R | CE_RD | CE_LD | CE_M | CE_X | CE_XD | CE_Y | CE_YD)

#define CE_PPC	B0
#define CE_PPV	B1
#define CE_PPCV	(CE_PPC | CE_PPV)
#define CE_TRAY	B2

struct MouseTypeListStruct {
	DWORD enables;
	const TCHAR type[6], *name;
} MouseTypeList[] ={
	{CE_PPCV | CE_ALL,	T("SPC"),  MES_MSSP},
	{CE_PPC | CE_ALL,	T("ENT"),  MES_MSEN},
	{CE_PPC | CE_ALL,	T("MARK"), MES_MSMA},
	{CE_PPC | CE_ALL,	T("TAIL"), MES_MSTA},
	{CE_PPC | CE_ALL,	T("PATH"), MES_MSPA},
	{CE_PPC | CE_TRAY | CE_ALL, T("ICON"), MES_MSIC},
	{CE_PPC | CE_CTRL,	T("TABB"), MES_MSTB},
	{CE_PPC | CE_CTRL,	T("TABS"), MES_MSTS},
	{CE_PPC | CE_CTRL,	T("HEAD"), MES_MSHA},

	{CE_PPCV | CE_CTRL,	T("MENU"), MES_MSME},
	{CE_PPCV | CE_ALL,	T("LINE"), MES_MSLI},
	{CE_PPC | CE_ALL,	T("INFO"), MES_MSIF},

	{CE_PPCV | CE_NC,	T("FRAM"), MES_MSFR},
	{CE_PPCV | CE_NC,	T("SYSM"), MES_MSSY},
	{CE_PPCV | CE_NC,	T("TITL"), MES_MSTI},
	{CE_PPCV | CE_NC,	T("MINI"), MES_MSMI},
	{CE_PPCV | CE_NC,	T("ZOOM"), MES_MSZO},
	{CE_PPCV | CE_NC,	T("CLOS"), MES_MSCL},
	{CE_PPCV | CE_NC,	T("SCRL"), MES_MSSC},
	{CE_PPCV | CE_ALL,	T("HMNU"), MES_MSHM},
	{0, T(""), NULL}
};

const TCHAR GALLOW[] = T("LUDR");

struct MenuMaskListStruct{
	const TCHAR *name;
	const TCHAR *mask;
} const MenuMaskList[] = {
	{MES_ALLI, NilStr},
	{MES_VNMG, T("!M_Ccr*,!M_menu*,!*_aux*")},
	{MES_VNMC, T("M_Ccr*")},
	{MES_VNMO, T("S_*")},
	{MES_VNMA, T("?_aux*")},
	{MES_VNMB, T("M_menu*")},
	{NULL, NULL}
};

// 機能・キー割当て一覧関連
#define GROUPCHAR '.'
TCHAR *KeyList = NULL;
const TCHAR *KeyListGroup;
HWND hOldKeyListDlg = NULL;
int OldKeyListID;
int SelectItemLV;

// ツールバー関連
void LoadBar(HWND hDlg);

struct {
	HBITMAP hBmp;			// ツールバーイメージ
	SIZE barsize;			// ツールバーイメージの大きさ
	int items;				// ボタンの種類数
	TCHAR filename[VFPS];	// 表示中ツールバーのファイル名
} BarBmpInfo;

#define ToolBarDefaultCommandCount 47+1
const TCHAR *ToolBarDefaultCommand[ToolBarDefaultCommandCount][2] = { // デフォルトツールバーのコマンド一覧
	{NilStr,		NilStr}, // ボタン画像なし
//00
	{T("@^LEFT"),	MES_VYPE},
	{T("@^RIGHT"),	MES_VYFW},
	{T("@0"),		MES_VYFV},
	{T("*customize M_pjump:%{&name=%1%}"),	MES_VYFA},
	{T("@\\T"),		MES_VYTR},

	{T("@^X"),		MES_VYCT},
	{T("@^C"),		MES_VYCP},
	{T("@^V"),		MES_VYPS},
	{T("*file undo"), MES_VYOU},
	{NilStr,		NilStr},
//10
	{T("@D"),		MES_VYDL},
	{T("@\\K"),		MES_VYNE},
	{T("@L"),		MES_VYLG},
	{T("@\\L"),		MES_VYLD},
	{T("@^W"),		MES_VYNE},

	{T("@&ENTER"),	MES_VYPP},
	{T("@F1"),		MES_HELP},
	{T("@^W"),		MES_VYWS},
	{T("@^F"),		MES_VYWS},
	{T("@^P"),		MES_7C08},
//20
	{T("@';'"),		MES_VYVS}, // アイコン表示
	{T("@';'"),		MES_VYVS}, // カタログ表示
	{T("@';'"),		MES_VYVS}, // 小さいアイコン
	{T("@';'"),		MES_VYVS}, // 詳細
	{T("*sortentry 0,-1,-1,B11111,1"),	MES_VYSN},
	{T("*sortentry 2,-1,-1,B11111,1"),	MES_VYSZ},
	{T("*sortentry 3,-1,-1,B11111,1"),	MES_VYSD},
	{T("@\\S"),		MES_VYSO},
	{T("@BS"),		MES_VYJP},
	{T("NETUSE"),	MES_VYNU},
//30
	{T("NETUNUSE"),	MES_VYFN},
	{T("@K"),		MES_VYND},
	{T("@';'"),		MES_VYVS}, // カタログ表示
	{NilStr,		T("\0")}, // 電話帳?
	{NilStr,		T("\0")}, // 電話帳?

	{NilStr,		T("\0")}, // 電話?
	{NilStr,		T("\0")}, // 電話?
	{T("@';'"),		MES_VYVS}, // 表示形式
	{T("@';'"),		MES_VYVS}, // カタログ表示
	{T("@';'"),		MES_VYVS}, // 小さいアイコン
//40
	{T("@';'"),		MES_VYVS}, // 詳細
	{T("@\\K"),		MES_VYNE},
	{T("@\\K"),		MES_VYNE}, // テンプレート?
	{T("@\\T"),		MES_VYFT},
	{T("@M"),		MES_VYFM},

	{T("@C"),		MES_VYFC},
	{T("CUSTOMIZE"), MES_VYCU},
};
// 隠しメニュー関連
COLORREF CharColor, BackColor; // 隠しメニューの色

const TCHAR StrTitleCommandBox[] = MES_7402;
const TCHAR StrMenuNew[] = MES_NEWI;
const TCHAR StrMenuNewMemo[] = MES_VTNM;
const TCHAR StrLabelTypeName[] = MES_VLTN;
const TCHAR StrLabelDetail[] = MES_VLDT;
const TCHAR StrLabelTargetKey[] = MES_VLTK;
const TCHAR StrLabelTargetKeyName[] = MES_VLTM;
const TCHAR StrLabelTargetButton[] = MES_VLTB;
const TCHAR StrLabelTargetButtonName[] = MES_VLBN;
const TCHAR StrLabelTargetArea[] = MES_VLTA;
const TCHAR StrLabelMenuItemName[] = MES_VLMI;
const TCHAR StrLabelItemName[] = MES_VLIN;

const TCHAR StrTitleMenuName[] = MES_VIMN;
const TCHAR StrQueryKeyMapMenu[] = MES_VQKM;
const TCHAR StrWarnBadAssc[] = MES_VQBA;
const TCHAR StrQueryNoAdd[] = MES_VQNA;
const TCHAR StrQueryDeleteTable[] = MES_VQDT;

HWND hCmdListWnd = NULL;

const TCHAR ArrowC[] = T("UDLR");
const TCHAR ArrowS[] = T("↑↓←→");

typedef struct {
	HWND hListViewWnd;
	int column;
	int order;
} LISTVIEWCOMPAREINFO;

const TCHAR StrOpenExe[] = MES_VTOE;
OPENFILENAME of_app = { sizeof(of_app), NULL, NULL,
	T("Executable File\0*.exe;*.com;*.bat;*.lnk;*.cmd\0")
	T("All Files\0*.*\0") T("\0"),
	NULL, 0, 0, NULL, VFPS, NULL, 0, NULL, NULL,
	OFN_HIDEREADONLY | OFN_SHAREAWARE, 0, 0, T("*"), 0, NULL,
	NULL OPENFILEEXTDEFINE
};

const TCHAR StrOpenBarImage[] = MES_VTBA;
const TCHAR StrOpenButtonImage[] = MES_VTBU;

OPENFILENAME of_bar = { sizeof(of_bar), NULL, NULL,
	GetFileExtsStr,
	NULL, 0, 0, NULL, VFPS, NULL, 0, NULL, NULL,
	OFN_HIDEREADONLY | OFN_SHAREAWARE, 0, 0, T("*"), 0, NULL,
	NULL OPENFILEEXTDEFINE
};

#define MENUID_NEW 0x7000
enum { InsertMacroExt_ChildMenu = 0x100, InsertMacroExt_EmbedMenu, InsertMacroExt_LAUNCH, InsertMacroExt_KEYCODE, InsertMacroExt_FILENAME, InsertMacroExt_HELP };

const WCHAR StrCommentInfo[] = BANNERTEXT(MES_COMT, L"コメント\0comment");

DLLDEFINED DWORD ButtonMenuTick DLLPARAM(0);

const TCHAR *GetURtext(const TCHAR *text)
{
	const TCHAR *newtext;

	if ( UseLcid != LCID_PPXDEF ){
		newtext = text + tstrlen(text) + 1;
		if ( *newtext != '\0' ) text = newtext;
	}
	return text;
}

int TrackButtonMenu(HWND hDlg, UINT ctrlID, HMENU hPopMenu)
{
	int index;

	if ( PPxCommonExtCommand(K_IsShowButtonMenu, ctrlID) != NO_ERROR ){
		return 0;
	}

	index = ButtonTrackPopupMenu(hDlg, hPopMenu, GetDlgItem(hDlg, ctrlID));
	if ( index <= 0 ) PPxCommonExtCommand(K_EndButtonMenu, 0);
	return index;
}


BOOL ShowMouseSetting(HWND hDlg, BOOL normal)
{
	int idc;

	ShowDlgWindow(hDlg, IDC_ALCMOUSET, normal);
	for ( idc = IDB_ALCMOUSEL ; idc <= IDB_ALCMOUSER ; idc++ ){
		ShowDlgWindow(hDlg, idc, !normal);
	}
	return normal;
}

TCHAR *MakeMouseDetailText(HWND hDlg, const TCHAR *text, TCHAR *dest)
{
	TCHAR *ptr, *top, gbuf[200];
	size_t size;
	const struct MouseButtonListStruct *blist;
	const TCHAR *button = NilStr, *area = NilStr;
	int count = 0;
							// 分割
	tstrcpy(dest, text);
	ptr = dest;
	while( !Isalpha(*ptr) ) ptr++;
	top = ptr;
	ptr = tstrchr(ptr, '_');
	if ( ptr != NULL ){
		*ptr++ = '\0';
		size = TSTROFF(ptr - top);
	}else{
		ptr = top;
		size = 0;
	}
							// ボタン名を取得
	blist = MouseButtonList;
	while( blist->type[0] != '\0' ){
		if ( !memcmp(blist->type, top, size) ){
			if ( hDlg != NULL ){
				SendDlgItemMessage(hDlg, IDC_ALCMOUSEB, CB_SETCURSEL, count, 0);
			}else{
				button = MessageText(blist->name);
			}
			break;
		}
		blist++;
		count++;
	}
	if ( hDlg != NULL ){
		if ( blist->type[0] == '\0' ){
			SendDlgItemMessage(hDlg, IDC_ALCMOUSEB, CB_SETCURSEL, 1, 0);
		}
		ShowMouseSetting(hDlg, count != GESTUREID);
	}
							// 対象を取得
	if ( count != GESTUREID ){
		const struct MouseTypeListStruct *mtl;

		if ( hDlg != NULL ){
			SendDlgItemMessage(hDlg, IDC_ALCMOUSET, CB_SETCURSEL, 0, 0);
		}
		size = TSTRLENGTH(ptr);
		mtl = MouseTypeList;
		count = 0;
		while( mtl->enables ){
			if ( !memcmp(mtl->type, ptr, size) ){
				if ( hDlg != NULL ){
					SendDlgItemMessage(hDlg, IDC_ALCMOUSET, CB_SETCURSEL, count, 0);
				}else{
					if ( *(ptr + 1) != '\0' ){
						area = MessageText(mtl->name);
					}
				}
				break;
			}
			count++;
			mtl++;
		}
	}else if ( hDlg == NULL ){ // ジェスチャー
		TCHAR *gdest;

		area = gbuf;
		gdest = gbuf;
		while ( *ptr ){
			const TCHAR *cp;

			cp = ArrowC;
			while( *cp ){
				if ( *cp == *ptr ){
					#ifdef UNICODE
						*gdest++ = ArrowS[cp - ArrowC];
					#else
						const TCHAR *dp;

						dp = &ArrowS[(cp - ArrowC) * sizeof(WCHAR)];
						*gdest++ = *dp++;
						*gdest++ = *dp;
					#endif
				}
				cp++;
			}
			ptr++;
		}
		*gdest = '\0';
	}
	tstrcpy(dest, area);
	if ( (button != NilStr) && !Isalpha(*text) ){
		TCHAR *dptr, *dbutton;

		dbutton = dptr = dest + tstrlen(dest) + 1;
		while ( !Isalpha(*text) ){
			switch ( *text++ ){
				case '&':
					dptr = tstpcpy(dptr, T("Alt+"));
					break;
				case '^':
					dptr = tstpcpy(dptr, T("Ctrl+"));
					break;
				case '\\':
					dptr = tstpcpy(dptr, T("Shift+"));
					break;
			}
		}
		tstrcpy(dptr, button);
		return dbutton;
	}
	return (TCHAR *)button;
}

BOOL MakeKeyDetailListText(TABLEINFO *tinfo, const TCHAR **text, TCHAR *dest)
{
	int key;

	key = GetKeyCode(text);
	if ( key < 0 ) return FALSE;
	if ( key & (K_internal | K_ex) ) return FALSE;

	MakeKeyDetailText(key, dest,
			(tinfo == NULL) || tstrcmp(tinfo->name_type, T("K_tray")) );

	return TRUE;
}

#if AddDefaultCommandList
// 既定の割当てを一覧表示に追加(&E)
void AddDefaultCmdList(HWND hLVAlcWnd)
{
	HWND hListViewWnd;
	TV_ITEM tvi;
	LV_ITEM lvi;
	TCHAR buf[CMDLINESIZE * 2], keydetail[64];
	const TCHAR *listp;

	hListViewWnd = hLVAlcWnd;
	SendMessage(hListViewWnd, WM_SETREDRAW, FALSE, 0);

	listp = KeyList;
	for ( ;; ){ // 列挙開始
		TCHAR *namestr, *keystr;

		if ( *listp == '\0' ) break;
		tvi.lParam = (LPARAM)listp;
		if ( *listp == GROUPCHAR ){
//		未実装
		}else{
			tstrcpy(buf, listp);
			namestr = tstrchr(buf, '\t');
			if ( namestr == NULL ) break;
			*namestr++ = '\0';
			keystr = tstrchr(namestr, '\t');
			if ( keystr == NULL ) break;
			*keystr++ = '\0';
			if ( UseLcid != LCID_PPXDEF ){
				namestr = MessageText(buf);
			}
			if ( *keystr == '@' ){ // キーなら登録
				keystr++;
				lvi.pszText = keystr;
				*keystr = '\0';
				MakeKeyDetailListText(NULL, (const TCHAR **)&keystr, keydetail);
				if ( *keystr == '\0' ){ // キーが１つ記載のみ
					lvi.mask = LVIF_TEXT | LVIF_PARAM; // キー割当て
					lvi.lParam = ListViewCounter++;
					lvi.iItem = INT32__MAX;
					lvi.iSubItem = 0;
					lvi.iItem = ListView_InsertItem(hListViewWnd, &lvi);

					lvi.mask = LVIF_TEXT;
					lvi.iSubItem = 1;		// キー説明
					lvi.pszText = keydetail;
					ListView_SetItem(hListViewWnd, &lvi);

					lvi.iSubItem = 2;		// 説明
					lvi.pszText = namestr;
					ListView_SetItem(hListViewWnd, &lvi);
				}
			}
		}
		listp += tstrlen(listp) + 1;

	}
	SendMessage(hListViewWnd, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE);
	SendMessage(hListViewWnd, LVM_SETCOLUMNWIDTH, 1, LVSCW_AUTOSIZE);
	SendMessage(hListViewWnd, LVM_SETCOLUMNWIDTH, 2, LVSCW_AUTOSIZE);
	SendMessage(hListViewWnd, WM_SETREDRAW, TRUE, 0);
}
#endif

void EnableSetButton(HWND hDlg, BOOL enable)
{
	EnableDlgWindow(hDlg, IDB_TB_SETITEM, enable);
}

void SetCommand(HWND hDlg, const TCHAR *str)
{
	TCHAR buf[CMDLINESIZE];

	str = tstrrchr(str, '\t');
	if ( str == NULL ) return;
	str++;

	if ( (*str == '*') || (*str == '%') ){
	}else if ( *str == '>' ){
		str++;
	}else{
		thprintf(buf, TSIZEOF(buf), T("%%K\"%s\""), str);
		str = buf;
	}
	SetDlgItemText(hDlg, IDE_ALCCMD, str);
	EnableSetButton(hDlg, TRUE);
}

void InitCommandTree(void)
{
	TV_ITEM tvi;
	TV_INSERTSTRUCT tvins;
	HTREEITEM hParentTree;
	TCHAR *listp;
	TCHAR buf[CMDLINESIZE];

	tvi.mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_PARAM;
	tvins.hInsertAfter = TVI_LAST;
	TreeInsertItemValue(tvins) = tvi;

	listp = KeyList;

	SendMessage(hCmdListWnd, WM_SETREDRAW, FALSE, 0);
	for ( ;; ){
		TCHAR *p;

		if ( *listp == '\0' ) break;
		tvi.lParam = (LPARAM)listp;
		if ( *listp == GROUPCHAR ){
			tstrcpy(buf, listp + 1);
			p = tstrchr(buf, '\t');
			if ( p == NULL ){
				p = (TCHAR *)MessageText(buf);
			}else{
				if ( UseLcid != LCID_PPXDEF ){
					*p = '\0';
					p = (TCHAR *)MessageText(buf);
				}else{
					p++;
				}
			}
			tvi.mask = TVIF_TEXT | TVIF_PARAM;
			tvi.pszText = p;
			tvi.cchTextMax = tstrlen32(tvi.pszText);
			tvi.cChildren = 1;
			tvins.hParent = TVI_ROOT;
			TreeInsertItemValue(tvins) = tvi;

			hParentTree = (HTREEITEM)SendMessage(hCmdListWnd, TVM_INSERTITEM,
								0, (LPARAM)(LPTV_INSERTSTRUCT)&tvins);
		}else{
			tstrcpy(buf, listp);
			p = tstrchr(buf, '\t');
			if ( p == NULL ) break;
			*p = '\0';
			if ( UseLcid != LCID_PPXDEF ){
				p = (TCHAR *)MessageText(buf);
			}else{
				TCHAR *p1;

				p++;
				p1 = tstrchr(p, '\t');
				if ( p1 != NULL ) *p1 = '\0';
			}
			tvi.mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_PARAM;
			tvi.pszText = p;
			tvi.cchTextMax = tstrlen32(p);
			tvi.cChildren = 0;
			tvins.hParent = hParentTree;
			TreeInsertItemValue(tvins) = tvi;

			SendMessage(hCmdListWnd, TVM_INSERTITEM,
					0, (LPARAM)(LPTV_INSERTSTRUCT)&tvins);
		}
		listp += tstrlen(listp) + 1;
	}
	SendMessage(hCmdListWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(hCmdListWnd, NULL, FALSE);
}

void USEFASTCALL InitCmdList(HWND hDlg)
{
	RECT box;

	if ( (hCmdListWnd != NULL) && IsWindow(hCmdListWnd) ){
		PostMessage(hCmdListWnd, WM_CLOSE, 0, 0);
		hCmdListWnd = NULL;
		return;
	}

	GetWindowRect(hDlg, &box);
	hCmdListWnd = CreateWindow(WC_TREEVIEW, MessageText(StrTitleCommandBox),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE |
		TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
		box.left - 8, box.top - 8,
		(box.right - box.left) / 3, ((box.bottom - box.top) * 2) / 3,
		hDlg, NULL, hInst, NULL);
	FixUxTheme(hCmdListWnd, WC_TREEVIEW);
	if ( X_dss & DSS_COMCTRL ) SendMessage(hCmdListWnd, CCM_DPISCALE, TRUE, 0);
	InitCommandTree();
}

void CommandTreeDlgBox_Init(HWND hDlg)
{
	GUILoadCust();
	LocalizeDialogText(hDlg, IDD_CMDTREE);
	KeyList = LoadTextResource(hInst, MAKEINTRESOURCE(DEFKEYLIST));
	if ( KeyList == NULL ) KeyList = T("");
	hCmdListWnd = GetDlgItem(hDlg, IDT_GENERAL);
	if ( X_dss & DSS_COMCTRL ) SendMessage(hCmdListWnd, CCM_DPISCALE, TRUE, 0);
	InitCommandTree();
	FixUxTheme(hCmdListWnd, WC_TREEVIEW);
}

LRESULT CmdTreeNotify(HWND hDlg, NMHDR *nhdr)
{
	if ( nhdr->code == TVN_SELCHANGED ){
		TV_ITEM tvi;

		tvi.hItem = ((NM_TREEVIEW *)nhdr)->itemNew.hItem;
		if ( tvi.hItem == NULL ){
			tvi.hItem = TreeView_GetSelection(hCmdListWnd);
			if ( tvi.hItem == NULL ) return 0;
		}
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hCmdListWnd, &tvi);
		if ( *(const TCHAR *)tvi.lParam != GROUPCHAR ){
			SetCommand(hDlg, (const TCHAR *)tvi.lParam/*, tinfo*/);
		}
	}else if ( nhdr->code == TVN_KEYDOWN ){
		if ( (((TV_KEYDOWN *)nhdr)->wVKey == VK_ESCAPE) && (GetParent(hCmdListWnd) == NULL) ){
			RemoveControlKeydown(hCmdListWnd);
			PostMessage(hCmdListWnd, WM_CLOSE, 0, 0);
		}
	}
	return 0;
}

void RunTreeCommand(HWND hDlg)
{
	HWND hPPcWnd = PPcGetWindow(0, CGETW_GETFOCUS);
	TCHAR param[CMDLINESIZE];
	COPYDATASTRUCT copydata;

	if ( hPPcWnd == NULL ) return;
	param[0] = '\0';
	GetDlgItemText(hDlg, IDE_ALCCMD, param, CMDLINESIZE);
	if ( param[0] == '\0' ) return;

	copydata.dwData = 'H';
	copydata.cbData = TSTRSIZE32(param);
	copydata.lpData = (PVOID)param;
	SendMessage(hPPcWnd, WM_COPYDATA, 0, (LPARAM)&copydata);
}

INT_PTR CALLBACK CommandTreeDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( iMsg ){
		case WM_INITDIALOG:
			CommandTreeDlgBox_Init(hDlg);
			break;

		case WM_NOTIFY:
			#define NHPTR ((NMHDR *)lParam)
			if ( NHPTR->hwndFrom == hCmdListWnd ){
				CmdTreeNotify(hDlg, NHPTR);
			}
			#undef NHPTR
			break;

		case WM_COMMAND:
			if ( LOWORD(wParam) == IDOK ){
				RunTreeCommand(hDlg);
			}else if ( LOWORD(wParam) == IDCANCEL ){
				PostMessage(hDlg, WM_CLOSE, 0, 0);
			}
			break;

		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}

int CALLBACK ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LISTVIEWCOMPAREINFO *cmpinfo)
{
	LV_FINDINFO lvfi;
	LV_ITEM lvi;
	TCHAR str1[CMDLINESIZE], str2[CMDLINESIZE];

	if ( lParam1 == 0 ) return -1;
	if ( lParam2 == 0 ) return 1;

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = lParam1;
	lvi.iItem = ListView_FindItem(cmpinfo->hListViewWnd, -1, &lvfi);
	lvi.mask = LVIF_TEXT;
	lvi.pszText = str1;
	lvi.cchTextMax = TSIZEOF(str1);
	lvi.iSubItem = cmpinfo->column;
	ListView_GetItem(cmpinfo->hListViewWnd, &lvi);

	lvfi.lParam = lParam2;
	lvi.iItem = ListView_FindItem(cmpinfo->hListViewWnd, -1, &lvfi);
	lvi.pszText = str2;
	ListView_GetItem(cmpinfo->hListViewWnd, &lvi);

	return (cmpinfo->order >= 0) ? tstrcmp(str1, str2) : tstrcmp(str2, str1);
}

/*-----------------------------------------------------------------------------
  実行ファイルを検索する
-----------------------------------------------------------------------------*/
enum {
	MENUID_REFNAME = 1, MENUID_DEFAULTLIST, MENUID_DEFAULTLIST_4BIT, MENUID_DEFAULTLIST_24BIT, MENUID_DEFAULTLIST_VECTOR, MENUID_DEFAULTLIST_4BIT_L, MENUID_DEFAULTLIST_24BIT_L, MENUID_DEFAULTLIST_VECTOR_L, MENUID_ADDBUTTONBMP, MENUID_DELBUTTONBMP
};
const TCHAR StrMenuBarRef[] = MES_VBRB;
const TCHAR StrMenuBarDefault[] = MES_VBDB;
const TCHAR StrMenuBarAddButton[] = MES_VBAB;
// const TCHAR StrMenuBarDelButton[] = T("&Delete button"); // ボタン画像削除(&D)\0

const TCHAR StrLoadDefToolBar[] = MES_VRTB;
const TCHAR StrUnsupportDisp[] = MES_VUSD;

void SaveToolBarListBitmapPath(HWND hDlg, const TCHAR *path)
{
	TCHAR type[VFPS], buf[VFPS + 8];

	GetControlText(hDlg, IDE_EXTYPE, type, TSIZEOF(type));

	if ( path[0] == '\0' ){
		DeleteCustTable(type, T("@"), 0);
	}else{
		*(DWORD *)buf = 0;
		*(TCHAR *)((BYTE *)buf + 4) = EXTCMD_CMD;
		tstrcpy( (TCHAR *)((BYTE *)buf + 4) + 1, path );

		SetCustTable(type, T("@"), buf, TSTRSIZE( (TCHAR *)((BYTE *)buf + 4) ) + 4);
	}
	LoadBar(hDlg);
	Changed(hDlg);
}

BOOL SaveToolBarBitmap(HWND hWnd, const TCHAR *path, HBITMAP hBmp)
{
	int memsize, infosize;
	BYTE *memdata;
	BITMAPFILEHEADER *bfh;
	BITMAPINFO *bi;
	HANDLE hFile;
	BOOL result;
	BITMAP bmpi;
	int height, y, linebytes;

	if ( (GetObject(hBmp, sizeof(bmpi), &bmpi) == 0) || (bmpi.bmBits == NULL) ){
		return FALSE;
	}
	height = (bmpi.bmHeight >= 0) ? bmpi.bmHeight : -bmpi.bmHeight;
	linebytes = DwordAlignment(bmpi.bmWidthBytes);

	infosize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	if ( bmpi.bmBitsPixel <= 8 ){
		XMessage(hWnd, StrCustTitle, XM_ImWRNld, StrUnsupportDisp);
		return FALSE;
		// infosize += sizeof(RGBQUAD) * (1 << bmpi.bmBitsPixel);
	}
	memsize = infosize + linebytes * height;
	memdata = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, infosize);
	bfh = (BITMAPFILEHEADER *)memdata;
	bfh->bfType = 'B' + ('M' << 8);
	bfh->bfSize = memsize;
	bfh->bfOffBits = infosize;

	bi = (BITMAPINFO *)(BYTE *)(memdata + sizeof(BITMAPFILEHEADER));
	bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi->bmiHeader.biWidth = bmpi.bmWidth;
	bi->bmiHeader.biHeight = bmpi.bmHeight;
	bi->bmiHeader.biPlanes = 1;
	bi->bmiHeader.biBitCount = bmpi.bmBitsPixel;
	bi->bmiHeader.biCompression = BI_RGB;
/*
	if ( bmpi.bmBitsPixel <= 8 ){
		GetDIBColorTable(hBmp, 0,	(1 << bmpi.bmBitsPixel), bi->bmiColors);
	}
*/
	hFile = CreateFileL(path, GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS,
			FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile != INVALID_HANDLE_VALUE ){
		DWORD wsize;

		result = WriteFile(hFile, memdata, infosize, &wsize, NULL);
		if ( linebytes == bmpi.bmWidthBytes ){
			result = WriteFile(hFile, bmpi.bmBits, bmpi.bmWidthBytes * height, &wsize, NULL);
		}else{ // bmpi.bmWidthBytes は WORD境界
			char *bp;
			WORD zero = 0;

			bp = (char *)bmpi.bmBits;
			for ( y = 0; y < height; y++, bp += bmpi.bmWidthBytes){
				result = WriteFile(hFile, bp, bmpi.bmWidthBytes, &wsize, NULL);
				if ( result == FALSE ) break;
				WriteFile(hFile, &zero, sizeof(zero), &wsize, NULL);
			}
		}
		CloseHandle(hFile);
		SaveToolBarListBitmapPath(hWnd, path);
	}else{
		result = FALSE;
	}
	HeapFree(GetProcessHeap(), 0, memdata);
	return result;
}

BOOL InitEditToolBarBitmap(HWND hWnd)
{
	PPXDBINFOSTRUCT dbinfo;
	TCHAR path[VFPS], buf[VFPS];

	if ( BarBmpInfo.filename[0] != '\0' ){
		TCHAR *ext = BarBmpInfo.filename + FindExtSeparator(BarBmpInfo.filename);
		if ( tstricmp(ext, T(".bmp")) == 0 ) return TRUE; // 加工可能
		thprintf(path, TSIZEOF(path), T("%s.bmp"), BarBmpInfo.filename);
	}else{
		dbinfo.structsize = sizeof dbinfo;
		dbinfo.custpath = buf;
		GetPPxDBinfo(&dbinfo);
		thprintf(path, TSIZEOF(path), T("..\\toolbar%d.bmp"), BarBmpInfo.barsize.cy);
		VFSFullPath(NULL, path, buf);
	}

	//加工できないのでコピー用意

	if ( GetFileAttributesL(path) != MAX32 ){
		SaveToolBarListBitmapPath(hWnd, path);
		XMessage(hWnd, StrCustTitle, XM_ImWRNld, StrLoadDefToolBar);
		return FALSE;
	}else{
		return SaveToolBarBitmap(hWnd, path, BarBmpInfo.hBmp);
	}
}

void AddToolBarButton(HWND hWnd, const TCHAR *path)
{
	HTBMP hTbmp;
	HBITMAP hDestBMP;
	HGDIOBJ hOldSrcBMP, hOldDstBMP;
	HDC hDC, hDstDC, hSrcDC;
	LPVOID lpBits;
	TCHAR buf[VFPS];
	BITMAPINFO bmpinfo;

	if ( LoadBMP(&hTbmp, path, BMPFIX_TOOLBAR) == FALSE ){
		XMessage(hWnd, path, XM_ImWRNld, MES_EOFI);
		return;
	}

	hDC = GetDC(hWnd);
	hDstDC = CreateCompatibleDC(hDC);
	hSrcDC = CreateCompatibleDC(hDC);
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biWidth = BarBmpInfo.barsize.cx + BarBmpInfo.barsize.cy;
	bmpinfo.bmiHeader.biHeight = BarBmpInfo.barsize.cy;
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = 32;
	bmpinfo.bmiHeader.biCompression = BI_RGB;
	bmpinfo.bmiHeader.biSizeImage = 0;
	bmpinfo.bmiHeader.biClrUsed = 0;
	bmpinfo.bmiHeader.biClrImportant = 0;
	hDestBMP = CreateDIBSection(hDC, &bmpinfo, DIB_RGB_COLORS, &lpBits, NULL, 0);
	if ( hDestBMP != NULL ){
		// 元画像を複写
		hOldSrcBMP = SelectObject(hSrcDC, BarBmpInfo.hBmp);
		hOldDstBMP = SelectObject(hDstDC, hDestBMP);
		BitBlt(hDstDC, 0, 0,
				BarBmpInfo.barsize.cx, BarBmpInfo.barsize.cy,
				hSrcDC, 0, 0, SRCCOPY);
		SelectObject(hSrcDC, hOldSrcBMP);
		DeleteDC(hSrcDC);

		// 追加画像を複写
		DrawBMP(hDstDC, &hTbmp, BarBmpInfo.barsize.cx, 0);
		SelectObject(hDstDC, hOldDstBMP);
		DeleteDC(hDstDC);
		FreeBMP(&hTbmp);

		// 保存
		tstrcpy(buf, BarBmpInfo.filename);
		BarBmpInfo.filename[0] = '\0';
		SaveToolBarBitmap(hWnd, buf, hDestBMP);
		tstrcpy(BarBmpInfo.filename, buf);
		DeleteObject(hDestBMP);
	}
	ReleaseDC(hWnd, hDC);
}

void ToolBarMenu(HWND hDlg)
{
	TCHAR buf[VFPS];
	HMENU hPopMenu = CreatePopupMenu();
	int index;

	thprintf(buf, TSIZEOF(buf), MessageText(StrMenuBarRef),
			(BarBmpInfo.filename[0] == '\0') ?
				T("Default") : BarBmpInfo.filename);
	AppendMenu(hPopMenu, MF_ES, MENUID_REFNAME, buf);
	if ( BarBmpInfo.filename[0] != '\0' ){
		AppendMenuString(hPopMenu, MENUID_DEFAULTLIST, StrMenuBarDefault);
	}
	AppendMenuString(hPopMenu, MENUID_DEFAULTLIST_4BIT, T("4bit color"));
	if ( (OSver.dwMajorVersion <= 10) && (OSver.dwBuildNumber < WINBUILD_11_21H1) ){
		AppendMenuString(hPopMenu, MENUID_DEFAULTLIST_24BIT, T("24bit color"));
		AppendMenuString(hPopMenu, MENUID_DEFAULTLIST_24BIT_L, T("24bit color (L)"));
	}
	if ( OSver.dwMajorVersion >= 10 ){
		AppendMenuString(hPopMenu, MENUID_DEFAULTLIST_VECTOR, T("monochrome Vector"));
		AppendMenuString(hPopMenu, MENUID_DEFAULTLIST_VECTOR_L, T("monochrome Vector (L)"));
	}
/*
	AppendMenu(hPopMenu, MF_ES, MENUID_REFNAME, "16");
	AppendMenu(hPopMenu, MF_ES, MENUID_REFNAME, "24");
	AppendMenu(hPopMenu, MF_ES, MENUID_DELBUTTONBMP, StrMenuBarDelButton);
*/
	AppendMenu(hPopMenu, MF_ES, MENUID_ADDBUTTONBMP, MessageText(StrMenuBarAddButton));

	index = TrackButtonMenu(hDlg, IDB_BREF, hPopMenu);
	DestroyMenu(hPopMenu);

	switch ( index ){
		case MENUID_REFNAME:
			tstrcpy(buf, BarBmpInfo.filename);
			of_bar.hwndOwner = hDlg;
			of_bar.hInstance = hInst;
			of_bar.lpstrFile = buf;
			of_bar.lpstrTitle = MessageText(StrOpenBarImage);
			if ( GetOpenFileName(&of_bar) ) SaveToolBarListBitmapPath(hDlg, buf);
			break;

		case MENUID_DEFAULTLIST:
			SaveToolBarListBitmapPath(hDlg, NilStr);
			break;

		case MENUID_DEFAULTLIST_4BIT:
		case MENUID_DEFAULTLIST_24BIT:
		case MENUID_DEFAULTLIST_VECTOR:
			thprintf(buf, TSIZEOF(buf), T("<#%d>"), index - MENUID_DEFAULTLIST_4BIT + 1);
			SaveToolBarListBitmapPath(hDlg, buf);
			break;

		case MENUID_DEFAULTLIST_24BIT_L:
		case MENUID_DEFAULTLIST_VECTOR_L:
			thprintf(buf, TSIZEOF(buf), T("<z24#%d>"), index - MENUID_DEFAULTLIST_4BIT_L + 1);
			SaveToolBarListBitmapPath(hDlg, buf);
			break;

		case MENUID_ADDBUTTONBMP:
			if ( InitEditToolBarBitmap(hDlg) == FALSE ) break;

			buf[0] = '\0';
			of_bar.hwndOwner = hDlg;
			of_bar.hInstance = hInst;
			of_bar.lpstrFile = buf;
			of_bar.lpstrTitle = MessageText(StrOpenButtonImage);
			if ( GetOpenFileName(&of_bar) ) AddToolBarButton(hDlg, buf);
			break;
	}
}

//------------------------------------------------ 種類名が正しいか
BOOL CheckTypeName(TCHAR key, const TCHAR *name)
{
	if ( key == 'm' ){
		if (!tstrcmp(name, T("MC_click")) ||
			!tstrcmp(name, T("MV_click")) ||
			!tstrcmp(name, T("MT_icon")) ){
			return TRUE;
		}
		return FALSE;
	}
	if ( key == 'B' ){
		if (!tstrcmp(name, T("HM_ppc")) ||
			!tstrcmp(name, T("HM_ppv")) ){
			return TRUE;
		}
	}
	if ( key == 'K' ){
		if (!tstrcmp(name, T("K_edit")) ||
			!tstrcmp(name, T("K_tray")) ||
			!tstrcmp(name, T("K_tree")) ||
			!tstrcmp(name, T("K_list")) ){
			return TRUE;
		}
	}
	if ( (key == 'M') && (name[0] == 'S') && (name[1] == '_') ) return TRUE;
	if ( name[0] != key ) return FALSE;
	if ( name[1] != '_' ){
		if ( !(Isalpha(name[1]) && (name[2] == '_')) ) return FALSE;
		if ( key == 'M' ){
			if (!tstrcmp(name, T("MC_celS")) ||
				!tstrcmp(name, T("MC_click")) ||
				!tstrcmp(name, T("MV_click")) ||
				!tstrcmp(name, T("MT_icon")) ){
				return FALSE;
			}
		}
	}
	return TRUE;
}

void InsertAliasMacro(HWND hDlg, TCHAR *buf)
{
	DWORD index, X_mwid = 60, id = 1;
	int size;
	HMENU hMenu;
	TCHAR value[CMDLINESIZE], key[CUST_NAME_LENGTH + CMDLINESIZE + 8], *envptr;
	const TCHAR *ep;

	GetCustData(T("X_mwid"), &X_mwid, sizeof(X_mwid));
	if ( X_mwid > (CMDLINESIZE - 10) ) X_mwid = CMDLINESIZE - 10;
	hMenu = CreatePopupMenu();

	// エイリアス一覧
	for( index = 0 ; ; index++ ){
		size = EnumCustTable(index, T("A_exec"), key, value, sizeof(value));
		if ( 0 > size ) break;
		tstrcpy(value + X_mwid, T("..."));
		tstrcat(key, T("\t"));
		tstrcat(key, value);
		AppendMenu(hMenu, MF_ES, id++, key);
	}
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	// 環境変数一覧
	ep = envptr = GetEnvironmentStrings();
	if ( ep != NULL ){
		while ( *ep != '\0' ){
			TCHAR *p;

			if ( *ep != '=' ){
				tstplimcpy(key, ep, X_mwid);
				tstrcpy(key + X_mwid, T("..."));
				p = tstrchr(key, '=');
				if ( p != NULL ){
					*p = '\t';
					AppendMenu(hMenu, MF_ES, id++, key);
				}
			}
			ep += tstrlen(ep) + 1;
		}
		FreeEnvironmentStrings(envptr);
	}

	id = TrackButtonMenu(hDlg, IDB_ALCCMDI, hMenu);
	if ( id > 0 ){
		TCHAR *p;

		GetMenuString(hMenu, id, buf, CMDLINESIZE, MF_BYCOMMAND);
		p = tstrchr(buf, '\t');
		if ( p != NULL ){
			*p = '\'';
			*(p + 1) = '\0';
		}
	}
	DestroyMenu(hMenu);
}

void InsertMenuMacro(HWND hDlg, TCHAR type, int menumode)
{
	int count, size, id = 1, extraid = id;
	TCHAR name[CUST_NAME_LENGTH], comment[VFPS], buf[CMDLINESIZE];
	HMENU hMenu;

	hMenu = CreatePopupMenu();
	AppendMenuString(hMenu, MENUID_NEW, StrMenuNew);
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

	if ( type == 'M' ){
		HMENU hSubMenu = CreatePopupMenu();
		const TCHAR **menustr = InsertExtraMenuString;
		const TCHAR **menutext = InsertExtraMenuStringText;

		for (  ; *menustr != NULL ; menustr++, menutext++ ){
			thprintf(buf, TSIZEOF(buf), T("%s\t%s"), *menustr, GetURtext(*menutext));
			AppendMenu(hSubMenu, MF_ES, id++, buf);
		}
		extraid = id;
		AppendMenu(hMenu, MF_EPOP, (UINT_PTR)hSubMenu, GetURtext(T("特殊なメニュー\0extra menu")));
	}

											// 使用済みを検索
	for( count = 0 ; ; count++ ){
		size = EnumCustData(count, name, NULL, 0);
		if ( 0 > size ) break;

		if ( CheckTypeName(type, name) == FALSE ) continue;
		comment[0] = '\0';
		GetCustTable(T("#Comment"), name, comment, sizeof(comment));
		if ( comment[0] != '\0' ){
			const TCHAR *ptr;

			ptr = comment;
			SkipSpace(&ptr);
			thprintf(buf, TSIZEOF(buf), T("%s\t%s"), name, ptr);
		}else{
			tstrcpy(buf, name);
		}
		AppendMenu(hMenu, MF_ES, id++, buf);
	}

	id = TrackButtonMenu(hDlg, IDB_ALCCMDI, hMenu);
	if ( id != 0 ){
		TCHAR *tabptr = NULL;

		if ( id == MENUID_NEW ){
			tstrcpy(buf, (type == 'E') ? T("E_newlist") : T("M_newmenu"));
			if ( tInput(hDlg, StrTitleMenuName, buf,
						VFPS, PPXH_GENERAL, PPXH_GENERAL) > 0 ){
				if ( (buf[0] == type) && (buf[1] == '_') ){
/*
					SetCustTable(buf, T("item"), NilStr, sizeof(TCHAR));
					LV_ITEM lvi;
					lvi.mask = LVIF_TEXT;
					lvi.iItem = INT32__MAX;
					lvi.iSubItem = 0;
					lvi.pszText = buf;
					ListView_InsertItem(GetDlgItem(hDlg, IDV_ALCLIST), &lvi);
					Changed(hDlg);
*/
				}else{
					buf[0] = '\0';
				}
			}else{
				buf[0] = '\0';
			}
		}else{
			if ( (id < extraid) && (menumode == 0) ){
				buf[0] = 'M';
				GetMenuString(hMenu, id, buf + 1, CMDLINESIZE - 1, MF_BYCOMMAND);
			}else{
				GetMenuString(hMenu, id, buf, CMDLINESIZE, MF_BYCOMMAND);
			}
			tabptr = tstrchr(buf, '\t');
			if ( tabptr != NULL ) *tabptr = '\0';
		}
		if ( buf[0] != '\0' ){
			if ( menumode == 0 ){ // (M) %M か (E) %ME の挿入
				thprintf(comment, TSIZEOF(comment),
						(type == 'E') ? T(" %%M%s") : T(" %%%s"), buf);
				SendDlgItemMessage(hDlg, IDE_ALCCMD, EM_REPLACESEL, 0, (LPARAM)comment);
			}else{ // 1 %M 2 ??M の設定
				TCHAR *fmt;

				if ( (menumode == 1) && (tabptr != NULL) ){
					SetDlgItemText(hDlg, IDE_EXITEM, tabptr + 1);
				}

				if ( id < extraid ){
					fmt = (menumode == 1) ? T("%s") : T("?%s");
				}else{
					fmt = (menumode == 1) ? T("%%%s") : T("??%s");
				}
				thprintf(comment, TSIZEOF(comment), fmt, buf);
				SetDlgItemText(hDlg, IDE_ALCCMD, comment);
				EnableSetButton(hDlg, TRUE);
			}
		}
	}
	DestroyMenu(hMenu);
}

void SetKeyComment(HWND hDlg, TABLEINFO *tinfo, TCHAR *label)
{
	TCHAR *p, buf[MAX_PATH];

	p = label;
	if ( MakeKeyDetailListText(tinfo, (const TCHAR **)&p, buf) ){
		p = buf;
	}else{
		p = label;
	}
	SetDlgItemText(hDlg, IDS_EXITEM, p);
}

//-------------------------------------------------------- キー入力
void ChooseKey(HWND hDlg, int id, TABLEINFO *tinfo)
{
	TCHAR temp[64];

	temp[0] = '\0';
	if ( KeyInput(GetParent(hDlg), temp) > 0 ){
		HWND hEdWnd = GetDlgItem(hDlg, id);
		if ( tinfo != NULL ){
			SetWindowText(hEdWnd, temp);
			SetKeyComment(hDlg, tinfo, temp);
		}else{
			SendMessage(hEdWnd, EM_REPLACESEL, 0, (LPARAM)temp);
		}
	}
	return;
}

void InsertMacroString(HWND hDlg, TABLEINFO *tinfo)
{
	HMENU hMenu;
	const TCHAR **menustr = InsertMacroMenuString;
	int id = 1;
	TCHAR buf[VFPS], *p;

	hMenu = CreatePopupMenu();

	if ( tinfo->key == 'M' ){
		AppendMenuString(hMenu, InsertMacroExt_ChildMenu, GetURtext(T("下層メニューの設定\0Child Menu")));
		AppendMenuString(hMenu, InsertMacroExt_EmbedMenu, GetURtext(T("メニューの埋め込み\0Embed Menu")));
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	}

	for (  ; *menustr != NULL ; menustr++ ){
		AppendMenu(hMenu, MF_ES, id++, GetURtext(*menustr));
	}
	AppendMenuString(hMenu, InsertMacroExt_LAUNCH, InsertMacroExt_LAUNCHstr);
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenuString(hMenu, InsertMacroExt_KEYCODE, InsertMacroExt_KEYCODEstr);
	AppendMenuString(hMenu, InsertMacroExt_FILENAME, InsertMacroExt_FILENAMEstr);
	AppendMenuString(hMenu, InsertMacroExt_HELP, MES_HELP);

	id = TrackButtonMenu(hDlg, IDB_ALCCMDI, hMenu);
	DestroyMenu(hMenu);
	switch (id){
		case InsertMacroExt_ChildMenu:
			InsertMenuMacro(hDlg, 'M', 1);
			return;

		case InsertMacroExt_EmbedMenu:
			InsertMenuMacro(hDlg, 'M', 2);
			return;

		case InsertMacroExt_LAUNCH:
			tstrcpy(buf, InsertMacroExt_LAUNCHstr);
			break;

		case InsertMacroExt_KEYCODE:
			ChooseKey(hDlg, IDE_ALCCMD, NULL);
			return;

		case InsertMacroExt_FILENAME:
			buf[0] = '\0';
			of_app.hwndOwner = hDlg;
			of_app.hInstance = hInst;
			of_app.lpstrFile = buf;
			of_app.lpstrTitle = MessageText(StrOpenExe);
			if ( GetOpenFileName(&of_app) == FALSE ) return;
			break;

		case InsertMacroExt_HELP:
			PPxHelp(hDlg, HELP_KEY, (DWORD_PTR)T("macro"));
			return;

		default:
			if ( id <= 0 ) return;
			tstrcpy(buf, InsertMacroMenuString[id - 1]);
			if ( buf[2] == '\'' ){ // %'
				InsertAliasMacro(hDlg, buf + 3);
			}else if ( buf[2] == 'M' ){ // %M / %ME
				InsertMenuMacro(hDlg, (TCHAR)((buf[3] == 'E') ? 'E' : 'M'), 0);
				return;
			}
			break;
	}
	p = tstrchr(buf, '\t');
	if ( p != NULL ) *p = '\0';
	SendDlgItemMessage(hDlg, IDE_ALCCMD, EM_REPLACESEL, 0, (LPARAM)buf);
}

void InsertMenuSeparator(HWND hDlg, BOOL toolbar)
{
	HMENU hMenu;
	const TCHAR **menustr = InsertMenuSeparatorString;
	int id = 1;

	hMenu = CreatePopupMenu();
	AppendMenuString(hMenu, id, *menustr);
	if ( !toolbar ){
		AppendMenuString(hMenu, ++id, *++menustr);
		AppendMenuString(hMenu, ++id, *++menustr);
		AppendMenuString(hMenu, ++id, *++menustr);
	}

	id = TrackButtonMenu(hDlg, IDB_MECKEYS, hMenu);
	DestroyMenu(hMenu);
	if ( id ){
		TCHAR buf[MAX_PATH];

		tstrcpy(buf, MessageText(InsertMenuSeparatorString[id - 1]));
		*tstrchr(buf, '\t') = '\0';
		if ( buf[0] == '&' ) buf[1] = '\0'; // & の場合は、&& 表記なので戻す
		if ( id <= 2 ){
			SetDlgItemText(hDlg, IDE_EXITEM, buf);
			SetDlgItemText(hDlg, IDE_ALCCMD, NilStr);
			SendMessage(hDlg, WM_COMMAND, IDB_ALCNEW, 0);
		}else{
			SendDlgItemMessage(hDlg, IDE_EXITEM, EM_REPLACESEL, 0, (LPARAM)buf);
		}
	}
}

BOOL KeymapMenu(HWND hWnd)
{
	if ( PMessageBox(hWnd, StrQueryKeyMapMenu, StrCustTitle,
			MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES ){
		return FALSE;
	}
	PPxCommonExtCommand(K_menukeycust, 0);
	return TRUE;
}

void SetKeyList(HWND hDlg, const TCHAR *listfirst)
{
	TCHAR buf[CMDLINESIZE];
	HWND hListWnd;

	hOldKeyListDlg = hDlg;
	hListWnd = GetDlgItem(hDlg, IDC_ALCKEYS);
	SendMessage(hListWnd, WM_SETREDRAW, FALSE, 0);
	SendMessage(hListWnd, CB_RESETCONTENT, 0, 0);
	for ( ;; ){
		TCHAR *p;

		if ( (*listfirst == '\0') || (*listfirst == GROUPCHAR) ) break;
		tstrcpy(buf, listfirst);
		p = tstrchr(buf, '\t');
		if ( p == NULL ) break;
		*p = '\0';
		if ( UseLcid != LCID_PPXDEF ){
			p = (TCHAR *)MessageText(buf);
		}else{
			TCHAR *p1;

			p++;
			p1 = tstrchr(p, '\t');
			if ( p1 != NULL ) *p1 = '\0';
		}
		SendMessage(hListWnd, CB_ADDSTRING, 0, (LPARAM)p);
		listfirst += tstrlen(listfirst) + 1;
	}
	SendMessage(hListWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(hListWnd, NULL, FALSE);
}

const TCHAR *GetKeyGroup(int id)
{
	const TCHAR *p;
	int i = 0;
							// 該当グループを検索
	p = KeyList;
	for ( ;; ){
		TCHAR c;

		c = *p;
		p += tstrlen(p) + 1;
		if ( c == GROUPCHAR ){
			if ( i == id ){
				KeyListGroup = p;
				return p;
			}
			i++;
		}
		if ( *p == '\0' ) return NULL; // 該当無し
	}
}

void SelectedKeySubID(HWND hDlg, int id/*, TABLEINFO *tinfo*/)
{
	const TCHAR *p = KeyListGroup;

	if ( hOldKeyListDlg != hDlg ){
		int i;

		i = (int)SendDlgItemMessage(hDlg, IDC_ALCKEYG, CB_GETCURSEL, 0, 0);
		if ( i == CB_ERR ) return;
		if ( GetKeyGroup(i) == NULL ) return;
		hOldKeyListDlg = hDlg;
		OldKeyListID = id;
	}
	{
		int i = 0;

		for ( ;; ){	// 該当IDを検索
			if ( (*p == '\0') || (*p == GROUPCHAR) ) return;
			if ( i == id ) break;
			i++;
			p += tstrlen(p) + 1;
		}
	}
	SetCommand(hDlg, p);
}

void SelectedKeyGroup(HWND hDlg, int id)
{
	const TCHAR *p;

	if ( (hOldKeyListDlg == hDlg) && (OldKeyListID == id) ) return; // 変更不要
	p = GetKeyGroup(id);
	if ( p == NULL ) return;
	OldKeyListID = id;
	SetKeyList(hDlg, p);
	SendDlgItemMessage(hDlg, IDC_ALCKEYS, CB_SETCURSEL, (WPARAM)-1, 0);
}

//------------------------------------------------ 項目を選択したときの表示処理
void SelectItemByIndex(TABLEINFO *tinfo, int index)
{
	LV_ITEM lvi;
	HWND hLVWnd;

	hLVWnd = tinfo->hLVAlcWnd;
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
	lvi.state = 0;
	SendMessage(hLVWnd, LVM_SETITEMSTATE, 0, (WPARAM)&lvi);
	lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
	SendMessage(hLVWnd, LVM_SETITEMSTATE, index, (WPARAM)&lvi);
	SendMessage(hLVWnd, LVM_ENSUREVISIBLE, index, FALSE);
}

void SelectedItemMenu(HWND hDlg, TABLEINFO *tinfo, const TCHAR *keyword)
{
	TCHAR label[CUST_NAME_LENGTH];
	TCHAR para[LONG_CMDSIZE];

	para[0] = '\0';
	label[0] = '\0';
	if ( keyword == NULL ){
		if ( EnumCustTable(tinfo->index_alc, tinfo->name_type, label, para, sizeof(para)) < 0 ) return;
	}else{
		tinfo->index_alc = 0;
		tstrcpy(label, keyword);
		GetCustTable(tinfo->name_type, label, para, sizeof(para));
	}
	para[TSIZEOF(para) - 1] = '\0';
	SetDlgItemText(hDlg, IDE_EXITEM, label);
	tstrreplace(para, T("\n"), T("\r\n"));
	SetDlgItemText(hDlg, IDE_ALCCMD, para);
	SendDlgItemMessage(hDlg, IDE_ALCCMD, EM_SETSEL, EC_LAST, EC_LAST);
}

void SetCommandNameList(HWND hDlg, TABLEINFO *tinfo, const TCHAR *keyname, TCHAR *text)
{
	const TCHAR *listfirst = NULL;
	int id = 0, subid = 0;
	TCHAR type = '\0';

	// ファイル判別→すべてPPc
	// キー割当て→２文字目 + K_tray, K_edit
	if ( tinfo->name_type[0] == 'E' ){	// E_
		type = 'c';
	}else if ( tinfo->name_type[0] == 'H' ){	// HM_ppc/HM_ppv
		type = TinyCharLower(tinfo->name_type[5]);
	}else if ( Isalpha(tinfo->name_type[1]) ){	// ?B_ , ?C_ , ?V_
		type = TinyCharLower(tinfo->name_type[1]);
	}else if ( tinfo->name_type[2] == 'e' ){	// K_edit
		type = 'e';
	}
	if ( keyname != NULL ){
		const TCHAR *p;
		BOOL skip C4701CHECK;

		p = KeyList;
		for ( ;; ){
			if ( *p == GROUPCHAR ){
				// ※必ず始めに実行される
				listfirst = p;
				id++;
				subid = 0;

				if ( !type ){ // 対象が不明
					skip = FALSE;
				}else if ( !Isalpha(*(p + 1)) ){ // 英字でない→共用？
					skip = FALSE;
				}else{	// 英字→各PPx固有
					skip = *(p + 3) != type;
				}
			}else if ( skip == FALSE ){ // C4701ok
				TCHAR *q;

				q = tstrchr(p, '\t'); // Jpn/Eng 間
				if ( q != NULL ){
					while ( *q == '\t' ) q++;
					q = tstrchr(q, '\t'); // Eng/Keyname 間
					if ( q != NULL ){
						while ( *q == '\t' ) q++;
						if ( tstrcmp(q, keyname) == 0 ){
							if ( text != NULL ){
								if ( UseLcid == LCID_PPXDEF ){
									TCHAR *np = tstrchr(p, '\t');
									if ( np != NULL ) p = np + 1;
								}
								while ( (*p != '\t') && (*p != '\0') ){
									*text++ = *p++;
								}
								*text = '\0';
								return;
							}
							id--;
							break;
						}
						subid++;
					}
				}
			}
			p += tstrlen(p) + 1;
			if ( *p == '\0' ){
				keyname = NULL;
				break;
			}
		}
	}
	if ( text != NULL ) return;

	if ( keyname == NULL ){
		id = -1;
		subid = -1;
		listfirst = KeyList;
	}
	listfirst += tstrlen(listfirst) + 1;	// グループ名行をスキップ

	if ( (hOldKeyListDlg != hDlg) || (OldKeyListID != id) ){
		KeyListGroup = listfirst;
		OldKeyListID = id;
		SetKeyList(hDlg, listfirst);
	}
	SendDlgItemMessage(hDlg, IDC_ALCKEYG, CB_SETCURSEL, id, 0);
	SendDlgItemMessage(hDlg, IDC_ALCKEYS, CB_SETCURSEL, subid, 0);
}

#define BARDATASIZE 32
void SelectedItemExecute(HWND hDlg, TABLEINFO *tinfo, const TCHAR *keyword)
{
	TCHAR label[MAX_PATH];
	TCHAR para[LONG_CMDSIZE + BARDATASIZE], *parap;
	TCHAR buf[CMDLINESIZE];
	const TCHAR *keyname = NULL;

	if ( keyword == NULL ){
		int sel;

		sel = TListView_GetFocusedItem(tinfo->hLVAlcWnd);
		if ( sel >= 0 ){
			ListView__GetText(tinfo->hLVAlcWnd, sel, label, TSIZEOF(label));
		}else{
			label[0] = '\0';
		}
	}else{
		tinfo->index_alc = 0;
		tstrcpy(label, keyword);
	}
	if ( (tinfo->key == 'E') && (label[0] == '/') ){
		 					// ファイル判別→ワイルドカードの調整
		SetDlgItemText(hDlg, IDE_EXITEM, label + 1);
	}else{
		SetDlgItemText(hDlg, IDE_EXITEM, label);
	}
	if ( tinfo->key == 'm' ){ // マウスの時は内容を解析
		MakeMouseDetailText(hDlg, label, buf);
	}else if ( tinfo->key == 'K' ){ // キー割当ての時はキーの説明
		SetKeyComment(hDlg, tinfo, label);
	}

	if ( tinfo->key == 'B' ){
		parap = (TCHAR *)((BYTE *)para + ((tinfo->name_type[0] == 'H') ? 8 : 4));
	}else{
		parap = para;
	}
	memset(para, 0, BARDATASIZE);
	GetCustTable(tinfo->name_type, label, para, sizeof(para));
	para[TSIZEOF(para) - 1] = '\0';
/*
	if ( tinfo->key == 'M' ){	// メニュー
		tstrreplace(para, T("\n"), T("\r\n"));
		SetDlgItemText(hDlg, IDE_ALCCMD, para);
		SendDlgItemMessage(hDlg, IDE_ALCCMD, EM_SETSEL, EC_LAST, EC_LAST);
		return;
	}
*/
	if ( tinfo->key == 'B' ){	// ボタン
		if ( tinfo->name_type[0] == 'B' ){
			int buttonindex = *(int *)para;

			buttonindex = (buttonindex < 0) ? 0 : buttonindex + 1;
			SendDlgItemMessage(hDlg, IDL_BLIST, LB_SETCURSEL, buttonindex, 0);
		}else{
			CharColor = ((COLORREF *)para)[0];
			BackColor = ((COLORREF *)para)[1];
		}
	}else if ( tinfo->key == 'E' ){	// ファイル判別は、判別名検索
		VFSFILETYPE vft;
		TCHAR *ext;

		vft.flags = VFSFT_TYPETEXT;
		vft.typetext[0] = '\0';
		ext = tstrchr(label, '.');
		if ( (label[0] == ':') && (ext != NULL) ){
			VFSGetFileType(ext + 1, NULL, VFSFTSIZE_FROMTYPE, &vft);
			if ( vft.typetext[0] == '\0' ){
				*ext = '\0';
				VFSGetFileType(label, NULL, VFSFTSIZE_FROMTYPE, &vft);
			}
		}else{
			VFSGetFileType(label, NULL, VFSFTSIZE_FROMTYPE, &vft);
		}
		SetDlgItemText(hDlg, IDS_EXITEM, vft.typetext);
	}
	if ( (UTCHAR)parap[0] == EXTCMD_CMD ){	// コマンド
		parap++;
		tstrreplace(parap, T("\n"), T("\r\n"));
		SetDlgItemText(hDlg, IDE_ALCCMD, parap);
		SendDlgItemMessage(hDlg, IDE_ALCCMD, EM_SETSEL, 0, EC_LAST);
		keyname = parap;
	}else{	// コマンド以外→キー割当て
		TCHAR *destptr;
		WORD *keyptr, key;

		if ( (UTCHAR)parap[0] == EXTCMD_KEY ){
			keyptr = (WORD *)&parap[1];
		}else{
			keyptr = (WORD *)parap;
		}
		destptr = buf;
		*destptr++ = '%';
		*destptr++ = 'K';
		*destptr++ = '\"';
		for ( ;; ){
			key = *keyptr++;
			if ( key == 0 ) break;
			if ( destptr > (buf + 3) ) *destptr++ = ' ';
			keyname = destptr;
			PutKeyCode(destptr, key);
			destptr += tstrlen(destptr);
		}
		*destptr = '\0';

		SetDlgItemText(hDlg, IDE_ALCCMD, buf);
		SendDlgItemMessage(hDlg, IDE_ALCCMD, EM_SETSEL,
				(WPARAM)(keyname - buf), (LPARAM)tstrlen(buf));
	}

	SetCommandNameList(hDlg, tinfo, keyname, NULL);
}

void SetListViewItemDetail(TABLEINFO *tinfo, int index, const TCHAR *label, const BYTE *param)
{
	HWND hLVAlcWnd = tinfo->hLVAlcWnd;
	TCHAR labelbuf[MAX_PATH];
	const BYTE *paramptr;
	LV_ITEM lvi;

	lvi.iItem = index;
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = 1;

	paramptr = param;
	switch ( tinfo->key ){
		case 'B':
			paramptr = (param + ((tinfo->name_type[0] == 'H') ? 8 : 4));
			break;

		case 'm': {
			lvi.pszText = MakeMouseDetailText(NULL, label, labelbuf);
			ListView_SetItem(hLVAlcWnd, &lvi);
			lvi.iSubItem = 2;
			lvi.pszText = labelbuf;
			ListView_SetItem(hLVAlcWnd, &lvi);
			lvi.iSubItem = 3;
			break;
		}

		case 'K': {
			const TCHAR *p;

			p = label;
			if ( MakeKeyDetailListText(tinfo, &p, labelbuf) ){
				lvi.pszText = labelbuf;
			}else{
				lvi.pszText = (TCHAR *)label;
			}
			ListView_SetItem(hLVAlcWnd, &lvi);
			lvi.iSubItem = 2;
			break;
		}
//		default:
	}

	if ( tinfo->key == 'M' ){ // メニューはそのまま
		lvi.pszText = (TCHAR *)param;
	}else{
		if ( (UTCHAR)paramptr[0] == EXTCMD_CMD ){	// コマンド
			const TCHAR *listp;

			listp = KeyList;
			lvi.pszText = (TCHAR *)paramptr + 1;
			for (;;){
				if ( *listp == '\0' ) break;
				if ( *listp != GROUPCHAR ){
					const TCHAR *p = tstrchr(listp, '\t'), *p2;

					if ( p != NULL ){
						p2 = tstrchr(p + 1, '\t');
						if ( p2 == NULL ) p2 = p;

						if ( tstrcmp(p2 + 1, lvi.pszText) == 0 ){
							if ( (p2 == p) || (UseLcid != LCID_PPXDEF) ){ //JP
								tstrcpy(labelbuf, listp);
								labelbuf[p - listp] = '\0';
							}else{ // EN
								tstrcpy(labelbuf, p + 1);
								labelbuf[p2 - (p + 1)] = '\0';
							}
							lvi.pszText = labelbuf;
							break;
						}
					}
				}
				listp += tstrlen(listp) + 1;
				if ( *listp == '\0' ) break;
			}
		}else{
			const WORD *kp;
			WORD key;
			TCHAR *p;

			lvi.pszText = labelbuf;
			kp = (WORD *)paramptr;
			if ( (UTCHAR)paramptr[0] == EXTCMD_KEY ){
				kp = (WORD *)&paramptr[sizeof(TCHAR)];
			}
			p = labelbuf;
			*p = '\0';
			for ( ;; ){
				key = *kp++;
				if ( !key ) break;
				if ( p != labelbuf ) *p++ = ' ';
				PutKeyCode(p, key);
				p += tstrlen(p);
			}
			SetCommandNameList(NULL, tinfo, labelbuf, labelbuf);
		}
	}
	ListView_SetItem(hLVAlcWnd, &lvi);
}

void SelectedItem(HWND hDlg, TABLEINFO *tinfo, TCHAR *keyword)
{
	if ( tinfo->key == 'M' ){
		SelectedItemMenu(hDlg, tinfo, keyword);
	}else{
		SelectedItemExecute(hDlg, tinfo, keyword);
	}
	EnableSetButton(hDlg, FALSE);
}

void SetButtonPositionList(HWND hDlg, const TCHAR *label)
{
	HWND hCmb;
	DWORD ppType;
	int bindex;
	const struct MouseTypeListStruct *mtl;
	TCHAR buf[MAX_PATH];

	bindex = (int)SendDlgItemMessage(hDlg, IDC_ALCMOUSEB, CB_GETCURSEL, 0, 0);

	hCmb = GetDlgItem(hDlg, IDC_ALCMOUSET);
	SendMessage(hCmb, WM_SETREDRAW, FALSE, 0);
	SendMessage(hCmb, CB_RESETCONTENT, 0, 0);

	if ( label[2] == 't' ){
		ppType = CE_TRAY;
	}else{
		ppType = (label[2] == 'c') ? CE_PPC : CE_PPV;
	}

	for ( mtl = MouseTypeList ; mtl->enables ; mtl++ ){
		if ( (mtl->enables & ppType) &&
			 (mtl->enables & (1 << (bindex + CE_SHIFT))) ){
			thprintf(buf, TSIZEOF(buf), T("%s %Ms"), mtl->type, mtl->name);
			SendMessage(hCmb, CB_ADDSTRING, 0, (LPARAM)buf);
		}
	}
	SendMessage(hCmb, WM_SETREDRAW, TRUE, 0);
}

#ifdef _WIN64
	const WCHAR CrStr[] = {0x21b5, '\0'};
#else
	const TCHAR CrStr[] = T("/");
#endif

//------------------------------------------------ 種類を選択したときの表示処理
void SelectedType(HWND hDlg, TABLEINFO *tinfo)
{
	int size;
	TCHAR label[CUST_NAME_LENGTH], labelbuf[CUST_NAME_LENGTH];
	HWND hLVTypeWnd = tinfo->hLVTypeWnd, hLVAlcWnd;
	LV_ITEM lvi;

	if ( hLVTypeWnd == NULL ) return;

	lvi.mask = LVIF_TEXT;
	lvi.pszText = label;
	lvi.cchTextMax = MAX_PATH;
	lvi.iSubItem = 0;
	if ( tinfo->index_type < 0 ){ // 選択されていないときは検索する
		TCHAR *p;

		GetControlText(hDlg, IDE_EXTYPE, labelbuf, TSIZEOF(labelbuf) - 2);
		lvi.iItem = 0;
		for ( ;; ){
			if ( ListView_GetItem(hLVTypeWnd, &lvi) == FALSE ) break;
			p = tstrrchr(label, '/');
			if ( p != NULL ){
				p++;
			}else{
				p = label;
			}
			if ( tstricmp(p, labelbuf) == 0 ){
				tinfo->index_type = lvi.iItem;
				break;
			}
			lvi.iItem++;
		}
		if ( tinfo->index_type >= 0 ){
			ListView__Select(tinfo->hLVTypeWnd, tinfo->index_type);
		}else{
			return;
		}
	}
										// Type 設定
	label[0] = '\0';
	lvi.iItem = tinfo->index_type;
	ListView_GetItem(hLVTypeWnd, &lvi);
	if ( label[0] == '\0' ) return;

	{
		TCHAR *p = tstrrchr(label, '/');
		if ( p != NULL ){
			p++;
		}else{
			p = label;
		}
		tstrcpy(tinfo->name_type, p);
		SetDlgItemText(hDlg, IDE_EXTYPE, p);
	}

	if ( tinfo->key == 'm' ){ // マウスのボタンリストを調整
		SetButtonPositionList(hDlg, label);
	}else if ( tinfo->key == 'B' ){ // ツールバーのボタン一覧を調整
		BOOL showH = TRUE, showB = FALSE;

		if ( tinfo->name_type[0] == 'B' ){
			LoadBar(hDlg);
			showB = TRUE;
			showH = FALSE;
		}
		ShowDlgWindow(hDlg, IDB_MECKEYS, showB);
		ShowDlgWindow(hDlg, IDB_BREF, showB);
		ShowDlgWindow(hDlg, IDL_BLIST, showB);
		ShowDlgWindow(hDlg, IDB_HMCHARC, showH);
		ShowDlgWindow(hDlg, IDB_HMBACKC, showH);
	}
										// Item 設定
	hLVAlcWnd = tinfo->hLVAlcWnd;
	if ( hLVAlcWnd != NULL ){
		LV_COLUMN lvc;
		LV_ITEM lvi;
		TCHAR param[CMDLINESIZE];
		int index, memoindex = 0;

		SendMessage(hLVAlcWnd, WM_SETREDRAW, FALSE, 0);
		SendMessage(hLVAlcWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
		ListView_DeleteAllItems(hLVAlcWnd);
		ListView_DeleteColumn(hLVAlcWnd, 0);
		ListView_DeleteColumn(hLVAlcWnd, 0);
		ListView_DeleteColumn(hLVAlcWnd, 0);
		SelectItemLV = -1;

		if ( tinfo->key == 'K' ){
			memoindex = 2;
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = (TCHAR *)MessageText(StrLabelTargetKey);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 0, &lvc);
			lvc.pszText = (TCHAR *)MessageText(StrLabelTargetKeyName);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 1, &lvc);
			lvc.pszText = (TCHAR *)MessageText(StrLabelDetail);
			lvc.cx = 800;
			ListView_InsertColumn(hLVAlcWnd, 2, &lvc);
		}else if ( tinfo->key == 'm' ){
			memoindex = 3;
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = (TCHAR *)MessageText(StrLabelTargetButton);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 0, &lvc);
			lvc.pszText = (TCHAR *)MessageText(StrLabelTargetButtonName);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 1, &lvc);
			lvc.pszText = (TCHAR *)MessageText(StrLabelTargetArea);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 2, &lvc);
			lvc.pszText = (TCHAR *)MessageText(StrLabelDetail);
			lvc.cx = 800;
			ListView_InsertColumn(hLVAlcWnd, 3, &lvc);
		}else if ( tinfo->key == 'M' ){
			memoindex = 1;
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = (TCHAR *)MessageText(StrLabelMenuItemName);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 0, &lvc);
			lvc.pszText = (TCHAR *)MessageText(StrLabelDetail);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 1, &lvc);
		}else if ( tinfo->key == 'E' ){
			memoindex = 1;
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = (TCHAR *)MessageText(StrLabelTypeName);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 0, &lvc);
			lvc.pszText = (TCHAR *)MessageText(StrLabelDetail);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 1, &lvc);
		}else if ( tinfo->key == 'B' ){
			memoindex = 1;
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = (TCHAR *)MessageText(StrLabelItemName);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 0, &lvc);
			lvc.pszText = (TCHAR *)MessageText(StrLabelDetail);
			lvc.cx = 100;
			ListView_InsertColumn(hLVAlcWnd, 1, &lvc);
		}

		lvi.mask = LVIF_TEXT | LVIF_PARAM; // 「新規」欄
		lvi.lParam = 0;

		lvi.pszText = (TCHAR *)NilStr;
		lvi.iItem = 0;
		lvi.iSubItem = 0;
		lvi.iItem = ListView_InsertItem(hLVAlcWnd, &lvi);

		lvi.mask = LVIF_TEXT;
		lvi.pszText = (TCHAR *)MessageText(StrMenuNewMemo);
		lvi.iSubItem = memoindex;
		ListView_SetItem(hLVAlcWnd, &lvi);

		index = 0;
		ListViewCounter = 1;

		param[TSIZEOFSTR(param)] = '\0';
		for ( ;; ){
			memset(param, 0, BARDATASIZE);
			size = EnumCustTable(index, tinfo->name_type, label, param, SIZEOFTSTR(param));
			if ( 0 > size ) break;

			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.lParam = ListViewCounter++;
			lvi.pszText = label;
			lvi.iItem++;
			lvi.iSubItem = 0;

			tstrreplace(param, T("\n"), CrStr);

			if ( (tinfo->key == 'M') && (tstrlen(param) < 50) ){
				if ( (label[2] == '\0') || (label[2] == '.') ){
					if ( (label[0] == '-') && (label[1] == '-') ){ // セパレータ ==
						tstrcat(param, GetURtext(T("  -- 区切り線 --\0  -- separator --")));
					}else if ( (label[0] == '|') && (label[1] == '|') ){ // 改桁 ==
						tstrcat(param, GetURtext(T("  == 縦線 ==\0  == column separator ==")));
					}
				}else if ( (param[0] == '?') && (param[1] == '?') ){
					tstrcat(param, GetURtext(T("  [埋め込みメニュー]\0  [embed menu]")));
				}else if ( (param[0] == '?') || ((param[0] == '%') && (param[1] == 'M')) ){
					tstrcat(param, GetURtext(T("  [下層メニュー]\0  [child menu]")));
				}
			}

			SetListViewItemDetail(tinfo,
					ListView_InsertItem(hLVAlcWnd, &lvi), label, (BYTE *)param);
			index++;
		}
		SendMessage(hLVAlcWnd, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE);
		SendMessage(hLVAlcWnd, LVM_SETCOLUMNWIDTH, 1, LVSCW_AUTOSIZE);
		SendMessage(hLVAlcWnd, WM_SETREDRAW, TRUE, 0);

		lvi.mask = LVIF_STATE;
		lvi.iItem = 0;
		lvi.state = LVNI_FOCUSED | LVIS_SELECTED;
		lvi.stateMask = LVNI_FOCUSED | LVIS_SELECTED;
		lvi.iSubItem = 0;
		ListView_SetItem(hLVAlcWnd, &lvi);

		EnableDlgWindow(hDlg, IDB_MEUP, TRUE);
		EnableDlgWindow(hDlg, IDB_MEDW, TRUE);
	}
	SelectedItem(hDlg, tinfo, NULL);
										// コメント取得
	label[0] = '\0';
	GetCustTable(T("#Comment"), tinfo->name_type, label, TSTROFF(VFPS));
	SetDlgItemText(hDlg, IDE_ALCCMT, label);
	EnableDlgWindow(hDlg, IDB_ALCCMT, FALSE);
}

//---------------------------------------------------------- 種類の一覧登録処理
void EnumTypeList(HWND hDlg, TABLEINFO *tinfo)
{
	int index, count, size;
	TCHAR name[CUST_NAME_LENGTH], typekey;
	const TCHAR *maskstr;
	struct LABELTEXT *list;
	HWND hLVTypeWnd = tinfo->hLVTypeWnd;
	LV_ITEM lvi;
	LV_COLUMN lvc;
	FN_REGEXP fn;

	if ( hLVTypeWnd == NULL ) return;

	lvi.mask = LVIF_TEXT; // 「新規」欄
	lvi.pszText = name;
	lvi.iItem = 0;
	lvi.iSubItem = 0;

	typekey = tinfo->key;

	index = SendDlgItemMessage(hDlg, IDC_TB_TYPEMASK, CB_GETCURSEL, 0, 0);
	if ( (index < 0) || (index > 5) ) index = 0;
	maskstr = MenuMaskList[index].mask;
	if ( (maskstr[0] == 'S') || (maskstr[0] == '?') ) typekey = maskstr[0];
	MakeFN_REGEXP(&fn, maskstr);

	SendMessage(hLVTypeWnd, WM_SETREDRAW, FALSE, 0);
	SendMessage(hLVTypeWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	ListView_DeleteAllItems(hLVTypeWnd);
	ListView_DeleteColumn(hLVTypeWnd, 0);
	lvc.mask = LVCF_WIDTH;
	lvc.cx = 200;
	ListView_InsertColumn(hLVTypeWnd, 0, &lvc);
											// 使用済みを検索
	for ( count = 0 ; ; count++ ){
		BOOL comment;

		size = EnumCustData(count, name, NULL, 0);
		if ( 0 > size ) break;

		if ( (typekey != '?') && (CheckTypeName(typekey, name) == FALSE) ){
			continue;
		}
		if ( FilenameRegularExpression(name, &fn) == FRRESULT_NO ) continue;

		comment = FALSE;
		for ( list = TypeLists ; list->name ; list++ ){
			if ( tstrcmp( name, list->name ) == 0 ){
				thprintf(name, TSIZEOF(name), T("%Ms /%s"), list->text, list->name);
				comment = TRUE;
				break;
			}
		}
		if ( comment == FALSE ){
			TCHAR kname[CUST_NAME_LENGTH], commentbuf[MAX_PATH], *cmtptr;

			commentbuf[0] = '\0';
			GetCustTable(T("#Comment"), name, commentbuf, sizeof(commentbuf));
			if ( commentbuf[0] != '\0' ){
				commentbuf[MAX_PATH - 1] = '\0';
				cmtptr = commentbuf;
				SkipSpace((const TCHAR **)&cmtptr);
				tstrcpy(kname, name);
				thprintf(name, TSIZEOF(name), T("%s %s /%s"), kname, cmtptr, kname);
			}
		}
		ListView_InsertItem(hLVTypeWnd, &lvi);
	}
											// 未使用を検索
	for ( list = TypeLists ; list->name ; list++ ){
		if ( CheckTypeName(typekey, list->name) == FALSE ) continue;
		if ( IsExistCustData(list->name) ) continue;
		if ( FilenameRegularExpression(list->name, &fn) == FRRESULT_NO ) continue;

		thprintf(name, TSIZEOF(name), T("%Ms /%s"), list->text, list->name);
		ListView_InsertItem(hLVTypeWnd, &lvi);
	}
	FreeFN_REGEXP(&fn);
	SendMessage(hLVTypeWnd, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE);
	SendMessage(hLVTypeWnd, WM_SETREDRAW, TRUE, 0);
}

void HideMenuColor(HWND hDlg, COLORREF *color)
{
	if ( PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SCOLOR),
			hDlg, HideMenuColorDlgBox, (LPARAM)color) > 0 ){
		EnableSetButton(hDlg, TRUE);
	}
}

// ワイルドカード用の処理を行う
void FixWildItemName(TCHAR *item)
{
	TCHAR *sp;

	if ( !((item[0] == '*') && (item[1] == '\0')) ){
		sp = item;
		if ( *sp == ':' ) sp++;
		for ( ; *sp ; sp++ ){
			if ( (*sp >= '!') && (*sp <= '?') ){
				if ( tstrchr(DetectWildcardLetter, *sp) != NULL ){
					memmove(&item[1], &item[0], TSTRSIZE(item));
					item[0] = '/';
					return;
				}
			}
		}
	}
	tstrupr(item);
}

//---------------------------------------------------------------- 項目登録処理
void AddItem(HWND hDlg, TABLEINFO *tinfo, DWORD mode)
{
	TCHAR typebuf[MAX_PATH], itemname[MAX_PATH], para[LONG_CMDSIZE + BARDATASIZE], *typename;
	TCHAR olditemname[MAX_PATH];
	int index;
	size_t size;
	HWND hLVAlcWnd;
	LV_ITEM lviALC, lviTYPE;

	GetControlText(hDlg, IDE_EXITEM, itemname, TSIZEOF(itemname));
	typename = typebuf + 1;
	GetControlText(hDlg, IDE_EXTYPE, typename, TSIZEOF(typebuf) - 2);
	if ( *typename == '\0' ) return;
	if ( *itemname == '\0' ) return;

	if ( (CheckTypeName(tinfo->key, typename) == FALSE) &&
		 ((tinfo->key != 'M') || (CheckTypeName('S', typename) == FALSE)) ){
		typebuf[0] = tinfo->key;
		typebuf[1] = '_';
		typename = typebuf;
		SetDlgItemText(hDlg, IDE_EXTYPE, typename);
	}
	hLVAlcWnd = tinfo->hLVAlcWnd;
	index = TListView_GetFocusedItem(hLVAlcWnd);

	lviALC.mask = LVIF_TEXT;
	lviALC.cchTextMax = MAX_PATH;

	if ( index >= 0 ){
		lviALC.pszText = olditemname;
		lviALC.iItem = index;
		lviALC.iSubItem = 0;
		ListView_GetItem(hLVAlcWnd, &lviALC);
	}else{
		index = 0;
		olditemname[0] = '\0';
	}

	if ( tinfo->key != 'M' ){
		TCHAR *parap;

		if ( tinfo->key == 'E' ) FixWildItemName(itemname);
		if ( tinfo->key == 'B' ){
			if ( tinfo->name_type[0] == 'H' ){ // 隠しメニューは色を保存
				((COLORREF *)para)[0] = CharColor;
				((COLORREF *)para)[1] = BackColor;
				parap = (TCHAR *)(BYTE *)((BYTE *)para + sizeof(COLORREF) * 2);
			}else{ // ツールバーはアイコンIDを保存
				int buttonindex = SendDlgItemMessage(hDlg, IDL_BLIST, LB_GETCURSEL, 0, 0);
				if ( buttonindex == 0 ){ // 非表示
					size_t itemlen;

					// ボタンテキストがないときは、追加する
					itemlen = tstrlen(itemname);
					if ( (tstrchr(itemname, '/') == NULL) &&
						 (itemlen < (MAX_PATH - 2)) ){
						tstrcat(itemname, T("/"));
						SetDlgItemText(hDlg, IDE_EXITEM, itemname);
					}
					buttonindex = -2;
				}else{
					buttonindex = buttonindex - 1;
				}
				*(int *)para = buttonindex;
				parap = (TCHAR *)((BYTE *)para + sizeof(DWORD));
			}
		}else{
			parap = para;
		}
		{
			const TCHAR *src;
			WORD *dst, keybuf[CMDLINESIZE];

			parap[0] = EXTCMD_CMD;
			parap[1] = '\0';
			GetDlgItemText(hDlg, IDE_ALCCMD, parap + 1, TSIZEOF(para) - BARDATASIZE);
			*(parap + LONG_CMDSIZE - 2) = '\0';
			FixCutReturnCode(parap + 1);
											// 「 ;」のエスケープ
			if ( tstrstr(parap + 1, T("%(")) == NULL ){
				tstrreplace(parap + 1, T(" ;"), T(" %;"));
			}

			size = TSTRSIZE(parap);
			src = parap + 1;
			if ( SkipSpace(&src) == '%' ){ // %K"～ を EXTCMD_KEY 形式に変換
				if ( *(src + 1) == 'K' ){
					src += 2;
					if ( SkipSpace(&src) == '\"' ){
						src++;
						dst = keybuf;

						for ( ;; ){
							int key;

							SkipSpace(&src);
							if ( *src == '\"' ){
								src++;
								if ( SkipSpace(&src) != '\0' ){ // \" は末尾？
									break;
								}
							}
							if ( *src == '\0' ){ // %K のみだったので key 扱い
								*dst++ = 0;
								*parap++ = EXTCMD_KEY;
								size = (dst - keybuf) * sizeof(WORD);
								memcpy(parap, keybuf, size);
								break;
							}
							key = GetKeyCode((const TCHAR **)&src);
							if ( key < 0 ) break;
							*dst++ = (WORD)key;
						}
					}
				}
			}
		}
		size += TSTROFF(parap - para); // ヘッダ分を加算
		if ( tinfo->key == 'K' ){
			int i;

			i = CheckRegistKey(itemname, itemname, typename);
			if ( i < 0 ){
				XMessage(hDlg, StrCustTitle, XM_GrERRld, MES_EKFT);
				return;
			}else if ( i == CHECKREGISTKEY_WARNKEY ){
				XMessage(hDlg, StrCustTitle, XM_ImWRNld, StrWarnBadAssc);
			}
		}
	}else{									// メニューのとき
		para[0] = '\0';
		GetDlgItemText(hDlg, IDE_ALCCMD, para, TSIZEOF(para) - 1);
		FixCutReturnCode(para);
		size = TSTRSIZE(para);
		if ( (index == 0) && (mode == IDB_TB_SETITEM) ){
			index = 0x7fffffff; // 末尾に新規挿入
		}

	}

	if ( (tinfo->key == 'B') || (tinfo->key == 'M') ){
		if ( ((itemname[2] == '\0') || (itemname[2] == '.')) &&
			 ( ((itemname[0] == '-') && (itemname[1] == '-')) ||
			   ((itemname[0] == '|') && (itemname[1] == '|')) ) ){
			if ( (index > 1) && (mode == IDB_ALCNEW) ) index--;
			if ( itemname[2] == '\0' ) { // 自動ナンバリング
				int num = 1;
				for(;;){
					thprintf(itemname + 2, TSIZEOF(itemname) - 2, T(".%d"), num);
					if ( !IsExistCustTable(typename, itemname) ) break;
					num++;
				}
			}
		}
	}

	if ( !IsExistCustTable(typename, itemname) ){ // 新規
		InsertCustTable(typename, itemname, index, para, size);
	}else{ // 変更
		SetCustTable(typename, itemname, para, size);
	}

	if ( tinfo->index_type >= 0 ){ // 種類が変わっていたら種類を再取得させる
		TCHAR selectedtypename[MAX_PATH];

		lviTYPE.mask = LVIF_TEXT;
		lviTYPE.cchTextMax = MAX_PATH;
		lviTYPE.pszText = selectedtypename;
		lviTYPE.iSubItem = 0;
		lviTYPE.iItem = tinfo->index_type;

		if ( ListView_GetItem(tinfo->hLVTypeWnd, &lviTYPE) == FALSE ){
			tinfo->index_type = -1;
			olditemname[0] = '\0';
		}else{
			TCHAR *seltype;

			seltype = tstrrchr(selectedtypename, '/');
			if ( seltype != NULL ){
				seltype++;
			}else{
				seltype = selectedtypename;
			}
			if ( tstrcmp(seltype, typename) != 0 ){
				olditemname[0] = '\0';
				tinfo->index_type = -1;
			}
		}
	}

	if ( (mode == IDB_TB_SETITEM) && (olditemname[0] != '\0') && tstricmp(olditemname, itemname) ){ // 項目名入替
		DeleteCustTable(typename, olditemname, 0); // 元は削除

		lviALC.pszText = itemname;
		lviALC.iSubItem = 0;
		ListView_SetItem(hLVAlcWnd, &lviALC);

		SetListViewItemDetail(tinfo, lviALC.iItem, itemname, (BYTE *)para);
	}else{ // 新規
		LV_FINDINFO lvfi;

		EnumTypeList(hDlg, tinfo);
		SelectedType(hDlg, tinfo);

		lvfi.flags = LVFI_STRING;
		lvfi.psz = itemname;
		index = ListView_FindItem(hLVAlcWnd, -1, &lvfi);
		SelectItemByIndex(tinfo, index);
		if ( index > 6 ){
			SendMessage(hLVAlcWnd, WM_VSCROLL, SB_LINEDOWN, 0);
			SendMessage(hLVAlcWnd, WM_VSCROLL, SB_LINEDOWN, 0);
			SendMessage(hLVAlcWnd, WM_VSCROLL, SB_LINEDOWN, 0);
		}
	}
	EnableSetButton(hDlg, FALSE);
	Changed(hDlg);
	SetFocus(hLVAlcWnd);
}

//---------------------------------------------------------------- 項目削除処理
void DeleteItem(HWND hDlg, TABLEINFO *tinfo)
{
	TCHAR type[MAX_PATH], item[MAX_PATH];
	int count, index;

	GetControlText(hDlg, IDE_EXTYPE, type, TSIZEOF(type));
	if ( CheckTypeName(tinfo->key, type) == FALSE ) return;
												// 項目が選択されているか確認
	// ※「新規」があるので -1
	count = ListView_GetItemCount(tinfo->hLVAlcWnd);
	if ( count < 0 ) return;
	index = TListView_GetFocusedItem(tinfo->hLVAlcWnd);
	if ( index < 0 ) return;
	if ( (index == 0) && (count > 1) ){
		thprintf(item, TSIZEOF(item), MessageText(StrQueryDeleteTable), type);
		if ( PMessageBox(hDlg, item, StrCustTitle,
				MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES ){
			return;
		}
		count = 1; // 完全削除状態に
	}

	if ( count <= 2 ){ // 残り一つ(新規を含めると2)なら完全削除
		DeleteCustData(type);
		EnumTypeList(hDlg, tinfo);
				// メニュー・ツールバーの削除（位置に基づく削除）
	}else if ( (tinfo->key == 'M') || (tinfo->key == 'B') ){
		DeleteCustTable(type, NULL, index - 1); // 「新規」分を減らす
	}else{	 // 一般削除（名前に基づく削除）
		GetControlText(hDlg, IDE_EXITEM, item, TSIZEOF(item));
		if ( tinfo->key == 'E' ) FixWildItemName(item);
		if ( *item != '\0' ) DeleteCustTable(type, item, 0);
	}
	SelectedType(hDlg, tinfo);
	if ( count > 2 ){
		if ( index >= (count - 1) ) index = count - 2;
		SelectItemByIndex(tinfo, index);
	}
	Changed(hDlg);
	return;
}

//-------------------------------------------------------- 項目を一つ上下に移動
void ItemUpDown(HWND hDlg, TABLEINFO *tinfo, int offset)
{
	TCHAR label[CUST_NAME_LENGTH], para[CMDLINESIZE * 11];
	int size, newpos;
	DWORD shift = GetShiftKey();

	if ( offset < 0 ){
		if ( tinfo->index_alc <= 0 ) return;
		if ( shift & K_c ) offset = -tinfo->index_alc;
	}else{
		int maxi;

		maxi = ListView_GetItemCount(tinfo->hLVAlcWnd) - 2;
		if ( tinfo->index_alc < 0 ) return;
		if ( tinfo->index_alc >= maxi ) return;
		if ( shift & K_c ) offset = maxi - tinfo->index_alc;
	}
	newpos = tinfo->index_alc + offset;

	size = EnumCustTable(tinfo->index_alc, tinfo->name_type, label, para, sizeof(para));
	if ( size < 0 ) return;
	DeleteCustTable(tinfo->name_type, NULL, tinfo->index_alc);
	InsertCustTable(tinfo->name_type, label, newpos, para, size);

	EnumTypeList(hDlg, tinfo);
	SelectedType(hDlg, tinfo);
	Changed(hDlg);

	SelectItemByIndex(tinfo, newpos + 1);
	tinfo->index_alc = newpos;
}

//-------------------------------------------------------------------- 動作試験
void TestTable(HWND hDlg, TABLEINFO *tinfo)
{
	TCHAR label[MAX_PATH];
	HMENU hMenu;
	DWORD id = 1;

	if ( tinfo->key != 'M' ){
		Test();
		return;
	}
										// メニューの場合はメニュー表示
	GetControlText(hDlg, IDE_EXTYPE, label, TSIZEOF(label));

	hMenu = PP_AddMenu(NULL, hDlg, NULL, &id, label, NULL);
	if ( hMenu != NULL){
		TrackButtonMenu(hDlg, IDB_TEST, hMenu);
		DestroyMenu(hMenu);
	}
	return;
}

const TCHAR Arrows[4][3] = {T("<-"), T("^"), T("v"), T("->") };

void InitMouseList(HWND hDlg)
{
	const struct MouseButtonListStruct *blist;
	TCHAR buf[MAX_PATH];

	blist = MouseButtonList;
	for (;;){
		thprintf(buf, TSIZEOF(buf), T("%s %Ms"), blist->type, blist->name);
		SendDlgItemMessage(hDlg, IDC_ALCMOUSEB, CB_ADDSTRING, 0, (LPARAM)buf);
		blist++;
		if ( blist->type[0] == '\0' ) break;
	}

	if ( (OSver.dwMajorVersion < 6) && (UseLcid != LCID_PPXDEF) ){ // Vista 以前の英語モードでは矢印が使えないかも
		int index;
		for ( index = 0; index < 4; index++ ){
			SetDlgItemText(hDlg, IDB_ALCMOUSEL + index, Arrows[index]);
		}
	}
	SetDlgItemText(hDlg, IDS_EXITEML, MessageText(MES_VLIL));
}

void SetKeyGroup(HWND hDlg)
{
	const TCHAR *p;
	TCHAR buf[MAX_PATH];

	p = KeyList;
	for ( ;; ){
		if ( *p == GROUPCHAR ){
			TCHAR *pb;

			tstrcpy(buf, p + 1);
			pb = tstrchr(buf, '\t');
			if ( pb == NULL ){
				pb = buf;
			}else{
				if ( UseLcid != LCID_PPXDEF ){
					*pb = '\0';
					pb = (TCHAR *)MessageText(buf);
				}else{
					pb++;
				}
			}
			SendDlgItemMessage(hDlg, IDC_ALCKEYG, CB_ADDSTRING, 0, (LPARAM)pb);
		}
		p += tstrlen(p) + 1;
		if ( *p == '\0' ) return;
	}
}

void SetComment(HWND hDlg, TABLEINFO *tinfo)
{
	TCHAR comment[MAX_PATH];

	comment[0] = '\0';
	GetControlText(hDlg, IDE_ALCCMT, comment, TSIZEOF(comment));
	if ( NO_ERROR == SetCustStringTable(T("#Comment"), tinfo->name_type, comment, 0) ){
		EnableDlgWindow(hDlg, IDB_ALCCMT, FALSE);
		Changed(hDlg);
	}
}

// ボタンの種類を指定
void SelectedMouseButton(HWND hDlg, TABLEINFO *tinfo, int index)
{
	TCHAR buf[MAX_PATH], item[MAX_PATH], *src, *dest;

	GetControlText(hDlg, IDE_EXITEM, item, TSIZEOF(item));
	src = item;
	dest = buf;
	while ( (*src != '\0') && !Isalpha(*src) ) *dest++ = *src++;

	if ( (index == GESTUREID) && (tinfo->name_type[1] != 'C') && (tinfo->name_type[1] != 'V') ){
		SendDlgItemMessage(hDlg, IDC_ALCMOUSEB, CB_SETCURSEL, 1, 0);
		return;
	}

	dest = tstpcpy(dest, MouseButtonList[index].type);
	*dest++= '_';
	*dest = '\0';
	if ( ShowMouseSetting(hDlg, index != GESTUREID) ){
		src = tstrchr(item, '_');
		if ( src != NULL ) tstrcpy(dest, src + 1);
	}
	SetDlgItemText(hDlg, IDE_EXITEM, buf);

	ListView__GetText(tinfo->hLVTypeWnd, tinfo->index_type, buf, TSIZEOF(buf));
	SetButtonPositionList(hDlg, buf);
}

// クリックの場所を指定
void SelectedMouseType(HWND hDlg, int index)
{
	TCHAR item[MAX_PATH], *p;

	GetControlText(hDlg, IDE_EXITEM, item, TSIZEOF(item));
	p = tstrchr(item, '_');
	if ( p == NULL ){
		p = item + tstrlen(item);
		*p = '_';
	}
	p++;

	SendDlgItemMessage(hDlg, IDC_ALCMOUSET, CB_GETLBTEXT, (WPARAM)index, (LPARAM)p);
	p = tstrchr(p, ' ');
	if ( p != NULL ) *p = '\0';
	SetDlgItemText(hDlg, IDE_EXITEM, item);
}

void LoadBar(HWND hDlg)
{
	TCHAR type[VFPS], name[VFPS];
	HWND hListWnd;
	int i, iconsize;
	TBADDBITMAP tbab;
	RECT box = {0, 0, IDL_BLIST_W, IDL_BLIST_H};
	int lineSize, scrollH;

	tstrcpy(((TOOLBARCUSTTABLESTRUCT *)name)->text + 1, T("toolbar.bmp"));
	GetControlText(hDlg, IDE_EXTYPE, type, TSIZEOF(type));
	if ( NO_ERROR != GetCustTable(type, T("@"), name, sizeof(name)) ){
		name[0] = '\0';
	}else{
		if ( ((TOOLBARCUSTTABLESTRUCT *)name)->text[1] != '<' ){
			VFSFixPath(name, ((TOOLBARCUSTTABLESTRUCT *)name)->text + 1, NULL, VFSFIX_FULLPATH | VFSFIX_REALPATH);
		}else{
			tstrcpy(name, ((TOOLBARCUSTTABLESTRUCT *)name)->text + 1);
		}
	}
	if ( tstrcmp(BarBmpInfo.filename, name) == 0 ) return;
	DeleteObject(BarBmpInfo.hBmp);
								// ツールバーのイメージを取得する
	LoadToolBarBitmap(hDlg, name, &tbab, &BarBmpInfo.barsize);
	if ( tbab.hInst != NULL ){
		BarBmpInfo.hBmp = LoadBitmap(tbab.hInst, MAKEINTRESOURCE(tbab.nID));
	}else{
		BarBmpInfo.hBmp = (HBITMAP)tbab.nID;
	}
	tstrcpy(BarBmpInfo.filename, name);
	BarBmpInfo.items = BarBmpInfo.barsize.cx / BarBmpInfo.barsize.cy;

	iconsize = BarBmpInfo.barsize.cy + 2;
	MapDialogRect(hDlg, &box);
	hListWnd = GetDlgItem(hDlg, IDL_BLIST);
	SendMessage(hListWnd, LB_RESETCONTENT, 0, 0);
	SendMessage(hListWnd, LB_SETCOLUMNWIDTH, (WPARAM)iconsize, 0);
	SendMessage(hListWnd, LB_SETITEMHEIGHT, 0, (LPARAM)iconsize);

	scrollH = GetSystemMetrics(SM_CYHSCROLL) + 4;
	box.bottom -= scrollH;
#if 1
	lineSize = iconsize;
	if ( lineSize > box.bottom) lineSize = box.bottom;
#else
	lineSize = box.bottom / iconsize;
	lineSize = (lineSize > 0) ? lineSize * iconsize : box.bottom;
#endif
	SetWindowPos(hListWnd, NULL, 0, 0, box.right, lineSize + scrollH, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

	for ( i = BarBmpInfo.items + 1 ; i ; i-- ){ // 非表示分を含めて追加
		SendMessage(hListWnd, LB_ADDSTRING, 0, (LONG)1);
	}
}

void DrawBar(DRAWITEMSTRUCT *lpdis)
{
	RECT rect;

	if ( lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT) ){
		// ビットマップがないとき用の枠の描画
		FrameRect(lpdis->hDC, &lpdis->rcItem, GetStockObject(WHITE_BRUSH));

		if ( BarBmpInfo.hBmp == NULL ) return;
		if ( lpdis->itemID > 0 ){
			HGDIOBJ hOldBmp;
			HDC hDC;

			hDC = CreateCompatibleDC(lpdis->hDC);
			hOldBmp = SelectObject(hDC, BarBmpInfo.hBmp);

			BitBlt(lpdis->hDC, lpdis->rcItem.left + 1, lpdis->rcItem.top + 1,
					BarBmpInfo.barsize.cy, BarBmpInfo.barsize.cy, hDC,
					(lpdis->itemID - 1) * BarBmpInfo.barsize.cy, 0, SRCCOPY);

			SelectObject(hDC, hOldBmp);
			DeleteDC(hDC);
		}
		if ( lpdis->itemState & ODS_SELECTED ){ // 選択枠(2pixel幅)を描画
			FrameRect(lpdis->hDC, &lpdis->rcItem, GetStockObject(BLACK_BRUSH));

			rect.left = lpdis->rcItem.left + 1;
			rect.top = lpdis->rcItem.top + 1;
			rect.right = lpdis->rcItem.right - 1;
			rect.bottom = lpdis->rcItem.bottom - 1;
			FrameRect(lpdis->hDC, &rect, GetStockObject(LTGRAY_BRUSH));
		}
	}
	if ( lpdis->itemAction & ODA_FOCUS ){ // フォーカス枠(2pixel幅の点線)を描画
		int i;

		for ( i = 0 ; i < 2 ; i++ ){
			rect.left = lpdis->rcItem.left + i;
			rect.top = lpdis->rcItem.top + i;
			rect.right = lpdis->rcItem.right - i;
			rect.bottom = lpdis->rcItem.bottom - i;
			DrawFocusRect(lpdis->hDC, &rect);
		}
	}
}

void SetButtonCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, TABLEINFO *tinfo)
{
	int index = -1;
	TCHAR buf[CMDLINESIZE];
	const TCHAR *command;

	if ( GetListCursorIndex(wParam, lParam, &index) == 0 ) return;

	// 既定のコマンド・コメントを設定する
	if ( (index < 0) || (index >= ToolBarDefaultCommandCount) ) return;
	command = ToolBarDefaultCommand[index][0];

	SetDlgItemText(hDlg, IDE_EXITEM, MessageText(ToolBarDefaultCommand[index][1]));
	SetCommandNameList(hDlg, tinfo, command, NULL);
	if ( command[0] != '*' ){ // キー割当て
		thprintf(buf, TSIZEOF(buf), T("%%K\"%s"), command);
		command = buf;
	}
	SetDlgItemText(hDlg, IDE_ALCCMD, command);
}

WNDPROC hOldAlclistProc = NULL;

LRESULT CALLBACK AlcListProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg){
		case WM_NOTIFY:
			#define NHPTR ((NMHDR *)lParam)
			if ( (NHPTR->idFrom == 0) && (NHPTR->code == NM_CUSTOMDRAW) ){
				if ( ((NMCUSTOMDRAW *)lParam)->dwDrawStage == CDDS_PREPAINT ) {
					return CDRF_NOTIFYITEMDRAW;
				}
				if ( ((NMCUSTOMDRAW *)lParam)->dwDrawStage == CDDS_ITEMPREPAINT ) {
					SetTextColor(((NMCUSTOMDRAW *)lParam)->hdc, C_DialogText); // ヘッダーの文字色
				}
				return CDRF_DODEFAULT;
			}
	}
	return CallWindowProc(hOldAlclistProc, hWnd, iMsg, wParam, lParam);
}

void InitTablePage(HWND hDlg, TABLEINFO *tinfo)
{
	InitPropSheetsUxtheme(hDlg);
	tinfo->hLVAlcWnd = GetDlgItem(hDlg, IDV_ALCLIST);
	tinfo->hLVTypeWnd = GetDlgItem(hDlg, IDV_EXTYPE);
	tinfo->ListViewLastSort = 0;

	if ( X_uxt_color >= UXT_MINMODIFY ){ // Listview の処理アドレスは同じなので、分けていない
		hOldAlclistProc = (WNDPROC)SetWindowLongPtr(tinfo->hLVAlcWnd, GWLP_WNDPROC, (LONG_PTR)AlcListProc);
	}

	if ( KeyList == NULL ){
		KeyList = LoadTextResource(hInst, MAKEINTRESOURCE(DEFKEYLIST));
		if ( KeyList == NULL ) KeyList = T("");
#if 0		// キーコードの正当性チェック
		{
			const TCHAR *p, *group;
			TCHAR buf[100];
			int key;
			const TCHAR *err = NULL;

			p = KeyList;
			for ( ;; ){
				if ( *p == GROUPCHAR ){
					group = p;
				}else{
					TCHAR *q, *r;

					q = tstrchr(p, '\t');
					if ( q != NULL ){
						while ( *q == '\t' ) q++;
						q = tstrchr(q, '\t');
						if ( q != NULL ){
							while ( *q == '\t' ) q++;
							r = q;
							if ( (*q != '%') && (*q != '*') && (*q != '>') ){
								key = GetKeyCode((const TCHAR **)&q);
								if ( *q ) err = T("InitTablePage Error - KeyCode");
//								↓登録済みキーでなければエラー
								if ( !(key & (K_raw | K_ex)) ){
									if ( (key != 'P') &&
										(key != (K_s | 'P')) && (key != 'E') ){
										err = T("InitTablePage Error - UnknownKey");
									}
								}
								PutKeyCode(buf, key);
								if ( tstrcmp(buf, r) != 0 ){
									err = T("InitTablePage Error - KeyUnmatch");
								}
							}
						}
					}
					if ( err != NULL ){
						XMessage(hDlg, NULL, XM_DbgDIA, T("%s\n%s\n%s"), err, group, p);
						err = FALSE;
					}
				}
				p += tstrlen(p) + 1;
				if ( *p == '\0' ) break;
			}
		}
#endif
	}
	if ( tinfo->key == 'm' ) InitMouseList(hDlg);
	if ( tinfo->key == 'M' ){
		const struct MenuMaskListStruct *MaskList;
		TCHAR buf[MAX_PATH];

		for ( MaskList = MenuMaskList; MaskList->name != NULL; MaskList++ ){
			thprintf(buf, TSIZEOF(buf), T("%Ms; %s"), MaskList->name, MaskList->mask);
			SendDlgItemMessage(hDlg, IDC_TB_TYPEMASK, CB_ADDSTRING, 0, (LPARAM)buf );
		}
		SendDlgItemMessage(hDlg, IDC_TB_TYPEMASK, CB_SETCURSEL, 0, 0);
		SetDlgItemText(hDlg, IDS_EXTYPE, MessageText(MES_VMET));
		SetDlgItemText(hDlg, IDS_EXITEML, MessageText(MES_VMIL));
	}

	SetKeyGroup(hDlg);

	SendDlgItemMessage(hDlg, IDE_EXTYPE, EM_LIMITTEXT, MAX_PATH - 1, 0);
	SendDlgItemMessage(hDlg, IDE_ALCCMT, EM_LIMITTEXT, MAX_PATH - 1, 0);
	SendDlgItemMessage(hDlg, IDE_EXITEM, EM_LIMITTEXT, MAX_PATH - 1, 0);
	SendDlgItemMessage(hDlg, IDE_ALCCMD, EM_LIMITTEXT, (WPARAM)(LONG_CMDSIZE - 1), 0);
	EnableTextChangeNotifyItem(hDlg, IDE_ALCCMT);
	EnableTextChangeNotifyItem(hDlg, IDE_ALCCMD);
	EnableTextChangeNotifyItem(hDlg, IDE_EXITEM);
	if ( OSver.dwMajorVersion >= 6 ){
		PPxRegistExEdit(NULL, GetDlgItem(hDlg, IDE_ALCCMD), LONG_CMDSIZE - 1,
				NULL, PPXH_GENERAL, 0, PPXEDIT_USEALT | PPXEDIT_NOWORDBREAK);
	}

	SendDlgItemMessage(hDlg, IDE_ALCCMT, EM_SETCUEBANNER, 0, (LPARAM)GetBannerText(StrCommentInfo));

	EnumTypeList(hDlg, tinfo);

	if ( FirstTypeName == NULL ){
		const WCHAR *iteminfo;

		switch ( tinfo->key ){
			case 'm':
				FirstTypeName = T("MC_click");
				iteminfo = BANNERTEXT(MES_VBCL, L"x_xxx\0x_xxx");
				break;

			case 'M':
				FirstTypeName = T("M_pjump");
				iteminfo = BANNERTEXT(MES_VBPJ, L"表示項目\0item name");
				break;

			case 'B':
				FirstTypeName = T("B_cdef");
				iteminfo = BANNERTEXT(MES_VBCD, L"[ボタンテキスト/]項目名\0[btn. text/]tip name");
				break;

			case 'E':
				FirstTypeName = T("E_cr");
				iteminfo = BANNERTEXT(MES_VBCR, L"ext\0ext");
				break;

			case 'K':
				FirstTypeName = T("KC_main");
				iteminfo = BANNERTEXT(MES_VBMA, L"キー割当て\0key name");
				break;

			default:
				iteminfo = NULL;
		}
		if ( iteminfo != NULL ){
			SendDlgItemMessage(hDlg, IDE_EXITEM, EM_SETCUEBANNER, 0, (LPARAM)GetBannerText(iteminfo));
		}
	}

	tinfo->index_type = 0;
	tinfo->index_alc = 0;
	if ( FirstTypeName == NULL ){
		ListView__Select(tinfo->hLVTypeWnd, tinfo->index_type);
		SelectedType(hDlg, tinfo);
	}else{
		tinfo->index_type = LB_ERR;
		SetDlgItemText(hDlg, IDE_EXTYPE, FirstTypeName);
		SelectedType(hDlg, tinfo);
		if ( FirstItemName == NULL ){
			tinfo->index_alc = 0;
		}else{
			TCHAR *p;

			p = FirstItemName;
			if ( *p == '#' ){
				p++;
				tinfo->index_alc = GetNumber((const TCHAR **)&p);
				if ( tinfo->index_alc == LB_ERR ){
					SetDlgItemText(hDlg, IDE_EXITEM, FirstItemName);
				}else{
					tinfo->index_alc++;  // 新規枠
				}
			}
		}
		FirstTypeName = NULL;
		FirstItemName = NULL;
	}
	SelectItemByIndex(tinfo, tinfo->index_alc);
}

INT_PTR CALLBACK TablePage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, TABLEINFO *tinfo)
{
	LV_ITEM lvi;
	TCHAR buf[CMDLINESIZE];

	switch (msg){
		case WM_INITDIALOG:
			InitTablePage(hDlg, tinfo);
			break;

		case WM_CONTEXTMENU:
			if ( GetDlgCtrlID((HWND)wParam) == IDB_TB_SETITEM ){
				HMENU hPopMenu = CreatePopupMenu();

				AppendMenuString(hPopMenu, 1, StrMenuNew);
				if ( 0 < TrackButtonMenu(hDlg, IDB_TB_SETITEM, hPopMenu) ){
					AddItem(hDlg, tinfo, IDB_ALCNEW);
				}
				DestroyMenu(hPopMenu);
				break;
			}
			if ( (HWND)wParam == hDlg ) break;
			// WM_HELP へ
		case WM_HELP:
			PPxHelp(hDlg, HELP_CONTEXT, tinfo->helpID);
			break;

		case WM_DRAWITEM:
			if ( wParam == IDL_BLIST ) DrawBar((DRAWITEMSTRUCT *)lParam);
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case ID_COMMAND_ENUMTYPE:
					EnumTypeList(hDlg, tinfo);
					break;

				case IDB_ALCCMT:
					if (HIWORD(wParam) == BN_CLICKED) SetComment(hDlg, tinfo);
					break;

				case IDE_ALCCMT:
					if ( HIWORD(wParam) == EN_CHANGE ){
						EnableDlgWindow(hDlg, IDB_ALCCMT, TRUE);
					}
					break;

				case IDC_ALCKEYG:
					if ( HIWORD(wParam) == CBN_SELCHANGE ){
						int index;

						index = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
						if ( index != CB_ERR ) SelectedKeyGroup(hDlg, index);
					}
					break;

				case IDC_ALCKEYS:
					if ( HIWORD(wParam) == CBN_SELCHANGE ){
						int index;

						index = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
						if ( index != CB_ERR ) SelectedKeySubID(hDlg, index);
					}
					break;

				case IDC_TB_TYPEMASK:
					if ( (HIWORD(wParam) == CBN_SELCHANGE) ||
						 (HIWORD(wParam) == CBN_EDITUPDATE) ){
						PostMessage(hDlg, WM_COMMAND, ID_COMMAND_ENUMTYPE, 0);
					}
					break;
										// コマンド関連
				case IDB_ALCCMDI:
					InsertMacroString(hDlg, tinfo);
					SetDlgFocus(hDlg, IDE_ALCCMD);
					break;

				case IDE_ALCCMD:
					// IDE_EXITEM へ
				case IDE_EXITEM:
					if ( HIWORD(wParam) == EN_CHANGE ){
						EnableSetButton(hDlg, TRUE);
					}
					break;

				case IDB_TB_SETITEM:
				case IDB_ALCNEW:
					AddItem( hDlg, tinfo, LOWORD(wParam) );
					break;

				case IDB_TB_DELITEM:
					DeleteItem(hDlg, tinfo);
					break;

				case IDB_TEST:
					TestTable(hDlg, tinfo);
					break;

				case IDB_ALCCMDLIST:
					InitCmdList(hDlg);
					break;
// Item 位置操作関連
				case IDB_MEUP:
					ItemUpDown(hDlg, tinfo, -1);
					break;

				case IDB_MEDW:
					ItemUpDown(hDlg, tinfo, +1);
					break;
// キー割当て専用
				case IDB_ALCKEY:
					ChooseKey(hDlg, IDE_EXITEM, tinfo);
					break;
// メニュー・ツールバー専用
				case IDB_MECKEYS:
					InsertMenuSeparator(hDlg, tinfo->key == 'B');
					SetFocus(tinfo->hLVAlcWnd);
					break;
// メニュー専用
				case IDB_MEKEY:
					if ( IsTrue(KeymapMenu(hDlg)) ){
						EnumTypeList(hDlg, tinfo);
						SelectedType(hDlg, tinfo);
						Changed(hDlg);
					}
					break;
// 隠しメニュー専用
				case IDB_HMCHARC:
					HideMenuColor(hDlg, &CharColor);
					break;
				case IDB_HMBACKC:
					HideMenuColor(hDlg, &BackColor);
					break;
// マウス専用
				case IDB_ALCMOUSEL:
				case IDB_ALCMOUSEU:
				case IDB_ALCMOUSED:
				case IDB_ALCMOUSER:
					SendDlgItemMessage(hDlg, IDE_EXITEM, EM_SETSEL,
							EC_LAST, EC_LAST);
					SendDlgItemMessage(hDlg, IDE_EXITEM, WM_CHAR,
							(LPARAM)GALLOW[LOWORD(wParam) - IDB_ALCMOUSEL], 0);
					break;

				case IDC_ALCMOUSEB:
					if ( HIWORD(wParam) == CBN_SELCHANGE ){
						int index;

						index = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
						if ( index != CB_ERR ) SelectedMouseButton(hDlg, tinfo, index);
					}
					break;

				case IDC_ALCMOUSET:
					if ( HIWORD(wParam) == CBN_SELCHANGE ){
						int index;

						index = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
						if ( index != CB_ERR ){
							SelectedMouseType(hDlg, index);
						}
					}
					break;
// ツールバー専用
				case IDL_BLIST:
					if ( HIWORD(wParam) == LBN_SELCHANGE ){
						EnableSetButton(hDlg, TRUE);
						SetButtonCommand(hDlg, wParam, lParam, tinfo);
					}
					break;

				case IDB_BREF:
					ToolBarMenu(hDlg);
					break;

				#if AddDefaultCommandList
				case IDX_ALCEXLIST:
					AddDefaultCmdList(tinfo->hLVAlcWnd);
					break;
				#endif
			}
			break;

		case WM_NOTIFY:
			if ( wParam == IDV_ALCLIST ){
				#define PNM ((NM_LISTVIEW *)lParam)
				switch ( NHPTR->code ){
					case LVN_ITEMCHANGED:
						if ( SelectItemLV == PNM->iItem ) break;
						SelectItemLV = PNM->iItem;
						lvi.mask = LVIF_TEXT | LVIF_PARAM;
						lvi.pszText = buf;
						lvi.cchTextMax = CMDLINESIZE;
						lvi.iItem = SelectItemLV;
						lvi.iSubItem = 0;
						ListView_GetItem(PNM->hdr.hwndFrom, &lvi);
						SelectedItem(hDlg, tinfo, buf);
						tinfo->index_alc = (int)lvi.lParam - 1;
						EnableSetButton(hDlg, FALSE);
						break;

					case LVN_COLUMNCLICK: {
						LISTVIEWCOMPAREINFO lvfi;
						int neworder;

						neworder = PNM->iSubItem + 1;
						if ( tinfo->ListViewLastSort >= 0 ){
							if ( tinfo->ListViewLastSort == neworder ){
								neworder = -neworder;
							}
						}
						lvfi.hListViewWnd = PNM->hdr.hwndFrom;
						lvfi.column = PNM->iSubItem;
						lvfi.order = neworder;
						ListView_SortItems(PNM->hdr.hwndFrom, (PFNLVCOMPARE)
								ListViewCompareFunc, (LPARAM)&lvfi);
						tinfo->ListViewLastSort = neworder;
						EnableDlgWindow(hDlg, IDB_MEUP, FALSE);
						EnableDlgWindow(hDlg, IDB_MEDW, FALSE);
						break;
					}
				}
				#undef PNM
				return TRUE;
			}
			if ( wParam == IDV_EXTYPE ){
				#define PNM ((NM_LISTVIEW *)lParam)
				switch ( NHPTR->code ){
					case LVN_ITEMCHANGED:
						if ( tinfo->index_type != PNM->iItem ){
							tinfo->index_type = PNM->iItem;
							SelectedType(hDlg, tinfo);
						}
						break;
				}
				#undef PNM
				return TRUE;
			}

			if ( NHPTR->code == PSN_SETACTIVE ){
				InitWndIcon(hDlg, (tinfo->key == 'B') ? IDE_ALCCMD : IDB_ALCCMDI);
				if ( (hCmdListWnd != NULL) && IsWindow(hCmdListWnd) ){
					PostMessage(hCmdListWnd, WM_CLOSE, 0, 0);
					hCmdListWnd = NULL;
					return 0;
				}
			}
			// Ok を選んだときに、"設定"を忘れていないか確認する
			if ( NHPTR->code == PSN_APPLY ){
				if ( IsWindowEnabled(GetDlgItem(hDlg, IDB_TB_SETITEM)) ){
					if ( PMessageBox(hDlg, StrQueryNoAdd, StrCustTitle,
						MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES ){
						SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
					}
					return TRUE;
				}
			}

			if ( (hCmdListWnd != NULL) && (NHPTR->hwndFrom == hCmdListWnd) ){
				return CmdTreeNotify(hDlg, NHPTR);
			}
//			return DlgSheetProc(hDlg, msg, wParam, lParam, tinfo->helpID);
			#undef NHPTR
		default:
			return DlgSheetProc(hDlg, msg, wParam, lParam, tinfo->helpID);
	}
	return TRUE;
}

INT_PTR CALLBACK KeyPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_INITDIALOG ){
		keyinfo.key = 'K';
		keyinfo.helpID = IDD_KEYD;
	}
	return TablePage(hDlg, msg, wParam, lParam, &keyinfo);
}

INT_PTR CALLBACK MousePage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_INITDIALOG ){
		mouseinfo.key = 'm';
		mouseinfo.helpID = IDD_MOUSED;
	}
	return TablePage(hDlg, msg, wParam, lParam, &mouseinfo);
}

INT_PTR CALLBACK ExtPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_INITDIALOG ){
		extinfo.key = 'E';
		extinfo.helpID = IDD_EXT;
	}
	return TablePage(hDlg, msg, wParam, lParam, &extinfo);
}

INT_PTR CALLBACK MenuPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_INITDIALOG ){
		menuinfo.key = 'M';
		menuinfo.helpID = IDD_MENUD;
	}
	return TablePage(hDlg, msg, wParam, lParam, &menuinfo);
}

INT_PTR CALLBACK BarPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_INITDIALOG ){
		barinfo.key = 'B';
		barinfo.helpID = IDD_BARD;
		BarBmpInfo.hBmp = NULL;
		BarBmpInfo.filename[0] = '\1';
	}
	return TablePage(hDlg, msg, wParam, lParam, &barinfo);
}
