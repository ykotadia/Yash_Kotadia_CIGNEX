set xrange [0:10]
set yrange [0:100]
set title "CPU Utilization"
set key font ",20"
set title font ", 25"
plot for [col=2:3] "../plotEff" using 1:col smooth bezier title columnheader
pause 1
reread
