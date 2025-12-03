/**
 * @file drv_audio.h
 * @brief Infrastructure audio TDM (SAI + DMA + callbacks de traitement).
 */

#ifndef DRV_AUDIO_H
#define DRV_AUDIO_H

#include "ch.h"
#include "hal.h"
#include "audio_conf.h"

/* -------------------------------------------------------------------------- */
/* API publique                                                               */
/* -------------------------------------------------------------------------- */

void drv_audio_init(void);
void drv_audio_start(void);
void drv_audio_stop(void);

const int32_t* drv_audio_get_input_buffer(uint8_t *index, size_t *frames);
int32_t*       drv_audio_get_output_buffer(uint8_t *index, size_t *frames);
void           drv_audio_release_buffers(uint8_t in_index, uint8_t out_index);

/* Buffers audio SPI-LINK (4 cartouches, 4 canaux). */
int32_t (*drv_audio_get_spi_in_buffers(void))[AUDIO_FRAMES_PER_BUFFER][4];
int32_t (*drv_audio_get_spi_out_buffers(void))[AUDIO_FRAMES_PER_BUFFER][4];
size_t   drv_audio_get_spi_frames(void);

void drv_audio_set_master_volume(float vol);
void drv_audio_set_route(uint8_t track, bool to_main, bool to_cue);
void drv_audio_set_route_gain(uint8_t track, float gain_main, float gain_cue);

/* Hook faible pour le traitement DSP. */
__attribute__((weak)) void drv_audio_process_block(
    const int32_t               *adc_in,   /* [frames][AUDIO_NUM_INPUT_CHANNELS]   */
    const spilink_audio_block_t  spi_in,   /* [4][frames][4] cartouches entrantes */
    int32_t                     *dac_out,  /* [frames][AUDIO_NUM_OUTPUT_CHANNELS]  */
    spilink_audio_block_t        spi_out,  /* [4][frames][4] cartouches sortantes */
    size_t                       frames);

/* Interfaces SPI-LINK (stubs extensibles). */
typedef void (*drv_spilink_pull_cb_t)(spilink_audio_block_t dest, size_t frames);
typedef void (*drv_spilink_push_cb_t)(const spilink_audio_block_t src, size_t frames);

void drv_audio_register_spilink_pull(drv_spilink_pull_cb_t cb);
void drv_audio_register_spilink_push(drv_spilink_push_cb_t cb);

#endif /* DRV_AUDIO_H */
