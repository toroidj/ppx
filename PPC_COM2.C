/*-----------------------------------------------------------------------------
	Paper Plane cUI												コマンド処理
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPCOMBO.H"
#include "PPC_DD.H"
#include "sha.h"
#pragma hdrstop

#define DSET_SEPARATOR 1

const TCHAR Str_X_dlf[] = T("X_dlf");
const TCHAR Str_X_dsst[] = T("X_dsst");

const TCHAR StrDsetCancel[] = MES_DSCA;
const TCHAR StrDsetForce[] = MES_DSFO;
const TCHAR StrDsetDefault[] = MES_DSHO;
const TCHAR StrDsetThisPath[] = MES_DSTP;
const TCHAR StrDsetThisBranch[] = MES_DSTL;
const TCHAR StrDsetArchive[] = MES_DSTA;
const TCHAR StrDsetListfile[] = MES_DSTF;
const TCHAR StrDsetPathSeparate[] = MES_DSTS;
const TCHAR StrDsetTemporality[] = MES_DSTE;
const TCHAR StrDsetDirmode[] = MES_DSTD;
const TCHAR StrDsetEdit[] = MES_EDIT;
const TCHAR StrDsetSave[] = MES_SAVE;
const TCHAR StrDsetThumbMag[] = MES_DSMA;
const TCHAR StrDsetThumbMini[] = MES_DSMI;

const TCHAR StrDsetSlowMode[] = MES_DSSM;
const TCHAR StrDsetWatchDirModified[] = MES_DSWM;
const TCHAR StrDsetCacheView[] = MES_DSCV;
const TCHAR StrDsetAsyncRead[] = MES_DSAR;
const TCHAR StrDsetSaveDisk[] = MES_DSSD;
const TCHAR StrDsetSaveEveryTime[] = MES_DSSE;

const TCHAR cStrAuxBase[] = T("base");
const TCHAR cStrAuxSep[] = T(" %; ");
const TCHAR cStrRegTsClient[] = T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SessionInfo");
const TCHAR cStrRegTsTarget[] = T("Target");
const TCHAR cStrRegWslPath[] = T("SYSTEM\\CurrentControlSet\\Services\\P9NP\\NetworkProvider");
const TCHAR cStrRegWslTarget[] = T("TriggerStartPrefix");

const struct PopStringList SortPopStringList[] = {
	{DSET_SEPARATOR, NULL},
{CRID_SORT_TEMP, StrDsetTemporality},
{CRID_SORT_REGID, StrDsetForce},
{CRID_SORT_THIS_PATH, StrDsetThisPath},
{CRID_SORT_THIS_BRANCH, StrDsetThisBranch},
{0, NULL},
};

const struct PopStringList DispPopStringList1[] = {
	{CRID_VIEWFORMAT_CANCEL, StrDsetCancel},
{DSET_SEPARATOR, NULL},
{CRID_VIEWFORMAT_EDIT, StrDsetEdit},
{0, NULL},
};

const struct PopStringList DispPopStringList2[] = {
	{DSET_SEPARATOR, NULL},
{CRID_VIEWFORMAT_MAG_THUMBS, StrDsetThumbMag},
{CRID_VIEWFORMAT_MINI_THUMBS, StrDsetThumbMini},
{0, NULL},
};

const struct PopStringList DispPopStringList3[] = {
	{DSET_SEPARATOR, NULL},
{CRID_VIEWFORMAT_THIS_PATH, StrDsetThisPath},
{CRID_VIEWFORMAT_THIS_BRANCH, StrDsetThisBranch},
{CRID_VIEWFORMAT_REGID, StrDsetDefault},
{0, NULL},
};

const struct PopStringList DispDiroptStringList[] = {
	{DSET_SEPARATOR, NULL},
{CRID_DIROPT_TEMP, StrDsetTemporality},
{CRID_DIROPT_THIS_PATH, StrDsetThisPath},
{CRID_DIROPT_THIS_BRANCH, StrDsetThisBranch},
{0, NULL},
};

enum {
	CM_RENAME = 1, CM_CRC32, CM_MD5, CM_SHA1, CM_SHA224, CM_SHA256,
	CM_FTYPE, CM_HARDLINKS, CM_LINKEDPATH,
	CM_OWNER, CM_INFOTIP,
	// ここまで CommentsMenu で一括
	CM_CMPHASH, CM_CLEAR,
	CM_WINHASH, CM_WINHASH_MAX = 0xfff,
	CM_EXT = 0x1000, // 必ず最後

//	CM_MEMOEX0
};

const TCHAR *CommentsMenu[] = {
	MES_TCME,
	MES_TC32,
	MES_TCM5,
	MES_TCS1,
	MES_TC22,
	MES_TC25,
	MES_TCFC,
	MES_TCHL,
	MES_TCLP,
	MES_TCOW,
	MES_TCIT,
	NULL
};

const TCHAR Pastemode_Link[] = T("MakeShortCut");
const TCHAR StrInvokePasteLink[] = T("pastelink");
const TCHAR StrInvokePaste[] = T("paste");
const TCHAR StrInvokeExplorer1[] = T("E"); // Ver 4.90未満(Win98/4.0まで)
const TCHAR StrInvokeExplorer2[] = T("X"); // Ver 4.90以上(WinMe/2000～Vista)
const TCHAR StrInvokeExplorer3[] = T("P"); // Ver 6.1以上(Win7以降)
const TCHAR StrInvokeExplorer4[] = T("O"); // Win10以降

#define MAXSORTITEM 23
const BYTE DescendingSortTable[MAXSORTITEM + 1] = {
	8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 16, 18, 17, 19, 21, 20, 23, 22};

XC_SORT nosort_xc = {{{-1, -1, -1, 0x7f}}, FILEATTRMASK_DIR_FILES, SORTE_DEFAULT_VALUE};

struct DirOptionMenuDataStruct {
	const TCHAR *CommandName;
	const TCHAR *MenuItemName;
	const TCHAR *IconShowModeList[DSETI_OVLSINGLE + 2];
};

const TCHAR StrIcon_OverlaySafety[] = MES_DSIA;
const TCHAR StrIcon_Blank[] = MES_DSIB;
const TCHAR StrIcon_Frame[] = MES_DSIF;
const TCHAR StrIcon_OverlayNocache[] = MES_DSIH;
const TCHAR StrIcon_Info[] = MES_DSII;
const TCHAR StrIcon_Normal[] = MES_DSIN;
const TCHAR StrIcon_Overlay[] = MES_DSIO;
const TCHAR StrIcon_Simple[] = MES_DSIS;
const TCHAR StrIcon_Through[] = MES_DSIT;
const TCHAR StrIcon_NoShow[] = MES_DSIW;

const struct DirOptionMenuDataStruct DirOptionMenuData_Info =
	{ T("infoicon"), MES_DSIL,
		{ StrIcon_Through, StrIcon_NoShow, StrIcon_Blank, StrIcon_Frame,
		  StrIcon_Simple, StrIcon_Normal,
		  StrIcon_Overlay, StrIcon_OverlayNocache, StrIcon_OverlaySafety
	}
};

const struct DirOptionMenuDataStruct DirOptionMenuData_Entry =
	{ T("entryicon"), MES_DSEI,
		{ StrIcon_Through, StrIcon_Info, T("[?]"), NULL,
		  StrIcon_Simple, StrIcon_Normal,
		  StrIcon_Overlay, StrIcon_OverlayNocache, StrIcon_OverlaySafety
	}
};

LPARAM CommonCustTickID = 0; // K_Lcust 時に繰り返し再カスタマイズしないようにするためのID

//============================================= 表示ディレクトリ操作
// 「:」に移動し、希望カーソル位置を指定する
void FixRootEntryCursor(PPC_APPINFO *cinfo)
{
	switch ( cinfo->path[0] ){
		case '\\': {
			TCHAR *sep1, *sep2;

			sep1 = FindPathSeparator(cinfo->path + 2);
			if ( sep1 != NULL ){
				if ( *(sep1 + 1) != '\0' ){ // \\pcname\share
					sep2 = FindPathSeparator(sep1 + 1);
					if ( sep2 != NULL ){ // \\pcname\share\...
						*sep2 = '\0';
					}
					tstrcpy(cinfo->Jfname, cinfo->path);
					break;
				}
			}
			tstrcpy(cinfo->Jfname, T("\\\\"));
			break;
		}
		case '#':
			tstrcpy(cinfo->Jfname, T("#:"));
			break;
		default:
			thprintf(cinfo->Jfname, TSIZEOF(cinfo->Jfname), T("%c:"), cinfo->path[0]);
			break;
	}
	ChangePath(cinfo, T(":"), CHGPATH_SETABSPATH);
}

// ルートディレクトリへ移動する
void PPC_RootDir(PPC_APPINFO *cinfo)
{
	TCHAR *p, *q;
	int mode;
	TCHAR pathbuf[VFPS], newpath[VFPS];

	if ( TinyCheckCellEdit(cinfo) ) return;

	if ( cinfo->UnpackFix ) OffArcPathMode(cinfo);
	if ( cinfo->UseArcPathMask != ARCPATHMASK_OFF ){ // 書庫内
		if ( cinfo->ArcPathMaskDepth > cinfo->MinArcPathMaskDepth ){
			SetPPcDirPos(cinfo);
			while ( cinfo->ArcPathMaskDepth > cinfo->MinArcPathMaskDepth ){
				tstrcpy(pathbuf, cinfo->ArcPathMask);
				p = VFSFindLastEntry(cinfo->ArcPathMask);
				*p = '\0';
				cinfo->ArcPathMaskDepth--;
			}
							// 「..」にカーソルが当たるように調整
			cinfo->e.cellN = 0;
			MaskEntryMain(cinfo, &cinfo->mask, pathbuf);
			SetCaption(cinfo);
			return;
		}
	}
	tstrcpy(newpath, cinfo->path);
	p = VFSGetDriveType(newpath, &mode, NULL);
	if ( p == NULL ) return; // 既にroot

	if ( IsTrue(cinfo->ChdirLock) ){ // ロック
		if ( VFSFullPath(pathbuf, T("\\"), newpath) != NULL ){
			PPCuiWithPathForLock(cinfo, pathbuf);
			return;
		}
	}

	if ( IsTrue(cinfo->ModifyComment) ){
		WriteComment(cinfo, cinfo->CommentFile);
	}
	SavePPcDir(cinfo, FALSE);
	if ( cinfo->OrgPath[0] != '\0' ){ // 書庫内書庫／検索結果なら元のパスへ
		ChangePath(cinfo, cinfo->OrgPath, CHGPATH_SETABSPATH);
		cinfo->OrgPath[0] = '\0';
	}
	DxSetMotion(cinfo->DxDraw, DXMOTION_Root);

	#ifdef USESLASHPATH
	if ( (*p == '/') && (*(p + 1) != '\0') ){
		*(p + 1) = '\0';
		ChangePath(cinfo, newpath, CHGPATH_SETABSPATH);
		read_entry(cinfo, RENTRY_READ);
		return;
	}
	#endif
	if ( *p == '\\' ) p++;
	if ( *p ){
		if ( mode == VFSPT_UNC ){	// UNC は共有ルートを求める
			q = FindPathSeparator(p); // PC名をスキップ
			if ( q != NULL ){
				TCHAR *r;

				q++;
				r = FindPathSeparator(q);	// 共有名をスキップ
				if ( r != NULL ){
					*r = '\0';
					if ( *(r + 1) != '\0' ) p = r + 1;
				}
			}
		}
	} else{
		p = newpath;
	}

	if ( p == newpath ){ // 既にrootに移動済み
		if ( *p == ':' ) return;	// 「:」に移動済み
		ChangePath(cinfo, newpath, CHGPATH_SETABSPATH);
		FixRootEntryCursor(cinfo);
		read_entry(cinfo, RENTRY_JUMPNAME | RENTRY_JUMPNAME_INC);
	} else{
		q = FindPathSeparator(p);
		if ( q != NULL ) *q = '\0';

		tstrcpy(cinfo->Jfname, p);
		*p = '\0';
		ChangePath(cinfo, newpath, CHGPATH_SETABSPATH);
		read_entry(cinfo, RENTRY_JUMPNAME);
	}
}

//=============================================================================
// 親ディレクトリへ移動する
void PPC_UpDir(PPC_APPINFO *cinfo)
{
	TCHAR *p;
	TCHAR pathbuf[VFPS], newpath[VFPS];

	if ( TinyCheckCellEdit(cinfo) ) return;

	if ( cinfo->UnpackFix ) OffArcPathMode(cinfo);
	if ( cinfo->UseArcPathMask != ARCPATHMASK_OFF ){ // 書庫内

		cinfo->ArcPathMaskDepth--;
		if ( cinfo->ArcPathMaskDepth >= cinfo->MinArcPathMaskDepth ){
			TCHAR *last;

			SetPPcDirPos(cinfo);
			tstrcpy(pathbuf, cinfo->ArcPathMask);
			last = FindBothLastEntry(cinfo->ArcPathMask);
			if ( *last == '\0' ) last = cinfo->ArcPathMask;
			*last = '\0';
							// 「..」にカーソルが当たるように調整
			cinfo->e.cellN = 0;
			MaskEntryMain(cinfo, &cinfo->mask, pathbuf);
			SetCaption(cinfo);
			return;
		}
	}

	if ( IsTrue(cinfo->ChdirLock) ){ // ロック
		if ( VFSFullPath(pathbuf, T(".."), cinfo->path) != NULL ){
			PPCuiWithPathForLock(cinfo, pathbuf);
			return;
		}
	}

	if ( IsTrue(cinfo->ModifyComment) ){
		WriteComment(cinfo, cinfo->CommentFile);
	}
	SavePPcDir(cinfo, FALSE);
	if ( cinfo->OrgPath[0] != '\0' ){ // 書庫内書庫／検索結果なら元のパスへ
		ChangePath(cinfo, cinfo->OrgPath, CHGPATH_SETABSPATH);
		cinfo->OrgPath[0] = '\0';

		if ( cinfo->e.Dtype.mode == VFSDT_LFILE ){
			DxSetMotion(cinfo->DxDraw, DXMOTION_UpDir);
			read_entry(cinfo, RENTRY_READ);
			return;
		}
	}

	tstrcpy(newpath, cinfo->path);
	if ( tstrchr(newpath, '/') != NULL ){ // http:// など
		p = VFSGetDriveType(newpath, NULL, NULL);
		if ( p == NULL ) return; // 既にroot
		if ( *p == '/' ) p++;
		if ( *p == '\0' ){ // root→drive list
			if ( !(GetCustXDword(Str_X_dlf, NULL, 0) & XDLF_UPDIRJUMP) ){
				tstrcpy(cinfo->Jfname, newpath);
				ChangePath(cinfo, T(":"), CHGPATH_SETABSPATH);
			} else{
				return;
			}
		} else{ // subdir
			for ( ; ; ){
				TCHAR *q;

				q = tstrchr(p, '/');
				if ( q == NULL ){
					tstrcpy(cinfo->Jfname, p);
					if ( p > (newpath + 1) ) p--;
					*p = '\0';
					break;
				}
				// http の時は ～/dir/ の形式にする
				if ( (cinfo->e.Dtype.mode == VFSDT_HTTP) && (*(q + 1) == '\0') ){
					*p = '\0';
					break;
				}
				p = q + 1;
			}
		}
	} else{ // GNC等
		p = VFSFindLastEntry(newpath);
		if ( (*p != '\0') &&
			(((GetDriveRoot(newpath) - 1) <= p) || !(GetCustXDword(Str_X_dlf, NULL, 0) & XDLF_UPDIRJUMP)) ){ // subdir
			tstrcpy(cinfo->Jfname, p + (*p == '\\'));
			*p = '\0';
		} else{ // root
			if ( (newpath[0] != ':') &&
				!(GetCustXDword(Str_X_dlf, NULL, 0) & XDLF_UPDIRJUMP) ){
				ChangePath(cinfo, newpath, CHGPATH_SETABSPATH);
				FixRootEntryCursor(cinfo);
				read_entry(cinfo, RENTRY_JUMPNAME | RENTRY_JUMPNAME_INC);
			}
			return;
		}
	}
	DxSetMotion(cinfo->DxDraw, DXMOTION_UpDir);
	ChangePath(cinfo, newpath, CHGPATH_SETABSPATH);
	read_entry(cinfo, RENTRY_JUMPNAME);
}

BOOL USEFASTCALL CheckComma(const TCHAR **param)
{
	if ( SkipSpace(param) == ',' ){
		(*param)++;
		return TRUE;
	}
	return FALSE;
}

BOOL LoadLs(LOADSETTINGS_MOD *ls, const TCHAR *path)
{
	ls->cust.dset.flags = ls->cust.dset.deflags = 0;
	ls->cust.dset.infoicon = ls->cust.dset.cellicon = DSETI_DEFAULT;
	ls->cust.dset.sort.mode.block = -1;
	ls->cust.dset.sort.atr = FILEATTRMASK_DIR_FILES;
	ls->cust.dset.sort.option = SORTE_DEFAULT_VALUE;
	ls->cust.buf[0] = '\0';
	ls->buf = (LOADSETTINGS_CUST *)GetCustValue(StrXC_dset, path, &ls->cust, sizeof(ls->cust));

	if ( ls->buf == NULL ){
		ls->buf = &ls->cust;
		return FALSE;
	}

	if ( ls->buf != &ls->cust ){ // 別バッファを取ったときは、別バッファから反映
		ls->cust.dset = ls->buf->dset;
	}
	return TRUE;
}

void SaveLs(LOADSETTINGS_MOD *ls, const TCHAR *path)
{
	// 何か変更点がある設定があれば保存する
	if ( ls->cust.dset.flags || ls->cust.dset.deflags ||
		(ls->cust.dset.infoicon != DSETI_DEFAULT) ||
		(ls->cust.dset.cellicon != DSETI_DEFAULT) ||
		(ls->cust.dset.sort.mode.dat[0] >= 0) ||
		(ls->buf->buf[0] != '\0') ){
		if ( ls->buf == NULL ){
			ls->buf = &ls->cust;
		}else if ( ls->buf != &ls->cust ){ // 別バッファに反映
			ls->buf->dset = ls->cust.dset;
		}
		SetCustTable(StrXC_dset, path, ls->buf, sizeof(XC_DSET) + TSTRSIZE(ls->buf->buf));
	} else{
		DeleteCustTable(StrXC_dset, path, 0);
	}
}

void FreeLs(LOADSETTINGS_MOD *ls)
{
	if ( (ls->buf != &ls->cust ) && (ls->buf != NULL) ){
		ProcHeapFree(ls->buf);
	}
}

int tstrlenAddQ(const TCHAR *param)
{
	int count = 0;
	for (;;){
		TCHAR chr;

		chr = *param++;
		if ( chr == '\0' ) return count;
		if ( chr == '\"' ) count++;
		count++;
	}
}

void SetNewXdir(const TCHAR *path, const TCHAR *header, const TCHAR *param)
{
	TCHAR *dstp;
	const TCHAR *srcp;
	size_t headsize;
	size_t oldbufsize;
	LOADSETTINGS_MOD ls;
	LOADSETTINGS_CUST *newbuf;

	if ( path == NULL ) return; // MaskEntry, path = NULL のとき用

	LoadLs(&ls, path);

	headsize = TSTRLENGTH(header);
	oldbufsize = (ls.buf != &ls.cust) ? HeapSize(hProcessHeap, 0, ls.buf) : 0;
	newbuf = (LOADSETTINGS_CUST *)HeapAlloc(hProcessHeap, 0, oldbufsize + headsize + (tstrlenAddQ(param) + 8) * sizeof(TCHAR)); // 8 は、空白区切り、" x 2分
	if ( newbuf == NULL ){
		FreeLs(&ls);
		return;
	}
	srcp = ls.buf->buf;
	dstp = newbuf->buf;

	while ( SkipSpace(&srcp) != '\0' ){
		TCHAR *headtop;

		headtop = dstp;
		for ( ;; ){ // 1項目分(header:"～")を保存
			UTCHAR chr;

			chr = (UTCHAR)*srcp++;
			if ( chr <= ' ' ){ // 区切り
				if ( chr == '\0' ) srcp--;
				*dstp++ = ' ';
				break;
			}
			*dstp++ = chr;
			if ( chr != '\"' ) continue;
			// 「"」処理
			for ( ;; ){
				chr = (UTCHAR)*srcp++;
				if ( chr < ' ' ){ // 閉じないまま末端
					if ( chr == '\0' ) srcp--;
					*dstp++ = '\"';
					*dstp++ = ' ';
					break;
				}
				*dstp++ = chr;
				if ( chr != '\"' ) continue;
				break;
			}
		}

		if ( memcmp(headtop, header, headsize) == 0 ){ // 同名項目の場合は破棄
			dstp = headtop;
		}
	}

	if ( param[0] != '\0' ){ // 新項目を追加
		dstp = tstpcpy(dstp, header);
		*dstp++ = '\"';
		for ( ;; ){
			UTCHAR chr;

			chr = (UTCHAR)*param++;
			if ( chr == '\0' ) break;
			*dstp++ = chr;
			if ( chr != '\"' ) continue;
			*dstp++ = chr;
		}
		*dstp++ = '\"';
	}

	*dstp = '\0';
	FreeLs(&ls); // 旧ls->buf を破棄
	ls.buf = newbuf;
	SaveLs(&ls, path);
	FreeLs(&ls);
}

//-----------------------------------------------------------------------------
BOOL AppendMenuPopString(HMENU hMenu, const struct PopStringList *list)
{
	BOOL settab;
	TCHAR buf[VFPS];
	const TCHAR *str;

	buf[0] = '\0';
	GetMenuString(hMenu, 0, buf, VFPS, MF_BYPOSITION);
	settab = (tstrchr(buf, '\t') != NULL) ? TRUE : FALSE;
	while ( list->id ){
		if ( list->id == DSET_SEPARATOR ){
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		} else{
			str = MessageText(list->string);
			if ( settab ){
				buf[0] = '\t';
				tstrcpy(buf + 1, str);
				str = buf;
			}
			AppendMenuString(hMenu, list->id, str);
		}
		list++;
	}
	return settab;
}

void GetSetPath(PPC_APPINFO *cinfo, TCHAR *setpath, int mode, int offset, HMENU hMenu)
{
	if ( mode == (offset + DSMD_THIS_PATH) ){
		LoadSetting_FixThisPath(setpath, cinfo->path);
	} else if ( mode == (offset + DSMD_THIS_BRANCH) ){
		LoadSetting_FixThisBranch(setpath, cinfo->path);
	} else if ( mode == (offset + DSMD_ARCHIVE) ){
		tstrcpy(setpath, StrArchiveMode);
	} else if ( mode == (offset + DSMD_LISTFILE) ){
		tstrcpy(setpath, StrListfileMode);
	} else{	// DSMD_PATHL
		GetMenuString(hMenu, offset + DSMD_PATH_BRANCH, setpath, VFPS, MF_BYCOMMAND);
		if ( setpath[0] == '\t' ){
			memmove(setpath, (BYTE *)(TCHAR *)(setpath + 1), TSTROFF(tstrlen(setpath) + 1 - 1));
		}
	}
}

void LoadSortOpt(LPCTSTR *param, XC_SORT *xc)
{
	const TCHAR *p;
	int i = 1;

	p = *param;
	xc->mode.block = -1;	// 0-3 をまとめて -1
	if ( SkipSpace(&p) == '\0' ){ // 指定無し
		xc->atr = xc->option = 0;
		return;
	}

	xc->mode.dat[0] = (char)GetNumber(&p);
	CheckComma(&p);
	xc->atr = GetNumber(&p);
	CheckComma(&p);
	xc->option = GetNumber(&p);
	while ( IsTrue(CheckComma(&p)) ){
		xc->mode.dat[i] = (char)xc->atr;
		xc->atr = xc->option;
		xc->option = GetNumber(&p);
		i++;
		if ( i >= 4 ) break;
	}
	*param = p;
}

// 現在使用中のソート項目をチェック
void SortMenuCheck(HMENU hMenu, const TCHAR *str, int indexmax, XC_SORT *xc, int descending_sort)
{
	MENUITEMINFO minfo;
	int index = CRID_SORT, checkindex = 0, detailindex = 0;
	XC_SORT des_xc;
	XC_SORT tmpxc;

	if ( descending_sort ){ // 降順指定時なら降順化した内容を用意する
		des_xc = *xc;
		if ( (des_xc.mode.dat[0] >= 0) && (des_xc.mode.dat[0] <= MAXSORTITEM) ){
			des_xc.mode.dat[0] = DescendingSortTable[des_xc.mode.dat[0]];
		}
	}
	minfo.cbSize = sizeof(minfo);

	for ( ; index < indexmax; str += tstrlen(str) + 1, index++ ){
		if ( *str == '\0' ){
			detailindex = index;
			continue;
		}
		if ( *str == MENUNAMEID_SIG ){
			str += TSIZEOF(MENUNAMEID);
			index--;
			continue;
		}
		if ( (*str == 'a') || (*str == 'd') ){ // ascending / descending指定
			minfo.fMask = MIIM_STATE | MIIM_ID;
			minfo.fState = descending_sort ? (MFS_ENABLED | MFS_CHECKED) : MFS_ENABLED;
			minfo.wID = CRID_SORT_DESCENDING;
			if ( *str == 'a' ){
				minfo.fState ^= MFS_CHECKED;
				minfo.wID = CRID_SORT_ASCENDING;
			}
			// ↓ID 変更後はここで調整。
			SetMenuItemInfo(hMenu, minfo.wID, MF_BYCOMMAND, &minfo);
			// ID 変更前は後ろのSetMenuItemInfoで調整。
		} else{ // 通常のソート指定
			minfo.fMask = MIIM_STATE;
			LoadSortOpt(&str, &tmpxc);
			if ( (memcmp(xc, &tmpxc, sizeof(XC_SORT)) == 0) ||
				(descending_sort && !memcmp(&des_xc, &tmpxc, sizeof(XC_SORT))) ){
				checkindex = index;
				minfo.fState = MFS_ENABLED | MFS_CHECKED;
			} else{
				minfo.fState = MFS_ENABLED;
			}
		}
		SetMenuItemInfo(hMenu, index, MF_BYCOMMAND, &minfo);
	}
	if ( (checkindex == 0) && (xc->mode.dat[0] >= 0) && (detailindex != 0) ){
		minfo.fMask = MIIM_STATE;
		minfo.fState = MFS_ENABLED | MFS_CHECKED;

		SetMenuItemInfo(hMenu, detailindex, MF_BYCOMMAND, &minfo);
	}
}

int FindSortSetting(PPC_APPINFO *cinfo, TCHAR *findpath)
{
	TCHAR path[VFPS], *p;
	LOADSETTINGS_TMP ls;
	int mode = CRID_SORT_NOMODE;

	// 全指定
	ls.dset.sort.mode.dat[0] = -1;
	LoadSettingMain(&ls, T("*"));
	if ( ls.dset.sort.mode.dat[0] >= 0 ){
		tstrcpy(findpath, T("*"));
		mode = CRID_SORT_PATH_BRANCH;
	}

	// ドライブ限定
	tstrcpy(path, cinfo->path);
	p = VFSGetDriveType(path, NULL, NULL);
	if ( p != NULL ){
		TCHAR backup;

		if ( *p == '\\' ) p++;
		backup = *p;
		*p = '\0';
		ls.dset.sort.mode.dat[0] = -1;
		LoadSettingMain(&ls, path);
		if ( ls.dset.sort.mode.dat[0] >= 0 ){
			tstrcpy(findpath, path);
			mode = CRID_SORT_PATH_BRANCH;
		}
		*p = backup;
	} else{
		p = path;
	}
	// 下層パス
	if ( *p != '\0' ){
		for ( ; ; ){	// 途中
			TCHAR backup, *q;

			q = FindPathSeparator(p);
			if ( q == NULL ) break;
			p = q + 1;
			backup = *p;
			*p = '\0';
			ls.dset.sort.mode.dat[0] = -1;
			LoadSettingMain(&ls, path);
			if ( ls.dset.sort.mode.dat[0] >= 0 ){
				tstrcpy(findpath, path);
				mode = CRID_SORT_PATH_BRANCH;
			}
			*p = backup;
		}
		// 最終層の更に下層指定
		p += tstrlen(p);
		*p = '\\';
		*(p + 1) = '\0';

		ls.dset.sort.mode.dat[0] = -1;
		LoadSettingMain(&ls, path);
		if ( ls.dset.sort.mode.dat[0] >= 0 ){
			tstrcpy(findpath, path);
			mode = CRID_SORT_THIS_BRANCH;
		}
		// 最終層
		*p = '\0';
		ls.dset.sort.mode.dat[0] = -1;
		LoadSettingMain(&ls, path);
		if ( ls.dset.sort.mode.dat[0] >= 0 ){
			tstrcpy(findpath, path);
			mode = CRID_SORT_THIS_PATH;
		}
	}

	// Archive
	if ( (cinfo->e.Dtype.mode == VFSDT_UN) ||
		(cinfo->e.Dtype.mode == VFSDT_SUSIE) ||
		(cinfo->e.Dtype.mode == VFSDT_ARCFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_CABFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_LZHFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_ZIPFOLDER) ){
		ls.dset.sort.mode.dat[0] = -1;
		LoadSettingMain(&ls, StrArchiveMode);
		if ( ls.dset.sort.mode.dat[0] >= 0 ){
			tstrcpy(findpath, StrArchiveMode);
			mode = CRID_SORT_ARCHIVE;
		}
	}
	// Listfile
	if ( cinfo->e.Dtype.mode == VFSDT_LFILE ){
		ls.dset.sort.mode.dat[0] = -1;
		LoadSettingMain(&ls, StrListfileMode);
		if ( ls.dset.sort.mode.dat[0] >= 0 ){
			tstrcpy(findpath, StrListfileMode);
			mode = CRID_SORT_LISTFILE;
		}
	}
	return mode;
}

HMENU AddPPcSortMenu(PPC_APPINFO *cinfo, HMENU hMenu, ThSTRUCT *PopupTbl, DWORD *mmmode, DWORD *sid, DWORD sortmode, const TCHAR *path)
{
	DWORD id = CRID_SORT;
	BOOL settab;
	TCHAR setpath[VFPS];
	DWORD newmmmode;

	hMenu = PP_AddMenu(&cinfo->info,
		cinfo->info.hWnd, hMenu, &id, T("MC_sort"), PopupTbl);
	if ( hMenu == NULL ) return NULL;

	setpath[0] = '\t';
	newmmmode = FindSortSetting(cinfo, setpath + 1);
	if ( newmmmode == CRID_SORT_NOMODE ){
		GetCustData(Str_X_dsst, &X_dsst, sizeof(X_dsst));
		newmmmode = X_dsst[0] + CRID_SORTEX;
	}
	settab = AppendMenuPopString(hMenu, SortPopStringList);
	if ( newmmmode == CRID_SORT_PATH_BRANCH ){
		if ( (sortmode == CRID_SORT_PATH_BRANCH) && (path != NULL) && (path[0] != '\0') ){
			tstrcpy(setpath, path);
		}
	}else{
		setpath[1] = '*';
		setpath[2] = '\0';
	}
	AppendMenuString(hMenu, CRID_SORT_PATH_BRANCH, settab ? setpath : setpath + 1);
	if ( (cinfo->e.Dtype.mode == VFSDT_LFILE) || (sortmode == CRID_SORT_LISTFILE) ){
		AppendMenuString(hMenu, CRID_SORT_LISTFILE, StrDsetListfile);
	}
	if ( (cinfo->e.Dtype.mode == VFSDT_UN) ||
		(cinfo->e.Dtype.mode == VFSDT_SUSIE) ||
		(cinfo->e.Dtype.mode == VFSDT_ARCFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_CABFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_LZHFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_ZIPFOLDER) ||
		(sortmode == CRID_SORT_ARCHIVE) ){
		tstrcpy(settab ? setpath + 1 : setpath, MessageText(StrDsetArchive));
		AppendMenuString(hMenu, CRID_SORT_ARCHIVE, setpath);
	}

	if ( sortmode != 0 ){ // \[S] 保持設定が有効なら強制設定にする
		if ( sortmode >= CRID_SORTEX ){
			newmmmode = sortmode;
		} else if ( cinfo->XC_sort.mode.dat[0] >= 0 ){
			newmmmode = CRID_SORT_REGID;
		}
	} else{		// [S] 一時設定
		newmmmode = CRID_SORTEX + X_dsst[1];
	}
	CheckMenuRadioItem(hMenu, CRID_SORT_TEMP, CRID_SORT_MODELAST - 1, newmmmode, MF_BYCOMMAND);
	if ( mmmode != NULL ){
		*mmmode = newmmmode;
		*sid = id;
	}
	return hMenu;
}

void SaveSortSetting(PPC_APPINFO *cinfo, int mode, const TCHAR *path, XC_SORT *xc)
{
	switch ( mode ){
		case CRID_SORT_TEMP:	// 一時設定
			if ( xc->mode.dat[0] == -1 ){
				xc->mode.block = 0xffffff00 + 19;	// まとめて -1
				xc->atr = 0;
				xc->option = 0;
			}
			break;
		case CRID_SORT_NOMODE:
		case CRID_SORT_REGID: // 強制設定
			cinfo->XC_sort = *xc;
			SetCustTable(T("XC_sort"), cinfo->RegCID + 1, xc, sizeof(XC_SORT));
			break;

//		case CRID_SORT_THIS_PATH:	// パス設定
//		case CRID_SORT_THIS_BRANCH:	// 下層設定
//		case CRID_SORT_PATH_BRANCH:
//		case CRID_SORT_ARCHIVE:
		default:
			if ( cinfo->XC_sort.mode.dat[0] >= 0 ){
				// 保持ソートが使われていたら、解除する
				cinfo->XC_sort.mode.dat[0] = -1;
				SetCustTable(T("XC_sort"), cinfo->RegCID + 1, &cinfo->XC_sort, sizeof(XC_SORT));
			}
			// setpath は、PPcTrackPopupMenu の前で準備済み
			{
				LOADSETTINGS_MOD ls;

				LoadLs(&ls, path);
				ls.cust.dset.sort = *xc;
				SaveLs(&ls, path);
				FreeLs(&ls);
			}
			break;
	}
}


BOOL PPC_SortMenu(PPC_APPINFO *cinfo, XC_SORT *xc, DWORD sortmode, const TCHAR *path)
{
	BOOL result = FALSE;
	DWORD id;
	DWORD index, mmmode;
	const TCHAR *p;
	HMENU hMenu;
	ThSTRUCT PopupTbl;		// 処理内容
	TCHAR setpath[VFPS];
	BOOL descending_sort = FALSE;
	XC_SORT tmpxc;

	if ( TinyCheckCellEdit(cinfo) ) return FALSE;
	ThInit(&PopupTbl);

	if ( sortmode == CRID_SORT_DESCENDING ) descending_sort = 1;
	hMenu = AddPPcSortMenu(cinfo, NULL, &PopupTbl, &mmmode, &id, sortmode, path);
	if ( hMenu != NULL ){
		for ( ; ; ){
			switch ( mmmode ){ // モード切替
				case CRID_SORT_TEMP:
					tmpxc = nosort_xc;
					break;
				case CRID_SORT_REGID:
					tmpxc = cinfo->XC_sort;
					break;
				case CRID_SORT_THIS_PATH:
				case CRID_SORT_THIS_BRANCH:
				case CRID_SORT_PATH_BRANCH:
				case CRID_SORT_ARCHIVE:
				case CRID_SORT_LISTFILE: {
					LOADSETTINGS_MOD ls;

					GetSetPath(cinfo, setpath, mmmode, CRID_SORTEX, hMenu);
					LoadLs(&ls, setpath);
					tmpxc = ls.cust.dset.sort;
					FreeLs(&ls);
					break;
				}
				default:
					tmpxc = *xc;
					break;
			}
			if ( (sortmode >= CRID_SORT) && (sortmode < CRID_SORTEX) ){
				index = sortmode;
				break;
			}
			SortMenuCheck(hMenu, (TCHAR *)PopupTbl.bottom, id, &tmpxc, descending_sort);
			index = PPcTrackPopupMenu(cinfo, hMenu);

			if ( index == CRID_SORT_ASCENDING ){
				descending_sort = 0;
				continue;
			}
			if ( index == CRID_SORT_DESCENDING ){
				descending_sort ^= 1;
				continue;
			}
			if ( (index < CRID_SORT_TEMP) || (index >= CRID_SORT_MODELAST) ){
				break;
			}
			mmmode = index;
			CheckMenuRadioItem(hMenu, CRID_SORT_TEMP, CRID_SORT_MODELAST - 1, mmmode, MF_BYCOMMAND);
		}
		DestroyMenu(hMenu);
	} else{
		mmmode = sortmode ? CRID_SORT_REGID : CRID_SORT_TEMP;
		index = CRID_SORT;	// 登録メニューがないので詳細指定を用意しておく
		ThAddString(&PopupTbl, NilStr);
		tmpxc = *xc;
	}
	while ( index ){
		GetMenuDataMacro2(p, &PopupTbl, index - CRID_SORT);
		if ( p == NULL ) break; // 異常値
		if ( *p == '\0' ){ // 空欄…編集
			PPCSORTDIALOGPARAM psdp;

			psdp.cinfo = cinfo;
			psdp.xc = &tmpxc;
			if ( PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SORT),
				cinfo->info.hWnd, SortDlgBox, (LPARAM)&psdp) <= 0 ){
				break;
			}
			*xc = tmpxc;
		} else if ( *p == 'r' ){ // react
			*xc = cinfo->sort_last;
		} else{ // 一般
			char modedat;
			LoadSortOpt(&p, xc);

			modedat = xc->mode.dat[0];
			if ( descending_sort && (modedat >= 0) && (modedat <= MAXSORTITEM) ){
				xc->mode.dat[0] = DescendingSortTable[modedat];
			}
		}
		SaveSortSetting(cinfo, mmmode, setpath, xc);
		result = TRUE;
		break;
	}
	ThFree(&PopupTbl);
	return result;
}

//-----------------------------------------------------------------------------
void PPC_SortMain(PPC_APPINFO *cinfo, XC_SORT *xc)
{
	const TCHAR *filename;

	filename = CEL(cinfo->e.cellN).f.cFileName;
	CellSort(cinfo, xc);
	if ( cinfo->hHeaderWnd != NULL ) FixHeader(cinfo);
	cinfo->DrawTargetFlags = DRAWT_ALL;
	InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
	if ( XC_acsr[1] ){
		FindCell(cinfo, filename);
	} else{
		MoveCellCsr(cinfo, 0, NULL);
	}
}

//-----------------------------------------------------------------------------
// ソート
ERRORCODE SortKeyCommand(PPC_APPINFO *cinfo, DWORD sortmode, const TCHAR *path)
{
	XC_SORT xc;

	xc = cinfo->XC_sort;
	if ( PPC_SortMenu(cinfo, &xc, sortmode, path) == FALSE ) return ERROR_CANCELLED;
	if ( (sortmode == CRID_SORT_PATH_BRANCH) && (path != NULL) && (path[0] != '\0') ){
		return NO_ERROR; // -"path" 指定の時はソートしない
	}

	PPC_SortMain(cinfo, &xc);
	return NO_ERROR;
}

//-----------------------------------------------------------------------------
// 全マーク
void PPC_AllMark(PPC_APPINFO *cinfo)
{
	ENTRYINDEX index;
	int mark;

	if ( TinyCheckCellEdit(cinfo) ) return;
	mark = (cinfo->e.markC == 0) ? MARK_CHECK : MARK_REMOVE;
	for ( index = 0; index < cinfo->e.cellIMax; index++ ){
		CellMark(cinfo, index, mark);
	}
	Repaint(cinfo);
	if ( (OSver.dwMajorVersion >= 6) && (cinfo->hHeaderWnd != NULL) ) FixHeader(cinfo);
}

//-----------------------------------------------------------------------------
// マーク反転
void PPC_ReverseMark(PPC_APPINFO *cinfo)
{
	ENTRYINDEX index;

	if ( TinyCheckCellEdit(cinfo) ) return;
	for ( index = 0; index < cinfo->e.cellIMax; index++ ){
		CellMark(cinfo, index, MARK_REVERSE);
	}
	Repaint(cinfo);
	if ( (OSver.dwMajorVersion >= 6) && (cinfo->hHeaderWnd != NULL) ) FixHeader(cinfo);
}

//=============================================================================
ERRORCODE MarkEntry(PPC_APPINFO *cinfo, const TCHAR *wildcard, int mode, DWORD attr)
{
	FN_REGEXP fn;
	ENTRYINDEX index;
	DWORD DirMask, result;
	ENTRYCELL *cell;

	if ( TinyCheckCellEdit(cinfo) ) return ERROR_PATH_BUSY;

	result = MakeFN_REGEXP(&fn, wildcard);
	if ( result & REGEXPF_ERROR ) return ERROR_INVALID_PARAMETER;
	cinfo->MarkMask = MARKMASK_DIRFILE;
	DirMask = ((result & REGEXPF_PATHMASK) || (attr & MASKEXTATTR_DIR)) ?
			0 : FILE_ATTRIBUTE_DIRECTORY;
	if ( mode < MARK_HIGHLIGHTOFF ){ // マーク
		for ( index = 0; index < cinfo->e.cellIMax; index++ ){
			cell = &CEL(index);
			if ( (cell->f.dwFileAttributes & DirMask) ) continue;
			if ( cell->attr & (ECA_PARENT | ECA_THIS) ) continue;
			if ( FinddataRegularExpression(&cell->f, &fn) != FRRESULT_NO ){
				CellMark(cinfo, index, mode);
			}
		}
	}else{ // highlight
		BYTE hldata;

		hldata = (BYTE)(mode - MARK_HIGHLIGHTOFF);
		for ( index = 0; index < cinfo->e.cellIMax; index++ ){
			cell = &CEL(index);
			if ( (cell->f.dwFileAttributes & DirMask) ) continue;
			if ( cell->attr & (ECA_PARENT | ECA_THIS) ) continue;
			if ( FinddataRegularExpression(&cell->f, &fn) != FRRESULT_NO ){
				cell->highlight = hldata;
			}
		}
	}
	FreeFN_REGEXP(&fn);
	Repaint(cinfo);
	return NO_ERROR;
}

// ファイルマーク *markentry
ERRORCODE PPC_FindMark(PPC_APPINFO *cinfo, const TCHAR *defmask, int mode)
{
	FILEMASKDIALOGSTRUCT pfs = {NULL, NULL, NULL, NULL, T("XC_rmrk"), NULL, NULL, NULL, FALSE, 0, {1, 0, 0, 0}};
	XC_MASK mask;
	TCHAR listname[VFPS];
	int result;
	BOOL dialog = TRUE;

	if ( TinyCheckCellEdit(cinfo) ) return ERROR_PATH_BUSY;

	mask.attr = 0;
	mask.file[0] = '\0';
	listname[0] = '\0';

	if ( defmask != NULL ){
		dialog = FALSE;
		while ( SkipSpace(&defmask) != '\0' ){
			if ( *defmask == '-' ){
				const TCHAR *ptr = defmask, *more;
				TCHAR buf[CMDLINESIZE];

				GetOptionParameter(&ptr, buf, (TCHAR **)&more);
				if ( (tstrcmp(buf + 1, T("LIST")) == 0) || (tstrcmp(buf + 1, T("SET")) == 0) ){
					VFSFixPath(listname, (TCHAR *)more, cinfo->path, VFSFIX_VFPS);
					if ( buf[1] == 'S' ) mode = MARK_SET;
				}else if ( tstrcmp(buf + 1, T("DIALOG")) == 0 ){
					dialog = TRUE;
				}else if ( tstrcmp(buf + 1, T("MARK")) == 0 ){
					mode = MARK_CHECK;
				}else if ( tstrcmp(buf + 1, T("UNMARK")) == 0 ){
					mode = MARK_REMOVE;
				}else if ( tstrcmp(buf + 1, T("REVERSEMARK")) == 0 ){
					mode = MARK_REVERSE;
				}else if ( tstrcmp(buf + 1, T("HIGHLIGHT")) == 0 ){
					if ( Isdigit(*more) ){
						mode = GetNumber(&more);
						if ( mode > ECS_HLMAX ) return ERROR_INVALID_PARAMETER;
						mode += MARK_HIGHLIGHT1 - 1;
					}else{
						mode = MARK_HIGHLIGHT1;
					}
				}else if ( tstrcmp(buf + 1, T("K")) == 0 ){
					dialog = TRUE;
					if ( *more == '\0' ){
						more = ptr;
						ptr = NilStr;
					}
					if ( pfs.initcmd != NULL ){
						PPcHeapFree(pfs.initcmd);
					}
					pfs.initcmd = PPcHeapAlloc(TSTRSIZE(more));
					tstrcpy(pfs.initcmd, more);
				}else{
					XMessage(cinfo->info.hWnd, NULL, XM_GrERRld, StrBadOption, buf);
					return ERROR_INVALID_PARAMETER;
				}
				defmask = ptr;
			}else{
				if ( mask.file[0] != '\0' ) break;
				if ( *defmask == '\"' ){ // セパレータ有り
					GetCommandParameter(&defmask, mask.file, TSIZEOF(mask.file));
					continue;
				}else{
					SetExtParam(defmask, mask.file, TSIZEOF(mask.file));
					break;
				}
			}
		}
	}
	pfs.mode = mode;

	if ( listname[0] != '\0' ) {
		return ApplyByListFile(cinfo, listname, mode);
	}

	if ( dialog || (SearchVLINEwild(mask.file) != NULL) ){
		pfs.title = (mode != MARK_REMOVE) ? MES_TFMK : MES_TFUM;
		pfs.mask = &mask;
		pfs.filename = CEL(cinfo->e.cellN).f.cFileName;
		pfs.cinfo = cinfo;
		result = (int)PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_MASK),
			cinfo->info.hWnd, FileMaskDialog, (LPARAM)&pfs);
		if ( result <= 0 ) return ERROR_CANCELLED;
	}
	return MarkEntry(cinfo, mask.file, pfs.mode, mask.attr);
}

//-----------------------------------------------------------------------------
// ツリー表示
//-----------------------------------------------------------------------------
void PPC_CloseTree(PPC_APPINFO *cinfo)
{
	if ( cinfo->hTreeWnd == NULL ) return;

	SendMessage(cinfo->hTreeWnd, WM_CLOSE, 0, 0);
	cinfo->hTreeWnd = NULL;
	cinfo->TreeX = 0;
	cinfo->XC_tree.mode = PPCTREE_OFF;
	if ( X_fles | cinfo->bg.X_WallpaperType ){
		SetWindowLong(cinfo->info.hWnd, GWL_STYLE,
			GetWindowLongPtr(cinfo->info.hWnd, GWL_STYLE) & ~WS_CLIPCHILDREN);
	}
	WmWindowPosChanged(cinfo);
	InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
}

void PPc_SetTreeFlags(HWND hParentWnd, HWND hTreeWnd)
{
	DWORD treeflag;

	treeflag = (TouchMode & TOUCH_DISABLEDRAG) ?
		(VFSTREE_SELECT | VFSTREE_PATHNOTIFY | VFSTREE_PPC | VFSTREE_SPLITR | VFSTREE_DISABLEDRAG) :
		(VFSTREE_SELECT | VFSTREE_PATHNOTIFY | VFSTREE_PPC | VFSTREE_SPLITR);
	if ( hParentWnd == Combo.hWnd ) resetflag(treeflag, VFSTREE_SPLITR);

	SendMessage(hTreeWnd, VTM_SETFLAG, (WPARAM)hParentWnd, (LPARAM)treeflag);
}
//------------------------------------------------
void PPC_Tree(PPC_APPINFO *cinfo, int mode, BOOL sync)
{
	PPCTREESETTINGS pts;

	if ( cinfo->combo && (X_combos[0] & CMBS_COMMONTREE) ){
		SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND, TMAKELPARAM(KCW_tree, mode), (LPARAM)cinfo->path);
		if ( Combo.hTreeWnd != NULL ){
			SetFocus(Combo.hTreeWnd);
		}
		return;
	}

	if ( cinfo->hTreeWnd != NULL ){
		if ( mode == PPCTREE_SELECT ){
			SetFocus(cinfo->hTreeWnd);
		} else{
			PPC_CloseTree(cinfo);
		}
		return;
	}
	InitVFSTree();

	if ( (cinfo->BoxEntries.right > 16) && ((cinfo->XC_tree.width < 16) ||
		(cinfo->XC_tree.width > (cinfo->BoxEntries.right - 8))) ){
		cinfo->XC_tree.width = cinfo->BoxEntries.right / 3;
	}
	// pts.typename を取得する (mode, widthは使わない)
	pts.name[0] = '\0';
	GetCustTable(T("XC_tree"), cinfo->RegCID + 1, &pts, sizeof(pts));

	cinfo->TreeX = cinfo->XC_tree.width;
	cinfo->XC_tree.mode = mode;
	cinfo->hTreeWnd = CreateWindowEx(0, Str_TreeClass, Str_TreeClass,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, cinfo->BoxEntries.top - cinfo->HeaderHeight, cinfo->TreeX,
		cinfo->BoxEntries.bottom - cinfo->BoxEntries.top + cinfo->HeaderHeight,
		cinfo->info.hWnd, CHILDWNDID(IDW_PPCTREE), hInst, 0);

	PPc_SetTreeFlags(cinfo->info.hWnd, cinfo->hTreeWnd);
	SendMessage(cinfo->hTreeWnd, VTM_INITTREE, (WPARAM)sync,
		(LPARAM)((pts.name[0] != '\0') ? pts.name : cinfo->path));
	ShowWindow(cinfo->hTreeWnd, SW_SHOWNORMAL);

	#ifndef USEDIRECTX
	if ( X_fles | cinfo->bg.X_WallpaperType )
		#endif
	{ // if に接続
		SetWindowLong(cinfo->info.hWnd, GWL_STYLE,
			GetWindowLongPtr(cinfo->info.hWnd, GWL_STYLE) | WS_CLIPCHILDREN);
	}
	WmWindowPosChanged(cinfo);
	InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
}
//-----------------------------------------------------------------------------
// ファイルの閲覧
void PPC_View(PPC_APPINFO *cinfo)
{
	TCHAR exe[VFPS];

	if ( (NO_ERROR != GetCustTable(T("A_exec"), T("viewer"), exe, sizeof(exe))) || (exe[0] == '\0') ){
		tstrcpy(exe, T("notepad"));
		if ( PPctInput(cinfo, MES_QUSV, exe, TSIZEOF(exe) - 2,
			PPXH_PATH_R, PPXH_PATH) <= 0 ){
			return;
		}
		if ( (tstrchr(exe, ' ') != NULL) && (exe[0] != '\"') ){
			tstrgap(exe, 1);
			exe[0] = '\"';
			tstrcat(exe, T("\""));
		}
		SetCustStringTable(T("A_exec"), T("viewer"), exe, 0);
	}
	PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, T("*execinarc %: viewer %*name(DC,\"%R\")"), NULL, XEO_NOUSEPPB);
}

//-----------------------------------------------------------------------------
// パスジャンプ
ERRORCODE PPC_PathJump(PPC_APPINFO *cinfo)
{
	TCHAR param[CMDLINESIZE];
	const TCHAR *p;
	DWORD flags = VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_NOFIXEDGE;

	if ( PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, T("%M@M_pjump"), param, 0) ){
		return ERROR_CANCELLED;
	}
	p = param;
	if ( SkipSpace(&p) == '\"' ) flags = VFSFIX_SEPARATOR | VFSFIX_FULLPATH;
	if ( IsTrue(cinfo->ChdirLock) ){
		VFSFixPath(param, param, cinfo->path, flags);
		PPCuiWithPathForLock(cinfo, param);
	} else{
		SetPPcDirPos(cinfo);
		VFSFixPath(param, param, cinfo->path, flags);
		ChangePath(cinfo, param, CHGPATH_SETABSPATH);
		read_entry(cinfo, RENTRY_READ);
	}
	return NO_ERROR;
}
//-----------------------------------------------------------------------------
// ドライブジャンプ

HMENU MakeDriveJumpMenu(PPC_APPINFO *cinfo, HMENU hPopupMenu, DWORD *index, ThSTRUCT *TH, BOOL advanced)
{
	TCHAR buf[VFPS], rpath[VFPS];
	DWORD drv;
	int i;
	DWORD X_dlf[2] = {0, 0};

	#ifndef WINEGCC
	#define DRIVEOFF 0
	#else
	#define DRIVEOFF 1
	#endif
	if ( hPopupMenu == NULL ) hPopupMenu = CreatePopupMenu();
	drv = GetLogicalDrives();
	GetCustData(Str_X_dlf, &X_dlf, sizeof(X_dlf));
	if ( advanced ) setflag(X_dlf[1], XDLF_DISPFREE);
	for ( i = 0; i < 26; i++ ){
		thprintf(buf, TSIZEOF(buf), T("Network\\%c"), (TCHAR)(i + 'A'));
		rpath[0] = '\0';
		GetRegString(HKEY_CURRENT_USER, buf, RMPATHSTR, rpath, TSIZEOF(rpath));
		if ( rpath[0] == '\0' ){
			thprintf(buf, TSIZEOF(buf), T("%c:"), (TCHAR)(i + 'A'));
			GetRegString(HKEY_LOCAL_MACHINE, RegDeviceNamesStr, buf, rpath, TSIZEOF(rpath));
		}
		if ( (drv & LSBIT) || rpath[0] ){
			#define BUFDRVTAIL (buf + DRIVEOFF + 3)
			#ifdef WINEGCC
			buf[0] = '&';
			#endif
			buf[DRIVEOFF] = (TCHAR)(i + 'A');
			buf[DRIVEOFF + 1] = ':';
			buf[DRIVEOFF + 2] = '\t';
			if ( drv & LSBIT ){
				GetDriveNameTitle(BUFDRVTAIL, buf[DRIVEOFF]);

				if ( ((DefDriveList >> i) & LSBIT) == 0 ){
					tstrcat(BUFDRVTAIL, T(" *"));
				}
				if ( (tstrlen(rpath) > 5) && (rpath[2] == '?') ){
					tstrcat(BUFDRVTAIL, T(" "));
					tstrcat(BUFDRVTAIL, rpath + 4);
				}

				if ( (X_dlf[1] & XDLF_DISPFREE) && (tstrcmp(buf + DRIVEOFF, T("Floppy")) != 0) ){
					ULARGE_INTEGER UserFree, Total, TotalFree;

					LetHL_0(Total);
					thprintf(rpath, TSIZEOF(rpath), T("%c:\\"), buf[DRIVEOFF]);
					#ifndef UNICODE
					if ( DGetDiskFreeSpaceEx )
						#endif
					{
						DGetDiskFreeSpaceEx(rpath, &UserFree, &Total, &TotalFree);
					}
					if ( Total.u.LowPart | Total.u.HighPart ){
						FormatNumber(BUFDRVTAIL + tstrlen(BUFDRVTAIL), XFN_SEPARATOR | XFN_RIGHT, 5, Total.u.LowPart, Total.u.HighPart);
					}
				}
			} else{
				thprintf(BUFDRVTAIL, MAX_PATH, T("offline %s"), rpath);
			}
			AppendMenuCheckString(hPopupMenu, *index, buf,
				buf[DRIVEOFF] == cinfo->path[0]);
			(*index)++;
			if ( TH != NULL ){
				buf[DRIVEOFF + 2] = '\0';
				ThAddString(TH, buf + DRIVEOFF);
			}
		}
		drv = drv >> 1;
	}

	{ // \\tsclient 列挙
		HKEY hSessionInfo, hNameSpace;

		if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, cStrRegTsClient, 0, KEY_READ, &hSessionInfo)){
			int count = 0;
			TCHAR KeyName[128], comid[128], path[MAX_PATH];
			FILETIME write;
			DWORD rsize;

			for ( ; ; count++ ){
				rsize = TSIZEOF(KeyName);
				if ( ERROR_SUCCESS != RegEnumKeyEx(hSessionInfo, count, KeyName, &rsize, NULL, NULL, NULL, &write) ){
					break;
				}
				tstrcat(KeyName, T("\\MyComputer\\Namespace"));

				if ( ERROR_SUCCESS == RegOpenKeyEx(hSessionInfo, KeyName, 0, KEY_READ, &hNameSpace)){
					int count2;

					count2 = 0;
					rsize = 200;
					tstrcpy(comid, T("CLSID\\"));
					if ( ERROR_SUCCESS == RegEnumKeyEx(hNameSpace, count2, comid + 6, &rsize, NULL, NULL, NULL, &write) ){
						tstrcat(comid, T("\\Instance\\InitPropertyBag"));
						GetRegString(HKEY_CLASSES_ROOT, comid, cStrRegTsTarget, path, TSIZEOF(path));
						AppendMenuString(hPopupMenu, *index, path);
						(*index)++;
						if ( TH != NULL ){
							ThAddString(TH, path);
						}
					}
					RegCloseKey(hNameSpace);
				}
			}
			RegCloseKey(hSessionInfo);
		}
	}

	// wsl列挙
	buf[2] = '\0';
	GetRegString(HKEY_LOCAL_MACHINE, cStrRegWslPath, cStrRegWslTarget, buf + 2, TSIZEOF(buf) - sizeof(TCHAR) * 2);
	if ( buf[2] != '\0' ){
		buf[0] = buf[1] = '\\';
		AppendMenuString(hPopupMenu, *index, buf);
		(*index)++;
		if ( TH != NULL ) ThAddString(TH, buf);
	}

	{ // MTP デバイス列挙
		LPITEMIDLIST idl;
		LPSHELLFOLDER pSF;
		LPENUMIDLIST pEID;
		LPMALLOC pMA;

		pSF = VFPtoIShell(NULL, T("#17:\\"), NULL);
		if ( pSF != NULL ){
			(void)SHGetMalloc(&pMA);

			if ( S_OK == pSF->lpVtbl->EnumObjects(pSF, NULL,
				(OSver.dwMajorVersion >= 6) ?
				ENUMOBJECTSFORFOLDERFLAG_VISTA : ENUMOBJECTSFORFOLDERFLAG_XP,
				&pEID) ){ // S_FALSE のときは、pEID = NULL
				for ( ; ; ){
					DWORD getsi;

					if ( pEID->lpVtbl->Next(pEID, 1, &idl, &getsi) != S_OK ){
						break;
					}
					if ( getsi == 0 ) break;

					if ( IsTrue(RPIDL2DisplayNameOf(buf, pSF, idl)) ){
						// XMessage(NULL, NULL, XM_DbgLOG, T("%s"),buf);
						// ::{xxx} ライブラリ
						// \\?\usb#vid_… MTP
						// X: drive
						if ( (buf[0] == '\\') && (buf[2] == '?') ){
							if ( IsTrue(PIDL2DisplayNameOf(rpath, pSF, idl)) ){
								thprintf(buf, TSIZEOF(buf), T("%s\tmtp:"), rpath);
								AppendMenuString(hPopupMenu, *index, buf);
								(*index)++;
								if ( TH != NULL ){
									thprintf(buf, TSIZEOF(buf), T("#17:\\%s"), rpath);
									ThAddString(TH, buf);
								}
							}
						}
					}
					pMA->lpVtbl->Free(pMA, idl);
				}
				pEID->lpVtbl->Release(pEID);
			}
			pMA->lpVtbl->Release(pMA);
			pSF->lpVtbl->Release(pSF);
		}
	}

	{ // AUX: 列挙
		HMENU hAuxMenu = NULL;
		int count = 0;
		TCHAR key[CUST_NAME_LENGTH], param[CMDLINESIZE], *sepptr;

		for( ; EnumCustData(count, key, NULL, 0) >= 0; count++ ){
			if ( ((key[0] != 'M') && (key[0] != 'S')) ||
				 (key[1] != '_') ||
				 (key[2] != 'a') ||
				 (key[3] != 'u') ||
				 (key[4] != 'x') ){
				continue;
			}

			param[0] = '\0';
			GetCustTable(key, cStrAuxBase, param, sizeof(param));
			if ( param[0] == '\0' ) continue;
			if ( hAuxMenu == NULL ){
				hAuxMenu = CreatePopupMenu();
				AppendMenu(hPopupMenu, MF_EPOP, (UINT_PTR)hAuxMenu, T("aux:"));
			}
			sepptr = tstrstr(param, cStrAuxSep);
			if ( sepptr != NULL ){
				*sepptr = '\0';
				sepptr += TSIZEOFSTR(cStrAuxSep);
			}else{
				sepptr = param;
			}
			if ( TH == NULL ){
				thprintf(buf, TSIZEOF(buf), T("%s\t%s"), key, sepptr);
				AppendMenuString(hAuxMenu, *index, buf);
			}else{
				AppendMenuString(hAuxMenu, *index, sepptr);
				ThAddString(TH, param);
			}
			(*index)++;
		}
	}
	return hPopupMenu;
	#undef DRIVEOFF
}

void USEFASTCALL PPC_DriveJumpMain(PPC_APPINFO *cinfo, TCHAR *menubuf)
{
	TCHAR newpath[VFPS];

	SetPPcDirPos(cinfo);

	#ifdef WINEGCC
		if ( *menubuf == '&' ) menubuf++;
	#endif

	if ( (menubuf[1] == ':') || (menubuf[0] == '\\') ){ // drive "x:..." "\\...
		DWORD X_dlf[2] = {0, 0};

		GetCustData(Str_X_dlf, &X_dlf, sizeof(X_dlf));
		if ( X_dlf[1] & XDLF_ROOTJUMP ){
			if ( menubuf[1] == ':' ){
				menubuf[2] = '\\';
				menubuf[3] = '\0';
			}
		} else{
			if ( menubuf[1] == ':' ){
				menubuf[2] = '\0';
			}else{
				tstrcat(menubuf, T("\\:"));
			}
		}
		VFSFixPath(NULL, menubuf, cinfo->path, VFSFIX_VFPS);
	}else{ // mtp: mtppath \t "mtpname" / aux: key \t comment
		TCHAR *p;

		p = tstrchr(menubuf, '\t');
		if ( p != NULL ) *p = '\0';
		if ( ((menubuf[0] == 'M') || (menubuf[0] == 'S')) && (menubuf[1] == '_') ){
			newpath[0] = '\0';
			GetCustTable(menubuf, cStrAuxBase, newpath, sizeof(newpath));
			p = tstrstr(newpath, cStrAuxSep);
			if ( p != NULL ){
				*p = '\0';
				p += TSIZEOFSTR(cStrAuxSep);
			}else{
				p = newpath;
			}
			if ( PPctInput(cinfo, p, newpath, TSIZEOF(newpath),
					PPXH_PATH | PPXH_PPCPATH, PPXH_PATH) <= 0 ){
				return;
			}
		}else{
			thprintf(newpath, TSIZEOF(newpath), T("#17:\\%s"), menubuf);
		}
		tstrcpy(menubuf, newpath);
	}
	tstrcpy(newpath, cinfo->path);
	if ( DirChk(menubuf, newpath) ) tstrcpy(newpath, menubuf);
	if ( cinfo->ChdirLock == FALSE ){
		DxSetMotion(cinfo->DxDraw, DXMOTION_ChangeDrive);
		ChangePath(cinfo, newpath, CHGPATH_SETABSPATH);
		read_entry(cinfo, RENTRY_READ);
	} else{
		PPCuiWithPathForLock(cinfo, newpath);
	}
}

ERRORCODE PPC_DriveJump(PPC_APPINFO *cinfo, BOOL advanced)
{
	HMENU hMenu;
	int i;
	TCHAR buf[VFPS];

	DWORD index = CRID_DRIVELIST;
	PPXMENUINFO xminfo;
	POINT pos;
	BOOL popmode;

	for(;;){
		index = CRID_DRIVELIST;
		hMenu = MakeDriveJumpMenu(cinfo, NULL, &index, NULL, advanced);

		xminfo.info = &cinfo->info;
		xminfo.hMenu = hMenu;
		xminfo.commandID = 0x10000;
		ThInit(&xminfo.th); // 未使用だけど初期化はしておく

		RemoveControlKeydown(cinfo->info.hWnd);
		GetPopupPosition(cinfo, &pos);
		popmode = PPxSetMenuInfo(hMenu, &xminfo);

		i = TrackPopupMenu(hMenu,
			popmode ? TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RECURSE : TPM_TDEFAULT,
			pos.x, pos.y, 0, cinfo->info.hWnd, NULL);
		if ( (i != 0) || !(xminfo.commandID & B31) ) break;
		DestroyMenu(hMenu);
		advanced = !advanced;
	}

	if ( i >= CRID_DRIVELIST ){
		GetMenuString(hMenu, i, buf, TSIZEOF(buf), MF_BYCOMMAND);
		DestroyMenu(hMenu);

		PPC_DriveJumpMain(cinfo, buf);
		return NO_ERROR;
	} else{
		DestroyMenu(hMenu);
		return ERROR_CANCELLED;
	}
}
//-----------------------------------------------------------------------------
void PPcChangeWindow(PPC_APPINFO *cinfo, int direction)
{
	HWND nhWnd;

	if ( cinfo->combo ){
		PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
			TMAKELPARAM(KCW_nextppc, direction), (LPARAM)cinfo->info.hWnd);
		return;
	}

	// ※PPCHGWIN_PAIR は、CGETW_NEXTNOPREV の動作を先に行う
	nhWnd = PPcGetWindow(cinfo->RegNo, direction);
	if ( nhWnd != NULL ){
		if ( direction == PPCHGWIN_PAIR ){
			nhWnd = PPcGetWindow(cinfo->RegNo, CGETW_PAIR);
		}

		if ( nhWnd != NULL ){
			if ( IsIconic(nhWnd) ){
				SendMessage(nhWnd, WM_SYSCOMMAND, SC_RESTORE, 0xffff0000);
			}
			ForceSetForegroundWindow(nhWnd);
			SetFocus(nhWnd);
			return;
		}
	}
	if ( cinfo->swin & SWIN_WBOOT ){
		BootPairPPc(cinfo);
	} else{
		PPCui(cinfo->info.hWnd, NULL);
	}
}

ERRORCODE WriteComment(PPC_APPINFO *cinfo, TCHAR *cname)
{
	ENTRYDATAOFFSET cofs;
	HANDLE hFile;
	TCHAR buf[VFPS];
	DWORD savemode;
	DWORD tmp;
	DWORD orgattr;

#define csave_no 0
#define csave_check 1
#define csave_always 2

	if ( cinfo->e.Dtype.mode == VFSDT_LFILE ){
		if ( IsTrue(cinfo->ModifyComment) || (cname == NULL) ){
			savemode = GetCustXDword(T("XC_cwrt"), NULL, 1);
			if ( (cname == NULL) && (savemode == csave_no) ) savemode = csave_check;
			if ( savemode == csave_no ) return NO_ERROR;
			if ( (savemode == csave_check) &&
				(PMessageBox(cinfo->info.hWnd, MES_QMCF, MES_TMCF, MB_QYES) != IDOK) ){
				if ( IsTrue(cinfo->ModifyComment) && (cname != NULL) ){
					cinfo->ModifyComment = FALSE; // read_entry で確認しないように
				}
				return ERROR_CANCELLED;
			}
			WriteListFileForRaw(cinfo, NULL);
			cinfo->ModifyComment = FALSE;
		}
		return NO_ERROR;
	}

	if ( cname == NULL ){
		CatPath(buf, cinfo->path, Str_CommentFile);
		cname = buf;
		savemode = csave_check;
	} else{
		savemode = GetCustXDword(T("XC_cwrt"), NULL, csave_check);
	}
	if ( savemode == csave_no ) return NO_ERROR;
	if ( (savemode == csave_check) &&
		(PMessageBox(cinfo->info.hWnd, MES_QMCF, MES_TMCF, MB_QYES) != IDOK) ){
		if ( IsTrue(cinfo->ModifyComment) && (cname != buf) ){
			cinfo->ModifyComment = FALSE; // read_entry で確認しないように
		}
		return ERROR_CANCELLED;
	}

	orgattr = GetFileAttributesL(cname);
	if ( (orgattr != BADATTR) && (orgattr & FILE_ATTRIBUTE_HIDDEN) ) {
		SetFileAttributesL(cname, orgattr & ~FILE_ATTRIBUTE_HIDDEN);
	}
	hFile = CreateFileL(cname, GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ){
		if ( orgattr != BADATTR ) SetFileAttributesL(cname, orgattr);
		SetPopMsg(cinfo, POPMSG_GETLASTERROR, MES_TMCF);
		return ERROR_ACCESS_DENIED;
	}
	#ifdef UNICODE
	WriteFile(hFile, UCF2HEADER, UCF2HEADERSIZE, &tmp, NULL);
	#endif
	for ( cofs = 0; cofs < cinfo->e.cellDataMax; cofs++ ){
		TCHAR *namep, *textp;

		if ( CELdata(cofs).comment == EC_NOCOMMENT ) continue;
		namep = tstrchr(CELdata(cofs).f.cFileName, ' ');
		if ( namep != NULL ) WriteFile(hFile, T("\""), TSTROFF(1), &tmp, NULL);
		WriteFile(hFile, CELdata(cofs).f.cFileName,
			TSTRLENGTH32(CELdata(cofs).f.cFileName), &tmp, NULL);
		if ( namep != NULL ){
			WriteFile(hFile, T("\"\t"), TSTROFF(2), &tmp, NULL);
		} else{
			WriteFile(hFile, T("\t"), TSTROFF(1), &tmp, NULL);
		}
		textp = ThPointerT(&cinfo->e.Comments, CELdata(cofs).comment);
		if ( *textp == ' ' ) {
			WriteFile(hFile, T("\""), TSTROFF(1), &tmp, NULL);
		}
		WriteFile(hFile, textp, TSTRLENGTH32(textp), &tmp, NULL);
		WriteFile(hFile, T("\r\n"), TSTROFF(2), &tmp, NULL);
	}
	CloseHandle(hFile);
	if ( orgattr != BADATTR ) SetFileAttributesL(cname, orgattr);
	cinfo->ModifyComment = FALSE;
	return NO_ERROR;
}


#if !NODLL
const LONG crc32_half[16] = {
	0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
	0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
	0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
	0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};

DWORD crc32(const BYTE *bin, DWORD size, DWORD r)
{
	DWORD crc = ~r;
	const BYTE *ptr = bin;
	BYTE chr;

	if ( size == MAX32 ) size = TSTRLENGTH32((TCHAR *)bin);
	while ( size-- ){
		chr = *ptr++;
		crc = crc32_half[(crc ^  chr      ) & 0x0F] ^ (crc >> 4);
		crc = crc32_half[(crc ^ (chr >> 4)) & 0x0F] ^ (crc >> 4);
	}
	return ~crc;
}
#endif

void SetComment(PPC_APPINFO *cinfo, DWORD CommentID, ENTRYCELL *cell, const TCHAR *comment)
{
	if ( CommentID == 0 ){
		if ( (cell->comment != EC_NOCOMMENT) &&
			(cinfo->e.Comments.bottom != NULL) &&
			(tstrcmp(ThPointerT(&cinfo->e.Comments, cell->comment), comment) == 0) ){
			return; // 変更無し
		}

		if ( cinfo->e.Comments.top == 0 ){
			ThAppend(&cinfo->e.Comments, &NilDWORD, sizeof(NilDWORD)); // ターミネータを設定
		}
					// ※※※ガーページコレクションなどはしない
		cell->comment = cinfo->e.Comments.top;
		ThAddString(&cinfo->e.Comments, comment);
		cinfo->ModifyComment = TRUE;
	} else{
		EntryExtData_SetString(cinfo, CommentID, cell, comment);
	}
}
void MakeDigestStrings(BYTE *digest, TCHAR *deststr, int size, TCHAR *text)
{
	TCHAR *dest;
	int i;

	dest = tstpcpy(deststr, text);
	for ( i = 0; i < size; i++ ){
		dest = thprintf(dest, 4, T("%02x"), digest[i]);
	}
}

#define COMMENTENUM_MARK 0
#define COMMENTENUM_ALL 1
#define COMMENTENUM_FILES 2

typedef struct {
	PPC_APPINFO *cinfo;
	TCHAR *filesptr;
	ENTRYCELL *cell;
	TCHAR name[VFPS];
	int work;
	int enummode;
	SENDSETCOMMENT ssc;
} COMMENTENUMINFO;

void InitCommentEnumInfo(PPC_APPINFO *cinfo, COMMENTENUMINFO *cei, DWORD CommentID, TCHAR *files)
{
	cei->cinfo = cinfo;
	cei->filesptr = files;
	cei->ssc.CommentID = CommentID;
	switch ( cei->enummode ){
		case COMMENTENUM_ALL:
			cei->work = 0;
			break;
		case COMMENTENUM_FILES:
			cei->ssc.LoadCounter = cinfo->LoadCounter;
			break;
		default: // COMMENTENUM_MARK
			InitEnumMarkCell(cinfo, &cei->work);
	}
}

BOOL CommentEnum(COMMENTENUMINFO *cei)
{
	switch ( cei->enummode ){
		case COMMENTENUM_ALL:
			for ( ;;){
				if ( cei->work >= cei->cinfo->e.cellIMax ) return FALSE;
				cei->cell = &((ENTRYCELL *)cei->cinfo->e.CELLDATA.p)[((ENTRYINDEX *)cei->cinfo->e.INDEXDATA.p)[cei->work]];
				cei->work++;
				if ( cei->cell->attr & (ECA_PARENT | ECA_THIS) ) continue;
				break;
			}
			break;

		case COMMENTENUM_FILES:
			if ( *cei->filesptr == '\0' ) return FALSE;

			tstrcpy(cei->name, cei->filesptr);
			cei->filesptr = (TCHAR *)(BYTE *)((BYTE *)(cei->filesptr + tstrlen(cei->filesptr) + 1) + sizeof(ENTRYDATAOFFSET));
			cei->ssc.dataindex = *(ENTRYDATAOFFSET *)((BYTE *)cei->filesptr - sizeof(ENTRYDATAOFFSET));
			return TRUE;

		default: // COMMENTENUM_MARK
			if ( (cei->cell = EnumMarkCell(cei->cinfo, &cei->work)) == NULL ){
				return FALSE;
			}
			break;
	}
	VFSFullPath(cei->name, cei->cell->f.cFileName, cei->cinfo->path);
	return TRUE;
}

void SetComment_CommentEnum(COMMENTENUMINFO *cei, const TCHAR *text)
{
	switch ( cei->enummode ){
		case COMMENTENUM_FILES:
			cei->ssc.comment = text;
			SendMessage(cei->cinfo->info.hWnd, WM_PPCSETCOMMENT, (WPARAM)&cei->ssc, 0);
			break;
//		case COMMENTENUM_ALL:
		default: // COMMENTENUM_MARK
			SetComment(cei->cinfo, cei->ssc.CommentID, cei->cell, text);
	}
}

void SetCommentUseFlag(PPC_APPINFO *cinfo, DWORD CommentID)
{
	if ( CommentID > 0 ) CommentID = DFC_COMMENTEX_MAX + 1 - CommentID;
	setflag(cinfo->UseCommentsFlag, (1 << CommentID));
}

void SetCommentErrorMessage(COMMENTENUMINFO *cei, ERRORCODE result)
{
	TCHAR text[VFPS];

	PPErrorMsg(text, result);
	SetComment_CommentEnum(cei, text);
}

ERRORCODE SetHashComment(PPC_APPINFO *cinfo, COMMENTENUMINFO *cei, int type, DWORD CommentID, TCHAR *files, JOBINFO *jinfo)
{
	TCHAR text[VFPS];
	ERRORCODE result = NO_ERROR;
	DWORD binsize;
	BOOL asyncmode = FALSE;
	DWORD openflag;
	OVERLAPPED ovr, *ovrptr;
	int readid, calcid;
#define data_empty 0
#define data_read 1
#define data_have 2
	int datastate[2];
	BYTE *binbuf[2], *calcbin;
	DWORD readsize[2];

	binsize = GetNT_9xValue(1 * MB, 256 * KB); // NT:1Mbytes / 9x:256kbytes
	if ( OSver.dwMajorVersion >= 6 ){
		asyncmode = TRUE;
		ovrptr = &ovr;
		openflag = FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED;
		binbuf[1] = PPcHeapAlloc(binsize);
		if ( binbuf[1] == NULL ) return ERROR_NOT_ENOUGH_MEMORY;
	}else{
		ovrptr = NULL;
		openflag = FILE_FLAG_SEQUENTIAL_SCAN;
		binbuf[1] = NULL;
	}
	binbuf[0] = PPcHeapAlloc(binsize);
	if ( binbuf[0] == NULL ) return ERROR_NOT_ENOUGH_MEMORY;
	InitCommentEnumInfo(cinfo, cei, CommentID, files);
	while ( IsTrue(CommentEnum(cei)) ){
		HANDLE hFile;
		union {
			DWORD crc;
			MD5_CTX md5;
			SHA1Context sha1;
			SHA224Context sha224;
			SHA256Context sha256;
		} context;

		context.crc = 0;
		switch ( type ){
			case CM_MD5:
				MD5Init(&context.md5);
				break;
			case CM_SHA1:
				SHA1Reset(&context.sha1);
				break;
			case CM_SHA224:
				SHA224Reset(&context.sha224);
				break;
			case CM_SHA256:
				SHA256Reset(&context.sha256);
				break;
//			default:
		}

		readid = calcid = 0;
		datastate[0] = datastate[1] = data_empty;
		hFile = CreateFileL(cei->name, GENERIC_READ, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, openflag, NULL);
		if ( hFile == INVALID_HANDLE_VALUE ){
			SetCommentErrorMessage(cei, PPERROR_GETLASTERROR);
			continue;
		}
		if ( asyncmode ){
			memset(&ovr, 0, sizeof(ovr) ); // 読み込み位置を初期化
			// 一度に hFile を使用する I/O が１つの時だけ省略可能
			// ovr.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		}
		for ( ; ; ){
			DWORD calcsize;

			switch ( datastate[readid] ){
				case data_empty: // 空の状態→読み込み開始
					if ( IsTrue(ReadFile(hFile, binbuf[readid], binsize, &readsize[readid], ovrptr)) ){
						datastate[readid] = data_have;
						if ( asyncmode ){
							AddDD(ovr.Offset, ovr.OffsetHigh, readsize[readid], 0);
							readid ^= 1;
							if ( datastate[readid] == data_empty ) continue;
						}
						break;
					}else{
						ERRORCODE result;

						result = GetLastError();
						if ( result == ERROR_HANDLE_EOF ){
							datastate[readid] = data_have;
							readsize[readid] = 0;
							break;
						}
						if ( asyncmode && (result == ERROR_IO_PENDING) ){
							datastate[readid] = data_read;
							if ( datastate[readid ^ 1] == data_have ){ // 他の作業ができる
								break;
							}
							continue;
						}
						SetCommentErrorMessage(cei, result);
						goto readbreak;
					}

				case data_read: // 読み込み中→完了しているかを確認
					// 他の作業ができるなら、待機しない
					if ( GetOverlappedResult(hFile, &ovr, &readsize[readid],
						((datastate[readid ^ 1] == data_have) ? FALSE : TRUE)) ){
						// 読み込み終了
						datastate[readid] = data_have;
						AddDD(ovr.Offset, ovr.OffsetHigh, readsize[readid], 0);
						readid ^= 1;
						if ( datastate[readid] == data_empty ) continue;
						break;
					}else{
						ERRORCODE result;

						result = GetLastError();
						if ( result == ERROR_HANDLE_EOF ){
							datastate[readid] = data_have;
							readsize[readid] = 0;
							break;
						}
						if ( (datastate[readid ^ 1] == data_have) &&
							 (result == ERROR_IO_PENDING) ){
							break; // 未完了、他の作業へ
						}
						SetCommentErrorMessage(cei, result);
						goto readbreak;
					}
				// dafault / data_have なら、他の作業をする
			}

			if ( datastate[calcid] != data_have ){ // データが無いのに来た…異常動作だが、ループさせるように。
				Sleep(10);
			}else{
				calcbin = binbuf[calcid];
				calcsize = readsize[calcid];
				switch ( type ){
					case CM_MD5:
						MD5Update(&context.md5, calcbin, calcsize);
						break;
					case CM_SHA1:
						SHA1Input(&context.sha1, calcbin, calcsize);
						break;
					case CM_SHA224:
						SHA224Input(&context.sha224, calcbin, calcsize);
						break;
					case CM_SHA256:
						SHA256Input(&context.sha256, calcbin, calcsize);
						break;
					default: // CRC32
						context.crc = crc32(calcbin, calcsize, context.crc);
						break;
				}
				if ( calcsize < binsize ){
					BYTE digest[128];

					switch ( type ){
						case CM_MD5:
							MD5Final(digest, &context.md5);
							MakeDigestStrings(digest, text, 16, T("MD5:"));
							break;
						case CM_SHA1:
							SHA1Result(&context.sha1, (uint8_t *)&digest);
							MakeDigestStrings(digest, text, SHA1HashSize, T("SHA1:"));
							break;
						case CM_SHA224:
							SHA224Result(&context.sha224, (uint8_t *)&digest);
							MakeDigestStrings(digest, text, SHA224HashSize, T("SHA224:"));
							break;
						case CM_SHA256:
							SHA256Result(&context.sha256, (uint8_t *)&digest);
							MakeDigestStrings(digest, text, SHA256HashSize, T("SHA256:"));
							break;
						default: // CRC32
							thprintf(text, TSIZEOF(text), T("CRC:%08x"), context.crc);
							break;
					}
					SetComment_CommentEnum(cei, text);
					break;
				}
				datastate[calcid] = data_empty;
				if ( asyncmode ) calcid ^= 1;
			}
			if ( IsTrue(BreakCheck(cinfo, jinfo, cei->name)) ){
				StopEnumMarkCell(cei->work);
				cei->filesptr = (TCHAR *)NilStr;
				result = ERROR_CANCELLED;
				break;
			}
		}
readbreak:
		CloseHandle(hFile);
		// if ( asyncmode ) CloseHandle(ovr.hEvent);
		jinfo->count++;
	}
	PPcHeapFree(binbuf[0]);
	if ( binbuf[1] != NULL ) PPcHeapFree(binbuf[1]);
	return result;
}

ERRORCODE CommentsMain(PPC_APPINFO *cinfo, ThSTRUCT *thEcdata, int type, const TCHAR *typename, int enummode, DWORD CommentID, TCHAR *files)
{
	JOBINFO jinfo;
	TCHAR text[VFPS + 16];
	ERRORCODE result = NO_ERROR;
	COMMENTENUMINFO cei;

	InitJobinfo(&jinfo);
	cinfo->BreakFlag = FALSE;
	cei.enummode = enummode;
	PPxCommonCommand(NULL, JOBSTATE_COMMENT, K_ADDJOBTASK);

	if ( (type != CM_RENAME) && (type != CM_CLEAR) ){
		SetCommentUseFlag(cinfo, CommentID);
	}

	if ( type >= CM_WINHASH ){
		if ( type >= CM_EXT ){
			ExtExec(cinfo, thEcdata, type - CM_EXT, CommentID);
		}else{
			TCHAR *dest;

			InitCommentEnumInfo(cinfo, &cei, CommentID, files);
			dest = thprintf(text, TSIZEOF(text), T("%s:"), typename);
			while ( IsTrue(CommentEnum(&cei)) ){
				if ( GetFileHash(cei.name, typename, dest) ){
					SetComment_CommentEnum(&cei, text);
				}
				jinfo.count++;
			}
		}
	} else switch ( type ){
		case CM_RENAME:{
			TCHAR buf[VFPS];
			TINPUT tinput;

			tinput.hOwnerWnd = cinfo->info.hWnd;
			tinput.hWtype = PPXH_GENERAL;
			tinput.hRtype = PPXH_GENERAL;
			tinput.title = CommentsMenu[0];
			tinput.buff = buf;
			tinput.size = TSIZEOF(buf);
			tinput.flag = TIEX_USEREFLINE | TIEX_USEINFO | TIEX_SINGLEREF;
			tinput.info = &cinfo->info;

			buf[0] = '\0';
			if ( CommentID <= 0 ){
				DWORD comment;

				comment = CEL(cinfo->e.cellN).comment;
				if ( comment != EC_NOCOMMENT ){
					tstplimcpy(buf, ThPointerT(&cinfo->e.Comments, comment), VFPS);
				}
			} else{
				ENTRYEXTDATASTRUCT eeds;

				eeds.id = CommentID;
				eeds.size = VFPS * sizeof(TCHAR);
				eeds.data = (BYTE *)buf;
				EntryExtData_GetDATA(cinfo, &eeds, &CEL(cinfo->e.cellN));
			}

			if ( tInputEx(&tinput) <= 0 ){
				result = ERROR_CANCELLED;
				break;
			}

			SetComment(cinfo, CommentID, &CEL(cinfo->e.cellN), buf);
			SetCommentUseFlag(cinfo, CommentID);
			break;
		}
		case CM_CRC32:		// CRC32
		case CM_SHA1:		// SHA-1
		case CM_SHA224:		// SHA-224
		case CM_SHA256:		// SHA-256
		case CM_MD5:		// MD5
			result = SetHashComment(cinfo, &cei, type, CommentID, files, &jinfo);
			break;

		case CM_FTYPE:		// FileType
			InitCommentEnumInfo(cinfo, &cei, CommentID, files);
			while ( IsTrue(CommentEnum(&cei)) ){
				char *image = NULL;
				DWORD imgsize;

				if ( LoadFileImage(cei.name, VFS_check_size, (char **)&image,
					&imgsize, LFI_ALWAYSLIMIT) ){
					SetComment_CommentEnum(&cei, T("read error"));
				} else{
					ERRORCODE typeresult;
					VFSFILETYPE vft;

					vft.flags = VFSFT_TYPETEXT;
					typeresult = VFSGetFileType(cei.name, image, imgsize, &vft);
					ProcHeapFree(image);
					if ( typeresult == ERROR_NO_DATA_DETECTED ){
						SetComment_CommentEnum(&cei, T("Unknown"));
					} else if ( typeresult != NO_ERROR ){
						SetComment_CommentEnum(&cei, T("Error"));
					} else{
						SetComment_CommentEnum(&cei, vft.typetext);
					}
				}
				if ( IsTrue(BreakCheck(cinfo, &jinfo, cei.name)) ){
					StopEnumMarkCell(cei.work);
					result = ERROR_CANCELLED;
					break;
				}
				jinfo.count++;
			}
			break;

		case CM_OWNER:		// File owner
			InitCommentEnumInfo(cinfo, &cei, CommentID, files);
			while ( IsTrue(CommentEnum(&cei)) ){
				SECURITY_DESCRIPTOR *sd;
				BYTE sdbuf[0x400];
				DWORD size;

				sd = (SECURITY_DESCRIPTOR *)sdbuf;
				if ( FALSE != GetFileSecurity(cei.name,
					OWNER_SECURITY_INFORMATION, sd, sizeof sdbuf, &size) ){
					PSID psid;
					BOOL ownflag;

					GetSecurityDescriptorOwner(sd, &psid, &ownflag);

					if ( IsTrue(ownflag) ){
						thprintf(text, TSIZEOF(text), T("Owner:<inheritance>"));
					} else{
						TCHAR oname[0x400], domain[0x400], *db = NULL;
						DWORD namesize, domainsize;
						SID_NAME_USE snu;

						namesize = TSIZEOF(oname);
						domainsize = TSIZEOF(domain);
						if ( cei.name[0] == '\\' ){
							db = FindPathSeparator(cei.name + 2);
							if ( db != NULL ) *db = '\0';
							db = cei.name;
						}
						if ( IsTrue(LookupAccountSid(db, psid, oname, &namesize,
							domain, &domainsize, &snu)) ){
							thprintf(text, TSIZEOF(text), T("Owner:%s@%s"), oname, domain);
						} else{
							tstrcpy(text, T("Owner:unknown"));
						}
					}
				} else{
					tstrcpy(text, T("Owner:error"));
				}
				SetComment_CommentEnum(&cei, text);
				if ( IsTrue(BreakCheck(cinfo, &jinfo, cei.name)) ){
					StopEnumMarkCell(cei.work);
					result = ERROR_CANCELLED;
					break;
				}
				jinfo.count++;
			}
			break;

		case CM_HARDLINKS:
			InitCommentEnumInfo(cinfo, &cei, CommentID, files);
			while ( IsTrue(CommentEnum(&cei)) ){
				HANDLE hFile;
				BY_HANDLE_FILE_INFORMATION fi;

				hFile = CreateFileL(cei.name, GENERIC_READ, FILE_SHARE_READ, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if ( hFile != INVALID_HANDLE_VALUE ){
					if ( IsTrue(GetFileInformationByHandle(hFile, &fi)) ){
						thprintf(text, TSIZEOF(text), T("hardlinks:%d"), fi.nNumberOfLinks);
						SetComment_CommentEnum(&cei, text);
					}
					CloseHandle(hFile);
				}
				if ( IsTrue(BreakCheck(cinfo, &jinfo, cei.name)) ){
					StopEnumMarkCell(cei.work);
					result = ERROR_CANCELLED;
					break;
				}
				jinfo.count++;
			}
			break;

		case CM_LINKEDPATH:
			InitCommentEnumInfo(cinfo, &cei, CommentID, files);
			tstrcpy(text, T("linked:"));
			while ( IsTrue(CommentEnum(&cei)) ){
				if ( GetReparsePath(cei.name, text + 7) ||
					 ( (tstricmp(GetPathExt(cei.name), StrShortcutExt) == 0) &&
						SUCCEEDED(GetLink(cinfo->info.hWnd, cei.name, text + 7))) ){
					SetComment_CommentEnum(&cei, text);
				}
				jinfo.count++;
			}
			break;

		case CM_INFOTIP:
			ExtInfoTip(cinfo);
			break;

		case CM_CLEAR:
			InitCommentEnumInfo(cinfo, &cei, CommentID, files);
			while ( IsTrue(CommentEnum(&cei)) ){
				if ( CommentID == 0 ){
					if ( cei.cell->comment != EC_NOCOMMENT ){
						// ※※※ガーページコレクションなどはしない
						cei.cell->comment = EC_NOCOMMENT;
						cinfo->ModifyComment = TRUE;
					}
				} else{
					SetComment_CommentEnum(&cei, NilStr);
				}
			}
			break;

		default:
			result = ERROR_CANCELLED;
	}

	if ( thEcdata->bottom != NULL ){
		GetColumnExtMenu(thEcdata, NULL, NULL, 0); // thEcdata 解放
	}
	if ( files != NULL ) ProcHeapFree(files);
	FinishJobinfo(cinfo, &jinfo, result);
	PPxCommonCommand(NULL, 0, K_DELETEJOBTASK);
	Repaint(cinfo);
	return result;
}

typedef struct {
	PPXAPPINFO info;
	PPC_APPINFO *parent;
	int work;
} COMMENTINFOSTRUCT;

DWORD_PTR USECDECL ExtractCommentFunc(COMMENTINFOSTRUCT *cis, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	switch ( cmdID ){
		case PPXCMDID_STARTENUM:	// 検索開始(マーク無しもあり)
		case PPXCMDID_STARTNOENUM:	// 検索開始(マーク無しはなし)
			uptr->enums.enumID = cis->work;
			break;

		case PPXCMDID_NEXTENUM:		// 次へ
		case PPXCMDID_ENDENUM:		//列挙終了
			uptr->enums.enumID = -1;
			return 0;

		// その他は親に任せる
		default:
			return cis->parent->info.Function(&cis->parent->info, cmdID, uptr);
	}
	return PPXA_NO_ERROR;
}

void SetExtractComment(PPC_APPINFO *cinfo, DWORD CommentID, int enummode, const TCHAR *param)
{
	COMMENTINFOSTRUCT info;
	COMMENTENUMINFO cei;
	TCHAR ExtractText[CMDLINESIZE];

	info.info = cinfo->info;
	info.info.Function = (PPXAPPINFOFUNCTION)ExtractCommentFunc;
	info.parent = cinfo;

	cei.enummode = enummode;
	InitCommentEnumInfo(cinfo, &cei, CommentID, NULL);

	for ( ;;){
		// cei->work は、CommentEnum の時点で次entryになっている
		if ( cei.enummode == COMMENTENUM_MARK ){
			info.work = cei.work;
		}
		if ( CommentEnum(&cei) == FALSE ) break;
		if ( cei.enummode == COMMENTENUM_ALL ){
			info.work = ((ENTRYINDEX *)cinfo->e.INDEXDATA.p)[cei.work - 1];
		}
		PP_ExtractMacro(cinfo->info.hWnd, &info.info, NULL, param, ExtractText, XEO_EXTRACTEXEC);
		SetComment(cinfo, CommentID, cei.cell, ExtractText);
	}
	SetCommentUseFlag(cinfo, CommentID);
	Repaint(cinfo);
}

typedef struct {
	PPC_APPINFO *cinfo;
	TCHAR *files;
	ThSTRUCT thEcdata;
	int type;
	int enummode;
	DWORD CommentID;
	TCHAR typename[32];
} CommentsThreadInfo;
const TCHAR CommentsThreadName[] = T("PPc Comments");

THREADEXRET THREADEXCALL CommentsThread(CommentsThreadInfo *cti)
{
	THREADSTRUCT threadstruct = {CommentsThreadName, XTHREAD_EXITENABLE | XTHREAD_TERMENABLE, NULL, 0, 0};

	(void)CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	PPxRegisterThread(&threadstruct);

	cti->cinfo->CommentRentry = 1;
	CommentsMain(cti->cinfo, &cti->thEcdata, cti->type, cti->typename, cti->enummode, cti->CommentID, cti->files);
	cti->cinfo->CommentRentry = 0;

	PPcAppInfo_Release(cti->cinfo);
	PPxUnRegisterThread();
	CoUninitialize();
	PPcHeapFree(cti);
	t_endthreadex(0);
}

// *comment コマンド
ERRORCODE CommentCommand(PPC_APPINFO *cinfo, const TCHAR *param)
{
	HMENU hMenu, hSubMenu;
	BOOL EnableSubThread = FALSE;
	int type = 0;
	int enummode = COMMENTENUM_MARK;
	DWORD CommentID = 0;
	const TCHAR **menus;
	ThSTRUCT thEcdata;
	TCHAR text[CMDLINESIZE];

	if ( param != NULL ){
		TCHAR pc = *param;

		if ( pc == '&' ){
			EnableSubThread = TRUE;
			pc = *(++param);
		}

		if ( Isdigit(pc) ){
			CommentID = GetNumber(&param);
			if ( CommentID ) CommentID = DFC_COMMENTEX_MAX - (CommentID - 1);
			if ( (pc = SkipSpace(&param)) == ',' ) pc = *(++param);
		}

		if ( pc == 'a' ){ // all 指定
			enummode = COMMENTENUM_ALL;
			while ( *(++param) == 'l' );
			pc = SkipSpace(&param);
		}

		switch ( pc ){
			case '\"':
				GetLineParamS(&param, text, TSIZEOF(text));
				SetComment(cinfo, CommentID, &CEL(cinfo->e.cellN), text);
				SetCommentUseFlag(cinfo, CommentID);
				Repaint(cinfo);
				return NO_ERROR;

			case 'c':
				switch ( *(param + 1) ){
					case 'r': // crc32
						type = CM_CRC32;
						break;
					case 'l':
						type = CM_CLEAR;
						break;
				}
				break;

			case 'e':
				switch ( *(param + 1) ){
					case 'd':
						type = CM_RENAME;
						break;
					case 'x':
						while ( Isalpha(*param) ) param++;
						if ( SkipSpace(&param) == ',' ) param++;
						GetLineParamS(&param, text, TSIZEOF(text));
						SetExtractComment(cinfo, CommentID, enummode, text);
						return NO_ERROR;
				}
				break;

			case 'l':
				type = CM_LINKEDPATH;
				break;

			case 'm':
				type = CM_MD5;
				break;

			case 's':
				type = CM_SHA1;
				if ( tstrlen(param) >= 6 ){
					if ( *(param + 4) == '2' ) type = CM_SHA224;
					if ( *(param + 4) == '5' ) type = CM_SHA256;
				}
				break;

			case 'h':
				type = CM_HARDLINKS;
				break;

			case 'f':
				type = CM_FTYPE; // filetype
				break;

			case 'o':
				type = CM_OWNER;
				break;

			case 'i':
				type = CM_INFOTIP;
				break;
		}
	}

	if ( cinfo->CommentRentry ){
		SetPopMsg(cinfo, POPMSG_MSG, MES_EACO);
		return ERROR_SHARING_PAUSED;
	}

	ThInit(&thEcdata);
	text[0] = '\0';
	if ( type == 0 ){
		hMenu = CreatePopupMenu();
		menus = CommentsMenu;
		for ( type = 0; *menus != NULL; type++, menus++ ){
			AppendMenuString(hMenu, type + 1, *menus);
		}
		hSubMenu = CreatePopupMenu();
		GetColumnExtMenu(&thEcdata, cinfo->RealPath, hSubMenu, CM_EXT);
		AppendMenu(hMenu, MF_EPOP, (UINT_PTR)hSubMenu, MessageText(MES_TCLE));
		type = CM_WINHASH;
		hSubMenu = PP_AddMenu(&cinfo->info, cinfo->info.hWnd, NULL,
			(DWORD *)&type, T("M?winhashlist"), &thEcdata);
		if ( hSubMenu != NULL ){
			AppendMenu(hMenu, MF_EPOP, (UINT_PTR)hSubMenu, T("bcrypt"));
		}
		if ( CompareHashFromClipBoard(cinfo, 0) != 0 ){
			AppendMenuString(hMenu, CM_CMPHASH, StrMesCMPH);
		}
		AppendMenuString(hMenu, CM_CLEAR, MES_TCLR);

		type = PPcTrackPopupMenu(cinfo, hMenu);
		if ( (type >= CM_WINHASH) && (type < CM_WINHASH_MAX) ){
			GetMenuString(hMenu, type, text, TSIZEOF(text), MF_BYCOMMAND);
		}
		DestroyMenu(hMenu);
		if ( type == 0 ) return ERROR_CANCELLED;
	}

	if ( type == CM_CMPHASH ){
		CompareHashFromClipBoard(cinfo, 1);
		return NO_ERROR;
	}

	if ( ((param == NULL) || EnableSubThread) &&
		(type > CM_RENAME) && (type != CM_CLEAR) &&
		(type != CM_INFOTIP) && (type < CM_EXT)
		#ifndef _WIN64
		&& (OSver.dwMajorVersion >= 5) // 次の行に続く
		#endif
		){
		CommentsThreadInfo *cti;
		HANDLE hThread;
		TCHAR *files = GetFiles(cinfo, GETFILES_FULLPATH | GETFILES_REALPATH | GETFILES_DATAINDEX);

		if ( files == NULL ){
			XMessage(cinfo->info.hWnd, NULL, XM_FaERRd, StrTagetListError);
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		cti = (CommentsThreadInfo *)PPcHeapAlloc(sizeof(CommentsThreadInfo));
		cti->cinfo = cinfo;
		cti->files = files;
		cti->thEcdata = thEcdata;
		cti->type = type;
		cti->enummode = COMMENTENUM_FILES;
		cti->CommentID = CommentID;
		tstrcpy(cti->typename, text);

		PPcAppInfo_AddRef(cinfo);
		hThread = t_beginthreadex(NULL, 0,
				(THREADEXFUNC)CommentsThread, (void *)cti, 0, NULL);
		if ( hThread != NULL ){
			SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
			CloseHandle(hThread);
			return NO_ERROR;
		}else{
			PPcAppInfo_Release(cinfo);
		}
		ProcHeapFree(files);
	}
	return CommentsMain(cinfo, &thEcdata, type, text, enummode, CommentID, NULL);
}

void FileInfo(PPC_APPINFO *cinfo)
{
	ThSTRUCT text;

	MakeFileInformation(cinfo, &text, &CEL(cinfo->e.cellN));
	PPEui(cinfo->info.hWnd, MES_TEIF, (TCHAR *)text.bottom);
	ThFree(&text);
}

void SyncFileInfo(PPC_APPINFO *cinfo)
{
	if ( (cinfo->hSyncInfoWnd != NULL) && IsWindow(cinfo->hSyncInfoWnd) ){
		SendMessage(cinfo->hSyncInfoWnd, WM_CLOSE, 0, 0);
		cinfo->hSyncInfoWnd = NULL;
		SetPopMsg(cinfo, POPMSG_MSG, MES_SIOF);
	} else{
		ThSTRUCT text;

		MakeFileInformation(cinfo, &text, &CEL(cinfo->e.cellN));
		cinfo->hSyncInfoWnd = PPEui(cinfo->info.hWnd, MES_TSEI, (TCHAR *)text.bottom);
		ThFree(&text);
		SetPopMsg(cinfo, POPMSG_MSG, MES_SION);
		SetForegroundWindow(cinfo->info.hWnd);
	}
}

typedef struct {
	LOADSETTINGS_TMP ls;
	int mode;
	TCHAR *itemptr;
	TCHAR *findpath;
	TCHAR *finditem;
} FDSOS;

void FindDirSettingOne(FDSOS *fds, const TCHAR *path, int findmode)
{
	fds->itemptr[0] = '\0';
	LoadSettingMain(&fds->ls, path);
	if ( fds->itemptr[0] != '\0' ){
		tstrcpy(fds->findpath, path);
		tstrcpy(fds->finditem, fds->itemptr);
		fds->mode = findmode;
	}
}

int FindDirSetting(PPC_APPINFO *cinfo, int type, TCHAR *findpath, TCHAR *finditem)
{
	FDSOS fds;
	TCHAR path[VFPS], *p;

	fds.mode = DSMD_NOMODE;
	fds.findpath = findpath;
	fds.finditem = finditem;
	switch ( type ){
		case ITEMSETTING_MASK:
			fds.itemptr = fds.ls.mask;
			break;
		case ITEMSETTING_CMD:
			fds.itemptr = fds.ls.cmd;
			break;
		default: /* case ITEMSETTING_DISP: */
			fds.itemptr = fds.ls.disp;
			break;
	}

	// 全指定
	FindDirSettingOne(&fds, T("*"), DSMD_PATH_BRANCH);

	// ドライブ限定
	tstrcpy(path, cinfo->path);
	p = VFSGetDriveType(path, NULL, NULL);
	if ( p != NULL ){
		TCHAR backup;

		if ( *p == '\\' ) p++;
		backup = *p;
		*p = '\0';
		FindDirSettingOne(&fds, path, DSMD_PATH_BRANCH);
		*p = backup;
	} else{
		p = path;
	}
	// 下層パス
	if ( *p != '\0' ){
		for ( ; ; ){	// 途中
			TCHAR backup, *q;

			q = FindPathSeparator(p);
			if ( q == NULL ) break;
			p = q + 1;
			backup = *p;
			*p = '\0';

			FindDirSettingOne(&fds, path, DSMD_PATH_BRANCH);
			*p = backup;
		}
		// 最終層の更に下層指定
		p += tstrlen(p);
		*p = '\\';
		*(p + 1) = '\0';

		FindDirSettingOne(&fds, path, DSMD_THIS_BRANCH);

		// 最終層
		*p = '\0';
		FindDirSettingOne(&fds, path, DSMD_THIS_PATH);
	}

	// Archive
	if ( (cinfo->e.Dtype.mode == VFSDT_UN) ||
		(cinfo->e.Dtype.mode == VFSDT_SUSIE) ||
		(cinfo->e.Dtype.mode == VFSDT_ARCFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_CABFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_LZHFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_ZIPFOLDER) ){
		FindDirSettingOne(&fds, StrArchiveMode, DSMD_ARCHIVE);
	}

	// Listfile
	if ( cinfo->e.Dtype.mode == VFSDT_LFILE ){
		FindDirSettingOne(&fds, StrListfileMode, DSMD_LISTFILE);
	}
	return fds.mode;
}

BOOL IsSameDispFormat(XC_CFMT *cfmt, BYTE *targetformat, size_t fmtsize)
{
	BYTE *fmt;

	if ( fmtsize < 5 ) return FALSE;

	// 可変桁数の補正
	fmt = targetformat + 4;
	for ( ; ; ){
		if ( (*fmt == DE_WIDEV) || (*fmt == DE_WIDEW) ){
			int nameLen, cellminLen;

			cellminLen = (int)*(WORD *)targetformat - fmt[fmt[3]];
			nameLen = cfmt->width - cellminLen;

			if ( nameLen < fmt[1] )	nameLen = fmt[1];
			if ( nameLen > 255 )	nameLen = 255;
			*(WORD *)targetformat = (WORD)(cellminLen + nameLen);
			fmt[fmt[3]] = (BYTE)nameLen;
		}
		if ( *(WORD *)(fmt - 2) == 0 ) break;
		fmt += *(WORD *)(fmt - 2); // 次の行をチェックする
	}
	return !memcmp(cfmt->fmtbase + 2, targetformat + 2, fmtsize - 2);
}

void AddPPcCellDisplayMenu(PPC_APPINFO *cinfo, HMENU hMenu, int *rmode, DWORD *rfmtsize, const TCHAR *optpath)
{
	int id, mode;
	TCHAR path[VFPS], sname[CUST_NAME_LENGTH];
	BYTE temp[0x8000];
	DWORD fmtsize;
	BOOL found = FALSE;

	if ( cinfo->celF.fmtbase == NULL ){
		fmtsize = 0;
		found = TRUE;
	} else{
		fmtsize = HeapSize(hProcessHeap, 0, cinfo->celF.fmtbase);
	}
	for ( id = 0; ; id++ ){
		if ( 0 > EnumCustTable(id, T("MC_celS"), sname, temp, sizeof(temp)) ) break;
		PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, sname, sname, 0);
		if ( IsSameDispFormat(&cinfo->celF, temp, fmtsize) ){
			AppendMenu(hMenu, MF_ES | MF_CHECKED, id + CRID_VIEWFORMAT, sname);
			found = TRUE;
		} else{
			AppendMenuString(hMenu, id + CRID_VIEWFORMAT, sname);
		}
	}

	mode = FindDirSetting(cinfo, ITEMSETTING_DISP, path, sname) + CRID_VIEWFORMATEX;
	if ( mode == CRID_VIEWFORMAT_NOMODE ){
		GetCustData(Str_X_dsst, &X_dsst, sizeof(X_dsst));
		mode = X_dsst[0] + CRID_VIEWFORMATEX;
	}

	AppendMenuPopString(hMenu, DispPopStringList1);
	if ( !found ) AppendMenuString(hMenu, CRID_VIEWFORMAT_SAVE, StrDsetSave);
	if ( 0 <= FindCellFormatImagePosition(cinfo->celF.fmt) ){
		AppendMenuPopString(hMenu, DispPopStringList2);
	}
	if ( (rmode != NULL) && (*rmode == CRID_VIEWFORMAT_TEMP) ){
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenuString(hMenu, CRID_VIEWFORMAT_TEMP, StrDsetTemporality);
		AppendMenuPopString(hMenu, DispPopStringList3 + 1);
	} else{
		AppendMenuPopString(hMenu, DispPopStringList3);
	}

	if ( optpath != NULL ){
		tstrcpy(path, optpath);
	}else if ( mode != CRID_VIEWFORMAT_PATH_BRANCH ){
		path[0] = '*';
		path[1] = '\0';
	}
	AppendMenuString(hMenu, CRID_VIEWFORMAT_PATH_BRANCH, path);

	if ( (cinfo->e.Dtype.mode == VFSDT_LFILE) ||
		 (cinfo->e.Dtype.mode == VFSDT_UN) ||
		 (cinfo->e.Dtype.mode == VFSDT_SUSIE) ||
		(cinfo->e.Dtype.mode == VFSDT_ARCFOLDER) ||
		 (cinfo->e.Dtype.mode == VFSDT_CABFOLDER) ||
		 (cinfo->e.Dtype.mode == VFSDT_LZHFOLDER) ||
		 (cinfo->e.Dtype.mode == VFSDT_ZIPFOLDER) ||
		 ((rmode != NULL) && (*rmode == CRID_VIEWFORMAT_ARCHIVE)) ||
		 ((rmode != NULL) && (*rmode== CRID_VIEWFORMAT_LISTFILE)) ){
		if ( (cinfo->e.Dtype.mode == VFSDT_LFILE) || ((rmode != NULL) && (*rmode == CRID_VIEWFORMAT_LISTFILE)) ){
			AppendMenuString(hMenu, CRID_VIEWFORMAT_LISTFILE, StrDsetListfile);
			if ( (rmode != NULL) && (*rmode == CRID_VIEWFORMAT_ARCHIVE) ){
				AppendMenuString(hMenu, CRID_VIEWFORMAT_ARCHIVE, StrDsetArchive);
			}
		}else{
			AppendMenuString(hMenu, CRID_VIEWFORMAT_ARCHIVE, StrDsetArchive);
		}
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenuCheckString(hMenu, CRID_VIEWFORMAT_PATHMASK,
				StrDsetDirmode, cinfo->UseArcPathMask);
		if ( cinfo->UseArcPathMask == ARCPATHMASK_OFF ){
			AppendMenuCheckString(hMenu, CRID_VIEWFORMAT_SPLITPATH,
				StrDsetPathSeparate, cinfo->UseSplitPathName);
		}
	}
	CheckMenuRadioItem(hMenu, CRID_VIEWFORMAT_THIS_PATH, CRID_VIEWFORMAT_MODELAST - 1, mode, MF_BYCOMMAND);
	if ( rmode != NULL ){
		*rmode = mode;
		*rfmtsize = fmtsize;
	}
}

ERRORCODE SetCellDisplayFormat(PPC_APPINFO *cinfo, int selectindex, const TCHAR *path)
{
	int index, mode = selectindex;
	TCHAR sname[CUST_NAME_LENGTH];
	// , txt[VFPS];
	HMENU hMenu;
	ERRORCODE result = NO_ERROR;
	DWORD fmtsize;

	if ( (selectindex != CRID_VIEWFORMAT_PATH_BRANCH) ||
		 ( (path != NULL) && (path[0] == '\0')) ){
		path = NULL;
	}
	hMenu = CreatePopupMenu();
	AddPPcCellDisplayMenu(cinfo, hMenu, &mode, &fmtsize, path);
	for ( ; ; ){
		MENUITEMINFO minfo;

		minfo.cbSize = sizeof(minfo);
		minfo.fMask = MIIM_STATE;
		minfo.fState = (mode != CRID_VIEWFORMAT_REGID) ? MFS_ENABLED : MFS_GRAYED;
		SetMenuItemInfo(hMenu, CRID_VIEWFORMAT_CANCEL, MF_BYCOMMAND, &minfo);
		if ( selectindex ){
			index = selectindex;
			selectindex = 0;
		} else{
/*
			sname[0] = '\0';
			switch( mode ){ // モード切替
				case CRID_VIEWFORMAT_TEMP:
					break;
				case CRID_VIEWFORMAT_REGID:
					break;
				case CRID_VIEWFORMAT_THIS_PATH:
				case CRID_VIEWFORMAT_THIS_BRANCH:
				case CRID_VIEWFORMAT_PATH_BRANCH:
				case CRID_VIEWFORMAT_ARCHIVE:
					GetSetPath(cinfo, sname, mode, CRID_VIEWFORMAT, hMenu);
					break;
			}
			minfo.fMask = MIIM_STATE | MIIM_TYPE;
			minfo.dwTypeData = txt;
			minfo.cch = VFPS;
			Message(sname);
			for ( index = CRID_VIEWFORMAT ; ; index++ ){
				minfo.fMask = MIIM_STATE | MIIM_TYPE;
				if ( GetMenuItemInfo(hMenu, index, MF_BYCOMMAND, &minfo) == FALSE ){
					break;
				}
				if ( tstrcmp(sname, txt) == 0 ){
					minfo.fMask = MIIM_STATE;
					minfo.fState = MFS_ENABLED | MFS_CHECKED;

					SetMenuItemInfo(hMenu, index, MF_BYCOMMAND, &minfo);
					break;
				}else{
					if ( minfo.fState & MFS_CHECKED ){
						minfo.fMask = MIIM_STATE;
						minfo.fState = MFS_CHECKED;
						SetMenuItemInfo(hMenu, index, MF_BYCOMMAND, &minfo);
					}
				}
			}
*/
			index = PPcTrackPopupMenu(cinfo, hMenu);
		}
		if ( (index < CRID_VIEWFORMAT_TEMP) ||
			(index >= CRID_VIEWFORMAT_MODELAST) ){
			break;
		}
		mode = index;
		CheckMenuRadioItem(hMenu, CRID_VIEWFORMAT_TEMP, CRID_VIEWFORMAT_MODELAST - 1, mode, MF_BYCOMMAND);
	}
	if ( index == CRID_VIEWFORMAT_EDIT ){
		if ( fmtsize ){
			SetCustTable(T("MC_celS"), MC_CELS_TEMPNAME, cinfo->celF.fmtbase, fmtsize);
		}
		PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, T("%Obqs *ppcust /:"), NULL, 0);
		if ( IsExistCustTable(T("MC_celS"), MC_CELS_TEMPNAME) ){
			BYTE *fmtwork = NULL;
			BOOL nffix;

			FreeCFMT(&cinfo->celF);
			nffix = LoadCelFFMT(cinfo, T("MC_celS"), MC_CELS_TEMPNAME);
			if ( cinfo->celF.fmtbase != NULL ){
				fmtwork = PPcHeapAlloc(HeapSize(hProcessHeap, 0, cinfo->celF.fmtbase));
				if ( fmtwork != NULL ){
					memcpy(fmtwork, cinfo->celF.fmtbase, fmtsize);
				}
			}
			DeleteCustTable(T("MC_celS"), MC_CELS_TEMPNAME, 0);
			FixCellDisplayFormat(cinfo);
			UpdateWindow_Part(cinfo->info.hWnd); // 表示させて、異常がないか確認
			if ( fmtwork != NULL ){
				SetCustTable(T("XC_celF"), cinfo->RegCID + 1, fmtwork, fmtsize);
				PPcHeapFree(fmtwork);
			}
			cinfo->FixcelF = FALSE;
			if ( nffix ) LoadCelFFMTfix(cinfo);
		}
	} else if ( index == CRID_VIEWFORMAT_SAVE ){
		TCHAR buf1[CMDLINESIZE];

		tstrcpy(sname, T("tempname"));
		if ( PPctInput(cinfo, MES_SAVE, sname, TSIZEOF(sname),
			PPXH_GENERAL, PPXH_GENERAL) > 0 ){
			if ( (sname[0] != '\0') && (cinfo->celF.fmtbase != NULL) ){
				SetCustTable(T("MC_celS"), sname, cinfo->celF.fmtbase, fmtsize);
				// 桁数計算をやり直しさせる
				thprintf(buf1, TSIZEOF(buf1), T("*setcust MC_celS:%s=%%*getcust(\"MC_celS:%s\")"), sname, sname);
				PP_ExtractMacro(NULL, NULL, NULL, buf1, NULL, 0);
			}
		}
	} else if ( index == CRID_VIEWFORMAT_MAG_THUMBS ){
		SetMag(cinfo, 10);
	} else if ( index == CRID_VIEWFORMAT_MINI_THUMBS ){
		SetMag(cinfo, -10);
	} else if ( index == CRID_VIEWFORMAT_SPLITPATH ){
		cinfo->UseSplitPathName = !cinfo->UseSplitPathName;
		InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
	} else if ( index == CRID_VIEWFORMAT_PATHMASK ){
		if ( cinfo->UseArcPathMask == ARCPATHMASK_OFF ){
			cinfo->UseArcPathMask = ARCPATHMASK_ARCHIVE; // 階層有効・生成
			cinfo->ArcPathMask[0] = '\0';
			MaskPathMain(cinfo, cinfo->ArcPathMaskDepth, NilStr);
		} else{
/*
			ENTRYDATAOFFSET offset;

			for ( offset = cinfo->e.cellDMax - 1 ; offset >= 0 ; offset-- ){
				if ( CELdata(offset).f.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY ){
					if ( offset == cinfo->e.cellDMax - 1 ){
						cinfo->e.cellDMax--; // 不要dirなので削除
					}else{
						// 削除できないので非表示
					}
				}
			}

			cinfo->UseSplitPathName = FALSE;
			cinfo->UseArcPathMask = ARCPATHMASK_OFF;
			cinfo->ArcPathMask[0] = '\0';
			MaskEntryMain(cinfo, &cinfo->mask, NilStr);
*/
			read_entry(cinfo, RENTRY_NOUSEPATHMASK); // 階層無効・再読み込み
		}
	} else if ( ((index >= CRID_VIEWFORMAT) && (index < CRID_VIEWFORMATEX)) || (index == CRID_VIEWFORMAT_CANCEL) ){
		DWORD size;
		BOOL nzfix;

		FreeCFMT(&cinfo->celF);
		if ( index != CRID_VIEWFORMAT_CANCEL ){ //名前取得
			EnumCustTable(index - CRID_VIEWFORMAT, T("MC_celS"), sname, NULL, 0);
			nzfix = LoadCelFFMT(cinfo, T("MC_celS"), sname);
		} else{
			sname[0] = '\0';
			nzfix = LoadCelFFMT(cinfo, T("XC_celF"), cinfo->RegCID + 1);
		}

		if ( mode == CRID_VIEWFORMAT_REGID ){ // 窓別設定・保持
			if ( (sname[0] != '\0') && (cinfo->celF.fmtbase != NULL) ){ // 切り替えたので保存
				size = GetCustTableSize(T("MC_celS"), sname);
				SetCustTable(T("XC_celF"), cinfo->RegCID + 1, cinfo->celF.fmtbase, size);
				cinfo->FixcelF = FALSE;
			}
		} else if ( mode == CRID_VIEWFORMAT_TEMP ) {
			cinfo->FixcelF = TRUE;
		} else {	// パス指定設定
			TCHAR setpath[VFPS];

			GetSetPath(cinfo, setpath, mode, CRID_VIEWFORMATEX, hMenu);
			SetNewXdir(setpath, LOADDISPSTR, sname);
			cinfo->FixcelF = TRUE;
		}
		FixCellDisplayFormat(cinfo);
		if ( nzfix ) LoadCelFFMTfix(cinfo);
	} else{
		result = ERROR_CANCELLED;
	}
	DestroyMenu(hMenu);
	return result;
}

void FixCellDisplayFormat(PPC_APPINFO *cinfo)
{
	ClearCellIconImage(cinfo);

	if ( cinfo->celF.fmt[0] == DE_WIDEV ) FixCellWideV(cinfo);
	if ( cinfo->hHeaderWnd != NULL ) FixHeader(cinfo);
	InitCli(cinfo);
	Repaint(cinfo);
	if ( XC_awid ) FixWindowSize(cinfo, 0, 0);
	MoveCellCsr(cinfo, 0, &XC_mvUD);
}

const TCHAR PairHasEntry[] = T("/bootid:%c \"%s\" /noactive /k *jumppath \"%s\" /entry /nolock");
const TCHAR PairCommand[] = T("/bootid:%c \"%s\" /noactive");

BOOL SetPairPath(PPC_APPINFO *cinfo, const TCHAR *path, const TCHAR *entry)
{
	COPYDATASTRUCT copydata;
	HWND nhWnd;
	TCHAR fullpath[CMDLINESIZE];
	int swapmode = 0;

	if ( path == NULL ){
		if ( entry == NilStr ) swapmode = 0x100;
		path = cinfo->path;
		if ( (cinfo->e.Dtype.mode == VFSDT_LFILE) && (cinfo->e.Dtype.BasePath[0] != '\0') ){
			path = cinfo->e.Dtype.BasePath;
		}
		entry = CEL(cinfo->e.cellN).f.cFileName;
		if ( *entry == FINDOPTION_LONGNAME ){
			const TCHAR *longname = (const TCHAR *)EntryExtData_GetDATAptr(cinfo, DFC_LONGNAME, &CEL(cinfo->e.cellN));
			if ( longname != NULL ) entry = longname;
		}
	}

	nhWnd = GetPairWnd(cinfo);
	if ( nhWnd == NULL ){
		if ( swapmode == 0 ){
			thprintf(fullpath, TSIZEOF(fullpath),
					*entry ? PairHasEntry : PairCommand,
					(TCHAR)(((cinfo->RegID[2] - 1) ^ 1) + 1), path, entry);
			PPCui(cinfo->info.hWnd, fullpath);
			return TRUE;
		}
		return FALSE;
	}

	if ( swapmode ||
		(cinfo->e.Dtype.mode != VFSDT_LFILE) ||
		(CEL(cinfo->e.cellN).attr & (ECA_PARENT | ECA_THIS)) ){
		thprintf(fullpath, TSIZEOF(fullpath), T("%s\1%s"), path, entry);
	} else{
		TCHAR *p;

		p = VFSFullPath(fullpath, (TCHAR *)entry, path);
		if ( (p > fullpath) && (*(p - 1) == '\\') ) *(p - 1) = '\1';
	}
	copydata.dwData = '=' + swapmode;
	copydata.cbData = TSTRSIZE32(fullpath);
	copydata.lpData = fullpath;
	SendMessage(nhWnd, WM_COPYDATA, (WPARAM)cinfo->info.hWnd, (LPARAM)&copydata);
	return TRUE;
}

void SetMag(PPC_APPINFO *cinfo, int offset)
{
	int fmtoffset;

	if ( 0 <= (fmtoffset = FindCellFormatImagePosition(cinfo->celF.fmt)) ){
		BYTE *fmt;

		fmt = cinfo->celF.fmt + fmtoffset + 1; // +0:X +1:Y
		if ( offset == 0 ){

		}else if ( offset > 0 ){
			if ( (cinfo->celF.width >= 230) || (cinfo->celF.height >= 230) ||
				(fmt[0] >= 200) || (fmt[1] >= 200) ){
				return;
			}
			cinfo->celF.width += 2;
			cinfo->celF.height += 1;
			fmt[0] += (BYTE)2;
			fmt[1] += (BYTE)1;
		} else{
			if ( (cinfo->celF.width <= 3) || (cinfo->celF.height <= 2) ||
				(fmt[0] <= 3) || (fmt[1] <= 2) ){
				if ( ((fmt[2] == 0) || (fmt[2] == DE_BLANK)) &&
					(fmt[0] > 2) && (fmt[1] > 2) && (cinfo->celF.width > 2) ){
					fmt[0] -= (BYTE)2;
					fmt[1] -= (BYTE)1;
					cinfo->celF.width -= 2;
				} else{
					return;
				}
			} else{
				cinfo->celF.width -= 2;
				cinfo->celF.height -= 1;
				fmt[0] -= (BYTE)2;
				fmt[1] -= (BYTE)1;
			}
		}
	} else{
		if ( offset == 0 ) cinfo->X_textmag = 100;
		cinfo->X_textmag += offset;
		if ( cinfo->X_textmag <= 0 ){
			cinfo->X_textmag = 10;
			return;
		}
		if ( cinfo->X_textmag > 9990 ){
			cinfo->X_textmag = 9990;
			return;
		}
		DeleteObject(cinfo->hBoxFont);
		InitFont(cinfo);
	}
	ClearCellIconImage(cinfo);

	WmWindowPosChanged(cinfo);
	InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
};

BOOL OpenClipboardCheck(PPC_APPINFO *cinfo)
{
	int trycount = 6;

	for (;;){
		if ( IsTrue(OpenClipboard(cinfo->info.hWnd)) ) return TRUE;
		if ( --trycount != 0 ) {
			Sleep(20);
			continue;
		}
		SetPopMsg(cinfo, POPMSG_MSG, T("Clipboard open error"));
		return FALSE;
	}
}

void ClipFiles(PPC_APPINFO *cinfo, DWORD effect, DWORD cliptypes)
{
	UINT CF_xDROPEFFECT C4701CHECK;	// DropEffect のクリップボードID

	if ( OpenClipboardCheck(cinfo) == FALSE ) return;
	EmptyClipboard(); // ※ WM_DESTROYCLIPBOARD が実行され、CLIPDATAS初期化

	if ( cliptypes & CFT_DnD ){
		CF_FILENAMEW = RegisterClipboardFormat(CFSTR_FILENAMEW);
		CF_xDROPEFFECT = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);

		if ( (cinfo->CLIPDATAS[CLIPTYPES_DROPEFFECT] = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD))) != NULL ){
			*(DWORD *)GlobalLock(cinfo->CLIPDATAS[CLIPTYPES_DROPEFFECT]) = effect;
			GlobalUnlock(cinfo->CLIPDATAS[CLIPTYPES_DROPEFFECT]);
		}

		cinfo->CLIPDATAS[CLIPTYPES_HDROP] = CreateHDrop(cinfo);
		if ( cinfo->CLIPDATAS[CLIPTYPES_HDROP] != NULL ){
			SetClipboardData(CF_HDROP, cinfo->CLIPDATAS[CLIPTYPES_HDROP]);
		}
	}

	if ( cliptypes & CFT_SHN ){
		CF_xSHELLIDLIST = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
		cinfo->CLIPDATAS[CLIPTYPES_SHN] = CreateShellIdList(cinfo);
		SetClipboardData(CF_xSHELLIDLIST, NULL);
	}

	if ( cliptypes & CFT_TEXT ){
		cinfo->CLIPDATAS[CLIPTYPES_TEXT] = CreateHText(cinfo);
/*
		if ( FindWindow(T("CabinetWClass"), NULL) != NULL ){
			SetClipboardData(CF_FILENAMEW, NULL);
		}
*/
		SetClipboardData(CF_TTEXT, NULL);
	}
	if ( cliptypes & CFT_DnD ){
		SetClipboardData(CF_xDROPEFFECT, cinfo->CLIPDATAS[CLIPTYPES_DROPEFFECT]);  // C4701ok
	}
	CloseClipboard();
}

void ClipDirectory(PPC_APPINFO *cinfo)
{
	TMS_struct files = {{NULL, 0, NULL}, 0};
	HGLOBAL hClip;

	TMS_reset(&files);
	TMS_set(&files, cinfo->path);
	TMS_off(&files);
	hClip = GlobalReAlloc(files.tm.h, files.p, GMEM_MOVEABLE);
	if ( hClip == NULL ) hClip = files.tm.h;
	if ( OpenClipboardCheck(cinfo) == FALSE ) return;
	EmptyClipboard();
	SetClipboardData(CF_TTEXT, hClip);
	CloseClipboard();
	SetPopMsg(cinfo, POPMSG_MSG, MES_CPPN);
}

void OpenFileWithPPe(PPC_APPINFO *cinfo)
{
	TCHAR name[VFPS];

	if ( (cinfo->e.Dtype.mode == VFSDT_SUSIE) ||
		(cinfo->e.Dtype.mode == VFSDT_UN) ){
		PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, T("*execinarc %: *ppe %FDC"), NULL, 0);
		return;
	}

	{
		const TCHAR *path, *entryname;

		path = cinfo->path;
		if ( (cinfo->e.Dtype.mode == VFSDT_LFILE) && (cinfo->e.Dtype.BasePath[0] != '\0') ){
			path = cinfo->e.Dtype.BasePath;
		}
		entryname = CEL(cinfo->e.cellN).f.cFileName;
		if ( *entryname == FINDOPTION_LONGNAME ){
			const TCHAR *longname = (const TCHAR *)EntryExtData_GetDATAptr(cinfo, DFC_LONGNAME, &CEL(cinfo->e.cellN));
			if ( longname != NULL ) entryname = longname;
		}
		VFSFullPath(name, (TCHAR *)entryname, path);
	}

	if ( cinfo->e.Dtype.mode == VFSDT_SHN ){
		TCHAR realname[VFPS];
		if ( IsTrue(VFSGetRealPath(cinfo->info.hWnd, realname, name)) ){
			tstrcpy(name, realname);
		}
	}

	PPEui(cinfo->info.hWnd, name, NULL);
}

void OpenExplorer(PPC_APPINFO *cinfo)
{
	TCHAR cmdline[CMDLINESIZE], path[VFPS];

	if ( cinfo->e.Dtype.mode != VFSDT_PATH ){
		const TCHAR *verb;

		if ( OSver.dwMajorVersion >= 10 ){
			verb = StrInvokeExplorer4; // Win10以降
		} else if ( (OSver.dwMajorVersion > 6) ||
			((OSver.dwMajorVersion == 6) && (OSver.dwMinorVersion > 0)) ){
			verb = StrInvokeExplorer3; // Win7以降
		} else if ( (OSver.dwMajorVersion >= 5) ||
			((OSver.dwMajorVersion == 4) && (OSver.dwMinorVersion >= 90)) ){
			verb = StrInvokeExplorer2; // 4.90以降
		} else{
			verb = StrInvokeExplorer1; // 98/NT4まで
		}
		if ( IsTrue(PPcSHContextMenu(cinfo, StrThisDir, verb)) ) return;
	}
	if ( CEL(cinfo->e.cellN).attr & (ECA_PARENT | ECA_THIS) ){
		thprintf(cmdline, TSIZEOF(cmdline), T("explorer /n,/e,%s"), cinfo->path);
	}else{
		VFSFullPath(path, (TCHAR *)GetCellFileName(cinfo, &CEL(cinfo->e.cellN), path), cinfo->path);
		thprintf(cmdline, TSIZEOF(cmdline), T("explorer /n,/e,/select,%s"), path);
	}
	ComExec(cinfo->info.hWnd, cmdline, cinfo->path);
}

BOOL MaskEntryMain(PPC_APPINFO *cinfo, XC_MASK *maskorg, const TCHAR *filename)
{
	DWORD AttrMask, DirMask, result;
	XC_MASK mask;
	FN_REGEXP fn;
	ENTRYDATAOFFSET index;
	TCHAR maskbuf[CMDLINESIZE];

	size_t asize, alen;
	alen = tstrlen(cinfo->ArcPathMask);
	if ( alen != 0 ){ // 通常は C:\ABC / C: になるように加工する
		TCHAR *sep;

		sep = FindPathSeparator(cinfo->ArcPathMask);
		if ( sep != NULL ){
			if ( *(sep + 1) == '\0' ){ // C:\ → C: に加工
				*sep = '\0';
				alen--;
			}
		}
	}
	asize = TSTROFF(alen);

	mask = *maskorg;
	AttrMask = mask.attr & MASKEXTATTR_MASK;
	tstrcpy(tstpcpy(maskbuf, T("o:e,")), mask.file);
	result = MakeFN_REGEXP(&fn, maskbuf);
	if ( result & REGEXPF_ERROR ) return FALSE;
	DirMask = ((result & REGEXPF_PATHMASK) || (mask.attr & MASKEXTATTR_DIR)) ?
			0 : FILE_ATTRIBUTE_DIRECTORY;

	cinfo->MarkMask = MARKMASK_DIRFILE;
	cinfo->e.cellIMax = 0;
	cinfo->e.RelativeDirs = 0;
	cinfo->e.Directories = 0;
	cinfo->e.cellStack = 0;
	for ( index = 0; index < cinfo->e.cellDataMax; index++ ){
		ENTRYCELL *cell;

		cell = &CELdata(index);

		// 書庫内仮想ディレクトリ用のマスク
		if ( (cinfo->UseArcPathMask != ARCPATHMASK_OFF) &&
			!(cell->attr & (ECA_PARENT | ECA_THIS)) ){
			if ( cell->f.dwFileAttributes & FILE_ATTRIBUTEX_FOLDER ){
				ResetMark(cinfo, cell);
				continue;
			}
			if ( asize && (memcmp(cell->f.cFileName, cinfo->ArcPathMask, asize) ||
				((cell->f.cFileName[alen] != '\\') && (cell->f.cFileName[alen] != '/')) ||
				(cell->f.cFileName[alen + 1] == '\0')) ){
				ResetMark(cinfo, cell);
				continue;
			}
			if ( FindPathSeparator(cell->f.cFileName + alen + 1) != NULL ){
				ResetMark(cinfo, cell);
				continue;
			}
		}

		if ( !(cell->attr & rdirmask) ){
			if ( (cell->attr & (ECA_PARENT | ECA_THIS)) ||
				(!(cell->f.dwFileAttributes & AttrMask) &&
				((cell->f.dwFileAttributes & DirMask) ||
					(FinddataRegularExpression(&cell->f, &fn) != FRRESULT_NO))) ){
				CELt(cinfo->e.cellIMax) = index;

				if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
					if ( cell->attr & (ECA_PARENT | ECA_THIS) ){
						cinfo->e.RelativeDirs++;
					} else{
						cinfo->e.Directories++;
					}
				}
				cinfo->e.cellIMax++;
			} else{
				ResetMark(cinfo, cell);
			}
		} else{
			ResetMark(cinfo, cell);
		}
	}
	FreeFN_REGEXP(&fn);
	if ( cinfo->e.cellIMax == 0 ){
		if ( FALSE != TM_check(&cinfo->e.CELLDATA, sizeof(ENTRYCELL) * (cinfo->e.cellDataMax + 2)) ){
			if ( FALSE != TM_check(&cinfo->e.INDEXDATA, sizeof(ENTRYINDEX) * (cinfo->e.cellDataMax + 2)) ){
				SetDummyCell(&CELdata(cinfo->e.cellDataMax), MessageText(StrNoEntries));
				setflag(CELdata(cinfo->e.cellDataMax).attr, ECA_PARENT);
				CELdata(cinfo->e.cellDataMax).f.dwFileAttributes = FILE_ATTRIBUTEX_MESSAGE;
				CELdata(cinfo->e.cellDataMax).f.nFileSizeLow = ERROR_FILE_NOT_FOUND;
				CELt(0) = cinfo->e.cellDataMax;
				cinfo->e.cellIMax++;
			}
		}
	} else{
		CellSort(cinfo, (cinfo->XC_sort.mode.dat[0] >= 0) ?
			&cinfo->XC_sort : &cinfo->dset.sort);
	}

	if ( cinfo->UseArcPathMask == ARCPATHMASK_DIRECTORY ){
		TCHAR buf[VFPS];

		VFSFullPath(buf, cinfo->ArcPathMask,
			((cinfo->e.Dtype.mode == VFSDT_LFILE) &&
			(cinfo->e.Dtype.BasePath[0] != '\0')) ?
			cinfo->e.Dtype.BasePath : cinfo->path);
		PPxSetPath(cinfo->RegNo, buf);
	}

	InitCli(cinfo);
	Repaint(cinfo);
	FindCell(cinfo, filename);
	return TRUE;
}

ERRORCODE MaskEntry(PPC_APPINFO *cinfo, int mode, const TCHAR *defmask, const TCHAR *path, BOOL usedefmask, TCHAR *initcmd)
{
	FILEMASKDIALOGSTRUCT pfs = {NULL, MES_TFEM, NULL, NULL, StrXC_rmsk, NULL, NULL, NULL, TRUE, 0, {1, 0, 0, 0}};
	XC_MASK mask;
	int result;
	TCHAR maskpath[VFPS], maskitem[VFPS];
	const TCHAR *usemask = NULL;

	mask.attr = 0;
	mask.file[0] = '\0';
	pfs.mask = &mask;
	pfs.initcmd = initcmd;
	/*
	if ( (mode == DSMD_PATH_BRANCH) && (path != NULL) && (path[0] != '\0') ){
		FDSOS fds;

		fds.mode = DSMD_NOMODE;
		fds.findpath = findpath;
		fds.finditem = finditem;
		fds.itemptr = fds.ls.mask;
		FindDirSettingOne(&fds, path, DSMD_PATH_BRANCH);
	}else
	*/
	if ( mode != DSMD_TEMP ){
		*pfs.mask = cinfo->mask; // Holdmask(\F)

		if ( mode == DSMD_NOMODE ){
			maskpath[0] = '\0';
			mode = FindDirSetting(cinfo, ITEMSETTING_MASK, maskpath, maskitem);
			path = maskpath;
			if ( mode == DSMD_NOMODE ){
				GetCustData(Str_X_dsst, &X_dsst, sizeof(X_dsst));
				mode = X_dsst[0];
			}else{
				usemask = maskitem;
			}
		}else{
			LOADSETTINGS_TMP ls;

			ls.mask[0] = '\0';
			switch ( mode ){
				case DSMD_THIS_PATH:
					LoadSetting_FixThisPath(maskpath, cinfo->path);
					LoadSettingMain(&ls, maskpath);
					usemask = ls.mask;
					break;

				case DSMD_THIS_BRANCH:
					LoadSetting_FixThisBranch(maskpath, cinfo->path);
					LoadSettingMain(&ls, maskpath);
					usemask = ls.mask;
					break;

				case DSMD_REGID:
					usemask = cinfo->mask.file;
					break;

				case DSMD_LISTFILE:
					LoadSettingMain(&ls, StrListfileMode);
					usemask = ls.mask;
					break;

				case DSMD_ARCHIVE:
					LoadSettingMain(&ls, StrArchiveMode);
					usemask = ls.mask;
					break;

				case DSMD_PATH_BRANCH:
					if ( path != NULL ){
						LoadSettingMain(&ls, path);
						usemask = ls.mask;
					}
					break;
			}
		}
	}

	if ( usedefmask ) usemask = defmask;
	if ( usemask != NULL ) tstrcpy(mask.file, usemask);

	pfs.filename = CEL(cinfo->e.cellN).f.cFileName;
	pfs.mode = mode;
	pfs.path = path;
	pfs.cinfo = cinfo;
	result = (int)PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_MASK),
		cinfo->info.hWnd, FileMaskDialog, (LPARAM)&pfs);
	if ( result <= 0 ) return ERROR_CANCELLED;

	if ( pfs.mode == DSMD_TEMP ){ // Temporarilymask(F)
		if ( MaskEntryMain(cinfo, &mask, pfs.filename) == FALSE ){
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
	} else{		 // Holdmask(\F)
		if ( (pfs.mode == DSMD_NOMODE) || (pfs.mode == DSMD_REGID) ){
			cinfo->mask = *pfs.mask;
			SetCustTable(T("XC_mask"), cinfo->RegCID + 1,
				&cinfo->mask, TSTRSIZE(cinfo->mask.file) + 4);
		} else{ // DSMD_NOMODE, DSMD_TEMP, DSMD_REGID 以外
			// ディレクトリ指定があれば、"p:," / "o:d" に置き換え
			if ( pfs.mask->attr & MASKEXTATTR_DIR ){
				if ( (pfs.mask->file[0] == 'o') && (pfs.mask->file[1] == ':') && (pfs.mask->file[2] != 'd') ){
					tstrinsert(pfs.mask->file + 2, T("d"));
				}else if ( pfs.mask->file[0] != '\0' ){
					tstrinsert(pfs.mask->file, T("o:d,"));
				}
			}

			switch ( pfs.mode ){
				case DSMD_THIS_PATH:
					LoadSetting_FixThisPath(maskpath, cinfo->path);
					path = maskpath;
					break;
				case DSMD_LISTFILE:
					path = StrListfileMode;
					break;
				case DSMD_ARCHIVE:
					path = StrArchiveMode;
					break;
				case DSMD_PATH_BRANCH:
					if ( (path != NULL) && (*path != '\0') ) break;
//				case DSMD_THIS_BRANCH:
				default:
					LoadSetting_FixThisBranch(maskpath, cinfo->path);
					path = maskpath;
					break;
			}
			SetNewXdir(path, LOADMASKSTR, pfs.mask->file);
		}
		tstrcpy(cinfo->Jfname, pfs.filename);
		read_entry(cinfo, RENTRY_JUMPNAME | RENTRY_SAVEOFF);
	}
	return NO_ERROR;
}

void SwapTargetWindow(HWND hTargetWnd, int TargetShow, RECT *TargetBox, int RefShow, RECT *RefBox)
{
#if 1
	if ( !( (RefShow == SW_SHOWMAXIMIZED) &&
			(TargetBox->left == RefBox->left) &&
			(TargetBox->bottom == RefBox->bottom) ) ){
		if ( (TargetShow == SW_SHOWMAXIMIZED) ||
			 (TargetShow == SW_SHOWMINIMIZED) ){
			ShowWindow(hTargetWnd, SW_SHOWNOACTIVATE);
		}
		if ( RefShow == SW_SHOWMAXIMIZED ){
			SetWindowPos(hTargetWnd, NULL,
					RefBox->left, RefBox->top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			ShowWindow(hTargetWnd, SW_SHOWMAXIMIZED);
			return;
		}
	}
#endif
	if ( TargetShow != RefShow ) ShowWindow(hTargetWnd, RefShow);
	if ( (RefShow != SW_SHOWMINIMIZED) &&
		 (RefShow != SW_HIDE) &&
		 ((RefBox->right - RefBox->left) >= 16) ){
			SetWindowPos(hTargetWnd, NULL,
					RefBox->left, RefBox->top,
					RefBox->right - RefBox->left, RefBox->bottom - RefBox->top,
					SWP_NOZORDER);
		}
}

void SwapWindow(PPC_APPINFO *cinfo)
{
	HWND TargetWnd;

	if ( XC_gmod ){
		TCHAR pair[VFPS];

		GetPairPath(cinfo, pair);
		SetPPcDirPos(cinfo);
		if ( SetPairPath(cinfo, NULL, NilStr) == FALSE ) return;
		ChangePath(cinfo, pair, CHGPATH_SETABSPATH);
		read_entry(cinfo, RENTRY_READ);
		UpdateWindow_Part(cinfo->info.hWnd);
		return;
	}

	if ( cinfo->combo ){
		SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND, KCW_swapwnd, 0);
		return;
	}

	cinfo->swin ^= SWIN_SWAPMODE;
	SendX_win(cinfo);

	TargetWnd = GetPairWnd(cinfo);
	if ( TargetWnd != NULL ){
		RECT thisBox, owpBox;
		WINDOWPLACEMENT owp;
		UINT showCmd;

		showCmd = cinfo->WinPos.show;
		owp.length = sizeof(owp);
		GetWindowPlacement(TargetWnd, &owp);
		GetWindowRect(TargetWnd, &owpBox);
		thisBox = cinfo->wnd.NCRect;
										// 自分の位置を変更
		SwapTargetWindow(cinfo->info.hWnd, showCmd, &thisBox, owp.showCmd, &owpBox);
										// 反対窓の位置を変更
		SwapTargetWindow(TargetWnd, owp.showCmd, &owpBox, showCmd, &thisBox);
	}
}

void USEFASTCALL UnpackExec(PPC_APPINFO *cinfo, const TCHAR *param)
{
	BOOL arcmode;

	arcmode = OnArcPathMode(cinfo);
	PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, param, NULL, 0);
	if ( IsTrue(arcmode) ) OffArcPathMode(cinfo);
}

ERRORCODE ExecuteCommandline(PPC_APPINFO *cinfo)
{
	TCHAR buf[CMDLINESIZE];

	buf[0] = '\0';
	if ( PPctInput(cinfo, MES_TSHL, buf, TSIZEOF(buf),
		PPXH_COMMAND, PPXH_COMMAND) <= 0 ){
		return ERROR_CANCELLED;
	}
	PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, buf, NULL, 0);
	return NO_ERROR;

}

ERRORCODE ExecuteEntry(PPC_APPINFO *cinfo)
{
	TCHAR buf[CMDLINESIZE], namebuf[VFPS];
	const TCHAR *name;
	TINPUT ti;
	FN_REGEXP fn;
	BOOL result;

	ti.hOwnerWnd = cinfo->info.hWnd;
	ti.hRtype = PPXH_COMMAND;
	ti.hWtype = PPXH_COMMAND;
	ti.title = MES_TEXE;
	ti.buff = buf;
	ti.size = TSIZEOF(buf);
	ti.flag = TIEX_USESELECT | TIEX_USEINFO;
	ti.info = &cinfo->info;

	buf[0] = '\0';
	#define ExpandBufSize (TSIZEOF(buf) - TSIZEOF(EXTPATHEXT) - 1)
	if ( ExpandEnvironmentStrings(T("%PATHEXT%"), buf, ExpandBufSize) >=
			ExpandBufSize ){
		buf[0] = '\0';
	}
	#undef ExpandBufSize
	if ( buf[0] != '\0' ) tstrcat(buf, T(";"));
	tstrcat(buf, T(EXTPATHEXT));
	MakeFN_REGEXP(&fn, buf);
	if ( FilenameRegularExpression(CEL(cinfo->e.cellN).f.cFileName, &fn) != FRRESULT_NO ){
		ti.firstC = EC_LAST;
		ti.lastC = EC_LAST;
	} else{
		ti.firstC = 0;
		ti.lastC = 0;
	}
	FreeFN_REGEXP(&fn);
	name = GetCellFileName(cinfo, &CEL(cinfo->e.cellN), namebuf);

	if ( (cinfo->e.Dtype.mode == VFSDT_SUSIE) ||
		(cinfo->e.Dtype.mode == VFSDT_UN) ||
//		 IsNodirShnPath(cinfo) ||
		(cinfo->e.Dtype.mode == VFSDT_HTTP) ){
		cinfo->UnpackFix = TRUE;
		name = FindLastEntryPoint(name);
	}

	if ( tstrchr(name, ' ') != NULL ){
		thprintf(buf, TSIZEOF(buf), T(" \"%s\" "), name);
	} else{
		thprintf(buf, TSIZEOF(buf), T(" %s "), name);
	}
	result = tInputEx(&ti);
	if ( cinfo->UnpackFix ) OffArcPathMode(cinfo);
	if ( result <= 0 ) return ERROR_CANCELLED;
	UnpackExec(cinfo, buf);
	return NO_ERROR;
}

void PPcShellExecute(PPC_APPINFO *cinfo)
{
	TCHAR newpath[VFPS];
	TCHAR *cfilename;
	BOOL archivemode;

	if ( (cinfo->e.Dtype.mode == VFSDT_SHN) ||
		(CEL(cinfo->e.cellN).f.dwFileAttributes & FILE_ATTRIBUTEX_VIRTUAL) ){
		SCmenu(cinfo, NilStr);
		return;
	}
	tstrcpy(newpath, cinfo->RealPath);
	cfilename = CEL(cinfo->e.cellN).f.cFileName;
	archivemode = PPcUnpackForAction(cinfo, newpath, UFA_ALL);
	ShellExecEntries(cinfo, NULL, cfilename, newpath, NULL, archivemode);
}

void CountOrEdit(PPC_APPINFO *cinfo)
{
	if ( DispMarkSize(cinfo) ) return;
	UnpackExec(cinfo, T("%ME_scr"));
}

void JumpDrive(PPC_APPINFO *cinfo, WORD key)
{
	TCHAR buf[VFPS];
	DWORD X_dlf[3] = {0, 0, XDLF_NOOVERLAP};

	GetCustData(Str_X_dlf, &X_dlf, sizeof(X_dlf));

	buf[0] = (BYTE)('A' + key - '1');
	if ( key & K_s ) buf[0] += (TCHAR)9;
	buf[1] = ':';
	buf[2] = '\\';
	buf[3] = '\0';

	if ( (GetLogicalDrives() & (LSBIT << (buf[0] - 'A'))) && // ドライブがある
		(GetFileAttributesL(buf) != BADATTR) ){ // 実在する
		if ( IsTrue(cinfo->ModifyComment) ){
			WriteComment(cinfo, cinfo->CommentFile);
		}
		if ( cinfo->path[0] != buf[0] ){ // カレントドライブと異なるときに取得
			if ( !(X_dlf[2] & XDLF_ROOTJUMP) ){ // カレントディレクトリ指定
				const TCHAR *hisp;

				UsePPx();
				hisp = SearchPHistory((X_dlf[2] & XDLF_NOOVERLAP) ?
					(PPXH_PPCPATH | PPXH_NOOVERLAP) : PPXH_PPCPATH,
					buf);
				if ( hisp != NULL ) tstrcpy(buf, hisp);
				FreePPx();
			}
			if ( IsTrue(cinfo->ChdirLock) ){ // ロック
				PPCuiWithPathForLock(cinfo, buf);
				return;
			}
			SetPPcDirPos(cinfo);
			ChangePath(cinfo, buf, CHGPATH_SETABSPATH);

			DxSetMotion(cinfo->DxDraw, DXMOTION_ChangeDrive);
		}
		read_entry(cinfo, RENTRY_READ);
	} else{
		SetPopMsg(cinfo, ERROR_INVALID_DRIVE, NULL);
	}
}

void PPcBackDirectory(PPC_APPINFO *cinfo)
{
	const TCHAR *p;

	UsePPx();
	p = EnumHistory(PPXH_PPCPATH, 1);
	if ( p != NULL ){
		TCHAR buf[VFPS];

		tstrcpy(buf, p);
		FreePPx();
		SetPPcDirPos(cinfo);
		ChangePath(cinfo, buf, CHGPATH_SETABSPATH);
		read_entry(cinfo, RENTRY_READ);
	} else{
		FreePPx();
	}
}

void PPcBackDirectoryList(PPC_APPINFO *cinfo)
{
	HMENU hMenu;
	TCHAR path[VFPS], *p;
	int depth = 1;

	tstrcpy(path, cinfo->path);
	hMenu = CreatePopupMenu();

	for ( ; ; ){
		p = VFSFindLastEntry(path);
		if ( *p ){
			*p = '\0';
		} else{
			if ( path[0] != ':' ){
				tstrcpy(path, T(":"));
			} else{
				break;
			}
		}
		AppendMenuString(hMenu, depth++, path);
	}
	depth = PPcTrackPopupMenu(cinfo, hMenu);
	if ( depth > 0 ){
		GetMenuString(hMenu, depth, path, VFPS, MF_BYCOMMAND);
		SetPPcDirPos(cinfo);
		ChangePath(cinfo, path, CHGPATH_SETABSPATH);
		read_entry(cinfo, RENTRY_READ);
	}
	DestroyMenu(hMenu);
}

void PPcPPv(PPC_APPINFO *cinfo)
{
	TCHAR buf[VFPS];

	// 書庫内のディレクトリと思われるエントリは表示しない
	if ( (cinfo->e.Dtype.mode == VFSDT_UN) ||
		(cinfo->e.Dtype.mode == VFSDT_SUSIE) ){
		if ( (CEL(cinfo->e.cellN).attr & (ECA_THIS | ECA_PARENT | ECA_DIR)) ||
			(CEL(cinfo->e.cellN).f.nFileSizeHigh != 0) ||
			(CEL(cinfo->e.cellN).f.nFileSizeLow == 0) ){
			return;
		}
	}

	#ifndef UNICODE
	if ( (strchr(CEL(cinfo->e.cellN).f.cFileName, '?') != NULL) &&
		 (CEL(cinfo->e.cellN).f.cAlternateFileName[0] != '\0') ){
		VFSFullPath(buf, CEL(cinfo->e.cellN).f.cAlternateFileName, cinfo->RealPath);
	} else
	#endif
	{
		VFSFullPath(buf, (TCHAR *)GetCellFileName(cinfo, &CEL(cinfo->e.cellN), buf), cinfo->path);
	}
	PPxView(cinfo->info.hWnd, buf, cinfo->NormalViewFlag);
}

//------------------------------------- UNC の root を決定する
// \\pc\share\path....
//   ^ ^     ^
const TCHAR *GetUncRoot(const TCHAR *path)
{
	const TCHAR *pc, *share;

	pc = FindPathSeparator(path);	// PC名をスキップ
	if ( pc == NULL ) return path + tstrlen(path);

	pc++;	// '\\' をスキップ
	share = FindPathSeparator(pc);	// 共有名をスキップ
	if ( share != NULL ) return share;	// 共有名以降もあり
	if ( *pc == '\0' ) return pc;		// 共有名無し→pcまで
	return pc + tstrlen(pc);			// 共有名の末尾まで
}

void USEFASTCALL FocusAddressBar(HWND hWnd)
{
	SetFocus(hWnd);
	SendMessage(hWnd, EM_SETSEL, 0, EC_LAST);
}

BOOL USEFASTCALL FocusAddressBars(HWND hSingleWnd, PPXDOCKS *docks)
{
	if ( hSingleWnd != NULL ){
		FocusAddressBar(hSingleWnd);
	} else if ( docks->t.hAddrWnd != NULL ){
		FocusAddressBar(docks->t.hAddrWnd);
	} else if ( docks->b.hAddrWnd != NULL ){
		FocusAddressBar(docks->b.hAddrWnd);
	} else{
		return FALSE;
	}
	return TRUE;
}

ERRORCODE LogBarDisk(PPC_APPINFO *cinfo)
{
	if ( cinfo->combo ){
		if ( SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND, KCW_addressbar, 0) ){
			return NO_ERROR;
		}
	}
	if ( FocusAddressBars(NULL, &cinfo->docks) ) return NO_ERROR;
	return LogDisk(cinfo);
}

ERRORCODE LogDisk(PPC_APPINFO *cinfo)
{
	TCHAR buf[VFPS];

	buf[0] = 0;
	if ( PPctInput(cinfo, MES_TLDS, buf, TSIZEOF(buf),
		PPXH_DIR_R, PPXH_DIR) <= 0 ){
		return ERROR_CANCELLED;
	}
	if ( VFSFixPath(NULL, buf, cinfo->path, VFSFIX_VFPS) == NULL ){
		SetPopMsg(cinfo, POPMSG_MSG, MES_EPTH);
		return ERROR_BAD_COMMAND;
	}
	SetPPcDirPos(cinfo);
	ChangePath(cinfo, buf, CHGPATH_SETABSPATH);
	read_entry(cinfo, RENTRY_READ);
	return NO_ERROR;
}

void ViewOnCursor(PPC_APPINFO *cinfo, int flags)
{
	TCHAR buf[VFPS];

	VFSFullPath(buf, (TCHAR *)GetCellFileName(cinfo, &CEL(cinfo->e.cellN), buf), cinfo->RealPath);
	PPxView(cinfo->info.hWnd, buf, flags);
	SetForegroundWindow(cinfo->info.hWnd);
}

void SetGrayStatus(PPC_APPINFO *cinfo)
{
	ENTRYDATAOFFSET offset;

	if ( cinfo->FDirWrite < FDW_SENDREQUEST ) return;
	for ( offset = 0; offset < cinfo->e.cellDataMax; offset++ ){
		setflag(CELdata(offset).attr, ECA_GRAY);
	}
	SetLinebrush(cinfo, LINE_GRAY);
	Repaint(cinfo);
	cinfo->FDirWrite = FDW_REQUEST;
}

void MakePathTrackingListMenuString(TCHAR *dest, int offset, TCHAR *itemname)
{
	if ( offset < 10 ){
		thprintf(dest, VFPS + 8, T("&%d %s"), offset, itemname);
	} else{
		thprintf(dest, VFPS + 8, T("  %s"), itemname);
	}
}

#ifdef UNICODE
#define S16TO32(i) i
#else
#define S16TO32(i) (int)(signed short)i
#endif

#define TRACKINGMENUS 16
BOOL PathTrackingListMenu(PPC_APPINFO *cinfo, int dest)
{
	int index = 1;
	HMENU hMenu;
	TCHAR buf[VFPS + 8];
	TCHAR *p;

	hMenu = CreatePopupMenu();
	if ( cinfo->PathTrackingList.top ){
		if ( dest > 0 ){
			p = (TCHAR *)(cinfo->PathTrackingList.bottom +
				cinfo->PathTrackingList.top);
			for ( ; index < TRACKINGMENUS; index++ ){
				if ( *p == '\0' ) break;
				MakePathTrackingListMenuString(buf, index, p);
				AppendMenuString(hMenu, index, buf);
				p += tstrlen(p) + 1;
			}
		} else{
			DWORD top;

			top = BackPathTrackingList(cinfo, cinfo->PathTrackingList.top);
			if ( top ) while ( index < TRACKINGMENUS ){
				top = BackPathTrackingList(cinfo, top);
				MakePathTrackingListMenuString(buf, index,
					(TCHAR *)(cinfo->PathTrackingList.bottom + top));
				AppendMenuString(hMenu, -index, buf);
				index++;
				if ( !top ) break;
			}
		}
	}
	if ( index == 1 ) AppendMenu(hMenu, MF_GS, 0, T("none"));

	// Win9x では戻り値が16bitなので、符号拡張が必要
	index = S16TO32(PPcTrackPopupMenu(cinfo, hMenu));
	DestroyMenu(hMenu);
	if ( index == 0 ) return FALSE;
	JumpPathTrackingList(cinfo, index);
	return TRUE;
}

void SetSyncPath(PPC_APPINFO *cinfo, const TCHAR *param)
{
	HWND hPairWnd;
	int mode = -1, oldmode;
	BOOL pair = FALSE;

	hPairWnd = GetPairWnd(cinfo);
	if ( hPairWnd == NULL ) return;
	if ( param != NULL ){
		for (;;){
			int type;

			type = GetStringCommand(&param, T("OFF\0") T("ENTRY\0") T("ON\0") T("PAIR\0") T("SYNC\0") );
			if ( type >= 3 ){
				INT_PTR pairwnd;

				if ( type == 4 ) pair = TRUE;
				if ( *param == ':' ) param++;
				pairwnd = GetNumber(&param);
				if ( pairwnd != 0 ) hPairWnd = (HWND)pairwnd;
				continue;
			}
			if ( type < 0 ) break;
			mode = type;
		}
	}
	oldmode = cinfo->SyncPathFlag;
	if ( mode >= 0 ){
		cinfo->SyncPathFlag = mode;
	}else{
		cinfo->SyncPathFlag = (cinfo->SyncPathFlag == SyncPath_off) ? SyncPath_dir : SyncPath_off;
	}
	if ( !pair ) {
		COPYDATASTRUCT copydata;
		TCHAR cmd[CMDLINESIZE];

		thprintf(cmd, TSIZEOF(cmd), T("*syncpath %d -sync:%d"), cinfo->SyncPathFlag, cinfo->info.hWnd);
		copydata.dwData = 'H';
		copydata.cbData = TSTRSIZE32(cmd);
		copydata.lpData = (PVOID)cmd;
		SendMessage(hPairWnd, WM_COPYDATA, 0, (LPARAM)&copydata);

		if ( cinfo->SyncPathFlag != oldmode ){
			SetPopMsg(cinfo, POPMSG_NOLOGMSG, cinfo->SyncPathFlag ? MES_SPPN : MES_SPPF);
		}
	}
	if ( cinfo->SyncPathFlag == SyncPath_off ) return;
	cinfo->hSyncViewPairWnd = hPairWnd;

	if ( cinfo->SyncPathFlag == SyncPath_dir ){
		TCHAR thispath[VFPS], pairpath[VFPS], *thistail, *pairtail, *pairsep;

		tstrcpy(thispath, cinfo->path);
		thistail = thispath + tstrlen(thispath);

		GetPairPath(cinfo, pairpath);
		pairtail = pairpath + tstrlen(pairpath);

		// 末尾が共通している部分を検出する
		while ( (thistail > thispath) && (pairtail > pairpath) ){
			if ( TinyCharLower(*(thistail - 1)) != TinyCharLower(*(pairtail - 1)) ){
				break;
			}
			thistail--;
			pairtail--;
		}

		pairsep = VFSGetDriveType(pairpath, NULL, NULL);
		if ( pairsep == NULL ) pairsep = pairpath;
		if ( pairsep >= pairtail ){
			*pairsep = '\0';
			*(thistail + (pairsep - pairtail)) = '\0';
		}else for ( ;; ){
			TCHAR *nextptr;

			nextptr = FindPathSeparator(pairsep + 1);
			if ( nextptr == NULL ) break; // 最終エントリ→未加工
			if ( nextptr < pairtail ){
				pairsep = nextptr;
				continue;
			}
			*nextptr = '\0';
			*(thistail + (nextptr - pairtail)) = '\0';
			break;
		}

		ThSetString(&cinfo->StringVariable, StrSyncPathThis, thispath);
		ThSetString(&cinfo->StringVariable, StrSyncPathPair, pairpath);
	}

	SyncPairEntry(cinfo);
}

void SetSyncView(PPC_APPINFO *cinfo, int mode)
{
	TCHAR param[16];

	if ( mode ){
		if ( mode == 1 ){
			param[0] = '0';
			GetCustTable(Str_others, T("SyncViewID"), param, sizeof(param));
			if ( Isalpha(param[0]) ) mode = param[0];
		}

		if ( Isalpha(mode) ){ // 使用ID指定
			mode = (upper((TCHAR)mode) << 24) | PPV_BOOTID;
		}

		if ( cinfo->SyncViewFlag == 0 ){
			SetPopMsg(cinfo, POPMSG_MSG, MES_SVON);
		}
		ViewOnCursor(cinfo, (mode & ~1) | PPV_NOFOREGROUND | PPV_SYNCVIEW);
	} else{
		if ( cinfo->SyncViewFlag & PPV_BOOTID ){
			thprintf(param, TSIZEOF(param), T("*closeppx V%c"), cinfo->SyncViewFlag >> 24);
			PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, param, NULL, 0);
		}
		if ( cinfo->SyncViewFlag != 0 ){
			SetPopMsg(cinfo, POPMSG_MSG, MES_SVOF);
		}
	}
	cinfo->SyncViewFlag = mode;
}

void PPcReload(PPC_APPINFO *cinfo)
{
	if ( IsTrue(cinfo->ModifyComment) ){
		WriteComment(cinfo, cinfo->CommentFile);
	}
	if ( CEL(cinfo->e.cellN).type == ECT_SYSMSG ){
		TCHAR buf[VFPS], *p;

		p = GetDriveName(buf, cinfo->path);
		if ( p != NULL ){
			if ( *p == '\\' ) p++;
			if ( *p == '\0' ){
				buf[tstrlen(buf) - 1] = '\0';
				ChangePath(cinfo, buf, CHGPATH_SETRELPATH);
			}
		}
		read_entry(cinfo, RENTRY_CACHEREFRESH);
	} else if ( XC_acsr[0] ){
		tstrcpy(cinfo->Jfname, CEL(cinfo->e.cellN).f.cFileName);
		read_entry(cinfo,
			RENTRY_JUMPNAME | RENTRY_SAVEOFF | RENTRY_CACHEREFRESH);
	} else{
		read_entry(cinfo, RENTRY_SAVEOFF | RENTRY_CACHEREFRESH);
	}
}

void PPcPasteSub(PPC_APPINFO *cinfo, HGLOBAL hData, UINT type)
{
	SaveClipboardData(hData, type, cinfo);
	CloseClipboard();
	if ( cinfo->Jfname[0] ) read_entry(cinfo, RENTRY_UPDATE | RETRY_FLAGS_NEWFILE);
}

void PPcPaste(PPC_APPINFO *cinfo, BOOL makeshortcut)
{
	UINT cptype;
	TMS_struct files = {{NULL, 0, NULL}, 0};
	HGLOBAL hG;
	UINT cfFileContents;

	if ( OpenClipboardCheck(cinfo) == FALSE ) return;

	hG = GetClipboardData(CF_HDROP);	// ファイル等
	if ( hG != NULL ){
		const TCHAR *action;
		DWORD effect = DROPEFFECT_COPY;

		HdropdataToFiles(hG, &files);
									// ドロップ方法を取得
		hG = GetClipboardData(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT));
		if ( hG != NULL ){
			#pragma warning(suppress: 6011) // GlobalLock は失敗しないと見なす
			effect = *(DWORD *)GlobalLock(hG);
			GlobalUnlock(hG);
		}

		if ( makeshortcut == FALSE ){
			action = (effect == DROPEFFECT_MOVE) ?
					PasteMode_Move : PasteMode_Copy;
			if ( !IsExistCustTable(T("X_fopt"), action) ){
				action = (effect == DROPEFFECT_MOVE) ?
						FileOperationMode_Move : FileOperationMode_Copy;
			}
		} else{
			action = Pastemode_Link;
		}
		CloseClipboard();

		// コピー実体
		PPc_DoFileOperation(cinfo, action, &files, cinfo->path, NULL, VFSFOP_AUTOSTART | VFSFOP_SPECIALDEST);
		return;
	}

	cfFileContents = RegisterClipboardFormat(CFSTR_FILECONTENTS);
	cptype = 0;
	for ( ;;) {
		cptype = EnumClipboardFormats(cptype);
		if ( cptype == 0 ) break;
		if ( cptype == cfFileContents ) break;
		if ( (cptype == CF_DIB) || (cptype == CF_DIBV5) ){
			hG = GetClipboardData(cptype);
			if ( hG != NULL ){
				PPcPasteSub(cinfo, hG, cptype);
				return;
			}
			break;
		}
	}

	if ( cptype != cfFileContents ){
		hG = GetClipboardData(CF_UNICODETEXT);
		if ( hG != NULL ){
			WCHAR *src;
			int size;
			BOOL inwchar;
			UINT codepage = CP_ACP;

			GetCustData(T("X_newcp"), &codepage, sizeof(codepage));

			// UNICODEでないとだめな文字があればUNICODEそうでなければTEXT
			if ( (codepage == CP_UTF8) || (codepage == VTYPE_UTF8) || (codepage == CP__UTF16L) ){
				size = 1;
				inwchar = TRUE;
			}else{
				src = GlobalLock(hG);
				size = WideCharToMultiByte(codepage, 0, src, -1, NULL, 0, NULL, &inwchar);
				GlobalUnlock(hG);
			}
			if ( (size > 0) && inwchar ){
				PPcPasteSub(cinfo, hG, CF_UNICODETEXT);
				return;
			}
		}

		hG = GetClipboardData(CF_TEXT);
		if ( hG != NULL ){
			PPcPasteSub(cinfo, hG, CF_TEXT);
			return;
		}
	}
	CloseClipboard();

	// cfFileContents などの貼り付けは Explorer にまかせる
	// ※OleGetClipboardの代行相当
	PPcSHContextMenu(cinfo, StrThisDir, (makeshortcut == FALSE) ?
		StrInvokePaste : StrInvokePasteLink);
}

BOOL PPcSHContextMenu(PPC_APPINFO *cinfo, const TCHAR *dir, const TCHAR *verb)
{
	POINT pos;

	GetPopupPosition(cinfo, &pos);
	return VFSSHContextMenu(cinfo->info.hWnd, &pos, cinfo->path, dir, verb);
}

void PPcSystemMenu(PPC_APPINFO *cinfo)
{
	DYNAMICMENUSTRUCT *dms;

	dms = !(cinfo->combo) ? &cinfo->DynamicMenu : &ComboDMenu;
	dms->Sysmenu = TRUE;
	// キャプションのアイコンにフォーカスを設定
	PostMessage(cinfo->combo ? cinfo->hComboWnd : cinfo->info.hWnd,
		WM_SYSCOMMAND, SC_KEYMENU, TMAKELPARAM(0, 0));

	// メニューを表示させるキーを予め送信
	PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL,
			(cinfo->PopupPosType == PPT_FOCUS) ? T("%k\"&down") : T("%k\"down"),
			NULL, 0);
}

HWND GetMWbase(PPC_APPINFO *cinfo, TCHAR *buf)
{
	HWND hWnd;
	TCHAR *p;

	if ( cinfo->combo ){
		hWnd = cinfo->hComboWnd;
		p = ComboID;
	} else{
		hWnd = cinfo->info.hWnd;
		p = cinfo->RegCID;
	}
	thprintf(buf, 10, T("%s_"), p);
	return hWnd;
}

void PPcMoveWindowSavedPosition(PPC_APPINFO *cinfo)
{
	TCHAR buf[10];
	WINPOS WPos;
	HWND hWnd;

	hWnd = GetMWbase(cinfo, buf);
	if ( NO_ERROR == GetCustTable(Str_WinPos, buf, &WPos, sizeof(WPos)) ){
		SetWindowPos(hWnd, NULL, WPos.pos.left, WPos.pos.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
		MoveWindowByKey(hWnd, 0, 0);
	} else{
		CenterWindow(hWnd);
	}
}

void PPcSaveWindowPosition(PPC_APPINFO *cinfo)
{
	TCHAR buf[10];
	WINPOS WPos;
	HWND hWnd;

	hWnd = GetMWbase(cinfo, buf);
	WPos.show = 0xff;
	WPos.reserved = 0xff;
	GetWindowRect(hWnd, &WPos.pos);

	SetCustTable(Str_WinPos, buf, &WPos, sizeof(WPos));
	SetPopMsg(cinfo, POPMSG_MSG, MES_SAVP);
}

void PPcReloadCustomize(PPC_APPINFO *cinfo, LPARAM idparam)
{
	BOOL cust1st = TRUE; // 別PPcで共通カスタマイズを行っていた場合は FALSE

	PPxCommonCommand(cinfo->info.hWnd, (LPARAM)idparam, K_Lcust);

	if ( idparam != 0 ){
		if ( idparam == CommonCustTickID ){
			cust1st = FALSE;
		}else{
			CommonCustTickID = idparam;
		}
	}

	if ( cinfo->combo ){
		if ( cinfo->info.hWnd == hComboFocusWnd ){
			SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND, K_Lcust, (LPARAM)cinfo->info.hWnd);
		}
	} else{
		if ( cust1st ){
			GetCustData(T("X_combos"), &X_combos, sizeof(X_combos));
			BackupLog();
		}
	}

	PPcLoadCust(cinfo, TRUE);
	UnloadWallpaper(&cinfo->bg);
	LoadWallpaper(&cinfo->bg, cinfo->info.hWnd, cinfo->RegCID);
	cinfo->FullDraw = X_fles | cinfo->bg.X_WallpaperType;
	DeleteObject(cinfo->hBoxFont);
	InitFont(cinfo);
	DeleteObject(cinfo->C_BackBrush);
	cinfo->C_BackBrush = CreateSolidBrush(cinfo->BackColor);
	SetLinebrush(cinfo, LINE_NORMAL);
	if ( cust1st ){
		CloseAnySizeIcon(&DirIcon);
		CloseAnySizeIcon(&UnknownIcon);
	}

	InitGuiControl(cinfo, TRUE);
	if ( cinfo->hTreeWnd != NULL ){
		PPc_SetTreeFlags(cinfo->info.hWnd, cinfo->hTreeWnd);
	}
	if ( (cinfo->combo == 0) &&
		 (PPcGetWindow(0, CGETW_GETFOCUS) != cinfo->info.hWnd) ){
		WmPPxCommand(cinfo, KC_UNFOCUS, 0);
	}

	if ( cinfo->celF.fmt[0] == DE_WIDEV ) FixCellWideV(cinfo);
	if ( cust1st && !cinfo->combo ) RestoreLog();

	InitCli(cinfo);
	InvalidateRect(cinfo->info.hWnd, NULL, TRUE);
}

void PPcPopCell(PPC_APPINFO *cinfo)
{
	if ( (XC_emov || cinfo->e.cellStack) &&
		!(CEL(cinfo->e.cellN).attr & ECA_THIS) ){
		ENTRYINDEX maxi;
		ENTRYDATAOFFSET cofs;

		maxi = cinfo->e.cellIMax + cinfo->e.cellStack - 1;
		cofs = CELt(maxi);
		if ( cinfo->e.cellN < maxi ){
			memmove(&CELt(cinfo->e.cellN + 2),
				&CELt(cinfo->e.cellN + 1),
				sizeof(ENTRYINDEX) * (maxi - cinfo->e.cellN - 1));
		}
		CELt(cinfo->e.cellN + 1) = cofs;
		if ( !XC_emov ){
			cinfo->e.cellIMax++;
			cinfo->e.cellStack--;
			if ( CELdata(cofs).f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				if ( CELdata(cofs).attr & (ECA_PARENT | ECA_THIS) ){
					cinfo->e.RelativeDirs++;
				} else{
					cinfo->e.Directories++;
				}
			}
		}
		RefleshStatusLine(cinfo);
		InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
	}
}

void PPcPushCell(PPC_APPINFO *cinfo)
{
	if ( (cinfo->e.cellIMax > 1) &&
		!(CEL(cinfo->e.cellN).attr & (ECA_PARENT | ECA_THIS)) ){
		ENTRYINDEX maxi;
		ENTRYDATAOFFSET cofs;

		maxi = cinfo->e.cellIMax + cinfo->e.cellStack - 1;
		cofs = CELt(cinfo->e.cellN);
		if ( cinfo->e.cellN < maxi ){
			memmove(&CELt(cinfo->e.cellN), &CELt(cinfo->e.cellN + 1),
				sizeof(ENTRYINDEX) * (maxi - cinfo->e.cellN));
		}
		CELt(maxi) = cofs;
		if ( !XC_emov ){
			cinfo->e.cellStack++;
			cinfo->e.cellIMax--;
			if ( CELdata(cofs).f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				if ( CELdata(cofs).attr & (ECA_PARENT | ECA_THIS) ){
					cinfo->e.RelativeDirs--;
				} else{
					cinfo->e.Directories--;
				}
			}
		}
		cinfo->DrawTargetFlags = DRAWT_ALL;
		MoveCellCsr(cinfo, 0, NULL);
		RefleshStatusLine(cinfo);
		InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
	}
}

ERRORCODE PPcDirContextMenu(PPC_APPINFO *cinfo)
{
	return PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, T("%M_DirMenu"), NULL, 0);
}

void DivMark(PPC_APPINFO *cinfo)
{
	int m, i;

	cinfo->MarkMask = MARKMASK_FILE;
	m = !IsCEL_Marked(cinfo->e.cellN);
	for ( i = 0; i < cinfo->e.cellN; i++ ) CellMark(cinfo, i, 1 - m);
	for ( ; i < cinfo->e.cellIMax; i++ ) CellMark(cinfo, i, m);
	Repaint(cinfo);
}

void FirstCommand(PPC_APPINFO *cinfo)
{
	PPCSTARTPARAM *psp;

	psp = cinfo->FirstCommand;
	if ( psp != NULL ){
		BOOL oldlock = cinfo->ChdirLock;

		cinfo->FirstCommand = NULL;
		cinfo->ChdirLock = FALSE; // ロックを一時的に解除
		PP_ExtractMacro(cinfo->info.hWnd, &cinfo->info, NULL, psp->cmd, NULL, 0);
		if ( cinfo->ChdirLock == FALSE ) cinfo->ChdirLock = oldlock;
					// 解放の必要有り & psp の初期化が完了していればここで解放
		if ( IsTrue(psp->AllocCmd) && (psp->next == NULL) ){
			ThFree(&psp->th);
		} else{
			psp->AllocCmd = FALSE;
		}
	}
}

// ファイル情報を再取得する
void UpdateEntryData(PPC_APPINFO *cinfo)
{
	int work;
	ENTRYCELL *cell;
	TCHAR name[VFPS];

	InitEnumMarkCell(cinfo, &work);
	while ( (cell = EnumMarkCell(cinfo, &work)) != NULL ){
		if ( VFSFullPath(name, cell->f.cFileName, cinfo->path) != NULL ){
			FILE_STAT_DATA ff;

			if ( IsTrue(GetFileSTAT(name, &ff)) ){
				memcpy(&cell->f, &ff, (BYTE *)&ff.cFileName - (BYTE *)&ff.dwFileAttributes);
				if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
					setflag(cell->attr, ECA_DIR);
				}
				if ( cell->state == ECS_DELETED ){
					cell->state = ECS_ADDED;
				}
			} else{
				cell->state = ECS_DELETED;
				ResetMark(cinfo, cell);
			}
		}
	}
	Repaint(cinfo);
}

struct DirOptionMenuArgs {
	HMENU hPopupMenu;
	DWORD *index;
	ThSTRUCT *TH;
	const TCHAR *opttype;
};

void DirOption_Icon_Menu(struct DirOptionMenuArgs *DOMA, const struct DirOptionMenuDataStruct *dos, WORD icontype)
{
	HMENU hSubMenu;

	TCHAR buf[VFPS];
	int i;

	icontype = (icontype > DSETI_OVLSINGLE) ? (WORD)0 : (WORD)(icontype + 1);
	hSubMenu = CreatePopupMenu();
	for ( i = 0; i <= (DSETI_OVLSINGLE + 1); i++ ){
		if ( dos->IconShowModeList[i] != NULL ){
			AppendMenuCheckString(hSubMenu, (*DOMA->index)++,
				dos->IconShowModeList[i], (i == icontype));
			thprintf(DOMA->TH, THP_ADD, T("*diroption %s %s %d"),
				DOMA->opttype, dos->CommandName, i - 1);
		}
	}
	thprintf(buf, TSIZEOF(buf), T("%Ms : %Ms"),
			dos->MenuItemName, dos->IconShowModeList[icontype]);
	AppendMenu(DOMA->hPopupMenu, MF_EPOP, (UINT_PTR)hSubMenu, buf);
}

void DirOption_SetDirOptCommand(struct DirOptionMenuArgs *DOMA, const TCHAR *cmd, const TCHAR *menuname, BOOL check)
{
	AppendMenuCheckString(DOMA->hPopupMenu, (*DOMA->index)++, menuname, check);
	thprintf(DOMA->TH, THP_ADD, T("*diroption %s %s"), DOMA->opttype, cmd);
}

struct DirOptionOptCommandStruct {
	const TCHAR *menuname;
	const TCHAR *command;
	TCHAR menuacc;
};
struct DirOptionOptCommandStruct DirOptionOptCommand_view = {MES_MTVF, T("*viewstyle %s"), 'V'};
struct DirOptionOptCommandStruct DirOptionOptCommand_mask = {MES_TFEM, T("*setmaskentry %s |%s"), 'M'};
struct DirOptionOptCommandStruct DirOptionOptCommand_sort = {MES_MTSO, T("*setsortentry %s"), 'S'};
struct DirOptionOptCommandStruct DirOptionOptCommand_cmdedit = {MES_JMRU, T("*diroption %s cmde %s"), 'X'};

void DirOption_OptCommand(struct DirOptionMenuArgs *DOMA, struct DirOptionOptCommandStruct *doo, const TCHAR *param)
{
	TCHAR buf[CMDLINESIZE];
	const TCHAR *paramptr;
	size_t len;

	paramptr = (param != NULL) ? param : NilStr;

	// menu command
	if ( DOMA->opttype == NilStr ){
		buf[0] = '\0';
	} else{
		thprintf(buf, TSIZEOF(buf), doo->command, DOMA->opttype, paramptr);
	}
	ThAddString(DOMA->TH, buf);

	// menu title
	len = thprintf(buf, TSIZEOF(buf), T("%Ms...(&%c) : "),
			doo->menuname, doo->menuacc) - buf;
	GetLineParamS(&paramptr, buf + len, TSIZEOF(buf) - len);
	AppendMenuString(DOMA->hPopupMenu, (*DOMA->index)++, buf);
}

// *diroption
HMENU DirOptionMenu(PPC_APPINFO *cinfo, HMENU hPopupMenu, DWORD *index, ThSTRUCT *TH, DWORD nowmode, const TCHAR *path)
{
	TCHAR buf[VFPS], optpath[VFPS + 8];
	LOADSETTINGS_MOD ls;
	struct DirOptionMenuArgs DOMA;

	if ( hPopupMenu == NULL ) hPopupMenu = CreatePopupMenu();

	DOMA.hPopupMenu = hPopupMenu;
	DOMA.index = index;
	DOMA.TH = TH;
	DOMA.opttype = NilStr;

	PresetLs(ls, cinfo->dset);

	if ( nowmode == DSMD_TEMP ){
		DirOption_SetDirOptCommand(&DOMA, T("slow"),
			StrDsetSlowMode, (cinfo->SlowMode));
	} else if ( nowmode == DSMD_THIS_PATH ){
		LoadSetting_FixThisPath(buf, cinfo->path);
		LoadLs(&ls, buf);
		DOMA.opttype = T("-thispath");
	} else if ( nowmode == DSMD_THIS_BRANCH ){
		LoadSetting_FixThisBranch(buf, cinfo->path);
		LoadLs(&ls, buf);
		DOMA.opttype = T("-thisbranch");
	} else if ( nowmode == DSMD_ARCHIVE ){
		LoadLs(&ls, StrArchiveMode);
		DOMA.opttype = T("-archive");
	} else if ( nowmode == DSMD_LISTFILE ){
		LoadLs(&ls, StrListfileMode);
		DOMA.opttype = T("-listfile");
	} else if ( nowmode == DSMD_PATH_BRANCH ){
		if ( LoadLs(&ls, path) == FALSE ){
			thprintf(buf, TSIZEOF(buf), T("/%s"), path);
			LoadLs(&ls, buf);
		}
		thprintf(optpath, TSIZEOF(optpath), T("-\"%s\""), path);
		DOMA.opttype = optpath;
	}

	DirOption_SetDirOptCommand(&DOMA, T("watch"),
		StrDsetWatchDirModified, !(ls.cust.dset.flags & DSET_NODIRCHECK));

	DirOption_SetDirOptCommand(&DOMA, T("cache"),
		StrDsetCacheView, ls.cust.dset.flags & DSET_CACHEONLY);
	if ( !(ls.cust.dset.flags & DSET_CACHEONLY) ){
		DirOption_SetDirOptCommand(&DOMA, T("async"),
			StrDsetAsyncRead, ls.cust.dset.flags & DSET_ASYNCREAD);

		if ( ls.cust.dset.flags & DSET_ASYNCREAD ){
			DirOption_SetDirOptCommand(&DOMA, T("savetodisk"),
				StrDsetSaveDisk, !(ls.cust.dset.flags & DSET_NOSAVE_ACACHE));
			if ( !(ls.cust.dset.flags & DSET_NOSAVE_ACACHE) ){
				DirOption_SetDirOptCommand(&DOMA, T("everysave"),
					StrDsetSaveEveryTime, ls.cust.dset.flags & DSET_REFRESH_ACACHE);
			}
		}
	}
	DirOption_Icon_Menu(&DOMA, &DirOptionMenuData_Info, ls.cust.dset.infoicon);
	DirOption_Icon_Menu(&DOMA, &DirOptionMenuData_Entry, ls.cust.dset.cellicon);

	if ( nowmode == DSMD_TEMP ){
		thprintf(buf, TSIZEOF(buf), T("%Ms(&M): %s"), MES_TFEM,
				(cinfo->DsetMask[0] == MASK_NOUSE) ? cinfo->mask.file : cinfo->DsetMask);
		AppendMenuString(hPopupMenu, (*index)++, buf);
		ThAddString(TH, T("%K\"@\\F"));
	} else {
		TCHAR *p;
		const TCHAR *disp = NULL, *mask = NULL, *cmd = NULL, *sort = NULL;

		p = ls.buf->buf;
		while ( SkipSpace((const TCHAR **)&p) ){
			if ( !memcmp(p, LOADDISPSTR, SIZEOFTSTR(LOADDISPSTR)) ){
				p += TSIZEOFSTR(LOADDISPSTR);
				disp = p;
				GetLineParamS((const TCHAR **)&p, buf, TSIZEOF(buf));
				if ( *p >= ' ' ) *p++ = '\0';
				continue;
			}
			if ( !memcmp(p, LOADMASKSTR, SIZEOFTSTR(LOADMASKSTR)) ){
				p += TSIZEOFSTR(LOADMASKSTR);
				mask = p;
				GetLineParamS((const TCHAR **)&p, buf, TSIZEOF(buf));
				if ( *p >= ' ' ) *p++ = '\0';
				continue;
			}
			if ( !memcmp(p, LOADCMDSTR, SIZEOFTSTR(LOADCMDSTR)) ){
				p += TSIZEOFSTR(LOADCMDSTR);
				cmd = p;
				GetLineParamS((const TCHAR **)&p, buf, TSIZEOF(buf));
				if ( *p >= ' ' ) *p++ = '\0';
				continue;
			}
		}
		{ // sort
			int index = 0;

			for ( ; SortItems[index] != NULL ; index++ ){
				if ( ls.cust.dset.sort.mode.dat[0] == sortIDtable[index] ){
					thprintf(buf, TSIZEOF(buf), T("\"%Ms\""), SortItems[index]);
					sort = buf;
					break;
				}
			}
		}

		DirOption_OptCommand(&DOMA, &DirOptionOptCommand_view, disp);
		DirOption_OptCommand(&DOMA, &DirOptionOptCommand_mask, mask);
		DirOption_OptCommand(&DOMA, &DirOptionOptCommand_sort, sort);
		DirOption_OptCommand(&DOMA, &DirOptionOptCommand_cmdedit, cmd);
	}
	AppendMenuPopString(hPopupMenu, DispDiroptStringList);
	if ( cinfo->e.Dtype.mode == VFSDT_LFILE ){
		AppendMenuString(hPopupMenu, CRID_DIROPT_LISTFILE, StrDsetListfile);
	}
	if ( (cinfo->e.Dtype.mode == VFSDT_UN) ||
		(cinfo->e.Dtype.mode == VFSDT_SUSIE) ||
		(cinfo->e.Dtype.mode == VFSDT_ARCFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_CABFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_LZHFOLDER) ||
		(cinfo->e.Dtype.mode == VFSDT_ZIPFOLDER) ){
		AppendMenuString(hPopupMenu, CRID_DIROPT_ARCHIVE, StrDsetArchive);
	}

	if ( nowmode == DSMD_PATH_BRANCH ){
		AppendMenuString(hPopupMenu, CRID_DIROPT_PATH_BRANCH, path);
	}

	CheckMenuRadioItem(hPopupMenu, CRID_DIROPT, CRID_DIROPT_MODELAST - 1, CRID_DIROPT + nowmode, MF_BYCOMMAND);
	FreeLs(&ls);
	return hPopupMenu;
}
