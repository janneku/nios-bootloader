#!/usr/bin/perl

use GD;
use strict;

GD::Image->trueColor(1);

my $img = GD::Image->new($ARGV[0]);
my($w, $h) = $img->getBounds();

my $n = $ARGV[0];
$n =~ s/\./_/g;

print "#define ${n}_height $h\n\n";
print "static const unsigned char ${n}_data[] = {\n";

for (my $k = 0; $k < $w / 8; ++$k) {
	for (my $y = 0; $y < $h; ++$y) {
		my $bitit = 0;
		for (my $i = 0; $i < 8; ++$i) {
			my $x = $k * 8 + $i;
			my ($val,$g,$b) = $img->rgb( $img->getPixel($x,$y) );
			if ($val > 150) {
				$bitit |= 1 << $i;
			}
		}
		printf "0x%02x, ", $bitit;
	}
	print "\n";
}
print "};\n";
