/*
 * Copyright (C) 2003 Christophe Saout <christophe@saout.de>
 *
 * This file is released under the GPL.
 */

/*
 * Copyright (C) 2011 Akira Hayakawa <ruby.wktk@gmail.com>
 */

#include <string.h>

#include <linux/device-mapper.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>

#define DM_MSG_PREFIX "repeat"

static char *str_rep = "akiradeveloper"; /* default */
module_param(str_rep, charp, S_IRUGO);
MODULE_PARM_DESC(str_rep, "string to repeat");

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

void repeat_fill_mem(void *data, size_t len_data, char *s, size_t offset)
{
	int i;
	int sl = strlen(s);
	int step = sl - offset;
	for(i=0; i<len_data; ){
		memcpy(data+i, s+offset, step);
		i+=step;
		offset = 0;
		step = sl;
		if(unlikely(i+step >= len_data)){
			step = len_data - i;
		}
	}  
}

void repeat_fill_bio(struct bio *bio, char *s)
{
	unsigned long flags;
	struct bio_vec *bv;
	int i;

	int len = strlen(s);
	int offset = (bio->sector << 9) % len;

	bio_for_each_segment(bv, bio, i) {
		char *data = bvec_kmap_irq(bv, &flags);
		repeat_fill_mem(data, bv->bv_len, s, offset);
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
		repeat_fill_bio(bio, str_rep);
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
MODULE_DESCRIPTION(DM_NAME " dummy target repeating given string");
MODULE_LICENSE("GPL");
