#include "brick_asc.h"

void brick_asc_array_set_factors(struct brick_asc* asc, size_t capacity, uint8_t start, uint8_t length, uint8_t factor) {
  assert(start < capacity);

  uint8_t end = start + length;
  assert(end <= capacity);

  for (uint8_t i = start; i < end; ++i) {
    asc[i].factor = factor;
  }
}

bool brick_asc_process(struct brick_asc* asc, uint16_t rx, uint16_t* tx) {
  asc->sum += rx;

  *tx = asc->sum / (asc->count + 1);

  asc->count = (asc->count + 1) % asc->factor;

  asc->sum *= asc->count != 0;

  return asc->count == 0;
}
