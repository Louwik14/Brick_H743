# SDRAM Driver Architecture (STM32H743 + Winbond W9825G6KH-6I)

## 1. Objectifs et périmètre
- Driver bas niveau + petite abstraction mémoire, auto-suffisant côté drivers, sans dépendance aux couches UI / audio / applicatives encore absentes.
- Aucun allocateur dynamique : seules des régions statiques prédéfinies sont exposées.
- Compatible ChibiOS/RT avec D-Cache actif ; ne doit pas perturber la chaîne audio validée (buffers audio restent en SRAM interne non-cacheable).

## 2. Couche FMC / HAL SDRAM (bas niveau)
**Rôle :** configurer le contrôleur FMC pour la W9825G6KH-6I, appliquer la séquence d'initialisation JEDEC/datasheet, surveiller les erreurs matérielles et fournir l'accès brut mémoire.

### 2.1 Paramètres FMC dérivés de la datasheet
- **Fréquence cible FMC/SDRAM :** 100 MHz (tCK = 10 ns) pour rester sous la limite 166 MHz du grade -6I, avec marge SI et timing, tout en simplifiant les conversions de timings en cycles.
- **CAS latency :** 3 cycles recommandés (supporté par le grade -6 pour tCK ≤ 6 ns ; accepté à 10 ns pour marge supplémentaire).
- **Timings principaux (grade -6/-6I, AC characteristics) :**
  - tRC ≥ 60 ns → 6 cycles @100 MHz.
  - tRAS ≥ 42 ns → 5 cycles (arrondi à 6 pour marge).
  - tRCD ≥ 18 ns → 2 cycles (arrondi à 3).
  - tRP ≥ 18 ns → 2 cycles (arrondi à 3).
  - tWR = 2 tCK → 2 cycles (forcer 3 pour marge).
  - tXSR ≥ 72 ns → 8 cycles @100 MHz.
  - tMRD = 2 tCK minimum (utiliser 2 cycles).
- **Refresh :** 8K lignes en 64 ms → période 7.8125 µs. À 100 MHz, le compteur de refresh FMC doit déclencher ≈ 781 cycles d’horloge (valeur arrondie à 780 ou 781 selon granularité FMC).
- **Mode Register SDRAM :**
  - Burst length : 4 (compromis entre débit et bruit SI ; compatible avec FMC burst).
  - Burst type : séquentiel.
  - CAS latency : 3.
  - Write burst : burst (pas de single write).

### 2.2 Séquence d’initialisation conceptuelle
1. **Power-up / reset** : attendre tINIT ≥ 200 µs après stabilité VDD/VDDQ avant toute commande SDRAM (gérée par délai blocant ou timer RTOS).
2. **Clock enable** : activer l’horloge FMC + GPIO, mettre la SDRAM en état NOP avec CKE = 1.
3. **PRECHARGE ALL** : fermer toutes les lignes.
4. **Auto-refresh** : émettre 8 cycles d’auto-refresh consécutifs (datasheet) avant de charger le mode register.
5. **Mode Register Set** : charger BL=4, CL=3, burst séquentiel, write burst enable ; respecter tMRD ≥ 2 tCK avant toute commande suivante.
6. **Programmer le compteur de refresh** (tREF = 64 ms / 8192) et lancer le rafraîchissement périodique automatique.
7. **Stabilisation** : attendre au moins tXSR après éventuellement un self-refresh, puis autoriser les accès.

### 2.3 Points de configuration FMC spécifiques
- Brochage FMC déjà défini dans board.h/board.c ; vérifier bank utilisée (FMC Bank1 SDRAM à base 0xC0000000 typiquement).
- Largeur bus : 16 bits, 4 banks internes, 12 lignes adresse + A10/AP pour auto-precharge, A11/A12 selon câblage.
- Temps de latence de sortie (tOH/tAC) non configurables directement mais pris en compte par choix tCK/CAS.
- Délai d’insertion de délai d’horloge (clock period) aligné sur le PLL FMC ; surveiller jitter pour SAI (indépendant ici).

## 3. Couche driver SDRAM (logique au-dessus du FMC)
**Rôle :** encapsuler la configuration FMC, l’état d’initialisation, la détection d’erreurs, et fournir des services de test et d’information aux couches supérieures.

### 3.1 Responsabilités
- Appliquer la séquence d’init de façon idempotente ; ignorer les appels multiples si déjà initialisé avec succès.
- Conserver un état interne : NON_INITIALIZED, INITIALIZING, READY, FAULT, DEGRADED.
- Traiter les retours FMC/HAL (timeouts, flag d’erreur) et stocker un code d’erreur détaillé.
- Offrir un auto-test mémoire (BIST) configurable (rapide vs complet) sur une tâche non critique.

### 3.2 Opérations principales exposées
- **Init SDRAM** : configure FMC + séquence d’init ; option pour lancer un BIST court immédiat ; renvoie un statut (succès/échec) et code d’erreur.
- **Interroger l’état** : savoir si la SDRAM est prête ; obtenir le dernier code d’erreur ; savoir si le mode dégradé est actif.
- **BIST manuel** : lancer un test en tâche de fond ; fournir un résultat structuré (nombre d’octets testés, patterns testés, adresse première erreur, nombre d’erreurs, timestamp).
- **Infos mémoire** : retourner l’adresse de base, la taille totale utilisable, et la liste des régions logiques exposées (voir couche memory map).
- **Verrouillage d’accès** : offrir un mécanisme léger pour empêcher l’usage de la SDRAM avant fin d’init (flag global consultable par les modules supérieurs).

## 4. Couche “memory map” (abstraction de régions)
**Rôle :** définir des régions nommées, à taille fixe, alignées, avec attributs cache/MPU prédéfinis. Aucun allocateur ni sous-allocation dynamique.

### 4.1 Propositions de régions
- **SAMPLES_CACHE** : grande zone cacheable pour stockage de samples audio ou tables wavetable (lecture CPU seulement, pas de DMA). Alignement 32 octets, taille majoritaire (~4–5 MiB).
- **PATTERNS_CACHE** : zone cacheable pour structures séquenceur/Patterns volumineux ; alignement 32 octets, taille ~1–2 MiB.
- **WORKSPACE_NONCACHE** : zone non-cacheable (MPU) pour futurs périphériques DMA non audio (ex: accélération graphique, stockage temporaire de données partagées) ; alignement 32 octets, taille ~0.5–1 MiB.
- **TRACE_LOG** : zone cacheable pour buffers de log/trace volumineux ; taille ~0.5 MiB.

### 4.2 Attributs et alignement
- Alignement minimum 32 octets pour compatibilité D-Cache line size et éventuels nettoyages/invalidation ciblés.
- Régions non-cacheables configurées via MPU subregioning sur la base SDRAM (zones multiples de 256 KiB/1 MiB selon alignement MPU H7).
- Les tailles finales seront définies statiquement dans un en-tête driver ; aucune exportation de symboles linker dynamiques.

### 4.3 Usage par couches supérieures
- Chaque région est référencée par un identifiant énuméré ; le driver fournit base + taille + attribut cache. Les couches UI/Backend/AUDIO pourront obtenir ces métadonnées sans connaître la configuration FMC.
- Aucune dépendance inverse : les modules supérieurs ne peuvent modifier ni la table de régions ni les attributs cache à l’exécution.

## 5. Threads, timing et priorités
- **Init SDRAM** : exécutée au boot avant démarrage du thread audio, idéalement dans un thread d’init dédié de priorité juste en dessous des init critiques horloge/MPU. L’init peut être bloquante mais doit terminer avant activation audio.
- **BIST** : lancé dans un thread basse priorité (inférieure à l’UI future et largement inférieure à l’audio). Configurable :
  - Mode “rapide” (quelques patterns sur un sous-ensemble d’adresses) activable au boot avec timeout court.
  - Mode “complet” (walking 1/0, inversions, pseudo-aléatoire déterministe) déclenchable manuellement en maintenance ; peut prolonger le boot → option à désactiver en release.
- Pendant l’init/BIST, l’accès SDRAM par d’autres modules est interdit ; un flag global READY doit être vérifié par toute demande d’accès.

## 6. Stratégie mémoire / cache / MPU
- **Mapping attendu** : FMC Bank1 SDRAM, base 0xC0000000, taille 8 MiB (512K x 16). FMC mapping confirmé par STM32H7 RM ; la totalité est adressable linéairement.
- **Partitionnement cache** :
  - Régions cacheables : SAMPLES_CACHE, PATTERNS_CACHE, TRACE_LOG (accès CPU uniquement, haut débit CPU favorisé).
  - Régions non-cacheables : WORKSPACE_NONCACHE (pour futurs usages DMA potentiels via SDRAM). Configurées via MPU avec attributs Strongly Ordered/Device ou Normal non-cacheable selon besoin.
- **Contraintes D-Cache** : alignement 32 bytes ; si DMA futur sur SDRAM, prévoir buffers entiers en région non-cacheable pour éviter clean/invalidate en boucle. Aucune interaction avec pipeline audio (reste en SRAM D2).

## 7. API publique (conceptuelle, sans prototypes C)
- **Fonctions** :
  - `sdram_driver_init` : lance l’init FMC + séquence JEDEC ; option BIST rapide ; renvoie statut + code d’erreur.
  - `sdram_driver_is_ready` : indique si la SDRAM est initialisée et non en faute.
  - `sdram_driver_get_error` : renvoie le dernier code d’erreur (enum) et l’état global (READY/FAULT/DEGRADED).
  - `sdram_driver_run_bist` : déclenche un test mémoire (mode rapide/complet) et retourne un handle pour consulter le résultat.
  - `sdram_driver_get_region_info` : fournit base, taille, attribut cache d’une région identifiée.
  - `sdram_driver_get_base_address` / `get_total_size` : informations brutes d’adressage.
- **Types/Enums conceptuels** :
  - Codes d’erreur : NONE, FMC_TIMEOUT, FMC_CONFIG_ERROR, INIT_SEQUENCE_ERROR, REFRESH_CONFIG_ERROR, BIST_FAILED, PARAM_INVALID, NOT_INITIALIZED.
  - États : NON_INITIALIZED, INITIALIZING, READY, FAULT, DEGRADED.
  - Résultat BIST : statut (SUCCESS/FAIL), nombre d’octets testés, patterns exécutés, adresse de première faute, compteur de fautes, durée du test.
  - Identifiants de régions : SAMPLES_CACHE, PATTERNS_CACHE, WORKSPACE_NONCACHE, TRACE_LOG.

## 8. Gestion des erreurs & mode dégradé
- **Catégories d’erreurs** :
  - Absence/mauvais câblage (aucune réponse FMC, timeout tRCD/tRP).
  - Erreur de séquence d’init (ex : refresh non accepté, mode register refusé).
  - Refresh mal configuré (RAF trop lent → BIST détecte bitflips).
  - Échec BIST (erreurs d’intégrité mémoire).
- **Comportement en cas d’échec** :
  - Init échoue : bascule en mode FAULT, aucune région exposée, le firmware continue avec RAM interne uniquement.
  - BIST échoue après init : marquer l’état DEGRADED, désactiver l’exposition des régions ou marquer certaines régions comme “non fiables”.
  - Codes d’erreur persistants consultables par les couches supérieures pour afficher un message ou désactiver certaines fonctionnalités.

## 9. Risques techniques à valider en labo
- **Marges de timing** : valider la fermeture SI à 100 MHz ; vérifier que l’arrondi de cycles (tRCD/tRP/tRAS) reste > min datasheet avec marge température.
- **Stabilité refresh** : vérifier la programmation FMC du compteur (≈781 cycles) ; surveiller la dérive PLL vs exigences 64 ms/8K lignes.
- **Effet D-Cache sur BIST** : BIST doit opérer en mode cache cohérent (flush/invalidate) ou en région non-cacheable pour éviter les faux positifs.
- **MPU** : alignement des régions non-cacheables sur tailles MPU ; s’assurer de ne pas fragmenter en dessous de granularité MPU (256 KiB/1 MiB).
- **SI/Layout** : à valider sur carte (longueurs de lignes, terminaisons) ; si erreurs sporadiques, réduire fréquence FMC ou allonger timings.
- **Interaction audio** : confirmer qu’aucun accès SDRAM n’est fait depuis les ISR/threads audio ; vérifier latence d’init pour ne pas retarder l’audio bring-up.

