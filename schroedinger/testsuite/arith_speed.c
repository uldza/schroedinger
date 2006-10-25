
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <schroedinger/schro.h>
#include <schroedinger/schroarith.h>
#include <schroedinger/schrotables.h>

#include <liboil/liboilprofile.h>
#include <liboil/liboilrandom.h>

#define BUFFER_SIZE 1000000

int debug=1;

static void init_2(SchroArith *arith);
static void test_arith_reload_nextcode (SchroArith *arith);
static int test_arith_context_decode_bit (SchroArith *arith, int i);
static int test_arith_context_decode_bit_2 (SchroArith *arith, int i);

int
decode(SchroBuffer *buffer, int n, OilProfile *prof, int type)
{
  SchroArith *a;
  int i;
  int j;
  int x = 0;

  oil_profile_init (prof);
  for(j=0;j<10;j++){
    a = schro_arith_new();

    schro_arith_decode_init (a, buffer);
    schro_arith_init_contexts (a);

    switch (type) {
      case 0:
        oil_profile_start (prof);
        for(i=0;i<n;i++){
          x += test_arith_context_decode_bit (a, 0);
        }
        oil_profile_stop (prof);
        break;
      case 1:
        init_2(a);
        oil_profile_start (prof);
        for(i=0;i<n;i++){
          x += test_arith_context_decode_bit_2 (a, 0);
        }
        oil_profile_stop (prof);
        break;
    }

    schro_arith_free(a);
  }

  return x;
}

void
encode (SchroBuffer *buffer, int n, int freq)
{
  SchroArith *a;
  int i;
  int bit;

  a = schro_arith_new();

  schro_arith_encode_init (a, buffer);
  schro_arith_init_contexts (a);

  for(i=0;i<n;i++){
    bit = oil_rand_u8() < freq;
    schro_arith_context_encode_bit (a, 0, bit);
  }
  schro_arith_flush (a);

  schro_arith_free(a);
}

int
check (int n, int freq)
{
  SchroBuffer *buffer;
  OilProfile prof;
  double ave, std;
  int x;
  int y;

  buffer = schro_buffer_new_and_alloc (1000000);

  encode(buffer, n, freq);

  x = decode(buffer, n, &prof, 0);
  oil_profile_get_ave_std (&prof, &ave, &std);
  printf("ref  %d,%d: %g (%g) %d\n", n, freq, ave, std, x);

  y = decode(buffer, n, &prof, 1);
  oil_profile_get_ave_std (&prof, &ave, &std);
  printf("test %d,%d: %g (%g) %d\n", n, freq, ave, std, y);
  if (x != y) {
    printf("BROKEN\n");
  }

  return 0;
}

void *x;

int
main (int argc, char *argv[])
{
  int i;

  schro_init();

  /* force it to be non-inlined */
  x = &test_arith_context_decode_bit;
  x = &test_arith_context_decode_bit_2;

  for(i=100;i<=1000;i+=100) {
    check(i, 128);
  }
#if 0
  for(i=0;i<=256;i+=16) {
    check(100, i);
  }
#endif

  return 0;
}



static void
test_arith_reload_nextcode (SchroArith *arith)
{
  while(arith->nextbits <= 24) {
    if (arith->offset < arith->buffer->length) {
      arith->nextcode |=
        arith->buffer->data[arith->offset] << (24-arith->nextbits);
    } else {
      arith->nextcode |= 0xff << (24-arith->nextbits);
    }
    arith->nextbits+=8;
    arith->offset++;
  }
}


static int
test_arith_context_decode_bit (SchroArith *arith, int i)
{
  unsigned int count;
  unsigned int value;
  unsigned int range;
  unsigned int scaler;
  unsigned int weight;
  unsigned int probability0;
  unsigned int range_x_prob;

  weight = arith->contexts[i].count[0] + arith->contexts[i].count[1];
  scaler = arith->division_factor[weight];
  probability0 = arith->contexts[i].count[0] * scaler;
  count = arith->code - arith->range[0] + 1;
  range = arith->range[1] - arith->range[0] + 1;
  range_x_prob = (range * probability0) >> 16;
  value = (count > range_x_prob);

  arith->range[1 - value] = arith->range[0] + range_x_prob - 1 + value;
  arith->contexts[i].count[value]++;

  if (arith->contexts[i].count[0] + arith->contexts[i].count[1] > 255) {
    arith->contexts[i].count[0] >>= 1;
    arith->contexts[i].count[0]++;
    arith->contexts[i].count[1] >>= 1;
    arith->contexts[i].count[1]++;
  }

  do {
    if ((arith->range[1] & (1<<15)) == (arith->range[0] & (1<<15))) {
      /* do nothing */
    } else if ((arith->range[0] & (1<<14)) && !(arith->range[1] & (1<<14))) {
      arith->code ^= (1<<14);
      arith->range[0] ^= (1<<14);
      arith->range[1] ^= (1<<14);
    } else {
      break;
    }

    arith->range[0] <<= 1;
    arith->range[1] <<= 1;
    arith->range[1]++;

    arith->code <<= 1;
    arith->code |= (arith->nextcode >> 31);
    arith->nextcode <<= 1;
    arith->nextbits--;
    if (arith->nextbits == 0) {
      test_arith_reload_nextcode(arith);
    }
  } while (1);

  return value;
}


#if 0
static int
test_arith_context_decode_bit_2 (SchroArith *arith, int i)
{
  unsigned int count;
  unsigned int value;
  unsigned int range;
  unsigned int scaler;
  unsigned int weight;
  unsigned int probability0;
  unsigned int range_x_prob;

  __volatile__ __asm__ (
      //weight = arith->contexts[i].count[0] + arith->contexts[i].count[1];
      "  movl 0(%[context]), %%eax\n"
      "  addl 4(%[context]), %%eax\n"
      "  movl %%eax, %[weight]\n"

      //scaler = arith->division_factor[weight];
      "  movzwl 0(%[division_factor],%[weight],2), %[scaler]\n"

      //probability0 = arith->contexts[i].count[0] * scaler;
      "  movl %[scaler], %[probability0]\n"
      "  imul 0(%[context]), %[probability0]\n"

      //count = arith->code - arith->range[0] + 1;
      "  movw 0(%[arith]), %[count]\n"
      "  subw 2(%[arith]), %[count]\n"
      "  addiw 1, %[count]\n"

      //range = arith->range[1] - arith->range[0] + 1;
      "  movw 4(%[arith]), %[range]\n"
      "  subw 2(%[arith]), %[range]\n"
      "  addiw 1, %[range]\n" 

      //range_x_prob = (range * probability0) >> 16;
      "  movw %[range], %%eax\n"
      "  mulw %[probability0]\n" // subtle, (result>>16) ends up in %dx
      "  movw %%dx, %[range_x_prob]\n"

      // value = (count > range_x_prob);
      "  movzwl %[range_x_prob], %[value]\n"
      "  subl %[count], %[value]\n"
      "  shr $31, %[value]\n" // value contains 1 is range_x_prob-count < 0

      // arith->range[1 - value] = arith->range[0] + range_x_prob - 1 + value;
      "  movl 0(%[arith]), %%ecx\n"
      "  movl %[value], %%eax\n"
      "  xor $1, %%eax\n"

      //arith->contexts[i].count[value]++;
      "  addl $1, 0(%[context],%[value],4)\n"

      //if (arith->contexts[i].count[0] + arith->contexts[i].count[1] > 255) {
      //  arith->contexts[i].count[0] >>= 1;
      //  arith->contexts[i].count[0]++;
      //  arith->contexts[i].count[1] >>= 1;
      //  arith->contexts[i].count[1]++;
      //}
      "  movl 0(%[context]), %%eax\n"
      "  addl 4(%[context]), %%eax\n"
      "  shrl $8, %%eax\n"
      "  shrl %%eax, 0(%[context])\n"
      "  addl %%eax, 0(%[context])\n"
      "  shrl %%eax, 4(%[context])\n"
      "  addl %%eax, 4(%[context])\n"
      :
      :
      : "eax", "ecx"
      );

  do {
    if ((arith->range[1] & (1<<15)) == (arith->range[0] & (1<<15))) {
      /* do nothing */
    } else if ((arith->range[0] & (1<<14)) && !(arith->range[1] & (1<<14))) {
      arith->code ^= (1<<14);
      arith->range[0] ^= (1<<14);
      arith->range[1] ^= (1<<14);
    } else {
      break;
    }

    arith->range[0] <<= 1;
    arith->range[1] <<= 1;
    arith->range[1]++;

    arith->code <<= 1;
    arith->code |= (arith->nextcode >> 31);
    arith->nextcode <<= 1;
    arith->nextbits--;
    if (arith->nextbits == 0) {
      test_arith_reload_nextcode(arith);
    }
  } while (1);

  return value;
}
#endif

static inline void
calc_prob0 (SchroArith *arith, SchroArithContext *context)
{
#if 0
  unsigned int weight;
  unsigned int scaler;

  weight = context->count[0] + context->count[1];
  scaler = arith->division_factor[weight];
  arith->probability0 = context->count[0] * scaler;
#endif

#if 1
  __asm__ __volatile__ (
      "  movzwl 0(%0), %%eax\n"
      "  addw 2(%0), %%ax\n"
      "  movzwl 20(%1,%%eax,2), %%eax\n"
      "  movzwl 0(%0), %%ecx\n"
      "  imul %%ecx, %%eax\n"
      "  movl %%eax, 8(%1)\n"
      :
      : "r" (context), "r" (arith)
      : "ecx", "eax", "memory");
#endif
}

static inline void
calc_count_range (SchroArith *arith)
{
#if 0
  arith->count = arith->code - arith->range[0] + 1;
  arith->range_value = arith->range[1] - arith->range[0] + 1;
#endif
#if 1
  __asm__ __volatile__ (
      "  movzwl 2(%0), %%ecx\n"
      "  subl $1, %%ecx\n"
      "  movzwl 0(%0), %%eax\n"
      "  subl %%ecx, %%eax\n"
      "  movl %%eax, 12(%0)\n"
      "  movzwl 4(%0), %%eax\n"
      "  subl %%ecx, %%eax\n"
      "  movl %%eax, 16(%0)\n"
      :
      : "r" (arith)
      : "ecx", "eax", "memory");
#endif
}

static inline void
maybe_shift_context (SchroArithContext *context)
{
#if 0
  int shift;

  shift = (context->count[0] + context->count[1]) >> 8;
  context->count[0] >>= shift;
  context->count[0] += shift;
  context->count[1] >>= shift;
  context->count[1] += shift;
#endif
#if 1
  __asm__ __volatile__ (
      "  movw 0(%0), %%cx\n"
      "  addw 2(%0), %%cx\n"
      "  shrw $8, %%cx\n"
      "  shrw %%cl, 0(%0)\n"
      "  addw %%cx, 0(%0)\n"
      "  shrw %%cl, 2(%0)\n"
      "  addw %%cx, 2(%0)\n"
      :
      : "r" (context)
      : "memory", "ecx"
      );
#endif
}

static inline void
fixup_range (SchroArith *arith)
{
#if 0
  do {
    if ((arith->range[1] & (1<<15)) == (arith->range[0] & (1<<15))) {
      /* do nothing */
    } else if ((arith->range[0] & (1<<14)) && !(arith->range[1] & (1<<14))) {
      arith->code ^= (1<<14);
      arith->range[0] ^= (1<<14);
      arith->range[1] ^= (1<<14);
    } else {
      break;
    }

    arith->range[0] <<= 1;
    arith->range[1] <<= 1;
    arith->range[1]++;

    arith->code <<= 1;
    arith->code |= (arith->nextcode >> 31);
    arith->nextcode <<= 1;
    arith->nextbits--;
    if (arith->nextbits == 0) {
      test_arith_reload_nextcode(arith);
    }
  } while (1);
#endif
#if 0
  int n = 0;

  while (((arith->range[0] ^ arith->range[1]) & (1<<15)) == 0) {
    arith->range[0] <<= 1;
    arith->range[1] <<= 1;
    arith->range[1]++;
    n++;
  }
  while (((arith->range[0] & (~arith->range[1]))) & (1<<14)) {
    arith->code ^= (1<<(14-n));
    arith->range[0] ^= (1<<14);
    arith->range[1] ^= (1<<14);

    arith->range[0] <<= 1;
    arith->range[1] <<= 1;
    arith->range[1]++;
    n++;
  }

  while(n) {
    arith->code <<= 1;
    arith->code |= (arith->nextcode >> 31);
    arith->nextcode <<= 1;
    arith->nextbits--;
    if (arith->nextbits == 0) {
      test_arith_reload_nextcode(arith);
    }
    n--;
  }
#endif
#if 1
  int i;
  int n;
  int flip;

  i = ((arith->range[1]&0xf000)>>8) | ((arith->range[0]&0xf000)>>12);

  n = arith->fixup_shift[i] & 0xf;
  if (n == 0) return;

  flip = arith->fixup_shift[i] & 0x8000;

  arith->range[0] <<= n;
  arith->range[1] <<= n;
  arith->range[1] |= (1<<n)-1;

  arith->code <<= n;
  arith->code |= (arith->nextcode >> ((32-n)&0x1f));
  arith->nextcode <<= n;
  arith->nextbits-=n;

  arith->code ^= flip;
  arith->range[0] ^= flip;
  arith->range[1] ^= flip;

  while (n>=3 ) {
    i = ((arith->range[1]&0xf000)>>8) | ((arith->range[0]&0xf000)>>12);

    n = arith->fixup_shift[i] & 0xf;
    if (n == 0) break;
  
    flip = arith->fixup_shift[i] & 0x8000;

    arith->range[0] <<= n;
    arith->range[1] <<= n;
    arith->range[1] |= (1<<n)-1;

    arith->code <<= n;
    arith->code |= (arith->nextcode >> ((32-n)&0x1f));
    arith->nextcode <<= n;
    arith->nextbits-=n;

    arith->code ^= flip;
    arith->range[0] ^= flip;
    arith->range[1] ^= flip;

  }
  if (arith->nextbits <= 16) {
    test_arith_reload_nextcode(arith);
  }
#endif
}

void
calc_value (SchroArith *arith, SchroArithContext *context)
{
#if 1
  unsigned int value;
  unsigned int range_x_prob;

  range_x_prob = (arith->range_value * arith->probability0) >> 16;
  value = (arith->count > range_x_prob);

  arith->range[1 - value] = arith->range[0] + range_x_prob - 1 + value;
  context->count[value]++;

  arith->value = value;
#endif
#if 0
  /* segfaults */
  __asm__ __volatile__ (
      "  movl 16(%0), %%eax\n"
      "  imul 8(%0), %%eax\n"
      "  shrl $16, %%eax\n"
      "  movl 12(%0), %%ecx\n"
      "  subl %%eax, %%ecx\n"
      "  shrl $31, %%ecx\n"
      "  movw %%cx, 6(%0)\n"
      "  addw $1, (%1,%%ecx,2)\n"
      "  xor $1, %%ecx\n"
      "  subl %%ecx, %%eax\n"
      "  addw 2(%0), %%ax\n"
      "  movw %%ax, 2(%0,%%ecx,2)\n"
      :
      : "r" (arith), "r" (context)
      : "memory", "ecx"
      );
#endif
}

static void
init_2(SchroArith *arith)
{
  calc_count_range(arith);
}

#if 0
static int
test_arith_context_decode_bit_2 (SchroArith *arith, int i)
{
  SchroArithContext *context = arith->contexts + i;

  calc_prob0(arith, context);

  calc_value(arith, context);

  maybe_shift_context(context);

  fixup_range(arith);

  calc_count_range(arith);

  return arith->value;
}
#endif

#if 1
static int
test_arith_context_decode_bit_2 (SchroArith *arith, int i)
{
  SchroArithContext *context = arith->contexts + i;

  //calc_prob0(arith, context);
  __asm__ __volatile__ (
      "  movzwl 0(%0), %%eax\n"
      "  addw 2(%0), %%ax\n"
      "  movzwl 20(%1,%%eax,2), %%eax\n"
      "  movzwl 0(%0), %%ecx\n"
      "  imul %%ecx, %%eax\n"
      "  movl %%eax, 8(%1)\n"
      :
      : "r" (context), "r" (arith)
      : "ecx", "eax", "memory");

#if 1
  {
    unsigned int value;
    unsigned int range_x_prob;

    range_x_prob = (arith->range_value * arith->probability0) >> 16;
    value = (arith->count > range_x_prob);

    arith->range[1 - value] = arith->range[0] + range_x_prob - 1 + value;
    context->count[value]++;

    arith->value = value;
  }
#else
  //calc_value(arith, context);
  __asm__ __volatile__ (
      "  movl 16(%0), %%eax\n"
      "  imul 8(%0), %%eax\n"
      "  shrl $16, %%eax\n"
      "  movl 12(%0), %%ecx\n"
      "  subl %%eax, %%ecx\n"
      "  shrl $31, %%ecx\n"
      "  movw %%cx, 6(%0)\n"
      "  addw $1, (%1,%%ecx,2)\n"
      "  xor $1, %%ecx\n"
      "  subl %%ecx, %%eax\n"
      "  addw 2(%0), %%ax\n"
      "  movw %%ax, 2(%0,%%ecx,2)\n"
      :
      : "r" (arith), "r" (context)
      : "memory", "ecx"
      );
#endif

  //maybe_shift_context(context);
  __asm__ __volatile__ (
      "  movw 0(%0), %%cx\n"
      "  addw 2(%0), %%cx\n"
      "  shrw $8, %%cx\n"
      "  shrw %%cl, 0(%0)\n"
      "  addw %%cx, 0(%0)\n"
      "  shrw %%cl, 2(%0)\n"
      "  addw %%cx, 2(%0)\n"
      :
      : "r" (context)
      : "memory", "ecx"
      );

  fixup_range(arith);

  //calc_count_range(arith);
  __asm__ __volatile__ (
      "  movzwl 2(%0), %%ecx\n"
      "  subl $1, %%ecx\n"
      "  movzwl 0(%0), %%eax\n"
      "  subl %%ecx, %%eax\n"
      "  movl %%eax, 12(%0)\n"
      "  movzwl 4(%0), %%eax\n"
      "  subl %%ecx, %%eax\n"
      "  movl %%eax, 16(%0)\n"
      :
      : "r" (arith)
      : "ecx", "eax", "memory");

  return arith->value;
}
#endif

