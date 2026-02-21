/*-----------------------------------------------------------------------------
	Paper Plane cUI												Where is
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "PPX_64.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPCOMBO.H"
#pragma hdrstop

#define PAUSECOUNT	10

const TCHAR BASEHEADER[] = T(";Base=");
const TCHAR BASETYPEHEADER[] = T("|1\r\n");

const TCHAR *InsertMacroMenuSourceE[] = {
	T(";	path separator"),
	T(".;%path%	program directories"),
	NULL
};

const TCHAR *InsertMacroMenuSourceJ[] = {
	T(";	検索パスの区切り"),
	T(".;%path%	実行可能なディレクトリ全部"),
	NULL
};

const TCHAR **InsertMacroMenuSource[] = {
	InsertMacroMenuSourceE, InsertMacroMenuSourceJ
};

const TCHAR *InsertMacroMenuMaskE[] = {
	T("*	any number of characters"),
	T("?	a specified number of characters"),
	T("/formats/	regular expression"),
	T(",	mask separator"),
	T("r:kanji,	roma-ji"),
	T("&a:r+s+h-,	file attributes(*TOP* readonly,system,no hidden)"),
	T("&d:<=1,	written date(*TOP* 1day)"),
	T("&d:>=2000-1-2,	written date(*TOP* after 2000-1-2)"),
	T("&s:<=10k,	file size(*TOP* in 10,000bytes)"),
	T("&o:x,	no split extention(*TOP*)"),
	T("&o:w,	word seach(a b c OR d, *TOP*)"),
	NULL
};

const TCHAR *InsertMacroMenuMaskJ[] = {
	T("*	0文字以上の任意の文字列"),
	T("?	1文字の任意の文字列"),
	T("/正規表現/	正規表現"),
	T(",	区切り"),
	T("r:kanji,	ローマ字検索"),
	T("&a:r+s+h-,	属性限定(要先頭、readonly,system,hidden無)"),
	T("&d:<=1,	変更日付限定(要先頭、1日以内)"),
	T("&d:>=2000-1-2,	変更日付限定(要先頭、2000年1月2日以降)"),
	T("&s:<=10k,	サイズ限定(要先頭、10,000バイト以内)"),
	T("&o:x,	拡張子を分離しない(要先頭)"),
	T("&o:w,	単語単位(a b c OR d、要先頭)"),
	NULL
};

const TCHAR **InsertMacroMenuMask[] = {
	InsertMacroMenuMaskE, InsertMacroMenuMaskJ
};

const TCHAR *ExWhereMenuList[] = {
	MES_TPTX,	// テキスト検索
	MES_TCMT,	// コメント
	MES_TCIT,	// チップテキスト
	MES_TCLE,	// カラム拡張
	MES_TSMD,
	NULL
};
#define WHERE_COLUMNEXT 1000

typedef struct {
	BOOL result;
	WHERESTRUCT *Pws;
} TIPMATCHSTRUCT;


BOOL CheckStringMatch(const TCHAR *target, WHERESTRUCT *Pws);

typedef struct {
	PPXAPPINFO info;
	WriteTextStruct *sts;
	PPC_APPINFO *cinfo;
	WHERESTRUCT *pws;
} WHEREISAPPINFO;

const TCHAR *swstr[] = { T("off"), T("on") };

void ExWhereMenu(WHERESTRUCT *pws, HWND hDlg, HWND hButtonWnd)
{
	HMENU hMenu, hColumnMenu;
	int exmode;
	const TCHAR **menus;

	if ( PPxCommonExtCommand(K_IsShowButtonMenu, IDS_WHERE_STR) != NO_ERROR ){
		return;
	}

	hMenu = CreatePopupMenu();
	menus = ExWhereMenuList;
	for ( exmode = WDEXM_UNKNOWN ; exmode <= WDEXM_COMMENT ; exmode++, menus++ ){
		AppendMenuString(hMenu, exmode + 1, *menus);
	}
	hColumnMenu = CreatePopupMenu();
	GetColumnExtMenu(&pws->thEcdata, pws->st.cinfo->RealPath, hColumnMenu, WHERE_COLUMNEXT);
	AppendMenu(hMenu, MF_EPOP, (UINT_PTR)hColumnMenu, MessageText(*menus));

	AppendMenuString(hMenu, WDEXM_MODULE, ExWhereMenuList[WDEXM_MODULE - 1]);

	exmode = PopupButtonMenu(hMenu, hDlg, hButtonWnd);
	DestroyMenu(hMenu);

	if ( exmode > WDEXM_UNKNOWN ){
		if ( exmode >= WHERE_COLUMNEXT ){
			pws->indexEc = exmode - WHERE_COLUMNEXT;
			exmode = WDEXM_COLUMNMIN;
		}
		pws->st.exmode = exmode;
		SetWindowText(hButtonWnd, MessageText(ExWhereMenuList[exmode - 1]));
	}
}

#define ID_MBMFIRST 1
#define ID_MBMHELP 0x1000
#define ID_MBM_WFLAT 0x1001
#define ID_MBM_WWORD 0x1002

void MaskButtonMenu(PPC_APPINFO *cinfo, HWND hDlg, HWND hButtonWnd, UINT editID, int type)
{
	if ( PPxCommonExtCommand(K_IsShowButtonMenu, editID) == NO_ERROR ){
		HMENU hPopupMenu;
		const TCHAR ***menulists, *usermenu, **menulist = NULL;
		WildCardOptions option;
		ThSTRUCT PopupTbl;
		DWORD id = ID_MBMFIRST;
		int index;

		ThInit(&PopupTbl);

		if ( type == MaskButton_WhereSrc ){
			usermenu = T("M_wsrc");
			menulists = InsertMacroMenuSource;
		}else{ // MaskButton_GenMask / MaskButton_WhereMask
			usermenu = T("M_mask");
			menulists = InsertMacroMenuMask;
		}
		hPopupMenu = CreatePopupMenu();
		if ( IsExistCustData(usermenu) ){
			PP_AddMenu(&cinfo->info, cinfo->info.hWnd,
					hPopupMenu, &id, usermenu, &PopupTbl);
		}
		if ( id == ID_MBMFIRST ){
			const TCHAR **menustr;

			menulist = menulists[(LOWORD(GetUserDefaultLCID()) == LCID_JAPANESE) ? 1 : 0];

			for ( menustr = menulist ; *menustr != NULL ; menustr++ ){
				AppendMenuString(hPopupMenu, id++, *menustr);
			}
		}

		if ( type != MaskButton_WhereSrc ){ // MaskButton_GenMask / MaskButton_WhereMask
			AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);
			if ( type == MaskButton_WhereMask ){
				GetCustData(StrXC_rmsk, &option, sizeof(option));
				AppendMenuCheckString(hPopupMenu, ID_MBM_WWORD, MES_7885, option.wordmatch );
				AppendMenuCheckString(hPopupMenu, ID_MBM_WFLAT, MES_COSE, !option.flat );
			}
			AppendMenuString(hPopupMenu, ID_MBMHELP, MES_HELP);
		}

		index = PopupButtonMenu(hPopupMenu, hDlg, hButtonWnd);
		DestroyMenu(hPopupMenu);

		if ( index >= ID_MBMHELP ){
			if ( index == ID_MBMHELP ){
				PPxHelp(hDlg, HELP_KEY, (DWORD_PTR)T("wildcard"));
			}else{
				if ( index == ID_MBM_WWORD ){
					option.wordmatch = !option.wordmatch;
					CheckDlgButton(hDlg, IDX_MASK_WORDMATCH, option.wordmatch);
				}else{ // id == ID_MBM_WFLAT
					option.flat = !option.flat;
				}
				SetCustData(StrXC_rmsk, &option, sizeof(option));
			}
		}else{
			TCHAR parambuf[CMDLINESIZE];
			const TCHAR *param = NULL;

			if ( index >= ID_MBMFIRST ){
				if ( menulist != NULL ){ // 組み込みメニュー
					TCHAR *ptr;

					tstrcpy(parambuf, menulist[index - 1]);
					ptr = tstrchr(parambuf, '\t');
					if ( ptr != NULL ) *ptr = '\0';
					param = parambuf;
				}else{ // user menu
					GetMenuDataMacro2(param, &PopupTbl, index - ID_MBMFIRST);
				}
			}

			if ( param != NULL ){
				if ( type >= MaskButton_WhereSrc ){ // MaskButton_WhereSrc, MaskButton_WhereMask
					SendDlgItemMessage(hDlg, editID, EM_REPLACESEL, 1, (LPARAM)param);
				}else{ // MaskButton_GenSrc
					SetDlgItemText(hDlg, editID, param);
				}
			}
		}
		ThFree(&PopupTbl);
	}
	SetFocus(GetDlgItem(hDlg, editID));
}

void WINAPI TipMatchCallback(TIPMATCHSTRUCT *tms, const TCHAR *text)
{
	tms->result = CheckStringMatch(text, tms->Pws);
}

BOOL TipMatch(const TCHAR *name, WHERESTRUCT *Pws)
{
	TIPMATCHSTRUCT tms;
	const TCHAR *ls;

	tms.Pws = Pws;
	ls = VFSFindLastEntry(name);
	GetInfoTipText(name, (ls - name) + FindExtSeparator(ls), (GETINFOTIPCALLBACK)TipMatchCallback, (void *)&tms);
	return tms.result;
}

BOOL ColumnMatch(WHERESTRUCT *Pws, const TCHAR *name, WIN32_FIND_DATA *ff)
{
	TIPMATCHSTRUCT tms;

	tms.Pws = Pws;
	ExtGetData(&Pws->thEcdata, Pws->indexEc, name, ff->dwFileAttributes, (GETINFOTIPCALLBACK)TipMatchCallback, (void *)&tms);
	return tms.result;
}

BOOL FileMatch(const TCHAR *name, WHERESTRUCT *Pws)
{
	TCHAR *image, *text;
	BOOL result;

	if ( NOERROR != LoadTextData(name, &image, &text, NULL, 0) ){
		return FALSE;
	}
	result = CheckStringMatch(text, Pws);
	ProcHeapFree(image);
	return result;
}

BOOL CheckStringMatch(const TCHAR *target, WHERESTRUCT *Pws)
{
	if ( Pws->UseFnText == FALSE ){	// 部分一致
		return tstristr(target, Pws->st.text) ? TRUE : FALSE;
	}else{		// 正規表現
		return FilenameRegularExpression(target, &Pws->fnText);
	}
}

ERRORCODE wjobinfo(WHERESTRUCT *pws, TCHAR *dir)
{
	DWORD NewTime;
	MSG msg;
	TCHAR mesbuf[CMDLINESIZE];

	if ( --pws->pausecount ) return NO_ERROR;
	pws->pausecount = PAUSECOUNT;

	while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ){
		if ( msg.message == WM_QUIT ) break;
		if ( X_MultiThread || (msg.hwnd == pws->hWnd) ){
			if ( (msg.message == WM_RBUTTONUP) ||
				 ((msg.message == WM_KEYDOWN) &&
				 (((int)msg.wParam == VK_ESCAPE)||((int)msg.wParam == VK_PAUSE)))){
				if ( PMessageBox(pws->hWnd, MES_AbortCheck, dir, MB_QYES) == IDOK ){
					return ERROR_CANCELLED;
				}
			}
			if (((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST))||
				((msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST)) ||
				(msg.message == WM_COMMAND) || (msg.message == WM_PPXCOMMAND) ){
				continue;
			}
		}
		if ( DialogKeyProc(&msg) ) continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	NewTime = GetTickCount();
	if ( NewTime > (pws->OldTime + dispNwait) ){
		pws->OldTime = NewTime;
		if ( pws->hitcount ){
			thprintf(mesbuf, TSIZEOF(mesbuf), T("Hit:%d  %s"), pws->hitcount, dir);
		}else{
			tstrcpy(mesbuf, dir);
		}
		SetPopMsg(pws->st.cinfo, POPMSG_PROGRESSBUSYMSG, mesbuf);
		UpdateWindow_Part(pws->st.cinfo->info.hWnd);
	}
	return NO_ERROR;
}

void MarkBo(WHERESTRUCT *pws, HWND hDlg)
{
	if ( IsDlgButtonChecked(hDlg, IDX_WHERE_MARK) ){
		SetDlgItemText(hDlg, IDE_WHERE_SRC, pws->st.cinfo->RealPath);
		EnableDlgWindow(hDlg, IDE_WHERE_SRC, FALSE);
	}else{
		EnableDlgWindow(hDlg, IDE_WHERE_SRC, TRUE);
	}
}

void InitWhereFileMaskText(HWND hDlg, WHERESTRUCT *Pws, UINT control, const TCHAR *text, DWORD wordmatch)
{
	HWND hMaskWnd;

	hMaskWnd = GetDlgItem(hDlg, control);
	PPxRegistExEdit(&Pws->st.cinfo->info, hMaskWnd, TSIZEOFSTR(Pws->st.mask1),
		(wordmatch && (memcmp(text, StrWordMatchWildCard, SIZEOFTSTR(StrWordMatchWildCard) ) == 0)) ? (text + TSIZEOFSTR(StrWordMatchWildCard)) : text,
		PPXH_WILD_R, PPXH_MASK, 0);
	PostMessage(hMaskWnd, WM_PPXCOMMAND, K_E_LOAD, 0);
}

#define ID_INIT_COMMAND 0xc106
void InitWhereDialog(HWND hDlg, LPARAM lParam)
{
	WHERESTRUCT *PWS;
	WildCardOptions option;
	HWND hStrWnd;

	PWS = (WHERESTRUCT *)lParam;
	SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
	CenterPPcDialog(hDlg, PWS->st.cinfo);
	LocalizeDialogText(hDlg, IDD_WHERE);

	GetCustData(StrXC_rmsk, &option, sizeof(option));

	PWS->hEdWnd = PPxRegistExEdit(&PWS->st.cinfo->info,
		GetDlgItem(hDlg, IDE_WHERE_SRC),
		TSIZEOFSTR(PWS->srcpath), PWS->srcpath, PPXH_DIR_R, PPXH_DIR,
		PPXEDIT_WANTEVENT | PPXEDIT_REFTREE);
	InitWhereFileMaskText(hDlg, PWS, IDE_WHERE_F1, PWS->st.mask1, option.wordmatch);
	InitWhereFileMaskText(hDlg, PWS, IDE_WHERE_F2, PWS->st.mask2, option.wordmatch);
	hStrWnd = GetDlgItem(hDlg, IDE_WHERE_STR);
	PPxRegistExEdit(&PWS->st.cinfo->info, hStrWnd,
		TSIZEOFSTR(PWS->st.text), PWS->st.text, PPXH_SEARCH_R, PPXH_SEARCH, 0);
	PostMessage(hStrWnd, WM_PPXCOMMAND, K_E_LOAD, 0);

	CheckDlgButton(hDlg, IDX_WHERE_MARK, PWS->st.marked);
	CheckDlgButton(hDlg, IDX_WHERE_DIR, PWS->st.dir);
	CheckDlgButton(hDlg, IDX_WHERE_SDIR, PWS->st.sdir);
	CheckDlgButton(hDlg, IDX_WHERE_VFS, PWS->vfs);
	CheckDlgButton(hDlg, IDX_MASK_WORDMATCH, option.wordmatch);
	MarkBo(PWS, hDlg);
	SetFocus(GetDlgItem(hDlg, IDE_WHERE_F1));

	if ( (PWS->st.exmode == WDEXM_UNKNOWN) ||
		 (PWS->st.exmode == WDEXM_COLUMNMIN) ){
		PWS->st.exmode = WDEXM_PLAINTEXT;
	}

	SetDlgItemText(hDlg, IDS_WHERE_STR, MessageText(ExWhereMenuList[PWS->st.exmode - 1]));
	ActionInfo(hDlg, &PWS->st.cinfo->info, AJI_SHOW, T("whereis"));

	if ( PWS->initcmd != NULL ){
		PostMessage(hDlg, WM_COMMAND, ID_INIT_COMMAND, 0);
	}
}

void GetMaskText(TCHAR *edittextbuf, TCHAR *gettext, WildCardOptions *option)
{
	int offset = 0;

	edittextbuf[MAX_PATH - 1] = '\0';

	// 行頭コメントを除去
	if ( (edittextbuf[0] == ';') && (edittextbuf[1] == '<') ){
		TCHAR *p;

		p = tstrstr(edittextbuf + 2, T(">;"));
		if ( p != NULL ){
			p += 2;
			if ( *p == ' ' ) p++;
			memmove(edittextbuf, p, TSTRSIZE(p));
		}
	}

	// 行頭にオプションあり…オプション部分をスキップして記載があるか確認
	if ( (edittextbuf[0] == 'o') && (edittextbuf[1] == ':') ){
		TCHAR *p;

		gettext[0] = '\0';
		p = edittextbuf + 2;
		for(;;){
			TCHAR chr;

			chr = *p;
			if ( chr == '\0' ) return; // ワイルドカード記載なし
			if ( (chr == ',') || (chr == ';') ){
				if ( *(p + 1) == '\0' ) return; // ワイルドカード記載なし
				break; // ワイルドカード記載あり
			}
			p++;
		}
	}else{
		if ( edittextbuf[0] == '\0' ){ // ワイルドカード記載なし
			gettext[0] = '\0';
			return;
		}
		// デフォルトオプションを設定
		if ( (option->wordmatch || option->flat) ){
			gettext[0] = 'o';
			gettext[1] = ':';
			gettext[2] = 'e';
			offset = 3;
			if ( option->wordmatch ) gettext[offset++] = 'w';
			if ( option->flat ) gettext[offset++] = 'x';
			gettext[offset++] = ',';
		}
	}
	tstplimcpy(gettext + offset, edittextbuf, MAX_PATH - offset);
}

void GetMaskTextFromEdit(HWND hDlg, UINT control, TCHAR *gettext, WildCardOptions *option)
{
	TCHAR edittextbuf[MAX_PATH];

	GetDlgItemText(hDlg, control, edittextbuf, MAX_PATH);
	GetMaskText(edittextbuf, gettext, option);
	WriteHistory(PPXH_MASK, gettext, 0, NULL);
}

void OkWhereDialog(HWND hDlg, WHERESTRUCT *PWS)
{
	WildCardOptions option;

	memset(&option, 0, sizeof(option));
	GetCustData(StrXC_rmsk, &option, sizeof(option));
	option.wordmatch = IsDlgButtonChecked(hDlg, IDX_MASK_WORDMATCH);
	SetCustData(StrXC_rmsk, &option, sizeof(option));

	GetDlgItemText(hDlg, IDE_WHERE_SRC, PWS->srcpath, TSIZEOF(PWS->srcpath));
	WriteHistory(PPXH_DIR, PWS->srcpath, 0, NULL);

	GetMaskTextFromEdit(hDlg, IDE_WHERE_F1, PWS->st.mask1, &option);
	GetMaskTextFromEdit(hDlg, IDE_WHERE_F2, PWS->st.mask2, &option);
	GetDlgItemText(hDlg, IDE_WHERE_STR, PWS->st.text, TSIZEOF(PWS->st.text));
	WriteHistory(PPXH_SEARCH, PWS->st.text, 0, NULL);

	PWS->st.marked = IsDlgButtonChecked(hDlg, IDX_WHERE_MARK);
	PWS->st.dir = IsDlgButtonChecked(hDlg, IDX_WHERE_DIR);
	PWS->st.sdir = IsDlgButtonChecked(hDlg, IDX_WHERE_SDIR) ?
			FILE_ATTRIBUTE_DIRECTORY : 0;
	PWS->vfs = IsDlgButtonChecked(hDlg, IDX_WHERE_VFS);
	EndDialog(hDlg, 1);
}

INT_PTR CALLBACK WhereDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message){
							// ダイアログ初期化 (w:focus, l:User からの param)--
		case WM_INITDIALOG:
			InitWhereDialog(hDlg, lParam);
			return FALSE;

		case WM_COMMAND: {
			WHERESTRUCT *PWS;

			PWS = (WHERESTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER);
			switch ( LOWORD(wParam) ){
				case IDX_WHERE_MARK:
					MarkBo(PWS, hDlg);
					break;

				case IDOK:
					OkWhereDialog(hDlg, PWS);
					break;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;

				case IDS_WHERE_SRC:
					if ( HIWORD(wParam) == BN_CLICKED ){
						MaskButtonMenu(PWS->st.cinfo, hDlg, (HWND)lParam, IDE_WHERE_SRC, MaskButton_WhereSrc);
					}
					break;
				case IDS_WHERE_F1:
					if ( HIWORD(wParam) == BN_CLICKED ){
						MaskButtonMenu(PWS->st.cinfo, hDlg, (HWND)lParam, IDE_WHERE_F1, MaskButton_WhereMask);
					}
					break;
				case IDS_WHERE_F2:
					if ( HIWORD(wParam) == BN_CLICKED ){
						MaskButtonMenu(PWS->st.cinfo, hDlg, (HWND)lParam, IDE_WHERE_F2, MaskButton_WhereMask);
					}
					break;

				case IDS_WHERE_STR:
					if ( HIWORD(wParam) == BN_CLICKED ){
						ExWhereMenu(PWS, hDlg, (HWND)lParam);
						SetFocus(GetDlgItem(hDlg, IDE_WHERE_STR));
					}
					break;

				case IDB_REF:
					SendMessage(PWS->hEdWnd,
							WM_PPXCOMMAND, K_raw | K_s | K_c | 'I', 0);
					break;

				case IDHELP:
					return PPxDialogHelper(hDlg, WM_HELP, wParam, lParam);

				case IDQ_GETDIALOGID:
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)IDD_WHERE);
					break;

				case ID_INIT_COMMAND:
					SendMessage(PWS->hEdWnd, WM_PPXCOMMAND, K_EXECUTE, (LPARAM)PWS->initcmd);
					PPcHeapFree(PWS->initcmd);
					PWS->initcmd = NULL;
					break;
			}
			break;
		}

		case WM_SYSCOMMAND:
			if ( wParam == SC_KEYMENU ){
				WHERESTRUCT *PWS;

				PWS = (WHERESTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER);
				if ( PWS != NULL ){
					SendMessage(PWS->hEdWnd, WM_PPXCOMMAND,
							(WPARAM)(upper((TCHAR)lParam) | GetShiftKey()), 0);
				}
				break;
			}
			// default:
		default:
			return PPxDialogHelper(hDlg, message, wParam, lParam);
	}
	return TRUE;
}

void WriteFF_basic(WriteTextStruct *sts, const TCHAR *name)
{
	TCHAR buf[VFPS + 900], *dest;
	WIN32_FIND_DATA *ff;

	buf[0] = T('\"');
	dest = tstpcpy(buf + 1, name);

	ff = sts->w.ff;
	dest = thprintf(dest, 900, ListFileFormatStr,
			ff->cAlternateFileName, ff->dwFileAttributes,
			ff->ftCreationTime.dwHighDateTime,
			ff->ftCreationTime.dwLowDateTime,
			ff->ftLastAccessTime.dwHighDateTime,
			ff->ftLastAccessTime.dwLowDateTime,
			ff->ftLastWriteTime.dwHighDateTime,
			ff->ftLastWriteTime.dwLowDateTime,
			ff->nFileSizeHigh, ff->nFileSizeLow);

	if ( sts->w.comment != NULL ){
		dest = WriteLFcomment(sts, sts->w.comment, buf, dest);
	}
	*dest++ = '\r';
	*dest++ = '\n';
	sts->Write(sts, buf, dest - buf);
}

void WriteFF_name(WriteTextStruct *sts, const TCHAR *name)
{
	TCHAR buf[VFPS + 900], *dest;

	if ( sts->wlfc_flags & WLFC_JSON ){
		WIN32_FIND_DATA *ff;
		TCHAR *dest;

		dest = thprintf(buf, TSIZEOF(buf), T("{\"N\":\"%s\","), name);
		ff = sts->w.ff;
		dest = thprintf(dest, 900,
				T("\"Y\":\"%s\",\"A\":%u,\"C\":\"%u.%u\",\"L\":\"%u.%u\",\"W\":\"%u.%u\",\"S\":"),
				ff->cAlternateFileName, ff->dwFileAttributes,
				ff->ftCreationTime.dwHighDateTime,
				ff->ftCreationTime.dwLowDateTime,
				ff->ftLastAccessTime.dwHighDateTime,
				ff->ftLastAccessTime.dwLowDateTime,
				ff->ftLastWriteTime.dwHighDateTime,
				ff->ftLastWriteTime.dwLowDateTime);
		if ( ff->nFileSizeHigh < DOUBLEMAXH ){
			dest = FormatNumber(dest, 0, XFNW_FULL_NOSEP, ff->nFileSizeLow, ff->nFileSizeHigh);
		}else{
			dest = thprintf(dest, 32, T("\"%u.%u\""), ff->nFileSizeHigh, ff->nFileSizeLow);
		}
		if ( sts->w.comment != NULL ){
			dest = WriteLFcomment(sts, sts->w.comment, buf, dest);
		}
		*dest++ = '}';
		*dest++ = ',';
		*dest++ = '\r';
		*dest++ = '\n';
		sts->Write(sts, buf, dest - buf);
	}else{
		dest = tstpcpy(buf, name);
		if ( sts->w.ff->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) *dest++ = '\\';
		if ( sts->w.comment != NULL ){
			dest = WriteLFcomment(sts, sts->w.comment, buf, dest);
		}
		*dest++ = '\r';
		*dest++ = '\n';
		sts->Write(sts, buf, dest - buf);
	}
}

void WriteFF2(WriteTextStruct *sts, const TCHAR *name)
{
	if ( !(sts->wlfc_flags & (WLFC_NAMEONLY | WLFC_JSON)) ){
		WriteFF_basic(sts, name);
	}else{
		WriteFF_name(sts, name);
	}
}

void CheckText(WHERESTRUCT *Pws, WriteTextStruct *sts, const TCHAR *name)
{
	if ( Pws->st.exmode >= WDEXM_TIPTEXT ){
		if ( Pws->st.exmode == WDEXM_COLUMNMIN ){
			if ( IsTrue(ColumnMatch(Pws, name, sts->w.ff)) ){
				Pws->hitcount++;
				WriteFF2(sts, name + Pws->baselen);
			}
		}else if ( IsTrue(TipMatch(name, Pws)) ){
			Pws->hitcount++;
			WriteFF2(sts, name + Pws->baselen);
		}
	}else{
		if ( !sts->w.ff->nFileSizeHigh &&
			 (sts->w.ff->nFileSizeLow < Pws->imgsize) &&
			 IsTrue(FileMatch(name, Pws)) ){
			Pws->hitcount++;
			WriteFF2(sts, name + Pws->baselen);
		}
	}
}

ERRORCODE WhereIsDirComment(TCHAR *dir, WHERESTRUCT *pws, WriteTextStruct *sts)
{
	HANDLE hFF;	// FindFile 用ハンドル
	WIN32_FIND_DATA ff;		// ファイル情報
	TCHAR src[VFPS];
	TCHAR name[VFPS];
	ThSTRUCT comments;
	ERRORCODE result;
	int dirlen = tstrlen32(dir);

	if ( (dirlen + 4) >= VFPS ) return NO_ERROR; // OVER_VFPS_MSG
	// このディレクトリのコメントファイルのチェック

	ThInit(&comments);
	CatPath(src, dir, Str_CommentFile);
	GetCommentText(&comments, src);
	if ( comments.bottom != NULL ){
		COMMENTENTRY *p;

		sts->w.ff = &ff;
		p = (COMMENTENTRY *)comments.bottom;
		while( p->nextoffset != 0 ){
			if ( FilenameRegularExpression(p->name, &pws->fn1) &&
				 FilenameRegularExpression(p->name, &pws->fn2) ){
				TCHAR *cp;

				cp = p->name + p->comment / sizeof(TCHAR);
				if ( CheckStringMatch(cp, pws) ){
					VFSFullPath(name, p->name, dir);
					if ( IsTrue(GetFileSTAT(src, &ff)) ){
						pws->hitcount++;
						sts->w.comment = cp;
						WriteFF2(sts, name + pws->baselen);
					}
				}
			}
			p = (COMMENTENTRY *)ThPointer(&comments, p->nextoffset);
		}
	}
	ThFree(&comments);

	CatPath(src, dir, T("*"));
	hFF = FindFirstFileL(src, &ff);
	if ( hFF == INVALID_HANDLE_VALUE ) return NO_ERROR;
	do{
		if ( IsRelativeDir(ff.cFileName) ) continue;
		if ( (dirlen + tstrlen(ff.cFileName) + 4) >= VFPS ) continue; // OVER_VFPS_MSG
		CatPath(name, dir, ff.cFileName);

		if ( ff.dwFileAttributes & pws->st.sdir ){
			if ( (result = WhereIsDirComment(name, pws, sts)) != NO_ERROR ){
				goto end;
			}
			if ( (result = wjobinfo(pws, name)) != NO_ERROR ){
				goto end;
			}
		}
	}while( IsTrue(FindNextFile(hFF, &ff)) );
	result = NO_ERROR;
end:
	FindClose(hFF);
	return result;
}

// エントリに対するコメント検索
void WhereIsCellComment(PPC_APPINFO *cinfo, ENTRYCELL *cell, WHERESTRUCT *pws, WriteTextStruct *sts)
{
	if ( FinddataRegularExpression(&cell->f, &pws->fn1) &&
		 FinddataRegularExpression(&cell->f, &pws->fn2) ){
		if ( (cell->comment != EC_NOCOMMENT) &&
		 	 CheckStringMatch(ThPointerT(&cinfo->e.Comments, cell->comment), pws) ){
			pws->hitcount++;
			sts->w.comment = ThPointerT(&cinfo->e.Comments, cell->comment);
			sts->w.ff = &cell->f;
			WriteFF2(sts, cell->f.cFileName);
		}
	}
}

// 現在パスに対するコメント検索
ERRORCODE WhereIsThisPathComment(PPC_APPINFO *cinfo, WHERESTRUCT *pws, WriteTextStruct *sts)
{
	ENTRYINDEX index;
	ENTRYCELL *cell;
	ERRORCODE result = NO_ERROR;

	for ( index = 0; index < cinfo->e.cellIMax ; index++){
		cell = &CELdata(index);

		if ( IsRelativeDir(cell->f.cFileName) ) continue;
		if ( cell->state == ECS_DELETED ) continue;

		WhereIsCellComment(cinfo, cell, pws, sts);

		if ( cell->f.dwFileAttributes & pws->st.sdir ){
			TCHAR name[VFPS];

			CatPath(name, pws->srcpath, cell->f.cFileName);
			if ( (result = WhereIsDirComment(name, pws, sts)) != NO_ERROR ){
				break;
			}
			if ( (result = wjobinfo(pws, name)) != NO_ERROR ){
				break;
			}
		}
	}
	return result;
}

ERRORCODE WhereIsDir(const TCHAR *dir, WHERESTRUCT *pws, WriteTextStruct *sts)
{
	HANDLE hFF;	// FindFile 用ハンドル
	WIN32_FIND_DATA ff;	// ファイル情報
	TCHAR src[VFPS];
	TCHAR name[VFPS];
	ERRORCODE result;
	int dirlen = tstrlen32(dir);

	if ( (dirlen + 4) >= VFPS ) return NO_ERROR; // return ERROR_FILENAME_EXCED_RANGE; OVER_VFPS_MSG
	CatPath(src, (TCHAR *)dir, T("*"));
	hFF = FindFirstFileL(src, &ff);
	if ( hFF == INVALID_HANDLE_VALUE ) return NO_ERROR; // return ERROR_PATH_NOT_FOUND;
	sts->w.ff = &ff;
	do{
		if ( IsRelativeDir(ff.cFileName) ) continue;
		if ( (dirlen + tstrlen(ff.cFileName) + 4) >= VFPS ) continue; // OVER_VFPS_MSG

		name[MAX_PATH] = '\0'; // 名前の大きさ検出用
		CatPath(name, (TCHAR *)dir, ff.cFileName);

		if ( FinddataRegularExpression(&ff, &pws->fn1) &&
			 FinddataRegularExpression(&ff, &pws->fn2) ){
			if ( pws->st.dir || !(ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ){
				if ( !(ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
						pws->st.text[0] ){
					CheckText(pws, sts, name);
				}else{
					pws->hitcount++;

					if ( (name[MAX_PATH] != '\0') && !memcmp(name, pws->srcpath, TSTRLENGTH(pws->srcpath)) ){
						TCHAR *p = name + tstrlen(pws->srcpath); // 共通パスを省略して名前を少し縮める
						if ( *p == '\\' ) p++;
						WriteFF2(sts, p);
					}else{
						WriteFF2(sts, name + pws->baselen);
					}
				}
			}
		}

		if ( ff.dwFileAttributes & pws->st.sdir ){
			if ( (result = WhereIsDir(name, pws, sts)) != NO_ERROR ){
				goto end;
			}
			sts->w.ff = &ff;
		}

		if ( (result = wjobinfo(pws, name)) != NO_ERROR ){
			goto end;
		}
	}while( IsTrue(FindNextFile(hFF, &ff)) );
	result = NO_ERROR;
end:
	FindClose(hFF);
	return result;
}

BOOL IsWhereIsVFSDir(const TCHAR *path)
{
	TCHAR pathbuf[VFPS];
	BYTE header[VFS_check_size];
	DWORD fsize;

	fsize = GetFileHeader(path, header, sizeof(header));
	if ( fsize == 0 ) return FALSE;
	tstrcpy(pathbuf, path);
	return VFSCheckDir(pathbuf, header, fsize, NULL);
}

ERRORCODE WhereIsDirVFS(TCHAR *dir, WHERESTRUCT *pws, WriteTextStruct *sts)
{
	HANDLE hFF;	// FindFile 用ハンドル
	WIN32_FIND_DATA ff;	// ファイル情報
	TCHAR src[VFPS];
	TCHAR name[VFPS];
	ERRORCODE result;
	int dirlen = tstrlen32(dir);

	if ( (dirlen + 4) >= VFPS ) return NO_ERROR; // OVER_VFPS_MSG
	CatPath(src, dir, T("*"));
	hFF = VFSFindFirst(src, &ff);
	if ( hFF == INVALID_HANDLE_VALUE ) return NO_ERROR;
	sts->w.ff = &ff;

	do{
		if ( IsRelativeDir(ff.cFileName) ) continue;
		if ( (dirlen + tstrlen(ff.cFileName) + 4) >= VFPS ) continue; // OVER_VFPS_MSG
		CatPath(name, dir, ff.cFileName);

		if ( FinddataRegularExpression(&ff, &pws->fn1) &&
			 FinddataRegularExpression(&ff, &pws->fn2) ){
			if ( pws->st.dir || !(ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ){
				if ( !(ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
						pws->st.text[0] ){
					CheckText(pws, sts, name);
				}else{
					pws->hitcount++;
					WriteFF2(sts, name + pws->baselen);
				}
			}
		}

		if ( (result = wjobinfo(pws, name)) != NO_ERROR ){
			goto end;
		}

		if ( (pws->st.sdir) &&
				(IsWhereIsVFSDir(name) ||
				 (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) ){
			if ( (result = WhereIsDirVFS(name, pws, sts)) != NO_ERROR ){
				goto end;
			}
			sts->w.ff = &ff;
		}
	}while( IsTrue(VFSFindNext(hFF, &ff)) );
	result = NO_ERROR;
end:
	VFSFindClose(hFF);
	return result;
}

DWORD_PTR USECDECL SearchReportModuleFunction(WHEREISAPPINFO *winfo, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	TCHAR buf[VFPS + 900], *name, *last;

	if ( cmdID == PPXCMDID_REPORTSEARCH_FDATA ){
		name = ((WIN32_FIND_DATA *)uptr)->cFileName;
		winfo->sts->w.ff = (WIN32_FIND_DATA *)uptr;
		WriteFF2(winfo->sts, name);
	}else if ( (cmdID == PPXCMDID_REPORTSEARCH) ||
		 (cmdID == PPXCMDID_REPORTSEARCH_FILE) ||
		 (cmdID == PPXCMDID_REPORTSEARCH_DIRECTORY) ){
		if ( cmdID == PPXCMDID_REPORTSEARCH ){
			last = thprintf(buf, TSIZEOF(buf), T("\"%s\"\r\n"), uptr->str);
		}else{
			last = thprintf(buf, TSIZEOF(buf), T("\"%s\",A:%d\r\n"), uptr->str,
				(cmdID == PPXCMDID_REPORTSEARCH_DIRECTORY) ?
					FILE_ATTRIBUTE_DIRECTORY : 0);
		}
		winfo->sts->Write(winfo->sts, buf, last - buf);
		name = uptr->str;
	}else{
		return winfo->cinfo->info.Function((PPXAPPINFO *)winfo->cinfo, cmdID, uptr);
	}
	winfo->pws->hitcount++;
	return (wjobinfo(winfo->pws, name) == NO_ERROR) ? PPXA_NO_ERROR : PPXARESULT(ERROR_PATH_NOT_FOUND);
}

const TCHAR * USEFASTCALL GetSWstr(BOOL state)
{
	return swstr[state ? 1 : 0];
}

void FreePws(WHERESTRUCT *Pws)
{
	if ( Pws->thEcdata.bottom != NULL ){
		GetColumnExtMenu(&Pws->thEcdata, NULL, NULL, 0); // thEcdata 解放
	}
	if ( Pws->st.exmode == WDEXM_COLUMNMIN ) Pws->st.exmode = WDEXM_UNKNOWN;
	if ( Pws->initcmd != NULL ) PPcHeapFree(Pws->initcmd);
}

ERRORCODE WhereIsMain(PPC_APPINFO *cinfo, WHERESTRUCT *Pws)
{
	WriteTextStruct sts;
	TCHAR templistfile[VFPS], temp[CMDLINESIZE * 3];
	DWORD tmp;
	int fixflags;
	ERRORCODE result = NO_ERROR;
	#define fixflags_Normal (VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_NOFIXEDGE | VFSFIX_REALPATH)
	#define fixflags_VFS (VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_NOFIXEDGE)

	Pws->pausecount = PAUSECOUNT;
	Pws->hitcount = 0;
	Pws->hWnd = cinfo->info.hWnd;
	Pws->OldTime = GetTickCount();
	Pws->imgsize = GetCustXDword(T("X_wsiz"), NULL, IMAGESIZELIMIT);
	Pws->st.cinfo = cinfo;
	Pws->baselen = 0;
	sts.w.comment = NULL;

	fixflags = Pws->vfs;
	if ( (fixflags == WHEREIS_NORMAL) &&
		 (tstrcmp(Pws->srcpath, cinfo->path) == 0) &&
		 ((cinfo->e.Dtype.mode >= VFSDT_FATIMG) && (cinfo->e.Dtype.mode <= VFSDT_CDDISK)) ){
		fixflags = WHEREIS_INVFS;
	}
	fixflags = (fixflags == WHEREIS_NORMAL) ? fixflags_Normal : fixflags_VFS;

	if ( Pws->listfilename != NULL ){
		tstrcpy(templistfile, Pws->listfilename);
	}else{
		MakeTempEntry(TSIZEOF(templistfile), templistfile, FILE_ATTRIBUTE_NORMAL);
	}
	sts.hFile = CreateFileL(templistfile, GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			(Pws->wlfc_flags & WLFC_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS,
			FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( sts.hFile == INVALID_HANDLE_VALUE ){
		result = GetLastError();
		thprintf(temp, TSIZEOF(temp), T("%s: %Mm"), templistfile, result);
		SetPopMsg(Pws->st.cinfo, POPMSG_MSG, temp);
		goto fin;
	}

	if ( Pws->wlfc_flags & WLFC_APPEND ){
		DWORD high = 0, low;

		sts.wlfc_flags = Pws->wlfc_flags;
		sts.Write = (sts.wlfc_flags & WLFC_UTF8) ? WriteTextUTF8 : WriteTextNative;

		low = SetFilePointer(sts.hFile, 0, (PLONG)&high, FILE_END);
		if ( (low != MAX32) && ((low | high) != 0) ){
			setflag(sts.wlfc_flags, WLFC_NOHEADER);
		}
	}else{
		WriteListFileHeader(&sts, Pws->wlfc_flags);
	}

	if ( !(sts.wlfc_flags & WLFC_NOHEADER) ){
		VFSFixPath(temp, Pws->srcpath, cinfo->path, fixflags);
		if ( GetFileAttributesL(temp) != BADATTR ){
			TCHAR temp2[CMDLINESIZE], *last2;

			last2 = thprintf(temp2, TSIZEOF(temp2),
					(sts.wlfc_flags & WLFC_JSON) ?
							T("\"Base\":\"%s|1\",\r\n") : T(";Base=%s|1\r\n"),
							temp);
			sts.Write(&sts, temp2, last2 - temp2);
		}

		tmp = thprintf(temp, TSIZEOF(temp), (sts.wlfc_flags & WLFC_JSON) ?
				 T("\"Search\":\"-type:%d -marked:%s -dir:%s -subdir:%s -vfs:%s -path:%s -mask:%s -mask2:%s -text:%s\",\r\n") :
				 T(";Search=-type:%d -marked:%s -dir:%s -subdir:%s -vfs:%s -path:\"%s\" -mask:\"%s\" -mask2:\"%s\" -text:\"%s\"\r\n"),
			Pws->st.exmode,
			GetSWstr(Pws->st.marked), GetSWstr(Pws->st.dir),
			GetSWstr(Pws->st.sdir), GetSWstr(Pws->vfs),
			Pws->srcpath, Pws->st.mask1, Pws->st.mask2, Pws->st.text) - temp;
		sts.Write(&sts, temp, tmp);
	}
	if ( sts.wlfc_flags & WLFC_JSON ) sts.Write(&sts, T("\"entry\":[\r\n"), 11);

	if ( Pws->st.exmode == WDEXM_MODULE ){
		PPXMSEARCHSTRUCT msearch;
		WHEREISAPPINFO winfo = {{(PPXAPPINFOFUNCTION)SearchReportModuleFunction, T("Where is"), NilStr, NULL}, NULL, NULL, NULL};
		PPXMODULEPARAM pmp;

		#ifndef UNICODE
			WCHAR keywordW[VFPS];

			AnsiToUnicode(Pws->st.mask1[0] ? Pws->st.mask1 : Pws->st.text, keywordW, VFPS);
			msearch.keyword = keywordW;
		#else
			msearch.keyword = Pws->st.mask1[0] ? Pws->st.mask1 : Pws->st.text;
		#endif
		msearch.maxresults = MAX32;
		msearch.searchtype = PPXH_PATH | PPXH_DIR;
		winfo.info.hWnd = cinfo->info.hWnd;
		winfo.sts = &sts;
		winfo.cinfo = cinfo;
		winfo.pws = Pws;
		pmp.search = &msearch;
		CallModule(&winfo.info, PPXMEVENT_SEARCH, pmp, NULL);
	}else{
		MakeFN_REGEXP(&Pws->fn1, Pws->st.mask1);
		MakeFN_REGEXP(&Pws->fn2, Pws->st.mask2);

		Pws->UseFnText = FALSE;
		if ( Pws->st.text[0] == '\0' ){
			Pws->st.exmode = WDEXM_UNKNOWN;
		}else{
			if ( Pws->st.text[0] == '/' ){
				Pws->UseFnText = TRUE;
				thprintf(temp, TSIZEOF(temp), T("o:,%s"), Pws->st.text);
				MakeFN_REGEXP(&Pws->fnText, temp);
			}
		}

		if ( Pws->st.marked != 0 ){		// マーク処理あり
			ENTRYCELL *cell;
			int work;

			InitEnumMarkCell(cinfo, &work);
			while ( (cell = EnumMarkCell(cinfo, &work)) != NULL ){
				if ( IsRelativeDir(cell->f.cFileName) ) continue;
				if ( cell->state == ECS_DELETED ) continue;
				if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
					VFSFullPath(temp, cell->f.cFileName, cinfo->RealPath);
					if ( Pws->st.exmode == WDEXM_COMMENT ){
						WhereIsCellComment(cinfo, cell, Pws, &sts);
						result = WhereIsDirComment(temp, Pws, &sts);
					}else if ( fixflags & VFSFIX_REALPATH ){
						result = WhereIsDir(temp, Pws, &sts);
					}else{
						result = WhereIsDirVFS(temp, Pws, &sts);
					}
					if ( result != NO_ERROR ) break;
				}else{
					if ( Pws->st.exmode == WDEXM_COMMENT ){
						WhereIsCellComment(cinfo, cell, Pws, &sts);
					}else if (
							FinddataRegularExpression(&cell->f, &Pws->fn1) &&
							FinddataRegularExpression(&cell->f, &Pws->fn2) ){
						VFSFullPath(temp, cell->f.cFileName, cinfo->RealPath);
						sts.w.ff = &cell->f;
						if ( Pws->st.text[0] != '\0' ){
							CheckText(Pws, &sts, temp);
						}else{
							Pws->hitcount++;
							WriteFF2(&sts, temp);
						}
					}

					if ( !(fixflags & VFSFIX_REALPATH) && (Pws->st.sdir) ){
						if ( Pws->st.exmode == WDEXM_COMMENT ){
							VFSFullPath(temp, cell->f.cFileName, cinfo->RealPath);
						}
						if ( IsWhereIsVFSDir(temp) ){
							if ( (result = WhereIsDirVFS(temp, Pws, &sts)) != NO_ERROR ){
								break;
							}
						}
					}

					if ( (result = wjobinfo(Pws, cell->f.cFileName)) != NO_ERROR ){
						break;
					}
				}
			}
		}else{					// マーク処理なし
			TCHAR *pathsrc, *pathnext, extractpath[VFPS];
			BOOL multipath = TRUE;

			if ( (Pws->st.exmode == WDEXM_COMMENT) &&
				 (tstrcmp(Pws->srcpath, cinfo->path) == 0) ){
				result = WhereIsThisPathComment(cinfo, Pws, &sts);
			}else{
				if ( (tstrchr(Pws->srcpath, '%') != NULL) && // 環境変数がありそう
					 (VFSFullPath(temp, Pws->srcpath, cinfo->path) != NULL) && // 絶対化可能
					 (GetFileAttributesL(temp) == BADATTR ) && // 実体無し
					 (tstrstr(temp, T("//")) == NULL) ){ // http:// 等でない
					if ( TSIZEOF(temp) <= ExpandEnvironmentStrings(Pws->srcpath, temp, TSIZEOF(temp)) ){
						tstrcpy(temp, Pws->srcpath);
					}
				}else{
					tstrcpy(temp, Pws->srcpath);
				}
				if ( (VFSFullPath(extractpath, Pws->srcpath, cinfo->path) != NULL) && // 絶対化可能
					 (GetFileAttributesL(temp) != BADATTR ) ){ // 実体あり
					multipath = FALSE;
					pathnext = NULL;
					tstrcpy(temp, extractpath);
				}

				pathsrc = temp;
				for ( ; ; ){
					if ( multipath ){
						pathnext = tstrchr(pathsrc, ';');
						if ( pathnext != NULL ) *pathnext = '\0';
						VFSFixPath(extractpath, pathsrc, cinfo->path, fixflags);
					}
					if ( Pws->st.exmode == WDEXM_COMMENT ){
						result = WhereIsDirComment(extractpath, Pws, &sts);
					}else if ( fixflags & VFSFIX_REALPATH ){
						result = WhereIsDir(extractpath, Pws, &sts);
					}else{
						result = WhereIsDirVFS(extractpath, Pws, &sts);
					}
					if ( pathnext == NULL ) break;
					if ( result != NO_ERROR ) break;
					pathsrc = pathnext + 1;
				}
			}
		}

		if ( IsTrue(Pws->UseFnText) ) FreeFN_REGEXP(&Pws->fnText);

		FreeFN_REGEXP(&Pws->fn2);
		FreeFN_REGEXP(&Pws->fn1);
	}
	if ( sts.wlfc_flags & WLFC_JSON ) sts.Write(&sts, T("{}\r\n]\r\n}\r\n"), 6);
	CloseHandle(sts.hFile);
	StopPopMsg(cinfo, PMF_STOP);

	if ( IsTrue(Pws->readlistfile) ){
		PPcChangeDirectory(cinfo, templistfile, RENTRY_READ);
		if ( cinfo->e.Dtype.BasePath[0] != '\0' ){
			tstrcpy(cinfo->OrgPath, cinfo->e.Dtype.BasePath);
		}
		ActionInfo(cinfo->info.hWnd, &cinfo->info, AJI_COMPLETE, T("whereis"));
	}
fin:
	FreePws(Pws);
	return result;
}

ERRORCODE WhereIsDialogMain(WHERESTRUCT *Pws)
{
	if ( PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_WHERE),
			Pws->st.cinfo->info.hWnd, WhereDialogProc, (LPARAM)Pws) <= 0 ){
		FreePws(Pws);
		return ERROR_CANCELLED;
	}
	Pws->st.cinfo->WhereIsSettings = Pws->st;
	return WhereIsMain(Pws->st.cinfo, Pws);
}

ERRORCODE WhereIsDialog(PPC_APPINFO *cinfo, int mode)
{
	WHERESTRUCT ws;

	ThInit(&ws.thEcdata);

		// Shell's Namespace だったら仮想モードに移行
	if ( cinfo->RealPath[0] == '?' ) mode = WHEREIS_INVFS;

	ws.st = cinfo->WhereIsSettings;
	if ( ws.st.cinfo == NULL ){	// 新規の場合は初期化
		ws.st.cinfo = cinfo;
		ws.st.marked = FALSE;
		ws.st.dir = FALSE;
		ws.st.sdir = FILE_ATTRIBUTE_DIRECTORY;
		ws.st.exmode = WDEXM_PLAINTEXT;
		ws.st.mask1[0] = '\0';
		ws.st.mask2[0] = '\0';
		ws.st.text[0] = '\0';
	}

	tstrcpy(ws.srcpath, cinfo->path);
	ws.initcmd = NULL;
	ws.readlistfile = TRUE;
	ws.wlfc_flags = 0;
	ws.listfilename = NULL;
	ws.vfs = mode;
	return WhereIsDialogMain(&ws);
}

DWORD GetOnOffOption(const TCHAR *more, DWORD defvalue)
{
	if ( SkipSpace(&more) < ' ' ) return defvalue;
	return GetStringCommand(&more, T("OFF\0") T("ON\0"));
}

// *where / *whereis
ERRORCODE WhereIsCommand(PPC_APPINFO *cinfo, const TCHAR *param, BOOL usedialog)
{
	WHERESTRUCT ws;
	TCHAR listfile[VFPS];
	TCHAR paramtmp[VFPS], code, *more;

	if ( IsTrue(usedialog) ){
		if ( *param == '!' ){ // 即時実行
			param++;
			usedialog = FALSE;
		}
	}
	ThInit(&ws.thEcdata);
	tstrcpy(ws.srcpath, cinfo->path);
	ws.initcmd = NULL;
	ws.listfilename = NULL;
	ws.readlistfile = TRUE;
	ws.wlfc_flags = 0;
	ws.vfs = FALSE;

	ws.st.marked = FALSE;
	ws.st.dir = FALSE;
	ws.st.sdir = FILE_ATTRIBUTE_DIRECTORY;
	ws.st.exmode = WDEXM_PLAINTEXT;
	ws.st.mask1[0] = '\0';
	ws.st.mask2[0] = '\0';
	ws.st.text[0] = '\0';
	listfile[0] = '\0';

	code = SkipSpace(&param);
	if ( (code != '-') && (code != '/') ){
		GetCommandParameterDual(&param, ws.srcpath, TSIZEOF(ws.srcpath));
		if ( ws.srcpath[0] == '\0' ) tstrcpy(ws.srcpath, cinfo->path);
	}
	if ( SkipSpace(&param) != ',' ){ // 空白区切り形式
		for (;;){
			code = GetOptionParameter(&param, paramtmp, &more);
			if ( code == '\0' ) break;
			if ( code != '-' ) break;
			if ( (tstrcmp(paramtmp + 1, T("MASK")) == 0) ||
				 (tstrcmp(paramtmp + 1, T("MASK1")) == 0) ){
				tstplimcpy(ws.st.mask1, more, TSIZEOF(ws.st.mask1));
				continue;
			}
			if ( (tstrcmp(paramtmp + 1, T("MASK2")) == 0) ){
				tstplimcpy(ws.st.mask2, more, TSIZEOF(ws.st.mask2));
				continue;
			}
			if ( (tstrcmp(paramtmp + 1, T("START")) == 0) ){
				usedialog = FALSE;
				continue;
			}
			if ( (tstrcmp(paramtmp + 1, T("PATH")) == 0) ){
				tstplimcpy(ws.srcpath, more, TSIZEOF(ws.srcpath));
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("TEXT")) == 0 ){
				tstplimcpy(ws.st.text, more, TSIZEOF(ws.st.text));
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("DIR")) == 0 ){
				ws.st.dir = GetOnOffOption(more, TRUE);
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("SEARCH")) == 0 ){
				ws.readlistfile = FALSE;
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("SUBDIR")) == 0 ){
				ws.st.sdir = GetOnOffOption(more, TRUE);
				if ( ws.st.sdir != 0 ) ws.st.sdir = FILE_ATTRIBUTE_DIRECTORY;
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("VFS")) == 0 ){
				ws.vfs = GetOnOffOption(more, TRUE);
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("TYPE")) == 0 ){
				ws.st.exmode = GetOnOffOption(more, TRUE);
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("MARKED")) == 0 ){
				ws.st.marked = GetOnOffOption(more, TRUE);
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("NAME")) == 0 ){
				ws.readlistfile = FALSE;
				ws.wlfc_flags |= WLFC_NAMEONLY | WLFC_NOHEADER;
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("LISTFILE")) == 0 ){
				tstplimcpy(listfile, more, TSIZEOF(listfile));
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("UTF8")) == 0 ){
				ws.wlfc_flags |= WLFC_UTF8;
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("UTF8BOM")) == 0 ){
				ws.wlfc_flags |= WLFC_UTF8 | WLFC_BOM;
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("DIRSHOW")) == 0 ){
				ws.wlfc_flags |= WLFC_DIRSHOW;
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("JSON")) == 0 ){
				ws.wlfc_flags |= WLFC_JSON | WLFC_UTF8;
				if ( *more != '\0' ){
					tstplimcpy(listfile, more, TSIZEOF(listfile));
				}
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("APPEND")) == 0 ){
				ws.wlfc_flags |= WLFC_APPEND;
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("K")) == 0 ){
				if ( *more == '\0' ){
					more = (TCHAR *)param;
					param = NilStr;
				}
				if ( ws.initcmd != NULL ) PPcHeapFree(ws.initcmd);
				ws.initcmd = PPcHeapAlloc(TSTRSIZE(more));
				tstrcpy(ws.initcmd, more);
				continue;
			}
			FreePws(&ws);
			XMessage(NULL, NULL, XM_GrERRld, StrBadOption, paramtmp);
			return ERROR_INVALID_PARAMETER;
		}
	}else{ // , 区切り旧形式
		param++; // "," をスキップ
		GetCommandParameter(&param, ws.st.mask1, TSIZEOF(ws.st.mask1));
		NextParameter(&param);
		GetCommandParameter(&param, ws.st.mask2, TSIZEOF(ws.st.mask2));
		NextParameter(&param);
		GetCommandParameter(&param, ws.st.text, TSIZEOF(ws.st.text));
		NextParameter(&param);
		ws.st.dir = GetNumber(&param);
		NextParameter(&param);
		if ( Isdigit(*param) ){
			ws.st.sdir = GetNumber(&param) ? FILE_ATTRIBUTE_DIRECTORY : 0;
		}else{
			ws.st.sdir = FILE_ATTRIBUTE_DIRECTORY;
 		}
		NextParameter(&param);
		ws.vfs = GetNumber(&param);
		NextParameter(&param);
		ws.st.exmode = GetNumber(&param);
		if ( (ws.st.exmode <= WDEXM_UNKNOWN) ||
			 (ws.st.exmode == WDEXM_COLUMNMIN) ||
			 (ws.st.exmode > WDEXM_MODULE) ){
			ws.st.exmode = WDEXM_PLAINTEXT;
		}
		NextParameter(&param);
		ws.st.marked = GetNumber(&param);
		NextParameter(&param);
		GetCommandParameter(&param, listfile, TSIZEOF(listfile));
	}

	if ( listfile[0] != '\0' ){
		VFSFixPath(NULL, listfile, cinfo->path, VFSFIX_VFPS | VFSFIX_NOFIXEDGE);
		ws.listfilename = listfile;
	}

	ws.st.cinfo = cinfo;

	if ( IsTrue(usedialog) ){
		return WhereIsDialogMain(&ws);
	}else{
		if ( ws.initcmd != NULL ) PPcHeapFree(ws.initcmd);
		return WhereIsMain(cinfo, &ws);
	}
}
