# Driver SD Groovebox STM32H743 – Architecture proposée

## 1. Schéma d’architecture textuel
- Couche HAL SDMMC (bas niveau) : gestion matérielle STM32H743/ChibiOS pour SDMMC + DMA. Configure GPIO/clocks/SDMMC, prépare les descripteurs DMA en mode bloc (512 octets), détecte les erreurs matérielles (CRC, timeouts, underrun/overrun, abort), et signale l’état au dessus via un statut interne. Aucun accès direct depuis les threads critiques ; uniquement exposée à la couche Filesystem. Aucun lien direct avec l’audio ; l’interface se limite à initialiser le contrôleur, lancer/arrêter les transferts et exposer l’état/presence de la carte.
- Couche Filesystem (FatFS) : s’appuie uniquement sur l’interface HAL SDMMC pour les lectures/écritures bloc. Monte/démonte le volume FAT, convertit les codes FatFS en erreurs internes simplifiées, fournit des opérations de fichiers/dossiers (ouvrir/fermer/lire/écrire/énumérer) limitées aux répertoires /patterns/, /samples/, /projects/. Aucune logique applicative : elle gère seulement le FS et traduit les erreurs.
- Couche Données Projet (haut niveau) : manipule des buffers fournis par l’application pour charger/sauver patterns et samples, et lister les projets. Utilise exclusivement l’API Filesystem depuis le thread SD dédié. Aucun accès aux périphériques matériels ni aux threads audio. Les données sont transférées en mémoire système ; aucune dépendance temps réel stricte à la SD.
- Dépendances : Données Projet → Filesystem → HAL SDMMC. Les threads audio et SPI critiques ne dépendent pas de ces couches et ne peuvent pas appeler leurs fonctions. L’UI peut poster des requêtes asynchrones vers le thread SD sans bloquer l’audio.

## 2. Liste des threads & priorités
- Thread Audio (référence temporelle) : priorité la plus haute parmi les tâches applicatives. Attente sur sémaphore DMA et traitement DSP. Aucun appel vers la SD ou le FS.
- Threads SPI/Clock critiques : juste en dessous du thread Audio. Gestion des cartouches et horloges ; aucun accès à la SD.
- Thread UI : priorité intermédiaire, en dessous des threads Audio et SPI/Clock, au-dessus ou équivalent au thread SD selon réactivité UI désirée. Envoie des requêtes au thread SD via mailboxes/queues sans bloquer.
- Thread SD (dédié) : priorité strictement inférieure aux threads Audio et SPI/Clock (ex. Audio > SPI > UI ≥ SD). Responsabilités : init/mount/unmount, opérations FatFS, lecture/écriture de patterns et samples, gestion d’état et erreurs. Traite des messages (file de requêtes) provenant de l’UI ou d’autres threads non critiques. Peut utiliser sémaphores ou mailboxes ChibiOS pour signaler la fin des opérations aux requérants. Aucun traitement dans les ISR ; toutes les fonctions publiques postent des requêtes à ce thread.

## 3. Stratégie mémoire/cache
- Localisation buffers SD : buffers DMA en RAM non cacheable (section dédiée en D2 SRAM) ou marqués via MPU non-cacheable. Buffers applicatifs (patterns, samples) en SRAM cacheable ou SDRAM selon taille, mais les buffers utilisés directement par le DMA SDMMC doivent être non cacheables.
- Politique D-Cache : privilégier une zone non cacheable pour les buffers DMA. Si des buffers cacheables doivent être utilisés pour de grandes lectures/écritures, imposer un alignement minimal de 32 octets et effectuer clean/invalidate explicites avant/après les transferts. Les buffers de travail FatFS (secteur 512 octets) restent en RAM non cacheable pour éviter les opérations de maintenance cache répétées.
- Alignement : minimum 32 octets pour tous les buffers DMA SDMMC. Les buffers de fichiers doivent être multiples de 512 octets pour les accès alignés.
- Tailles typiques :
  - Patterns : buffers statiques de quelques kilo-octets (ex. 4–8 KiB) couvrant la taille maximale d’un pattern sérialisé. Lecture/écriture par blocs 4 KiB pour limiter la latence.
  - Samples : lecture par blocs de 32–64 KiB via un buffer de transfert dédié (non cacheable) puis copie vers la RAM principale ou SDRAM cacheable. Pas de streaming temps réel ; la taille totale peut être grande, mais le transfert est segmenté.
- Pas d’allocation dynamique : tous les buffers sont statiquement définis (FatFS work area, buffers DMA, buffers temporaires patterns/samples, structures de requêtes). Utilisation possible de pools statiques/queues préallouées pour les messages au thread SD.

## 4. API complète
- Initialisation et état :
  - drv_sd_init : configure le contrôleur SDMMC, les GPIO et le DMA ; prépare les buffers statiques ; doit être appelée depuis un contexte non critique (ex. thread init). Retourne un code d’erreur interne.
  - drv_sd_mount / drv_sd_unmount : monte ou démonte le volume FAT depuis le thread SD ; échoue si la carte est absente ou si le FS est invalide. L’état de montage est stocké et requérable.
  - drv_sd_is_present : indique la présence de la carte selon le statut HAL ; ne déclenche pas d’accès.
  - drv_sd_get_fs_state : retourne l’état courant (non initialisé, monté, erreur, absent).
  - drv_sd_get_last_error : retourne le dernier code d’erreur interne enregistré par le thread SD.
- Accès fichiers/dossiers (toutes via thread SD) :
  - drv_sd_load_pattern / drv_sd_save_pattern : lit ou écrit un pattern identifié (nom ou ID) dans un buffer fourni avec sa taille maximale. Échec si la carte n’est pas montée, si le fichier est absent/corrompu ou si la taille dépasse le buffer.
  - drv_sd_load_sample : lit un sample par blocs avec paramètres offset/longueur dans un buffer fourni ; segmentation interne pour les gros fichiers ; échoue si hors limites ou I/O error.
  - drv_sd_list_projects : énumère les dossiers dans /projects/ et remplit une liste fournie (tableau statique) jusqu’à max_projects.
  - Fonctions additionnelles possibles : obtention de la taille d’un fichier, suppression/renommage dans les répertoires dédiés, récupération d’informations de volume. Toutes renvoient un code d’erreur interne et ne sont valides que depuis le thread SD ou via requêtes postées.
- Comportement en erreur : codes d’erreurs explicites (ex. SD_OK, SD_ERR_NO_CARD, SD_ERR_NOT_MOUNTED, SD_ERR_FS_INIT, SD_ERR_FILE_NOT_FOUND, SD_ERR_IO, SD_ERR_CORRUPTED, SD_ERR_PARAM, SD_ERR_BUSY). Absence carte → fonctions retournent SD_ERR_NO_CARD ; FS absent/non montable → SD_ERR_FS_INIT/SD_ERR_NOT_MOUNTED ; fichier manquant → SD_ERR_FILE_NOT_FOUND ; corruption ou CRC → SD_ERR_CORRUPTED ; échec d’écriture (plein/I/O) → SD_ERR_IO et last_error mis à jour.
- Thread-safety : API callable uniquement depuis des threads non critiques qui postent des requêtes au thread SD. Interdiction explicite depuis ISR ou thread audio. Le thread SD sérialise les opérations ; les requêtes peuvent optionnellement fournir une synchro (sémaphore ou message retour) pour attendre la fin hors des threads critiques.

## 5. Risques techniques restants
- Performance : latences de lecture/écriture variables selon la carte SD ; nécessité de tester avec plusieurs classes/UHS pour garantir que les transferts segmentés n’affectent pas la réactivité UI.
- Fragmentation FS : l’allocation de gros samples sur une carte fragmentée peut augmenter la latence ; recommandation de pré-allouer ou de défragmenter les médias de test.
- Corruption : retirer la carte pendant l’alimentation coupée ou en cas d’échec d’écriture peut corrompre le FS ; prévoir des tests de robustesse et éventuellement un mode lecture seule par défaut.
- Taille fichier : bornes maximales des patterns/samples à valider pour garantir que les buffers statiques suffisent ; définir des limites dans l’application.
- Charge système : vérifier que le thread SD à faible priorité n’allonge pas trop les temps de réponse de l’UI ; ajuster la priorité UI/SD après mesures.
- Cache/MPU : hypothèse que la section non cacheable pour DMA est correctement configurée par le linker/MPU ; à valider sur cible réelle avec tests D-Cache (clean/invalidate).
- Tests à planifier : tests de charge prolongés (lecture/écriture répétée), tests de corruption (carte retirée/FS altéré), benchmarks de latence par blocs (4–64 KiB), validation des erreurs FatFS → erreurs internes, et vérification qu’aucune fonction SD n’est appelée depuis l’audio ou une ISR.
