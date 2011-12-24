#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	unsigned int c;
	sscanf("ff", "%x", &c);
	unsigned char p = c;
	printf("%d\n", p);
	printf("%d\n", c);
	printf("%x\n", c);
	return 0;
}
