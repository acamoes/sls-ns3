#!/usr/bin/gnuplot
#
# Grafico Distribuicao Data e Overhead para diferentes cenarios
#
# AUTHOR: Andre Camoes

#plot 'monitori.dat' using ($0):($1) title "Packets Send" with boxes lc rgb "green", 'monitori.dat' using ($0):($2) title "Packets Receive" with boxes lc rgb"green"

set terminal png size 640,480 enhanced font 'Helvetica,10' 
set output "DatavsOverhead.png"

set yrange [0:2500]
set xrange [0:10]

set ytics (" " 1000)

set style data boxes
set boxwidth 1.2
set style fill   solid 1.00 border -1


set xtics 20 20

set title "Data Tx and Overhead associated in AODV and GPSR" font "Bold,14"
set ylabel "Percentagem de pacotes por Trafego"

set xlabel "Captura de Sabado 17/04/10 10:00h" font "Italic,10"
set grid

set label "0.31%" at 1.25, 4
set label "99.6%" at 3.30, 103

set label "0.31%" at 1.25, 4

plot 'monitori.dat' using ($0+1.5):($2) title "Trafego AFS", 'monitori.dat' using ($0+3.5):($3) title "Restante Trafego",\
 'monitori.dat' using ($0+5):($4) title "Restante Trafego",\
 'monitori.dat' using ($0+6.5):($5) title "Restante Trafego"

