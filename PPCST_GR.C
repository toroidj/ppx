/*-----------------------------------------------------------------------------
	Paper Plane xUI	customizer								全般シート
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPCUST.H"
#pragma hdrstop

#define IDM_SETEDITFOCUS	0xe001f000
#define IDM_RCLICKED		0xe0020000
#ifndef UDM_SETRANGE32
#define UDM_SETRANGE32 (WM_USER+111)
#endif

// itemname:key[:sub][+offset]=(edittype)
typedef enum {
	ITEM_INDEXLABEL,	// 左ペイン表示用文字列(str)
	ITEM_LABEL,			// ラベル表示用文字列(str)
	ITEM_DOCKLABEL,		// 折り畳み表示用文字列(str)
	ITEM_INT,	// int(num)				i/指定無し
	ITEM_DWORD,	// DWORD(num)			d
	ITEM_WORD,	// WORD(num)			w
	ITEM_BYTE,	// byte(num)			b
	ITEM_STRING, // 文字列(str)			s
	ITEM_EXT_STRING, // ファイル判別のコマンド(str)			e
	ITEM_KEY,	// キー(num)
	ITEM_FONT,	// フォント(font)
	ITEM_LANG,	// 言語詳細
	ITEM_RESERVED	// その他
} ITEMTYPES;

typedef enum {
	EDIT_NONE,	// 表示用文字列
	EDIT_CHECK,	// チェックボックス		?[itemtype][B]check[/def]:uncheck
	EDIT_RADIO,	// ラジオボタン			@[L][itemtype]hit/def
	EDIT_EDIT,	// エディットボックス	<def
	EDIT_KEY,	// キー					K
	EDIT_FILE,	// ファイル名			N
	EDIT_DIR,	// ディレクトリ名		P
	EDIT_FONT,	// フォント				F
	EDIT_ppchotkey,	// PPcのホットキー	c
	EDIT_LANG,	// 言語詳細				L
	EDIT_DOCKLABEL, // 折り畳み表示
} EDITTYPES;

typedef enum {
	ITEMOPTION_NONE,
	ITEMOPTION_NOMACROSTR, // %0 等は使えない
	ITEMOPTION_CHANGENOW, // 即時反映
} ITEMOPTION;

typedef struct {
	DWORD def, data, truedata, falsedata, mask, size;
} INTITEM;
#define IITEM_VALUE MAX32

typedef struct {
	TCHAR data[VFPS * 2];
	TCHAR setdata[VFPS * 2];
} STRITEM;

typedef union {
	INTITEM num;
	STRITEM str;
	LOGFONTWITHDPI fontdata;
} ITEMS;

typedef struct {
	ITEMTYPES itemtype;		// アイテムの種別
	ITEMOPTION option;		// 追加情報
	EDITTYPES edittype;		// アイテムの編集方法
	int layer;				// 階層
	BOOL edit;				// エディットボックスで編集中か
	TCHAR itemname[0x200];	// 表示用のアイテム名
	TCHAR *keyname;			// アイテムの保存名
	TCHAR *subname;			// テーブルのアイテムの保存名
	DWORD offset;			// 読み書きするオフセット
	ITEMS d;
} ITEMSTRUCT;


LOGFONT DefaultFont = {							// フォント構造体
	0, 0,						// Width, Height
	0, 0, FW_NORMAL,				// Escapement, Orientation, Weight
	FALSE, FALSE, FALSE,			// Italic, Underline, StrikeOut
	SHIFTJIS_CHARSET,			// CharSet
	OUT_DEFAULT_PRECIS,			// OutPrecision
	CLIP_DEFAULT_PRECIS,		// ClipPrecision
	DEFAULT_QUALITY,			// Quality
	FIXED_PITCH | FF_DONTCARE,	// PitchAndFamily
	T("")			// FaceName
};
const TCHAR BrankSetting[] = MES_VBRK;

const TCHAR EDIT[] = WC_EDIT;
const TCHAR BUTTON[] = WC_BUTTON;
WNDPROC OldFindBoxProc, OldIndexTreeProc, OldItemTreeProc;

const TCHAR *CustomList, *CustomItemList;
int CustomItemListOffset;

#define FLOAT_WIDTH 150
#define FLOAT_HEIGHT 22
#define FLOAT_BUTTONWIDTH 50
HWND hEditWnd, hButtonWnd, hSpinWnd;		// フローティングコントロール
HWND hIndexTreeWnd = NULL, hItemTreeWnd;	// 左ペイン,右ペイン
HTREEITEM hedititem;
int useedit = 0;
int useeditmodify = 0;
ITEMSTRUCT edititem;
#if !NODLL
int X_pmc[4] = {X_pmc_defvalue};
#else
extern int X_pmc[4];
#endif

struct cmdstrstruct { // BCC で "\x9f*focus" ができない対策
	TCHAR precode;
	TCHAR str[9];
} TrayHotKeyPPcCustParam = {EXTCMD_CMD, T("*focus !")};

const TCHAR TrayHotKeyCust[] = T("K_tray");
const TCHAR DelKeyMsg[] = MES_QDKM;
const TCHAR WarnNoMacroMsg[] = MES_WNMM;

const TCHAR StrMenuHelp[] = MES_HELP;
const TCHAR StrMenuDefault[] = MES_VHDE;
const TCHAR StrFontDefault[] = MES_VRDF;
const TCHAR StrFontList[] = T("mdfutc");
const WCHAR StrFindInfo[] = BANNERTEXT(MES_VBFI, L"説明文 or X_xxx\0Word or X_xxx");

#define maxlayer 4	// 対応階層数

typedef enum {
	ICON_BLANK = 0,
	ICON_LABEL,
	ICON_CHECKOFF,
	ICON_CHECKON,
	ICON_RADIOOFF,
	ICON_RADIOON,
	ICON_BUTTON,
	ICON_DOCKCLOSE,
	ICON_DOCKOPEN,
	// 以下は、画像作成時のみ使用
	IMAGE_CHECKMARK
} LABELICONS;

#define LABELICONS_COUNT IMAGE_CHECKMARK
const TCHAR TreeImageChars[LABELICONS_COUNT + 1] = {' ', 0xa7, 0xa8, 0xa8, 0xa1, 0xa4, 0x6e, 0xf0, 0xda, 0xfc}; // 最後は、チェックボックスのチェック

void SearchPPcHotKey(ITEMSTRUCT *item)
{
	int count = 0;
	TCHAR keyword[CUST_NAME_LENGTH], param[CMDLINESIZE];

	while( EnumCustTable(count, TrayHotKeyCust, keyword, param, sizeof(param)) >= 0 ){
		if ( !tstrcmp(param, (TCHAR *)&TrayHotKeyPPcCustParam) ){
			const TCHAR *ptr;

			ptr = keyword;
			item->d.num.data = GetKeyCode(&ptr);
			break;
		}
		count++;
	}
	item->d.num.def = item->d.num.data;
}

void SetPPcHotKey(ITEMSTRUCT *item)
{
	TCHAR keyword[CMDLINESIZE];

	if ( item->d.num.def != 0 ){
		PutKeyCode(keyword, item->d.num.def);
		DeleteCustTable(TrayHotKeyCust, keyword, 0);
	}
	if ( item->d.num.data != 0 ){
		PutKeyCode(keyword, item->d.num.data);
		SetCustTable(TrayHotKeyCust, keyword, &TrayHotKeyPPcCustParam, sizeof(TrayHotKeyPPcCustParam));
	}
}

const TCHAR *SearchList(int line)
{
	const TCHAR *p;

	if ( line <= 0 ) return NULL;
	p = CustomList;
	while( --line ){
		p += tstrlen(p) + 1;
		if ( *p == '\0' ) return NULL;
	}
	return p;
}

void GetCustItemType(ITEMSTRUCT *item, TCHAR **line)
{
	item->d.num.size = 4;
	switch ( *((*line)++) ){
		case 'b':
			item->itemtype = ITEM_BYTE;
			item->d.num.size = 1;
			break;
		case 'd':
			item->itemtype = ITEM_DWORD;
			break;
		case 'e':
			item->itemtype = ITEM_EXT_STRING;
			break;
		case 'i':
			item->itemtype = ITEM_INT;
			break;
		case 's':
			item->itemtype = ITEM_STRING;
			break;
		case 'w':
			item->itemtype = ITEM_WORD;
			item->d.num.size = 2;
			break;
		default:
			(*line)--;
			break;
	}
}

void GetData(ITEMSTRUCT *item, void *data, DWORD size)
{
	TCHAR work[CMDLINESIZE * 2];

	memset((char *)work + item->offset, 0, size);
	if ( item->subname == NULL ){
		if ( GetCustDataSize(item->keyname) <= (int)item->offset ) return;
		if ( NO_ERROR != GetCustData(item->keyname, work, sizeof work) ) return;
	}else{
		if ( GetCustTableSize(item->keyname, item->subname) <= (int)item->offset){
			return;
		}
		if ( NO_ERROR != GetCustTable(item->keyname, item->subname, work, sizeof work) ){
			return;
		}
	}
	memcpy(data, (char *)work + item->offset, size);
}

void SetData(ITEMSTRUCT *item, void *data, DWORD size)
{
	TCHAR work[CMDLINESIZE * 2];
	int datasize;

	memset(work, 0, sizeof work);
	if ( item->subname == NULL ){
		datasize = GetCustDataSize(item->keyname);
		GetCustData(item->keyname, work, sizeof work);
	}else{
		datasize = GetCustTableSize(item->keyname, item->subname);
		GetCustTable(item->keyname, item->subname, work, sizeof work);
	}
	memcpy((char *)work + item->offset, data, size);
	if ( (datasize == -1) || ((DWORD)datasize < (item->offset + size)) ){
		datasize = item->offset + size;
	}
	if ( item->subname == NULL ){
		SetCustData(item->keyname, work, datasize);
	}else{
		SetCustTable(item->keyname, item->subname, work, datasize);
	}
}

void GetCustItemData(ITEMSTRUCT *item, TCHAR **line)
{
	switch ( item->itemtype ){
		case ITEM_INT:
		case ITEM_DWORD:
		case ITEM_WORD:
		case ITEM_BYTE:
		case ITEM_LANG: {
			TCHAR *p;

			item->d.num.truedata = GetNumber((const TCHAR **)line);
			p = tstrchr(*line, '/');
			if ( p != NULL ){
				p++;
				item->d.num.def = GetNumber((const TCHAR **)&p);
				*line = p;
			}else{
				item->d.num.def = 0;
			}
			item->d.num.data = item->d.num.def;
			GetData(item, &item->d.num.data, item->d.num.size);
			break;
		}
		case ITEM_EXT_STRING:
		case ITEM_STRING:
			tstrcpy(item->d.str.setdata, *line);
			item->d.str.data[0] = '\0';
			GetData(item, &item->d.str.data, sizeof(item->d.str.data));
			if ( item->itemtype == ITEM_STRING ) break;
			// ITEM_EXT_STRING は、キー割当て／コマンドの判別
			if ( (UTCHAR)item->d.str.data[0] == EXTCMD_CMD ){ // コマンド
				memmove(item->d.str.data, (char *)(TCHAR *)(item->d.str.data + 1), TSTRSIZE(item->d.str.data + 1));
			}
			break;
	}
}

void GetCustItem(ITEMSTRUCT *item, const TCHAR *line, BOOL getdata)
{
	const TCHAR *linekey;
	TCHAR *param, *key, *subkey, *offset;

	item->itemtype = ITEM_LABEL;
	item->option = ITEMOPTION_NONE;
	item->edittype = EDIT_NONE;
	item->layer = 1;
	item->offset = 0;
	item->edit = FALSE;
	item->keyname = NULL;
									// layer 数の取得
	if ( *line == '.' ){
		item->itemtype = ITEM_INDEXLABEL;
		while ( *line == '.' ){
			line++;
			item->layer++;
		}
	}else{
		while ( *line == '\t' ){
			line++;
			item->layer++;
		}
	}
	// 表示テキストの決定と、設定内容の分離
	linekey = NULL;
	key = item->itemname;
	for(;;){
		TCHAR chr;

		chr = *line;
		if ( chr == '\0' ) break;
		if ( chr == '\1' ){ // 日本語表記開始位置
			line++;
			if ( UseLcid == LCID_PPXDEF ){
				key = item->itemname; // 保存をやり直す
				continue;
			}else{ // 英語なので日本語表記をスキップ
				for (;;){
					chr = *line;
					if ( chr == '\0' ) break;
					if ( chr == ':' ){
						linekey = line;
						break;
					}
					line++;
				}
				break;
			}
		}else if ( chr == ':' ){
			linekey = line;
			break;
		}
		*key++ = chr;
		line++;
	}

	*key++ = '\0'; // terminate
	if ( item->itemname[MESTEXTIDLEN] == '|' ){ // '|'が terminate より後になることがあるが、異常動作はしないため、チェックを簡略化
		key = tstpcpy(item->itemname, MessageText(item->itemname)) + 1;
	}

	if ( linekey++ == NULL ) return; // なし

		// Mes のときは、 MesXXXX に置換 現在 Mes:DIRS のみ使用
	if ( (linekey[0] == 'M') && (linekey[1] == 'e') && (linekey[2] == 's') ){
		thprintf(key, 0x200, T("Mes%04X%s"), UseLcid, linekey + 3);
	}else{
		if ( linekey[0] == '+' ){
			item->itemtype = ITEM_DOCKLABEL;
			item->edittype = EDIT_DOCKLABEL;
			return;
		}
		tstrcpy(key, linekey);
	}

	param = tstrchr(key, '=');
	if ( param == NULL ) return;
	*param++ = '\0';
	item->keyname = key;
									// offset 取得
	offset = tstrchr(key, '+');
	if ( offset != NULL ){
		*offset++ = '\0';
		item->offset = GetNumber((const TCHAR **)&offset);
	}
									// subname 取得
	subkey = tstrchr(key, ':');
	if ( subkey == NULL ){
		item->subname = NULL;
	}else{
		*subkey++ = '\0';
		item->subname = subkey;
	}
									// 中身の取得はしないか確認
	if ( getdata == FALSE ){
		item->itemtype = ITEM_RESERVED;
		return;
	}
									// 各項目の処理
	switch ( *param++ ){
		case '?':
			item->edittype = EDIT_CHECK;
			item->itemtype = ITEM_DWORD;
			GetCustItemType(item, &param);
			if ( *param == 'B' ){
				param++;
				item->d.num.mask = 0;
			}else{
				item->d.num.mask = IITEM_VALUE;
			}
			GetCustItemData(item, &param);
			if ( item->itemtype == ITEM_STRING ){
				TCHAR *sepp;

				sepp = tstrchr(param, ':');
				if ( sepp != NULL ){
					*(item->d.str.setdata + (sepp - param)) = '\0';
					param = sepp;
				}
			}

			if ( *param == ':' ){
				param++;
				item->d.num.falsedata = GetNumber((const TCHAR **)&param);
			}else{
				item->d.num.falsedata = 0;
			}
			if ( item->d.num.mask == 0 ){
				item->d.num.truedata = LSBIT << item->d.num.truedata;
				item->d.num.mask = ~item->d.num.truedata;
				if ( *param == '!' ){
					param++;
					item->d.num.falsedata = 1;
				}
			}
			break;
		case '@':
			item->edittype = EDIT_RADIO;
			item->itemtype = ITEM_DWORD;
			if ( *param == 'L' ){
				param++;
				item->option = ITEMOPTION_CHANGENOW;
			}
			GetCustItemType(item, &param);
			GetCustItemData(item, &param);
			break;
		case '<':
			item->edittype = EDIT_EDIT;
			item->itemtype = ITEM_STRING;
			GetCustItemType(item, &param);
			GetCustItemData(item, &param);
			break;
		case 'K':
			item->edittype = EDIT_KEY;
			item->itemtype = ITEM_KEY;
			item->d.num.data = 0;
			GetData(item, &item->d.num.data, sizeof(WORD));
			break;
		case 'N':
			item->edittype = EDIT_FILE;
			item->itemtype = ITEM_STRING;
			GetCustItemType(item, &param);
			GetCustItemData(item, &param);
			if ( *param == 'D' ) item->option = ITEMOPTION_NOMACROSTR;
			break;
		case 'P':
			item->edittype = EDIT_DIR;
			item->itemtype = ITEM_STRING;
			GetCustItemType(item, &param);
			GetCustItemData(item, &param);
			if ( *param == 'D' ) item->option = ITEMOPTION_NOMACROSTR;
			break;
		case 'F':
			item->edittype = EDIT_FONT;
			item->itemtype = ITEM_FONT;
			item->d.fontdata.font = DefaultFont;
			item->d.fontdata.dpi = 0;
			tstrcpy(item->d.fontdata.font.lfFaceName, MessageText(BrankSetting));
			GetData(item, &item->d.fontdata, sizeof(item->d.fontdata));
			break;
		case 'c':
			item->edittype = EDIT_ppchotkey;
			item->itemtype = ITEM_KEY;
			item->d.num.data = 0;
			SearchPPcHotKey(item);
			break;
		case 'L':
			item->edittype = EDIT_LANG;
			item->itemtype = ITEM_LANG;
			item->d.num.data = 0;
			item->d.num.size = 4;
			GetCustItemData(item, &param);
			break;
//		default:
	}
}

HANDLE hMlang = NULL;

const TCHAR RegMIME1766[] = T("MIME\\Database\\Rfc1766");
void LangName(TCHAR *dest, int lcid, const TCHAR *tail)
{
	TCHAR lcidstr[16], *ptr;

	if ( lcid == 0 ) lcid = LOWORD(GetUserDefaultLCID());

	thprintf(lcidstr, TSIZEOF(lcidstr), T("%04X"), lcid);
	dest[0] = '\0';
	GetRegString(HKEY_CLASSES_ROOT, RegMIME1766, lcidstr, dest, 0x100);
	if ( dest[0] == '\0' ){
		tstrcpy(dest, lcidstr);
	}else{
		ptr = tstrchr(dest, ';');
		if ( (ptr != NULL) && (*(ptr + 1) == '@') ){
			*ptr = '\0';
			if ( hMlang == NULL ) hMlang = LoadSystemDLL(SYSTEMDLL_MLANG);
			if ( hMlang != NULL ){
				TCHAR *ptr2;

				ptr2 = tstrrchr(ptr + 1, ',');
				if ( ptr2 != NULL ){
					ptr2++;
					if ( LoadString(hMlang, -GetNumber((const TCHAR **)&ptr2), ptr + 1, 0x40) != 0 ){
						*ptr = ' ';
					}
				}
			}
		}
	}
	if ( tail != NULL ) tstrcat(dest, tail);
}

UINT USEFASTCALL GetGDIFontDPI(HWND hWnd)
{
	HDC hDC;
	UINT dpi;

	hDC = GetDC(hWnd);
	dpi = GetDeviceCaps(hDC, LOGPIXELSX);
	ReleaseDC(hWnd, hDC);
	return dpi;
}

void MakeEditText(TCHAR *dest, ITEMSTRUCT *item)
{
	switch ( item->itemtype ){
		case ITEM_INT:
			thprintf(dest, 16, T("%d"), item->d.num.data);
			break;
		case ITEM_DWORD:
		case ITEM_WORD:
		case ITEM_BYTE:
			thprintf(dest, 16, T("%u"), item->d.num.data);
			break;
		case ITEM_STRING:
		case ITEM_EXT_STRING:
			tstrcpy(dest, item->d.str.data);
			break;
		case ITEM_KEY:
			if ( item->d.num.data ){
				MakeKeyDetailText(item->d.num.data, dest, FALSE);
			}else{
				dest[0] = '\0';
			}
			break;
		case ITEM_FONT: {
			int pix, pt;

			pix = item->d.fontdata.font.lfHeight;
			if ( pix == 0 ){
				tstrcpy(dest, item->d.fontdata.font.lfFaceName);
			}else{
				int windpi = (int)PPxCommonExtCommand(K_GETDISPDPI, (WPARAM)hItemTreeWnd);

				if ( tstrcmp(item->keyname, T("F_dlg")) == 0 ){
					pix = pt = (item->d.fontdata.font.lfHeight >= 0) ? item->d.fontdata.font.lfHeight : -item->d.fontdata.font.lfHeight;
					pix = (pix * windpi + (DEFAULT_DTP_DPI / 2)) / DEFAULT_DTP_DPI;
				}else{
					int fontdpi = item->d.fontdata.dpi;

					if ( fontdpi == 0 ) fontdpi = DEFAULT_WIN_DPI;
					pix = (item->d.fontdata.font.lfHeight * windpi) / fontdpi;
					pt = (((pix >= 0) ? pix : -pix) * DEFAULT_DTP_DPI + (windpi / 2) ) / windpi;
				}
				thprintf(dest, 128, T("%s, %dpt(%d)"),
						item->d.fontdata.font.lfFaceName, pt, pix);
			}
			break;
		}
		case ITEM_LANG:
			LangName(dest, item->d.num.data,
					(item->d.num.data == 0) ? T(" - auto") : NULL);
			break;

		default:
			dest[0] = '\0';
			break;
	}
}

int MakeDisplayText(TCHAR *dest, ITEMSTRUCT *item, TV_ITEM *tvi)
{
	LABELICONS state = ICON_LABEL;
	TCHAR *foot = NULL, footbuf[0x1000], destbuf[0x1000];

	switch(item->edittype){
		case EDIT_CHECK:
			if ( item->itemtype == ITEM_STRING ){ // 文字列変更
				state = !tstrcmp(item->d.str.data, item->d.str.setdata) ?
					ICON_CHECKON : ICON_CHECKOFF;
			}else if ( item->d.num.mask == IITEM_VALUE ){	// 値変更
				state = (item->d.num.data == item->d.num.falsedata) ?
					ICON_CHECKOFF : ICON_CHECKON;
			}else{							// bit 変更
				state = (item->d.num.data & item->d.num.truedata) ?
					ICON_CHECKON : ICON_CHECKOFF;
				if ( item->d.num.falsedata ){
					state = (state == ICON_CHECKOFF) ?
						ICON_CHECKON : ICON_CHECKOFF;
				}
			}
			break;

		case EDIT_RADIO:
			switch ( item->itemtype ){
				case ITEM_DWORD:
				case ITEM_INT:
				case ITEM_WORD:
				case ITEM_BYTE:
					state = ( item->d.num.data == item->d.num.truedata ) ?
							ICON_RADIOON : ICON_RADIOOFF;
					break;
				case ITEM_STRING:
				case ITEM_EXT_STRING:
					state = !tstrcmp(item->d.str.data, item->d.str.setdata) ?
							ICON_RADIOON : ICON_RADIOOFF;
					break;
//				default:
			}
			break;
		case EDIT_FONT:
		case EDIT_FILE:
		case EDIT_DIR:
		case EDIT_EDIT:
		case EDIT_KEY:
		case EDIT_ppchotkey:
		case EDIT_LANG:
			state = ICON_BUTTON;
			if ( IsTrue(item->edit) ) break;
			foot = footbuf;
			MakeEditText(destbuf, item);
			thprintf(footbuf, TSIZEOF(footbuf), T(" [ %s ]"), destbuf);
			break;
		case EDIT_DOCKLABEL:
			state = ICON_DOCKCLOSE;
			break;
//		default:
	}
	tstrcpy(dest, item->itemname);
	if ( foot != NULL ) tstrcat(dest, foot);
	tvi->iImage = tvi->iSelectedImage = state;
	setflag(tvi->mask, TVIF_IMAGE | TVIF_SELECTEDIMAGE);
	return state;
}


void LoadListTree(BOOL itemtree)
{
	HWND hTwnd;
	HTREEITEM hParent[maxlayer + 1]; // 0:Root 1:分類 2:項目 3:選択肢
	HTREEITEM hFirst = NULL;
	int ParentType[maxlayer + 1]; // 親のラベル種類
	int linecount = 1;
	const TCHAR *line, *lines;
	int layer, oldlayer = maxlayer;

	hParent[0] = TVI_ROOT; // 右ツリーの時はこちらが root
	hParent[1] = TVI_ROOT; // 左ツリーの時はこちらが root
	if ( IsTrue(itemtree) ){ // 右ツリーを表示
		hTwnd = hItemTreeWnd;
		lines = CustomItemList;
		linecount = CustomItemListOffset;
		SendMessage(hTwnd, WM_SETREDRAW, FALSE, 0);
		SendMessage(hTwnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)NULL); // 選択解除(TVN_SELCHANGED が連続発行されないように)
		TreeView_DeleteAllItems(hTwnd);
	}else{ // 左(+右)ツリーの表示
		hTwnd = hIndexTreeWnd;
		lines = CustomList;
		SendMessage(hTwnd, WM_SETREDRAW, FALSE, 0);
	}

	while ( *lines ){
		TCHAR dest[0x1000];
		ITEMSTRUCT item;
		TV_ITEM tvi;
		TV_INSERTSTRUCT tvins;

		line = lines;
		GetCustItem(&item, line, itemtree); // itemtree なら内容も取得
		if ( IsTrue(itemtree) ){ // item のときは、次のIndexで終了
			if ( item.itemtype == ITEM_INDEXLABEL ) break;
			item.layer--;
		}else{ // indexのときは、Itemをスキップ
			if ( item.itemtype != ITEM_INDEXLABEL ) goto next;
		}
		layer = item.layer;
		if ( layer > maxlayer ){
#ifndef RELEASE
			XMessage(hTwnd, NULL, XM_DbgDIA, T("Flow layer %d"), linecount);
#endif
			goto next;
		}

		tvi.mask = TVIF_TEXT | TVIF_PARAM;
		MakeDisplayText(dest, &item, &tvi);

		if ( (layer == 1) && (tvi.iImage == ITEM_INDEXLABEL) ){
			tvi.iImage = tvi.iSelectedImage = ICON_DOCKCLOSE;
		}

		tvi.pszText = dest;
		tvi.cchTextMax = tstrlen32(dest);
		tvi.lParam = (LPARAM)linecount;

		tvins.hParent = hParent[layer - 1];
		tvins.hInsertAfter = 0;
		TreeInsertItemValue(tvins) = tvi;
		hParent[layer] = (HTREEITEM)SendMessage(hTwnd,
				TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT)&tvins);
		ParentType[layer] = item.itemtype;
		if ( hFirst == NULL ) hFirst = hParent[layer];
		if ( (layer > oldlayer) &&
			 (ParentType[layer - 1] != ITEM_DOCKLABEL) ){
			TreeView_Expand(hTwnd, tvins.hParent, TVE_EXPAND);
		}
		oldlayer = layer;
next:
		linecount++;
		lines += tstrlen(lines) + 1;
	}
	SendMessage(hTwnd, TVM_SELECTITEM, TVGN_FIRSTVISIBLE, (LPARAM)hFirst);
	SendMessage(hTwnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(hTwnd, NULL, TRUE);
}

#ifndef TVM_SETITEMHEIGHT
	#define TVM_SETITEMHEIGHT (TV_FIRST + 27)
	#define TVM_GETITEMHEIGHT (TV_FIRST + 28)
#endif

void EnterTouchMode(HWND hDlg)
{
	int dpi = (int)PPxCommonExtCommand(K_GETDISPDPI, (WPARAM)hDlg);
	int nowheight = (int)SendMessage(hItemTreeWnd, TVM_GETITEMHEIGHT, 0, 0);
	int minheight;
	int X_tous[X_tousItems] = {X_tous_defvalue};

	GetCustData(T("X_tous"), &X_tous, sizeof(X_tous));
	minheight = CalcMinDpiSize(dpi, X_tous[tous_ITEMSIZE]);

	if ( nowheight < minheight ){
		SendMessage(hIndexTreeWnd, TVM_SETITEMHEIGHT, minheight, 0);
		SendMessage(hItemTreeWnd, TVM_SETITEMHEIGHT, minheight, 0);
	}
}

#ifndef WM_POINTERUP
#define WM_POINTERUP 0x0247
#define POINTER_FLAG_INCONTACT 4
#endif

void USEFASTCALL CheckTouch(HWND hWnd, UINT iMsg/*, WPARAM wParam*/)
{
	if ( // ((iMsg == WM_POINTERUP) && (X_pmc[pmc_mode] < 0) && (wParam & POINTER_FLAG_INCONTACT)) ||
		 ((iMsg == WM_GESTURE) && (X_pmc[pmc_mode] < 0))
	){
		EnterTouchMode(GetParent(hWnd));
	}
}

LRESULT CALLBACK IndexTreeProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CheckTouch(hWnd, iMsg);
	return CallWindowProc(OldIndexTreeProc, hWnd, iMsg, wParam, lParam);
}

LRESULT CALLBACK ItemTreeProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CheckTouch(hWnd, iMsg);
	return CallWindowProc(OldItemTreeProc, hWnd, iMsg, wParam, lParam);
}

#ifndef SM_MAXIMUMTOUCHES
 #define SM_MAXIMUMTOUCHES 95
#endif

size_t GetCustomListSize(const TCHAR *list)
{
	const TCHAR *last;

	last = list;
	for (;;){
		if ( *last == '\0' ) break;
		last = last + tstrlen(last) + 1;
	}
	return TSTROFF(last - list + 1);
}

void LoadCustomList(void)
{
	TCHAR *list, *extlist, *newlist, *path;
	HANDLE hFF;
	TCHAR custpath[MAX_PATH];//, path[MAX_PATH];
	WIN32_FIND_DATA ff;

	list = LoadTextResource(hInst, MAKEINTRESOURCE(DEFCUSTLIST));
	if ( list == NULL ) list = T("");

	GetModuleFileName(hInst, custpath, MAX_PATH);
	path = VFSFindLastEntry(custpath) + 1;
	tstrcpy(path, T("PPX*.DLL"));
	hFF = FindFirstFile(custpath, &ff);
	if ( hFF != INVALID_HANDLE_VALUE ){
		do {
			HINSTANCE hInst;

			// リソースしか使用しないが、32/64の区別が必要なため、DONT_RESOLVE_DLL_REFERENCESを指定
			tstrcpy(path, ff.cFileName);
			hInst = LoadLibraryEx(path, NULL, DONT_RESOLVE_DLL_REFERENCES);
			if ( hInst != NULL ){
				extlist = LoadTextResource(hInst, MAKEINTRESOURCE(PPXRCDATA_CUSTOMIZE_LIST));
				if ( extlist != NULL ){
					size_t listsize, extsize;

					listsize = GetCustomListSize(list);
					extsize = GetCustomListSize(extlist);
					newlist = HeapAlloc(GetProcessHeap(), 0, listsize + extsize + 4 );
					if ( newlist != NULL ){
						memcpy(newlist, list, listsize);
						memcpy((char *)newlist + listsize - sizeof(TCHAR), extlist , extsize );
						HeapFree(GetProcessHeap(), 0 ,list);
						list = newlist;
					}
					HeapFree(GetProcessHeap(), 0 ,extlist);
				}
			}
			FreeLibrary(hInst);
		}while( IsTrue(FindNextFile(hFF, &ff)) );
		FindClose(hFF);
	}
	CustomList = list;
}

#ifndef TVM_GETITEMHEIGHT
#define TVM_GETITEMHEIGHT (TV_FIRST + 28)
#endif

const TCHAR ButtonFont[] = T("Wingdings");

void LoadButtonImages(HWND hDlg)
{
	HIMAGELIST hImage = NULL;
	int TreeHeight;

	TreeHeight = (int)SendMessage(hItemTreeWnd, TVM_GETITEMHEIGHT, 0, 0) - 2;
	// チェックボックス等の画像を用意
	if ( (X_uxt[0] >= UXT_MINMODIFY) || (GetSystemMetrics(SM_CXICON) > 32) || (TreeHeight > 16) ){
		HBITMAP hBMP;
		HDC hDC, hMemDC;
		RECT box;
		HGDIOBJ hOldBmp, hOldFont;
		int i;
		NONCLIENTMETRICS ncm;
		HFONT hControlFont;
		TEXTMETRIC tm;
		SIZE chrsize;
		COLORREF backcolor;
		BITMAPINFO bmi;
		LPVOID lpBits;
	#ifdef _WIN64
		DWORD gres;
		WORD gindex = 0;
	#endif
		hDC = GetDC(hDlg);
		hMemDC = CreateCompatibleDC(hDC);
		ncm.cbSize = sizeof(ncm);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
		tstrcpy(ncm.lfStatusFont.lfFaceName, ButtonFont);
		ncm.lfStatusFont.lfCharSet = SYMBOL_CHARSET;
		ncm.lfStatusFont.lfHeight += (ncm.lfStatusFont.lfHeight > 0) ? +4 : -4;

		if ( (ncm.lfStatusFont.lfHeight < 0) ?
			(-ncm.lfStatusFont.lfHeight < TreeHeight) :
			( ncm.lfStatusFont.lfHeight < TreeHeight) ){
			ncm.lfStatusFont.lfHeight = -TreeHeight;
		}

		ncm.lfStatusFont.lfWidth = 0;
		hControlFont = CreateFontIndirect(&ncm.lfStatusFont);

		hOldFont = SelectObject(hMemDC, hControlFont);
		GetTextFace(hMemDC, LF_FACESIZE, ncm.lfStatusFont.lfFaceName);
	#ifdef _WIN64
		gres = GetGlyphIndices(hMemDC, T("1"), 1, &gindex, GGI_MARK_NONEXISTING_GLYPHS);

		// 設定したフォント名が違う、対応グリフがない場合は、内蔵画像を使用する
		if ( (tstrcmp(ncm.lfStatusFont.lfFaceName, ButtonFont) != 0) || (gres < 1) || (gindex == 0xffff) )
	#else
		if ( tstrcmp(ncm.lfStatusFont.lfFaceName, ButtonFont) != 0 )
	#endif
		{
		//	hImage = NULL;
		}else{
			GetTextMetrics(hMemDC, &tm);
			if ( tm.tmHeight == 0 ){
				tm.tmHeight = GetSystemMetrics(SM_CYMENU) - 5;
			}

			memset(&bmi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = bmi.bmiHeader.biHeight = tm.tmHeight - tm.tmInternalLeading;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = (WORD)((OSver.dwMajorVersion < 6) ? 24 : 32);
			hBMP = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &lpBits, NULL, 0);
			if ( hBMP != NULL ){
				HBRUSH hBackBrush;

				hImage = ImageList_Create(bmi.bmiHeader.biWidth, bmi.bmiHeader.biWidth, bmi.bmiHeader.biBitCount, LABELICONS_COUNT, LABELICONS_COUNT);

				box.left = box.top = 0;
				box.right = box.bottom = bmi.bmiHeader.biWidth;
				SetTextColor(hMemDC, C_WindowText);

				backcolor = TreeView_GetBkColor(hItemTreeWnd);
				hBackBrush = CreateSolidBrush(backcolor);
				SetBkColor(hMemDC, backcolor);

				for ( i = 0 ; i < LABELICONS_COUNT ; i++ ){
					hOldBmp = SelectObject(hMemDC, hBMP);

					FillRect(hMemDC, &box, hBackBrush);
					GetTextExtentPoint32(hMemDC, TreeImageChars + i, 1, &chrsize);
					TextOut(hMemDC,
							(tm.tmHeight - chrsize.cx) / 2,
							-tm.tmInternalLeading,
							TreeImageChars + i, 1);

					if ( i == ICON_CHECKON ){ // チェックボックスのチェックを別個描画
						SetBkMode(hMemDC, TRANSPARENT);
						GetTextExtentPoint32(hMemDC, TreeImageChars + IMAGE_CHECKMARK, 1, &chrsize);
						TextOut(hMemDC,
								(tm.tmHeight - chrsize.cx) / 2 + 1,
								-tm.tmInternalLeading,
								TreeImageChars + IMAGE_CHECKMARK, 1);
					}
					SelectObject(hMemDC, hOldBmp);
					ImageList_Add(hImage, hBMP, NULL);
				}
				DeleteObject(hBackBrush);
				DeleteObject(hBMP);
			}
		}
		SelectObject(hMemDC, hOldFont);
		DeleteObject(hControlFont);
		DeleteDC(hMemDC);
		ReleaseDC(hDlg, hDC);
	}

	if ( hImage == NULL ){ // 内蔵画像を使用する
		hImage = ImageList_LoadImage(hInst, MAKEINTRESOURCE(BUTTONIMAGE),
				16, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_DEFAULTCOLOR);
	}
	hImage = (HIMAGELIST)SendMessage(hItemTreeWnd, TVM_SETIMAGELIST, (WPARAM)TVSIL_NORMAL, (LPARAM)hImage);
	if ( hImage != NULL ) ImageList_Destroy(hImage);
}

void LoadList(HWND hDlg)
{
	LoadCustomList();
	hIndexTreeWnd = GetDlgItem(hDlg, IDT_GENERAL);
	hItemTreeWnd = GetDlgItem(hDlg, IDT_GENERALITEM);

	GetCustData(T("X_pmc"), &X_pmc, sizeof(X_pmc));
	if ( X_pmc[pmc_mode] > 0 ){
		EnterTouchMode(hDlg);
	}else if ( X_pmc[pmc_mode] < 0 ){
		if ( GetSystemMetrics(SM_MAXIMUMTOUCHES) != 0 ){
			OldIndexTreeProc =(WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg,
					IDT_GENERAL), GWLP_WNDPROC, (LONG_PTR)IndexTreeProc);
			OldItemTreeProc =(WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg,
					IDT_GENERALITEM), GWLP_WNDPROC, (LONG_PTR)ItemTreeProc);
		}
	}
	LoadButtonImages(hDlg);
	LoadListTree(FALSE);
#ifdef WINEGCC // Windows では↑で右側も表示されるので不要
	CustomItemListOffset = 2;
	CustomItemList = SearchList(2);
	LoadListTree(TRUE);
#endif
	// 編集関係を用意
	hEditWnd = CreateWindow(EDIT, NilStr,
			WS_BORDER | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
			0, 0, FLOAT_WIDTH, FLOAT_HEIGHT, hDlg,
			CHILDWNDID(IDE_FLOAT), hInst, NULL);
	EnableTextChangeNotify(hEditWnd);
	hSpinWnd = CreateWindow(T("msctls_updown32"), NilStr,
			UDS_SETBUDDYINT | UDS_AUTOBUDDY | UDS_ARROWKEYS |
			UDS_NOTHOUSANDS |
			WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
			0, 0, FLOAT_WIDTH, FLOAT_HEIGHT, hDlg,
			CHILDWNDID(IDU_FLOATUPDOWN), hInst, NULL);
	SendMessage(hSpinWnd, UDM_SETRANGE, 0, TMAKELPARAM(0, 0x7fff));
	SendMessage(hSpinWnd, UDM_SETRANGE32, 0, 0x7fffffff);
	hButtonWnd = CreateWindow(BUTTON, MessageText(MES_VREF),
			WS_BORDER | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
			0, 0, FLOAT_BUTTONWIDTH, FLOAT_HEIGHT, hDlg,
			CHILDWNDID(IDB_FLOAT), hInst, NULL);
	FixUxTheme(hButtonWnd, BUTTON);
}

void ChangeCustEnv(HWND hDlg)
{
	HWND hPropWnd;
	int old_uxt;

	old_uxt = X_uxt[0];
	GetCustData(T("X_uxt"), &X_uxt, sizeof(X_uxt));
	// Dark から Off にすると 色反映されないところがあるので UXT_GDI を経由する
	if ( (X_uxt[0] == UXT_OFF) && (old_uxt == UXT_DARK) ){
		X_uxt[0] = UXT_GDI;
		SetCustData(T("X_uxt"), &X_uxt, sizeof(X_uxt));
		PPxCommonExtCommand(K_UxTheme, KUT_LOADCUST);
		hPropWnd = GetParent(hDlg);
		LocalizeDialogText(hPropWnd, 0);
		X_uxt[0] = UXT_OFF;
		SetCustData(T("X_uxt"), &X_uxt, sizeof(X_uxt));
	}

	PPxCommonCommand(hDlg, 0, K_Lcust);
	X_uxt[0] = PPxCommonExtCommand(K_UxTheme, KUT_LOADCUST);
	GUILoadCust();
	if ( hIndexTreeWnd != NULL ){
		TreeView_DeleteAllItems(hIndexTreeWnd);
		LoadListTree(FALSE);
		LoadListTree(TRUE);
	}
	hPropWnd = GetParent(hDlg);
	SetTabTitles(hPropWnd);
	LocalizeDialogText(hPropWnd, 0);
	if ( (hIndexTreeWnd != NULL) && (X_uxt[0] != old_uxt) ){
		if ( X_uxt[0] == UXT_OFF ){
			SendMessage(hIndexTreeWnd, TVM_SETBKCOLOR, 0, (LPARAM)WinColors.c.DialogBack);
			SendMessage(hIndexTreeWnd, TVM_SETTEXTCOLOR, 0, (LPARAM)WinColors.c.DialogText);
			SendMessage(hItemTreeWnd, TVM_SETBKCOLOR, 0, (LPARAM)WinColors.c.DialogBack);
			SendMessage(hItemTreeWnd, TVM_SETTEXTCOLOR, 0, (LPARAM)WinColors.c.DialogText);
		}
		LoadButtonImages(hDlg);
		InitPropSheetsUxtheme(hDlg);
	}
	InvalidateRect(hPropWnd, NULL, TRUE);
}

#define LANGMENU_AUTO 1
#define LANGMENU_LOADFILE 2
#define LANGMENU_ONLINELIST 3
#define LANGMENU_DOWNLOAD B15

const TCHAR UrlLangList[] = T(TOROsWEB) T("/checkver/checklang.cgi?name=ppx&lcid=%04X");
const TCHAR UrlLangWeb[] = T(TOROsWEB) T("/checkver/checklang.cgi?name=ppxweb&lcid=%04X");

#if !NODLL
void USEFASTCALL AppendMenuString(HMENU hMenu, UINT id, const TCHAR *string)
{
	AppendMenu(hMenu, MF_ES, id, MessageText(string));
}
#endif

void USEFASTCALL AppendMenuLang(HMENU hPopupMenu, int lcid, int index, const TCHAR *string)
{
	TCHAR buf[MAX_PATH];

	LangName(buf, lcid, string);
	AppendMenuString(hPopupMenu, index, buf);
}

void LangMenu(HWND hDlg, ITEMSTRUCT *item)
{
	HMENU hPopupMenu;
	int index;
	POINT pos;
	int count = 0;
	TCHAR buf[CUST_NAME_LENGTH];
	ThSTRUCT th;
	TCHAR *onlinelist;

	ThInit(&th);
	hPopupMenu = CreatePopupMenu();

	AppendMenuString(hPopupMenu, LANGMENU_AUTO, MES_SAUT);
	// 内蔵
	AppendMenuLang(hPopupMenu, LCID_ENGLISH, LCID_ENGLISH, T(" - embedded"));

	// カスタマイズ済み
	while( EnumCustData(count++, buf, buf, 0) >= 0 ){
		if ( (buf[0] == 'M') && (buf[1] == 'e') && (buf[2] == 's') ){
			const TCHAR *ptr;
			int lcid;

			buf[2] = 'H';
			ptr = buf + 2;
			lcid = GetNumber(&ptr);
			if ( lcid == LCID_ENGLISH ){
				DeleteMenu(hPopupMenu, LCID_ENGLISH, MF_BYCOMMAND);
			}
			AppendMenuLang(hPopupMenu, lcid, lcid, T(" - included"));
		}
	}
	if ( !IsExistCustData(T("Mes0411")) ){
		AppendMenuLang(hPopupMenu, LCID_JAPANESE, LCID_JAPANESE, T(" - embedded"));
	}
	AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);

	{ // オンラインのリストから取得
		thprintf(buf, TSIZEOF(buf), UrlLangList, LOWORD(GetUserDefaultLCID()));
		if ( GetImageByHttp(buf, &th) == FALSE ){
			AppendMenuString(hPopupMenu, 0, MES_VDLE);
		}
		if ( th.bottom != NULL ){
		#ifdef UNICODE
			DWORD len;

			len = AnsiToUnicode((char *)th.bottom, NULL, 0) + 1;
			onlinelist = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
			if ( onlinelist != NULL ){
				AnsiToUnicode((char *)th.bottom, onlinelist, len);
			}
		#else
			onlinelist = (char *)th.bottom;
		#endif
			if ( onlinelist != NULL ) onlinelist = tstrstr(onlinelist, T("\r\n\r\n"));
			if ( onlinelist != NULL ){
				const TCHAR *param;

				onlinelist += 4;
				param = onlinelist;
				while ( *param != '\0' ){
					TCHAR *last, *name;

					last = tstrchr(param, '\r');
					if ( last != NULL ){
						*last++ = '\0';
						if ( *last == '\n' ) last++;
					}
					name = tstrrchr(param, '/');
					if ( name != NULL ){
						for (;;){
							if ( *name == '\0' ) break;
							if ( !Isxdigit(*name) ){
								name++;
								continue;
							}
							{
								TCHAR backup;
								const TCHAR *ptr;
								int lcid;

								name--;
								ptr = name;
								backup = *ptr;
								*name = 'H';
								lcid = GetNumber(&ptr);
								AppendMenuLang(hPopupMenu, lcid, (param - onlinelist) | LANGMENU_DOWNLOAD,
										(backup == 'L') ? T(" - download") :
											T(" (download sample for test)") );
								*name = backup;
							}
							break;
						}
					}
					if ( last == NULL ) break;
					param = last;
				}
				AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);
			}
		}
	}

	AppendMenuString(hPopupMenu, LANGMENU_ONLINELIST, MES_VOLF);
	AppendMenuString(hPopupMenu, LANGMENU_LOADFILE, MES_VOPF);

	GetMessagePosPoint(pos);
	index = CustTrackPopupMenu(hDlg, hPopupMenu, &pos);
	DestroyMenu(hPopupMenu);
	if ( index == LANGMENU_LOADFILE ){ // ファイル読み込み
		TCHAR LangFileName[MAX_PATH];
		OPENFILENAME ofile = { sizeof(ofile), NULL, NULL, GetFileExtsStr, NULL,
		0, 0, NULL, 0, NULL, 0, NULL, NULL, OFN_HIDEREADONLY | OFN_SHAREAWARE,
		0, 0, NilStr, 0, NULL, NULL OPENFILEEXTDEFINE };

		ofile.hwndOwner = hDlg;
		ofile.lpstrFile = LangFileName;
		ofile.nMaxFile = TSIZEOF(LangFileName);
		thprintf(LangFileName, TSIZEOF(LangFileName), T("L%04X.txt"), LOWORD(GetUserDefaultLCID()));

		if ( GetOpenFileName(&ofile) ){
			CustStore(LangFileName, PPXCUSTMODE_STORE | PPXCUSTMODE_LANG_FILE, hDlg);
			GetCustData(T("X_LANG"), &UseLcid, sizeof(UseLcid));
			ChangeCustEnv(hDlg);
		}
	}else if ( index == LANGMENU_AUTO ){ // 自動選択
		item->d.num.data = 0;
		SetData(item, &item->d.num.data, item->d.num.size);
		ChangeCustEnv(hDlg);
	}else if ( index == LANGMENU_ONLINELIST ){ // オンラインのリストへジャンプ
		thprintf(buf, TSIZEOF(buf), UrlLangWeb, LOWORD(GetUserDefaultLCID()));
		ComExec(hDlg, buf, NULL);
	}else if ( index & LANGMENU_DOWNLOAD ){ // ダウンロード
		ThSTRUCT thlang;
		const TCHAR *url;

		#pragma warning(suppress: 6001) // LANGMENU_DOWNLOAD 付きは、onlinelist を使用済み
		url = onlinelist + (index & ~LANGMENU_DOWNLOAD);
		GetImageByHttp(url, &thlang);
		if ( thlang.bottom != NULL ){
			char *filetop;

			filetop = strstr(thlang.bottom, "\r\n\r\n");
			if ( filetop != NULL ){
				DWORD size;

				filetop += 4;
				size = thlang.top - (filetop - thlang.bottom);

				if ( VerifyImage((BYTE *)filetop, size - 1) == VERIFYZIP_FAILED ){
					XMessage(hDlg, NULL, XM_FaERRd, MES_EVFA);
				}else{
					int fix;
					TCHAR *log = NULL;
					TCHAR *newimage;

					fix = FixTextImage(filetop, size, &newimage, 0);
					if ( fix < 0 ) filetop = (char *)newimage;

					if ( PPcustCStore((TCHAR *)filetop, (TCHAR *)filetop + tstrlen((TCHAR *)filetop), PPXCUSTMODE_STORE | PPXCUSTMODE_LANG_FILE, &log, NULL) == 0 ){
						XMessage(hDlg, NULL, XM_FaERRd, T("download file store fault"));
					}else{
						GetCustData(T("X_LANG"), &UseLcid, sizeof(UseLcid));
						ChangeCustEnv(hDlg);
					}
					if ( log != NULL ){
						if ( tstrlen(log) >= 4 ) PPEui(NULL, NilStr, log);
						HeapFree(GetProcessHeap(), 0, log);
					}

					if ( fix < 0 ) HeapFree(GetProcessHeap(), 0, newimage);
				}
			}
		}
		ThFree(&thlang);
	}else if ( index > 0x400 ){ // 切替のみ
		if ( GetShiftKey() & K_s ){
			if ( PMessageBox(hDlg, MES_QRLA, StrCustTitle, MB_YESNO) == IDYES ){
				thprintf(buf, TSIZEOF(buf), T("Mes%04X"), index);
				DeleteCustData(buf);
			}
		}else{
			item->d.num.data = index;
			SetData(item, &item->d.num.data, item->d.num.size);
			ChangeCustEnv(hDlg);
		}
	}
	ThFree(&th);
}

void DispFix(HWND hTwnd, HTREEITEM titem, ITEMSTRUCT *item)
{
	TCHAR disp[0x1000];
	TV_ITEM tvi;

	tvi.mask = TVIF_TEXT;
	MakeDisplayText(disp, item, &tvi);
	tvi.hItem = titem;
	tvi.pszText = disp;
	tvi.cchTextMax = tstrlen32(disp);
	TreeView_SetItem(hTwnd, &tvi);
}

void ModifyEditItem(HWND hWnd)
{
	TCHAR editbuf[VFPS * 2];
	const TCHAR *p;

	GetWindowText(hEditWnd, editbuf, TSIZEOF(editbuf));
	switch ( edititem.itemtype ){
		case ITEM_INT:
		case ITEM_DWORD:
		case ITEM_WORD:
		case ITEM_BYTE:
			p = editbuf;
			edititem.d.num.data = GetNumber(&p);
			SetData(&edititem, &edititem.d.num.data, edititem.d.num.size);
			break;
		case ITEM_STRING:
		case ITEM_EXT_STRING:
			if ( edititem.option == ITEMOPTION_NOMACROSTR ){
				if ( tstrchr(editbuf, '%') != NULL ){
					SetDlgItemText(hWnd, IDE_FIND, MessageText(WarnNoMacroMsg));
				}else{
					SetDlgItemText(hWnd, IDE_FIND, NilStr);
				}
			}
			if ( edititem.itemtype == ITEM_EXT_STRING ){
				edititem.d.str.data[0] = EXTCMD_CMD;
				tstrcpy(edititem.d.str.data + 1, editbuf);
				SetData(&edititem, &edititem.d.str.data,
						TSTRSIZE32(edititem.d.str.data));
				tstrcpy(edititem.d.str.data, editbuf);
			}else{
				tstrcpy(edititem.d.str.data, editbuf);
				SetData(&edititem, &edititem.d.str.data,
						TSTRSIZE32(edititem.d.str.data));
			}
			break;
//		default:
	}
}

void RefreshItems(HWND hTwnd, ITEMSTRUCT *item, HTREEITEM hItem1)
{
	HTREEITEM hItem;
	TV_ITEM tvi;
	ITEMSTRUCT enumitem;
	const TCHAR *p;

	if ( item->layer <= 2 ){
		DispFix(hTwnd, hItem1, item);
		return;
	}
	hItem = TreeView_GetParent(hTwnd, hItem1);
	hItem = TreeView_GetChild(hTwnd, hItem);

	while ( hItem != NULL ){
		tvi.hItem = hItem;
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hTwnd, &tvi);
		p = SearchList((int)tvi.lParam);
		if ( p == NULL ) break;
		GetCustItem(&enumitem, p, TRUE);
		DispFix(hTwnd, hItem, &enumitem);
		hItem = TreeView_GetNextSibling(hTwnd, hItem);
	}
}

int used_edit = 0;
DWORD used_tick;
void CloseEdit(HWND hDlg, HWND hTwnd)
{
	if ( !useedit ) return;
	used_edit = useedit;
	used_tick = GetTickCount();
	ShowWindow(hEditWnd, SW_HIDE);
	ShowWindow(hSpinWnd, SW_HIDE);
	ShowWindow(hButtonWnd, SW_HIDE);
	useedit = 0;
	edititem.edit = FALSE;
	ModifyEditItem(hDlg);
	RefreshItems(hTwnd, &edititem, hedititem);
}

void SetEditWindow(HWND hDlg, HWND hTwnd, ITEMSTRUCT *item, HTREEITEM hTitem)
{
	RECT box;
	POINT pos;

	item->edit = TRUE;
	DispFix(hTwnd, hTitem, item);
	TreeView_GetItemRect(hTwnd, hTitem, &box, TRUE);
	pos.x = box.right;
	pos.y = (box.top + box.bottom) / 2 - (FLOAT_HEIGHT / 2);
	ClientToScreen(hTwnd, &pos);
	ScreenToClient(hDlg, &pos);

	SetWindowPos(hEditWnd, HWND_TOP, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	if ( item->edittype != EDIT_EDIT ){
		SetWindowPos(hButtonWnd, HWND_TOP,
				pos.x + FLOAT_WIDTH, pos.y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);
	}else{
		if ( (item->itemtype != ITEM_STRING) && (item->itemtype != ITEM_EXT_STRING) ){
			SetWindowPos(hSpinWnd, HWND_TOP,
				pos.x + FLOAT_WIDTH, pos.y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);
		}
	}
	PostMessage(hDlg, WM_COMMAND, IDM_SETEDITFOCUS, 0);
}

void SelectItem_Font(HWND hDlg, HWND hTwnd, HTREEITEM titem, ITEMSTRUCT *item)
{
	CHOOSEFONT cfont;
	int GDIdpi, fontdpi, pix;

// ※仮想中はdpiの変化無し
	memset(&cfont, 0, sizeof(CHOOSEFONT));
	cfont.lStructSize = sizeof(CHOOSEFONT);
	cfont.hwndOwner = hDlg;
	// cfont.hDC = NULL;
	cfont.lpLogFont = &item->d.fontdata.font;
	cfont.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_FORCEFONTEXIST |
			CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT;
	// cfont.rgbColors = 0;

	// (未設定) なら、初期値を設定する
	if ( item->d.fontdata.font.lfFaceName[0] == '(' ){
		const TCHAR *pos = tstrchr(StrFontList, item->keyname[2]);

		item->d.fontdata.font.lfFaceName[0] = '\0';
		if ( pos != NULL ){
			GetPPxFont((int)(pos - StrFontList), PPxCommonExtCommand(K_GETDISPDPI, (WPARAM)hDlg), &item->d.fontdata);
		}
	}
	GDIdpi = GetGDIFontDPI(hDlg);

	pix = (item->d.fontdata.font.lfHeight >= 0) ?
			item->d.fontdata.font.lfHeight : -item->d.fontdata.font.lfHeight;

	// ダイアログは Point を Pixel に変換する
	if ( tstrcmp(item->keyname, T("F_dlg")) == 0 ){
		pix = (pix * GDIdpi + (DEFAULT_DTP_DPI / 2)) / DEFAULT_DTP_DPI;
		item->d.fontdata.font.lfHeight = -pix;
	}else{ // dpi 調整
		fontdpi = item->d.fontdata.dpi;
		if ( fontdpi == 0 ) fontdpi = DEFAULT_WIN_DPI;
		pix = (pix * GDIdpi + (fontdpi / 2)) / fontdpi;
		item->d.fontdata.font.lfHeight = (item->d.fontdata.font.lfHeight >= 0) ? pix : -pix;
	}

	if ( IsTrue(ChooseFont(&cfont)) ){
		GDIdpi = GetGDIFontDPI(hDlg); // ダイアログ中で解像度が変わった時用
		// ダイアログは Pixel を Point に変換する
		if ( tstrcmp(item->keyname, T("F_dlg")) == 0 ){
			if ( item->d.fontdata.font.lfHeight < 0 ){ // 正に揃える(Wine / ReactOS 対策)
				item->d.fontdata.font.lfHeight = -item->d.fontdata.font.lfHeight;
			}
			item->d.fontdata.font.lfHeight = (item->d.fontdata.font.lfHeight * DEFAULT_DTP_DPI + (GDIdpi / 2)) / GDIdpi;
		}
		item->d.fontdata.dpi = GDIdpi;
		SetData(item, &item->d.fontdata, sizeof(item->d.fontdata));
		RefreshItems(hTwnd, item, titem);
		Changed(hDlg);
	}
}

BOOL SelectItem(HWND hDlg, HWND hTwnd, HTREEITEM titem, int linenumber)
{
	ITEMSTRUCT item;
	const TCHAR *listp;

	listp = SearchList(linenumber);
	if ( listp == NULL ) return FALSE;
	GetCustItem(&item, listp, TRUE);
	switch ( item.edittype ){
		case EDIT_NONE:			// ラベル／折り畳み処理
		case EDIT_DOCKLABEL:
			if ( (item.itemtype == ITEM_INDEXLABEL) &&
				(CustomItemListOffset != (linenumber + 1)) ){
				CustomItemListOffset = linenumber + 1;
				CustomItemList = SearchList(linenumber + 1);
				LoadListTree(TRUE);
				return TRUE;
			}

			if ( (item.layer == 1) || (item.edittype == EDIT_DOCKLABEL) ){
				TreeView_Expand(hTwnd, titem, TVE_TOGGLE);
			}
			return TRUE;

		case EDIT_CHECK:		// チェックボックス／更新も行う
			if ( item.itemtype == ITEM_STRING ){
				if ( !tstrcmp(item.d.str.data, item.d.str.setdata) ){
					thprintf(item.d.str.data, 12, T("%d"), item.d.num.falsedata);
				}else{
					tstrcpy(item.d.str.data, item.d.str.setdata);
				}
				SetData(&item, &item.d.str.data, TSTRSIZE32(item.d.str.data));
			}else{
				if ( item.d.num.mask == IITEM_VALUE ){	// 値変更
					item.d.num.data = ( item.d.num.data == item.d.num.falsedata ) ?
						item.d.num.truedata : item.d.num.falsedata;
				}else{							// bit 変更
					item.d.num.data ^= item.d.num.truedata;
				}
				SetData(&item, &item.d.num.data, item.d.num.size);
			}
			RefreshItems(hTwnd, &item, titem);
			Changed(hDlg);
			return TRUE;

		case EDIT_RADIO:		// ラジオボックス／更新も行う
			switch ( item.itemtype ){
				case ITEM_INT:
				case ITEM_DWORD:
				case ITEM_WORD:
				case ITEM_BYTE:
					if ( item.d.num.data == item.d.num.truedata ) return FALSE;
					Changed(hDlg);
					item.d.num.data = item.d.num.truedata;
					SetData(&item, &item.d.num.data, item.d.num.size);
					break;
				case ITEM_EXT_STRING:
				case ITEM_STRING:
					if ( !tstrcmp(item.d.str.data, item.d.str.setdata) ){
						return FALSE;
					}
					Changed(hDlg);
					if ( item.itemtype == ITEM_EXT_STRING ){
						item.d.str.data[0] = EXTCMD_CMD;
						tstrcpy(item.d.str.data + 1, item.d.str.setdata);
						SetData(&item, &item.d.str.data, TSTRSIZE32(item.d.str.data));
						tstrcpy(item.d.str.data, item.d.str.setdata);
					}else{
						tstrcpy(item.d.str.data, item.d.str.setdata);
						SetData(&item, &item.d.str.data, TSTRSIZE32(item.d.str.data));
					}
					break;
//				default:
			}
			if ( item.option == ITEMOPTION_CHANGENOW ){
				ChangeCustEnv(hDlg);
			}else{
				RefreshItems(hTwnd, &item, titem);
			}
			break;

		case EDIT_FILE:			// ファイル名ボックス／編集開始
		case EDIT_DIR:			// ディレクトリ／編集開始
		case EDIT_EDIT: {		// エディットボックス／編集開始
			TCHAR text[0x1000];

			// 直前に同じ箇所を表示していた場合は、閉じたままを維持する
			if ( (used_edit == linenumber) &&
				 ((GetTickCount() - used_tick) < 300) ){
				break;
			}

			SendMessage(hEditWnd, EM_LIMITTEXT, VFPS - 1, 0);
			MakeEditText(text, &item);
			SetWindowText(hEditWnd, text);

			SetEditWindow(hDlg, hTwnd, &item, titem);

			useedit = linenumber;
			hedititem = titem;
			edititem = item;
			edititem.keyname = edititem.itemname +
					(item.keyname - item.itemname);
			if ( item.subname ){
				edititem.subname = edititem.itemname +
						(item.subname - item.itemname);
			}
			break;
		}
		case EDIT_KEY:
		case EDIT_ppchotkey:
		{							// キー／編集＆更新
			TCHAR temp[64];
			const TCHAR *p;

			PutKeyCode(temp, item.d.num.data);
			if ( KeyInput(GetParent(hDlg), temp) <= 0 ){
				if ( item.edittype != EDIT_ppchotkey ){
					return TRUE;
				}
				// EDIT_ppchotkey で、既に割当てがあるときは削除するか確認
				if ( (item.d.num.def == 0) ||
					 (PMessageBox(hDlg, DelKeyMsg, StrCustTitle, MB_YESNO) != IDYES) ){
					return TRUE;
				}
				item.d.num.data = 0;
			}else{
				p = temp;
				item.d.num.data = (WORD)GetKeyCode(&p);
			}
			if ( item.edittype == EDIT_ppchotkey ){
				SetPPcHotKey(&item);
			}else{
				SetData(&item, &item.d.num.data, sizeof item.d.num.data);
			}
			RefreshItems(hTwnd, &item, titem);
			Changed(hDlg);
			return TRUE;
		}
		case EDIT_FONT:		// フォント／編集＆更新
			SelectItem_Font(hDlg, hTwnd, titem, &item);
			return TRUE;

		case EDIT_LANG:
			LangMenu(hDlg, &item);
			return TRUE;
//		default:
	}
	return FALSE;
}

void TreeHelp(HWND hDlg, HWND hTwnd, BOOL point)
{
	TV_ITEM tvi;
	ITEMSTRUCT item;
	const TCHAR *p;
	TV_HITTESTINFO hit;

	if ( point ){ // マウスなど
		GetMessagePosPoint(hit.pt);
		ScreenToClient(hTwnd, &hit.pt);
		tvi.hItem = TreeView_HitTest(hTwnd, &hit);
	}else{ // キー操作
		tvi.hItem = TreeView_GetSelection(hTwnd);
	}

	tvi.mask = TVIF_PARAM;
	TreeView_GetItem(hTwnd, &tvi);

	p = SearchList((int)(tvi.lParam));
	if ( p == NULL ) return;

	GetCustItem(&item, p, FALSE);
	if ( item.keyname == NULL ){
		int layer;

		layer = item.layer;
		if ( layer == 1 ) return;
		p = SearchList((int)(tvi.lParam) + 1);
		if ( p == NULL ) return;
		GetCustItem(&item, p, FALSE);
		if ( item.keyname == NULL ) return;
		if ( item.layer != (layer + 1) ) return;
	}
	if ( (item.keyname[0] == 'F') && (item.keyname[1] == '_') ){
		HMENU hPopMenu;
		POINT pos;
		int index;

		hPopMenu = CreatePopupMenu();
		AppendMenuString(hPopMenu, 1, StrMenuDefault);
		AppendMenuString(hPopMenu, 2, StrMenuHelp);
		GetMessagePosPoint(pos);
		index = CustTrackPopupMenu(hTwnd, hPopMenu, &pos);
		DestroyMenu(hPopMenu);
		switch ( index ){
			case 1: // 初期化
				if ( PMessageBox(hTwnd, StrFontDefault, StrCustTitle, MB_YESNO) == IDYES ){
					DeleteCustData(item.keyname);
					GetCustItem(&item, p, TRUE);
					RefreshItems(hTwnd, &item, tvi.hItem);
					Changed(hDlg);
				}
				return;

			case 2: // help
				break;

			default:
				return;
		}
	}
	PPxHelp(hTwnd, HELP_KEY, (DWORD_PTR)item.keyname);
}

void ModifyItem2(HWND hDlg, HWND hTwnd)
{
	TV_ITEM tvi;

	RemoveControlKeydown(hDlg);
	tvi.hItem = TreeView_GetSelection(hTwnd);
	tvi.mask = TVIF_PARAM;
	TreeView_GetItem(hTwnd, &tvi);
	SelectItem(hDlg, hTwnd, tvi.hItem, (int)(tvi.lParam));
}

#ifndef PSN_QUERYINITIALFOCUS
#define PSN_QUERYINITIALFOCUS (PSN_FIRST-13)
#endif

INT_PTR GeneralNotify(HWND hDlg, NMHDR *nmh)
{
	switch (nmh->code){
		case PSN_SETACTIVE:
			InitWndIcon(hDlg, IDB_TEST);
			break;

		case PSN_APPLY:
			if ( useedit ){
				HWND hFocus = GetFocus();

				CloseEdit(hDlg, hItemTreeWnd);
				if ( hFocus == hEditWnd ){
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
					SetFocus(hItemTreeWnd);
					break;
				}
			}
		// PSN_HELP へ
		case PSN_HELP:
			DlgSheetProc(hDlg, WM_NOTIFY, 0, (LPARAM)nmh, IDD_GENERAL);
			break;
/*
		case PSN_QUERYINITIALFOCUS:
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)hItemTreeWnd);
			return TRUE;
*/
		case NM_CLICK:
			if ( nmh->hwndFrom == hItemTreeWnd ){
			// おまじない。この時点ではまだカーソル位置のアイテムが
			// 選択されていない。
				PostMessage(hDlg, WM_COMMAND, IDM_RCLICKED, (LPARAM)nmh->hwndFrom);
			}
			break;

		case NM_RCLICK:
			TreeHelp(hDlg, nmh->hwndFrom, TRUE);
			return 1;

		case NM_DBLCLK:
			ModifyItem2(hDlg, nmh->hwndFrom);
			break;

		case TVN_KEYDOWN:
			switch ( ((TV_KEYDOWN *)nmh)->wVKey ){
				case VK_F1:
					TreeHelp(hDlg, nmh->hwndFrom, FALSE);
					break;

				case VK_SPACE:
					ModifyItem2(hDlg, nmh->hwndFrom);
					break;
			}
			break;

		case TVN_SELCHANGED:
			if ( nmh->hwndFrom == hIndexTreeWnd ){
				PostMessage(hDlg, WM_COMMAND, IDM_RCLICKED, (LPARAM)nmh->hwndFrom);
			}
			break;

//		default:
	}
	return 1;
}

void PushFloatButton_File(HWND hDlg)
{
	TCHAR Name[VFPS];
	TCHAR Path[VFPS], *p;

	OPENFILENAME ofile = {sizeof(ofile), NULL, NULL, GetFileExtsStr, NULL, 0, 0,
		NULL, TSIZEOF(Name), NULL, 0, NULL, NULL,
		OFN_HIDEREADONLY | OFN_SHAREAWARE, 0, 0, NilStr, 0, NULL, NULL OPENFILEEXTDEFINE };

	ofile.hwndOwner = hDlg;
	ofile.lpstrFile = Name;
	ofile.lpstrInitialDir = Path;

	GetWindowText(hEditWnd, Path, TSIZEOF(Path));
	p = (Path[0] == '\"') ? (Path + 1) : Path;
	if ( *p != '<' ){
		p = VFSFindLastEntry(p);
		tstrcpy(Name, (*p == '\\') ? p + 1 : p);
		*p = '\0';
		p = tstrchr(Name, '\"');
		if ( p != NULL ) *p = '\0';
	}else{
		Name[0] = '\0';
	}
	if ( IsTrue(GetOpenFileName(&ofile)) ){
		if ( (edititem.keyname[0] == 'A') && (tstrchr(Name, ' ') != NULL) ){
			VFSFullPath(NULL, Name, Path);
			thprintf(Path, TSIZEOF(Path), T("\"%s\""), Name);
		}else{
			VFSFullPath(Path, Name, Path);
		}
		SetWindowText(hEditWnd, Path);
	}
	RefreshItems(hIndexTreeWnd, &edititem, hedititem);
}

void PushFloatButton_Dir(HWND hDlg)
{
	TCHAR Path[VFPS];

	TINPUT tinput;

	tinput.hOwnerWnd	= hDlg;
	tinput.hWtype		= PPXH_DIR;
	tinput.hRtype		= PPXH_DIR_R;
	tinput.title		= NilStr;
	tinput.buff			= Path;
	tinput.size			= TSIZEOF(Path);
	tinput.flag			= TIEX_REFTREE | TIEX_SINGLEREF | TIEX_REFMODE;
	tinput.firstC		= 0;
	tinput.lastC		= EC_LAST;

	GetWindowText(hEditWnd, Path, TSIZEOF(Path));

	if ( tInputEx(&tinput) > 0 ) SetWindowText(hEditWnd, Path);
	RefreshItems(hIndexTreeWnd, &edititem, hedititem);
}

BOOL FindItem(HWND hTreeWnd, HTREEITEM hItem, int findline, HTREEITEM *hFoundItem)
{
	while ( hItem != NULL ){
		TV_ITEM tvi;

		tvi.hItem = hItem;
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hTreeWnd, &tvi);

		if ( tvi.lParam == findline ){
			*hFoundItem = hItem;
			SendMessage(hTreeWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hItem);
			return TRUE;
		}
		if ( IsTrue(FindItem(hTreeWnd, TreeView_GetChild(hTreeWnd, hItem), findline, hFoundItem)) ){
			return TRUE;
		}
		hItem = TreeView_GetNextSibling(hTreeWnd, hItem);
	}
	return FALSE;
}

void FindKeyword(HWND hDlg)
{
	TCHAR findtext[VFPS];
	TV_ITEM tvi;
	int firstline = 1;
	const TCHAR *posptr;

	int line, indexline;
	HTREEITEM hItem;

	findtext[0] = '\0';
	if ( FirstTypeName != NULL ){
		tstrcpy(findtext, FirstTypeName);
		FirstTypeName = NULL;
		FirstItemName = NULL;
		SetFocus(hItemTreeWnd);
	}else{
		GetControlText(hDlg, IDE_FIND, findtext, TSIZEOF(findtext));
	}
	if ( findtext[0] == '\0' ){
		SetFocus(GetDlgItem(hDlg, IDE_FIND));
		return;
	}
	// index の位置を求める
	tvi.hItem = TreeView_GetSelection(hIndexTreeWnd);
	if ( tvi.hItem != NULL ){
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hIndexTreeWnd, &tvi);
		indexline = (int)tvi.lParam;
	}else{
		indexline = 1;
	}
	// item の位置を求める
	tvi.hItem = TreeView_GetSelection(hItemTreeWnd);
	if ( tvi.hItem == NULL ){
		tvi.hItem = TreeView_GetRoot(hItemTreeWnd);
	}
	if ( tvi.hItem != NULL ){
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hItemTreeWnd, &tvi);
		posptr = SearchList((int)tvi.lParam);
		if ( (posptr != NULL) /*&& (tstristr(posptr, findtext) != NULL)*/ ){
			firstline = (int)tvi.lParam;
		}
	}
	if ( firstline == 1 ) posptr = CustomList;
	line = firstline;

	// 全般タグ内
	for ( ;; ){
		posptr += tstrlen(posptr) + 1;
		line++;

		if ( *posptr == '\0' ){
			line = 1;
			posptr = CustomList;
		}
		if ( line == firstline ) break; // 一周した

		if ( *posptr == '.' ){	// index だった
			indexline = line;
		}else{ // item
			if ( tstristr(posptr, findtext) != NULL ){
				// index の位置を求める
				if ( IsTrue(FindItem(hIndexTreeWnd, TreeView_GetRoot(hIndexTreeWnd), indexline, &hItem)) ){
					CustomItemListOffset = indexline + 1;
					CustomItemList = SearchList(indexline + 1);
					LoadListTree(TRUE);
					// item の位置を求める
					FindItem(hItemTreeWnd, TreeView_GetRoot(hItemTreeWnd), line, &hItem);
				}
				return;
			}
		}
	}
	// その他タグ内
	{
		const struct EtcLabelsStruct *el;

		for ( el = EtcLabels ; el->name != NULL ; el++ ){
			if ( tstristr(el->name, findtext) || tstristr(el->key, findtext) ){
				PropSheet_SetCurSel(GetParent(hDlg), NULL, 9);
			}
		}
	}
}

LRESULT CALLBACK FindBoxProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if ( iMsg == WM_GETDLGCODE ) return DLGC_WANTALLKEYS;
	if ( iMsg == WM_KEYDOWN ){
		if ( wParam == VK_TAB ){
			SetFocus(GetNextDlgTabItem(GetParent(hWnd), hWnd,
					GetKeyState(VK_SHIFT) < 0));
			return 0;
		}
		if ( wParam == VK_RETURN ){
			FindKeyword(GetParent(hWnd));
			return 0;
		}
		if ( wParam == VK_ESCAPE ){
			SetFocus(GetNextDlgTabItem(GetParent(hWnd), hWnd, TRUE));
		}
	}
	return CallWindowProc(OldFindBoxProc, hWnd, iMsg, wParam, lParam);
}

INT_PTR CALLBACK GeneralPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg){
		case WM_INITDIALOG:
			InitPropSheetsUxtheme(hDlg);
			SendDlgItemMessage(hDlg, IDE_FIND, EM_LIMITTEXT, MAX_PATH - 1, 0);
			SendDlgItemMessage(hDlg, IDE_FIND, EM_SETCUEBANNER, 0, (LPARAM)GetBannerText(StrFindInfo));
			OldFindBoxProc =(WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg,
					IDE_FIND), GWLP_WNDPROC, (LONG_PTR)FindBoxProc);
			LoadList(hDlg);
			if ( FirstTypeName == NULL ) break;
			FindKeyword(hDlg);
			return FALSE;
/*
		case WM_DESTROY: {	// １回しか作成しないのでイメージリストの廃棄を省略
			HIMAGELIST hImage;

			hImage = SendMessage(hItemTreeWnd, TVM_SETIMAGELIST, (WPARAM)TVSIL_NORMAL, (LPARAM)NULL);
			if ( hImage != NULL ) ImageList_Destroy(hImage);

		}
*/
		case WM_NOTIFY:
			return GeneralNotify(hDlg, (NMHDR *)lParam );

		case WM_COMMAND:
			if ( wParam == IDM_SETEDITFOCUS ){
				SetFocus(hEditWnd);
				break;
			}
			if ( wParam == IDM_RCLICKED ){
				CloseEdit(hDlg, hItemTreeWnd);
				ModifyItem2(hDlg, (HWND)lParam);
				break;
			}
			if ( (HIWORD(wParam) == EN_CHANGE) &&
				 (useedit != 0) &&
				 (useeditmodify == 0) &&
				 ((HWND)lParam == hEditWnd) ){
				useeditmodify = 1;
				ModifyEditItem(hDlg);
				useeditmodify = 0;
				Changed(hDlg);
				break;
			}
			if ( LOWORD(wParam) == IDB_FLOAT ){
				if ( edititem.edittype == EDIT_FILE ){
					PushFloatButton_File(hDlg);
				}
				if ( edititem.edittype == EDIT_DIR ){
					PushFloatButton_Dir(hDlg);
				}
				break;
			}
			if ( LOWORD(wParam) == IDB_TEST ){
				Test();
				break;
			}
			if ( LOWORD(wParam) == IDB_FIND ){
				FindKeyword(hDlg);
				break;
			}
			break;

		case WM_HELP:
		case WM_CONTEXTMENU:
			//if ( (HWND)wParam == GetDlgItem(hDlg, IDT_GENERALITEM) )
			break;

//		case WM_DESTROY:
			// default へ

		default:
			return DlgSheetProc(hDlg, msg, wParam, lParam, IDD_GENERAL);
	}
	return TRUE;
}
