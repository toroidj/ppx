#!/usr/bin/perl
# Win Help 用簡易コンバータ (c)TORO
	use Encode;

	$helpname = 'PPX';
	$src = 'xhelp.txt';
	$linenum = 0;
	$myurl = 'https://toro.d.dooo.jp';

	($sec, $min, $hour, $mday, $mon, $copyyear, $wday) = localtime(time);
	$copyyear += 1900;

	$html = $ARGV[0];

	if ( $html ){
		$helpname =~ tr/A-Z/a-z/;
		$dest = $helpname.'help.html';
		$cnt = $helpname.'index.html';
		$words = $helpname.'words.html';
		$commands = $helpname.'cmd.txt';
		$idn = "ID";

		$tag_tail = '<br>';
		$tag_ul = "<hr>";
		$tabmode = '&nbsp;&nbsp;';
	}else{
		$dest = $helpname.'temp.rtf';
		$cnt = $helpname.'.CNT';
		$idn = "";

		$tag_tail = '\par';
		$tag_ul = "\\pard\\brdrb\\brdrs\\par\\pard";
	}

	$indexcnt = 1;
	$anchorcnt = 1000;
#   $anchor{'keyword'} アンカーと ID の対応表
	$tail = "\n";
# 1 ならポップアップ用
	$popmode = 0;
# 1 なら一覧の下に書くコメントを保存する
	$grouptitlememomode = 0;

#	open(IN, "< PPXVER.H");
#	$version ="?";
#	while (<IN>) {
#		if ( $_ =~ /FileProp_Version[\s\t]*\"([\S]*)\"/ ){
#			$version = $1;
#		}
#	}
#	close(IN);

# ID→数値 変換テーブルを作成
	if ( $html ){
		open(IN, "< $helpname.rh");
		while (<IN>) {
			chop;

			if ( $_ =~ /^#define[\s\t]+([\S]+)[\s\t]+([\S]+)/ ){
				$helpid{$1} = eval($2);
			}
		}
		close(IN);
		open(IN, "< PPXID.H");
		while (<IN>) {
			chop;

			if ( $_ =~ /^#define[\s\t]+([\S]+)[\s\t]+([\S]+)/ ){
				$helpid{$1} = eval($2);
			}
		}
		close(IN);
	}else{ # CustIDチェック用テーブルを作成
		open(IN, "< PPD_CSTD.C");
		while (<IN>) {
			chop;

			while ( $_ =~ /([A-Z]+\_[0-9A-Za-z]+)/g ){
				$CustID{$1} = 1;
			}
		}
		close(IN);
	}

	open(IN, "< $src");
	open(OUT, "> $dest");
	open(OUTCNT, "> $cnt");

	if ( $html ){
		$head = <<_last;
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd"><html lang="ja">
<head><meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style type="text/css"><!--
body { line-height: 150%; word-break: break-word; }
hr { border: solid 1px; }
pre { line-height: 110%; font-size: 105%; white-space: pre-wrap; background-color: #f6f69f; }
img { max-width: 100%; height: auto; }
--></style>
<title>PPx help</title></head>
<body bgcolor="#eeee99" text="#000000" link="#0000CC" vlink="#8800cc" alink="#440088">
_last
		printOut($head);
		$head = <<_last;
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd"><html lang="ja">
<head><meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style type="text/css"><!--
body { line-height: 150%; word-break: break-word; }
--></style>
<title>index</title></head>
<body bgcolor="#eeee99" text="#000000" link="#0000CC" vlink="#8800cc" alink="#440088">
目次 <a href="$words">(索引)<br><br></a>
_last
		printIndex($head);
	}else{
		print OUT <<_last;
{\\rtf\\deff1
{\\fonttbl{\\f0\\froman\\fcharset128\\fprq1 明朝{\\*\\falt ＭＳ 明朝};}
{\\f1\\froman\\fcharset128\\fprq1 ＭＳ Ｐゴシック;}
{\\f2\\froman\\fcharset128\\fprq1 ＭＳ Ｐ明朝;}
{\\f3\\fmodern\\fcharset128\\fprq1 ＭＳ ゴシック;}
{\\f4\\fmodern\\fcharset128\\fprq1 ＭＳ 明朝;}
{\\f5\\fnil\\fcharset2\\fprq2 Wingdings;}
{\\f6\\froman\\fcharset0\\fprq2 Century;}
{\\f7\\froman\\fcharset0\\fprq3 Arial;}
{\\f8\\froman\\fcharset0\\fprq3 Courier New;}
{\\f9\\froman\\fcharset0\\fprq3 Times New Roman;}}\\fs22
{\\colortbl;
\\red000\\green000\\blue000;\\red128\\green128\\blue128;\\red175\\green175\\blue175;
\\red255\\green000\\blue000;\\red128\\green000\\blue000;\\red000\\green255\\blue000;
\\red000\\green128\\blue000;\\red000\\green000\\blue255;\\red000\\green000\\blue128;
\\red255\\green255\\blue000;\\red128\\green128\\blue000;\\red000\\green255\\blue255;
\\red000\\green128\\blue128;\\red255\\green000\\blue255;\\red128\\green000\\blue128;
\\red255\\green255\\blue255;}
_last
		print OUTCNT ":Base ".$helpname.".HLP\n";
	}

	while ($line = <IN>){
		chop($line);
		$linenum++;

		if ( $html ){
			$line =~ s/\\\{/\{/g;
			$line =~ s/\\\}/\}/g;
			$line =~ s/\\\\/\\/g;
			$line =~ s/<blue>/<b>/g;
			$line =~ s/<\/blue>/<\/b>/g;
			$line =~ s/&/&amp;/g;
			$line =~ s/"/&quot;/g;
			$line =~ s/< /&lt;/g;
			$line =~ s/ >/&gt;/g;
		}else{
			if ( $line =~ /[\!-\<\>-\[\]-z]\\[\!-\<\>-\[\]-z]/ ){
				printf("%d : '\\' not escape\n", $linenum);
			}
			$line =~ s/([\t\s\!-\[\]-z])\{/$1\\\{/g;
			$line =~ s/^\{/\\{/;
			$line =~ s/([\t\s\!-\[\]-z])\}/$1\\\}/g;
			$line =~ s/^\}/\\}/;
			$line =~ s/<blue>/\{\\cf8 /g;
			$line =~ s/<\/blue>/\}/g;
			$retry = 1;
			while ( $retry ){
				$retry = 0;
				if ( $line =~ /<u>/ ){
					if ( $ulcount ){printf("%d : '<u>' over written from %d\n", $linenum, $ulcount);}
					$ulcount = $linenum;
					$line =~ s/<u>/{\\ul /;
					$retry = 1;
				}
				if ( $line =~ /<\/u>/ ){
					if ( !$ulcount ){printf("%d : '<\/u>' over written\n", $linenum);}
					$ulcount = 0;
					$line =~ s/<\/u>/\}/;
					$retry = 1;
				}
				if ( $line =~ /<b>/ ){
					if ( $boldcount ){printf("%d : '<b>' over written from %d\n", $linenum, $boldcount);}
					$boldcount = $linenum;
					$line =~ s/<b>/{\\b /;
					$retry = 1;
				}
				if ( $line =~ /<\/b>/ ){
					if ( !$boldcount){printf("%d : '<\/b>' over written\n", $linenum);}
					$boldcount = 0;
					$line =~ s/<\/b>/\}/;
					$retry = 1;
				}
				if ( $line =~ /<i>/ ){
					if ( $itccount ){printf("%d : '<i>' over written from %d\n", $linenum, $itccount);}
					$itccount = $linenum;
					$line =~ s/<i>/{\\i /;
					$retry = 1;
				}
				if ( $line =~ /<\/i>/ ){
					if ( !$itccount ){printf("%d : '<\/i>' over written\n", $linenum);}
					$itccount = 0;
					$line =~ s/<\/i>/\}/;
					$retry = 1;
				}
			}
			$line =~ s/< /</g;
			$line =~ s/ >/>/g;
		}

		# 行末の処理方法 :tail=\par 本文
		if ( $line =~ /^\:tail=(.*)/ ){
			$tail = $tag_tail."\n";
			next;
		}
		# :popmode ポップアップ用データ開始位置
		if ( $line =~ /:popmode/ ){
			if ( $html ){
				last;
			}
			$popmode = 1;
			next;
		}

		if ( $line =~ /<grouptitlememo>/ ){
			CheckTermTag();
			$grouptitlememo = $tail;
			$grouptitlememomode = 1;
			next;
		}
		# <grouptitle:title> グループ一覧
		if ( $line =~ /<grouptitle:([^>]*)>/ ){
			$grouptitlememomode = 0;
			DumpGroupItem();
			$tab = 2000;

			if ( $html ){
				printOut("<a href=\"#$idn$anchorcnt\">$1<\/a>$tail");
				$grouplist .= "$tag_ul$tail\n".
						"<a name=\"$idn$anchorcnt\"><b>$1<\/b><\/a>$tail";
				$groupitem .= "$tag_ul$tail\n"."<b>$1<\/b>$tail";
			}else{
				print OUT "\{\\uldb $1\}\{\\v $anchorcnt\}$tail\n";
				$grouplist .= "$tag_ul$tail\n".
						"#\{\\footnote  $anchorcnt\}\{\\b $1\}$tail$tail\n".
						"\\pard \\li$tab\\fi-$tab\\tx$tab";
				$groupitem .= "$tag_ul$tail\n"."\{\\b $1\}$tail\n";
			}
			$anchorcnt++;
			next;
		}
		# <group:name::title> グループアイテム(二種表記)
		if ( $line =~ /<group:(.*)::(.*)>/ ){
			DumpGroupItem();
			$groupitem1 = $1;
			$groupitem2 = $2;
			$groupitemA = "";
			if ( $html ){
				$t = $1;
				$t =~ s/^#//;
				$groupitemA = $wordlist{$t} = $idn.$anchorcnt;
				$anchor{$groupitemA} = $idn.$anchorcnt++;
			}
			next;
		}
		if ( $line =~ /<groupa:(.*)::(.*)::(.*)>/ ){
			DumpGroupItem();
			$groupitem1 = $1;
			$groupitem2 = $2;
			$groupitemA = $3;
			if ( $html ){ $t = $1; $t =~ s/^#//; $wordlist{$t} = $groupitemA; }
			next;
		}
		# <group:title> グループアイテム
		if ( $line =~ /<group:(.*)>/ ){
			DumpGroupItem();
			if ( $1 eq "dump" ){
				printOut("$grouptitlememo$grouplist\n$groupitem\n");
				$grouplist = "";
				$groupitem = "";
				$grouptitlememomode = 0;
				$grouptitlememo = "";
			}else{
				$groupitem1 = $1;
				$groupitem2 = "";
				$groupitemA = "";
			}
			next;
		}
		if ( $line =~ /<groupa:(.*)::(.*)>/ ){
			DumpGroupItem();
			$groupitem1 = $1;
			$groupitem2 = "";
			$groupitemA = $2;
			if ( $html ){ $t = $1; $t =~ s/^#//; $wordlist{$t} = $groupitemA; }
			next;
		}

		if ( $html ){
			if ( $line =~ s/<option:(.*)::([^>]*)>/<b>$1<\/b> $2/ ){
				$t1 = $1;
				$t2 = $2;
				$t = $groupitem1; $t =~ s/^#//;
				$optiontext .= "\t$t1 ;$t2\n";
			}
		}else{
			$line =~ s/<option:(.*)::([^>]*)>/{\\b $1\} $2/;
		}

		# <a:keyword> アンカー生成
		while ( $line =~ /<a:([^>]*)>/ ){
			if ( $anchorset{$1} ){
				printf("%d : '<a:%s>' red(%d)\n", $linenum, $1, $anchorset{$1});
			}else{
				$anchorset{$1} = $linenum;
			}
			if ( !$anchor{$1} ){
				$anchor{$1} = $idn.$anchorcnt++;
			}
			if ( $html ){
				$line =~ s/<a:([^>]*)>/<a name="$1"><\/a>/;
			}else{
				$line =~ s/<a:([^>]*)>/#\{\\footnote  $anchor{$1}\}/;
			}
		}
		# <atk:keyword> アンカー&タイトル生成
		if ( $line =~ /<atk:([^>]*)>/ ){
			if ( $anchorset{$1} ){
				printf("%d : '<jmpa:%s>' red(%d)\n", $linenum, $1, $anchorset{$1});
			}else{
				$anchorset{$1} = $linenum;
			}
			if ( !$anchor{$1} ){
				$anchor{$1} = $idn.$anchorcnt++;
			}
			if ( $html ){
				$wordlist{$1} = $1;
				$line =~ s/<atk:([^>]*)>/<a name="$1"><b>$1<\/b><\/a>/;
			}else{
				$line =~ s/<atk:([^>]*)>/#\{\\footnote  $anchor{$1}\} K\{\\footnote  $1\}{\\b $1}/;
			}
		}
		# <jmpa:string&keyword> アンカーへのリンク
		while ( $line =~ /<jmpa:([^>:]*)>/ ){
			if ( !$anchor{$1} ){ $anchor{$1} = $idn.$anchorcnt++; }
			if ( $html ){
				$line =~ s/<jmpa:([^>:]*)>/<a href="#$1">$1<\/a>/;
			}else{
				$line =~ s/<jmpa:([^>:]*)>/\{\\uldb $1\}\{\\v $anchor{$1}\}/;
			}
		}
		# <jmpa:string:keyword> アンカーへのリンク
		while ( $line =~ /<jmpa:([^>:]*):([^>]*)>/ ){
			if ( !$anchor{$2} ){ $anchor{$2} = $idn.$anchorcnt++; }
			if ( $html ){
				$line =~ s/<jmpa:([^>:]*):([^>]*)>/<a href="#$2">$1<\/a>/;
			}else{
				$line =~ s/<jmpa:([^>:]*):([^>]*)>/\{\\uldb $1\}\{\\v $anchor{$2}\}/;
			}
		}
		# <img:filepath> 画像
		if ( $line =~ /<img:([^>]*)>/ ){
			if ( $html ){
				print OUT "<img src=\"./$1\"><br>";
			}
			next;
		}
		# <myurl>
		$line =~ s/<myurl>/$myurl/g;
		# <copyyear>
		$line =~ s/<copyyear>/$copyyear/g;

		# .n=title 新しいブックとそのタイトル
		if ( $line =~ /^\.(\d*)\=(.*)/ ){
			if ( $html ){
				printIndex("<b>$2<\/b><br>\n");
			}else{
				print OUTCNT "$1 $2\n";
			}
			$titlestr[$1] = $2;
			next;
		}
		# .n:title[=id] 新しいページとそのタイトル
		if ( $line =~ /^\.(\d*)\:(.*)/ ){
			$depth = $1;
			$name = $2;
			if ( $2 =~ /^(.*)\=(.*)/ ){
				$name = $1;
				$id = $2;
				if ( $html ){
					$idn = $2;
					$anchorcnt = 1;
				}
			}else{
				$id = $idn.$anchorcnt++;
			}
			if ( $html ){
				printIndex("<a href=\"$dest#$id\" target=\"main\">$name<\/a><br>\n");
			}else{
				print OUTCNT "$depth $name=$id\n";
			}
			$titlestr[$depth] = $name;
			$title = $titlestr[1];
			for ( $i = 2 ; $i <= $depth ; $i++ ){
				$title = $title." - ".$titlestr[$i];
			}
			if ( $html ){
				$line = "<a name=\"$id\"></a><a name=\"$name\"></a><h3>$title</h3>";
			}else{
				$line = '#{\\footnote  '.$id.'}${\\footnote  '.$name.
					'}K{\\footnote  '.$name.'}+{\\footnote 0}\\pard{\\b '.
					$title."}\n\\sl280\\par";
			}
		}
		# +footnote の連番処理
		if ( $line =~ /\+\{\\footnote .*\}/ ){
			$id = sprintf("index:%03d", $indexcnt++);
			if ( $html ){
				$line = "";
			}else{
				$line =~ s/\+\{\\footnote [0-9A-Za-z\:]\}/\+\{\\footnote $id\}/g;
			}
		}

		if ( $html ){
			if ( $line =~ /\\page/ ){
				CheckTermTag();
				$line =~ s/\\page/<hr>/;
			}
			if ( $line =~ /<page>/ ){
				CheckTermTag();
				$line =~ s/<page>/<nocr><hr>/;
			}
			if ( $line =~ /<(tk|key):([^>]*)/ ){
				$wordlist{$2} = $2;
			}
			$line =~ s/<key:([^>]*)>/<a name="$1"><\/a>/g;	# <key:keyword>
			$line =~ s/<tk:([^>]*)>/<a name="$1"><b>$1<\/b><\/a>/g;	# <tk:title>
			while ( $line =~ /<hid:([^>]*)>/ ){				# <hid:ResourceId>
				$hid = $1;
				if ( $helpid{$hid} ne '' ){ $hid = $helpid{$hid};}
				$line =~ s/<hid:([^>]*)>/<a name="$hid"><\/a>/;
			}
			if ( $line =~ /<tb:0>/ ){							# <tb:0> 列挙終了
				if ( !$listmode ){printf("%d : '<tb:0>' over written\n", $linenum);}
				$listmode = 0;
				$line =~ s/^<tb:0>/<\/dt><\/dl>/g;
				$line =~ s/<tb:0>/<\/dl>/g;
				$tabcode='&nbsp;&nbsp;';
				$tail="<br>\n";
			}
			if ( $line =~ /<tb:[^>]*>/ ){						# <tb:n> 列挙開始
				if ( $listmode ){printf("%d : '<tb:>' over written from %d\n", $linenum, $listmode);}
				$listmode = $linenum;
				$line =~ s/<tb:[^>]*>/<dl compact>/g;
				$tabcode = '</dt><dd>';
				$tail = '';
			}
			if ( $listmode ){
				if ( $line =~ /\t/ ){
					$line =~ s/\t/<\/dt><dd>/;		# tab(最初のみ)
					$line =~ s/\t/<\/dd><dd>/g;	# tab(２つ目以降)
					if ( $line ne '' ){
						$line .= "</dd>\n<dt>";
					}else{
						$line .= "</dt>\n<dt>";
					}
				}else{
					$line .= "</dt>\n<dt>";
				}
				$line =~ s/<dl compact><\/dd>/<dl compact>/g;
				$line =~ s/<dl compact><\/dt>/<dl compact>/g;
			}else{
				$line =~ s/\t/$tabcode/g;								# tab
			}

			# nothing											# <hr>
			if ( $line =~ /<pre>/ ){
				if ( $precount ){printf("%d : '<pre>' over written from %d\n", $linenum, $precount);}
				$precount = $linenum;
				$tail = "\n";
			}
			if ( $line =~ /<\/pre>/ ){
				if ( !$precount ){printf("%d : '</pre>' over written\n", $linenum);}
				$precount = 0;
				$tail = "<br>\n";
			}
			$line =~ s/<http:([^>]*)>/<a href="http:$1" target="_top">http:$1<\/a>/g;
			$line =~ s/<https:([^>]*)>/<a href="https:$1" target="_top">https:$1<\/a>/g;
			$line =~ s/<mailto:([^>]*)>/<a href="mailto:$1"><i>$1<\/i><\/a>/g;
			$line =~ s/<url:([^>]*)>/<a href="$1">$1<\/a>/g;
			$line =~ s/<copyright:([^>]*)>/<i>$1<\/i>/g;
			$line =~ s/<windowskey>/<font face="Wingdings">&#255;<\/font>/g;
		}else{
			$line =~ s/<key:([^>]*)>/K\{\\footnote  $1\}/g;		# <key:keyword>
			$line =~ s/<tk:([^>]*)>/K\{\\footnote  $1\}{\\b $1}/g;	# <tk:title>
			$line =~ s/<hid:([^>]*)>/#\{\\footnote  $1\}/g;	# <hid:ResourceId>

			$line =~ s/<tb:0>/\\pard /g;							# <tb:0>
			$line =~ s/<tb:([^>]*)>/\\pard \\li$1\\fi-$1\\tx$1 /g;	# <tb:n>
			$line =~ s/\t/\\tab /g;								# tab

			$line =~ s/<hr>/$tag_ul/g;								# <hr>
			$line =~ s/<pre>/{\\f3 /g;								# <pre>
			$line =~ s/<\/pre>/ }/g;								# </pre>
			$line =~ s/<http:([^>]*)>/{\\uldb http:$1}{\\v !ExecFile(http:$1)}/g;
			$line =~ s/<https:([^>]*)>/{\\uldb https:$1}{\\v !ExecFile(https:$1)}/g;
			$line =~ s/<mailto:([^>]*)>/{\\uldb $1}{\\v !ExecFile(mailto:$1)}/g;
			$line =~ s/<url:([^>]*)>/\{\\uldb $1\}\{\\v !ExecFile($1)\}/g;
			$line =~ s/<copyright:([^>]*)>/\{\\cf8 $1\}/g;
			$line =~ s/<windowskey>/\{\\f5\\fs30 \\'ff\}/g;
			$line =~ s/<page>/\\page<nocr>/;

			if ( $line =~ /<NotID:([^>]*)>/ ){
				$CustID{$1} = 1;
				$line =~ s/<NotID:([^>]*)>/$1/g;
			}

			foreach ( $line =~ /([ABEFPSV]|HM|[CKMX][BCETV]?)\_[0-9A-Za-z]+/g ){
				$cid = $&;
				if ( ($cid eq '') | ($cid =~ /(V_H[0-9A-F]|[ACMP]_[A-Z])|XX|xxx|sample|test/) ) {next;}
				if ( !$CustID{$cid} && ($cid =~ /[a-z]/) ){
					print "$linenum : No custid = $cid\n";
				}
			}
		}

		if ( $grouptitlememomode ){
			$grouptitlememo .= "$line$tail\n";
			next;
		}
		if ( $groupitem1 ne "" ){
			$groupdata .= "$line$tail";
			next;
		}

		if ( $popmode ){
			if ( $line =~ /^\#\{\\footnote/ ){
				print OUT $1, "\\page\n";
			}
		}
		if ( $line =~ /<nocr>/ ){ # 行末処理の無効化
			$line =~ s/<nocr>//g;
			printOut("$line\n");
		}else{
			printOut("$line$tail");
		}
	}
	if ( $html ){
		printOut("</body></html>");
		printIndex("</body></html>");

		open(OUTWORDS, "> $words");

		$head = <<_last;
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd"><html lang="ja">
<head><meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style type="text/css"><!--
body { line-height: 150%; word-break: break-word; }
--></style>
<title>index</title></head>
<body bgcolor="#eeee99" text="#000000" link="#0000CC" vlink="#8800cc" alink="#440088">
索引 <a href="$cnt">(目次)</a><br><br>
_last
		print OUTWORDS encode('utf8', decode('cp932', $head));

		foreach(sort {$a cmp $b} keys %wordlist){
			print OUTWORDS encode('utf8', decode('cp932', "<a href=\"$dest#$wordlist{$_}\" target=\"main\">$_<\/a><br>\n"));
		}
		print OUTWORDS '</body></html>';
		close(OUTWORDS);

		open(OUTLIST, "> $commands");
		@commandlist = sort @commandlist;
		foreach( @commandlist ){
			print OUTLIST $_;
			if ( $optionlist{$_} ne "" ){ print OUTLIST $optionlist{$_}; }
		}
		close(OUTLIST);
	}else{
		printOut("}");
	}
	close(OUTCNT);
	close(OUT);
	close(IN);

	if ( $html ){
		$online = "../script/ppxwhelp.pl";
		if (-f $online){
			system("perl $online $dest");
		}
	}else{
		system("hcrtf /x ".$helpname.".HPJ"); # /xh なら作った help を開く
	}
	while (($name, $value) = each(%anchor)) {
		if ( !$anchorset{$name} ){ printf("anchor $name not defined\n", $name)};
	}
0;

sub DumpGroupItem
{
	CheckTermTag();
	if ( $groupitem1 ne "" ){
		local($footnote);

		if ( $groupitemA ne "" ){
			if ( $anchorset{$groupitemA} ){
				printf("%d : '<a:%s>' red(%d)\n", $linenum, $groupitemA, $anchorset{$groupitemA});
			}else{
				$anchorset{$groupitemA} = $linenum;
			}
			if ( !$anchor{$groupitemA} ){
				$anchor{$groupitemA} = $idn.$anchorcnt++;
			}
			if ( $html ){
				$ancname = $groupitemA;
			}else{
				$ancname = $anchor{$groupitemA};
			}
		}else{
			$ancname = $idn.$anchorcnt++;
		}

		if ( $html ){
			if ( $groupitem1 =~ /^\#?([\%?\*|\%].*)/ ){
				$commandtext = "$1 $groupitem2\n";
				$commandtext =~ s/\&amp;/\&/g;
				$commandtext =~ s/\&quot;/\"/g;
				$commandtext =~ s/<\/?i>//g;
				$commandtext =~ s/\s/ ;/;
				push @commandlist, $commandtext;
				$optiontext =~ s/\&amp;/\&/g;
				$optiontext =~ s/\&quot;/\"/g;
				$optiontext =~ s/;\t/;/g;
				$optionlist{$commandtext} = $optiontext;
				$optiontext = "";
			}
		}

		if ( $groupitem1 =~ /^\#(.*)/ ){
			if ( $html ){
#				$footnote = "<a name=\"$1\"><\/a>";
#				print $footnote, "\n";
			}else{
				$footnote = "K\{\\footnote  $1\}";
			}
			$groupitem1 = $1;
		}else{
			$footnote = "";
		}
		if ( $groupitem2 eq "" ){
			$groupitem2 = $groupitem1;
			$groupitem1 = "";
		}else{
			if ( $html ){
				$groupitem1 .= "";
			}else{
				$groupitem1 .= "\\tab";
			}
		}
		if ( $groupdata ne "" ){
			if ( $html ){
				$grouplist .= "・<a href=\"#$ancname\">$groupitem1 $groupitem2<\/a>$tail";
				$groupitem .= "$tag_ul$tail"."<a name=\"$ancname\"><b>$groupitem1 $groupitem2<\/b><\/a>$tail".$groupdata;
			}else{
				$grouplist .= "・$groupitem1\{\\uldb $groupitem2\}\{\\v $ancname\}$tail\n";
				$groupitem .= "$tag_ul$tail\n"."$footnote#\{\\footnote $ancname\}$groupitem1\{\\b $groupitem2\}$tail".$groupdata;
			}
			$groupdata = "";
		}else{
			$grouplist .= "$footnote・$groupitem1 $groupitem2$tail";
		}
		$groupitem1 = "";
		$groupitemA = "";
	}
}

sub CheckTermTag
{
	if ( $listmode ){
		printf("%d : '<tb:' not terminate\n", $linenum); $listmode = 0;
	}
	if ( $html && ($tail eq "\n") ){
		printf("%d : '<pre>' not terminate\n", $linenum); $tail = '<br>\n';
	}
	if ( $precount ){
		printf("%d : '<pre>' not terminate %d\n", $linenum, $precount); $precount = 0;
	}
	if ( $ulcount ){
		printf("%d : '<u>' not terminate %d\n", $linenum, $ulcount); $ulcount = 0;
	}
	if ( $boldcount ){
		printf("%d : '<b>' not terminate %d\n", $linenum, $boldcount); $boldcount = 0;
	}
	if ( $itccount ){
		printf("%d : '<i>' not terminate %d\n", $linenum, $itccount); $itccount = 0;
	}
	if ( $listmode ){
		printf("%d : '<tb:>' not terminate %d\n", $linenum, $listmode); $listmode = 0;
	}
}

sub printOut
{
	if ( $html ){
		print OUT encode('utf8', decode('cp932', $_[0]));
	}else{
		print OUT $_[0];
	}
}

sub printIndex
{
	print OUTCNT encode('utf8', decode('cp932', $_[0]));
}
