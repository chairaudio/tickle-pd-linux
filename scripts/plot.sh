#!/usr/bin/env gnuplot
set datafile commentschars ";"
set terminal svg size 1200,400
set output "out.svg"
plot "out.dat" every ::2000::3000 with lines
