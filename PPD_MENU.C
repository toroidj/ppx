/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library				メニュー処理
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#pragma hdrstop

#define MCID 1
DWORD MenuEnterCount = 0; // メニュー再入によるスタックオーバフローチェッカ

const TCHAR SetWindowPosToDeskStr[] = MES_SWPD;

void AddInternalMenu(HMENU hTopMenu, const PPXINMENU *menus, PPXAPPINFO *info, DWORD *PopupID, ThSTRUCT *thMenuData);

#define HELPID_DATAISSTRUCT 0x1061

DefineWinAPI(BOOL, GetMenuInfo, (HMENU hMenu, LPMENUINFO menuinfo)) = NULL;
DefineWinAPI(BOOL, SetMenuInfo, (HMENU hMenu, LPCMENUINFO menuinfo)) = NULL;

LOADWINAPISTRUCT MENUINFODLL[] = {
	LOADWINAPI1(GetMenuInfo),
	LOADWINAPI1(SetMenuInfo),
	{NULL, NULL}
};


int TrackPopupMenuConsole(HMENU hMenu)
{
	int index;
	TCHAR buf[CMDLINESIZE];
	DWORD old;

	index = PPxInfoFunc32i(&PPxDefInfo, PPXCMDID_HOOK_POPUPMENU, hMenu);
	if ( index > 0 ) return index - 2;

	index = 0;
	for (;;){
		if ( GetMenuString(hMenu, index, buf, TSIZEOF(buf), MF_BYPOSITION) == 0 ){
			break;
		}
		tstrcat(buf, T("\r\n"));
		PrintToConsole(buf);
		index++;
	}
	PrintToConsole(T("** dummy function(always cancel) **\r\n"));

	old = UseConsoleKey();
	ReadConsoleKey();
	FreeConsoleKey(old);
	return -1;
}

// ウィンドウがないとメニューが表示できないので、仮ウィンドウを用意して表示
#define NULLPOPUPCLASSNAME T("NULLPOPUPCLASS") T(TAPITAIL)
LRESULT CALLBACK NullPopupMenuProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

WNDCLASS nullpopupClass = {
	0, NullPopupMenuProc, 0, 0,
	NULL, NULL, NULL, NULL, NULL, NULLPOPUPCLASSNAME
};

struct NULLPOPUPSTRUCT {
	HMENU hMenu;
	POINT pos;
	int result;
};

const TCHAR winedetectpath[] = T("Software\\Wine");
extern BOOL ExMenuAdd(PPXAPPINFO *info, ThSTRUCT *thMenuData, HMENU hMenu, const TCHAR *exmenu, const TCHAR *name, DWORD *PopupID);

const TCHAR MountedDevices[] = T("SYSTEM\\MountedDevices");

DefineWinAPI(BOOL, GetDiskFreeSpaceEx, (LPCTSTR lpDirectoryName,
	PULARGE_INTEGER lpFreeBytesAvailableToCaller,
	PULARGE_INTEGER lpTotalNumberOfBytes,
	PULARGE_INTEGER lpTotalNumberOfFreeBytes));

ERRORCODE ExtDriveMenuItem(HMENU hPopupMenu, DWORD *index, ThSTRUCT *THmenu, const TCHAR *KeyName, const TCHAR *head, BOOL command)
{
	ULARGE_INTEGER UserFree, Total, TotalFree;
	TCHAR sizestr[128];
	TCHAR VolName[MAX_PATH + 2];
	TCHAR FSName[MAX_PATH + 2];
	TCHAR buf[0x800];
	ERRORCODE result;

	if ( FALSE == GetVolumeInformation(KeyName, VolName, TSIZEOF(VolName) - 1,
			NULL, NULL, NULL, FSName, TSIZEOF(FSName) - 1) ){
		result = GetLastError();
		if ( result == ERROR_NOT_READY ){
			thprintf(buf, TSIZEOF(buf), T("%s offline\t%s"), head, KeyName);
		}else if ( result == ERROR_INVALID_PARAMETER ){
			thprintf(buf, TSIZEOF(buf), T("%s raw\t%s"), head, KeyName);
		}else{
			return result;
		}
		AppendMenu(hPopupMenu, command ? MF_GS : MF_ES, (*index)++, buf);
		ThAddString(THmenu, command ? NilStr : KeyName);
		return result;
	}

	LetHL_0(Total);
	#ifndef UNICODE
	if ( DGetDiskFreeSpaceEx )
	#endif
	{
		DGetDiskFreeSpaceEx(KeyName, &UserFree, &Total, &TotalFree);
	}
	if ( Total.u.LowPart | Total.u.HighPart ){
		FormatNumber(sizestr, XFN_SEPARATOR, 5, Total.u.LowPart, Total.u.HighPart);
		thprintf(buf, TSIZEOF(buf), T("%s %s %s %s\t%s"), VolName, head, FSName, sizestr, KeyName);
	}else{
		thprintf(buf, TSIZEOF(buf), T("%s %s %s\t%s"), VolName, head, FSName, KeyName);
	}
	AppendMenuString(hPopupMenu, (*index)++, buf);

	thprintf(buf, TSIZEOF(buf), T("*jumppath \"%s\""), KeyName);
	ThAddString(THmenu, command ? buf : KeyName);
	return NO_ERROR;
}

void ExtDriveMenu(HMENU hPopupMenu, ThSTRUCT *THmenu, DWORD *index, BOOL command)
{
	HKEY hRegList;
	DWORD len;

	TCHAR VolName[0x1000];
	TCHAR KeyName[0x1000];
	TCHAR buf[0x1000];
	ThSTRUCT idlist;

	DefineWinAPI(BOOL, GetVolumePathNamesForVolumeName, (LPCTSTR, LPTSTR, DWORD, PDWORD));
	DefineWinAPI(HANDLE, FindFirstVolume, (LPTSTR, DWORD));
	DefineWinAPI(BOOL, FindNextVolume, (HANDLE, LPTSTR, DWORD));
	DefineWinAPI(BOOL, FindVolumeClose, (HANDLE));

	GETDLLPROCT(hKernel32, GetDiskFreeSpaceEx);
	GETDLLPROCT(hKernel32, GetVolumePathNamesForVolumeName);
	GETDLLPROCT(hKernel32, FindFirstVolume);
	GETDLLPROCT(hKernel32, FindNextVolume);
	GETDLLPROC(hKernel32, FindVolumeClose);
	ThInit(&idlist);

	if ( hPopupMenu == NULL ) hPopupMenu = CreatePopupMenu();

	{ // ハードディスクの一覧を作成
		int disk = 0, partition;

		for (;;){
			partition = 1;
			for (;;){
				ERRORCODE result;

				thprintf(KeyName, TSIZEOF(KeyName), T("\\\\.\\Harddisk%dPartition%d\\"), disk, partition);
				thprintf(VolName, TSIZEOF(VolName), T("disk%d-%d"), disk, partition);
				result = ExtDriveMenuItem(hPopupMenu, index, THmenu, KeyName, VolName, command);

				if ( result != NO_ERROR ){
					if ( (result == ERROR_PATH_NOT_FOUND) && (partition == 1) ){
						disk = partition = 10000;
					}
					break;
				}
				partition++;
				if ( partition >= 1000 ) break;
			}
			disk++;
			if ( disk >= 1000 ) break;
		}
	}

	// APIマウント済みハードディスクの一覧
	if ( DFindFirstVolume != NULL ){
		HANDLE hFV;
		hFV = DFindFirstVolume(KeyName, MAX_PATH);
		if ( hFV != INVALID_HANDLE_VALUE ){
			 for(;;){
				VolName[0] = '\0';
				if ( DGetVolumePathNamesForVolumeName != NULL ){
					DGetVolumePathNamesForVolumeName(KeyName, VolName, TSIZEOF(VolName), &len);
				}
				KeyName[2] = '.';
				if ( ThGetString(&idlist, KeyName, buf, 2) == NULL ){
					ThSetString(&idlist, KeyName, T("."));
					ExtDriveMenuItem(hPopupMenu, index, THmenu, KeyName, VolName, command);
				}

				if ( DFindNextVolume(hFV, KeyName, MAX_PATH) == FALSE ) break;
			}
			DFindVolumeClose(hFV);
		}
	}

	// レジストリマウント済みデバイスの一覧
	if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, MountedDevices, 0, KEY_READ, &hRegList) ){
		int cnt = 0;

		for ( ; ; ){
			DWORD keySize, valueSize;
			BYTE Value[0x1000];
			DWORD Rtyp;

			keySize = TSIZEOF(KeyName);
			valueSize = TSIZEOF(Value);
			KeyName[0] = '\0';
			Value[0] = 0;

			if ( ERROR_SUCCESS != RegEnumValue(hRegList, cnt, KeyName, &keySize,
					NULL, &Rtyp, (BYTE *)Value, &valueSize) ){
				break;
			}
			cnt++;
			Value[TSIZEOF(Value) - 1] = '\0';

			if ( KeyName[1] == '\0' ) continue;
			if ( KeyName[0] == '#' ) continue;

			if ( (KeyName[0] == '\\') && (KeyName[1] == '?') ){
				KeyName[1] = '\\';
				tstrcat(KeyName, T("\\"));
				KeyName[2] = '.';
			}else if ( memcmp(KeyName, T("\\DosDevices\\"), TSTROFF(12)) == 0 ){
				thprintf(KeyName, TSIZEOF(KeyName), T("%c:\\"), KeyName[12]);
			}

			if ( ThGetString(&idlist, KeyName, buf, 2) == NULL ){
				ThSetString(&idlist, KeyName, T("."));

				if ( (memcmp(Value + 8, L"USBSTOR#", 16) == 0) ||
					 (memcmp(Value + 8, L"SCSI#", 10) == 0) ){
					WCHAR *wp;
					TCHAR *dst;

					wp = (WCHAR *)Value + 4 + ((Value[8] == 'U') ? 8 : 5);
					dst = tstpcpy(VolName, T("USB "));
					for ( ;; ){
						WCHAR wc;
						wc = *wp++;
						if ( (wc == '\0') || (wc == '#') ) break;
						if ( wc == '&' ) wc = ' ';
						*dst++ = (TCHAR)wc;
					}
					*dst = '\0';
				}else if ( memcmp(Value + 9, "Volume", 6) == 0 ){
					thprintf(VolName, TSIZEOF(VolName), T("Volume %c"), Value[15]);
				}else{
					VolName[0] = '\0';
				}

				ExtDriveMenuItem(hPopupMenu, index, THmenu, KeyName, VolName, command);
			}
		}
	}
	ThFree(&idlist);
}

void RegisterFavoriteItem(PPXAPPINFO *info)
{
	TCHAR itemname[VFPS], itempath[VFPS], itembuf[VFPS];
	const TCHAR *p;

	PP_ExtractMacro(info->hWnd, info, NULL, T("%FNVD"), itempath, 0);
	p = EscapeMacrochar(itempath, itembuf);
	if ( p != itempath ) tstrcpy(itempath, p);
	p = FindLastEntryPoint(itempath);
	if ( *p == '\0' ) p = itempath;
	thprintf(itemname, TSIZEOF(itemname), T("%%\"%s\"%%{%%|%s%%|%%}"), MessageText(MES_ADTE), p);
	if ( PP_ExtractMacro(info->hWnd, info, NULL, itemname, itemname, 0) != NO_ERROR ){
		return;
	}
	SetCustStringTable(PathJumpName, itemname, itempath, 0);
}
const TCHAR EjectCmdStr[] = T("*freedriveuse %c:%%:%%z%c:\\,eject");
void GetEjectMenu(PPXAPPINFO *info, HMENU hMenuDest, ThSTRUCT *thMenuData, DWORD *PopupID, const TCHAR *name)
{
	TCHAR path[VFPS];
	UINT drivetype;
	PPXCMDENUMSTRUCT work;

	path[0] = '\0';
	PPxEnumInfoFunc(info, '1', path, &work);
	if ( (path[0] == '\0') || (path[1] != ':') ) return;

	path[2] = '\\';
	path[3] = '\0';
	drivetype = GetDriveType(path);
	if ( (drivetype == DRIVE_REMOVABLE) || (drivetype == DRIVE_CDROM) ){
		AppendMenuString(hMenuDest, (*PopupID)++, ((name == NULL) || ( name[0] == '\0')) ? T("eject") : name);
		thprintf(path + 4, TSIZEOF(path) - 4, EjectCmdStr, path[0], path[0]);
		ThAddString(thMenuData, path + 4);
	}
}
// AppendMenu の文字列専用版 --------------------------------------------------
void USEFASTCALL AppendMenuString(HMENU hMenu, UINT id, const TCHAR *string)
{
	AppendMenu(hMenu, MF_ES, id, MessageText(string));
}
void USEFASTCALL AppendMenuCheckString(HMENU hMenu, UINT id, const TCHAR *string, BOOL check)
{
	AppendMenu(hMenu, check ? (MF_ES | MF_CHECKED) : MF_ES,
			id, MessageText(string));
}

PPXDLL BOOL PPXAPI PPxSetMenuInfo(HMENU hMenu, PPXMENUINFO *xminfo)
{
	MENUINFO minfo;

	xminfo->index = 0;
	xminfo->Command = NULL;
	if ( DGetMenuInfo == NULL ){
		HKEY HK;

		if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, winedetectpath, 0, KEY_READ, &HK) == ERROR_SUCCESS ){ // Wine 対策
			DGetMenuInfo = INVALID_VALUE(impGetMenuInfo);
			RegCloseKey(HK);
			DSetMenuInfo = NULL;
			return FALSE;
		}

		if ( NULL == LoadWinAPI(User32DLL, NULL,
					MENUINFODLL, LOADWINAPI_GETMODULE) ){
			DGetMenuInfo = INVALID_VALUE(impGetMenuInfo);
			DSetMenuInfo = NULL;
			return FALSE;
		}
	}
	if ( DSetMenuInfo == NULL ) return FALSE;

	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIM_APPLYTOSUBMENUS | MIM_MENUDATA | MIM_STYLE | MIM_HELPID;
	minfo.dwStyle = 0; //MNS_DRAGDROP;
	minfo.dwContextHelpID = HELPID_DATAISSTRUCT;
	minfo.dwMenuData = (DWORD_PTR)xminfo;
	DSetMenuInfo(hMenu, &minfo);
	return TRUE;
}

PPXMENUINFO *GetPPxMenuInfo(HMENU *hMenu)
{
	MENUINFO minfo;
	PPXMENUINFO *xminfo;

	if ( DSetMenuInfo == NULL ) return NULL;

	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIM_MENUDATA | MIM_HELPID;
	if ( DGetMenuInfo(*hMenu, &minfo) == FALSE ) return NULL;
	if ( minfo.dwContextHelpID != HELPID_DATAISSTRUCT ) return NULL;
	xminfo = (PPXMENUINFO *)minfo.dwMenuData;
	if ( IsBadReadPtr(xminfo, sizeof(DWORD)) ) return NULL;
	return xminfo;
}

#define EDITMENU_ADD	1
const TCHAR EDITMENU_ADDstring[] = MES_MEAD;
#define EDITMENU_DELETE	2
const TCHAR EDITMENU_DELETEstring[] = MES_MEDE;
#define EDITMENU_RENAME	3
const TCHAR EDITMENU_RENAMEstring[] = MES_MERE;
#define EDITMENU_MODIFY	4
const TCHAR EDITMENU_MODIFYstring[] = MES_MEMD;
#define EDITMENU_DETAIL	5
const TCHAR EDITMENU_DETAILstring[] = MES_MEDT;

#define MENUNAMEID_ENDSIG '\2'
const TCHAR MenuEndSig[] = {MENUNAMEID_ENDSIG, '\0'};
#define MAX_MENU_NAME_LEVEL 10

// 編集するときのカスタマイズ名を保存する
void SetMenuIDSig(ThSTRUCT *thMenuData, HMENU hMenu, const TCHAR *name)
{
	DWORD size;
	TCHAR buf[CMDLINESIZE];

	if ( thMenuData == NULL ) return;
	size = TSTRSIZE32(name);
	((MENUNAMEID *)buf)->id = MENUNAMEID_SIG;
	((MENUNAMEID *)buf)->hMenu = hMenu;
	memcpy(((MENUNAMEID *)buf)->menuname, name, size);
	ThAppend(thMenuData, buf, sizeof(MENUNAMEID) + size);
}

void SetMenuEndSig(ThSTRUCT *thMenuData, DWORD *PopupID)
{
	if ( thMenuData == NULL ) return;
	ThAppend(thMenuData, MenuEndSig, sizeof(MenuEndSig));
	(*PopupID)++;
}

int OverwriteExistCustDlg(HWND hWnd, const TCHAR *name, const TCHAR *subname)
{
	if ( !IsExistCustTable(name, subname) ) return IDYES;

	return PMessageBox(hWnd, T("Over write item?"), name,
				MB_APPLMODAL | MB_YESNOCANCEL |
				MB_DEFBUTTON1 | MB_ICONQUESTION);
}

// メニュー編集を行う
LRESULT NullPopupMenuEdit(HWND hWnd, HMENU hMenu, DWORD index, int flags)
{
	HMENU hCMenu;
	PPXMENUINFO *xminfo;
	int i;
	POINT pos;
	TCHAR keyname[MAX_PATH];
	DWORD search__id = 0;
	TCHAR *ptr, *maxptr, *custname = NULL;
	TCHAR data[CMDLINESIZE];
	TINPUT tinput;
	int level = 0;
	MENUNAMEID *MenuNameInfo[MAX_MENU_NAME_LEVEL];

										// xminfo を取得
	xminfo = GetPPxMenuInfo(&hMenu);
	if ( xminfo == NULL ) return 0;
										// custname を取得
	if ( xminfo->commandID != 0 ){
		if ( xminfo->commandID >= 0x10000 ){
			xminfo->commandID |= B31; // TAB/BS の押下を通知
			return 0x10000;
		}

		xminfo->index = index;
		xminfo->hMenu = hMenu;
		PPxInfoFunc(xminfo->info, xminfo->commandID, xminfo);
		return 0;
	}
	ptr = ThStrT(&xminfo->th);
	if ( ptr == NULL ) return 0;
	maxptr = ThStrLastT(&xminfo->th);

	for ( ; ; ){
		if ( ptr >= maxptr ) break;
		if ( index <= search__id ) break;
		if ( *ptr == MENUNAMEID_SIG ){
			if ( level < MAX_MENU_NAME_LEVEL ){
				MenuNameInfo[level] = (MENUNAMEID *)ptr;
			}
			ptr += TSIZEOF(MENUNAMEID);
			custname = ptr;
			level++;
		}else if ( *ptr == MENUNAMEID_ENDSIG ){
			if ( level ) level--;
			if ( (level > 0) && (level <= MAX_MENU_NAME_LEVEL) ){
				custname = MenuNameInfo[level - 1]->menuname;
			}else{
				custname = NULL;
			}
			search__id++;
		}else{
			search__id++;
		}
		ptr += tstrlen(ptr) + 1;
	}
	if ( (custname == NULL) || (*custname == '\0') ) return 0;
										// subname を取得・確認
	keyname[0] = '\0';
	// ※下位階層のメニューもこれで取得できる
	GetMenuString(hMenu, index, keyname, TSIZEOF(keyname), MF_BYCOMMAND);

	if ( (keyname[0] == '&') && (keyname[2] == ' ') ){
		keyname[0] = keyname[1];
		keyname[1] = '\0';
	}
	if ( !IsExistCustTable(custname, keyname) ){
		keyname[0] = '\0';
	}
										// メニュー作成
	hCMenu = CreatePopupMenu();
	if ( xminfo != NULL ) AppendMenu(hCMenu, MF_GS, 0, custname);
	AppendMenuString(hCMenu, EDITMENU_ADD, EDITMENU_ADDstring);
	if ( keyname[0] ){
		AppendMenuString(hCMenu, EDITMENU_RENAME, EDITMENU_RENAMEstring);
		AppendMenuString(hCMenu, EDITMENU_MODIFY, EDITMENU_MODIFYstring);
		AppendMenuString(hCMenu, EDITMENU_DELETE, EDITMENU_DELETEstring);
		AppendMenuString(hCMenu, EDITMENU_DETAIL, EDITMENU_DETAILstring);
	}
	if ( flags == MF_BYPOSITION ){ // マウスの場合
		GetCursorPos(&pos);
	}else{ // キーボードの場合
		RECT box;

		GetWindowRect(FindWindow(T(WNDCLASS_POPUPMENU), NULL), &box); // メニューウィンドウ
		pos.x = (box.left + box.right) / 2;
		pos.y = (box.top + box.bottom) / 2;
	}
	i = TrackPopupMenu(hCMenu,
			TPM_TDEFAULT | TPM_RECURSE, pos.x, pos.y, 0, hWnd, NULL);
	DestroyMenu(hCMenu);

	switch (i){
		case EDITMENU_DELETE:
			if ( IDYES == PMessageBox(hWnd, keyname,
					T("Delete?"), MB_ICONQUESTION | MB_YESNO) ){
				DeleteCustTable(custname, keyname, 0);
			}
			break;
		case EDITMENU_RENAME: {
			TCHAR name[MAX_PATH];
			int result;

			tstrcpy(name, keyname);
			for (;;){
				tinput.hOwnerWnd = NULL;
				tinput.hRtype = PPXH_GENERAL;
				tinput.hWtype = PPXH_GENERAL;
				tinput.title	= T("Rename");
				tinput.buff		= name;
				tinput.size		= TSIZEOF(name);
				tinput.flag		= TIEX_USEREFLINE | TIEX_USESELECT | TIEX_USEINFO;
				tinput.info		= xminfo->info;
				tinput.firstC	= 0;
				tinput.lastC	= 0;
				if ( tInputEx(&tinput) <= 0 ) break;

				result = OverwriteExistCustDlg(hWnd, custname, name);
				if ( result == IDNO ) continue;
				if ( result == IDYES ){
					GetCustTable(custname, keyname, data, sizeof(data));
					SetCustStringTable(custname, name, data, 0);
				}
				break;
			}
			break;
		}

		case EDITMENU_MODIFY:
			GetCustTable(custname, keyname, data, sizeof(data));
			tinput.hOwnerWnd = NULL;
			tinput.hRtype = PPXH_GENERAL;
			tinput.hWtype = PPXH_GENERAL;
			tinput.title	= T("Modify");
			tinput.buff		= data;
			tinput.size		= TSIZEOF(data);
			tinput.flag		= TIEX_USEREFLINE | TIEX_USESELECT | TIEX_USEINFO;
			tinput.info		= xminfo->info;
			tinput.firstC	= 0;
			tinput.lastC	= 0;
			if ( tInputEx(&tinput) > 0 ){
				SetCustStringTable(custname, keyname, data, 0);
			}
			break;

		case EDITMENU_ADD:
			PP_ExtractMacro(hWnd, xminfo->info, NULL, T("&tmp=%FCDB"), data, 0);
			for (;;){
				TCHAR *p;
				int result;

				tinput.hOwnerWnd = NULL;
				tinput.hRtype = PPXH_GENERAL;
				tinput.hWtype = PPXH_GENERAL;
				tinput.title	= T("Add item");
				tinput.buff		= data;
				tinput.size		= TSIZEOF(data);
				tinput.flag		= TIEX_USESELECT | TIEX_USEINFO;
				tinput.firstC	= 0;
				tinput.lastC	= 4;
				tinput.info		= xminfo->info;

				if ( tInputEx(&tinput) <= 0 ) break;

				p = tstrchr(data, '=');
				if ( p == NULL ) break;
				*p++ = '\0';

				result = OverwriteExistCustDlg(hWnd, custname, data);
				if ( result == IDNO ){
					*(p - 1) = '=';
					continue;
				}
				if ( result == IDYES ){
					SetCustStringTable(custname, data, p, 0);
				}
				break;
			}
			break;

		case EDITMENU_DETAIL: {
			PostMessage(xminfo->info->hWnd, WM_CHAR, 0x1f, 0); // メニューを閉じる
			xminfo->Command = HeapAlloc(ProcHeap, 0, TSTROFF(CMDLINESIZE));
			if ( xminfo->Command == NULL ) break;
			thprintf(xminfo->Command, CMDLINESIZE, T("%%Obqs *ppcust /:%s"), custname);
			break;
		}
	}
	return 0;
}


LRESULT CALLBACK NullPopupMenuProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message){
		case WM_MENUCHAR: // ※ lParam は必ず root menu になる
			if ( (LOWORD(wParam) == '\t') || (LOWORD(wParam) == '\x8') ){
				PPXMENUINFO *xminfo;
										// xminfo を取得
				xminfo = GetPPxMenuInfo((HMENU *)&lParam);
				if ( xminfo == NULL ) return 0;
				return NullPopupMenuEdit(hWnd, (HMENU)lParam, xminfo->index, MF_BYCOMMAND);
			}
			return GetCustDword(T("X_menu"), 1) ? 0x10000 : 0;

		case WM_MENUSELECT: { // 選択された項目のindexを記憶
			PPXMENUINFO *xminfo;
										// xminfo を取得
			xminfo = GetPPxMenuInfo((HMENU *)&lParam);
			if ( xminfo != NULL ) xminfo->index = LOWORD(wParam);
			return 0;
		}

		case WM_MENUDRAG:
			return 0;

		case WM_MENURBUTTONUP: { // ※ lParam は必ず root menu になる
			DWORD index;

			index = GetMenuItemID((HMENU)lParam, (int)wParam);
			if ( index == MAX32 ) return 0;
			return NullPopupMenuEdit(hWnd, (HMENU)lParam, index, MF_BYPOSITION);
		}

		case WM_APP: { // メニューを表示開始する
			struct NULLPOPUPSTRUCT *nps;
			UINT flags = TPM_TDEFAULT;

			if ( DSetMenuInfo != NULL ){
				flags = TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RECURSE;
			}
			nps = (struct NULLPOPUPSTRUCT *)lParam;
			nps->result = TrackPopupMenu(nps->hMenu, flags,
					nps->pos.x, nps->pos.y, 0, hWnd, NULL);
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			return 0;
		}

		case WM_CREATE:
			PostMessage(hWnd, WM_APP,
				0, (LPARAM)((CREATESTRUCT *)lParam)->lpCreateParams);
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

PPXDLL LRESULT PPXAPI PPxMenuProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return NullPopupMenuProc(hWnd, Msg, wParam, lParam);
}

void RemoveControlKeydown(HWND hWnd)
{
	MSG WndMsg;

	if ( IsTrue(PeekMessage(&WndMsg, hWnd, WM_CHAR, WM_CHAR, PM_NOREMOVE)) ){
		if ( WndMsg.wParam <= ' ' ){
			PeekMessage(&WndMsg, hWnd, WM_CHAR, WM_CHAR, PM_REMOVE);
		}
	}
}

int TTrackPopupMenu(EXECSTRUCT *Z, HMENU hMenu, PPXMENUINFO *xminfo)
{
	HWND hWnd;
	struct NULLPOPUPSTRUCT nps;

	if ( Z != NULL ){
		GetPopupPoint(Z, &nps.pos);
		hWnd = Z->hWnd;
	}else{
		if ( !PPxInfoFunc(&PPxDefInfo, PPXCMDID_POPUPPOS, &nps.pos) ){
			RECT WndBox;

			GetWindowRect(GetForegroundWindow(), &WndBox);
			GetCursorPos(&nps.pos);
			if ( PtInRect(&WndBox, nps.pos) == FALSE ){
				nps.pos.x = (WndBox.left + WndBox.right) / 2;
				nps.pos.y = (WndBox.top + WndBox.bottom) / 2;
			}
		}
		xminfo = NULL;
		hWnd = NULL;
	}
	if ( ConsoleMode >= ConsoleMode_Console ){
		if ( ConsoleMode == ConsoleMode_ConsoleOnly ){
			return TrackPopupMenuConsole(hMenu);
		}
		hWnd = NULL;//(HWND)PPxInfoFunc(&PPxDefInfo, PPXCMDID_TEMP_WINDOW, hMenu);
	}
	RemoveControlKeydown(hWnd);

	if ( hWnd != NULL ){
		UINT flags = TPM_TDEFAULT;
		int result;

		if ( DSetMenuInfo != NULL ){
			flags = TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RECURSE;
		}
		result = TrackPopupMenu(hMenu, flags, nps.pos.x, nps.pos.y, 0, hWnd, NULL);
		if ( (xminfo == NULL) || (xminfo->Command == NULL) ) return result;
	}else{ // ウィンドウがないとメニューが表示できないので、仮ウィンドウを用意して表示
		HWND hNullWnd, hOldWnd;

		nps.hMenu = hMenu;
		hOldWnd = GetForegroundWindow();
		if ( DLLWndClass.item.PopupMenu == 0 ){
			nullpopupClass.hCursor   = LoadCursor(NULL, IDC_ARROW);
			nullpopupClass.hInstance = DLLhInst;
			DLLWndClass.item.PopupMenu = RegisterClass(&nullpopupClass);
		}
		hNullWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
				NULLPOPUPCLASSNAME, NULLPOPUPCLASSNAME,
				0, 0, 0, 0, 0, hWnd, NULL, DLLhInst, (LPVOID)&nps);
		ForceSetForegroundWindow(hNullWnd);
		while ( IsWindow(hNullWnd) ){
			MSG msg;

			if ( (int)GetMessage(&msg, NULL, 0, 0) <= 0 ) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		SetForegroundWindow(hOldWnd);
		DestroyWindow(hNullWnd);
		if ( (xminfo == NULL) || (xminfo->Command == NULL) ) return nps.result;
	}

	PP_ExtractMacro(xminfo->info->hWnd, xminfo->info, NULL, xminfo->Command, NULL, 0);
	HeapFree(ProcHeap, 0, xminfo->Command);
	return 0;
}


const TCHAR * USEFASTCALL GetMenuDataString(ThSTRUCT *thMenuData, int index)
{
	const TCHAR *p;

	GetMenuDataMacro2(p, thMenuData, index);
	if ( p == NULL ) p = NilStr;
	return p;
}

void SetMenuIndextext(EXECSTRUCT *Z, HMENU hMenu, UINT menuID)
{
	TCHAR buf[CMDLINESIZE];
	ThSTRUCT *thSV;

	GetMenuString(hMenu, menuID, buf, TSIZEOF(buf), MF_BYCOMMAND);
	thSV = (ThSTRUCT *)PPxInfoFunc(Z->Info, PPXCMDID_GETWNDVARIABLESTRUCT, NULL);
	if ( thSV == NULL ) thSV = &ProcessStringValue;
	ThSetString(thSV, T("Menu_Index"), buf);
}

void GetAuxMenu(PPXAPPINFO *info, ThSTRUCT *thMenuData, const TCHAR *exmenu, DWORD *PopupID, HMENU hMenuDest)
{
	TCHAR buf[VFPS], path[VFPS];
	PPXCMDENUMSTRUCT work;

	buf[0] = '\0';
	PPxEnumInfoFunc(info, '1', buf, &work);
	if ( (buf[0] != 'a') || (buf[1] != 'u') || (buf[2] != 'x') || (buf[3] != ':') ){
		return;
	}
	SplitAuxName(buf, path);
	path[0] = 'M';
	path[1] = '_';
	tstrcat(path, T("."));
	tstrcat(path, exmenu + 3);

	SetMenuIDSig(thMenuData, hMenuDest, path);
	PP_AddMenu(info, info->hWnd, hMenuDest, PopupID, path, thMenuData);
	SetMenuEndSig(thMenuData, PopupID);
}

void SaveMenuData(EXECSTRUCT *Z, int menuflag, TCHAR *menuname, HMENU hMenu, ThSTRUCT *thMenuData, int menuID)
{
	const TCHAR *newsrc;

	newsrc = GetMenuDataString(thMenuData, menuID - MCID);
	if ( menuflag & MENUFLAG_NOEXTACT ){
		if ( *newsrc == '!' ){
			PP_ExtractMacro(Z->Info->hWnd, Z->Info, NULL, newsrc + 1, NULL, 0);
			Z->result = ERROR_CANCELLED;
			return;
		}
		tstrcpy(Z->dst, newsrc);
		Z->dst += tstrlen(newsrc);
	}else{
		ThAddString(&Z->ExpandCache, menuname);
		ThAddString(&Z->ExpandCache, newsrc);
		InsertSrc(Z, newsrc);
	}
	if ( menuflag & MENUFLAG_SETINDEX ){
		SetMenuIndextext(Z, hMenu, menuID);
	}
}
// メニュー名指定がないときに表示するメニュー
BOOL MenuMenu(EXECSTRUCT *Z)
{
	HMENU hPopupMenu = CreatePopupMenu();
	int count = 0, index;
	TCHAR key[CUST_NAME_LENGTH], comment[CMDLINESIZE], text[CMDLINESIZE * 2 + 8], *str;

	while( EnumCustData(count, key, key, 0) >= 0 ){
		if ( (key[1] == '_') && (upper(key[0]) == 'M') ){
			comment[0] = '\0';
			GetCustTable(T("#Comment"), key, comment, sizeof(comment));
			if ( comment[0] == '\0' ){
				str = key;
			}else{
				thprintf(text, TSIZEOF(text), T("%s\t%s"), key, comment);
				str = text;
			}
			AppendMenuString(hPopupMenu, count + 1, str);
		}
		count++;
	}
	index = TTrackPopupMenu(Z, hPopupMenu, NULL);
	if ( index > 0 ){
		TCHAR *p;

		GetMenuString(hPopupMenu, index, text, TSIZEOF(text), MF_BYCOMMAND);
		p = tstrchr(text, '\t');
		if ( p != NULL ) *p = '\0';
		DestroyMenu(hPopupMenu);
		return MenuCommand(Z, text, NilStr, 0);
	}
	DestroyMenu(hPopupMenu);
	Z->result = ERROR_NO_MORE_ITEMS;
	return FALSE;
}
/*-----------------------------------------------------------------------------
	%M 本体
-----------------------------------------------------------------------------*/
BOOL MenuCommand(EXECSTRUCT *Z, TCHAR *menuname, const TCHAR *def, int menuflag)
{
	PPXMENUINFO xminfo;
	HMENU hPopupMenu;
	DWORD id = MCID;

	if ( Z->ExpandCache.top ){
		TCHAR *p;

		p = (TCHAR *)Z->ExpandCache.bottom;
		while( *p ){
			if ( !tstrcmp(p, menuname) ){
				InsertSrc(Z, p + tstrlen(p) + 1);
				p = NULL;
				break;
			}
			p += tstrlen(p) + 1;
			p += tstrlen(p) + 1;
		}
		if ( p == NULL ) return FALSE;
	}

	ThInit(&xminfo.th);

	RemoveControlKeydown(Z->hWnd); // Enter があるとビープ音が出るので廃棄

	if ( menuname[1] == ':' ){ // %M:M_xxxx / %M:?xxx
		setflag(menuflag, MENUFLAG_NOEXTACT);
		menuname += (menuname[2] == '?') ? 1 : 2;
	}

	if ( menuname[1] == '?' ){
		hPopupMenu = CreatePopupMenu();
		ExMenuAdd(Z->Info, &xminfo.th, hPopupMenu, menuname + 1, NULL, &id);
	}else{
		hPopupMenu = PP_AddMenu(Z->Info, Z->hWnd, NULL, &id, menuname, &xminfo.th);
	}

	if ( hPopupMenu == NULL ){
		if ( menuname[1] == '\0' ) return MenuMenu(Z);
		XMessage(NULL, NULL, XM_GrERRld, MES_EMEU, menuname);
		Z->result = ERROR_NO_MORE_ITEMS;
		return FALSE;
	}else{
		UINT menuID;

		xminfo.info = Z->Info;
		xminfo.commandID = 0;
		PPxSetMenuInfo(hPopupMenu, &xminfo);
		menuID = 0;

		for(;;){
			TCHAR type;
			int index;
			MENUITEMINFO minfo;

			if ( (def[0] != '?') || (def[2] != ':') ) break;
			type = def[1];
			if ( (type != 'b') && (type != 'c') ) break;

			def += 3;
			index = GetIntNumber(&def);

			minfo.cbSize = sizeof(minfo);
			minfo.fMask = MIIM_STATE | MIIM_FTYPE;
			GetMenuItemInfo(hPopupMenu, index, MF_BYPOSITION, &minfo);
			minfo.fState |= MFS_CHECKED;
			if ( type == 'b' ) minfo.fType |= MFT_RADIOCHECK;
			SetMenuItemInfo(hPopupMenu, index, TRUE, &minfo);
			if ( *def != ';' ) break;
			def++;
		}

		if ( *def != '\0' ){ // 初期選択を行う
			int offset;
			UINT mID;

			mID = FindMenuItem(hPopupMenu, def, &offset);
			if ( mID != 0 ){
				if ( menuflag & MENUFLAG_SELECT ){ // 即時選択
					menuID = mID;
				}else{ // カーソル移動 ---------
					for ( ; offset >= 0 ; offset-- ){
						PostMessage(Z->hWnd, WM_KEYDOWN, VK_DOWN, 0);
						PostMessage(Z->hWnd, WM_KEYUP, VK_DOWN, 0);
					}
				}
			}
		}

		if ( menuID == 0 ){
			menuID = TTrackPopupMenu(Z, hPopupMenu, &xminfo);
			if ( menuID <= 0 ) EndButtonMenu();
		}
		if ( menuID != 0 ){
			if ( menuID >= IDW_INTERNALMIN ){
				EXECEXMENUINFO execmenu;

				execmenu.hMenu = hPopupMenu;
				execmenu.index = menuID;
				PPxInfoFunc(Z->Info, PPXCMDID_EXECEXMENU, &execmenu);
			}else{
				if ( menuID == IDW_MENU_REGPJUMP ){
					RegisterFavoriteItem(Z->Info);
					Z->result = ERROR_CANCELLED;
				}else{
					SaveMenuData(Z, menuflag, menuname, hPopupMenu, &xminfo.th, menuID);
				}
			}
		}else{
			Z->result = ERROR_CANCELLED;
		}
		DestroyMenu(hPopupMenu);
	}
	ThFree(&xminfo.th);
	return TRUE;
}

BOOL ExMenuAdd(PPXAPPINFO *info, ThSTRUCT *thMenuData, HMENU hMenu, const TCHAR *exmenu, const TCHAR *name, DWORD *PopupID)
{
	HMENU hMenuDest;
	ADDEXMENUINFO addmenu;
	HWND hWnd;
	TCHAR namebuf[CMDLINESIZE];

	if ( *exmenu == '?' ){ // 埋め込み指示あり
		hMenuDest = hMenu;
		exmenu++;
	}else if ( name == NULL ){ // 強制埋め込み
		hMenuDest = hMenu;
	}else{ // 下層メニュー
		if ( info != NULL ){
			PP_ExtractMacro(info->hWnd, info, NULL, name, namebuf, 0);
		}else{
			tstrcpy(namebuf, name);
		}
		hMenuDest = CreatePopupMenu();
		AppendMenu(hMenu, MF_EPOP, (UINT_PTR)hMenuDest, namebuf);
	}

	if ( *exmenu == 'M' ){ // M_xxx
		SetMenuIDSig(thMenuData, hMenuDest, exmenu);
		hWnd = (info == NULL) ? NULL : info->hWnd;
		PP_AddMenu(info, hWnd, hMenuDest, PopupID, exmenu, thMenuData);
		SetMenuEndSig(thMenuData, PopupID);
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("favorites")) ){
		AppendMenuString(hMenuDest, IDW_MENU_REGPJUMP, MES_PJAD);
		AppendMenu(hMenuDest, MF_SEPARATOR, 0, NULL);

		SetMenuIDSig(thMenuData, hMenuDest, PathJumpName);
		hWnd = (info == NULL) ? NULL : info->hWnd;
		PP_AddMenu(info, hWnd, hMenuDest, PopupID, PathJumpNameEx, thMenuData);
		SetMenuEndSig(thMenuData, PopupID);
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("packlist")) ){
		GetPackMenu(hMenuDest, thMenuData, PopupID);
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("ppclist")) ){
		GetPPxList(hMenuDest, GetPPcList_Path, thMenuData, PopupID);
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("ppxidlist")) ){
		GetPPxList(hMenuDest, GetPPxList_Id, thMenuData, PopupID);
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("selectppx")) ){
		GetPPxList(hMenuDest, GetPPxList_Select, thMenuData, PopupID);
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("eject")) ){
		if ( info != NULL ){
			if ( name != NULL ){
				PP_ExtractMacro(info->hWnd, info, NULL, name, namebuf, 0);
				name = namebuf;
			}
			GetEjectMenu(info, hMenuDest, thMenuData, PopupID, name);
		}
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("additem")) ){ // ?additem
		AppendMenuString(hMenuDest, IDW_MENU_REGPJUMP, MES_PJAD);
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("extdrivemenu")) ){
		ExtDriveMenu(hMenuDest, thMenuData, PopupID, TRUE);
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("extdrivelist")) ){
		ExtDriveMenu(hMenuDest, thMenuData, PopupID, FALSE);
		return TRUE;
	}else if ( !tstrcmp(exmenu, T("winhashlist")) ){
		GetWindowHashList(hMenuDest, PopupID);
		return TRUE;
	}
	else if (exmenu[0] == '>') { // メニューツールバー
		int count;
		PPXINMENUBAR* inmp;

		exmenu++;
		count = GetNumber(&exmenu);
		if ( info != NULL ) {
			inmp = (PPXINMENUBAR*)PPxInfoFunc(info, PPXCMDID_GETINMENUBAR, NULL);
			if ( inmp != PPXA_INVALID_FUNCTION ) {
				for (; inmp->name != NULL; inmp++) {
					if ( --count <= 0 ) {
						AddInternalMenu(hMenuDest, inmp->menus, info, PopupID, thMenuData);
						return TRUE;
					}
				}
			}
		}
		return FALSE;
	}else if ( info != NULL ){
		if ( (exmenu[0] == 'a') && (exmenu[1] == 'u') && (exmenu[2] == 'x') ){
			GetAuxMenu(info, thMenuData, exmenu, PopupID, hMenuDest);
			return TRUE;
		}

		addmenu.hMenu = hMenuDest;
		addmenu.exname = exmenu;
		addmenu.TH = thMenuData;
		addmenu.index = PopupID;
		if ( PPxInfoFunc(info, PPXCMDID_ADDEXMENU, &addmenu)
				== PPXCMDID_ADDEXMENU){
			return TRUE;
		}
	}
	AppendMenuString(hMenuDest, 0, exmenu);
	return FALSE;
}

typedef struct
{
	UINT cbSize;
	UINT fMask;
	UINT fType;
	UINT fState;
	UINT wID;
	HMENU hSubMenu;
	HBITMAP hbmpChecked;
	HBITMAP hbmpUnchecked;
	ULONG_PTR dwItemData;
	LPTSTR dwTypeData;
	UINT cch;
	HBITMAP hbmpItem;
} xMENUITEMINFO;
#ifndef MIIM_BITMAP
	#define MIIM_BITMAP      0x00000080
#endif

/*-----------------------------------------------------------------------------
	hWnd	親ウィンドウ
	hMenu	挿入するメニューハンドル、NULL なら内部で生成
	id		登録を開始する識別番号、使用された分だけ増える
	Cname	メニュー定義がされているカスタマイズ名
	TH		登録した順に文字列が格納される、ThInit などで初期化しておくこと
-----------------------------------------------------------------------------*/
PPXDLL HMENU PPXAPI PP_AddMenu(PPXAPPINFO *ParentInfo, HWND hWnd, HMENU hMenu, DWORD *id, const TCHAR *Cname, ThSTRUCT *thMenuData)
{
	const TCHAR *headerptr;
	TCHAR keyword[CMDLINESIZE], param[CMDLINESIZE], buf[CMDLINESIZE];
	int count = 0;
	int height = 0, heightMax, heightStep;	// メニューの高さ調整用
	int menuflag;
	HMENU hTmpMenu;
	HBITMAP hIconBmp = NULL;
	int headersize = 0;

	if ( *Cname == '<' ){
		const TCHAR *lastptr;

		lastptr = tstrchr(Cname, '>');
		if ( lastptr != NULL ){
			headerptr = Cname + 1;
			headersize = (lastptr - headerptr) * sizeof(TCHAR);
			Cname = lastptr + 1;
		}
	}
										// 初期化 -----------------------------
	hTmpMenu = hMenu;
	if ( hTmpMenu == NULL ){
		hTmpMenu = CreatePopupMenu();
		if ( hTmpMenu == NULL ) return NULL;

		if ( *(Cname + 1) == '@' ){  // PPc [0]
			Cname += 2;
			AppendMenuString(hTmpMenu, IDW_MENU_REGPJUMP, MES_PJAD);
			AppendMenu(hTmpMenu, MF_SEPARATOR, 0, NULL);
		}

		if ( *(Cname + 1) == '?' ){
			if ( FALSE == ExMenuAdd(ParentInfo, thMenuData, hTmpMenu, Cname + 2, NULL, id) ){
				DestroyMenu(hTmpMenu);
				hTmpMenu = NULL;
			}
			return hTmpMenu;
		}

		SetMenuIDSig(thMenuData, hTmpMenu, Cname);
	}else if ( *(Cname + 1) == '?' ){
		if ( FALSE == ExMenuAdd(ParentInfo, thMenuData, hMenu, Cname + 2, NULL, id) ){
			return NULL;
		}
		return hMenu;
	}

	if ( MenuEnterCount > 30 ) return NULL;

	heightMax = GetSystemMetrics(SM_CYFULLSCREEN);
	heightStep = GetSystemMetrics(SM_CYMENU);
	MenuEnterCount++;
										// 列挙の開始 -------------------------
	param[CMDLINESIZE - 1] = '\0';
	while( EnumCustTable(count, Cname, keyword, param, sizeof(param)) >= 0 ){
		const TCHAR *paramp;
		UTCHAR code;

		count++;
		menuflag = MF_ENABLED | MF_STRING; // ※ どちらの値も0
		if ( height >= heightMax ){				// 改桁処理 ==================
			height = 0;
			setflag(menuflag, MF_MENUBARBREAK);
		}
		if ( (keyword[2] == '\0') || (keyword[2] == '.') ){
			if ( (keyword[0] == '-') && (keyword[1] == '-') ){ // セパレータ ==
				AppendMenu(hTmpMenu, MF_SEPARATOR | menuflag, 0, NULL);
				continue;
			}else if ( (keyword[0] == '|') && (keyword[1] == '|') ){ // 改桁 ==
				height = heightMax;
				continue;
			}
		}
		paramp = param;
		code = SkipSpace(&paramp);
												// 階層メニュー ==============
		if ( (code == '%') && (paramp[1] == 'M') && (paramp[2] != 'E') ){
			HMENU hSubMenu;

			GetCommandParameter(&paramp, buf, TSIZEOF(buf));
			hSubMenu = PP_AddMenu(ParentInfo, hWnd, NULL, id, &buf[1], thMenuData);
			if ( hSubMenu == NULL ){
				buf[0] = '\"';
				tstrcat(&buf[1], T("\"Create error"));
				AppendMenu(hTmpMenu, menuflag, 0, buf);
			}else{
				AppendMenu(hTmpMenu, MF_POPUP | menuflag,
						(UINT_PTR)hSubMenu, keyword);
				SetMenuEndSig(thMenuData, id);
			}
		}else{								// 通常の項目 ================
			TCHAR *itemname;
			UTCHAR *itemnameptr;
			int showlen;

			// メニューに表示する文字列を準備する
			if ( keyword[0] != '\0' ){ // keyword が有効
				itemname = keyword;
				// param が長い場合は、keyword を参照するので加工用コピーを用意
				if ( param[CMDLINESIZE - 1] != '\0' ){
					tstrcpy(buf, itemname);
					itemname = buf;
				}

				if ( itemname[1] == '\0' ){	// キーワードが１文字の処理
					itemname[1] = itemname[0];
					itemname[0] = '&';
					itemname[2] = ' ';
					tstplimcpy(&itemname[3], param, TSIZEOF(keyword) - 3);
				}else{
					TCHAR *wp;

					// tab の展開
					for ( wp = itemname ; *wp ; wp++ ){
#ifndef UNICODE
						if ( IskanjiA(*wp) ){
							wp++;
							continue;
						}
#endif
						if ( *wp != '\\' ) continue;
						wp++;
						if ( *wp == 't' ){
							*(wp - 1) = '\t';
							memmove(wp, wp + 1, TSTRLENGTH(wp));
							break;
						}
						if ( (*wp == '\\') && (*(wp + 1) == 't') ){
							memmove(wp, wp + 1, TSTRLENGTH(wp));
							break;
						}
					}
				}
			}else{
				itemname = param;
			}

			if ( code == '?' ){	// 拡張 ================
				if ( (menuflag & MF_MENUBARBREAK) && (*(paramp + 1) == '?') ){
					// 拡張だと改桁を登録できなくなるので代用品を挿入
					AppendMenu(hTmpMenu, MF_SEPARATOR | MF_MENUBARBREAK, 0, NULL);
				}
				if ( IsTrue(ExMenuAdd(ParentInfo, thMenuData, hTmpMenu,
						paramp + 1, keyword, id)) ){
					continue;
				}
			}
			if ( thMenuData != NULL ){
				if ( headersize && !ThAppend(thMenuData, headerptr, headersize) ){
					break;
				}
				if ( param[CMDLINESIZE - 1] == '\0' ){
					if ( ThAddString(thMenuData, param) == FALSE ) break;
				}else{
					DWORD size;
					TCHAR *longparamptr;
					BOOL radd;

					param[CMDLINESIZE - 1] = '\0'; // 次の Enum のための初期化
					longparamptr = GetCustValue(Cname, keyword, &size, 0);
					if ( longparamptr != NULL ){
						radd = ThAddString(thMenuData, longparamptr);
						HeapFree(ProcHeap, 0, longparamptr);
						if ( radd == FALSE ) break;
					}
				}
			}

			PP_ExtractMacro(hWnd, ParentInfo, NULL, itemname, itemname, XEO_DISPONLY);

			// 表示内容のオプション
			for ( ; itemname[0] == '?' ; itemname += 3 ){
				if ( itemname[1] == ';' ){
					itemname--;
					continue;
				}

				if ( itemname[1] == '<' ){
					itemname += 2;
					if ( hIconBmp != NULL ) DeleteObject(hIconBmp);
					hIconBmp = CreateScriptBitmap((const TCHAR **)&itemname, heightStep, heightStep, ICONTYPE_TOOLBAR_O);
					if ( itemname[0] == ';' ) itemname++;
					itemname -= 3;
					continue;
				}

				if ( itemname[2] == ';' ) switch ( itemname[1] ){
					case '0': // 何もしない
						continue;
					case 'b':
						setflag(menuflag, MF_CHECKED | MF_USECHECKBITMAPS); // MFT_RADIOCHECK
						continue;
					case 'c':
					case '1':
						setflag(menuflag, MF_CHECKED);
						continue;
					case 'd':
						setflag(menuflag, MF_DISABLED);
						continue;
					case 'g':
						setflag(menuflag, MF_GRAYED);
						continue;
					case 'h':
						setflag(menuflag, MF_HILITE);
						continue;
					case 'r':
						setflag(menuflag, MFT_RIGHTORDER);
						continue;
				}
				break;
			}

			// 長さ制限を行う
			showlen = X_mwid;
			itemnameptr = (UTCHAR *)itemname;
			for ( ;; ){
				if ( *itemnameptr == '\0' ) break;
#ifndef UNICODE
				if ( IskanjiA(*itemnameptr) ){
					if ( showlen >= 2 ){
						itemnameptr++;
						showlen--;
					}else{
						break;
					}
				}
#endif
				itemnameptr++;
				showlen--;
				if ( showlen == 0 ){
					if ( *itemnameptr ){
						itemnameptr[0] = '.';
						itemnameptr[1] = '.';
						itemnameptr[2] = '.';
						itemnameptr += 3;
					}
					break;
				}
			}
			*itemnameptr = '\0';
			AppendMenu(hTmpMenu, menuflag, *id, itemname);

			if ( hIconBmp != NULL ){
				xMENUITEMINFO mii;

				mii.cbSize = sizeof(xMENUITEMINFO);
				mii.fMask = MIIM_BITMAP;
				mii.hbmpItem = hIconBmp;
				SetMenuItemInfo(hTmpMenu, *id, FALSE, (MENUITEMINFO *)&mii);
				hIconBmp = NULL;
			}

			(*id)++;
		}
		height += heightStep;
	}
	if ( (count == 0) && (hMenu == NULL) ){
		DestroyMenu(hTmpMenu);
		hTmpMenu = NULL;
	}
//	if ( hIconBmp != NULL ) DeleteObject(hIconBmp);
	MenuEnterCount--;
	return hTmpMenu;
}

PPXDLL UINT PPXAPI FindMenuItem(HMENU hMenu, const TCHAR *name, int *menuPosition)
{
	int index = 0, excheck = 0, menuPos = 0;
	MENUITEMINFO minfo;
	TCHAR buf[CMDLINESIZE], *p, cmdkey = '\0';

	if ( *name == '\0' ){
		excheck = 1;
	}else if ( *(name + 1) == '\0' ){
		excheck = 1;
		cmdkey = upper(*name);
	}
	for ( ; ; index++ ){
		minfo.cbSize = sizeof(minfo); // GetMenuItemInfo の度に内容が破壊？
		minfo.fMask = (WinType < WINTYPE_2000) ?
				(MIIM_STATE | MIIM_TYPE | MIIM_ID) :
				(MIIM_STATE | MIIM_FTYPE | MIIM_STRING | MIIM_ID);
		minfo.cch = VFPS;
		minfo.dwTypeData = buf;
		if ( GetMenuItemInfo(hMenu, index, MF_BYPOSITION, &minfo) == FALSE ) break;
		if ( !(minfo.fType & (MFT_SEPARATOR | MFT_MENUBARBREAK | MFT_MENUBREAK)) ){
			if ( excheck ){
				if ( cmdkey ){
					p = tstrchr(buf, '&');
					if ( upper((p != NULL) ? *(p + 1) : buf[0]) == cmdkey ){
						goto found;
					}
				}else{
					if ( minfo.fState & MFS_DEFAULT ) goto found;
				}
			}else{
				#pragma warning(suppress:6054) // GetMenuItemInfo でbufが用意される
				if ( tstricmp(buf, name) == 0 ) goto found;
			}
			menuPos++;
		}else if ( minfo.fType & (MFT_MENUBARBREAK | MFT_MENUBREAK) ){
			menuPos++;
		}
	}
	return 0;
found:
	if ( menuPosition != NULL ) *menuPosition = menuPos;
	return minfo.wID;
}

void MakeRootMenu(HMENU hRootMenu, const TCHAR *CustName, const PPXINMENUBAR *inmenu)
{
	int count = 0;
	TCHAR keyword[CUST_NAME_LENGTH], param[CMDLINESIZE];

	if ( CustName != NULL ){ // 外部メニュー
		while( EnumCustTable(count, CustName, keyword, param, sizeof(param)) >= 0){
			AppendMenuString(hRootMenu, count + IDW_MENU, keyword);
			count++;
		}
	}
	if ( count == 0 ){ // 内蔵メニュー
		const PPXINMENUBAR *inmp;

		for ( inmp = inmenu ; inmp->name != NULL ; inmp++ ){
			AppendMenuString(hRootMenu, count + IDW_MENU, inmp->name);
			count++;
		}
	}
}

PPXDLL void PPXAPI FreeDynamicMenu(DYNAMICMENUSTRUCT *dms)
{
	ThFree(&dms->thMenuData);
}

PPXDLL HMENU PPXAPI InitDynamicMenu(DYNAMICMENUSTRUCT *dms, const TCHAR *barname /* static! */, const PPXINMENUBAR *inmenu)
{
	int count = 0, countmax;
	TCHAR keyword[CUST_NAME_LENGTH], param[CMDLINESIZE];
	HMENU hMenuBar;

	ThInit(&dms->thMenuData);
	dms->Sysmenu = FALSE;
	dms->MenuName = barname;
	dms->BarIDmin = IDW_MENU;
	dms->hMenuBarMenu = hMenuBar = CreateMenu();

	countmax = CountCustTable(barname);
	if ( countmax > 0 ) {
		dms->hMenuPopups = HeapAlloc(ProcHeap, 0, sizeof(HMENU) * countmax);
		if ( dms->hMenuPopups != NULL ) {
			while ( EnumCustTable(count, barname, keyword, param, sizeof(param)) >= 0 ){
				AppendMenuString(hMenuBar, count + IDW_MENU, keyword);
				dms->hMenuPopups[count] =
					(((param[0] == '%') && (param[1] == 'M')) || (param[0] == '?')) ?
					INVALID_HANDLE_VALUE : NULL;
				count++;
			}
		}
	}else { // 内蔵メニューを使用
		const PPXINMENUBAR* inmp;

		countmax = 0;
		dms->MenuName = NULL;
		dms->inmenu = inmenu;

		for ( inmp = inmenu; inmp->name != NULL; inmp++ ) {
			countmax++;
		}
		dms->hMenuPopups = HeapAlloc(ProcHeap, 0, sizeof(HMENU) * countmax);
		if ( dms->hMenuPopups != NULL ) {
			for ( inmp = inmenu; count < countmax; inmp++ ) {
				AppendMenuString(hMenuBar, count + IDW_MENU, inmp->name);
				dms->hMenuPopups[count] = INVALID_HANDLE_VALUE;
				count++;
			}
		}
	}
	dms->BarIDmax = count + IDW_MENU;
	return hMenuBar;
}

PPXDLL void PPXAPI DynamicMenu_InitMenu(DYNAMICMENUSTRUCT *dms, HMENU hMenuBar, BOOL showbar)
{
	DWORD offset, offset_max;
	MENUITEMINFO minfo;

	if ( hMenuBar != dms->hMenuBarMenu ){
		if ( IsTrue(showbar) ){
			dms->Sysmenu = FALSE;
			return;
		}else{
			if ( dms->Sysmenu == FALSE ) return;
			hMenuBar = NULL;
		}
	}
	// 項目の初期化
	ThFree(&dms->thMenuData);
	dms->PopupID = dms->BarIDmax + 1;

	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_SUBMENU;

	if ( GetMenuItemID(dms->hSystemMenu, dms->SysmenuOffset) != IDW_MENU ){
		int pos = 0, posmax;

		posmax = GetMenuItemCount(dms->hSystemMenu);
		for ( ; pos < posmax ; pos++ ){
			if ( GetMenuItemID(dms->hSystemMenu, pos) == IDW_MENU ){
				dms->SysmenuOffset = pos;
				break;
			}
		}
	}

	offset = 0;
	offset_max = dms->BarIDmax - dms->BarIDmin;
	for ( ; offset < offset_max ; offset++ ){
		if ( dms->hMenuPopups[offset] == NULL ) continue;

		dms->hMenuPopups[offset] = minfo.hSubMenu = CreatePopupMenu();
		if ( hMenuBar != NULL ){
			SetMenuItemInfo(hMenuBar, offset, TRUE, &minfo);
		}
		SetMenuItemInfo(dms->hSystemMenu, offset + dms->SysmenuOffset, TRUE, &minfo);
	}
	dms->Sysmenu = FALSE;
}

void AddInternalMenu(HMENU hTopMenu, const PPXINMENU *menus, PPXAPPINFO *info, DWORD *PopupID, ThSTRUCT *thMenuData)
{
	DWORD layer = 1;
	HMENU hMenus[3], hMenu;
	TCHAR keybuf[64];
	TCHAR strbuf[0x200];

	keybuf[0] = '%';
	keybuf[1] = 'K';
	keybuf[2] = '\"';

	hMenus[0] = hTopMenu;
	while( menus->key != 0 ){
		DWORD key;

		layer--;
		hMenu = hMenus[layer];
		while ( 0 != (key = (DWORD)menus->key) ){
			if ( menus->str != NULL ){
				const TCHAR *str;

				if ( *menus->str == '%' ){
					PP_ExtractMacro(NULL, NULL, NULL, menus->str, strbuf, 0);
					str = strbuf;
				}else{
					str = menus->str;
				}

				if ( thMenuData == NULL ){
					AppendMenuString(hMenu, menus->key | K_M, str);
				}else{
					if ( key >= 0x10000 ){
						if ( *(const TCHAR *)menus->key == '?' ){
							ExMenuAdd(info, thMenuData, hMenu,
							 (const TCHAR *)menus->key + 1, str, PopupID);
							menus++;
							continue;
						}
						ThAddString(thMenuData, (const TCHAR *)menus->key);
					}else{
						if ( !(key & K_ex) ){
							if ( (key & K_v) || !Islower(menus->key) ){
								key |= K_raw;
							}else{
								key -= 0x20;
							}
						}
						PutKeyCode(keybuf + 3, key);
						ThAddString(thMenuData, keybuf);
					}
					AppendMenu(hMenu, MF_ES, (*PopupID)++, str);
				}
			}else{
				if ( key == 1 ){ // 下層
					layer++;
					hMenu = hMenus[layer] = CreatePopupMenu();
				}else{
					AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				}
			}
			menus++;
		}
		if ( layer > 0 ){
			AppendMenu(hMenus[layer - 1], MF_EPOP, (UINT_PTR)hMenu, menus->str);
			menus++;
		}
	}
}

PPXDLL BOOL PPXAPI DynamicMenu_InitPopupMenu(DYNAMICMENUSTRUCT *dms, HMENU hPopupMenu, PPXAPPINFO *info)
{
	DWORD index;
	TCHAR keyword[CUST_NAME_LENGTH], param[CMDLINESIZE];

	for ( index = dms->BarIDmin ; index < dms->BarIDmax ; index++ ){
		int offset = index - dms->BarIDmin;

		if ( dms->hMenuPopups[offset] == hPopupMenu ){
			if ( GetMenuItemCount(hPopupMenu) == 0 ){
				if ( dms->MenuName != NULL ){ // 外部メニュー
					if ( EnumCustTable(offset, dms->MenuName, keyword, param, sizeof(param)) > 0 ){
						if ( param[0] == '?' ){
							ExMenuAdd(info, &dms->thMenuData, hPopupMenu,
									(param[1] == '?') ? param + 1 : param,
									NULL, &dms->PopupID);
						}else{
							PP_AddMenu(info, info->hWnd, hPopupMenu, &dms->PopupID, param + 1, &dms->thMenuData);
						}
					}
				}else{ // 内蔵メニュー
					AddInternalMenu(hPopupMenu, dms->inmenu[offset].menus, info, &dms->PopupID, &dms->thMenuData);
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}

PPXDLL void PPXAPI InitSystemDynamicMenu(DYNAMICMENUSTRUCT *dms, HWND hWnd)
{
	HMENU hSysMenu = GetSystemMenu(hWnd, FALSE);
	RECT WndBox, DeskBox;

	dms->hSystemMenu = hSysMenu;
	AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL);

	GetWindowRect(hWnd, &WndBox);
	WndBox.left = (WndBox.left + WndBox.right) / 2;
	WndBox.top = (WndBox.top + WndBox.bottom) / 2;
	GetDesktopRect(hWnd, &DeskBox);
	if ( (WndBox.left <= DeskBox.left) ||
		 (WndBox.left >= DeskBox.right) ||
		 (WndBox.top <= DeskBox.top) ||
		 (WndBox.top >= DeskBox.bottom) ){
		AppendMenuString(hSysMenu, IDW_MENU_POSFIX, SetWindowPosToDeskStr);
		AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL);
	}
	dms->SysmenuOffset = GetMenuItemCount(hSysMenu);

	MakeRootMenu(/*NULL, */hSysMenu, dms->MenuName, dms->inmenu);
}

TCHAR USEFASTCALL GetMenuShortcutkey(const TCHAR *menustring)
{
	const TCHAR *p;

	p = tstrchr(menustring, '&');
	if (p){
		p++;
	}else{
		p = menustring;
	}
	return upper(*p);
}

void DynamicMenuExtract(PPXAPPINFO *info, const TCHAR *itemname, const TCHAR *param)
{
	if ( *param == '?' ){
		PPXMENUINFO xminfo;
		HMENU hPopupMenu = CreatePopupMenu();
		DWORD index = 1;
		UINT flags = TPM_TDEFAULT;
		POINT pos;

		ThInit(&xminfo.th);
		if ( *(param + 1) == '?' ) param++;
		ExMenuAdd(info, &xminfo.th, hPopupMenu, param, itemname, &index);

		if ( !PPxInfoFunc(info, PPXCMDID_POPUPPOS, &pos) ){
			GetCursorPos(&pos);
		}
		xminfo.info = info;
		xminfo.commandID = 0;
		PPxSetMenuInfo(hPopupMenu, &xminfo);
		if ( DSetMenuInfo != NULL ){
			flags = TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RECURSE;
		}
		index = TrackPopupMenu(hPopupMenu, flags,
				pos.x, pos.y, 0, info->hWnd, NULL);
		if ( index > 0 ){
			if ( (index > IDW_INTERNALMIN) && (index <= IDW_INTERNALMAX) ){
				SendMessage(info->hWnd, WM_COMMAND, index, 0);
			}else{
				if ( index == IDW_MENU_REGPJUMP ){
					RegisterFavoriteItem(info);
				}else{
					const TCHAR *exeparam;

					exeparam = GetMenuDataString(&xminfo.th, index - 1);
					PP_ExtractMacro(info->hWnd, info, NULL, exeparam, NULL, 0);
				}
			}
		}
		ThFree(&xminfo.th);
		DestroyMenu(hPopupMenu);
	}else{
		PP_ExtractMacro(info->hWnd, info, NULL, param, NULL, 0);
	}
}


PPXDLL void PPXAPI SystemDynamicMenu(DYNAMICMENUSTRUCT *dms, PPXAPPINFO *info, WORD key)
{
	TCHAR c = upper((BYTE)key);
	TCHAR keyword[CUST_NAME_LENGTH], param[CMDLINESIZE];
	int count = 0;

	if ( dms->MenuName != NULL ){ // 外部メニュー
		while( EnumCustTable(count, dms->MenuName, keyword, param, sizeof(param)) >= 0 ){
			if ( GetMenuShortcutkey(keyword) == c ){
				DynamicMenuExtract(info, keyword, param);
				return;
			}
			count++;
		}
	}else{ // 内蔵メニュー
		const PPXINMENUBAR *inmp;

		for ( inmp = dms->inmenu ; inmp->name != NULL ; inmp++ ){

			if ( GetMenuShortcutkey(inmp->name) == c ){
				HMENU hPopupMenu = CreatePopupMenu();
				DWORD index = 1;
				ThSTRUCT thMenuData;
				POINT pos;

				ThInit(&thMenuData);
				AddInternalMenu(hPopupMenu, inmp->menus, info, &index, &thMenuData);

				if ( !PPxInfoFunc(info, PPXCMDID_POPUPPOS, &pos) ){
					GetCursorPos(&pos);
				}
				index = TrackPopupMenu(hPopupMenu, TPM_TDEFAULT,
						pos.x, pos.y, 0, info->hWnd, NULL);
				if ( index > 0 ){
					const TCHAR *extparam;

					extparam = GetMenuDataString(&thMenuData, index - 1);
					DynamicMenuExtract(info, keyword, extparam);
				}
				ThFree(&thMenuData);
				DestroyMenu(hPopupMenu);
				return;
			}
		}
	}
}

void FixWindowPosition(HWND hWnd)
{
	POINT pos;
	RECT desk;
	HWND hParentWnd;

	for (;;){
		hParentWnd = GetParent(hWnd);
		if ( hParentWnd == NULL ) break;
		hWnd = hParentWnd;
	}
	GetCursorPos(&pos);
	GetDesktopRect(WindowFromPoint(pos), &desk);
	SetWindowPos(hWnd, NULL, desk.left, desk.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

PPXDLL void PPXAPI CommandDynamicMenu(DYNAMICMENUSTRUCT *dms, PPXAPPINFO *info, WPARAM wParam)
{
	const TCHAR *ptr;
	TCHAR keyword[CUST_NAME_LENGTH], param[CMDLINESIZE];
	DWORD id = LOWORD(wParam);

	if ( id < dms->BarIDmax ){ // メニューバーから直接実行
		if ( dms->MenuName != NULL ){ // 外部メニュー
			if ( 0 >= EnumCustTable(id - dms->BarIDmin, dms->MenuName, keyword, param, sizeof(param)) ){
				return;
			}
			ptr = param;
		}else{ // 内蔵メニュー ※普通はここに来ないはず
			return; //
		}
	}else{
		if ( id >= IDW_MENU_EX ){
			if ( id == IDW_MENU_REGPJUMP ){
				RegisterFavoriteItem(info);
			}else{ // IDW_MENU_POSFIX
				FixWindowPosition(info->hWnd);
			}
			return;
		}
		ptr = GetMenuDataString(&dms->thMenuData, LOWORD(wParam) - (dms->BarIDmax + 1));
		if ( ptr == NilStr ){
			thprintf(param, TSIZEOF(param), T("Menu err.ID:%x "), (DWORD)wParam);
			GetMenuString(dms->hMenuBarMenu, LOWORD(wParam), param + tstrlen(param), 80, MF_BYCOMMAND);
			PPxCommonExtCommand(K_SENDREPORT, (WPARAM)param);
			return;
		}
	}
	PP_ExtractMacro(info->hWnd, info, NULL, ptr, NULL, 0);
}
