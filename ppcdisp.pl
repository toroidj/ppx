#!/usr/bin/perl
# (c)TORO
#
	$dest = 'PPC_DISP.H';

	open(OUT, "> $dest");
	while (<DATA>) {
		chop;
		if ( $_ eq '' ){
			print OUT "\n";
			next;
		}
		$top = substr($_, 0, 1);
		if ( $top eq ';' ){ next; }
		if ( ($top eq '#') || ($top eq '/') ){
			print OUT "$_\n";
			next;
		}
		@tmp = split(/\t+/, $_);
		print OUT "#define @tmp[1]\t@tmp[0]\n";
		($list[$cnt][0], $list[$cnt][1], $list[$cnt][2], $list[$cnt][3]) = @tmp;
		$cnt++;
	}
	$max = $cnt - 1;
	print OUT "// max ID:$max";

# --------- サイズ
	print OUT "\n\n//各サイズ\n";
	for ( 0 .. ($cnt - 1) ){
		print OUT "#define $list[$_][1]_SIZE $list[$_][2]\n";
	}
# --------- ID順にソート
	for ( 0 .. ($cnt - 1) ){
		push(@sortkey, $list[$_][0]);
	}
	@list = @list[sort {$sortkey[$a] <=> $sortkey[$b]} 0..($cnt - 1)];
# --------- スキップテーブル
	print OUT "\n//スキップテーブル\n#define DE_SKIPTABLE	";
	for ( 0 .. ($cnt - 1) ){
		print OUT "$list[$_][2]";
		if ( $_ != ($cnt - 1) ){ print OUT ","; }
	}
# --------- 属性テーブル
	print OUT "\n//属性テーブル\n#define DE_ATTRTABLE	";
	for ( 0 .. ($cnt - 1) ){
		print OUT $list[$_][3];
		if ( $_ != ($cnt - 1) ){ print OUT ","; }
	}
# ---------
	print OUT "\n";
	close(OUT);

__END__
// PPc 表示書式関連(ppcdisp.plによる自動生成)
// 各定義一覧
// PPC_DISP 定義用定数

; ※1 属性
#define DE_ATTR_WIDTH_L	B0	// 幅指定要素として桁数のみ
#define DE_ATTR_WIDTH	B1	// 幅有り

#define DE_ATTR_STATIC	B2	// 画面構成要素など
#define DE_ATTR_ENTRY	B3	// エントリ情報
#define DE_ATTR_MARK	B4	// マーク情報
#define DE_ATTR_PAGE	B5	// ページ情報
#define DE_ATTR_DIR		B6	// ディレクトリ情報
#define DE_ATTR_PATH	B7	// ディレクトリパス行
#define DE_ATTR_DISP	0xfc // 表示要素情報

#define DE_ATTR_WIDEV	B16	// 桁数に合わせて窓枠調整が必要
#define DE_ATTR_WIDEW	B17	// 窓枠に合わせて桁数調整が必要

#define DE_ENABLE_FIXLENGTH	T("CFHRSVfns") // 窓枠可変長に対応

#define GetIcon2Len(iconsize) ((iconsize + 7) / 8) // アイコンpixを文字列長に変換

#define DE_HEAD_SIZE 4 // DE_HEAD_WIDTH sizeof(WORD) + DE_HEAD_NEXTLINE sizeof(WORD)
#define DE_HEAD_WIDTH_OFF 0
#define DE_HEAD_NEXTLINE_OFF 2
#define DE_HEAD_NEXTLINE_SIZE 2

#define DE_WIDE_SIZE 4

#define DE_FN_ALL_WIDTH	0xff // ファイル名の長さ:ファイル名部の長さの制限が無い
#define DE_FN_WITH_EXT	0xff // 拡張子の長さ:拡張子はファイル名に続いて描画

; 使用済み:/ABbCcDEeFfGgHIiLMmNnOPpQRSsTtUuvWwXYZz
;ID	名前	  size ※1  コメント
// static 系
0	DE_END		0	0	末尾
38	DE_SKIP		1	0	境界合わせ用のダミーコード(UNICODE版用)
1	DE_SPC		2	7	Sn	空白				(BYTE:文字幅)
2	DE_BLANK	2	7	sn	空欄				(BYTE:文字幅)
14	DE_sepline	1	6	L	区切り線
50	DE_hline	2	7	Hn	水平線				(BYTE:文字幅)
47	DE_NEWLINE	3	4	/	改行				(WORD:次の行へのオフセット)
16	DE_itemname	-1	6	I"str"	アイテム名		(z string)
27	DE_string	-1	6	i"str"	文字列			(z string)
57	DE_ivalue	-1	6	vi"str" id別特殊環境変数	(z string)
48	DE_fcolor	5	4	O"color" 文字色			(COLORREF:色)
49	DE_bcolor	5	4	Ob"color" 背景色		(COLORREF:色)
56	DE_fc_def	2	4	Odn 文字色を定義済み色に	(BYTE:種類)
58	DE_bc_def	2	4	Og 背景色を定義済み色に	(BYTE:種類)
64	DE_pix_Lgap	3	4	gn	インデント(左端基準)	(WORD:Pix幅)
65	DE_chr_Lgap	2	4	Gn	インデント(左端基準)	(BYTE:文字幅)
66	DE_pix_Rgap	3	4	grn	インデント(右端基準)	(WORD:Pix幅)
67	DE_chr_Rgap	2	4	Grn	インデント(右端基準)	(BYTE:文字幅)
68	DE_lineNZS	2	0	Qn	配置	(BYTE:種類 B1:並び B0:スクロール)
69	DE_gapless	2	0	j	詰めて表示
// エントリ系
; マーク/アイコン表示
3	DE_MARK		1	26	M	*
36	DE_ICON		1	26	N	アイコン
46	DE_ICON2	2	26	Nn	アイコン			(BYTE:大きさ)
37	DE_IMAGE	3	27	nm,n	画像			(BYTE:文字幅,BYTE:丈)
59	DE_CHECK	1	26	b	チェック
60	DE_CHECKBOX	1	26	B	チェックボックス
; エントリ名
4	DE_LFN		3	10	Fm,n	LFNファイル名	(ファイル名幅,拡張子幅)
5	DE_SFN		3	10	fm,n	SFNファイル名	(ファイル名幅,拡張子幅)
52	DE_LFN_MUL	3	10	FMm,n	LFN複数行(最終行以外)
53	DE_LFN_LMUL	3	10	FMm,n	LFN複数行(最終行)
62	DE_LFN_EXT	3	10	FEm,n	LFNファイル名(拡張子優先)
63	DE_SFN_EXT	3	10	fEm,n	SFNファイル名(拡張子優先)
; ファイルサイズ
6	DE_SIZE1	1	10	Z	ファイルサイズ1(7)
11	DE_SIZE2	2	11	zn	ファイルサイズ2		(BYTE:幅)
12	DE_SIZE3	2	11	Zn	ファイルサイズ3		(BYTE:幅)
44	DE_SIZE4	2	11	zKn	ファイルサイズ4,k	(BYTE:幅)
; 時刻
7	DE_TIME1	2	11	Tn	更新時刻			(BYTE:幅)
13	DE_TIME2	-2	10	t"s"時刻(強化版)		(種別,ASCIIZ)
; 属性
8	DE_ATTR1	2	11	An	表示属性			(BYTE:幅)
; コメント
9	DE_MEMO		2	11	Cn	コメント			(BYTE:幅)
39	DE_MEMOS	2	11	Cn	コメント(スキップ有)(BYTE:幅)
10	DE_MSKIP	1	8	c	コメントスキップ
61	DE_MEMOEX	3	11	u	拡張コメント		(BYTE:幅,BYTE:ID)
; カラム
54	DE_COLUMN	-3	11	U	カラム拡張			(BYTE:幅,WORD:項目memo,BYTE:自身の大きさ,TCHARZ 名称)
; 拡張
55	DE_MODULE	16	11	X	Module拡張			(BYTE:幅,BYTE:丈,DWORD:ハッシュ BYTE[9]:文字列)

; ※先頭配置 (BYTE:最小値、BYTE:最大値、BYTE:幅加工off)
34	DE_WIDEV	4	8	W	可変長処理
35	DE_WIDEW	4	8	w	可変長処理(窓枠依存型)
// マーク
28	DE_MNUMS	1	18	mn	マーク数
19	DE_MSIZE1	2	19	mSn	マークサイズ「,」なし(BYTE:幅)
20	DE_MSIZE2	2	19	msn	マークサイズ「,」あり(BYTE:幅)
45	DE_MSIZE3	2	19	mKn	マークサイズ「,」あり,k(BYTE:幅)
// ページ
29	DE_ALLPAGE	1	34	P	全ページ数
30	DE_NOWPAGE	1	34	p	現在ページ
// ディレクトリ情報
15	DE_FStype	1	66	Y	ファイルシステム名	3桁固定
42	DE_vlabel	2	67	V	ボリュームラベル	(BYTE:幅)
43	DE_path		2	131	R	現在のディレクトリ	(BYTE:幅)
51	DE_pathmask	2	67	RM	ディレクトリマスク	(BYTE:幅)

17	DE_ENTRYSET	1	66	E	エントリ数 表示/全て
40	DE_ENTRYA0	1	66	E0	エントリ数 全て
41	DE_ENTRYA1	1	66	E1	エントリ数 全て(./..を除く)

18	DE_ENTRYV0	2	67	en	エントリ数 表示		(BYTE:幅)		S2
31	DE_ENTRYV1	1	66	e1	エントリ数 表示(./..を除く)	S3
32	DE_ENTRYV2	1	66	e2	エントリ数 表示 dir	S4
33	DE_ENTRYV3	1	66	e3	エントリ数 表示 file	S5

21	DE_DFREE1	2	67	DF	空き容量「,」なし	(BYTE:幅)
22	DE_DFREE2	2	67	Df	空き容量「,」あり	(BYTE:幅)
23	DE_DUSE1	2	67	DU	使用容量「,」なし	(BYTE:幅)
24	DE_DUSE2	2	67	Du	使用容量「,」あり	(BYTE:幅)
25	DE_DTOTAL1	2	67	DT	総容量「,」なし		(BYTE:幅)
26	DE_DTOTAL2	2	67	Dt	総容量「,」あり		(BYTE:幅)
