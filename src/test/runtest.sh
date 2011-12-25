insmod ../dm-repeat.ko && 
dmsetup create repeatdev --table "0 5 repeat" &&
hexdump -C /dev/mapper/repeatdev && 
echo 0x19840529 > /sys/module/dm_repeat/parameters/repseq && 
hexdump -C /dev/mapper/repeatdev && 
hexdump -C -s 1 /dev/mapper/repeatdev &&
dmsetup remove /dev/mapper/repeatdev &&
rmmod dm_repeat
