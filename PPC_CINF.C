/*-----------------------------------------------------------------------------
	Paper Plane cUI								PPC_APPINFO.info.Function
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPCOMBO.H"

#include "PPC_DD.H"
#pragma hdrstop

const TCHAR StrTemp[] = T("temp");
const TCHAR StrThisPath[] = T("thispath");
const TCHAR StrThisBranch[] = T("thisbranch");
const TCHAR StrRegID[] = T("id");

const TCHAR *StrDSMDlist[] = {
	NilStr,			// DSMD_NOMODE
	StrTemp,		// DSMD_TEMP
	StrThisPath,	// DSMD_THIS_PATH
	StrThisBranch,	// DSMD_THIS_BRANCH
	StrRegID,		// DSMD_REGID
	StrListfileMode, // DSMD_LISTFILE
	NilStr,			// DSMD_PATH_BRANCH
	StrArchiveMode	// DSMD_ARCHIVE
};

ERRORCODE PPcDirOptionCommand(PPC_APPINFO *cinfo, const TCHAR *param);

typedef struct {
	HWND hChildWnd;
	int id;
} CCIPSTRUCT;

// ウィンドウを閉じた後も生きているときに使用する
DWORD_PTR USECDECL PPcClosedIInfo(PPC_APPINFO *cinfo, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	switch( cmdID ){
		case PPXCMDID_STARTENUM:	// 検索開始(マーク無しもあり)
		case PPXCMDID_STARTNOENUM:	// 検索開始(マーク無しはなし)
			PPcAppInfo_AddRef(cinfo);
			uptr->enums.enumID = CELLENUMID_END;
			break;

		case PPXCMDID_ENDENUM:	// 列挙終了
			PPcAppInfo_Release(cinfo);
			break;

		default:
			if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
			return PPXA_INVALID_FUNCTION;
	}
	return PPXA_NO_ERROR;
}

void GetPairVpath(PPC_APPINFO *cinfo, TCHAR *destbuf)
{
	const TCHAR *result;
	HWND hTargetWnd;

	destbuf[0] = '\0';
	hTargetWnd = GetPairWnd(cinfo);
	if ( hTargetWnd == NULL ) return;
	result = SendExtractCall(hTargetWnd, T("%3"));
	if ( result != NULL ){
		tstplimcpy(destbuf, result, VFPS);
		ProcHeapFree((void *)result);
	}
}

int GetInfoShowIndex(PPC_APPINFO *cinfo, int index)
{
	HWND hCWnd;

	if ( (cinfo->combo == 0) || (index >= Combo.ShowCount) ) return -1;
	if ( index >= 0 ) return index;
	if ( index == -1 ){
		hCWnd = cinfo->info.hWnd;
	}else if ( index == -2 ){
		hCWnd = hComboFocusWnd;
	}else if ( index == -3 ){
		hCWnd = (HWND)SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
				KCW_getpairwnd, (LPARAM)hComboFocusWnd);
	}else{
		hCWnd = NULL;
	}
	return GetComboShowIndex(hCWnd);
}

int GetInfoTabBaseIndex(PPC_APPINFO *cinfo, int *index)
{
	int showindex, tabindex;
	HWND hCWnd = BADHWND;
	TC_ITEM tie;

	if ( cinfo->combo == 0 ) return -1;
	if ( index[1] < 0 ){ // tab デフォルト値
		if ( index[0] == -1 ){
			hCWnd = cinfo->info.hWnd;
		}else if ( index[0] == -2 ){ // フォーカス窓
			hCWnd = hComboFocusWnd;
		}else if ( index[0] == -3 ){ // 反対窓
			hCWnd = (HWND)SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
					KCW_getpairwnd, (LPARAM)hComboFocusWnd);
		}
	}
	if ( hCWnd == BADHWND ){
		showindex = GetInfoShowIndex(cinfo, index[0]);
		if ( showindex < 0 ) return -1;

		tabindex = index[1];
		if ( (Combo.Tabs == 0) ||
			 ( (tabindex < 0) && !(X_combos[0] & CMBS_TABSEPARATE) ) ){
			return Combo.show[showindex].baseNo;
		}

		if ( tabindex < 0 ){
			tabindex = TabCtrl_GetCurSel(Combo.show[showindex].tab.hWnd);
			if ( tabindex < 0 ) tabindex = 0;
		}

		tie.mask = TCIF_PARAM;
		if ( TabCtrl_GetItem(Combo.show[showindex].tab.hWnd, tabindex, &tie) == FALSE ){
			return -1;
		}
		hCWnd = (HWND)tie.lParam;
	}
	return GetComboBaseIndex(hCWnd);
}

PPC_APPINFO *GetInfoTabInfo(PPC_APPINFO *cinfo, int *index)
{
	int baseindex;

	if ( (index[0] == -1) && (index[1] == -1) ) return cinfo;
	baseindex = GetInfoTabBaseIndex(cinfo, index);
	if ( baseindex < 0 ) return NULL;
	return Combo.base[baseindex].cinfo;
}

DWORD_PTR ComboGetTabInfo(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	HWND hTargetWnd;
	int showindex, tabindex;

	hTargetWnd = GetPPxhWndFromID(&cinfo->info, (const TCHAR **)&uptr->tabidxstr.str, NULL);
	if ( (hTargetWnd == NULL) || (hTargetWnd == BADHWND) ){
		uptr->tabidxstr.pane = -1;
		uptr->tabidxstr.tab = -1;
		return ERROR_INVALID_PARAMETER;
	}
	showindex = GetComboShowIndex(hTargetWnd);
	if ( showindex >= 0 ){ // 表示中
		tabindex = GetTabItemIndex(hTargetWnd, showindex);
	}else if ( Combo.Tabs == 0 ){ // タブ無し
		showindex = 0;
		tabindex = 0;
	}else if ( !(X_combos[0] & CMBS_TABEACHITEM) ){ // 独立していない
		int si;
		tabindex = -1;

		for ( si = 0 ; si < Combo.ShowCount ; si++ ){
			if ( Combo.base[Combo.show[si].baseNo].hWnd == hTargetWnd ){
				showindex = si;
				tabindex = GetTabItemIndex(hTargetWnd, si);
				break;
			}
		}
		if ( tabindex < 0 ){
			showindex = 0;
			tabindex = GetTabItemIndex(hTargetWnd, 0);
		}
	}else{ // 独立
		int si;

		tabindex = -1;
		for ( si = 0 ; si < Combo.ShowCount ; si++ ){
			tabindex = GetTabItemIndex(hTargetWnd, si);
			if ( tabindex >= 0 ){
				showindex = si;
				break;
			}
		}
	}
	uptr->tabidxstr.pane = showindex;
	uptr->tabidxstr.tab = tabindex;
	return PPXA_NO_ERROR;
}

void ComboTabExecute(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	HWND hTargetWnd;
	COPYDATASTRUCT copydata;
	PPC_APPINFO *ainfo = GetInfoTabInfo(cinfo, uptr->inums);

	if ( ainfo == NULL ) return;
	hTargetWnd = ainfo->info.hWnd;

	if ( hTargetWnd == cinfo->info.hWnd ){ // 自前実行
		PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL,
				uptr->tabidxstr.str, NULL, 0);
	}else{ // 指定PPxで実行
		copydata.dwData = 'H';
		copydata.cbData = TSTRSIZE32(uptr->tabidxstr.str);
		copydata.lpData = (PVOID)uptr->tabidxstr.str;
		SendMessage(hTargetWnd, WM_COPYDATA, 0, (LPARAM)&copydata);
	}
}

// PPXSCR.DLL (R18以前)用
void ComboTabExtract(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	HWND hTargetWnd;
	PPC_APPINFO *ainfo = GetInfoTabInfo(cinfo, uptr->inums);

	if ( ainfo == NULL ) return;
	hTargetWnd = ainfo->info.hWnd;

	if ( hTargetWnd == cinfo->info.hWnd ){ // 自前実行
		PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL,
				uptr->tabidxstr.str,
				uptr->tabidxstr.str, XEO_EXTRACTEXEC);
	}else{ // 指定PPxで実行
		const TCHAR *result;

		result = SendExtractCall(hTargetWnd, uptr->tabidxstr.str);
		if ( result != NULL ){
			tstplimcpy(uptr->tabidxstr.str, result, CMDLINESIZE);
			ProcHeapFree((void *)result);
		}else{
			*uptr->tabidxstr.str = '\0';
		}
	}
}

/*
	in  nums[0]:pane no  nums[1]:tab no
	out nums[0]:tab no   nums[1]:pane no
*/
void ComboTabIndexInfo(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	int showindex;

	if ( Combo.Tabs == 0 ){
		uptr->nums[0] = (DWORD)-1;
		uptr->nums[1] = (DWORD)-1;
		return;
	}
	if ( (uptr->inums[0] < 0) && (uptr->inums[1] == -1) ){ // デフォルト値
		HWND hCWnd;

		if ( uptr->inums[0] == -1 ){
			hCWnd = cinfo->info.hWnd;
		}else if ( uptr->inums[0] == -2 ){ // フォーカス窓
			hCWnd = hComboFocusWnd;
		}else { // 反対窓
			hCWnd = (HWND)SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
					KCW_getpairwnd, (LPARAM)hComboFocusWnd);
		}
		showindex = GetComboShowIndex(hCWnd);
		if ( showindex < 0 ){
			int si;

			uptr->inums[0] = -1;
			uptr->inums[1] = -1;
			if ( Combo.Tabs == 0 ) return;
			for ( si = 0 ; si < Combo.ShowCount ; si++ ){
				int tabindex = GetTabItemIndex(hCWnd, si);
				if ( tabindex >= 0 ){
					uptr->nums[0] = tabindex;
					uptr->nums[1] = si;
					return;
				}
			}
			return;
		}
	}else{
		showindex = GetInfoShowIndex(cinfo, uptr->nums[0]);
	}
	if ( showindex >= Combo.ShowCount ){
		uptr->nums[0] = (DWORD)-1;
		return;
	}

	if ( showindex < 0 ){
		if ( !(X_combos[0] & CMBS_TABEACHITEM) ){
			showindex = 0;
		}else{
			uptr->nums[0] = (DWORD)-1;
			return;
		}
	}

	{
		int tabindex = uptr->nums[1];

		if ( tabindex < 0 ){
			tabindex = TabCtrl_GetCurSel(Combo.show[showindex].tab.hWnd);
		}else{
			if ( tabindex >= TabCtrl_GetItemCount(Combo.show[showindex].tab.hWnd) ){
				tabindex = -1;
			}
		}
		uptr->nums[0] = tabindex;
	}
	uptr->nums[1] = showindex;
}

void ComboTabPaneInfo(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	int baseindex = GetInfoTabBaseIndex(cinfo, uptr->inums);
	int showindex;
	int tabshow;

	if ( baseindex < 0 ){ // 存在しない
		uptr->nums[0] = (DWORD)-1;
		return;
	}
	showindex = GetComboShowIndex(Combo.base[baseindex].hWnd);
	if ( showindex >= 0 ){ // 表示中
		uptr->nums[0] = showindex;
		return;
	}
	if ( !(X_combos[0] & CMBS_TABEACHITEM) ){ // 独立していない & 非表示
		uptr->nums[0] = (DWORD)-2;
		return;
	}
	tabshow = -1;
	for ( showindex = 0 ; showindex < Combo.ShowCount ; showindex++ ){

		if ( GetTabItemIndex(Combo.base[baseindex].hWnd, showindex) >= 0){
			tabshow = showindex;
			break;
		}
	}
	uptr->nums[0] = tabshow;
}

void ComboTabGroup_SetName(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	int showindex = GetInfoShowIndex(cinfo, uptr->tabidxstr.pane);
	COMBOTABINFO *tabs;
	int index;
	HWND hTabWnd;

	if ( showindex < 0 ) return;

	tabs = &Combo.show[showindex].tab;
	index = uptr->tabidxstr.tab;
	if ( tabs->hSelecterWnd != NULL ){
		TC_ITEM tie;

		if ( index < 0 ) index = TabCtrl_GetCurSel(tabs->hSelecterWnd);
		tie.mask = TCIF_PARAM;
		if ( TabCtrl_GetItem(tabs->hSelecterWnd, index, &tie) == FALSE ){
			return;
		}
		hTabWnd = (HWND)tie.lParam;
	}else{
		if ( index < 0 ){
			int i;

			for ( i = 0 ; i < tabs->groupcount; i++ ){
				if ( tabs->hWnd == tabs->group[i].hWnd ){
					index = i;
					break;
				}
			}
		}
		if ( (index < 0) || (index >= tabs->groupcount) ) return;
		hTabWnd = tabs->group[index].hWnd;
	}
	RenameGroupNameMain(tabs, hTabWnd, uptr->tabidxstr.str);
}

void ComboTabGroup_SetIndex(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	int showindex = GetInfoShowIndex(cinfo, uptr->nums[0]);

	if ( showindex < 0 ){
		uptr->nums[1] = -1;
		return;
	}
	SelectGroup(showindex, uptr->nums[1]);
}

void ComboTabGroup_GetName(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	int showindex = GetInfoShowIndex(cinfo, uptr->tabidxstr.pane);
	COMBOTABINFO *tabs;
	int index;
	HWND hTabWnd;

	uptr->tabidxstr.str[0] = '\0';
	if ( showindex < 0 ) return;

	tabs = &Combo.show[showindex].tab;
	index = uptr->tabidxstr.tab;
	if ( tabs->hSelecterWnd != NULL ){
		TC_ITEM tie;

		if ( index < 0 ) index = TabCtrl_GetCurSel(tabs->hSelecterWnd);
		tie.mask = TCIF_PARAM;
		if ( TabCtrl_GetItem(tabs->hSelecterWnd, index, &tie) == FALSE ){
			return;
		}
		hTabWnd = (HWND)tie.lParam;
	}else{
		if ( index < 0 ){
			int i;

			for ( i = 0 ; i < tabs->groupcount; i++ ){
				if ( tabs->hWnd == tabs->group[i].hWnd ){
					index = i;
					break;
				}
			}
		}
		if ( (index < 0) || (index >= tabs->groupcount) ) return;
		hTabWnd = tabs->group[index].hWnd;
	}
	GetWindowText(hTabWnd, uptr->tabidxstr.str, VFPS);
}

void ComboTabGroup_GetIndex(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	int showindex = GetInfoShowIndex(cinfo, uptr->nums[0]);
	COMBOTABINFO *tabs;

	if ( showindex < 0 ){
		uptr->nums[1] = -1;
		return;
	}
	tabs = &Combo.show[showindex].tab;
	if ( tabs->hSelecterWnd != NULL ){
		uptr->nums[1] = TabCtrl_GetCurSel(tabs->hSelecterWnd);
	}else{
		int i;

		for ( i = 0 ; i < tabs->groupcount; i++ ){
			if ( tabs->hWnd == tabs->group[i].hWnd ){
				uptr->nums[1] = i;
				return;
			}
		}
		uptr->nums[1] = -1;
	}
}

void ComboTabGroup_GetCount(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	int showindex = GetInfoShowIndex(cinfo, uptr->nums[0]);

	if ( showindex < 0 ){
		uptr->nums[1] = 0;
		return;
	}
	uptr->nums[1] = Combo.show[showindex].tab.groupcount;
}

void SetStatline(PPC_APPINFO *cinfo, DELETESTATUS *dinfo)
{
	TCHAR buf[VFPS + 16];

	thprintf(buf, TSIZEOF(buf), T("%d %s"), dinfo->count, dinfo->path);
	SetPopMsg(cinfo, POPMSG_PROGRESSBUSYMSG, buf);
	UpdateWindow_Part(cinfo->info.hWnd);
}

// %*dirhistory
void USEFASTCALL DirHistFunction(PPC_APPINFO *cinfo, PPXMDLFUNCSTRUCT *fparam)
{
	const TCHAR *param;
	DWORD top;
	int no;

	fparam->dest[0] = '\0';
	param = fparam->optparam;
	no = GetNumber(&param);

	if ( no == 0 ){
		tstrcpy(fparam->dest, cinfo->path);
		return;
	}

	if ( cinfo->PathTrackingList.top ){
		if ( no > 0 ){ // 戻る
			top = BackPathTrackingList(cinfo, cinfo->PathTrackingList.top);
			while ( top != 0 ){
				top = BackPathTrackingList(cinfo, top);
				if ( no <= 1 ){
					tstrcpy(fparam->dest, (TCHAR *)
							(cinfo->PathTrackingList.bottom + top));
					break;
				}
				no--;
			}
		}else{ // 進む
			top = cinfo->PathTrackingList.top;
			while ( top != 0 ){
				TCHAR *p;

				p = (TCHAR *)(cinfo->PathTrackingList.bottom + top);
				if ( *p == '\0' ) break;
				if ( no >= -1 ){
					tstrcpy(fparam->dest, (TCHAR *)
							(cinfo->PathTrackingList.bottom + top));
					break;
				}
				top += TSTRSIZE32(p);
				no++;
			}
		}
	}
}

void USEFASTCALL EntryExtData_GetID(DWORD *id)
{
	*id = ColumnExtUserID;
	// 割り振りIDがDFC_USERMINに到達したときは、バグと思われるので固定値にする
	if ( ColumnExtUserID > DFC_USERMIN ) ColumnExtUserID--;
}

void EntryExtData_SetString(PPC_APPINFO *cinfo, DWORD CommentID, ENTRYCELL *cell, const TCHAR *comment)
{
	ENTRYEXTDATASTRUCT eeds;

	eeds.id = CommentID;
	eeds.size = TSTRSIZE32(comment);
	eeds.data = (BYTE *)comment;
	EntryExtData_SetDATA(cinfo, &eeds, cell);
}

void EntryExtData_SetDATA(PPC_APPINFO *cinfo, ENTRYEXTDATASTRUCT *eeds, ENTRYCELL *cell)
{
	CELLEXTRASTRUCT cds, *cdsptr;

	EnterCellEdit(cinfo);
	if ( cell->cellcolumn < 0 ){ // 未生成
		cell->cellcolumn = cinfo->e.CellExtra.top;
		cdsptr = NULL;
	}else{
		cdsptr = CellExtraFirst(cinfo, cell);
		for ( ; ; ){
			if ( cdsptr->itemindex == (WORD)eeds->id ) break; // 発見
			if ( cdsptr->nextoffset == 0 ){ // 存在しない→新規
				cdsptr->nextoffset = cinfo->e.CellExtra.top;
				cdsptr = NULL;
				break;
			}
			cdsptr = CellExtraNext(cinfo, cdsptr);
		}
	}
	if ( cdsptr == NULL ){
		cds.nextoffset = 0;
		cds.itemindex = (WORD)eeds->id;
		ThAppend(&cinfo->e.CellExtra, &cds, sizeof(CELLEXTRASTRUCT));
		cdsptr = (CELLEXTRASTRUCT *)(BYTE *)(cinfo->e.CellExtra.bottom + cinfo->e.CellExtra.top - sizeof(CELLEXTRASTRUCT));
	}
	if ( eeds->size >= 0x10000 ) eeds->size = 0xffff;

	cdsptr->textoffset = cinfo->e.CellExtra.top;
	cdsptr->size = (WORD)eeds->size;
	// dataを保存する
	ThAppend(&cinfo->e.CellExtra, eeds->data, eeds->size);
	LeaveCellEdit(cinfo);
}

BOOL EntryExtData_GetDATA(PPC_APPINFO *cinfo, ENTRYEXTDATASTRUCT *eeds, ENTRYCELL *cell)
{
	CELLEXTRASTRUCT *cdsptr;
	BOOL result;

	if ( cell->cellcolumn < 0 ){ // 未生成
		return FALSE;
	}
	EnterCellEdit(cinfo);
	cdsptr = CellExtraFirst(cinfo, cell);
	for ( ; ; ){
		if ( cdsptr->itemindex == (WORD)eeds->id ){ // 発見
			BYTE *src;
			DWORD size;

			src = GetCellExtraData(cinfo, cdsptr);
			size = cdsptr->size;
			if ( size <= eeds->size ){
				memcpy(eeds->data, src, size);
				result = TRUE;
			}else{
				result = FALSE;
			}
			break;
		}
		if ( cdsptr->nextoffset == 0 ){ // 存在しない
			result = FALSE;
			break;
		}
		cdsptr = CellExtraNext(cinfo, cdsptr);
	}
	LeaveCellEdit(cinfo);
	return result;
}

BYTE *EntryExtData_GetDATAptr(PPC_APPINFO *cinfo, WORD id, ENTRYCELL *cell)
{
	CELLEXTRASTRUCT *cdsptr;
	BYTE *result;

	if ( cell->cellcolumn < 0 ){ // 未生成
		return NULL;
	}
	EnterCellEdit(cinfo);
	cdsptr = CellExtraFirst(cinfo, cell);
	for ( ; ; ){
		if ( cdsptr->itemindex == id ){ // 発見
			result = GetCellExtraData(cinfo, cdsptr);
			break;
		}
		if ( cdsptr->nextoffset == 0 ){ // 存在しない
			result = NULL;
			break;
		}
		cdsptr = CellExtraNext(cinfo, cdsptr);
	}
	LeaveCellEdit(cinfo);
	return result;
}

void USEFASTCALL CommentFunction(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	DWORD CommentOffset;
	int CommentID;
	const TCHAR *param;

	param = uptr->funcparam.optparam;
	CommentID = GetNumber(&param);
	if ( CommentID <= 0 ){
		CommentOffset = CEL(cinfo->e.cellN).comment;
		if ( CommentOffset != EC_NOCOMMENT ){
			tstplimcpy(uptr->funcparam.dest, ThPointerT(&cinfo->e.Comments, CommentOffset), CMDLINESIZE);
		}else{
			uptr->funcparam.dest[0] = '\0';
		}
	}else{
		ENTRYEXTDATASTRUCT eeds;

		uptr->funcparam.dest[0] = '\0';
		eeds.id = (DWORD)(DFC_COMMENTEX_MAX - (CommentID - 1) );
		eeds.size = CMDLINESIZE * sizeof(TCHAR);
		eeds.data = (BYTE *)uptr->funcparam.dest;
		EntryExtData_GetDATA(cinfo, &eeds, &CEL(cinfo->e.cellN));
	}
}

void USEFASTCALL SetExtParam(const TCHAR *src, TCHAR *dest, size_t size)
{
	const TCHAR *srcptr;
	TCHAR *destptr, *maxptr, *spaceptr;

	srcptr = src;
	if ( SkipSpace(&srcptr) == '\"' ){ // セパレータ有り
		GetCommandParameter(&src, dest, size);
		return;
	}
	// セパレータ無し。先頭末尾の空白を除去する
	maxptr = dest + size - 1;
	spaceptr = NULL;
	destptr = dest;
	while ( destptr < maxptr ){
		TCHAR chr;

		chr = *srcptr;
		if ( chr == '\0' ) break;
		if ( (chr == ' ') || (chr == '\t') ){
			if ( spaceptr == NULL ) spaceptr = destptr;
		}else{
			spaceptr = NULL;
		}
		*destptr++ = chr;
		srcptr++;
	}
	if ( spaceptr != NULL ) destptr = spaceptr;
	*destptr = '\0';
}

// *jumppath
void USEFASTCALL PPcJumpPathCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	TCHAR buf[CMDLINESIZE], path[VFPS], code, *more;
	DWORD flags = RENTRY_READ;

	path[0] = '\0';
	for ( ;; ){
		code = GetOptionParameter(&param, buf, &more);
		if ( code == '\0' ) break;
		if ( code != '-' ){
			if ( VFSFullPath(path, buf, cinfo->path) == NULL ) return;
		}else{
			if ( tstrcmp(buf + 1, T("UPDATE")) == 0 ){
				setflag(flags, RENTRY_UPDATE);
			}else
			if ( tstrcmp(buf + 1, T("SAVELOCATE")) == 0 ){
				setflag(flags, RENTRY_SAVEOFF);
			}else
			if ( tstrcmp(buf + 1, T("ENTRY")) == 0 ){
				setflag(flags, RENTRY_JUMPNAME);
				if ( *more == '\0' ){
					cinfo->Jfname[0] = '\0';
				}else{
					tstrcpy(cinfo->Jfname, more);
				}
			}else
			if ( tstrcmp(buf + 1, T("NOASYNC")) == 0 ){
				setflag(flags, RENTRY_NOASYNC);
			}else
			if ( tstrcmp(buf + 1, T("NOLOCK")) == 0 ){
				setflag(flags, RENTRY_NOLOCK);
			}else
			if ( tstrcmp(buf + 1, T("NOFIXPATH")) == 0 ){
				setflag(flags, RENTRY_NOFIXDIR);
			}else
			if ( tstrcmp(buf + 1, T("USECACHE")) == 0 ){
				setflag(flags, RENTRY_USECACHE);
			}else
			if ( tstrcmp(buf + 1, T("REFRESHCACHE")) == 0 ){
				setflag(flags, RENTRY_CACHEREFRESH);
			}else{
				XMessage(cinfo->info.hWnd, T("*jumppath"), XM_GrERRld, StrBadOption, buf);
			}
		}
		NextParameter(&param);
	}
	if ( path[0] == '\0' ) tstrcpy(path, cinfo->path);
	if ( (flags & RENTRY_JUMPNAME) && (cinfo->Jfname[0] == '\0') ){
		JumpPathEntry(cinfo, path, flags);
		return;
	}
	PPcChangeDirectory(cinfo, path, flags);
}

void USEFASTCALL PPcJumpDirMacro(PPC_APPINFO *cinfo, const TCHAR *param)
{
	TCHAR path[VFPS];

	if ( VFSFullPath(path, (TCHAR *)param, cinfo->path) == NULL ) return;
	PPcChangeDirectory(cinfo, path, RENTRY_READ);
}

void USEFASTCALL PPcMoveCsr(PPC_APPINFO *cinfo, int *param)
{
	if ( param[CURSOR_PARAM_ITEMS] >= (CURSOR_PARAM_ITEMS - 1) ){
		MoveCellCursor(cinfo, (CURSORMOVER *)param);
	}else{
		CURSORMOVER cm;

		cm = *cinfo->list.XC_mvFB;
		memcpy(&cm, param, param[CURSOR_PARAM_ITEMS] * sizeof(int));
		MoveCellCursor(cinfo, &cm);
	}
}

#define ZOOMMODE_DEFAULT	0
#define ZOOMMODE_FONT		1
#define ZOOMMODE_IMAGE		2
#define ZOOMMODE_IMAGE_H	3
#define ZOOMMODE_AUTO		4
void USEFASTCALL PPcZoom(PPC_APPINFO *cinfo, const TCHAR *param)
{
	int mode = ZOOMMODE_DEFAULT;
	BOOL sign;
	int num;
	int fmtoffset;

	fmtoffset = FindCellFormatImagePosition(cinfo->celF.fmt);

	switch ( SkipSpace(&param) ){
		case 'a': // auto
			mode = ZOOMMODE_AUTO;
			break;
		case 'i': // image
			mode = ZOOMMODE_FONT;
			break;
		case 'f': // font
			mode = ZOOMMODE_IMAGE;
			break;
	}
	if ( mode != ZOOMMODE_DEFAULT ){
		param = tstrchr(param, ' ');
		if ( param == NULL ) return;
		SkipSpace(&param);
	}

	sign = (*param == '+') || (*param == '-');
	num = GetIntNumber(&param);

	if ( (mode == ZOOMMODE_DEFAULT) || (mode == ZOOMMODE_AUTO) ){
		if ( fmtoffset < 0 ){
			mode = ZOOMMODE_FONT;
		}else{
			mode = ZOOMMODE_IMAGE;
			num /= 10;
		}
	}

	if ( mode == ZOOMMODE_FONT ){ // font
		if ( sign ) num += cinfo->X_textmag;
		if ( num <= 0 ) num = 10;
		if ( num > 9990 ) num = 9990;
		if ( num == cinfo->X_textmag ) return;
		cinfo->X_textmag = num;
		DeleteObject(cinfo->hBoxFont);
		InitFont(cinfo);
	}
	if ( mode == ZOOMMODE_IMAGE ){ // image
		int height;
		BYTE *fmt;

		if ( 0 > (fmtoffset = FindCellFormatImagePosition(cinfo->celF.fmt)) ){
			return;
		}

		if ( SkipSpace(&param) == ',' ){
			mode = ZOOMMODE_IMAGE_H;
			param++;
			height = GetIntNumber(&param);
		}

		fmt = cinfo->celF.fmt + fmtoffset + 1; // +0:X +1:Y

		if ( sign ){
			if ( mode == ZOOMMODE_IMAGE ){
				height = fmt[1] + num;
				num = fmt[0] + (num * 2);
			}else{ // ZOOMMODE_IMAGE_H
				height = fmt[1] + height;
				num = fmt[0] + num;
			}
		}else{
			if ( mode == ZOOMMODE_IMAGE ) height = num / 2;
		}
		if ( num <= 0 ) num = 1;
		if ( num > 255 ) num = 255;
		if ( height <= 0 ) height = 1;
		if ( height > 255 ) height = 255;

		cinfo->celF.width += num - (int)fmt[0];
		cinfo->celF.height += height - (int)fmt[1];
		fmt[0] = (BYTE)num;
		fmt[1] = (BYTE)height;
	}
	ClearCellIconImage(cinfo);
	WmWindowPosChanged(cinfo);
	InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
}

void USEFASTCALL PPcTreeCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	TCHAR cmdstr[VFPS];

	if ( cinfo->combo && (X_combos[0] & CMBS_COMMONTREE) ){ // combo へ送信
		SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND, TMAKELPARAM(KCW_tree, PPCTREECOMMAND), (LPARAM)param);
		return;
	}
	if ( cinfo->hTreeWnd == NULL ){
		const TCHAR *pp;

		pp = param;
		GetLineParamS(&param, cmdstr, TSIZEOF(cmdstr));
		param = pp;
		if ( !tstrcmp(cmdstr, T("off")) ) return; // close 済み
		if ( !tstrcmp(cmdstr, T("once")) ){
			PPC_Tree(cinfo, PPCTREE_SELECT, FALSE);
			if ( cinfo->hTreeWnd != NULL ) SetFocus(cinfo->hTreeWnd);
			return;
		}
		PPC_Tree(cinfo, PPCTREE_SYNC, TRUE);
		if ( cmdstr[0] == '\0' ) return;
	}
	if ( cinfo->hTreeWnd != NULL ){
		SendMessage(cinfo->hTreeWnd, VTM_TREECOMMAND, 0, (LPARAM)param); // 整形前を渡す
	}
}

DWORD_PTR USEFASTCALL PPcExecExMenu(PPC_APPINFO *cinfo, EXECEXMENUINFO *execmenu)
{
	TCHAR buf[CMDLINESIZE];
	int index;

	index = execmenu->index;
	buf[0] = '\0';
	GetMenuString(execmenu->hMenu, index, buf, TSIZEOF(buf), MF_BYCOMMAND);
	if ( buf[0] == '\0' ){
		GetMenuString(GetSystemMenu(
				  (cinfo->combo ? cinfo->hComboWnd : cinfo->info.hWnd) , FALSE),
				index, buf, TSIZEOF(buf), MF_BYCOMMAND);
		if ( buf[0] == '\0' ) return 0;
	}
	if ( (index >= CRID_SORT) && (index <= CRID_SORT_MAX) ){
		SortKeyCommand(cinfo, index, NULL);
	}else if ( (index >= CRID_VIEWFORMAT) && (index <= CRID_VIEWFORMAT_MAX) ){
		SetCellDisplayFormat(cinfo, index, NULL);
	}else if ( (index >= CRID_NEWENTRY) && (index <= CRID_NEWENTRY_MAX) ){
		MakeEntryMain(cinfo, index, buf);
	}else if ( (index >= CRID_DRIVELIST) && (index <= CRID_DRIVELIST_MAX) ){
		PPC_DriveJumpMain(cinfo, buf);
	}else if ( (index >= CRID_DIRTYPE) && (index <= CRID_DIRTYPE_MAX) ){
		DoActionMenu(cinfo, execmenu->hMenu, index, cinfo->e.cellN, NULL, NULL, 0);
	}else if ( (index >= CRID_DIROPT) && (index <= CRID_DIROPT_ARCHIVE) ){
		thprintf(buf, TSIZEOF(buf), T("-%s"), StrDSMDlist[index - CRID_DIROPT]);
		PPcDirOptionCommand(cinfo, buf);
	}
	return PPXCMDID_EXECEXMENU;
}

// ワイルドカード用の処理を行う
void PathWildItemName(TCHAR *dest, const TCHAR *src)
{
	const TCHAR *sp;

	if ( !((src[0] == '*') && (src[1] == '\0')) ){
		sp = src;
		if ( *sp != ':' ){ // 強制パス指定(/区切り用)以外ならワイルドカードチェック
			if ( tstrchr(sp, '/') != NULL ){ // 正規表現か ftp:/ 系か
				int drive;
				if ( VFSGetDriveType(sp, &drive, NULL) != NULL ){
					if ( drive >= VFSPT_FILESCHEME ){ // VFSPT_FTP, VFSPT_SHELLSCHEME, VFSPT_HTTP
						sp = NilStr;
					}
				}
			}
			for ( ; *sp ; sp++ ){
				if ( (*sp >= '!') && (*sp <= '?') ){
					if ( tstrchr(DetectPathWildcardLetter, *sp) != NULL ){
						dest[0] = '/';
						tstrcpy(dest + 1, src);
						return;
					}
				}
			}
		}
	}
	tstrcpy(dest, src);
}

int GetDirSettingOption(PPC_APPINFO *cinfo, const TCHAR **param, int type, TCHAR *path)
{
	TCHAR buf[CMDLINESIZE];
	const TCHAR *ptr = *param;
	int mode;

	(*param)++;
	GetLineParamS(param, buf, TSIZEOF(buf));

	path[0] = '\0';
	if ( (*ptr == '\"') || (*(ptr + 1) == '\"') ){
		PathWildItemName(path, buf);
		mode = DSMD_PATH_BRANCH;
		if ( tstrcmp(buf, StrArchiveMode) == 0 ) mode = DSMD_ARCHIVE;
		if ( tstrcmp(buf, StrListfileMode) == 0 ) mode = DSMD_LISTFILE;
	}else if ( tstrcmp(buf, T("used")) == 0 ){
		path[0] = '\0';
		if ( type == ITEMSETTING_SORT ){
			if ( cinfo->XC_sort.mode.dat[0] >= 0 ){ // 保持設定が有効
				mode = CRID_SORT_REGID - CRID_SORTEX;
			}else{
				mode = FindSortSetting(cinfo, path) - CRID_SORTEX;
			}
		}else{
			mode = FindDirSetting(cinfo, type, path, buf);
		}
	}else if ( tstrcmp(buf, StrTemp) == 0 ){
		mode = DSMD_TEMP;
	}else if ( tstrcmp(buf, StrThisPath) == 0 ){
		mode = DSMD_THIS_PATH;
		LoadSetting_FixThisPath(path, cinfo->path);
	}else if ( tstrcmp(buf, StrThisBranch) == 0 ){
		mode = DSMD_THIS_BRANCH;
		LoadSetting_FixThisBranch(path, cinfo->path); // 末尾「\」付きにする
	}else if ( tstrcmp(buf, StrRegID) == 0 ){
		mode = DSMD_REGID;
	}else if ( tstrcmp(buf, StrArchiveMode) == 0 ){
		mode = DSMD_ARCHIVE;
		tstrcpy(path, StrArchiveMode);
	}else if ( tstrcmp(buf, StrListfileMode) == 0 ){
		mode = DSMD_LISTFILE;
		tstrcpy(path, StrListfileMode);
	}else {
		SetPopMsg(cinfo, POPMSG_MSG, T("target path option error"));
		return -1;
	}
	return mode;
}

#define tstrtopcmp(str, partstr, partlength) memcmp((str), (partstr), (partlength) * sizeof(TCHAR))

const TCHAR TempSetMC_celS[] = T("*setcust MC_celS:?temp?=");
// *viewstyle
ERRORCODE ViewStyleCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	const TCHAR *optlast, *ptr;
	TCHAR buf[CMDLINESIZE], path[VFPS];
	int mode = DSMD_NOMODE, custsize;
	BOOL nzfix;

	ptr = param;
	if ( SkipSpace(&ptr) == '-' ){
		mode = GetDirSettingOption(cinfo, &ptr, ITEMSETTING_DISP, path);
		if ( mode < 0 ) return ERROR_INVALID_PARAMETER;
		mode += CRID_VIEWFORMATEX;
		if ( mode == CRID_VIEWFORMAT_NOMODE ){
			mode = CRID_VIEWFORMAT_REGID;
		}
	}
	if ( SkipSpace(&ptr) == '\0' ){
		return SetCellDisplayFormat(cinfo, mode, path);
	}
	optlast = ptr;
	GetLineParamS(&ptr, buf, TSIZEOF(buf));

	if ( tstrcmp(buf, T("edit")) == 0 ){
		return SetCellDisplayFormat(cinfo, CRID_VIEWFORMAT_EDIT, NULL);
	}
	if ( tstrcmp(buf, T("directory")) == 0 ){
		return SetCellDisplayFormat(cinfo, CRID_VIEWFORMAT_PATHMASK, NULL);
	}
	if ( tstrcmp(buf, T("separate")) == 0 ){
		cinfo->UseSplitPathName = !cinfo->UseSplitPathName;
		InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
		return NO_ERROR;
	}
	if ( tstrcmp(buf, T("format")) == 0 ){
		tstrcpy(buf, TempSetMC_celS);
		GetLineParamS(&ptr, buf + (sizeof(TempSetMC_celS) / sizeof(TCHAR)) - 1,
				TSIZEOF(buf) - (sizeof(TempSetMC_celS) / sizeof(TCHAR)) - 1);
		PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, buf, NULL, 0);

		memcpy(buf, param, (optlast - param) * sizeof(TCHAR));
		tstrcpy(buf + (optlast - param), T(" ?temp?"));
		ViewStyleCommand(cinfo, buf);
		DeleteCustTable(T("MC_celS"), T("?temp?"), 0);
		return NO_ERROR;
	}

	custsize = GetCustTableSize(T("MC_celS"), buf);
	if ( custsize <= 0 ){
		SetPopMsg(cinfo, POPMSG_MSG, T("view setting not found"));
		return ERROR_INVALID_PARAMETER;
	}
	FreeCFMT(&cinfo->celF);
	nzfix = LoadCelFFMT(cinfo, T("MC_celS"), buf);

	if ( (mode == DSMD_NOMODE) || (mode == CRID_VIEWFORMAT_NOMODE) || (mode == CRID_VIEWFORMAT_REGID) ){
		SetCustTable(T("XC_celF"), cinfo->RegCID + 1, cinfo->celF.fmtbase,
				custsize);
		cinfo->FixcelF = FALSE;
	}else if ( mode == CRID_VIEWFORMAT_TEMP ){
		cinfo->FixcelF = TRUE;
	}else{ // DSMD_THIS_PATH, DSMD_THIS_BRANCH, DSMD_PATH_BRANCH, DSMD_ARCHIVE
		SetNewXdir(path, LOADDISPSTR, buf);
		cinfo->FixcelF = TRUE;
	}
	FixCellDisplayFormat(cinfo);
	if ( nzfix ) LoadCelFFMTfix(cinfo);
	return NO_ERROR;
}

DWORD_PTR PPcAddExMenu(PPC_APPINFO *cinfo, ADDEXMENUINFO *addmenu)
{
	if ( !tstrcmp(addmenu->exname, T("sortmenu")) ){
		ThSTRUCT PopupTbl;
		HMENU hSortMenu;
		DWORD mid, id;

		ThInit(&PopupTbl);
		hSortMenu = AddPPcSortMenu(cinfo, addmenu->hMenu, &PopupTbl, &mid, &id, 1, NULL);
		SortMenuCheck(hSortMenu, (TCHAR *)PopupTbl.bottom, id, &cinfo->XC_sort, 0);
		ThFree(&PopupTbl);
		return PPXCMDID_ADDEXMENU;
	}
	if ( !tstrcmp(addmenu->exname, T("viewmenu")) ){
		AddPPcCellDisplayMenu(cinfo, addmenu->hMenu, NULL, NULL, NULL);
		return PPXCMDID_ADDEXMENU;
	}
	if ( !tstrcmp(addmenu->exname, T("newmenu")) ){
		MakeMakeEntryItem(addmenu->hMenu);
		return PPXCMDID_ADDEXMENU;
	}
	if ( !tstrcmp(addmenu->exname, T("drivemenu")) ){
		DWORD index = CRID_DRIVELIST;

		MakeDriveJumpMenu(cinfo, addmenu->hMenu, &index, NULL, FALSE);
		return PPXCMDID_ADDEXMENU;
	}
	if ( !tstrcmp(addmenu->exname, T("drivelist")) ){
		MakeDriveJumpMenu(cinfo, addmenu->hMenu, addmenu->index, addmenu->TH, FALSE);
		return PPXCMDID_ADDEXMENU;
	}
	if ( !tstrcmp(addmenu->exname, T("layoutmenu")) ){
		MakeLayoutMenu(cinfo, addmenu->hMenu, addmenu->index, addmenu->TH);
		return PPXCMDID_ADDEXMENU;
	}
	if ( !tstrcmp(addmenu->exname, T("docktmenu")) ){
		PPXDOCKS *d;

		d = ( cinfo->combo ) ? &Combo.Docks : &cinfo->docks;
		MakeDockMenu(&d->t, addmenu->hMenu, addmenu->index, addmenu->TH);
		return PPXCMDID_ADDEXMENU;
	}
	if ( !tstrcmp(addmenu->exname, T("dockbmenu")) ){
		PPXDOCKS *d;

		d = ( cinfo->combo ) ? &Combo.Docks : &cinfo->docks;
		MakeDockMenu(&d->b, addmenu->hMenu, addmenu->index, addmenu->TH);
		return PPXCMDID_ADDEXMENU;
	}
	if ( !tstrcmp(addmenu->exname, T("exjumpmenu")) ){
		DelayLoadMenuStruct DelayMenus;

		DelayMenus.hMenu = addmenu->hMenu;
		DelayMenus.lParam = (LPARAM)cinfo;
		DirTypeDelayLoad(&DelayMenus);
		return PPXCMDID_ADDEXMENU;
	}

	if ( !tstrcmp(addmenu->exname, T("diroptionmenu")) ){
		DirOptionMenu(cinfo, addmenu->hMenu, addmenu->index, addmenu->TH, DSMD_TEMP, NULL);
		return PPXCMDID_ADDEXMENU;
	}

	return 0;
}

void HideEntry(PPC_APPINFO *cinfo, int index)
{
	if ( StartCellEdit(cinfo) ) return;
	HideOneEntry(cinfo, index);
	FixHideEntry(cinfo);
	EndCellEdit(cinfo);
	cinfo->DrawTargetFlags = DRAWT_ALL;
	MoveCellCsr(cinfo, 0, NULL);
}

void InsertEntry(PPC_APPINFO *cinfo, int index, const TCHAR *name, WIN32_FIND_DATA *ff)
{
	ENTRYCELL *cell;

	if ( cinfo->e.cellDataMax >= ENTRYCELL_LIMIT ) return;
	if ( StartCellEdit(cinfo) ) return;

	// ※TM_checkでcellアドレスが変わる場合有
	if ( FALSE == TM_check(&cinfo->e.CELLDATA, sizeof(ENTRYCELL) * (cinfo->e.cellDataMax + 2) ) ){
		goto fin;
	}
	if ( FALSE == TM_check(&cinfo->e.INDEXDATA, sizeof(ENTRYINDEX) * (cinfo->e.cellDataMax + 2) ) ){
		goto fin;
	}
	cell = &CELdata(cinfo->e.cellDataMax);
	SetDummyCell(cell, name);
	if ( ff != NULL ){
		cell->f = *ff;
		SetCellInfo(cinfo, cell, NULL);
	}

	if ( (index < 0) || (index >= cinfo->e.cellIMax) ){
		index = cinfo->e.cellIMax;
	}else{
		memmove(&CELt(index + 1), &CELt(index),
			((BYTE *)&CELt(cinfo->e.cellIMax) - (BYTE *)&CELt(index)));
	}
	CELt(index) = cinfo->e.cellDataMax;
	cinfo->e.cellDataMax++;
	cinfo->e.cellIMax++;
	InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
fin:
	EndCellEdit(cinfo);
}

void USEFASTCALL PPcInsertDir(PPC_APPINFO *cinfo, const TCHAR *param)
{
	WIN32_FIND_DATA ff;

	memset(&ff, 0, sizeof(ff));
	SetExtParam(param, ff.cFileName, TSIZEOF(ff.cFileName));
	if ( ff.cFileName[0] == '\0' ){
		SetPopMsg(cinfo, ERROR_FILE_NOT_FOUND, NULL);
	}
	ff.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	InsertEntry(cinfo, -1, ff.cFileName, &ff);
}

int GetStringCommand(const TCHAR **param, const TCHAR *commands)
{
	TCHAR name[VFPS], *destp, code;
	const TCHAR *oldparam;
	int count;

	code = SkipSpace(param);
	if ( (code == '-') || (code == '/') ) (*param)++;
	oldparam = *param;
	count = GetIntNumber(param);
	if ( oldparam != *param ) return count;
	destp = name;
	while ( Isalnum(**param) ){
		*destp++ = upper(*(*param)++);
	}
	*destp = '\0';
	count = 0;
	while ( *commands ){
		if ( !tstrcmp(name, commands) ) return count;
		commands += tstrlen(commands) + 1;
		count++;
	}
	return -1;
}

int PPcGetSite(PPC_APPINFO *cinfo)
{
	int site = PPCSITE_SINGLE;

	if ( cinfo->combo ){
		return (int)SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
				KCW_getsite, (LPARAM)cinfo->info.hWnd);
	}

	if ( GetJoinWnd(cinfo) != NULL ){
		site = cinfo->RegID[2] & PAIRBIT;
		if ( (cinfo->swin & SWIN_JOIN) && (cinfo->swin & SWIN_SWAPMODE) ){
			site ^= PAIRBIT;
		}
		site = 2 - site;
	}
	return site;
}

void PPcCountSize(PPC_APPINFO *cinfo, const TCHAR *param) // *countsize
{
	int emode = ENUMCELL_MARKED;
	BOOL clear = FALSE;
	TCHAR code, buf[CMDLINESIZE], *cmd;

	for (;;){
		code = GetOptionParameter(&param, buf, NULL);
		if ( code == '\0' ) break;
		cmd = (code == '-') ? (buf + 1) : buf;
		if ( tstricmp(cmd, T("clear")) == 0 ){
			clear = TRUE;
			continue;
		}
		if ( tstricmp(cmd, T("all")) == 0 ){
			emode = ENUMCELL_ALL;
			continue;
		}
		if ( tstricmp(cmd, T("cursor")) == 0 ){
			emode = ENUMCELL_CURSOR;
			continue;
		}
		if ( tstricmp(cmd, T("marked")) == 0 ){
			emode = ENUMCELL_MARKED;
			continue;
		}
	}

	if ( clear ){
		ClearMarkSize(cinfo, emode);
	}else{
		CountMarkSize(cinfo, emode);
	}
}

void PPcClipEntry(PPC_APPINFO *cinfo, const TCHAR *param, DWORD effect)
{
	UTCHAR code;

	code = SkipSpace(&param);
	if ( code == '\0' ){ // 指定なし→Text & File
		ClipFiles(cinfo, effect, CFT_FILE | CFT_TEXT);
	}else{
		code |= 0x20; // 小文字化
		if ( code == 't' ){ // Text
			ClipFiles(cinfo, effect, CFT_TEXT);
		}else if ( code == 'f' ){ // File
			ClipFiles(cinfo, effect, CFT_FILE);
		}
	}
}

void PPcSyncProperties(PPC_APPINFO *cinfo, const TCHAR *param)
{
	SYNCPROPINFO si;
	int mode;

	mode = GetStringCommand(&param, T("OFF\0") T("ON\0") T("FOCUS\0"));

	if ( mode < 0 ){ // トグル
		mode = (hPropWnd != NULL) ? 0 : 1;
	}
	switch ( mode ){
		case 0: // off
			if ( hPropWnd == NULL ) break;
			SyncProperties(cinfo->info.hWnd, NULL);
			SetPopMsg(cinfo, POPMSG_MSG, MES_SPOF);
			break;

		case 1: // on
			if ( hPropWnd != NULL ) break;
			si.ff = CEL(cinfo->e.cellN).f;
			VFSFullPath(si.filename, CellFileNameIndex(cinfo, cinfo->e.cellN), cinfo->path);
			SyncProperties(cinfo->info.hWnd, &si);
			SetPopMsg(cinfo, POPMSG_MSG, MES_SPON);
			break;

		default: // focus
			if ( hPropWnd != NULL ) SetForegroundWindow(hPropWnd);
	}
}

// 改行使用可能なパラメータ取得
TCHAR *GetAdvGetParam(const TCHAR **param, TCHAR *dest, DWORD destlength)
{
	const TCHAR *src = *param;
	TCHAR *destptr = dest;
	size_t srclen;

	srclen = tstrlen(src) + 1;
	if ( srclen > destlength ){
		TCHAR *newptr = PPcHeapAllocT(srclen);
		if ( newptr != NULL ){
			destptr = dest =newptr;
			destlength = srclen;
		}
	}
	destlength--;
	for (;;){
		TCHAR c;

		c = *src;
		if ( c == '\0' ) break;
		if ( c != '\"' ){
			if ( ((UTCHAR)c <= ' ') || (c == ',') ){
				break;
			}
			if ( destlength ){
				*destptr++ = c;
				destlength--;
			}
			src++;
			continue;
		}
		// " 処理
		src++;
		for (;;){
			c = *src;
			if ( c == '\0' ) goto end;
			if ( c == '\"' ){
				if ( *(src + 1) != '\"' ){ // 末尾？
					src++;
					break;
				}
				src++; // "" ... " 自身
			}
			if ( destlength ){
				*destptr++ = c;
				destlength--;
			}
			src++;
			continue;
		}
	}
end:
	*param = src;
	*destptr = '\0';
	return dest;
}

void PPcEntryTip(PPC_APPINFO *cinfo, const TCHAR *param) // *entrytip
{
	TCHAR buf[CMDLINESIZE];
	int mode = 0;
	int flags;
	int newid;

	flags = (cinfo->PopupPosType == PPT_FOCUS) ? 0 : STIP_CMD_MOUSE;
	for (;;){
		TCHAR chr;

		chr = SkipSpace(&param);
		if ( chr == '\0' ) break;
		if ( chr == '\"' ){
			TCHAR *ptr;

			ptr = GetAdvGetParam(&param, buf, TSIZEOF(buf));
			ThSetString(&cinfo->StringVariable, T("TipText"), ptr);
			if ( ptr != buf ) PPcHeapFree(ptr);
			mode = stip_mode_text;
		}
		GetOptionParameter(&param, buf, NULL);
		// command
		if ( tstrcmp(buf, T("filename") ) == 0 ) mode = stip_mode_filename;
		if ( tstrcmp(buf, T("fileinfo") ) == 0 ) mode = stip_mode_fileinfo;
		if ( tstrcmp(buf, T("comment") ) == 0 ) mode = stip_mode_comment;
		if ( tstrcmp(buf, T("preview") ) == 0 ) mode = stip_mode_preview;
		if ( tstrcmp(buf, T("previous") ) == 0 ) mode = cinfo->Tip.prev_stip_mode;
		if ( tstrcmp(buf, T("toggle") ) == 0 ) mode = 0;
		if ( tstricmp(buf, T("close") ) == 0 ){
			HideEntryTip(cinfo);
			return;
		}
		// option
		if ( tstrcmp(buf + 1, T("MOUSE") ) == 0 ) setflag(flags, STIP_CMD_MOUSE);
		if ( tstrcmp(buf + 1, T("CELL") ) == 0 ) resetflag(flags, STIP_CMD_MOUSE);
		if ( tstrcmp(buf + 1, T("COMBO") ) == 0 ) setflag(flags, STIP_CMD_COMBO );
		if ( tstrcmp(buf + 1, T("PPC") ) == 0 ) setflag(flags, STIP_CMD_PPC);
		if ( tstrcmp(buf + 1, T("PAIR") ) == 0 ) setflag(flags, STIP_CMD_PAIR);
		if ( tstrcmp(buf + 1, T("PPV") ) == 0 ) setflag(flags, STIP_CMD_PPV);
		if ( tstrcmp(buf + 1, T("FOCUS") ) == 0 ) setflag(flags, STIP_FOCUS);
		if ( tstrcmp(buf + 1, T("C") ) == 0 ){
			mode = stip_mode_preview;
			ThSetString(&cinfo->StringVariable, T("TipCommand"), param);
			break;
		}
	}
	// 前回と同じ内容かどうかのIDを算出（オーバフロー前提の計算）
	newid = cinfo->e.cellN * cinfo->e.cellN * cinfo->LoadCounter;
	if ( mode <= 0 ){
		mode = cinfo->Tip.prev_stip_mode;
		if ( newid == cinfo->Tip.prev_cell_id ) mode--;
		if ( (mode == stip_mode_comment) &&
			 (CEL(cinfo->e.cellN).comment == EC_NOCOMMENT) ){
			mode--;
		}
		if ( mode <= 0 ) mode = stip_mode_togglemax;
	}
	cinfo->Tip.prev_stip_mode = mode;
	cinfo->Tip.prev_cell_id = newid;
	ShowEntryTip(cinfo, flags, mode, cinfo->e.cellN);
}

void PPcOCX(PPC_APPINFO *cinfo, const TCHAR *param, int ocx_mode) // *APPVIEW / .*IE / *WMP
{
	TCHAR buf[CMDLINESIZE], execpath[CMDLINESIZE], *more;
	HWND hParentWnd;
	TCHAR chr;

	ThGetString(&cinfo->StringVariable, T("TipWnd"), buf, TSIZEOF(buf));
	more = buf;
	hParentWnd = (HWND)GetNumber((const TCHAR **)&more);

	buf[0] = '\0';
	for (;;){
		chr = SkipSpace(&param);
		if ( chr == '\0' ) break;
		GetCommandParameter(&param, buf, TSIZEOF(buf));
	}
	if ( buf[0] == '\0' ){
		ThGetString(&cinfo->StringVariable, T("TipTarget"), buf, CMDLINESIZE);
	}
	if ( (ocx_mode == OCX_APPVIEW) || (ocx_mode == OCX_APPTEXT) ){
		if ( (ocx_mode == OCX_APPTEXT) || (SetPreviewWindow(hParentWnd, buf) == NULL) ){
			if ( TextFromIFilter(hParentWnd, buf) == FALSE ){
				thprintf(execpath, TSIZEOF(execpath), T(PPVEXE) T(" -r -bootid:PX -P%u \"%s\""),
					(DWORD)(DWORD_PTR)hParentWnd, buf);
				ComExec(hParentWnd, execpath, PPcPath);
			}
		}
//		if ( cinfo->Tip.states & STIP_FOCUS ){ // 機能せず
//			SetForegroundWindow(GetWindow(hParentWnd, GW_CHILD));
//		}
		return;
	}
	LoadOcx(hParentWnd, buf, ocx_mode);
}

void SetEntryImage(PPC_APPINFO *cinfo, const TCHAR *param) // *setentryimage
{
	TCHAR buf[CMDLINESIZE];
	int offset;

	GetLineParamS(&param, buf, TSIZEOF(buf));
	if ( cinfo->EntryIcons.hImage == INVALID_HANDLE_VALUE ){ //アイコン保存場所の準備
		SetPopMsg(cinfo, POPMSG_MSG, T("no image mode"));
		return;
	}else if ( 0 <= (offset = FindCellFormatImagePosition(cinfo->celF.fmt)) ){
		if ( buf[0] != '\0' ){
			VFSFullPath(buf, buf, cinfo->RealPath);
			if ( LoadCellImage(cinfo, &CEL(cinfo->e.cellN), cinfo->celF.fmt + offset, buf, NULL) == FALSE ){
				SetPopMsg(cinfo, POPMSG_MSG, T("load error"));
				return;
			}
		}
	}else{
		SetPopMsg(cinfo, POPMSG_MSG, T("no image mode"));
		return;
	}
	if ( (GetOptionParameter(&param, buf, NULL) == '-') &&
		 (tstrcmp(buf + 1, T("SAVE")) == 0) &&
		 (CEL(cinfo->e.cellN).icon >= 0) ){
		VFSFullPath(buf, CEL(cinfo->e.cellN).f.cFileName, cinfo->RealPath);
		if ( LoadImageSaveAPI() != FALSE ){
			LPVOID lpBits;
			BITMAPINFOHEADER bmiHeader;
			HBITMAP hbmp;
			HGDIOBJ hOldBmp;
			HDC hDC;
			size_t seplen;

			memset(&bmiHeader, 0, sizeof(BITMAPINFOHEADER));
			bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmiHeader.biWidth = cinfo->fontX * max(cinfo->celF.fmt[offset + 1], 1);
			bmiHeader.biHeight = cinfo->fontY * max(cinfo->celF.fmt[offset + 2], 1);
			bmiHeader.biPlanes = 1;
			bmiHeader.biBitCount = 32;

			hbmp = CreateDIBSection(NULL, (LPBITMAPINFO)&bmiHeader, DIB_RGB_COLORS, &lpBits, NULL, 0);
			if ( hbmp != NULL ){
				hDC = CreateCompatibleDC(NULL);
				hOldBmp = SelectObject(hDC, hbmp);

				DImageList_Draw(cinfo->EntryIcons.hImage, CEL(cinfo->e.cellN).icon, hDC, 0, 0, ILD_NORMAL);

				SelectObject(hDC, hOldBmp);
				DeleteDC(hDC);

				seplen = tstrlen(buf);
				tstrcpy(buf + seplen, EntryImageThumbName);

				SaveCacheFile(cinfo, buf, seplen, (BITMAPINFO *)&bmiHeader, lpBits);

				DeleteObject(hbmp); // ビットマップのメモリも削除
			}
		}
	}
}

void PPcDockCommand(PPC_APPINFO *cinfo, const TCHAR *param) // *dock
{
	TCHAR buf[CMDLINESIZE];
	POINT pos;
	int mode = dock_menu;
	BOOL redraw = FALSE;

	// コマンド
	GetCommandParameter(&param, buf, TSIZEOF(buf));
	if ( buf[0] != '\0' ){
		if ( !tstrcmp(buf, T("menu")) ){ // mode 設定済み
		}else if ( !tstrcmp(buf, T("add")) ){
			mode = dock_add;
		}else if ( !tstrcmp(buf, T("delete")) ){
			mode = dock_delete;
			redraw = TRUE;
		}else if ( !tstrcmp(buf, T("toggle")) ){
			mode = dock_toggle;
			redraw = TRUE;
		}else if ( !tstrcmp(buf, T("focus")) ){
			mode = dock_focus;
		}else if ( !tstrcmp(buf, T("drop")) ){
			mode = dock_drop;
		}else if ( !tstrcmp(buf, T("sendkey")) ){
			mode = dock_sendkey;
		}else if ( !tstrcmp(buf, T("size")) ){
			mode = dock_size;
			redraw = TRUE;
		}else{
			return;
		}
	}
	NextParameter(&param);
	// 場所 t / b
	GetCommandParameter(&param, buf, TSIZEOF(buf));
	buf[0] = upper(buf[0]);
	NextParameter(&param);
	GetPopupPosition(cinfo, &pos);
	if ( cinfo->combo ){
		if ( mode == dock_menu ){
			PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND, TMAKELPARAM(KCW_dock, buf[0]), (LPARAM)&pos);
		}else{
			TCHAR *ptr;

			// パラメータ & choptail
			SkipSpace(&param);
			tstrcpy(buf + 1, param);
			ptr = buf + 1;
			ptr += tstrlen(ptr);
			while ( (ptr > (buf + 1)) && (*(ptr - 1) == ' ') ) ptr--;
			*ptr = '\0';
			SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND, TMAKELPARAM(KCW_dock, buf[0] | (mode * 0x100) ), (LPARAM)(buf + 1));
		}
	}else{
		PPXDOCK *dock;

		dock = (buf[0] == 'B') ? &cinfo->docks.b : &cinfo->docks.t;
		if ( mode == dock_menu ){
			DockModifyMenu(cinfo->info.hWnd, dock, &pos);
			WmWindowPosChanged(cinfo);
		}else if ( mode == dock_drop ){
			DockDropBar(cinfo, dock, param);
		}else{
			tstrcpy(buf, param);
			DockCommands(cinfo->info.hWnd, dock, mode, buf);
			if ( redraw ) WmWindowPosChanged(cinfo);
		}
	}
}

// *makelistfile
void MakeListFileCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	TCHAR path[VFPS], chr;
	int flags = WLFC_COMMAND_DEFAULT;

	path[0] = '\0';
	for (;;){
		chr = SkipSpace(&param);
		if ( (chr != '-') && (chr != '/') ){ // オプション以外
			if ( path[0] == '\0' ){
				GetCommandParameterDual(&param, path, TSIZEOF(path));
				if ( path[0] == '\0' ){
					GetNewName(path, T("listfile.txt"), cinfo->RealPath);
				}
				VFSFixPath(NULL, path, cinfo->path, VFSFIX_PATH | VFSFIX_NOFIXEDGE);
				if ( NextParameter(&param) == FALSE ) break;
				continue;
			}
		}// オプション
		switch ( GetStringCommand(&param, T("NORMAL\0") T("MARKED\0") T("MARKTAG\0") T("NAME\0") T("BASIC\0") T("COLOR\0") T("HIGHLIGHT\0") T("COMMENT\0") T("COLUMNS\0") T("MESSAGE\0") T("NOHEADER\0") T("UTF8\0") T("UTF8BOM\0") T("APPEND\0") T("ALL\0") T("DIRSHOW\0") T("JSON")) ){
			case 0: // normal
				flags = FormatResetWLFC(flags) | WLFC_COMMAND_DEFAULT;
				break;

			case 1: // marked
				flags |= WLFC_MARKEDONLY;
				break;

			case 2: // marktag
				flags |= WLFC_WITHMARK;
				break;

			case 3: // name
				flags = FormatResetWLFC(flags) | WLFC_NAMEONLY;
				break;

			case 4: // basic
				flags = FormatResetWLFC(flags) | WLFC_DETAIL;
				break;

			case 5: // color
				flags |= WLFC_COLOR;
				break;

			case 6: // highlight
				flags |= WLFC_HIGHLIGHT;
				break;

			case 7: // comment
				flags |= WLFC_COMMENT;
				break;

			case 8: // columns
				flags |= WLFC_COLUMNS;
				break;

			case 9: // message
				flags |= WLFC_SYSMSG;
				break;

			case 10: // noheader
				flags |= WLFC_NOHEADER;
				break;

			case 11: // utf8
				flags |= WLFC_UTF8;
				break;

			case 12: // utf8bom
				flags |= WLFC_UTF8 | WLFC_BOM;
				break;

			case 13: // append
				flags |= WLFC_APPEND;
				break;

			case 14: // all
				flags = FormatResetWLFC(flags) | WLFC_ALL;
				break;

			case 15: // dirshow
				flags = WLFC_DIRSHOW;
				break;

			case 16: // json
				flags = (flags & ~WLFC_OPTIONSTR) | WLFC_JSON | WLFC_UTF8;
				break;

			case -1:
				XMessage(NULL, NULL, XM_GrERRld, StrBadOption, param);
		}
		if ( NextParameter(&param) == FALSE ) break;
	}
	WriteListFileForUser(cinfo, path, flags);
}

void PPvOptionCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	TCHAR buf[CMDLINESIZE];
	BOOL setsync;

	GetLineParamS(&param, buf, TSIZEOF(buf));
	NextParameter(&param);
	setsync = (tstrcmp(buf, T("setsync")) == 0) ? TRUE : FALSE;

	if ( (tstrcmp(buf, T("sync")) == 0) || setsync ){
		GetLineParamS(&param, buf, TSIZEOF(buf));
		if ( tstrcmp(buf, T("on")) == 0 ){
			SetSyncView(cinfo, 1);
		}else if ( tstrcmp(buf, T("off")) == 0 ){
			SetSyncView(cinfo, 0);
		}else if ( Isalpha(buf[0]) ){
			SetSyncView(cinfo, buf[0]);
		}else{
			PPcCommand(cinfo, K_raw | K_s | 'Y');
		}
		if ( setsync ){
			buf[0] = '\0';
			if ( cinfo->SyncViewFlag & PPV_BOOTID ){
				buf[0] = (TCHAR)(cinfo->SyncViewFlag >> 24);
				buf[1] = '\0';
			}
			SetCustStringTable(Str_others, T("SyncViewID"), buf, 0);
		}
	}else if ( tstrcmp(buf, T("id")) == 0 ){
		if ( Isalpha(*param) ){
			cinfo->NormalViewFlag = (upper(*param) << 24) | PPV_BOOTID;
		}else{
			cinfo->NormalViewFlag = 0;
		}
	}
}

void PPcFileCommand(PPC_APPINFO *cinfo, const TCHAR *param) // *ppcfile
{
	TCHAR mode[VFPS];
	const TCHAR *optionptr = NULL;
	TCHAR path[VFPS], *destpath = NULL, *modeptr;

	modeptr = mode;
	if ( SkipSpace(&param) == '!' ){
		*modeptr++ = '!';
		param++;
	}

	if ( SkipSpace(&param) == '\"' ){ // actionname
		GetCommandParameter(&param, modeptr, TSIZEOF(mode));
	}else{
		TCHAR code;

		for ( ;; ){
			code = *param;
			if ( (code == '\0') || (code == ',') ||
				 (code == ' ')  || (code == '\t') ){
				break;
			}
			*modeptr++ = code;
			param++;
		}
		*modeptr = '\0';
	}

	if ( mode[0] == '\0' ) tstrcpy(mode, FileOperationMode_Copy);

	{
		TCHAR code = SkipSpace(&param);

		if ( (code != ',') && (code != '\0') ){
			optionptr = param;
		}else if ( IsTrue(NextParameter(&param)) ){
			GetCommandParameter(&param, path, TSIZEOF(path));
			if ( path[0] != '\0' ) destpath = path;
			if ( IsTrue(NextParameter(&param)) ){
				if ( SkipSpace(&param) != '\0' ) optionptr = param;
			}
		}
	}
	PPcFileOperation(cinfo, mode, destpath, optionptr);
}

BOOL RangeOption(EnumCellStruct *enums, PPC_APPINFO *cinfo, const TCHAR *param)
{
	ENTRYINDEX index = NO_MARK_ID;

	if ( tstricmp(param, T("CURSOR")) == 0 ){
		index = cinfo->e.cellN;
	}else if ( Isdigit(*param) ){
		index = GetNumber(&param);
	}else if ( tstricmp(param, T("ALL")) == 0 ){
		enums->index = 0;
		enums->last = cinfo->e.cellIMax - 1;
	}else if ( tstricmp(param, T("FIRST")) == 0 ){
		index = 0;
		for (;;){
			if ( !(CEL(index).attr & (ECA_THIS | ECA_PARENT | ECA_ETC)) &&
				 (CEL(index).state >= ECS_NORMAL) ){
				break;
			}
			index++;
			if ( index >= cinfo->e.cellIMax ) break;
		}
	}else if ( tstricmp(param, T("FIRSTMARK")) == 0 ){
		index = GetCellIndexFromCellData(cinfo, cinfo->e.markTop);
	}else if ( tstricmp(param, T("LAST")) == 0 ){
		index = cinfo->e.cellIMax - 1;
	}else if ( tstricmp(param, T("LASTMARK")) == 0 ){
		index = GetCellIndexFromCellData(cinfo, cinfo->e.markLast);
	}else if ( tstricmp(param, T("POINT")) == 0 ){
		index = cinfo->e.cellPoint;
	}else if ( tstricmp(param, T("MARKED")) == 0 ){
		enums->index = ENDMARK_ID;
	}else{
		return FALSE;
	}
	if ( index >= 0 ){
		if ( index >= cinfo->e.cellIMax ) index = cinfo->e.cellIMax - 1;
		if ( enums->index < 0 ){
			enums->index = index;
		}else{
			enums->last = index;
		}
	}
	return TRUE;
}

ERRORCODE RangeCommand(PPC_APPINFO *cinfo, const TCHAR *params) // *range
{
	#define MARKOFFSET -2
	int mode = MARK_CHECK + MARKOFFSET; // 0< マーク関係、0>:highlight
	int MarkMask = MARKMASK_DIRFILE;
	TCHAR parambuf[CMDLINESIZE], paramtmp[VFPS], code;
	const TCHAR *more, *opt, *param;
	BOOL order = FALSE;
	EnumCellStruct enums;

	tstrcpy(parambuf, params);
	for (;;){
		TCHAR *ptr;
		ptr = tstrchr(parambuf, ',');
		if ( ptr == NULL ) break;
		*ptr = ' ';
	}
	enums.index = enums.last = NO_MARK_ID;

	param = parambuf;
	for ( ;; ){
		code = GetOptionParameter(&param, paramtmp, (TCHAR **)&more);
		if ( code == '\0' ) break;
		opt = (code == '-') ? paramtmp + 1 : paramtmp;
		if ( IsTrue(RangeOption(&enums, cinfo, opt)) ) continue;

//		if ( tstricmp(opt, T("DIR")) == 0 ){
//			MarkMask = MARKMASK_DIR;
//		}else
		if ( tstricmp(opt, T("FILE")) == 0 ){
			MarkMask = MARKMASK_FILE;
		}else if ( tstricmp(opt, T("MARK")) == 0 ){
			mode = MARK_CHECK + MARKOFFSET;
		}else if ( tstricmp(opt, T("UNMARK")) == 0 ){
			mode = MARK_REMOVE + MARKOFFSET;
		}else if ( tstricmp(opt, T("REVERSEMARK")) == 0 ){
			mode = MARK_REVERSE + MARKOFFSET;
		}else if ( tstricmp(opt, T("HIGHLIGHT")) == 0 ){
			if ( Isdigit(*more) ){
				mode = GetNumber(&more);
				if ( mode > ECS_HLMAX ) return ERROR_INVALID_PARAMETER;
			}else{
				mode = 1;
			}
		}else if ( tstricmp(opt, T("ORDER")) == 0 ){
			order = TRUE;
		}else{
			XMessage(cinfo->info.hWnd, NULL, XM_GrERRld, StrBadOption, paramtmp);
			return ERROR_INVALID_PARAMETER;
		}
	}
	if ( enums.index == NO_MARK_ID ) return ERROR_INVALID_PARAMETER;
	if ( enums.last < 0 ) enums.last = enums.index;
	if ( mode < 0 ){ // mark
		ENTRYINDEX i;
		ENTRYCELL *cell;

		cinfo->MarkMask = MarkMask;
		if ( enums.index == ENDMARK_ID ){ // marked
			if ( mode == MARK_CHECK + MARKOFFSET ) return NO_ERROR; // 何も起きない
			// MARK_UNMARK / MARK_REVERSE →マークが全て消える
			ClearMark(cinfo);
			return NO_ERROR;
		}

		if ( (enums.index > enums.last) && (order == FALSE) ){
			ENTRYINDEX index;

			index = enums.index;
			enums.index = enums.last;
			enums.last = index;
		}

		mode -= MARKOFFSET;
		if ( enums.index <= enums.last ){ // 正順
			for ( i = enums.index ; i <= enums.last ; i++ ){
				cell = &CEL(i);
				if ( cell->state < ECS_NORMAL ) continue; // SysMsgやDeletedなのでだめ
				CellMark(cinfo, i, mode);
			}
		}else{ // 逆順
			for ( i = enums.index ; i >= enums.last ; i-- ){
				cell = &CEL(i);
				if ( cell->state < ECS_NORMAL ) continue; // SysMsgやDeletedなのでだめ
				CellMark(cinfo, i, mode);
			}
		}
	}else{ // highlight
		BYTE hldata;
		int emode;
		ENTRYCELL *cell;

		MarkMask ^= MARKMASK_DIRFILE;
		hldata = (BYTE)mode;

		if ( enums.index == ENDMARK_ID ){
			emode = ENUMCELL_MARKED;
		}else{
			emode = ENUMCELL_RANGE;
		}
		InitEnumCell(&enums, cinfo, emode);
		while ( (cell = enums.next(&enums, cinfo)) != NULL ){
			if ( !(cell->f.dwFileAttributes & MarkMask) ){
				cell->highlight = hldata;
			}
		}
	}
	Repaint(cinfo);
	return NO_ERROR;
}

void PairRateCommand(PPC_APPINFO *cinfo, const TCHAR *param) // *pairrate
{
	int rate;
	int site = 0; // 左側なら -1, 右側なら +1
	int mode = FPS_RATE;
	TCHAR c;

	c = SkipSpace(&param);
	switch ( TinyCharLower(c) ){
		case 'a':
			if ( cinfo->combo ){
				mode = FPS_ALLSAMERATE;
			}else{
				param = T("50");
			}
			break;
		case 'l':
		case 't':
			c = *(++param);
			site = -1;
			break;
		case 'r':
		case 'b':
			c = *(++param);
			site = +1;
			break;
	}
	if ( (c == '+') || (c == '-') ) mode = FPS_FONTRATE;
	rate = GetIntNumber(&param);
	if ( (rate == 0) && (c != '0') ) rate = 50;
	if ( rate > 100 ) rate = 100;
	if ( site ){
		// 反対窓が対象なら、入れ替えを行う
		if ( (PPcGetSite(cinfo) >= PPCSITE_RIGHT) ? (site < 0) : (site > 0) ){
			if ( mode == FPS_RATE ){
				rate = 100 - rate;
			}else{
				rate = -rate;
			}
		}
	}
	if ( cinfo->combo ? (X_combos[0] & CMBS_VPANE) : (cinfo->swin & SWIN_UDJOIN) ){
		FixPaneSize(cinfo, 0, rate, mode);
	}else{
		FixPaneSize(cinfo, rate, 0, mode);
	}
}

#if NODLL
extern void GetLfGetParam(const TCHAR **param, TCHAR *dest, DWORD destlength);
#else
// 改行使用可能なパラメータ取得(空白区切り。「,」区切り不可)
void GetLfGetParam(const TCHAR **param, TCHAR *dest, DWORD destlength)
{
	const TCHAR *src = *param;
	TCHAR *destptr;

	destptr = dest;
	if ( SkipSpace(&src) == '\"' ){ // " 処理
		src++;
		for (;;){
			TCHAR code;

			code = *src;
			if ( code == '\0' ) goto end;
			if ( code == '\"' ){
				if ( *(src + 1) != '\"' ){ // 末尾？
					src++;
					break;
				}
				src++; // "" ... " 自身
			}
			if ( destlength ){
				*destptr++ = code;
				destlength--;
			}
			src++;
			continue;
		}
		dest = destptr; // 末尾空白調整位置を変更
	}

	for (;;){
		TCHAR code;

		code = *src;
		if ( code == '\0' ) break;

		if ( (UTCHAR)code <= ' ' ){
			break;
		}

		if ( code == '\"' ){
			if ( destlength ){
				*destptr++ = code;
				destlength--;
			}
			while( '\0' != (code = *src) ){
				src++;
				if ( destlength ){
					*destptr++ = code;
					destlength--;
				}
				if ( (code == '\"') && (*src != '\"') ) break;
			}
			dest = destptr; // 末尾空白調整位置を変更
		}else{
			if ( destlength ){
				*destptr++ = code;
				destlength--;
			}
			src++;
		}
		continue;
	}
	while ( (destptr > dest) && (*(destptr - 1) == ' ') ) destptr--;
end:
	*param = src;
	*destptr = '\0';
}
#endif

void LogwindowCommand(const TCHAR *param)
{
	int mode;
	const TCHAR *ptr;
	TCHAR buf[CMDLINESIZE];

	SkipSpace(&param);
	ptr = param;
	#define LOGWINDOW_FOCUS 2
//	#define LOGWINDOW_WINDOW 3
	mode = GetStringCommand(&ptr, T("OFF\0") T("ON\0") T("FOCUS\0") /* T("WINDOW\0") */ );
	if ( mode < 0 ){	// 指定無し…トグル or 文字列
		if ( *ptr != '\0' ){
			if ( *ptr == '\"' ){
				const TCHAR *ptr2 = ptr;

				buf[CMDLINESIZE - 2] = '\0';
				GetLfGetParam(&ptr, buf, CMDLINESIZE - 1);
				if ( buf[CMDLINESIZE - 2] != '\0' ){
					TCHAR *extbuf;
					int len;

					len = tstrlen(ptr2); // 「"」 が含まれているので Nil分不要
					extbuf = PPcHeapAllocT(len);
					if ( extbuf != NULL ){
						ptr = ptr2;
						GetLfGetParam(&ptr, extbuf, len - 1);
						SetReportText(extbuf);
						PPcHeapFree(extbuf);
						return;
					}
				}
				ptr = buf;
			}
			SetReportText(ptr);
			return;
		}
	}

	if ( Combo.hWnd != NULL ){
		if ( mode < 0 ){ // toggle
			ptr = REPORTTEXT_TOGGLE;
		}else{
			if ( mode >= LOGWINDOW_FOCUS ){
				ptr = (const TCHAR *)(DWORD_PTR)mode;
			}else{
				ptr = mode ? REPORTTEXT_OPEN : REPORTTEXT_CLOSE;
			}
		}
		SendMessage(Combo.hWnd, WM_PPXCOMMAND, TMAKEWPARAM(K_WINDDOWLOG, PPLOG_REPORT), (LPARAM)ptr);
		return;
	}
	if ( mode < 0 ){ // toggle
		if ( (hCommonLog == NULL) || (IsWindow(hCommonLog) == FALSE) ){
			mode = 1;
		}else{
			mode = 0;
		}
	}
	if ( mode ){ // on / focus
		SetReportText(NULL);
		if ( mode == LOGWINDOW_FOCUS ) SetFocus(hCommonLog);
	}else{ // off
		if ( hCommonLog != NULL ){
			PPxCommonExtCommand(K_SETLOGWINDOW, 0);
			PostMessage(GetParent(hCommonLog), WM_CLOSE, 0, 0);
			hCommonLog = NULL;
		}
	}
}

// *unpack
ERRORCODE UnpackCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	TCHAR destpath[VFPS];

	VFSFixPath(destpath, (TCHAR *)param, cinfo->path, VFSFIX_PATH);
	return PPC_Unpack(cinfo, destpath);
}

// *sortentry / *setsortentry
ERRORCODE SortEntryCommand(PPC_APPINFO *cinfo, const TCHAR *param, int mode)
{
	TCHAR buf[CMDLINESIZE], path[VFPS];
	const TCHAR *ptr;
	XC_SORT xc;

	if ( (SkipSpace(&param) == '-') && !Isdigit(*(param + 1)) ){
		mode = GetDirSettingOption(cinfo, &param, ITEMSETTING_SORT, path);
		if ( mode < 0 ) return ERROR_INVALID_PARAMETER;
	}
	mode += CRID_SORTEX;

	if ( SkipSpace(&param) == '\0' ){ // メニュー表示
		return SortKeyCommand(cinfo, mode, path);
	}

	SetExtParam(param, buf, TSIZEOF(buf));
	ptr = buf;
	if ( GetCustTable(T("MC_sort"), buf, buf, sizeof(buf)) >= 0 ){
		if ( SkipSpace(&ptr) == '\0' ){ // メニュー表示
			return SortKeyCommand(cinfo, mode, path);
		}
	}
	if ( tstrcmp(ptr, T("react")) == 0 ){
		PPC_SortMain(cinfo, &cinfo->sort_last);
		return NO_ERROR;
	}

	LoadSortOpt(&ptr, &xc);
	SaveSortSetting(cinfo, mode, path, &xc);
	PPC_SortMain(cinfo, &xc);
	return NO_ERROR;
}

void MaskPathCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	int mode;

	mode = GetStringCommand(&param, T("OFF\0") T("ON\0") T("SHORT\0") );
	if ( mode == 2 ){
		cinfo->UseSplitPathName = TRUE;
		InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
		return;
	}
	if ( mode < 0 ){
		if ( cinfo->UseSplitPathName ){
			mode = 0;
		}else{
			mode = cinfo->UseArcPathMask ? ARCPATHMASK_OFF : ARCPATHMASK_ARCHIVE;
		}
	}
	cinfo->UseSplitPathName = FALSE;
	read_entry(cinfo, mode ? RENTRY_USEPATHMASK : RENTRY_NOUSEPATHMASK);
}

void PPcIDClose(PPC_APPINFO *cinfo, const TCHAR *param) // *idclose 、*closeppxからの呼び出し用
{
	TCHAR text[MAX_PATH];
	FN_REGEXP fn;
	int baseindex;

	GetCommandParameter(&param, text, TSIZEOF(text));
	if ( (text[0] == '\0') || (Combo.hWnd == NULL) ){
		PostMessage(cinfo->info.hWnd, WM_CLOSE, 0, 0);
		return;
	}
	if ( MakeFN_REGEXP(&fn, text) & REGEXPF_ERROR ) return;

	for ( baseindex = 0 ; baseindex < Combo.BaseCount ; baseindex++ ){
		PPC_APPINFO *ccinfo;

		ccinfo = Combo.base[baseindex].cinfo;
		if ( ccinfo != NULL ){
			if ( FilenameRegularExpression(ccinfo->RegSubCID, &fn) ){
				PostMessage(ccinfo->info.hWnd, WM_CLOSE, 0, 0);
			}
		}
	}
	FreeFN_REGEXP(&fn);
}

void PPcEllipsisType(PPC_APPINFO *cinfo, const TCHAR *param) // *ellipsis
{
	int mode;

	mode = GetStringCommand(&param, T("NONE\0") T("END\0") T("TOP\0") T("MID\0"));
	if ( mode < 0 ){ // toggle
		EllipsisType++;
		if ( EllipsisType > ELLIPSIS_MAX ) EllipsisType = ELLIPSIS_END;
	}else{
		EllipsisType = mode;
	}
#ifndef UNICODE
	UseDrawText = UsePFont || (EllipsisType != ELLIPSIS_END);
#endif
	DrawNameFlags = (EllipsisType == ELLIPSIS_NONE) ? DRAWNAME_NOEL : DRAWNAME_NOMAL;
	InvalidateRect(cinfo->info.hWnd, NULL, TRUE);
}

void CacheCommand(PPC_APPINFO *cinfo, const TCHAR *param) // *cache
{
	int mode;
	LOADSETTINGS_MOD ls;

	if ( SkipSpace(&param) == 't' ){
		DirTaskCommand(cinfo);
		return;
	}

	mode = GetStringCommand(&param, T("OFF\0") T("ON\0") T("ASYNC\0"));

	LoadLs(&ls, cinfo->path);

	if ( mode == -1 ){ // toggle
		ls.cust.dset.flags = (WORD)((ls.cust.dset.flags & ~DSET_ASYNCREAD) ^ DSET_CACHEONLY);
		mode = (ls.cust.dset.flags & DSET_CACHEONLY) ? 1 : 0;
	}else if ( mode == 2 ){ // async
		ls.cust.dset.flags = (WORD)((ls.cust.dset.flags & ~DSET_CACHEONLY) | DSET_ASYNCREAD);
	}else if ( mode ){ // on
		ls.cust.dset.flags = (WORD)((ls.cust.dset.flags & ~DSET_ASYNCREAD) | DSET_CACHEONLY);
	}else{
		resetflag(ls.cust.dset.flags, DSET_CACHEONLY);
	}

	SaveLs(&ls, cinfo->path);
	FreeLs(&ls);
	SetPopMsg(cinfo, POPMSG_MSG, mode ? MES_CAON : MES_CAOF);
}

BOOL CALLBACK CheckChildIdProc(HWND hWnd, LPARAM lParam)
{
	if ( GetWindowLongPtr(hWnd, GWLP_ID) == ((CCIPSTRUCT *)lParam)->id ){
		((CCIPSTRUCT *)lParam)->hChildWnd = hWnd;
		return FALSE;
	}
	return TRUE;
}

void ADD_Caption(PPC_APPINFO *cinfo, AUTODDINFO *ainfo, const TCHAR **param)
{
	TCHAR paramtmp[VFPS];

	if ( ('\0' == GetCommandParameterDual(param, paramtmp, TSIZEOF(paramtmp))) || (paramtmp[0] == '\0') ){
		ainfo->hTargetWnd = NULL;
	}else{
		CCIPSTRUCT ccip;

		ainfo->hTargetWnd = GetWindowHandleByText(&cinfo->info, paramtmp);
		if ( **param == ',' ) (*param)++;
		ccip.id = GetNumber(param);

		if ( ccip.id ){ // 子IDがあるなら検索
			ccip.hChildWnd = NULL;
			EnumChildWindows(ainfo->hTargetWnd, CheckChildIdProc, (LPARAM)&ccip);
			ainfo->hTargetWnd = ccip.hChildWnd;
		}
	}
}

void ADD_Mode(AUTODDINFO *ainfo, const TCHAR **param)
{
	TCHAR code;

	code = TinyCharLower(**param);
	if ( code == 'r' ){
		(*param)++;
		ainfo->droptype = DROPTYPE_HOOK | DROPTYPE_RIGHT;
	}else if ( code == 'c' ){
		(*param)++;
		ainfo->droptype = DROPTYPE_HOOK | DROPTYPE_COPY;
	}else if ( code == 'm' ){
		(*param)++;
		ainfo->droptype = DROPTYPE_HOOK | DROPTYPE_MOVE;
	}
}

ERRORCODE AutoDragDropCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	AUTODDINFO ainfo;
	TCHAR paramtmp[VFPS], code, *more;
	TCHAR src[VFPS];
	TCHAR title[VFPS];
	TCHAR filename[VFPS];

	ainfo.hTargetWnd = NULL;
	ainfo.src = NULL;
	ainfo.mask_title = NULL;
	ainfo.mask_filename = NULL;
	ainfo.droptype = DROPTYPE_HOOK | DROPTYPE_LEFT;

	for (;;){
		code = SkipSpace(&param);
		if ( code == '\0' ) break;
		if ( (code != '-') && (code != '/') ){
			ADD_Caption(cinfo, &ainfo, &param);
			if ( IsTrue(NextParameter(&param)) ){
				if ( !Isalpha(*param) ) continue;
				ADD_Mode(&ainfo, &param);
				if ( IsTrue(NextParameter(&param)) ){
					code = SkipSpace(&param);
					if ( (code == '-') || (code == '/') ) continue;
					GetCommandParameter(&param, src, TSIZEOF(src));
					if ( src[0] != '\0' ) ainfo.src = src;
				}
			}
		}else{
			code = GetOptionParameter(&param, paramtmp, &more);
			if ( code == '\0' ) break;
			if ( tstrcmp(paramtmp + 1, T("MODE")) == 0 ){
				ADD_Mode(&ainfo, (const TCHAR **)&more);
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("SRC")) == 0 ){
				if ( more[0] != '\0' ){
					tstrcpy(src, more);
					ainfo.src = src;
				}
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("MASKTITLE")) == 0 ){
				if ( more[0] != '\0' ){
					tstrcpy(title, more);
					ainfo.mask_title = title;
				}
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("MASKFILE")) == 0 ){
				if ( more[0] != '\0' ){
					tstrcpy(filename, more);
					ainfo.mask_filename = filename;
				}
				continue;
			}
		}
	}
	if ( ainfo.hTargetWnd == NULL ){
		AutoDDDialog(cinfo, &ainfo, 0);
		return NO_ERROR; // AutoDDDialog はモードレス
	}else{
		return StartAutoDD(cinfo, ainfo.hTargetWnd, ainfo.src, ainfo.droptype);
	}
}

ERRORCODE CheckOffScreenMarkCommand(PPC_APPINFO *cinfo)
{
	return (CheckOffScreenMark(cinfo, NULL) == IDYES) ? NO_ERROR : ERROR_CANCELLED;
}

ERRORCODE SetTempColor(PPC_APPINFO *cinfo, const TCHAR *param) // *color
{
	TCHAR type;
	COLORREF clr;

	type = SkipSpace(&param);
	while ( *param > ' ' ) param++;
	while ( (*param == ' ') || (*param == ',') ) param++;
	clr = (*param > ' ') ? GetColor(&param, TRUE) : C_AUTO;
	if ( type == 'c' ){
		cinfo->CursorColor = clr;
		RefleshCell(cinfo, cinfo->e.cellN);
	}else if ( type == 'b' ){
		if ( clr == C_AUTO ){
			clr = C_back;
			LoadWallpaper(&cinfo->bg, cinfo->info.hWnd, cinfo->RegCID);
		}else{
			UnloadWallpaper(&cinfo->bg);
		}
		cinfo->BackColor = clr;
		DeleteObject(cinfo->C_BackBrush);
		cinfo->C_BackBrush = CreateSolidBrush(cinfo->BackColor);
		InvalidateRect(cinfo->info.hWnd, NULL, TRUE);
	}
	return NO_ERROR;
}

// *capturewindow
ERRORCODE CaptureWindowCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	HWND hWnd;
	TCHAR paramtmp[VFPS], code, *more;
	DWORD command = KCW_capture;

	hWnd = NULL;
	for ( ;; ){
		code = GetOptionParameter(&param, paramtmp, &more);
		if ( code == '\0' ) break;
		if ( code != '-' ){
			hWnd = GetWindowHandleByText(&cinfo->info, paramtmp);
		}else{
			if ( tstrcmp(paramtmp + 1, T("NOACTIVE")) == 0 ){
				setflag(command, KCW_entry_NOACTIVE);
			}else
			if ( tstrcmp(paramtmp + 1, T("SELECTNOACTIVE")) == 0 ){
				setflag(command, KCW_entry_SELECTNA);
			}else
			if ( tstrcmp(paramtmp + 1, T("PANE")) == 0 ){
				if ( *more == '\0' ){
					command |= (PSPONE_PANE_NEWPANE + 1) * KCW_entry_DEFPANE;
				}else
				if ( tstrcmp(more, T("~")) == 0 ){
					command |= (PSPONE_PANE_PAIR + 1) * KCW_entry_DEFPANE;
				}else
				if ( tstrcmp(more, T("l")) == 0 ){
					command |= (PSPONE_PANE_SETPANE + 1) * KCW_entry_DEFPANE;
				}else
				if ( tstrcmp(more, T("r")) == 0 ){
					int showindex = GetComboShowIndex(hComboSubPaneFocus);
					if ( showindex < 0 ) break;
					command |= (PSPONE_PANE_SETPANE + 1 + showindex) * KCW_entry_DEFPANE;
				}else
				{
					int num = GetNumber((const TCHAR **)&more);
					if ( num >= 0 ){
						command |= (PSPONE_PANE_SETPANE + 1 + num) * KCW_entry_DEFPANE;
					}
				}
			}else{
				XMessage(cinfo->info.hWnd, NULL, XM_GrERRld, StrBadOption, paramtmp);
				return ERROR_INVALID_PARAMETER;
			}
		}
	}
	if ( hWnd == NULL ) return ERROR_INVALID_DATA;

	// 子にするときは先にフラグを設定する必要がある。※親なら逆
	SetWindowLong(hWnd, GWL_STYLE,
			(GetWindowLongPtr(hWnd, GWL_STYLE) & ~(WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX) ) | WS_CHILD | WS_CLIPSIBLINGS);

	if ( cinfo->combo ){
		UseCapture = TRUE;
		SetParent(hWnd, cinfo->hComboWnd);
		PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND, (WPARAM)command, (LPARAM)hWnd);
	}else{
		RECT box;

		SetParent(hWnd, cinfo->info.hWnd);
		GetClientRect(cinfo->info.hWnd, &box);
		SetWindowPos(hWnd, NULL, 0, 0, box.right - box.left, box.bottom - box.top, SWP_NOZORDER);
	}
	return NO_ERROR;
}

// 補助関数 -------------------------------------------------------------------
void PPcGetIInfoEnumFix(PPC_APPINFO *cinfo, INT_PTR *work)
{
	int i = 0;

	while ( *work >= 0 ){
		i++;
		if ( i > 0xfffffff ){
			XMessage(cinfo->info.hWnd, NULL, XM_NsERRd, T("PPcGetIInfoEnumFix Error"));
			*work = (INT_PTR)(int)ENDMARK_ID;
			break;
		}
		if ( CELdata(*work).state >= ECS_NORMAL ){
			break;
		}else{				// SysMsgやDeletedなのでスキップ
			*work = (INT_PTR)(int)CELdata(*work).mark_fw;
		}
	}
}

void GetInfoEntryComment(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr, ENTRYCELL *cell)
{
	if ( cell->comment != EC_NOCOMMENT ){
		tstrcpy(uptr->str, ThPointerT(&cinfo->e.Comments, cell->comment));
	}else{
		uptr->str[0] = '\0';
	}
}

DWORD_PTR EntryFromName(PPC_APPINFO *cinfo, PPXUPTR_ENTRYINFO *entryinfo)
{
	TCHAR *name = entryinfo->result;
	int index;

	for ( index = 0 ; index < cinfo->e.cellIMax ; index++ ){
		if ( tstricmp(CEL(index).f.cFileName, name) == 0 ){
			entryinfo->index = index;
			return PPXA_NO_ERROR;
		}
	}
	return ERROR_INVALID_PARAMETER;
}

const TCHAR *GetCellSfnName(PPC_APPINFO *cinfo, ENTRYCELL *cell, BOOL uselfn)
{
	DWORD X_fstff = 1;
	TCHAR path[VFPS], *lp;

	if ( cell->f.cAlternateFileName[0] ) return cell->f.cAlternateFileName;
	GetCustData(T("X_fstff"), &X_fstff, sizeof(X_fstff));
	if ( X_fstff < 2 ) return uselfn ? CellFileName(cell) : NilStr;
	if ( VFSFullPath(path, CellFileName(cell), cinfo->RealPath) == NULL ){
		return NilStr;
	}
	if ( GetShortPathName(path, path, VFPS) == 0 ) return NilStr;
	lp = FindLastEntryPoint(path);
	if ( tstrlen(lp) >= 13 ) return NilStr;
	tstrcpy(cell->f.cAlternateFileName, lp);
	return cell->f.cAlternateFileName;
}

TCHAR *GetCellLongFileName(PPC_APPINFO *cinfo, ENTRYCELL *cell)
{
	TCHAR *name;

	name = cell->f.cFileName;
	if ( *name == FINDOPTION_LONGNAME ){
		TCHAR *longname = (TCHAR *)EntryExtData_GetDATAptr(cinfo, DFC_LONGNAME, cell);
		if ( longname != NULL ) name = longname;
	}
	return name;
}

const TCHAR *GetCellFileName(PPC_APPINFO *cinfo, ENTRYCELL *cell, TCHAR *namebuf)
{
	const TCHAR *name;

	name = cell->f.cFileName;
	if ( *name == FINDOPTION_LONGNAME ){
		const TCHAR *longname = (const TCHAR *)EntryExtData_GetDATAptr(cinfo, DFC_LONGNAME, cell);
		if ( longname != NULL ) name = longname;
	}

	if ( cinfo->e.Dtype.mode == VFSDT_PATH ) return name;
	if ( (cinfo->e.Dtype.mode == VFSDT_LFILE) && (cinfo->e.Dtype.BasePath[0] != '\0') ){
		VFSFullPath(namebuf, (TCHAR *)name, cinfo->e.Dtype.BasePath);
		name = namebuf;
/* 有効にすると、listfile とかも機能するため無効中
		int len = tstrlen(cinfo->RealPath);
		if ( (memcmp(name, cinfo->RealPath, TSTROFF(len)) == 0) &&
			 (name[len] == '\\') ){
			name += len + 1;
		}else{
			VFSFullPath(namebuf, name, cinfo->e.Dtype.BasePath);
			name = namebuf;
		}
*/
	}else if ( cinfo->e.Dtype.mode == VFSDT_DLIST ){
		TCHAR *comment;

		comment = tstrchr(name, '>');
		if ( comment != NULL ){
			size_t len = (comment - name) + 1;

			tstplimcpy(namebuf, name, min(len, VFPS));
			return namebuf;
		}
		return name;
	}

	if ( !(cell->f.dwFileAttributes & FILE_ATTRIBUTEX_VIRTUAL) && (cinfo->e.Dtype.mode != VFSDT_SHN) ){
		return name;
	}
	namebuf[0] = '\0';
	// SHN の WIN32_FIND_DATA が取得できなかったエントリ向けの処理
	if ( (VFSFullPath(namebuf, (TCHAR *)name, cinfo->path) != NULL) &&
		/* ●1.2x↓RealPathが取得できる見込みがないエントリを判別する。
			 以下の判別は暫定の方法(時刻の取得ができない特殊フォルダ)。
			 より確実な方法を思いついたら変える。
		*/
			(cell->f.ftCreationTime.dwHighDateTime != 0) ){
		int len = tstrlen32(cinfo->RealPath);

		// 試しに実体化させ、RealPath と name で合成可能なら name を返す
		VFSGetRealPath(cinfo->info.hWnd, namebuf, namebuf);
		if ( (memcmp(namebuf, cinfo->RealPath, TSTROFF(len)) == 0) &&
			 (namebuf[len] == '\\') ){
			if ( tstrcmp(namebuf + len + 1, name) == 0 ) return name;
			// RealPath 部は一致するが、name が異なる
			// 例) ライブラリ内のファイル
			memmove(namebuf, (char *)(TCHAR *)(namebuf + len + 1), TSTRSIZE(namebuf + len + 1));
		}
	}
	return (namebuf[0] != '\0') ? namebuf : name;
}

#define FMOPT_EX_FILESIZE B20
#define FMOPT_EX_FILEATTR B21
#define FMOPT_EX_WRITEDATE B22

void GetInfoFormat_DATE(ENTRYCELL *cell, TCHAR *buf)
{
	FILETIME lTime;
	SYSTEMTIME sTime;

	if ( cell->f.ftLastWriteTime.dwLowDateTime |
		 cell->f.ftLastWriteTime.dwHighDateTime ){

		FileTimeToLocalFileTime(&cell->f.ftLastWriteTime, &lTime);
		FileTimeToSystemTime(&lTime, &sTime);
		thprintf(buf, 16, T("%04d%02d%02d"), sTime.wYear, sTime.wMonth, sTime.wDay);
	}else{
		tstrcpy(buf, T("00000000"));
	}
}

#define InfoDir(cinfo, flags) ( \
	((cinfo->e.Dtype.mode == VFSDT_LFILE) && (cinfo->e.Dtype.BasePath[0] != '\0')) ? \
	cinfo->e.Dtype.BasePath : \
	((flags & FMOPT_ENABLEVFS) ? cinfo->path : cinfo->RealPath) )


void GetInfoFormat(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr) // %F
{
	const TCHAR *ptr;
	TCHAR buf[VFPS + 4];
	DWORD flags = 0;

	ptr = ((PPXCMD_F *)uptr->enums.buffer)->source;
	while( Isalpha(*ptr) ){
		switch ( *ptr++ ){
			case 'A': setflag(flags, FMOPT_EX_FILEATTR); continue;
			case 'B': setflag(flags, FMOPT_BLANKET); continue;
			case 'C': setflag(flags, FMOPT_FILENAME); continue;
			case 'D': setflag(flags, FMOPT_DIR); continue;
			// 'H' は下で処理
//			case 'I': setflag(flags, FMOPT_IDL); continue;
//			case 'K': setflag(flags, FMOPT_SEP_BACKSLASH); continue;
//			case 'L': setflag(flags, FMOPT_SEP_SLASH); continue;
			case 'M': // setflag(flags, FMOPT_MARK); continue;
				if ( cinfo->e.markC == 0 ){
					((PPXCMD_F *)uptr->enums.buffer)->dest[0] = '\0';
					return;
				}
				continue;
			case 'N': setflag(flags, FMOPT_NOBLANKET); continue;
			// 'P' は下で処理
			// 'R' は下で処理
			case 'S': setflag(flags, FMOPT_USESFN); continue;
			case 'T': setflag(flags, FMOPT_FILEEXT); continue;
			// 'U' は下で処理
			case 'V': setflag(flags, FMOPT_ENABLEVFS); continue;
			case 'W': setflag(flags, FMOPT_EX_WRITEDATE); continue;
			case 'X': setflag(flags, FMOPT_FILENAME | FMOPT_FILENOEXT); continue;

			case 'Z': setflag(flags, FMOPT_EX_FILESIZE); continue;

			case 'H':
			case 'R':
			case 'U':
			case 'P':
				XMessage(cinfo->info.hWnd, T("%%F error '%C'"), XM_NsERRd, MES_EIFC, *(ptr - 1));
				continue;
		}
		ptr--;
		break;
	}
	((PPXCMD_F *)uptr->enums.buffer)->source = ptr;

	if ( (flags & (FMOPT_FILENAME | FMOPT_FILEEXT)) ||
		 ((cinfo->e.Dtype.mode == VFSDT_LFILE) && (flags & FMOPT_DIR)) ){
		const TCHAR *ss, *ext;
		TCHAR *lastentry;
		ENTRYCELL *cell;

		if ( uptr->enums.enumID != CELLENUMID_ONCURSOR ){
			if ( uptr->enums.enumID < 0 ){
				if ( (flags & (FMOPT_DIR | FMOPT_FILENAME | FMOPT_FILEEXT)) == FMOPT_DIR ){
					if ( cinfo->e.markC == 0 ){
						cell = &CEL(cinfo->e.cellN);
					}else{
						cell = &CELdata(0); // InitFindFirstListFile で "." 挿入済
					}
				}else{
					((PPXCMD_F *)uptr->enums.buffer)->dest[0] = '\0';
					return;
				}
			}else{
				cell = &CELdata(uptr->enums.enumID);
			}
		}else{
			cell = &CEL(cinfo->e.cellN);
		}
		ext = CellFileName(cell) + cell->ext;

		if ( flags & FMOPT_USESFN ){
			ss = GetCellSfnName(cinfo, cell, TRUE);
			if ( ss == NilStr ){
				ss = GetCellFileName(cinfo, cell, buf);
			}else if ( cinfo->e.Dtype.mode != VFSDT_PATH ){
				const TCHAR *tempss = GetCellFileName(cinfo, cell, buf);

				if ( tempss == buf ){
					tstrcpy(FindLastEntryPoint(tempss), FindLastEntryPoint(ss));
					ss = buf;
				}else if ( FindPathSeparator(tempss) != NULL ){
					tstrcpy(buf, tempss);
					tstrcpy(FindLastEntryPoint(buf), FindLastEntryPoint(ss));
					ss = buf;
				}
			}
		}else{
			ss = GetCellFileName(cinfo, cell, buf);
		}
		if ( IsTrue(cinfo->UnpackFix) ) ss = FindLastEntryPoint(ss);

		if ( flags & FMOPT_DIR ){
			// ListFile のとき
			if ( (flags & (FMOPT_DIR | FMOPT_FILENAME | FMOPT_FILEEXT)) == FMOPT_DIR ){
				resetflag(flags, FMOPT_FILEEXT | FMOPT_FILENOEXT); // lastentry を使わないように
				if ( flags & FMOPT_ENABLEVFS ){
					tstrcpy(buf, cinfo->path);
				}else{
					if ( cinfo->e.Dtype.BasePath[0] == '\0' ){
						VFSFullPath(buf, (TCHAR *)ss, cinfo->RealPath);
					}else{
						VFSFullPath(buf, (TCHAR *)ss, cinfo->e.Dtype.BasePath);
					}
					if ( !(cell->attr & (ECA_THIS | ECA_PARENT | ECA_ETC)) ){
						*VFSFindLastEntry(buf) = '\0';
					}
				}
			}else if ( buf != ss ){
				lastentry = VFSFullPath(buf, (TCHAR *)ss, InfoDir(cinfo, flags) );
				if ( lastentry == NULL ){
					((PPXCMD_F *)uptr->enums.buffer)->dest[0] = '\0';
					return;
				}
			}else{
				lastentry = buf;
			}
		}else{
			if ( buf != ss ) tstrcpy(buf, ss);
			lastentry = buf;
		}
		if ( (flags & (FMOPT_FILEEXT | FMOPT_FILENOEXT)) ){
			TCHAR *extptr;

			extptr = lastentry + FindExtSeparator(lastentry);
			if ( flags & FMOPT_FILENOEXT ){
				if ( *ext ) *extptr = '\0';
			}else{ // FMOPT_FILEEXT
				if ( *ext ){
					if ( *extptr == '.' ) extptr++;
					memmove(lastentry, extptr, TSTRSIZE(extptr));
				}else{
					*lastentry = '\0';
				}
			}
		}
	}else if ( flags & FMOPT_DIR ){
		tstrcpy(buf, (flags & FMOPT_ENABLEVFS) ? cinfo->path : cinfo->RealPath);
	}else if ( flags & (FMOPT_EX_FILESIZE | FMOPT_EX_FILEATTR | FMOPT_EX_WRITEDATE) ){
		ENTRYCELL *cell;

		if ( uptr->enums.enumID != CELLENUMID_ONCURSOR ){
			if ( uptr->enums.enumID < 0 ){
				((PPXCMD_F *)uptr->enums.buffer)->dest[0] = '\0';
				return;
			}else{
				cell = &CELdata(uptr->enums.enumID);
			}
		}else{
			cell = &CEL(cinfo->e.cellN);
		}

		if ( flags & FMOPT_EX_FILESIZE ){
			FormatNumber(buf, 0, XFNW_FULL_NOSEP, cell->f.nFileSizeLow, cell->f.nFileSizeHigh);
		}else if ( flags & FMOPT_EX_WRITEDATE ){
			GetInfoFormat_DATE(cell, buf);
		}else{ // FMOPT_EX_FILEATTR
			MakeAttributesString(buf, cell);
		}
	}else{
		buf[0] = '\0';
	}

	if ( flags & FMOPT_USESFN ) GetShortPathName(buf, buf, VFPS);

	if ( !(flags & FMOPT_NOBLANKET) &&
		 ((flags & FMOPT_BLANKET) || tstrchr(buf, ' ') || tstrchr(buf, ',')) ){
		thprintf(((PPXCMD_F *)uptr->enums.buffer)->dest, TSIZEOF(buf), T("\"%s\""), buf);
	}else{
		tstrcpy(((PPXCMD_F *)uptr->enums.buffer)->dest, buf);
	}
}

ENTRYINDEX USEFASTCALL GetCellIndexFromCellData(PPC_APPINFO *cinfo, ENTRYDATAOFFSET index)
{
	ENTRYINDEX celln, last;
	ENTRYDATAOFFSET target;

	last = cinfo->e.cellIMax;
	target = index;
	for ( celln = 0 ; celln < last ; celln++ ){
		if ( CELt(celln) == target ) return celln;
	}
	return NO_MARK_ID;
}

ENTRYINDEX USEFASTCALL GetCellIndex_HS(PPC_APPINFO *cinfo, ENTRYINDEX index)
{
	if ( index >= 0 ){	// INDEXDATA index
		if ( index >= cinfo->e.cellIMax ) index = 0;
		return index;
	}else{				// CELLDATA index
		resetflag(index, B31);
		if ( index >= cinfo->e.cellDataMax ) return 0;
		index = GetCellIndexFromCellData(cinfo, index);
		if ( index < 0 ) index = 0;
		return index;
	}
}

ENTRYCELL * USEFASTCALL GetCellData_HS(PPC_APPINFO *cinfo, ENTRYINDEX index)
{
	if ( index >= 0 ){	// INDEXDATA index
		if ( index >= cinfo->e.cellIMax ) index = 0;
		return &CEL(index);
	}else{				// CELLDATA index
		resetflag(index, B31);
		if ( index >= cinfo->e.cellDataMax ) index = 0;
		return &CELdata(index);
	}
}

// *diroption
ERRORCODE PPcDirOptionCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	int mode = DSMD_TEMP;
	TCHAR dset_path[CMDLINESIZE], *dset_path_a;
	ThSTRUCT PopupTbl;
	LOADSETTINGS_MOD ls;

	dset_path[0] = '\0';
	if ( SkipSpace(&param) == '-' ){ // 設定パスを取得
		mode = GetDirSettingOption(cinfo, &param, ITEMSETTING_DIROPT, dset_path);
		if ( mode < 0 ) return ERROR_INVALID_PARAMETER;
	}
	dset_path_a = (dset_path[0] == '/') ? dset_path + 1 : dset_path;

	ThInit(&PopupTbl);
	if ( SkipSpace(&param) == '\0' ){ // 指定が無いときはメニュー表示
		HMENU hPopupMenu;

		for (;;){
			DWORD makeindex, menuindex;
			ThFree(&PopupTbl);

			makeindex = IDW_INTERNALMIN;
			hPopupMenu = DirOptionMenu(cinfo, NULL, &makeindex, &PopupTbl, mode, dset_path_a);
			menuindex = PPcTrackPopupMenu(cinfo, hPopupMenu);
			if ( menuindex >= CRID_DIROPT ){
				mode = menuindex - CRID_DIROPT;
				continue;
			}
			if ( menuindex == 0 ) return ERROR_CANCELLED;
			GetMenuDataMacro2(param, &PopupTbl, menuindex - IDW_INTERNALMIN);
			if ( (param == NULL) || (*param == '\0') ) return ERROR_CANCELLED;
			if ( (*param == '*') && (*(param + 1) == 'd') ){ // *diroption をスキップ
				while ( (UTCHAR)*param > ' ' ) param++;

				break;
			}
			return PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL,
					param, NULL, 0);
		}
		if ( SkipSpace(&param) == '-' ){ // 設定パスを取得
			mode = GetDirSettingOption(cinfo, &param, ITEMSETTING_DIROPT, dset_path);
			if ( mode < 0 ) return ERROR_INVALID_PARAMETER;
		}
	}

	if ( mode == DSMD_THIS_PATH ){ // 設定パスを用意
		LoadSetting_FixThisPath(dset_path, cinfo->path);
	}else if ( mode == DSMD_THIS_BRANCH ){
		LoadSetting_FixThisBranch(dset_path, cinfo->path);
	}else if ( dset_path[0] == '\0' ){
		mode = DSMD_TEMP;
	}

	if ( mode == DSMD_TEMP ){ // 現在の設定内容を用意
		PresetLs(ls, cinfo->dset);
	}else{
		LoadLs(&ls, dset_path);
	}

	for(;;){ // 設定を反映
		int type, state;

		type = GetStringCommand(&param,
			T("CACHE\0") T("WATCH\0") T("ASYNC\0") T("EVERYSAVE\0") T("SAVETODISK\0") // 0-4 DSET_
			T("SLOW\0") // 5 cinfo->SlowMode
			T("CMD\0") T("CMDE\0") // 6-7 XC_dset cmd:
			T("INFOICON\0") T("ENTRYICON\0") ); // 8-9 infoicon / cellicon
		#define DIROPTTYPE_WATCH 1
		#define DIROPTTYPE_SAVETODISK 4
		#define DIROPTTYPE_SLOW 5
		#define DIROPTTYPE_CMD 6
		#define DIROPTTYPE_CMDEDIT 7
		#define DIROPTTYPE_INFOICON 8
		#define DIROPTTYPE_ENTRYICON 9
/*
			T("CACHE\0") T("WATCH\0") T("ASYNC\0") T("EVERYSAVE\0") T("SAVETODISK\0") T("REFRESHAFTER")// 0-5 DSET_
			T("SLOW\0") // 6 cinfo->SlowMode
			T("CMD\0") T("CMDE\0") // 7-8 XC_dset cmd:
			T("INFOICON\0") T("ENTRYICON\0") ); // 9-10 infoicon / cellicon
		#define DIROPTTYPE_WATCH 1
		#define DIROPTTYPE_REFRESHAFTER 5
		#define DIROPTTYPE_SLOW 6
		#define DIROPTTYPE_CMD 7
		#define DIROPTTYPE_CMDEDIT 8
		#define DIROPTTYPE_INFOICON 9
		#define DIROPTTYPE_ENTRYICON 10
*/
		#define DIROPTTYPE_ICONSETTING DIROPTTYPE_INFOICON

		if ( type < 0 ) break;
		if ( *param == ':' ) param++;
		if ( type <= DIROPTTYPE_SLOW ){
			state = GetStringCommand(&param, T("OFF\0") T("ON\0"));

			if ( type == DIROPTTYPE_SLOW ){ // slow
				if ( state < 0 ){
					cinfo->SlowMode = !cinfo->SlowMode;
				}else{
					cinfo->SlowMode = state;
				}
			}else{ // 各種 DSET_xxx
				if ( type == DIROPTTYPE_SAVETODISK ) state ^= 1; // on/off反転
//				if ( type == DIROPTTYPE_REFRESHAFTER ) state ^= 1; // on/off反転
				setflag(ls.cust.dset.deflags, (WORD)(B0 << type));
				switch ( state ){
					case 0:
						ls.cust.dset.flags = ls.cust.dset.flags & (WORD)~(B0 << type);
						break;
					case 1:
						ls.cust.dset.flags = ls.cust.dset.flags | (WORD)(B0 << type);
						break;
					default:
						ls.cust.dset.flags ^= (WORD)(B0 << type);
						break;
				}
				if ( type == DIROPTTYPE_WATCH ){ // WATCH
					if ( (cinfo->e.Dtype.ExtData == INVALID_HANDLE_VALUE) ||
						 (ls.cust.dset.flags & DSET_NODIRCHECK) ){
						setflag(cinfo->SubTCmdFlags, SUBT_STOPDIRCHECK);	// 監視しない
					}else{
						setflag(cinfo->SubTCmdFlags, SUBT_INITDIRCHECK);	// 監視開始
					}
					SetEvent(cinfo->SubT_cmd);
				}
			}
		}else if ( (type == DIROPTTYPE_CMD) || (type == DIROPTTYPE_CMDEDIT) ){ // CMD / CMDE
			TCHAR cmdbuf[LOADSETTINGS_CMDLEN];

			GetLineParamS(&param, cmdbuf, TSIZEOF(cmdbuf));
			if ( (type == DIROPTTYPE_CMDEDIT) &&
				 (PPctInput(cinfo, T("*diroption"), cmdbuf, LOADSETTINGS_CMDLEN, PPXH_COMMAND, PPXH_COMMAND) <= 0) ){
				break;
			}
			if ( mode != DSMD_TEMP ){
				SetNewXdir(dset_path, LOADCMDSTR, cmdbuf);
				mode = DSMD_TEMP; // 保存済みなので SaveLs を行わない
			}
		}else{ // >=DIROPTTYPE_ICONSETTING
			state = GetNumber(&param);
			if ( type == DIROPTTYPE_INFOICON ){
				ls.cust.dset.infoicon = (WORD)state;
			}else{ // DIROPTTYPE_ENTRYICON
				ls.cust.dset.cellicon = (WORD)state;
			}
		}

		if ( mode != DSMD_TEMP ) SaveLs(&ls, dset_path);
		cinfo->dset = ls.cust.dset;
		if ( type >= DIROPTTYPE_ICONSETTING ){
			ClearCellIconImage(cinfo);
			InitCli(cinfo);
			InvalidateRect(cinfo->info.hWnd, NULL, TRUE);
		}
	}
	FreeLs(&ls);
	ThFree(&PopupTbl);
	return NO_ERROR;
}

// *setmaskentry, *maskentry
ERRORCODE MaskEntryCommand(PPC_APPINFO *cinfo, const TCHAR *param, int mode)
{
	TCHAR path[VFPS], *initcmd = NULL;
	XC_MASK mask;
	BOOL dialog = FALSE, usedefmask = FALSE;

	mask.attr = 0;
	mask.file[0] = '\0';

	path[0] = '\0';
	while ( SkipSpace(&param) != '\0' ){
		if ( *param == '-' ){
			if ( tstrcmppart(param + 1, T("dialog"), 6) == 0 ){
				TCHAR buf[32];
				GetLineParamS(&param, buf, TSIZEOF(buf));
				dialog = TRUE;
				continue;
			}
			if ( param[1] == 'k' ){
				int len;

				len = tstrlen(param + 1);
				if ( len > 3 ){
					initcmd = PPcHeapAlloc(TSTROFF(len));
					if ( param[2] == ' ' ){
						tstrcpy(initcmd, param + 3);
						param = NilStr;
					}else if ( param[2] == ':' ){
						param += 3;
						GetLineParamS(&param, initcmd, TSTROFF(len));
					}
					dialog = TRUE;
				}
				continue;
			}
			mode = GetDirSettingOption(cinfo, &param, ITEMSETTING_MASK, path);
			if ( mode < 0 ){
				if ( initcmd != NULL ) PPcHeapFree(initcmd);
				return ERROR_INVALID_PARAMETER;
			}
		}else{
			if ( mask.file[0] != '\0' ) break;
			if ( *param == '\"' ){ // セパレータ有り
				GetCommandParameter(&param, mask.file, TSIZEOF(mask.file));
			}else{
				SetExtParam(param, mask.file, TSIZEOF(mask.file));
			}
			usedefmask = TRUE;
		}
	}

	if ( dialog || (SearchVLINEwild(mask.file) != NULL) ){
		return MaskEntry(cinfo, mode, mask.file, path, usedefmask, initcmd);
	}
	if ( initcmd != NULL ) PPcHeapFree(initcmd);

	// 設定を保存
	switch ( mode ){
		case DSMD_TEMP:
			break; // 保存不要
		case DSMD_NOMODE:
		case DSMD_REGID:
			cinfo->mask.attr = 0;
			tstrcpy(cinfo->mask.file, mask.file);
			SetCustTable(T("XC_mask"), cinfo->RegCID + 1,
					&cinfo->mask, TSTRSIZE(cinfo->mask.file) + 4);
			break;
		default: // DSMD_THIS_PATH, DSMD_THIS_BRANCH, DSMD_PATH_BRANCH, DSMD_ARCHIVE
			SetNewXdir(path, LOADMASKSTR, mask.file);
			break;
	}

//	if ( mode == DSMD_TEMP ){
		if ( MaskEntryMain(cinfo, &mask, CEL(cinfo->e.cellN).f.cFileName) == FALSE ){
			return ERROR_INVALID_PARAMETER;
		}
		tstrcpy(cinfo->DsetMask, mask.file);
		if ( XC_dpmk != 0 ){
			SetCaption(cinfo);
			InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
			if ( cinfo->combo ){
				PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
						KCW_setpath, (LPARAM)cinfo->info.hWnd);
			}
		}
#if 0 // 1.76+4以前は再読み込みで反映していた
	}else{
		if ( (mode == DSMD_NOMODE) || (mode == DSMD_REGID) ){
			cinfo->mask.attr = 0;
			tstrcpy(cinfo->mask.file, mask.file);
			SetCustTable(T("XC_mask"), cinfo->RegCID + 1,
					&cinfo->mask, TSTRSIZE(cinfo->mask.file) + 4);
		}else{ // DSMD_THIS_PATH, DSMD_THIS_BRANCH, DSMD_PATH_BRANCH, DSMD_ARCHIVE
			SetNewXdir(path, LOADMASKSTR, mask.file);
		}
		tstrcpy(cinfo->Jfname, CEL(cinfo->e.cellN).f.cFileName);
		read_entry(cinfo, RENTRY_JUMPNAME | RENTRY_SAVEOFF);
	}
#endif
	return NO_ERROR;
}

void Cmd_CursorMarkLast(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	ENTRYDATAOFFSET celln;

	if ( cinfo->e.markC == 0 ){
		uptr->num = 0;
		return;
	}
	celln = GetLastMarkCell(cinfo);
	if ( celln >= 0 ){
		MoveCellCsr(cinfo, celln - cinfo->e.cellN, NULL);
		uptr->num = (celln == cinfo->e.cellN) ? 1 : 0;
	}
}

void Cmd_GetComboTabName(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	TC_ITEM tie;

	ComboTabIndexInfo(cinfo, uptr);
	if ( (uptr->inums[0] < 0) || (uptr->inums[1] < 0) ){
		uptr->str[0] = '\0';
		return;
	}
	tie.mask = TCIF_TEXT;
	tie.pszText = uptr->str;
	tie.cchTextMax = CMDLINESIZE;
	if ( TabCtrl_GetItem(Combo.show[uptr->nums[1]].tab.hWnd, uptr->nums[00], &tie) == FALSE ){
		uptr->str[0] = '\0';
	}
}

void Cmd_ComboIdCount(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	if ( cinfo->combo == 0 ){
		uptr->nums[0] = 0;
	}else if ( uptr->nums[0] == 0 ){
		uptr->nums[0] = Combo.BaseCount;
	}else{
		int i, count = 0;

		for ( i = 0 ; i < Combo.BaseCount ; i++ ){
			if ( Combo.base[i].cinfo != NULL ) count++;
		}
		uptr->nums[0] = count;
	}
}

void Cmd_ComboWindowType(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	PPC_APPINFO *ainfo = GetInfoTabInfo(cinfo, uptr->inums);
	int r;
	HWND hCWnd;

	if ( ainfo == NULL ){
		uptr->nums[0] = (DWORD)-10;
		return;
	}
	hCWnd = ainfo->info.hWnd;
	r = (hCWnd == hComboFocusWnd) ? 0 : 2;
	if ( hCWnd == (HWND)SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
			KCW_getpairwnd, (LPARAM)hComboFocusWnd) ){
		r = 1;
	}
	if ( GetComboShowIndex(hCWnd) < 0 ) r = -r;
	uptr->nums[0] = r;
}

DWORD_PTR Cmd_GetComboPane(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	HWND hTargetWnd;
	const TCHAR *tmpp = uptr->str;

	hTargetWnd = GetPPxhWndFromID(&cinfo->info, &tmpp, NULL);
	return GetComboShowIndex(hTargetWnd);
}

void Cmd_EntryInfo(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	ThSTRUCT th;

	MakeFileInformation(cinfo, &th, GetCellData_HS(cinfo, uptr->entryinfo.index));
	uptr->entryinfo.result = (TCHAR *)th.bottom;
}

ERRORCODE PPcGetIInfo_Command(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	TCHAR *param;

	param = uptr->str + tstrlen(uptr->str) + 1;
	if ( *uptr->str <= 'M' ){ //------------------------------------------- A-M
		if ( !tstrcmp(uptr->str, T("APPTEXT")) ){
			PPcOCX(cinfo, param, OCX_APPTEXT);
		}else if ( !tstrcmp(uptr->str, T("APPVIEW")) ){
			PPcOCX(cinfo, param, OCX_APPVIEW);
		}else if ( !tstrcmp(uptr->str, T("AUTODRAGDROP")) ){
			return AutoDragDropCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("CAPTUREWINDOW")) ){
			return CaptureWindowCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("CACHE")) ){
			CacheCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("CHECKOFFMARK")) ){
			return CheckOffScreenMarkCommand(cinfo);
		}else if ( !tstrcmp(uptr->str, T("COLOR")) ){
			return SetTempColor(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("COMMENT")) ){
			return CommentCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("CLEARCHANGE")) ){
			ClearChangeState(cinfo);
		}else if ( !tstrcmp(uptr->str, T("COMPAREMARK")) ){
			return CompareMarkEntry(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("COUNTSIZE")) ){
			PPcCountSize(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("CLIPENTRY")) ){
			PPcClipEntry(cinfo, param, DROPEFFECT_COPY | DROPEFFECT_LINK);
		}else if ( !tstrcmp(uptr->str, T("CUTENTRY")) ){
			PPcClipEntry(cinfo, param, DROPEFFECT_MOVE);
		}else if ( !tstrcmp(uptr->str, T("DIROPTION")) ){
			return PPcDirOptionCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("DOCK")) ){
			PPcDockCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("EXECINARC")) ){
			OnArcPathMode(cinfo);
		}else if ( !tstrcmp(uptr->str, T("ELLIPSIS")) ){
			PPcEllipsisType(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("ENTRYTIP")) ){
			PPcEntryTip(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("IDCLOSE")) ){
			PPcIDClose(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("IE")) ){
			PPcOCX(cinfo, param, OCX_IE);
		}else if ( !tstrcmp(uptr->str, T("INSERTDIR")) ){
			PPcInsertDir(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("JUMPPATH")) ){
			PPcJumpPathCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("LAYOUT")) ){
			PPcLayoutCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("LOGWINDOW")) ){
			LogwindowCommand(param);
		}else if ( !tstrcmp(uptr->str, T("MAKELISTFILE")) ){
			MakeListFileCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("MASKPATH")) ){
			MaskPathCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("MASKENTRY")) ){
			return MaskEntryCommand(cinfo, param, DSMD_TEMP);
		}else if ( !tstrcmp(uptr->str, T("MARKENTRY")) ){
			return PPC_FindMark(cinfo, param, MARK_CHECK);
		}else{
			return ERROR_INVALID_FUNCTION;
		}
	}else{ //-------------------------------------------------------------- N-Z
		if ( !tstrcmp(uptr->str, T("PAIRRATE")) ){
			PairRateCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("PANE")) ){
			if ( cinfo->combo ){
				return SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND, TMAKEWPARAM(KCW_panecommand, GetComboBaseIndex(cinfo->info.hWnd)), (LPARAM)param);
			}
		}else if ( !tstrcmp(uptr->str, T("PPCFILE")) ){
			PPcFileCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("PPVOPTION")) ){
			PPvOptionCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("RANGE")) ){
			return RangeCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("SENDTO")) ){
			ExecSendTo(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("SETMASKENTRY")) ){
			return MaskEntryCommand(cinfo, param, DSMD_REGID);
		}else if ( !tstrcmp(uptr->str, T("SETSORTENTRY")) ){
			return SortEntryCommand(cinfo, param, DSMD_REGID);
		}else if ( !tstrcmp(uptr->str, T("SORTENTRY")) ){
			return SortEntryCommand(cinfo, param, DSMD_TEMP);
		}else if ( !tstrcmp(uptr->str, T("SYNCPROP")) ){
			PPcSyncProperties(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("SYNCPATH")) ){
			SetSyncPath(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("SETENTRYIMAGE")) ){
			SetEntryImage(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("TREE")) ){
			PPcTreeCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("UNPACK")) ){
			return UnpackCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("UNMARKENTRY")) ){
			return PPC_FindMark(cinfo, param, MARK_REMOVE);
		}else if ( !tstrcmp(uptr->str, T("VIEWSTYLE")) ){
			return ViewStyleCommand(cinfo, param);
		}else if ( !tstrcmp(uptr->str, T("WHEREIS")) ){
			return WhereIsCommand(cinfo, param, FALSE);
		}else if ( !tstrcmp(uptr->str, T("WHERE")) ){
			return WhereIsCommand(cinfo, param, TRUE);
		}else if ( !tstrcmp(uptr->str, T("WMP")) ){
			PPcOCX(cinfo, param, OCX_WMP);
		}else if ( !tstrcmp(uptr->str, T("ZOOM")) ){
			PPcZoom(cinfo, param);
		}else{
			return ERROR_INVALID_FUNCTION;
		}
	}
	return NO_ERROR; // コマンド実行
}

DWORD_PTR PPcGetIInfo_Function(PPC_APPINFO *cinfo, PPXAPPINFOUNION *uptr)
{
	if ( !tstrcmp(uptr->funcparam.param, T("COMMENT")) ){ // %*comment
		CommentFunction(cinfo, uptr);
	}else if ( !tstrcmp(uptr->funcparam.param, T("DIRHISTORY")) ){ // %*dirhistory
		DirHistFunction(cinfo, &uptr->funcparam);
	}else if ( !tstrcmp(uptr->funcparam.param, T("LINEMESSAGE")) ){ // %*linemessage
		TCHAR param = uptr->funcparam.optparam[0];

		if ( param == '-' ) param = uptr->funcparam.optparam[1];
		if ( param == 'f' ){
			thprintf(uptr->funcparam.dest, 20, T("%d"), cinfo->PopMsgFlag);
		}else if ( (param == 'r') || (cinfo->PopMsgFlag & PMF_DISPLAYMASK) ){
			tstrcpy(uptr->funcparam.dest, cinfo->PopMsgStr);
		}else{
			uptr->funcparam.dest[0] = '\0';
		}
	}else if ( !tstrcmp(uptr->funcparam.param, T("MASKENTRY")) ){ // %*maskentry
		tstrcpy(uptr->funcparam.dest, (cinfo->DsetMask[0] == MASK_NOUSE) ? cinfo->mask.file : cinfo->DsetMask);
	}else if ( !tstrcmp(uptr->funcparam.param, T("SORTENTRY")) ){ // %*sortentry
		GetSortString( (tstrchr(uptr->funcparam.optparam, 'd') != NULL) ?
				&cinfo->XC_sort : &cinfo->sort_last,
				uptr->funcparam.dest);
	}else if ( !tstrcmp(uptr->funcparam.param, T("VIEWSTYLE")) ){ // %*viewstyle
		uptr->funcparam.dest[0] = '\0';
		if ( cinfo->celF.fmtbase != NULL ){
			DWORD fmtsize = HeapSize(hProcessHeap, 0, cinfo->celF.fmtbase);
			SetCustTable(T("XC_celF"), T("?temp?"), cinfo->celF.fmtbase, fmtsize);
			PP_ExtractMacro(NULL, NULL, NULL, T("%*getcust(\"XC_celF:?temp?\")"), uptr->funcparam.dest, 0);
			DeleteCustTable(T("XC_celF"), T("?temp?"), 0);
		}
	}else{
		return PPXA_INVALID_FUNCTION;
	}
	return PPXA_NO_ERROR;
}

DWORD_PTR USECDECL PPcGetIInfo(PPC_APPINFO *cinfo, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	switch( cmdID ){
		case PPXCMDID_STARTENUM:	// 検索開始(マーク無しもあり)
			PPcAppInfo_AddRef(cinfo);
			if ( cinfo->e.markC == 0 ){
				if ( CEL(cinfo->e.cellN).state >= ECS_NORMAL ){
					uptr->enums.enumID = CELLENUMID_ONCURSOR;
				}else{				// SysMsgやDeletedなのでだめ
					uptr->enums.enumID = CELLENUMID_END;
				}
			}else{
				uptr->enums.enumID = cinfo->e.markTop;
				PPcGetIInfoEnumFix(cinfo, &uptr->enums.enumID);
			}
			break;

		case PPXCMDID_STARTNOENUM:	// 検索開始(マーク無しはなし)
			PPcAppInfo_AddRef(cinfo);
			if ( cinfo->e.markC == 0 ){
				uptr->enums.enumID = CELLENUMID_END;
			}else{
				uptr->enums.enumID = cinfo->e.markTop;
				PPcGetIInfoEnumFix(cinfo, &uptr->enums.enumID);
			}
			break;

		case PPXCMDID_NEXTENUM:		// 次へ
			if ( uptr->enums.enumID != CELLENUMID_ONCURSOR ){
				if ( uptr->enums.enumID >= 0 ){
					uptr->enums.enumID = CELdata(uptr->enums.enumID).mark_fw;
					PPcGetIInfoEnumFix(cinfo, &uptr->enums.enumID);
				}
				if ( uptr->enums.enumID == CELLENUMID_JUMPFIRST ){
					if ( cinfo->e.markC > 0 ){
						DWORD_PTR result;
						result = PPcGetIInfo(cinfo, PPXCMDID_STARTNOENUM, uptr);
								// PPXCMDID_STARTNOENUM で増えた参照を戻す
						PPcAppInfo_Release(cinfo);
						return result;
					}
				}
			}else{
				uptr->enums.enumID = CELLENUMID_END;
			}
			return ( uptr->enums.enumID >= 0 );

		case PPXCMDID_ENDENUM:	// 列挙終了
			uptr->enums.enumID = CELLENUMID_END;
			PPcAppInfo_Release(cinfo);
			break;

		case PPXCMDID_TRIMENUM:{	// 現在マークを削除して次のマークに移動できるようにする
			ENTRYCELL *cell;

			if ( uptr->enums.enumID < 0 ) break;

			cinfo->MarkMask = MARKMASK_DIRFILE;
			cell = &CELdata(uptr->enums.enumID);
			ResetMark(cinfo, cell);
			uptr->enums.enumID = CELLENUMID_JUMPFIRST;
//			RefleshCell(cinfo, index);
			RefleshInfoBox(cinfo, DE_ATTR_MARK);
			break;
		}
		case 'F':	// %F 処理
			GetInfoFormat(cinfo, uptr);
			break;

		case '0':	// %0
			CatPath(uptr->enums.buffer, PPcPath, NilStr);
			break;

		case '1':	// %1
			tstrcpy(uptr->enums.buffer, cinfo->RealPath);
			break;

		case '3':	// %3
			if ( cinfo->UseArcPathMask == ARCPATHMASK_DIRECTORY ){
				VFSFullPath(uptr->enums.buffer, cinfo->ArcPathMask,
						((cinfo->e.Dtype.mode == VFSDT_LFILE) &&
						 (cinfo->e.Dtype.BasePath[0] != '\0') ) ?
						cinfo->e.Dtype.BasePath : cinfo->path);
			}else{
				tstrcpy(uptr->enums.buffer, cinfo->path);
			}
			break;

		case '2':	// %2
			GetPairPath(cinfo, uptr->enums.buffer);
			if ( uptr->enums.buffer[0] == '?' ){
				GetPairVpath(cinfo, uptr->enums.buffer);
			}
			break;

		case 'C':	// %C
			if ( uptr->enums.enumID != CELLENUMID_ONCURSOR ){
				if ( uptr->enums.enumID < 0 ){
					*uptr->enums.buffer = '\0';
				}else{
					const TCHAR *cfilename;

					cfilename = GetCellFileName(cinfo, &CELdata(uptr->enums.enumID), uptr->enums.buffer);
					if ( IsTrue(cinfo->UnpackFix) ){
						cfilename = FindLastEntryPoint(cfilename);
					}
					tstrcpy(uptr->enums.buffer, cfilename);
				}
				break;
			}
			// 'R' へ
		case 'R': {
			const TCHAR *cfilename;

			cfilename = GetCellFileName(cinfo, &CEL(cinfo->e.cellN), uptr->enums.buffer);
			if ( IsTrue(cinfo->UnpackFix) ){
				cfilename = FindLastEntryPoint(cfilename);
			}
			tstrcpy(uptr->enums.buffer, cfilename);
			break;
		}
		case 'X':
			if ( uptr->enums.enumID != CELLENUMID_ONCURSOR ){
				if ( uptr->enums.enumID < 0 ){
					*uptr->enums.buffer = '\0';
				}else{
					tstrcpy(uptr->enums.buffer, CELdata(uptr->enums.enumID).f.cFileName);
					*(uptr->enums.buffer + CELdata(uptr->enums.enumID).ext) = '\0';
				}
				break;
			}
			// 'Y' へ
		case 'Y':
			tstrcpy(uptr->enums.buffer, CEL(cinfo->e.cellN).f.cFileName);
			*(uptr->enums.buffer + CEL(cinfo->e.cellN).ext) = '\0';
			break;

		case 'T':
			if ( uptr->enums.enumID != CELLENUMID_ONCURSOR ){
				if ( uptr->enums.enumID < 0 ){
					*uptr->enums.buffer = '\0';
				}else{
					TCHAR *extptr;

					extptr = CELdata(uptr->enums.enumID).f.cFileName +
							CELdata(uptr->enums.enumID).ext;
					if ( *extptr != '\0' ){
						tstrcpy(uptr->enums.buffer, extptr + 1);
					}else{
						*uptr->enums.buffer = '\0';
					}
				}
				break;
			}
			// 't' へ
		case 't': {
			TCHAR *extptr;

			extptr = CEL(cinfo->e.cellN).f.cFileName + CEL(cinfo->e.cellN).ext;
			if ( *extptr != '\0' ){
				tstrcpy(uptr->enums.buffer, extptr + 1);
			}else{
				*uptr->enums.buffer = '\0';
			}
			break;
		}
		case PPXCMDID_GETSUBID:
			tstrcpy(uptr->str, cinfo->RegSubCID);
			break;

		case PPXCMDID_COMBOWINDOW:
			uptr->dptrs[0] = (DWORD_PTR)cinfo->hComboWnd;
			break;

		case PPXCMDID_COMBOIDNAME:
			if ( cinfo->combo ){
				tstrcpy(uptr->str, ComboID);
			}else{
				uptr->str[0] = '\0';
			}
			break;

		case PPXCMDID_DIRMARKSIZE:
			uptr->unumhl = cinfo->e.MarkSize;
			break;

		case PPXCMDID_DRIVELABEL:
			GetDriveVolumeName(cinfo);
			tstrcpy(uptr->str, cinfo->VolumeLabel);
			break;

		case PPXCMDID_DRIVETOTALSIZE:
			uptr->unumhl = cinfo->DiskSizes.total;
			break;

		case PPXCMDID_DRIVEFREE:
			uptr->unumhl = cinfo->DiskSizes.free;
			break;

		case PPXCMDID_COMBOGROUPCOUNT:
			ComboTabGroup_GetCount(cinfo, uptr);
			break;

		case PPXCMDID_COMBOGROUPINDEX:
			ComboTabGroup_GetIndex(cinfo, uptr);
			break;

		case PPXCMDID_COMBOGROUPNAME:
			ComboTabGroup_GetName(cinfo, uptr);
			break;

		case PPXCMDID_POPUPPOS:
			GetPopupPosition(cinfo, (POINT *)uptr);
			break;

		case PPXCMDID_SETPOPUPPOS:
			cinfo->PopupPosType = PPT_SAVED;
			cinfo->PopupPos.x = uptr->nums[0];
			cinfo->PopupPos.y = uptr->nums[1];
			break;

		case PPXCMDID_ENUMFINDDATA:
			if ( uptr->enums.enumID != CELLENUMID_ONCURSOR ){
				if ( uptr->enums.enumID < 0 ){
					return 0;
				}else{
					memcpy(uptr->enums.buffer, &CELdata(uptr->enums.enumID).f, sizeof(WIN32_FIND_DATA));
				}
				break;
			}else{
				memcpy(uptr->enums.buffer, &CEL(cinfo->e.cellN).f, sizeof(WIN32_FIND_DATA));
			}
			break;

		case PPXCMDID_ENUMATTR:
			if ( uptr->enums.enumID != CELLENUMID_ONCURSOR ){
				if ( uptr->enums.enumID < 0 ){
					return 0;
				}else{
					*(DWORD *)uptr->enums.buffer = CELdata(uptr->enums.enumID).f.dwFileAttributes;
				}
				break;
			}else{
				*(DWORD *)uptr->enums.buffer = CEL(cinfo->e.cellN).f.dwFileAttributes;
			}
			break;

		case PPXCMDID_CSRATTR:
			uptr->num = CEL(cinfo->e.cellN).f.dwFileAttributes;
			break;

		case PPXCMDID_ENTRYATTR:
			uptr->num = GetCellData_HS(cinfo, uptr->num)->f.dwFileAttributes;
			break;

		case PPXCMDID_CSRMSIZE:
			uptr->nums[0] = CEL(cinfo->e.cellN).f.nFileSizeLow;
			uptr->nums[1] = CEL(cinfo->e.cellN).f.nFileSizeHigh;
			break;

		case PPXCMDID_GETREQHWND:
			if ( uptr != NULL ){
				switch ( uptr->str[0] ){
					case '\0':
						return (DWORD_PTR)cinfo->info.hWnd;
					case 'A': {
						PPXDOCKS *docks;
						if ( cinfo->combo ){
							if ( Combo.hAddressWnd != NULL ){
								return (DWORD_PTR)Combo.hAddressWnd;
							}
							docks = &Combo.Docks;
						}else{
							docks = &cinfo->docks;
						}
						return (docks->t.hAddrWnd != NULL) ? (DWORD_PTR)docks->t.hAddrWnd : (DWORD_PTR)docks->b.hAddrWnd;
					}
					case 'F':
						return (DWORD_PTR)cinfo->Tip.hTipWnd;
					case 'I':
						return (DWORD_PTR)cinfo->hSyncInfoWnd;
					case 'P':
						return (DWORD_PTR)hPropWnd;
					case 'R':
						return (hCommonLog != NULL) ? (DWORD_PTR)hCommonLog : (DWORD_PTR)Combo.Report.hWnd;
					case 'T':
						return (cinfo->hTreeWnd != NULL) ? (DWORD_PTR)cinfo->hTreeWnd : (DWORD_PTR)Combo.hTreeWnd;
				}
			}
			return 0;

		case PPXCMDID_ENTRYMSIZE: {
			ENTRYCELL *cell;

			cell = GetCellData_HS(cinfo, uptr->num);
			uptr->nums[0] = cell->f.nFileSizeLow;
			uptr->nums[1] = cell->f.nFileSizeHigh;
			break;
		}

		case PPXCMDID_ENTRYCTIME:
			uptr->ftime = GetCellData_HS(cinfo, uptr->num)->f.ftCreationTime;
			break;
		case PPXCMDID_ENTRYMTIME:
			uptr->ftime = GetCellData_HS(cinfo, uptr->num)->f.ftLastWriteTime;
			break;
		case PPXCMDID_ENTRYATIME:
			uptr->ftime = GetCellData_HS(cinfo, uptr->num)->f.ftLastAccessTime;
			break;

		case PPXCMDID_CSRINDEX:	uptr->num = cinfo->e.cellN;		break;

		case PPXCMDID_CSRRECT:
			((POINT *)uptr)->x = CalcCellX(cinfo, cinfo->e.cellN - cinfo->cellWMin);
			((POINT *)uptr)->y = CalcCellY(cinfo, cinfo->e.cellN - cinfo->cellWMin);
			uptr->nums[2] = cinfo->cel.Size.cx;
			uptr->nums[3] = cinfo->cel.Size.cy;
			break;

		case PPXCMDID_CSRDINDEX: uptr->num = cinfo->cellWMin;	break;
		case PPXCMDID_CSRSETDINDEX:
			MoveWinOff(cinfo, uptr->num - cinfo->cellWMin);
			break;


		case PPXCMDID_CSRDISPW:	uptr->num = cinfo->cel.VArea.cx;	break;
		case PPXCMDID_CSRDISPH:	uptr->num = cinfo->cel.VArea.cy;	break;

		case PPXCMDID_WINDOWDIR:
			uptr->num = PPcGetSite(cinfo);
			break;

		case PPXCMDID_ENTRYNAME:
			tstrcpy(uptr->indexpath.path, GetCellData_HS(cinfo, uptr->indexpath.index)->f.cFileName);
			break;
		case PPXCMDID_ENTRYANAME: {
			ENTRYCELL *cell;

			cell = GetCellData_HS(cinfo, uptr->indexpath.index);
			tstrcpy(uptr->indexpath.path, GetCellSfnName(cinfo, cell, TRUE));
			break;
		}
		case PPXCMDID_CSRMARK:
			uptr->num = IsCEL_Marked(cinfo->e.cellN) ? 1 : 0;
			break;

		case PPXCMDID_ENTRYMARK:
			uptr->num = (GetCellData_HS(cinfo, uptr->num)->mark_fw != NO_MARK_ID) ? 1 : 0;
			break;

		case PPXCMDID_CSRSTATE:
			uptr->num = CEL(cinfo->e.cellN).state | (CEL(cinfo->e.cellN).highlight * ECS_HLBIT);
			break;

		case PPXCMDID_ENTRYSTATE: {
			ENTRYCELL *cell;

			cell = GetCellData_HS(cinfo, uptr->num);
			uptr->num = cell->state | (cell->highlight * ECS_HLBIT);
			break;
		}
		case PPXCMDID_CSREXTCOLOR:
			uptr->color = CEL(cinfo->e.cellN).extC;
			break;

		case PPXCMDID_ENTRYEXTCOLOR:
			uptr->color = GetCellData_HS(cinfo, uptr->num)->extC;
			break;

		case PPXCMDID_CSRMARKFIRST:
			if ( cinfo->e.markC == 0 ){
				uptr->num = 0;
				break;
			}
			{
				int cn;
				cn = GetFirstMarkCell(cinfo);
				if ( cn >= 0 ){
					MoveCellCsr(cinfo, cn - cinfo->e.cellN, NULL);
					uptr->num = (cn == cinfo->e.cellN) ? 1 : 0;
				}
			}
			break;

		case PPXCMDID_CSRMARKNEXT: {
			ENTRYINDEX cn;
			ENTRYDATAOFFSET next;

			next = CEL(cinfo->e.cellN).mark_fw;
			if ( next < 0 ){
				uptr->num = 0;
				break;
			}

			for ( cn = 0 ; cn < cinfo->e.cellIMax ; cn++ ){
				if ( CELt(cn) == next ){
					MoveCellCsr(cinfo, cn - cinfo->e.cellN, NULL);
					uptr->num = (cn == cinfo->e.cellN) ? 1 : 0;
					break;
				}
			}
			break;
		}

		case PPXCMDID_CSRMARKPREV: {
			ENTRYINDEX cn;
			ENTRYDATAOFFSET prev;

			if ( !IsCEL_Marked(cinfo->e.cellN) ){
				uptr->num = 0;
				break;
			}
			prev = CEL(cinfo->e.cellN).mark_bk;
			if ( prev == ENDMARK_ID ){
				uptr->num = 0;
				break;
			}

			for ( cn = 0 ; cn < cinfo->e.cellIMax ; cn++ ){
				if ( CELt(cn) == prev ){
					MoveCellCsr(cinfo, cn - cinfo->e.cellN, NULL);
					uptr->num = (cn == cinfo->e.cellN) ? 1 : 0;
					break;
				}
			}
			break;
		}

		case PPXCMDID_SETCOMBOGROUPINDEX:
			ComboTabGroup_SetIndex(cinfo, uptr);
			break;

		case PPXCMDID_SETCOMBOGROUPNAME:
			ComboTabGroup_SetName(cinfo, uptr);
			break;

		case PPXCMDID_CSRMARKLAST:
			Cmd_CursorMarkLast(cinfo, uptr);
			break;

		case PPXCMDID_ENTRYMARKFIRST_HS:
			if ( cinfo->e.markC == 0 ){
				uptr->num = (DWORD)-1;
				break;
			}
			uptr->num = cinfo->e.markTop | B31;
			break;

		case PPXCMDID_ENTRYMARKFIRST:
			if ( cinfo->e.markC == 0 ){
				uptr->num = (DWORD)-1;
				break;
			}
			uptr->num = GetFirstMarkCell(cinfo);
			break;

		case PPXCMDID_ENTRYMARKNEXT_HS: {
			ENTRYINDEX cn;

			cn = uptr->num;
			if ( cn >= 0 ){	// INDEXDATA index
				if ( (cn >= cinfo->e.cellIMax) ){
					cn = -1;
				}else{
					cn = CEL(cn).mark_fw;
				}
			}else{			// CELLDATA index
				if ( cn >= CELLENUMID_ONCURSOR ) break;
				resetflag(cn, B31);
				if ( (cn >= cinfo->e.cellDataMax) ){
					cn = -1;
				}else{
					cn = CELdata(cn).mark_fw;
				}
			}
			if ( cn < 0 ) cn = -1;
			uptr->num = cn | B31;
			break;
		}

		case PPXCMDID_ENTRYMARKNEXT:
			uptr->num = GetNextMarkCell(cinfo, uptr->num);
			break;

		case PPXCMDID_ENTRYMARKPREV_HS: {
			ENTRYINDEX cn;

			cn = uptr->num;
			if ( cn >= 0 ){	// INDEXDATA index
				if ( (cn >= cinfo->e.cellIMax) || !IsCEL_Marked(cn) ){
					cn = -1;
				}else{
					cn = CEL(cn).mark_bk;
				}
			}else{			// CELLDATA index
				if ( cn >= CELLENUMID_ONCURSOR ) break;
				resetflag(cn, B31);
				if ( (cn >= cinfo->e.cellDataMax) || (CELdata(cn).mark_fw == NO_MARK_ID) ){
					cn = -1;
				}else{
					cn = CELdata(cn).mark_bk;
				}
			}
			if ( cn < 0 ) cn = NO_MARK_ID;
			uptr->num = cn | B31;
			break;
		}

		case PPXCMDID_ENTRYMARKLAST_HS:
			if ( cinfo->e.markC == 0 ){
				uptr->num = (DWORD)-1;
				break;
			}
			uptr->num = cinfo->e.markLast | B31;
			break;

		case PPXCMDID_ENTRYINSERTMSG:
			InsertEntry(cinfo, uptr->dptrs[0], (TCHAR *)uptr->dptrs[1], NULL);
			break;

		case PPXCMDID_ENTRYHIDEENTRY:
			HideEntry(cinfo, uptr->nums[0]);
			break;

		case PPXCMDID_CSRSETINDEX:
			MoveCellCsr(cinfo, uptr->num - cinfo->e.cellN, NULL);
			break;

		case PPXCMDID_CSRSETMARK:
			cinfo->MarkMask = MARKMASK_DIRFILE;
			CellMark(cinfo, cinfo->e.cellN, uptr->nums[0]);
			setflag(cinfo->DrawTargetFlags, DRAWT_ENTRY | DRAWT_STATUSLINE | DRAWT_INFOLINE);
			RefleshCell(cinfo, cinfo->e.cellN);
			RefleshInfoBox(cinfo, DE_ATTR_MARK);
			break;

		case PPXCMDID_ENTRYSETMARK: {
			int index;

			uptr->nums[1] = index = GetCellIndex_HS(cinfo, uptr->nums[1]);
			cinfo->MarkMask = MARKMASK_DIRFILE;
			CellMark(cinfo, index, uptr->nums[0]);
			setflag(cinfo->DrawTargetFlags, DRAWT_ENTRY | DRAWT_STATUSLINE | DRAWT_INFOLINE);
			RefleshCell(cinfo, index);
			RefleshInfoBox(cinfo, DE_ATTR_MARK);
			break;
		}
		case PPXCMDID_CSRSETSTATE:
			CEL(cinfo->e.cellN).state = (BYTE)(uptr->nums[0] & ECS_STATEMASK);
			CEL(cinfo->e.cellN).highlight = (BYTE)(uptr->nums[0] / ECS_HLBIT);
			RefleshCell(cinfo, cinfo->e.cellN);
			break;

		case PPXCMDID_ENTRYSETSTATE: {
			int index;

			uptr->nums[1] = index = GetCellIndex_HS(cinfo, uptr->nums[1]);
			CEL(index).state = (BYTE)(uptr->nums[0] & ECS_STATEMASK);
			CEL(index).highlight = (BYTE)(uptr->nums[0] / ECS_HLBIT);
			RefleshCell(cinfo, index);
			break;
		}
		case PPXCMDID_CSRSETEXTCOLOR:
			CEL(cinfo->e.cellN).extC = uptr->num;
			RefleshCell(cinfo, cinfo->e.cellN);
			break;

		case PPXCMDID_ENTRYSETEXTCOLOR: {
			int index;

			uptr->nums[1] = index = GetCellIndex_HS(cinfo, uptr->nums[1]);
			CEL(index).extC = uptr->nums[0];
			RefleshCell(cinfo, index);
			break;
		}
		case PPXCMDID_CSRCOMMENT:
			GetInfoEntryComment(cinfo, uptr, &CEL(cinfo->e.cellN));
			break;

		case PPXCMDID_ENTRYCOMMENT:
			GetInfoEntryComment(cinfo, uptr, GetCellData_HS(cinfo, uptr->num));
			break;

		case PPXCMDID_CSRSETCOMMENT:
			SetComment(cinfo, 0, &CEL(cinfo->e.cellN), uptr->str);
			Repaint(cinfo);
			break;

		case PPXCMDID_ENTRYSETCOMMENT:
			SetComment(cinfo, 0, GetCellData_HS(cinfo, uptr->nums[0]),
					(TCHAR *)&uptr->nums[1]);
			Repaint(cinfo);
			break;

		case PPXCMDID_ENTRYFROMNAME:
			return EntryFromName(cinfo, &uptr->entryinfo);

		case PPXCMDID_PPXCOMMAD:
			return PPcCommand(cinfo, uptr->key);

		case PPXCMDID_PAIRWINDOW:
			return (DWORD_PTR)GetPairWnd(cinfo);

		case PPXCMDID_DIRTOTAL:
			uptr->num = cinfo->e.cellDataMax;
			break;

		case PPXCMDID_DIRTTOTAL:
			uptr->num = cinfo->e.cellIMax;
			break;

		case PPXCMDID_DIRTTOTALFILE:
			uptr->num = cinfo->e.cellIMax -
					cinfo->e.Directories - cinfo->e.RelativeDirs;
			break;

		case PPXCMDID_DIRTTOTALDIR:
			uptr->num = cinfo->e.Directories;
			break;

		case PPXCMDID_DIRMARKS:
			uptr->num = cinfo->e.markC;
			break;

		case PPXCMDID_DIRLOADCOUNT:
			uptr->num = (uptr->num == 1) ? cinfo->ModifyCounter : cinfo->LoadCounter;
			break;

		case PPXCMDID_DIRTYPE:
			uptr->num = cinfo->e.Dtype.mode;
			break;

		case PPXCMDID_SYNCVIEW:
			uptr->num = (cinfo->SyncViewFlag & PPV_BOOTID) ?
					(cinfo->SyncViewFlag >> 24) :  cinfo->SyncViewFlag;
			break;

		case PPXCMDID_SETSYNCVIEW:
			SetSyncView(cinfo, uptr->num);
			break;

		case PPXCMDID_SLOWMODE:
			uptr->num = cinfo->SlowMode;
			break;

		case PPXCMDID_POINTINFO:
			uptr->nums[0] = cinfo->e.cellPointType;
			uptr->nums[1] = cinfo->e.cellPoint;
			uptr->nums[2] = 0; // reserved(Y指定?)
			break;

		case PPXCMDID_SETSLOWMODE:
			cinfo->SlowMode = uptr->num;
			break;

		case PPXCMDID_CHDIR: // %j
			PPcJumpDirMacro(cinfo, uptr->str);
			break;

		case PPXCMDID_PATHJUMP: // %J
			if ( FindPathSeparator(uptr->str) != NULL ){	// 階層指定あり
				JumpPathEntry(cinfo, uptr->str, RENTRY_JUMPNAME);
			}else{		// 階層指定なし→ジャンプ
				FindCell(cinfo, uptr->str);
			}
			break;

		case PPXCMDID_MOVECSR: // *cursor
			PPcMoveCsr(cinfo, (int *)uptr);
			break;

		case PPXCMDID_SETPOPLINE: {
			const TCHAR *text = uptr->str;
			ERRORCODE msgtype = POPMSG_MSG;

			if ( *text == '!' ){
				if ( (text[1] == 'K') && (text[2] == '\"') ){
					msgtype = POPMSG_KEEPMSG;
					text += 3;
				}else if ( (text[1] == 'L') && (text[2] == '\"') ){
					text += 3;
				}else if ( (text[1] == 'P') && (text[2] == '\"') ){
					msgtype = POPMSG_PROGRESSMSG;
					text += 3;
				}else if ( (text[1] == 'R') && (text[2] == '\"') ){
					setflag(cinfo->PopMsgFlag, PMF_WAITTIMER | PMF_WAITKEY);
					cinfo->PopMsgTimer = 3;
					RefleshStatusLine(cinfo);
					break;
				}else if ( text[1] == '\"' ){
					msgtype = POPMSG_NOLOGMSG;
					text += 2;
				}
			}
			if ( *text != '\0' ){
				SetPopMsg(cinfo, msgtype, text);
				break;
			}
			StopPopMsg(cinfo, (msgtype == POPMSG_KEEPMSG) ? PMF_KEEP : PMF_STOP);
			break;
		}

		case PPXCMDID_SETSTATLINE:
			SetStatline(cinfo, (DELETESTATUS *)uptr);
			break;

		case PPXCMDID_EXTREPORTTEXT:
			if ( // 独立窓で、ログ窓がないとき
				 (((cinfo->combo == 0) || (Combo.Report.hWnd == NULL)) &&
				 ((hCommonLog == NULL) || (IsWindow(hCommonLog) == FALSE))) ||
				// 一体化窓で、ログ窓がないとき
				 (X_combos[0] & CMBS_NOFORCEREPORT) ){
				if ( (uptr->str != NULL) && X_Logging ){
					XMessage(NULL, NULL, X_Logging, T("%s"), uptr->str);
				}
				return PPXCMDID_EXTREPORTTEXT_CLOSE;
			}
			if ( uptr->str != NULL ) SetReportText(uptr->str);
			return PPXCMDID_EXTREPORTTEXT_LOG;

		case PPXCMDID_REPORTTEXT:
			if ( uptr->str != NULL ){
				HWND hLogWnd;

				if ( hCommonLog != NULL ){
					if ( IsWindow(hCommonLog) ){
						hLogWnd = hCommonLog;
					}else{
						hLogWnd = hCommonLog = NULL;
					}
				}else{
					hLogWnd = Combo.Report.hWnd;
				}
				if ( hLogWnd != NULL ){
					SetReportTextMain(hLogWnd, uptr->str, TRUE);
				}else{
					SetReportText(uptr->str);
				}
			}
			return PPXCMDID_EXTREPORTTEXT_LOG;

		case PPXCMDID_ADDEXMENU:
			return PPcAddExMenu(cinfo, (ADDEXMENUINFO *)uptr);

		case PPXCMDID_EXECEXMENU:
			return PPcExecExMenu(cinfo, (EXECEXMENUINFO *)uptr);

		case PPXCMDID_GETINMENUBAR:
			return (DWORD_PTR)&ppcbar;

		case PPXCMDID_ENTRYINFO:
			Cmd_EntryInfo(cinfo, uptr);
			break;

		case PPXCMDID_ENTRY_HS_GETINDEX:
			uptr->num = GetCellIndex_HS(cinfo, uptr->num);
			break;

		case PPXCMDID_MENUONMENU:
			PPcCRMenuOnMenu(cinfo, (PPCMENUINFO *)uptr);
			break;

		case PPXCMDID_REQUIREKEYHOOK:
			cinfo->KeyHookEntry = FUNCCAST(CALLBACKMODULEENTRY, uptr);
			break;

		case PPXCMDID_COMBOITEMS:
			uptr->nums[0] = cinfo->combo ? Combo.BaseCount : 0;
			break;

		case PPXCMDID_COMBOSHOWPANES:
			uptr->nums[0] = cinfo->combo ? Combo.ShowCount : 0;
			break;

		case PPXCMDID_COMBOSHOWINDEX:
			uptr->nums[0] = (DWORD)GetInfoShowIndex(cinfo, uptr->nums[0]);
			break;

		case PPXCMDID_COMBOIDCOUNT:
			Cmd_ComboIdCount(cinfo, uptr);
			break;

		case PPXCMDID_COMBOGETPANE:
			return Cmd_GetComboPane(cinfo, uptr);

		case PPXCMDID_COMBOGETHWNDEX:
			return (DWORD_PTR)GetHwndFromIDCombo(uptr->str);

		case PPXCMDID_COMBOGETTAB:
			return ComboGetTabInfo(cinfo, uptr);

		case PPXCMDID_COMBOTABEXTRACT:
			ComboTabExtract(cinfo, uptr);
			break;

		case PPXCMDID_COMBOTABEXECUTE:
			ComboTabExecute(cinfo, uptr);
			break;

		case PPXCMDID_COMBOTABCOUNT: {
			int showindex = GetInfoShowIndex(cinfo, uptr->nums[0]);

			uptr->nums[0] = 0;
			if ( cinfo->combo == 0 ) break;
			if ( (showindex >= 0) && (showindex < Combo.ShowCount) && (Combo.Tabs >= 1) ){
				if ( Combo.Tabs < Combo.ShowCount ){
					uptr->nums[0] = TabCtrl_GetItemCount(
							GetTabWndForGetPane(
								Combo.base[Combo.show[showindex].baseNo].hWnd,
							 	showindex));
				}else{
					uptr->nums[0] = TabCtrl_GetItemCount(Combo.show[showindex].tab.hWnd);
				}
			}
			break;
		}

		case PPXCMDID_COMBOTABINDEX:
			ComboTabIndexInfo(cinfo, uptr);
			break;

		case PPXCMDID_TABTEXTCOLOR: {
			int baseindex = GetInfoTabBaseIndex(cinfo, uptr->inums);

			uptr->nums[0] = (baseindex < 0) ? (DWORD)-1 : Combo.base[baseindex].tabtextcolor;
			break;
		}

		case PPXCMDID_TABBACKCOLOR: {
			int baseindex = GetInfoTabBaseIndex(cinfo, uptr->inums);

			uptr->nums[0] = (baseindex < 0) ? (DWORD)-1 : Combo.base[baseindex].tabbackcolor;
			break;
		}

		case PPXCMDID_DIRLOCK: {
			PPC_APPINFO *ainfo = GetInfoTabInfo(cinfo, uptr->inums);

			uptr->nums[0] = (ainfo != NULL) ? ainfo->ChdirLock : -1;
			break;
		}

		case PPXCMDID_COMBOTABIDNAME:
		case PPXCMDID_COMBOTABLONGID: {
			PPC_APPINFO *ainfo = GetInfoTabInfo(cinfo, uptr->inums);

			if ( ainfo == NULL ){
				uptr->str[0] = '\0';
				break;
			}
			tstrcpy(uptr->str, (cmdID == PPXCMDID_COMBOTABIDNAME) ?
					ainfo->RegID : ainfo->RegSubCID);
			break;
		}

		case PPXCMDID_COMBOTABHWND: {
			PPC_APPINFO *ainfo = GetInfoTabInfo(cinfo, uptr->inums);
			return (ainfo == NULL) ? 0 : (DWORD_PTR)ainfo->info.hWnd;
		}

		case PPXCMDID_COMBOTABNAME:
			Cmd_GetComboTabName(cinfo, uptr);
			break;

		case PPXCMDID_COMBOWNDTYPE:
			Cmd_ComboWindowType(cinfo, uptr);
			break;

		case PPXCMDID_SETCOMBOWNDTYPE: {
			PPC_APPINFO *ainfo = GetInfoTabInfo(cinfo, uptr->inums);

			if ( ainfo == NULL ) break;
			if ( (int)uptr->nums[0] >= 0 ){
				SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
						TMAKEWPARAM(((uptr->nums[2] == 0) ? KCW_ActivateWnd : KCW_SelectWnd), uptr->nums[0]),
						(LPARAM)ainfo->info.hWnd);
			}else{
				SetFocus(ainfo->info.hWnd);
			}
			break;
		}
/*
		case PPXCMDID_SETTABNAME: {
			int baseindex = GetInfoTabBaseIndex(cinfo, uptr->inums);

			if ( baseindex < 0 ) break;
			SetTabtext(baseindex, uptr->tabidxstr.str);
			break;
		}
*/
		case PPXCMDID_SETTABTEXTCOLOR: {
			int baseindex = GetInfoTabBaseIndex(cinfo, uptr->inums);

			if ( baseindex < 0 ) break;
			Combo.base[baseindex].tabtextcolor = uptr->nums[2];
			SetTabColor(baseindex);
			break;
		}

		case PPXCMDID_SETTABBACKCOLOR: {
			int baseindex = GetInfoTabBaseIndex(cinfo, uptr->inums);

			if ( baseindex < 0 ) break;
			Combo.base[baseindex].tabbackcolor = uptr->nums[2];
			SetTabColor(baseindex);
			break;
		}

		case PPXCMDID_SETDIRLOCK: {
			PPC_APPINFO *ainfo = GetInfoTabInfo(cinfo, uptr->inums);

			if ( ainfo == NULL ) break;
			if ( uptr->nums[2] > 1 ){
				ainfo->ChdirLock = !ainfo->ChdirLock;
			}else{
				ainfo->ChdirLock = uptr->nums[2];
			}
			if ( ainfo->combo ){
				PostMessage(ainfo->hComboWnd, WM_PPXCOMMAND,
						KCW_setpath, (LPARAM)ainfo->info.hWnd);
			}
			break;
		}

		case PPXCMDID_COMBOTABPANE:
			ComboTabPaneInfo(cinfo, uptr);
			break;

		case PPXCMDID_ENTRYEXTDATA_GETID:
			EntryExtData_GetID(&uptr->num);
			break;

		case PPXCMDID_ENTRYEXTDATA_SETDATA:
			EntryExtData_SetDATA(cinfo, (ENTRYEXTDATASTRUCT *)uptr,
					GetCellData_HS(cinfo, ((ENTRYEXTDATASTRUCT *)uptr)->entry));
			break;

		case PPXCMDID_ENTRYEXTDATA_GETDATA:
			return EntryExtData_GetDATA(cinfo, (ENTRYEXTDATASTRUCT *)uptr,
					GetCellData_HS(cinfo, ((ENTRYEXTDATASTRUCT *)uptr)->entry));

		case PPXCMDID_GETWNDVARIABLESTRUCT:
			return (DWORD_PTR)&cinfo->StringVariable;

		case PPXCMDID_COMMAND:
			return PPXARESULT(PPcGetIInfo_Command(cinfo, uptr));

		case PPXCMDID_FUNCTION:
			return PPcGetIInfo_Function(cinfo, uptr);

		default:
			if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
			return PPXA_INVALID_FUNCTION;
	}
	return PPXA_NO_ERROR;
}
