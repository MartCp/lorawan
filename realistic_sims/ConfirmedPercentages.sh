#!/bin/bash

# Number of devices variables
nDevices=200
maxnDevices=2000
incrementDev=200

# Maximum number of tx variables
maxNumbTx=1
maxMaxNumbTx=8
numTransientPeriods=1
numPeriodsToSimulate=3

# Application period
useMixedPeriods=true

# Confirmed message variables
confirmPercent=0
maxConfirmPercent=100
incrementPercent=5

# Runs
maxRuns=10

# Directory management
mkdir -p percentDevicesPlots
rm percentDevicesPlots/* || exit

# Move to the waf directory
cd ../../../ || exit

filename="src/lorawan/realistic_sims/percentDevicesPlots/percentDevices.txt"
echo -n "" > $filename

./waf build > /dev/null

# Run the script with a fixed application period

while [ $maxNumbTx -le $maxMaxNumbTx ]
do
    while [ $nDevices -le $maxnDevices ]
    do
        while [ $confirmPercent -le $maxConfirmPercent ]
        do
            runsFilename="src/lorawan/realistic_sims/percentDevicesPlots/percentDev-$maxNumbTx-$nDevices-$confirmPercent.txt"

            ###########################
            #  Perform multiple runs  #
            ###########################

            run=1
            pSuccSum=0
            echo -n "" > "$runsFilename"
            confirmPercent=$(echo "$confirmPercent / 100" | bc -l)
            # echo "ConfirmPercent:  $confirmPercent"
            # echo "Num of devices: $nDevices "
            while [ $run -le $maxRuns ]
            do

                output="$(./waf --run "RawCompleteNetworkPerformances
                        --radius=1000
                        --nDevices=$nDevices
                        --confirmPercent=$confirmPercent
                        --maxNumbTx=$maxNumbTx
                        --mixedPeriods=$useMixedPeriods
                        --transientPeriods=$numTransientPeriods
                        --periodsToSimulate=$numPeriodsToSimulate
                        --RngRun=$run" | grep -v "build" | tr -d '\n' )"


                echo "MaxNumbTx: $maxNumbTx NDevs: $nDevices Confirm%: $confirmPercent Run#: $run Output: $output"
                echo "$output" >> "$runsFilename"

                pSucc=$(echo "$output" | awk '{print $3}')
                #pSucc=$(echo "$nSucc/$nDevices" | bc -l)
                pSuccSum=$(echo "$pSucc + $pSuccSum" | bc -l)
                # echo "pSucc: $pSucc,    pSuccSum: $pSuccSum"

                run=$((run+1))
            done

            avgPSucc=$(echo "$pSuccSum / $maxRuns" | bc -l)
            # echo "Mean probability of success: $avgPSucc"

            echo "$maxNumbTx $nDevices $confirmPercent $avgPSucc" >> $filename

            confirmPercent=$(echo "$confirmPercent * 100" | bc -l)
            confirmPercent=${confirmPercent/.*}	#float to integer
            confirmPercent=$(echo "$confirmPercent + $incrementPercent" | bc -l)

        done

        confirmPercent=0
        nDevices=$(echo "$nDevices + $incrementDev" | bc -l)

    done

    echo "Current maxNumbTx = $maxNumbTx"
    maxNumbTx=$(echo "$maxNumbTx * 2" | bc -l)

done
