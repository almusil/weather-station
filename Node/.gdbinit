target extended-remote /dev/ttyBmpGdb
monitor tpwr enable
monitor connect_srst enable
monitor swdp_scan
attach 1
load
compare-sections
kill
quit
