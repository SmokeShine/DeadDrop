#!/bin/bash
# FYI, this command removes file abc if it is empty: [ -s abc ] || rm -f abc

usage="usage: $0 port"

#use the standard version of echo
echo=/bin/echo

#Make sure we have the right number of arguments
if test $# -gt 1 -o $# -lt 1
then
	${echo} $usage 1>&2
	exit 1
fi

#Clean up any previous runs
${echo} '#Initializing - Cleaning up - ignore Operation Not Permitted errors'
killall -q -u $USER otp_*
rm -f plaintext*_*
rm -f key20
rm -f key70000

#Record the ports passed in
encport=$1

history=()
hist_count=0

#Run the daemons
#otp_d $encport >/dev/null &
coproc otp_fd { otp_d $encport; }
enc_id=$!
out=${otp_fd[0]}

sleep 5

${echo}
${echo} '#-----------------------------------------'
${echo} '#START OF GRADING SCRIPT'
${echo} '#keygen 20 > key20'
keygen 20 > key20
${echo} "===01=== #5 POINTS: key20 must exist"
[ -s key20 ] || rm -f key20 
if [ -f key20 ]; then ${echo} 'key20 exists!'; else ${echo} 'key20 DOES NOT EXIST'; fi 
${echo}
${echo} "#-----------------------------------------"
${echo} "===02=== #5 POINTS: Number of characters in key20, should be 21:"
wc -m key20
${echo}
${echo} "#-----------------------------------------"
${echo} '#keygen 70000 > key70000'
keygen 70000 > key70000
${echo} "===03=== #5 POINTS: Number of characters in key70000, should be 70001:"
[ -s key70000 ] || rm -f key70000 
wc -m key70000
${echo}
${echo} "#-----------------------------------------"
${echo} '#otp post TJ plaintext1 key20 $encport'
${echo} "===04=== #10 POINTS: Should return error about too-short key"
otp post TJ plaintext1 key20 $encport
${echo}
${echo} "#-----------------------------------------"
${echo} '#otp post TJ plaintext1 key70000 $encport'
${echo} "===05=== #20 POINTS: Should return encrypted version of plaintext1"
otp post TJ plaintext1 key70000 $encport
read line <&"${otp_fd[0]}"
history+=($line)
cat "$line"
${echo}
${echo} '#-----------------------------------------'
${echo} '#otp post TJ plaintext1 key70000 $encport'
otp post TJ plaintext1 key70000 $encport
read ciphertext1 <&"${otp_fd[0]}"
history+=($ciphertext1)
${echo} "===06=== #10 POINTS: ciphertext1 must exist"
[ -s $ciphertext1 ] || rm -f $ciphertext1 
if [ -f $ciphertext1 ]; then ${echo} 'ciphertext1 exists!'; else ${echo} 'ciphertext1 DOES NOT EXIST'; fi 
${echo}
${echo} '#-----------------------------------------'
${echo} '===07=== #10 POINTS: ciphertext1 must be same number of chars as source'
${echo} '#wc -m plaintext1'
wc -m plaintext1
${echo} '#Should be same: wc -m ciphertext1'
wc -m $ciphertext1
${echo}
${echo} '#-----------------------------------------'
${echo} '===08=== #5 POINTS: ciphertext1 should look encrypted'
cat $ciphertext1
${echo}
${echo} '#-----------------------------------------'
${echo} '#otp_get "Ben" ciphertext1 key70000 $encport'

${echo} '===10=== #20 POINTS: should return decrypted ciphertext1 that matches source'
${echo} '#cat plaintext1'
cat plaintext1
${echo} '#otp get TJ key70000 $encport'
otp get TJ key70000 $encport
${echo}
${echo} '#-----------------------------------------'
${echo} '#otp get TJ key70000 $encport > plaintext1_a'
echo $encport
otp get TJ key70000 $encport > plaintext1_a
