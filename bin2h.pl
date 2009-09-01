#!/usr/bin/perl

my $bin;
while (read(STDIN, $bin, 1)) {
	printf "0x%02x, ", unpack("C", $bin);
}
