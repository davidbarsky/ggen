#!/bin/sh

if git describe --tags --always > version.tmp
then
    echo "m4_define([VERSION_NUMBER], [`tr -d '\n' < version.tmp`])" > version.m4
else
    echo "m4_define([VERSION_NUMBER], [local])" > version.m4
fi
rm version.tmp 
