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

## 7. Contrats d’intégrité SD – 2025-02-09

### 7.1 Atomicité des écritures patterns/métadonnées
- **Stratégie unique** : écriture intégrale vers un fichier temporaire dans le même répertoire (`<nom>.tmp`) suivie d’un `rename` vers le nom final. Aucun append, aucune mise à jour partielle : toujours réécriture complète du fichier.
- **Format de fichier** : en-tête statique (magic, version, taille utile, compteur de génération monotone stocké dans l’en-tête, CRC-32 sur les données) pour détecter les écritures incomplètes ou corrompues. Le compteur de génération est incrémenté à chaque sauvegarde réussie et persiste dans le fichier final ; il n’est pas global au système.
- **Atomicité observée** :
  - Avant `rename` : l’ancien fichier reste intact et l’application voit uniquement l’ancienne version. Le `.tmp` peut exister mais n’est jamais consommé.
  - `rename` intra-répertoire : considéré atomique au niveau de l’entrée de répertoire (FatFS) ; aucune fenêtre où un fichier partiel est visible sous le nom final. Le `.tmp` est supprimé ou écrasé par l’opération.
  - Après `rename` : seule la nouvelle version est visible ; l’en-tête + CRC garantissent que la version visible est complète.
- **Détection d’états invalides** : présence d’un `.tmp` au boot déclenche suppression + signalement “sauvegarde incomplète”; un fichier final dont la CRC ou la taille déclarée ne correspondent pas est marqué “corrompu” et ignoré par l’application, qui conserve l’ancien état en RAM jusqu’à action explicite.

### 7.2 Tolérance aux coupures d’alimentation
- **Pendant écriture (avant `rename`)** : l’ancien fichier reste valide ; le `.tmp` est ignoré au reboot. Résultat garanti : fichier final intact (ancien état) ou perdu uniquement si jamais existant (cas création initiale non terminée), mais jamais partiellement lisible.
- **Pendant `rename`** : l’opération est traitée comme atomique ; au reboot, soit l’ancien fichier subsiste, soit le nouveau est complet (CRC valide). Cas non garanti : si la FAT elle-même est corrompue par coupure en pleine mise à jour du répertoire, un fsck externe peut être requis.
- **Pendant update/append** : interdit par conception. Toute modification passe par réécriture complète + `rename` ; aucune fenêtre d’append partiel.
- **Garanties post-reboot** :
  - **FAT intégrité** : supposée saine tant que la coupure intervient hors mise à jour de répertoire ; en cas de corruption détectée par FatFS, montage refusé ou basculé en lecture seule.
  - **Patterns/projets** : dernière sauvegarde validée (CRC + génération cohérente) est disponible ; une sauvegarde interrompue est rejetée et ne remplace jamais la dernière version valide.
  - **Ce qui n’est pas garanti** : aucune garantie de persistance d’une sauvegarde dont le `f_sync` n’a pas été réalisé avant coupure ; nécessité d’un fsck externe si la FAT est incohérente.

### 7.3 Cohérence FAT & anti-corruption
- **Fréquence d’écriture** : limitée aux actions explicites de l’utilisateur (save pattern/projet). Pas d’écriture périodique ni de journalisation continue. Pas de mise à jour partielle ; chaque save réécrit l’intégralité du fichier cible.
- **Politique flush/sync** : `f_sync` systématique après écriture complète du `.tmp` et avant `rename`. Aucune écriture laissée en cache après notification de succès à l’application. Le thread SD force un flush FatFS global avant tout démontage ou passage en mode dégradé.
- **Démontage propre** : lors d’un shutdown logiciel ou d’un passage en “degraded/fault”, le thread SD vide la FIFO, rejette les nouvelles requêtes, `f_sync` le volume, puis démonte (si possible) pour minimiser les risques de FAT sale.
- **fsck requis** : uniquement quand FatFS signale un volume non montable ou une incohérence de répertoire après coupure. L’application signale à l’utilisateur la nécessité d’un check PC ; aucune tentative automatique de réparation embarquée.
- **Montage lecture seule** : si des erreurs récurrentes (CRC multiples, incohérences répertoire) sont détectées sans possibilité de démontage propre, le thread SD force un remontage en lecture seule ou refuse le montage pour éviter toute écriture supplémentaire.

### 7.4 Stratégie de montage / boot / erreurs au démarrage
- **Séquence boot** :
  1. Init HAL SDMMC + DMA + buffers non cacheables.
  2. Détection présence carte (GPIO/SDMMC). Si absente : état “no card”, pas de tentative de mount.
  3. Si présente : montage FAT (FatFS) avec work area statique.
  4. Validation répertoires `/patterns`, `/samples`, `/projects` (création interdite si montage RO ; en RW, création si absent échoue → état “fs error”).
  5. Purge des `.tmp` éventuels laissés par un arrêt brutal et journalisation de l’événement.
- **Comportements** :
  - Carte absente : système fonctionne en mode dégradé sans accès SD ; UI indique “SD absente”; audio et séquenceur continuent avec ressources en RAM uniquement ; navigation projet limitée aux éléments déjà en RAM.
  - FAT non montable : montage refusé, état “fs error”; UI propose de retirer la carte et d’effectuer un fsck sur PC ; aucun accès écriture, audio inaltéré.
  - Répertoires manquants : en RW, tentative de création ; si échec ou montage RO, passage en mode “partial/degraded” avec accès lecture seule aux répertoires existants ; sauvegarde désactivée.
  - Mode dégradé : thread SD reste actif pour détection/remontage manuel mais refuse toute écriture ; UI verrouille les actions save/export ; audio inchangé.

### 7.5 Garanties d’intégrité des données utilisateur
- **Pattern sauvegardé** : un pattern validé (CRC OK + génération incrémentée + `rename` réussi) ne peut pas redevenir illisible sans nouvelle opération d’écriture. Si un fichier final devient corrompu (CRC invalide), il est rejeté et l’état RAM n’est pas remplacé silencieusement.
- **Pas de rollback silencieux** : grâce au compteur de génération stocké dans le fichier final, un fichier plus ancien (génération inférieure) est refusé comme source de vérité après un reboot ; l’application signale l’anomalie et conserve la dernière version cohérente en RAM si disponible.
- **Projets non partiels** : aucun projet n’est visible si son fichier échoue la validation (CRC/taille/génération). Le listing ignore les entrées invalides ou .tmp. Aucune partie d’un projet n’est chargée si l’ensemble n’est pas validé.
- **Chargement cohérent** : la fonction de load vérifie l’en-tête (magic/version), la taille déclarée (≤ buffer) et la CRC avant exposition à l’application. En cas d’échec, le pattern/projet est marqué “corrompu” et non utilisé, empêchant un état incohérent.
- **Limites connues** : la protection dépend de l’intégrité de la FAT ; une coupure pendant la mise à jour du répertoire peut exiger un fsck externe. Le compteur de génération est local à chaque fichier : il ne protège pas contre une restauration manuelle d’un ancien fichier par l’utilisateur sur PC (considéré explicite).

## 8. Sûreté extrême & conformité finale – 2025-02-22

### 8.1 Scénarios extrêmes & défaillances combinées
- **Carte SD extrêmement lente (débit faible / latences élevées)** :
  - Thread SD : découpe systématique en blocs ≤64 KiB, reste en état **normal** tant que les opérations aboutissent ; si les latences saturent la FIFO → retour BUSY et refus des requêtes supplémentaires.
  - UI : reçoit des statuts « en cours » puis « BUSY » si la file se remplit ; doit échelonner les demandes ou indiquer un sablier. Pas de blocage ; la réponse peut être tardive.
  - Audio : impact strictement nul (priorités plus hautes, aucune ressource partagée).
  - Driver : reste **normal** ; en cas de file pleine répétée, instrumentation incrémente le compteur de saturation.
  - Garanti : aucune préemption audio, sérialisation des requêtes, refus explicite quand la file est pleine.
  - Non garanti : temps de service maximum ; réactivité UI si la carte est très lente.
- **Carte SD instable (CRC fréquents)** :
  - Thread SD : jusqu’à 3 tentatives par bloc, sinon passage en état **degraded** (lectures/écritures refusées ou basculées en RO).
  - UI : reçoit erreur explicite (CRC) puis notification d’état dégradé ; doit bloquer les actions d’écriture et proposer remount.
  - Audio : aucun impact.
  - Driver : passe **degraded** ou **read-only** selon la capacité à monter le volume ; purge de la requête fautive et poursuite des suivantes si monté en RO.
  - Garanti : aucune écriture après bascule RO, préservation des données existantes.
  - Non garanti : réussite de l’opération en cours ; temps de recovery automatique (besoin d’action UI pour remount).
- **Carte SD pleine à 100 %** :
  - Thread SD : l’écriture échoue immédiatement (SD_ERR_IO) et l’état reste **normal** (volume monté) ; aucune tentative de tronquer.
  - UI : signale « espace insuffisant », n’envoie pas de nouvelles saves tant que l’utilisateur n’a pas libéré de l’espace.
  - Audio : aucun impact.
  - Driver : reste **normal** ; instrumentation enregistre l’échec.
  - Garanti : absence de fichiers partiels (stratégie .tmp + rename), intégrité des données précédentes.
  - Non garanti : finalisation de la sauvegarde courante.
- **Carte SD absente après boot (perte mécanique)** :
  - Thread SD : si présence se désactive → passe en état **unmounted** + drapeau **fault** si une opération était en cours ; refuse toute nouvelle requête jusqu’à remount manuel.
  - UI : affiche « carte retirée », purge sa file locale ; toute nouvelle commande reçoit NO_CARD.
  - Audio : aucun impact.
  - Driver : état **fault** si perte pendant I/O, sinon **unmounted** ; nécessite remount explicite après réinsertion.
  - Garanti : arrêt immédiat des écritures, pas de crash audio.
  - Non garanti : sauvegarde en cours (peut être perdue), cohérence FAT (fsck PC peut être requis).
- **Saturation permanente de la FIFO SD** :
  - Thread SD : refuse toute requête dès que la file atteint la capacité max ; reste en état **busy** mais **normal**.
  - UI : reçoit BUSY et doit temporiser ou annuler les demandes massives ; pas de blocage du thread UI.
  - Audio : aucun impact.
  - Driver : instrumentation saturations++ ; aucune perte de données sauf requêtes refusées.
  - Garanti : isolation audio, absence de deadlock.
  - Non garanti : ordre de service au-delà du FIFO (les requêtes refusées doivent être reproposées par l’UI).
- **Enchaînement rapide de sauvegardes utilisateur** :
  - Thread SD : traite en FIFO stricte ; si le cumul excède la capacité → BUSY et rejet immédiat des nouvelles saves.
  - UI : doit dédupliquer/retarder les saves successives ; fournit un statut « sauvegarde en cours » unique.
  - Audio : aucun impact.
  - Driver : état **normal** ; atomicité préservée par .tmp + rename pour chaque save.
  - Garanti : aucune sauvegarde partielle visible ; dernier succès reste valide.
  - Non garanti : latence totale pour vider la queue si la carte est lente.
- **UI spam de requêtes sur bus SD lent** :
  - Thread SD : FIFO se remplit → BUSY sur les requêtes supplémentaires ; traite les requêtes existantes sans priorité spéciale.
  - UI : reçoit BUSY, doit appliquer backoff et informer l’utilisateur ; pas de blocage.
  - Audio : aucun impact.
  - Driver : reste **normal** ; instrumentation trace la surcharge.
  - Garanti : pas de starvation audio, pas d’inversion de priorité.
  - Non garanti : délai UI pour recevoir les résultats.

### 8.2 Bornes de capacité & comportements au dépassement
- **Taille maximale d’un pattern** : 8 KiB (borne logicielle liée au buffer statique pattern). Dépassement → refus immédiat (SD_ERR_PARAM), aucune écriture ; état **normal** conservé.
- **Nombre maximal de patterns par projet** : 128 (borne logicielle pour tables statiques et parcours déterministe). Dépassement au listing → ignore les entrées supplémentaires et retourne un statut « limité » ; au save → refus avec erreur « quota patterns ».
- **Taille maximale d’un sample** : 64 MiB (borne mémoire + temps réel pour transfert segmenté). Dépassement → refus de chargement/sauvegarde, état inchangé.
- **Nombre maximal de samples chargeables simultanément** : 16 (borne mémoire et gestion de buffers applicatifs). Si limite atteinte → refus de chargement supplémentaire, le driver ne libère pas automatiquement.
- **Longueur maximale des chemins/noms** : 64 caractères (borne FatFS + tables statiques). Dépassement → refus avant toute I/O, erreur paramètre.
- **Occupation maximale de la FIFO SD** : 8 requêtes (borne logicielle pour mailbox statique). Au-delà → BUSY et rejet immédiat, pas de mise en attente supplémentaire.
- **Nombre maximal de requêtes SD en vol** : 1 en traitement + 7 en file (sérialisation stricte). Les requêtes suivantes sont refusées (BUSY).
- **Nature des bornes** : pattern/sample/chemins = bornes logicielles ; FIFO/requêtes = bornes temps réel/déterminisme ; aucune borne matérielle exposée (hors taille carte). Aucun mode dégradé automatique, uniquement refus explicite.

### 8.3 Contrat d’interaction SD ↔ UI / reste du firmware
- **Ce que le driver reçoit** : requêtes asynchrones (load/save/list/delete) avec buffers fournis, informations de taille/chemin, handle de synchronisation optionnel. Jamais de données audio ni d’horloge.
- **Ce que le driver renvoie** : statuts finis (OK/BUSY/NO_CARD/FS_ERROR/RO/DEGRADED/FAULT), erreurs catégorisées (paramètre, CRC, IO, quota, plein), événements d’état (monté, démonté, RO, degraded, fault) via messages/flags consultables par l’UI.
- **Ce qu’il est interdit qu’il connaisse** : état du séquenceur, tempo, règles musicales, compteur audio, positions de lecture, horloges audio ou SPI. Aucun champ de message ne transporte ces informations.
- **Couplage temporel** : inexistant ; aucune attente sur horloge audio, aucune IRQ audio sollicitée. Les requêtes SD ne modifient pas les priorités ou sémaphores audio.
- **Types d’événements** : changement d’état (mount/unmount/ro/degraded/fault), complétion de requête (succès/erreur), saturation FIFO (BUSY).
- **Types de statuts** : états internes listés en 8.4, codes d’erreurs, métriques de latence (lecture seule, consultables en debug).
- **Types d’erreurs** : paramètre invalide, fichier absent, CRC, IO (plein ou secteur défectueux), contexte interdit (appel ISR/audio), busy.

### 8.4 États internes et transitions
- **États possibles** : `unmounted`, `mounted-rw` (normal), `mounted-ro`, `degraded`, `fault`, `busy` (transitoire pendant traitement), `initializing`.
- **Transitions et causes** :
  - `initializing` → `mounted-rw` : init + mount FAT réussis.
  - `initializing` → `unmounted` : carte absente ou mount refusé.
  - `mounted-rw` → `busy` : requête en cours ; retour à `mounted-rw` après fin.
  - `mounted-rw` → `mounted-ro` : erreurs répétées en écriture ou demande explicite d’ouverture RO ; actions : flush, remount RO, purge FIFO d’écritures.
  - `mounted-rw` → `degraded` : CRC persistants ou incohérence FAT détectée ; actions : démonter proprement, refuser nouvelles requêtes d’écriture, autoriser lecture si remount possible.
  - `mounted-rw` → `fault` : timeout HAL critique, perte carte en cours d’I/O, contrôleur bloqué ; actions : purge FIFO, verrouillage complet des I/O jusqu’au reboot.
  - `mounted-ro` → `mounted-rw` : remount manuel réussi après action utilisateur (extraction/replace) ; audio continue pendant toute la séquence.
  - `mounted-ro`/`degraded` → `fault` : nouvelle erreur matérielle bloquante ; actions : blocage total, notification UI.
  - `unmounted` → `mounted-rw` : insertion carte + mount demandé par UI ; audio continue.
- **Actions automatiques** : flush/sync avant changement d’état quand possible, purge de la FIFO en cas de fault, refus immédiat des écritures en RO/degraded, maintien des compteurs d’erreurs.
- **Actions attendues de l’application/UI** : afficher l’état, proposer remount, empêcher l’écriture en RO/degraded, déclencher un reboot si `fault`.
- **Audio** : continue sans condition pour toutes transitions ; aucune intervention requise.
- **Intervention utilisateur** : nécessaire pour remount après retrait, libérer espace, remplacer carte défectueuse ou redémarrer en cas de fault.

### 8.5 Check-list de conformité industrielle finale
- **Temps réel respecté** : priorité SD < audio/SPI, aucune section critique partagée, transferts segmentés ≤64 KiB ; vérifié par mesures de latence et absence de préemption audio.
- **Absence de dépendance audio** : aucune API SD accessible depuis audio/ISR ; contrôlé par audit de code et tests d’appel forcé (doit retourner erreur contexte).
- **Robustesse power-fail** : stratégie `.tmp` + `rename`, flush + démontage propre ; tests coupure pendant écriture montrent absence de fichier partiel et état attendu (ancienne version intacte ou RO/degraded).
- **Gestion d’erreurs complète** : classification récupérable/semi-critique/fatale, états `degraded`/`fault`, transitions définies ; tests injectés (CRC, timeouts, carte retirée) vérifient les bascules.
- **Aucune allocation dynamique** : buffers/file statiques ; vérifié par audit de binaire (aucun appel malloc) et inspection des sections de l’ELF.
- **Testabilité** : instrumentation accessible (counters, latence, saturations), scénarios de test listés en 6.6 + nouveaux cas extrêmes ; scripts de stress sur plusieurs heures.
- **Traçabilité** : chaque changement d’état/statut consigné dans la structure de stats consultable ; versions et décisions documentées (sections datées 6, 7, 8).
- **Intégrité FAT/données** : atomicité pattern, refus si pleine, montages RO en cas d’erreurs ; vérifié via tests fsck et validation CRC/génération.

## Implémentation – retour terrain (2025-05-24)
- Couche HAL : `SDCD1` initialisée statiquement en 4 bits @50 MHz, déconnexion/reconnexion systématique par requête. Présence carte lue via `sdcIsCardInserted`.
- Mémoire/DMA : buffers FatFS (work area) et transferts SD placés en `.ram_d2` aligné 32 octets (`SD_DMA_BUFFER_ATTR`). Tampon échantillons 64 KiB segmenté pour garantir D-Cache cohérent sans opérations dynamiques.
- Thread SD : unique thread `sdThread` (priorité `NORMALPRIO-2`, pile 2048) alimenté par mailbox statique (profondeur 8). Pool statique de requêtes, rejet immédiat en cas de saturation (BUSY ++ instrumentation).
- Machine d’état : `initializing → unmounted` après init, `mounted-rw/ro` après mount, état `busy` transitoire par requête. CRC/IO/FS entraînent bascule `degraded` (ou `unmounted` si NO_CARD), perte critique → `fault`. Écritures refusées en `mounted-ro` ou `degraded`.
- Atomicité : sauvegardes pattern via fichier `.tmp` + `f_sync` puis `rename`, en-tête structuré (magic/version/taille/génération/CRC). Chargements vérifient header + CRC.
- Intégration FatFS : montage/démontage encapsulés, répertoires `/projects/<name>/patterns` et `/samples` utilisés ; listing limité aux dossiers sous `/projects`.
- Protection temps réel : toutes API publiques rejettent immédiatement les appels en ISR ou depuis le thread audio (`audioProcess`). Aucune allocation dynamique, aucune attente active hors thread SD (attente uniquement sur sémaphore utilisateur si demande bloquante).
