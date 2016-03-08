#! /bin/bash

set -m # Enable Job Control

function usage()
{
    cmnd=$(basename $0);
    cat<<EOF

$cmnd - estimates a language model file and saves it in intermediate ARPA format

USAGE:
       $cmnd [options]

OPTIONS:
       -i|--InputFile          Input training file e.g. 'gunzip -c train.gz'
       -o|--OutputFile         Output gzipped LM, e.g. lm.gz
       -k|--Parts              Number of splits (default 5)
       -n|--NgramSize          Order of language model (default 3)
       -d|--Dictionary         Define subdictionary for n-grams (optional, default is without any subdictionary)
       -s|--LanguageModelType  Smoothing methods: witten-bell (default), shift-beta, improved-shift-beta, stupid-backoff; kneser-ney and improved-kneser-ney still accepted for back-compatibility, but mapped into shift-beta and improved-shift-beta, respectively
       -p|--PruneSingletons    Prune singleton n-grams (default false)
       -f|--PruneFrequencyThreshold      Pruning frequency threshold for each level; comma-separated list of values; (default is '0,0,...,0', for all levels)
       -t|--TmpDir             Directory for temporary files (default ./stat_PID)
       -l|--LogFile            File to store logging info (default /dev/null)
       -u|--uniform            Use uniform word frequency for dictionary splitting (default false)
       -b|--boundaries         Include sentence boundary n-grams (optional, default false)
       --zipping               enabling zipping of files (default: enabled)
       --no-zipping            disabling zipping of files (default: enabled)
       --add-start-end         add start and end symbols before and after each line
       -v|--verbose            Verbose
       -irstlm|--irstlm        optionally set the directory of installation of IRSTLM; if not specified, the environment variable IRSTLM is used
       -h|-?|--help            Show this message

EOF
}

function fileExists()
{
    file=$1; shift;

    if [ ! -e $file ] ; then
        echo "file ($file) does not exist" >&2
        exit 1
    else
        if [ $verbose ] ; then
            echo "file ($file) exists" >> $logfile 2>&1
        fi
    fi
}

function fileIsOpen()
{
    file=$1; shift;
    $IRSTLM/bin/check_if_closed.sh $file

    err_str=$(lsof $file 2>&1 > /dev/null ; echo $?)
    until [ $err_str != 0 ]; do
        echo "file ($file) is still open" >&2 
        # lsof returned 1 but didn't print an error string, assume the file is open
        sleep 1.0
        err_str=$(lsof $file 2>&1 >/dev/null ; echo $?)
    done
    echo "file ($file) is closed" >&2 
}

#default parameters
logfile=/dev/null
tmpdir=stat_$$
order=3
parts=3
inpfile="";
outfile=""
zipping="--zipping";
addstartend="";
verbose="";
smoothing="witten-bell";
prune="";
prune_thr_str="";
boundaries="";
dictionary="";
uniform="-f=y";
backoff=""

while [ "$1" != "" ]; do
    case $1 in
        -i | --InputFile )	shift;
				inpfile=$1;
				;;
        -o | --OutputFile )	shift;
				outfile=$1;
                                ;;
        -n | --NgramSize )	shift;
				order=$1;
                                ;;
        -k | --Parts )		shift;
				parts=$1;
                                ;;
        -d | --Dictionary )     shift;
                                dictionary="-sd=$1";
                                ;;
        -s | --LanguageModelType )	shift;
					smoothing=$1;
					;;
        -f | --PruneFrequencyThreshold )	shift;
						prune_thr_str="--PruneFrequencyThreshold=$1";
                                        	;;
        -p | --PruneSingletons )	prune='--prune-singletons';
					;;
        -l | --LogFile )        shift;
				logfile=$1;
                                ;;
        -t | --TmpDir )         shift;
				tmpdir=$1;
                                ;;
        -u | --uniform )        uniform=' ';
                                ;;
        -b | --boundaries )     boundaries='--cross-sentence';
				;;
        --zipping )         zipping='--zipping';
				            ;;
        --no-zipping )      zipping='';
	                        ;;
        --add-start-end )       addstartend='--addstartend';
				                ;;
        -v | --verbose )        verbose='--verbose';
                                ;;
        -irstlm | --irstlm )	shift;
				                IRSTLM=$1;
                                ;;
        -h | -? | --help )      usage;
                                exit 0;
                                ;;
        * )                     usage;
                                exit 1;
    esac
    shift
done

if [ -e $logfile -a $logfile != "/dev/null"  -a $logfile != "/dev/stderr" -a $logfile != "/dev/stdout" ]; then
   echo "Logfile $logfile already exists! either remove or rename it."
   exit 7
fi

if [ $verbose ] ; then
    if [ $logfile == "/dev/null" ] ; then
        logfile="/dev/stderr"
    fi
fi

if [ ! $IRSTLM ]; then
   echo "Set IRSTLM environment variable with path to irstlm"
   exit 2
fi

#paths to scripts and commands in irstlm
scr=$IRSTLM/bin
bin=$IRSTLM/bin
gzip=`which gzip 2> /dev/null`;
gunzip=`which gunzip 2> /dev/null`;

#check irstlm installation
if [ ! -e $bin/dict -o  ! -e $scr/split-dict.pl ]; then
   echo "$IRSTLM does not contain a proper installation of IRSTLM"
   exit 3
fi


case $smoothing in
witten-bell) 
smoothing="--witten-bell";
;; 
kneser-ney)
## kneser-ney still accepted for back-compatibility, but mapped into shift-beta
smoothing="--shift-beta";
;;
improved-kneser-ney)
## improved-kneser-ney still accepted for back-compatibility, but mapped into improved-shift-beta
smoothing="--improved-shift-beta"; 
;;
shift-beta)
smoothing="--shift-beta";
;;
improved-shift-beta)
smoothing="--improved-shift-beta";
;;
stupid-backoff)
smoothing="--stupid-backoff";
backoff="--backoff"
;;
*) 
echo "wrong smoothing setting; '$smoothing' does not exist";
exit 4
esac
			


if [ $zipping ] ; then
echo "ZIPPING:yes" >> $logfile 2>&1
else
echo "ZIPPING:no" >> $logfile 2>&1
fi

if [ ! "$inpfile" -o ! "$outfile" ] ; then
    usage
    exit 5
fi
 
if [ -e $outfile ]; then
   echo "Output file $outfile already exists! either remove or rename it."
   exit 6
fi


if [ $verbose ] ; then
echo inpfile='"'$inpfile'"' outfile=$outfile order=$order parts=$parts prune=$prune smoothing=$smoothing dictionary=$dictionary prune_thr_str=$prune_thr_str >> $logfile 2>&1
echo tmpdir=$tmpdir verbose=$verbose zipping=$zipping addstartend=$addstartend >> $logfile 2>&1
fi

#check tmpdir
tmpdir_created=0;
if [ ! -d $tmpdir ]; then
   echo "Temporary directory $tmpdir does not exist"  >> $logfile 2>&1
   echo "creating $tmpdir"  >> $logfile 2>&1
   mkdir -p $tmpdir
   tmpdir_created=1
else
   echo "Cleaning temporary directory $tmpdir" >> $logfile 2>&1
    rm $tmpdir/* 2> /dev/null
    if [ $? != 0 ]; then
        echo "Warning: some temporary files could not be removed" >> $logfile 2>&1
    fi
fi

res=`echo $inpfile | grep ' '`
actual_inpfile=""
if [[ $res ]] ; then
    echo "the inpfile is a command, actually" >> $logfile 2>&1
    if [ $addstartend ] ; then
        actual_inpfile="$inpfile | $scr/add-start-end.sh"
    else
        actual_inpfile="$inpfile"
    fi
else
    echo "the inpfile is a file, actually" >> $logfile 2>&1
    if [ $addstartend ] ; then
        actual_inpfile="$scr/add-start-end.sh < $inpfile"
    else
        actual_inpfile="$inpfile"
    fi
fi


if [ $verbose ] ; then
    echo "input is the following:\'$actual_inpfile\'" >> $logfile 2>&1
fi

echo >> $logfile 2>&1
date >> $logfile 2>&1
echo "Extracting dictionary from training corpus" >> $logfile 2>&1
$bin/dict -i="$actual_inpfile" -o=$tmpdir/dictionary.tmp $uniform -sort=no 2> $logfile

#checking whether the temporary files exist
#checking whether the temporary files are closed
#renaming the temporary filenames into the official filenames
tmp_out_file="$tmpdir/dictionary"
fileExists ${tmp_out_file}.tmp
fileIsOpen ${tmp_out_file}.tmp
mv ${tmp_out_file}.tmp  ${tmp_out_file}

res=$?
if [ $res -ne 0 ] ; then
    echo "creation of the dictionary failed with error status $res"
    exit $res
fi


echo >> $logfile 2>&1
date >> $logfile 2>&1
echo "Splitting dictionary into $parts lists" >> $logfile 2>&1
sfxList=`$scr/split-dict.pl --input $tmpdir/dictionary --output $tmpdir/dict. --parts $parts 2>> $logfile`
echo "sfxList:$sfxList" >> $logfile 2>&1

#checking whether the temporary files exist
#checking whether the temporary files are closed
for sfx in $sfxList ; do
    tmp_out_file="$tmpdir/dict.${sfx}"
    fileExists ${tmp_out_file}
    fileIsOpen ${tmp_out_file}
done

echo >> $logfile 2>&1
date >> $logfile 2>&1
echo "Extracting n-gram statistics for each word list" >> $logfile 2>&1
echo "Important: dictionary must be ordered according to order of appearance of words in data" >> $logfile 2>&1
echo "used to generate n-gram blocks,  so that sub language model blocks results ordered too" >> $logfile 2>&1

for sfx in $sfxList ; do
sdict="$tmpdir/dict.$sfx"
sdict=`basename $sdict`
echo "Extracting n-gram statistics for $sdict" >> $logfile 2>&1
if [ $smoothing = "--shift-beta" -o $smoothing = "--improved-shift-beta" ]; then
additional_parameters="-iknstat=$tmpdir/ikn.stat.$sdict"
else
additional_parameters=""
fi

if [ $zipping ] ; then
$bin/ngt -i="$actual_inpfile" -n=$order -gooout=y -o="$gzip -c > $tmpdir/ngram.${sdict}.gz" -fd="$tmpdir/$sdict" $dictionary $additional_parameters >> $logfile 2>&1 &
else
$bin/ngt -i="$actual_inpfile" -n=$order -gooout=y -o="$tmpdir/ngram.${sdict}" -fd="$tmpdir/$sdict" $dictionary $additional_parameters >> $logfile 2>&1 &
fi
done

# Wait for all parallel jobs to finish
while [ 1 ]; do fg 2> /dev/null; [ $? == 1 ] && break; sleep 0.1; done

#checking whether the temporary files exist
#checking whether the temporary files are closed
for sfx in $sfxList ; do
    tmp_out_file="$tmpdir/ngram.dict.${sfx}"
    if [ $zipping ] ; then tmp_out_file="${tmp_out_file}.gz" ; fi
    fileExists ${tmp_out_file}
    fileIsOpen ${tmp_out_file}
done

echo >> $logfile 2>&1
date >> $logfile 2>&1
echo "Estimating language models for each word list" >> $logfile 2>&1
for sdict in `ls $tmpdir/dict.*` ; do
sdict=`basename $sdict`
echo "Estimating language models for $sdict" >> $logfile 2>&1

if [ $smoothing = "--shift-beta" -o $smoothing = "--improved-shift-beta" ]; then
additional_smoothing_parameters="cat $tmpdir/ikn.stat.dict.*"
additional_parameters="$backoff"
else
additional_smoothing_parameters=""
additional_parameters=""
fi

ngrams="$tmpdir/ngram.${sdict}"
if [ $zipping ] ; then ngrams="$gunzip -c $tmpdir/ngram.${sdict}.gz" ; fi
$scr/build-sublm.pl $verbose $prune $prune_thr_str $smoothing "$additional_smoothing_parameters" --size $order --ngrams "$ngrams" -sublm $tmpdir/lm.$sdict $additional_parameters $zipping >> $logfile 2>&1 &

done

# Wait for all parallel jobs to finish
while [ 1 ]; do fg 2> /dev/null; [ $? == 1 ] && break; sleep 0.1; done

#checking whether the temporary files exist
#checking whether the temporary files are closed
#renaming the temporary filenames into the official filenames
for sfx in $sfxList ; do
    o=1
    while [ $o -le $order ] ; do
        tmp_out_file="$tmpdir/lm.dict.${sfx}.${o}gr"
        if [ $zipping ] ; then tmp_out_file="${tmp_out_file}.gz" ; fi
        fileExists ${tmp_out_file}
        fileIsOpen ${tmp_out_file}
        o=$(( $o + 1))
    done
done

echo >> $logfile 2>&1
date >> $logfile 2>&1
echo "Merging language models into $outfile" >> $logfile 2>&1
$scr/merge-sublm.pl --size $order --sublm $tmpdir/lm.dict -lm ${outfile} $backoff $zipping >> $logfile 2>&1

#checking whether the temporary files exist
#checking whether the temporary files are closed
tmp_out_file="${outfile}"
if [ $zipping ] ; then tmp_out_file="${tmp_out_file}.gz" ; fi
fileExists ${tmp_out_file}
fileIsOpen ${tmp_out_file}

echo >> $logfile 2>&1
date >> $logfile 2>&1
echo "ARPA LM file created into $outfile" >> $logfile 2>&1

if [ ! $verbose ] ; then
    echo "Cleaning temporary directory $tmpdir" >> $logfile 2>&1
    rm $tmpdir/* 2> /dev/null

    if [ $tmpdir_created -eq 1 ]; then
        echo "Removing temporary directory $tmpdir" >> $logfile 2>&1
        rmdir $tmpdir 2> /dev/null
        if [ $? != 0 ]; then
            echo "Warning: the temporary directory could not be removed." >> $logfile 2>&1
        fi
    fi
fi
 
exit 0




