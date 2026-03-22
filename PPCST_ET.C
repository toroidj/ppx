/*-----------------------------------------------------------------------------
	Paper Plane xUI	customizer	その他 シート
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPCUST.H"
#pragma hdrstop

void EnumEtcItem(HWND hDlg);
void EnumHistoryItem(HWND hDlg);

WORD HistFlag = PPXH_GENERAL;
HWND hEtcTreeWnd;

int seltree, EtcEditFormat;
#define ETCID 0x100

// ヒストリ関連 ---------------------------------------------------------------
struct HistLabelsStruct {
	DWORD flag;
	TCHAR *name;
};
const struct HistLabelsStruct HistLabels[] = {
	{PPXH_GENERAL,		MES_VEHG},
	{PPXH_NUMBER,		MES_VEHN},
	{PPXH_COMMAND,		MES_VEHH},

	{PPXH_DIR,			MES_VEHD},
	{PPXH_FILENAME,		MES_VEHC},
	{PPXH_PATH,			MES_VEHF},

	{PPXH_SEARCH,		MES_VEHS},
	{PPXH_MASK,			MES_VEHM},

	{PPXH_PPCPATH,		MES_VEHP},
	{PPXH_PPVNAME,		MES_VEHV},
	{PPXH_NETPCNAME,	MES_VEHW},
	{PPXH_ROMASTR,		MES_VEHR},

	{PPXH_USER1,		MES_VEHU},
	{PPXH_USER2,		MES_VEHX},
	{0, NULL}
};

// 全般 -----------------------------------------------------------------------
const struct EtcLabelsStruct EtcLabels[] = {
	{MES_VEAB, T("A_exec"),		MES_VEAD, ETC_TEXT},
	{MES_VEDB, T("XC_dset"),	MES_VEDD, ETC_DSET},
	{MES_VESB, T("XC_stat"),	MES_VESC, ETC_INFODISP},
	{MES_VEIA, T("XC_inf1"),	MES_VESC, ETC_INFODISP},
	{MES_VEJA, T("XC_inf2"),	MES_VESC, ETC_INFODISP},
	{MES_VECA, T("MC_celS"),	MES_VECC, ETC_CELLDISP},
	{MES_VENL, T("X_icnl"),		MES_VESC, ETC_TEXT}, // 参照
	{MES_VELA, T("XV_cols"),	MES_VELC, ETC_TEXT},
	{MES_VETA, T("XV_tab"),		MES_VETC, ETC_TEXT},
	{MES_VEOA, T("XV_opts"),	MES_VEOC, ETC_TEXT},
	{MES_VEHA, T("CV_hkey"),	MES_VESC, ETC_HLKEY},
	{MES_VEFA, T("X_fopt"),		MES_VEFC, ETC_NOPARAM},
	{MES_VEMA, T("X_jinfc"),	MES_VEMC, ETC_TEXT}, // 参照
	{MES_VEGA, T("_IDpwd"),		MES_VEGC, ETC_NOPARAM},
	{MES_VEYA, T("_others"),	MES_VEYC, ETC_EXTEXT},
	{MES_VEUA, T("_Command"),	MES_VEUC, ETC_EXTEXT}, // コマンドライン編集
	{MES_VEVA, T("_User"),		MES_VEVC, ETC_EXTEXT}, // コマンドライン編集
	{MES_VEAA, T("P_arc"),		MES_VEAC, ETC_NOPARAM},
	{MES_VEPA, T("_Path"),		MES_VEPC, ETC_TEXT},
	{MES_VENA, T("_WinPos"),	MES_VENC, ETC_WINPOS},
	{MES_VEKA, T("_Execs"),		MES_VEKC, ETC_EXECS},
	{MES_VEEA, T("_Delayed"),	MES_VEEC, ETC_TEXT},
	#ifndef RELEASE
	{T("Message JA-JP"), T("Mes0411"),	T(""), ETC_TEXT},
	{T("IDL cache"), T("#IdlC"), T(""), ETC_NOPARAM},
	#endif
	{NULL, NULL, NULL, 0}
};

const TCHAR StrBrokenData[] = MES_BRKD;

TCHAR *SetCText(TCHAR *dest, const TCHAR *ctext)
{
	return tstpcpy(dest, MessageText(ctext));
}

// ヒストリ ===================================================================
#ifdef UNICODE
void AddListItem_U(HWND hWnd, const WCHAR *text)
{
	TCHAR buf[0x1000];

	strcpyW(buf, text);
	SendMessage(hWnd, LB_ADDSTRING, 0, (LPARAM)buf);
}
#endif

void EnumHistoryItem(HWND hDlg)
{
	int count = 0;
	HWND hListWnd;

	hListWnd = GetDlgItem(hDlg, IDL_EXITEM);
	SendMessage(hListWnd, WM_SETREDRAW, FALSE, 0);
	SendMessage(hListWnd, LB_RESETCONTENT, 0, 0);

	for ( ;; ){
		const TCHAR *histptr;

		UsePPx();
		histptr = EnumHistory(HistFlag, count);
		if ( histptr == NULL ) break;

#ifdef UNICODE
		if ( ALIGNMENT_BITS(histptr) & 1 ){
			AddListItem_U(hListWnd, histptr);
		}else
#endif
			SendMessage(hListWnd, LB_ADDSTRING, 0, (LPARAM)histptr);

		FreePPx();
		count++;
	}
	FreePPx();
	SendMessage(hListWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(hListWnd, NULL, TRUE);
	SetDlgItemText(hDlg, IDS_ETCLIST, MessageText(MES_VELH) );
}

void DeleteHistoryItem(HWND hDlg)
{
	int index;
	TCHAR buf[CMDLINESIZE * 4];
	HWND hListWnd;

	hListWnd = GetDlgItem(hDlg, IDL_EXITEM);
	index = (int)SendMessage(hListWnd, LB_GETCURSEL, 0, 0);
	if ( index == LB_ERR ) return;

	if ( LB_ERR != SendMessage(hListWnd, LB_GETTEXT, (WPARAM)index, (LPARAM)buf)){
		DeleteHistory(HistFlag, buf);
		EnumHistoryItem(hDlg);

		SendMessage(hListWnd, LB_SETCURSEL, (WPARAM)index, 0);
		SendMessage(hListWnd, LB_SETTOPINDEX, (WPARAM)max(index - 6, 0), 0);
	}
}
// 全般 =======================================================================
// 初期化 ---------------------------------------------------------------
void InitEtcTree(HWND hDlg)
{
	HTREEITEM hHisRoot, hTempItem;
	TV_ITEM tvi;
	TV_INSERTSTRUCT tvins;
	const struct HistLabelsStruct *hl;
	const struct EtcLabelsStruct *el;
	TCHAR buf[0x60];

	InitPropSheetsUxtheme(hDlg);
	SendDlgItemMessage(hDlg, IDE_EXTYPE, EM_LIMITTEXT, (WPARAM)VFPS - 1, 0);
	SendDlgItemMessage(hDlg, IDE_ALCCMD, EM_LIMITTEXT, (WPARAM)CMDLINESIZE - 1, 0);

	hEtcTreeWnd = GetDlgItem(hDlg, IDT_GENERAL);
	SendMessage(hEtcTreeWnd, WM_SETREDRAW, FALSE, 0);

	tvi.mask = TVIF_TEXT | TVIF_PARAM;
	tvi.lParam = ETCID;
	tvi.pszText = buf;
	tvins.hParent = TVI_ROOT;
	tvins.hInsertAfter = 0;
									// 各種一覧
	for ( el = EtcLabels ; el->name != NULL ; el++ ){
		thprintf(buf, TSIZEOF(buf), T("%Ms/%s"), el->name, el->key);
		tvi.cchTextMax = tstrlen32(buf);
		TreeInsertItemValue(tvins) = tvi;
		hTempItem = (HTREEITEM)SendMessage(hEtcTreeWnd, TVM_INSERTITEM,
				0, (LPARAM)(LPTV_INSERTSTRUCT)&tvins);
		if ( el == EtcLabels ){
			SendMessage(hEtcTreeWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hTempItem);
		}
		tvi.lParam++;
	}
									// ヒストリ
	tvi.lParam = MAXLPARAM;
	tvi.pszText = (TCHAR *)MessageText(MES_HIST);
	tvi.cchTextMax = tstrlen32(tvi.pszText);
	TreeInsertItemValue(tvins) = tvi;
	hHisRoot = (HTREEITEM)SendMessage(hEtcTreeWnd, TVM_INSERTITEM,
								0, (LPARAM)(LPTV_INSERTSTRUCT)&tvins);
	tvi.lParam = 0;
	tvins.hParent = hHisRoot;
	for ( hl = HistLabels ; hl->name != NULL ; hl++ ){
		tvi.pszText = (TCHAR *)MessageText(hl->name);
		tvi.cchTextMax = tstrlen32(tvi.pszText);
		TreeInsertItemValue(tvins) = tvi;
		SendMessage(hEtcTreeWnd, TVM_INSERTITEM,
				0, (LPARAM)(LPTV_INSERTSTRUCT)&tvins);
		tvi.lParam++;
	}
	TreeView_Expand(hEtcTreeWnd, hHisRoot, TVE_EXPAND);
	SendMessage(hEtcTreeWnd, WM_SETREDRAW, TRUE, 0);
}



// 項目をテキストに変換
void FormatEtcItem(TCHAR *data, int size, int type)
{
	TCHAR buf[CMDLINESIZE], *p = buf;
	LOADSETTINGS_CUST *ls;

	if ( type == ETC_HLKEY ) return;		// CV_hkey

	if ( type == ETC_NOPARAM ){			// X_fopt
		data[0] = '\0';
		return;
	}

	if ( type == ETC_WINPOS ){			// _WinPos
		RECT *box;

		if ( size < (int)(sizeof(RECT) + sizeof(WORD)) ){
			SetCText(data, StrBrokenData);
			return;
		}
		box = (RECT *)data;
		thprintf(buf, TSIZEOF(buf), T("(%d,%d) %dx%d"),
				box->left, box->top,
				box->right - box->left, box->bottom - box->top);
		tstrcpy(data, buf);
		return;
	}
	if ( type == ETC_EXECS ){			// _Execs
		DWORD *dat;

		if ( size < (int)(sizeof(DWORD) * 2) ){
			SetCText(data, StrBrokenData);
			return;
		}
		dat = (DWORD *)data;
		thprintf(buf, TSIZEOF(buf), T("%08x"), dat[dat[0]]);
		tstrcpy(data, buf);
		return;
	}
	if ( size < (int)sizeof(XC_DSET) ){
		SetCText(data, StrBrokenData);
		return;
	}
										// XC_dset
	*(TCHAR *)(BYTE *)((BYTE *)data + size) = '\0';
	ls = (LOADSETTINGS_CUST *)data;
	if ( ls->dset.flags & DSET_CACHEONLY )		p = SetCText(p, MES_VEDC);
	if ( ls->dset.flags & DSET_NODIRCHECK )		p = SetCText(p, MES_VEDH);
	if ( ls->dset.flags & DSET_ASYNCREAD )		p = SetCText(p, MES_VEDA);
	if ( ls->dset.flags & DSET_REFRESH_ACACHE ) p = SetCText(p, MES_VEDS);
	if ( ls->dset.flags & DSET_NOSAVE_ACACHE )	p = SetCText(p, MES_VEDN);
	if ( ls->dset.infoicon != DSETI_DEFAULT )	p = SetCText(p, MES_VEDI);
	if ( (ls->dset.cellicon != DSETI_DEFAULT) && ls->dset.cellicon ){
		p = SetCText(p, MES_VEDL);
	}
	if ( ls->dset.sort.mode.dat[0] != -1 )		p = SetCText(p, MES_VEDO);
	if ( ls->buf[0] ) p = tstpcpy(p, ls->buf);
	*p = '\0';
	tstrcpy(data, buf);
}

void EnumEtcItem(HWND hDlg)
{
	HWND hListWnd;
	TCHAR itemname[CUST_NAME_LENGTH + CMDLINESIZE], data[CMDLINESIZE * 2 + CUST_NAME_LENGTH + 16];
	const TCHAR *key;
	int count = 0;

	hListWnd = GetDlgItem(hDlg, IDL_EXITEM);
	SendMessage(hListWnd, WM_SETREDRAW, FALSE, 0);
	SendMessage(hListWnd, LB_RESETCONTENT, 0, 0);
	EtcEditFormat = EtcLabels[seltree - ETCID].form;
	key = EtcLabels[seltree - ETCID].key;

	EnableDlgWindow(hDlg, IDB_TB_SETITEM, EtcEditFormat < ETC__VIEW );
	SetDlgItemText(hDlg, IDB_TB_SETITEM, MessageText(
		((EtcEditFormat > ETC__EDITBUTTON) && (EtcEditFormat < ETC__VIEW)) ?
			MES_VESE : MES_VESA) );
	ShowDlgWindow(hDlg, IDB_TB_DELITEM, EtcEditFormat != ETC_INFODISP);

	if ( EtcEditFormat == ETC_INFODISP ){
		FormatCellDispSample(data, key, 1);
		SendMessage(hListWnd, LB_ADDSTRING, 0, (LPARAM)data);
	}else for ( ;; ){
		TCHAR *dest, *src;
		int csize;
		size_t len;

		data[0] = '\0';
		csize = EnumCustTable(count, key, itemname, data, CMDLINESIZE * sizeof(TCHAR));
		data[CMDLINESIZE] = '\0';
		if ( csize < 0 ) break;

		if ( EtcEditFormat == ETC_CELLDISP ){		// MC_celS
			FormatCellDispSample(data, itemname, 0);
		}else{
			if ( EtcEditFormat >= ETC__EDITBUTTON ){
				FormatEtcItem(data, csize, EtcEditFormat);
			}
		}
		// ～ = ～ 形式に加工
		len = tstrlen(itemname);
		dest = itemname + len;
		while ( len++ < 10 ) *dest++ = ' ';
		*dest++ = ' ';
		*dest++ = '=';
		*dest++ = ' ';
		tstrcpy(dest, data);

		src = itemname;
		if ( (EtcEditFormat == ETC_HLKEY) && (*src == '/') ) src++;
		SendMessage(hListWnd, LB_ADDSTRING, 0, (LPARAM)src);
		count++;
	}

	SendMessage(hListWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(hListWnd, NULL, TRUE);
	SetDlgItemText(hDlg, IDS_ETCLIST, MessageText(EtcLabels[seltree - ETCID].comment));
}

void EtcSelectItem(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	int index;
	TCHAR buf[CMDLINESIZE * 2], *p, *param;

	if ( GetListCursorIndex(wParam, lParam, &index) == 0 ) return;
	buf[0] = '\0';
	SendMessage((HWND)lParam, LB_GETTEXT, (WPARAM)index, (LPARAM)buf);

	p = param = tstrchr(buf, '=');
	if ( p == NULL ){ // 編集欄を使用しない
		buf[0] = '\0';
		param = buf;
	}else{
		while( p > buf ){
			if ( *(p - 1) != ' ' ) break;
			p--;
		}
		*p = '\0';
		if ( *(++param) == ' ' ) param++;
	}
	SetDlgItemText(hDlg, IDE_EXTYPE, buf);
	SetDlgItemText(hDlg, IDE_ALCCMD, param);
}

void AddEtcItem(HWND hDlg)
{
	TCHAR label[CMDLINESIZE], item[CMDLINESIZE], buf[CMDLINESIZE];
	const TCHAR *key;

	if ( EtcEditFormat > ETC__VIEW ) return;

	GetControlText(hDlg, IDE_EXTYPE, label, TSIZEOF(label));
	GetControlText(hDlg, IDE_ALCCMD, item, TSIZEOF(item));
	key = EtcLabels[seltree - ETCID].key;
	if ( EtcEditFormat > ETC__EDITBUTTON ){
		BOOL tempcreate = FALSE;

		if ( EtcEditFormat == ETC_DSET ) {
			const TCHAR *path = (label[0] == '/') ? label + 1 : label;
			thprintf(buf, TSIZEOF(buf), T("*ppc \"%s\" -single -k *diroption -\"%s\""), path, path);
			PP_ExtractMacro(NULL, NULL, NULL, buf, NULL, 0);
			return;
		}

		if ( EtcEditFormat == ETC_HLKEY ){
			if ( (label[0] != '\0') && (item[0] != '\0') ){
				tstrcpy(buf, label);
				FixWildItemName(buf);
				SetCustStringTable(T("CV_hkey"), buf, item, 0);
			}
			if ( PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_HIGHLIGHT),
					hDlg, HighlightDialogBox, (LPARAM)label) > 0 ){
				Changed(hDlg);
				EnumEtcItem(hDlg);
				SetDlgItemText(hDlg, IDE_EXTYPE, NilStr);
				SetDlgItemText(hDlg, IDE_ALCCMD, NilStr);
			}
			return;
		}

		// 以降は ETC_CELLDISP, ETC_INFODISP
		if ( EtcEditFormat == ETC_CELLDISP ){ // [;]menu
			if ( label[0] == '\0' ) return;
			if ( !IsExistCustTable(T("MC_celS"), label) && (item[0] != '\0') ){
				TCHAR labelbuf[CMDLINESIZE * 2];

				tstrcpy(labelbuf, label);
				tstrreplace(labelbuf, T("%"), T("%%"));

				// 未登録→仮登録する
				thprintf(buf, TSIZEOF(buf), T("*setcust MC_celS:%s=%s"), labelbuf, item);
				PP_ExtractMacro(NULL, NULL, NULL, buf, NULL, 0);
				tempcreate = TRUE;
			}
		}

		if ( PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DISPFOMAT),
				hDlg, DispFormatDialogBox,
				(LPARAM)(EtcEditFormat == ETC_INFODISP ? key : label)) > 0 ){
			Changed(hDlg);
			EnumEtcItem(hDlg);
			SetDlgItemText(hDlg, IDE_EXTYPE, NilStr);
			SetDlgItemText(hDlg, IDE_ALCCMD, NilStr);
		}else if ( IsTrue(tempcreate) ){
			DeleteCustTable(T("MC_celS"), label, 0);
		}
		return;
	}

	if ( NO_ERROR == SetCustStringTable(key, label, item, 0) ){
		Changed(hDlg);
		EnumEtcItem(hDlg);
	}
}

void ExEditItem(HWND hDlg)
{
	TCHAR label[CMDLINESIZE], item[LONG_CMDSIZE], title[CMDLINESIZE];
	const TCHAR *key;
	TINPUT tinput = {
		NULL, PPXH_GENERAL_R, PPXH_GENERAL, NULL, NULL, LONG_CMDSIZE - 1,
		TIEX_LINE_MULTILINE, 0, 0, NULL, NULL};

	if ( EtcEditFormat > ETC__VIEW ) return;

	key = EtcLabels[seltree - ETCID].key;
	GetControlText(hDlg, IDE_EXTYPE, label, TSIZEOF(label));
	item[0] = '\0';
	if ( NO_ERROR == GetCustTable(key, label, item, sizeof(item)) ){
		item[TSIZEOF(item) - 100] = '\0'; // tstrreplace 分を適当に確保
	}else{
		GetControlText(hDlg, IDE_ALCCMD, item, TSIZEOF(item) - 1);
	}
	tstrreplace(item, T("\n"), T("\r\n"));

	tinput.hOwnerWnd = hDlg;
	tinput.title = title;
	tinput.buff = item;

	thprintf(title, TSIZEOF(title), T("%s:%s ([OK] : alt + enter)"), key, label);

	if ( tInputEx(&tinput) <= 0 ) return;

	FixCutReturnCode(item);
											// 「 ;」のエスケープ
	if ( tstrstr(item, T("%(")) == NULL ){
		tstrreplace(item, T(" ;"), T(" %;"));
	}

	SetDlgItemText(hDlg, IDE_ALCCMD, item);
	if ( label[0] != '\0' ){
		if ( NO_ERROR == SetCustStringTable(key, label, item, 0) ){
			Changed(hDlg);
			EnumEtcItem(hDlg);
		}
	}
}

void DeleteEtcItem(HWND hDlg)
{
	TCHAR label[CMDLINESIZE];
	const TCHAR *key;

	key = EtcLabels[seltree - ETCID].key;
	GetControlText(hDlg, IDE_EXTYPE, label, TSIZEOF(label));
	if ( NO_ERROR == DeleteCustTable(key, label, 0) ){
		HWND hListWnd;
		int index;

		hListWnd = GetDlgItem(hDlg, IDL_EXITEM);
		index = (int)SendMessage(hListWnd, LB_GETCURSEL, 0, 0);
		Changed(hDlg);
		EnumEtcItem(hDlg);
		if ( index != LB_ERR ){
			SendMessage(hListWnd, LB_SETCURSEL, (WPARAM)index, 0);
			SendMessage(hListWnd, LB_SETTOPINDEX, (WPARAM)max(index - 6, 0), 0);
			EtcSelectItem(hDlg, TMAKEWPARAM(IDL_EXITEM, LBN_SELCHANGE), (LPARAM)hListWnd);
		}
	}
}

// 共通 -----------------------------------------------------------------------
void EtcSelectType(HWND hDlg, HWND hTwnd)
{
	TV_ITEM tvi;
	BOOL etc;
	HWND hExitem;

	tvi.hItem = TreeView_GetSelection(hTwnd);
	tvi.mask = TVIF_PARAM;
	TreeView_GetItem(hTwnd, &tvi);
	seltree = tvi.lParam;
	etc = (seltree >= ETCID);
	ShowDlgWindow(hDlg, IDB_TB_SETITEM, etc);
	ShowDlgWindow(hDlg, IDB_MEUP, etc);
	ShowDlgWindow(hDlg, IDB_MEDW, etc);
	if ( etc ){ // Etc
		const struct EtcLabelsStruct *label;

		label = &EtcLabels[seltree - ETCID];
		etc = (label->form != ETC_INFODISP);
		EnumEtcItem(hDlg);

		ShowDlgWindow(hDlg, IDB_ETCEDIT, (label->form == ETC_EXTEXT) );
	}else if ( (tvi.lParam >= 0) && (tvi.lParam < 14) ){ // Histroy
		HistFlag = (WORD)HistLabels[tvi.lParam].flag;
		EnumHistoryItem(hDlg);
	}
	ShowDlgWindow(hDlg, IDE_EXTYPE, etc);
	SetDlgItemText(hDlg, IDE_EXTYPE, NilStr);
	ShowDlgWindow(hDlg, IDE_ALCCMD, etc);
	SetDlgItemText(hDlg, IDE_ALCCMD, NilStr);

	hExitem = GetDlgItem(hDlg, IDL_EXITEM);
	SendMessage(hExitem, LB_SETCURSEL, 0, 0);
	EtcSelectItem(hDlg, TMAKEWPARAM(IDL_EXITEM, LBN_SELCHANGE), (LPARAM)hExitem);
}

// 項目を一つ上下に移動
void EItemUpDown(HWND hDlg, int offset)
{
	TCHAR itemname[CUST_NAME_LENGTH], para[0x8000];
	const TCHAR *key;
	int size, index;
	DWORD shift = GetShiftKey();

	key = EtcLabels[seltree - ETCID].key;

	index = (int)SendDlgItemMessage(hDlg, IDL_EXITEM, LB_GETCURSEL, 0, 0);
	if ( offset < 0 ){
		if ( index <= 0 ) return;
		if ( shift & K_c ) offset = -index;
	}else{
		int count = SendDlgItemMessage(hDlg, IDL_EXITEM, LB_GETCOUNT, 0, 0);

		if ( index < 0 ) return;
		if ( (index + 1) >= count ) return;
		if ( shift & K_c ) offset = (count - index - 1);
	}
	size = EnumCustTable(index, key, itemname, para, sizeof(para));
	if ( size < 0 ) return;
	DeleteCustTable(key, NULL, index);
	InsertCustTable(key, itemname, index + offset, para, size);
	EtcSelectType(hDlg, GetDlgItem(hDlg, IDT_GENERAL));

	index += offset;
	Changed(hDlg);
	SendDlgItemMessage(hDlg, IDL_EXITEM, LB_SETCURSEL, (WPARAM)index, 0);
}


BOOL EtcTreeNotify(HWND hDlg, NMHDR *nmh)
{
	switch (nmh->code){
		case PSN_SETACTIVE:
			InitWndIcon(hDlg, IDB_TB_DELITEM);
			break;

		case PSN_APPLY:
		case PSN_HELP:
			DlgSheetProc(hDlg, WM_NOTIFY, 0, (LPARAM)nmh, IDD_ETCTREE);
			break;

		case NM_DBLCLK:
			EtcSelectType(hDlg, nmh->hwndFrom);
			break;

		case TVN_KEYDOWN:
			switch ( ((TV_KEYDOWN *)nmh)->wVKey ){
				case VK_SPACE:
					EtcSelectType(hDlg, nmh->hwndFrom);
					break;
			}
			break;

		case TVN_SELCHANGED:
			if ( nmh->hwndFrom == hEtcTreeWnd ){
				EtcSelectType(hDlg, nmh->hwndFrom);
			}
			break;

//		default:
	}
	return 0;
}

INT_PTR CALLBACK EtcPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg){
		case WM_INITDIALOG:
			InitEtcTree(hDlg);
			return FALSE;

		case WM_NOTIFY:
			EtcTreeNotify(hDlg, (NMHDR *)lParam);
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDL_EXITEM:
					if ( seltree >= ETCID ) EtcSelectItem(hDlg, wParam, lParam);
					break;

				case IDB_TB_SETITEM:
					if ( seltree >= ETCID ) AddEtcItem(hDlg);
					break;

				case IDB_TB_DELITEM:
					if ( seltree >= ETCID ){
						DeleteEtcItem(hDlg);
					}else{
						DeleteHistoryItem(hDlg);
					}
					break;

				case IDB_MEUP:
					EItemUpDown(hDlg, -1);
					break;

				case IDB_MEDW:
					EItemUpDown(hDlg, +1);
					break;

				case IDB_TEST:
					Test();
					break;

				case IDB_ETCEDIT:
					if ( seltree >= ETCID ) ExEditItem(hDlg);
					break;
			}
			break;
		default:
			return DlgSheetProc(hDlg, msg, wParam, lParam, IDD_ETCTREE);
	}
	return TRUE;
}
