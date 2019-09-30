#!/bin/bash
show_usage()
{
    printf "\ngpiod.sh <chip> <pin number> <value>\n"
}

if [ $# -ne 3 ] ; then
    show_usage
    printf "\n\nERROR: incorrect number of parameters\n"
    exit 255
fi

mkdir -p /tmp/gpiod

if [ -e /tmp/gpiod/$1_$2 ]; then
        kill `cat /tmp/gpiod/$1_$2`
fi

gpioset --mode=signal $1 $2=$3 &
result=$?
echo $! > /tmp/gpiod/$1_$2
exit $result
