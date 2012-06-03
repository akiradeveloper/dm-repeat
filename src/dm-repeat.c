/*
 * device-mapper module
 * "dm-repeat"
 * Copyright (C) 2011-2012 Akira Hayakawa <ruby.wktk@gmail.com>
 */

#include <linux/device-mapper.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/string.h>
#include <linux/lcm.h>

#define DM_MSG_PREFIX "repeat"

#define BITS_PER_BYTE 8

int start_with(char *s, char *prefix){
	int n = strlen(prefix);
	char buf[n+1];
	// copy the first few bytes of s to buf.
	strncpy(buf, s, n);
	int r = strcmp(prefix, buf); 
	return (r == 0);
}

enum prefix {
	PREFIX_HEX,
	PREFIX_OCT,
	PREFIX_BIN,
};
size_t bits_prefix(enum prefix prefix){
	if(prefix == PREFIX_HEX) { return 4; }
	if(prefix == PREFIX_OCT) { return 3; }
	if(prefix == PREFIX_BIN) { return 1; }
	return 0;
}

// if seq is 0xABCDE\0
struct orig_seq {
	enum prefix prefix; // PREFIX_HEX
	char *s; // ABCDE\0
};
void os_destroy(struct orig_seq *os){
	kfree(os->s);
	kfree(os);
}
size_t os_len(struct orig_seq *os){
	return strlen(os->s);
}
size_t os_unit(struct orig_seq *os){
	return bits_prefix(os->prefix);
}
size_t os_bits(struct orig_seq *os){
	return os_unit(os) * os_len(os);
}
	
struct orig_seq *decompose(char *s){
	struct orig_seq *os;
	os = kmalloc(sizeof(*os), GFP_KERNEL);

	// dropping first 2 characters (0x, 0X, ...)
	size_t n = strlen(s) - 2;
	char *ss = kmalloc(sizeof(char) * (n+1), GFP_KERNEL);
	strncpy(ss, s+2, n);
	os->s = ss;

	if( start_with(s, "0x") || start_with(s, "0X") ){
		os->prefix = PREFIX_HEX;
		return os;
	}
	if( start_with(s, "0o") || start_with(s, "0O") ){
		os->prefix = PREFIX_OCT;
		return os;
	}
	if( start_with(s, "0b") || start_with(s, "0B") ){
		os->prefix = PREFIX_BIN;
		return os;
	}

	return NULL;
} 

/*
 * align the seq to bytes
 * uses LCM to calc boundary.
 * e.g.
 * 0b: 10 (2 bits) -> 10101010 (1 byte)
 * 0x: F (4 bits) -> FF (1 byte)
 * 0O: 5 (3 bits) -> 55555555 (3 bytes)
 */
void os_align(struct orig_seq *os){
	size_t _lcm = lcm(os_bits(os), BITS_PER_BYTE);
	char *ss = kmalloc(sizeof(char) * (_lcm + 1), GFP_KERNEL);
	*(ss + _lcm) = '\0';
	size_t nr_repeat = _lcm / os_bits(os);
	size_t i;
	for(i=0; i < nr_repeat; i++){
		memcpy(ss + os_len(os) * i, os->s, os_len(os));
	}
	kfree(os->s);
	os->s = ss;
}

/* 
 * e.g.
 * (10, 1) -> 1
 * (10, 0) -> 0
 */
typedef unsigned int uint_t;
typedef unsigned char bit_t;
typedef unsigned char byte_t;
bit_t __get_bit(uint_t n, int bit){
	int b = n & (1 << bit);
	if(b == 0){
		return 0;
	}
	return 1;
}

char *to_format(enum prefix prefix){
	if(prefix == PREFIX_HEX) { return "%x"; }
	if(prefix == PREFIX_OCT) { return "%o"; }
	return NULL;
}

/*
 * e.g.
 * 0x: A -> 10
 */
uint_t __read(enum prefix prefix, char c){
	if(prefix == PREFIX_BIN){
		if(c == '0'){
			return 0;
		}
		return 1;
	}
	if(prefix == PREFIX_HEX || prefix == PREFIX_OCT){
		uint_t x;
		char buf[2];
		buf[1] = '\0';
		buf[0] = c;
		sscanf(buf, to_format(prefix), &x);
		return x;
	}
	return 16; // FIXME: ???
}

/*
 * e.g.
 * 0x: A -> [1, 0, 1, 0]
 * 0o: 5 -> [1, 0, 1]
 * 0b: 1 -> [1]
 */
bit_t *char_bit_array(enum prefix prefix, char c){
	size_t unit = bits_prefix(prefix);
	bit_t *array = kmalloc(sizeof(bit_t) * unit, GFP_KERNEL);
	
	uint_t x;
	x = __read(prefix, c);

	size_t i;
	for(i=0; i < unit; ++i){
		*(array + i) = __get_bit(x, i);
	}

	return array;
}

bit_t *os_bit_array(struct orig_seq *os){
	bit_t *array = kmalloc(sizeof(bit_t) * os_bits(os), GFP_KERNEL);
	size_t unit = bits_prefix(os->prefix);
	size_t cur = 0;
	size_t i;
	for(i=0; i < os_len(os); ++i){
		char c = *(os->s + i);
		bit_t *tmp = char_bit_array(os->prefix, c);
		memcpy(array + cur, tmp, unit);			
		cur += unit;
		kfree(tmp);
	}
	return array;
}

struct repeat_seq {
	size_t len;
	byte_t *data;
};
void rs_destroy(struct repeat_seq *rs){
	kfree(rs->data);
	kfree(rs);
}

byte_t bit_array_to_byte(bit_t *array){
	byte_t c = 0;
	size_t i;
	for(i=0; i < BITS_PER_BYTE; ++i){
		if(*(array + i) == 1){
			c |= (1 << i); // up bit at i 
		}
	}
	return c;
}

struct repeat_seq *parse_seq(char *s){
	struct orig_seq *os = decompose(s);
	os_align(os);

	struct repeat_seq *rs = kmalloc(sizeof(*rs), GFP_KERNEL);
	rs->len = os_bits(os) / BITS_PER_BYTE;
	rs->data = kmalloc(sizeof(byte_t) * rs->len, GFP_KERNEL);
	
	bit_t *array = os_bit_array(os);
	size_t i;
	bit_t *tmp = kmalloc(sizeof(bit_t) * BITS_PER_BYTE, GFP_KERNEL);
	for(i=0; i < rs->len; ++i){
		memcpy(tmp, array + BITS_PER_BYTE * i, BITS_PER_BYTE);
		byte_t b = bit_array_to_byte(tmp);
		*(rs->data + i) = b;
	}
	kfree(array);

	os_destroy(os);
	return rs;
}

/*
 * repeately copy src data to dest.
 * it starts from the offset position in src and fills the dest until the dest buffer will be fullfilled.
 */
void repeat_fill_mem(void *dest, size_t destlen, void *src, size_t srclen, size_t offset){
	int i;
	int step = srclen - offset;
	for(i=0; i<destlen; ){
		memcpy(dest+i, src+offset, step);
		i += step;
		offset = 0;
		if(unlikely(i+srclen >= destlen)){
			step = destlen - i;
		} else {
			step = srclen;
		}
	}  
}

void repeat_fill_bio(struct bio *bio, char *repseq){
	unsigned long flags;
	struct bio_vec *bv;
	int i;

	struct repeat_seq *rs = parse_seq(repseq);
	size_t offset = to_bytes(bio->bi_sector) % rs->len; // initial offset.

	bio_for_each_segment(bv, bio, i) {
		char *data = bvec_kmap_irq(bv, &flags);

		repeat_fill_mem(data, bv->bv_len, rs->data, rs->len, offset);

		flush_dcache_page(bv->bv_page);

		bvec_kunmap_irq(data, &flags);
		offset = (offset + bv->bv_len) % rs->len; // update offset.
	}
	rs_destroy(rs);
}

struct repeat_c {
	char *repseq;
};

static int repeat_map(struct dm_target *ti, struct bio *bio, union map_info *map_context){
	struct repeat_c *cc = ti->private;
	switch(bio_rw(bio)){
	case READ:
		repeat_fill_bio(bio, cc->repseq);
		break;
	case READA:
		return -EIO;
	case WRITE:
		break;
	}

	bio_endio(bio, 0);

	return DM_MAPIO_SUBMITTED;
}

static int repeat_message(struct dm_target *ti, unsigned argc, char **argv){
	struct repeat_c *cc = ti->private;
	char *cmdname = argv[0];

	if(! strcmp(cmdname, "seq.config")){
		// FIXME: new seq should be checked.
		char *seq = argv[1];
		kfree(cc->repseq); // free the current seq.
		cc->repseq = seq;
		printk("dm-repeat: repseq is now %s\n", cc->repseq);
		return 0;
	}

	if(! strcmp(cmdname, "seq.show")){
		printk("dm-repeat: repseq is: %s\n", cc->repseq);
		return 0;
	}
			
	return -EINVAL;
}

static int repeat_ctr(struct dm_target *ti, unsigned int argc, char **argv){
	if (argc != 0) {
		ti->error = "No arguments required";
		return -EINVAL;
	}

	struct repeat_c *cc;
	cc = kmalloc(sizeof(*cc), GFP_KERNEL);
	cc->repseq = "0xFF"; // default.

	ti->num_discard_requests = 1;

	ti->private = cc;

	return 0;
}

static void repeat_dtr(struct dm_target *ti){
	struct repeat_c *cc = ti->private;
	kfree(cc->repseq);
	kfree(cc);
}

static struct target_type repeat_target = {
	.name = "repeat",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.map = repeat_map,
	.message = repeat_message,
	.ctr = repeat_ctr,
	.dtr = repeat_dtr,
};

static int __init dm_repeat_init(void){
	int r = dm_register_target(&repeat_target);
	if (r < 0){
		DMERR("dm-repeat: register failed %d", r);
	}

	return r;
}

static void __exit dm_repeat_exit(void){
	dm_unregister_target(&repeat_target);
}

module_init(dm_repeat_init)
module_exit(dm_repeat_exit)

MODULE_AUTHOR("Akira Hayakawa");
MODULE_DESCRIPTION(DM_NAME " repeating sequence.");
MODULE_LICENSE("GPL");
