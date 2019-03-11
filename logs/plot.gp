#!/usr/bin/gnuplot
#
# Grafico Packet Delivery Rate in GPSR
#
# AUTHOR: Andre Camoes

set yrange [0:100]

set terminal png size 640,480 enhanced font 'Gill Sans,10' 

set output "PacketDeliveryRate-GPSR.png"

set title "Packet Delivery in GPSR Scenario"

set ylabel "Number of Packets"

set grid

set boxwidth 0.95 relative

set style fill transparent solid 0.5 noborder

plot 'monitori.dat' using ($0+1.5):($1) title "Packets Send" with boxes lc rgb"green", 'monitori.dat' using ($0+3.5):($2) title "Packets Receive" with boxes lc rgb"green"

exit

