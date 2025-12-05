/**
 * @file mpu_map.h
 * @brief Identifiants centralisés des régions MPU utilisées dans le projet.
 *
 * Ces IDs doivent rester uniques à l'échelle du firmware afin d'éviter tout
 * écrasement silencieux lors de la configuration progressive du MPU.
 */

#ifndef MPU_MAP_H
#define MPU_MAP_H

#define MPU_REGION_SDRAM_MAIN        1u
#define MPU_REGION_SDRAM_RESIDUAL    2u
#define MPU_REGION_D2_NOCACHE        3u

/* Détection triviale des collisions d'ID à la compilation. */
#if (MPU_REGION_SDRAM_MAIN == MPU_REGION_SDRAM_RESIDUAL) || \
    (MPU_REGION_SDRAM_MAIN == MPU_REGION_D2_NOCACHE)   || \
    (MPU_REGION_SDRAM_RESIDUAL == MPU_REGION_D2_NOCACHE)
#error "MPU region IDs must remain unique"
#endif

#endif /* MPU_MAP_H */
