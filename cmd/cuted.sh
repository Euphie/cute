#!/bin/bash
# chkconfig: 2345 10 90 
# description: cute

   
PROG="cute"  
PROG_PATH="/bin"  
PROG_ARGS="-d -c /etc/cute.ini"   
PID_PATH="/var/run/"  
  
start() {  
    if [ -e "$PID_PATH/$PROG.pid" ]; then  
        echo "Error! $PROG is currently running!" 1>&2  
        exit 1  
    else  
        $PROG_PATH/$PROG $PROG_ARGS 2>&1 >/var/log/$PROG &  
 	pid=`ps -ef|grep $PROG|grep -v grep|awk '{print $2}'`
        echo "$PROG started"  
        echo $pid > "$PID_PATH/$PROG.pid"  
    fi  
}  
  
stop() {  
    echo "begin stop"  
    if [ -e "$PID_PATH/$PROG.pid" ]; then  
    	rm -f "$PID_PATH/$PROG.pid"
	pid=`ps -ef|grep $PROG|grep -v grep|awk '{print $2}'`
    	kill $pid
	echo "$PROG stopped"  
    else  
        echo "Error! $PROG not started!" 1>&2  
        exit 1  
    fi  
}  
  
if [ "$(id -u)" != "0" ]; then  
    echo "This script must be run as root" 1>&2  
    exit 1  
fi  
  
case "$1" in  
    start)  
        start  
        exit 0  
    ;;  
    stop)  
        stop  
        exit 0  
    ;;  
    reload|restart|force-reload)  
        stop  
        start  
        exit 0  
    ;;  
    **)  
        echo "Usage: $0 {start|stop|reload}" 1>&2  
        exit 1  
    ;;  
esac  
