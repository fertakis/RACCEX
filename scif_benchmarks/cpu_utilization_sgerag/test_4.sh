#!/bin/bash 
# by Paul Colby (http://colby.id.au), no rights reserved ;) 

####cd ../icons 

####cp -ax terminal-black.png /tmp/deskiconsramdrive/terminal.png 

PREV_TOTAL=0 
PREV_IDLE=0 

while true; do 
  CPUALL=(`cat /proc/stat | grep '^cpu '`) # Get the total CPU statistics. 
  CPU0=(`cat /proc/stat | grep '^cpu0'`) # Get the total CPU statistics. 
  CPU1=(`cat /proc/stat | grep '^cpu1'`) # Get the total CPU statistics. 
  CPU2=(`cat /proc/stat | grep '^cpu2'`) # Get the total CPU statistics. 
  CPU3=(`cat /proc/stat | grep '^cpu3'`) # Get the total CPU statistics. 
  read
  CPUALL1=(`cat /proc/stat | grep '^cpu '`) # Get the total CPU statistics. 
  CPU01=(`cat /proc/stat | grep '^cpu0'`) # Get the total CPU statistics. 
  CPU11=(`cat /proc/stat | grep '^cpu1'`) # Get the total CPU statistics. 
  CPU21=(`cat /proc/stat | grep '^cpu2'`) # Get the total CPU statistics. 
  CPU31=(`cat /proc/stat | grep '^cpu3'`) # Get the total CPU statistics. 
  unset CPUALL[0]                          # Discard the "cpu" prefix. 
  unset CPUALL1[0]                          # Discard the "cpu" prefix. 
  unset CPU0[0]                          # Discard the "cpu" prefix. 
  unset CPU01[0]                          # Discard the "cpu" prefix. 
  unset CPU1[0]                          # Discard the "cpu" prefix. 
  unset CPU11[0]                          # Discard the "cpu" prefix. 
  unset CPU2[0]                          # Discard the "cpu" prefix. 
  unset CPU21[0]                          # Discard the "cpu" prefix. 
  unset CPU3[0]                          # Discard the "cpu" prefix. 
  unset CPU31[0]                          # Discard the "cpu" prefix. 
  IDLEALL=${CPUALL[4]}                        # Get the idle CPU time. 
  IDLE0=${CPU0[4]}                        # Get the idle CPU time. 
  IDLE1=${CPU1[4]}                        # Get the idle CPU time. 
  IDLE2=${CPU2[4]}                        # Get the idle CPU time. 
  IDLE3=${CPU3[4]}                        # Get the idle CPU time. 
  IDLEALL1=${CPUALL1[4]}                        # Get the idle CPU time. 
  IDLE01=${CPU01[4]}                        # Get the idle CPU time. 
  IDLE11=${CPU11[4]}                        # Get the idle CPU time. 
  IDLE21=${CPU21[4]}                        # Get the idle CPU time. 
  IDLE31=${CPU31[4]}                        # Get the idle CPU time. 

  for x in 1 2 3 5 6 7 8 9 4
	  do
		  DIFFALL=`echo "${CPUALL1[$x]} - ${CPUALL[$x]}" | bc -l`;
	  	  echo -n "$DIFFALL "
	  done
	  echo " "
  for x in 1 2 3 5 6 7 8 9 4
	  do
		  DIFF0=`echo "${CPU01[$x]} - ${CPU0[$x]}" | bc -l`;
	  	  echo -n "$DIFF0 "
	  done
	  echo " "
  for x in 1 2 3 5 6 7 8 9 4
	  do
		  DIFF1=`echo "${CPU11[$x]} - ${CPU1[$x]}" | bc -l`
	  	  echo -n "$DIFF1 "
	  done		  
	  echo " "
  for x in 1 2 3 5 6 7 8 9 4
	  do
		  DIFF2=`echo "${CPU21[$x]} - ${CPU2[$x]}" | bc -l`
	  	  echo -n "$DIFF2 "
	  done		  
	  echo " "
  for x in 1 2 3 5 6 7 8 9 4
	  do
		  DIFF3=`echo "${CPU31[$x]} - ${CPU3[$x]}" | bc -l`
	  	  echo -n "$DIFF3 "
	  done		  
	  echo " "

  # Calculate the total CPU time. 
  TOTAL=0 
  TOTAL1=0 
  for VALUE in "${CPUALL[@]}"; do 
    let "TOTAL=$TOTAL+$VALUE" 
  done 

  for VALUE in "${CPUALL1[@]}"; do 
    let "TOTAL1=$TOTAL1+$VALUE" 
  done 

  # Calculate the CPU usage since we last checked. 
  let "DIFF_IDLE=$IDLEALL1-$IDLEALL" 
  let "DIFF_TOTAL=$TOTAL1-$TOTAL" 
  let "DIFF_USAGE=(1000*($DIFF_TOTAL-$DIFF_IDLE)/$DIFF_TOTAL+5)/10" 

  echo "CPU: $DIFF_USAGE"  

  TOTAL=0 
  TOTAL1=0 
  for VALUE in "${CPU0[@]}"; do 
    let "TOTAL=$TOTAL+$VALUE" 
  done 

  for VALUE in "${CPU01[@]}"; do 
    let "TOTAL1=$TOTAL1+$VALUE" 
  done 

  # Calculate the CPU usage since we last checked. 
  let "DIFF_IDLE=$IDLE01-$IDLE0" 
  let "DIFF_TOTAL=$TOTAL1-$TOTAL" 
  let "DIFF_USAGE=(1000*($DIFF_TOTAL-$DIFF_IDLE)/$DIFF_TOTAL+5)/10" 

  echo "CPU: $DIFF_USAGE"  
TOTAL=0 
TOTAL1=0 

  for VALUE in "${CPU1[@]}"; do 
    let "TOTAL=$TOTAL+$VALUE" 
  done 

  for VALUE in "${CPU11[@]}"; do 
    let "TOTAL1=$TOTAL1+$VALUE" 
  done 

  # Calculate the CPU usage since we last checked. 
  let "DIFF_IDLE=$IDLE11-$IDLE1" 
  let "DIFF_TOTAL=$TOTAL1-$TOTAL" 
  let "DIFF_USAGE=(1000*($DIFF_TOTAL-$DIFF_IDLE)/$DIFF_TOTAL+5)/10" 

  echo "CPU: $DIFF_USAGE"  
TOTAL=0 
TOTAL1=0 

  for VALUE in "${CPU2[@]}"; do 
    let "TOTAL=$TOTAL+$VALUE" 
  done 

  for VALUE in "${CPU21[@]}"; do 
    let "TOTAL1=$TOTAL1+$VALUE" 
  done 

  # Calculate the CPU usage since we last checked. 
  let "DIFF_IDLE=$IDLE21-$IDLE2" 
  let "DIFF_TOTAL=$TOTAL1-$TOTAL" 
  let "DIFF_USAGE=(1000*($DIFF_TOTAL-$DIFF_IDLE)/$DIFF_TOTAL+5)/10" 

  echo "CPU: $DIFF_USAGE"  

TOTAL=0 
TOTAL1=0 

  for VALUE in "${CPU3[@]}"; do 
    let "TOTAL=$TOTAL+$VALUE" 
  done 

  for VALUE in "${CPU31[@]}"; do 
    let "TOTAL1=$TOTAL1+$VALUE" 
  done 

  # Calculate the CPU usage since we last checked. 
  let "DIFF_IDLE=$IDLE31-$IDLE3" 
  let "DIFF_TOTAL=$TOTAL1-$TOTAL" 
  let "DIFF_USAGE=(1000*($DIFF_TOTAL-$DIFF_IDLE)/$DIFF_TOTAL+5)/10" 

  echo "CPU: $DIFF_USAGE"  


done
