# RAM Audit (STM32H743 + ChibiOS)

## Résumé
- La commande `size` reporte 528 kio de BSS car elle mesure l'écart d'adresses entre segments RAM (AXI → DTCM → D2), mais les sections réellement allouées dans l'ELF occupent ~10 kio : `.mstack` 1 kio, `.pstack` 1 kio, `.nocache` 516 o, `.ram_d2` 1376 o, `.bss` 7136 o, `.eth` vide.【F:ddc79d†L1-L7】
- Le **top 30** des symboles RAM est dominé par le noyau ChibiOS (`ch0` + idle WA ~2,7 kio) et le driver WS2812 (`pwm_buffer` 1360 o), suivis des buffers display (1024 o) et des stacks des threads pots/boutons (512/256).【2c0989†L1-L31】【F:d43545†L20-L33】【F:47e133†L33-L34】【F:e7724f†L70-L77】【F:dea07e†L101-L109】
- Les buffers DMA audio théoriques ne sont pas liés dans l'ELF actuel (driver non référencé) ; leur empreinte attendue serait ~3,5 kio en `.ram_d2`, bien inférieure aux 128 kio disponibles.【F:a63abb†L24-L42】【F:a63abb†L88-L106】【F:2ad840†L1-L4】
- Les stacks configurées sont modestes (256–2048 o) ; aucune ne dépasse 2 kio et toutes restent compatibles avec la RAM disponible. Les optimisations possibles portent surtout sur le WS2812 (RAM_D2) ou sur la réduction progressive des WAs une fois la charge mesurée.【F:d43545†L14-L28】【F:0c66c1†L88-L92】【F:697fd8†L16-L23】【F:e1d667†L17-L23】【F:44a113†L236-L244】

## Commande reproductible

Un script dédié produit le TOP 30 des symboles en sections RAM (bss/data/ram_d2/nocache/eth) :

```bash
./tools/ram_report.sh [build/.elf]
```

## Top 30 symboles RAM

```
$ ./tools/ram_report.sh
      2288 .bss         ch0
      1360 .ram_d2      pwm_buffer
      1024 .bss         buffer
       992 .bss         waPotReader
       736 .bss         waButtons
       512 .nocache     __nocache_sd1_buf
       448 .bss         ch_c0_idle_thread_wa
       156 .bss         USBD2
       156 .bss         USBD1
       132 .bss         dma
        96 .bss         SD3
        80 .bss         SDCD1
        75 .bss         led_buffer
        72 .bss         ch_factory
        68 .bss         bdma
        60 .bss         SPID6
        60 .bss         SPID5
        60 .bss         SPID4
        60 .bss         SPID3
        60 .bss         SPID2
        60 .bss         SPID1
        60 .bss         I2CD3
        56 .bss         ADCD3
        56 .bss         ADCD1
        36 .bss         default_heap
        32 .bss         PWMD8
        16 .bss         spi5_mutex
        16 .bss         sd_out_buf3
        16 .bss         sd_in_buf3
        16 .bss         leds_mutex
```

*(Sortie directe du script ci-dessus.)*

## Taille par section RAM

| Section | Taille (octets) | Commentaire |
| --- | ---:| --- |
| `.mstack` | 1024 | Main stack (DTCM).【F:ddc79d†L1-L7】 |
| `.pstack` | 1024 | Process stack (DTCM).【F:ddc79d†L1-L7】 |
| `.nocache` | 516 | Buffer SDMMC non-cacheable interne ChibiOS.【F:ddc79d†L1-L7】【F:2475ac†L11-L15】 |
| `.eth` | 0 | Section définie mais vide.【F:ddc79d†L1-L7】 |
| `.ram_d2` | 1376 | WS2812 PWM buffer (RAM_D2 non-cacheable via MPU).【F:ddc79d†L1-L7】【F:d43545†L24-L33】 |
| `.bss` | 7136 | Noyau ChibiOS + buffers display/pots/boutons etc.【F:ddc79d†L1-L7】【F:47e133†L33-L34】【F:e7724f†L70-L77】【F:dea07e†L101-L109】 |

## Analyse par driver / symbole

- **ChibiOS noyau / OS** : `ch0` (2288 o) et `ch_c0_idle_thread_wa` (448 o) proviennent du cœur RT.【2c0989†L1-L31】【F:662512†L52-L63】
- **LEDs WS2812** : `pwm_buffer` (1360 o en `.ram_d2`) + `led_buffer` (75 o) + mutex (16 o). Dimensionné pour 25 LEDs × 24 bits + 80 slots de reset.【2c0989†L2-L31】【F:d43545†L24-L33】【F:ed2d73†L1-L5】
- **Display OLED** : framebuffer `buffer` de 1024 o (128×64/8).【2c0989†L1-L31】【F:47e133†L33-L34】
- **Entrées (pots/boutons)** : stacks `waPotReader` (512 o) et `waButtons` (256 o).【2c0989†L3-L11】【F:e7724f†L70-L77】【F:dea07e†L101-L109】
- **USB/SD cores ChibiOS** : structures `USBDx`, `SDx`, `dma` (~300 o cumulés) et buffer SDMMC non-cacheable (512 o) côté HAL.【2c0989†L7-L25】【F:2475ac†L11-L15】
- **Audio / MIDI / USB Host** : les WAs sont présents (2048 o audio, 2048 o USB host, 512 o MIDI) mais les gros buffers audio DMA ne sont pas liés dans ce binaire (probablement code inactif).【F:0c66c1†L88-L106】【F:1341fe†L104-L109】【F:697fd8†L16-L23】【F:44a113†L236-L244】

## Stacks des threads (WA)

| Thread | Taille déclarée | Fichier source | Reco |
| --- | --- | --- | --- |
| Audio temps réel | 2048 o (`THD_WORKING_AREA_SIZE`) | `drivers/audio/drv_audio.c` / `audio_conf.h`【F:1341fe†L104-L109】【F:0c66c1†L88-L92】 | OK (maintenir pour pipeline audio). |
| USB Host MIDI | 2048 o | `drivers/usb/usb_host/usb_host_midi.c`【F:697fd8†L16-L23】 | OK, marge nécessaire pour stack USB STM32 MW. |
| SD thread | 2048 o | `drivers/sd/drv_sd_thread.c`【F:e1d667†L17-L23】 | Probable à surveiller, peut-être -25 % après mesures. |
| Display refresh | 512 o | `drivers/drv_display.c`【F:de0b52†L237-L244】 | OK pour boucle simple. |
| MIDI USB TX | 512 o | `drivers/midi/midi.c`【F:44a113†L236-L244】 | OK. |
| Pots reader | 512 o | `drivers/drv_pots.c`【F:e7724f†L70-L77】 | Peut descendre à 384 o si la pile reste simple. |
| LEDs DMA worker | 256 o (macro) | `drivers/drv_leds_addr.c`【F:d43545†L14-L33】【F:6b97fa†L47-L49】 | OK, charger léger. |
| Buttons | 256 o | `drivers/drv_buttons.c`【F:dea07e†L101-L109】 | OK. |

## Buffers audio (théorie vs binaire)

- Paramètres : 16 frames/buffer, 8 canaux entrée, 4 canaux sortie, 2 buffers ping/pong, `int32` (4 octets).【F:a63abb†L24-L42】
- Empreinte théorique `.ram_d2` :
  - Entrée : 2 × 16 × 8 × 4 = **1024 o**.
  - Sortie : 2 × 16 × 4 × 4 = **512 o**.
  - SPI link blocs (in/out) : 2 × (4 × 16 × 4 × 4) = **2048 o**.
  - Total attendu ~**3,5 kio** alignés 32 o.
- Empreinte réelle ELF : `.ram_d2` ne contient que `pwm_buffer` (1360 o), signe que les buffers audio ne sont pas encore liés (probablement code pas référencé).【F:d43545†L24-L33】【F:ddc79d†L1-L7】
- Réduction possible : si les buffers sont réintroduits, ils restent largement sous les 128 kio de RAM_D2 ; une réduction prudente (-25 %) reste envisageable en gardant ≥12 frames/buffer.

## Sections spéciales

- `.ram_d2` : 1376 o utilisés sur 128 kio (LDSCRIPT `LENGTH(RAM_D2)`).【F:ddc79d†L1-L7】【F:2ad840†L1-L4】
- `.nocache` : 516 o (buffer SDMMC) sur 128 kio DTCM disponibles pour la région non-cacheable (ram3).【F:ddc79d†L1-L7】
- `.eth` : vide.
- `.mstack` / `.pstack` : 1 kio chacun en DTCM.

## Recommandations ciblées

1. **Clarifier la mesure de RAM** : se baser sur `objdump -h` / `tools/ram_report.sh` plutôt que sur `size`, qui surestime à cause des gaps d’adresses multi-banks.【F:ddc79d†L1-L7】
2. **LEDs WS2812** : si le nombre de LED reste à 25, garder `pwm_buffer` (1360 o). En cas d’augmentation, surveiller `.ram_d2`; en cas de besoin, on peut réduire `RESET_SLOTS` à 64 (80→64) pour gagner 32 o sans risque majeur sur le reset.
3. **Stacks** : conserver 2048 o pour audio/USB/SD tant que la charge n’est pas mesurée. Réduction prudente possible sur `waPotReader` (512→384) après tests runtime.
4. **Audio buffers** : lors de l’intégration effective, placer/maintenir en `.ram_d2` non-cacheable et ajuster `AUDIO_FRAMES_PER_BUFFER` si la latence peut être réduite (ex. 12 frames → -25 % footprint) tout en validant le timing SAI.
5. **SDMMC HAL** : buffer non-cacheable de 512 o est minimal (taille bloc). Pas d’action.

