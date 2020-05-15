

set xrange [0:240]
set yrange [0:120000]

plot 'xyzdata.txt' using 1:($2+20000) with lines, \
     '' using 1:($3+60000) with lines, \
     '' using 1:($4+100000) with lines


