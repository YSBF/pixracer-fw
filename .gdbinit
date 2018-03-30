set history save
set confirm off
set verbose off
set print pretty on
set print array off
set print array-indexes on

target remote localhost:3333
file build/ch.elf
