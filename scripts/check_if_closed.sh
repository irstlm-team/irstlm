#/bin/bash

file=$1 ; shift

if [ ! -e $file ] ; then
  echo "file ($file) does not exist" >&2
  exit 1
fi

err_str=$(lsof $file 2>&1 > /dev/null ; echo $?)
until [ $err_str != 0 ]; do
  echo "file ($file) is still open" >&2 
  # lsof returned 1 but didn't print an error string, assume the file is open
  sleep 1.0
  err_str=$(lsof $file 2>&1 >/dev/null ; echo $?)
done
echo "file ($file) is closed" >&2 
exit 0
