/*-----------------------------------------------------------------------------
	Paper Plane cUI								各種ダイアログボックス
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "WINAPIIO.H"
#include "VFS.H"
#include "PPX_64.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#pragma hdrstop

UINT CWIN_JXX_GROUP[] = {IDX_CWIN_DBL, IDX_CWIN_DAC, 0};
UINT CWIN_JLR_GROUP[] = {IDX_CWIN_JUD, IDX_CWIN_JP, 0};
UINT CWIN_JUD_GROUP[] = {IDX_CWIN_JLR, IDX_CWIN_JP, 0};
UINT CWIN_JP_GROUP[] = {IDX_CWIN_JLR, IDX_CWIN_JUD, 0};
UINT CWIN_FP_GROUP[] = {IDX_CWIN_FUD, 0};
UINT CWIN_FUD_GROUP[] = {IDX_CWIN_FP, 0};

UINT CWIN_CMB_GROUP[] = {IDX_CWIN_JLR, IDX_CWIN_JUD, IDX_CWIN_JP, IDX_CWIN_JF, IDX_CWIN_FP, IDX_CWIN_FUD, IDX_CWIN_DBL, IDX_CWIN_CLO, IDX_CWIN_DAC, 0};

const TCHAR *SortItems[] = {
	T("SORG|No sort"),			// -1

	T("SRTN|N Name"),			// 0
	T("SRTE|E Ext"),			// 1
	T("SRTS|S file Size"),		// 2
	T("SRTT|T Time stamp"),		// 3
	T("SRTC|C Create time"),	// 4
	T("SRTA|A Access time"),	// 5
	T("SRTM|M Mark"),			// 6
	T("SRTH|H Changed"),		// 7

	T("SRTO|O Comment"),		// 17
	T("SRTX|X Ext color"),		// 20

	T("STnR|n R)Name"),			// 8
	T("STeR|e R)Ext"),			// 9
	T("STsR|s R)file Size"),	// 10
	T("STtR|t R)Time stamp"),	// 11
	T("STcR|c R)Create time"),	// 12
	T("STaR|a R)Access time"),	// 13
	T("STmR|m R)Mark"),			// 14
	T("SThR|h R)Changed"),		// 15

	T("SToR|o R)Comment"),		// 18
	T("STxR|x R)Ext color"),	// 21

	T("SRTP|P Plain"),			// 19
	T("SRTR|R Attributes"),		// 16
	T("SRTD|D Directory"),		// 22
	T("STdR|d R)Directory"),	// 23
	NULL
};
char sortIDtable[] =
	{ -1, 0, 1, 2, 3, 4, 5, 6, 7,		17, 20,
	   8, 9, 10, 11, 12, 13, 14, 15,	18, 21, 19, 16, 22, 23 };

void GetSortSettings(HWND hDlg, XC_SORT *xs);

#ifndef FILE_RETURNS_CLEANUP_RESULT_INFO
#define FILE_RETURNS_CLEANUP_RESULT_INFO	0x00000200
#define FILE_SUPPORTS_POSIX_UNLINK_RENAME	0x00000400
#define FILE_SUPPORTS_GHOSTING				0x40000000
#endif

#ifndef FILE_READ_ONLY_VOLUME
#define FILE_READ_ONLY_VOLUME				0x00080000
#endif
#ifndef FILE_SEQUENTIAL_WRITE_ONCE
#define FILE_SEQUENTIAL_WRITE_ONCE			0x00100000
#define FILE_SUPPORTS_TRANSACTIONS			0x00200000
#endif
#ifndef FILE_SUPPORTS_HARD_LINKS
#define FILE_SUPPORTS_HARD_LINKS			0x00400000
#define FILE_SUPPORTS_EXTENDED_ATTRIBUTES	0x00800000
#define FILE_SUPPORTS_OPEN_BY_FILE_ID		0x01000000
#define FILE_SUPPORTS_USN_JOURNAL			0x02000000
#endif
#ifndef FILE_SUPPORTS_INTEGRITY_STREAMS
#define FILE_SUPPORTS_INTEGRITY_STREAMS		0x04000000
#endif
#ifndef FILE_SUPPORTS_BLOCK_REFCOUNTING
#define FILE_SUPPORTS_BLOCK_REFCOUNTING		0x08000000
#endif
#ifndef FILE_SUPPORTS_SPARSE_VDL
#define FILE_SUPPORTS_SPARSE_VDL		    0x10000000
#endif
#ifndef FILE_DAX_VOLUME
#define FILE_DAX_VOLUME					    0x20000000
#endif


struct DISKINFOSTRUCT {
	DWORD flag;
	const TCHAR *name;
} DiskInfoFlags[] = {
// x
	{FS_CASE_SENSITIVE,			MES_7861}, // FILE_CASE_SENSITIVE_SEARCH
	{FS_CASE_IS_PRESERVED,		MES_7860}, // FILE_CASE_PRESERVED_NAMES
	{FS_UNICODE_STORED_ON_DISK,	MES_7862}, // FILE_UNICODE_ON_DISK
	{FS_PERSISTENT_ACLS,		MES_7863}, // FILE_PERSISTENT_ACLS
// x0
	{FS_FILE_COMPRESSION,		MES_7864}, // FILE_FILE_COMPRESSION
	{FILE_VOLUME_QUOTAS,		MES_7867},
	{FILE_SUPPORTS_SPARSE_FILES, MES_7868},
	{FILE_SUPPORTS_REPARSE_POINTS, MES_7869},
// x00
	{FILE_SUPPORTS_REMOTE_STORAGE, MES_786A},
	{FILE_RETURNS_CLEANUP_RESULT_INFO, MES_7878},
	{FILE_SUPPORTS_POSIX_UNLINK_RENAME, MES_7879},
// x000
	{FS_VOL_IS_COMPRESSED,		MES_7865}, // FILE_VOLUME_IS_COMPRESSED
// x0000
	{FILE_SUPPORTS_OBJECT_IDS,	MES_786B},
	{FS_FILE_ENCRYPTION,		MES_786C}, // FILE_SUPPORTS_ENCRYPTION
	{FILE_NAMED_STREAMS,		MES_7866},
	{FILE_READ_ONLY_VOLUME,		MES_786D},
// x00000
	{FILE_SEQUENTIAL_WRITE_ONCE,	MES_786E},
	{FILE_SUPPORTS_TRANSACTIONS,	MES_786F},
	{FILE_SUPPORTS_HARD_LINKS,		MES_7870}, // Win7
	{FILE_SUPPORTS_EXTENDED_ATTRIBUTES,	MES_7871}, // Win7
// x000000
	{FILE_SUPPORTS_OPEN_BY_FILE_ID,	MES_7872}, // Win7
	{FILE_SUPPORTS_USN_JOURNAL,		MES_7873}, // Win7
	{FILE_SUPPORTS_INTEGRITY_STREAMS,	MES_7874},
	{FILE_SUPPORTS_BLOCK_REFCOUNTING,	MES_7875},
// x0000000
	{FILE_SUPPORTS_SPARSE_VDL,	MES_7876},
	{FILE_DAX_VOLUME,			MES_7877}, // Win10 1607
	{FILE_SUPPORTS_GHOSTING,	MES_787A},
	{0, 0}
};
const TCHAR Disktype_Remote[] = MES_DIRE;
const TCHAR Disktype_CDROM[] = MES_DICD;
const TCHAR Disktype_RAMDISK[] = MES_DIRA;
const TCHAR Disktype_REMOVABLE[] = MES_DIRM;
const TCHAR Disktype_FIXED[] = MES_DIFI;

#if !NODLL
void EnableDlgWindow(HWND hDlg, int id, BOOL state)
{
	HWND hControlWnd;

	hControlWnd = GetDlgItem(hDlg, id);
	if ( hControlWnd == NULL ) return;
	EnableWindow(hControlWnd, state);
}
#endif

void CenterPPcDialog(HWND hDlg, PPC_APPINFO *cinfo)
{
	if ( X_combos[1] & CMBS1_DIALOGNOPANE ){
		CenterWindow(hDlg);
	}else{
		MoveCenterWindow(hDlg, cinfo->info.hWnd);
	}
}

DWORD GetAttibuteSettings(HWND hDlg)
{
	DWORD attr;

	attr =
		(IsDlgButtonChecked(hDlg, IDX_FOP_RONLY) ? FILE_ATTRIBUTE_READONLY : 0) |
		(IsDlgButtonChecked(hDlg, IDX_FOP_HIDE) ?  FILE_ATTRIBUTE_HIDDEN : 0) |
		(IsDlgButtonChecked(hDlg, IDX_FOP_SYSTEM) ? FILE_ATTRIBUTE_SYSTEM : 0) |
		(IsDlgButtonChecked(hDlg, IDX_FOP_ARC) ?   FILE_ATTRIBUTE_ARCHIVE : 0);
	if ( GetDlgItem(hDlg, IDX_FOP_DIR) != NULL ){
		if ( IsDlgButtonChecked(hDlg, IDX_FOP_DIR) ){
			setflag(attr, FILE_ATTRIBUTE_DIRECTORY);
		}
	}
	return attr;
}

void SetAttibuteSettings(HWND hDlg, DWORD attr)
{
	CheckDlgButton(hDlg, IDX_FOP_RONLY,  attr & FILE_ATTRIBUTE_READONLY);
	CheckDlgButton(hDlg, IDX_FOP_HIDE,   attr & FILE_ATTRIBUTE_HIDDEN);
	CheckDlgButton(hDlg, IDX_FOP_SYSTEM, attr & FILE_ATTRIBUTE_SYSTEM);
	CheckDlgButton(hDlg, IDX_FOP_ARC,    attr & FILE_ATTRIBUTE_ARCHIVE);
	if ( GetDlgItem(hDlg, IDX_FOP_DIR) != NULL ){
		CheckDlgButton(hDlg, IDX_FOP_DIR, attr & FILE_ATTRIBUTE_DIRECTORY);
	}
}

// Window Option --------------------------------------------------------------
void SetWindowOption(HWND hDlg)
{
	PPC_APPINFO *cinfo;
	int new_combo, doubleboot;

	cinfo = (PPC_APPINFO *)GetWindowLongPtr(hDlg, DWLP_USER);
	new_combo = IsDlgButtonChecked(hDlg, IDX_CWIN_CMB);
	if ( new_combo != X_combo ){
		SetPopMsg(cinfo, POPMSG_MSG, MES_RRPC);
		SetCustData(T("X_combo"), &new_combo, sizeof(new_combo));
	}
	doubleboot = IsDlgButtonChecked(hDlg, IDX_CWIN_DBL);
	if ( new_combo ){
		int X_mpane[2] = {32, 1};

		GetCustData(T("X_mpane"), X_mpane, sizeof(X_mpane));
		X_mpane[1] = doubleboot ? max(X_mpane[1], 2) : 1;
		SetCustData(T("X_mpane"), X_mpane, sizeof(X_mpane));
		doubleboot = 0;
	}
	cinfo->swin = (cinfo->swin & ~0xffff) |
	 (IsDlgButtonChecked(hDlg, IDX_CWIN_JLR) ? (SWIN_JOIN | SWIN_LRJOIN): 0) |
	 (IsDlgButtonChecked(hDlg, IDX_CWIN_JUD) ? (SWIN_JOIN | SWIN_UDJOIN): 0) |
	 (IsDlgButtonChecked(hDlg, IDX_CWIN_JP)  ? (SWIN_JOIN | SWIN_PILEJOIN): 0) |
	 (IsDlgButtonChecked(hDlg, IDX_CWIN_JF)  ? SWIN_FIT : 0) |
	 (doubleboot							 ? SWIN_WBOOT : 0) |
	 (IsDlgButtonChecked(hDlg, IDX_CWIN_CLO) ? SWIN_WQUIT : 0) |
	 (IsDlgButtonChecked(hDlg, IDX_CWIN_FP ) ? SWIN_FIXACTIVE : 0) |
	 (IsDlgButtonChecked(hDlg, IDX_CWIN_FUD) ? (SWIN_FIXACTIVE | SWIN_FIXPOSIZ) : 0) |
	 (IsDlgButtonChecked(hDlg, IDX_CWIN_DAC) ? SWIN_WACTIVE : 0);
	SendX_win(cinfo);
	if (cinfo->swin & SWIN_JOIN){
		JoinWindow(cinfo);
		if ( !(cinfo->swin & SWIN_WBOOT) ) BootPairPPc(cinfo);
	}
}

void CheckDlgButtonGroup(HWND hDlg, UINT *checkgroup, int Check, int Enable)
{
	while ( *checkgroup ){
		HWND hControlWnd;

		hControlWnd = GetDlgItem(hDlg, *checkgroup++);
		if ( hControlWnd == NULL ) continue;
		if ( Check >= 0 ) SendMessage(hControlWnd, BM_SETCHECK, (WPARAM)Check, 0);
		if ( Enable >= 0 ) EnableWindow(hControlWnd, Enable);
	}
}

BOOL UnCheckFix(HWND hDlg, UINT ID, UINT *checkgroup)
{
	if ( IsDlgButtonChecked(hDlg, ID) == FALSE ) return FALSE;
	CheckDlgButtonGroup(hDlg, checkgroup, FALSE, -1);
	return TRUE;
}

INT_PTR CALLBACK WindowDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg){
		case WM_INITDIALOG: {
			PPC_APPINFO *cinfo;

			cinfo = (PPC_APPINFO *)lParam;
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)cinfo);
			CenterPPcDialog(hDlg, cinfo);
			LocalizeDialogText(hDlg, IDD_CWINDOW);
			CheckDlgButton(hDlg, IDX_CWIN_DBL, cinfo->swin & SWIN_WBOOT);
			CheckDlgButton(hDlg, IDX_CWIN_CLO, cinfo->swin & SWIN_WQUIT);
			if ( X_combo != COMBO_OFF ){
				CheckDlgButton(hDlg, IDX_CWIN_CMB, TRUE);
				CheckDlgButtonGroup(hDlg, CWIN_CMB_GROUP, -1, FALSE);
			}
			CheckDlgButton(hDlg, IDX_CWIN_DAC, cinfo->swin & SWIN_WACTIVE);
			if (cinfo->swin & SWIN_JOIN){
				CheckDlgButton(hDlg,
					IDX_CWIN_JLR + ((cinfo->swin & SWIN_JOINMASK) >> 6), TRUE);
			}
			CheckDlgButton(hDlg, IDX_CWIN_JF, (cinfo->swin & SWIN_FIT));
			if (cinfo->swin & SWIN_FIXACTIVE){
				CheckDlgButton(hDlg, IDX_CWIN_FP, !(cinfo->swin & SWIN_FIXPOSIZ));
				CheckDlgButton(hDlg, IDX_CWIN_FUD, (cinfo->swin & SWIN_FIXPOSIZ));
			}
			return TRUE;
		}
		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDOK:
					SetWindowOption(hDlg);
					EndDialog(hDlg, 1);
					break;

				case IDCANCEL:
					EndDialog(hDlg, 1);
					break;

				case IDX_CWIN_JLR:
					if ( UnCheckFix(hDlg, IDX_CWIN_JLR, CWIN_JLR_GROUP) ){
						CheckDlgButtonGroup(hDlg, CWIN_JXX_GROUP, TRUE, -1);
					}
					break;
				case IDX_CWIN_JUD:
					if ( UnCheckFix(hDlg, IDX_CWIN_JUD, CWIN_JUD_GROUP) ){
						CheckDlgButtonGroup(hDlg, CWIN_JXX_GROUP, TRUE, -1);
					}
					break;
				case IDX_CWIN_JP:
					UnCheckFix(hDlg, IDX_CWIN_JP, CWIN_JP_GROUP);
					break;
				case IDX_CWIN_FP:
					UnCheckFix(hDlg, IDX_CWIN_FP, CWIN_FP_GROUP);
					break;
				case IDX_CWIN_FUD:
					UnCheckFix(hDlg, IDX_CWIN_FUD, CWIN_FUD_GROUP);
					break;
				case IDX_CWIN_CMB:
					if (IsDlgButtonChecked(hDlg, IDX_CWIN_CMB)){
						CheckDlgButtonGroup(hDlg, CWIN_CMB_GROUP, FALSE, FALSE);
					}else{
						CheckDlgButtonGroup(hDlg, CWIN_CMB_GROUP, -1, TRUE);
					}
					break;

				case IDHELP:
					return PPxDialogHelper(hDlg, WM_HELP, wParam, lParam);

				case IDQ_GETDIALOGID:
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)IDD_CWINDOW);
					break;
			}
			break;

		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}

// Disk[I]nfo -----------------------------------------------------------------
#define DDmulDHL(src1, src2, dest) DDmul((src1), (src2), &(dest)->s.L, &(dest)->s.H)

DWORD CalRate(UINTHL *value, UINTHL *total)
{
	DWORD sizeL, sizeH, totalper;

	sizeL = value->u.LowPart;
	sizeH = value->u.HighPart;

	if ( (total->u.LowPart & 0xffe00000) || total->u.HighPart ){
		if ( total->u.HighPart >= 0x20 ){
			if ( sizeH & 0xffffe000 ){ // 64T over
				if ( sizeH & 0xffe00000 ){ // 16P over
					totalper = ((sizeH >> 12) * 1000) / (total->u.HighPart >> 12);
				}else{ // 64T over
					totalper = (sizeH * 1000) / total->u.HighPart;
				}
			}else{ // 0x0004 0000 0000(16GB) ～ 0x3fff ffff ffff(64T)

				totalper =
						( ((((DWORD)sizeH) << 8) | (sizeL >> 24)) * 1000) /
						( ((((DWORD)total->u.HighPart) << 8) |
							(total->u.LowPart >> 24)) );
			}
		}else{	// 0x0040 0000(4M) ～ 0x3 ffff ffff(16G)
			totalper =
					( ((((DWORD)sizeH) << 20) | (sizeL >> 12)) * 1000) /
					( ((((DWORD)total->u.HighPart) << 20) |
						(total->u.LowPart >> 12)) );
		}
	}else{		// 0x0000 0001      ～  0x003f ffff(4M)
		if ( total->u.LowPart ){
			totalper = (sizeL * 1000) / total->u.LowPart;
		}else{	// 0x0000 0000
			totalper = 1000;
		}
	}
	return totalper;
}

void SetDriveSizeItem(HWND hDlg, UINT bytes, UINT rate, UINTHL *value, int rate_value)
{
	TCHAR buf[VFPS];

	FormatNumber(buf, XFN_SEPARATOR, 18, value->u.LowPart, value->u.HighPart);
	SetDlgItemText(hDlg, bytes, buf);

	thprintf(buf, TSIZEOF(buf), T("%d.%d%%"), rate_value / 10, rate_value % 10); // wsprintf
	SetDlgItemText(hDlg, rate, buf);
}

#if !NODLL
DefineWinAPI(BOOL, GetVolumePathNameW, (LPCWSTR, LPWSTR, DWORD) );
DefineWinAPI(BOOL, GetVolumeNameForVolumeMountPointW, (LPCWSTR, LPWSTR, DWORD) );
#else
ExternWinAPI(BOOL, GetVolumePathNameW, (LPCWSTR, LPWSTR, DWORD) );
ExternWinAPI(BOOL, GetVolumeNameForVolumeMountPointW, (LPCWSTR, LPWSTR, DWORD) );
#endif

#define BusTypeName_max 21
const TCHAR *BusTypeName[BusTypeName_max] = {
	T("Unknown"),
	T("SCSI"),
	T("ATAPI"),
	T("ATA"),
	T("IEEE-1394"),
	T("SSA"), // Serial Storage Architecture
	T("Fibre Channel"),
	T("USB"),
	T("RAID"),
	T("iSCSI"),
	T("Serial Attached SCSI"),
	T("SATA"),
	T("SD card"),
	T("MMC card"),
	T("Virtual"),
	T("File Backed Virtual"),
	T("Spaces"), // 記憶域スペース
	T("NVMe"),
	T("SCM"), // Strage Class Memory (DAX)
	T("UFS"), // Universal Flash Storage
	T("NVMe-oF"), // NVMe over Fibre
};

void SetTemperatureInfo(ThSTRUCT *thText, xSTORAGE_TEMPERATURE_DATA_DESCRIPTOR *tdd, const TCHAR *title)
{
	int count;
	thprintf(thText, 0, T("%s:\r\n")
			T("  Critical Temperature: %d\r\n")
			T("  Warning Temperature: %d\r\n"),
			title,
			tdd->CriticalTemperature,
			tdd->WarningTemperature);
	for ( count = 0; count < tdd->InfoCount; count++ ){
		thprintf(thText, 0, T("  Temperature #%d: %d\r\n"),
//				T("Temperature #%d %d (%d...%d)\r\n"),
				tdd->TemperatureInfo[count].Index,
				tdd->TemperatureInfo[count].Temperature);
//				tdd->TemperatureInfo[count].UnderThreshold,
//				tdd->TemperatureInfo[count].OverThreshold);
//				tdd->TemperatureInfo[count].UnderThresholdChangable,
//				tdd->TemperatureInfo[count].OverThresholdChangable,
//				tdd->TemperatureInfo[count].EventGenerated);
	}
}

BOOL GetVolumeNameC(const TCHAR *path, WCHAR *volume)
{
	WCHAR volumeW[MAX_PATH];
	size_t length;
	HMODULE hKernel32;

	if ( DGetVolumePathNameW == NULL ){
		hKernel32 = GetModuleHandle(StrKernel32DLL);
		GETDLLPROC(hKernel32, GetVolumePathNameW);
		if ( DGetVolumePathNameW == NULL ) return FALSE;
		GETDLLPROC(hKernel32, GetVolumeNameForVolumeMountPointW);
	}
	#ifdef UNICODE
		if ( DGetVolumePathNameW(path, volumeW, TSIZEOFW(volumeW)) == FALSE ){
			return FALSE;
		}
	#else
	{
		WCHAR pathW[VFPS];

		strcpyToW(pathW, path, VFPS);
		if ( DGetVolumePathNameW(pathW, volumeW, TSIZEOFW(volumeW)) == FALSE ){
			return FALSE;
		}
	}
	#endif
	if ( IsTrue(DGetVolumeNameForVolumeMountPointW(volumeW, volume, VFPS)) ) {
		length = strlenW(volume);
		if ( length && (volume[length - 1] == L'\\') ){
			volume[length - 1] = L'\0';
		}
		return TRUE;
	}
	return FALSE;
}

#define MediaTypes_Max 8
const TCHAR *MediaTypes[MediaTypes_Max] = {
	T("unknown"), T("CFast"), T("CompactFlash"), T("MemoryStick"),
	T("MultiMediaCard"), T("SDcard"), T("QXD"), T("UFS")
};

void InitDiskinfoDlgBox(HWND hDlg, PPC_APPINFO *cinfo)
{
	ThSTRUCT thText;
	TCHAR drv[VFPS], vname[MAX_PATH + 2], fs[MAX_PATH], buf[VFPS];
	const TCHAR *typep;
	int cluster;
	DWORD i1, i2, i3, i4, BpC;
	UINTHL UserFree, Total, Free;
	WCHAR volumeNameW[MAX_PATH];

	ThInit(&thText);

	LetHL_0(Total);
	CenterPPcDialog(hDlg, cinfo);
	LocalizeDialogText(hDlg, IDD_DSKI);
			// ドライブ名
	GetDriveName(drv, cinfo->RealPath);
	SetDlgItemText(hDlg, IDS_DSKI_DN, drv);

			// ネットワークドライブ
	if ( Isalpha(drv[0]) ){
		thprintf(vname, TSIZEOF(vname), T("Network\\%c"), drv[0]);
		buf[0] = fs[0] = '\0';
		GetRegString(HKEY_CURRENT_USER, vname, RMPATHSTR, fs, TSIZEOF(fs));
		if ( fs[0] != '\0' ){
			thprintf(buf, TSIZEOF(buf), T("%s (%s)"), drv, fs);
		}else{
			thprintf(vname, TSIZEOF(vname), T("%c:"), drv[0]);
			GetRegString(HKEY_LOCAL_MACHINE, RegDeviceNamesStr, vname, fs, TSIZEOF(fs));
			if ( (tstrlen(fs) > 5) && (fs[2] == '?') ){
				thprintf(buf, TSIZEOF(buf), T("%s (%s)"), drv, fs + 4);
			}
		}
		if ( buf[0] != '\0' ){
			SetDlgItemText(hDlg, IDS_DSKI_DN, buf);
			thprintf(&thText, 0, T("Remote: %s\r\n"), buf);
		}
	}

	if ( IsTrue(GetVolumeInformation(drv, vname, TSIZEOF(vname) - 1,
			&i1, &i2, &i3, fs, MAX_PATH)) ){
		int i;

		for ( i = 0 ; DiskInfoFlags[i].flag ; i++ ){
			if ( i3 & DiskInfoFlags[i].flag ){
				thprintf(&thText, 0, T("%s, "), MessageText(DiskInfoFlags[i].name));
			}
		}
		thprintf(&thText, 0, T("\r\n"));

		EnableTextChangeNotifyItem(hDlg, IDE_DSKI_VN);
		SetDlgItemText(hDlg, IDE_DSKI_VN, vname );
		EnableDlgWindow(hDlg, IDB_APPLY, FALSE);
		thprintf(&thText, 0, T("Volume name: %s\r\n"), vname);

		thprintf(buf, TSIZEOF(buf), T("%04X-%04X"), HIWORD(i1), LOWORD(i1));
		SetDlgItemText(hDlg, IDS_DSKI_SN, buf);
		thprintf(&thText, 0, T("Volume SN: %s\r\n"), buf);

		FormatNumber(buf, XFN_SEPARATOR, 8, i2, 0);
		SetDlgItemText(hDlg, IDS_DSKI_MF, buf);
		thprintf(&thText, 0, T("Max File Length: %s\r\n"), buf);
		SetDlgItemText(hDlg, IDS_DSKI_FS, fs);
		thprintf(&thText, 0, T("File system: %s\r\n"), fs);

	}

	BpC = cluster = i1 = i2 = i3 = i4 = 0;
	if ( IsTrue(GetDiskFreeSpace(drv, &i1, &i2, &i3, &i4)) ){
		cluster = 1;
		BpC = i1 * i2;
		DDmulDHL(BpC, i3, &UserFree);
		DDmulDHL(BpC, i4, &Total);
	}
	if (
#ifndef UNICODE
			DGetDiskFreeSpaceEx &&
#endif
			DGetDiskFreeSpaceEx(GetNT_9xValue(cinfo->RealPath, drv),
					&UserFree.ULI, &Total.ULI, &Free.ULI) ){
		cluster = 1;
		SetDriveSizeItem(hDlg, IDS_DSKI_NRF, IDS_DSKI_RRF,
				&Free, CalRate(&Free, &Total));
		thprintf(&thText, 0, T("Free space(real): %'Lu\t%7MLdB\t%7MLDiB\r\n"),
				Free.rawdata, Free.rawdata, Free.rawdata);
	}
	if ( cluster ){
		int free_rate;

		free_rate = CalRate(&UserFree, &Total);
		SetDriveSizeItem(hDlg, IDS_DSKI_NFS, IDS_DSKI_RFS, &UserFree, free_rate);
		DDmulDHL(BpC, i4 - i3, &UserFree);
		SetDriveSizeItem(hDlg, IDS_DSKI_NUS, IDS_DSKI_RUS, &UserFree, 1000 - free_rate);
		thprintf(&thText, 0, T("Used space: %'Lu\t%7MLdB\t%7MLDiB \r\n"), UserFree.rawdata, UserFree.rawdata, UserFree.rawdata);

		FormatNumber(buf, XFN_SEPARATOR, 18, Total.u.LowPart, Total.u.HighPart);
		SetDlgItemText(hDlg, IDS_DSKI_NTS, buf);
		thprintf(&thText, 0, T("Total space: %'Lu\t%7MLdB\t%7MLDiB\r\n"), Total.rawdata, Total.rawdata, Total.rawdata);

		FormatNumber(buf, XFN_SEPARATOR, 18, i2, 0);
		SetDlgItemText(hDlg, IDS_DSKI_NBS, buf);
		thprintf(&thText, 0, T("Bytes/Sector: %'u\r\n"), i2);

		FormatNumber(buf, XFN_SEPARATOR, 18, BpC, 0);
		SetDlgItemText(hDlg, IDS_DSKI_NBC, buf);
		thprintf(&thText, 0, T("Bytes/Clustor: %'u\r\n"), BpC);

		FormatNumber(buf, XFN_SEPARATOR, XFNW_FULL32_SEP, i3, 0);
		SetDlgItemText(hDlg, IDS_DSKI_CFS, buf);
		thprintf(&thText, 0, T("Free clustor: %'u\r\n"), i3);

		FormatNumber(buf, XFN_SEPARATOR, XFNW_FULL32_SEP, i4 - i3, 0);
		SetDlgItemText(hDlg, IDS_DSKI_CUS, buf);
		thprintf(&thText, 0, T("Used clustor: %'u\r\n"), i4 - i3);

		FormatNumber(buf, XFN_SEPARATOR, XFNW_FULL32_SEP, i4, 0);
		SetDlgItemText(hDlg, IDS_DSKI_CTS, buf);
		thprintf(&thText, 0, T("Total clustor: %'u\r\n"), i4);
	}

	switch( GetDriveType(drv) ){
		case DRIVE_REMOTE:
			typep = Disktype_Remote;
			break;
		case DRIVE_CDROM:
			typep = Disktype_CDROM;
			break;
		case DRIVE_RAMDISK:
			typep = Disktype_RAMDISK;
			break;
		case DRIVE_REMOVABLE:
			typep = Disktype_REMOVABLE;
			break;
		case DRIVE_FIXED:
			typep = Disktype_FIXED;
			break;
		default:
			typep = T("?");
	}
	SetDlgItemText(hDlg, IDS_DSKI_DT, MessageText(typep));
	thprintf(&thText, 0, T("Drive type: %s\r\n"), MessageText(typep));
	thprintf(&thText, 0, T("Drivename: %s\r\n"), drv);

	if ( !GetVolumeNameC(cinfo->RealPath, volumeNameW) ){
		strcpyToW(volumeNameW, T("\\\\.\\A:"), MAX_PATH);
		volumeNameW[4] = cinfo->RealPath[0];
	}
	{
		HANDLE hDF;

		thprintf(&thText, 0, T("volume name: %Ls\r\n"), volumeNameW);

		hDF = CreateFileW(volumeNameW, FILE_READ_ATTRIBUTES,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if ( hDF == INVALID_HANDLE_VALUE ){
				thprintf(&thText, 0, T("volue open error(%Mm)\r\n"), GetLastError());
		}else{
			STORAGE_PROPERTY_QUERY query;
			DWORD bytesWritten;
			xDEVICE_SEEK_PENALTY_DESCRIPTOR result;
			BYTE tmp[0x2000];

			xSTORAGE_ADAPTER_DESCRIPTOR *sad = (xSTORAGE_ADAPTER_DESCRIPTOR *)tmp;
/*

					if ( DeviceIoControl(hDF, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
							NULL, 0, &tmp, sizeof(tmp), &bytesWritten, NULL) ){
						struct tagDISK_GEOMETRY_EX {
							struct {
								LARGE_INTEGER Cylinders;
								int MediaType;
								DWORD TracksPerCylinder;
								DWORD SectorsPerTrack;
								DWORD BytesPerSector;
							} Geometry;
							UINTHL DiskSize;
							BYTE Data[1];
						} *dge = (struct tagDISK_GEOMETRY_EX *)&tmp;
						thprintf(&thText, 0,
								T("Size: %'Lu\t%7MLdB\t%7MLDiB\r\n")
								T("Cylinders: %'Lu\r\n")
								T("TracksPerCylinder: %u\r\n")
								T("SectorsPerTrack: %u\r\n")
								T("BytesPerSector: %u\r\n"),
								dge->DiskSize.rawdata, dge->DiskSize.rawdata, dge->DiskSize.rawdata,
								dge->Geometry.Cylinders,
								dge->Geometry.TracksPerCylinder,
								dge->Geometry.SectorsPerTrack,
								dge->Geometry.BytesPerSector);
					}
*/
			query.PropertyId = StorageDeviceProperty;
			query.QueryType = 0; // PropertyStandardQuery;
			bytesWritten = 0;
			if ( FALSE == DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
				&query, DwordAlignment(sizeof(query)),
				&tmp, sizeof(tmp), &bytesWritten, NULL) ||
				(bytesWritten < sizeof(xSTORAGE_DEVICE_DESCRIPTOR)) ){
				thprintf(&thText, 0, T("StorageDeviceProperty error(%Mm)\r\n"), GetLastError());
			}else{
				xSTORAGE_DEVICE_DESCRIPTOR *sdd = (xSTORAGE_DEVICE_DESCRIPTOR *)tmp;

				thprintf(&thText, 0, T("Device type: %d\r\n")
						T("DeviceTypeModifier: %d\r\n")
						T("RemovableMedia: %d\r\n")
						T("CommandQueueing: %d\r\n")
						T("VendorId: %hs\r\n")
						T("ProductId: %hs\r\n")
						T("ProductRevision: %hs\r\n")
						T("SerialNumber: %hs\r\n")
						T("BusType: %s\r\n"),
						sdd->DeviceType,
						sdd->DeviceTypeModifier,
						sdd->RemovableMedia,
						sdd->CommandQueueing,
						sdd->VendorIdOffset ?
							(TCHAR *)((char *)sdd + sdd->VendorIdOffset) : NilStr,
						sdd->ProductIdOffset ?
							(TCHAR *)((char *)sdd + sdd->ProductIdOffset) : NilStr,
						sdd->ProductRevisionOffset ?
							(TCHAR *)((char *)sdd + sdd->ProductRevisionOffset) : NilStr,
						sdd->SerialNumberOffset ?
							(TCHAR *)((char *)sdd + sdd->SerialNumberOffset) : NilStr,
						sdd->BusType < BusTypeName_max ? BusTypeName[sdd->BusType] : BusTypeName[0]);
			}

			query.PropertyId = 1; // StorageAdapterProperty;
			query.QueryType = 0; // PropertyStandardQuery;
			bytesWritten = 0;
			if ( !DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
				&query, DwordAlignment(sizeof(query)),
				&tmp, sizeof(tmp), &bytesWritten, NULL) ||
				(bytesWritten < sizeof(struct tagSTORAGE_ADAPTER_DESCRIPTOR)) ){
				thprintf(&thText, 0, T("StorageAdapterProperty error(%Mm)\r\n"), GetLastError());
			}else{
				thprintf(&thText, 0, T("TransferLength: %'dB / %'7MDiB\r\n")
						T("PhysicalPages: %d\r\n")
						T("Alignment: %d bytes\r\n")
						T("AdapterUsesPio: %d\r\n")
						T("CommandQueueing: %d\r\n")
						T("AcceleratedTransfer: %d\r\n")
						T("BusType: %s\r\n"),
						sad->MaximumTransferLength, sad->MaximumTransferLength,
						sad->MaximumPhysicalPages,
						sad->AlignmentMask + 1,
						sad->AdapterUsesPio,
						sad->CommandQueueing,
						sad->AcceleratedTransfer,
						sad->BusType < BusTypeName_max ? BusTypeName[sad->BusType] : BusTypeName[0]
						);
			}

			if ( !DeviceIoControl(hDF, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
					NULL, 0, &tmp, sizeof(tmp), &bytesWritten, NULL) ){
				thprintf(&thText, 0, T("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS error(%u)\r\n"), GetLastError());
				// 仮想ドライブ iso
			}else{
				VOLUME_DISK_EXTENTS *extents = (VOLUME_DISK_EXTENTS *)tmp;
				DWORD count = 0, countMax = extents->NumberOfDiskExtents;

				for( ; count < countMax; count++){
					HANDLE hDD;
					TCHAR pdrive[MAX_PATH];

					thprintf(&thText, 0, T("\\\\.\\PhysicalDrive%u\r\n"), extents->Extents[count].DiskNumber);

					thprintf(pdrive, TSIZEOF(pdrive), T("\\\\.\\PhysicalDrive%u"), extents->Extents[count].DiskNumber);
					hDD = CreateFileL(pdrive, FILE_READ_ATTRIBUTES,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
					if ( hDD == INVALID_HANDLE_VALUE ){
						thprintf(&thText, 0, T("%Mm\r\n"), GetLastError());
						continue;
					}
					if ( DeviceIoControl(hDD, IOCTL_DISK_GET_LENGTH_INFO,
							NULL, 0, &tmp, 100, &bytesWritten, NULL) ){
						thprintf(&thText, 0, T("Drive size: %'MLd\r\n"),
								((UINTHL *)tmp)->rawdata );
					}

					if ( DeviceIoControl(hDD, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
							NULL, 0, &tmp, sizeof(tmp), &bytesWritten, NULL) ){
						xDISK_GEOMETRY_EX *dge = (xDISK_GEOMETRY_EX *)&tmp;
						thprintf(&thText, 0,
								T("Size: %'Lu\t%7MLdB\t%7MLDiB\r\n")
								T("Cylinders: %'Lu\r\n")
								T("TracksPerCylinder: %u\r\n")
								T("SectorsPerTrack: %u\r\n")
								T("BytesPerSector: %u\r\n"),
								dge->DiskSize.rawdata, dge->DiskSize.rawdata, dge->DiskSize.rawdata,
								dge->Geometry.Cylinders,
								dge->Geometry.TracksPerCylinder,
								dge->Geometry.SectorsPerTrack,
								dge->Geometry.BytesPerSector);
					}
					CloseHandle(hDD);
				}
			}
			{
				xSTORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR *smpt;

				query.PropertyId = 15;//StorageDeviceMediumProductType;
				query.QueryType = 0; // PropertyStandardQuery;
				bytesWritten = 0;
				if ( DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
						&query, DwordAlignment(sizeof(query)),
						&result, sizeof(result), &bytesWritten, NULL) &&
						(bytesWritten >= sizeof(xSTORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR)) ) {
					int type;
					smpt = (xSTORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR *)&result;
					type = smpt->MediumProductType;
					if ( type >= MediaTypes_Max ) type = 0;
					thprintf(&thText, 0, T("Removal media type: %s\r\n"), MediaTypes[type]);
				}
			}
			{
				xSTORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR *sdicd;
				query.PropertyId = 48;//StorageDeviceIoCapabilityProperty;
				query.QueryType = 0; // PropertyStandardQuery;
				bytesWritten = 0;
				if ( DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
						&query, DwordAlignment(sizeof(query)),
						&result, sizeof(result), &bytesWritten, NULL) &&
						(bytesWritten >= sizeof(xSTORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR)) ) {
					sdicd = (xSTORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR *)&result;
					thprintf(&thText, 0, T("Max IO count: unit:%d adapter:%d\r\n"), sdicd->LunMaxIoCount, sdicd->AdapterMaxIoCount);
				}
			}

			query.PropertyId = 7;//StorageDeviceSeekPenaltyProperty;
			query.QueryType = 0; // PropertyStandardQuery;
			bytesWritten = 0;
			if ( DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
					&query, DwordAlignment(sizeof(query)),
					&result, sizeof(result), &bytesWritten, NULL) &&
				 (bytesWritten >= sizeof(STORAGE_PROPERTY_QUERY)) ) {
				thprintf(&thText, 0, T("drive seek type: %s\r\n"), result.IncursSeekPenalty ? T("Hard disk") : T("Solid State Drive"));
			}

			query.PropertyId = 8;//StorageDeviceTrimProperty;
			query.QueryType = 0; // PropertyStandardQuery;
			bytesWritten = 0;
			if ( DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
					&query, DwordAlignment(sizeof(query)),
					&result, sizeof(result), &bytesWritten, NULL) &&
				 (bytesWritten >= sizeof(STORAGE_PROPERTY_QUERY)) ) {
				thprintf(&thText, 0, T("drive Trim: %s\r\n"), result.IncursSeekPenalty ? T("yes") : T("no"));
			}

			query.PropertyId = 51; // StorageAdapterTemperatureProperty
			query.QueryType = 0; // PropertyStandardQuery;
			bytesWritten = 0;
			if ( DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
					&query, DwordAlignment(sizeof(query)),
					&tmp, sizeof(tmp), &bytesWritten, NULL) &&
				 (bytesWritten >= sizeof(STORAGE_PROPERTY_QUERY)) ) {
				SetTemperatureInfo(&thText,
						(xSTORAGE_TEMPERATURE_DATA_DESCRIPTOR *)tmp,
						T("Adapter Temperature") );
			}

			query.PropertyId = 52; // StorageDeviceTemperatureProperty
			query.QueryType = 0; // PropertyStandardQuery;
			bytesWritten = 0;
			if ( DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
					&query, DwordAlignment(sizeof(query)),
					&tmp, sizeof(tmp), &bytesWritten, NULL) &&
				 (bytesWritten >= sizeof(STORAGE_PROPERTY_QUERY)) ) {
				SetTemperatureInfo(&thText,
						(xSTORAGE_TEMPERATURE_DATA_DESCRIPTOR *)tmp,
						T("Device Temperature") );
			}
			// IOCTL_DISK_GET_LENGTH_INFO IOCTL_DISK_GET_DRIVE_GEOMETRY
			query.PropertyId = 62; // StorageDeviceEnduranceProperty
			query.QueryType = 0; // PropertyStandardQuery;
			bytesWritten = 0;

			{
				xSTORAGE_HW_ENDURANCE_INFO *shei = (xSTORAGE_HW_ENDURANCE_INFO *)&tmp;

			if ( DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
					&query, DwordAlignment(sizeof(query)),
					&tmp, sizeof(tmp), &bytesWritten, NULL) &&
					(bytesWritten >= sizeof(xSTORAGE_HW_ENDURANCE_INFO)) ) {
				thprintf(&thText, 0, T("Read bytes?: %'LdGB Write bytes?: %'LdGB\r\n"),
					shei->BytesReadCount.rawdata, shei->ByteWriteCount.rawdata);
			}
			}
			// IOCTL_DISK_GET_LENGTH_INFO IOCTL_DISK_GET_DRIVE_GEOMETRY
			#if 0
			query.PropertyId = 0;
			for( ; query.PropertyId < 70; query.PropertyId++ ){
				query.QueryType = 0; // PropertyStandardQuery;
				bytesWritten = 0;
				if ( DeviceIoControl(hDF, IOCTL_STORAGE_QUERY_PROPERTY,
						&query, DwordAlignment(sizeof(query)),
						&tmp, sizeof(tmp), &bytesWritten, NULL) &&
						(bytesWritten >= sizeof(STORAGE_PROPERTY_QUERY)) ) {
					thprintf(&thText, 0, T("id : %d: ok\r\n"),query.PropertyId);
				}else{
					thprintf(&thText, 0, T("id : %d: xx\r\n"),query.PropertyId);
						}
			}
			#endif
			CloseHandle(hDF);
		}
	}
	SetDlgItemText(hDlg, IDE_DSKI_LOG, (TCHAR *)thText.bottom);
	ThFree(&thText);
}

INT_PTR CALLBACK DiskinfoDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg){
		case WM_INITDIALOG:
			InitDiskinfoDlgBox(hDlg, (PPC_APPINFO *)lParam);
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDOK:
					if ( IsWindowEnabled(GetDlgItem(hDlg, IDB_APPLY)) ){
						if ( DiskinfoDlgBox(hDlg, iMsg, IDB_APPLY, lParam) == FALSE){
							break;
						}
					}
					EndDialog(hDlg, 1);
					break;

				case IDCANCEL:
					EndDialog(hDlg, 1);
					break;

				case IDB_APPLY:
					if ( IsWindowEnabled((HWND)lParam) ){
						TCHAR dir[MAX_PATH], vname[MAX_PATH];

						GetDlgItemText(hDlg, IDS_DSKI_DN, dir, MAX_PATH);
						GetDlgItemText(hDlg, IDE_DSKI_VN, vname, MAX_PATH);
						if ( SetVolumeLabel(dir, vname) == FALSE ){
							ErrorPathBox(hDlg, T("Label"), vname, PPERROR_GETLASTERROR);
							return FALSE;
						}else{
							EnableDlgWindow(hDlg, IDB_APPLY, FALSE);
						}
					}
					break;

				case IDE_DSKI_VN:
					if ( HIWORD(wParam) == EN_CHANGE ){
						EnableDlgWindow(hDlg, IDB_APPLY, TRUE);
					}
					break;

				case IDHELP:
					return PPxDialogHelper(hDlg, WM_HELP, wParam, lParam);

				case IDQ_GETDIALOGID:
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)IDD_DSKI);
					break;
			}
			break;

		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}

/*-----------------------------------------------------------------------------
	File[M]ask / Wildcard ダイアログボックス/コールバック
-----------------------------------------------------------------------------*/
#define TMIN_HIGHLIGHTOFF 3
#define TMIN_REVERSE 2
const TCHAR *MarkItemName[] = {MES_LMAK, MES_LUMA, MES_LRMA, MES_LHIO};

void SetMarkTarget(HWND hDlg, FILEMASKDIALOGSTRUCT *PFS)
{
	TCHAR buf[0x40], buf2[0x40];
	const TCHAR *typename = NilStr;

	if ( PFS->mode >= MARK_HIGHLIGHT1 ){
		typename = buf2;
		thprintf(buf2, TSIZEOF(buf2), T("%s %d"), MessageText(MES_LHIL), PFS->mode - MARK_HIGHLIGHT1 + 1);
	}else if ( PFS->mode >= MARK_REVERSE ){
		typename = MessageText(
				MarkItemName[ (PFS->mode == MARK_HIGHLIGHTOFF) ?
					TMIN_HIGHLIGHTOFF :
					TMIN_REVERSE - PFS->mode - 1]);
	}

	thprintf(buf, TSIZEOF(buf), T("%s %s"), MessageText(MES_FBAC), typename);
	SetDlgItemText(hDlg, IDX_MASK_MODE, buf);
}

#define ID_MARKMIN 1
// unmark:2 reverse:3
#define ID_HIGHTLIGHTOFF 4
#define ID_HIGHTLIGHT1 5

#define ID_WILDCARD 64
#define ID_WILDCARD_WORD 64
#define ID_WILDCARD_FLAT 65
#define ID_WILDCARD_ROMA 66

#define ID_WILDCARD_MIN 64
#define ID_WILDCARD_WORD 64
#define ID_WILDCARD_FLAT 65
#define ID_WILDCARD_ROMA 66
#define ID_WILDCARD_MAX 70

void AddWildcardSettings(HMENU hMenu, FILEMASKDIALOGSTRUCT *PFS)
{
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenuCheckString(hMenu, ID_WILDCARD_WORD, MES_7885, PFS->option.wordmatch );
	AppendMenuCheckString(hMenu, ID_WILDCARD_FLAT, MES_COSE, !PFS->option.flat );
//	AppendMenuCheckString(hMenu, ID_WILDCARD_ROMA, T("roma search"), PFS->option.wordmatch );
}

BOOL SetWildcardSettings(HWND hDlg, FILEMASKDIALOGSTRUCT *PFS, int index)
{
	if ( (index < ID_WILDCARD_MIN) || (index > ID_WILDCARD_MAX) ) return FALSE;

	switch (index){
		case ID_WILDCARD_WORD:
			PFS->option.wordmatch = !PFS->option.wordmatch;
			CheckDlgButton(hDlg, IDX_MASK_WORDMATCH, PFS->option.wordmatch);
			break;
		case ID_WILDCARD_FLAT:
			PFS->option.flat = !PFS->option.flat;
			break;
	}
	SetCustData(PFS->settingkey, &PFS->option, sizeof(PFS->option));
	return TRUE;
}

const TCHAR *dsmd_list[] = {
	StrDsetTemporality, StrDsetThisPath, StrDsetThisBranch, StrDsetForce,
	StrDsetListfile, NilStr, StrDsetArchive
};

void SetMaskTarget(HWND hDlg, FILEMASKDIALOGSTRUCT *PFS)
{
	TCHAR buf[CMDLINESIZE];
	const TCHAR *typename = NilStr;

	if ( (PFS->mode >= DSMD_TEMP) && (PFS->mode <= DSMD_ARCHIVE) ){
		if ( PFS->mode == DSMD_PATH_BRANCH ){
			typename = PFS->path;
			if ( (typename == NULL) || (*typename == '\0') ) typename = T("*");
		}else{
			typename = MessageText(dsmd_list[PFS->mode - DSMD_TEMP]);
		}
	}
	thprintf(buf, TSIZEOF(buf), T("%s %s"), MessageText(MES_FBAC), typename);
	SetDlgItemText(hDlg, IDX_MASK_MODE, buf);
}

#define REALTIMEMASKLIMIT(cinfo) ((cinfo)->e.cellDataMax > (int)(OSver.dwMajorVersion * 3000 - 8000)) // Ver4:4000 5:7000 6:10000

const DWORD HideMaskItemList[] = {
		// FindMark の時は隠す
		IDX_MASK_RTM, /*IDX_MASK_MODE,*/
		// FindMark / FindMask で一時等以外の時は隠す
		IDG_FOP_ATTR, IDX_FOP_RONLY, IDX_FOP_SYSTEM, IDX_FOP_HIDE,
		IDX_FOP_ARC, /* IDX_MASK_DIR,*/ 0};

void HideMaskItem(HWND hDlg, FILEMASKDIALOGSTRUCT *PFS)
{
	const DWORD *item = HideMaskItemList;
	int showstate = SW_HIDE;

	if ( IsTrue(PFS->maskmode) ){ // ファイルマスク
		item += 1; // 2;
		if ( (PFS->mode == DSMD_TEMP) || (PFS->mode == DSMD_REGID) ){
			showstate = SW_SHOWNOACTIVATE;
		}
	}
	for ( ; *item != 0 ; item++ ){
		ShowWindow(GetDlgItem(hDlg, *item), showstate);
	}
}

#define ID_INIT_COMMAND 0xc106
void InitMaskDialog(HWND hDlg, FILEMASKDIALOGSTRUCT *PFS)
{
	SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)PFS);
	CenterPPcDialog(hDlg, PFS->cinfo);
	LocalizeDialogText(hDlg, IDD_MASK);
	SetWindowText(hDlg, MessageText((TCHAR *)PFS->title));

	memset(&PFS->option, 0, sizeof(PFS->option));
	GetCustData(PFS->settingkey, &PFS->option, sizeof(PFS->option));
	if ( (!PFS->maskmode || (PFS->mode == DSMD_TEMP)) &&
		 PFS->option.dirmask ){
		setflag(PFS->mask->attr, MASKEXTATTR_DIR);
	}

	// ディレクトリ指定があれば、チェックボックスのチェックに置き換え
	{
		int attr;
		const TCHAR *skipptr;

		attr = GetWildcardAttributes(PFS->mask->file, &skipptr);
		if ( attr != 0 ){
			if ( attr & MASKEXTATTR_DIR ){
				setflag(PFS->mask->attr, MASKEXTATTR_DIR);
			}
			if ( attr & MASKEXTATTR_WORDMATCH ){
				PFS->option.wordmatch = TRUE;
			}
			if ( !(attr & MASKEXTATTR_GA_ETC) ){
				tstrtrimhead(PFS->mask->file, skipptr - PFS->mask->file);
			}
		}
	}

	CheckDlgButton(hDlg, IDX_MASK_DIR, PFS->mask->attr & MASKEXTATTR_DIR);
	CheckDlgButton(hDlg, IDX_MASK_WORDMATCH, PFS->option.wordmatch);

	if ( IsTrue(PFS->maskmode) ){ // ファイルマスク
		SetAttibuteSettings(hDlg, PFS->mask->attr ^ BADATTR);
		CheckDlgButton(hDlg, IDX_MASK_RTM, PFS->option.realtime);
		EnableDlgWindow(hDlg, IDX_MASK_RTM, !REALTIMEMASKLIMIT(PFS->cinfo));
		SetMaskTarget(hDlg, PFS);
	}else{ // ファイルマーク
		SetMarkTarget(hDlg, PFS);
	}
	HideMaskItem(hDlg, PFS);

	PFS->hEdWnd = PPxRegistExEdit(&PFS->cinfo->info,
			GetDlgItem(hDlg, IDE_INPUT_LINE), TSIZEOF(PFS->mask->file),
			PFS->mask->file, PPXH_MASK_R, PPXH_MASK,
			SearchVLINEwild(PFS->mask->file) ?
				PPXEDIT_COMBOBOX | PPXEDIT_WANTEVENT |
				PPXEDIT_REFTREE | PPXEDIT_SINGLEREF | PPXEDIT_INSTRSEL :
				PPXEDIT_COMBOBOX | PPXEDIT_WANTEVENT |
				PPXEDIT_REFTREE | PPXEDIT_SINGLEREF);
	ActionInfo(hDlg, &PFS->cinfo->info, AJI_SHOW,
			IsTrue(PFS->maskmode) ? T("mask") : T("mark") );
	if ( PFS->initcmd != NULL ){
		PostMessage(hDlg, WM_COMMAND, ID_INIT_COMMAND, 0);
	}
}

void MaskTargetMenu(HWND hDlg, HWND hButtonWnd, FILEMASKDIALOGSTRUCT *PFS)
{
	HMENU hMenu;
	int index;
	TCHAR setpath[VFPS];
	BOOL settab;

	hMenu = CreatePopupMenu();

	setpath[0] = '\t';
	setpath[1] = '\0';
	settab = AppendMenuPopString(hMenu, SortPopStringList + 1);

	if ( (PFS->cinfo->e.Dtype.mode == VFSDT_LFILE) ||
		 (PFS->mode == DSMD_LISTFILE) ){
		AppendMenuString(hMenu, CRID_SORT_LISTFILE, StrDsetListfile);
	}
	if ( (PFS->cinfo->e.Dtype.mode == VFSDT_UN) ||
		(PFS->cinfo->e.Dtype.mode == VFSDT_SUSIE) ||
		(PFS->cinfo->e.Dtype.mode == VFSDT_ARCFOLDER) ||
		(PFS->cinfo->e.Dtype.mode == VFSDT_CABFOLDER) ||
		(PFS->cinfo->e.Dtype.mode == VFSDT_LZHFOLDER) ||
		(PFS->cinfo->e.Dtype.mode == VFSDT_ZIPFOLDER) ||
		(PFS->mode == DSMD_ARCHIVE) ){
		tstrcpy(settab ? setpath + 1 : setpath, MessageText(StrDsetArchive));
		AppendMenuString(hMenu, CRID_SORT_ARCHIVE, setpath);
	}

	if ( (PFS->mode == DSMD_PATH_BRANCH) && (PFS->path != NULL) ){
		tstrcpy(setpath + 1, PFS->path);
	}else{
		setpath[1] = '*';
		setpath[2] = '\0';
	}
	AppendMenuString(hMenu, (CRID_SORTEX + DSMD_PATH_BRANCH), settab ? setpath : setpath + 1);

	CheckMenuRadioItem(hMenu, CRID_SORT_TEMP, CRID_SORT_LISTFILE, PFS->mode + CRID_SORTEX, MF_BYCOMMAND);

	AddWildcardSettings(hMenu, PFS);

	index = PopupButtonMenu(hMenu, hDlg, hButtonWnd);
	DestroyMenu(hMenu);
	if ( index ){
		if ( IsTrue(SetWildcardSettings(hDlg, PFS, index)) ) return;

		index -= CRID_SORTEX;
		if ( PFS->mode != index ){
			LOADSETTINGS_TMP ls;
			TCHAR *path = NULL, pathbuf[VFPS];

			PFS->mode = index;
			ls.mask[0] = '\0';
			switch ( index ){
				case DSMD_THIS_PATH:
					LoadSetting_FixThisPath(pathbuf, PFS->cinfo->path);
					LoadSettingMain(&ls, pathbuf);
					path = ls.mask;
					break;

				case DSMD_THIS_BRANCH:
					LoadSetting_FixThisBranch(pathbuf, PFS->cinfo->path);
					LoadSettingMain(&ls, pathbuf);
					path = ls.mask;
					break;

				case DSMD_REGID:
					path = PFS->cinfo->mask.file;
					break;

				case DSMD_LISTFILE:
					LoadSettingMain(&ls, StrListfileMode);
					path = ls.mask;
					break;

				case DSMD_ARCHIVE:
					LoadSettingMain(&ls, StrArchiveMode);
					path = ls.mask;
					break;

				case DSMD_PATH_BRANCH:
					LoadSettingMain(&ls, setpath + 1);
					path = ls.mask;
					break;
			}
			if ( path != NULL ) SetDlgItemText(hDlg, IDE_INPUT_LINE, path);
		}
	}
	SetMaskTarget(hDlg, PFS);
}

void MarkTargetMenu(HWND hDlg, HWND hButtonWnd, FILEMASKDIALOGSTRUCT *PFS)
{
	HMENU hMenu;
	int index, i;
	TCHAR buf[VFPS];

	hMenu = CreatePopupMenu();

// 設定の保存先
	for ( i = 0 ; i <= TMIN_HIGHLIGHTOFF ; i++ ){
		AppendMenuString(hMenu,
				(i == TMIN_HIGHLIGHTOFF) ? ID_HIGHTLIGHTOFF :
				1 - MARK_REVERSE - i + ID_MARKMIN, MarkItemName[i]);
	}
	for ( i = 0 ; i < PPC_HIGHLIGHT_COLORS ; i++ ){
		thprintf(buf, TSIZEOF(buf), T("%s &%d"), MessageText(MES_LHIL), i + 1);
		AppendMenuString(hMenu, i + ID_HIGHTLIGHT1, buf);
	}

	CheckMenuRadioItem(hMenu, ID_MARKMIN, ID_HIGHTLIGHT1 + PPC_HIGHLIGHT_COLORS - 1, PFS->mode - MARK_REVERSE + 1, MF_BYCOMMAND);

// ワイルドカード設定
	AddWildcardSettings(hMenu, PFS);

	index = PopupButtonMenu(hMenu, hDlg, hButtonWnd);
	DestroyMenu(hMenu);
	if ( index > 0 ){
		if ( IsTrue(SetWildcardSettings(hDlg, PFS, index)) ) return;

		index = index - ID_MARKMIN + MARK_REVERSE;
		if ( PFS->mode != index ){
			PFS->mode = index;
			SetMarkTarget(hDlg, PFS);
		}
	}
}

DWORD USEFASTCALL GetFileMaskAttr(HWND hDlg)
{
	return
		(GetAttibuteSettings(hDlg) ^
			(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN |
			 FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE)) |
		(IsDlgButtonChecked(hDlg, IDX_MASK_DIR) ? MASKEXTATTR_DIR : 0) |
		(IsDlgButtonChecked(hDlg, IDX_MASK_WORDMATCH) ? MASKEXTATTR_WORDMATCH : 0);
}

void RealtimeMask(HWND hDlg, FILEMASKDIALOGSTRUCT *PFS, UINT CtlMes, HWND hCtlWnd)
{
	TCHAR edittextbuf[MAX_PATH];

	if ( CtlMes == CBN_EDITUPDATE ){
		GetWindowText(hCtlWnd, edittextbuf, TSIZEOF(edittextbuf));
	}else{
		int index;

		index = (int)SendMessage(hCtlWnd, CB_GETCURSEL, 0, 0);
		if ( (index < 0) ||
			 (SendMessage(hCtlWnd, CB_GETLBTEXTLEN, (WPARAM)index, 0) >= MAX_PATH) ){
			return;
		}
		SendMessage(hCtlWnd, CB_GETLBTEXT, (WPARAM)index, (LPARAM)edittextbuf);
	}
	GetMaskText(edittextbuf, PFS->mask->file, &PFS->option);

//	if ( *writep == '\0' ) PFS->mask->file[0] = '\0';
	PFS->mask->attr = GetFileMaskAttr(hDlg);
	if ( MaskEntryMain(PFS->cinfo, PFS->mask, PFS->filename) ){
		SetMessageOnCaption(hDlg, NULL);
	}else{
		SetMessageOnCaption(hDlg, MES_EBWA);
		PostMessage(PFS->hEdWnd, WM_PPXCOMMAND, KE_closeUList, 0);
	}
}

// FindMark / FindMask ダイアログ
INT_PTR CALLBACK FileMaskDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	FILEMASKDIALOGSTRUCT *PFS;

	PFS = (FILEMASKDIALOGSTRUCT *)GetWindowLongPtr(hDlg, DWLP_USER);
	switch (message){
							// ダイアログ初期化 (w:focus, l:User からの param)--
		case WM_INITDIALOG:
			InitMaskDialog(hDlg, (FILEMASKDIALOGSTRUCT *)lParam);
			return TRUE;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDE_INPUT_LINE:
					if ( (PFS->maskmode == FALSE) ||
						 (PFS->option.realtime == FALSE) ||
						 REALTIMEMASKLIMIT(PFS->cinfo) ){
						break;
					}
					if ( (HIWORD(wParam) == CBN_SELCHANGE) ||
						 (HIWORD(wParam) == CBN_EDITUPDATE) ){
						RealtimeMask(hDlg, PFS, HIWORD(wParam), (HWND)lParam);
						break;
					}
					break;

				case IDOK:
					GetMaskTextFromEdit(hDlg, IDE_INPUT_LINE, PFS->mask->file, &PFS->option);
					PFS->mask->attr = GetFileMaskAttr(hDlg);
					SetCaption(PFS->cinfo);
					EndDialog(hDlg, 1);
					break;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;

				case IDB_REF:
					SendMessage(PFS->hEdWnd, WM_PPXCOMMAND,
							K_raw | K_s | K_c | 'I', 0);
					break;

				case IDX_MASK_DIR:
					PFS->option.dirmask = IsDlgButtonChecked(hDlg, IDX_MASK_DIR);
					if ( PFS->option.dirmask ){
						setflag(PFS->mask->attr, MASKEXTATTR_DIR);
					}else{
						resetflag(PFS->mask->attr, MASKEXTATTR_DIR);
					}
				// IDX_MASK_WORDMATCH / IDX_MASK_RTM へ続けて設定保存
				case IDX_MASK_WORDMATCH:
					PFS->option.wordmatch = IsDlgButtonChecked(hDlg, IDX_MASK_WORDMATCH);
				// IDX_MASK_RTM へ続けて設定保存
				case IDX_MASK_RTM:
					PFS->option.realtime = IsDlgButtonChecked(hDlg, IDX_MASK_RTM);
					SetCustData(PFS->settingkey, &PFS->option, sizeof(PFS->option));

					FileMaskDialog(hDlg, WM_COMMAND,
							TMAKEWPARAM(IDE_INPUT_LINE, CBN_EDITUPDATE),
							(LPARAM)GetDlgItem(hDlg, IDE_INPUT_LINE) );
					break;

				case IDX_MASK_MODE:
					if ( HIWORD(wParam) == BN_CLICKED ){
						if ( PPxCommonExtCommand(K_IsShowButtonMenu, IDX_MASK_MODE) != NO_ERROR ){
							break;
						}
						if ( IsTrue(PFS->maskmode) ){ // ファイルマスク
							MaskTargetMenu(hDlg, (HWND)lParam, PFS);
							HideMaskItem(hDlg, PFS);
						}else{ // ファイルマーク
							MarkTargetMenu(hDlg, (HWND)lParam, PFS);
						}
					}
					break;

				case IDQ_GETDIALOGID:
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)IDD_MASK);
					break;

				case IDB_WILDFORMAT:
					if ( HIWORD(wParam) == BN_CLICKED ){
						MaskButtonMenu(PFS->cinfo, hDlg, (HWND)lParam, IDE_INPUT_LINE, MaskButton_GenMask);
					}
					break;

				case ID_INIT_COMMAND:
					SendMessage(PFS->hEdWnd, WM_PPXCOMMAND, K_EXECUTE, (LPARAM)PFS->initcmd);
					PPcHeapFree(PFS->initcmd);
					PFS->initcmd = NULL;
					break;
			}
			break;

		case WM_SYSCOMMAND:
			if ( wParam == SC_KEYMENU ){
				SendMessage(PFS->hEdWnd, WM_PPXCOMMAND,
						(WPARAM)(upper((TCHAR)lParam) | GetShiftKey()), 0);
				break;
			}
			// default へ
		default:
			return PPxDialogHelper(hDlg, message, wParam, lParam);
	}
	return TRUE;
}


void BinString(TCHAR *dest, DWORD data, int bits)
{
	int i;

	*dest++ = 'B';
	for ( i = 1 ; i <= bits ; i++ ){
		*dest++ = (TCHAR)((data & (1 << (bits - i))) ? '1':'0');
	}
	*dest = '\0';
}

void GetSortString(XC_SORT *xsort, TCHAR *dest)
{
	TCHAR attr[16], opt[32];

	BinString(attr, xsort->atr, 6);
	BinString(opt, xsort->option, 26);

	thprintf(dest, MAX_PATH, T("%d,%d,%d,%s,%s"),
			xsort->mode.dat[0], xsort->mode.dat[1], xsort->mode.dat[2],
			attr, opt);
}

void SaveSortSettings(HWND hDlg)
{
	TCHAR itemname[MAX_PATH], itemparam[MAX_PATH];
	XC_SORT xsort;

	tstrcpy(itemname, T("name"));
	if ( tInput(hDlg, MES_TINS, itemname, TSIZEOF(itemname),
			PPXH_GENERAL, PPXH_GENERAL) <= 0 ){
		return;
	}
	GetSortSettings(hDlg, &xsort);
	GetSortString(&xsort, itemparam);
	SetCustStringTable(T("MC_sort"), itemname, itemparam, 0);
}

BYTE GetSortID(HWND hDlg, int id)
{
	int index = (int)SendDlgItemMessage(hDlg, id, CB_GETCURSEL, 0, 0);
	int data = (int)SendDlgItemMessage(hDlg, id, CB_GETITEMDATA, index, 0);
	if ( data > 0 ){
		return (BYTE)(data + SORT_COLUMNTYPE - 2);
	}else{
		return sortIDtable[index];
	}
}

void GetSortSettings(HWND hDlg, XC_SORT *xs)
{
	xs->mode.dat[0] = GetSortID(hDlg, IDC_SORT_TYPE1);
	xs->mode.dat[1] = GetSortID(hDlg, IDC_SORT_TYPE2);
	xs->mode.dat[2] = GetSortID(hDlg, IDC_SORT_TYPE3);
	xs->mode.dat[3] = -1;
	xs->option =
		(IsDlgButtonChecked(hDlg, IDX_SORT_CASE) ?	0 : SORTE_IGNORECASE) |
		(IsDlgButtonChecked(hDlg, IDX_SORT_KANA) ?	SORTE_KANATYPE : 0) |
		(IsDlgButtonChecked(hDlg, IDX_SORT_SPACE) ?	SORTE_NONSPACE : 0) |
		(IsDlgButtonChecked(hDlg, IDX_SORT_SYM) ?	SORTE_SYMBOLS : 0) |
		(IsDlgButtonChecked(hDlg, IDX_SORT_WIDE) ?	SORTE_WIDE : 0) |
		(IsDlgButtonChecked(hDlg, IDX_SORT_NUM) ?	SORTE_NUMBER : 0) |
		(IsDlgButtonChecked(hDlg, IDX_SORT_LE) ?	SORTE_LASTENTRY : 0) |
		(IsDlgButtonChecked(hDlg, IDX_SORT_SS) ?	SORT_STRINGSORT : 0);
	xs->atr = GetAttibuteSettings(hDlg) | FILE_ATTRIBUTE_LABEL;
}

void SendDlgSortCB(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	SendDlgItemMessage(hDlg, IDC_SORT_TYPE1, msg, wParam, lParam);
	SendDlgItemMessage(hDlg, IDC_SORT_TYPE2, msg, wParam, lParam);
	SendDlgItemMessage(hDlg, IDC_SORT_TYPE3, msg, wParam, lParam);
}

void ReverseSortType(HWND hDlg, int typeID)
{
	int index = (int)SendDlgItemMessage(hDlg, typeID, CB_GETCURSEL, 0, 0);

	if ( (index >= 1) && (index <= 10) ){
		index += 10;
	}else if ( (index >= 11) && (index <= 20) ){
		index -= 10;
	}
	SendDlgItemMessage(hDlg, typeID, CB_SETCURSEL, index, 0);
}

//  --------------------------------------------------------------
INT_PTR CALLBACK SortDlgBox(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg){
		case WM_INITDIALOG: {
			int CommentID, index;
			PPC_APPINFO *cinfo;
			XC_SORT *xc;

			CenterWindow(hDlg);
			LocalizeDialogText(hDlg, IDD_SORT);
			xc = ((PPCSORTDIALOGPARAM *)lParam)->xc;
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)xc);
			for ( index = 0 ; SortItems[index] ; index++ ){
				SendDlgSortCB(hDlg, CB_ADDSTRING,
						0, (LPARAM)MessageText(SortItems[index]));
			}

			cinfo = ((PPCSORTDIALOGPARAM *)lParam)->cinfo;
			for ( CommentID = 1 ; CommentID <= 10 ; CommentID++ ){
				TCHAR buf[100];

				if ( !((cinfo->UseCommentsFlag) & (1 << CommentID)) ){
					thprintf(buf, TSIZEOF(buf), T("COMMENTEVENT%d"), CommentID);
					if ( IsExistCustTable(T("KC_main"), buf) == FALSE ) continue;
				}
				thprintf(buf, TSIZEOF(buf), T("%d Ex Comment #%d"), CommentID, CommentID);
				SendDlgSortCB(hDlg, CB_ADDSTRING, 0, (LPARAM)buf );
				SendDlgSortCB(hDlg, CB_SETITEMDATA, (WPARAM)index, (LPARAM)(CommentID * 2));
				thprintf(buf, TSIZEOF(buf), T("%d R)Ex Comment #%d"), CommentID, CommentID);
				SendDlgSortCB(hDlg, CB_ADDSTRING, 0, (LPARAM)buf );
				SendDlgSortCB(hDlg, CB_SETITEMDATA, (WPARAM)(index + 1), (LPARAM)(CommentID * 2 + 1));
				index += 2;
			}
			{
				int type;

				type = xc->mode.dat[0] + 1;
				if ( type == 0 ) type = 1;
				SendDlgItemMessage(hDlg, IDC_SORT_TYPE1, CB_SETCURSEL, type, 0);
			}
			SendDlgItemMessage(hDlg, IDC_SORT_TYPE2, CB_SETCURSEL,
				(WPARAM)(xc->mode.dat[1] + 1), 0);
			SendDlgItemMessage(hDlg, IDC_SORT_TYPE3, CB_SETCURSEL,
				(WPARAM)(xc->mode.dat[2] + 1), 0);
			CheckDlgButton(hDlg, IDX_SORT_CASE,
					!(xc->option & SORTE_IGNORECASE));
			CheckDlgButton(hDlg, IDX_SORT_KANA,
					(xc->option & SORTE_KANATYPE));
			CheckDlgButton(hDlg, IDX_SORT_SPACE,
					(xc->option & SORTE_NONSPACE));
			CheckDlgButton(hDlg, IDX_SORT_SYM,
					(xc->option & SORTE_SYMBOLS));
			CheckDlgButton(hDlg, IDX_SORT_WIDE,
					(xc->option & SORTE_WIDE));
			CheckDlgButton(hDlg, IDX_SORT_NUM,
					(xc->option & SORTE_NUMBER));
			CheckDlgButton(hDlg, IDX_SORT_LE,
					(xc->option & SORTE_LASTENTRY));
			CheckDlgButton(hDlg, IDX_SORT_SS,
					(xc->option & SORT_STRINGSORT));

			SetAttibuteSettings(hDlg, xc->atr);
			ActionInfo(hDlg, &cinfo->info, AJI_SHOW, T("sort"));
			return TRUE;
		}
		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDOK:
					GetSortSettings(hDlg,
							(XC_SORT *)GetWindowLongPtr(hDlg, DWLP_USER));
					EndDialog(hDlg, 1);
					break;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;

				case IDB_SAVE:
					SaveSortSettings(hDlg);
					break;

				case IDX_SORT_REV1:
					ReverseSortType(hDlg, IDC_SORT_TYPE1);
					break;
				case IDX_SORT_REV2:
					ReverseSortType(hDlg, IDC_SORT_TYPE2);
					break;
				case IDX_SORT_REV3:
					ReverseSortType(hDlg, IDC_SORT_TYPE3);
					break;

				case IDHELP:
					return PPxDialogHelper(hDlg, WM_HELP, wParam, lParam);

				case IDQ_GETDIALOGID:
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)IDD_SORT);
					break;
			}
			break;

		default:
			return PPxDialogHelper(hDlg, iMsg, wParam, lParam);
	}
	return TRUE;
}

