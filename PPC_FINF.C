/*-----------------------------------------------------------------------------
	Paper Plane cUI										ファイル情報
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "WINAPIIO.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#pragma hdrstop

typedef enum {
  xFileBasicInfo,
  xFileStandardInfo,
  xFileNameInfo,
  xFileRenameInfo,
  xFileDispositionInfo,
  xFileAllocationInfo,
  xFileEndOfFileInfo,
  xFileStreamInfo,
  xFileCompressionInfo,
  xFileAttributeTagInfo,
  xFileIdBothDirectoryInfo,
  xFileIdBothDirectoryRestartInfo,
  xFileIoPriorityHintInfo,
  xFileRemoteProtocolInfo,
  xFileFullDirectoryInfo,
  xFileFullDirectoryRestartInfo,
  xFileStorageInfo,
  xFileAlignmentInfo,
  xFileIdInfo,
  xFileIdExtdDirectoryInfo,
  xFileIdExtdDirectoryRestartInfo,
  xFileDispositionInfoEx,
  xFileRenameInfoEx,
  xMaximumFileInfoByHandleClass,
  xFileCaseSensitiveInfo,
  xFileNormalizedNameInfo
} xFILE_INFO_BY_HANDLE_CLASS;

typedef struct {
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  DWORD         FileAttributes;
} xFILE_BASIC_INFO;

typedef struct {
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER EndOfFile;
  DWORD         NumberOfLinks;
  BOOLEAN        DeletePending;
  BOOLEAN       Directory;
} xFILE_STANDARD_INFO;

typedef struct {
  USHORT StructureVersion;
  USHORT StructureSize;
  ULONG  Protocol;
  USHORT ProtocolMajorVersion;
  USHORT ProtocolMinorVersion;
  USHORT ProtocolRevision;
  USHORT Reserved;
  ULONG  Flags;
  struct {
	ULONG Reserved[8];
  } GenericReserved;
  struct {
	ULONG Reserved[16];
  } ProtocolSpecificReserved;
  union {
	struct {
	  struct {
		ULONG Capabilities;
	  } Server;
	  struct {
		ULONG Capabilities;
		ULONG CachingFlags;
	  } Share;
	} Smb2;
	ULONG Reserved[16];
  } ProtocolSpecific;
} xFILE_REMOTE_PROTOCOL_INFO;

/*

typedef struct _FILE_ALLOCATION_INFO {
  LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFO, *PFILE_ALLOCATION_INFO;
typedef struct  {
  DWORD         NextEntryOffset;
  DWORD         FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  DWORD         FileAttributes;
  DWORD         FileNameLength;
  DWORD         EaSize;
  CCHAR         ShortNameLength;
  WCHAR         ShortName[12];
  LARGE_INTEGER FileId;
  WCHAR         FileName[1];
} xFILE_ID_BOTH_DIR_INFO;
typedef struct  {
  ULONG         NextEntryOffset;
  ULONG         FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG         FileAttributes;
  ULONG         FileNameLength;
  ULONG         EaSize;
  WCHAR         FileName[1];
} xFILE_FULL_DIR_INFO;
typedef struct _FILE_STORAGE_INFO {
  ULONG LogicalBytesPerSector;
  ULONG PhysicalBytesPerSectorForAtomicity;
  ULONG PhysicalBytesPerSectorForPerformance;
  ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
  ULONG Flags;
  ULONG ByteOffsetForSectorAlignment;
  ULONG ByteOffsetForPartitionAlignment;
} FILE_STORAGE_INFO, *PFILE_STORAGE_INFO;
typedef struct _FILE_ID_INFO {
  ULONGLONG   VolumeSerialNumber;
  FILE_ID_128 FileId;
} FILE_ID_INFO, *PFILE_ID_INFO;
*/
DefineWinAPI(BOOL, GetFileInformationByHandleEx, (HANDLE, xFILE_INFO_BY_HANDLE_CLASS, LPVOID, DWORD));


#define ATTR_FLAGS_COUNT 32
static const TCHAR *AttrsLabels[ATTR_FLAGS_COUNT] = {
	T("Read only"), T("Hidden"), T("System"), T("Label"),
	T("Directory"), T("Archive"), T("Device"), T("Normal"),
	T("Temporary"), T("Sparse file"), T("Reparse point"), T("Compressed"),
	T("Offline"), T("Not content indexed"), T("Encrypted"), T("Integrity stream"),

	T("Virtual"), T("No scrub data"), T("EA/Recall on open"), T("Pinned"),
	T("Unpinned"), T("B21"), T("Recall on data access"), T("B23"),
	T("B24"), T("B25"), T("no detail"), T("folder"),
	T("virtual entry"), T("SMR BLOB"), T("B30"), T("B31")
};

static const TCHAR *status[] = {
	T("System Message"), T("Deleted"), T("Normal"), T("Gray"),
	T("Changed"), T("Added"), NilStr, NilStr
};

#pragma pack(push, 1)
struct LXSSEA {
	BYTE ud1[4];	// 00 00 00 00
	WORD namelen;		// 00 07
	WORD EAlen;			// 38 00
	BYTE name[8];		// LXATTRB\0

	BYTE ud2[4];	// 00 00 01 00
	DWORD attr;			// 6d 81 00 00
	DWORD gp1, gp2;	// e8 03 00 00 e8 03 00 00

	DWORD ud3, ud4, ud5, ud6;	// 00 00 00 00 80 91 d4 14  58 01 c3 15 58 01 c3 15

	FILETIME date1, date2, date3;
};
#pragma pack(pop)

/*
void ConvEAtime(FILETIME *ftime, TCHAR **dstptr, const TCHAR *label)
{
	FILETIME sttime;
	FILETIME lTime;
	SYSTEMTIME sTime;

	DDmul(ftime->dwLowDateTime, 10000000, &sttime.dwLowDateTime, &sttime.dwHighDateTime);
	sttime.dwHighDateTime += (ftime->dwHighDateTime >> 32) * 10000000;

	AddDD(sttime.dwLowDateTime, sttime.dwHighDateTime, 0xd53e8000, 0x19db1de);

	FileTimeToLocalFileTime(&sttime, &lTime);
	FileTimeToSystemTime(&lTime, &sTime);
	*dstptr = thprintf(*dstptr, 128, T("  %s : %04d-%02d-%02d%3d:%02d:%02d.%03d\r\n"),
			label,
			sTime.wYear, sTime.wMonth, sTime.wDay,
			sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds);
}
*/

void GetLxssEA(HANDLE hF, LPVOID *context, TCHAR **dstptr)
{
	struct LXSSEA eadata;
	DWORD reads;

	*dstptr = tstpcpy(*dstptr, T("lxss EA(72)\r\n"));

	BackupRead(hF, (LPBYTE)&eadata, 72, &reads, FALSE, FALSE, context);
	if ( reads != 72 ) return;
	if ( memcmp(eadata.name, "LXATTRB", 8) != 0 ) return;
	*dstptr = tstpcpy(*dstptr, T("   Attr: "));
	MakeStatString(*dstptr, eadata.attr, ECS_NORMAL, 15);
	*dstptr += tstrlen(*dstptr);
/*
	ConvEAtime(&eadata.date1, &dstptr, T("time1"));
	ConvEAtime(&eadata.date2, &dstptr, T("time2"));
	ConvEAtime(&eadata.date3, &dstptr, T("time3"));
*/
}

void LoadSummary(TCHAR *name, TCHAR *type, TCHAR *text)
{
	TCHAR filename[VFPS + 100];
	char *Summary = NULL;
	DWORD sizeL, fileL;		// ファイルの大きさ
	DWORD off = 0x40;

	thprintf(filename, TSIZEOF(filename), T("%s:\5%s"), name, type);
	if ( !LoadFileImage(filename, 0x40, &Summary, &sizeL, &fileL) ){
		while( off < fileL ){
			if ( *(DWORD *)(Summary + off) == 0x1e ){
				DWORD size;

				size = *(DWORD *)(Summary + off + 4);
				if (size){
					tstrcat(text, T("\r\nSummary       :"));
					text += tstrlen(text);
					#ifdef UNICODE
						AnsiToUnicode((Summary + off + 8), text, 200);
					#else
						strcpy(text, (Summary + off + 8));
					#endif
				}
				off += size + 1 + 4;
			}
			if ( *(DWORD *)(Summary + off) == 0x1f ){
				DWORD size;

				size = *(DWORD *)(Summary + off + 4);
				if (size){
					tstrcat(text, T("\r\nSummaryU      :"));
					text += tstrlen(text);
					#ifdef UNICODE
						tstrcpy(text, (TCHAR *)(Summary + off + 8));
					#else
						UnicodeToAnsi((WCHAR *)(Summary + off + 8), text, 200);
					#endif
				}
				off += (size * 2) + 4;
			}
			off = (off + 4) & ~(4 - 1);
		}
		ProcHeapFree(Summary);
	}
}

TCHAR *DumpSID(TCHAR *dst, PSID psid, TCHAR *mes, const TCHAR *name)
{
	TCHAR fname[VFPS], oname[0x400], domain[0x400], *db = NULL;
	DWORD namesize, domainsize;
	SID_NAME_USE snu;

	namesize = TSIZEOF(oname);
	domainsize = TSIZEOF(domain);

	if ( name[0] == '\\' ){
		tstrcpy(fname, name);
		db = FindPathSeparator(fname + 2);
		if ( db != NULL ) *db = '\0';
		db = fname;
	}

	if ( IsTrue(LookupAccountSid(db, psid,
			oname, &namesize, domain, &domainsize, &snu)) ){
		return thprintf(dst, 512, T("\r\n%s:%s@%s"), mes, oname, domain);
	}else{
		return thprintf(dst, 512, T("\r\n%s:(error)"), mes);
	}
}

struct ACCESSNAMES {
	DWORD flag;
	TCHAR *name;
};
struct ACCESSNAMES fileaccess[] = {
						 //FILE_LIST_DIRECTORY
	{ FILE_READ_DATA, T("Read/List,") },
						// FILE_ADD_FILE
	{ FILE_WRITE_DATA, T("Write/AddFile,")},
						// FILE_ADD_SUBDIRECTORY,FILE_CREATE_PIPE_INSTANCE
	{ FILE_APPEND_DATA ,T("Add,")},
	{ FILE_READ_EA, T("ReadEA,")},
	{ FILE_WRITE_EA, T("WriteEA,")},
	{ FILE_EXECUTE, T("Execute/Traverse,")}, //FILE_TRAVERSE
	{ FILE_DELETE_CHILD, T("DeleteChild,")},
	{ FILE_READ_ATTRIBUTES, T("ReadAttributes,")},
	{ FILE_WRITE_ATTRIBUTES, T("WriteAttributs,")},

	{ DELETE, T("Delete,") },
	{ READ_CONTROL, T("ReadControl,")},
	{ WRITE_DAC, T("WriteDAC,")},
	{ WRITE_OWNER, T("WriteOwner,")},
	{ SYNCHRONIZE, T("Synchronize,")},
	{ ACCESS_SYSTEM_SECURITY,T("SystemACL,")},
	{ MAXIMUM_ALLOWED,T("AllAccess,")},
	{ GENERIC_ALL, T("GenericAll,")},
	{ GENERIC_EXECUTE, T("GenericExecute,")},
	{ GENERIC_WRITE, T("GenericWrite,")},
	{ GENERIC_READ, T("GenericRead,")},
	{ 0, NULL }
};

TCHAR *DumpAMask(TCHAR *dst, ACCESS_MASK am)
{
	dst = thprintf(dst, 32, T("\r\n (%04x),"), am);

	if ( am == FILE_ALL_ACCESS ){
		dst = thprintf(dst, 32, T("(All)"));
	}else if ( am == FILE_GENERIC_READ ){
		dst = thprintf(dst, 32, T("(Read)"));
	}else if ( am == FILE_GENERIC_WRITE ){
		dst = thprintf(dst, 32, T("(Write)"));
	}else if ( am == FILE_GENERIC_EXECUTE ){
		dst = thprintf(dst, 32, T("(Execute)"));
	}else if ( am == (FILE_GENERIC_READ | FILE_GENERIC_EXECUTE) ){
		dst = thprintf(dst, 32, T("(Read & Execute)"));
	}else if ( am == 0x1301bf ){
		dst = thprintf(dst, 32, T("(Change)"));
	}else{
		struct ACCESSNAMES *an;

		an = fileaccess;
		while ( an->flag ){
			if ( am & an->flag ){
				dst = thprintf(dst, 32, an->name);
			}
			an++;
		}
	}
	return dst;
}

#ifndef IO_REPARSE_TAG_HSM
#define IO_REPARSE_TAG_HSM	0xC0000004
#define IO_REPARSE_TAG_SIS	0x80000007 // シングル インスタンス ストレージ
#endif

#ifndef IO_REPARSE_TAG_DFS
#define IO_REPARSE_TAG_DFS	0x8000000A
#endif

#ifndef IO_REPARSE_TAG_HSM2
#define IO_REPARSE_TAG_HSM2	0x80000006 //  HD と Tape/MO との組み合わせ
#define IO_REPARSE_TAG_WIM	0x80000008
#define IO_REPARSE_TAG_CSV	0x80000009
#endif

#ifndef IO_REPARSE_TAG_DFSR
#define IO_REPARSE_TAG_DFSR	0x80000012
#endif

#ifndef IO_REPARSE_TAG_DEDUP
#define IO_REPARSE_TAG_DEDUP	0x80000013
#define IO_REPARSE_TAG_NFS		0x80000014
#endif

#define ReparseNamesMax 0x27
const TCHAR *ReparseNames[ReparseNamesMax + 1] = {
	T("Reserved"), T("Reserved"), T("Reserved"),
	T("Junction/Volume Mount Point"), // IO_REPARSE_TAG_MOUNT_POINT
	T("Hierarchical Storage Management"), // IO_REPARSE_TAG_HSM
	T("Home server drive"), // IO_REPARSE_TAG_DRIVE_EXTENDER
	T("Hierarchical Storage Management"), // IO_REPARSE_TAG_HSM2
	T("Single Instance Storage"), // IO_REPARSE_TAG_SIS
// 0x08
	T("Windows Imaging Format"), // IO_REPARSE_TAG_WIM
	T("Cluster Shared Volumes"), // IO_REPARSE_TAG_CSV
	T("Distributed File System"), // IO_REPARSE_TAG_DFS
	T("Filter manager"), // IO_REPARSE_TAG_FILTER_MANAGER
	T("Symbolic link"), // IO_REPARSE_TAG_SYMLINK
	T("unknown"),
	T("unknown"),
	T("unknown"),
// 0x10
	T("IIS caching"), // IO_REPARSE_TAG_IIS_CACHE
	T("unknown"),
	T("DFS Replication"), // IO_REPARSE_TAG_DFSR
	T("DEDuplication"), // IO_REPARSE_TAG_DEDUP
	T("Network File System"), // IO_REPARSE_TAG_NFS
	T("Placeholder"), // IO_REPARSE_TAG_FILE_PLACEHOLDER
	T("DFM"), // IO_REPARSE_TAG_DFM
	T("WOF"), // IO_REPARSE_TAG_WOF
// 0x18
	T("WCI"), // IO_REPARSE_TAG_WCI
	T("NPFS"), // IO_REPARSE_TAG_GLOBAL_REPARSE
	T("Cloud"), // IO_REPARSE_TAG_CLOUD
	T("UWP AppExec"), // IO_REPARSE_TAG_APPEXECLINK
	T("PROJFS"), // IO_REPARSE_TAG_PROJFS
	T("WSL symbolic link"), // IO_REPARSE_TAG_LX_SYMLINK
	T("AFS"), // IO_REPARSE_TAG_STORAGE_SYNC
	T("WCI_TOMBS"), // IO_REPARSE_TAG_WCI_TOMBSTONE
// 0x20
	T("WCI"), // IO_REPARSE_TAG_UNHANDLED
	T("onedrive"), // IO_REPARSE_TAG_ONEDRIVE (not used)
	T("PROJFS"), // IO_REPARSE_TAG_PROJFS_TOMBSTONE
	T("WSL AF_UNIX"), // IO_REPARSE_TAG_AF_UNIX
	T("WSL LX_FIFO"), // IO_REPARSE_TAG_LX_FIFO
	T("WSL LX_CHR"), // IO_REPARSE_TAG_LX_CHR
	T("WSL LX_BLK"), // IO_REPARSE_TAG_LX_BLK
	T("WCI link"), // IO_REPARSE_TAG_WCI_LINK
};

TCHAR *DumpReparsePoint(TCHAR *dst, LPCTSTR lpFileName)
{
	TCHAR rpath[VFPS];
	const TCHAR *tagname;
	DWORD tag = GetReparsePath(lpFileName, rpath);

	if ( (tag & 0xff) <= ReparseNamesMax ){
		tagname = ReparseNames[tag & 0xff];
	}else{
		tagname = T("unknown");
	}
	return thprintf(dst, VFPS,
			T("\r\nReparseTag    :%08x (%s)\r\n")
			T("ReparsePath   :%s"),
			tag, tagname,
			rpath);
}

const TCHAR strMSNET[] = T("MS network");
const TCHAR strSMB[] = T("SMB/LANMAN");
const TCHAR strNETWARE[] = T("NETWARE");
const TCHAR strSUNNFS[] = T("NFS(SUN)");
const TCHAR strFTPNFS[] = T("NFS(FTP)");
const TCHAR strDAV[] = T("DAV");
const TCHAR strTERMSRV[] = T("RDP");
const TCHAR strDFS[] = T("DFS");
const TCHAR strWEB[] = T("Drive On Web");
const TCHAR strVMWARE[] = T("VMWARE");
const TCHAR strRSFX[] = T("RSFX");
const TCHAR strMFILES[] = T("MFILES");
const TCHAR strMSNFS[] = T("NFS(MS)");
const TCHAR strGOOGLE[] = T("Google");
const TCHAR strNDFS[] = T("NDFS");
const TCHAR strWSLFS[] = T("WSLFS");

const TCHAR *RemoteTypeName(ULONG type, TCHAR *dst)
{
	const TCHAR *str;

	switch(type){
		case 0x00010000: // WNNC_NET_MSNET
			str = strMSNET;
			break;

		case 0x00020000: // WNNC_NET_SMB
			str = strSMB;
			break;

		case 0x00030000: // WNNC_NET_NETWARE
			str = strNETWARE;
			break;

		case 0x00070000: // WNNC_NET_SUN_PC_NFS
			str = strSUNNFS;
			break;

		case 0x000C0000: // WNNC_NET_FTP_NFS
			str = strFTPNFS;
			break;

		case 0x002E0000: // WNNC_NET_DAV
			str = strDAV;
			break;

		case 0x00360000: // WNNC_NET_TERMSRV
			str = strTERMSRV;
			break;

		case 0x003B0000: // WNNC_NET_DFS
			str = strDFS;
			break;

		case 0x003E0000: // WNNC_NET_DRIVEONWEB
			str = strWEB;
			break;

		case 0x003F0000: // WNNC_NET_VMWARE
			str = strVMWARE;
			break;

		case 0x00400000: // WNNC_NET_RSFX
			str = strRSFX;
			break;

		case 0x00410000: // WNNC_NET_MFILES
			str = strMFILES;
			break;

		case 0x00420000: // WNNC_NET_MS_NFS
			str = strMSNFS;
			break;

		case 0x00430000: // WNNC_NET_GOOGLE
			str = strGOOGLE;
			break;

		case 0x00440000: // WNNC_NET_NDFS
			str = strNDFS;
			break;

		case 0x00480000: //
			str = strWSLFS;
			break;

		default:
			thprintf(dst, 64, T("unknown(%x)"), type);
			return dst;
	}
	return str;
}

const TCHAR *RemoteFlagsName(ULONG flags, TCHAR *dstptr)
{
	TCHAR *dst = dstptr;

	*dst = '\0';
	if ( flags & 1 ) dst = tstpcpy(dst, T("loopback,")); // REMOTE_PROTOCOL_FLAG_LOOPBACK
	if ( flags & 2 ) dst = tstpcpy(dst, T("offline cache,")); // REMOTE_PROTOCOL_FLAG_OFFLINE
	if ( flags & 4 ) dst = tstpcpy(dst, T("persistent handle,")); // REMOTE_PROTOCOL_INFO_FLAG_PERSISTENT_HANDLE
	if ( flags & 8 ) dst = tstpcpy(dst, T("privacy,")); // REMOTE_PROTOCOL_INFO_FLAG_PRIVACY
	if ( flags & 16 ) dst = tstpcpy(dst, T("data is signed,")); // REMOTE_PROTOCOL_INFO_FLAG_INTEGRITY
	if ( flags & 32 ) dst = tstpcpy(dst, T("mutual authentication,")); // REMOTE_PROTOCOL_INFO_FLAG_MUTUAL_AUTH
	if ( flags & 0xffc0 ) thprintf(dst, 32, T("unknown 0x%x"), flags);
	return dstptr;
}

#if 0

DefineWinAPI(BOOL, GetVolumePathNameW, (LPCWSTR, LPWSTR, DWORD) );
DefineWinAPI(BOOL, GetVolumeNameForVolumeMountPointW, (LPCWSTR, LPWSTR, DWORD) );

	{
		WCHAR filePath[VFPS],volumePath[MAX_PATH];
		WCHAR volumeName[MAX_PATH];
		HMODULE hKernel32 = GetModuleHandle(StrKernel32DLL);

		GETDLLPROC(hKernel32, GetVolumePathNameW);
		GETDLLPROC(hKernel32, GetVolumeNameForVolumeMountPointW);

		strcpyToW(filePath,name,VFPS);
		if ( !DGetVolumePathNameW(filePath, volumePath, TSIZEOFW(volumePath)) ){
			MessageA("GetVolumePathName error");
		}else{
			MessageW(volumePath);
			if ( !DGetVolumeNameForVolumeMountPointW(volumePath,
								volumeName, TSIZEOFW(volumeName)) ) {
				// RAMDISK等 IMDISK , ネットワークドライブ
				MessageA("GetVolumeNameForVolumeMountPoint error");
			}else{
				int length = strlenW(volumeName);
				HANDLE hDF;
				MessageW(volumeName);
				if (length && volumeName[length - 1] == L'\\'){
					volumeName[length - 1] = L'\0';
				}
				hDF= CreateFileW(volumeName, FILE_READ_ATTRIBUTES,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
				if ( hDF == INVALID_HANDLE_VALUE ){
				MessageA("CreateFileW error");
				}else{
					VOLUME_DISK_EXTENTS extents;
					STORAGE_PROPERTY_QUERY query;
					DWORD bytesWritten;
					DEVICE_SEEK_PENALTY_DESCRIPTOR result;

					 if (!DeviceIoControl(hDF, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
						NULL, 0, &extents, sizeof(extents), &bytesWritten, NULL) ){
					// 仮想ドライブ iso
					Messagef("DeviceIoControl error %d",GetLastError());
					}else{
						XMessage(NULL, NULL, XM_DbgDIA, T("\\\\.\\PhysicalDrive%u"),
							 extents.Extents[0].DiskNumber);
					}

					query.PropertyId = 7;//StorageDeviceSeekPenaltyProperty;
					query.QueryType = 0; // PropertyStandardQuery;

					if ( DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
						&query, sizeof(query), &result, sizeof(result),
						  &bytesWritten, NULL) &&
						(bytesWritten >= sizeof(STORAGE_PROPERTY_QUERY))

						) {
						Messagef("HD %d",	result.IncursSeekPenalty);
					}
					CloseHandle(hDF);
				}
			}
		}
	}
#endif


void MakeFileInformation(PPC_APPINFO *cinfo, ThSTRUCT *text, ENTRYCELL *cell)
{
	TCHAR name[VFPS], ext[VFPS];
	VFSFILETYPE vft;
	ERRORCODE err;
	TCHAR *dst;
	UINTHL hl;

	ThInit(text);

	VFSFullPath(name, (TCHAR *)GetCellFileName(cinfo, cell, name),
			(cinfo->RealPath[0] != '?') ? cinfo->RealPath : cinfo->path);
	if ( cinfo->e.Dtype.mode == VFSDT_SHN ){
		if ( IsTrue(VFSGetRealPath(cinfo->info.hWnd, ext, name)) ){
			tstrcpy(name, ext);
		}
	}
	LetHLFilesize(hl, cell->f);
	ThSize(text, 0x2000);

	thprintf(text, 0,
			T("Path          :%s\r\n")
			T("Long  name    :%s\r\n")
			T("Short name    :%s\r\n")
			T("Full path name:%s\r\n")
			T("Size          :%'Lu\r\n")
			T("Internal Type :"),
			cinfo->path,
			(cell->f.cFileName[0] != FINDOPTION_LONGNAME) ? cell->f.cFileName :
				(TCHAR *)EntryExtData_GetDATAptr(cinfo, DFC_LONGNAME, cell),
			cell->f.cAlternateFileName,
			name,
			hl.rawdata);

	vft.flags = VFSFT_TYPE | VFSFT_TYPETEXT | VFSFT_EXT | VFSFT_INFO | VFSFT_BIGBUF;
	err = VFSGetFileType(name, NULL, 0, &vft);
	if ( err == ERROR_NO_DATA_DETECTED ){
		ThCatString(text, T("Unknown File Type"));
	}else if ( err != NO_ERROR ){
		thprintf(text, 0, T("%Mm"), err);
	}else{
		thprintf(text, 0, T("%s(%s,*.%s)"),
				vft.typetext, vft.type, vft.ext);
		if ( vft.info != NULL ){
			thprintf(text, 0, T("\r\n"));
			if ( tstrlen(vft.info) > 0x1000 ){
				thprintf(text, 0, T("*info too large*"));
			}else{
				ThCatString(text, vft.info);
			}
			ProcHeapFree(vft.info);
		}
	}

	ThSize(text, 0x2000);
	dst = ThStrLastT(text);

	if ( VFSCheckFileByExt(name, ext) == FALSE ){
		tstrcpy(ext, T("Not registration"));
	}
	{
		DWORD state;

		state = cell->state;
		if ( cell->attr & ECA_GRAY ) state = ECS_GRAY;
		dst = thprintf(dst, CMDLINESIZE,
			T("\r\n")
			T("Extension type:%s\r\n")
			T("Attributes    :(%x),%s\r\n "),
			ext,
			cell->f.dwFileAttributes, status[state]);
		if ( cell->f.dwFileAttributes == BADATTR ){
			dst = thprintf(dst, 32, T("attribute error"));
		}else{
			int i, usesep = 0;

			for ( i = 0 ; i < ATTR_FLAGS_COUNT ; i++){
				if ( cell->f.dwFileAttributes & ( 1 << i ) ){
					if ( usesep != 0 ){
						tstrcat(dst, T(","));
					}else{
						usesep = 1;
					}
					tstrcat(dst, AttrsLabels[i]);
				}
			}
			dst += tstrlen(dst);

			if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED ){
				UINTHL chl;

				chl.s.L = GetCompressedFileSize(name, &chl.s.H);
				dst = thprintf(dst, 64, T("\r\nCompressedSize:%'Lu"), chl.rawdata);
			}
		}
		if ( cell->f.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ){
			dst = DumpReparsePoint(dst, name);
		}
	}

	dst = thprintf(dst, 300, T("\r\nCreate time   :%MF")
							 T("\r\nLast Write    :%MF")
							 T("\r\nLast Access   :%MF"),
			&cell->f.ftCreationTime , &cell->f.ftLastWriteTime,
			&cell->f.ftLastAccessTime);
	{
		HANDLE hFile;
		BY_HANDLE_FILE_INFORMATION fi;

		hFile = CreateFileL(name, GENERIC_READ, FILE_SHARE_READ, NULL,
				OPEN_EXISTING,
				(cell->f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
					FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,
				NULL);
		if ( hFile != INVALID_HANDLE_VALUE ){
			LPVOID context = NULL;
			HMODULE hKernel32 = GetModuleHandle(StrKernel32DLL);

			GETDLLPROC(hKernel32, GetFileInformationByHandleEx);
			if ( DGetFileInformationByHandleEx != NULL ){
				BYTE buf[0x400];
				if ( IsTrue(DGetFileInformationByHandleEx(hFile,
						xFileBasicInfo, &buf, sizeof(buf))) ){
					dst = thprintf(dst, 100, T("\r\nChange Time   :%MF"),
							&((xFILE_BASIC_INFO *)buf)->ChangeTime);
				}
				if ( IsTrue(DGetFileInformationByHandleEx(hFile,
						xFileStandardInfo, &buf, sizeof(buf))) ){
					dst = thprintf(dst, 100, T("\r\nAllocationSize:%'Lu"),
							((UINTHL *)(&((xFILE_STANDARD_INFO *)buf)->AllocationSize))->rawdata);
				}
				if ( IsTrue(DGetFileInformationByHandleEx(hFile,
						xFileRemoteProtocolInfo, &buf, sizeof(buf))) ){
					TCHAR buf1[64], buf2[256];

					dst = thprintf(dst, CMDLINESIZE, T("\r\nRemote Protocol name:%s\r\nRemote Protocol types:%s"),
						RemoteTypeName(((xFILE_REMOTE_PROTOCOL_INFO *)buf)->Protocol, buf1),
						RemoteFlagsName(((xFILE_REMOTE_PROTOCOL_INFO *)buf)->Flags, buf2)
					);
				}
			}
			text->top = (char *)dst - text->bottom;
			ThSize(text, 0x2000);
			dst = (TCHAR *)(char *)(text->bottom + text->top);

			if ( IsTrue(GetFileInformationByHandle(hFile, &fi)) ){
				DefineWinAPI(HANDLE, FindFirstFileNameW, (LPCWSTR lpFileName, DWORD dwFlags, LPDWORD StringLength, PWCHAR LinkName));
				DefineWinAPI(BOOL, FindNextFileNameW, (HANDLE hFindStream, LPDWORD StringLength, PWCHAR LinkName));

				dst = thprintf(dst, 64, T("\r\nHard Links    :%d"),
						fi.nNumberOfLinks);
				if ( fi.nNumberOfLinks >= 2){
					GETDLLPROC(hKernel32, FindFirstFileNameW);
					GETDLLPROC(hKernel32, FindNextFileNameW);
					if ( DFindFirstFileNameW != NULL ){
						HANDLE hFFFN;
						WCHAR linknameW[VFPS];
						DWORD len;
						#ifdef UNICODE
							#define nameW name
						#else
							char nameT[VFPS];
							WCHAR nameW[VFPS];

							AnsiToUnicode(name, nameW, VFPS);
						#endif
						len = VFPS;
						hFFFN = DFindFirstFileNameW(nameW, 0, &len, linknameW);
						if ( hFFFN != INVALID_HANDLE_VALUE ) for(;;){
							#ifdef UNICODE
								#define nameT linknameW
							#else
								UnicodeToAnsi(linknameW, nameT, VFPS);
							#endif
							dst = thprintf(dst, VFPS, T("\r\n*HardLink name:%s"), nameT);
							len = VFPS;
							if ( FALSE == DFindNextFileNameW(hFFFN, &len, linknameW) ){
								FindClose(hFFFN);
								break;
							}
						}
					}
				}
			}
			{
				WIN32_STREAM_ID stid;
				DWORD size, reads;
				WCHAR wname[VFPS];
				TCHAR *baseptr;

				for ( ; ; ){
					size = (LPBYTE)&stid.cStreamName - (LPBYTE)&stid;
					if ( FALSE == BackupRead(hFile, (LPBYTE)&stid,
							size, &reads, FALSE, FALSE, &context) ){
						break;
					}
					if ( reads < size ) break;
					if ( FALSE == BackupRead(hFile, (LPBYTE)&wname,
							stid.dwStreamNameSize,
							&reads, FALSE, FALSE, &context) ){
						break;
					}
					if ( reads < stid.dwStreamNameSize ) break;
					baseptr = dst;
					dst = tstpcpy(dst, T("\r\n*Stream Name  :"));
					if ( stid.dwStreamNameSize ){
						wname[stid.dwStreamNameSize >> 1] = 0;
						#ifdef UNICODE
							tstrcpy(dst, wname);
						#else
							UnicodeToAnsi(wname, dst, VFPS);
						#endif
					}else{
						tstrcpy(dst, T("(none)"));
					}
					dst += tstrlen(dst);
					dst = tstpcpy(dst, T("\r\n  Stream Type :"));
					switch ( stid.dwStreamId ){
						case BACKUP_DATA: // $DATA
							dst = baseptr;
							*dst = '\0';
							break;
						case BACKUP_EA_DATA: // $EA
							if ( (*(DWORD *)(&stid.Size) == 0x48) && (*((DWORD *)(&stid.Size) + 1) == 0) ){
								GetLxssEA(hFile, &context, &dst);
								continue;
							}
							tstrcpy(dst, T("Enhanced attributes"));
							break;
						case BACKUP_SECURITY_DATA: // $SECURITY_DESCRIPTOR
							tstrcpy(dst, T("Securities"));
							break;
						case BACKUP_ALTERNATE_DATA:
							tstrcpy(dst, T("Alternate data"));
							break;
						case BACKUP_LINK: // $FILE_NAME
							tstrcpy(dst, T("Hard Link"));
							break;
						case BACKUP_PROPERTY: // BACKUP_PROPERTY_DATA
							tstrcpy(dst, T("Properties"));
							break;
						case BACKUP_OBJECT_ID: // $OBJECT_ID
							tstrcpy(dst, T("Object ID"));
							break;
						case BACKUP_REPARSE_DATA: // $REPARSE_POINT
							tstrcpy(dst, T("Reparse DATA"));
							break;
						case BACKUP_SPARSE_BLOCK:
							tstrcpy(dst, T("Sparse blocks"));
							break;
						#ifndef BACKUP_TXFS_DATA
						  #define BACKUP_TXFS_DATA 0xa
						#endif
						case BACKUP_TXFS_DATA: // $TXF_DATA
							tstrcpy(dst, T("Transactional NTFS (TxF)"));
							break;
						#ifndef BACKUP_GHOSTED_FILE_EXTENTS
						  #define BACKUP_GHOSTED_FILE_EXTENTS 0xb
						#endif
						case BACKUP_GHOSTED_FILE_EXTENTS:
							tstrcpy(dst, T("Ghosted file extent"));
							break;
						default:
							thprintf(dst, 32, T("Unknown(%x)"), stid.dwStreamId);
							break;
					}
					if ( *dst != '\0' ){
						dst += tstrlen(dst);
						dst = thprintf(dst, 100, T("\r\n  Stream Size :%'Lu"),
								((UINTHL *)&stid.Size)->rawdata);
					}

					if ( FALSE == BackupSeek(hFile,
								*(DWORD *)(&stid.Size),
								*((DWORD *)(&stid.Size) + 1),
								&reads, &reads, &context) ){
						break;
					}
				}
				if ( context != NULL ){
					BackupRead(hFile, (LPBYTE)&stid, 0, &reads, TRUE, FALSE, &context);
				}
			}
			CloseHandle(hFile);
		}
	}
	if ( tstricmp(GetPathExt(name), StrShortcutExt) == 0 ){
		TCHAR *np;

		np = tstpcpy(dst, T("\r\nLinked path   :"));
		if ( SUCCEEDED(GetLink(cinfo->info.hWnd, name, np)) ){
			dst = np + tstrlen(np);
		}
	}

	if ( cell->comment != EC_NOCOMMENT ){
		dst = tstpcpy(dst, T("\r\nComment       :"));
		dst = tstpcpy(dst, ThPointerT(&cinfo->e.Comments, cell->comment));
	}
	if ( cell->cellcolumn >= 0 ){
		ENTRYEXTDATASTRUCT eeds;
		int CommentID;

		for ( CommentID = 1 ; CommentID <= 10 ; CommentID++ ){
			eeds.id = (DWORD)(DFC_COMMENTEX_MAX - (CommentID - 1) );
			eeds.size = VFPS * sizeof(TCHAR);
			eeds.data = (BYTE *)ext;
			if ( IsTrue(EntryExtData_GetDATA(cinfo, &eeds, cell)) ){
				dst = thprintf(dst, CMDLINESIZE, T("\r\nComment%d      :%s"), CommentID, ext);
			}
		}
	}
	{
		SECURITY_DESCRIPTOR *sd;
		BYTE sdbuf[0x400];
		DWORD size;

		sd = (SECURITY_DESCRIPTOR *)sdbuf;
		if ( FALSE != GetFileSecurity(
				name, OWNER_SECURITY_INFORMATION, sd, sizeof sdbuf, &size) ){
			PSID psid;
			BOOL ownflag;

			GetSecurityDescriptorOwner(sd, &psid, &ownflag);

			if ( IsTrue(ownflag) ){
				dst = thprintf(dst, 64, T("\r\nOwner         :<inheritance>"));
			}else{
				dst = DumpSID(dst, psid, T("Owner         "), name);
			}
		}
	}
	{
		SECURITY_DESCRIPTOR *sd;
		BYTE sdbuf[0x400];
		DWORD size;

		sd = (SECURITY_DESCRIPTOR *)sdbuf;
		if ( FALSE != GetFileSecurity(name, DACL_SECURITY_INFORMATION,
				sd, sizeof sdbuf, &size) ){
			PACL pacl;
			BOOL have, def;

			GetSecurityDescriptorDacl(sd, &have, &pacl, &def);
			if ( IsTrue(have) ){
				int index = 0;
				ACE_HEADER *ace;

				if ( pacl == NULL ){
					dst = thprintf(dst, 32, T("\r\n*Access All"));
				}else while( IsTrue(GetAce(pacl, index++, (void **)&ace)) ){
					switch ( ace->AceType ){
						case ACCESS_ALLOWED_ACE_TYPE:
							dst = DumpSID(dst,
								(PSID)&((ACCESS_ALLOWED_ACE *)ace)->SidStart,
								T("*Access Allow."), name);
							dst = DumpAMask(dst,
										((ACCESS_ALLOWED_ACE *)ace)->Mask);
							break;
						case ACCESS_DENIED_ACE_TYPE:
							dst = DumpSID(dst,
								(PSID)&((ACCESS_DENIED_ACE *)ace)->SidStart,
								T("*Access Denied"), name);
							dst = DumpAMask(dst,
										((ACCESS_ALLOWED_ACE *)ace)->Mask);
							break;
						case SYSTEM_AUDIT_ACE_TYPE:
							dst = DumpSID(dst,
								(PSID)&((SYSTEM_AUDIT_ACE *)ace)->SidStart,
								T("*Audit        "), name);
							dst = DumpAMask(dst,
										((ACCESS_ALLOWED_ACE *)ace)->Mask);
							break;
						default:
							dst = thprintf(dst, 32, T("\r\n*Access Unknown:"));
					}
				}
			}
		}
	}
	dst = GetColumnExtTextInfo(cinfo, cell->cellcolumn, dst);
	if ( tstrlen(name) < VFPS ){
		LoadSummary(name, T("SummaryInformation"), dst);
		LoadSummary(name, T("DocumentSummaryInformation"), dst);
		LoadSummary(name, T("SebiesnrMkudrfcoIaamtykdDa"), dst);
	}
	dst += tstrlen(dst);
	*dst++ = '\r';
	*dst++ = '\n';
	*dst = '\0';
	text->top = (char *)dst - text->bottom;
	VistaProperties(name, text);
}
