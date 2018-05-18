#!/usr/bin/env bash

# DOWNLINK Network Performances and transmissions required - loop on ED


# Useful variables
# i= number of nDevices
# increment= increment in EDs
gatewayRings=1
i=1
increment=500
maxEndDevices=6001
radius=6300
#gwrad=$(echo "10000/(2*($gatewayRings - 1)+1)" | bc -l)
globalrun=1
maxRuns=5
appPeriod=$1;
periodsToSimulate=$2;
transientPeriods=$3;
DRAdapt=$4;
maxNumbTx=$5;

if [ $appPeriod -eq 0 ]
    then
        mixedPeriods=true
        maxEndDevices=3001
    else
        mixedPeriods=false
fi

# echo "***** DOWNLINK VARYING NUMBER OF END DEVICES *****"
# echo "            INPUT PARAMETERS    "
# echo "appPeriod= $appPeriod"
# echo "periodsToSimulate= $periodsToSimulate"
# echo "transientPeriods= $transientPeriods"
# echo "Data Rate adaptation= $DRAdapt"
# echo "max number of transmission= $maxNumbTx"
# echo "mixedPeriods= $mixedPeriods"
# echo "***************************"

# Move to the waf directory
cd ../../

# Configure waf to use the optimized build
# echo "GR:$gatewayRings, r:$radius, gwr:$gwrad, sim:$simTime, runs:$maxRuns"
# echo "Warning: remember to correctly set up the following:"
# echo "- Channels"
# echo "- Receive paths"
# echo "- Path loss"
# echo -n "Configuring and building..."
./waf --build-profile=optimized --out=build/optimized configure > /dev/null
./waf build > /dev/null
# echo " done."

# Run the script with a fixed period
while [ $i -le $maxEndDevices ]
do
    
    # Perform multiple runs
    currentrun=1
    centraldevicessum=0
    appPeriodLoop=0

    centraltotpacketsum=0
    centralreceivedsum=0
    centralinterferedsum=0
    centralnomorerxsum=0
    centralundersenssum=0

    centralSF7sum=0
    centralSF8sum=0
    centralSF9sum=0
    centralSF10sum=0
    centralSF11sum=0
    centralSF12sum=0
    centralSFoorsum=0

# echo "Done initialization"
    while [ $currentrun -le $maxRuns ]
    do
        # nGateways=$[3*$gatewayRings*$gatewayRings-$[3*$gatewayRings]+1]
        # echo -n "Simulating a system with $i end devices and a transmission period of $simTime seconds...  "
        # START=$(date +%s)
        output="$(./waf --run "RawCompleteNetworkPerformancesWithSFConfirmed
            --nDevices=$i
            --gatewayRings=$gatewayRings
            --radius=$radius
            --gatewayRadius=1500
            --appPeriod=$appPeriod
            --periodsToSimulate=$periodsToSimulate
	        --transientPeriods=$transientPeriods
	        --mixedPeriods=$mixedPeriods
            --maxNumbTx=$maxNumbTx
            --DRAdapt=$DRAdapt
            --RngRun=$globalrun" | grep -v "build" | tr -d '\n')"
 #         echo "$output"
        
        centraldevices=$(echo "$output" | awk '{print $1}')             # nDevices
        appPeriodLoop=$(echo "$output" | awk '{print $2}')
        centraltotpacket=$(echo "$output" | awk '{print $3}')             # total packets sent
        centralreceived=$(echo "$output" | awk '{print $4}')            # received packets
        centralinterfered=$(echo "$output" | awk '{print $5}')          # interfered packets
        centralnomorerx=$(echo "$output" | awk '{print $6}')            # packets discarded because no more receivers available
        centralundersens=$(echo "$output" | awk '{print $7}')           # packets discarded because under sensitivity

        centralS1=$(echo "$output" | awk '{print $8}')
        centralS2=$(echo "$output" | awk '{print $9}')
        centralS3=$(echo "$output" | awk '{print $10}')
        centralS4=$(echo "$output" | awk '{print $11}')
        centralS5=$(echo "$output" | awk '{print $12}')
        centralS6=$(echo "$output" | awk '{print $13}')
        centralS7=$(echo "$output" | awk '{print $14}')
        centralS8=$(echo "$output" | awk '{print $15}')
        centralF1=$(echo "$output" | awk '{print $16}')
        centralF2=$(echo "$output" | awk '{print $17}')
        centralF3=$(echo "$output" | awk '{print $18}')
        centralF4=$(echo "$output" | awk '{print $19}')
        centralF5=$(echo "$output" | awk '{print $20}')
        centralF6=$(echo "$output" | awk '{print $21}')
        centralF7=$(echo "$output" | awk '{print $22}')
        centralF8=$(echo "$output" | awk '{print $23}')

        centralavgdelay=$(echo "$output" | awk '{print $24}')
        centralavgAckdelay=$(echo "$output" | awk '{print $25}')        
        centraltotRetx=$(echo "$output" | awk '{print $26}')

        centralSF7=$(echo "$output" | awk '{print $28}')
        centralSF8=$(echo "$output" | awk '{print $29}')
        centralSF9=$(echo "$output" | awk '{print $30}')
        centralSF10=$(echo "$output" | awk '{print $31}')
        centralSF11=$(echo "$output" | awk '{print $32}')
        centralSF12=$(echo "$output" | awk '{print $33}')
        centralSFoor=$(echo "$output" | awk '{print $34}')

        # echo "Got results"

        # Sum the results
        centraldevicessum=$(echo "$centraldevicessum + $centraldevices" | bc -l)
        centralreceivedsum=$(echo "$centralreceivedsum + $centralreceived" | bc -l)
        centralinterferedsum=$(echo "$centralinterferedsum + $centralinterfered" | bc -l)
        centralnomorerxsum=$(echo "$centralnomorerxsum + $centralnomorerx" | bc -l)
        centralundersenssum=$(echo "$centralundersenssum + $centralundersens" | bc -l)
        centraltotpacketsum=$(echo "$centraltotpacketsum + $centraltotpacket" | bc -l)

        centralS1sum=$(echo "$centralS1sum + $centralS1" | bc -l)
        centralS2sum=$(echo "$centralS2sum + $centralS2" | bc -l)
        centralS3sum=$(echo "$centralS3sum + $centralS3" | bc -l)
        centralS4sum=$(echo "$centralS4sum + $centralS4" | bc -l)
        centralS5sum=$(echo "$centralS5sum + $centralS5" | bc -l)
        centralS6sum=$(echo "$centralS6sum + $centralS6" | bc -l)
        centralS7sum=$(echo "$centralS7sum + $centralS7" | bc -l)
        centralS8sum=$(echo "$centralS8sum + $centralS8" | bc -l)

        centralF1sum=$(echo "$centralF1sum + $centralF1" | bc -l)
        centralF2sum=$(echo "$centralF2sum + $centralF2" | bc -l)
        centralF3sum=$(echo "$centralF3sum + $centralF3" | bc -l)
        centralF4sum=$(echo "$centralF4sum + $centralF4" | bc -l)
        centralF5sum=$(echo "$centralF5sum + $centralF5" | bc -l)
        centralF6sum=$(echo "$centralF6sum + $centralF6" | bc -l)
        centralF7sum=$(echo "$centralF7sum + $centralF7" | bc -l)
        centralF8sum=$(echo "$centralF8sum + $centralF8" | bc -l)

        centralavgdelaysum=$(echo "$centralavgdelaysum + $centralavgdelay" | bc -l)
        centralavgAckdelaysum=$(echo "$centralavgAckdelaysum + $centralavgAckdelay" | bc -l)
        centraltotRetxsum=$(echo "$centraltotRetxsum + $centraltotRetx" | bc -l)
        
        centralSF7sum=$(echo "$centralSF7sum + $centralSF7" | bc -l)
        centralSF8sum=$(echo "$centralSF8sum + $centralSF8" | bc -l)
        centralSF9sum=$(echo "$centralSF9sum + $centralSF9" | bc -l)
        centralSF10sum=$(echo "$centralSF10sum + $centralSF10" | bc -l)
        centralSF11sum=$(echo "$centralSF11sum + $centralSF11" | bc -l)
        centralSF12sum=$(echo "$centralSF12sum + $centralSF12" | bc -l)
        centralSFoorsum=$(echo "$centralSFoorsum + $centralSFoor" | bc -l)
        
        currentrun=$(( $currentrun+1 ))
        globalrun=$(( $globalrun+1 ))
    done
    
    # Average in runs
    #echo "Central averaged results centraldevicessum= $centraldevicessum , maxRuns= $maxRuns"
    echo -n " $(echo "$centraldevicessum/$maxRuns" | bc -l)"
    echo -n " $(echo "$appPeriodLoop" | bc -l)"

    echo -n " $(echo "$centraltotpacketsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralreceivedsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralinterferedsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralnomorerxsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralundersenssum/$maxRuns" | bc -l)"
       
    echo -n " $(echo "$centralS1sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralS2sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralS3sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralS4sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralS5sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralS6sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralS7sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralS8sum/$maxRuns" | bc -l)"

    echo -n " $(echo "$centralF1sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralF2sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralF3sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralF4sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralF5sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralF6sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralF7sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralF8sum/$maxRuns" | bc -l)"
    
    echo -n " $(echo "$centralavgdelaysum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralavgAckdelaysum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centraltotRetxsum/$maxRuns" | bc -l)"

    echo -n " $(echo "$centralSF7sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralSF8sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralSF9sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralSF10sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralSF11sum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralSF12sum/$maxRuns" | bc -l)"
    echo " $(echo "$centralSFoorsum/$maxRuns" | bc -l)"

    i=$(( $i+$increment ))
done
