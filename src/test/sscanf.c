#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	unsigned int c;
	sscanf("fffffff", "%x", &c);
	unsigned char p = c;
	printf("%d\n", c);
	printf("%d\n", p);
	printf("%x\n", c);
	return 0;
}
