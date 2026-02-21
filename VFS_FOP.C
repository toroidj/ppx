/*-----------------------------------------------------------------------------
	Paper Plane xUI	 Virtual File System			ファイル一括操作
-----------------------------------------------------------------------------*/
#define ONVFSDLL		// VFS.H の DLL export 指定
#include "WINAPI.H"
#include <shlobj.h>
#include "WINOLE.H"
#include "PPX.H"
#include "VFS.H"
#include "PPD_DEF.H"
#include "VFS_STRU.H"
#include "VFS_FOP.H"
#include "PPCOMMON.RH"
#include "FATTIME.H"
#pragma hdrstop

#define LOGDELAYTIME 120
#define HIDE_FLOG_COMPLETED		B0
#define HIDE_FLOG_SOURCE		B1
#define HIDE_FLOG_DESTINATION	B2
#define HIDE_FLOG_BACKUP		B3
#define HIDE_FLOG_SKIP			B4
#define HIDE_FLOG_MAKR_DIR		B5
DWORD X_flogh = 0; // 非表示にする処理結果ログの項目

DWORD X_fopi = 0; // 0:累積コピーサイズ 1:経過,残り時間 2:転送レート 3:なし
DWORD X_fopg = 0; // 0:ファイル単位グラフ、1:全体グラフ
DWORD X_fost = 0; // 0:左揃え(はみ出し時は右揃え) 1:左(最終エントリ左) 2:右(左) 3:左(ディレクトリ部のみ右)

volatile DWORD ProcessRunID = 0; // 現在 Mutex を使っている Fop の ID。0 = 未使用
DWORD RunIDcounter = 0; // 次の RunID (※ B31 は常に 1になる。0との区別用)

const TCHAR STR_FOP[] = T("File operation");
const TCHAR STR_FOPWARN[] = T("File operation warning");
const TCHAR STR_FOPUNDOLOG[] = T("Undo log file");

// IDOK の文字列
const TCHAR STR_FOPOK[] = MES_FOOK;
const TCHAR STR_FOPSTART[] = MES_FOST;
const TCHAR STR_FOPCONTINUE[] = MES_FOCO;
const TCHAR STR_FOPPAUSE[] = MES_FOCP;
const TCHAR STR_FOPSKIP[] = MES_FOCS;
// IDCANCEL の文字列
const TCHAR STR_FOPCANCEL[] = MES_FOCA;
const TCHAR STR_FOPCLOSE[] = MES_FOCL;

const TCHAR StrLogCopy[] = FOPLOGACTION_COPY;
const TCHAR StrLogOverwrite[] = FOPLOGACTION_OVERWRITE;
const TCHAR StrLogMove[] = FOPLOGACTION_MOVE;
const TCHAR StrLogReplace[] = FOPLOGACTION_REPLACE;
const TCHAR StrLogAppend[] = FOPLOGACTION_APPEND;
const TCHAR StrLogBackup[] = FOPLOGACTION_BACKUP;
const TCHAR StrLogLink[] = FOPLOGACTION_LINK;
const TCHAR StrLogTest[] = FOPLOGACTION_TEST;
const TCHAR StrLogExist[] = FOPLOGACTION_EXIST;
const TCHAR StrLogMoveDir[] = FOPLOGACTION_MOVEDIR;	// ディレクトリ移動
const TCHAR StrMenuItem_PPc[] = T("PP&c");
const TCHAR StrMenuItem_PPv[] = T("PP&v");
const TCHAR StrMenuItem_Sh[] = MES_SHCM;
const TCHAR StrAddOldName[] = T("-old");

const TCHAR *loghead[] = {
	StrLogCopy, StrLogOverwrite, StrLogMove, StrLogReplace, StrLogAppend, StrLogBackup, StrLogLink, StrLogMoveDir
};

#if !defined(ES_CONTINUOUS) || defined(__GNUC__)
#undef ES_SYSTEM_REQUIRED
#undef ES_CONTINUOUS
#define ES_SYSTEM_REQUIRED  ((DWORD)1)
#define ES_CONTINUOUS       ((DWORD)0x80000000)
typedef DWORD EXECUTION_STATE;
#endif
DefineWinAPI(EXECUTION_STATE, SetThreadExecutionState, (EXECUTION_STATE)) = INVALID_VALUE(impSetThreadExecutionState);

enum {
	FOPVIEWMENU_PPC = 1, FOPVIEWMENU_PPV, FOPVIEWMENU_SHMENU
};

typedef struct {
	HINTERNET hFTP[2];
	int srctype, dsttype;
	TCHAR srcPath[VFPS], dstPath[VFPS];
	TCHAR srcDIR[VFPS] , dstDIR[VFPS];
} ODVALS;


void FopViewMenu(HWND hWnd, const TCHAR *path)
{
	POINT pos;
	HMENU hPopupMenu = CreatePopupMenu();
	int index;
	TCHAR buf[CMDLINESIZE];

	GetCursorPos(&pos);
	AppendMenuString(hPopupMenu, FOPVIEWMENU_PPC, StrMenuItem_PPc);
	AppendMenuString(hPopupMenu, FOPVIEWMENU_PPV, StrMenuItem_PPv);
	AppendMenuString(hPopupMenu, FOPVIEWMENU_SHMENU, StrMenuItem_Sh);
	index = TrackPopupMenu(hPopupMenu, TPM_TDEFAULT, pos.x, pos.y, 0, hWnd, NULL);
	DestroyMenu(hPopupMenu);
	if ( index == FOPVIEWMENU_SHMENU ){
		VFSSHContextMenu(hWnd, &pos, NULL, path, NULL);
	}else if ( index != 0 ){
		thprintf(buf, TSIZEOF(buf), T("\"%s\\%s\" \"%s\""),
				DLLpath,
				(index != FOPVIEWMENU_PPC) ? PPvExeName : PPcExeName,
				path);
		ComExecSelf(hWnd, buf, DLLpath, 0, NULL);
	}
}

#pragma argsused
VOID CALLBACK PreventSleepFopProc(HWND hWnd, UINT msg, UINT_PTR id, DWORD time)
{
	FOPSTRUCT *FS;
	UnUsedParam(msg);UnUsedParam(id);UnUsedParam(time);

	FS = (FOPSTRUCT *)GetWindowLongPtr(hWnd, DWLP_USER);
	if ( FS->state != FOP_PAUSE ) DSetThreadExecutionState(ES_SYSTEM_REQUIRED);
}

#pragma argsused
VOID CALLBACK PreventSleepProc(HWND hWnd, UINT msg, UINT_PTR id, DWORD time)
{
	UnUsedParam(hWnd);UnUsedParam(msg);UnUsedParam(id);UnUsedParam(time);

	DSetThreadExecutionState(ES_SYSTEM_REQUIRED);
}

void StartPreventSleep(HWND hWnd, BOOL fop)
{
	if ( DSetThreadExecutionState == INVALID_VALUE(impSetThreadExecutionState) ){
		GETDLLPROC(hKernel32, SetThreadExecutionState);
	}
	if ( DSetThreadExecutionState != NULL ){
		SetTimer(hWnd, TIMERID_PREVENTSLEEP, TIME_PREVENTSLEEP,
				fop ? PreventSleepFopProc : PreventSleepProc);
		PreventSleepProc(hWnd, WM_TIMER, TIMERID_PREVENTSLEEP, 0);
	}
}

void EndPreventSleep(HWND hWnd)
{
	if ( (DSetThreadExecutionState != NULL) &&
		 (DSetThreadExecutionState != INVALID_VALUE(impSetThreadExecutionState)) ){
		KillTimer(hWnd, TIMERID_PREVENTSLEEP);
		DSetThreadExecutionState(ES_CONTINUOUS);
	}
}

void USEFASTCALL SetHideMode(FOPSTRUCT *FS)
{
	if ( (Sm->JobList.hWnd != NULL) &&
		 (GetWindowLongPtr(FS->hDlg, GWL_STYLE) & WS_VISIBLE) ){
		ShowWindow(FS->hDlg, SW_HIDE);
	}
}

ERRORCODE FileOperationSourceHttp(FOPSTRUCT *FS, ODVALS *odv, const TCHAR *fileptr)
{
	TCHAR destnamebuf[VFPS];
	ThSTRUCT th;
	const char *bottom, *body;
	DWORD size;
	HANDLE hFile;
	ERRORCODE result;

	SetTinyProgress(FS);
	FopMessageLoopOrPause(FS);
	VFSFixPath(odv->dstPath, FS->DestDir, FS->opt.source, VFSFIX_VFPS | VFSFIX_REALPATH | VFSFIX_NOFIXEDGE);

	tstplimcpy(destnamebuf, fileptr, VFPS);
	tstrreplace(destnamebuf, T("\\"), T("_"));
	tstrreplace(destnamebuf, T("/"), T("_"));
	tstrreplace(destnamebuf, T("?"), T("%3F"));
	if ( VFSFullPath(odv->dstPath, (TCHAR *)destnamebuf, FS->DestDir) == NULL ){
		return PPERROR_GETLASTERROR;
	}

	GetImageByHttp(odv->srcPath, &th);
	bottom = (char *)th.bottom;
	if ( bottom == NULL ) return ERROR_FILE_NOT_FOUND;
	size = th.top - 1;

	body = strstr(bottom, "\r\n\r\n");
	if ( body != NULL ){
		size -= body - bottom + 4;
		bottom = body + 4;
	}
								// 書き込み ---------------------------
	hFile = CreateFileL(odv->dstPath, GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile != INVALID_HANDLE_VALUE ){
		WriteFile(hFile, bottom, size, &size, NULL);
		CloseHandle(hFile);
		result = NO_ERROR;
		FS->progs.info.donefiles++;
	}else{
		result = GetLastError();
	}
	ThFree(&th);
	return result;
}


ERRORCODE FileOperationSourceFtp(FOPSTRUCT *FS, HINTERNET *hFTP, const TCHAR *srcentry, TCHAR *dst)
{
	ERRORCODE result;

	SetTinyProgress(FS);
	FopMessageLoopOrPause(FS);
	if ( FS->state == FOP_TOBREAK ) return ERROR_CANCELLED;

	while( GetFileAttributesL(dst) != BADATTR ){
		BY_HANDLE_FILE_INFORMATION srcfinfo, dstfinfo;
		HANDLE hTFile;

		memset(&srcfinfo, 0, sizeof(srcfinfo));
		hTFile = CreateFileL(dst, GENERIC_READ, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		result = SameNameAction(FS, hTFile, &srcfinfo, &dstfinfo, srcentry, dst);
		switch( result ){
			case ACTION_RETRY:
				continue;

			case ACTION_APPEND:
			case ACTION_OVERWRITE:
			case ACTION_CREATE:
				break;

			case ACTION_SKIP:
				FS->progs.info.EXskips++;
				FS->progs.info.donefiles++;
				return NO_ERROR;

			case ERROR_CANCELLED:
				return ERROR_CANCELLED;

			default:
				return ERROR_FILE_EXISTS; // error
		}

		if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_BACKUPOLD ){
			BackupFile(FS, dst);
		}else {
			if ( dstfinfo.dwFileAttributes & OPENERRORATTRIBUTES ){
				SetFileAttributesL(dst, FILE_ATTRIBUTE_NORMAL);
			}
			DeleteFileL(dst);
		}
	}
	if ( DFtpGetFile(hFTP[FTPHOST], srcentry, dst, FALSE,
			FILE_ATTRIBUTE_ARCHIVE, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_DONT_CACHE, 0) == FALSE ){
		result = GetInetError();
		if ( result == ERROR_REQ_NOT_ACCEP ){ // ディレクトリの可能性
			TCHAR name[VFPS];
			WIN32_FIND_DATA findfile;
			HINTERNET hFtpFF;
			ThSTRUCT th;

			if ( DFtpSetCurrentDirectory(hFTP[FTPHOST], srcentry) == FALSE ){
				result = GetInetError();
			}else{
				hFtpFF = DFtpFindFirstFile(hFTP[FTPHOST], WildCard_All, &findfile, INTERNET_FLAG_RELOAD, 0);
				if ( hFtpFF == NULL ){
					result = GetInetError();
				}else{
					TCHAR *p;

					// エントリ一覧を作成(DFtpFindFirstFileの重複がダメだから)
					ThInit(&th);
					do {
						ThAddString(&th, findfile.cFileName);
					} while( IsTrue(DInternetFindNextFile(hFtpFF, &findfile)) );
					DInternetCloseHandle(hFtpFF);
					ThAddString(&th, NilStr);

					CreateDirectoryL(dst, NULL);
					p = (TCHAR *)th.bottom;
					while ( *p != '\0' ){
						CatPath(name, dst, p);
						result = FileOperationSourceFtp(FS, hFTP, p, name);
						if ( result != NO_ERROR ) break;
						p += tstrlen(p) + 1;
					};
					if ( DFtpSetCurrentDirectory(hFTP[FTPHOST], T("..")) == FALSE ){
						result = GetInetError();
					}
					ThFree(&th);

					if ( (result == NO_ERROR) && (FS->opt.fop.mode == FOPMODE_MOVE) ){
						DFtpRemoveDirectory(hFTP[FTPHOST], srcentry);
					}
					return result;
				}
			}
		}
		return result;
	}else{
		if ( FS->opt.fop.mode == FOPMODE_MOVE ){
			DFtpDeleteFile(hFTP[FTPHOST], srcentry);
		}
		FS->progs.info.donefiles++;
		FopLog(FS, srcentry, dst, LOG_COPY);
		return NO_ERROR;
	}
}

ERRORCODE FileOperationDestinationFtp(FOPSTRUCT *FS, HINTERNET *hFTP, const TCHAR *src, const TCHAR *dstentry)
{
	ERRORCODE result;
	DWORD attr;

	SetTinyProgress(FS);
	FopMessageLoopOrPause(FS);
	if ( FS->state == FOP_TOBREAK ) return ERROR_CANCELLED;

	attr = GetFileAttributesL(src);
	if ( attr == BADATTR ) return GetLastError();
	if ( attr & FILE_ATTRIBUTE_DIRECTORY ){
		TCHAR path[VFPS];
		WIN32_FIND_DATA findfile;
		HANDLE hFile;

		if ( DFtpCreateDirectory(hFTP[FTPHOST], dstentry) == FALSE ){
			return GetInetError();
		}
		CatPath(path, (TCHAR *)src, WildCard_All);
		hFile = FindFirstFileL(path, &findfile);
		if ( hFile == INVALID_HANDLE_VALUE ) return GetLastError();
		DFtpSetCurrentDirectory(hFTP[FTPHOST], dstentry);
		result = ERROR_PATH_NOT_FOUND;
		do {
			if ( !IsRelativeDirectory(findfile.cFileName) ){
				CatPath(path, (TCHAR *)src, findfile.cFileName);
				result = FileOperationDestinationFtp(FS, hFTP, path, findfile.cFileName);
				if ( result != NO_ERROR ) break;
			}
		}while( IsTrue(FindNextFile(hFile, &findfile)) );
		FindClose(hFile);
		DFtpSetCurrentDirectory(hFTP[FTPHOST], T(".."));
		return result;
	}

	if ( DFtpPutFile(hFTP[FTPHOST], src, dstentry,
			FTP_TRANSFER_TYPE_BINARY, 0) == FALSE ){
		return GetInetError();
	}else{
		if ( FS->opt.fop.mode == FOPMODE_MOVE ) DeleteFileL(src);
		FS->progs.info.donefiles++;
		FopLog(FS, src, dstentry, LOG_COPY);
		return NO_ERROR;
	}
}

TCHAR *GetDriveEnd(TCHAR *p, int mode)
{
	TCHAR *q;

	if ( *p ){
		if ( mode == VFSPT_UNC ){	// UNC
			if ( *p == '\\' ) p++;
			q = FindPathSeparator(p);	// pc name
			if ( q != NULL ){
				p = q;
				q = FindPathSeparator(q + 1);	// share name
				if ( q != NULL ){
					p = q;
				}else{
					p += tstrlen(p);
				}
			}else{
				p += tstrlen(p);
			}
		}
	}
	return p;
}

void CreateFWriteLogWindowMain(FOPSTRUCT *FS)
{
	HWND hWnd, hOkWnd;
	RECT box = {3, 135, 249, 50}, okbox;
	POINT okpos;

	hOkWnd = GetDlgItem(FS->hDlg, IDOK);
	GetWindowRect(hOkWnd, &okbox);
	okpos.x = okbox.right;
	okpos.y = okbox.top;
	ScreenToClient(FS->hDlg, &okpos);

	MapDialogRect(FS->hDlg, &box);
	box.right = okpos.x - box.left;
	box.bottom -= box.left;
	hWnd = FS->log.hWnd = CreateWindow(EditClassName, NilStr,
			WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_LEFT | ES_MULTILINE,
			box.left, box.top, box.right, box.bottom, FS->hDlg,
			CHILDWNDID(IDE_FOP_LOG), DLLhInst, 0);
	PPxRegistExEdit(NULL, hWnd, 0x100000, NULL, 0, 0,
			PPXEDIT_NOWORDBREAK | PPXEDIT_ENABLE_SIZE_CHANGE);
	SendMessage(hWnd, WM_SETFONT, SendMessage(FS->hDlg, WM_GETFONT, 0, 0), TRUE);
	SetWindowY(FS, 0); // 大きさ調整＆表示
	ShowDlgWindow(FS->hDlg, IDB_FOP_LOG, SW_SHOWNOACTIVATE);
	SetFocus(hOkWnd);
}

void CreateFWriteLogWindow(FOPSTRUCT *FS)
{
	if ( FS->log.hWnd != NULL ){
		if ( IsTrue(IsWindow(FS->log.hWnd)) ) return;
		FS->log.hWnd = NULL;
	}
	if ( FS->UI_ThreadID == GetCurrentThreadId() ){
		CreateFWriteLogWindowMain(FS);
	}else{
		SendMessage(FS->hDlg, WM_FOP_COMMAND, WM_FOP_CMD_CREATE_LOGWINDOW, 0);
	}
}

void FWriteLog(FOPSTRUCT *FS, const TCHAR *message)
{
	HWND hWnd = FS->log.hWnd;

	FShowLog(FS);

	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		PrintToConsole(message);
		PrintToConsole(T("\r\n"));
	}

	SendMessage(hWnd, EM_SETSEL, EC_LAST, EC_LAST);
	SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)message);
	SendMessage(hWnd, EM_SCROLL, SB_LINEDOWN, 0);
}

void FShowLog(FOPSTRUCT *FS)
{
	if ( FS->log.cache.top == 0 ) return;

	if ( (FS->opt.hReturnWnd != NULL) &&
		 IsWindow(FS->opt.hReturnWnd) &&
		 (SendMessage(FS->opt.hReturnWnd, WM_PPXCOMMAND, TMAKEWPARAM(K_WINDDOWLOG, PPLOG_FASTLOG), (LPARAM)FS->log.cache.bottom) != 0) ){ // 共用ログ

	}else{ // 共用ログがつかえないので自前で表示
		HWND hWnd = FS->log.hWnd;

		CreateFWriteLogWindow(FS);
		if ( hWnd != NULL ){
			SendMessage(hWnd, EM_SETSEL, EC_LAST, EC_LAST);
			SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)FS->log.cache.bottom);
			SendMessage(hWnd, EM_SCROLL, SB_LINEDOWN, 0);
		}
	}
	FS->log.cache.top = 0;
	FS->log.tick = GetTickCount();
}

void FWriteErrorLogs(FOPSTRUCT *FS, const TCHAR *mes, const TCHAR *type, ERRORCODE error)
{
	TCHAR buf[CMDLINESIZE], errormsg[CMDLINESIZE];

	PPErrorMsg(errormsg, error);
	if ( FS->opt.fop.flags & (VFSFOP_OPTFLAG_LOGWINDOW | VFSFOP_OPTFLAG_LOGGING) ){
		if ( type == NULL ) type = NilStr;
		thprintf(buf, TSIZEOF(buf), T("%s Error\t%s\r\n\t%s\r\n"), type, mes, errormsg);
		FopLog(FS, buf, NULL, LOG_STRING);
		return;
	}
	FS->NoAutoClose = TRUE;
	CreateFWriteLogWindow(FS);
	FWriteLog(FS, mes);
	FWriteLog(FS, T(":\t"));
	FWriteLog(FS, errormsg);
	if ( type != NULL ){
		FWriteLog(FS, T(" - "));
		FWriteLog(FS, type);
	}
	FWriteLog(FS, T("\r\n"));
}

void FWriteLogMsg(FOPSTRUCT *FS, const TCHAR *mes)
{
	if ( FS->opt.fop.flags & (VFSFOP_OPTFLAG_LOGWINDOW | VFSFOP_OPTFLAG_LOGGING) ){
		FopLog(FS, mes, NULL, LOG_STRING);
		return;
	}
//	FS->NoAutoClose = TRUE;
	CreateFWriteLogWindow(FS);
	FWriteLog(FS, mes);
}

void FopLog(FOPSTRUCT *FS, const TCHAR *src, const TCHAR *dst, enum foplogtypes type)
{
	TCHAR buf[VFPS * 3], *srcp, *dstp;

	if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_UNDOLOG ){
		DWORD size;
		int len;

		switch ( type ){
			case LOG_SKIP:
				len = thprintf(buf, TSIZEOF(buf), FOPLOGACTION_SKIP T("\t%s\r\n"), src) - buf;
				break;
			case LOG_DIR:
			case LOG_DIR_DELETE:
				return;
			case LOG_DELETE:
				len = thprintf(buf, TSIZEOF(buf), FOPLOGACTION_DELETE T("\t%s\r\n"), src) - buf;
				break;
			case LOG_MAKEDIR:
				len = thprintf(buf, TSIZEOF(buf), FOPLOGACTION_MAKEDIR T("\t%s\r\n"), src) - buf;
				break;
			case LOG_STRING:
				len = tstpcpy(buf, src) - buf;
				break;
			case LOG_COMPLETED:
				goto undologskip;
			default:
				len = thprintf(buf, TSIZEOF(buf),
						T("%s\t%s\r\n") FOPLOGACTION_DESTARROW
						T("\t%s\r\n"), loghead[type], src, dst) - buf;
				if ( type <= LOG_MOVEDIR ){
					FILE_STAT_DATA fdata;
					if ( IsTrue(GetFileSTAT(dst, &fdata)) ){
						len = thprintf(buf + len, TSIZEOF(buf),
							FOPLOGACTION_FILESTAT T("\tS:%u.%u,W:%u.%u\r\n"),
							fdata.nFileSizeHigh, fdata.nFileSizeLow,
							fdata.ftLastWriteTime.dwHighDateTime,
							fdata.ftLastWriteTime.dwLowDateTime ) - buf;
					}
				}
		}
												// 内容を出力 -----------------
		WriteFile(FS->hUndoLogFile, buf, len * sizeof(TCHAR), &size, NULL);
undologskip: ;
	}

	if ( FS->opt.fop.flags & (VFSFOP_OPTFLAG_LOGWINDOW | VFSFOP_OPTFLAG_LOGGING)){
		if ( (FS->opt.fop.flags & VFSFOP_OPTFLAG_LOGWINDOW) &&
			 (FS->log.cache.bottom == NULL) ){
			ThInit(&FS->log.cache);
			FS->log.tick = 0; // GetTickCount() は大抵 LOGDELAYTIME 以上だから、すぐに表示できない確率は低い。できなくても問題ない
			GetCustData(T("X_flogh"), &X_flogh, sizeof(X_flogh));
		}

		dstp = NilStrNC;
		srcp = FindLastEntryPoint(src);
		if ( dst != NULL ){
			dstp = FindLastEntryPoint(dst);
		}

		switch ( type ){
			case LOG_SKIP:
				if ( X_flogh & HIDE_FLOG_SKIP ) return;
				thprintf(buf, TSIZEOF(buf), FOPLOGACTION_SKIP T("\t%s\r\n"), srcp);
				break;

			case LOG_DELETE:
				thprintf(buf, TSIZEOF(buf), FOPLOGACTION_DELETE T("\t%s\r\n"), srcp);
				break;

			case LOG_COMPLETED:
				if ( X_flogh & HIDE_FLOG_COMPLETED ) return;
			// LOG_STRING へ
			case LOG_STRING:
				tstrcpy(buf, src);
				break;

			case LOG_DIR: {
				TCHAR *bufp;

				if ( X_flogh & HIDE_FLOG_SOURCE ){
					if ( X_flogh & HIDE_FLOG_DESTINATION ) return;
					bufp = buf;
					buf[0] = '\0';
				}else{
					bufp = thprintf(buf, TSIZEOF(buf), FOPLOGACTION_SOURCE T("\t%s\r\n"), src);
				}
				if ( !(X_flogh & HIDE_FLOG_DESTINATION) ){
					thprintf(bufp, VFPS, FOPLOGACTION_DESTINATION T(" %s\r\n"), dst);
				}
				break;
			}

			case LOG_DIR_DELETE:
				thprintf(buf, TSIZEOF(buf), FOPLOGACTION_DELETE T("\t%s\r\n"), src);
				break;

			case LOG_MAKEDIR:
				if ( X_flogh & HIDE_FLOG_MAKR_DIR ) return;
				thprintf(buf, TSIZEOF(buf), FOPLOGACTION_MAKEDIR T("\t%s\r\n"), src);
				break;

			default: {
				TCHAR *bufp;

				if ( (type == LOG_BACKUP) && (X_flogh & HIDE_FLOG_BACKUP) ){
					return;
				}
				bufp = thprintf(buf, TSIZEOF(buf), T("%s\t%s\r\n"), loghead[type], srcp);
				if ( tstrcmp(srcp, dstp) != 0 ){
					thprintf(bufp, VFPS,
							FOPLOGACTION_DESTARROW T("\t%s\r\n"), dstp);
				}
			}
		}

		if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_LOGWINDOW ){
			ThCatString(&FS->log.cache, buf);
			if ( (GetTickCount() - FS->log.tick) > LOGDELAYTIME ){ // overflow は考慮しなくても大丈夫
				FShowLog(FS);
			}
		}
		if ( (FS->opt.fop.flags & VFSFOP_OPTFLAG_LOGGING) &&
			 ( !(FS->opt.fop.flags & VFSFOP_OPTFLAG_LOGWINDOW) ||
			   (FS->opt.hReturnWnd == NULL) ) ){
			XMessage(NULL, NULL,
					(X_log & (1 << XM_INFOLLog)) ? XM_INFOLLog : XM_INFOlog,
					T("%s"), buf);
		}
	}
}

void DisplaySrcNameNow(FOPSTRUCT *FS)
{
	if ( FS->progs.srcpath != NULL ){
		SetWindowText(FS->progs.hSrcNameWnd, FS->progs.srcpath);
		FS->progs.srcpath = NULL;
	}
}

void TinyDisplayProgress(FOPSTRUCT *FS)
{
	TCHAR buf[200];
	DWORD tick = GetTickCount();

	if ( (tick - FS->progs.ProgTick) >= PROGRESS_INTERVAL_TICK ){ // 1file進捗状況更新
		if ( FS->progs.info.filesall == 0 ){
			SetTaskBarButtonProgress(FS->progs.hWnd, TBPF_INDETERMINATE, 0);
		}
		FS->progs.ProgTick = tick;

		thprintf(buf, TSIZEOF(buf), T("\x1b\x64%c%2d/%d marks  %d/%d files"),
				FS->progs.info.busybar,
				FS->progs.info.mark, FS->progs.info.markall,
				FS->progs.info.donefiles, FS->progs.info.filesall);
		SetWindowText(FS->progs.hProgressWnd, buf);
		FS->progs.info.busybar++;
		if ( FS->progs.info.busybar > 10) FS->progs.info.busybar = 1;

		if ( FS->progs.srcpath != NULL ){
			SetWindowText(FS->progs.hSrcNameWnd, FS->progs.srcpath);
			FS->progs.srcpath = NULL;
		}
	}
}

DWORD GetSizeRate(INTHL *nowsize, INTHL *totalsize)
{
#if _INTEGRAL_MAX_BITS >= 64
	if ( totalsize->QuadPart != 0 ){
		if ( totalsize->QuadPart < 0x200000000000000 ){
			// 0 ～ 0x1ff ffff ffff ffff(128P)
			return (DWORD)(unsigned __int64)((nowsize->QuadPart * 100) / totalsize->QuadPart);
		}else{
			// ≧ 0x200 0000 0000 0000(128P)
			return (DWORD)(unsigned __int64)((nowsize->QuadPart / (0x10000 / 100)) / (totalsize->QuadPart / 0x10000) );
		}
	}
	return 100;
#else
	if ( totalsize->u.HighPart || (totalsize->u.LowPart & 0xfe000000) ){
		if ( totalsize->u.HighPart >= 0x200 ){
			if ( totalsize->u.HighPart >= 0x2000000 ){
				// 0x0200 0000 0000(2TB) ～ 0x1ff ffff ffff ffff(128P)
				return (nowsize->u.HighPart * 100) / totalsize->u.HighPart;
			}else{
				// ≧ 0x200 0000 0000 0000(128P)
				return (nowsize->u.HighPart / (0x10000 / 100)) / (totalsize->u.HighPart / 0x10000);
			}
		}	// 0x0200 0000(32M) ～ 0x1ff ffff ffff(2TB)
		return ( ((((DWORD)nowsize->u.HighPart) << 16) | (nowsize->u.LowPart >> 16)) * 100) /
			   ( ((((DWORD)totalsize->u.HighPart) << 16) | (totalsize->u.LowPart >> 16)) );
	}
					// 0x0000 0001      ～  0x01ff ffff(32M)
	if ( totalsize->u.LowPart != 0 ){
		return (nowsize->u.LowPart * 100) / totalsize->u.LowPart;
	}				// 0x0000 0000
	return 100;
#endif
}

/*	Progressが呼び出される単位
	WindowsVista-10 : 4M未満 0.5M  4M以上 1M
	WindowsXP : 64k
*/
void FullDisplayProgress(struct _ProgressWndInfo *Progs)
{
	DWORD tick;
	TCHAR buf[VFPS + 256], pbuf[VFPS + 256];

	if ( (DWORD)Progs->FileSize.u.HighPart == FFD ){
		TinyDisplayProgress(Progs->FS);
		return;
	}

	tick = GetTickCount();
				// over flow 問題は考慮せず
	if ( (tick - Progs->ProgTick) >= PROGRESS_INTERVAL_TICK ){ // 1file進捗状況
		DWORD per;
		TCHAR *dest;

		if ( Progs->info.filesall == 0 ){
			SetTaskBarButtonProgress(Progs->hWnd, TBPF_INDETERMINATE, 0);
		}
		Progs->ProgTick = tick;

		if ( X_fopg ){
			INTHL sizeHL;

			sizeHL = Progs->info.donesize;
			AddHLHL(sizeHL, Progs->FileTransSize);
			per = GetSizeRate(&sizeHL, &Progs->FileSize);
		}else{
			per = GetSizeRate(&Progs->FileTransSize, &Progs->FileSize);
		}

		if ( Progs->info.filesall == 0 ){
			dest = thprintf(buf, 256,
					T("\x1b%c%c\x1d%2d/%d marks  %d files"),
					per + 1,
					Progs->info.busybar,
					Progs->info.mark, Progs->info.markall,
					Progs->info.donefiles);
		}else{
			dest = thprintf(buf, 256,
					T("\x1b%c%c\x1d%2d/%d marks  %d/%d files"),
					per + 1,
					Progs->info.busybar,
					Progs->info.mark, Progs->info.markall,
					Progs->info.donefiles, Progs->info.filesall);
		}
		switch ( X_fopi ){
			case 0: { // 転送済みサイズ
				INTHL sizeHL;

				sizeHL = Progs->info.donesize;
				AddHLHL(sizeHL, Progs->FileTransSize);

				*dest++ = ' ';
				dest = FormatNumber(dest, XFN_DECIMALPOINT | XFN_MINKILO, 7, sizeHL.s.L, sizeHL.s.H);
				tstrcpy(dest, T(" done"));
				break;
			}

			case 1: { // 経過時間,残り時間
				#if _INTEGRAL_MAX_BITS >= 64
					DWORD timetick;
					double remain, timetickF;
					__int64 AllDoneSize;

					timetick = tick - Progs->StartTick;
					if ( (int)timetick < 0 ) timetick = (DWORD)-(int)timetick;

					AllDoneSize = Progs->info.donesize.HL + Progs->FileTransSize.HL;

					if ( (Progs->info.filesall == 0) || (AllDoneSize == 0) ){ // 経過表示のみ
						thprintf(dest, 100, T(" %ds"), timetick / 1000);
					}else{
						timetickF = (double)timetick;
						remain = (timetickF * (double)Progs->info.allsize.HL / (double)AllDoneSize - timetickF + 999.) / 1000.;
						thprintf(dest, 100, T(" %ds,%ds"),
								timetick / 1000, (int)remain);
					}
				#else
					DWORD timetick, remain, totalper;
					INTHL AllDoneSize;

					timetick = tick - Progs->StartTick;
					if ( (int)timetick < 0 ) timetick = (DWORD)-(int)timetick;

					AllDoneSize = Progs->info.donesize;
					AddHLHL(AllDoneSize, Progs->FileTransSize);

					if ( (Progs->info.filesall == 0) ||
						 ((AllDoneSize.s.L | AllDoneSize.s.H) == 0) ){ // 経過表示のみ
						thprintf(dest, 100, T(" %ds"), timetick / 1000);
					}else{
						totalper = GetSizeRate(&AllDoneSize, &Progs->info.allsize);

						timetick /= 1000;
						remain = (((totalper > 0) ? 25600 / totalper : 25600) * timetick + 255) / 256 - (ULONG_PTR)timetick;
						thprintf(dest, 100, T(" %ds,%ds"), timetick, remain);
					}
				#endif
				break;
			}

			case 2: { // 転送レート
				INTHL donesize;
				DWORD timetick;

				donesize = Progs->info.donesize;
				AddHLHL(donesize, Progs->FileTransSize);

				timetick = tick - Progs->StartTick;
				if ( (int)timetick < 0 ) timetick = (DWORD)-(int)timetick;
				#if _INTEGRAL_MAX_BITS >= 64
					if ( donesize.QuadPart < 0x40000000000000 ){
						if ( timetick != 0 ){
							donesize.QuadPart = (donesize.QuadPart * 1000) / (__int64)timetick;
						}
					}else{
						timetick /= 1000;
						if ( timetick != 0 ){
							donesize.QuadPart = donesize.QuadPart / (__int64)timetick;
						}
					}
				#else
					timetick /= 1000;
					if ( timetick != 0 ){
						if ( donesize.s.H == 0 ){
							donesize.s.L = donesize.s.L / timetick;
						}else if ( donesize.s.H < 0x10000 ){
							donesize.s.L = ((donesize.s.H << 16) + (donesize.s.L >> 16)) / timetick;
							donesize.s.H = donesize.s.L >> 16;
							donesize.s.L = donesize.s.L << 16;
						}else{
							donesize.s.H = donesize.s.H / timetick;
							donesize.s.L = 0;
						}
					}
				#endif
				*dest++ = ' ';
				dest = FormatNumber(dest, XFN_DECIMALPOINT | XFN_MINKILO, 7, donesize.s.L, donesize.s.H);
				dest[0] = '/';
				dest[1] = 's';
				dest[2] = '\0';
				break;
			}
		}

		SetWindowText(Progs->hProgressWnd, buf);
		Progs->info.busybar++;
		if ( Progs->info.busybar > 10 ) Progs->info.busybar = 1;

		if ( Progs->srcpath != NULL ){
			SetWindowText(Progs->hSrcNameWnd, Progs->srcpath);
			Progs->srcpath = NULL;
		}
	}
				// over flow 問題は考慮せず
	if ( (tick - Progs->CapTick) >= CAPTION_INTERVAL_TICK ){ // キャプション/全体進捗更新
		const TCHAR *sfmt;
		DWORD totalper;

		Progs->CapTick = tick;
		if ( Progs->info.filesall &&
			 (Progs->info.allsize.s.L | Progs->info.allsize.s.H) ){
			INTHL donesize;

			donesize = Progs->info.donesize;
			AddHLHL(donesize, Progs->FileTransSize);
			totalper = GetSizeRate(&donesize, &Progs->info.allsize);
			sfmt = T("%d%% / %s");
		}else{
			sfmt = T("%d / %s");
			totalper = Progs->info.donefiles;
		}
		if ( Progs->nowper != (int)totalper ){
			TCHAR *path;

			Progs->nowper = (int)totalper;

			pbuf[0] = '\0';
			GetWindowText(Progs->hWnd, pbuf, TSIZEOF(pbuf));
			path = tstrchr(pbuf, '/');
			path = (path != NULL) ? path + 2 : pbuf;

			thprintf(buf, TSIZEOF(buf), sfmt, totalper, path);
			SetWindowText(Progs->hWnd, buf);

			if ( Progs->info.filesall != 0 ){
				SetTaskBarButtonProgress(Progs->hWnd, totalper, 100);
			}
			SetJobTask(Progs->hWnd, JOBSTATE_SETRATE | totalper);
		}
	}
}

void SetFullProgress(struct _ProgressWndInfo *Progs)
{
	if ( Progs->FS->hOperationThread != NULL ){
		PostMessage(Progs->FS->hDlg, WM_FOP_COMMAND, WM_FOP_CMD_PROGRESS, 0);
	}else{
		FullDisplayProgress(Progs);
		FopMessageLoopOrPause(Progs->FS);
	}
}

void SetTinyProgress(FOPSTRUCT *FS)
{
	if ( FS->hOperationThread != NULL ){
		FS->progs.FileSize.u.HighPart = FFD;
		PostMessage(FS->hDlg, WM_FOP_COMMAND, WM_FOP_CMD_PROGRESS, 0);
	}else{
		TinyDisplayProgress(FS);
	}
}

void WaitLoopInOperationThread(FOPSTRUCT *FS)
{
//	XMessage(NULL, NULL, XM_DbgLOG, T("wait start"));
	for (;;){
		DWORD result;
		result = MsgWaitForMultipleObjects(1, &FS->hStateChangeEvent, FALSE,
				INFINITE, QS_ALLEVENTS | QS_SENDMESSAGE);
		if ( result != (WAIT_OBJECT_0 + 1) ) break;
		// Message
		FopPeekMessageLoop();
	}
//	XMessage(NULL, NULL, XM_DbgLOG, T("wait end"));
}

#pragma argsused
DWORD CALLBACK CopyProgress(LARGE_INTEGER TotalSize, LARGE_INTEGER TotalTransSize, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamTransSize, DWORD StreamNumber, DWORD reason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
	UnUsedParam(StreamSize);UnUsedParam(StreamTransSize);UnUsedParam(StreamNumber);UnUsedParam(hSourceFile);UnUsedParam(hDestinationFile);
	#define PWI ((struct _ProgressWndInfo *)lpData)

	if ( reason == CALLBACK_CHUNK_FINISHED ){
		FOPSTRUCT *FS;

		PWI->FileTransSize.LI = TotalTransSize;
		PWI->FileSize.LI = TotalSize;
		FS = PWI->FS;
		if ( FS->hOperationThread != NULL ){
			DWORD tick;

			tick = GetTickCount();
			if ( (tick - PWI->ProgTick) >= PROGRESS_INTERVAL_TICK ){
				PostMessage(FS->hDlg, WM_FOP_COMMAND, WM_FOP_CMD_PROGRESS, 0);
			}
			if ( FS->state == FOP_TOPAUSE ) FS->state = FOP_PAUSE;
			while ( FS->state == FOP_PAUSE ){ // 待機指示中
				WaitLoopInOperationThread(FS);
			}
		}else{
			FullDisplayProgress(PWI);
			FopMessageLoopOrPause(FS);
		}
	}
	#undef PWI
	return PROGRESS_CONTINUE;
}


ERRORCODE TestDest(FOPSTRUCT *FS, const TCHAR *src, TCHAR *dst)
{
	TCHAR buf[VFPS * 2 + 0x100];
	DWORD attr;

	FopMessageLoopOrPause(FS);
	if ( FS->state == FOP_TOBREAK ) return ERROR_CANCELLED;
	if ( FS->opt.fop.mode == FOPMODE_SHORTCUT ){
		attr = (FOPMakeShortCut(src, dst,
				GetFileAttributesL(src) & FILE_ATTRIBUTE_DIRECTORY,
				TRUE) == NO_ERROR) ? BADATTR : 0;
	}else{
		attr = GetFileAttributesL(dst);
	}

	if ( attr != BADATTR ){
		FS->errorcount++;
	}else{
		FS->progs.info.donefiles++;
	}
	thprintf(buf, TSIZEOF(buf),
			T("%s\t%s\r\n") FOPLOGACTION_DESTARROW T("\t%s\r\n"),
			(attr == BADATTR) ? StrLogTest : StrLogExist, src, dst);

	FWriteLogMsg(FS, buf);
	return NO_ERROR;
}

ERRORCODE RenameDestFile(FOPSTRUCT *FS, const TCHAR *dst, BOOL addOldStr)
{
	TCHAR newolddstname[VFPS];
	TCHAR extbuf[VFPS], *extp;

	tstrcpy(newolddstname, dst);
	if ( addOldStr &&
		 (tstrlen(newolddstname) < (VFPS - (TSIZEOFSTR(StrAddOldName) + 2))) ){
		extp = VFSFindLastEntry(newolddstname);
		extp += FindExtSeparator(extp);
		tstrcpy(extbuf, extp);
		tstrcpy(extp, StrAddOldName);
		tstrcpy(extp + TSIZEOFSTR(StrAddOldName), extbuf);
	}

	if ( GetUniqueEntryName(newolddstname) == FALSE ){
		return ERROR_ALREADY_EXISTS;
	}
	if ( IsTrue(MoveFileL(dst, newolddstname)) ){
		FopLog(FS, dst, newolddstname, LOG_BACKUP);
		return NO_ERROR;
	}
	return GetLastError();
}

const TCHAR SameAction_addnumber[] = MES_SNAN;
const TCHAR SameAction_append[] = MES_SNAP;
const TCHAR SameAction_backup[] = MES_SNBK;
const TCHAR SameAction_changename[] = MES_SNCN;
const TCHAR SameAction_copy[] = MES_SNCP;
const TCHAR SameAction_delete[] = MES_SNDE;
const TCHAR SameAction_keep[] = MES_SNKP;
const TCHAR SameAction_link[] = MES_SNLN;
const TCHAR SameAction_move[] = MES_SNMV;
const TCHAR SameAction_overwrite[] = MES_SNOW;
const TCHAR SameAction_restore[] = MES_SNRE;
const TCHAR SameAction_skip[] = MES_SNSK;

const TCHAR *SameAction_Srclist[] = {
	SameAction_move,
	SameAction_copy,
	SameAction_copy,
	SameAction_link,
	SameAction_link,
	SameAction_delete,
	SameAction_restore,
	SameAction_link,
};

void ChangeEntryActionHelp(FOPSTRUCT *FS, BY_HANDLE_FILE_INFORMATION *srcfinfo, BY_HANDLE_FILE_INFORMATION *dstfinfo, DWORD dstattr)
{
	TCHAR buf[VFPS], *lastp;
	const TCHAR *srcmemo = NULL, *dstmemo = NULL;

	switch ( FS->opt.fop.same ){
		case FOPSAME_NEWDATE:
			if ( FuzzyCompareFileTime(&srcfinfo->ftLastWriteTime,
					&dstfinfo->ftLastWriteTime) > 0){
				break;
			}
			// FOPSAME_SKIP へ
		case FOPSAME_SKIP:
			srcmemo = SameAction_skip;
			dstmemo = SameAction_keep;
			break;

		case FOPSAME_ARCHIVE:
			if ( srcfinfo->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ) break;
			srcmemo = SameAction_skip;
			dstmemo = SameAction_keep;
			break;

		case FOPSAME_RENAME:
		case FOPSAME_ADDNUMBER:
			srcmemo = SameAction_changename;
			dstmemo = SameAction_keep;
			break;

		case FOPSAME_APPEND:
			srcmemo = SameAction_append;
			dstmemo = SameAction_append;
			break;

		case FOPSAME_SIZE:
			if ( (srcfinfo->nFileSizeLow != dstfinfo->nFileSizeLow) ||
				 (srcfinfo->nFileSizeHigh != dstfinfo->nFileSizeHigh) ){
				break;
			}
			srcmemo = SameAction_skip;
			dstmemo = SameAction_keep;
			break;

							// これらは常に上書き
//		case FOPSAME_OVERWRITE:
//		default:
//			break;
	}
	if ( srcmemo == NULL ){
		srcmemo = SameAction_Srclist[FS->opt.fop.mode];
	}
	// コピー元の更新
	GetDlgItemText(FS->hDlg, IDS_FOP_SRCTITLE, buf, VFPS);
	lastp = tstrrchr(buf , '(');
	if ( lastp != NULL ) *lastp = '\0';
	tstrcat(buf, MessageText(srcmemo));
	SetDlgItemText(FS->hDlg, IDS_FOP_SRCTITLE, buf);

	if ( dstmemo == NULL ){
		if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_ADDNUMDEST ){
			dstmemo = SameAction_addnumber;
		}else if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_BACKUPOLD ){
			dstmemo = SameAction_backup;
		}else{
			dstmemo = SameAction_overwrite;
		}
	}
	// コピー先の更新
	GetDlgItemText(FS->hDlg, IDS_FOP_DESTTITLE, buf, VFPS);
	lastp = tstrrchr(buf , '(');
	if ( lastp != NULL ) *lastp = '\0';
	tstrcat(buf, MessageText(dstmemo));
	if ( (dstattr & FILE_ATTRIBUTE_REPARSE_POINT) || (dstfinfo->nNumberOfLinks >= 2) ){
		tstrcat(buf, MessageText(T("[link]")));
	}

	SetDlgItemText(FS->hDlg, IDS_FOP_DESTTITLE, buf);
}

const TCHAR *SameActionList[] = {
	MES_7022, MES_7023, MES_7024, MES_7025,
	MES_7026, MES_7027, MES_7028, MES_7064, NULL
};

void FopConsoleSameAction(FOPSTRUCT *FS, const TCHAR *src, const TCHAR *dst)
{
	HMENU hPopup;
	const TCHAR **list = SameActionList;
	int index = 1;
	TCHAR buf[CMDLINESIZE];

	thprintf(buf, TSIZEOF(buf), T("Source:      %s\r\n"), src);
	PrintToConsole(buf);
	thprintf(buf, TSIZEOF(buf), T("Destination: %s\r\n"), dst);
	PrintToConsole(buf);
	thprintf(buf, TSIZEOF(buf), T("%s\r\n"), MessageText(MES_QSAO));
	PrintToConsole(buf);

	hPopup = CreatePopupMenu();
	for ( ; *list != NULL; list++ ){
		AppendMenuString(hPopup, index++, *list);
	}
	index = TrackPopupMenuConsole(hPopup);
	DestroyMenu(hPopup);
	if ( index <= 0 ){
		FopChangeStateAndNotify(FS, FOP_TOBREAK);
	}else{
		FS->opt.fop.same = index - 1;
		FopChangeStateAndNotify(FS, FOP_BUSY);
	}
	PostMessage(FS->hDlg, WM_NULL, 0, 0);
}

ERRORCODE SameNameAction(FOPSTRUCT *FS, HANDLE dstH, BY_HANDLE_FILE_INFORMATION *srcfinfo, BY_HANDLE_FILE_INFORMATION *dstfinfo, const TCHAR *src, TCHAR *dst)
{
	BOOL samefile = FALSE;
	HANDLE hDlg = FS->hDlg;
	struct FopOption *opt = &FS->opt;
	ERRORCODE result;	// 負の時はスキップ
	TCHAR pathbuf[VFPS];
	DWORD dstattr = BADATTR;

										// 処理先情報の取得 -------------------
	if ( dstH != NULL ){ // 処理先ファイル有り／存在しない
		if ( IsTrue(GetFileInformationByHandle(dstH, dstfinfo)) ){
			dstattr = dstfinfo->dwFileAttributes;
		}
		CloseHandle(dstH);
	}
	if ( dstattr == BADATTR ){
		memset(dstfinfo, 0, sizeof(BY_HANDLE_FILE_INFORMATION));
		// FILE_STAT_DATA と BY_HANDLE_FILE_INFORMATION の共通部分(dwFileAttributes～nFileSizeLow)を利用して読み込み
		if ( IsTrue(GetFileSTAT(dst, (FILE_STAT_DATA *)dstfinfo)) ){
				// 構造体の補正
			dstfinfo->nFileSizeLow = ((FILE_STAT_DATA *)dstfinfo)->nFileSizeLow;
			dstfinfo->nFileSizeHigh = ((FILE_STAT_DATA *)dstfinfo)->nFileSizeHigh;
		}else{
			dstattr = 0;
//			dstfinfo->dwFileAttributes = 0;
			dstfinfo->ftCreationTime.dwHighDateTime =
					dstfinfo->ftLastAccessTime.dwHighDateTime =
					dstfinfo->ftLastWriteTime.dwHighDateTime = 0x24c800cb;
			dstfinfo->nFileSizeHigh = 0x6fffffff;
		}
	}
										// 同一ファイルなら名前変更に切替 -----
	if ( (srcfinfo->dwVolumeSerialNumber == dstfinfo->dwVolumeSerialNumber) &&
		 (srcfinfo->nFileIndexHigh == dstfinfo->nFileIndexHigh) &&
		 (srcfinfo->nFileIndexLow  == dstfinfo->nFileIndexLow) &&
		 // ↓ 別PCの同名パスのときに上記条件を満たしてしまうための対策
		 ((src[0] != '\\') || (dst[0] != '\\') || !tstricmp(src, dst)) ){
		// ファイルカウント時は何もしない
		if ( FS->progs.info.count_exists >= 2 ) return NO_ERROR;

		samefile = TRUE;

		if ( (opt->fop.same != FOPSAME_RENAME) &&
			 (opt->fop.same != FOPSAME_SKIP) &&
			 (opt->fop.same != FOPSAME_ADDNUMBER) ){
			opt->fop.same = FOPSAME_RENAME;
			opt->fop.sameSW = 0; // 始めの設定と異なるので再選択させる
			CheckRadioButton(hDlg, IDR_FOP_NEWDATE, IDR_FOP_ARC, IDR_FOP_RENAME);
		}
	}

	for ( ; ; ){
		if ( opt->fop.sameSW == 0 ){		// 対処方法選択UI -----------------
			int now_same = -1;
			DWORD now_flags = 0;

			if ( !(GetWindowLongPtr(hDlg, GWL_STYLE) & WS_VISIBLE) ){
				ShowWindow(hDlg, SW_SHOWNORMAL);
			}

			SetTaskBarButtonProgress(hDlg, TBPF_PAUSED, 0);
			SetJobTask(FS->hDlg, JOBSTATE_PAUSE);
			FS->state = FOP_PAUSE;			// 準備
			FS->Command = 0;

			if ( (FS->hOperationThread != NULL) && (FS->UI_ThreadID != GetCurrentThreadId()) ){
				if ( FS->page.showID != FOPTAB_GENERAL ){
					// コントロールの削除・作成で問題が起きるので UI thread で。
					SendMessage(hDlg, WM_COMMAND, TMAKEWPARAM(IDB_FOP_GENERAL, BN_CLICKED), 0);
				}
			}else{
				SetFopTab(FS, FOPTAB_GENERAL);
			}
			FopUiEnable(FS, samefile ? FUE_ENABLE_SAMEFILE : FUE_ENABLE_SAMEACTION);
			SetDlgItemText(hDlg, IDOK, MessageText(STR_FOPCONTINUE));
			SetDlgItemText(hDlg, IDB_TEST, MessageText(MES_FOFC));
			DisplaySrcNameNow(FS); // 処理ファイル名が未表示なら表示
			FShowLog(FS); // 未表示のログを表示

			DispAttr(hDlg, IDS_FOP_SRCINFO, srcfinfo, dstfinfo);
			DispAttr(hDlg, IDS_FOP_DESTINFO, dstfinfo, srcfinfo);

			if ( opt->fop.aall != 0 ){
				opt->fop.sameSW = 1;
				CheckDlgButton(hDlg, IDX_FOP_SAME, TRUE);
			}

			if ( FS->progs.info.count_exists >= 2 ){ // 存在一覧を表示
				TCHAR *pbufp;

				pbufp = thprintf(pathbuf, TSIZEOF(pathbuf), MessageText(MES_QSAN), FS->progs.info.count_exists);
				switch ( opt->fop.same ){
					case FOPSAME_NEWDATE:
						thprintf(pbufp, 200, T("%d %s, %d %s"),
								FS->progs.info.exists[2],
								MessageText(MES_LGOW),
								FS->progs.info.filesall - FS->progs.info.count_exists,
								MessageText(MES_JMCP));
						break;
					case FOPSAME_RENAME:
						thprintf(pbufp, 200, T("%d %s, %d %s"),
								FS->progs.info.count_exists,
								MessageText(MES_TREN),
								FS->progs.info.filesall - FS->progs.info.count_exists,
								MessageText(MES_JMCP));
						break;
					case FOPSAME_SKIP:
						thprintf(pbufp, 200, T("%d %s"),
								FS->progs.info.filesall - FS->progs.info.count_exists,
								MessageText(MES_JMCP));
						break;
					case FOPSAME_ADDNUMBER:
						thprintf(pbufp, 200, T("%d number copy"), FS->progs.info.count_exists);
						break;
				}

				SetWindowText(FS->progs.hProgressWnd, pathbuf);
				SetWindowY(FS, 0);
			}else{ // １ファイルの詳細比較を表示
				SetWindowText(FS->progs.hProgressWnd, MessageText(MES_QSAO));
				SetWindowY(FS, IDD_FOP_Y_FULL);
			}

			if ( FS->UI_ThreadID == GetCurrentThreadId() ){
				SetDlgFocus(hDlg, opt->fop.same + IDR_FOP_NEWDATE);
				ActionInfo(hDlg, FS->info, AJI_SELECT, T("fopexist")); // 通知
			}else{
				PostMessage(FS->hDlg, WM_FOP_COMMAND, WM_FOP_CMD_START_SELECT_SAME, 0);
			}
										// 選択
			for ( ; ; ){
				if ( (now_same != opt->fop.same) || (FS->opt.fop.flags != now_flags) ){
					now_same = opt->fop.same;
					now_flags = FS->opt.fop.flags;
					ChangeEntryActionHelp(FS, srcfinfo, dstfinfo, dstattr);
				}

				if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
					FopConsoleSameAction(FS, src, dst);
				}

				if ( (FS->hOperationThread != NULL) && (FS->UI_ThreadID != GetCurrentThreadId()) ){
					WaitLoopInOperationThread(FS);
				}else{
					MSG msg;

					if ( (int)GetMessage(&msg, NULL, 0, 0) <= 0 ){
						FS->state = FOP_TOBREAK;
						break;
					}
					if ( DialogKeyProc(&msg) == FALSE ){
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}

				if ( FS->Command != 0 ){
					if ( FS->Command == IDB_TEST ){
						FopCompareFile(hDlg, src, dst);
					}else{
						FopViewMenu(hDlg, (FS->Command == IDS_FOP_SRCINFO) ? src : dst);
					}
					FS->Command = 0;
				}
				if ( (FS->state == FOP_BUSY) || (FS->state == FOP_TOBREAK) ){
					break;
				}
			}
											// 実行状態に戻す
			SetWindowY(FS, IDD_FOP_Y_STAT);
			SetTaskBarButtonProgress(hDlg, TBPF_NORMAL, 0);
			SetJobTask(FS->hDlg, JOBSTATE_UNPAUSE);
			if ( FS->hide ) SetHideMode(FS);
			if ( FS->state == FOP_TOBREAK ) return ERROR_CANCELLED;

			if ( FS->progs.info.count_exists >= 2 ){ // 存在一覧のときは、この場で処理しない
				return NO_ERROR;
			}
		}else if ( FS->progs.srcpath != NULL ){
			DWORD tick = GetTickCount();

			if ( (tick - FS->progs.ProgTick) >= PROGRESS_INTERVAL_TICK ){ // 1file進捗状況更新
				FS->progs.ProgTick = tick;
				DisplaySrcNameNow(FS); // 処理ファイル名が未表示なら表示
				FShowLog(FS); // 未表示のログを表示
			}
		}
									// 対処する -----------------------
		switch ( opt->fop.same ){
			case FOPSAME_NEWDATE:
				result = (FuzzyCompareFileTime(&srcfinfo->ftLastWriteTime,
						&dstfinfo->ftLastWriteTime) > 0) ?
						ACTION_OVERWRITE : ACTION_SKIP;
				break;

			case FOPSAME_RENAME: {
				TCHAR *fname;
				TINPUT tinput;
				FILENAMEINFOSTRUCT finfo;

				tstrcpy(pathbuf, dst);
				fname = FindLastEntryPoint(pathbuf);

				finfo.info.Function = (PPXAPPINFOFUNCTION)FilenameInfoFunc;
				finfo.info.Name = MES_TENN;
				finfo.info.RegID = NilStr;
				finfo.filename = dst;

				tinput.hOwnerWnd = hDlg;
				tinput.hWtype	= PPXH_FILENAME;
				tinput.hRtype	= PPXH_NAME_R;
				tinput.title	= MES_TENN;
				tinput.buff		= fname;
				tinput.flag		= TIEX_USEREFLINE | TIEX_SINGLEREF | TIEX_REFEXT | TIEX_USEINFO | TIEX_FIXFORPATH;
				tinput.size		= (int)(VFPS - (fname - pathbuf));
				tinput.info		= &finfo.info;
				if ( tinput.size > MAX_PATH ) tinput.size = MAX_PATH;

				if ( (srcfinfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
					!GetCustDword(T("XC_sdir"), 0) ){
					tinput.flag = (tinput.flag & ~TIEX_REFEXT) | TIEX_USESELECT;
					tinput.firstC = 0;
					tinput.lastC = EC_LAST;
				}
				if ( tInputEx(&tinput) <= 0 ){
					opt->fop.sameSW = 0; // キャンセルされたので再選択させる
					continue;
				}
				VFSFixPath(NULL, fname, NULL, 0);
				if ( opt->fop.flags & VFSFOP_OPTFLAG_ADDNUMDEST ){
					if ( MoveFileL(dst, pathbuf) == FALSE ) continue;
					FopLog(FS, dst, pathbuf, LOG_BACKUP);
				}else{
					tstrcpy(dst, pathbuf);
				}
				return ACTION_RETRY;
			}
			case FOPSAME_OVERWRITE:
				result = ACTION_OVERWRITE;
				break;

			case FOPSAME_SKIP:
				return ACTION_SKIP;

			case FOPSAME_ARCHIVE:
				result = (srcfinfo->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) ?
						ACTION_OVERWRITE : ACTION_SKIP;
				break;

			case FOPSAME_APPEND:
				result = ACTION_APPEND;
				break;

			case FOPSAME_SIZE:
				result = ((srcfinfo->nFileSizeLow != dstfinfo->nFileSizeLow) ||
						(srcfinfo->nFileSizeHigh != dstfinfo->nFileSizeHigh)) ?
						ACTION_OVERWRITE : ACTION_SKIP;
				break;

//			case FOPSAME_ADDNUMBER:
			default:
				if ( opt->fop.flags & VFSFOP_OPTFLAG_ADDNUMDEST ){
					ERRORCODE error = RenameDestFile(FS, dst, FALSE);
					if ( error != NO_ERROR ) return error;
				}else{
					if ( GetUniqueEntryName(dst) == FALSE ){
						return ERROR_ALREADY_EXISTS;
					}
				}
				return ACTION_CREATE;
		}
		break;
	}
	if ( result == ACTION_SKIP ) return result;
	// ここに来るのは、ACTION_OVERWRITE か ACTION_APPEND のみになる
	if ( opt->fop.flags & VFSFOP_OPTFLAG_ADDNUMDEST ){
		ERRORCODE error = RenameDestFile(FS, dst, TRUE);
		if ( error != NO_ERROR ) return error;
	}else{
		if ( opt->fop.flags & VFSFOP_OPTFLAG_BACKUPOLD ){
			ERRORCODE error = BackupFile(FS, dst);
			if ( error != NO_ERROR ) return error;
		}

		// 上書きするファイルが書き込み禁止等ならそれを解除
		if ( dstattr & OPENERRORATTRIBUTES ){
			SetFileAttributesL(dst, FILE_ATTRIBUTE_NORMAL);
		}
		// 上書きするファイルがシンボリックリンク・ハードリンクなら解除
		if ( (result == ACTION_OVERWRITE) &&
			 ((dstattr & FILE_ATTRIBUTE_REPARSE_POINT) ||
			  (dstfinfo->nNumberOfLinks >= 2)) ){
			if ( DeleteFileL(dst) == FALSE ){
				return GetLastError();
			}
		}
	}
	return result;
}

typedef struct {
	TCHAR SrcPath[VFPS], DestPath[VFPS];
	DWORD CountTick, LogDrawTick;
} COUNTWORKSTRUCT;

#define CountSrcFF  CountFF[0]
#define CountDestStat CountFF[1]

const TCHAR *StrExist[4] = {MES_EXNW, MES_EXEX, MES_EXOL, MES_EXNF};

ERRORCODE ShowExistsList(FOPSTRUCT *FS, COUNTWORKSTRUCT *cws)
{
	HANDLE hSrcFile, hDestFile;
	BY_HANDLE_FILE_INFORMATION srcfinfo, destfinfo;
	ERRORCODE result;
	TCHAR buf[200];

	thprintf(buf, TSIZEOF(buf), T("[%s:%d][%s:%d][%s:%d]%s:%d\r\n"),
		MessageText(StrExist[2]), FS->progs.info.exists[2],
		MessageText(StrExist[1]), FS->progs.info.exists[1],
		MessageText(StrExist[0]), FS->progs.info.exists[0],
		MessageText(StrExist[3]), FS->progs.info.filesall - FS->progs.info.count_exists
	);

	FShowLog(FS);
	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		PrintToConsole(buf);
	}

	SendMessage(FS->log.hWnd, EM_SETSEL, 0, 0);
	SendMessage(FS->log.hWnd, EM_REPLACESEL, 0, (LPARAM)buf);
	SendMessage(FS->log.hWnd, WM_VSCROLL, SB_TOP, 0);

	hSrcFile = CreateFile_OpenSource(cws->SrcPath, 0);
	if ( hSrcFile != INVALID_HANDLE_VALUE ){
		if ( GetFileInformationByHandle(hSrcFile, &srcfinfo) == FALSE ){
			memset(&srcfinfo, 0, sizeof(srcfinfo));
		}
		CloseHandle(hSrcFile);
	}
	hDestFile = CreateFile_OpenSource(cws->DestPath, 0);
	result = SameNameAction(FS, hDestFile, &srcfinfo, &destfinfo, cws->SrcPath, cws->DestPath);
	FS->progs.info.count_exists = 0;
	return result;
}

void Count_Exists(FOPSTRUCT *FS, COUNTWORKSTRUCT *cws, WIN32_FIND_DATA *CountFF, const TCHAR *filename, const TCHAR *SrcPath, const TCHAR *DestPath)
{
	TCHAR buf[VFPS + 64];
	DWORD tick;
	int compare;

	CreateFWriteLogWindow(FS);
	compare = FuzzyCompareFileTime(&CountSrcFF.ftLastWriteTime, &CountDestStat.ftLastWriteTime) + 1;
	FS->progs.info.exists[compare]++;
	thprintf(buf, TSIZEOF(buf), T("%s\t%s\r\n"), MessageText(StrExist[compare]), filename);
	FWriteLog(FS, buf);

	tick = GetTickCount();
	if ( FS->progs.info.count_exists++ == 0 ){
		cws->LogDrawTick = tick;

		tstrcpy(cws->SrcPath, SrcPath);
		tstrcpy(cws->DestPath, DestPath);

		SendMessage(FS->log.hWnd, WM_SETREDRAW, FALSE, 0);
	}else if ( (tick - cws->LogDrawTick) > 100 ){
		cws->LogDrawTick = tick;
		SendMessage(FS->log.hWnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(FS->log.hWnd, NULL, TRUE);
		UpdateWindow(FS->log.hWnd);
		SendMessage(FS->log.hWnd, WM_SETREDRAW, FALSE, 0);
	}
}

void CountSub(FOPSTRUCT *FS, COUNTWORKSTRUCT *cws, TCHAR *dir, TCHAR *dest)
{
	TCHAR *dirsep, *destlast;
	HANDLE hFF; // FindFile 用ハンドル
	WIN32_FIND_DATA CountFF[2]; // ファイル情報

	dirsep = dir + tstrlen(dir);
	if ( (dirsep - dir + 5) >= VFPS ) return; // OVER_VFPS_MSG

	if ( *dest != '\0' ){
		CatPath(NULL, dest, NilStr);
		destlast = dest + tstrlen(dest);
	}else{
		destlast = dest;
	}

	CatPath(NULL, dir, WildCard_All);
	hFF = VFSFindFirst(dir, &CountSrcFF);
	if ( hFF != INVALID_HANDLE_VALUE ){
		do{
			if ( IsRelativeDirectory(CountSrcFF.cFileName) ) continue;
			FopPeekMessageLoop();
											// 省略希望 || 長時間 のため省略
									// ※ over flow でいきなり省略が起きるが、
									//    対策するほどではないでしょう(^^;
			if ( (FS->state != FOP_COUNT) || (cws->CountTick < GetTickCount()) ){
				FS->progs.info.filesall = 0;
				FS->progs.info.count_exists = 0;
				break;
			}
			if ( dest[0] != '\0' ){
				tstplimcpy(destlast, CountSrcFF.cFileName, VFPS - (destlast - dest));
			}
			if ( CountSrcFF.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				*dirsep = '\0';
				if ( (dirsep - dir + tstrlen(CountSrcFF.cFileName) + 5) < VFPS ){
					CatPath(NULL, dir, CountSrcFF.cFileName);
					CountSub(FS, cws, dir, dest);
				}
			}else{
				AddHLFilesize(FS->progs.info.allsize, CountSrcFF);
				FS->progs.info.filesall++;
				if ( dest[0] != '\0' ){
					if ( IsTrue(GetFileSTAT(dest, &CountDestStat)) ){
						*dirsep = '\0';
						CatPath(NULL, dir, CountSrcFF.cFileName);
						Count_Exists(FS, cws, CountFF, dest, dir, dest);
					}
				}
			}
		}while( IsTrue(VFSFindNext(hFF, &CountSrcFF)) );
		VFSFindClose(hFF);
	}
	return;
}

BOOL CountMain(FOPSTRUCT *FS, const TCHAR *current, const TCHAR *destdir)
{
	const TCHAR *p;
	TCHAR src[VFPS], dest[VFPS], *destlast;
	int X_cntt;
	COUNTWORKSTRUCT cws;

	X_cntt = 10;
	GetCustData(T("X_cntt"), &X_cntt, sizeof X_cntt);
	if ( X_cntt <= 0 ) return TRUE;

	if ( (FS->opt.SrcDtype != VFSDT_PATH) &&
		 (FS->opt.SrcDtype != VFSDT_DLIST) &&
		 (FS->opt.SrcDtype != VFSDT_SHN) &&
		 (FS->opt.SrcDtype != VFSDT_FATIMG) &&
		 (FS->opt.SrcDtype != VFSDT_CDIMG) &&
		 (FS->opt.SrcDtype != VFSDT_STREAM) ){
		return TRUE;
	}

	if ( !(FS->opt.fop.flags & VFSFOP_OPTFLAG_NOFIRSTEXIST) &&
		 (FS->opt.SrcDtype == VFSDT_PATH) &&
		 (destdir != NULL) &&
		 !FS->opt.fop.sameSW ){
		CatPath(dest, (TCHAR *)destdir, NilStr);
		destlast = dest + tstrlen(dest);
	}else{
		dest[0] = '\0';
	}

	cws.CountTick = GetTickCount() + X_cntt * 1000;
	FS->state = FOP_COUNT;

	SetDlgItemText(FS->hDlg, IDOK, MessageText(STR_FOPSKIP));
	SetWindowText(FS->progs.hProgressWnd, MessageText(MES_IFCN));
	for ( p = FS->opt.files ; *p ; p = p + tstrlen(p) + 1 ){
		WIN32_FIND_DATA CountFF[2]; // ファイル情報
		HANDLE hFF;

		if ( IsParentDirectory(p) ) continue;
		if ( FS->opt.SrcDtype == VFSDT_SHN ){
			tstrcpy(src, p);
		}else{
			if ( VFSFullPath(src, (TCHAR *)p, current) == NULL ){
				continue;
			}
		}

		FopPeekMessageLoop();
					// 省略希望 || 長時間 のため省略
						// ※ over flow でいきなり省略が起きるが、
						//    対策するほどではないでしょう(^^;
		if ( (FS->state != FOP_COUNT) || (cws.CountTick < GetTickCount()) ){
			FS->progs.info.filesall = 0;
			FS->progs.info.count_exists = 0;
			break;
		}

		hFF = VFSFindFirst(src, &CountSrcFF);
		if ( hFF != INVALID_HANDLE_VALUE ){
			VFSFindClose(hFF);
			if ( dest[0] != '\0' ) tstrcpy(destlast, CountSrcFF.cFileName);
			if ( CountSrcFF.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				CountSub(FS, &cws, src, dest);
			}else{
				AddHLFilesize(FS->progs.info.allsize, CountSrcFF);
				FS->progs.info.filesall++;
				if ( dest[0] != '\0' ){
					if ( IsTrue(GetFileSTAT(dest, &CountDestStat)) ){
						Count_Exists(FS, &cws, CountFF, CountSrcFF.cFileName, src, dest);
					}
				}
			}
		}
	}
	if ( FS->state == FOP_TOBREAK ) return FALSE;

	if ( FS->progs.info.count_exists != 0 ){
		SendMessage(FS->log.hWnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(FS->log.hWnd, NULL, TRUE);
		if ( FS->progs.info.count_exists >= 2 ){
			if ( ShowExistsList(FS, &cws) != NO_ERROR ) return FALSE;
		}
	}

	InvalidateRect(FS->progs.hProgressWnd, NULL, TRUE);
	SetWindowText(FS->progs.hProgressWnd, NilStr);
	return TRUE;
}

void ReleaseStartMutex(FOPSTRUCT *FS)
{
	if ( ProcessRunID == FS->RunID ) ProcessRunID = 0;
	if ( FS->hStartMutex == NULL ) return;
	ReleaseMutex(FS->hStartMutex);
	CloseHandle(FS->hStartMutex);
	FS->hStartMutex = NULL;
}

void USEFASTCALL OperationStart_End(FOPSTRUCT *FS)
{
	if ( FS->ifo != NULL ) FreeIfo(FS);
	DisplaySrcNameNow(FS); // 処理ファイル名が未表示なら表示
	FShowLog(FS); // 未表示のログを表示
	if ( FS->hUndoLogFile != NULL ) CloseHandle(FS->hUndoLogFile);

	// ●OperationResultまで移動する？
	if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_LOGWINDOW ){
		if ( (FS->opt.hReturnWnd != NULL) && IsWindow(FS->opt.hReturnWnd) ){
			SendMessage(FS->opt.hReturnWnd, WM_PPXCOMMAND, TMAKEWPARAM(K_WINDDOWLOG, PPLOG_SHOWLOG), 0);
		}
	}
	// ●OperationResultまで移動する？
	SetJobTask(FS->hDlg, JOBSTATE_ENDJOB);
}

int Fop_ShellNameSpace(FOPSTRUCT *FS, const TCHAR *srcDIR, const TCHAR *dstDIR)
{
	LPSHELLFOLDER pParentFolder = NULL;
	LPITEMIDLIST *PIDLs = NULL;
	IDataObject *DataObject;
	IDropTarget *DropTarget = NULL;
	int count C4701CHECK;
	int result = Operation_END_ERROR;
	HRESULT ComInitResult;

	ComInitResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if ( !((FS->opt.fop.mode == FOPMODE_COPY) ||
		   (FS->opt.fop.mode == FOPMODE_MOVE)) ){
		XMessage(FS->hDlg, STR_FOP, XM_FaERRd, T("No support this mode"));
		goto fin;
	}
	DropTarget = (IDropTarget *)GetPathInterface(FS->hDlg, dstDIR, &IID_IDropTarget, NULL);
	if ( DropTarget == NULL ){
		XMessage(FS->hDlg, STR_FOP, XM_FaERRd, T("Destination bind error"));
		goto fin;
	}
	count = MakePIDLTableFromFileZ(srcDIR, FS->opt.files, &PIDLs, &pParentFolder);
	if ( count == 0 ) goto fin;

	if ( FAILED(pParentFolder->lpVtbl->GetUIObjectOf(pParentFolder, NULL, count, (LPCITEMIDLIST *)PIDLs, &IID_IDataObject, NULL, (void **)&DataObject) )){
		XMessage(FS->hDlg, STR_FOP, XM_FaERRd, T("Source bind error"));
		goto fin;
	}
	result = CopyToDropTarget(DataObject, DropTarget, FALSE, FS->hDlg,
			(FS->opt.fop.mode == FOPMODE_COPY) ?
				DROPEFFECT_COPY : DROPEFFECT_MOVE);
	{ // DataObject の使用が終わるまで待機する(MTPがバッググラウンドで使用する)
		ULONG ref;
		TCHAR progs[] = T("\x1b\x64\x1");

		for ( ;; ){
			DataObject->lpVtbl->AddRef(DataObject);
			ref = DataObject->lpVtbl->Release(DataObject); // 使用が終わっていたら、直前の AddRef 分だけ(1)になるはず
			if ( ref <= 1 ) break;
			Sleep(100);
			FopPeekMessageLoop();
			if ( FS->state == FOP_TOBREAK ) break;

			SetWindowText(FS->progs.hProgressWnd, progs);
			progs[2] += (TCHAR)1;
			if ( progs[2] > '\xa' ) progs[2] = '\x1';
		}
		if ( ref > 0 ) DataObject->lpVtbl->Release(DataObject);
	}


	if ( result == Operation_END_ERROR ){
		XMessage(FS->hDlg, STR_FOP, XM_FaERRd, T("Operation fault"));
	}else{
		FS->progs.info.donefiles = MAX32;
	}
	FS->DestroyWait = TRUE; // メディアプレイヤーへ処理を行うとき、メッセージポンプをしばらく動かす対策を有効にする
fin:
	if ( pParentFolder != NULL ) pParentFolder->lpVtbl->Release(pParentFolder);
	if ( PIDLs != NULL ){
		FreePIDLS(PIDLs, count); // C4701ok
		HeapFree(ProcHeap, 0, PIDLs);
	}
	if ( DropTarget != NULL ) DropTarget->lpVtbl->Release(DropTarget);
	if ( SUCCEEDED(ComInitResult) ) CoUninitialize();

	SetJobTask(FS->hDlg, JOBSTATE_ENDJOB);
	ReleaseStartMutex(FS);
	OperationStart_End(FS);

/*	処理完了後に通知を送りたいが、処理完了にかなり遅延があるのでこれだとむりぽい
	if ( FS->info != NULL ){
		TCHAR path[VFPS];
		PPXCMDENUMSTRUCT IInfo;

		if ( PPxEnumInfoFunc(FS->info, '2', path, &IInfo) ){
			if ( tstrcmp(path, dstDIR) == 0 ){
				PostMessage(
			}
		}
	}
*/
	return result;
}


void JobErrorBox(FOPSTRUCT *FS, const TCHAR *msg, ERRORCODE err)
{
	HWND hWnd = FS->hDlg;

	DisplaySrcNameNow(FS); // 処理ファイル名が未表示なら表示
	FShowLog(FS);
	SetTaskBarButtonProgress(hWnd, TBPF_ERROR, 0);
	SetJobTask(hWnd, JOBSTATE_ERROR);
	PPErrorBox(FopGetMsgParentWnd(FS), msg, err);
	SetJobTask(hWnd, JOBSTATE_DEERROR);
}

BOOL InitUndoLog(FOPSTRUCT *FS)
{
	HWND hWaitDlg = NULL;
	WAITDLGSTRUCT wds;
	TCHAR buf[VFPS];

	wds.choose = WDS_UNCHOOSE;
	GetUndoLogFileName(buf);
	for ( ; ; ){
		ERRORCODE result;

		FS->hUndoLogFile = CreateFileL(buf, GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if ( FS->hUndoLogFile != INVALID_HANDLE_VALUE ) break;
		FS->hUndoLogFile = NULL;
		result = GetLastError();

		if ( result != ERROR_SHARING_VIOLATION ){
			ErrorPathBox(FS->hDlg, STR_FOPUNDOLOG, buf, result);
			ReleaseStartMutex(FS);
			return FALSE;
		}

		if ( hWaitDlg == NULL ){
			wds.md.title = T("Fild operation(undo)");
			hWaitDlg = CreateDialogParam(DLLhInst, MAKEINTRESOURCE(IDD_NULL), NULL, WaitDlgBox, (LPARAM)&wds);
			ShowWindow(hWaitDlg, SW_SHOWNOACTIVATE);
		}
		if ( IsChooseWDS(wds.choose) ) break;

		PeekDialogMessageLoop(NULL, hWaitDlg);
		Sleep(100);
	}
	if ( hWaitDlg != NULL ) DestroyWindow(hWaitDlg);
	if ( wds.choose == WDS_CANCEL ){
		ReleaseStartMutex(FS);
		return FALSE;
	}
	#ifdef UNICODE
	{
		DWORD size;
		WriteFile(FS->hUndoLogFile, UCF2HEADER, UCF2HEADERSIZE, &size, NULL);
	}
	#endif
	return TRUE;
}

typedef struct {
	PPXAPPINFO info;
	FOPSTRUCT *FS;
	const TCHAR *srcDIR;
	const TCHAR *dstDIR;
	int srcLen;
} FIO_INFO;

DWORD_PTR USECDECL FioInfo(FIO_INFO *fio, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	switch(cmdID){
		case '1':
			tstrcpy(uptr->enums.buffer, fio->srcDIR);
			break;
		case 'C':
			if ( memcmp((const TCHAR *)uptr->enums.enumID,
					fio->srcDIR, TSTROFF(fio->srcLen)) ){
				tstrcpy(uptr->enums.buffer, (const TCHAR *)uptr->enums.enumID);
			}else{
				tstrcpy(uptr->enums.buffer, (const TCHAR *)uptr->enums.enumID + fio->srcLen + 1);
			}
			break;
		case '2':
			tstrcpy(uptr->enums.buffer, fio->dstDIR);
			break;

		case PPXCMDID_STARTENUM:	// 検索開始(マーク無しもあり)
		case PPXCMDID_STARTNOENUM:	// 検索開始(マーク無しはなし)
			uptr->enums.enumID = (INT_PTR)fio->FS->opt.files;
			break;

		case PPXCMDID_NEXTENUM:		// 次へ
			if ( *(const TCHAR *)uptr->enums.enumID != '\0' ){
				uptr->enums.enumID = (INT_PTR)(uptr->enums.enumID +
						tstrlen((const TCHAR *)uptr->enums.enumID) + 1);
				if ( *(const TCHAR *)uptr->enums.enumID != '\0' ){
					return 1;
				}
			}
			return 0;

//		case PPXCMDID_ENDENUM:		//列挙終了…何もしない

		default:
			if ( cmdID <= PPXCMDID_FILL ) *uptr->enums.buffer = '\0';
			return PPXA_INVALID_FUNCTION;
	}
	return PPXA_NO_ERROR;
}

BOOL USEFASTCALL CheckSaveDriveMove(struct FopOption *opt, _In_ const TCHAR *src, _In_ const TCHAR *dst)
{
	const TCHAR *srcdend;

	if ( opt->OnDriveMove ) return TRUE;
	if ( opt->fop.mode != FOPMODE_MOVE ) return FALSE;

	srcdend = VFSGetDriveType(src, NULL, NULL);
	if ( (srcdend == NULL) || (srcdend == src) ) return FALSE;
	if ( tstrnicmp(src, dst, srcdend - src) ) return FALSE;
	return TRUE;
}

DWORD GetFileHeaderImg(const TCHAR *filename, BYTE *header, DWORD headersize)
{
	HANDLE hFile;
	DWORD fsize;

	hFile = CreateFileL(filename, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) return 0;

	fsize = ReadFileHeader(hFile, header, headersize);
	CloseHandle(hFile);
	return fsize;
}

int FileImageOperation(FOPSTRUCT *FS, TCHAR *srcDIR, const TCHAR *dstDIR)
{
	void *dt_opt;
	DWORD fsize;
	BYTE header[VFS_check_size];
	FIO_INFO fio;

	fio.info.Function = (PPXAPPINFOFUNCTION)FioInfo;
	fio.info.Name = T("Fop_arc");
	fio.info.RegID = NilStr;
	fio.info.hWnd = FS->hDlg;
	fio.FS = FS;
	fio.srcDIR = srcDIR;
	fio.dstDIR = dstDIR;
	fio.srcLen = tstrlen32(srcDIR);

	fsize = GetFileHeaderImg(srcDIR, header, sizeof(header));
	if ( fsize == 0 ) return Operation_END_ERROR;
	switch( VFSCheckDir(srcDIR, header, fsize, &dt_opt) ){
		case VFSDT_UN: {
			TCHAR buf[CMDLINESIZE];

			if ( NO_ERROR == UnArc_Extract(&fio.info, dt_opt,
					UNARCEXTRACT_PART, buf, XEO_NOEDIT) ){
				TCHAR OldCurrentDir[VFPS];
				int result;

				GetCurrentDirectory(TSIZEOF(OldCurrentDir), OldCurrentDir);
				SetCurrentDirectory(srcDIR);
				// ●1.27 fio.info でログ出力
				result = RunUnARCExec(&fio.info, dt_opt, buf, NilStr, NULL);
				SetCurrentDirectory(OldCurrentDir);
				if ( result == 0 ) break;
			}
			return Operation_END_ERROR;
		}
		default:
			return Operation_END_ERROR;
	}
	FS->progs.info.donefiles = MAX32;
	ReleaseStartMutex(FS);
	OperationStart_End(FS);
	return Operation_END_SUCCESS;
}

HRESULT InitFopClasses(void)
{
	WNDCLASS wcClass;

	if ( DLLWndClass.item.PPxStatic == 0 ){
		wcClass.style			= CS_PARENTDC;
		wcClass.lpfnWndProc		= PPxStaticProc;
		wcClass.cbClsExtra		= 0;
		wcClass.cbWndExtra		= sizeof(LONG_PTR); // 0:進捗バーの描画位置保存
		wcClass.hInstance		= DLLhInst;
		wcClass.hIcon			= NULL;
		wcClass.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wcClass.hbrBackground	= (hDialogBackBrush == NULL) ? WNDCLASSBRUSH(COLOR_3DFACE + 1) : hDialogBackBrush;
		wcClass.lpszMenuName	= NULL;
		wcClass.lpszClassName	= T(PPXSTATICCLASS);
		DLLWndClass.item.PPxStatic = RegisterClass(&wcClass);
	}

	if ( DLLWndClass.item.FileOp == 0 ){
		wcClass.style			= 0;
		wcClass.lpfnWndProc		= DefDlgProc;
//		wcClass.cbClsExtra		= 0;
		wcClass.cbWndExtra		= DLGWINDOWEXTRA;
//		wcClass.hInstance		= DLLhInst; // ↑と同じ
		wcClass.hIcon			= LoadIcon(DLLhInst, MAKEINTRESOURCE(Ic_FOP));
//		wcClass.hCursor			= LoadCursor(NULL, IDC_ARROW); // ↑と同じ
//		wcClass.hbrBackground	= WNDCLASSBRUSH(COLOR_3DFACE + 1); // ↑と同じ
//		wcClass.lpszMenuName	= NULL; // ↑と同じ
		wcClass.lpszClassName	= T(PPFileOpWinClass);
		DLLWndClass.item.FileOp = RegisterClass(&wcClass);
	}

	GetCustData(T("X_flst"), &X_flst, sizeof(X_flst));
	if ( X_flst_mode == X_fmode_api ){ // OS 自動補完を使うときは初期化
		return CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	}else{
		return E_FAIL;
	}
}

VFSDLL BOOL PPXAPI PPxFileOperation(HWND hWnd, VFSFILEOPERATION *fileop)
{
	FILEOPERATIONDLGBOXINITPARAMS fopip;
	BOOL result;
	HRESULT ComInitResult;

	ComInitResult = InitFopClasses();

	fopip.fileop = fileop;
	fopip.FS = HeapAlloc(DLLheap, 0, sizeof(FOPSTRUCT));
	if ( fopip.FS != NULL ){
		if ( fileop->flags & VFSFOP_MODELESSDIALOG ){
			DLGTEMPLATE *dialog;

			dialog = GetDialogTemplate(hWnd, DLLhInst, MAKEINTRESOURCE(IDD_FOP));
			result = (NULL != CreateDialogIndirectParam(DLLhInst, dialog, hWnd, FileOperationDlgBox, (LPARAM)&fopip));
			HeapFree(ProcHeap, 0, dialog);
			if ( SUCCEEDED(ComInitResult) ) CoUninitialize();
			return result;
		}
		result = (BOOL)PPxDialogBoxParam(DLLhInst, MAKEINTRESOURCE(IDD_FOP),
				hWnd, FileOperationDlgBox, (LPARAM)&fopip);
	}else{
		result = FALSE;

		if ( (fileop->flags & VFSFOP_FREEFILES) && (fileop->files != NULL) ){
			HeapFree(ProcHeap, 0, (LPVOID)fileop->files);
		}
	}
	if ( fileop->flags & VFSFOP_NOTIFYREADY ){ // 初期化前に終了した時用
		if ( fileop->hReadyEvent != NULL ) SetEvent(fileop->hReadyEvent);
		EnableWindow(fileop->hReturnWnd, TRUE);
	}
	if ( SUCCEEDED(ComInitResult) ) CoUninitialize();
	return result;
}

#define STRSTEP 0x400
TCHAR *MakeFOPlistFromPPx(PPXAPPINFO *info)
{
	HANDLE heap;
	TCHAR *names, *p, *next;
	PPXCMDENUMSTRUCT work;
	DWORD allocsize = TSTROFF(STRSTEP);
	PPXCMD_F fbuf;

	if ( info == NULL ) return NULL;
										// 保存用のメモリを確保する
	heap = ProcHeap;
	names = HeapAlloc(heap, 0, allocsize);
	if ( names == NULL ){
		XMessage(NULL, STR_FOP, XM_FaERRd, T("GetFiles - AllocError"));
		return NULL;
	}
	p = names;
	next = names + STRSTEP - VFPS;

	PPxEnumInfoFunc(info, PPXCMDID_STARTENUM, fbuf.dest, &work);
	for( ; ; ){
		fbuf.source = FullPathMacroStr;
		fbuf.dest[0] = '\0';
		Get_F_MacroData(info, &fbuf, &work);
		if ( fbuf.dest[0] == '\0' ) break;
		if ( IsParentDirectory(fbuf.dest) == FALSE ){
			if ( p >= next ){				// メモリの追加
				TCHAR *np;

				allocsize += TSTROFF(STRSTEP);
				np = HeapReAlloc(heap, 0, names, allocsize);
				if ( np == NULL ){
					XMessage(NULL, STR_FOP, XM_FaERRd, T("GetFiles - ReAllocError"));
					HeapFree(heap, 0, names);
					names = NULL;
					break;
				}else{
					p = np + (p - names);
					next = np + TSIZEOF(allocsize) - VFPS;
					names = np;
				}
			}
			tstrcpy(p, fbuf.dest);
			p += tstrlen(p) + 1;
		}
		if ( PPxEnumInfoFunc(info, PPXCMDID_NEXTENUM, fbuf.dest, &work) == 0 ){
			break;
		}
	}
	PPxEnumInfoFunc(info, PPXCMDID_ENDENUM, fbuf.dest, &work);
	*p++ = '\0';
	*p = '\0';
	return names;
}

BOOL IsResponseFilePath(TCHAR *src, const TCHAR *param, const TCHAR *path)
{
	if ( param[0] != '@' ) return FALSE;
	VFSFullPath(src, (TCHAR *)param + 1, path);
	return GetFileAttributesL(src) != BADATTR;
}

TCHAR *MakeFOPlistFromParam(const TCHAR *param, const TCHAR *path, DWORD *fopflags)
{
	ThSTRUCT th;
	TCHAR src[VFPS];

	setflag(*fopflags, VFSFOP_FREEFILES);
									// レスポンスファイル
	if ( IsResponseFilePath(src, param, path) ){
		TCHAR *mem, *ps, *pd;

		if ( LoadTextData(src, (TCHAR **)&mem, (TCHAR **)&ps, NULL, 0) != NO_ERROR){
			return NULL;
		}
		pd = mem;
		while ( *ps != '\0' ){
			TCHAR *oldpd;

			oldpd = pd;
			if ( *ps == ';' ){ // 行頭「;」はコメント扱い
				while ( (*ps != '\r') && (*ps != '\n') ) ps++;
			}else if ( *ps == '\"' ){
				ps++;
				while( *ps != '\0' ){
					if ( *ps == '\r' || *ps == '\n' ) break;
					if ( *ps == '\"' ){
						ps++;
						break;
					}
					*pd++ = *ps++;
				}
				// 改行までスキップ
				while ( (*ps != '\r') && (*ps != '\n') ) ps++;
			}else{
				while( *ps != '\0' ){
					if ( *ps == '\r' || *ps == '\n' ) break;
					*pd++ = *ps++;
				}
			}
			while ( (*ps == '\r') || (*ps == '\n') ) ps++;
			if ( oldpd != pd ) *pd++ = '\0'; // 中身があったので区切りを入れる
		}
		*pd = '\0';
		return mem;
	}
									// ワイルドカード
	ThInit(&th);
	if ( tstrchr(param, '*') || tstrchr(param, '?') ){
		HANDLE hFF;
		WIN32_FIND_DATA ff;
		FN_REGEXP fn;
		const TCHAR *p;
		TCHAR mask[VFPS], hpath[VFPS];
		TCHAR name[VFPS];

		p = VFSFindLastEntry(param);
#if 0
		if ( ((*p == '\\') || (*p == '/')) && (*(p + 1) == '*') && (*(p + 2) == '\0') ){
			setflag(*fopflags, VFSFOP_FREEFILES | VFSFOP_NEWDIRNAME);
			ThAddString(&th, param);
			th.top -= 2; // \* を無くす
			*ThPointerT(&th, th.top - 1) = '\0';
			*ThPointerT(&th, th.top) = '\0';
			return (TCHAR *)th.bottom;
		}
#endif
		if ( *p == '\\' ){
			tstrcpy(mask, p + 1);
			tstrcpy(hpath, param);
			hpath[p - param] = '\0';
			VFSFullPath(NULL, hpath, path);
			CatPath(src, hpath, WildCard_All);
		} else {
			tstrcpy(mask, p);
			tstrcpy(hpath, path);
			CatPath(src, (TCHAR *)path, WildCard_All);
		}
		hFF = FindFirstFileL(src, &ff);

		if ( hFF == INVALID_HANDLE_VALUE ) return NULL;

		MakeFN_REGEXP(&fn, mask);
		do{
			if ( IsRelativeDirectory(ff.cFileName) ) continue;
			if ( FinddataRegularExpression(&ff, &fn) ){
				CatPath(name, hpath, ff.cFileName);
				ThAddString(&th, name);
			}
		}while( IsTrue(FindNextFile(hFF, &ff)) );
		FreeFN_REGEXP(&fn);
		FindClose(hFF);
		ThAddString(&th, NilStr);
		return (TCHAR *)th.bottom;
	}
	ThAddString(&th, param);
	ThAddString(&th, NilStr);
	return (TCHAR *)th.bottom;
}

HWND FopGetMsgParentWnd(FOPSTRUCT *FS)
{
	HWND hWnd = FS->hDlg;
	WINDOWPLACEMENT wp;

	wp.length = sizeof(wp);
	GetWindowPlacement(hWnd, &wp);
	if ( (wp.showCmd == SW_SHOWNORMAL) || (wp.showCmd == SW_SHOWMAXIMIZED) ){
		return hWnd;
	}
	if ( (FS->opt.hReturnWnd != NULL) && IsWindow(FS->opt.hReturnWnd) ){
		// メッセージボックスから戻ったときに、フォーカスが合わないので調整
		SetForegroundWindow(FS->opt.hReturnWnd);
		SetForegroundWindow(hWnd);
		return FS->opt.hReturnWnd;
	}
	return hWnd;
}

BOOL IsExistAuxOperation(const TCHAR *AuxPath, const TCHAR *sub)
{
	TCHAR str[CMDLINESIZE];

	SplitAuxName(AuxPath, str);
	return IsExistCustTable(str, sub);
}

const TCHAR *GetTailPathSep(const TCHAR *fileptr)
{
	if ( *fileptr != '\0' ){
		const TCHAR *lastsep;

		#ifdef UNICODE
			lastsep = fileptr + tstrlen(fileptr) - 1;
		#else
			lastsep = fileptr;
			for (;;){
				const TCHAR *next;
				next = lastsep + Chrlen(*lastsep);
				if ( *next == '\0' ) break;
				lastsep = next;
			}
		#endif
		if ( (*lastsep != '\\') && (*lastsep != '/') ) lastsep = NULL;
		return lastsep;
	}else{
		return NULL;
	}
}

ERRORCODE FileOperationSourceAux(FOPSTRUCT *FS, ODVALS *odv, const TCHAR *fileptr)
{
	const TCHAR *lastsepptr, *srcdir, *srcname;
	TCHAR filepath[VFPS], lastsep = '\0';

	if ( (FS->opt.fop.mode != FOPMODE_COPY) &&
		 (FS->opt.fop.mode != FOPMODE_MOVE) ){
		return ERROR_INVALID_FUNCTION;
	}

	SetTinyProgress(FS);

	lastsepptr = GetTailPathSep(fileptr);
	if ( lastsepptr != NULL ) lastsep = *lastsepptr;

	if ( VFSGetDriveType(fileptr, NULL, NULL) != NULL ){
		TCHAR *last;

		tstrcpy(filepath, fileptr);
		// 文字列末尾のパス区切りは除去
		if ( lastsep != '\0' ) filepath[lastsepptr - fileptr] = '\0';

		last = VFSFindLastEntry(filepath);
		if ( (*last == '/') || (*last == '\\') ){
			*last++ = '\0';
		}
		srcdir = filepath;
		srcname = last;
	}else{
		srcdir = odv->srcDIR;
		srcname = fileptr;
	}


	if ( odv->dsttype == VFSPT_AUXOP ){
		TCHAR *srcterm, *dstterm;

		srcterm = VFSGetDriveType(srcdir, NULL, NULL);
		dstterm = VFSGetDriveType(odv->dstDIR, NULL, NULL);
		// aux:/xxxx/ が一致するか
		if ( (srcterm != NULL) && (dstterm != NULL) &&
			 ((srcterm - srcdir) == (dstterm - odv->dstDIR)) &&
			 (memcmp(srcdir, odv->dstDIR, (srcterm - srcdir) * sizeof(TCHAR)) == 0) ){
			const TCHAR *id;

			if ( (*dstterm == '/') || (*dstterm == '\\') ) dstterm++;

			id = (FS->opt.fop.mode == FOPMODE_MOVE) ? T("move-d") : T("copy-d");
			if ( IsExistAuxOperation(srcdir, id) ){
				return FileOperationAux(FS, id, srcdir, srcname, dstterm);
			}else{
				return FileOperationAux(FS,
						(FS->opt.fop.mode == FOPMODE_MOVE) ?
							T("move") : StrFopActionCopy,
						srcdir, srcname, dstterm);
			}
		}
	}

	if ( lastsep != '\0' ){
		const TCHAR *id;

		id = (FS->opt.fop.mode == FOPMODE_MOVE) ? T("get-dm") : T("get-d");
		if ( IsExistAuxOperation(srcdir, id) ){
			return FileOperationAux(FS, id, srcdir, srcname, odv->dstPath);
		}else{
			if ( lastsep == '/' ){
				TCHAR dstfile[VFPS], *ptr;

				ptr = thprintf(dstfile, TSIZEOF(VFPS), T("%s%s"),
						odv->dstPath, srcname);
				MakeDirectories(dstfile, NULL);
				thprintf(ptr, 4, T("%c."), lastsep);
				return FileOperationAux(FS,
						(FS->opt.fop.mode == FOPMODE_MOVE) ?
							T("get-m") : T("get"),
						srcdir, srcname, dstfile);
			}
		}
	}

	return FileOperationAux(FS,
			(FS->opt.fop.mode == FOPMODE_MOVE) ? T("get-m") : T("get"),
			srcdir, srcname, odv->dstPath);
}

ERRORCODE FileOperationDestinationAux(FOPSTRUCT *FS, ODVALS *odv, const TCHAR *fileptr)
{
	if ( (FS->opt.fop.mode != FOPMODE_COPY) &&
		 (FS->opt.fop.mode != FOPMODE_MOVE) ){
		return ERROR_INVALID_FUNCTION;
	}

	SetTinyProgress(FS);

	if ( GetTailPathSep(fileptr) != NULL ){
		return FileOperationAux(FS,
				(FS->opt.fop.mode == FOPMODE_MOVE) ?
					T("store-dm") : T("store-d"),
				odv->dstDIR, odv->srcPath, fileptr);
	}

	return FileOperationAux(FS,
			(FS->opt.fop.mode == FOPMODE_MOVE) ?
				T("store-m") : T("store"),
			odv->dstDIR, odv->srcPath, fileptr);
}

void Operation_Drive(FOPSTRUCT *FS, ODVALS *odv)
{
	const TCHAR *fileptr;
	ERRORCODE mainresult = NO_ERROR;

	if ( odv->srctype == VFSPT_FTP ){
		ERRORCODE result = OpenFtp(odv->srcDIR, odv->hFTP);

		if ( result != NO_ERROR ){
			ReleaseStartMutex(FS);
			OperationStart_End(FS);
			XMessage(FS->hDlg, NULL, XM_GrERRld, T("FTP error"));
			return;
		}
	}else if ( odv->dsttype == VFSPT_FTP ){
		ERRORCODE result = OpenFtp(odv->dstDIR, odv->hFTP);
		if ( result != NO_ERROR ){
			ReleaseStartMutex(FS);
			OperationStart_End(FS);
			XMessage(FS->hDlg, NULL, XM_GrERRld, T("FTP error"));
			return;
		}
	}

	for ( fileptr = FS->opt.files;
		 *fileptr != '\0';
		 fileptr = fileptr + tstrlen(fileptr) + 1, FS->progs.info.mark++ ){

		if ( IsParentDirectory(fileptr) ) continue;
		if ( FS->opt.SrcDtype == VFSDT_SHN ){
			tstrcpy(odv->srcPath, fileptr);
		}else{
			if ( VFSFullPath(odv->srcPath, (TCHAR *)fileptr, odv->srcDIR) == NULL ){
				FWriteErrorLogs(FS, odv->srcPath, T("Srcpath"), PPERROR_GETLASTERROR);
				continue;
			}
		}
		if ( FS->renamemode == FALSE ){
			const TCHAR *entry;

			if ( FS->opt.fopflags & VFSFOP_NEWDIRNAME ){
				tstrcpy(odv->dstPath, odv->dstDIR);
			}else{
				if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_KEEPDIR ){
					entry = fileptr;
				}else{
					entry = VFSFindLastEntry(fileptr);
				}
				if ( *entry == '\\' ) entry++;
				if ( VFSFullPath(odv->dstPath, (TCHAR *)entry, odv->dstDIR) == NULL ){
					FWriteErrorLogs(FS, odv->dstPath, T("Dstpath"), PPERROR_GETLASTERROR);
					continue;
				}
				if ( (FS->opt.fop.flags & VFSFOP_OPTFLAG_KEEPDIR) &&
					 (FS->testmode == FALSE) ){
					TCHAR *lastentry;

					lastentry = VFSFindLastEntry(odv->dstPath);
					*lastentry = '\0';
					if ( BADATTR == GetFileAttributesL(odv->dstPath) ){
						MakeDirectories(odv->dstPath, NULL);
					}
					*lastentry = '\\';
				}
			}
		}else{
			tstrcpy(odv->dstPath, odv->srcPath);
		}
		FS->progs.srcpath = odv->srcPath;

		if ( odv->srctype == VFSPT_FTP ){
			mainresult = FileOperationSourceFtp(FS, odv->hFTP, fileptr, odv->dstPath);
		}else if ( odv->dsttype == VFSPT_FTP ){
			mainresult = FileOperationDestinationFtp(FS, odv->hFTP, odv->srcPath, fileptr);
		}else if ( odv->srctype == VFSPT_HTTP ){
			mainresult = FileOperationSourceHttp(FS, odv, fileptr);
		}else if ( odv->srctype == VFSPT_AUXOP ){
			mainresult = FileOperationSourceAux(FS, odv, fileptr);
			if ( mainresult == NO_ERROR ) FS->progs.info.donefiles++;
		}else if ( odv->dsttype == VFSPT_AUXOP ){
			mainresult = FileOperationDestinationAux(FS, odv, fileptr);
			if ( mainresult == NO_ERROR ) FS->progs.info.donefiles++;
		}else{
			FILE_STAT_DATA fdata; // ※cFileName無効

			if ( GetFileSTAT(odv->srcPath, &fdata) == FALSE ){
				int mode;

				fdata.dwFileAttributes = 0;
				if ( VFSGetDriveType(odv->srcPath, &mode, NULL) != NULL ){
					if ( (mode <= VFSPT_SHN_DESK) || (mode == VFSPT_SHELLSCHEME) ){
						VFSGetRealPath(NULL, odv->srcPath, odv->srcPath);
						if ( GetFileSTAT(odv->srcPath, &fdata) == FALSE ){
							fdata.dwFileAttributes = 0;
						}
					}
				}
			}
			if ( fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				if ( !(FS->opt.fop.filter & VFSFOP_FILTER_NODIRFILTER) ){
					if ( LFNfilter(&FS->opt, odv->dstPath, fdata.dwFileAttributes) > ERROR_NO_MORE_FILES ){
						break;
					}
				}
				if ( IsTrue(FS->flat) ) tstrcpy(odv->dstPath, odv->dstDIR);
				SetTinyProgress(FS);
				mainresult = DlgCopyDir(FS, odv->srcPath, odv->dstPath, fdata.dwFileAttributes);
				if ( mainresult == NO_ERROR ) FopLog(FS, odv->srcDIR, odv->dstDIR, LOG_DIR);
			}else{
				if ( !(FS->opt.fop.filter & VFSFOP_FILTER_NOFILEFILTER) ){
					if ( LFNfilter(&FS->opt, odv->dstPath, fdata.dwFileAttributes) != NO_ERROR ){
						break;
					}
				}else{
				//名前変更モード && ファイル名変更無し なので処理をスキップ
					if ( IsTrue(FS->renamemode) ) continue;
				}
				if ( IsTrue(FS->testmode) ){
					mainresult = TestDest(FS, odv->srcPath, odv->dstPath);
				}else{
					mainresult = DlgCopyFile(FS, odv->srcPath, odv->dstPath, &fdata);
				}
			}
		}

		if ( FS->state == FOP_TOBREAK ) break;

		if ( mainresult != NO_ERROR ){
			if ( mainresult != ERROR_CANCELLED ){
				JobErrorBox(FS, STR_FOP, mainresult);
			}
			break;
		}
	}

	if ( FS->opt.fop.mode == FOPMODE_DELETE ){
		if ( !mainresult && FS->DelStat.noempty ){
			JobErrorBox(FS, STR_FOP, ERROR_DIR_NOT_EMPTY);
		}
	}

	if ( (odv->srctype == VFSPT_FTP) || (odv->dsttype == VFSPT_FTP) ){
		DInternetCloseHandle(odv->hFTP[FTPHOST]);
		DInternetCloseHandle(odv->hFTP[FTPINET]);
//	}else if ( (odv.srctype == VFSPT_AUXOP) || (odv.dsttype == VFSPT_AUXOP) ){
//		AuxCloseOperation
	}

	ReleaseStartMutex(FS);
	if ( FS->testmode == FALSE ){
		PPxPostMessage(WM_PPXCOMMAND, K_ENDCOPY, odv->dstPath[0]);
		if ( (FS->opt.fop.mode == FOPMODE_MOVE) && (odv->dstPath[0] != odv->srcPath[0]) ){
			PPxPostMessage(WM_PPXCOMMAND, K_ENDCOPY, odv->srcPath[0]);
		}
	}
	OperationStart_End(FS);
}

typedef struct {
	FOPSTRUCT *FS;
	ODVALS odv;
} ODSTRUCT;

THREADEXRET THREADEXCALL Operation_Drive_Thread(ODSTRUCT *ods)
{
	THREADSTRUCT threadstruct = {T("FileOp.Main"), XTHREAD_TERMENABLE, NULL, 0, 0};

	PPxRegisterThread(&threadstruct);
	(void)CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	Operation_Drive(ods->FS, &ods->odv);
/*
	if ( (ods->FS->testmode == FALSE) &&
		 ((ods->FS->NoAutoClose == FALSE) &&
		  !(ods->FS->opt.fop.flags & VFSFOP_OPTFLAG_LOGRWAIT)) ){
//		ShowWindow(ods->FS->hDlg, SW_HIDE);
	}
*/
	CloseHandle(ods->FS->hStateChangeEvent);
	ods->FS->hStateChangeEvent = NULL;
	PostMessage(ods->FS->hDlg, WM_FOP_COMMAND, WM_FOP_CMD_END_OPERATION, Operation_END_SUCCESS);
	HeapFree(DLLheap, 0, ods);

	PPxUnRegisterThread();
	CoUninitialize();
	t_endthreadex(0);
}

int OperationStart(FOPSTRUCT *FS)
{
	ODVALS odv;
	TCHAR *srcdir, *dstdir;
	DWORD dstattr;
	HWND hDlg = FS->hDlg;

	if ( (X_log & ((1 << XM_INFOLLog) | (1 << XM_INFOlog))) ){
		setflag(FS->opt.fop.flags, VFSFOP_OPTFLAG_LOGGING);
	}

	CheckAndInitIfo(FS);

	FS->errorcount = 0;
	memset(&FS->progs.info, 0, sizeof(FS->progs.info));
	FS->flat = FS->opt.fop.flags & VFSFOP_OPTFLAG_FLATMODE;
	FS->BackupDirCreated = FALSE;
	FS->reparsemode = FOPR_UNKNOWN;
	if ( FS->opt.fop.flags & (VFSFOP_OPTFLAG_SYMCOPY_SYM | VFSFOP_OPTFLAG_SYMCOPY_FILE) ){
		if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_SYMCOPY_SYM ){
			FS->reparsemode = IDYES; // シンボリックとしてコピー
		}else{
			FS->reparsemode = IDNO; // ファイルとしてコピー
		}
	}

	FS->hUndoLogFile = NULL;
	SaveControlData(FS);
	SetFopLowPriority(FS);

	if ( FS->opt.files == NULL ){
		XMessage(NULL, STR_FOP, XM_FaERRd, T("No source"));
		return Operation_END_ERROR;
	}
	FS->hide = SetJobTask(hDlg, JOBSTATE_STARTJOB | JOBFLAG_ENABLEHIDE | (FS->opt.fop.mode + JOBSTATE_FOP_MOVE));
	if ( FS->hide ){
		if ( (Sm->JobList.hWnd != NULL) && (Sm->JobList.hidemode == JOBLIST_HIDE) && !FS->testmode ){
			ShowWindow(hDlg, SW_HIDE);
		}else{
			FS->hide = FALSE;
		}
	}

	if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_PREVENTSLEEP ){
		StartPreventSleep(hDlg, TRUE);
	}

	FS->progs.StartTick = GetTickCount();
	FS->progs.ProgTick = FS->progs.CapTick = 0;
	FS->progs.nowper = -1;

	FS->hotc.hSrcDisk = FS->hotc.hDestDisk = NULL;
	FS->hotc.LastTick = 0;

	if ( (FS->opt.fop.mode == FOPMODE_LINK) && (DCreateHardLink == NULL) ){
		GETDLLPROCT(hKernel32, CreateHardLink);
		if ( DCreateHardLink == NULL ){
			XMessage(hDlg, MES_TMHL, XM_FaERRd, MES_ENSO);
			return Operation_END_ERROR;
		}
	}
	if ( (FS->opt.fop.mode == FOPMODE_MOVE) && (FS->DestDir[0] == '\0') ){
		FS->renamemode = TRUE;
	}else{
		FS->renamemode = FALSE;
	}
	if ( FS->opt.fop.mode == FOPMODE_DELETE ){
		DWORD X_wdel[4] = X_wdel_default;

		if ( FS->testmode ){
			XMessage(hDlg, NULL, XM_GrERRld, T("test mode not support"));
			return Operation_END_ERROR;
		}

		GetCustData(T("X_wdel"), &X_wdel, sizeof(X_wdel));

		FS->DelStat.OldTime = GetTickCount();
		FS->DelStat.count = 0;
		FS->DelStat.useaction = 0;
		FS->DelStat.noempty = FALSE;
		FS->DelStat.flags = X_wdel[0];
		FS->DelStat.warnattr = X_wdel[1];
		FS->DelStat.info = &FS->DelInfo;
		FS->DelInfo.Name = STR_FOP;
		FS->DelInfo.Function = (PPXAPPINFOFUNCTION)FopDeleteInfoFunc;
		FS->DelInfo.hWnd = hDlg;
		if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_NOWDELEXIST ){
			resetflag(FS->DelStat.flags, VFSDE_WARNFILEINDIR);
		}
		if ( FS->opt.erroraction ){
			FS->DelStat.useaction = FS->opt.erroraction;
		}
		if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_SKIPERROR ){
			FS->DelStat.useaction = IDIGNORE;
		}
		FS->DelStat.flags |= (FS->opt.fop.flags & (3 << VFSFOP_OPTFLAG_SYMDEL_SHIFT)) >> (VFSFOP_OPTFLAG_SYMDEL_SHIFT - VFSDE_SYMDEL_SHIFT);
	}
	if ( FS->opt.SrcDtype == VFSDT_UNKNOWN ){
		// SrcDtype が一時的に VFSPT_ を扱う
		VFSGetDriveType(FS->opt.files, &FS->opt.SrcDtype, 0);
		if ( FS->opt.SrcDtype == VFSPT_UNKNOWN ){
			VFSGetDriveType(FS->opt.source, &FS->opt.SrcDtype, 0);
		}else if ( FS->opt.SrcDtype == VFSPT_RAWDISK ){
			tstrcpy(FS->opt.source, FS->opt.files);
		}
		// VFSPT_ を VFSDT_ に変換
		if ( (FS->opt.SrcDtype <= VFSPT_SHN_DESK) || (FS->opt.SrcDtype == VFSPT_SHELLSCHEME) ){
			FS->opt.SrcDtype = VFSDT_SHN;
		}else if ( FS->opt.SrcDtype == VFSPT_AUXOP ){
			FS->opt.SrcDtype = VFSDT_AUXOP;
		}else if ( (FS->opt.SrcDtype == VFSPT_FTP) || (FS->opt.SrcDtype == VFSPT_HTTP) ){
			// そのまま
		}else{ // 不明
			FS->opt.SrcDtype = VFSDT_PATH;
		}
	}
										// Window サイズ変更 ----------
	SetWindowY(FS, IDD_FOP_Y_STAT);
										// コピー元のパス形成
	if ( FS->opt.SrcDtype != VFSDT_SHN ){
		VFSFixPath(odv.srcDIR, FS->opt.source, NULL, VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_NOFIXEDGE);
		srcdir = VFSGetDriveType(odv.srcDIR, &odv.srctype, NULL);
		if ( srcdir == NULL ){
			XMessage(hDlg, NULL, XM_GrERRld, T("Bad source path"));
			return Operation_END_ERROR;
		}
		if ( odv.srctype <= VFSPT_SHN_DESK ){
			odv.srcDIR[0] = '?';
			odv.srcDIR[1] = '\0';
			srcdir = odv.srcDIR;
		}
	}else{
		odv.srcDIR[0] = '?';
		odv.srcDIR[1] = '\0';
		srcdir = odv.srcDIR;
		odv.srctype = VFSPT_SHN_DESK;
	}
										// コピー先のパス形成
	VFSFixPath(odv.dstDIR, FS->DestDir, FS->opt.source, VFSFIX_VFPS | VFSFIX_REALPATH | VFSFIX_NOFIXEDGE);
	dstdir = VFSGetDriveType(odv.dstDIR, &odv.dsttype, NULL);
	if ( (odv.dsttype <= VFSPT_SHN_DESK) || (dstdir == NULL) ){
		VFSFixPath(odv.dstDIR, FS->DestDir, FS->opt.source, VFSFIX_VFPS | VFSFIX_NOFIXEDGE);
		dstdir = VFSGetDriveType(odv.dstDIR, &odv.dsttype, NULL);
		if ( (odv.dsttype >= VFSPT_UNKNOWN) || (dstdir == NULL) ){
			XMessage(hDlg, NULL, XM_GrERRld, T("Bad dest. path"));
			return Operation_END_ERROR;
		}
	}

	if ( !(FS->opt.fopflags & VFSFOP_AUTOSTART) ){
		TCHAR *lastp = NULL;

		// ヒストリ登録内容を必ず末尾「\」にする
		if ( (odv.dsttype == VFSPT_DRIVE) || (odv.dsttype == VFSPT_UNC) ){
			lastp = VFSFindLastEntry(odv.dstDIR);
			if ( *lastp == '\\' ) lastp++;
			if ( *lastp != '\0' ){
				lastp += tstrlen(lastp);
				*lastp = '\\';
				*(lastp + 1) = '\0';
			}
		}
		WriteHistory(PPXH_DIR, odv.dstDIR, 0, NULL);
		if ( lastp != NULL ) *lastp = '\0';
	}

	SetWindowText(hDlg, odv.dstDIR);
										// コントロール変更 -----------
	FopUiEnable(FS, FUE_DISABLE_ALL);

	dstdir = GetDriveEnd(dstdir, odv.dsttype);
	srcdir = GetDriveEnd(srcdir, odv.srctype);

	if ( FS->opt.fop.mode == FOPMODE_UNDO ) return UndoCommand(FS);
	if ( !(FS->opt.fop.flags & VFSFOP_OPTFLAG_QSTART) ){ // 他のFOPがあれば待機
		DWORD X_fopw = GetCustDword(T("X_fopw"), 0);
		BOOL waitmutex;

		FS->RunID = RunIDcounter++ | B31;

		if ( X_fopw == 1 ){
			FS->hStartMutex = CreateMutex(NULL, FALSE, PPXJOBMUTEX);
			waitmutex = (FS->hStartMutex != NULL) &&
					(WaitForSingleObject(FS->hStartMutex, 0) != WAIT_OBJECT_0);
			if ( !waitmutex ){
				if ( ProcessRunID == 0 ){
					ProcessRunID = FS->RunID;
				}else{
					waitmutex = TRUE;
				}
			}
		}else{
			FS->hStartMutex = NULL;
			waitmutex = (X_fopw == 0) ? FALSE : CheckJobWait();
		}

		if ( waitmutex ){
			const TCHAR *waitstr;

			FS->state = FOP_WAIT;
			SetDlgItemText(hDlg, IDOK, MessageText(STR_FOPSTART));
			waitstr = MessageText(STR_WAITOPERATION);
			SetWindowText(FS->progs.hProgressWnd, waitstr);
			SetStateOnCaption(FS, waitstr);
			SetJobTask(hDlg, JOBSTATE_WAITJOB | JOBFLAG_CHANGESTATE);
			for ( ; ; ){
				if ( FS->hStartMutex != NULL ){ // X_fopw == 1
					DWORD result;

					result = MsgWaitForMultipleObjects(1, &FS->hStartMutex,
							FALSE, INFINITE, QS_ALLEVENTS | QS_SENDMESSAGE);
					if ( result == WAIT_OBJECT_0 ){
						if ( ProcessRunID == 0 ){
							ProcessRunID = FS->RunID;
							break;
						}
						Sleep(100);
					}
					if ( result == WAIT_ABANDONED_0 ) break;
					FopMessageLoopOrPause(FS);
				}else{ // X_fopw == 2
					MSG msg;

					while( (int)GetMessage(&msg, NULL, 0, 0) > 0 ){
						if ( DialogKeyProc(&msg) == FALSE ){
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
						if ( FS->state != FOP_WAIT ) break;
					}
				}
				if ( FS->state != FOP_WAIT ){
					ReleaseStartMutex(FS);
					if ( IsTrue(FS->Cancel) ){
						SetJobTask(hDlg, JOBSTATE_FINWJOB);
						SetJobTask(hDlg, JOBSTATE_ENDJOB);
						OperationStart_End(FS);
						return Operation_END_ERROR;
					}
					break;
				}
			}
			SetJobTask(hDlg, JOBSTATE_FINWJOB);
			SetStateOnCaption(FS, NULL);
		}
	}
	// ========================================================================

	if ( odv.dsttype < 0 ){ // shell name space
		return Fop_ShellNameSpace(FS, odv.srcDIR, odv.dstDIR);
	}

	if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_UNDOLOG ){
		if ( InitUndoLog(FS) == FALSE ) return Operation_END_ERROR;
	}

	// ========================================================================
	// src が FTP/HTTP/aux: なら、サイズ算出ができない
	if ( (odv.srctype == VFSPT_FTP) || (odv.srctype == VFSPT_HTTP) || (odv.srctype == VFSPT_AUXOP) ){
		setflag(FS->opt.fop.flags, VFSFOP_OPTFLAG_NOCOUNT);
	// dst の調査。先がFTPだったり削除モードなら調査できない
	}else if ( (odv.dsttype != VFSPT_FTP) && (odv.dsttype != VFSPT_AUXOP) && (FS->opt.fop.mode != FOPMODE_DELETE) ){
//		TCHAR *sptr = tstrstr(dstDIR, T("::"));

//		if ( sptr != NULL ) *sptr = '\0';
		dstattr = GetFileAttributesL(odv.dstDIR);
//		if ( sptr != NULL ) *sptr = ':';
		if ( (FS->testmode == FALSE) && (dstattr == BADATTR) ){
			DWORD result;

			result = GetLastError();
			// dstDIR が "\\.\Volume{…}" とかの場合、この状態になる
			if ( (result == ERROR_INVALID_PARAMETER) && (odv.dstDIR[0] == '\\') ){
				result = NO_ERROR;
			}

			if ( (result != ERROR_NOT_READY) && (result != NO_ERROR) ){
			// 実際は ERROR_PATH_NOT_FOUND が多いらしい…TT
				TCHAR *ptr;

				ptr = tstrstr(odv.dstDIR, StrListFileID);
				if ( ptr != NULL ){
					int listresult;

					*ptr = '\0';
					listresult = OperationStartListFile(FS, odv.srcDIR, odv.dstDIR);
					ReleaseStartMutex(FS);
					OperationStart_End(FS);
					return listresult;
				}

				if ( !(FS->opt.fop.flags & VFSFOP_OPTFLAG_NOWCREATEDIR) &&
				   PMessageBox(FopGetMsgParentWnd(FS), MES_QCRD, STR_FOPWARN, MB_OKCANCEL) != IDOK){
					ReleaseStartMutex(FS);
					OperationStart_End(FS);
					return Operation_END_ERROR;
				}
				result = MakeDirectories(odv.dstDIR, NULL);
				if ( result == NO_ERROR ){
					FopLog(FS, odv.dstDIR, NULL, LOG_MAKEDIR);
				}
			}
			if ( result != NO_ERROR ){
				JobErrorBox(FS, odv.dstDIR, result);
				ReleaseStartMutex(FS);
				OperationStart_End(FS);
				return Operation_END_ERROR;
			}
		}else if ( (FS->renamemode == FALSE) &&
					!(dstattr & FILE_ATTRIBUTE_DIRECTORY) ){
			int result;

			if ( !((FS->opt.fop.mode == FOPMODE_COPY) ||
				   (FS->opt.fop.mode == FOPMODE_MOVE)) ){
				XMessage(FS->hDlg, STR_FOP, XM_FaERRd, T("No support this mode"));
				return Operation_END_ERROR;
			}

			result = OperationStartToFile(FS, odv.srcDIR, odv.dstDIR);
			if ( result >= 0 ){
				ReleaseStartMutex(FS);
				OperationStart_End(FS);
				return result;
			}
		}
	}
										// ファイル数算出 ---------------------
	{
		const TCHAR *cfileptr;

		SetTaskBarButtonProgress(hDlg, TBPF_INDETERMINATE, 0);
		cfileptr = FS->opt.files;
		while ( *cfileptr != '\0' ){
			cfileptr = cfileptr + tstrlen(cfileptr) + 1;
			FS->progs.info.markall++;
		}
	}

	if ( (FS->opt.fop.mode != FOPMODE_SHORTCUT) &&
		 (FS->opt.fop.mode != FOPMODE_LINK) &&
		 (FS->opt.fop.mode != FOPMODE_UNDO) &&
		 (FS->opt.fop.mode != FOPMODE_SYMLINK) &&
		 !(FS->opt.fop.flags & VFSFOP_OPTFLAG_NOCOUNT) &&
		 (FS->testmode == FALSE) &&
		 (CountMain(FS, odv.srcDIR, (odv.dsttype == VFSPT_FTP) ? NULL : odv.dstDIR) == FALSE) ){
		ReleaseStartMutex(FS);
		OperationStart_End(FS);
		return Operation_END_ERROR;
	}
										// コントロール変更 -----------
	FS->state = FOP_BUSY;
	SetDlgItemText(hDlg, IDOK, MessageText(STR_FOPPAUSE));
	UpdateWindow(hDlg);

							// (複写&削除)か移動かを判断
	FS->opt.OnDriveMove =
			(FS->opt.fop.mode == FOPMODE_MOVE) &&
			( ((srcdir - odv.srcDIR) == (dstdir - odv.dstDIR)) &&
					!memcmp(odv.srcDIR, odv.dstDIR, TSTROFF(srcdir - odv.srcDIR)) );

	// 名前加工を行うかを確認する
	FS->opt.UseNameFilter = 0;

	if ( FS->opt.rename[0] != '\0' ){
		setflag(FS->opt.UseNameFilter, NameFilter_Use | NameFilter_Rename);
										// マクロ展開
		if ( (FS->opt.rename[0] == RENAME_EXTRACTNAME) ||
			 (FS->opt.fop.filter & VFSFOP_FILTER_EXTRACTNAME) ){
			setflag(FS->opt.UseNameFilter, NameFilter_ExtractName);
										// 名前変更に正規表現を使うか
		}else if ( tstrchr(FS->opt.rename, '/') != NULL ){
			if ( FALSE == InitRegularExpression( // ※ FS->opt.rename 破壊
					&FS->opt.rexps, FS->opt.rename, SLASH_FREE) ){
				OperationStart_End(FS);
				return Operation_END_ERROR;
			}
		}
	}
	if ( IsTrue(FS->opt.fop.delspc) ||
		 IsTrue(FS->opt.fop.sfn) ||
		 (FS->opt.fop.chrcase != 0) ||
		 (FS->opt.fop.filter & VFSFOP_FILTER_DELNUM) ){
		setflag(FS->opt.UseNameFilter, NameFilter_Use);
	}

	FS->opt.burst = FS->opt.fop.useburst;

	if ( FS->opt.SrcDtype >= VFSDT_FATIMG ){
		int Dtype = FS->opt.SrcDtype;
			// VFSDT_FATIMG, VFSDT_FATDISK, VFSDT_CDIMG, VFSDT_CDDISK
		if ( (Dtype <= VFSDT_CDDISK) ||
			 (Dtype == VFSDT_RAWIMG) ||
			// VFSDT_ARCFOLDER, VFSDT_CABFOLDER, VFSDT_LZHFOLDER, VFSDT_ZIPFOLDER
			 ((Dtype >= VFSDT_ARCFOLDER) && (Dtype <= VFSDT_ZIPFOLDER)) ){
			ImgExtract(FS, odv.srcDIR, odv.dstDIR);
			OperationStart_End(FS);
			ReleaseStartMutex(FS);
			return Operation_END_SUCCESS;
		}
	}

	if ( (FS->opt.fop.flags & VFSFOP_OPTFLAG_AUTOROFF) &&
		 (FS->opt.fop.AtrMask & FILE_ATTRIBUTE_READONLY) ){
		VFSFullPath(odv.dstPath, (TCHAR *)FS->opt.files, odv.srcDIR);
		GetDriveName(odv.srcPath, odv.dstPath);
		if ( GetDriveType(odv.srcPath) == DRIVE_CDROM ){
			resetflag(FS->opt.fop.AtrMask, FILE_ATTRIBUTE_READONLY);
			resetflag(FS->opt.fop.AtrFlag, FILE_ATTRIBUTE_READONLY);
		}
	}
	FopLog(FS, odv.srcDIR, odv.dstDIR, LOG_DIR);

	// srcDIR が書庫ファイルの時の展開処理
	if ( (FS->opt.SrcDtype != VFSDT_SHN) &&
		 (FS->opt.SrcDtype != VFSDT_LFILE) ){
		DWORD srcdirattr;

		srcdirattr = GetFileAttributesL(odv.srcDIR);
		if ( (srcdirattr != BADATTR) && !(srcdirattr & FILE_ATTRIBUTE_DIRECTORY) ){
			int result;

			result = FileImageOperation(FS, odv.srcDIR, odv.dstDIR);
			if ( IsTrue(result) ) return result;
		}
	}
										// 通常ファイルの時のメインループ
//	if ( (FS->ifo == NULL) && (WinType >= WINTYPE_VISTA) )
	// スレッド分離
	if ( (FS->ifo == NULL) && (FS->opt.fopflags & VFSFOP_SPLITTHREAD) ){
		ODSTRUCT *ods;

		ods = (ODSTRUCT *)HeapAlloc(DLLheap, 0, sizeof(ODSTRUCT));
		if ( ods != NULL ){
			DWORD tmp;

			ods->FS = FS;
			ods->odv = odv;
			FS->hStateChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			FS->hOperationThread = t_beginthreadex(NULL, 0,
					(THREADEXFUNC)Operation_Drive_Thread, ods, 0, &tmp);
			if ( FS->hOperationThread != NULL ) return Operation_WORKING;
			HeapFree(DLLheap, 0, ods);
		}
	}
	// 同一スレッド
	Operation_Drive(FS, &odv);
	return Operation_END_SUCCESS;
}

void EndingOperation(FOPSTRUCT *FS)
{
	FShowLog(FS);
	SetFopLowPriority(NULL);
	if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_PREVENTSLEEP ){
		EndPreventSleep(FS->hDlg);
	}

	if ( FS->hotc.hSrcDisk != NULL ) CloseHandle(FS->hotc.hSrcDisk);
	if ( FS->hotc.hDestDisk != NULL ) CloseHandle(FS->hotc.hDestDisk);

	if ( FS->opt.rexps != NULL ){
		FreeRegularExpression(FS->opt.rexps);
		FS->opt.rexps = NULL;
	}
	if ( FS->opt.CopyBuf != NULL ){
		if ( VirtualFree(FS->opt.CopyBuf, 0, MEM_RELEASE) == FALSE ){
			FWriteErrorLogs(FS, NilStr, T("Free Buffer"), PPERROR_GETLASTERROR);
		}
		FS->opt.CopyBuf = NULL;
	}
}
