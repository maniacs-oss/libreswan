# bring up OE
ping -n -c 4 -I 192.1.3.209 192.1.2.23
ipsec whack --trafficstatus
ipsec stop
ip xfrm state
ipsec start
/testing/pluto/bin/wait-until-pluto-started
# give OE policies time to load
sleep 5
# bring up OE again
ping -n -c 4 -I 192.1.3.209 192.1.2.23
ipsec whack --trafficstatus
killall ip > /dev/null 2> /dev/null
cp /tmp/xfrm-monitor.out OUTPUT/road.xfrm-monitor.txt
echo done
