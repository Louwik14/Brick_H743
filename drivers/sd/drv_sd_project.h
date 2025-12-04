/**
 * @file drv_sd_project.h
 * @brief Couche données projet : patterns, samples, listing.
 */

#ifndef DRV_SD_PROJECT_H
#define DRV_SD_PROJECT_H

#include "drv_sd.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

sd_error_t drv_sd_project_load_pattern(const char *project_name,
                                       const char *pattern_name,
                                       uint8_t    *buffer,
                                       size_t      buffer_size,
                                       size_t     *loaded_size,
                                       uint32_t   *generation);

sd_error_t drv_sd_project_save_pattern(const char *project_name,
                                       const char *pattern_name,
                                       const uint8_t *data,
                                       size_t data_size,
                                       uint32_t generation);

sd_error_t drv_sd_project_load_sample(const char *sample_name,
                                      uint8_t    *buffer,
                                      size_t      buffer_size,
                                      size_t     *loaded_size);

sd_error_t drv_sd_project_list_projects(sd_project_info_t *projects,
                                        size_t max_projects,
                                        size_t *listed);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SD_PROJECT_H */
