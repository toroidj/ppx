/*-----------------------------------------------------------------------------
	Paper Plane cUI											File size 関連
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPCOMBO.H"
#pragma hdrstop

typedef struct {
	UINTHL sizeN;	// この窓のクラスタ換算サイズ
	UINTHL sizeP;	// 反対窓のクラスタ換算サイズ
	UINTHL sizeA;	// 加算サイズ
	DWORD file_count;	// ファイル数
	DWORD dir_count;	// ディレクトリ数
	DWORD addN;		// この窓の (byte per cluster - 1)
	DWORD andN;		// この窓の ~(byte per cluster - 1)
	DWORD addP;		// 反対窓の (byte per cluster - 1)
	DWORD andP;		// 反対窓の ~(byte per cluster - 1)
	DWORD links;	// シンボリックリンク等
	DWORD skips;	// アクセスできなかったディレクトリ
	volatile BOOL *BreakFlag;	// cinfo->BreakFlag を示す
} COUNTSTRUCT;

typedef struct {
	PPC_APPINFO *cinfo;
	TCHAR *files; // カウント対象を保存する文字列群
	COUNTSTRUCT cs;
} COUNTPARAM;

const TCHAR CountSizeMainName[] = T("PPc count");

// 特定ディレクトリの容量計算 -------------------------------------------------
void DispMarkSize_ReparsePoint(TCHAR *dir, WIN32_FIND_DATA *ff)
{
	TCHAR buf[VFPS];

	if ( 0 != GetReparsePath(dir, buf) ){
		GetFileSTAT(buf, ff);
	}
}

BOOL DispMarkSizeDir(TCHAR *dir, COUNTSTRUCT *cs)
{
	TCHAR *dirtail;
	HANDLE hFF;
	WIN32_FIND_DATA ff;

	cs->dir_count++;
	dirtail = dir + tstrlen(dir);
	CatPath(NULL, dir, T("*"));
	hFF = VFSFindFirst(dir, &ff);
	*dirtail = '\0';
	if ( hFF == INVALID_HANDLE_VALUE ){
		if ( GetLastError() != ERROR_NO_MORE_FILES ){
			cs->skips++;
			return FALSE;
		}else{
			return TRUE;
		}
	}else{
		do{
			if ( IsRelativeDir(ff.cFileName) ) continue;
			if ( IsTrue(*cs->BreakFlag) ) break;
			if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				if ( ff.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ){
					cs->links++;
				}
				CatPath(NULL, dir, ff.cFileName);
				DispMarkSizeDir(dir, cs);
				*dirtail = '\0';
			}else{
				if ( ff.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ){
					cs->links++;

					CatPath(NULL, dir, ff.cFileName);
					DispMarkSize_ReparsePoint(dir, &ff);
					*dirtail = '\0';
				}
				cs->file_count++;
				AddHLFilesize(cs->sizeA, ff);
				AddMaskedFilesize(cs->sizeN, ff, cs->andN, cs->addN);
				AddMaskedFilesize(cs->sizeP, ff, cs->andP, cs->addP);
			}
		}while( IsTrue(VFSFindNext(hFF, &ff)) );
//		while( IsTrue(FindNextFile(hFF, &ff)));
		VFSFindClose(hFF);
//		FindClose(hFF);
		return TRUE;
	}
}

// マークされたCellの容量計算 -------------------------------------------------
THREADEXRET THREADEXCALL CountSizeMain(COUNTPARAM *cp)
{
	THREADSTRUCT threadstruct = {CountSizeMainName, XTHREAD_EXITENABLE | XTHREAD_TERMENABLE, NULL, 0, 0};
	PPC_APPINFO *cinfo;
	COUNTSTRUCT cs;
	struct dirinfo di;
	TCHAR dir[VFPS], *files, *path;
	ThSTRUCT th;
	UINTHL all_size;
	DWORD all_file_count, all_dir_count = 0;
	COPYDATASTRUCT cds;
	DWORD tick = 0, nowtick;

	(void)CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
	PPxRegisterThread(&threadstruct);

	cinfo = cp->cinfo;
	path = cp->files;					// パラメータを回収
	cs = cp->cs;
	ProcHeapFree(cp);
	all_file_count = cs.file_count;

	th.bottom = (char *)path;
	files = path + tstrlen(path) + 1;
	cinfo->BreakFlag = FALSE;
	all_size = cs.sizeA;
	tstrcpy(di.path, path);

	cds.dwData = KC_StDS;
	cds.cbData = sizeof(di);
	cds.lpData = &di;

// 計算メインループ ---------------------------
	PPxCommonCommand(NULL, JOBSTATE_COUNTSIZE, K_ADDJOBTASK);
	while ( *files != '\0' ){
		cs.sizeA.s.L = cs.sizeA.s.H = cs.file_count = cs.dir_count = 0;
		VFSFullPath(dir, files, path);
		if ( IsTrue(DispMarkSizeDir(dir, &cs)) ){
			if ( IsTrue(cinfo->BreakFlag) ) break;
			AddHLHL(all_size, cs.sizeA);
			di.size = cs.sizeA;
			all_file_count += di.file_count = cs.file_count;
			all_dir_count += di.dir_count = cs.dir_count;
			tstrcpy(di.path, path);
			tstrcpy(di.name, files);
			SendMessage(cinfo->info.hWnd, WM_COPYDATA,
					0, (LPARAM)(PCOPYDATASTRUCT)&cds);
		}
		files += tstrlen(files) + 1;
		nowtick = GetTickCount();
		if ( (nowtick - tick) > 1000 ){
			tick = nowtick;
			InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
		}
	}
	PPxCommonCommand(NULL, 0, K_DELETEJOBTASK);

	if ( IsTrue(cinfo->BreakFlag) ){
		SetPopMsg(cinfo, POPMSG_MSG, MES_BRAK);
	}else{
		TCHAR *ptr;

		ptr = thprintf(dir, TSIZEOF(dir), cs.andP ?
				T("%7MLd (%'Lu) bytes/ %'u files/ %'u directories[%'Lu/%'Lu] "):
				T("%7MLd (%'Lu) bytes/ %'u files/ %'u directories[%'Lu] "),
				all_size.rawdata, all_size.rawdata,
				all_file_count, all_dir_count,
				cs.sizeN.rawdata, cs.sizeP.rawdata);

		if ( cs.skips != 0 ){
			ptr = thprintf(ptr, 32, T("/ %d skip dir. "), cs.skips);
		}

		if ( cs.links != 0 ){
			thprintf(ptr, 32, T("/ %d s.links "), cs.links);
		}
		SetPopMsg(cinfo, POPMSG_MSG, dir);
	}
	ThFree(&th);
	InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
	RefleshInfoBox(cinfo, DE_ATTR_ENTRY);
	cinfo->sizereentry = 0;

	PPcAppInfo_Release(cinfo);
	PPxUnRegisterThread();
	CoUninitialize();
	t_endthreadex(TRUE);
}

void CountMarkSize(PPC_APPINFO *cinfo, int emode)
{
	COUNTPARAM cp, *pcp;
	ThSTRUCT th;
	HANDLE hThread;
	DWORD tmp;
	DWORD spc, bps, fc, tc;
	TCHAR dir[VFPS], buf[VFPS];
	ENTRYCELL *cell;
	EnumCellStruct enums;

	if ( cinfo->sizereentry ){
		SetPopMsg(cinfo, POPMSG_MSG, MES_EACO);
		return;
	}
	cinfo->sizereentry = 1;

	if ( (Combo.Report.hWnd != NULL) && (cinfo->e.markC == 0) ){
		VFSFullPath(buf, CEL(cinfo->e.cellN).f.cFileName, cinfo->path);
		SetPopMsg(cinfo, POPMSG_MSG, buf);
	}else{
		SetPopMsg(cinfo, POPMSG_MSG, MES_SACO);
	}
	UpdateWindow_Part(cinfo->info.hWnd);
										// カウント準備 ===============
	memset(&cp.cs, 0, sizeof(cp.cs));
	cp.cs.BreakFlag = &cinfo->BreakFlag;
	ThInit(&th);
	ThAddString(&th, cinfo->path);
								// クラスタ換算用定数を作成 -----------
	GetPairPath(cinfo, buf);	// 反対窓
	if ( *buf != '\0' ){
		GetDriveName(dir, buf);
		if ( GetDiskFreeSpace(dir, &spc, &bps, &fc, &tc) ){
			cp.cs.addP = (spc * bps) - 1;
			cp.cs.andP = ~cp.cs.addP;
		}
	}
	GetDriveName(dir, cinfo->path);		// 現在窓
	if (GetDiskFreeSpace(dir, &spc, &bps, &fc, &tc)){
		cp.cs.addN = (spc * bps) - 1;
		cp.cs.andN = ~cp.cs.addN;
	}
								// マークファイルをカウント ---------
	InitEnumCell(&enums, cinfo, emode);
	while ( (cell = enums.next(&enums, cinfo)) != NULL ){
		if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
			if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ){
				cp.cs.links++;
			}
			ThAddString(&th, cell->f.cFileName);
		}else{
			cp.cs.file_count++;
			if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ){
				cp.cs.links++;

				VFSFullPath(dir, cell->f.cFileName, cinfo->path);
				if ( 0 != GetReparsePath(dir, buf) ){
					FILE_STAT_DATA ff;

					if ( IsTrue(GetFileSTAT(buf, &ff)) ){
						cell->f.nFileSizeLow = ff.nFileSizeLow;
						cell->f.nFileSizeHigh = ff.nFileSizeHigh;
					}
				}
			}
			AddHLFilesize(cp.cs.sizeA, cell->f);
			AddMaskedFilesize(cp.cs.sizeN, cell->f, cp.cs.andN, cp.cs.addN);
			AddMaskedFilesize(cp.cs.sizeP, cell->f, cp.cs.andP, cp.cs.addP);
		}
	}
	cp.files = (TCHAR *)th.bottom;
	cp.cinfo = cinfo;
	pcp = PPcHeapAlloc(sizeof(COUNTPARAM));
	*pcp = cp;

	PPcAppInfo_AddRef(cinfo);
	hThread = t_beginthreadex(NULL, 0, (THREADEXFUNC)CountSizeMain, pcp, 0, &tmp);
	if ( hThread != NULL ){
		SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
		CloseHandle(hThread);
	}else{
		PPcAppInfo_Release(cinfo);
	}
}

BOOL DispMarkSize(PPC_APPINFO *cinfo)
{
	EnumCellStruct enums;

	if ( cinfo->e.Dtype.mode == VFSDT_HTTP ) return FALSE;
	InitEnumCell(&enums, cinfo, ENUMCELL_MARKED);
	for (;;){
		ENTRYCELL *cell;

		cell = enums.next(&enums, cinfo);
		if ( cell == NULL ) return FALSE;
		if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
			CountMarkSize(cinfo, ENUMCELL_MARKED);
			return TRUE;
		}
	}
}

void ClearMarkSize(PPC_APPINFO *cinfo, int emode)
{
	TCHAR dirpath[VFPS];
	EnumCellStruct enums;
	BYTE *hist;
	WORD datasize;

	InitEnumCell(&enums, cinfo, emode);
	for (;;){
		ENTRYCELL *cell;

		cell = enums.next(&enums, cinfo);
		if ( cell == NULL ) break;
		if ( !(cell->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) continue;

		if ( XC_szcm[0] == 1 ){
			if ( cell->comment != EC_NOCOMMENT ){
				// ※※※ガーページコレクションなどはしない
				cell->comment = EC_NOCOMMENT;
				cinfo->ModifyComment = TRUE;
			}
		}

		if ( cell->attr & ECA_DIRC ){
			VFSFullPath(dirpath, CellFileName(cell), cinfo->path);
			UsePPx();
			hist = (BYTE *)SearchHistory(PPXH_PPCPATH, dirpath);
			if ( hist != NULL ){
				datasize = GetHistoryDataSize(hist);
				if ( datasize >= ( sizeof(DWORD) * 4 ) ){
					DWORD *WritePtr = (DWORD *)(BYTE *)GetHistoryData(hist);

					WritePtr[2] = 0;
					WritePtr[3] = 0xffffffff;
				}
			}
			FreePPx();

			if ( IsCellPtrMarked(cell) ){
				SubHLFilesize(cinfo->e.MarkSize, cell->f);
			}
			cell->f.nFileSizeLow = cell->f.nFileSizeHigh = 0;
			resetflag(cell->attr, ECA_DIRC);
		}
	}
	Repaint(cinfo);
}
