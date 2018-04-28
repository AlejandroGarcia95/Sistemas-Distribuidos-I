#!/bin/bash

m=`ipcs -m | grep -w 644 | cut -d' ' -f2`
for i in $m
do
        echo Removing shm id $i
        ipcrm -m $i
done

s=`ipcs -s | grep -w 644 | cut -d' ' -f2`
for i in $s
do
        echo Removing sem id $i
        ipcrm -s $i
done

q=`ipcs -q | grep -w 644 | cut -d' ' -f2`
for i in $q
do
        echo Removing queue id $i
        ipcrm -q $i
done
