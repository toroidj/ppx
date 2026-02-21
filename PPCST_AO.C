/*-----------------------------------------------------------------------------
	Paper Plane xUI	customizer									AddonÉVÅ[Ég
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPCUST.H"
#pragma hdrstop

const SUSIE_DLL *susiedll_list = NULL;
BYTE *sustrings;
int susiedll_items;
CONFIGURATIONDLG susieconfig = NULL;
int SusieIndex = -1;
const TCHAR LoadingText[] = T("Loading plugin...");

void AddOnSusieSetting(HWND hDlg)
{
	if ( susieconfig == NULL ) return;
	susieconfig(hDlg, 1);
}

void AddOnGetSusieExt(HWND hDlg)
{
	const SUSIE_DLL *sudll;
	char mask[SUSIE_MASK_LENGTH];
	int index = 2;
	size_t offset = 0;

	if ( (susiedll_list == NULL) || (SusieIndex < 0) || (SusieIndex >= susiedll_items) ){
		return;
	}
	sudll = susiedll_list + SusieIndex;
	if ( sudll->hadd == NULL ) return;

	mask[0] = '\0';
	if ( sudll->GetPluginInfo != NULL ){
		for (;;){
			char buf[SUSIE_MASK_LENGTH];

			if ( sudll->GetPluginInfo(index, buf, (int)sizeof(buf) - 1) <= 0 ){
				break;
			}
			buf[sizeof(buf) - 1] = '\0';
			if ( buf[0] != '\0' ){
				if ( (offset != 0) && (mask[offset - 1] != ';') ){
					mask[offset++] = ';';
				}

				for (;;){
					int len;
					char *last;

					len = strlen(buf);
					if ( (offset + len) < SUSIE_MASK_LENGTH ){
						strcpy(mask + offset, buf);
						offset += len;
						break;
					}
					if ( (last = strrchr(buf, ';')) == NULL ){
						break;
					}
					*last = '\0';
				}
			}
			index += 2;
		}
	}
	if ( (offset != 0) && (mask[offset - 1] == ';') ){
		mask[offset - 1] = '\0';
	}
	SetDlgItemTextA(hDlg, IDE_AOSMASK, mask);
	EnableDlgWindow(hDlg, IDB_ADDSETEXT, TRUE);
}

void AddonListSusie(HWND hDlg)
{
	const SUSIE_DLL *sudll;
	int i;
	HDC hDC;

	hDC = GetDC(hDlg);
	TextOut(hDC, 0, 0, LoadingText, TSIZEOFSTR(LoadingText));
	ReleaseDC(hDlg, hDC);

	susiedll_items = VFSGetSusieList(&susiedll_list, &sustrings);
	sudll = susiedll_list;
	for ( i = 0 ; i < susiedll_items ; i++, sudll++ ){
		TCHAR *ptr;

		ptr = (TCHAR *)(sustrings + sudll->DllNameOffset);
		SendDlgItemMessage(hDlg, IDL_AOSUSIE, LB_ADDSTRING, 0, (LPARAM)ptr);
	}
}

void AddonSelectSusie(HWND hDlg)
{
	const SUSIE_DLL *sudll;
	char buf[SUSIE_MASK_LENGTH];
	SUSIE_DLLSTRINGS sp;

	if ( (susiedll_list == NULL) || (SusieIndex < 0) || (SusieIndex >= susiedll_items) ){
		return;
	}
	sudll = susiedll_list + SusieIndex;

	sp.filemask[0] = '\0';
	sp.flags = VFSSUSIE_BMP | VFSSUSIE_ARC;
	GetCustTable(T("P_susie"), (TCHAR *)(sustrings + sudll->DllNameOffset),
			&sp, sizeof(sp));
	sp.filemask[TSIZEOF(sp.filemask) - 1] = '\0';

	strcpy(buf, "* Plug-in load error *");
	if ( (sudll->hadd != NULL) && (sudll->GetPluginInfo != NULL) ){
		strcpy(buf, "* GetPluginInfo error *");
		sudll->GetPluginInfo(1, buf, TSIZEOFA(buf));
	}
	SetDlgItemTextA(hDlg, IDE_AOSINFO, buf);
	CheckDlgButton(hDlg, IDX_AOSUSE, sp.flags & (VFSSUSIE_BMP | VFSSUSIE_ARC));
	CheckDlgButton(hDlg, IDX_AOSDETECT, !(sp.flags & VFSSUSIE_NOAUTODETECT) );
#ifdef UNICODE
	CheckDlgButton(hDlg, IDX_NOUNICODE, sp.flags & VFSSUSIE_DISABLEUNICODE );
#endif
	SetDlgItemText(hDlg, IDE_AOSMASK, sp.filemask);
	susieconfig = (sudll->hadd == NULL) ? NULL :
		(CONFIGURATIONDLG)GetProcAddress( sudll->hadd, "ConfigurationDlg" );
	EnableDlgWindow(hDlg, IDB_AOSSETTING, (susieconfig != NULL));
	EnableDlgWindow(hDlg, IDB_ADDSETEXT, FALSE);
}

void AddonSaveSusie(HWND hDlg)
{
	const SUSIE_DLL *sudll;
	SUSIE_DLLSTRINGS sp;

	if ( (susiedll_list == NULL) || (SusieIndex < 0) || (SusieIndex >= susiedll_items) ){
		return;
	}
	sudll = susiedll_list + SusieIndex;
	sp.flags = IsDlgButtonChecked(hDlg, IDX_AOSUSE) ?
					(VFSSUSIE_BMP | VFSSUSIE_ARC) : 0;
	if ( !IsDlgButtonChecked(hDlg, IDX_AOSDETECT) ){
		setflag(sp.flags, VFSSUSIE_NOAUTODETECT);
	}
#ifdef UNICODE
	if ( IsDlgButtonChecked(hDlg, IDX_NOUNICODE) ){
		setflag(sp.flags, VFSSUSIE_DISABLEUNICODE);
	}
#endif
	sp.filemask[0] = '\0';
	GetControlText(hDlg, IDE_AOSMASK, sp.filemask, VFPS);
	sp.filemask[TSIZEOF(sp.filemask) - 1] = '\0';
	SetCustTable(T("P_susie"), (TCHAR *)(sustrings + sudll->DllNameOffset),
			&sp, sizeof(DWORD) + TSTRSIZE(sp.filemask));
	Changed(hDlg);
	EnableDlgWindow(hDlg, IDB_ADDSETEXT, FALSE);
}

#pragma argsused
INT_PTR CALLBACK AddonPage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnUsedParam(lParam);

	switch (msg){
		case WM_DESTROY:
			VFSOff();
			return DlgSheetProc(hDlg, msg, wParam, lParam, IDD_ADDON);

		case WM_INITDIALOG:
			InitPropSheetsUxtheme(hDlg);
			VFSOn(VFS_DIRECTORY | VFS_BMP | VFS_ALL);
			SendDlgItemMessage(hDlg, IDE_AOSMASK, EM_LIMITTEXT, MAX_PATH - 1, 0);
			EnableTextChangeNotifyItem(hDlg, IDE_AOSMASK);

			PostMessage(hDlg, WM_COMMAND, K_FIRSTCMD, 0);
			return FALSE;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ){
				case IDL_AOSUSIE:
					if ( GetListCursorIndex(wParam, lParam, &SusieIndex) > 0 ){
						AddonSelectSusie(hDlg);
					}
					break;
				case IDB_AOSSETTING:
					AddOnSusieSetting(hDlg);
					break;
				case IDB_ADDDEFEXT:
					AddOnGetSusieExt(hDlg);
					break;
				case IDE_AOSMASK:
					if ( HIWORD(wParam) == EN_CHANGE ){
						EnableDlgWindow(hDlg, IDB_ADDSETEXT, TRUE);
					}
					break;
				case IDB_ADDSETEXT:
				case IDX_AOSUSE:
				case IDX_AOSDETECT:
#ifdef UNICODE
				case IDX_NOUNICODE:
#endif
					AddonSaveSusie(hDlg);
					break;
				case K_FIRSTCMD:
					AddonListSusie(hDlg);
					break;
			}
			break;

		case WM_NOTIFY:
			if ( ((NMHDR *)lParam)->code == (UINT)PSN_SETACTIVE ){
				InitWndIcon(hDlg, IDE_AOSINFO);
			}
		// default Ç÷
		default:
			return DlgSheetProc(hDlg, msg, wParam, lParam, IDD_ADDON);
	}
	return TRUE;
}
