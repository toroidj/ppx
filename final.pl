#!/usr/bin/perl
# (c)TORO
#
# map êÆå`
	opendir(handle, '.');
	@filelist = readdir(handle);
	closedir(handle);
	foreach $name (@filelist) {
		if ( $name =~ /\.map$/ ){
			$mode = 0;
			$map = "";
			open(IN, "< $name");
			while (<IN>) {
				chop;
				if ( $_ =~ /Publics by Name/ ){ $mode = 1; }
				if ( $mode && ($_ =~ /Publics by Value/) ){ $mode = 2; }
				if ( $mode != 1 ){ $map .= "$_\n"; }
			}
			close(IN);
			if ( $mode ){
				open(OUT, "> $name");
				print OUT $map;
				close(OUT);
			}
		}
	}
