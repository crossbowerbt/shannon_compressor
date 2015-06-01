/*
  Implementation of the shannon compressor.
 	
  Emanuele Acri - crossbower@gmail.com - 2015
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

/*
  Global running options
*/
struct {
  uint16_t bits;    /* size of the data chunks */
  char *in_fname;   /* input file name */
  char *out_fname;  /* output file name */
} opts;

/*
  Descriptor of a chunk of data
*/
typedef struct {
  uint16_t bits;    /* size of the data in bits */
  uint32_t count;   /* count of the occurrences in file */
  uint8_t *p;       /* pointer to the data */
} chunk_t;

/*
  Global chunk table
*/
struct {
  uint32_t count;   /* number of different chunks */
  chunk_t *chunks;  /* chunk array */
} table;

/*
  Read 8 bits from a file (at a given bit offset).

  Returns the number of bits read.
*/
uint8_t
read_8_bits (FILE *fp, uint8_t *out, uint64_t offset)
{
  uint8_t byte;

  if(fseek(fp, offset / 8, SEEK_SET) != 0)
    return 0;

  if(fread(&byte, 1, 1, fp) < 1)
    return 0;  

  *out = byte << (offset % 8);

  if(fread(&byte, 1, 1, fp) < 1)
    return 8 - (offset % 8);  

  *out |= byte >> (8 - (offset % 8));

  return 8;
}

/*
  Write 1 to 8 bits to a file (at a given bit offset).

  Note: maintain an internal variable containing trailing
  bits to write to the next byte, for non byte-aligned
  writes.

  Note: To flush the trailing bits call the function
  with count set to zero.

  Returns the number of bits written.

  TODO: carefully check this...
*/
uint8_t
write_8_bits (FILE *fp, uint8_t *in, uint8_t count)
{
  static uint8_t saved_byte = 0, trailing_bits = 0;

  assert(count <= 8);

  if(count == 0 && trailing_bits > 0) {
    
    if(fwrite(&saved_byte, 1, 1, fp) < 1)
      return 0;

    saved_byte = 0;
    trailing_bits = 0;

    return 8;
  }

  else if(count + trailing_bits < 8) {
    saved_byte |= (*in >> trailing_bits);
    saved_byte &= 0xff << (8 - trailing_bits - count);
    trailing_bits += count;

    return 0;
  }

  else if(count + trailing_bits >= 8) {
    uint8_t byte;

    byte = saved_byte | *in >> trailing_bits;
    
    if(fwrite(&byte, 1, 1, fp) < 1)
      return 0;
    
    saved_byte  = *in  << (8 - trailing_bits);
    saved_byte &= 0xff << ((8 - count) + (8 - trailing_bits));
    trailing_bits = count - (8 - trailing_bits);

    return 8;
  }

  return 0;
}

/*
  Read a chunk from a file.

  Note: maintains an internal bit offset
  to read non-byte aligned chunks.

  Returns the number of bits read.
*/
uint16_t
read_chunk (FILE *fp, uint8_t *dst, uint16_t bits)
{
  uint16_t bits_read = 0;
  uint8_t r;

  static uint64_t offset = 0;

  if(bits < 1)
    return 0;

  while(bits > 0) {
    r = read_8_bits(fp, dst, offset);
    
    bits_read += bits < r ? bits : r;
    offset    += bits < r ? bits : r;
    dst++;

    bits -= bits < r ? bits : r;

    if(r < 8) break;
  }

  return bits_read;
}

/*
  Write a chunk to a file.

  Note: see the note of write_8_bits,
  since it is a dependency of this function.

  Returns the number of bits written.
*/
uint16_t
write_chunk (FILE *fp, uint8_t *src, uint16_t bits)
{
  uint16_t bits_written = 0;

  if(bits < 1)
    return 0;

  while(bits > 0) {
    write_8_bits(fp, src, bits > 8 ? 8 : bits);
    
    /* TODO: check errors */

    bits_written += bits < 8 ? bits : 8;
    src++;

    bits -= bits < 8 ? bits : 8;
  }

  return bits_written;
}

void
usage (char *progname)
{
  printf("Usage: %s bits in_file out_file\n", progname);
}

int
main (int argc, char *argv[])
{
  
  if(argc < 4) {
    usage(argv[0]);
    return 1;
  }

  opts.bits = atoi(argv[1]);
  opts.in_fname = argv[2];
  opts.out_fname = argv[3];

  return 0;
}
