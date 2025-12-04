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

## 6. Complément d’architecture SD – 2025-02-05

### 6.1 Contrat temps réel formel du thread SD
- **Modèle de requêtes** : toute interaction SD passe par une file de messages (mailbox/queue statique) alimentée uniquement par des threads non critiques. Chaque message inclut la demande (lecture/écriture/listing), le buffer fourni par l’appelant et un handle de synchronisation optionnel (sémaphore/statut partagé) pour signaler la fin. Les appels exposés à l’application sont non bloquants par défaut (post de requête) avec possibilité d’attendre hors threads critiques via synchronisation explicite.
- **Blocage et synchronisme** : aucune API ne peut bloquer le thread audio ou un ISR car ces contextes n’ont pas le droit de poster. Les threads UI ou de gestion peuvent choisir d’attendre la fin via une synchronisation dédiée, mais cette attente se fait en dehors des threads audio/SPI. Pas d’appel direct synchrone vers FatFS ou HAL depuis l’UI : tout passe par le thread SD.
- **Garantie de non-perturbation audio** : priorité du thread SD strictement inférieure à Audio et SPI/Clock ; la durée théorique maximale d’une opération SD traitée est bornée par un quantum de traitement de 64 KiB (lecture/écriture segmentée) équivalent à quelques millisecondes sur carte lente. Si plusieurs requêtes sont en attente, le thread SD vide la file en séquence sans préemption par type ; l’audio n’est jamais bloqué car aucune ressource partagée temps réel n’est prise. Une requête longue pendant une interaction UI retarde uniquement la réponse UI ; l’audio reste isolé.
- **Interdiction d’accès SD** : seuls les threads UI et de gestion non critiques peuvent poster des requêtes. Interdiction stricte depuis ISR, thread audio, et threads SPI/Clock. Cette interdiction est rendue effective par conception : les fonctions publiques de l’API SD sont des wrappers qui postent dans la file SD et vérifient le contexte courant (assert/return immédiat) pour refuser ISR/audio ; le thread SD est le seul détenteur des primitives FatFS/HAL et aucune fonction FatFS/HAL n’est exportée ailleurs.
- **Garanties vs non-garanties** : garanti par conception : sérialisation des accès, absence d’appel SD dans l’audio/ISR, priorités audio > SPI > UI ≥ SD, pas de malloc. Non garanti : temps de service exact dépendant de la carte (latence variable), absence d’étoilement UI si la file se remplit fortement, performance sur cartes très lentes non bornée au-delà du quantum de découpage.

### 6.2 Politique de sérialisation des accès
- **File d’attente** : plusieurs requêtes peuvent coexister dans la queue statique (taille bornée). Pas d’annulation une fois postée (si la queue est pleine : refus immédiat). Pas de priorité par type : politique FIFO stricte pour préserver la prédictibilité et éviter d’affamer une opération en cours.
- **Conflits lecture/écriture** : si une écriture est en cours et qu’une lecture arrive, la lecture attend sa place dans la FIFO. Idem si une lecture de gros sample est en cours et qu’une sauvegarde de pattern arrive : la sauvegarde attend la fin du bloc courant, le découpage par tranches de 64 KiB limite la latence accumulée. Aucune préemption interne n’est autorisée.
- **Erreurs en cours d’opération** : sur erreur FatFS/HAL durant un transfert, l’opération courante est abortée, un code d’erreur est renvoyé au requérant, le thread SD consomme l’élément suivant de la FIFO. La file continue d’être drainée ; pas de purge globale sauf erreur fatale (voir 6.5).
- **Politique de refus** : si la queue est pleine, le posteur reçoit immédiatement un code “BUSY” et doit réessayer. Aucun blocage d’insertion.

### 6.3 Politique cache / MPU — verrouillage formel
- **Choix unique** : buffers SD exclusivement en zone MPU non-cacheable (section dédiée en D2 SRAM). Justification : STM32H743 avec D-Cache actif → éviter toute maintenance cache coûteuse et le risque d’incohérence DMA. Les copies vers/depuis RAM cacheable sont faites par le thread SD ou l’application après transfert.
- **Alignement minimal** : 32 octets minimum pour tous les buffers DMA SD ; alignement sectoriel de 512 octets pour les transferts de blocs FatFS.
- **Tailles maximales garanties** :
  - Patterns : buffer statique unique ≤ 8 KiB en RAM non-cacheable pour transfert, puis copie en RAM cacheable si nécessaire.
  - Buffer de transfert sample : taille fixe ≤ 64 KiB en RAM non-cacheable, utilisé en ping/pong interne au thread SD pour charger par segments.
- **Interdictions explicites** : interdiction d’utiliser un buffer cacheable directement en DMA SDMMC sans nettoyage/invalidation (scénario exclu par le choix non-cacheable). Interdiction d’emprunter des buffers audio (non partagés) pour le SD. Aucune allocation dynamique de buffers SD.

### 6.4 Politique “une seule ressource en RAM”
- **Propriété des buffers** : le thread SD possède le buffer de transfert non-cacheable pendant toute la durée d’une opération. Pour les patterns, un unique buffer statique est rempli puis la propriété passe à l’application seulement après notification de fin (synchro). Pour les samples, le thread SD remplit un buffer de transfert puis copie vers un buffer applicatif cacheable fourni ; la propriété du buffer applicatif revient à l’appelant uniquement après completion signalée.
- **Prévention d’écrasement** : le thread SD ne réutilise jamais le buffer applicatif tant que l’appelant n’a pas accusé réception via la synchronisation de fin. Les buffers audio sont distincts et jamais partagés avec le SD. L’application doit demander explicitement le chargement suivant (pas de streaming automatique), garantissant qu’un seul pattern est en RAM et qu’aucun sample n’est écrasé pendant usage audio.
- **Synchronisation de disponibilité** : fin de chargement signalée par sémaphore/statut dans le message de requête. Le moteur audio ne consomme un pattern ou sample que lorsque ce statut indique “done”. Aucune lecture audio tant que la notification n’est pas reçue, éliminant les courses entre threads.

### 6.5 Classification des erreurs et comportements
- **Récupérables** : fichier absent, limite hors bornes, I/O ponctuelle, CRC isolé. Comportement driver : retourne un code d’erreur sans modifier l’état monté ; continue à traiter la FIFO. Comportement application : informer l’utilisateur ou réessayer. Audio : garanti non impacté. Redémarrage non requis.
- **Semi-critiques** : FAT corrompu partiellement, CRC répétés sur plusieurs blocs, erreurs de lecture persistantes d’un secteur. Driver : démonte le volume, marque l’état “degraded”, refuse nouvelles opérations jusqu’à remount explicite. Application : signaler état dégradé, proposer sauvegarde ailleurs ou remount. Audio : continue (isolation). Redémarrage logiciel facultatif après remount.
- **Fatales** : contrôleur SDMMC bloqué ou timeout matériel systématique, échec d’initialisation HAL, perte d’alimentation pendant écriture laissant le contrôleur en fault. Driver : place l’état “fault”, purge la FIFO, refuse toute requête future jusqu’au reboot ; pas de tentative de récupération automatique. Application : notifier l’utilisateur, inhiber les fonctions SD. Audio : reste actif (pas de dépendance). Redémarrage logiciel requis pour rétablir le service SD.

### 6.6 Testabilité & traçabilité industrielles
- **Instrumentation interne** : compteurs statiques pour erreurs (par catégorie), opérations réussies, aborts ; histogramme ou min/avg/max de latence par opération (mesurée entre post et complétion) ; compteur de saturation de queue (BUSY). Toutes les stats sont tenues dans une structure statique consultable via une API de debug thread-safe (lecture seule, non bloquante).
- **Consultation sans perturber le temps réel** : accès en lecture aux statistiques via le thread UI ou un shell bas priorité ; aucune collecte dans le thread audio. Pas de logs en IRQ. Export possible par snapshot périodique non critique.
- **Scénarios de test minimum** :
  - Stress lecture/écriture prolongé avec patterns et samples (séquences de 4–64 KiB) sur plusieurs heures.
  - Tests sur cartes lentes/défectueuses (classes faibles, cartes usées) pour mesurer latence et erreurs CRC.
  - FS corrompu (FAT altérée) : vérification passage en état “degraded” et réactions applicatives.
  - Coupure d’alimentation simulée pendant écriture : vérification classification semi-critique/fatale et absence d’impact audio.
  - Saturation du thread SD : queue pleine, vérification des retours “BUSY”, absence de blocage UI/audio.
  - Erreurs HAL (timeouts) : validation de passage en état “fault” et nécessité de reboot.
