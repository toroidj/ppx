/*-----------------------------------------------------------------------------
	Paper Plane xUI	 Virtual File System		*file 仮想ディレクトリ関連
-----------------------------------------------------------------------------*/
#define ONVFSDLL		// VFS.H の DLL export 指定
#include "WINAPI.H"
#include "WINOLE.H"
#include "PPX.H"
#include "WINAPIIO.H"
#include "VFS.H"
#include "PPD_DEF.H"
#include "VFS_STRU.H"
#include "VFS_FOP.H"
#include "PPX_64.H"
#pragma hdrstop

BOOL ImgExtractFile(FOPSTRUCT *FS, HANDLE hFile, const TCHAR *srcDir, const TCHAR *srcPath, const TCHAR *dstDIR);

typedef BOOL (WINAPI *WriteTextFileFunction)(HANDLE hFile, const TCHAR *text, size_t textlen);

BOOL WINAPI WriteTextFileNative(HANDLE hFile, const TCHAR *text, size_t textlen)
{
	DWORD tmp;

	return WriteFile(hFile, text, TSTROFF(textlen), &tmp, NULL);
}

BOOL WINAPI WriteTextFileUTF8(HANDLE hFile, const TCHAR *text, size_t textlen)
{
	DWORD tmp;
	int len;
	char utf8text[CMDLINESIZE * 5];

#ifdef UNICODE
	len = WideCharToMultiByte(CP_UTF8, 0, text, textlen, utf8text, sizeof(utf8text), NULL, NULL);
#else
	WCHAR utf16text[CMDLINESIZE * 2];

	len = MultiByteToWideChar(CP_ACP, 0, text, textlen, utf16text, TSIZEOFW(utf16text));
	len = WideCharToMultiByteU8(CP_UTF8, 0, utf16text, len, utf8text, sizeof(utf8text), NULL, NULL);
#endif
	if ( len <= 0 ) return (textlen > 0) ? FALSE : TRUE;
	return WriteFile(hFile, utf8text, len, &tmp, NULL);
}

#ifdef UNICODE
BOOL WINAPI WriteTextFileA(HANDLE hFile, const WCHAR *text, size_t textlen)
{
	DWORD tmp;
	int len;
	char textA[CMDLINESIZE * 5];

	len = WideCharToMultiByte(CP_ACP, 0, text, textlen, textA, sizeof(textA), NULL, NULL);
	if ( len <= 0 ) return (textlen > 0) ? FALSE : TRUE;
	return WriteFile(hFile, textA, len, &tmp, NULL);
}
#else
BOOL WINAPI WriteTextFileUTF16(HANDLE hFile, const char *text, size_t textlen)
{
	DWORD tmp;
	int len;

	WCHAR utf16text[CMDLINESIZE * 2];

	len = MultiByteToWideChar(CP_ACP, 0, text, textlen, utf16text, TSIZEOFW(utf16text));
	if ( len <= 0 ) return (textlen > 0) ? FALSE : TRUE;
	return WriteFile(hFile, utf16text, len * sizeof(WCHAR), &tmp, NULL);
}
#endif

// "filename","alternate", A:atr, C:create, L:access, W:write, S:size, comment
const TCHAR LFfmt[] = T("\",\"%s\",A:H%x,C:%u.%u,L:%u.%u,W:%u.%u,S:%u.%u\r\n");
const TCHAR LFfmtJ[] = T("\",\"Y\":\"%s\",\"A\":%d,\"C\":\"%u.%u\",\"L\":\"%u.%u\",\"W\":\"%u.%u\",\"S\":\"%u.%u\"\r\n");
int OperationStartListFile(FOPSTRUCT *FS, const TCHAR *srcDIR, TCHAR *dstDIR)
{
	HANDLE hFile;
	const TCHAR *p;
	union {
		char A[VFPS];
		TCHAR T[VFPS];
	} src;
	DWORD size;
	int i;
	int charcode;
	WriteTextFileFunction WriteText;
	BOOL JSON = FALSE;
	if ( FS->testmode ) return Operation_END_SUCCESS;

	hFile = CreateFileL(dstDIR, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) return Operation_END_ERROR;

	size = 0;
	(void)ReadFile(hFile, src.A, sizeof(src), &size, NULL);
	charcode = GetFileCodeType(dstDIR, (BYTE *)src.T, size);
	if ( (charcode == VTYPE_UTF8) || (charcode == CP_UTF8) ){
		WriteText = WriteTextFileUTF8;
		if ( ((src.A[0] == '[') || (src.A[0] == '{')) &&
			 ((src.A[1] == '\r') || (src.A[1] == '\n') || (src.A[1] == '\"') || (src.A[1] == '\r') || (src.A[1] == '[') || (src.A[1] == '{')) ){
			JSON = TRUE;
		}
	}else if ( (charcode == VTYPE_UNICODE) || (charcode == CP__UTF16L) ){
		#ifdef UNICODE
			WriteText = WriteTextFileNative;
		#else
			WriteText = WriteTextFileUTF16;
		#endif
	}else{ // Ansi
		#ifdef UNICODE
			WriteText = WriteTextFileA;
		#else
			WriteText = WriteTextFileNative;
		#endif
	}
	// 改行追加
	size = 0;
	SetFilePointer(hFile, 0, (PLONG)&size, FILE_END);
	WriteText(hFile, T("\r\n"), 2);

	// エントリ追加
	for ( p = FS->opt.files ; *p ; p = p + tstrlen(p) + 1 ){
		FILE_STAT_DATA fsd;

		if ( IsParentDirectory(p) ) continue;
		if ( FS->opt.SrcDtype == VFSDT_SHN ){
			tstrcpy(src.T, p);
		}else{
			if ( VFSFullPath(src.T, (TCHAR *)p, srcDIR) == NULL ) continue;
		}
		if ( GetFileSTAT(src.T, &fsd) ){
			TCHAR buf[VFPS * 2];

			if ( JSON ){
				tstrcpy(tstpcpy(buf, T("{\"N\":\"")), src.T);
				tstrreplace(buf, T("\\"), T("\\\\"));
				WriteText(hFile, buf, tstrlen(buf));
			}else{
				WriteText(hFile, T("\""), 1);
				WriteText(hFile, src.T, tstrlen(src.T));
			}

			// "filename","alternate", A:atr, C:create, L:access, W:write, S:size, comment
			size = thprintf(buf, TSIZEOF(buf),
					( JSON ) ? LFfmtJ : LFfmt,
					fsd.cAlternateFileName,
					fsd.dwFileAttributes,
					fsd.ftCreationTime.dwHighDateTime,
					fsd.ftCreationTime.dwLowDateTime,
					fsd.ftLastAccessTime.dwHighDateTime,
					fsd.ftLastAccessTime.dwLowDateTime,
					fsd.ftLastWriteTime.dwHighDateTime,
					fsd.ftLastWriteTime.dwLowDateTime,
					fsd.nFileSizeHigh,
					fsd.nFileSizeLow) - buf;
			WriteText(hFile, buf, size);
		}
		FS->progs.info.donefiles++;
	}
	CloseHandle(hFile);

	// 更新したので各PPcに通知
	tstrcpy(src.T, dstDIR);
	tstrcat(src.T, StrListFileID);

	FixTask();
	for ( i = 0 ; i < X_Mtask ; i++ ){
		if ( (Sm->P[i].ID[0] != '\0') &&
			 (!tstrcmp(Sm->P[i].path, dstDIR) || !tstrcmp(Sm->P[i].path, src.T)) ){
			PostMessage(Sm->P[i].hWnd, WM_PPXCOMMAND,
					K_raw | K_c | K_v | VK_F5, 0);
		}
	}
	return Operation_END_SUCCESS;
}

int OperationStartToFile(FOPSTRUCT *FS, const TCHAR *srcDIR, TCHAR *dstDIR)
{
	VFSFILETYPE vft;
										// ファイルの内容で判別 ---------------
	vft.flags = VFSFT_TYPETEXT;
	if ( VFSGetFileType(dstDIR, NULL, 0, &vft) != NO_ERROR ){
		vft.type[0] = '\0';
	}
	/* ショートカットをコピー先に(●準備中)
	if ( !tstrcmp(vft.type, T(":LINK")) ){
		if ( SUCCEEDED(GetLink(FS->hDlg, dstDIR, dstDIR)) && dstDIR[0] ){
			Message(dstDIR);
			return Operation_END_ERROR;
		}
	}
	*/
	// listfile
	if ( !tstrcmp(vft.type, T(":XLF")) ){
		return OperationStartListFile(FS, srcDIR, dstDIR);
	}
	if ( Fop_ShellNameSpace(FS, srcDIR, dstDIR) ) return Operation_END_SUCCESS;

	ErrorPathBox(FS->hDlg, NULL, dstDIR, ERROR_DIRECTORY);
	return Operation_END_ERROR;
}

HANDLE tOpenFile(const TCHAR *filename, LPCTSTR *wp)
{
	TCHAR buf[VFPS], *fp;
	HANDLE hFile;
	TCHAR *vp;
	int openmode, offset = 0;
	DWORD openflags = FILE_ATTRIBUTE_NORMAL;
	TCHAR *separator;

	tstrcpy(buf, filename);
	vp = VFSGetDriveType(buf, &openmode, NULL);
	if ( vp == NULL ){		// 種類が分からない→相対指定の可能性→絶対化
		VFSFullPath(NULL, buf, NULL);
		vp = VFSGetDriveType(buf, &openmode, NULL);
		if ( vp == NULL ){	// それでも種類が分からない→エラー
			return INVALID_HANDLE_VALUE;
		}
	}
	if ( openmode == VFSPT_RAWDISK ){
		openflags = FILE_FLAG_NO_BUFFERING ;//FILE_FLAG_BACKUP_SEMANTICS;
		if ( buf[0] == '#' ){
			TCHAR drive;
			vp -= 2;
			drive = *vp;
			thprintf(buf, TSIZEOF(buf), T("\\\\.\\%c:"), drive);
			offset = 1;
		}
	}

	separator = tstrrchr(buf, ':'); // "::" を検索
	if ( (separator != NULL) && (separator >= (buf + 2)) && (*(separator - 1) == ':') ){
		*(separator - 1) = '\0';

		fp = FindPathSeparator(separator);
		if ( fp == NULL ) fp = separator + tstrlen(separator);
		hFile = CreateFileL(buf, GENERIC_READ,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
				OPEN_EXISTING, openflags, NULL);
	}else{
		for ( ; ; ){
			fp = buf + tstrlen(buf);
			hFile = CreateFileL(buf, GENERIC_READ,
					FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
					OPEN_EXISTING, openflags, NULL);
			if ( hFile != INVALID_HANDLE_VALUE ) break;
			fp = VFSFindLastEntry(buf);
			if ( fp == NULL ){
				hFile = INVALID_HANDLE_VALUE;
				break;
			}
			*fp = '\0';
			if ( (fp - buf) <= 3 ) break;
		}
	}
	if ( offset ) fp = vp + 2;
	*wp = filename + (fp - buf);
	return hFile;
}

extern void GetDriveRawSize(HANDLE hFile, UINTHL *size);
void GetDriveRawSize(HANDLE hFile, UINTHL *size)
{
	DISK_GEOMETRY diskinfo;
	xDISK_GEOMETRY_EX diskinfoex;
	DWORD tmp;

	// 始めにCD-ROMをチェックし、無ければDISKのチェック
	if ( FALSE != DeviceIoControl(hFile, IOCTL_DISK_GET_LENGTH_INFO,
			NULL, 0, size, sizeof(UINTHL), &tmp, NULL) ){
		return;
	}else if ( (FALSE != DeviceIoControl(hFile, IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX,
			NULL, 0, &diskinfoex, sizeof(xDISK_GEOMETRY_EX), &tmp, NULL)) ||
		 (FALSE != DeviceIoControl(hFile, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
			NULL, 0, &diskinfoex, sizeof(xDISK_GEOMETRY_EX), &tmp, NULL)) ){
		*size = diskinfoex.DiskSize;
	}else if (
		(FALSE != DeviceIoControl(hFile, IOCTL_CDROM_GET_DRIVE_GEOMETRY,
			NULL, 0, &diskinfo, sizeof(DISK_GEOMETRY), &tmp, NULL)) ||
		(FALSE != DeviceIoControl(hFile, IOCTL_DISK_GET_DRIVE_GEOMETRY,
			NULL, 0, &diskinfo, sizeof(DISK_GEOMETRY), &tmp, NULL)) ){
		DWORD tracksize, tmpL, tmpH;

		tracksize = diskinfo.TracksPerCylinder *
				diskinfo.SectorsPerTrack * diskinfo.BytesPerSector;

		DDmul(tracksize, diskinfo.Cylinders.u.LowPart,
				&size->s.L, &size->s.H);
		DDmul(tracksize, diskinfo.Cylinders.u.HighPart, &tmpL, &tmpH);
		size->s.H += tmpL;
	}else{
		size->s.L = 200 * MB;
		size->s.H = 0;
	}
}

/*
	WinXP 以降では SetFileValidData が追加されているが、
	特権SE_MANAGE_VOLUME_NAMEが必要なほか、ネットワークドライブ、圧縮ファイル
	暗号化ファイル等には使えない。

	DefineWinAPI(BOOL, SetFileValidData, (HANDLE hFile, LONGLONG ValidDataLength));
	GETDLLPROC(hKernel32, SetFileValidData);
	DSetFileValidData(dstH, srcfinfo.nFileSizeLow);
*/
BOOL NotifyFileSize(HANDLE hFile, DWORD sizeL, DWORD sizeH)
{
	SetFilePointer(hFile, sizeL, (PLONG)&sizeH, FILE_BEGIN);
	SetEndOfFile(hFile);
	sizeH = 0;
	SetFilePointer(hFile, 0, (PLONG)&sizeH, FILE_BEGIN);
	return TRUE;
}

// ディスクイメージを生成する
BOOL ImgExtractImage(FOPSTRUCT *FS, HANDLE hFile, const TCHAR *srcPath, const TCHAR *dstDIR)
{
	TCHAR path[VFPS], *p;
	DWORD size, sizeL, sizeH;
	HANDLE hDstFile;
	BYTE buff[0x10000];
	ERRORCODE result_error = FALSE;

	LetHL_0(FS->progs.FileTransSize);

	// イメージファイル名の作成
	thprintf(path, TSIZEOF(path), T("%s.img"), srcPath);
	while ( (p = tstrchr(path, ':')) != NULL ) *p = '-';
	while ( (p = FindPathSeparator(path)) != NULL ) *p = '-';
	VFSFullPath(NULL, path, dstDIR);
	if ( !(FS->opt.fop.filter & VFSFOP_FILTER_NOFILEFILTER) ){
		if ( LFNfilter(&FS->opt, path, 0) != NO_ERROR ) return FALSE;
	}

	// イメージサイズの取得
	if ( (FS->opt.SrcDtype == VFSDT_FATDISK) ||
		 (FS->opt.SrcDtype == VFSDT_CDDISK) ||
		 (FS->opt.SrcDtype == VFSDT_RAWIMG) ){
		GetDriveRawSize(hFile, (UINTHL *)&FS->progs.FileSize);
	}else{
		FS->progs.FileSize.u.LowPart = GetFileSize(hFile, (DWORD *)&FS->progs.FileSize.u.HighPart);
	}
//	XMessage(NULL, NULL, XM_DbgDIA, T("size: %x %x %'Ld %s"),FS->progs.FileSize.s.L,FS->progs.FileSize.s.H,FS->progs.FileSize.rawdata,path);

	for ( ; ; ){
		ERRORCODE error;
		BY_HANDLE_FILE_INFORMATION srcfinfo, dstfinfo;

		memset(&srcfinfo, 0, sizeof(srcfinfo));

		hDstFile = CreateFileL(path, GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_NEW,
				FILE_ATTRIBUTE_NORMAL, NULL);
		if ( hDstFile != INVALID_HANDLE_VALUE ) break;	// 成功

		error = GetLastError();
		if ( (error != ERROR_ALREADY_EXISTS) &&
			(error != ERROR_FILE_EXISTS) ){
			ErrorPathBox(NULL, NULL, path, error);
			return FALSE;
		}
		hDstFile = CreateFileL(path, GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		error = SameNameAction(FS, hDstFile, &srcfinfo, &dstfinfo, srcPath, path);
		switch( error ){
			case ACTION_RETRY:
				continue;

			case ACTION_APPEND:
			case ACTION_OVERWRITE:
				// ACTION_CREATE へ
			case ACTION_CREATE:
				break;

			case ACTION_SKIP:
				FS->progs.info.EXskips++;
				FS->progs.info.donefiles++;
				return TRUE;

			default: // error
				return FALSE;
		}
		if ( dstfinfo.dwFileAttributes & OPENERRORATTRIBUTES ){
			SetFileAttributesL(path, FILE_ATTRIBUTE_NORMAL);
		}
		DeleteFileL(path);
	}

	sizeL = FS->progs.FileSize.u.LowPart;
	sizeH = FS->progs.FileSize.u.HighPart;
	NotifyFileSize(hDstFile, sizeL, sizeH);
	size = 0;
	SetFilePointer(hFile, 0, (PLONG)&size, FILE_BEGIN);

	while( sizeL || sizeH ){
		if ( ReadFile(hFile, buff,
				sizeH ? sizeof(buff) : min(sizeL, sizeof(buff)),
				&size, NULL) == FALSE ){
			result_error = TRUE;
			break;
		}
		if ( size == 0 ) break;
		if ( WriteFile(hDstFile, buff, size, &size, NULL) == FALSE ){
			result_error = TRUE;
			break;
		}
		SubDD(sizeL, sizeH, size, 0);
		AddHLI(FS->progs.FileTransSize, size);
		SetFullProgress(&FS->progs);
		if ( FS->state == FOP_TOBREAK ){
			result_error = TRUE;
			break;
		}
	}
	SetEndOfFile(hDstFile);
	CloseHandle(hDstFile);
	if ( IsTrue(result_error) ){
		DeleteFileL(path);
		return FALSE;
	}
	FS->progs.info.donefiles++;
	return TRUE;
}

BOOL ImgExtractDir(FOPSTRUCT *FS, HANDLE hFile, const TCHAR *srcDir, const TCHAR *srcPath, const TCHAR *dstDIR)
{
	TCHAR src[VFPS], dst[VFPS];
	HANDLE hFF;
	WIN32_FIND_DATA ff;
	BOOL result = TRUE;

	CreateDirectoryL(dstDIR, NULL);
	CatPath(src, (TCHAR *)srcDir, srcPath);
	CatPath(NULL, src, WildCard_All);

	hFF = VFSFindFirst(src, &ff);
	if ( hFF != INVALID_HANDLE_VALUE ){
		do {
			if ( ff.cFileName[0] == '\0' ){ // 空欄エントリ名
				continue;
			}

			CatPath(src, (TCHAR *)srcPath, ff.cFileName);
			if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				if ( IsRelativeDirectory(ff.cFileName) ) continue;
				CatPath(dst, (TCHAR *)dstDIR, ff.cFileName);
				result = ImgExtractDir(FS, hFile, srcDir, src, dst);
			}else{
				result = ImgExtractFile(FS, hFile, srcDir, src, dstDIR);
			}
			if ( result == FALSE ) break;
		}while( IsTrue(VFSFindNext(hFF, &ff)) );
		VFSFindClose(hFF);
	}
	return result;
}
BOOL ImgExtractFile(FOPSTRUCT *FS, HANDLE hFile, const TCHAR *srcDir, const TCHAR *srcPath, const TCHAR *dstDIR)
{
	DWORD sizeL, sizeH;
	HGLOBAL hMap;
	BYTE *mem;
	const TCHAR *q;
	DWORD tmp;
	IMAGEGETEXINFO exinfo;
	ERRORCODE result;
	VFSGIFA gifa;

	FopMessageLoopOrPause(FS);
	if ( IsTrue(FS->Cancel) ) return FALSE;

	q = FindLastEntryPoint(srcPath);
	if ( IsParentDirectory(q) ) return TRUE;
	if ( VFSFullPath(exinfo.dest, (TCHAR *)q, dstDIR) == NULL ){
		FWriteErrorLogs(FS, exinfo.dest, T("Destpath"), PPERROR_GETLASTERROR);
		return FALSE;
	}
	if ( LFNfilter(&FS->opt, exinfo.dest, 0) > ERROR_NO_MORE_FILES ){
		return FALSE;
	}
	FS->progs.srcpath = srcPath;

	exinfo.Progress = (LPPROGRESS_ROUTINE)CopyProgress;
	exinfo.lpData = &FS->progs;
	exinfo.Cancel = &FS->Cancel;

	gifa.hWnd = INVALID_HANDLE_VALUE;
	gifa.hFile = hFile;

	mem = (BYTE *)&exinfo;
	for ( ; ; ){
		BY_HANDLE_FILE_INFORMATION srcfinfo, dstfinfo;
		HANDLE hTFile;

		gifa.ArchiveName = srcDir;
		gifa.EntryName = srcPath;
		gifa.hMap = &hMap;
		gifa.mem = &mem;
		gifa.sizeL = &sizeL;
		gifa.sizeH = &sizeH;
		gifa.flags = (FS->opt.fop.flags & VFSFOP_OPTFLAG_SKIPERROR) ?
				(GIFA_EXTRACTFILE | GIFA_IGNORE_ERROR) :
				GIFA_EXTRACTFILE;

		result = VFSGetArchiveItem(&gifa);
		if ( (result == ERROR_READ_FAULT) &&
			 (FS->opt.fop.flags & VFSFOP_OPTFLAG_SKIPERROR) ){
			FS->progs.info.LEskips++;
			FWriteErrorLogs(FS, srcPath, T("Srcpath"), result);
			return TRUE;
		}
		if ( result == ERROR_NOT_SUPPORTED ){
			return ImgExtractDir(FS, hFile, srcDir, srcPath, exinfo.dest);
		}
		if ( result == MAX32 ){
			FS->progs.info.donefiles++;

			AddDD(FS->progs.info.donesize.s.L, FS->progs.info.donesize.s.H,
					sizeL, sizeH);
			return TRUE; // VFSGetArchiveItem 内で処理済み
		}

		if ( result == NO_ERROR ){
			HANDLE hDstFile;

			hDstFile = CreateFileL(exinfo.dest, GENERIC_WRITE,
					FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_NEW,
					FILE_ATTRIBUTE_NORMAL, NULL);
			if ( hDstFile != INVALID_HANDLE_VALUE ){
				WriteFile(hDstFile, mem, sizeL, &tmp, NULL);
				CloseHandle(hDstFile);
				GlobalUnlock(hMap);
				GlobalFree(hMap);
				FS->progs.info.donefiles++;
				AddHLI(FS->progs.info.donesize, sizeL);
				return TRUE;
			}
			result = GetLastError();
			memset(&srcfinfo, 0, sizeof(srcfinfo));
		}else{
			srcfinfo.dwFileAttributes = exinfo.ff.dwFileAttributes;
			srcfinfo.ftCreationTime = exinfo.ff.ftCreationTime;
			srcfinfo.ftLastAccessTime = exinfo.ff.ftLastAccessTime;
			srcfinfo.ftLastWriteTime = exinfo.ff.ftLastWriteTime;
			srcfinfo.ftLastWriteTime = exinfo.ff.ftLastWriteTime;
			srcfinfo.nFileSizeHigh = exinfo.ff.nFileSizeHigh;
			srcfinfo.nFileSizeLow = exinfo.ff.nFileSizeLow;
		}

		if ( (result != ERROR_ALREADY_EXISTS) &&
			(result != ERROR_FILE_EXISTS) ){
			if ( FS->opt.fop.flags & VFSFOP_OPTFLAG_SKIPERROR ){
				if ( tstrchr(srcPath, '/') != NULL ){ // ファイル名が異常
					FWriteErrorLogs(FS, exinfo.dest, T("Dest open"), result);
					return TRUE;
				}
			}
			if ( result != ERROR_CANCELLED ) ErrorPathBox(NULL, STR_FOP, srcPath, result);
			return FALSE;
		}
		hTFile = CreateFileL(exinfo.dest, GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		result = SameNameAction(FS, hTFile, &srcfinfo, &dstfinfo, srcPath, exinfo.dest);
		switch( result ){
			case ACTION_RETRY:
				continue;

			case ACTION_APPEND:
			case ACTION_OVERWRITE:
				// ACTION_CREATE へ
			case ACTION_CREATE:
				break;

			case ACTION_SKIP:
				FS->progs.info.EXskips++;
				FS->progs.info.donefiles++;
				return TRUE;

			case ERROR_CANCELLED:
				return ERROR_CANCELLED;

			default: // error
				FWriteErrorLogs(FS, exinfo.dest, NULL, result);
				return FALSE;
		}
		if ( dstfinfo.dwFileAttributes & OPENERRORATTRIBUTES ){
			SetFileAttributesL(exinfo.dest, FILE_ATTRIBUTE_NORMAL);
		}
		DeleteFileL(exinfo.dest);
	}
}

void ImgExtract(FOPSTRUCT *FS, const TCHAR *srcDIR, const TCHAR *dstDIR)
{
	HANDLE hFile;
	TCHAR *wp, *sp, src[VFPS];
	const TCHAR *p;

	if ( FS->opt.SrcDtype == VFSDT_RAWIMG ){
		srcDIR = FS->opt.files;
		if ( (srcDIR[0] != '#') && (srcDIR[0] != '\\') ){
			src[0] = '#';
			tstrcpy(src + 1, srcDIR);
			srcDIR = src;
		}
	}

	hFile = tOpenFile(srcDIR, (const TCHAR **)&wp);
	if ( hFile == INVALID_HANDLE_VALUE ){
		FWriteErrorLogs(FS, srcDIR, T("Srcpath"), PPERROR_GETLASTERROR);
		return;
	}

	if ( FS->opt.SrcDtype == VFSDT_RAWIMG ){
		ImgExtractImage(FS, hFile, srcDIR, dstDIR);
	}else for ( p = FS->opt.files ; *p ; p = p + tstrlen(p) + 1, FS->progs.info.mark++ ){
		if ( VFSFullPath(src, (TCHAR *)p, srcDIR) == NULL ){
			FWriteErrorLogs(FS, src, T("Srcpath"), PPERROR_GETLASTERROR);
			continue;
		}

		sp = src + (wp - srcDIR);
		*sp++ = '\0';
		if ( ImgExtractFile(FS, hFile, src, sp, dstDIR) == FALSE ) break;
	}
	CloseHandle(hFile);
}

