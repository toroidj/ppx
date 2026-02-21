/*-----------------------------------------------------------------------------
	Paper Plane xUI	customizer	その他 シート - PPc 表示書式
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPCUST.H"
#include "PPC_DISP.H"
#pragma hdrstop

#define DFP_PARAMCOUNT 2 // 書式の最大パラメータ数

// パラメータの追加オプション用フラグ
#define COMMENTHID B31	// コメント時非表示
#define FIXFRAME B30	// W
#define FIXLENGTH B29	// w
// B16-23: W, w の長さ
#define PARAMMASK MAX16

// パラメータの形式
typedef enum { DFP_NONE = 0, DFP_WIDTH, DFP_EXT, DFP_COLOR, DFP_STRING, DFP_COLUMN, DFP_MODULE} DFPTYPES;

typedef struct {
	const TCHAR *name;
	DFPTYPES type;
	const TCHAR *list;
} DISPFORMATPARAM;

typedef struct {
	const TCHAR *name;
	TCHAR ID;
	DISPFORMATPARAM param[DFP_PARAMCOUNT];
	const TCHAR *defaultparam;
} DISPFORMATITEM;
DISPFORMATITEM dfitem[] = {
	{MES_VDNS, 'S', { // 空白(中用)
			{MES_VDLD, DFP_WIDTH,
				T("1\0")
				T(" - 空欄:残り全て\1fill blank\0")},
			{NULL, DFP_NONE, NULL}}, T("1")},
	{MES_VDMS, 's', { // 空白(端用)
			{MES_VDLD, DFP_WIDTH,
				T("1\0")
				T(" - 空欄:残り全て\1fill blank\0")},
			{NULL, DFP_NONE, NULL}}, T("1")},
	{MES_VDNM, 'M', { // マーク「*」
			{NULL, DFP_NONE, NULL},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDMB, 'b', { // チェックマーク
			{NULL, DFP_NONE, NULL},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDNB, 'B', { // チェックボックス
			{NULL, DFP_NONE, NULL},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDNN, 'N', { // アイコン
			{MES_VDLZ, DFP_WIDTH,
				T(" - 空欄:文字に合わせる\1blank: auto size\0")
				T("16 - 小\1small\0")
				T("32 - 中\1middle\0")
				T("48 - 大\1large\0")},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDMN, 'n', { // サムネイル画像
			{MES_VDLW, DFP_STRING, T("\0")},
			{MES_VDLH, DFP_STRING, T("\0")}}, T("20,8")},
	{MES_VDMF, 'f', { // 短いファイル名
			{MES_VDLA, DFP_WIDTH,
				T("8 - 通常\1normal\0")
				T("E8 - 拡張子優先\1for extention\0")},
			{MES_VDLX, DFP_EXT,
				T("5 - 通常\1normal\0")
				T("0 - 表示しない\1disable\0")
				T(" - 空欄:ファイル名と分離しない\1blank: joint filename\0")}}, T("8,5")},
	{MES_VDNF, 'F', { // ファイル名
			{MES_VDLA, DFP_WIDTH,
				T("14 - 短め\1short\0")
				T("30 - 長め\1long\0")
				T(" - 詳細表示(１行表示)\1full width\0")
				T("E14 - 拡張子優先\1for extention\0")
				T("M14 - 複数段表示\1multi line\0")},
			{MES_VDLX, DFP_EXT,
				T("5 - 通常\1normal\0")
				T("0 - 表示しない\1disable\0")
				T(" - 空欄:ファイル名と分離しない\1blank: joint filename\0")}}, T("17,5")},
	{MES_VDNZ, 'Z', { // ファイルサイズ
			{MES_VDLD, DFP_WIDTH,
				T("7 - 6桁分\1 6 columns\0")
				T("10 - 9桁分\1 9 columns\0")},
			{NULL, DFP_NONE, NULL}}, T("7")},
	{MES_VDMZ, 'z', { // ファイルサイズ「,」付
			{MES_VDLD, DFP_WIDTH,
				T("10 - 6桁分\1 6 columns\0")
				T("7 - 小数点表示\1 5 columns\0")
				T("14 - 9桁分\1 9 columns\0")
				T("K10 - エクスプローラ風K単位\1'K'\0")},
			{NULL, DFP_NONE, NULL}}, T("10")},
	{MES_VDNT, 'T', { // 更新時刻
			{MES_VDLD, DFP_WIDTH,
				T("8 - 年月日\1YY-MM-DD\0")
				T("14 - 年月日時分\1YY-MM-DD TT:MM\0")
				T("17 - 年月日時分秒\1YY-MM-DD TT:MM:SS\0")},
			{NULL, DFP_NONE, NULL}}, T("14")},
	{MES_VDMT, 't', { // 時刻詳細
			{MES_VDLF, DFP_STRING,
				T("\"Y-n-d(w) u:M:St\" - 更新時刻(JPN)\1modify(JPN)\0")
				T("C\"Y-n-d(w) u:M:St\" - 作成時刻\1create\0")
				T("A\"Y-n-d(w) u:M:St\" - アクセス時刻\1access\0")
				T("\"n/d/Y(w) u:M:St\" - 更新時刻(US)\1modify(US)\0")
				T("\"d/n/Y(w) u:M:St\" - 更新時刻(ENG)\1modify(ENG)\0")},
			{NULL, DFP_NONE, NULL}}, T("\"Y-n-d(w) u:M:St\"")},
	{MES_VDNA, 'A', { // 属性
			{MES_VDLD, DFP_WIDTH, T("\0")},
			{NULL, DFP_NONE, NULL}}, T("10")},
	{MES_VDNC, 'C', { // コメント
			{MES_VDLD, DFP_WIDTH, T("\0")},
			{NULL, DFP_NONE, NULL}}, T("10")},
	{MES_VDMU, 'u', { // コメント(拡張)
			{MES_VDLI, DFP_STRING, T("1\0") T("2\0") T("3\0") T("4\0") T("5\0")},
			{MES_VDLD, DFP_WIDTH, T("\0")}}, T("1,10")},
	{MES_VDNU, 'U', { // カラム拡張
			{MES_VDLY, DFP_COLUMN, NilStr},
			{MES_VDLD, DFP_WIDTH, T("\0")}}, T("\"タイトル\",10")},
	{MES_VDNX, 'X', { // モジュール拡張
			{MES_VDLY, DFP_MODULE, NilStr},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDNL, 'L', { // 縦線「|」
			{NULL, DFP_NONE, NULL},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDNH, 'H', { // 横線「-」
			{MES_VDLD, DFP_WIDTH, T("\0")},
			{NULL, DFP_NONE, NULL}}, T("10")},
	{MES_VDLN, '/', { // 改行
			{NULL, DFP_NONE, NULL},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDNO, 'O', { // 文字色
			{MES_VDLC, DFP_COLOR,
				T("_DWHI - 文字色\1letter color\0")
				T("b_WHI - 背景色\1back color\0")},
			{NULL, DFP_NONE, NULL}}, T("\"_DWHI\"")},
	{MES_VDMI, 'i', { // ユーザ文字列
			{MES_VDLT, DFP_STRING, T("\0")},
			{NULL, DFP_NONE, NULL}}, T("\"text\"")},
	{MES_VDNI, 'I', { // 文字列(項目名)
			{MES_VDLT, DFP_STRING, T("\0")},
			{NULL, DFP_NONE, NULL}}, T("\"text\"")},
	{MES_VDMV, 'v', { // id別特殊環境変数
			{MES_VDLV, DFP_STRING, T("\0")},
			{NULL, DFP_NONE, NULL}}, T("i\"name\"")},
	{MES_VDNY, 'Y', { // ディレクトリ種類
			{NULL, DFP_NONE, NULL},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDNV, 'V', { // ボリュームラベル
			{MES_VDLD, DFP_WIDTH, T("\0")},
			{NULL, DFP_NONE, NULL}}, T("8")},
	{MES_VDNR, 'R', { // ディレクトリ
			{MES_VDLD, DFP_WIDTH,
				T(" - 空欄:現在ディレクトリ\1current\0")
				T("M - ディレクトリマスク\1dir. mask\0")},
			{NULL, DFP_NONE, NULL}}, T("60")},
	{MES_VDNE, 'E', { // エントリ数
			{MES_VDLY, DFP_WIDTH,
				T(" - 空欄:表示分/全体\1show/all\0")
				T("0 - 全体\1all\0")
				T("1 - ./..除く全体\1all without ./..\0")},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDME, 'e', { // 表示エントリ数
			{MES_VDLY, DFP_WIDTH,
				T(" - 空欄:全て\1all\0")
				T("1 - ./..除く\1without ./..\0")
				T("2 - ディレクトリ数\1directories\0")
				T("3 - ファイル数\1files\0")},
			{NULL, DFP_NONE, NULL}}, T("1")},
	{MES_VDNP, 'P', { // 全ページ数
			{NULL, DFP_NONE, NULL},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDMP, 'p', { // 現在ページ数
			{NULL, DFP_NONE, NULL},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDMM, 'm', { // マーク情報
			{MES_VDLY, DFP_STRING,
				T("n - マーク数\1marks\0")
				T("S - 桁区切り無しサイズ\1size dddd\0")
				T("s - 桁区切り有りサイズ\1size d,ddd\0")
				T("K - エクスプローラ風サイズ\1size dddK\0")},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDND, 'D', { // ディスク容量
			{MES_VDLY, DFP_STRING,
				T("F - 空き\1free dddd\0")
				T("f - 空き(桁区切有)\1free d,ddd\0")
				T("U - 使用\1used dddd\0")
				T("u - 使用(桁区切有)\1used d,ddd\0")
				T("T - 総計\1total dddd\0")
				T("t - 総計(桁区切有)\1total d,ddd\0")},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_VDNG, 'G', { // インデント(文字数)
			{MES_VDLD, DFP_WIDTH,
				T("L10 - 左側\1left\0")
				T("R10 - 右側\1right\0")},
			{NULL, DFP_NONE, NULL}}, T("L10")},
	{MES_VDMG, 'g', { // インデント(画素数)
			{MES_VDLD, DFP_WIDTH,
				T("L10 - 左側\1left\0")
				T("R10 - 右側\1right\0")},
			{NULL, DFP_NONE, NULL}}, T("L10")},
	{MES_VDMQ, 'Q', { // 一覧の配置方法
			{MES_VDLY, DFP_WIDTH,
				T("0 - ページ,逆N配列\1page, mirror N order\0")
				T("1 - スクロール,逆N配列\1scroll, mirror N order\0")
				T("2 - (開発中)ページ,Z配列\1page, Z order\0")
				T("3 - (開発中)スクロール,Z配列\1scroll, Z order\0")},
			{NULL, DFP_NONE, NULL}}, T("0")},
	{MES_VDMJ, 'j', { // 詰めて表示
			{NULL, DFP_NONE, NULL},
			{NULL, DFP_NONE, NULL}}, NilStr},
	{MES_UNKN, '\0', {{NULL, DFP_NONE, NULL}, {NULL, DFP_NONE, NULL}}, NilStr}
};

#define LISTPARMSEP T("  ") // リスト内の名称と値の区切り
#define LISTPARMSEPLEN 2


const TCHAR X_stat_default[] = T("I\"Mark:\"mn i\"/\" ms8 L I\"Entry:\" e0 i\"/\" E0 L I\"Free:\" Df8 L I\"Used:\" Du8 L I\"Total:\" Dt8 L Y");

const TCHAR ColumnListComment[] = MES_VLFF;
const TCHAR StrParmSep[] = LISTPARMSEP;

DWORD_PTR USECDECL ModuleFunction(PPXAPPINFO *info, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	if ( cmdID == PPXCMDID_REPORTPPCCOLUMN ){
		SendMessage(info->hWnd, CB_ADDSTRING, 0, (LPARAM)uptr->str);
		return PPXA_NO_ERROR;
	}
	if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
	return PPXA_INVALID_FUNCTION;
}

// エントリ表示書式 ===========================================================
// w / W の長さを文字列化
TCHAR *GetFixMaxString(TCHAR *dest, int param)
{
	int width;

	width = (param >> 16) & 0xff;
	if ( (width == 0) || (width >= 255) ){
		dest[0] = '\0';
		return dest;
	}
	return thprintf(dest, 8, T("%d"), width);
}

void ShowFIXMAX(HWND hDlg, int param)
{
	BOOL show;

	if ( param < 0 ){
		int index;

		index = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETCURSEL, 0, 0);
		if ( index == LB_ERR ) return;
		param = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETITEMDATA, index, 0);
	}
	show = param & (FIXFRAME | FIXLENGTH);

	ShowDlgWindow(hDlg, IDE_FIXMAX, show);
	ShowDlgWindow(hDlg, IDS_FIXMAX, show);
}

// 選択した項目のパラメータを表示する
void SetDispFormatParamUI(HWND hDlg, int index)
{
	DISPFORMATPARAM *dfp;
	UINT sid = IDS_DFPARAM1, cid = IDC_DFPARAM1;
	int param, i;
	TCHAR formatBuf[CMDLINESIZE];
	TCHAR *format;
	BOOL show1, show2;
	RECT box1, box2;
	HWND hCWnd;

	if ( LB_ERR == SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETTEXT, index, (LPARAM)formatBuf) ){
		return;
	}
	#pragma warning(suppress: 6001) // LB_GETTEXT で取得
	format = tstrstr(formatBuf, StrParmSep);
	if ( format == NULL ){
		format = NilStrNC;
	}else{
		format += LISTPARMSEPLEN;
	}
	param = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETITEMDATA, index, 0);
	dfp = dfitem[param & PARAMMASK].param;

	CheckDlgButton(hDlg, IDX_COMMENTHID, param & COMMENTHID);
	CheckDlgButton(hDlg, IDX_FIXFRAME, param & FIXFRAME);
	CheckDlgButton(hDlg, IDX_FIXLENGTH, param & FIXLENGTH);
	show1 = dfp[1].type == DFP_EXT;
	show2 = show1 || (tstrchr(DE_ENABLE_FIXLENGTH, dfitem[param & PARAMMASK].ID) != NULL);
	ShowDlgWindow(hDlg, IDX_FIXFRAME, show2);
	ShowDlgWindow(hDlg, IDX_FIXLENGTH, show1);
	ShowFIXMAX(hDlg, param);

	hCWnd = GetDlgItem(hDlg, IDC_DFPARAM1);
	GetWindowRect(hCWnd, &box1);
	if ( (dfp[0].type == DFP_COLUMN) || (dfp[0].type == DFP_MODULE) ){
		GetWindowRect(GetDlgItem(hDlg, IDL_DFLIST), &box2);
		SetWindowPos(hCWnd, NULL, 0, 0, box2.right - box1.left, box1.bottom - box1.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}else{
		GetWindowRect(GetDlgItem(hDlg, IDC_DFPARAM2), &box2);
		SetWindowPos(hCWnd, NULL, 0, 0, box2.right - box2.left, box1.bottom - box1.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}

	for ( i = 0 ; i < DFP_PARAMCOUNT ; i++, dfp++, sid += 2, cid += 2 ){
		hCWnd = GetDlgItem(hDlg, cid);
		if ( dfp->type == DFP_NONE ){
			ShowDlgWindow(hDlg, sid, FALSE);
			ShowWindow(hCWnd, SW_HIDE);
		}else{
			SendMessage(hCWnd, CB_RESETCONTENT, 0, 0);
			if ( dfp->type == DFP_COLUMN ){
			/* メニュー表示に移行
				ThSTRUCT thEcdata;
				TCHAR path[VFPS];

				ThInit(&thEcdata);
				CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
				GetCurrentDirectory(VFPS, path);
				GetColumnExtMenu(&thEcdata, path, (HMENU)hCWnd, 0);
				GetColumnExtMenu(&thEcdata, NULL, NULL, 0);
				CoUninitialize();
			*/
				SendMessage(hCWnd, CB_ADDSTRING, 0, (LPARAM)MessageText(ColumnListComment));
			}else if ( dfp->type == DFP_MODULE ){
				PPXMODULEPARAM dummypmp;
				PPXAPPINFO info = {(PPXAPPINFOFUNCTION)ModuleFunction, T("PPcust"), NilStr, NULL};
				info.hWnd = hCWnd;
				dummypmp.command = NULL;
				CallModule(&info, PPXMEVENT_FILEDRAWINFO, dummypmp, NULL);
			}else{
				TCHAR comment[128];
				const TCHAR *src;
				TCHAR *dest;

				src = dfp->list;
				while ( *src != '\0' ){
					dest = comment;
					for (;;){
						if ( *src == '\0' ) break;
						if ( (*src == ' ') && (*(src - 1) == '-') && (*(src - 2) == ' ') ){
							src++;
							*dest++ = ' '; // LISTPARMSEP
							*dest++ = ' ';
							if ( UseLcid == LCID_PPXDEF ){
								for (;;){
									if ( *src == '\0' ) break;
									if ( *src == '\1' ) break;
									*dest++ = *src++;
								}
							}else{
								for (;;){
									if ( *src == '\0' ) break;
									if ( *src == '\1' ){
										src++;
										for (;;){
											if ( *src == '\0' ) break;
											*dest++ = *src++;
										}
										break;
									}
									src++;
								}
							}
							break;
						}
						*dest++ = *src++;
					}
					*dest = '\0';
					SendMessage(hCWnd, CB_ADDSTRING, 0, (LPARAM)comment);
					src += tstrlen(src) + 1;
				}
			}

			if ( format != NULL ){
				TCHAR *ptr;

				ptr = format;
				if ( dfp->type != DFP_MODULE ){ // module以外はカンマで切り出し
					while ( (*ptr != '\0') && (*ptr != ',') ) ptr++;
					if ( *ptr == ',' ) *ptr++ = '\0';
				}
				SetWindowText(hCWnd, format);
				format = ptr;
			}
			SetDlgItemText(hDlg, sid, MessageText(dfp->name));
			ShowDlgWindow(hDlg, sid, TRUE);
			ShowWindow(hCWnd, SW_SHOW);
		}
	}

	if ( show2 ){
		TCHAR num[8];

		GetFixMaxString(num, param);
		SetDlgItemText(hDlg, IDE_FIXMAX, num); // ここで、再設定が起きるので、最後に記載しないといけない
	}
}

// XC_stat/XC_inf1/XC_inf2 の 色指定をスキップする
TCHAR *GetInfoFormat(const TCHAR *src)
{
	int i;
	TCHAR *p;

	for ( i = 0 ; i < 4 ; i++ ){
		p = tstrchr(src, ',');
		if ( p != NULL ) src = p + 1;
	}
	return (TCHAR *)src;
}

BOOL SaveDispFormat(HWND hDlg)
{
	int index = 0, param;
	BOOL usefix = FALSE;
	TCHAR savetext[CMDLINESIZE], text[CMDLINESIZE], *dst, *p, ID;
	const TCHAR *key;

	key = (const TCHAR *)GetWindowLongPtr(hDlg, DWLP_USER);
	if ( EtcEditFormat == ETC_INFODISP ){
		dst = thprintf(savetext, TSIZEOF(savetext), T("*setcust %s="), key);

		// 書式以外の色やマーク指定等を用意する
		thprintf(dst, 100, T("%%*getcust(%s)"), key);
		PP_ExtractMacro(NULL, NULL, NULL, dst, dst, 0);
		dst = GetInfoFormat(dst);
	}else{
		thprintf(savetext, TSIZEOF(savetext), T("*setcust MC_celS:%s="), key);
		tstrreplace(savetext, T("%"), T("%%"));
		dst = savetext + tstrlen(savetext);
	}

	for ( ;; ){
		if ( SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETTEXT, index, (LPARAM)text) == LB_ERR ){
			break;
		}
		param = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETITEMDATA, index, 0);
		ID = dfitem[param & PARAMMASK].ID;
		if ( param & COMMENTHID ) *dst++ = 'c';
		if ( param & FIXFRAME ){
			*dst++ = 'w';
			dst = GetFixMaxString(dst, param);
		}
		if ( param & FIXLENGTH ){
			*dst++ = 'W';
			dst = GetFixMaxString(dst, param);
		}
		if ( param & (FIXFRAME | FIXLENGTH) ){
			if ( usefix ){
				SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETCURSEL, index, 0);
				PMessageBox(hDlg, MES_ECFD, StrCustTitle, MB_OK);
				return FALSE;
			}
			usefix = TRUE;
		}
		if ( ID == '/' ) usefix = FALSE;
		*dst++ = ID;
		#pragma warning(suppress: 6001) // LB_GETTEXT で取得
		p = tstrstr(text, StrParmSep);
		if ( p != NULL ){
			p += LISTPARMSEPLEN;
			while ( *p != '\0' ){
				*dst++ = *p++;
			}
		}
		index++;
	}
	*dst = '\0';
	PP_ExtractMacro(NULL, NULL, NULL, savetext, NULL, 0);
	return TRUE;
}

const TCHAR *GetDispFormat(TCHAR *data, const TCHAR *label, int offset)
{
	const TCHAR *src;
	TCHAR labelbuf[CMDLINESIZE * 2];

	tstrcpy(labelbuf, label);

	tstrreplace(labelbuf, T("\""), T("\"\""));
	tstrreplace(labelbuf, T("%"), T("%%"));

	thprintf(data, CMDLINESIZE,
			offset ? T("%%*getcust(%s)") : T("%%*getcust(\"MC_celS:%s\")"),
			labelbuf);
	PP_ExtractMacro(NULL, NULL, NULL, data, data, 0);
	src = data;
	if ( offset ){
		src = GetInfoFormat(src);
		if ( (*src == '\0') && (tstrcmp(label, T("XC_stat")) == 0) ){
			src = X_stat_default;
		}
	}
	return src;
}

void InitDispFormat(HWND hDlg, const TCHAR *key)
{
	TCHAR format[CMDLINESIZE], formc, textbuf[MAX_PATH];
	const TCHAR *src;
	int index, param;
	BOOL fromppc = FALSE;

	LocalizeDialogText(hDlg, IDD_DISPFOMAT);
	PPxRegist(hDlg, PPcustRegID, PPXREGIST_IDASSIGN);
	if ( key == NULL ){ // PPc の直接編集モード
		EtcEditFormat = ETC_CELLDISP;
		key = MC_CELS_TEMPNAME;
		fromppc = TRUE;
	}
	SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)key);

	// 書式を列挙する
	src = GetDispFormat(format, key, (EtcEditFormat == ETC_INFODISP));
	if ( IsTrue(fromppc) ) DeleteCustTable(T("MC_celS"), key, 0);
	index = 0;
	while( (formc = *src++) != '\0' ){ // 各書式をリストに列挙する
		DISPFORMATITEM *tdf;
		TCHAR *p;

		if ( formc == ' ') continue;

		// プレフィックス
		param = 0;
		if ( formc == 'c' ){
			setflag(param, COMMENTHID);
			formc = *src++;
		}
		if ( (formc == 'w') || (formc == 'W') ){
			setflag(param, (formc == 'w') ? FIXFRAME : FIXLENGTH ) ;
			param |= GetDwordNumber(&src) << 16;
			formc = *src++;
		}

		// 書式IDチェック
		for ( tdf = dfitem ; tdf->ID ; tdf++ ){
			if ( formc == tdf->ID ) break;
			param++;
		}

		p = tstpcpy(textbuf, MessageText(tdf->name));
		*p++ = ' '; // LISTPARMSEP
		*p++ = ' ';
		while( (UTCHAR)*src > ' ' ){ // パラメータ部分をコピー
			if ( *src == '\"' ){ // " で括った部分をまとめてコピー
				do {
					*p++ = *src++;
				}while ( ((UTCHAR)*src >= ' ') && (*src != '\"') );
			}
			*p++ = *src++;
		}
		*p = '\0';
		SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_ADDSTRING, 0, (LPARAM)textbuf);
		SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETITEMDATA, index, param);
		index++;
	}

	// 使用できる書式の一覧を列挙する
	{
		DISPFORMATITEM *tdf;

		for ( tdf = dfitem ; tdf->ID ; tdf++ ){
			SendDlgItemMessage(hDlg, IDL_DFLIST,
					LB_ADDSTRING, 0, (LPARAM)MessageText(tdf->name));
		}
	}
	if ( index ){
		SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETCURSEL, 0, 0);
		SetDispFormatParamUI(hDlg, 0);
	}
}

void SeletctNowDispFormat(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	int index;

	if ( GetListCursorIndex(wParam, lParam, &index) == 0 ) return;
	SetDispFormatParamUI(hDlg, index);
}

void InsertDispFormat(HWND hDlg)
{
	int index, templeateitem;
	TCHAR param[CMDLINESIZE];

	templeateitem = (int)SendDlgItemMessage(hDlg, IDL_DFLIST, LB_GETCURSEL, 0, 0);
	if ( templeateitem == LB_ERR ) templeateitem = 0;

	index = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETCURSEL, 0, 0);
	if ( index < 0 ) index = 0;

	thprintf(param, TSIZEOF(param), T("%s") LISTPARMSEP T("%s"), MessageText(dfitem[templeateitem].name), dfitem[templeateitem].defaultparam);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_INSERTSTRING, index, (LPARAM)param);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETITEMDATA, index, templeateitem);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETCURSEL, index, 0);
	SetDispFormatParamUI(hDlg, index);
}

void DeleteDispFormat(HWND hDlg)
{
	int index;

	index = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETCURSEL, 0, 0);
	if ( index == LB_ERR ) return;
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_DELETESTRING, index, 0);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETCURSEL, index, 0);
	SetDispFormatParamUI(hDlg, index);
}

void UpDownDispFormat(HWND hDlg, int dist)
{
	int index, param;
	TCHAR text[CMDLINESIZE];
	DWORD shift = GetShiftKey();

	index = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETCURSEL, 0, 0);
	if ( index == LB_ERR ) return;
	if ( dist < 0 ){
		if ( index <= 0 ) return;
//		if ( shift & K_s ) dist = -min(index , 5);
		if ( shift & K_c ) dist = -index;
	}else{
		int count = SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETCOUNT, 0, 0);
		if ( (index + 1) >= count ) return;
//		if ( shift & K_s ) dist = min(index , (count - index - 1));
		if ( shift & K_c ) dist = (count - index - 1);
	}

	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETTEXT, index, (LPARAM)text);
	param = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETITEMDATA, index, 0);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_DELETESTRING, index, 0);

	index += dist;
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_INSERTSTRING, index, (LPARAM)text);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETITEMDATA, index, param);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETCURSEL, index, 0);
}

void DeleteComment(HWND hDlg, UINT ID, TCHAR *text)
{
	TCHAR *p;

	p = tstrstr(text, T(" - "));
	if ( p != NULL ){
		*p = '\0';
		SetDlgItemText(hDlg, ID, text);
	}
}

void MakeFixMax(HWND hDlg, int *param, int flag)
{
	TCHAR num[10];
	const TCHAR *src;
	int width;

	GetDlgItemText(hDlg, IDE_FIXMAX, num, TSIZEOF(num));
	src = num;
	width = GetIntNumber(&src);
	if ( (width < 0) || (width >= 255 ) ) width = 0;
	*param |= (width << 16) | flag;
}

void ModifyDispFormat(HWND hDlg)
{
	int index, param;
	TCHAR text[CMDLINESIZE], buf[CMDLINESIZE], *ptr;

	index = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETCURSEL, 0, 0);
	if ( index == LB_ERR ) return;

	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETTEXT, index, (LPARAM)text);
	param = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETITEMDATA, index, 0) & PARAMMASK;
	#pragma warning(suppress: 6001) // LB_GETTEXT で取得
	ptr = tstrstr(text, StrParmSep);
	if ( ptr == NULL ) ptr = text + tstrlen(text);
	*ptr++ = ' '; // LISTPARMSEP 設定
	*ptr++ = ' ';

	if ( dfitem[param].param[0].type != DFP_NONE ){
		GetDlgItemText(hDlg, IDC_DFPARAM1, buf, TSIZEOF(buf));
		if ( dfitem[param].param[0].type == DFP_COLUMN ){
			if ( buf[0] != '\"' ) return; // 区分名の為更新しない
		}
		DeleteComment(hDlg, IDC_DFPARAM1, buf);
		ptr = tstpcpy(ptr, buf);
	}
	if ( dfitem[param].param[1].type != DFP_NONE ){
		GetDlgItemText(hDlg, IDC_DFPARAM2, buf, TSIZEOF(buf));
		if ( buf[0] != '\0' ){
			DeleteComment(hDlg, IDC_DFPARAM2, buf);
			*ptr++ = ',';
			tstrcpy(ptr, buf);
		}
	}
	if ( IsDlgButtonChecked(hDlg, IDX_COMMENTHID) ) setflag(param, COMMENTHID);
	if ( IsDlgButtonChecked(hDlg, IDX_FIXFRAME) ){
		MakeFixMax(hDlg, &param, FIXFRAME);
	}
	if ( IsDlgButtonChecked(hDlg, IDX_FIXLENGTH) ){
		MakeFixMax(hDlg, &param, FIXLENGTH);
	}

	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_DELETESTRING, index, 0);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_INSERTSTRING, index, (LPARAM)text);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETITEMDATA, index, param);
	SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_SETCURSEL, index, 0);
}

BOOL ModifyDispColumnFormat(HWND hDlg, HWND hCBwnd)
{
	int index;
	HMENU hPopMenu;
	ThSTRUCT thEcdata;
	TCHAR path[VFPS];
	HRESULT ComInitResult;
	#define MenuFirstID 1

	index = (int)SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETCURSEL, 0, 0);
	if ( index == LB_ERR ) return TRUE; // エラーなので無視させる

	if ( dfitem[SendDlgItemMessage(hDlg, IDL_DFOMAT, LB_GETITEMDATA, index, 0) & PARAMMASK].param[0].type != DFP_COLUMN ){
		return FALSE; // 別の種類だった
	}

	ThInit(&thEcdata);
	ComInitResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	GetCurrentDirectory(VFPS, path);

	hPopMenu = CreatePopupMenu();
	GetColumnExtMenu(&thEcdata, path, hPopMenu, MenuFirstID);

	index = ButtonTrackPopupMenu(hDlg, hPopMenu, hCBwnd);
	GetColumnExtMenu(&thEcdata, NULL, NULL, 0);
	if ( SUCCEEDED(ComInitResult) ) CoUninitialize();

	if ( index >= MenuFirstID ){
		GetMenuString(hPopMenu, index, path + 1, TSIZEOF(path) - 1, MF_BYCOMMAND);
		path[0] = '\"';
		tstrcat(path, T("\""));
		SetWindowText(hCBwnd, path);
		ModifyDispFormat(hDlg);
	}
	DestroyMenu(hPopMenu);
	PostMessage(hCBwnd, CB_SHOWDROPDOWN, 0, 0);
	return TRUE;
}

void FixSelectedItem(HWND hComboWnd)
{
	TCHAR buf[CMDLINESIZE], *ptr;

	GetWindowText(hComboWnd, buf, CMDLINESIZE);
	ptr = tstrstr(buf, T(" - "));
	if ( ptr != NULL ){
		*ptr = '\0';
		SetWindowText(hComboWnd, buf);
	}
}

#define WM_APP_CB_SELECTED (WM_APP + 106)
INT_PTR CALLBACK DispFormatDialogBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg){
		case WM_INITDIALOG:
			EnableTextChangeNotifyItem(hDlg, IDE_FIXMAX);
			InitDispFormat(hDlg, (TCHAR *)lParam);
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDOK:
					if ( IsTrue(SaveDispFormat(hDlg)) ) EndDialog(hDlg, 1);
					break;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;

				case IDL_DFOMAT:
					SeletctNowDispFormat(hDlg, wParam, lParam);
					break;

				case IDB_TB_INSERTTITEM:
					InsertDispFormat(hDlg);
					break;

				case IDB_TB_DELITEM:
					DeleteDispFormat(hDlg);
					break;

				case IDB_MEUP:
					UpDownDispFormat(hDlg, -1);
					break;

				case IDB_MEDW:
					UpDownDispFormat(hDlg, 1);
					break;

				case IDC_DFPARAM1:
					if ( HIWORD(wParam) == CBN_DROPDOWN ){
						if ( IsTrue(ModifyDispColumnFormat(hDlg, (HWND)lParam)) ){
							break;
						}
					}
					// IDC_DFPARAM2 へ
				case IDC_DFPARAM2:
					if ( HIWORD(wParam) == CBN_EDITCHANGE ){
						ModifyDispFormat(hDlg);
					}else if ( HIWORD(wParam) == CBN_SELCHANGE ){
						// 更新完了後にメッセージを受け取るようにする
						PostMessage(hDlg, WM_APP_CB_SELECTED, 0, lParam);
					}
					break;

				case IDX_COMMENTHID:
					if ( HIWORD(wParam) == BN_CLICKED ){
						ModifyDispFormat(hDlg);
					}
					break;

				case IDX_FIXFRAME:
					if ( HIWORD(wParam) == BN_CLICKED ){
						CheckDlgButton(hDlg, IDX_FIXLENGTH, FALSE);
						ModifyDispFormat(hDlg);
						ShowFIXMAX(hDlg, -1);
					}
					break;

				case IDX_FIXLENGTH:
					if ( HIWORD(wParam) == BN_CLICKED ){
						CheckDlgButton(hDlg, IDX_FIXFRAME, FALSE);
						ModifyDispFormat(hDlg);
						ShowFIXMAX(hDlg, -1);
					}
					break;

				case IDE_FIXMAX:
					if ( HIWORD(wParam) == EN_CHANGE ){
						ModifyDispFormat(hDlg);
					}
					break;
			}
			break;

		case WM_APP_CB_SELECTED:
			FixSelectedItem((HWND)lParam);
			ModifyDispFormat(hDlg);
			break;

		case WM_DESTROY:
			PPxRegist(NULL, PPcustRegID, PPXREGIST_FREE);
			// default: へ
		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}

void FormatCellDispSample(TCHAR *data, const TCHAR *label, int offset)
{
	TCHAR format[CMDLINESIZE];

	tstrcpy(data, GetDispFormat(format, label, offset));
}
