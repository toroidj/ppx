/*-----------------------------------------------------------------------------
	Paper Plane xUI	 setup wizard
-----------------------------------------------------------------------------*/
#include "PPSETUP.H"
#pragma hdrstop

#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x001c // %LOCALAPPDATA%
#endif
//------------------------------------------------------------------ プロセス
HMODULE hInst;
OSVERSIONINFO OSver;	// OS 情報
const TCHAR NilStr[] = T("");
TCHAR MyPath[VFPS];	// 自分自身のディレクトリ
int SetupResult;		// セットアップが成功したかを記憶する

int DeleteUpsrc = 0;	// 1: 終了時に XX_upsrc を削除, 2: PPc で削除
HWND hResultWnd;		// 結果を書きこむボックス

#ifndef _WIN64	// 32bit
const TCHAR *UNARCS[] = {
	T("7-zip32.DLL"), T("UNLHA32.DLL"), T("UNRAR32.DLL"), T("CAB32.DLL"), T("TAR32.DLL"),NULL};
#else
const TCHAR *UNARCS[] = {
	T("7-zip64.DLL"), T("UNLHA32.DLL"), T("UNRAR64J.DLL"), T("CAB32.DLL"), T("TAR32_64.DLL"),NULL};
#endif
//-------------------------------------
PAGEINFO PageInfoJpn[PROPPAGE] = {
	{IDD_GUIDE, GuideDialog},
	{IDD_SETTYPE, SetTypeDialog},
	{IDD_UN_EXEC, UnExecDialog},
	{IDD_DEST, DestDialog},
	{IDD_REG,RegDialog},
	{IDD_PPCW, PPcwDialog},
	{IDD_KEY, KeyDialog},
	{IDD_APP, AppDialog},
	{IDD_LINK, LinkDialog},
	{IDD_READY,ReadyDialog},
	{IDD_COPY, CopyDialog},
	{IDD_UP, UpDialog}
};

PAGEINFO PageInfoEng[PROPPAGE] = {
	{IDD_GUIDEE, GuideDialog},
	{IDD_SETTYPEE, SetTypeDialog},
	{IDD_UN_EXECE, UnExecDialog},
	{IDD_DESTE, DestDialog},
	{IDD_REGE,RegDialog},
	{IDD_PPCWE, PPcwDialog},
	{IDD_KEYE, KeyDialog},
	{IDD_APPE, AppDialog},
	{IDD_LINKE, LinkDialog},
	{IDD_READYE,ReadyDialog},
	{IDD_COPYE, CopyDialog},
	{IDD_UPE, UpDialog}
};

const TCHAR *MessageJpn[] = {
	T("\r\n"), // RESULT_NORMAL
	T(":作成失敗\r\n"), // RESULT_FAULT
	T(":完了\r\n"), // RESULT_SUCCESS
	T("コピー終了"), // RESULT_ALLDONE
	T("アンインストール情報"), // RESULTINFO_UNINSTALLDATA
	 T("\r\nインストール"), // RESULTINFO_INSTALL
	T("UNICODE版です。Windows 9xでは動作しません。"), // MSG_UNICODE
	T("PPx があるディレクトリを指定してください"), // MSG_SELECTPPX
	T("最新版のPPxを展開したディレクトリを指定してください"), // MSG_SELECTUPDATE
	T("既にインストール済みですが、上書きしますか？"), // MSG_OVERINSTALL
	T("PPx をインストールするディレクトリを指定してください"), // MSG_SELECTDIR
	T("通常のインストール先(&P)(%s)"), // MSG_DEFAULTDIR
	T("コピーなしでインストール(&T)(%s)"), // MSG_NOCOPY
	T("テキストエディタを指定してください。"), // MSG_SELECTEDITOR
	T("ビューアを指定してください。"), // MSG_SELECTVIEWER
	T("Susie Plug-in があるディレクトリを指定してください"), // MSG_SELECTSUSIE
	T("セットアップを中止してもかまいませんか？"), // MSG_ABORT
	T("失敗した項目があります。\r\n管理者として再度実行してみてください。"), // MSG_FAILACCESS
	T("失敗した項目があります。ログを確認してください。"), // MSG_FAILCOPY
	T(COMMONDLL) T("の起動に失敗"), // MSG_DLLLOADERROR
	T("ダウンロードページが見つからないか、取得できません"), // MSG_SITEERROR
	T("最新版%sはダウンロード済みですが、ダウンロードしますか？"), // MSG_RELOADARCHIVE
	T("以前の設定のバックアップを %s\\PPXold.CFG に保存しました"), // MSG_BACKUP
	T("見つかりました"), // MSG_FOUND
	T("管理者モードではありません。\n全ユーザを対象とする場合は一度終了後、\n管理者モードでセットアップしてください。"), // MSG_RUNAS
	T("7-ZIP32.DLLかUNZIP32.DLLが必要です"), // MSG_UNZIP
	T("ファイルの署名が異常です\r\n"), // MSG_BADSIG
	T("コピーするファイルがありません\r\n"), // MSG_COPYNOFILE
	T("の起動に失敗"), // MSG_DLL_LOADERROR_PART
};

const TCHAR *MessageEng[] = {
	T("\r\n"), T(":fault\r\n"), T(":ok\r\n"), T("complete."),
	T("Uninstall data"), T("\r\nInstall"),
	T("WindowsNT or later only"),
	T("Select PPx directory"),
	T("Select directory in extracted files"),
	T("Overwrite ?"),
	T("Select Install directory"),
	T("Default path(&P)(%s)"),
	T("Use this directory(&T)(%s)"),
	T("Select Text editor"),
	T("Select Viewer"),
	T("Select Susie plug-in directory"),
	T("Abort setup ?"),
	T("Copy failed. Retry setup on Administrators user."),
	T("Copy failed. Check log window."),
	T(COMMONDLL) T(" load error"),
	T("download site error"),
	T("retry download %s?"),
	T("Backuped setting to %s\\PPXold.CFG."),
	T("Found"),
	T("now limited accessmode."),
	T("Require 7-ZIP32.DLL or UNZIP32.DLL."),
	T("Bad file signature.\r\n"),
	T("No copy-file.\r\n"),
	T(" load error"),
};


PAGEINFO *UsePageInfo = PageInfoEng;
const TCHAR **MessageStr = MessageEng;
//-------------------------------------------------------- インストールファイル
#ifdef UNICODE
	#define MultiByteLink(name) NULL
	#define UnicodeLink(name) T(name)
#else
	#define MultiByteLink(name) T(name)
	#define UnicodeLink(name) NULL
#endif
INSTALLFILES InstallFiles[] = {
	{T("README.TXT"),	NULL},
	{T("PPX.TXT"),		NULL},
	{T("PPXDW.TXT"),	NULL},
	{T("PPX.CNT"),		NULL},
	{T("PPX.HLP"),		NULL},
	{T("SETUP.EXE"),	T("Setup")},
#ifndef _WIN64	// 32bit
	{T("PPB.EXE"),		MultiByteLink("PPb")},
	{T("PPC.EXE"),		MultiByteLink("PPc")},
	{T("PPV.EXE"),		MultiByteLink("PPv")},
	{T("PPCUST.EXE"),	MultiByteLink("Customize")},
	{T("PPTRAY.EXE"),	MultiByteLink("PPtray")},
	{T("PPFFIX.EXE"),	NULL},
#endif
	{T("PPBW.EXE"),		UnicodeLink("PPb")},
	{T("PPCW.EXE"),		UnicodeLink("PPc")},
	{T("PPVW.EXE"),		UnicodeLink("PPv")},
	{T("PPCUSTW.EXE"),	UnicodeLink("Customize")},
	{T("PPTRAYW.EXE"),	UnicodeLink("PPtray")},
	{T("PPFFIXW.EXE"),	NULL},

#ifndef _WIN64	// 32bit
	{T("PPLIB32.DLL"),	NULL},
	{T("PPLIB32W.DLL"),	NULL},
	{T("iftgdip.spi"),	NULL},
#else			// 64bit
	{T("PPLIB64W.DLL"),	NULL},
	#ifdef _M_ARM64
		{T("iftwic.spha"),	NULL},
	#else
		{T("iftwic.sph"),	NULL},
	#endif
	{T("UNBYPASS.DLL"),	NULL},
	{T("UNBYPASS.EXE"),	NULL},
#endif
	{T("PPXFCMD.TXT"),	NULL},
	{T("PPXFMASK.TXT"),	NULL},
	{T("PPXFPATH.TXT"),	NULL},
	{NULL, NULL}
};

const TCHAR msgboxtitle[] = T("PPx Setup Wizard");

// PPxの設定保存場所
const TCHAR Str_PPxRegPath[] = T(PPxSettingsRegPath); // レジストリ

// PPxのアンインストール情報保存場所
const TCHAR Str_InstallPPxPath[] = T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PPx");
const TCHAR Set_InstallPPxName[] = T("InstallLocation"); // インストール先

const TCHAR Str_ProgDirPath[] = T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
const TCHAR Str_ProgDirName[] = T("ProgramFilesDir");

//-------------------------------------------------------------------- 設定項目
int		XX_setupmode;					// セットアップ方法
TCHAR	XX_setupedPPx[VFPS];			// セットアップ済みの PPx path
//----------------------------------- インストール
int		XX_instdestM	= IDR_PROGRAMS;	// インストール先の種類
TCHAR	XX_instdestP[VFPS];				// Programs File のインストール先
TCHAR	XX_instdestS[VFPS];				// 指定のインストール先
TCHAR	*XX_setupPPx;					// インストール/アップデート先

TCHAR	XX_upsrc[VFPS];					// アップデート元
int		XX_usereg		= IDR_USEREG;	// レジストリを利用するか
int		XX_ppc_window	= IDR_PPC_1WINDOW;	// ppc の窓の形態
BOOL	XX_ppc_tree		= FALSE;			// ppc の窓にツリー表示
int		XX_ppc_pane		= IDR_PPC_1PANE;	// ppc の窓のペイン枚数
int		XX_ppc_tab		= IDR_PPC_TAB_NO;	// ppc の窓のタブ形態
int		XX_ppc_join		= IDR_PPC_JOIN_H;	// ppc の窓の連結形態
int		XX_keytype		= IDR_PPXKEY;	// キー割り当て方法
BOOL	XX_diakey		= FALSE;		// ダイアモンドカーソル
BOOL	XX_emenu		= TRUE;			// 英語メニュー
BOOL	XX_doscolor		= TRUE;			// DOS配色
TCHAR	XX_editor[VFPS];				// テキストエディタ
TCHAR	XX_viewer[VFPS];				// 汎用ビューア
TCHAR	XX_susie[VFPS];					// Susie plug-in
BOOL	XX_usesusie		= FALSE;		// Susie plug-in の設定を使用する
BOOL	XX_resultquiet	= FALSE;		// エラーがなければ結果画面を省略
int		XX_link_menu	= 1;			// スタートメニューに登録
int		XX_link_cmenu	= 0;			// 共用スタートメニューに登録
int		XX_link_boot	= 0;			// スタートアップに登録
int		XX_link_desk	= 0;			// デスクトップに登録
int		XX_link_app		= 1;			// 追加と削除に登録

DefineWinAPI(BOOL, SetDllDirectory, (LPCTSTR lpPathName));
const TCHAR StrKernel32[] = T("KERNEL32.DLL");
const TCHAR StrShell32[] = T("SHELL32.DLL");
/*=============================================================================
	WinMain
=============================================================================*/
#pragma warning(suppress:28251) // SDK によって hPrevInstance の属性が異なる
#pragma argsused
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	PROPSHEETHEADER head;
	PROPSHEETPAGE page[PROPPAGE];
	PAGEINFO *pinfo;
	int i, startpage;
	TCHAR *p, buf[MAX_PATH];
	LCID UseLcid; // 表示言語
	UnUsedParam(hPrevInstance);UnUsedParam(nShowCmd);

	OSver.dwOSVersionInfoSize = sizeof(OSver);
	GetVersionEx(&OSver);

	GETDLLPROCT(GetModuleHandle(StrKernel32),SetDllDirectory);
	if ( DSetDllDirectory != NULL ){
		DSetDllDirectory(NilStr); // DLLの検索パスからカレントを除去する
	}
	GETDLLPROCT(GetModuleHandle(StrShell32), SHGetFolderPath);

	UseLcid = LOWORD(GetUserDefaultLCID());
	if ( UseLcid == LCID_JAPANESE ){
		UsePageInfo = PageInfoJpn;
		MessageStr = MessageJpn;
		XX_emenu = FALSE;
	}
#if !defined(_WIN64) && defined(UNICODE)
	if ( OSver.dwPlatformId != VER_PLATFORM_WIN32_NT ){
		SMessage(MessageStr[MSG_UNICODE]);
		return EXIT_FAILURE;
	}
#endif
												// 初期設定
	hInst = hInstance;
	GetModuleFileName(hInstance, MyPath, TSIZEOF(MyPath));
	*(tstrrchr(MyPath, '\\')) = '\0';
												// コマンドライン解析
#ifdef UNICODE
	p = tstrchr(GetCommandLine(), '/');
#else
	p = tstrchr(lpCmdLine, '/');
#endif
	if ( p != NULL ){
		startpage = UsePageInfo[PAGE_COPY].rID;
		XX_setupmode = IDR_UPDATE;

		if ( AutoSetup(p + 1) == AUTOSETUP_FIN ) return EXIT_SUCCESS;
	}else{
			// カスタマイズファイルの検索＆インストール先の確認 ---------------
		SearchPPxPath();
			// 初期ページの決定 -----------------------------------------------
		if ( XX_setupedPPx[0] == '\0' ){		// インストール向け
			startpage = UsePageInfo[PAGE_GUIDE].rID;
			XX_setupmode = IDR_AUTOINST;
		}else{						// 再設定／アンインストール
			startpage = UsePageInfo[PAGE_SETTYPE].rID;
			if ( XX_setupedPPx[0] != '?' ){
				XX_setupmode = tstricmp(MyPath, XX_setupedPPx) ?
						IDR_UPDATE : IDR_CHECKUPDATE;
			}else{
				XX_setupmode = IDR_AUTOINST;
			}
		}
			// 初期インストール先を作成 ---------------------------------------
		GetRegStrLocal(HKEY_LOCAL_MACHINE, Str_ProgDirPath,
				Str_ProgDirName, XX_instdestP);
		tstrcat(XX_instdestP, T("\\") T(PPxProgramDataPath));

		if ( IsTrue(GetWinAppDir(CSIDL_LOCAL_APPDATA, XX_instdestS)) ){
			tstrcat(XX_instdestS, T("\\") T(PPxSettingsAppdataPath));
		}else{
			tstrcpy(XX_instdestS, XX_instdestP);
		}
			// NT 系なら共用にする --------------------------------------------
		if ( OSver.dwPlatformId == VER_PLATFORM_WIN32_NT ){
			XX_link_menu	= 0;
			XX_link_cmenu	= 1;
		}else{
			XX_link_menu	= 1;
			XX_link_cmenu	= 0;
		}
			// Text Editor ----------------------------------------------------
		XX_editor[0] = '\0';
		if ( GetRegStrLocal(HKEY_CLASSES_ROOT, T(".txt"),NilStr, buf) ){
			tstrcat(buf, T("\\shell\\open\\command"));
										// アプリケーションのシェル -----------
			GetRegStrLocal(HKEY_CLASSES_ROOT, buf,NilStr, XX_editor);
			if ( XX_editor[0] == '\"' ){	// 括りあり
				tstrcpy(buf, XX_editor + 1);
				*tstrchr(buf, '\"') = '\0';
				tstrcpy(XX_editor, buf);
			}else{						// 括り無し
				p = tstrchr(XX_editor, ' ');
				if ( p != NULL ) *p = '\0';
			}
		}
		if ( XX_editor[0] == '\0' ) tstrcpy(XX_editor, T("notepad.exe"));
			// Viewer ---------------------------------------------------------
		if ( !GetRegStrLocal(HKEY_CLASSES_ROOT,
				T("QuickView\\shell\\open\\command"),NilStr, XX_viewer) ){
			tstrcpy(XX_viewer, T("notepad.exe"));
		};
			// Susie ----------------------------------------------------------
		GetRegStrLocal(HKEY_CURRENT_USER,
				T("Software\\Takechin\\Susie\\Plug-in"), T("Path"), XX_susie);

	}
			// ウィザードを開始する -------------------------------------------
	head.dwSize = sizeof(PROPSHEETHEADER);
	head.dwFlags = PROPSTYLE | PSH_PROPSHEETPAGE | PSH_USEICONID;
	head.hwndParent = NULL;
	head.hInstance = hInstance;
	UNIONNAME(head, pszIcon) = MAKEINTRESOURCE(IC_SETUP);
	head.pszCaption = PROPTITLE;
	head.nPages = PROPPAGE;
	UNIONNAME2(head,nStartPage) = startpage - UsePageInfo[PAGE_GUIDE].rID;
	UNIONNAME3(head, ppsp) = page;

	pinfo = UsePageInfo;
	for ( i = 0 ; i < PROPPAGE ; i++, pinfo++ ){
		page[i].dwSize = sizeof(PROPSHEETPAGE);
		page[i].dwFlags = 0;
		page[i].hInstance = hInstance;
		UNIONNAME(page[i], pszTemplate) = MAKEINTRESOURCE(pinfo->rID);
		page[i].pfnDlgProc = pinfo->proc;
		page[i].lParam = (LPARAM)i;
	}
	PropertySheet(&head);

	if ( DeleteUpsrc == DU_DELETE_END ) DeleteDir(XX_upsrc);
	return EXIT_SUCCESS;
}
