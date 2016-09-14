#/bin/sh

# This script looks into the Registry of Internet Addresses filtered in Russia,
# as provided by https://github.com/zapret-info/z-i, converts it from
# the Windows-1251 codepage, removes all unnecessary data and saves it in the
# bot's data folder.

if [ $# -ne 1 ]; then
    echo "Usage: $0 [dump.csv file]"
    exit 1
fi

if ! [ -f $1 ]; then
    echo "Input file ($1) doesn't exist or is not a file."
    exit 1
fi

INMTIME=`stat -c %Y $1`

if ! [ -e "$HOME/.snowbot" ]; then
    echo "Home directory doesn't exist. Creating."
    mkdir $HOME/snowbot
elif ! [ -d "$HOME/.snowbot" ]; then
    echo "Home directory is occupied by a file."
    exit 1
fi

if [ -f filter.txt ]; then
    OUTMTIME=`istat -c %Y $HOME/.snowbot/filter.txt`
    else
    OUTMTIME=0
fi

if [ $OUTMTIME -ge $INMTIME ]; then
    echo "The output file is more recent than the input file. No conversion needed."
    exit 1
fi

iconv -f cp1251 dump.csv | cut -d ";" -f 1-3 > ~/.snowbot/notify_url.txt
