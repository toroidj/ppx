/*-----------------------------------------------------------------------------
	Paper Plane cUI			Combo Window ToolBar
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <commctrl.h>
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPCOMBO.H"
#pragma hdrstop

#define TOOLBAR_CMDID 0x1000

#define xTBSTYLE_CUSTOMERASE 0x2000

void ComboCreateToolBar(HWND hWnd)
{
	RECT box;
	UINT ID = TOOLBAR_CMDID;
	HWND hBarWnd;

	if ( WinColors.ExtraDrawFlags & EDF_DIALOG_BACK ) LoadCCDrawBack();
	hBarWnd = CreateToolBar(&Combo_thGuiWork, hWnd, &ID, T("B_cdef"), PPcPath,
			(WinColors.ExtraDrawFlags & EDF_DIALOG_BACK) ?
				xTBSTYLE_CUSTOMERASE : 0);
	Combo.ToolBar.hWnd = hBarWnd;
	if ( hBarWnd != NULL ){
		GetWindowRect(hBarWnd, &box);
		Combo.ToolBar.Height = box.bottom - box.top;
	}
}

void ComboToolbarCommand(int id, int orcode)
{
	RECT box;
	POINT pos;
	TCHAR *cmd;

	SendMessage(Combo.ToolBar.hWnd, TB_GETRECT, (WPARAM)id, (LPARAM)&box);
	pos.x = box.left;
	pos.y = box.bottom;
	ClientToScreen(Combo.ToolBar.hWnd, &pos);

	cmd = GetToolBarCmd(Combo.ToolBar.hWnd, &Combo_thGuiWork, id);
	if ( cmd == NULL ) return;
	if ( orcode ){
		WORD key;

		key = *(WORD *)(cmd + 1) | (WORD)orcode;
		if ( key == (K_s | K_raw | K_bs) ){
			key = K_raw | K_c | K_bs;
		}
		SendMessage(hComboFocusWnd, WM_PPXCOMMAND,
				TMAKELPARAM(K_POPOPS, PPT_SAVED), TMAKELPARAM(pos.x, pos.y));
		SendMessage(hComboFocusWnd, WM_PPXCOMMAND, key, 0);
	}else{
		int i;

		i = GetComboShowIndex(hComboFocusWnd);
		if ( i < 0 ) return;
		if ( Combo.base[Combo.show[i].baseNo].cinfo == NULL ) return;
		SendMessage(hComboFocusWnd, WM_PPCEXEC, (WPARAM)cmd, TMAKELPARAM(pos.x, pos.y));
	}
}
