/*-----------------------------------------------------------------------------
	Paper Plane cUI											対 Cell 処理
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPCUI.RH"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPC_DD.H"
#pragma hdrstop
						// 通常の指定に使う設定
const CURSORMOVER DefCM_page = { 0, 0, OUTTYPE_PAGE, 0, OUTTYPE_STOP, OUTHOOK_EDGE};
const CURSORMOVER DefCM_scroll = { 0, 0, OUTTYPE_LINESCROLL, 0, OUTTYPE_STOP, OUTHOOK_EDGE};

DWORD GetFileHeader(const TCHAR *filename, BYTE *header, DWORD headersize)
{
	HANDLE hFile;
	DWORD hsize;

	hFile = CreateFileL(filename, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) return 0;

	hsize = ReadFileHeader(hFile, header, headersize);
	CloseHandle(hFile);
	return hsize;
}

// ファイルをディレクトリ扱いにするか判定 -------------------------------------
BOOL IsFileDir(PPC_APPINFO *cinfo, const TCHAR *filename, TCHAR *newpath, TCHAR *newjumpname, HMENU hPopupMenu)
{
	BYTE header[VFS_check_size];
	DWORD fsize;					// 読み込んだ大きさ
	int i;
	TCHAR pathbuf[VFPS];

	if ( VFSFullPath(pathbuf, (TCHAR *)filename, cinfo->RealPath) == NULL ){
		if ( cinfo->RealPath[0] != '?' ) return FALSE;

		pathbuf[0] = '\0';
		if ( VFSFullPath(pathbuf, (TCHAR *)filename, cinfo->path) != NULL ){
			if ( VFSGetRealPath(cinfo->info.hWnd, pathbuf, pathbuf) == FALSE ){
				return FALSE;
			}
		}
		if ( pathbuf[0] == '\0' ) return FALSE;
	}
										// CLSID 形式 -------------------------
	i = FindExtSeparator(filename);
	if ( (*(filename + i) == '.') && (*(filename + i + 1) == '{') &&
			(tstrlen(filename + i) == 39)){
		if ( newpath != NULL ){
			CatPath(NULL, newpath, filename);
			*newjumpname = '\0';
		}
		if ( hPopupMenu == NULL ) return TRUE;
		AppendMenuString(hPopupMenu, CRID_DIRTYPE_CLSID, MES_JNCL);
		AppendMenuString(hPopupMenu, CRID_DIRTYPE_CLSID_PAIR, MES_JPCL);
	}
	fsize = GetFileHeader(pathbuf, header, sizeof(header));
	if ( fsize == 0 ){
		if ( hPopupMenu != NULL ){
			VFSCheckDir(pathbuf, header, fsize | VFSCHKDIR_GETDIRMENU, (void **)hPopupMenu);
			AppendMenuString(hPopupMenu, CRID_DIRTYPE_PAIR, MES_JPDI);
		}
		return FALSE;
	}
//------------------------------------------------ ショートカットファイルの判別
										// Link ?
	if ( (fsize > 100) &&
		 (*header == 0x4c) &&
		 (*(header + 1) == 0) &&
		 (*(header + 2) == 0) ){
		if ( newpath != NULL ){
			TCHAR lname[VFPS], lxname[VFPS];

			if ( SUCCEEDED(GetLink(cinfo->info.hWnd, pathbuf, lname)) && lname[0] ){
				DWORD attr;

				lxname[0] = '\0';
				if ( ExpandEnvironmentStrings(lname, lxname, TSIZEOF(lxname)) >= TSIZEOF(lxname) ){
					SetPopMsg(cinfo, POPMSG_MSG, MES_ESCT);
					return FALSE;
				}
				attr = GetFileAttributesL(lxname);
				if ( (attr & FILE_ATTRIBUTE_DIRECTORY) && (attr != BADATTR) ){
					tstrcpy(newpath, lxname);
					*newjumpname = '\0';
				}else{
					TCHAR *ptr;

					ptr = VFSFindLastEntry(lxname);
					tstrcpy(newjumpname, (*ptr == '\\') ? ptr + 1 : ptr);
					*ptr = '\0';
					tstrcpy(newpath, lxname);
				}
				return TRUE;
			}else{
				if ( hPopupMenu == NULL ){
					SetPopMsg(cinfo, POPMSG_MSG, MES_ESCT);
					return FALSE;
				}
			}
		}
		if ( hPopupMenu == NULL ) return TRUE;
		AppendMenuString(hPopupMenu, CRID_DIRTYPE_SHORTCUT, MES_JNSH);
		AppendMenuString(hPopupMenu, CRID_DIRTYPE_SHORTCUT_PAIR, MES_JPSH);
	}
//---------------------------------------------------------- VFS系の判別
	if ( VFSCheckDir(pathbuf, header, fsize | VFSCHKDIR_GETDIRMENU, (void **)hPopupMenu) ){
		if ( newpath != NULL ){
			tstrcpy(newpath, pathbuf);
			*newjumpname = '\0';
		}
		if ( hPopupMenu == NULL ) return TRUE;
	}else{
		if ( hPopupMenu == NULL ) return FALSE;
	}
//------------------------------------------------------------ ストリーム
	{
		HANDLE hFile;
		WIN32_STREAM_ID stid;
		DWORD size, reads;
		LPVOID context = NULL;
		WCHAR wname[VFPS];
		BY_HANDLE_FILE_INFORMATION finfo;

		hFile = CreateFileL(pathbuf, GENERIC_READ, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if ( hFile != INVALID_HANDLE_VALUE ){
			if ( IsTrue(GetFileInformationByHandle(hFile, &finfo)) ){
				if ( finfo.nNumberOfLinks >= 2){
					AppendMenuString(hPopupMenu, CRID_DIRTYPE_HARDLINKS, MES_JNHL);
					AppendMenuString(hPopupMenu, CRID_DIRTYPE_HARDLINKS_PAIR, MES_JPHL);
				}
			}

			for ( ; ; ){
				size = (LPBYTE)&stid.cStreamName - (LPBYTE)&stid;
				if ( FALSE == BackupRead(hFile, (LPBYTE)&stid,
						size, &reads, FALSE, FALSE, &context) ){
					break;
				}
				if ( reads < size ) break;
				if ( FALSE == BackupRead(hFile, (LPBYTE)&wname,
						stid.dwStreamNameSize, &reads, FALSE, FALSE, &context) ){
					break;
				}
				if ( reads < stid.dwStreamNameSize ) break;

				if ( stid.dwStreamNameSize ){
					AppendMenuString(hPopupMenu, CRID_DIRTYPE_STREAM, MES_JNST);
					AppendMenuString(hPopupMenu, CRID_DIRTYPE_STREAM_PAIR, MES_JPST);
					break;
//				}else{	// default name ... 登録不要
//
				}
				if ( FALSE == BackupSeek(hFile, *(DWORD *)(&stid.Size),
						*((DWORD *)(&stid.Size) + 1), &reads, &reads, &context) ){
					break;
				}
			}
			if ( context != NULL ){
				BackupRead(hFile, (LPBYTE)&stid, 0, &reads, TRUE, FALSE, &context);
			}
			CloseHandle(hFile);
		}
	}
	AppendMenuString(hPopupMenu, CRID_DIRTYPE_PAIR, MES_JPDI);
	return TRUE;
}

//-----------------------------------------------------------------------------
// ディレクトリかどうかを判別し、ファイルならフルパス名を生成する
ERRORCODE DirChk(const TCHAR *name, TCHAR *dest)
{
	TCHAR buf[VFPS], *wp;

	if ( GetFileAttributesL(name) == BADATTR ){
		ERRORCODE ec;

		ec = GetLastError();
		if ( (ec == ERROR_FILE_NOT_FOUND) ||
			 (ec == ERROR_PATH_NOT_FOUND) ||
			 (ec == ERROR_BAD_NETPATH) ||
			 (ec == ERROR_DIRECTORY) ){		// path not found
			tstrcpy(buf, name);
			wp = VFSFindLastEntry(buf);
			if ( *wp != '\0' ){
				*wp = '\0';
				if ( ec == ERROR_PATH_NOT_FOUND ){
					VFSTryDirectory(NULL, buf, TRYDIR_CHECKEXIST);
				}
				ec = DirChk(buf, dest);
			}
		}
		return ec;
	}else{
		VFSFullPath(dest, (TCHAR *)name, dest);
		return NO_ERROR;
	}
}

//-----------------------------------------------------------------------------
// ディレクトリ移動処理 IsFileRead =
BOOL CellLook(PPC_APPINFO *cinfo, int IsFileRead)
{
	TCHAR temp[VFPS], *p, *cellname;
	BOOL arcmode = FALSE;
	ENTRYCELL *cell;

	cell = &CEL(cinfo->e.cellN);
	if ( cell->attr & ECA_THIS ){	// "."
		PPC_RootDir(cinfo);
		return TRUE;
	}
	if ( cell->attr & ECA_PARENT ){	// ".."
		PPC_UpDir(cinfo);
		return TRUE;
	}
	// 書庫内書庫の判定＆展開
	if ( IsArcInArc(cinfo) || (IsFileRead == CellLook_ARCHIVE) ){
		arcmode = OnArcPathMode(cinfo);
	}
	cellname = CellFileName(cell);
																	// <dir>
	if ( cell->f.dwFileAttributes &
			(FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTEX_FOLDER) ){
		if ( IsTrue(cinfo->ChdirLock) ){
			if ( VFSFullPath(temp, cellname, cinfo->path) == NULL ){
				SetPopMsg(cinfo, POPMSG_GETLASTERROR, NULL);
				return TRUE;
			}
			PPCuiWithPathForLock(cinfo, temp);
			return TRUE;
		}

		if ( cinfo->UseArcPathMask != ARCPATHMASK_OFF ){
			SavePPcDir(cinfo, TRUE);
			DxSetMotion(cinfo->DxDraw, DXMOTION_DownDir);

			tstrcpy(cinfo->ArcPathMask, cellname);
			cinfo->ArcPathMaskDepth = GetEntryDepth(cinfo->ArcPathMask, NULL) + 1;
			cinfo->e.cellN = 0;
			MaskEntryMain(cinfo, &cinfo->mask, NilStr);

			if ( X_acr[1] ){ // カーソル位置を再現
				const TCHAR *vp;

				VFSFullPath(temp, cinfo->ArcPathMask, cinfo->OrgPath[0] ? cinfo->OrgPath : cinfo->path);
				UsePPx();
				vp = SearchHistory(PPXH_PPCPATH, temp);
				if ( vp != NULL ){
					const int *histp;

					histp = (const int *)GetHistoryData(vp);
					if ( (histp[1] >= 0) && (histp[1] < cinfo->e.cellIMax) ){
						cinfo->cellWMin = histp[1];
						InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
						RefleshInfoBox(cinfo, DE_ATTR_ENTRY | DE_ATTR_PAGE);
						if ( (histp[0] >= 0) && (histp[1] < cinfo->e.cellIMax) ){
							MoveCellCsr(cinfo, histp[0] - cinfo->e.cellN, NULL);
						}
					}
				}
				FreePPx();
			}
			SetCaption(cinfo);
			return TRUE;
		}
		SetPPcDirPos(cinfo);

#ifndef UNICODE
		if ( (strchr(cellname, '?') != NULL) &&
			cell->f.cAlternateFileName[0] ){
			if ( ChangePath(cinfo, cell->f.cAlternateFileName, CHGPATH_SETRELPATH) == FALSE ){
				SetPopMsg(cinfo, POPMSG_GETLASTERROR, NULL);
				return TRUE;
			}
		}else
#endif
		if ( tstrcmp(cinfo->path, T("\\\\+")) == 0 ){

			if ( tstrchr(cellname, ':') == NULL ){
				thprintf(temp, TSIZEOF(temp), T("\\\\%s"), cellname);
			}else{
				tstrcpy(temp, cellname);
			}
		}else if ( VFSFullPath(temp, cellname,
				( (cinfo->e.Dtype.mode == VFSDT_LFILE) &&
				  (cinfo->e.Dtype.BasePath[0] != '\0') ) ?
					cinfo->e.Dtype.BasePath : cinfo->path ) == NULL ){
			SetPopMsg(cinfo, POPMSG_GETLASTERROR, NULL);
			return TRUE;
		}
		ChangePath(cinfo, temp, CHGPATH_SETABSPATH);

										// PIDL UNC → FS UNC
		if ( (cinfo->path[0] == '\\') &&
			 (cinfo->path[1] == '\\') &&
			 (cinfo->e.Dtype.mode == VFSDT_SHN) ){
			temp[0] = '\0';
			VFSGetRealPath(cinfo->info.hWnd, temp, cinfo->path);
			if ( (temp[0] == '\\') && !tstrstr(temp, T("::")) ){
				ChangePath(cinfo, temp, CHGPATH_SETABSPATH);
			}
		}
		DxSetMotion(cinfo->DxDraw, DXMOTION_DownDir);

		read_entry(cinfo, RENTRY_READ | RENTRY_ENTERSUB);
		return TRUE;
	}
	tstrcpy(temp, cinfo->path);
	p = cellname;
	if ( IsTrue(cinfo->UnpackFix) ) p = FindLastEntryPoint(p);
	if ( (IsFileRead != PPEXTRESULT_NONE) &&
		 IsFileDir(cinfo, p, temp, cinfo->Jfname, NULL) ){
		if ( IsTrue(cinfo->ChdirLock) ){
			PPCuiWithPathForLock(cinfo, temp);
			return TRUE;
		}
		DxSetMotion(cinfo->DxDraw, DXMOTION_DownDir);

		SavePPcDir(cinfo, FALSE);
		if ( arcmode && (cinfo->OrgPath[0] == '\0') ){ // 書庫内書庫から出たときの場所を記憶する
			VFSFullPath(cinfo->OrgPath, cellname, cinfo->UnpackFixOldPath);
			OffArcPathMode(cinfo);
		}
		ChangePath(cinfo, temp, CHGPATH_SETABSPATH);
		if ( *cinfo->Jfname ){
			read_entry(cinfo, RENTRY_JUMPNAME);
		}else{
			read_entry(cinfo, RENTRY_READ);
		}
	}else{
		if ( IsTrue(arcmode) ) OffArcPathMode(cinfo);
		return FALSE;
	}
	return TRUE;
}

void SyncPairEntry(PPC_APPINFO *cinfo)
{
	TCHAR *entry;
	COPYDATASTRUCT copydata;

	if ( CEL(cinfo->e.cellN).type <= ECT_UPDIR ) return;
	entry = CEL(cinfo->e.cellN).f.cFileName;
	if ( *entry == '>' ){
		TCHAR *longname = (TCHAR *)EntryExtData_GetDATAptr(cinfo, DFC_LONGNAME, &CEL(cinfo->e.cellN));
		if ( longname != NULL ) entry = longname;
	}
	entry = FindLastEntryPoint(entry);

	copydata.dwData = KC_SYNCPATH;
	copydata.cbData = TSTRSIZE32(entry);
	copydata.lpData = entry;
	SendMessage(cinfo->hSyncViewPairWnd, WM_COPYDATA, 0, (LPARAM)&copydata);
}

//-----------------------------------------------------------------------------
// セル移動した時に行う処理
BOOL NewCellN(PPC_APPINFO *cinfo, int OcellN, BOOL fulldraw)
{
	TCHAR viewname[VFPS];
	const TCHAR *cellname;
	BOOL noreal;
	ENTRYCELL *cell;

	cell = &CEL(cinfo->e.cellN);
	if ( !(cell->f.dwFileAttributes & FILE_ATTRIBUTEX_VIRTUAL) && (cinfo->e.Dtype.mode != VFSDT_SHN) ){ // 通常パス
		cellname = GetCellFileName(cinfo, cell, viewname);
		noreal = FALSE;
	}else{ // 仮想のSHNパス...仮の名前を用意
		thprintf(viewname, TSIZEOF(viewname), T("%d%s"), cell->f.dwReserved1, cell->f.cFileName);
		cellname = viewname;
		noreal = TRUE;
	}

	if ( VFSFullPath(viewname, (TCHAR *)cellname, cinfo->path) == NULL ){
		cinfo->OldIconsPath[0] = '\0';
		if ( fulldraw == FALSE ){
			if ( !cinfo->FullDraw ){
				UpdateWindow_Part(cinfo->info.hWnd);	// 入れたほうが早い？
			}
			setflag(cinfo->DrawTargetFlags, DRAWT_1ENTRY);
			RefleshCell(cinfo, OcellN);
			if ( !cinfo->FullDraw ){
				UpdateWindow_Part(cinfo->info.hWnd);	// 入れたほうが早い？
			}
		}
		return TRUE;	// 表示できない
	}
															// 変化無し
	if ( (cinfo->e.cellN == OcellN) &&
		 (tstrcmp(viewname, cinfo->OldIconsPath) == 0) ){
		return FALSE;
	}
									// ドライブ一覧→タイトルバー修正
	if ( cinfo->path[0] == ':' ) SetCaption(cinfo);

	HideEntryTip(cinfo);
	if ( X_stip[TIP_LONG_TIME] != 0 ){
		setflag(cinfo->Tip.states, STIP_REQ_DELAY);
	}
									// カーソル位置のセルを描画
	tstrcpy(cinfo->OldIconsPath, viewname);
	if ( fulldraw == FALSE ){
		if ( !cinfo->FullDraw ){
			UpdateWindow_Part(cinfo->info.hWnd);	// 入れたほうが早い？
		}
		setflag(cinfo->DrawTargetFlags, DRAWT_1ENTRY);
		RefleshCell(cinfo, OcellN);
		if ( !cinfo->FullDraw ){
			UpdateWindow_Part(cinfo->info.hWnd);	// 入れたほうが早い？
		}
	}
									// 情報行アイコン取得
	#ifdef USEDIRECTX
		DxDrawFreeBMPCache(&cinfo->InfoIcon_Cache);
		cinfo->InfoIcon_DirtyCache = FALSE;
	#endif
	if ( cinfo->dset.infoicon >= DSETI_EXTONLY ){
		cinfo->hInfoIcon = LoadIcon(hInst, MAKEINTRESOURCE(Ic_LOADING));
		setflag(cinfo->SubTCmdFlags, SUBT_GETINFOICON);
		SetEvent(cinfo->SubT_cmd);
	}else{					 // その他
		cinfo->hInfoIcon = NULL;
	}
	if ( IsTrue(cinfo->UseSelectEvent) ){
		SendMessage(cinfo->info.hWnd, WM_PPXCOMMAND, K_E_SELECT, 0);
	}

	if ( cinfo->SlowMode == FALSE ){		// 連動関係の処理
		if ( cinfo->hSyncInfoWnd ){	// ^\[I] 処理
			HWND hEdit;

			hEdit = (HWND)SendMessage(cinfo->hSyncInfoWnd, WM_PPXCOMMAND, KE_getHWND, 0);
			if ( hEdit && IsWindow(hEdit) ){
				ThSTRUCT text;

				MakeFileInformation(cinfo, &text, cell);
				SetWindowText(hEdit, (TCHAR *)text.bottom);
				ThFree(&text);
			}else{
				cinfo->hSyncInfoWnd = NULL;
			}
		}
		if ( cinfo->SyncPathFlag != SyncPath_off ){	// ^[Y] 処理
			if ( GetFocus() == cinfo->info.hWnd ) SyncPairEntry(cinfo);
		}

		if ( cinfo->SyncViewFlag ){	// \[Y] 処理
			if ( !(cell->attr & (ECA_THIS | ECA_PARENT | ECA_DIR)) &&
				 (cell->state >= ECS_NORMAL) ){
				DWORD flags = PPV_NOFOREGROUND | PPV_NOENSURE | PPV_SYNCVIEW;

				if ( cell->f.nFileSizeHigh ||
					 (cell->f.nFileSizeLow > X_svsz) ){
					// でかいので時間が掛かる媒体は処理しない
					if (	(cinfo->e.Dtype.mode != VFSDT_PATH) &&
							(cinfo->e.Dtype.mode != VFSDT_SHN) &&
							(cinfo->e.Dtype.mode != VFSDT_LFILE) &&
							(cinfo->e.Dtype.mode != VFSDT_FATIMG) &&
							(cinfo->e.Dtype.mode != VFSDT_CDIMG) &&
							(cinfo->e.Dtype.mode != VFSDT_CDDISK) ){
						return TRUE;
					}
					setflag(flags, PPV_HEADVIEW);
				}
				// 書庫内のディレクトリと思われるエントリは表示しない
				if ( ((cinfo->e.Dtype.mode != VFSDT_UN) &&
					  (cinfo->e.Dtype.mode != VFSDT_SUSIE)) ||
					 ((cell->f.nFileSizeHigh == 0) &&
					  (cell->f.nFileSizeLow != 0)) ){
					if ( noreal ){
						 VFSFullPath(viewname, (TCHAR *)GetCellFileName(cinfo, cell, viewname), cinfo->path);
						 noreal = FALSE;
					}
					PPxView(cinfo->info.hWnd, viewname, flags | (cinfo->SyncViewFlag & ~1) );
				}
			}
		}
		if ( hPropWnd ){
			SYNCPROPINFO si;

			if ( noreal ){
				 VFSFullPath(viewname, (TCHAR *)GetCellFileName(cinfo, cell, viewname), cinfo->path);
//				 noreal = FALSE; // 未使用
			}
			si.ff = cell->f;
			tstrcpy(si.filename, viewname);
			SyncProperties(cinfo->info.hWnd, &si);
		}
	}
	DNotifyWinEvent(EVENT_OBJECT_FOCUS, cinfo->info.hWnd, OBJID_CLIENT, cinfo->e.cellN + 1);
	return TRUE;
}
									// 現在ドライブの次のドライブに移動
void JumpNextDrive(PPC_APPINFO *cinfo, int offset)
{
	DWORD drv;
	int nowdrive, i;
	TCHAR buf[VFPS];

	drv = GetLogicalDrives();
	nowdrive = upper(cinfo->path[0]) - 'A';	// '\\' は適当に処理
	for ( i = 0 ; i < 26 ; i++ ){
		nowdrive += offset;
		if ( nowdrive < 0 ) nowdrive = 'Z' - 'A';
		if ( nowdrive > ('Z' - 'A') ) nowdrive = 0;
		if ( drv & (LSBIT << nowdrive) ) break;
	}
	thprintf(buf, TSIZEOF(buf), T("%c:"), nowdrive + 'A');
	SetPPcDirPos(cinfo);
	ChangePath(cinfo, buf, CHGPATH_SETRELPATH);
	read_entry(cinfo, RENTRY_READ);
}
										// 別の移動設定でカーソル移動する
void USEFASTCALL UseMoveKey(PPC_APPINFO *cinfo, int offset, CURSORMOVER *cm)
{
	int type1, type2;

	type1 = cm->outw_type;
	type2 = cm->outr_type;
	if (	(type1 == OUTTYPE_LRKEY) ||
			(type2 == OUTTYPE_LRKEY) ||
			(type1 == OUTTYPE_PAGEKEY) ||
			(type2 == OUTTYPE_PAGEKEY) ){
		SetPopMsg(cinfo, POPMSG_MSG, T("nested error"));
		return;
	}
	if ( offset > 0 ){
		MoveCellCursor(cinfo, cm);
	}else{
		MoveCellCursorR(cinfo, cm);
	}
}
										// カーソルがはみ出したときの処理
void USEFASTCALL DoOutOfRange(PPC_APPINFO *cinfo, int type, int offset)
{
	switch (type){
		case OUTTYPE_PAIR:			// 反対窓
			PPcChangeWindow(cinfo, PPCHGWIN_PAIR);
			break;
		case OUTTYPE_FPPC:			// 前
			PPcChangeWindow(cinfo, offset > 0 ? PPCHGWIN_BACK : PPCHGWIN_NEXT);
			break;
		case OUTTYPE_NPPC:			// 次
			PPcChangeWindow(cinfo, offset > 0 ? PPCHGWIN_NEXT : PPCHGWIN_BACK);
			break;
		case OUTTYPE_PARENT:		// 親に移動
			PPC_UpDir(cinfo);
			break;
		case OUTTYPE_DRIVE:		// \[L]
			PPC_DriveJump(cinfo, FALSE);
			break;
		case OUTTYPE_FDRIVE:		// ドライブ移動
			JumpNextDrive(cinfo, offset > 0 ? -1 : 1);
			break;
		case OUTTYPE_NDRIVE:		// ドライブ移動
			JumpNextDrive(cinfo, offset > 0 ? 1 : -1);
			break;
		case OUTTYPE_LRKEY:			// ←→
			UseMoveKey(cinfo, offset, &XC_mvLR);
			break;
		case OUTTYPE_PAGEKEY:		// PgUp/dw
			UseMoveKey(cinfo, offset, &XC_mvPG);
			break;
		case OUTTYPE_RANGEEVENT12: // RANGEEVENT1～8
		case OUTTYPE_RANGEEVENT34:
		case OUTTYPE_RANGEEVENT56:
		case OUTTYPE_RANGEEVENT78:
			PPcCommand(cinfo, (WORD)( K_E_RANGE1 +
				(type - OUTTYPE_RANGEEVENT12) * 2 + ((offset > 0) ? 1 : 0)) );
			break;
		default:
			SetPopMsg(cinfo, POPMSG_MSG, T("XC_mv param error"));
	}
}

// 窓間移動優先処理
BOOL USEFASTCALL CheckPairJump(PPC_APPINFO *cinfo, int offset)
{
	int site;

	if ( cinfo->combo ){
		if ( NULL == (HWND)SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
				KCW_getpairwnd, (LPARAM)cinfo->info.hWnd) ){
			return FALSE;
		}
		site = PPcGetSite(cinfo);
		if ( site == PPCSITE_SINGLE ) return FALSE;
		if ( (offset >= 0) ? (site == PPCSITE_RIGHT) : (site == PPCSITE_LEFT) ){
			return FALSE;
		}
		PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
				TMAKELPARAM(KCW_nextppc, 0), (LPARAM)cinfo->info.hWnd);
		return TRUE;
	}

	if ( GetJoinWnd(cinfo) == NULL ) return FALSE;
	site = cinfo->RegID[2] & PAIRBIT;
	if ( (cinfo->swin & SWIN_JOIN) && (cinfo->swin & SWIN_SWAPMODE) ){
		site ^= PAIRBIT;
	}
	if ( offset < 0 ) site ^= PAIRBIT;
	if ( site == PPCSITE_SINGLE ) return FALSE;
	PPcChangeWindow(cinfo, (offset > 0) ? PPCHGWIN_NEXT : PPCHGWIN_BACK);
	return TRUE;
}

// カーソル位置を範囲内に補正する
void FixCellRange(PPC_APPINFO *cinfo, int scroll, int *NcellWMin, int cellN)
{
	int PageEntries; // １ページ分のエントリ数
	int MaxWMin; // NcellWMinの最大値

	scroll = scroll ? 0 : XC_fwin;
	PageEntries = cinfo->cel.Area.cx * cinfo->cel.Area.cy;
	if ( PageEntries > cinfo->e.cellIMax ) PageEntries = cinfo->e.cellIMax;

	if ( scroll == 0 ){			// 隙間無し
		MaxWMin = cinfo->e.cellIMax - PageEntries;
	}else if ( XC_mvUD.outw_type == OUTTYPE_LINESCROLL ){
		MaxWMin = cinfo->e.cellIMax - 1;
		if ( !cinfo->list.orderZ &&
			 (cinfo->cel.Area.cy > (XC_smar * SMAR_MIN)) ){
			 MaxWMin -= XC_smar;
		}
	}else if ( scroll == 1 ){		// 桁/行単位
		if ( cinfo->list.orderZ ){
			*NcellWMin -= *NcellWMin % cinfo->cel.Area.cx;
			if ( (cellN == -2) && (*NcellWMin == cinfo->cellWMin) ){
				*NcellWMin += cinfo->cel.Area.cx;
			}
		}else{
			*NcellWMin -= *NcellWMin % cinfo->cel.Area.cy;
			if ( (cellN == -2) && (*NcellWMin == cinfo->cellWMin) ){
				*NcellWMin += cinfo->cel.Area.cy;
			}
		}
		MaxWMin = (cinfo->e.cellIMax - PageEntries + cinfo->cel.Area.cy - 1) /
				cinfo->cel.Area.cy * cinfo->cel.Area.cy;
	}else{						// ページ単位
		*NcellWMin -= *NcellWMin % PageEntries;
		if ( (cellN == -2) && (*NcellWMin == cinfo->cellWMin) ){
			*NcellWMin += PageEntries;
		}
		if ( cellN >= 0 ){
			*NcellWMin += ((cellN - *NcellWMin) / PageEntries) * PageEntries;
		}
		MaxWMin = cinfo->e.cellIMax - (cinfo->e.cellIMax % PageEntries);
	}
	if ( *NcellWMin > MaxWMin ) *NcellWMin = MaxWMin;
	if ( *NcellWMin < 0 ) *NcellWMin = 0;
}

//-----------------------------------------------------------------------------
// 位置が変わったら真になる
BOOL MoveCellCsr(PPC_APPINFO *cinfo, int offset, const CURSORMOVER *cm)
{
	int PageEntries;			// １ページ分のエントリ数
	ENTRYINDEX NcellN, NcellWMin;		// 新しいカーソル位置と表示開始位置
	int limit, flag = 0;
	int scroll, OcellN;
	BOOL CellFix = TRUE;
	BOOL PointMode = FALSE;

	DEBUGLOGC("MoveCellCsr start (Focus:%x)", GetFocus());

	// cellN 関係の範囲調整(主にメモリ破損？)
	if ( cinfo->e.cellIMax <= 0 ){
		cinfo->e.cellIMax = 1;
	}
	if ( cinfo->e.cellN >= cinfo->e.cellIMax ){
		cinfo->e.cellN = cinfo->e.cellIMax - 1;
	}
	if ( cinfo->e.cellN < 0 ){
		cinfo->e.cellN = 0;
	}
										// カスタマイズ関係の情報を用意 -------
	if ( cm <= CMOVER_POINT_MAX ){
		if ( cm == CMOVER_POINT ) PointMode = TRUE;
		cm = cinfo->list.scroll ? &DefCM_scroll : &DefCM_page;
	}
	PageEntries = cinfo->cel.Area.cx * cinfo->cel.Area.cy;
	if ( PageEntries > cinfo->e.cellIMax ) PageEntries = cinfo->e.cellIMax;
	if ( cm->outw_type == OUTTYPE_LINESCROLL ){
		scroll = 1;
		if ( cinfo->list.orderZ ){
			limit = 0;
		}else{
			if ( !PointMode && (cinfo->cel.Area.cy > (XC_smar * SMAR_MIN)) ){
				limit = XC_smar + 1;
			}else{
				limit = 0;
			}
		}
	}else{
		scroll = 0;
		limit = 0;
	}
										// 新しい移動先（希望）を算出 ---------
	if ( cm->unit < CMOVEUNIT_EXTRA_MIN ){
		switch( cm->unit & CMOVEUNIT_TYPEMASK ){
			case CMOVEUNIT_TYPESCROLL: // カーソル移動＋スクロール移動
				if ( cinfo->list.orderZ ) offset *= cinfo->cel.Area.cx;
				NcellN = cinfo->e.cellN + offset;
				NcellWMin = cinfo->cellWMin + offset;
				break;

			case CMOVEUNIT_TYPELOCKSCROLL: // スクロール移動のみ
				if ( cinfo->list.orderZ ) offset *= cinfo->cel.Area.cx;
				NcellN = cinfo->e.cellN;
				NcellWMin = cinfo->cellWMin + offset;

				if ( (NcellWMin < 0) || (NcellWMin >= cinfo->e.cellIMax) ){
//					if ( cm->outw_hook & OUTHOOK_EDGE ){ // 先端移動オプション
//					}
					if ( (cm->outw_hook & OUTHOOK_HOOK) && cinfo->KeyRepeats ){  // 引っ掛けオプション
						return TRUE;
					}
					if ( cm->outw_hook & OUTHOOK_WINDOWJUMP ){	// 窓間移動優先
						if ( CheckPairJump(cinfo, offset) ){
							return TRUE;
						}
					}
					if ( cm->outw_type == OUTTYPE_STOP ){
						return FALSE;
					}
					if ( cm->outw_type >= OUTTYPE_PARENT ){
						DoOutOfRange(cinfo, cm->outw_type, offset);
						return FALSE;
					}
				}
											// 表示開始位置の範囲外補正
				FixCellRange(cinfo, scroll, &NcellWMin, (offset > 0) ? -2 : -1);
				if ( !XC_nsbf ){
					CellFix = FALSE;
				}else{
					int FixType = 0;
											// カーソルの範囲外補正
					if ( NcellN < (NcellWMin + limit) ){
						NcellN = NcellWMin + limit;
						FixType = 1;
					}
					if ( NcellN >= (NcellWMin + PageEntries - limit) ){
						NcellN = NcellWMin + PageEntries - limit - 1;
						FixType = 1;
					}
					if ( NcellN >= cinfo->e.cellIMax ){
						NcellN = cinfo->e.cellIMax - 1;
						FixType = 1;
					}
					if ( FixType ){
						if ( cm->outr_hook & OUTHOOK_EDGE ){
							if ( (NcellN != cinfo->e.cellN) ||
								 (NcellWMin != cinfo->cellWMin) ){
								break;
							}
						}
						if ( cm->outr_hook & OUTHOOK_WINDOWJUMP ){
							if ( CheckPairJump(cinfo, offset) ){
								return TRUE;
							}
						}
						if ( cm->outr_type == OUTTYPE_STOP ){
							return FALSE;
						}
						if ( cm->outr_type >= OUTTYPE_PARENT ){
							DoOutOfRange(cinfo, cm->outr_type, offset);
							return FALSE;
						}
					}
				}
				break;

			case CMOVEUNIT_TYPEDEFAULT: // 既定値
				scroll = cinfo->list.scroll;
				// default へ
			default:	// カーソル移動のみ(CMOVEUNIT_TYPEPAGE)/既定値
				NcellN = cinfo->e.cellN + offset;
				NcellWMin = cinfo->cellWMin;
				break;
		}
	}else{
		switch( cm->unit ){
			case CMOVEUNIT_MARKUPDOWN:
				scroll = cinfo->list.scroll;
				NcellN = cinfo->e.cellN;
				NcellWMin = cinfo->cellWMin;
				if ( offset > 0 ){
					while ( offset ){
						NcellN = DownSearchMarkCell(cinfo, NcellN);
						if ( NcellN < 0 ){
							NcellN = cinfo->e.cellN;
							break;
						}
						offset--;
					}
				}else{
					while ( offset ){
						NcellN = UpSearchMarkCell(cinfo, NcellN);
						if ( NcellN < 0 ){
							NcellN = cinfo->e.cellN;
							break;
						}
						offset++;
					}
				}
				break;

			case CMOVEUNIT_MARKPREVNEXT:
				scroll = cinfo->list.scroll;
				NcellN = cinfo->e.cellN;
				NcellWMin = cinfo->cellWMin;
				if ( !IsCEL_Marked(NcellN) ){
					if ( cinfo->e.markC == 0 ) break;
					NcellN = GetCellIndexFromCellData(cinfo,
						(offset >= 0) ? cinfo->e.markLast : cinfo->e.markTop);
					if ( NcellN < 0 ) NcellN = cinfo->e.cellN;
					break;
				}
				if ( offset > 0 ){
					while ( offset ){
						NcellN = GetNextMarkCell(cinfo, NcellN);
						if ( NcellN < 0 ){
							NcellN = cinfo->e.cellN;
							break;
						}
						offset--;
					}
				}else{
					while ( offset ){
						NcellN = CEL(NcellN).mark_bk;
						if ( NcellN == ENDMARK_ID ){
							NcellN = cinfo->e.cellN;
							break;
						}
						NcellN = GetCellIndexFromCellData(cinfo, NcellN);
						if ( NcellN < 0 ){
							NcellN = cinfo->e.cellN;
							break;
						}
						offset++;
					}
				}
				break;

			case CMOVEUNIT_MARKFIRSTLAST:
				scroll = cinfo->list.scroll;
				NcellN = cinfo->e.cellN;
				NcellWMin = cinfo->cellWMin;
				if ( cinfo->e.markC != 0 ){
					ENTRYINDEX index;

					index = GetCellIndexFromCellData(cinfo,
						(offset < 0) ? cinfo->e.markLast : cinfo->e.markTop);
					if ( index >= 0 ) NcellN = index;
				}
				break;

			case CMOVEUNIT_FILE: {
				ENTRYINDEX index;
				DWORD attr;

				scroll = cinfo->list.scroll;
				NcellN = cinfo->e.cellN;
				NcellWMin = cinfo->cellWMin;
				if ( offset >= 0 ){
					attr = (offset == 1) ? FILE_ATTRIBUTE_DIRECTORY : offset;
					for ( index = cinfo->e.cellN + 1; index < cinfo->e.cellIMax; index++){
						if ( !(CEL(index).f.dwFileAttributes & attr) ){
							NcellN = index;
							break;
						}
					}
				}else{
					attr = (offset == -1) ? FILE_ATTRIBUTE_DIRECTORY : offset & 0xffff;
					for ( index = cinfo->e.cellN - 1; index >= 0; index--){
						if ( !(CEL(index).f.dwFileAttributes & attr) ){
							NcellN = index;
							break;
						}
					}
				}
				break;
			}

			case CMOVEUNIT_DIRECTORY: {
				ENTRYINDEX index;
				DWORD attr;

				scroll = cinfo->list.scroll;
				NcellN = cinfo->e.cellN;
				NcellWMin = cinfo->cellWMin;
				if ( offset >= 0 ){
					attr = (offset == 1) ? FILE_ATTRIBUTE_DIRECTORY : offset;
					for ( index = cinfo->e.cellN + 1; index < cinfo->e.cellIMax; index++){
						if ( CEL(index).f.dwFileAttributes & attr ){
							NcellN = index;
							break;
						}
					}
				}else{
					attr = (offset == -1) ? FILE_ATTRIBUTE_DIRECTORY : offset & 0xffff;
					for ( index = cinfo->e.cellN - 1; index >= 0; index--){
						if ( CEL(index).f.dwFileAttributes & attr ){
							NcellN = index;
							break;
						}
					}
				}
				break;
			}

			case CMOVEUNIT_VIEWUPDOWN: {
				ENTRYINDEX index;
				ENTRYCELL *cell;

				scroll = cinfo->list.scroll;
				NcellN = cinfo->e.cellN;
				NcellWMin = cinfo->cellWMin;
				if ( offset >= 0 ){
					for ( index = cinfo->e.cellN + 1; index < cinfo->e.cellIMax; index++){
						NcellN = index;
						cell = &CEL(index);
						if ( cell->attr & (ECA_PARENT | ECA_THIS | ECA_ETC) ){
							continue;
						}
						if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
							break;
						}
						if ( cell->f.nFileSizeHigh | CEL(index).f.nFileSizeLow ){
							break;
						}
					}
				}else{
					for ( index = cinfo->e.cellN - 1; index >= 0; index--){
						NcellN = index;
						cell = &CEL(index);
						if ( cell->attr & ECA_ETC ){
							continue;
						}
						if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
							break;
						}
						if ( cell->f.nFileSizeHigh | CEL(index).f.nFileSizeLow ){
							break;
						}
					}
				}
				break;
			}
		}
	}
										// カーソルが両端を超えたときの処理 ---
	while ( NcellN < 0 ){					// 最小値--------------------------
		if ( NcellWMin > 0 ){
			NcellN = 0;
			break;
		}

		flag = 1;
		if ( cm->outr_hook & OUTHOOK_EDGE ){		// 先端移動オプション
			if ( cinfo->e.cellN != 0 ){
				NcellN = 0;
				break;
			}
		}
		if ( (cm->outr_hook & OUTHOOK_HOOK) && cinfo->KeyRepeats ){ // 引っ掛けオプション
			NcellN = cinfo->e.cellN;
			break;
		}
		if ( cm->outr_hook & OUTHOOK_WINDOWJUMP ){	// 窓間移動優先
			if ( CheckPairJump(cinfo, offset) ){
				NcellN = cinfo->e.cellN;
				NcellWMin = cinfo->cellWMin;
				break;
			}
		}
		switch ( cm->outr_type ){
			case OUTTYPE_STOP:			// 停止
				NcellN = cinfo->e.cellN;
				break;
			case OUTTYPE_WL:			// 画面反対(線対称)
				NcellN = (cinfo->cel.Area.cx - 1) * cinfo->cel.Area.cy +
						 (cinfo->e.cellN - cinfo->cellWMin) % cinfo->cel.Area.cy;
				if ( NcellN > (cinfo->e.cellIMax - 1) ){
					NcellN = cinfo->e.cellIMax - 1;
				}
				break;
			case OUTTYPE_WP:			// 画面反対(点対称)
				NcellN = cinfo->cellWMin + PageEntries - 1;
				if ( NcellN > (cinfo->e.cellIMax - 1) ){
					NcellN = cinfo->e.cellIMax - 1;
				}
				break;
			case OUTTYPE_LINESCROLL:	// 行スクロール
			case OUTTYPE_COLUMNSCROLL:	// 桁スクロール
			case OUTTYPE_PAGE:			// ページ切替
			case OUTTYPE_HALFPAGE:		// 半ページ切替
				NcellN = cinfo->e.cellIMax - 1;
				break;
			default:
				DoOutOfRange(cinfo, cm->outr_type, offset);
				DEBUGLOGC("MoveCellCsr nomove end %d", offset);
				return FALSE;
		}
		break;
	}
	while ( NcellN >= cinfo->e.cellIMax ){	// 最大値--------------------------
		if ( (NcellWMin + PageEntries) < cinfo->e.cellIMax ){
			NcellN = cinfo->e.cellIMax - 1;
			break;
		}

		flag = 1;
		if ( cm->outr_hook & OUTHOOK_EDGE ){ // 先端移動オプション
			if ( cinfo->e.cellN != (cinfo->e.cellIMax - 1) ){
				NcellN = cinfo->e.cellIMax - 1;
				break;
			}
		}
		if ( (cm->outr_hook & OUTHOOK_HOOK) && cinfo->KeyRepeats ){ // 引っ掛けオプション
			NcellN = cinfo->e.cellN;
			break;
		}
		if ( cm->outr_hook & OUTHOOK_WINDOWJUMP ){ // 窓間移動優先
			if ( CheckPairJump(cinfo, offset) ){
				NcellN = cinfo->e.cellN;
				NcellWMin = cinfo->cellWMin;
				break;
			}
		}
		switch ( cm->outr_type ){
			case OUTTYPE_STOP:			// 停止
				NcellN = cinfo->e.cellN;
				break;
			case OUTTYPE_WL:			// 画面反対(線対称)
				NcellN = (cinfo->e.cellN - cinfo->cellWMin) % cinfo->cel.Area.cy;
				break;
			case OUTTYPE_WP:			// 画面反対(点対称)
				NcellN = cinfo->cellWMin;
				break;
			case OUTTYPE_LINESCROLL:	// 行スクロール
			case OUTTYPE_COLUMNSCROLL:	// 桁スクロール
			case OUTTYPE_PAGE:			// ページ切替
			case OUTTYPE_HALFPAGE:		// 半ページ切替
				NcellN = 0;
				break;
			default:
				DoOutOfRange(cinfo, cm->outr_type, offset);
				DEBUGLOGC("MoveCellCsr nomove end %d", offset);
				return FALSE;
		}
		break;
	}

	if ( IsTrue(CellFix) ){
		DEBUGLOGC("MoveCellCsr CellFix %d", offset);
										// 表示領域の調節 ---
		FixCellRange(cinfo, scroll, &NcellWMin, -1);
										// 窓より左 ---------------------------
		while ( NcellN < (NcellWMin + limit ) ){
			int delta;

			if ( flag ){
				NcellWMin = (NcellN / cinfo->cel.Area.cy) * cinfo->cel.Area.cy;
				break;
			}

			if ( cm->outw_hook & OUTHOOK_EDGE ){
				if ( cm->outw_type == OUTTYPE_LINESCROLL ){
					if ( (NcellN < NcellWMin) && (cinfo->e.cellN != NcellWMin) ){
						NcellN = NcellWMin;
						break;
					}
				}else{
					if ( cinfo->e.cellN != NcellWMin ){
						NcellN = NcellWMin;
						break;
					}
				}
			}
			if ( (cm->outw_hook & OUTHOOK_HOOK) && cinfo->KeyRepeats ){ // 引っ掛け処理
				NcellN = cinfo->e.cellN;
				break;
			}
			if ( cm->outw_hook & OUTHOOK_WINDOWJUMP ){ //窓間移動優先
				if ( CheckPairJump(cinfo, offset) ){
					NcellN = cinfo->e.cellN;
					NcellWMin = cinfo->cellWMin;
					break;
				}
			}
			delta = (NcellWMin + limit) - NcellN - 1;
			if ( delta <= 0 ) delta = 1;
			switch ( cm->outw_type ){
				case OUTTYPE_STOP:			// 停止
					NcellN = cinfo->e.cellN;
					break;
				case OUTTYPE_WL:			// 画面反対
				case OUTTYPE_WP:			// 画面反対
					NcellN = NcellWMin + PageEntries - 1;
					if ( NcellN > (cinfo->e.cellIMax - 1) ){
						NcellN = cinfo->e.cellIMax - 1;
					}
					break;
				case OUTTYPE_LINESCROLL:	// 行スクロール
					if ( cinfo->list.orderZ ) delta *= cinfo->cel.Area.cx;
					NcellWMin -= delta;
					scroll = 1;
					break;
				case OUTTYPE_COLUMNSCROLL:
					if ( cinfo->list.orderZ == 0 ){	// 桁スクロール
						NcellWMin -= (delta / cinfo->cel.Area.cy + 1) *
								cinfo->cel.Area.cy;
					}else{	// 行スクロール
						NcellWMin -= delta * cinfo->cel.Area.cx;
						scroll = 1;
					}
					break;
				case OUTTYPE_PAGE:			// ページ切替
					NcellWMin -= (delta / PageEntries + 1) * PageEntries;
					break;
				case OUTTYPE_HALFPAGE:		// 半ページ切替
					NcellWMin -= (delta / (PageEntries/2) + 1) *(PageEntries/2);
					scroll = 1;
					break;
				default:
					DoOutOfRange(cinfo, cm->outw_type, offset);
					DEBUGLOGC("MoveCellCsr nomove end %d", offset);
					return FALSE;
			}
			break;
		}
										// 窓より右 ---------------------------
		while ( NcellN >= (NcellWMin + PageEntries - limit) ){
			int delta;

			if ( flag ){
				NcellWMin = (NcellN / cinfo->cel.Area.cy) * cinfo->cel.Area.cy -
						((cinfo->cel.Area.cx < 1) ?
							0 : (cinfo->cel.Area.cx - 1) * cinfo->cel.Area.cy);
				break;
			}
			if ( cm->outw_hook & OUTHOOK_EDGE ){
				if ( cinfo->e.cellN < (NcellWMin + PageEntries - limit - 1) ){
					NcellN = NcellWMin + PageEntries - limit - 1;
					if ( NcellN < NcellWMin ) NcellWMin = NcellN;
					break;
				}
			}
			if ((cm->outw_hook & OUTHOOK_HOOK) && cinfo->KeyRepeats){ // 引っ掛け処理
				NcellN = cinfo->e.cellN;
				break;
			}
			if ( cm->outw_hook & OUTHOOK_WINDOWJUMP ){ //窓間移動優先
				if ( CheckPairJump(cinfo, offset) ){
					NcellN = cinfo->e.cellN;
					NcellWMin = cinfo->cellWMin;
					break;
				}
			}
			delta = NcellN - (NcellWMin + PageEntries - limit);
			if ( delta <= 0 ) delta = 1;
			switch (cm->outw_type){
				case OUTTYPE_STOP:			// 停止
					NcellN = cinfo->e.cellN;
					break;
				case OUTTYPE_WL:			// 画面反対
				case OUTTYPE_WP:			// 画面反対
					NcellN = NcellWMin;
					break;
				case OUTTYPE_LINESCROLL:	// 行スクロール
					if ( cinfo->list.orderZ ) delta *= cinfo->cel.Area.cx;
					NcellWMin += delta;
					scroll = 1;
					break;
				case OUTTYPE_COLUMNSCROLL:	// 桁スクロール
					if ( cinfo->list.orderZ == 0 ){	// 桁スクロール
						NcellWMin += (delta / cinfo->cel.Area.cy + 1) *
								cinfo->cel.Area.cy;
					}else{	// 行スクロール
						NcellWMin += delta * cinfo->cel.Area.cx;
						scroll = 1;
					}
					break;
				case OUTTYPE_PAGE:			// ページ切替
					NcellWMin += (delta / PageEntries + 1) * PageEntries;
					break;
				case OUTTYPE_HALFPAGE:		// 半ページ切替
					NcellWMin += (delta / (PageEntries/2) + 1) *(PageEntries/2);
					scroll = 1;
					break;
				default:
					DoOutOfRange(cinfo, cm->outw_type, offset);
					DEBUGLOGC("MoveCellCsr nomove end %d", offset);
					return FALSE;
			}
			if ( NcellN < NcellWMin ) NcellWMin = NcellN;
			break;
		}
										// 表示領域の調節 ---
		FixCellRange(cinfo, scroll, &NcellWMin, NcellN);
	}
	OcellN = cinfo->e.cellN;
	if ( SelfDD_hWnd == NULL ) cinfo->e.cellN = NcellN;
										// Explorer風選択処理
	if ( cm->outw_hook & OUTHOOK_MARK ){
		int markstart = -1, markend C4701CHECK;
		int unmarkstart = -1, unmarkend C4701CHECK;

		if ( cinfo->e.cellNref == -1 ) cinfo->e.cellNref = OcellN;
		if ( cinfo->e.cellNref > OcellN ){	// 基準点より前が前回位置
			if ( cinfo->e.cellN < OcellN ){	// 前回位置より現在位置が前→増加
				markstart = cinfo->e.cellN;
				markend = OcellN;
			}else{							// 後→消去
				unmarkstart = OcellN;
				unmarkend = cinfo->e.cellN - 1;
				if ( cinfo->e.cellNref <= unmarkend ){ // 基準点より後が現在位置
					markstart = cinfo->e.cellNref;
					markend = cinfo->e.cellN;
					unmarkend = markstart - 1;
				}
			}
		}else{
			if ( cinfo->e.cellN > OcellN ){	// 前回位置より現在位置が後→増加
				markstart = OcellN;
				markend = cinfo->e.cellN;
			}else{							// 後→消去
				unmarkstart = cinfo->e.cellN + 1;
				unmarkend = OcellN;
				if ( cinfo->e.cellNref >= unmarkstart ){ // 基準点より前が現在位置
					markstart = cinfo->e.cellN;
					markend = cinfo->e.cellNref;
					unmarkstart = markend + 1;
				}
			}
		}
		cinfo->MarkMask = MARKMASK_DIRFILE;
		if ( unmarkstart != -1 ){
			setflag(cinfo->DrawTargetFlags, DRAWT_ENTRY);
			while ( unmarkstart <= unmarkend ){ // C4701ok
				if ( IsCEL_Marked(unmarkstart) ){
					CellMark(cinfo, unmarkstart, MARK_REMOVE);
					RefleshCell(cinfo, unmarkstart);
					RefleshInfoBox(cinfo, DE_ATTR_MARK);
				}
				unmarkstart++;
			}
		}
		if ( markstart != -1 ){
			setflag(cinfo->DrawTargetFlags, DRAWT_ENTRY);
			while ( markstart <= markend ){ // C4701ok
				if ( !IsCEL_Marked(markstart) ){
					CellMark(cinfo, markstart, MARK_CHECK);
					RefleshCell(cinfo, markstart);
					RefleshInfoBox(cinfo, DE_ATTR_MARK);
				}
				markstart++;
			}
		}
	}else{	// 選択基準位置を更新
		if ( offset != 0 ){
			cinfo->e.cellNref = cinfo->e.cellN;
		}
	}

	if ( cinfo->cellWMin != NcellWMin ){				// スクロール表示
		POINT DragSelectPos;
		RECT DragSelectArea;

		DEBUGLOGC("MoveCellCsr scroll %d", offset);
		if ( (cinfo->MouseStat.PushButton > MOUSEBUTTON_CANCEL) &&
			 (cinfo->MousePush == MOUSE_MARK) ){
			RECT DragSelectArea;

			GetCursorPos(&DragSelectPos);
			ScreenToClient(cinfo->info.hWnd, &DragSelectPos);

			CalcDragTarget(cinfo, &DragSelectPos, &DragSelectArea);
			DrawDragFrame(cinfo->info.hWnd, &DragSelectArea);
			MarkDragArea(cinfo, &DragSelectArea, MARK_REVERSE);
		}
		#ifndef USEDIRECTX
		if ( (cinfo->list.orderZ) || // ●暫定
			 (cinfo->bg.X_WallpaperType != 0) ||
			(cinfo->list.scroll && (C_eInfo[ECS_EVEN] != C_AUTO)) ){
		#endif
										// 壁紙表示時は全画面更新
			cinfo->e.cellPoint = -1;
			NewCellN(cinfo, OcellN, TRUE);
			cinfo->cellWMin = NcellWMin;
			cinfo->DrawTargetFlags = DRAWT_ALL;
			InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
			RefleshInfoBox(cinfo, DE_ATTR_ENTRY | DE_ATTR_PAGE);
		#ifndef USEDIRECTX
		}else{
			int xoffset, yoffset;

			xoffset = (cinfo->cellWMin - NcellWMin) / cinfo->cel.Area.cy;
			yoffset = (cinfo->cellWMin - NcellWMin) % cinfo->cel.Area.cy;
										// 表示の再利用不可なので全画面更新
			if ( (xoffset <= -cinfo->cel.Area.cx) ||
				 (xoffset >= cinfo->cel.Area.cx) ){
				cinfo->e.cellPoint = -1;
				NewCellN(cinfo, OcellN, TRUE);
				cinfo->cellWMin = NcellWMin;
				cinfo->DrawTargetFlags = DRAWT_ALL;
				InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
				RefleshInfoBox(cinfo, DE_ATTR_ENTRY | DE_ATTR_PAGE);
			}else{	// 再利用可能な表示があるのでスクロール
					// ※ xoffset, yoffset が共に !0 なら全画面更新が好ましい
					//    が、現状ではおきにくいため、対策せず
				RECT box, newbox;
				HDC hDC;
				int OcellWMin;

				if ( cinfo->e.cellPoint >= 0 ){
					int cellPoint = cinfo->e.cellPoint;

					cinfo->e.cellPoint = -1;
					if ( (cellPoint >= NcellWMin) &&
						 (cellPoint < (NcellWMin + PageEntries)) ){
						RefleshCell(cinfo, cellPoint);
						if ( !cinfo->FullDraw ) UpdateWindow_Part(cinfo->info.hWnd);
					}
				}
				NewCellN(cinfo, OcellN, FALSE);

				box.left	= cinfo->BoxEntries.left;
				box.right	= box.left + cinfo->cel.VArea.cx * cinfo->cel.Size.cx;
				box.top		= cinfo->BoxEntries.top;
				box.bottom	= box.top + cinfo->cel.Area.cy * cinfo->cel.Size.cy;

				hDC = GetDC(cinfo->info.hWnd);
				ScrollDC(hDC, xoffset * cinfo->cel.Size.cx,
							 yoffset * cinfo->cel.Size.cy,
							&box, &box, NULL, &newbox);
				ReleaseDC(cinfo->info.hWnd, hDC);

				OcellWMin = cinfo->cellWMin;
				cinfo->cellWMin = NcellWMin;
										// 旧カーソルを消去
				RefleshCell(cinfo, OcellN - OcellWMin + NcellWMin +
										xoffset * cinfo->cel.Area.cy + yoffset);
				if ( !cinfo->FullDraw ) UpdateWindow_Part(cinfo->info.hWnd);
										// 新規部分を表示
				setflag(cinfo->DrawTargetFlags, DRAWT_1ENTRY);
				UnionRect(&box, &newbox, &cinfo->DrawTargetCell);
				cinfo->DrawTargetCell = box;

				InvalidateRect(cinfo->info.hWnd, &newbox, FALSE);
				if ( !cinfo->FullDraw ) UpdateWindow_Part(cinfo->info.hWnd);

				RefleshCell(cinfo, cinfo->e.cellN);
//				if ( !cinfo->FullDraw ) UpdateWindow_Part(cinfo->info.hWnd);
				RefleshInfoBox(cinfo, DE_ATTR_ENTRY | DE_ATTR_PAGE);
			}
			if ( cinfo->combo && (X_combos[0] & CMBS_COMMONINFO) ){
				PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND, KCW_drawinfo, 0);
			}
		}
		#endif // USEDIRECTX
		if ( (cinfo->MouseStat.PushButton > MOUSEBUTTON_CANCEL) &&
			 (cinfo->MousePush == MOUSE_MARK) ){
			CalcDragTarget(cinfo, &DragSelectPos, &DragSelectArea);
			MarkDragArea(cinfo, &DragSelectArea, MARK_REVERSE);
			cinfo->DrawTargetFlags = DRAWT_ALL;
			InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
			UpdateWindow_Part(cinfo->info.hWnd);
			DrawDragFrame(cinfo->info.hWnd, &DragSelectArea);
		}
		DEBUGLOGC("MoveCellCsr SetScrollBar %d", offset);
		SetScrollBar(cinfo, scroll);
		if ( cinfo->EntryIcons.hImage != NULL ){
			setflag(cinfo->SubTCmdFlags, SUBT_GETCELLICON);
			SetEvent(cinfo->SubT_cmd);
		}
		if ( XC_szcm[0] == 2 ){ // ディレクトリサイズキャッシュの読み出し開始
			setflag(cinfo->SubTCmdFlags, SUBT_GETDIRSIZECACHE);
			SetEvent(cinfo->SubT_cmd);
		}
		DEBUGLOGC("MoveCellCsr end %d", offset);
		return TRUE;
	}else{ // 画面内位置変更
		DEBUGLOGC("MoveCellCsr newpos %d", offset);
		if ( !NewCellN(cinfo, OcellN, FALSE) ){ // 古いカーソルを消去/アイコン更新
			DEBUGLOGC("MoveCellCsr nomove end %d", offset);
							// 移動したが更新済み : カーソルは移動しなかった
			return (cinfo->e.cellN != OcellN) ? TRUE : FALSE;
		}
												// 新しいカーソルを表示
		SetScrollBar(cinfo, scroll);
		DEBUGLOGC("MoveCellCsr RefleshCell %d", offset);
		setflag(cinfo->DrawTargetFlags, DRAWT_1ENTRY);
		RefleshCell(cinfo, cinfo->e.cellN);
		if ( !cinfo->FullDraw ) UpdateWindow_Part(cinfo->info.hWnd);
												// 情報窓の更新
		RefleshInfoBox(cinfo, DE_ATTR_ENTRY);
		DEBUGLOGC("MoveCellCsr UpdateWindow %d", offset);
		UpdateWindow_Part(cinfo->info.hWnd);
		DEBUGLOGC("MoveCellCsr end %d", offset);
		return TRUE;
	}
}

/*-----------------------------------------------------------------------------
	特定の cell のマーク処理を行う

	flag = -1:反転 1:付ける 0:はずす -10:INDEX経由せずにはずす
-----------------------------------------------------------------------------*/
void CellMark(PPC_APPINFO *cinfo, ENTRYINDEX cellNo, int markmode)
{
	ENTRYINDEX fwdNo;
	ENTRYDATAOFFSET cellTNo;
	ENTRYCELL *cell;

	cellTNo = CELt(cellNo);
	cell = &CELdata(cellTNo);
	fwdNo = cell->mark_fw;

	if ( markmode == MARK_REVERSE ) markmode = ( fwdNo == NO_MARK_ID );

	if ( (markmode != MARK_REMOVE) && (cell->state >= ECS_NORMAL) &&
		 !(cell->attr & (ECA_THIS | ECA_PARENT | ECA_ETC)) &&
		 !(cell->f.dwFileAttributes & (MARKMASK_DIRFILE ^ cinfo->MarkMask)) ){
																// マークする
		if ( fwdNo == NO_MARK_ID ){
			cinfo->e.markC++;
			if ( cinfo->e.markTop == ENDMARK_ID ){		// 初めてのマーク
				cell->mark_bk = ENDMARK_ID;
				cinfo->e.markTop = cellTNo;
			}else{					// 既にある
				cell->mark_bk = cinfo->e.markLast;
				CELdata(cinfo->e.markLast).mark_fw = cellTNo;
			}
			cinfo->e.markLast = cellTNo;
			cell->mark_fw = ENDMARK_ID;
			AddHLFilesize(cinfo->e.MarkSize, cell->f);
		}
	}else{
		ResetMark(cinfo, cell);
	}
}

void USEFASTCALL ResetMark(PPC_APPINFO *cinfo, ENTRYCELL *cell)
{
	ENTRYDATAOFFSET backNo, nextNo;

	nextNo = cell->mark_fw;
	if ( nextNo == NO_MARK_ID ) return;
	cinfo->e.markC--;
	backNo = cell->mark_bk;
	if ( backNo != ENDMARK_ID ){			// 一つ前を接続
		CELdata(backNo).mark_fw = nextNo;
	}else{
		cinfo->e.markTop = nextNo;	// 末端
	}
	if ( nextNo != ENDMARK_ID ){			// 一つ次を接続
		CELdata(nextNo).mark_bk = backNo;
	}else{			// 先頭
		cinfo->e.markLast = backNo;
	}
	cell->mark_fw = NO_MARK_ID;
	SubHLFilesize(cinfo->e.MarkSize, cell->f);
}

//-----------------------------------------------------------------------------
BOOL MoveCellCursorR(PPC_APPINFO *cinfo, CURSORMOVER *cm)
{
	CURSORMOVER tmpcm;

	tmpcm = *cm;
	tmpcm.offset = -tmpcm.offset;
	return MoveCellCursor(cinfo, &tmpcm);
}

BOOL MoveCellCursor(PPC_APPINFO *cinfo, CURSORMOVER *cm)
{
	if ( cinfo->list.orderZ ){
		switch( cm->unit / CMOVEUNIT_RATEDIV ){
			case CMOVEUNIT_RATEUPDOWN:
				return MoveCellCsr(cinfo, cm->offset * cinfo->cel.Area.cx, cm);
			case CMOVEUNIT_RATEPAGE:
				return MoveCellCsr(cinfo,
					cm->offset * cinfo->cel.Area.cx * cinfo->cel.Area.cy, cm);
			case CMOVEUNIT_RATEDECIPAGE:
				return MoveCellCsr(cinfo,
					cm->offset * cinfo->cel.Area.cx * cinfo->cel.Area.cy / 10, cm);
//			case CMOVEUNIT_RATECOLUMN: , CMOVEUNIT_MARKUPDOWN等
		}
	}else{
		switch( cm->unit / CMOVEUNIT_RATEDIV ){
			case CMOVEUNIT_RATECOLUMN:
				return MoveCellCsr(cinfo, cm->offset * cinfo->cel.Area.cy, cm);
			case CMOVEUNIT_RATEPAGE:
				return MoveCellCsr(cinfo,
					cm->offset * cinfo->cel.Area.cx * cinfo->cel.Area.cy, cm);
			case CMOVEUNIT_RATEDECIPAGE:
				return MoveCellCsr(cinfo,
					cm->offset * cinfo->cel.Area.cx * cinfo->cel.Area.cy / 10, cm);
//			case CMOVEUNIT_RATEUPDOWN: , CMOVEUNIT_MARKUPDOWN等
		}
	}
	return MoveCellCsr(cinfo, cm->offset, cm);
}

void GetCellRealFullName(PPC_APPINFO *cinfo, ENTRYCELL *cell, TCHAR *dest)
{
	TCHAR namebuf[VFPS], *cellfilename;

	cellfilename = (TCHAR *)GetCellFileName(cinfo, cell, namebuf);
	if ( cinfo->e.Dtype.mode != VFSDT_SHN ){
		VFSFullPath(dest, cellfilename, cinfo->path);
		return;
	}

	VFSFixPath(dest, cellfilename, cinfo->path, VFSFIX_FULLPATH | VFSFIX_REALPATH);
	if ( dest[0] != '?' ) return;
	VFSFullPath(dest, cellfilename, cinfo->RealPath);
}
