set terminal png
set output 'timing.plot.png'
set xlabel 'Matrix size'
set ylabel 'Time(seconds)'
plot 'timing_results.txt' using 1:2 with lines title 'Parallel', '' using 1:3 with lines title 'Sequential'
