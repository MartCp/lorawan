#!/usr/bin/env bash

# Thorough
# echo "False False"
# ./downlinkCycleAP.sh 4000 2 40000 3 1 false 1 false false >> falsefalse.out
# echo "False True"
# ./downlinkCycleAP.sh 4000 2 40000 3 1 false 1 false true >> falsetrue.out
# echo "True False"
# ./downlinkCycleAP.sh 4000 2 40000 3 1 false 1 true false >> truefalse.out
# echo "True True"
# ./downlinkCycleAP.sh 4000 2 40000 3 1 false 1 true true >> truetrue.out

# Quick
echo "False False"
./downlinkCycleAP.sh 4000 4 400000 1 0 false 1 false false > falsefalse.out
echo "True False"
./downlinkCycleAP.sh 4000 4 400000 1 0 false 1 true false > truefalse.out
echo "False True"
./downlinkCycleAP.sh 4000 4 400000 1 0 false 1 false true > falsetrue.out
echo "True True"
./downlinkCycleAP.sh 4000 4 400000 1 0 false 1 true true > truetrue.out
