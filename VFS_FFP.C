/*-----------------------------------------------------------------------------
	Paper Plane xUI	 Virtual File System
				ネットワークリソース列挙 / ストリーム列挙 / POSIX directory
-----------------------------------------------------------------------------*/
#define ONVFSDLL		// VFS.H の DLL export 指定
#include "WINAPI.H"
#include <shlobj.h>
#ifdef WINEGCC
#include <errno.h>
#endif
#include "PPX.H"
#include "PPX_64.H"
#include "PPD_DEF.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "VFS_FF.H"
#pragma hdrstop

//=============================================================================
// ネットワークリソース列挙
//=============================================================================
HMODULE hIphlpapi = NULL;

DWORD EnumCheckTick = 0;
const TCHAR EnumNetServerThreadName[] = T("Enum PC name");

DWNETOPENENUM DWNetOpenEnum;
DWNETENUMRESOURCE DWNetEnumResource;
DWNETCLOSEENUM DWNetCloseEnum;
DWNETADDCONNECTION3 DWNetAddConnection3;

LOADWINAPISTRUCT MPRDLL[] = {
	LOADWINAPI1T(WNetOpenEnum),
	LOADWINAPI1T(WNetEnumResource),
	LOADWINAPI1 (WNetCloseEnum),
	LOADWINAPI1T(WNetAddConnection3),
	{NULL, NULL}
};

#ifndef IF_MAX_PHYS_ADDRESS_LENGTH
#define IF_MAX_PHYS_ADDRESS_LENGTH 32
#endif

#define AF_UNSPEC 0
#define AF_INET   2
#define AF_INET6  23

#ifndef _WIN64
  #pragma pack(push, 4)
#endif

typedef ULONG xIN_ADDR;
typedef struct {
	UCHAR       Byte[16];
} xIN6_ADDR;

typedef struct
{
	/* ADDRESS_FAMILY */ USHORT sin_family;
	USHORT sin_port;
	xIN_ADDR sin_addr;
	CHAR sin_zero[8];
} xSOCKADDR_IN;

typedef struct
{
	/* ADDRESS_FAMILY */ USHORT sin6_family;
	USHORT sin6_port;
	ULONG  sin6_flowinfo;
	xIN6_ADDR sin6_addr;
	ULONG sin6_scope_id;
} xSOCKADDR_IN6;

typedef union {
	xSOCKADDR_IN Ipv4;
	xSOCKADDR_IN6 Ipv6;
	/* ADDRESS_FAMILY */ USHORT si_family;
} xSOCKADDR_INET;

struct xNET_LUID { DWORD Value, Info; };

typedef struct {
	xSOCKADDR_INET Address;
	/* NET_IFINDEX */ ULONG InterfaceIndex;
	LARGE_INTEGER InterfaceLuid;
	UCHAR PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
	ULONG PhysicalAddressLength;
	/*NL_NEIGHBOR_STATE*/ int State;
	UCHAR Flags;
	ULONG ReachabilityTime;
} xMIB_IPNET_ROW2;

typedef struct {
	ULONG NumEntries;
#ifndef _WIN64
	int dummy;
#endif
	xMIB_IPNET_ROW2 Table[1];
} xMIB_IPNET_TABLE2;
#ifndef _WIN64
  #pragma pack(pop)
#endif

#define GetROW21stAddr4(ROW2) (((BYTE *)&(ROW2)->Address.Ipv4.sin_addr)[0])
#define GetROW2Addr4(ROW2, index) (((BYTE *)&(ROW2)->Address.Ipv4.sin_addr)[index])
#define GetROW21stAddr6(ROW2) (((BYTE *)&(ROW2)->Address.Ipv6.sin6_addr)[0])
#define GetROW2Addr6_16(ROW2, index) ((((BYTE *)&(ROW2)->Address.Ipv6.sin6_addr)[index * 2 + 1]) + (((BYTE *)&(ROW2)->Address.Ipv6.sin6_addr)[index * 2]) * 256)

#define PING_CHECK_TIME 50 // 50ms
#define PING_SIZE 8

DefineWinAPI(DWORD, GetIpNetTable2, (/*ADDRESS_FAMILY*/ USHORT Family, xMIB_IPNET_TABLE2 **Table)) = NULL; // Vista 以降
DefineWinAPI(void, FreeMibTable, (PVOID Memory));
DefineWinAPI(HANDLE, IcmpCreateFile, (VOID)); // Vista 以降: Iphlpapi.dll XP 以前は別 DLL
DefineWinAPI(HANDLE, Icmp6CreateFile, (VOID));
DefineWinAPI(BOOL, IcmpCloseHandle, (HANDLE));
DefineWinAPI(DWORD, IcmpSendEcho, (HANDLE, DWORD , LPVOID, WORD, void *, LPVOID, DWORD, DWORD));

BOOL LoadNetFunctions(void)
{
	if ( hMPR != NULL ) return TRUE;
	hMPR = LoadWinAPI("MPR.DLL", NULL, MPRDLL, LOADWINAPI_LOAD);
	if ( hMPR == NULL) return FALSE;
	return TRUE;
}

BOOL CheckComputerActive(const TCHAR *fname, size_t strsize)
{
	TCHAR path[VFPS];
	NETRESOURCE nr = {
			RESOURCE_GLOBALNET, RESOURCETYPE_DISK,
			RESOURCEDISPLAYTYPE_SERVER, RESOURCEUSAGE_CONTAINER,
			NULL, NULL, NULL, NULL};
	HANDLE hNet;
	ERRORCODE result;

	// Vista 以降は却って遅くなるのでチェックしない
	if ( WinType >= WINTYPE_VISTA ) return TRUE;

	if ( LoadNetFunctions() == FALSE ) return FALSE;

	memcpy(path, fname, strsize * sizeof(TCHAR));
	path[strsize] = '\0';
	nr.lpRemoteName = path;		// リソース列挙ができるか？

	result = DWNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_DISK, 0, &nr, &hNet);
	if ( result == NO_ERROR ){
		DWNetCloseEnum(hNet);
	}else{
		if ( result == ERROR_ACCESS_DENIED ) result = NO_ERROR;
	}
	return result == NO_ERROR;
}

// 該当PCのリソース一覧を取得
ERRORCODE MakeComputerResourceList(FF_MC *MC, const TCHAR *fname)
{
	char nrbuf[0x800];
	ERRORCODE neterr = NO_ERROR;
//	DWORD count = 1;

	ThInit(&MC->dirs);
	ThInit(&MC->files);
	ThAddString(&MC->dirs, T("."));
	ThAddString(&MC->dirs, T(".."));

	if ( LoadNetFunctions() ){
		NETRESOURCE nr = {
				RESOURCE_GLOBALNET, RESOURCETYPE_DISK,
				RESOURCEDISPLAYTYPE_SERVER, RESOURCEUSAGE_CONTAINER,
				NULL, NULL, NULL, NULL};
		HANDLE hNet;

		nr.lpRemoteName = (TCHAR *)fname;		// リソース列挙ができるか？

		if ( (WinType >= WINTYPE_2000) &&
			 ((neterr = DWNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_DISK, 0, &nr, &hNet)) == NO_ERROR) ){

			for ( ; ; ){
				DWORD cnt, size;
				const TCHAR *p, *q;

				cnt = 1;
				size = sizeof(nrbuf);
				if ( DWNetEnumResource(hNet, &cnt, &nrbuf, &size) != NO_ERROR ) break;
				p = ((NETRESOURCE *)nrbuf)->lpRemoteName;
				for ( ; ; ){
					q = FindPathSeparator(p);
					if ( q == NULL ) break;
					p = q + 1;
				}
				ThAddString(&MC->dirs, p);
//				count++;
			}
			DWNetCloseEnum(hNet);
		}
		if ( neterr == ERROR_NO_NET_OR_BAD_PATH ) neterr = ERROR_BAD_NET_NAME;
	}

	((DWORD *)nrbuf)[3] = 0;
	GetCustData(StrX_dlf, nrbuf, sizeof(DWORD) * 4);

	if ( (((DWORD *)nrbuf)[3] & XDLF_ADMINDRIVE) && (neterr != ERROR_BAD_NET_NAME) ){
		ThAddString(&MC->dirs, T("C$"));
		neterr = NO_ERROR;
//		count++;
	}

	if ( !(((DWORD *)nrbuf)[3] & XDLF_NODISPSHARE) ){
									// ヒストリから共有名を抽出
		TCHAR pcname[MAX_PATH];
		size_t taillen;
		int index = 0;

		CatPath(pcname, (TCHAR *)fname, NilStr); // \\pcname\ 形式に
		taillen = tstrlen(pcname);
		UsePPx();
		for ( ; ; ){
			const TCHAR *hisp, *nextp;

			hisp = EnumHistory(PPXH_PPCPATH, index++);
			if ( hisp == NULL ) break;

			if ( (tstrcmppart(pcname, hisp, taillen) == 0) &&
				 (hisp[taillen] != '\0') ){
				nextp = FindPathSeparator(hisp + taillen);
				if ( nextp == NULL ){ // \\pcname\share
					AddDriveList(&MC->dirs, hisp + taillen);
				}else{
					tstrcpypart(pcname + taillen, hisp + taillen, nextp - hisp - taillen);
					AddDriveList(&MC->dirs, pcname + taillen);
				}
//				count++;
			}
		}
		FreePPx();
	}

	if ( neterr != NO_ERROR ){
		ThFree(&MC->files);
		ThFree(&MC->dirs);
	}else{
		ThAddString(&MC->dirs, NilStr);
		ThAddString(&MC->files, NilStr);
		MC->d_off = 0;
		MC->f_off = 0;
	}
	return neterr;
}

// PC一覧を取得
void EnumWNetServer(NETRESOURCE *nr, BOOL extend)
{
	HANDLE hNet;
	char nrbuf[0x4000];
	const TCHAR *p, *q;
	NETRESOURCE *pnr;
	DWORD cnt, size;
	BOOL first = TRUE;

	if ( DWNetOpenEnum( (nr == NULL) ? RESOURCE_CONTEXT : RESOURCE_GLOBALNET,
			RESOURCETYPE_DISK, 0, nr, &hNet) != NO_ERROR ){
		return;
	}

	pnr = (NETRESOURCE *)nrbuf;
	for ( ; ; ){
		cnt = 1;
		size = sizeof(nrbuf);
		if ( DWNetEnumResource(hNet, &cnt, &nrbuf, &size) != NO_ERROR ) break;
		if ( pnr->dwDisplayType == RESOURCEDISPLAYTYPE_SERVER ){
			UsePPx();

			if ( first ){
				first = FALSE;
				while ( IsTrue(DeleteHistory(PPXH_NETPCNAME, NULL)) );
			}

			if ( extend == FALSE ){
				p = pnr->lpRemoteName;
				for ( ; ; ){
					q = FindPathSeparator(p);
					if ( q == NULL ) break;
					p = q + 1;
				}
				WriteHistory(PPXH_NETPCNAME, p, 0, NULL);
			}else{
				WriteHistory(PPXH_NETPCNAME, pnr->lpRemoteName, 0, NULL);
			}
			FreePPx();
		}else{
			if ( IsTrue(extend) ) EnumWNetServer(pnr, TRUE);
		}
	}
	DWNetCloseEnum(hNet);
	return;
}

extern DWORD CheckPorts(xMIB_IPNET_ROW2 *);
void EnumIPServer(void)
{
	ULONG i;
	unsigned long status;
	HANDLE hIcmp = NULL;

	xMIB_IPNET_TABLE2 *pipTable = NULL;
	xMIB_IPNET_ROW2 *ipRow;
	char ICMPData[64]; // sizeof(ICMP_ECHO_REPLY = 28 or 40) + PING_SIZE
	TCHAR addr[64];

	memset(ICMPData, 0, sizeof(ICMPData));
	if ( hIphlpapi == NULL ){
		hIphlpapi = LoadLibrary(T("Iphlpapi.dll"));
		if ( hIphlpapi != NULL ){
			GETDLLPROC(hIphlpapi, GetIpNetTable2);
			GETDLLPROC(hIphlpapi, FreeMibTable);
			GETDLLPROC(hIphlpapi, IcmpCreateFile);
			GETDLLPROC(hIphlpapi, IcmpCloseHandle);
			GETDLLPROC(hIphlpapi, IcmpSendEcho);
		}else{
			hIphlpapi = INVALID_HANDLE_VALUE;
		}
	}
	if ( hIphlpapi == INVALID_HANDLE_VALUE ) return;

	if ( (DGetIpNetTable2 == NULL) || (DIcmpCreateFile == NULL) ) return;

	status = DGetIpNetTable2(AF_UNSPEC, &pipTable);
	if ( status != NO_ERROR ) return;

	ipRow = pipTable->Table;
	for ( i = 0; i < pipTable->NumEntries; i++, ipRow++){
		if ( ipRow->Address.si_family == AF_INET ){
			if ( (GetROW21stAddr4(ipRow) & 0xf0) == 0xe0 ) continue; // マルチキャスト(224-239)
			if ( GetROW2Addr4(ipRow, 3) == 0xff ) continue; // ブロードキャストかも

			if ( hIcmp == NULL ) hIcmp = DIcmpCreateFile();
			if ( hIcmp != INVALID_HANDLE_VALUE ){
				DWORD dwRetVal;

				dwRetVal = DIcmpSendEcho(hIcmp,
						*(DWORD *)&ipRow->Address.Ipv4.sin_addr, ICMPData,
						PING_SIZE, NULL, ICMPData, sizeof(ICMPData),
						PING_CHECK_TIME);
				if ( dwRetVal != 0 ) {
					thprintf(addr, TSIZEOF(addr), T("%d.%d.%d.%d"),
							GetROW2Addr4(ipRow, 0),
							GetROW2Addr4(ipRow, 1),
							GetROW2Addr4(ipRow, 2),
							GetROW2Addr4(ipRow, 3) );

					if ( CheckPorts(ipRow) > B0 ){
						thprintf(addr, TSIZEOF(addr), T("http://%d.%d.%d.%d"),
								GetROW2Addr4(ipRow, 0),
								GetROW2Addr4(ipRow, 1),
								GetROW2Addr4(ipRow, 2),
								GetROW2Addr4(ipRow, 3) );
					}

					UsePPx();
					WriteHistory(PPXH_NETPCNAME, addr, 0, NULL);
					FreePPx();
#if 0
					ICMP_ECHO_REPLY *pEchoReply = (ICMP_ECHO_REPLY *)ReplyBuffer;
					struct in_addr ReplyAddr;
					ReplyAddr.S_un.S_addr = pEchoReply->Address;
					printf("[Received %ld]", dwRetVal);
					printf("[Status = %ld]", pEchoReply->Status);
					printf("[Roundtrip time = %ld ms]", pEchoReply->RoundTripTime);
				}else{
					printf("<IcmpSendEcho failed>");
#endif
				}
			}
		}
		if ( ipRow->Address.si_family == AF_INET6 ){
			if ( GetROW21stAddr6(ipRow) == 0xff ) continue; // マルチキャスト(ff00::/8)
#if 0
			char a[200];
			DWORD size;
			size = 200;
			WSAAddressToStringA((LPSOCKADDR)&ipRow->Address.Ipv6, sizeof(ipRow->Address.Ipv6), NULL, a, &size);
#endif
			thprintf(addr, TSIZEOF(addr), T("%x-%x-%x-%x-%x-%x-%x-%x.ipv6-literal.net"),
					GetROW2Addr6_16(ipRow, 0),
					GetROW2Addr6_16(ipRow, 1),
					GetROW2Addr6_16(ipRow, 2),
					GetROW2Addr6_16(ipRow, 3),
					GetROW2Addr6_16(ipRow, 4),
					GetROW2Addr6_16(ipRow, 5),
					GetROW2Addr6_16(ipRow, 6),
					GetROW2Addr6_16(ipRow, 7));
			UsePPx();
			WriteHistory(PPXH_NETPCNAME, addr, 0, NULL);
			FreePPx();
		}
		//printf("- After Reache: %lu ms\n", ipRow->ReachabilityTime);
	}
	DFreeMibTable(pipTable);
	if ( (hIcmp != NULL) && (hIcmp != INVALID_HANDLE_VALUE) ){
		DIcmpCloseHandle(hIcmp);
	}
}

void EnumNetServerMain(BOOL extend)
{
	EnumIPServer();
	EnumWNetServer(NULL, extend);
	EnumCheckTick = GetTickCount();
	if ( Sm != NULL ) Sm->EnumNerServerThreadID = 0;
}

THREADEXRET THREADEXCALL EnumNetServerThread(LPVOID param)
{
	THREADSTRUCT threadstruct = {EnumNetServerThreadName, XTHREAD_EXITENABLE | XTHREAD_TERMENABLE, NULL, 0, 0};

	PPxRegisterThread(&threadstruct);
	EnumNetServerMain(param != NULL);
	PPxUnRegisterThread();
	t_endthreadex(0);
}

// \\+  or 2000/XPの \\ で使用する PC 列挙
void EnumNetServer(ThSTRUCT *dirs, BOOL extend)
{
	int index = 0;

	if ( Sm->EnumNerServerThreadID == 0 ){
		// オーバーフローは無視しても問題ない
		if ( (GetTickCount() - EnumCheckTick) >= 15 * 1000 ){
			HANDLE hThread;

			if ( LoadNetFunctions() == FALSE ) return;

			#ifndef UNICODE
			if ( WinType == WINTYPE_9x ){
				EnumNetServerMain(extend);
				return;
			}
			#endif
			hThread = t_beginthreadex(NULL, 0,
					(THREADEXFUNC)EnumNetServerThread,
					(void *)(DWORD_PTR)extend, 0, &Sm->EnumNerServerThreadID);
			if ( hThread != NULL ){
				WaitForSingleObject(hThread, 200); // 少しだけ待ってみる
				SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
				CloseHandle(hThread);
			}
		}
	}

	UsePPx();
	for ( ; ; ){
		const TCHAR *hisp;

		hisp = EnumHistory(PPXH_NETPCNAME, index++);
		if ( hisp == NULL ) break;
		AddDriveList(dirs, hisp);
	}
	FreePPx();
	if ( Sm->EnumNerServerThreadID != 0 ){
		AddDriveList(dirs, T("<busy>"));
	}
	return;
}

//=============================================================================
// ストリーム列挙
//	将来は FileStreamInformation (NtQueryInformationFile)を使うことも検討。
//=============================================================================
void FindFirstStream(HANDLE hFile, FF_STREAM *fs, const TCHAR *filename)
{
	TCHAR *bp;

	fs->step = FFSTEP_THIS;
	fs->context = NULL;
	fs->hFile = hFile;

	bp = FindLastEntryPoint(filename);
	fs->basep = thprintf(fs->basename, TSIZEOF(fs->basename), T("..\\%s"), bp );
}

BOOL FindNextStream(FF_STREAM *fs, WIN32_FIND_DATA *findfile)
{
	WIN32_STREAM_ID stid;
	DWORD size, reads;
	WCHAR wname[VFPS];

	if ( fs->step < FFSTEP_ENTRY ) return FFStepInfo(findfile, &fs->step);
	for ( ; ; ){
		size = ToSIZE32_T((LPBYTE)&stid.cStreamName - (LPBYTE)&stid);
		if ( FALSE == BackupRead(fs->hFile, (LPBYTE)&stid,
				size, &reads, FALSE, FALSE, &fs->context) ){
			break;
		}
		if ( reads < size ) break;
		if ( FALSE == BackupRead(fs->hFile, (LPBYTE)&wname,
				stid.dwStreamNameSize, &reads, FALSE, FALSE, &fs->context) ){
			break;
		}
		if ( reads < stid.dwStreamNameSize ) break;

		if ( stid.dwStreamNameSize ){
			TCHAR *p;

			wname[stid.dwStreamNameSize >> 1] = '\0';
			#ifdef UNICODE
				tstrcpy(fs->basep, wname);
			#else
				UnicodeToAnsi(wname, fs->basep, VFPS - (fs->basep - fs->basename));
			#endif
			p = tstrrchr(fs->basep, ':');
			if ( (p != NULL) && (tstrcmp(p, T(":$DATA")) == 0) ) *p = '\0';
			fs->basename[MAX_PATH - 1] = '\0';

			tstrcpy(findfile->cFileName, fs->basename);

			SetDummyFindData(findfile);
			findfile->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			LetFilesizeHL(*findfile, stid.Size);
		}

		BackupSeek(fs->hFile, *(DWORD *)(&stid.Size),
				*((DWORD *)(&stid.Size) + 1), &reads, &reads, &fs->context);
		if ( stid.dwStreamNameSize == 0 ) continue;
		return TRUE;
	}
	SetLastError(ERROR_NO_MORE_FILES);
	return FALSE;
}

void FindCloseStream(FF_STREAM *fs)
{
	WIN32_STREAM_ID stid;
	DWORD reads;

	if ( fs->context != NULL ){
		BackupRead(fs->hFile, (LPBYTE)&stid, 0, &reads, TRUE, FALSE, &fs->context);
	}
	CloseHandle(fs->hFile);
}

//=============================================================================
// POSIX directory
//=============================================================================
#ifdef USESLASHPATH
#ifdef WINEGCC
// time_t から FILETIME に変換する
void FiletToFileTime(const time_t *timet, FILETIME *ftime)
{
	DDmul(*timet, 10000000, &ftime->dwLowDateTime, &ftime->dwHighDateTime);
	if ( sizeof(time_t) > sizeof(int) ){
		ftime->dwHighDateTime += (*timet >> 32) * 10000000;
	}
	AddDD(ftime->dwLowDateTime, ftime->dwHighDateTime, 0xd53e8000, 0x19db1de);
}

BOOL NextSlashDir(FF_SLASHDIR *sdptr, WIN32_FIND_DATA *findfile)
{
	struct dirent *entry;
	struct stat status;
#ifndef UNICODE
	WCHAR filenameW[VFPS];
#endif
	entry = readdir(sdptr->OpenedDir);
	if ( entry == NULL ) return FALSE;

	findfile->nFileSizeHigh	= 0;
	findfile->nFileSizeLow	= 0;
	findfile->cAlternateFileName[0] = '\0';
	findfile->dwFileAttributes = 0;

#ifdef UNICODE
	MultiByteToWideCharU8(CP_UTF8, 0, entry->d_name, -1, findfile->cFileName, VFPS);
#else
	MultiByteToWideCharU8(CP_UTF8, 0, entry->d_name, -1, filenameW, VFPS);
	UnicodeToAnsi(filenameW, findfile->cFileName, MAX_PATH);
#endif
	strcpy(sdptr->pathlast, entry->d_name);

	if ( stat(sdptr->path, &status) != -1 ){
		findfile->dwReserved0 = status.st_mode;
		if ( S_ISDIR(status.st_mode) ){
			setflag(findfile->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
		}else{
			if ( !S_ISREG(status.st_mode) ){
				setflag(findfile->dwFileAttributes, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DEVICE);
			}
			findfile->nFileSizeLow = status.st_size;
#ifdef _WIN64
			if ( sizeof(status.st_size) > 4 ){
				findfile->nFileSizeHigh = status.st_size >> 32;
			}
#endif
		}
		if ( S_ISCHR(status.st_mode) || S_ISBLK(status.st_mode) ){
			setflag(findfile->dwFileAttributes, FILE_ATTRIBUTE_DEVICE);
		}
		if ( S_ISLNK(status.st_mode) ){
			setflag(findfile->dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT);
		}
		if ( !(status.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) ){
			setflag(findfile->dwFileAttributes, FILE_ATTRIBUTE_READONLY);
		}
		if ( (entry->d_name[0] == '.') &&
			(entry->d_name[1] != '\0') &&
			(entry->d_name[1] != '.') ){
			setflag(findfile->dwFileAttributes, FILE_ATTRIBUTE_HIDDEN);
		}
		FiletToFileTime(&status.st_atime, &findfile->ftLastAccessTime);
		FiletToFileTime(&status.st_mtime, &findfile->ftLastWriteTime);
		FiletToFileTime(&status.st_ctime, &findfile->ftCreationTime);
	}else{
		findfile->dwReserved0 = 0;
		findfile->ftCreationTime.dwLowDateTime		= 0;
		findfile->ftCreationTime.dwHighDateTime		= 0;
		findfile->ftLastAccessTime.dwLowDateTime	= 0;
		findfile->ftLastAccessTime.dwHighDateTime	= 0;
		findfile->ftLastWriteTime.dwLowDateTime		= 0;
		findfile->ftLastWriteTime.dwHighDateTime	= 0;
	}
	return TRUE;
}

ERRORCODE FirstSlashDir(FF_SLASHDIR *sdptr, const TCHAR *dir, WIN32_FIND_DATA *findfile)
{
#ifndef UNICODE
	WCHAR dirW[VFPS];

	AnsiToUnicode(dir, dirW, VFPS);
	UnicodeToUtf8(dirW, sdptr->path, VFPS);
#else
	UnicodeToUtf8(dir, sdptr->path, VFPS);
#endif
	sdptr->OpenedDir = opendir(sdptr->path);
	if ( NULL != sdptr->OpenedDir ){
		sdptr->pathlast = sdptr->path + strlen(sdptr->path);
		*sdptr->pathlast++ = '/';
		if ( IsTrue(NextSlashDir(sdptr, findfile)) ) return NO_ERROR;
		return ERROR_NO_MORE_FILES;
	}
	switch ( errno ){
		case EACCES:
			return ERROR_ACCESS_DENIED;
		case EBADF:
			return ERROR_INVALID_DRIVE;
		case ENOENT:
		case ENOTDIR:
			return ERROR_DIRECTORY;
		case ENOMEM:
			return ERROR_OUTOFMEMORY;
		case ENFILE:
		case EMFILE:
			return ERROR_OUTOFMEMORY;
		default:
			return ERROR_PATH_NOT_FOUND;
	}
}

void CloseSlashDir(FF_SLASHDIR *sdptr)
{
	closedir( sdptr->OpenedDir );
}
#else
ERRORCODE FirstSlashDir(FF_SLASHDIR *sdptr, const TCHAR *dir, WIN32_FIND_DATA *findfile)
{
	TCHAR buf[VFPS], *p;

	buf[0] = 'C';
	buf[1] = ':';
	p = buf + 2;
	tstrcpy(p, dir);
	for ( ;; ){
		p = tstrchr(p, '/');
		if ( p == NULL ) break;
		*p++ = '\\';
	}
	sdptr->hFF = FindFirstFile(buf, findfile);
	if ( sdptr->hFF != INVALID_HANDLE_VALUE ){	// 成功
		return NO_ERROR;
	}
	return GetLastError();
}
BOOL NextSlashDir(FF_SLASHDIR *sdptr, WIN32_FIND_DATA *findfile)
{
	return FindNextFile(sdptr->hFF, findfile);
}
void CloseSlashDir(FF_SLASHDIR *sdptr)
{
	FindClose(sdptr->hFF);
}
#endif // WINEGCC
#endif // USESLASHPATH
