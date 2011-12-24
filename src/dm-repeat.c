/*
 * Copyright (C) 2003 Christophe Saout <christophe@saout.de>
 *
 * This file is released under the GPL.
 */

/*
 * Copyright (C) 2011 Akira Hayakawa <ruby.wktk@gmail.com>
 */


#include <linux/device-mapper.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/string.h>

#define DM_MSG_PREFIX "repeat"

static char *repseq = "0xff"; /* default */
module_param(repseq, charp, S_IRUGO);
MODULE_PARM_DESC(repseq, "byte sequence to repeat");

struct byteseq {
	size_t size;
	void *src;
};

void free_byteseq(struct byteseq *p)
{
	kfree(p->src);
	kfree(p);
}

int startw(char *s, char *prefix)
{
	int n = strlen(prefix);
	char buf[n+1]; buf[n] = '\0';
	memcpy(buf, s, n);
	int r = strcmp(prefix, buf);
	return r == 0;
}

struct byteseq *parse_arg(char *s)
{
	struct byteseq *result = kmalloc(sizeof(*result), GFP_KERNEL);
	size_t sz = (strlen(s) - 2) / 2;
	void *src = kmalloc(sz, GFP_KERNEL);
	result->src = src;
	result->size = sz;

	int r = startw(s, "0x"); BUG_ON( !r );

	char buf[3]; buf[2] = '\0';
	int i, j;
	unsigned int dx;
	char cx;
	for(i=0; i<sz; ++i){
		j = i*2 + 2; // skip 0x prefix and get each 2 cs.
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

void repeat_fill_bio(struct bio *bio, char *s)
{
	unsigned long flags;
	struct bio_vec *bv;
	int i;

	bio_for_each_segment(bv, bio, i) {
		char *data = bvec_kmap_irq(bv, &flags);

		struct byteseq *result = parse_arg(repseq);
		int offset = 0;
		repeat_fill_mem(data, bv->bv_len, result->src, result->size, offset);
		free_byteseq(result);

		flush_dcache_page(bv->bv_page);
		bvec_kunmap_irq(data, &flags);
		offset = (offset + bv->bv_len) % len;
	}
}

static int repeat_map(struct dm_target *ti, struct bio *bio,
		      union map_info *map_context)
{
	switch(bio_rw(bio)) {
	case READ:
		repeat_fill_bio(bio, repseq);
		break;
	case READA:
		/* readahead of null bytes only wastes buffer cache */
		return -EIO;
	case WRITE:
		/* writes get silently dropped */
		break;
	}

	bio_endio(bio, 0);

	/* accepted bio, don't make new request */
	return DM_MAPIO_SUBMITTED;
}

static int repeat_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	if (argc != 0) {
		ti->error = "No arguments required";
		return -EINVAL;
	}

	/*
	 * Silently drop discards, avoiding -EOPNOTSUPP.
	 */
	ti->num_discard_requests = 1;

	return 0;
}

static struct target_type repeat_target = {
	.name   = "repeat",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.ctr    = repeat_ctr,
	.map    = repeat_map,
};

static int __init dm_repeat_init(void)
{
	int r = dm_register_target(&repeat_target);

	if (r < 0)
		DMERR("register failed %d", r);

	return r;
}

static void __exit dm_repeat_exit(void)
{
	dm_unregister_target(&repeat_target);
}

module_init(dm_repeat_init)
module_exit(dm_repeat_exit)

MODULE_AUTHOR("Akira Hayakawa");
MODULE_DESCRIPTION(DM_NAME " dummy target repeating given byte sequence");
MODULE_LICENSE("GPL");
