#! /usr/bin/perl

#*****************************************************************************
# IrstLM: IRST Language Model Toolkit
# Copyright (C) 2007 Marcello Federico, ITC-irst Trento, Italy

# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

#******************************************************************************
#computes LM statistics over a string

use strict;
use Getopt::Long "GetOptions";
use File::Basename;
use File::Copy qw(move)

my ($lm,$txt)=();
my ($verbose,$help)=();

$help=1 unless

&GetOptions('lm=s' => \$lm,
            'txt=s' => \$txt,
            'v|verbose' => \$verbose,
            'h|help' => \$help);

if ($help || !$lm || !$txt) {
	my $cmnd = basename($0);
  print "\n$cmnd - computes LM statistics over a string\n",
	"\nUSAGE:\n",
	"       $cmnd [options]\n",
	"\nOPTIONS:\n",
    "       --lm  <string>        language model file \n",
    "       --txt <string>        text file\n",
    "       -v, --verbose         (optional) print debugging info on stderr\n"
    "       -h, --help            (optional) print these instructions\n",
    "\n";

  exit(1);
}

if (!$ENV{IRSTLM}){
  die "Set environment variable IRSTLM with path to the irstlm directory\n";
}

my $clm="$ENV{IRSTLM}/bin/compile-lm";

print STDERR "opening a stream on this command |$clm $lm --eval $txt --debug 1|\n" if $(verbose);
open (OUT,"$clm $lm --eval $txt --debug 1|");
while (<OUT>){
  print;
}

print STDERR "closing the stream on the command\n" if $(verbose);
close(OUT);
