#!/usr/bin/perl
# 組み込み言語ファイルの加工
#
# 識別ID($id)	リソースID($resid)	英語埋込用($en)	英語原文($en2)	日本語(&jp)	コメント($comment)

	$lang_jp = 'resource/L0411_JP.txt';
	$lang_en = 'resource/L0409_EN-US.txt';
	$lang_base = 'resource/L0000.txt';
	open(IN, "< $lang_base");
	open(OUT_I, "> PPXID.H");
	open(OUT_H, "> PPXMES.H");
	open(OUT_R, "> PPXMES.RH");
	open(OUT_E, "> $lang_en");
	open(OUT_J, "> $lang_jp");
	binmode OUT_J;
	print OUT_I "// This file generated from $lang_base\n";
	print OUT_H "// This file generated from $lang_base\n";
	print OUT_R "// This file generated from $lang_base\n";

	$title = <IN>;
	chop $title;
	if ( $title =~ /^([^\t]+)\t([^\t]+)\t([^\t]+)/ ){
		print OUT_E "Mes0409\t$2\n";
		print OUT_J "$1|Mes0411\t$3\n";
	}

	while (<IN>) {
		chop;

		if ( $_ =~ /^([0-9A-Za-z\+\.\|]+)\t([^\t]*)\t?([^\t]*)\t?([^\t]*)\t?([^\t]*)\t?([^\t]*)/ ){
			$id = $1;
			$resid = $2;
			$en = $3;
			$en2 = $4;
			$jp = $5;
			$comment = $6;
			$verid = '';
			if ( $id =~ /\|/ ){ print "$id : Bad '|'\n"; }
			if ( $id =~ /^(.*)(\+.*)/ ){
				$verid = "$2|";
				$id = $1;
			}
			if ( $id eq $oldid ){ print "$id : Same name\n"; }
			if ( $id lt $oldid ){ print "$id : name sort\n"; }
			$oldid = $id;

			if ( $id =~ /^[0-9]/ ){ # dialog
				if ( $resid ne '' ){ print OUT_I "#define $resid 0x$id\n"; }
			}

			if ( $en ne '' ){
				if ( $id eq '0000' ){
					print OUT_H "#define MES_REVISION T(\"$en\")\n";
					$id = '+|'.$id;
				}else{
					if ( $id =~ /^(([0-9A-W][0-9A-Z]+)|(XUU[A-Z]))$/ ){
						print OUT_H "#define MES_$id T(\"$id|$en\")\n";
					}
				}
				if ( $id =~ /^([078][^\t]+)$/ ){
					print OUT_R "#define MES_$id \"$en\"\n";
				}
				if ( $en2 ne '' ){ $en = $en2; }
				$en =~ s/\&([A-Za-z])(([^)]|$).*)/$1$2(&$1)/;
				print OUT_E "$id\t$en\n";
			}
			if ( ($jp ne '') && ($jp !~ /^\;/) ){
				print OUT_J "$verid$id\t$jp\n";
			}
		}
		if ( $_ =~ /^(-[^\t]+)/ ){
			print OUT_J "$_\t\n";
		}
	}
	print OUT_E "};";
	print OUT_J "};";
	close(OUT_J);
	close(OUT_E);
	close(OUT_R);
	close(OUT_H);
	close(OUT_I);
	close(IN);
	system("tfilesign sf $lang_en $lang_en");
	system("tfilesign sf $lang_jp $lang_jp");
