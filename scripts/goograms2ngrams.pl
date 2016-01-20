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



#transforms google n-grams into real n-grams so that  counts are
#consistent with respect to lower order n-grams

use strict;
use Getopt::Long "GetOptions";

my $cutoffword="<CUTOFF>"; #special word for Google 1T-ngram cut-offs
my $blocksize=10000000; #this is the blocksize of produced n-grams
my $from=2;             #starting n-gram level

my($help,$verbose,$maxsize,$googledir,$ngramdir)=();
my ($tmp_open,$gzip,$gunzip,$zipping)=("","","","");

$help=1 unless
&GetOptions('maxsize=i' => \$maxsize,
			'startfrom=i' => \$from,
			'googledir=s' => \$googledir,
			'ngramdir=s' => \$ngramdir,
			'h|help' => \$help,
			'verbose' => \$verbose,
			'zipping' => \$zipping);


if ($help || !$maxsize || !$googledir || !$ngramdir ) {
	my $cmnd = "goograms2ngrams.pl";
  print "\n$cmnd - transforms google n-grams into real n-grams so that\n",
	"       counts are consistent with respect to lower order n-grams\n",
	"\nUSAGE:\n",
	"       $cmnd [options]\n",
	"\nOPTIONS:\n",
	"       --maxsize <int>       maximum n-gram level of conversion\n",
	"       --startfrom <int>     skip initial levels if already available (default 2)\n",
	"       --googledir <string>  directory containing the google-grams dirs (1gms,2gms,...)\n",
	"       --ngramdir <string>   directory where to write the n-grams \n",
	"       --zipping             (optional) enable usage of zipped temporary files (disabled by default)\n",
	"       --verbose             (optional) very talktive output\n",
	"       -h, --help            (optional) print these instructions\n",
	"\n";

  exit(1);
}

if ($zipping){
  $gzip=`which gzip 2> /dev/null`;
  $gunzip=`which gunzip 2> /dev/null`;
  chomp($gzip);
  chomp($gunzip);
}

warn "goograms2ngrams: maxsize $maxsize from $from googledir $googledir ngramdir $ngramdir zipping $zipping\n"
if $verbose;

die "goograms2ngrams: value of --maxsize must be between 2 and 5\n" if $maxsize<2 || $maxsize>5;
die "goograms2ngrams: cannot find --googledir $googledir \n" if ! -d $googledir;
die "goograms2ngrams: cannot find --ngramdir  $ngramdir \n" if ! -d $ngramdir;


my ($n,$hgrams,$ggrams,$ngrams)=();
my ($ggr,$hgr,$hgrcnt,$ggrcnt,$totggrcnt)=();
my (@ggr,@hgr)=();

foreach ($n=$from;$n<=$maxsize;$n++){
  
  my $counter=0;
  	
  warn "Converting google-$n-grams into $n-gram\n"; 

  if ($zipping){
    $hgrams=($n==2?"${googledir}/1gms/vocab.gz":"${ngramdir}/".($n-1)."grams-*.gz");
    $tmp_open="$gunzip -c $hgrams |";
  }else{ $tmp_open="";
    $hgrams=($n==2?"${googledir}/1gms/vocab":"${ngramdir}/".($n-1)."grams-*");
    $tmp_open="cat $hgrams |";
  }
  open(HGR,"$tmp_open") || die "cannot open $hgrams\n";

  if ($zipping){
    $ggrams="${googledir}/".($n)."gms/".($n)."gm-*";
    $tmp_open="$gunzip -c $ggrams |";
  }else{ $tmp_open="";
    $hgrams=($n==2?"${googledir}/1gms/vocab":"${ngramdir}/".($n-1)."grams-*");
    $tmp_open="cat $ggrams |";
  }
  open(GGR,"$tmp_open") || die "cannot open $ggrams\n";

  my $id = sprintf("%04d", 0);

  if ($zipping){
    $ngrams="${ngramdir}/".($n)."grams-${id}.gz";
    next if -e $ngrams; #go to next step if file exists already;
    $tmp_open="|$gzip -c > $ngrams";
  }else{ $tmp_open="";
    $ngrams="${ngramdir}/".($n)."grams-${id}";
    next if -e $ngrams; #go to next step if file exists already;
    $tmp_open="$ngrams";
  }
  open(NGR,"$tmp_open") || die "cannot open $ngrams\n";

  chop($ggr=<GGR>); @ggr=split(/[ \t]/,$ggr);$ggrcnt=(pop @ggr);
  #warn "ggr: ",$ggrcnt," ",join(" ",@ggr[0..$n-1]),"\n";

  while ($hgr=<HGR>){	

	$counter++;
	printf(STDERR ".") if ($counter % 1000000)==0;
	  
	chop($hgr); @hgr=split(/[ \t]/,$hgr); $hgrcnt=(pop @hgr);
    #warn "hgr: ",$hgrcnt," ",join(" ",@hgr[0..$n-2]),"\n";
  
	if (join(" ",@hgr[0..$n-2]) eq join(" ",@ggr[0..$n-2])){ 

		$totggrcnt=0;
		do{
			$totggrcnt+=$ggrcnt;
			print NGR join(" ",@ggr[0..$n-1])," ",$ggrcnt,"\n";
			chop($ggr=<GGR>);@ggr=split(/[ \t]/,$ggr);$ggrcnt=(pop @ggr);
		}until (join(" ",@hgr[0..$n-2]) ne join(" ",@ggr[0..$n-2]));

		if ($hgrcnt > $totggrcnt){
			#warn "difference: $hgrcnt $totggrcnt =",$hgrcnt-$totggrcnt,"\n";
			print NGR join(" ",@hgr[0..$n-1])," ",$cutoffword," ",$hgrcnt-$totggrcnt,"\n";
		}
	}
	else{ 
		#warn "fully pruned context: $hgr\n";
		print NGR join(" ",@hgr[0..$n-1])," ",$cutoffword," ",$hgrcnt,"\n";
	}	
	
	if (($counter % $blocksize)==0){ 
		close(NGR);
		my $id = sprintf("%04d", int($counter / $blocksize));

		if ($zipping){
		  $ngrams="${ngramdir}/".($n)."grams-${id}.gz";
		  $tmp_open="|$gzip -c > $ngrams";
		}else{
		  $ngrams="${ngramdir}/".($n)."grams-${id}";
		  $tmp_open="$ngrams";
		}
		open(NGR,"$tmp_open") || die "cannot open $ngrams\n";
	}
	
  }

  close(HGR);close(NGR);close(GGR);
  
}
  
  
  
  
  
