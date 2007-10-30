
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <liboil/liboil.h>

#include <schroedinger/schrobits.h>
#include <schroedinger/schro.h>


SchroBits *
schro_bits_new (void)
{
  SchroBits *bits;
  
  bits = malloc (sizeof(*bits));
  memset (bits, 0, sizeof(*bits));

  return bits;
}

void
schro_bits_free (SchroBits *bits)
{
  free(bits);
}

void
schro_bits_copy (SchroBits *dest, SchroBits *src)
{
  memcpy (dest, src, sizeof(SchroBits));
}

static void
schro_bits_shift_out (SchroBits *bits)
{
  if (bits->n < bits->buffer->length) {
    bits->buffer->data[bits->n] = bits->value;
    bits->n++;
    bits->shift = 7;
    bits->value = 0;
    return;
  }
  if (bits->error == FALSE) {
    SCHRO_ERROR("buffer overrun");
  }
  bits->error = TRUE;
  bits->shift = 7;
  bits->value = 0;
}

void
schro_bits_encode_init (SchroBits *bits, SchroBuffer *buffer)
{
  bits->buffer = buffer;
  bits->n = 0;

  bits->value = 0;
  bits->shift = 7;
}

int
schro_bits_get_offset (SchroBits *bits)
{
  return bits->n;
}

int
schro_bits_get_bit_offset (SchroBits *bits)
{
  return bits->n*8 + (7 - bits->shift);
}

void
schro_bits_flush (SchroBits *bits)
{
  schro_bits_sync (bits);
}

void
schro_bits_sync (SchroBits *bits)
{
  if (bits->shift != 7) {
    schro_bits_shift_out (bits);
  }
}

void
schro_bits_append (SchroBits *bits, uint8_t *data, int len)
{
  if (bits->shift != 7) {
    SCHRO_ERROR ("appending to unsyncronized bits");
  }

  SCHRO_ASSERT(bits->n + len <= bits->buffer->length);

  oil_memcpy (bits->buffer->data + bits->n, data, len);
  bits->n += len;
}

void
schro_bits_encode_bit (SchroBits *bits, int value)
{
  value &= 1;
  bits->value |= (value << bits->shift);
  bits->shift--;
  if (bits->shift < 0) {
    schro_bits_shift_out (bits);
  }
}

void
schro_bits_encode_bits (SchroBits *bits, int n, unsigned int value)
{
  int i;
  for(i=0;i<n;i++){
    schro_bits_encode_bit (bits, (value>>(n - 1 - i)) & 1);
  }
}

static int
maxbit (unsigned int x)
{
  int i;
  for(i=0;x;i++){
    x >>= 1;
  }
  return i;
}

void
schro_bits_encode_uint (SchroBits *bits, int value)
{
  int i;
  int n_bits;

  value++;
  n_bits = maxbit(value);
  for(i=0;i<n_bits - 1;i++){
    schro_bits_encode_bit (bits, 0);
    schro_bits_encode_bit (bits, (value>>(n_bits - 2 - i))&1);
  }
  schro_bits_encode_bit (bits, 1);
}

void
schro_bits_encode_sint (SchroBits *bits, int value)
{
  int sign;

  if (value < 0) {
    sign = 1;
    value = -value;
  } else {
    sign = 0;
  }
  schro_bits_encode_uint (bits, value);
  if (value) {
    schro_bits_encode_bit (bits, sign);
  }
}

int
schro_bits_estimate_uint (int value)
{
  int n_bits;

  value++;
  n_bits = maxbit(value);
  return n_bits + n_bits - 1;
}

int
schro_bits_estimate_sint (int value)
{
  int n_bits;

  if (value < 0) {
    value = -value;
  }
  n_bits = schro_bits_estimate_uint (value);
  if (value) n_bits++;
  return n_bits;
}

