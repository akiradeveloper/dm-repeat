#include <string.h>

int main(void)
{
//	char *s = "akira";
	char s[6];
	char c = 'z';
	memcpy(s, &c, 1);
	return 0;
}
