/*-----------------------------------------------------------------------------
	Paper Plane xUI	 Virtual File System		ファイル操作, *file link/junc
-----------------------------------------------------------------------------*/
#define ONVFSDLL		// VFS.H の DLL export 指定
#include "WINAPI.H"
#include "WINOLE.H"
#include "PPX.H"
#include "WINAPIIO.H"
#include "VFS.H"
#include "PPD_DEF.H"
#include "VFS_FOP.H"
#pragma hdrstop

ValueWinAPI(CreateHardLink) = NULL;
DefineWinAPI(BOOLEAN, CreateSymbolicLink, (LPCTSTR lpSymlinkFileName, LPCTSTR lpTargetFileName, DWORD dwflags)) = NULL;

#define CHANGE_ATTR (FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)

VFSDLL ERRORCODE PPXAPI CreateJunction(const TCHAR *FileName, const TCHAR *ExistingFileName, SECURITY_ATTRIBUTES *sa)
{
	HANDLE hFile;
	DWORD size, attr;
	REPARSE_DATA_IOBUFFER rdio;
	TCHAR tmpname[VFPS];
#ifndef UNICODE
	char namebuf[VFPS];
#endif

	if ( (ExistingFileName[0] == '\\') &&
			(ExistingFileName[1] == '\\') &&
			(ExistingFileName[2] == '.') &&
			(ExistingFileName[3] == '\\') ){
		ExistingFileName += 4;
	}
	if ( tstrlen(ExistingFileName) == 3 ){ // ジャンクション元がルート(x:\)
		thprintf(tmpname, TSIZEOF(tmpname), T("%s\\%c"), FileName, *ExistingFileName);
		FileName = tmpname;
	}
#ifdef UNICODE
	CatPath(rdio.data.ReparseGuid.PathBuffer, T("\\??"), ExistingFileName);
#else
	CatPath(namebuf, T("\\??"), ExistingFileName);
	AnsiToUnicode(namebuf, rdio.data.ReparseGuid.PathBuffer, MAX_PATH);
#endif
	if ( CreateDirectoryL(FileName, sa) == FALSE ) goto error;
	hFile = CreateFileL(FileName, GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ){
		RemoveDirectoryL(FileName);
		goto error;
	}

	rdio.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
	rdio.Reserved = 0;
	rdio.data.ReparseGuid.SubstituteNameOffset = 0;
	rdio.data.ReparseGuid.SubstituteNameLength =
			(WORD)(strlenW(rdio.data.ReparseGuid.PathBuffer) * sizeof(WCHAR));
	rdio.data.ReparseGuid.PrintNameOffset =
			(WORD)(rdio.data.ReparseGuid.SubstituteNameLength + sizeof(WCHAR));
	rdio.data.ReparseGuid.PrintNameLength = 0;
	*(WCHAR *)(BYTE *)((BYTE *)rdio.data.ReparseGuid.PathBuffer +
			rdio.data.ReparseGuid.PrintNameOffset) = '\0';
	rdio.ReparseDataLength = (WORD)( (sizeof(WORD) * 4) +
			(rdio.data.ReparseGuid.PrintNameOffset + sizeof(WORD)) );
	size = sizeof(DWORD) + sizeof(WORD) * 2 + rdio.ReparseDataLength;
	if ( DeviceIoControl(hFile, FSCTL_SET_REPARSE_POINT,
			&rdio, size, NULL, 0, &size, NULL) == FALSE ){
		CloseHandle(hFile);
		RemoveDirectoryL(FileName);
		goto error;
	}
	attr = GetFileAttributesL(ExistingFileName);
	if ( attr != BADATTR ){
		SetFileAttributesL(FileName,
			(GetFileAttributesL(ExistingFileName) & ~CHANGE_ATTR) |
			(attr & CHANGE_ATTR) );
	}
	CloseHandle(hFile);
	return NO_ERROR;
error:
	return GetLastError();
}

ERRORCODE FOPMakeShortCut(const TCHAR *src, TCHAR *dst, BOOL directory, BOOL test)
{
	TCHAR linkpath[VFPS], linkpathdir[VFPS], *ptr;

	tstrcpy(linkpath, dst);
	ptr = VFSFindLastEntry(linkpath);
	if ( (ptr > linkpath) && (*ptr != '\\') ) ptr--; // c:\ の時の補正
	*ptr = '\0';
	tstrcpy(linkpathdir, linkpath);
	*ptr = '\\';
	if ( directory == FALSE ){ // ファイル名…拡張子or末尾
		ptr += FindExtSeparator(ptr);
	}else{ // ディレクトリ…末尾
		ptr += tstrlen(ptr);
	}
	tstrcpy(ptr, StrShortcutExt);
	if ( test == FALSE ){
		if ( FAILED(MakeShortCut(src, linkpath, linkpathdir)) ){
			return ERROR_ACCESS_DENIED;
		}
	}else{
		DWORD attr;

		tstrcpy(dst, linkpath);
		attr = GetFileAttributesL(linkpath);
		if ( attr != BADATTR ){
			return ERROR_ALREADY_EXISTS;
		}
	}
	return NO_ERROR;
}

#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 2
#endif

ERRORCODE FopCreateSymlink(const TCHAR *FileName, const TCHAR *ExistingFileName, DWORD flags)
{
	if ( DCreateSymbolicLink == NULL ){
		GETDLLPROCT(hKernel32, CreateSymbolicLink);
		if ( DCreateSymbolicLink == NULL ) return GetLastError();
	}
	SetLastError(NO_ERROR);

	if ( IsTrue(DCreateSymbolicLink(FileName, ExistingFileName, flags)) ||
		 IsTrue(DCreateSymbolicLink(FileName, ExistingFileName, flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) ){
		DWORD attr = GetFileAttributesL(ExistingFileName);

		if ( attr != BADATTR ){
			SetFileAttributesL(FileName,
					(GetFileAttributesL(ExistingFileName) & ~CHANGE_ATTR) |
					(attr & CHANGE_ATTR) );
		}
		return NO_ERROR;
	}
	return GetLastError();
}
