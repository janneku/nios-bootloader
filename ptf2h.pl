#!/usr/bin/perl

use strict;
my ($mod, $gotsize);

while(<STDIN>) {
	if (m/^\s*MODULE ([\w\d_]*)/) {
		$mod = $1;
		$gotsize = 0;
	}
	elsif (m/^\s*Base_Address = "0x([a-fA-F\d]*)"/) {
		printf "#define %-20s 0x%s\n", "\U$mod", $1 if $1 ne "N/A";
	}
	elsif (m/^\s*Address_Width = "(\d*)"/) {
		printf "#define %-20s %s\n", "\U${mod}_SIZE", 1 << ($1+1)
			if not $gotsize;
		$gotsize = 1;
	}
	elsif (m/^\s*IRQ_Number = "(\d*)"/) {
		printf "#define %-20s %s\n", "\U${mod}_IRQ", $1 if $1 ne "NC";
	}
}
