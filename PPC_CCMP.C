/*-----------------------------------------------------------------------------
	Paper Plane cUI		比較マーク
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "FATTIME.H"
#include "sha.h"
#pragma hdrstop

INT_PTR CALLBACK CompareDetailDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
#define CompareDetailDialog(hWnd, param) PPxDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CMARK), hWnd, CompareDetailDlgBox, (LPARAM)param)

typedef struct {
	const TCHAR *name;
	DWORD mode;
} COMPAREMENUSTRUCT;

#define ATTR_FILE	(MARKMASK_FILE << CMPMARKSHIFT)
#define ATTR_DIR	((MARKMASK_DIRFILE << CMPMARKSHIFT) | CMP_SUBDIR)
COMPAREMENUSTRUCT CompareMenu[] = {
	{MES_CMPN, ATTR_FILE | CMP_NEW},
	{MES_CMPW, ATTR_FILE | CMP_NEWONLY},
	{MES_CMPT, ATTR_FILE | CMP_TIME},
	{MES_CMPE, ATTR_FILE | CMP_EXIST},
	{MES_CMPX, ATTR_DIR  | CMP_EXIST},
	{MES_CMPS, ATTR_FILE | CMP_SIZE},
	{MES_CMPL, ATTR_FILE | CMP_LARGE},
	{MES_CMPA, ATTR_FILE | CMP_SAMESIZETIME},
	{MES_CMPB, ATTR_DIR  | CMP_BINARY},
	{MES_CMPG, ATTR_DIR  | CMP_BINARY | CMP_UNMATCHLOG},
	{MES_CMP1, ATTR_DIR  | CMP_SHA1},
	{MES_CMP7, ATTR_DIR  | CMP_SHA1 | CMP_UNMATCHLOG},
	{MES_CMPC, ATTR_FILE | CMP_COMMENT},
	{MES_CMPF, ATTR_FILE | CMP_EXIST | CMP_WITHOUT_EXT},
	{MES_CMPQ, ATTR_FILE | CMP_SIZEONLY},
//	{MES_CMP6, ATTR_FILE | CMP_SHA1ONLY},
	{MES_CMPY, ATTR_FILE | CMP_COMMENTONLY},
	{NULL, 0}
};

#define DETAILMENU_ITEMS 15
const TCHAR *CompareDetailMenu[DETAILMENU_ITEMS] = {
	// CMP_2
	MES_CMPN, MES_CMPT, MES_CMPE, MES_CMPS, MES_CMPL, // 1～
	MES_CMPA, MES_CMPB, MES_CMPI, MES_CMPW, MES_CMP1, // 5～
	MES_CMPC,
	// CMP_WITHOUT_EXT
	MES_CMPF, MES_CMPY,
	// CMP_1
	MES_CMPZ, MES_CMPM,
}; // 11～

const int CompareDetailIndex[DETAILMENU_ITEMS] = {
	// CMP_2
	CMP_NEW, CMP_TIME, CMP_EXIST, CMP_SIZE, CMP_LARGE,
	CMP_SAMESIZETIME, CMP_BINARY, CMP_SIZEONLY, CMP_NEWONLY, CMP_SHA1,
	CMP_COMMENT,
	// CMP_WITHOUT_EXT
	CMP_EXIST | CMP_WITHOUT_EXT, CMP_COMMENTONLY,
	// CMP_1
	CMP_1SIZE, CMP_1COMMENT
};

#define CPI_INTERVAL 1000
typedef struct {
	PPC_APPINFO *cinfo;
	TCHAR *path;
	ENTRYINDEX in;
	DWORD tick;
	int settings;
	WriteTextStruct sts;
} COMPPROGINFO;

struct COMPAREBINARYSTRUCT {
	COMPPROGINFO *cpi;
	char *srcbuf, *destbuf;
	SIZE32_T bufsize;
	int mode, subdir;
};

const TCHAR StrCCMP[] = MES_CCMP;

void Result_Match(PPC_APPINFO *cinfo, int count)
{
	TCHAR text[256];

	if ( count < 0 ){
		tstrcpy(text, MessageText(MES_CCMC));
	}else if ( count == 0 ){
		tstrcpy(text, MessageText(StrEENF));
	}else{
		thprintf(text, TSIZEOF(text), T("%Ms: %d"), StrCCMP, count);
	}
	SetPopMsg(cinfo, POPMSG_MSG, text);
}

void Result_Compare(PPC_APPINFO *cinfo, int count)
{

	StopPopMsg(cinfo, PMF_PROGRESS);
	Result_Match(cinfo, count);
	ActionInfo(cinfo->info.hWnd, &cinfo->info, AJI_COMPLETE, T("compare")); // 通知
}

//---------------------------------------------------------- 一覧情報を送信する
BOOL PPcCompareSend(PPC_APPINFO *cinfo, COMPAREMARKPACKET *cmp, HWND PairHWnd)
{
	BOOL result;
	DWORD qsize, wsize;

	if ( ((cmp->mode & CMPTYPEMASK)== CMP_COMMENT) || ((cmp->mode & CMPTYPEMASK)== CMP_COMMENTONLY) ){
		result = WriteListFileForRaw(cinfo, cmp->filename);
	}else{
		HANDLE hFile;

		hFile = CreateFileL(cmp->filename, GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if ( hFile == INVALID_HANDLE_VALUE ) return FALSE;

		qsize = sizeof(ENTRYCELL) * cinfo->e.cellDataMax;
		result = WriteFile(hFile, cinfo->e.CELLDATA.p, qsize, &wsize, NULL);

		if ( IsTrue(result) && (qsize != wsize) ){
			SetLastError(ERROR_DISK_FULL);
			result = FALSE;
		}

		CloseHandle(hFile);
	}

	if ( IsTrue(result) ){
		COPYDATASTRUCT copydata;

		cmp->DirType = cinfo->e.Dtype.mode;
		cmp->LoadCounter = cinfo->LoadCounter;
		copydata.dwData = 'O';
		copydata.cbData = sizeof(COMPAREMARKPACKET);
		copydata.lpData = cmp;
		SendMessage(PairHWnd, WM_COPYDATA,
				(WPARAM)cinfo->info.hWnd, (LPARAM)&copydata);
	}
	return result;
}

void CompareReport_2size(const TCHAR *message, WIN32_FIND_DATA *src, WIN32_FIND_DATA *pair)
{
	UINTHL src_size, pair_size;

	LetHLFilesize(src_size, *src);
	LetHLFilesize(pair_size, *pair);
	SetReportTextf(message,
			src->cFileName, src_size.rawdata,
			pair->cFileName, pair_size.rawdata);
}

BOOL USEFASTCALL CompareProgress(COMPPROGINFO *cpi)
{
	TCHAR buf[32 + VFPS];
	BOOL result;

	thprintf(buf, TSIZEOF(buf), T("%d/%d %s"), cpi->in, cpi->cinfo->e.cellIMax, cpi->path);
	SetPopMsg(cpi->cinfo, POPMSG_PROGRESSBUSYMSG, buf);
	UpdateWindow_Part(cpi->cinfo->info.hWnd);
	if ( IsTrue( result = ReadDirBreakCheck(cpi->cinfo) ) ){
		SetPopMsg(cpi->cinfo, POPMSG_MSG, MES_BRAK);
		cpi->in = cpi->cinfo->e.cellIMax;
	}
	if ( GetAsyncKeyState(VK_PAUSE) & KEYSTATE_FULLPUSH ){
		if ( PMessageBox(cpi->cinfo->info.hWnd, MES_AbortCheck, MES_TFCP, MB_QYES) == IDOK ){
			SetPopMsg(cpi->cinfo, POPMSG_MSG, MES_BRAK);
			cpi->in = cpi->cinfo->e.cellIMax;
			result = TRUE;
		}
	}
	return result;
}

void WriteCompareList(COMPPROGINFO *cpi, ENTRYCELL *cell, BOOL mark)
{
	ENTRYDATAOFFSET old_mark;
	old_mark = cell->mark_fw;
	cell->mark_fw = mark ? 0 : NO_MARK_ID;
	WriteLFcell(cpi->cinfo, &cpi->sts, cell);
	cell->mark_fw = old_mark;
}

ERRORCODE PPcCompareComment(PPC_APPINFO *cinfo, HWND hPairWnd, COMPAREMARKPACKET *cmp, COMPPROGINFO *cpi, DWORD unmarkmask)
{
	HANDLE hFF;
	ENTRYCELL cell;
	TCHAR path[VFPS], comment[CMDLINESIZE];
	int cmp_match = 0, cmp_total = 0, mode;

	cell.comment = EC_NOCOMMENT;
	if ( cmp->listfile[0] != '\0' ){
		CreateHandleForListFile(cinfo, &cpi->sts, cmp->listfile,
				(cpi->settings & CMPJSON) ?
					 WLFC_DETAIL | WLFC_COMMENT | WLFC_WITHMARK | WLFC_JSON :
					 WLFC_DETAIL | WLFC_COMMENT | WLFC_WITHMARK);
		if ( cpi->sts.hFile != NULL ) setflag(cpi->settings, CMPLISTFILE);
	}

	mode = cpi->settings & (CMPTYPEMASK | CMPDOWN);
	PPxCommonCommand(cinfo->info.hWnd, JOBSTATE_COMPARE, K_ADDJOBTASK);

	VFSFullPath(path, T("*"), cmp->filename);
	hFF = VFSFindFirst(path, &cell.f);
	if ( hFF != INVALID_HANDLE_VALUE ){
		cpi->cinfo = cinfo;
		cpi->tick = GetTickCount();

		SetPopMsg(cinfo, POPMSG_PROGRESSBUSYMSG, MES_SCMP);
		UpdateWindow_Part(cinfo->info.hWnd);
		for(;;){
			if ( cell.f.dwFileAttributes & FILE_ATTRIBUTEX_EXTRA ){
				if ( VFSFindOptionData(hFF, FINDOPTIONDATA_COMMENT, comment) ){
					TCHAR *nowname, *pairname;
					int extoffset;

					pairname = FindLastEntryPoint(cell.f.cFileName);
					for ( cpi->in = 0 ; cpi->in < cinfo->e.cellIMax ; cpi->in++ ){
						ENTRYCELL *cellN;
						DWORD nowtick;

						cellN = &CEL(cpi->in);
						if ( cellN->state < ECS_NORMAL ) continue; // SysMsgやDeletedはだめ
						if ( cellN->comment == EC_NOCOMMENT ) continue;
						if ( cellN->f.dwFileAttributes & unmarkmask ) continue;
						nowname = FindLastEntryPoint(cellN->f.cFileName);
						extoffset = cellN->ext - (nowname - cellN->f.cFileName);
						nowtick = GetTickCount();
						if ( (nowtick - cpi->tick) > CPI_INTERVAL ){
							cpi->tick = nowtick;
							cpi->path = cellN->f.cFileName;
							if ( IsTrue(CompareProgress(cpi)) ) break;
						}
						if ( mode == CMP_COMMENT ){
							if ( tstrnicmp(nowname, pairname, extoffset) ) continue; // 名前不一致
							// 名前末尾不一致
							if ( (pairname[extoffset] != '\0') && // 末尾でない
								  ((pairname[extoffset] != '.') || // 拡張子でない
								   (tstrchr(pairname + extoffset + 1, '.') != NULL)) ){
								continue;
							}
						}
						if ( tstrcmp(comment, ThPointerT(&cinfo->e.Comments, cellN->comment)) ){ // コメント不一致
							if ( (mode == CMP_COMMENT) &&
								 (cpi->settings & CMPMASK_UNMATCH) &&
								 !(cpi->settings & CMPDOWN) ){
								if ( cpi->settings & CMP_UNMATCHLOG ){
									SetReportTextf(T("unmatch comment: %s; %s:[%s] != %s:[%s]\r\n"),
										cellN->f.cFileName,
										ThPointerT(&cinfo->e.Comments, cellN->comment),
										cmp->path,
										comment,
										cinfo->RealPath);
								}
								if ( cpi->settings & CMPMASK_LISTFILE ){
									WriteCompareList(cpi, &cell, FALSE);
								}
							}
							continue;
						}
						if ( (cpi->settings & CMPMASK_MATCH) &&
							 !(cpi->settings & CMPDOWN) ){
							if ( cpi->settings & CMP_MATCHLOG ){
								if ( mode == CMP_COMMENT ){
									SetReportTextf(T("match comment: %s %s; %s == %s\r\n"),
											cellN->f.cFileName, comment,
											cmp->path, cinfo->RealPath);
								}else if ( mode == CMP_COMMENTONLY ){
									SetReportTextf(T("match comment: %s %s == %s %s; %s\r\n"),
											cmp->path,
											cellN->f.cFileName,
											cinfo->RealPath,
											cellN->f.cFileName,
											comment);
								}
							}
							if ( cpi->settings & CMPMASK_LISTFILE ){
								WriteCompareList(cpi, &cell, TRUE);
							}
						}
						CellMark(cinfo, cpi->in, MARK_CHECK);
						cmp_match++;
						if ( mode == CMP_COMMENT ) break;
					}
				}
			}
			if ( VFSFindNext(hFF, &cell.f) == FALSE ) break;
		}
		VFSFindClose(hFF);

		StopPopMsg(cinfo, PMF_STOP);
	}
	if ( !(mode & CMPDOWN) ){
		PostMessage( hPairWnd, WM_PPXCOMMAND, KC_ENDCOMPARE,
			((cmp_total != 0) && (cmp_match == cmp_total)) ? (LPARAM)-1 : (LPARAM)cmp_match);
	}
	Repaint(cinfo);
	if ( cpi->sts.hFile != NULL ) CloseHandle(cpi->sts.hFile);
	PPxCommonCommand(cinfo->info.hWnd, 0, K_DELETEJOBTASK);
	return NO_ERROR;
}

int CompareFilesize(WIN32_FIND_DATA *size1, WIN32_FIND_DATA *size2)
{
	return CmpDD(size1->nFileSizeLow, size1->nFileSizeHigh,
			size2->nFileSizeLow, size2->nFileSizeHigh);
}

/*----------------------------------------------------------
	NO_ERROR			一致した
	ERROR_INVALID_DATA	一致しなかった
	その他				比較時にエラーが発生した（この時点で終了）
*/

ERRORCODE PPcCompareDirMain(COMPPROGINFO *cpi, const TCHAR *srcpath, WIN32_FIND_DATA *cellN, const TCHAR *destpath)
{
	TCHAR srcname[VFPS], destname[VFPS];
	ERRORCODE result;
	HANDLE hFFsrc, hFFdest;
								// ディレクトリ処理 ---------------------------
	WIN32_FIND_DATA ffsrc;
	ENTRYCELL celldest;
	TCHAR tempname[VFPS];
	DWORD filecount = 0;
	int mode;

	VFSFullPath(srcname, cellN->cFileName, srcpath);
	VFSFullPath(destname, cellN->cFileName, destpath);
	CatPath(tempname, destname, T("*"));
								// 反対側のファイル数を算出
	hFFdest = FindFirstFileL(tempname, &celldest.f);
	if ( hFFdest == INVALID_HANDLE_VALUE ){
		if ( cpi->settings & (CMP_MATCHLOG | CMP_UNMATCHLOG) ){
			SetReportTextf(T("open error: %s; %Mm"), destname, GetLastError());
		}
		return ERROR_INVALID_DATA;
	}
	for ( ; ; ){
		if ( !IsRelativeDir(celldest.f.cFileName) ){
			filecount++;

			if ( (filecount & 0x3f) == 0 ){
				DWORD nowtick;
				nowtick = GetTickCount();

				if ( (nowtick - cpi->tick) > CPI_INTERVAL ){
					cpi->tick = nowtick;
					cpi->path = destname;
					if ( IsTrue(CompareProgress(cpi)) ){
						FindClose(hFFdest);
						return ERROR_CANCELLED;
					}
				}
			}
		}
		if ( FALSE == FindNextFile(hFFdest, &celldest.f) ) break;
	}
	FindClose(hFFdest);
								// こちら側のファイル情報を順次求め、比較
	CatPath(tempname, srcname, T("*"));
	hFFsrc = FindFirstFileL(tempname, &ffsrc);
	if ( hFFsrc == INVALID_HANDLE_VALUE ){
		if ( cpi->settings & (CMP_MATCHLOG | CMP_UNMATCHLOG) ){
			SetReportTextf(T("open error: %s; %Mm"), srcname, GetLastError());
		}
		return ERROR_INVALID_DATA;
	}
	mode = cpi->settings & (CMPTYPEMASK | CMPDOWN);

	result = NO_ERROR;
	for ( ; ; ){
		if ( !IsRelativeDir(ffsrc.cFileName) ){
			filecount--;
			CatPath(tempname, destname, ffsrc.cFileName);
			hFFdest = FindFirstFileL(tempname, &celldest.f);
			if ( hFFdest == INVALID_HANDLE_VALUE ){
				result = ERROR_INVALID_DATA;
				if ( (cpi->settings & CMPMASK_UNMATCH) &&
					 !(cpi->settings & CMPDOWN) ){
					if ( cpi->settings & CMP_UNMATCHLOG ){
						SetReportTextf(T("unmatch exist: %s != %s"), tempname, srcname);
					}
					if ( cpi->settings & CMPMASK_LISTFILE ){
						celldest.comment = EC_NOCOMMENT;
						WriteCompareList(cpi, &celldest, FALSE);
					}
					goto next;
				}else{
					break;
				}
			}else{
				FindClose(hFFdest);
			}

			if ( celldest.f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				if ( !(ffsrc.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ){
					result = ERROR_INVALID_DATA;
					if ( cpi->settings & CMP_UNMATCHLOG ){
						SetReportTextf(T("unmatch file != directory: %s != %s"), tempname, srcname);
						goto next;
					}else{
						break;
					}
				}else{
					DWORD subresult;
					subresult = PPcCompareDirMain(cpi, srcname, &ffsrc, destname);
					if ( subresult != NO_ERROR ){
						result = subresult;
						if ( !(cpi->settings & CMPMASK_ALLSEARCH) ){
							break;
						}else{
							goto next;
						}
					}
				}
			}else{
				if ( mode == CMP_EXIST ){
					if ( cpi->settings & CMP_MATCHLOG ){
						SetReportTextf(T("match exist: %s == %s"), tempname, srcname);
					}
					goto match;
				}

				if ( (mode == CMP_NEW) || (mode == CMP_TIME) ){
					if ( FuzzyCompareFileTime(&ffsrc.ftLastWriteTime,
						&celldest.f.ftLastWriteTime) < 0 ){
						if ( cpi->settings & CMP_MATCHLOG ){
							SetReportTextf(T("match new time: %s; %MF > %MF"),
									ffsrc.cFileName,
									&celldest.f.ftLastWriteTime,
									&ffsrc.ftLastWriteTime);
						}
						goto match;
					}
					result = ERROR_INVALID_DATA;
					if ( cpi->settings & CMP_UNMATCHLOG ){
						SetReportTextf(T("unmatch new time: %s; %MF <= %MF"),
								ffsrc.cFileName,
								&celldest.f.ftLastWriteTime,
								&ffsrc.ftLastWriteTime);
					}
					goto next;
				}

				if ( mode == CMP_SIZE ){
					if ( (ffsrc.nFileSizeHigh != celldest.f.nFileSizeHigh) ||
						 (ffsrc.nFileSizeLow != celldest.f.nFileSizeLow) ){
						if ( (cpi->settings & CMP_MATCHLOG) && !(mode & CMPDOWN) ){
							CompareReport_2size(
									T("match different size: %s(%'Ld) != %s(%'Ld)"),
									&ffsrc, &celldest.f);
						}
						goto match;
					}
					result = ERROR_INVALID_DATA;
					if ( (cpi->settings & CMP_UNMATCHLOG) && !(mode & CMPDOWN) ){
						CompareReport_2size(
								T("unmatch different size: %s(%'Ld) == %s(%'Ld)"),
								&ffsrc, &celldest.f);
					}
					goto next;
				}

				if ( mode == CMP_LARGE ){
					if ( CompareFilesize(&ffsrc, &celldest.f) < 0 ){
						if ( cpi->settings & CMP_MATCHLOG ){
							CompareReport_2size(
									T("match large size: %s(%'Ld) > %s(%'Ld)"),
									&ffsrc, &celldest.f);
						}
						goto match;
					}
					result = ERROR_INVALID_DATA;
					if ( cpi->settings & CMP_UNMATCHLOG ){
						CompareReport_2size(
								T("unmatch large size: %s(%'Ld) <= %s(%'Ld)"),
								&ffsrc, &celldest.f);
					}
					goto next;
				}
			}
	match: ;
			if ( cpi->settings & CMPLISTFILE ){
				ENTRYCELL cell;

				if ( GetFileSTAT(tempname, &cell.f) ){
					WriteCompareList(cpi, &cell, TRUE);
				}
			}
	next: ;
			if ( (filecount & 0x3f) == 0 ){
				DWORD nowtick;
				nowtick = GetTickCount();

				if ( (nowtick - cpi->tick) > CPI_INTERVAL ){
					cpi->tick = nowtick;
					cpi->path = srcname;
					if ( IsTrue(CompareProgress(cpi)) ){
						result = ERROR_CANCELLED;
						break;
					}
				}
			}
		}
		if ( FALSE == FindNextFile(hFFsrc, &ffsrc) ){
			if ( filecount ){
				result = ERROR_INVALID_DATA;
				if ( cpi->settings & CMP_UNMATCHLOG ){
					SetReportTextf(T("unmatch %d entries: %s != %s"), filecount, tempname, srcname);
				}
			}
			break;
		}
	}
	FindClose(hFFsrc);
	return result;
}

ERRORCODE PPcCompareSizeTimeDirMain(COMPPROGINFO *cpi, const TCHAR *srcpath, WIN32_FIND_DATA *cellN, const TCHAR *destpath)
{
	TCHAR srcname[VFPS], destname[VFPS];
	ERRORCODE result;
	HANDLE hFFsrc, hFFdest;
								// ディレクトリ処理 ---------------------------
	WIN32_FIND_DATA ffsrc;
	WIN32_FIND_DATA ffdest;
	TCHAR tempname[VFPS];
	DWORD filecount = 0;

	VFSFullPath(srcname, cellN->cFileName, srcpath);
	VFSFullPath(destname, cellN->cFileName, destpath);
	CatPath(tempname, destname, T("*"));
								// 反対側のファイル数を算出
	hFFdest = FindFirstFileL(tempname, &ffdest);
	if ( hFFdest == INVALID_HANDLE_VALUE ){
		if ( cpi->settings & (CMP_MATCHLOG | CMP_UNMATCHLOG) ){
			SetReportTextf(T("open error: %s; %Mm"), destname, GetLastError());
		}
		return ERROR_INVALID_DATA;
	}

	for ( ; ; ){
		if ( !IsRelativeDir(ffdest.cFileName) ){
			filecount++;

			if ( (filecount & 0x3f) == 0 ){
				DWORD nowtick;
				nowtick = GetTickCount();

				if ( (nowtick - cpi->tick) > CPI_INTERVAL ){
					cpi->tick = nowtick;
					cpi->path = destname;
					if ( IsTrue(CompareProgress(cpi)) ){
						FindClose(hFFdest);
						return ERROR_CANCELLED;
					}
				}
			}
		}
		if ( FALSE == FindNextFile(hFFdest, &ffdest) ) break;
	}
	FindClose(hFFdest);
								// こちら側のファイル情報を順次求め、比較
	CatPath(tempname, srcname, T("*"));
	hFFsrc = FindFirstFileL(tempname, &ffsrc);
	if ( hFFsrc == INVALID_HANDLE_VALUE ){
		if ( cpi->settings & (CMP_MATCHLOG | CMP_UNMATCHLOG) ){
			SetReportTextf(T("open error: %s; %Mm"), srcname, GetLastError());
		}
		return ERROR_INVALID_DATA;
	}
	result = NO_ERROR;
	for ( ; ; ){
		if ( !IsRelativeDir(ffsrc.cFileName) ){
			FILE_STAT_DATA ffd;
			filecount--;
			CatPath(tempname, destname, ffsrc.cFileName);

			if ( GetFileSTAT(tempname, &ffd) == FALSE ){
				if ( (cpi->settings & CMP_UNMATCHLOG) && !(cpi->settings & CMPDOWN) ){
					SetReportTextf(T("attribute error: %s; %Mm"), destname, GetLastError());
					goto next;
				}
				result = ERROR_INVALID_DATA;
				break;
			}

			if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				if ( !(ffsrc.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ){
					if ( (cpi->settings & CMP_UNMATCHLOG) && !(cpi->settings & CMPDOWN) ){
						SetReportTextf(T("file != directory error: %s; %Mm"), destname, GetLastError());
						goto next;
					}
					result = ERROR_INVALID_DATA;
					break;
				}
				result = PPcCompareSizeTimeDirMain(cpi, srcname, &ffsrc, destname);
				if ( result != NO_ERROR ){
					if ( (cpi->settings & CMPMASK_MATCH) && !(cpi->settings & CMPDOWN) ){
						goto next;
					}else{
						break;
					}
				}
			}

			if ( FuzzyCompareFileTime(&ffsrc.ftLastWriteTime,
					&ffd.ftLastWriteTime) ){
				if ( (cpi->settings & CMP_UNMATCHLOG) && !(cpi->settings & CMPDOWN) ){
					SetReportTextf(T("unmatch size & time: %s; [%MF] %s != [%MF] %s"),
							ffsrc.cFileName,
							&ffd.ftLastWriteTime, destpath,
							&ffsrc.ftLastWriteTime, srcpath);
				}
				result = ERROR_INVALID_DATA;
			}else if (
				(ffsrc.nFileSizeHigh != ffd.nFileSizeHigh) ||
				(ffsrc.nFileSizeLow != ffd.nFileSizeLow ) ){
				if ( (cpi->settings & CMPMASK_UNMATCH) && !(cpi->settings & CMPDOWN) ){
					if ( cpi->settings & CMP_UNMATCHLOG ){
						UINTHL ssize, psize;

						LetHLFilesize(ssize, ffsrc);
						LetHLFilesize(psize, ffd);
						SetReportTextf(T("unmatch size & time: %s; [%'Ld] %s != [%'Ld] %s"),
								ffsrc.cFileName,
								psize.rawdata, destpath,
								ssize.rawdata, srcpath);
					}
					if ( cpi->settings & CMPLISTFILE ){
						ENTRYCELL cell;

						if ( GetFileSTAT(destname, &cell.f) ){
							WriteCompareList(cpi, &cell, FALSE);
						}
					}
				}
				result = ERROR_INVALID_DATA;
			}else{
				if ( (cpi->settings & CMPMASK_MATCH) && !(cpi->settings & CMPDOWN) ){
					if ( cpi->settings & CMP_MATCHLOG ){
						UINTHL ssize;

						LetHLFilesize(ssize, ffsrc);
						SetReportTextf(T("match size & time: %s; %'Ld & %MF"),
								ffsrc.cFileName,
								ssize.rawdata, &ffd.ftLastWriteTime);
					}
					if ( cpi->settings & CMPLISTFILE ){
						ENTRYCELL cell;

						if ( GetFileSTAT(destname, &cell.f) ){
							WriteCompareList(cpi, &cell, TRUE);
						}
					}
				}
			}

			if ( (filecount & 0x3f) == 0 ){
				DWORD nowtick;
				nowtick = GetTickCount();

				if ( (nowtick - cpi->tick) > CPI_INTERVAL ){
					cpi->tick = nowtick;
					cpi->path = srcname;
					if ( IsTrue(CompareProgress(cpi)) ){
						result = ERROR_CANCELLED;
						break;
					}
				}
			}
		}
	next: ;
		if ( FALSE == FindNextFile(hFFsrc, &ffsrc) ){
			if ( filecount ){
				result = ERROR_INVALID_DATA;
				if ( cpi->settings & CMP_UNMATCHLOG ){
					SetReportTextf(T("unmatch %d entries: %s != %s"), filecount, destpath, srcpath);
				}
			}
			break;
		}
	}
	FindClose(hFFsrc);
	return result;
}

BOOL IsBreakCompare(COMPPROGINFO *cpi)
{
	DWORD nowtick;

	nowtick = GetTickCount();
	if ( (nowtick - cpi->tick) > CPI_INTERVAL ){
		cpi->tick = nowtick;
		if ( IsTrue(CompareProgress(cpi)) ) return TRUE;
	}
	return FALSE;
}

ERRORCODE PPcCompareBinary(struct COMPAREBINARYSTRUCT *cbs, const TCHAR *srcpath, WIN32_FIND_DATA *cellN, const TCHAR *destpath, ENTRYCELL *cellp)
{
	TCHAR srcname[VFPS], destname[VFPS];
	HANDLE hSrc, hDest;
	ERRORCODE result;
	DWORD srcsize, destsize;
								// ディレクトリ処理 ---------------------------
	if ( cellN->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
		if ( (cbs->subdir) && (cellp->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ){
			HANDLE hFFsrc, hFFdest;
			WIN32_FIND_DATA ffsrc;
			ENTRYCELL celldest;
			TCHAR tempname[VFPS];
			DWORD filecount = 0;

			VFSFullPath(srcname, cellN->cFileName, srcpath);
			VFSFullPath(destname, cellN->cFileName, destpath);
			CatPath(tempname, destname, T("*"));
			celldest.comment = EC_NOCOMMENT;
								// 反対側のファイル数を算出
			hFFdest = FindFirstFileL(tempname, &celldest.f);
			if ( hFFdest == INVALID_HANDLE_VALUE ){
				if ( cbs->cpi->settings & (CMP_MATCHLOG | CMP_UNMATCHLOG) ){
					SetReportTextf(T("open error: %s; %Mm"), destname, GetLastError());
				}
				return ERROR_INVALID_DATA;
			}
			cbs->cpi->path = destname;
			for ( ; ; ){
				if ( !IsRelativeDir(celldest.f.cFileName) ) filecount++;

				if ( (filecount & 0x3f) == 0 ){
					if ( IsBreakCompare(cbs->cpi) ){
						FindClose(hFFdest);
						return ERROR_CANCELLED;
					}
				}
				if ( FALSE == FindNextFile(hFFdest, &celldest.f) ) break;
			}
			FindClose(hFFdest);
								// こちら側のファイル情報を順次求め、比較
			CatPath(tempname, srcname, T("*"));
			hFFsrc = FindFirstFileL(tempname, &ffsrc);
			if ( hFFsrc == INVALID_HANDLE_VALUE ){
				if ( cbs->cpi->settings & (CMP_MATCHLOG | CMP_UNMATCHLOG) ){
					SetReportTextf(T("open error: %s; %Mm"), srcname, GetLastError());
				}
				return ERROR_INVALID_DATA;
			}
			result = NO_ERROR;
			for ( ; ; ){
				if ( !IsRelativeDir(ffsrc.cFileName) ){
					filecount--;
					CatPath(tempname, destname, ffsrc.cFileName);
					if ( FALSE == GetFileSTAT(tempname, &celldest.f) ){
						if ( cbs->cpi->settings & CMP_UNMATCHLOG ){
							SetReportTextf(T("open error: %s; %Mm"), tempname, GetLastError());
						}
						result = ERROR_INVALID_DATA;
					}else{
						ERRORCODE err;
						err = PPcCompareBinary(cbs,
								srcname, &ffsrc, destname, &celldest);
						if ( err != NO_ERROR ){
							result = err;
							if ( (result == ERROR_CANCELLED) || !(cbs->cpi->settings & CMPMASK_ALLSEARCH) ) break;
						}
					}
				}

				if ( FALSE == FindNextFile(hFFsrc, &ffsrc) ){
					if ( filecount ){
						result = ERROR_INVALID_DATA;
						if ( cbs->cpi->settings & CMP_UNMATCHLOG ){
							SetReportTextf(T("unmatch %d entries: %s != %s"), filecount, tempname, srcname);
						}
					}
					break;
				}
			}
			FindClose(hFFsrc);
			return result;
		}
		return ERROR_INVALID_DATA;
	}

	if ( cellp->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
		if ( cbs->cpi->settings & CMP_UNMATCHLOG ){
			SetReportTextf(T("unmatch file != directory: %s; %s != %s"), srcpath, cellN->cFileName, destpath);
		}
		return ERROR_INVALID_DATA;
	}
								// ファイルサイズが同じ？
	if ( (cellN->nFileSizeHigh != cellp->f.nFileSizeHigh) ||
		 (cellN->nFileSizeLow != cellp->f.nFileSizeLow) ){
		if ( cbs->cpi->settings & CMPMASK_UNMATCH ){
			if ( cbs->cpi->settings & CMP_UNMATCHLOG ){
				CompareReport_2size(
						T("unmatch same size: %s(%'Ld) != %s(%'Ld)"),
						cellN, &cellp->f);
//				SetReportTextf(T("unmatch same size: %s; %s(%'Ld) != %s(%'Ld)"),
//						srcpath, cellN->cFileName, ssize.rawdata,
//						destpath, psize.rawdata);
			}
			if ( cbs->cpi->settings & CMPLISTFILE ){
				ENTRYCELL cell;

				VFSFullPath(destname, cellN->cFileName, destpath);
				if ( GetFileSTAT(destname, &cell.f) ){
					WriteCompareList(cbs->cpi, &cell, FALSE);
				}
			}
		}
		return ERROR_INVALID_DATA;
	}
								// イメージチェック ---------------------------
#ifndef ERROR_SYMLINK_CLASS_DISABLED
#define ERROR_SYMLINK_CLASS_DISABLED 1463 // ネットワーク越しのショートカット先が見つからないとき
#endif

	VFSFullPath(srcname, cellN->cFileName, srcpath);
	VFSFullPath(destname, cellN->cFileName, destpath);
	hSrc = CreateFileL(srcname, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hSrc == INVALID_HANDLE_VALUE ){
		result = GetLastError();

		if ( cbs->cpi->settings & CMP_UNMATCHLOG ){
			SetReportTextf(T("open error: %s; %Mm"), srcname, result);
		}
		if ( (result == ERROR_FILE_NOT_FOUND) || (result == ERROR_SYMLINK_CLASS_DISABLED) ){
			result = ERROR_INVALID_DATA;
		}
		return result;
	}
	hDest = CreateFileL(destname, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hDest == INVALID_HANDLE_VALUE ){
		result = GetLastError();

		if ( cbs->cpi->settings & CMP_UNMATCHLOG ){
			SetReportTextf(T("open error: %s; %Mm"), destname, result);
		}
		if ( (result == ERROR_FILE_NOT_FOUND) || (result == ERROR_SYMLINK_CLASS_DISABLED) ){
			result = ERROR_INVALID_DATA;
		}
		CloseHandle(hSrc);
		return result;
	}
	result = NO_ERROR;
	cbs->cpi->path = cellN->cFileName; // srcname,destname はSHA内で破壊される
	if ( cbs->mode == CMP_BINARY ){ // CMP_BINARY
		for ( ; ; ){
			if ( FALSE == ReadFile(hSrc, cbs->srcbuf, cbs->bufsize, &srcsize, NULL) ){
				result = GetLastError();
				break;
			}
			if ( srcsize == 0 ) break;

			if ( FALSE == ReadFile(hDest, cbs->destbuf, cbs->bufsize, &destsize, NULL) ){
				result = GetLastError();
				break;
			}
			if ( (srcsize != destsize) ||
				memcmp(cbs->srcbuf, cbs->destbuf, srcsize) ){
				if ( cbs->cpi->settings & CMPMASK_UNMATCH ){
					if ( cbs->cpi->settings & CMP_UNMATCHLOG ){
						SetReportTextf(T("unmatch binary: %s != %s"),
								destname, srcname);
					}
					if ( cbs->cpi->settings & CMPLISTFILE ){
						WriteCompareList(cbs->cpi, cellp, FALSE);
					}
				}
				result = ERROR_INVALID_DATA;
				break;
			}

			if ( IsBreakCompare(cbs->cpi) ){
				result = ERROR_CANCELLED;
				break;
			}
		}
		if ( (result == NO_ERROR) && (cbs->cpi->settings & CMPMASK_MATCH) ){
			if ( cbs->cpi->settings & CMP_MATCHLOG ){
				SetReportTextf(T("match binary: %s == %s"),
						destname, srcname);
			}
			if ( cbs->cpi->settings & CMPLISTFILE ){
				WriteCompareList(cbs->cpi, cellp, TRUE);
			}
		}
	}else{ // CMP_SHA1
		SHA1Context sha1;
		BYTE digest_src[SHA1HashSize], digest_dest[SHA1HashSize];

		SHA1Reset(&sha1);
		for ( ; ; ){
			if ( FALSE == ReadFile(hSrc, cbs->srcbuf, cbs->bufsize, &srcsize, NULL) ){
				result = GetLastError();
				break;
			}
			if ( srcsize == 0 ) break;
			SHA1Input(&sha1, (uint8_t *)cbs->srcbuf, srcsize);

			if ( IsBreakCompare(cbs->cpi) ){
				result = ERROR_CANCELLED;
				break;
			}
		}
		SHA1Result(&sha1, (uint8_t *)&digest_src);

		if ( result == NO_ERROR ){
			SHA1Reset(&sha1);
			for ( ; ; ){
				if ( FALSE == ReadFile(hDest, cbs->destbuf, cbs->bufsize, &destsize, NULL) ){
					result = GetLastError();
					break;
				}
				if ( destsize == 0 ) break;
				SHA1Input(&sha1, (uint8_t *)cbs->destbuf, destsize);

				if ( IsBreakCompare(cbs->cpi) ){
					result = ERROR_CANCELLED;
					break;
				}
			}
			SHA1Result(&sha1, (uint8_t *)&digest_dest);
			if ( result == NO_ERROR ){
				if ( memcmp(digest_src, digest_dest, SHA1HashSize) ){
					if ( cbs->cpi->settings & CMPMASK_UNMATCH ){
						if ( cbs->cpi->settings & CMP_UNMATCHLOG ){
							SetReportTextf(T("unmatch hash: %s; %s != %s"),
								cellN->cFileName,
								destname, srcname);
						}
						if ( cbs->cpi->settings & CMPLISTFILE ){
							WriteCompareList(cbs->cpi, cellp, FALSE);
						}
					}
					result = ERROR_INVALID_DATA;
				}
			}
		}
		if ( (result == NO_ERROR) && (cbs->cpi->settings & CMPMASK_MATCH) ){
			if ( cbs->cpi->settings & CMP_MATCHLOG ){
				SetReportTextf(T("match hash: %s == %s"),
						destname, srcname);
			}
			if ( cbs->cpi->settings & CMPLISTFILE ){
				WriteCompareList(cbs->cpi, cellp, TRUE);
			}
		}
	}
	CloseHandle(hSrc);
	CloseHandle(hDest);
	return result;
}

ERRORCODE PPcCompareBinaryMain(COMPPROGINFO *cpi, int mode, int subdir, WIN32_FIND_DATA *cellN, const TCHAR *destpath, ENTRYCELL *cellp)
{
	struct COMPAREBINARYSTRUCT cbs;
	ERRORCODE result;

	GetAsyncKeyState(VK_PAUSE); // 読み捨て(最下位bit対策)
	cbs.cpi = cpi;
					// 2000以降:1Mbytes / 9x,NT4:256kbytes
	cbs.bufsize = (OSver.dwMajorVersion >= 5) ? (1 * MB) : (256 * KB);
	cbs.srcbuf = PPcHeapAlloc(cbs.bufsize);
	cbs.destbuf = PPcHeapAlloc(cbs.bufsize);
	cbs.mode = mode;
	cbs.subdir = subdir;
	result = PPcCompareBinary(&cbs, cpi->cinfo->RealPath, cellN, destpath, cellp);
	PPcHeapFree(cbs.srcbuf);
	PPcHeapFree(cbs.destbuf);
	return result;
}

void MessageMatchEntry(HWND hWnd, HWND hPairWnd, DWORD PairLoadCounter, TCHAR *filename)
{
	COPYDATASTRUCT copydata;

	copydata.dwData = TMAKELPARAM('O' + 0x100, PairLoadCounter);
	copydata.cbData = TSTRSIZE32(filename);
	copydata.lpData = filename;
	SendMessage(hPairWnd, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&copydata);
}

//------------------------------------------------------------------ 比較メイン
ERRORCODE PPcCompareMain(PPC_APPINFO *cinfo, HWND hPairWnd, COMPAREMARKPACKET *cmp)
{
	ENTRYCELL *paircell = NULL, *pairbase;
	int mode;
	int cmp_match = 0, cmp_total = 0;
	ENTRYCOUNT cells;			// 相手のセル数
	DWORD sizeL;		// ファイルの大きさ
//	TCHAR pairpath[VFPS];
	DWORD PairLoadCounter;
	DWORD dirmask, unmarkmask;
	int subdir;
	BOOL SimpleDir; // ファイル名がディレクトリ内で一意かどうか(速度最適化用)
	ERRORCODE result;
	COMPPROGINFO cpi;

	mode = cmp->mode;
	PairLoadCounter = cmp->LoadCounter;
	cinfo->MarkMask = (mode >> CMPMARKSHIFT) & CMPMARKMASK;
	subdir = (mode & CMP_SUBDIR) ? FILE_ATTRIBUTE_DIRECTORY : 0;
	dirmask = cinfo->MarkMask & FILE_ATTRIBUTE_DIRECTORY;
	unmarkmask = (cinfo->MarkMask | subdir) ^ CMPMARKMASK;
	cpi.settings = mode;
	cpi.sts.hFile = NULL;
	mode &= CMPTYPEMASK | CMPDOWN;

	if ( (mode == (CMP_BINARY | CMPDOWN)) ||
		 (mode == (CMP_SIZEONLY | CMPDOWN)) ||
		 (mode == (CMP_SHA1 | CMPDOWN)) /* ||
		 ((mode == (CMP_SAMESIZETIME | CMPDOWN)) && (subdir != 0)) */
	){
		Repaint(cinfo);
		return NO_ERROR;	// 済み
	}
	/*
	if ( (((mode == CMP_EXIST) || (mode == CMP_SAMESIZETIME)) && (subdir != 0)) ||
		 (mode == CMP_BINARY) || (mode == CMP_SHA1) ){


	#if 0 // ●shell: 系のファイル比較を禁止する処理だった思うが、listfile が使えなくなるので無効にした 2.02
		DWORD attr;
		attr = GetFileAttributesL(pairpath);
		if ( (attr == BADATTR) || !(attr & FILE_ATTRIBUTE_DIRECTORY) ){
			return ERROR_BAD_DEV_TYPE;
		}
	#endif
	}
	*/

	if ( ((mode & CMPTYPEMASK) == CMP_COMMENT) ||
		 ((mode & CMPTYPEMASK) == CMP_COMMENTONLY) ){
		return PPcCompareComment(cinfo, hPairWnd, cmp, &cpi, unmarkmask);
	}
										// File Open --------------------------
	result = LoadFileImage(cmp->filename, 0,
			(char **)&paircell, &sizeL, LFI_ALWAYSLIMITLESS);
	if ( result != NO_ERROR ){
		SetReportTextf(T("open error: %s; %Mm\r\n"), cmp->filename, result);
		return result;
	}

	if ( cmp->listfile[0] != '\0' ){
		CreateHandleForListFile(cinfo, &cpi.sts, cmp->listfile,
				(cpi.settings & CMPJSON) ?
					 WLFC_DETAIL | WLFC_COMMENT | WLFC_WITHMARK | WLFC_JSON :
					 WLFC_DETAIL | WLFC_COMMENT | WLFC_WITHMARK);
		if ( cpi.sts.hFile != NULL ) setflag(cpi.settings, CMPLISTFILE);
	}

	PPxCommonCommand(cinfo->info.hWnd, JOBSTATE_COMPARE, K_ADDJOBTASK);
	if ( (mode == CMP_BINARY) || (mode == CMP_SHA1) ){
		SetPopMsg(cinfo, POPMSG_PROGRESSBUSYMSG, MES_SCMP);
		UpdateWindow_Part(cinfo->info.hWnd);
	}else{
		StopPopMsg(cinfo, PMF_STOP);
	}
										// 比較準備 ---------------------------
	cpi.cinfo = cinfo;
	cpi.tick = GetTickCount();
	cells = sizeL / sizeof(ENTRYCELL);
	pairbase = paircell;
	// 相対指定を除去
	while ( cells && IsRelativeDir(pairbase->f.cFileName) ){
		pairbase++;
		cells--;
	}
	{
		ENTRYINDEX in;

		for ( in = 0 ; in < cells ; in++ ){
			if ( pairbase[in].state < ECS_NORMAL ){
				// SysMsgやDeletedは比較対象外にする
				pairbase[in].f.cFileName[0] = '\0';
			}
		}
	}
	SimpleDir = (cinfo->e.Dtype.mode == VFSDT_PATH) && (cmp->DirType == VFSDT_PATH);
										// 比較ループ -------------------------

	if ( mode & CMP_WITHOUT_EXT ){
		ENTRYINDEX ip;
		int extoffset;

		for ( cpi.in = 0 ; cpi.in < cinfo->e.cellIMax ; cpi.in++ ){
			TCHAR *nowname, *pairname;
			ENTRYCELL *cellN, *cellp;

			cellN = &CEL(cpi.in);
			if ( cellN->state < ECS_NORMAL ) continue; // SysMsgやDeletedはだめ
			if ( cellN->attr & (ECA_PARENT | ECA_THIS) ) continue;
			if ( cellN->f.dwFileAttributes & unmarkmask ) continue;

			nowname = FindLastEntryPoint(cellN->f.cFileName);
			extoffset = cellN->ext - (nowname - cellN->f.cFileName);
			cellp = pairbase;
			for ( ip = cells ; ip ; ip--, cellp++ ){
				DWORD nowtick;
														// マーク対象外属性？
				if ( cellp->f.dwFileAttributes & unmarkmask ) continue;

				nowtick = GetTickCount();
				if ( (nowtick - cpi.tick) > CPI_INTERVAL ){
					cpi.tick = nowtick;
					cpi.path = cellN->f.cFileName;
					if ( IsTrue(CompareProgress(&cpi)) ) break;
				}
				cmp_total++;
				pairname = FindLastEntryPoint(cellp->f.cFileName);
				if ( tstrnicmp(nowname, pairname, extoffset) ) continue; // 名前不一致
				// 名前末尾不一致
				if ( (pairname[extoffset] != '\0') && // 末尾でない
					  ((pairname[extoffset] != '.') || // 拡張子でない
					   (tstrchr(pairname + extoffset + 1, '.') != NULL)) ){
					continue;
				}
				if ( cellp->f.dwFileAttributes & dirmask ){
							// 一方がファイルで、他方がディレクトリなら対象外
					if ( (cellp->f.dwFileAttributes ^
						cellN->f.dwFileAttributes) & FILE_ATTRIBUTE_DIRECTORY){
						if ( cpi.settings & CMP_UNMATCHLOG ){
							SetReportTextf(T("unmatch file != directory: %s != %s"), cellN->f.cFileName, pairname);
						}
						continue;
					}
				}
				cmp_match++;
				if ( cpi.settings & CMP_MATCHLOG ){
					SetReportTextf(T("match name: %s == %s\r\n"),
							cellN->f.cFileName, pairname);
				}
				if ( cpi.settings & CMPLISTFILE ){
					WriteCompareList(&cpi, cellp, TRUE);
				}
				CellMark(cinfo, cpi.in, MARK_CHECK);
			}
		}
	}else if ( mode != CMP_SIZEONLY ){
		ENTRYINDEX ip;

		for ( cpi.in = 0 ; cpi.in < cinfo->e.cellIMax ; cpi.in++ ){
			TCHAR *nowname, *pairname;
			ENTRYCELL *cellN, *cellp;

			cellN = &CEL(cpi.in);
			if ( cellN->state < ECS_NORMAL ) continue; // SysMsgやDeletedはだめ
			if ( cellN->attr & (ECA_PARENT | ECA_THIS) ) continue;
			if ( cellN->f.dwFileAttributes & unmarkmask ) continue;

			nowname = FindLastEntryPoint(cellN->f.cFileName);
			cellp = pairbase;
			for ( ip = cells ; ip ; ip--, cellp++ ){
				DWORD nowtick;
														// マーク対象外属性？
				if ( cellp->f.dwFileAttributes & unmarkmask ) continue;

				nowtick = GetTickCount();
				if ( (nowtick - cpi.tick) > CPI_INTERVAL ){
					cpi.tick = nowtick;
					cpi.path = cellN->f.cFileName;
					if ( IsTrue(CompareProgress(&cpi)) ) break;
				}
				cmp_total++;
				pairname = FindLastEntryPoint(cellp->f.cFileName);
				if ( tstricmp(nowname, pairname) ) continue;

				if ( cellp->f.dwFileAttributes & dirmask ){
							// 一方がファイルで、他方がディレクトリなら対象外
					if ( (cellp->f.dwFileAttributes ^
							cellN->f.dwFileAttributes) & FILE_ATTRIBUTE_DIRECTORY){
						if ( (cpi.settings & CMP_UNMATCHLOG) && (mode & CMPDOWN) ){
							SetReportTextf(T("unmatch file != directory: %s != %s"), nowname, pairname);
						}
						continue;
					}
				}

				switch(mode){
					case CMP_NEW:				// old(反対窓 new file)
					case CMP_TIME:				// old(反対窓 TimeStamp)
						if ( cellN->f.dwFileAttributes & subdir ){
							if ( NO_ERROR != PPcCompareDirMain(
									&cpi, cinfo->RealPath,
									&cellN->f, cmp->path) ){
								break;
							}
							MessageMatchEntry(cinfo->info.hWnd, hPairWnd,
									PairLoadCounter, cellp->f.cFileName);
						}
						if ( FuzzyCompareFileTime(&cellN->f.ftLastWriteTime,
								&cellp->f.ftLastWriteTime) < 0 ){
							cmp_match++;

							if ( cpi.settings & CMP_MATCHLOG ){
								SetReportTextf(T("match new time: %s; %MF > %MF"),
										cellN->f.cFileName,
										&cellp->f.ftLastWriteTime,
										&cellN->f.ftLastWriteTime);
							}
							if ( cpi.settings & CMPLISTFILE ){
								WriteCompareList(&cpi, cellp, TRUE);
							}
							CellMark(cinfo, cpi.in, MARK_CHECK);
							break;
						}
						if ( cpi.settings & CMP_UNMATCHLOG ){
							SetReportTextf(T("unmatch new time: %s; %MF <= %MF"),
									cellN->f.cFileName,
									&cellp->f.ftLastWriteTime,
									&cellN->f.ftLastWriteTime);
						}
						break;

					case CMP_NEW | CMPDOWN:  // new(現在窓 new file)
					case CMP_TIME | CMPDOWN: // new(現在窓 TimeStamp)
						if ( FuzzyCompareFileTime(&cellN->f.ftLastWriteTime,
								&cellp->f.ftLastWriteTime) > 0 ){
							cmp_match++;
							// CMP_MATCHLOG ロギング不要
							CellMark(cinfo, cpi.in, MARK_CHECK);
						}
						// CMP_UNMATCHLOG ロギング不要
						break;

					case CMP_EXIST | CMPDOWN: // Exist(現在窓 Exist)
						if ( cellN->f.dwFileAttributes & subdir ){
							// CMP_UNMATCHLOG ロギング不要
							break;
						}
						cmp_match++;
						// CMP_MATCHLOG ロギング不要
						CellMark(cinfo, cpi.in, MARK_CHECK);
						break;

					case CMP_EXIST: // Exist(反対窓 Exist)
						if ( cellN->f.dwFileAttributes & subdir ){
							if ( NO_ERROR != PPcCompareDirMain(
									&cpi, cinfo->RealPath,
									&cellN->f, cmp->path) ){
								break;
							}
							MessageMatchEntry(cinfo->info.hWnd, hPairWnd,
									PairLoadCounter, cellp->f.cFileName);
						}
						if ( cpi.settings & CMP_MATCHLOG ){
							SetReportTextf(T("match exist: %s == %s"),
									nowname, pairname);
						}
						if ( cpi.settings & CMPLISTFILE ){
							WriteCompareList(&cpi, cellp, TRUE);
						}
						cmp_match++;
						CellMark(cinfo, cpi.in, MARK_CHECK);
						break;

					case CMP_SIZE:				// small size(反対窓 FileSize)
						if ( cellN->f.dwFileAttributes & subdir ){
							if ( NO_ERROR != PPcCompareDirMain(
									&cpi, cinfo->RealPath,
									&cellN->f, cmp->path) ){
								break;
							}
							MessageMatchEntry(cinfo->info.hWnd, hPairWnd,
									PairLoadCounter, cellp->f.cFileName);
						}
						// CMP_SIZE | CMPDOWN へ
					case CMP_SIZE | CMPDOWN:	// large size(現在窓 FileSize)
						if ( (cellN->f.nFileSizeHigh != cellp->f.nFileSizeHigh) ||
							 (cellN->f.nFileSizeLow != cellp->f.nFileSizeLow) ){
							cmp_match++;
							if ( (cpi.settings & CMP_MATCHLOG) && !(mode & CMPDOWN) ){
								UINTHL ssize, psize;

								LetHLFilesize(ssize, cellN->f);
								LetHLFilesize(psize, cellp->f);
								SetReportTextf(T("match different size: %s; %'Ld != %'Ld"),
										nowname, ssize.rawdata, psize.rawdata);
							}
							if ( (cpi.settings & CMPLISTFILE) && !(mode & CMPDOWN) ){
								WriteCompareList(&cpi, cellp, TRUE);
							}
							CellMark(cinfo, cpi.in, MARK_CHECK);
						}else{
							if ( (cpi.settings & CMP_UNMATCHLOG) && !(mode & CMPDOWN) ){
								UINTHL ssize;

								LetHLFilesize(ssize, cellN->f);
								SetReportTextf(T("unmatch different size: %s; %'Ld == %'Ld"),
										nowname, ssize.rawdata, ssize.rawdata);
							}
						}
						break;

					case CMP_LARGE:				// small size(反対窓 FileSize)
						if ( cellN->f.dwFileAttributes & subdir ){
							if ( NO_ERROR != PPcCompareDirMain(
									&cpi, cinfo->RealPath,
									&cellN->f, cmp->path) ){
								break;
							}
							MessageMatchEntry(cinfo->info.hWnd, hPairWnd,
									PairLoadCounter, cellp->f.cFileName);
						}

						if ( CompareFilesize(&cellN->f, &cellp->f) < 0 ){
							cmp_match++;
							if ( cpi.settings & CMP_MATCHLOG ){
								UINTHL ssize, psize;

								LetHLFilesize(ssize, cellN->f);
								LetHLFilesize(psize, cellp->f);
								SetReportTextf(T("match large size: %s(%'Ld) > %s(%'Ld)"),
									pairname, psize.rawdata,
									nowname, ssize.rawdata);
							}
							if ( cpi.settings & CMPLISTFILE ){
								WriteCompareList(&cpi, cellp, TRUE);
							}
							CellMark(cinfo, cpi.in, MARK_CHECK);
							break;
						}

						if ( cpi.settings & CMP_UNMATCHLOG ){
							CompareReport_2size(
									T("unmatch large size: %s(%'Ld) <= %s(%'Ld)"),
									&cellp->f, &cellN->f);
//								nowname, ssize.rawdata);
						}
						break;

					case CMP_LARGE | CMPDOWN:	// large size(現在窓 FileSize)
						if ( CompareFilesize(&cellN->f, &cellp->f) > 0 ){
							cmp_match++;
							// CMP_MATCHLOG ロギング不要
							CellMark(cinfo, cpi.in, MARK_CHECK);
							break;
						}
						// CMP_UNMATCHLOG ロギング不要
						break;

					case CMP_SAMESIZETIME | CMPDOWN:	// Same(現在窓 FileSize)
					case CMP_SAMESIZETIME:				// Same
						if ( cellN->f.dwFileAttributes & subdir ){
							if ( NO_ERROR != PPcCompareSizeTimeDirMain(
									&cpi, cinfo->RealPath,
									&cellN->f, cmp->path) ){
								if ( (cpi.settings & CMP_UNMATCHLOG) && !(mode & CMPDOWN) ){
									SetReportTextf(T("unmatch size & time subdir: %s"),
											cellN->f.cFileName);
								}
								break;
							}
							MessageMatchEntry(cinfo->info.hWnd, hPairWnd,
									PairLoadCounter, cellp->f.cFileName);
							if ( (cpi.settings & CMP_MATCHLOG) && !(mode & CMPDOWN) ){
								SetReportTextf(T("match size & time subdir: %s"),
										cellN->f.cFileName);
							}
							if ( (cpi.settings & CMPLISTFILE) && !(mode & CMPDOWN) ){
								WriteCompareList(&cpi, cellp, TRUE);
							}
							cmp_match++;
							CellMark(cinfo, cpi.in, MARK_CHECK);
							break;
						}
						if ( FuzzyCompareFileTime(&cellN->f.ftLastWriteTime,
								&cellp->f.ftLastWriteTime) ){
							if ( (cpi.settings & CMP_UNMATCHLOG) && !(mode & CMPDOWN) ){
								SetReportTextf(T("unmatch size & time: %s; %MF <= %MF"),
										cellN->f.cFileName,
										&cellN->f.ftLastWriteTime,
										&cellp->f.ftLastWriteTime);
							}
							break;
						}
						if ( (cellN->f.nFileSizeHigh != cellp->f.nFileSizeHigh) ||
							 (cellN->f.nFileSizeLow != cellp->f.nFileSizeLow) ){
							if ( (cpi.settings & CMP_UNMATCHLOG) && !(mode & CMPDOWN) ){
								UINTHL ssize, psize;

								LetHLFilesize(ssize, cellN->f);
								LetHLFilesize(psize, cellp->f);
								SetReportTextf(T("unmatch size & time: %s; %'Ld != %'Ld"),
									cellN->f.cFileName,
									ssize.rawdata, psize.rawdata);
							}
							break;
						}
						if ( (cpi.settings & CMP_MATCHLOG) && !(mode & CMPDOWN) ){
							UINTHL ssize;

							LetHLFilesize(ssize, cellN->f);
							SetReportTextf(T("match size & time: %s; %'Ld & %MF"),
									cellN->f.cFileName,
									ssize.rawdata,
									&cellN->f.ftLastWriteTime);
						}
						if ( (cpi.settings & CMPLISTFILE) && !(mode & CMPDOWN) ){
							WriteCompareList(&cpi, cellp, TRUE);
						}
						cmp_match++;
						CellMark(cinfo, cpi.in, MARK_CHECK);
						break;

					case CMP_BINARY | CMPDOWN:	// Same Binary(現在窓)
					case CMP_SHA1 | CMPDOWN:	// SHA1(現在窓)
						// 何もしない
						break;

					case CMP_SHA1:				// Same SHA-1
					case CMP_BINARY: {			// Same Binary
						ERRORCODE result;
												// SysMsgやDeletedなのでだめ
						if ( cellp->state < ECS_NORMAL ) break;
						result = PPcCompareBinaryMain(&cpi, mode, subdir,
								&cellN->f, cmp->path, cellp);
						if ( result == NO_ERROR ){
							cmp_match++;

							if ( (cpi.settings & CMP_MATCHLOG) && (cellN->f.dwFileAttributes & subdir) ){
								SetReportTextf(T("match subdir: %s"),
										cellN->f.cFileName);
							}
							CellMark(cinfo, cpi.in, MARK_CHECK);
							MessageMatchEntry(cinfo->info.hWnd, hPairWnd,
									PairLoadCounter, cellp->f.cFileName);
							break;
						}
						if ( result == ERROR_CANCELLED ) break;
						if ( result == ERROR_INVALID_DATA ){
							if ( (cpi.settings & CMP_MATCHLOG) && (cellN->f.dwFileAttributes & subdir) ){
								SetReportTextf(T("unmatch subdir: %s"),
										cellN->f.cFileName);
							}
							break;
						}
						// エラー
						SetReportTextf(T("error: %s; %Mm\r\n"),
								cellN->f.cFileName, result);
						break;
					}
//					default:	// 不明動作は処理しない
				}

				// SimpleDir の時は、比較済みの比較対象を次回の比較対象から外す
				if ( SimpleDir ){
					if ( pairbase == cellp ){ // 先頭の時
						pairbase++;
						cells--;

						while ( cells && (pairbase->f.cFileName[0] == '\0') ){
							pairbase++;
							cells--;
						}
					}else{
						cellp->f.cFileName[0] = '\0';	// 比較したので消去
					}
				}
				break;
			}
														// new file
			if ( (ip == 0) &&
				((mode == (CMP_NEW | CMPDOWN)) || (mode == CMP_NEWONLY) || (mode == (CMP_NEWONLY | CMPDOWN))) ){
				if ( cpi.settings & CMP_MATCHLOG ){
					SetReportTextf(T("match new file: %s"), cellN->f.cFileName);
				}
				if ( (cpi.settings & CMPLISTFILE) && !(mode & CMPDOWN) ){
					WriteCompareList(&cpi, cellp, TRUE);
				}
				cmp_match++;
				CellMark(cinfo, cpi.in, MARK_CHECK);
			}
		}
	}else{ // CMP_SIZEONLY
		ENTRYINDEX ip;
		int hilight = 0;

		for ( cpi.in = 0 ; cpi.in < cinfo->e.cellIMax ; cpi.in++ ){
			ENTRYCELL *cellN, *cellp;

			cellN = &CEL(cpi.in);
			if ( cellN->state < ECS_NORMAL ) continue; // SysMsgやDeletedはだめ
			if ( cellN->attr & (ECA_PARENT | ECA_THIS) ) continue;
			if ( cellN->f.dwFileAttributes & unmarkmask ) continue;

			cmp_total++;
			cellp = pairbase;
			for ( ip = cells ; ip ; ip--, cellp++ ){
				DWORD nowtick;
														// マーク対象外属性？
				if ( cellp->f.dwFileAttributes & unmarkmask ) continue;

				nowtick = GetTickCount();
				if ( (nowtick - cpi.tick) > CPI_INTERVAL ){
					cpi.tick = nowtick;
					cpi.path = cellN->f.cFileName;
					if ( IsTrue(CompareProgress(&cpi)) ) break;
				}

				if ( (cellN->f.nFileSizeHigh == cellp->f.nFileSizeHigh) &&
					 (cellN->f.nFileSizeLow  == cellp->f.nFileSizeLow) ){
					if ( cellp->f.dwFileAttributes & dirmask ){
							// 一方がファイルで、他方がディレクトリなら対象外
						if ( (cellp->f.dwFileAttributes ^ cellN->f.dwFileAttributes) &
								FILE_ATTRIBUTE_DIRECTORY ){
							continue;
						}
					}
					hilight = (hilight & 3) + 1;
					cellN->highlight = (BYTE)hilight;
					cmp_match++;
					if ( cpi.settings & CMPLISTFILE ){
						WriteCompareList(&cpi, cellp, TRUE);
					}
					CellMark(cinfo, cpi.in, MARK_CHECK);
					MessageMatchEntry(cinfo->info.hWnd, hPairWnd,
							PairLoadCounter, cellp->f.cFileName);
				}
			}
		}
	}
	ProcHeapFree(paircell);
	StopPopMsg(cinfo, PMF_STOP);

	if ( !(mode & CMPDOWN) ){
		PostMessage( ((mode == CMP_BINARY) || (mode == CMP_SHA1)) ?
				hPairWnd : cinfo->info.hWnd, WM_PPXCOMMAND, KC_ENDCOMPARE,
				((cmp_total != 0) && (cmp_match == cmp_total)) ? (LPARAM)-1 : (LPARAM)cmp_match);
	}
	Repaint(cinfo);
	if ( cpi.sts.hFile != NULL ) CloseHandle(cpi.sts.hFile);
	PPxCommonCommand(cinfo->info.hWnd, 0, K_DELETEJOBTASK);
	return NO_ERROR;
}

void ReceiveCompare(PPC_APPINFO *cinfo, COPYDATASTRUCT *copydata, WPARAM wParam)
{
	COMPAREMARKPACKET cmp;
	ERRORCODE result;

	cmp = *(COMPAREMARKPACKET *)copydata->lpData;

	if ( (cmp.mode & CMPDOWN) || !(cmp.mode & CMPWAIT) || ((cmp.mode & CMPTYPEMASK) == CMP_BINARY) ){
		ReplyMessage(TRUE);
	}

	result = PPcCompareMain(cinfo, (HWND)wParam, &cmp);
	if ( !(cmp.mode & CMPDOWN) ){		// 上りだったので下り処理をする
		if ( result == NO_ERROR ){
			setflag(cmp.mode, CMPDOWN);
			tstrcpy(cmp.path, cinfo->RealPath);
			cmp.listfile[0] = '\0';
			PPcCompareSend(cinfo, &cmp, (HWND)wParam);
		}else{ // 失敗時実行元で結果を表示
			DeleteFileL(cmp.filename);
			PostMessage( (HWND)wParam, WM_PPXCOMMAND, TMAKEWPARAM(KC_REPORT_ERROR, 0xf0fe), (LPARAM)result);
		}
	}else{							// 処理後、終了処理
		DeleteFileL(cmp.filename);
		if ( result != NO_ERROR ){
			WmPPxCommand(cinfo, TMAKEWPARAM(KC_REPORT_ERROR, 0xf0fe), (LPARAM)result);
		}
	}
}

void PPcCompareOneWindow(PPC_APPINFO *cinfo, int mode)
{
	int i, oldmatch = -1, hilight = 0, matchpair = 0;
	XC_SORT xc = { {{0, -1, -1, -1}}, 0, SORTE_DEFAULT_VALUE};

	cinfo->MarkMask = (mode >> CMPMARKSHIFT) & CMPMARKMASK;
	mode = (mode & CMPTYPEMASK) - CMP_1SIZE;
	xc.mode.dat[0] = (char)((mode == 0) ? 2 : 17); // 検索しやすいようにソート
	PPC_SortMain(cinfo, &xc);

	for ( i = 0 ; i < cinfo->e.cellIMax - 1 ; i++ ){
		if ( mode == 0 ){
			if ( (CEL(i).f.nFileSizeHigh == CEL(i + 1).f.nFileSizeHigh) &&
				 (CEL(i).f.nFileSizeLow == CEL(i + 1).f.nFileSizeLow) ){
				if ( (CEL(i).f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
					 (CEL(i+1).f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
					continue;
				}
			}else{
				continue;
			}
		}else{
			if ((CEL(i).comment != EC_NOCOMMENT) &&
				(CEL(i + 1).comment != EC_NOCOMMENT) &&
				!tstrcmp(ThPointerT(&cinfo->e.Comments, CEL(i).comment),
						 ThPointerT(&cinfo->e.Comments, CEL(i + 1).comment)) ){
				// 空行
			}else{
				continue;
			}
		}
		// 該当有り
		if ( i != oldmatch ){ // 非連続
			hilight = (hilight & 3) + 1;
			CellMark(cinfo, i, MARK_CHECK);
			CEL(i).highlight = (BYTE)hilight;
		}
		oldmatch = i + 1;
		CellMark(cinfo, i + 1 , MARK_CHECK);
		CEL(i + 1).highlight = (BYTE)hilight;
		matchpair++;
	}

	xc.mode.dat[0] = 6;					// マークエントリを集める
	PPC_SortMain(cinfo, &xc);
	Result_Match(cinfo, matchpair);
	Repaint(cinfo);
}

DWORD CompareHashMain(PPC_APPINFO *cinfo, const TCHAR *hashtop, DWORD hashlen)
{
	ENTRYCELL *cell;
	int work;
	int match = 0;
	TCHAR *cmtptr;

	InitEnumMarkCell(cinfo, &work);
	while ( (cell = EnumMarkCell(cinfo, &work)) != NULL ){
		if ( cell->comment == EC_NOCOMMENT ) continue;
		cmtptr = ThPointerT(&cinfo->e.Comments, cell->comment);
		cmtptr = tstrchr(cmtptr, ':');
		if ( cmtptr == NULL ) continue;

		if ( !memicmp(cmtptr + 1, hashtop, hashlen * sizeof(TCHAR)) ){
			cell->highlight = 1;
			Repaint(cinfo);
			match++;
		}
	}
	return match;
}

DWORD CompareHashFromClipBoard(PPC_APPINFO *cinfo, int comparemode)
{
	TCHAR ClipBoardText[0x800], *ptr, *hashtop;
	HGLOBAL hG;
	size_t size = 0;
	int match;
	DWORD oldhashlen = 0;

	if ( OpenClipboardCheck(cinfo) == FALSE ) return 0;

#ifdef UNICODE
	hG = GetClipboardData(CF_UNICODETEXT);
#else
	hG = GetClipboardData(CF_TEXT);
#endif
	if ( hG != NULL ) {
		size = GlobalSize(hG);
		if ( size < sizeof(ClipBoardText) ){
			memcpy(ClipBoardText, GlobalLock(hG), size);
			ClipBoardText[size / sizeof(TCHAR)] = '\0';
		}
	}
	CloseClipboard();
	if ( size == 0 ) return 0;

	match = 0;
	for ( ptr = ClipBoardText; ; ptr++ ){ // テキスト中のハッシュを探す
		TCHAR c, *type;
		DWORD hashlen;

		c = *ptr;
		if ( c == '\0' ) break;
		if ( !Isxdigit(c) ) continue;
		hashtop = ptr++;
		hashlen = 1;
		for ( ;; ){ // 16進文字列の長さを求める
			c = *ptr;
			if ( !Isxdigit(c) ) break;
			ptr++;
			hashlen++;
		}
		switch (hashlen){
			case 8:
				type = T("crc32");
				break;

			case 32:
				type = T("md5");
				break;

			case 40:
				type = T("sha1");
				break;

			case 56:
				type = T("sha224");
				break;

			case 64:
				type = T("sha256");
				break;

			default: // 該当するものが無かった
				continue;
		}
		if ( comparemode ){ // 各エントリと比較
			if ( hashlen != oldhashlen ){
				if ( CommentCommand(cinfo, type) != NO_ERROR ) return 0;
				oldhashlen = hashlen;
			}
			match += CompareHashMain(cinfo, hashtop, hashlen);
		}else{ // ハッシュ検出のみ
			return hashlen;
		}
	}
	if ( comparemode ) Result_Match(cinfo, match);
	return 0;
}

//--------------------------------------------------------------- 処理選択&準備
ERRORCODE PPcCompare(PPC_APPINFO *cinfo, int mode, const TCHAR *listfile)
{
	HWND hPairWnd;
	int mode_type;

	hPairWnd = GetPairWnd(cinfo);
	if ( (mode & CMPMODEMASK) == 0 ){ // 方法指定無し…メニュー選択
		HMENU hMenu;

		hMenu = CreatePopupMenu();
		if ( hPairWnd ){
			COMPAREMENUSTRUCT *cm;

			for ( cm = CompareMenu ; cm->mode ; cm++ ){
				if ( cm->mode & CMP_WITHOUT_EXT ){
					AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				}
				AppendMenuString(hMenu, cm->mode, cm->name);
			}
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		}
		AppendMenuString(hMenu, CMP_1SIZE, MES_CMPZ);
		AppendMenuString(hMenu, CMP_1COMMENT, MES_CMPM);
		AppendMenu(hMenu,
				(CompareHashFromClipBoard(cinfo, 0) != 0) ? MF_ES : MF_GS,
				CMP_CLIPBOARD_HASH, MessageText(StrMesCMPH));
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenuString(hMenu, CMP_DETAIL, MES_CMPD);
		mode = PPcTrackPopupMenu(cinfo, hMenu);
		DestroyMenu(hMenu);
	}
	mode_type = mode & CMPTYPEMASK;
	if ( (mode & CMPMODEMASK) == CMP_DETAIL ){
		mode = CompareDetailDialog(cinfo->info.hWnd, hPairWnd);
		if ( mode < 0 ) mode = 0;
		mode_type = mode & CMPTYPEMASK;
	}else if ( mode ){
		if ( !(mode & (CMPMARKMASK << CMPMARKSHIFT)) ) mode |= (MARKMASK_FILE << CMPMARKSHIFT); // 属性補正
	}

	if ( mode & (CMP_MATCHLOG | CMP_UNMATCHLOG) ){
		LogwindowCommand(T("1"));
	}

	if ( mode_type == CMP_CLIPBOARD_HASH ){
		CompareHashFromClipBoard(cinfo, 1);
		return NO_ERROR;
	}
	if ( mode_type >= CMP_2WINDOW ){
		COMPAREMARKPACKET cmp;

		if ( hPairWnd == NULL ) return ERROR_CANCELLED;
		if ( (mode_type == CMP_BINARY) || (mode_type == CMP_SHA1) ){
			SetPopMsg(cinfo, POPMSG_PROGRESSBUSYMSG, MES_SCMP);
		}
		cmp.mode = mode;
		MakeTempEntry(MAX_PATH, cmp.filename, FILE_ATTRIBUTE_NORMAL);
		tstrcpy(cmp.path, cinfo->RealPath);
		if ( listfile == NULL ){
			cmp.listfile[0] = '\0';
		}else{
			tstrcpy(cmp.listfile, listfile);
		}
		PPcCompareSend(cinfo, &cmp, hPairWnd);
		return NO_ERROR;
	}
	if ( mode_type >= CMP_1WINDOW ){
		PPcCompareOneWindow(cinfo, mode);
		return NO_ERROR;
	}
	return ERROR_CANCELLED;
}

int GetCMarkMode(HWND hDlg)
{
	int mode = (int)SendMessage(hDlg, CB_GETCURSEL, 0, 0);

	if ( mode >= 0 ) mode = (int)SendMessage(hDlg, CB_GETITEMDATA, (WPARAM)mode ,0);
	return mode;
}

int GetCMarkSettings(HWND hDlg)
{
	int mode = GetCMarkMode(GetDlgItem(hDlg, IDC_TYPE));

	mode |= (GetAttibuteSettings(hDlg) << CMPMARKSHIFT) |
		(IsDlgButtonChecked(hDlg, IDX_FOP_USELOGWINDOW) ? (CMP_MATCHLOG | CMP_UNMATCHLOG) : 0) |
		(IsDlgButtonChecked(hDlg, IDX_WHERE_SDIR) ?
			(CMP_SUBDIR | (FILE_ATTRIBUTE_DIRECTORY << CMPMARKSHIFT)) : 0);
	return mode;
}

//  --------------------------------------------------------------
INT_PTR CALLBACK CompareDetailDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg){
		case WM_INITDIALOG: {
			int index = 0;

			CenterWindow(hDlg);
			LocalizeDialogText(hDlg, IDD_CMARK);
//			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);

			if ( (HWND)lParam == NULL ){ // 反対なし？→２窓系をスキップ
				index = CMP2TYPES;
			}

			for ( ; index < DETAILMENU_ITEMS ; index++ ){
				int id;

				id = SendDlgItemMessage(hDlg, IDC_TYPE,
						CB_ADDSTRING, 0, (LPARAM)MessageText(CompareDetailMenu[index]) );
				SendDlgItemMessage(hDlg, IDC_TYPE, CB_SETITEMDATA, (WPARAM)id, (LPARAM)CompareDetailIndex[index]);
			}

			{ // if ( CompareHashFromClipBoard(cinfo, 0) != 0)
				int id;

				id = SendDlgItemMessage(hDlg, IDC_TYPE,
						CB_ADDSTRING, 0, (LPARAM)MessageText(StrMesCMPH) );
				SendDlgItemMessage(hDlg, IDC_TYPE, CB_SETITEMDATA, (WPARAM)id, (LPARAM)CMP_CLIPBOARD_HASH);
			}
			SendDlgItemMessage(hDlg, IDC_TYPE, CB_SETCURSEL, 0, 0);
/*
			CheckDlgButton(hDlg, IDX_WHERE_SDIR, );
			CheckDlgButton(hDlg, IDX_FOP_USELOGWINDOW, CMP_SUBDIR );
*/
			SetAttibuteSettings(hDlg, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);

			EnableDlgWindow(hDlg, IDX_WHERE_SDIR, 0);
			ActionInfo(hDlg, NULL, AJI_SHOW, T("compare"));
			return TRUE;
		}
		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDOK:
					EndDialog(hDlg, GetCMarkSettings(hDlg));
					break;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;

				case IDC_TYPE:
					if ( HIWORD(wParam) == CBN_SELCHANGE ){
						int mode = GetCMarkMode((HWND)lParam);
						EnableDlgWindow(hDlg, IDX_WHERE_SDIR,
								(mode == CMP_EXIST) ||
								(mode == CMP_SAMESIZETIME) ||
								(mode == CMP_BINARY) ||
								(mode == CMP_SHA1)
						);
						// EnableDlgWindow の直後は、WM_PAINT のオーバライトが効かないため
						if ( X_uxt_color >= UXT_MINMODIFY ) InvalidateRect(hDlg, NULL, TRUE);
					}
					break;

//				case IDB_SAVE:
//					SaveSortSettings(hDlg);
//					break;
			}
			break;

		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}

// *CompareMark
ERRORCODE CompareMarkEntry(PPC_APPINFO *cinfo, const TCHAR *param)
{
	TCHAR paramtmp[VFPS], listfile[VFPS], code;
	TCHAR *more;
	int mode = -1;
	int markattr = -1;
	int subdir = -1;
	int option = CMPWAIT;

	listfile[0] = '\0';
	for (;;){
		code = SkipSpace(&param);
		if ( (code != '-') && (code != '/') ){
			GetCommandParameterDual(&param, paramtmp, TSIZEOF(paramtmp));
			if ( paramtmp[0] == '\0' ) break;
			if ( mode < 0 ){
				more = paramtmp;
				mode = GetIntNumber((const TCHAR **)&more); // モード
				NextParameter(&param);
				continue;
			}
			if ( markattr < 0 ){
				more = paramtmp;
				markattr = GetDwordNumber((const TCHAR **)&more); // 属性
				if ( markattr <= 0 ) markattr = MARKMASK_DIRFILE;
				NextParameter(&param);
				continue;
			}
			if ( subdir < 0 ){
				more = paramtmp;
				subdir = GetDwordNumber((const TCHAR **)&more); // 属性
				if ( subdir ) setflag(option, CMP_SUBDIR);
				continue;
			}
		}else{
			code = GetOptionParameter(&param, paramtmp, &more);
			if ( code == '\0' ) break;
			if ( tstrcmp(paramtmp + 1, T("MODE")) == 0 ){
				mode = GetIntNumber((const TCHAR **)&more);
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("SUBDIR")) == 0 ){
				setflag(option, CMP_SUBDIR);
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("JSON")) == 0 ){
				setflag(option, CMPJSON);
				if ( *more != '\0' ){
					VFSFullPath(listfile, more, cinfo->RealPath);
				}
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("NOWAIT")) == 0 ){
				resetflag(option, CMPWAIT);
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("LOG")) == 0 ){
				if ( *more == '\0' ){
					setflag(option, CMP_MATCHLOG | CMP_UNMATCHLOG);
				}else{
					for (;;){
						TCHAR c;

						c = *more++;
						if ( c == '\0' ) break;
						if ( c == 'm' ) setflag(option, CMP_MATCHLOG);
						if ( c == 'u' ) setflag(option, CMP_UNMATCHLOG);
					}
				}
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("ATTR")) == 0 ){
				markattr = GetDwordNumber((const TCHAR **)&more); // 属性
				if ( markattr <= 0 ) markattr = MARKMASK_DIRFILE;
				continue;
			}
			if ( tstrcmp(paramtmp + 1, T("LISTFILE")) == 0 ){
				VFSFullPath(listfile, more, cinfo->RealPath);
				continue;
			}
			XMessage(NULL, NULL, XM_GrERRld, StrBadOption, paramtmp);
		}
	}
	if ( mode < 0 ) mode = 0;

	if ( option & CMP_SUBDIR ){
		if ( !( (mode <= 1) ||
				((mode >= CMP_TIME) && (mode <= CMP_BINARY)) ||
				(mode == CMP_SHA1) ) ){
			SetPopMsg(cinfo, POPMSG_MSG, T("bad subdir mode"));
			return ERROR_INVALID_PARAMETER;
		}
	}
	if ( markattr < 0 ) markattr = MARKMASK_DIRFILE;

	// XMessage(NULL, NULL, XM_DbgDIA, T("%d %x %x"),mode, markattr,option);

	return PPcCompare(cinfo, mode | (markattr << CMPMARKSHIFT) | option, listfile);
}
