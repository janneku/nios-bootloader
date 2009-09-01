#!/usr/bin/perl

use GD;
use strict;

my $img = GD::Image->new($ARGV[0]);
my($w, $h) = $img->getBounds();

my $n = $ARGV[0];
$n =~ s/\./_/g;

print "#define ${n}_width $w\n";
print "#define ${n}_height $h\n\n";

#
# paletti
#

print "static const uint16_t ${n}_palette[] = {\n";

for (my $i = 0; $i < 256; ++$i) {
	my ($r,$g,$b) = $img->rgb($i);
	printf "0x%04x, ", rgb16($r, $g, $b);
	if (($i & 7) == 7) {
		print "\n";
	}
}

print "};\n\n";

#
# kuvan pikselit
#

print "static const unsigned char ${n}_data[] = {\n";

my @yksittaiset = ();
my ($x, $y, $koko);
$x = 0;
$y = 0;
$koko = 0;

while ($y < $h) {
	my $cur = $img->getPixel($x,$y);
	
	# seuraava pikseli
	$x ++;
	if ($x >= $w) {
		$x = 0;
		$y ++;
	}
	
	# montako kertaa toistuu?
	my $count = 0;
	while ($y < $h) {
		my $now = $img->getPixel($x,$y);
		last if $now != $cur;
		
		# seuraava pikseli
		$x ++;
		if ($x >= $w) {
			$x = 0;
			$y ++;
		}
		
		$count ++;
		if ($count == 0x7F) {
			# ei mahdu enempää
			last;
		}
	}
	
	if ($count == 0) {
		# yksittäinen!
		
		push(@yksittaiset, $cur);
		if (scalar(@yksittaiset) - 1 == 0x7F) {
			# ei mahdu enempää
			kirjoita_yksittaiset();
		}
	}
	else {
		# toistuva
		
		if (scalar(@yksittaiset) > 0) {
			# kirjoitetaan ensin yksittäiset
			kirjoita_yksittaiset();
		}
		
		# kirjoitetaan toistuva

		printf "0x%02x, ", $count;
		$koko ++;
		if (($koko & 7) == 7) {
			print "\n";
		}
		printf "0x%02x, ", $cur;
		$koko ++;
		if (($koko & 7) == 7) {
			print "\n";
		}
	}
}
if (scalar(@yksittaiset) > 0) {
	kirjoita_yksittaiset();
}
print "};\n";

print STDERR "pakattu koko: $koko\n";

sub rgb16 {
	my ($r, $g, $b) = @_;
	return (($b) >> 3 | (($g) >> 2) << 5 | (($r) >> 3) << 11);
}

sub kirjoita_yksittaiset {
	printf "0x%02x, ", scalar(@yksittaiset) - 1 + 0x80;
	$koko ++;
	if (($koko & 7) == 7) {
		print "\n";
	}
	foreach (@yksittaiset) {
		printf "0x%02x, ", $_;
		$koko ++;
		if (($koko & 7) == 7) {
			print "\n";
		}
	}
	@yksittaiset = ();
}
