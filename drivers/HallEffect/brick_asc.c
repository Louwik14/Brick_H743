#include "brick_asc.h"

void brick_asc_array_set_factors(struct brick_asc* asc, size_t capacity, uint8_t start, uint8_t length, uint8_t factor) {
  assert(start < capacity);
  assert(start + length <= capacity);
  if (factor == 0 || factor > BRICK_ASC_MAX_FACTOR) {
    factor = BRICK_ASC_MAX_FACTOR;
  }

  for (uint8_t i = start; i < start + length; ++i) {
    asc[i].sum = 0;
    asc[i].head = 0;
    asc[i].count = 0;
    asc[i].factor = factor;
    for (uint8_t j = 0; j < BRICK_ASC_MAX_FACTOR; ++j) {
      asc[i].buffer[j] = 0;
    }
  }
}

bool brick_asc_process(struct brick_asc* asc, uint16_t rx, uint16_t* tx) {
  if (asc->factor == 0 || asc->factor > BRICK_ASC_MAX_FACTOR) {
    return false;
  }

  asc->sum -= asc->buffer[asc->head];
  asc->buffer[asc->head] = rx;
  asc->sum += rx;

  if (asc->count < asc->factor) {
    asc->count++;
  }

  asc->head = (asc->head + 1U) % asc->factor;
  *tx = (uint16_t)(asc->sum / asc->count);

  return asc->count == asc->factor;
}
