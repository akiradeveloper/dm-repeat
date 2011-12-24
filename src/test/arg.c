/* make sure the algorithm works in userland */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dump(void *p, size_t n)
{
	unsigned char c;	
	int i;
	for(i=0; i<n; ++i){
		memcpy(&c, p+i, 1);
		unsigned int d = c;
		printf("%x ", d);
	}
	printf("\n");
}

struct byteseq {
	size_t size;
	void *src;
};

void free_byteseq(struct byteseq *p)
{
	free(p->src);
	free(p);
}

int startw(char *s, char *prefix)
{
	int n = strlen(prefix);
	char buf[n+1];
	buf[n] = '\0';
	memcpy(buf, s, n);
	int r = strcmp(prefix, buf);
	return r == 0;
}

struct byteseq *parse_arg(char *s)
{
	struct byteseq *result = malloc(sizeof(*result));
	size_t sz = (strlen(s) - 2) / 2;
	void *src = malloc(sz);
	result->src = src;
	result->size = sz;
	int r = startw(s, "0x");
	if( !r ){
		printf("given arg not start with 0x\n");
		exit(1);
	}
	char *buf = "xx"; 
	int i, j;
	unsigned int dx;
	char cx;
	for(i=0; i<sz; ++i){
		j = i*2 + 2;	
		memcpy(buf, s+j, 2);
		sscanf(buf, "%x", &dx);
		cx = dx;
		memcpy(src+i, &cx, 1);			
	}		
	return result;
}

void repeat_fill_mem(void *dest, size_t destlen, void *src, size_t srclen, size_t offset)
{
	int i;
	int step = srclen - offset;
	for(i=0; i<destlen; ){
		memcpy(dest+i, src+offset, step);
		i += step;
		offset = 0;
		if(i+srclen >= destlen){
			step = destlen - i;
		} else {
			step = srclen;
		}
	}  
}

int main(int argc, char **argv)
{
	struct byteseq *result = parse_arg(argv[1]);
	char dest[5];
	repeat_fill_mem(dest, 5, result->src, result->size, 1);
	dump(dest, 5);
	free_byteseq(result);
	return 0;
}
