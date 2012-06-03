# dm-repeat
A Linux device-mapper module repeats configured sequence.

## Motivation
To test device-mapper modules.

## Usage
create a repeat device (size: 5120KB)  
```
dmsetup create repeat --table "0 10 dm-repeat"  
```

configure the repeat sequence with hex format (0xABC)  
```
dmsetup message repeat 0 seq.config 0xABC 
```

accepted formats are  
* HEX: 0x, 0X  
* OCT: 0o, 0O  
* BIN: 0b, 0B  

```
dmsetup message repeat 0 seq.show 
```

# Copyright
Copyright (c) 2011-2012 Akira Hayakawa (@akiradeveloper)  

