#!/usr/bin/env bash

# Useful variables
# i= number of nDevices
# increment= increment in EDs
gatewayRings=1
i=1
increment=50
maxEndDevices=2501 #2501
radius=7500
#gwrad=$(echo "10000/(2*($gatewayRings - 1)+1)" | bc -l)
simTime=86400 # simTime= appPeriod
appPeriod=86400
maxRuns=5
globalrun=1


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
    centralreceivedsum=0
    centralinterferedsum=0
    centralnomorerxsum=0
    centralundersenssum=0
    centraltotpacketsum=0

    while [ $currentrun -le $maxRuns ]
    do
        # nGateways=$[3*$gatewayRings*$gatewayRings-$[3*$gatewayRings]+1]
        # echo -n "Simulating a system with $i end devices and a transmission period of $simTime seconds...  "
        # START=$(date +%s)
        output="$(./waf --run "networkPerf2
            --nDevices=$i
            --gatewayRings=$gatewayRings 
            --radius=$radius
            --gatewayRadius=1500
            --simulationTime=$simTime
            --appPeriod=$appPeriod 
            --RngRun=$globalrun" | grep -v "build" | tr -d '\n')"
        # echo "$output"
        centraldevices=$(echo "$output" | awk '{print $1}')             # nDevices
        centraltotpacket=$(echo "$output" | awk '{print $2}')             # nDevices
        centralreceived=$(echo "$output" | awk '{print $3}')            # received packets
        centralinterfered=$(echo "$output" | awk '{print $4}')          # interfered packets
        centralnomorerx=$(echo "$output" | awk '{print $5}')            # packets discarded because no more receivers available
        centralundersens=$(echo "$output" | awk '{print $6}')           # packets discarded because under sensitivity

        # Sum the results
        centraldevicessum=$(echo "$centraldevicessum + $centraldevices" | bc -l)
        centraltotpacketsum=$(echo "$centraltotpacketsum + $centraltotpacket" | bc -l)
        centralreceivedsum=$(echo "$centralreceivedsum + $centralreceived" | bc -l)
        centralinterferedsum=$(echo "$centralinterferedsum + $centralinterfered" | bc -l)
        centralnomorerxsum=$(echo "$centralnomorerxsum + $centralnomorerx" | bc -l)
        centralundersenssum=$(echo "$centralundersenssum + $centralundersens" | bc -l)

        currentrun=$(( $currentrun+1 ))
        globalrun=$(( $globalrun+1 ))
    done
    
    # Average in runs
    #echo "Central averaged results centraldevicessum= $centraldevicessum , maxRuns= $maxRuns"
    echo -n " $(echo "$centraldevicessum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centraltotpacketsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralreceivedsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralinterferedsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralnomorerxsum/$maxRuns" | bc -l)"
    echo " $(echo "$centralundersenssum/$maxRuns" | bc -l)"


    i=$(( $i+$increment ))
done