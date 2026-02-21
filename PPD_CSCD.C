/*-----------------------------------------------------------------------------
	Paper Plane xUI	customizer							PPc表示書式

	基本構成
		{word 桁数} {word 次の行へのオフセット} {bytes ID[option]} [ID[option]]... {ID=DE_END}

	w / W 指定あり... DE_WIDEW / DE_WIDEV が行頭に配置される
		{word 桁数} {word 次の行へのオフセット} {ID=DE_WIDEW,min,max,off} {ID[option]}...[ID=DE_NEWLINE,offset] {ID=DE_WIDEW,min,max,off} {ID[option]}... {ID=DE_END}

-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include "PPX.H"
#include "PPD_DEF.H"
#include "PPD_CUST.H"
#include "PPC_DISP.H"
#pragma hdrstop

COLORREF CS_color(TCHAR **linesrc, DWORD flags)			// キーワードを抽出
{
	return GetColor((const TCHAR **)linesrc, !(flags & fRC)); // A_color は自己参照させない
}

TCHAR *CD_color(TCHAR *dest, COLORREF color, DWORD flags)
{
	int i;
	// 追加識別子を判別
	if ( !(flags & fRC) ){	// A_color は自己参照させない
		i = 0;
		for ( ; ; ){
			TCHAR name[CUST_NAME_LENGTH];
			COLORREF rc;
			int r;

			r = EnumCustTable(i, T("A_color"), name, &rc, sizeof(rc));
			if ( 0 > r ) break;
			if ( color == rc ) return tstpcpy(dest, name);
			i++;
		}
	}

	for ( i = 0; i < ColorNameListTotal; i++ ){
		if ( color == ColorNameList[i].num ){
			return tstpcpy(dest, ColorNameList[i].str);
		}
	}
	return thprintf(dest, 16, T("H%06X"), color);
}

void WidthCheck(PPCUSTSTRUCT *PCS, int nowline, int maxws, int ws)
{
	TCHAR buf[MAX_PATH];

	thprintf(buf, TSIZEOF(buf), MessageText(MES_WUML), nowline, ws, maxws);
	WarningMes(PCS, buf);
}

// エントリ表示書式 -----------------------------------------------------------
BOOL CS_ppcdisp(PPCUSTSTRUCT *PCS, TCHAR **linePtr, BYTE **binPtr)
{
	TCHAR *line;
	BYTE *destp;			// 格納内容の末尾
	BYTE *colsstore;		// 幅書き込み先
	BYTE *lineleft;
	BYTE *lastFM = NULL;	// 最後の FM を示す

	int nowline = 1;		// 現在の行数
	int nowcols = 0;		// 現在の桁数
	int maxcols = 0;		// 今までの最大桁数
	int vw_cols = 0;		// 可変長桁数 0:未確認 -1:発見、桁数未保存
	int skipcols = 0;		// c 指定によるコメント表示時の総桁数
	int leftmargin = 0; 	// 桁下げ量(アイコン表示時)
	int useMemoSkip = -1;	// c 指定があった場所の桁数
	BOOL checkcols = TRUE;

	destp = *binPtr;
	line = *linePtr;

	colsstore = destp;
	*(DWORD *)destp = 0; // 次の２行相当
//	*(WORD *)(destp + DE_HEAD_WIDTH_OFF) = 0;
//	*(WORD *)(destp + DE_HEAD_NEXTLINE_OFF) = 0;
	destp += DE_HEAD_SIZE;
	lineleft = destp;
	while ( *line != '\0' ){
		SkipSPC(line);
		switch ( *line++ ){
			case ',':
				break;
			case 'S':
				*destp++ = DE_SPC;
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, 1, 255);
				break;
			case 's':
				*destp++ = DE_BLANK;
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, 1, 255);
				break;
			case 'M':
				*destp++ = DE_MARK;
				nowcols++;
				break;
			case 'b':
				*destp++ = DE_CHECK;
				nowcols += 2;
				break;
			case 'B':
				*destp++ = DE_CHECKBOX;
				nowcols += 2;
				break;
			case 'N': {
				BYTE iconsize;

				iconsize = GetNumberWith((const TCHAR **)&line, 0, 255);
				if ( iconsize != 0 ){
					*destp++ = DE_ICON2;
					*destp++ = iconsize;
					leftmargin = GetIcon2Len(iconsize);
				} else{
					*destp++ = DE_ICON;
					leftmargin = 2;
				}
				nowcols += leftmargin;
				break;
			}
			case 'n': {
				BYTE len;

				*destp++ = DE_IMAGE;
				leftmargin = GetNumberWith((const TCHAR **)&line, 20, 255);
				nowcols += *destp++ = (BYTE)leftmargin;
				len = 8;
				if ( *line == ',' ){
					line++;
					len = (BYTE)GetDigitNumber((const TCHAR **)&line);
				}
				*destp++ = len;
				break;
			}
			case 'F':
			case 'f': {
				BYTE len;

				if ( *line == 'E' ){
					*destp++ = (BYTE)((*(line - 1) == 'F') ? DE_LFN_EXT : DE_SFN_EXT);
					line++;
				}else if ( *line == 'M' ){
					lastFM = destp;
					*destp++ = (BYTE)DE_LFN_MUL;
					line++;
				} else{
					*destp++ = (BYTE)((*(line - 1) == 'F') ? DE_LFN : DE_SFN);
				}
				len = GetNumberWith((const TCHAR **)&line, DE_FN_ALL_WIDTH, 255);
				nowcols += *destp++ = len;
				len = DE_FN_WITH_EXT;
				if ( *line == ',' ){
					line++;
					nowcols += len = (BYTE)GetDigitNumber((const TCHAR **)&line);
				}
				*destp++ = len;
				break;
			}
			case 'w':
			case 'W': {
				TCHAR *fmt2nd;
				BYTE mincols;

				if ( (destp > lineleft) &&
					((*lineleft == DE_WIDEV) || (*lineleft == DE_WIDEW)) ){
					ErrorMes(PCS, MES_ETMW);
					return FALSE;
				}

				memmove(lineleft + DE_WIDE_SIZE, lineleft, destp - lineleft);
				destp += DE_WIDE_SIZE;
				if ( *(line - 1) == 'w' ){ // 'w'
					lineleft[0] = DE_WIDEW;
					vw_cols = -1;
				}else{ // 'W'
					lineleft[0] = DE_WIDEV;
				}
				lineleft[2] = GetNumberWith((const TCHAR **)&line, 255, 255);
				lineleft[3] = (BYTE)(destp - lineleft + 1);
				SkipSPC(line);
				fmt2nd = line + 1;
				if ( (*line == 'F') || (*line == 'f') ){
					mincols = 16;
					if ( (*fmt2nd == 'M') || (*fmt2nd == 'E') ) fmt2nd++;
				}else if ( (lineleft[0] == DE_WIDEW) && (tstrchr(DE_ENABLE_FIXLENGTH, *line) != NULL) ){
					mincols = 2;
				}else{
					ErrorMes(PCS, MES_EWFT);
					return FALSE;
				}
				lineleft[1] = GetNumberWith((const TCHAR **)&fmt2nd, mincols, 255);
				break;
			}
			case 'Z':
				*destp++ = DE_SIZE3;
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, 7, 80);
				break;
			case 'z':
				if ( *line == 'K' ){
					line++;
					*destp++ = DE_SIZE4;
				} else{
					*destp++ = DE_SIZE2;
				}
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, 13, 80);
				break;
			case 'T':
				*destp++ = DE_TIME1;
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, 17, 17);
				break;
			case 'A':
				*destp++ = DE_ATTR1;
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, 10, 10);
				break;

			case 'G':
				if ( *line == 'R' ){
					checkcols = FALSE;
					*destp++ = DE_chr_Rgap;
					line++;
				}else{
					checkcols = FALSE;
					if ( *line == 'L' ) line++;
					*destp++ = DE_chr_Lgap;
				}
				*destp++ = GetNumberWith((const TCHAR **)&line, 1, 255);
				break;

			case 'g':
				if ( *line == 'R' ){
					*destp++ = DE_pix_Rgap;
					line++;
				}else{
					if ( *line == 'L' ) line++;
					*destp++ = DE_pix_Lgap;
				}
				*(WORD *)destp = GetNumberWith((const TCHAR **)&line, 1, 30000);
				destp += sizeof(WORD);
				break;

			case 'C': {
				int param;

				param = GetNumberWith((const TCHAR **)&line, 255, 255);
				if ( skipcols ){	// スキップあり
					*destp++ = DE_MEMOS;
					*destp++ = (BYTE)min(skipcols, 255);
					skipcols = 0;
				} else{			// スキップ無し
					*destp++ = DE_MEMO;
					*destp++ = (BYTE)param;
					nowcols += param;
				}
				break;
			}

			case 'c':
				useMemoSkip = nowcols;
				*destp++ = DE_MSKIP;
				continue;

			case 'u':
				*destp = DE_MEMOEX;
				*(destp + 2) = GetNumberWith((const TCHAR **)&line, 1, 255); // ID
				SkipSPC(line);
				if ( *line == ',' ) line++;
				nowcols += *(destp + 1) = GetNumberWith((const TCHAR **)&line, 8, 255); // 幅
				destp += DE_MEMOEX_SIZE;
				break;

			case 'j':
				checkcols = FALSE;
				*destp++ = DE_gapless;
				*destp++ = GetNumberWith((const TCHAR **)&line, 2, 2);
				break;

			case 'L':
				*destp++ = DE_sepline;
				nowcols++;
				break;

			case 'H':
				*destp++ = DE_hline;
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, 0, 255);
				break;

			case 'O':
				if ( *line == 'd' ){
					line++;
					*destp++ = DE_fc_def;
					*destp++ = GetNumberWith((const TCHAR **)&line, 0, 0);
					break;
				} else if ( *line == 'g' ){
					line++;
					*destp++ = DE_bc_def;
					*destp++ = GetNumberWith((const TCHAR **)&line, 0, 0);
					break;
				} else if ( *line == 'b' ){
					line++;
					*destp++ = DE_bcolor;
				} else{
					*destp++ = DE_fcolor;
				}
				if ( *line == '\"' ) line++;
				*(COLORREF *)destp = CS_color(&line, 0);
				destp += sizeof(COLORREF);
				if ( *line == '\"' ) line++;
				break;

			case 'Q':
				*destp++ = DE_lineNZS;
				*destp++ = GetNumberWith((const TCHAR **)&line, 0, 3);
				break;

			case '/':
				*destp++ = DE_NEWLINE;
				*destp++ = 0;
				*destp++ = 0;
				if ( vw_cols < 0 ) vw_cols = nowcols;
				maxcols = max(nowcols, maxcols);
				nowline++;
				nowcols = leftmargin;
				skipcols = 0;

				*(WORD *)(lineleft - DE_HEAD_NEXTLINE_SIZE) = (WORD)(destp - lineleft);
				lineleft = destp;
				break;

			case 'Y':
				*destp++ = DE_FStype;
				nowcols += 3;
				break;
			case 'v':
			case 'i':
			case 'I':
				#ifdef UNICODE
				if ( !((destp - *binPtr) & 1) ){	// WORD境界合わせ
					*destp++ = DE_SKIP;
				}
				#endif
				{
					BYTE vt;

					switch ( *(line - 1) ){
						case 'i':
							vt = DE_string;
							break;
						case 'v':
							if ( *line == 'i' ) line++;
							vt = DE_ivalue;
							break;
						default: // 'I'
							vt = DE_itemname;
							break;
					}
					*destp++ = vt;
				}
				if ( *line != '\"' ){
					ErrorMes(PCS, MES_ESEP);
					return FALSE;
				}
				line++;
				while ( ((UTCHAR)(*line) >= ' ') && (*line != '\"') ){
					#ifdef UNICODE
					TCHAR chr;
					chr = *line++;
					*destp++ = (BYTE)chr;
					*destp++ = (BYTE)(chr >> 8);
					nowcols += 2;
					#else
					*destp++ = *line++;
					nowcols++;
					#endif
				}
				if ( *line == '\"' ) line++;
				#ifdef UNICODE
				*(TCHAR *)destp = '\0';
				destp += sizeof(TCHAR);
				#else
				*destp++ = '\0';
				#endif
				break;
			case 'P':
				*destp++ = DE_ALLPAGE;
				nowcols += 3;
				break;
			case 'p':
				*destp++ = DE_NOWPAGE;
				nowcols += 3;
				break;

			case 'E':
				if ( Isdigit(*line) ){
					int n = 0;

					const BYTE d[] = {DE_ENTRYA0, DE_ENTRYA1};

					if ( Isdigit(*line) ){
						n = *line - '0';
						line++;
						if ( n > 1 ){
							ErrorMes(PCS, MES_EENM);
							return FALSE;
						}
					}
					*destp++ = d[n];
					nowcols += 3;
				} else{
					*destp++ = DE_ENTRYSET;
					nowcols += 7;
				}
				break;

			case 'e': {
				int n = 0;

				const BYTE d[] = {DE_ENTRYV0, DE_ENTRYV1,
					DE_ENTRYV2, DE_ENTRYV3};

				if ( Isdigit(*line) ){
					n = *line - '0';
					line++;
					if ( n > 3 ){
						ErrorMes(PCS, MES_EENM);
						return FALSE;
					}
				}
				*destp++ = d[n];
				nowcols += 3;
				break;
			}
			case 'm':{
				BYTE deflen;

				switch ( *line ){
					case 'n':
						*destp++ = DE_MNUMS;
						deflen = 0;
						nowcols += 3;
						break;
					case 'S':
						*destp++ = DE_MSIZE1;
						deflen = 7;
						break;
					case 's':
						*destp++ = DE_MSIZE2;
						deflen = 13;
						break;
					case 'K':
						*destp++ = DE_MSIZE3;
						deflen = 13;
						break;
					default:
						ErrorMes(PCS, MES_EUFM);
						return FALSE;
				}
				line++;
				if ( deflen ){
					deflen = GetNumberWith((const TCHAR **)&line, deflen, 40);
					if ( deflen < 3 ) deflen = 3;
					nowcols += *destp++ = deflen;
				}
				break;
			}
			case 'D':{
				BYTE deflen;

				switch ( *line ){
					case 'F':
						*destp++ = DE_DFREE1;
						deflen = 7;
						break;
					case 'f':
						*destp++ = DE_DFREE2;
						deflen = 13;
						break;
					case 'U':
						*destp++ = DE_DUSE1;
						deflen = 7;
						break;
					case 'u':
						*destp++ = DE_DUSE2;
						deflen = 13;
						break;
					case 'T':
						*destp++ = DE_DTOTAL1;
						deflen = 7;
						break;
					case 't':
						*destp++ = DE_DTOTAL2;
						deflen = 13;
						break;
					default:
						ErrorMes(PCS, MES_EUFM);
						return FALSE;
				}
				line++;
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, deflen, 40);
				break;
			}
			case 't':
				*destp++ = DE_TIME2;
				if ( *line == 'C' ){
					line++;
					*destp++ = 0;
				} else if ( *line == 'A' ){
					line++;
					*destp++ = 1;
				} else{
					if ( *line == 'W' ) line++;
					*destp++ = 2;
				}
				if ( *line != '\"' ){
					ErrorMes(PCS, MES_ESEP);
					return FALSE;
				}
				line++;
				while ( ((UTCHAR)(*line) >= ' ') && (*line != '\"') ){
					switch ( *line ){
						case 'y':
						case 'n':
						case 'N':
						case 'd':
						case 'D':
						case 'W':
						case 'h':
						case 'H':
						case 'm':
						case 'M':
						case 's':
						case 'S':
						case 'u':
						case 'U':
						case 't':
							nowcols += 2;
							break;
						case 'I':
						case 'g':
						case 'w':
						case 'a':
							nowcols += 3;
							break;
						case 'Y':
						case 'T':
							nowcols += 4;
							break;
						case 'G':
							nowcols += 6;
							break;
						default:
							nowcols++;
					}
					*destp++ = (BYTE)*line++;
				}
				if ( *line == '\"' ) line++;
				*destp++ = '\0';
				break;

			case 'V':
				*destp++ = DE_vlabel;
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, 14, 255);
				break;

			case 'R':
				if ( *line == 'M' ){
					line++;
					*destp++ = DE_pathmask;
				} else{
					*destp++ = DE_path;
				}
				nowcols += *destp++ = GetNumberWith((const TCHAR **)&line, 60, 255);
				break;

			case 'U': {
				TCHAR *dst;

				*destp++ = DE_COLUMN;
				dst = ((DISPFMT_COLUMN *)destp)->name;
				if ( *line == '\"' ){
					line++;
					while ( *line ){
						if ( *line == '\"' ){
							line++;
							break;
						}
						*dst++ = *line++;
					}
				}
				*dst++ = '\0';
				if ( *line == ',' ) line++;
				nowcols += ((DISPFMT_COLUMN *)destp)->width = GetNumberWith((const TCHAR **)&line, 10, 255);
				((DISPFMT_COLUMN *)destp)->itemindex = DFC_UNDEF;
				((DISPFMT_COLUMN *)destp)->fmtsize = (BYTE)(((BYTE *)dst - destp) + 1);
				destp = (BYTE *)dst;
				break;
			}

			case 'X': {
				BYTE *dst;
				BYTE lines;
				TCHAR c;
				DWORD hash = 0;

				*destp++ = DE_MODULE;
				// 名前
				dst = destp + 2 + 4;
				if ( *line == '\"' ){
					line++;
					while ( *line ){
						c = upper(*line++);
						if ( c == '\"' ){
							line++;
							break;
						}
						*dst++ = (BYTE)c;
						hash = (DWORD)(hash << 6) | (DWORD)(hash >> (32 - 6)) | (DWORD)c;
					}
				}
				*(destp + 4 + 8) = '\0';
				*dst = '\0';
				// ハッシュ
				*(DWORD *)(destp + 2) = hash | B31;
				// 桁数
				if ( *line == ',' ) line++;
				nowcols += *destp = GetNumberWith((const TCHAR **)&line, 4, 255);
				// 行数
				lines = 1;
				if ( *line == ',' ){
					line++;
					lines = (BYTE)GetDigitNumber((const TCHAR **)&line);
				}
				*(destp + 1) = lines;
				destp += 4 + 8 + 2 + 1;
				break;
			}

			default:
				ErrorMes(PCS, MES_EUFM);
				return FALSE;
		}
		if ( (size_t)(destp - colsstore) > (0x10000 - 0x100) ){
			ErrorMes(PCS, MES_EFLW);
			return FALSE;
		}
		if ( useMemoSkip >= 0 ){	// 直前が c ならスキップ幅算出
			skipcols += nowcols - useMemoSkip;
			useMemoSkip = -1;
		}
	}
	*destp++ = 0;

	if ( maxcols && checkcols && (nowcols < maxcols) ){
		WidthCheck(PCS, nowline, maxcols, nowcols);
	}
	*(WORD *)colsstore = (vw_cols > 0) ? (WORD)vw_cols : (WORD)max(nowcols, maxcols);
	if ( lastFM != NULL ) *lastFM = (BYTE)DE_LFN_LMUL;
	*binPtr = destp;
	*linePtr = line;
	return TRUE;
}


void CD_ppcdisp(PPCUSTSTRUCT *PCS, BYTE **binPtr, BYTE *binend)
{
	UINT widev = 0;
	BYTE widecode = DE_WIDEW, *widec = NULL, *bin;

	bin = *binPtr;
	bin += sizeof(DWORD);
	for ( ;;){
		BYTE binc;

		if ( bin >= binend ) break;
		binc = *bin++;
		if ( binc == 0 ) break;

		if ( widec == bin ){
			*PCS->Smes++ = (BYTE)((widecode == DE_WIDEV) ? 'W' : 'w');
			if ( widev < 255 ){
				PCS->Smes = Numer32ToString(PCS->Smes, widev);
			}
		}

		switch ( binc ){
			case DE_SPC:
				PCS->Smes = thprintf(PCS->Smes, 8, T("S%d"), *bin);
				bin++;
				break;
			case DE_BLANK:
				PCS->Smes = thprintf(PCS->Smes, 8, T("s%d"), *bin);
				bin++;
				break;
			case DE_MARK:
				*PCS->Smes++ = 'M';
				break;
			case DE_CHECK:
				*PCS->Smes++ = 'b';
				break;
			case DE_CHECKBOX:
				*PCS->Smes++ = 'B';
				break;
			case DE_ICON:
				*PCS->Smes++ = 'N';
				break;
			case DE_ICON2:
				PCS->Smes = thprintf(PCS->Smes, 8, T("N%d"), *bin);
				bin++;
				break;
			case DE_IMAGE:
				PCS->Smes = thprintf(PCS->Smes, 16, T("n%d,%d"), *bin, *(bin + 1));
				bin += 2;
				break;
			case DE_LFN:
			case DE_SFN:
			case DE_LFN_MUL:
			case DE_LFN_LMUL:
			case DE_LFN_EXT:
			case DE_SFN_EXT:
			{
				int i;

				*PCS->Smes++ = (BYTE)((*(bin - 1) == DE_SFN) ? 'f' : 'F');
				if ( *(bin - 1) >= DE_LFN_MUL ){
					if ( *(bin - 1) == DE_SFN_EXT ) *(PCS->Smes - 1) = 'f';
					*PCS->Smes++ = (*(bin - 1) >= DE_LFN_EXT) ? (BYTE)'E' : (BYTE)'M';
				}
				i = *bin++;						// ファイル名
				if ( i != DE_FN_ALL_WIDTH ){
					PCS->Smes = Int32ToString(PCS->Smes, i);
				}
				i = *bin++;						// 拡張子
				if ( i != DE_FN_WITH_EXT ){
					*PCS->Smes++ = ',';
					PCS->Smes = Int32ToString(PCS->Smes, i);
				}
				break;
			}
			case DE_SIZE1:
				*PCS->Smes++ = 'Z';
				break;
			case DE_SIZE2:
				PCS->Smes = thprintf(PCS->Smes, 8, T("z%d"), *bin);
				bin++;
				break;
			case DE_SIZE3:
				PCS->Smes = thprintf(PCS->Smes, 8, T("Z%d"), *bin);
				bin++;
				break;
			case DE_SIZE4:
				PCS->Smes = thprintf(PCS->Smes, 8, T("zK%d"), *bin);
				bin++;
				break;
			case DE_TIME1:
				PCS->Smes = thprintf(PCS->Smes, 8, T("T%d"), *bin);
				bin++;
				break;
			case DE_TIME2:
				*PCS->Smes++ = 't';
				switch ( *bin++ ){
					case 0:
						*PCS->Smes++ = 'C';
						break;
					case 1:
						*PCS->Smes++ = 'A';
						break;
					case 2:
						*PCS->Smes++ = 'W';
						break;
				}
				*PCS->Smes++ = '\"';
				while ( *bin ) *PCS->Smes++ = *bin++;
				bin++;
				*PCS->Smes++ = '\"';
				break;
			case DE_ATTR1:
				PCS->Smes = thprintf(PCS->Smes, 8, T("A%d"), *bin);
				bin++;
				break;
			case DE_MEMOS:
				*PCS->Smes++ = 'C';
				bin++;	// 桁数は出力しない
				break;
			case DE_MEMO: {
				UINT i;
				*PCS->Smes++ = 'C';
				i = *bin++;
				if ( i != 255 ){
					PCS->Smes = Numer32ToString(PCS->Smes, i);
				}
				break;
			}
			case DE_MSKIP:
				*PCS->Smes++ = 'c';
				continue;
			case DE_MEMOEX:
				PCS->Smes = thprintf(PCS->Smes, 16, T("u%d,%d"), *(bin + 1), *bin);
				bin += 2;
				break;
			case DE_sepline:
				*PCS->Smes++ = 'L';
				break;
			case DE_hline:
				PCS->Smes = thprintf(PCS->Smes, 8, T("H%d"), *bin);
				bin++;
				break;

			case DE_fcolor:
			case DE_bcolor:
				*PCS->Smes++ = 'O';
				if ( *(bin - 1) == DE_bcolor ) *PCS->Smes++ = 'b';
				*PCS->Smes++ = '\"';
				PCS->Smes = CD_color(PCS->Smes, *(COLORREF *)bin, 0);
				*PCS->Smes++ = '\"';
				bin += sizeof(COLORREF);
				break;

			case DE_fc_def:
				PCS->Smes = thprintf(PCS->Smes, 8, T("Od%d"), *bin);
				bin++;
				break;
			case DE_bc_def:
				PCS->Smes = thprintf(PCS->Smes, 8, T("Og%d"), *bin);
				bin++;
				break;

			case DE_lineNZS:
				PCS->Smes = thprintf(PCS->Smes, 8, T("Q%d"), *bin);
				bin++;
				break;

			case DE_gapless:
				*PCS->Smes++ = 'j';
				if ( *bin < 2 ) PCS->Smes += *bin + '0';
				bin++;
				break;

			case DE_NEWLINE:
				*PCS->Smes++ = '/';
				bin += 2;
				break;
			case DE_FStype:
				*PCS->Smes++ = 'Y';
				break;
			case DE_ivalue:
			case DE_itemname:
			case DE_string: {
				TCHAR *str, vc;

				switch ( *(bin - 1) ){
					case DE_string:
						vc = 'i';
						break;
					case DE_ivalue:
						*PCS->Smes++ = 'v';
						vc = 'i';
						break;
					default: // DE_itemname
						vc = 'I';
						break;
				}
				*PCS->Smes++ = vc;
				*PCS->Smes++ = '\"';
				str = (TCHAR *)bin;
				while ( *str ) *PCS->Smes++ = *str++;
				bin = (BYTE *)(TCHAR *)(str + 1);
				*PCS->Smes++ = '\"';
				break;
			}

			case DE_ALLPAGE:
				*PCS->Smes++ = 'P';
				break;
			case DE_NOWPAGE:
				*PCS->Smes++ = 'p';
				break;
			case DE_MNUMS:
				*PCS->Smes++ = 'm';
				*PCS->Smes++ = 'n';
				break;
			case DE_ENTRYSET:
				*PCS->Smes++ = 'E';
				break;
			case DE_ENTRYV0:
				*PCS->Smes++ = 'e';
				break;
			case DE_ENTRYV1:
				*PCS->Smes++ = 'e';
				*PCS->Smes++ = '1';
				break;
			case DE_ENTRYV2:
				*PCS->Smes++ = 'e';
				*PCS->Smes++ = '2';
				break;
			case DE_ENTRYV3:
				*PCS->Smes++ = 'e';
				*PCS->Smes++ = '3';
				break;
			case DE_ENTRYA0:
				*PCS->Smes++ = 'E';
				*PCS->Smes++ = '0';
				break;
			case DE_ENTRYA1:
				*PCS->Smes++ = 'E';
				*PCS->Smes++ = '1';
				break;
			case DE_MSIZE1:
				PCS->Smes = thprintf(PCS->Smes, 8, T("mS%d"), *bin);
				bin++;
				break;
			case DE_MSIZE2:
				PCS->Smes = thprintf(PCS->Smes, 8, T("ms%d"), *bin);
				bin++;
				break;
			case DE_MSIZE3:
				PCS->Smes = thprintf(PCS->Smes, 8, T("mK%d"), *bin);
				bin++;
				break;
			case DE_DFREE1:
				PCS->Smes = thprintf(PCS->Smes, 8, T("DF%d"), *bin);
				bin++;
				break;
			case DE_DFREE2:
				PCS->Smes = thprintf(PCS->Smes, 8, T("Df%d"), *bin);
				bin++;
				break;
			case DE_DUSE1:
				PCS->Smes = thprintf(PCS->Smes, 8, T("DU%d"), *bin);
				bin++;
				break;
			case DE_DUSE2:
				PCS->Smes = thprintf(PCS->Smes, 8, T("Du%d"), *bin);
				bin++;
				break;
			case DE_DTOTAL1:
				PCS->Smes = thprintf(PCS->Smes, 8, T("DT%d"), *bin);
				bin++;
				break;
			case DE_DTOTAL2:
				PCS->Smes = thprintf(PCS->Smes, 8, T("Dt%d"), *bin);
				bin++;
				break;
			case DE_WIDEW:
			case DE_WIDEV:
				widecode = *(bin - 1);
				widev = *(bin + 1);
				widec = bin + *(bin + 2) - 1;
				*widec = *bin;	// Fn を元に戻す
				bin += 3;
				continue;
			case DE_vlabel:
				PCS->Smes = thprintf(PCS->Smes, 8, T("V%d"), *bin);
				bin++;
				break;
			case DE_path:
				PCS->Smes = thprintf(PCS->Smes, 8, T("R%d"), *bin);
				bin++;
				break;
			case DE_pathmask:
				PCS->Smes = thprintf(PCS->Smes, 8, T("RM%d"), *bin);
				bin++;
				break;
			case DE_COLUMN: {
				int nextoffset;

				PCS->Smes = thprintf(PCS->Smes, CMDLINESIZE, T("U\"%s\",%d"),
					((DISPFMT_COLUMN *)bin)->name,
					((DISPFMT_COLUMN *)bin)->width);
				nextoffset = ((DISPFMT_COLUMN *)bin)->fmtsize - 1;
				if ( nextoffset >= 0 ) bin += nextoffset;
				break;
			}
			case DE_MODULE: {
				#ifdef UNICODE
				WCHAR bufw[8];

				AnsiToUnicode((char *)(bin + 2 + 4), bufw, 8);
				#define CNAME bufw
				#else
				#define CNAME (bin + 2 + 4)
				#endif
				PCS->Smes = thprintf(PCS->Smes, CMDLINESIZE, T("X\"%s\",%d,%d"),
					CNAME, *bin, *(bin + 1));
				bin += 16 - 1;
				break;
			}
			case DE_SKIP:
				continue;	// 空白を挿入させない

			case DE_pix_Lgap:
				PCS->Smes = thprintf(PCS->Smes, 16, T("gL%d"), *(WORD *)bin);
				bin += 2;
				break;

			case DE_chr_Lgap:
				PCS->Smes = thprintf(PCS->Smes, 8, T("GL%d"), *bin);
				bin++;
				break;

			case DE_pix_Rgap:
				PCS->Smes = thprintf(PCS->Smes, 16, T("gR%d"), *(WORD *)bin);
				bin += 2;
				break;

			case DE_chr_Rgap:
				PCS->Smes = thprintf(PCS->Smes, 8, T("GR%d"), *bin);
				bin++;
				break;

			default:
				goto term;
		}
		if ( *bin ) *PCS->Smes++ = ' ';
	}
term:
	*binPtr = bin;
	*PCS->Smes = '\0';
}
