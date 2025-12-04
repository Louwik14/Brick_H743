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

## 10. Pass 2 — Verrouillage formel FMC / MPU / BIST / Boot

### 10.1 Tableau formel de configuration FMC (W9825G6KH-6I @ 100 MHz)
Hypothèses : horloge FMC/SDRAM = 100 MHz (tCK = 10.000 ns), banque SDRAM de 8 MiB sur bus 16 bits. Valeurs minimales issues de la datasheet Winbond W9825G6KH-6I (grade -6I) section AC characteristics ; formule FMC de rafraîchissement d’après RM0433 (FMC_SDRTR COUNT = tREFI × FSDCLK − 20).

| Paramètre FMC | Valeur programmée (cycles) | Valeur obtenue (ns) @100 MHz | Minimum datasheet | Marge réelle | Commentaire de sûreté |
| --- | --- | --- | --- | --- | --- |
| tMRD | 2 cycles | 20.000 ns | 2 tCK = 20.000 ns | 0.000 ns | Respect exact du mini ; pas d’impact SI, délai JEDEC minimal garanti. |
| tXSR | 8 cycles | 80.000 ns | 72.000 ns | +8.000 ns | Couverture température/voltage, assure sortie self-refresh stable. |
| tRAS | 6 cycles | 60.000 ns | 42.000 ns | +18.000 ns | Ajout de marge pour gradients thermiques et jitter PLL. |
| tRC | 6 cycles | 60.000 ns | 60.000 ns | 0.000 ns | Choix minimal compatible datasheet ; SI validée en labo à confirmer. |
| tRCD | 3 cycles | 30.000 ns | 18.000 ns | +12.000 ns | Marge pour amortir éventuel décalage adressage/commande. |
| tRP | 3 cycles | 30.000 ns | 18.000 ns | +12.000 ns | Ferme les lignes avec marge confortable. |
| tWR | 3 cycles | 30.000 ns | 2 tCK = 20.000 ns | +10.000 ns | Sécurise fin d’écriture avant précharge/refresh. |
| CAS latency | 3 cycles | 30.000 ns (CL3) | CL3 autorisé pour tCK ≥ 6.000 ns | +24.000 ns vs tCK_min | CL3 choisi pour cohérence avec tCK=10 ns et marge de setup/hold. |
| Burst length | 4 | N/A | BL=4 supporté | N/A | Compromis débit/brouillard EMI ; aligné sur subregion 1 Mo (MPU). |
| Refresh period | COUNT = 761 | (761+20) / 100 MHz = 7.810 µs | 7.8125 µs (64 ms / 8192) | −0.0025 µs (rafraîchi plus souvent) | COUNT = round_down(7.8125 µs × 100 MHz − 20) = 761 ; garantit intervalle ≤ spec. |

Justification 100 MHz :
- 100 MHz (tCK = 10 ns) reste largement sous la limite 166 MHz du grade -6I, tout en permettant de respecter chaque timing avec arrondi entier supérieur (≥ datasheet) et marges positives sur tRAS/tRCD/tRP/tWR.
- À 100 MHz, la contrainte CL3 (30 ns) reste conforme aux conditions d’accès (tAA/tAC) et augmente la fenêtre de setup/hold côté bus FMC, réduisant la sensibilité au jitter de PLL et aux longueurs de pistes.
- Le calcul de refresh FMC (COUNT = tREFI × FSDCLK − 20) donne 761.25 ; l’arrondi inférieur à 761 induit un refresh toutes les 7.810 µs, soit plus fréquent que le maximum autorisé de 7.8125 µs, garantissant la rétention sur toute la plage de température.

### 10.2 Séquence d’initialisation FMC (diagramme d’états textuel)
```
POWER_OFF
 → POWER_STABLE
 → CLOCK_ENABLE
 → PRECHARGE_ALL
 → AUTO_REFRESH x8
 → LOAD_MODE_REGISTER
 → SET_REFRESH_COUNTER
 → SDRAM_READY
```

| Transition | Condition d’entrée | Délai minimal requis | Commande FMC émise | Erreurs possibles | Action en cas d’échec |
| --- | --- | --- | --- | --- | --- |
| POWER_OFF → POWER_STABLE | VDD/VDDQ ≥ plage nominale et stable | ≥ 200 µs après stabilité alimentation (tINIT datasheet) | Aucune (SDRAM au repos, CKE bas) | Alim instable | Abort init, remonter FAULT matériel. |
| POWER_STABLE → CLOCK_ENABLE | FMC clock validée, GPIO configurées | ≥ 100 µs après POWER_STABLE pour marge | NOP avec CKE=1 (FMC clock enable) | CKE refusé / pas d’oscillation | Réessayer après temporisation, sinon FAULT. |
| CLOCK_ENABLE → PRECHARGE_ALL | CKE haut, horloge FMC active | ≥ tCK × 2 avant commande | PRECHARGE ALL (SDCMD=PRECHARGE, A10=1) | Commande non ACK | Reboucler PRECHARGE jusqu’à ACK, sinon FAULT. |
| PRECHARGE_ALL → AUTO_REFRESH x8 | PRECHARGE_ALL achevé | ≥ tRP programmé (30 ns) | AUTO REFRESH envoyé 8 fois consécutives | Timeout sur cycle refresh | Arrêt séquence, FAULT. |
| AUTO_REFRESH x8 → LOAD_MODE_REGISTER | 8 refresh achevés | ≥ tRC entre rafraîchissements (60 ns) | LOAD MODE REGISTER avec BL=4, BT=seq, CL=3, WB=programmed | Adresse/commande erronée | Rejouer LOAD_MODE_REGISTER une fois ; si second échec → FAULT. |
| LOAD_MODE_REGISTER → SET_REFRESH_COUNTER | Mode register actif, tMRD respecté | ≥ tMRD (20 ns) | Programmation FMC_SDRTR COUNT=761 | Registre non écrit/erreur bus | FAULT + verrouillage SDRAM. |
| SET_REFRESH_COUNTER → SDRAM_READY | Compteur refresh actif | ≥ tREFI (7.810 µs) avant premier accès user | Aucune commande supplémentaire | Compteur inactif | FAULT si refresh status inactif après temporisation. |

### 10.3 Plan MPU final (STM32H7, granularité réelle)
- Granularité MPU Cortex-M7 : tailles puissance de deux de 32 B à 4 GiB, alignement sur la taille de région. Chaque région offre 8 sous-régions égales désactivables. Nombre total de régions disponibles : 16 ; hypothèse d’usage existant 4 régions (ITCM, DTCM, SRAM D2 non-cacheable audio, périphériques), laissant 12 libres pour SDRAM.
- SDRAM 8 MiB alignée sur 0xC0000000 permet une région de 8 MiB avec sous-régions de 1 MiB.

| Région | Base | Taille MPU | Sous-régions actives | Attributs | Usage |
| --- | --- | --- | --- | --- | --- |
| SDRAM_CACHE | 0xC0000000 | 8 MiB | Sous-régions 0–5 actives (0–5 MiB), 6–7 désactivées | Normal memory, cacheable (write-back write-allocate), non-shareable, bufferable | Zones SAMPLES_CACHE, PATTERNS_CACHE, TRACE_LOG regroupées dans les 6 MiB bas, accès CPU uniquement. |
| SDRAM_NONCACHE | 0xC0600000 | 2 MiB | Sous-régions 0–1 actives (2 blocs de 1 MiB) | Normal memory, non-cacheable, non-bufferable, shareable | WORKSPACE_NONCACHE pour buffers futurs DMA ou BIST sans pollution cache. |

Contraintes d’alignement linker :
- Toute section placée en SDRAM_CACHE doit être bornée dans l’intervalle [0xC0000000 ; 0xC05FFFFF] et alignée au minimum sur 32 B (taille de ligne D-Cache), idéalement sur 64 B pour futures optimisations.
- Toute section placée en SDRAM_NONCACHE doit être bornée dans [0xC0600000 ; 0xC07FFFFF] et alignée sur 32 B ; aucune structure ne doit chevaucher la frontière 0xC0600000.
- Pas de fragmentation sous 1 MiB : chaque sous-région reste homogène (cacheable ou non) pour éviter les artefacts de partage MPU.

BIST : à exécuter en région non-cacheable (SDRAM_NONCACHE) pour éliminer tout effet de cohérence ; si test d’une zone cacheable, imposer clean+invalidate complet avant lecture de comparaison pour éviter faux négatifs.

### 10.4 BIST — Spécification formelle
#### 10.4.1 Tests minimaux obligatoires
- Patterns : 0x0000, 0xFFFF, 0xAAAA, 0x5555, walking 1 (bit unique à 1), walking 0 (bit unique à 0).
- Taille minimale : 1 MiB contigu dans SDRAM_NONCACHE (aligné 32 B), couvrant au moins une sous-région complète.
- Ordre de balayage : linéaire croissant par mot 16 bits, pas de permutation ; adresse de départ alignée 32 B.
- Détection : comparer lecture vs motif écrit ; détecte fautes de données (bit collé), fautes d’adresse (aliasing) via motifs alternés, fautes de ligne bloquée via walking 1/0.

#### 10.4.2 Tests étendus (maintenance)
- Couverture : totalité des 8 MiB, motifs de 10.4.1 + motif pseudo-aléatoire déterministe (LFSR 16 bits seed fixe) exécuté en lecture/écriture séquentielle.
- Durée estimée @100 MHz (accès CPU) : ~35–40 ms par MiB pour jeu complet de motifs (6 motifs statiques + walking 1 + walking 0 + pseudo-aléatoire), soit ≈ 320–360 ms pour 8 MiB.
- Conditions : exécuté hors boot, en mode maintenance uniquement, audio suspendu ou buffers déconnectés de SDRAM ; interdiction d’accès SDRAM concurrent pendant le test.

#### 10.4.3 Rapport BIST normalisé
Champs obligatoires :
- État global : PASS / FAIL / ABORT.
- Nombre total de mots 16 bits testés.
- Nombre total d’erreurs détectées.
- Première adresse fautive (adresse absolue SDRAM, alignée au mot testé).
- Type d’erreur : DATA_MISMATCH / ADDRESS_ALIAS / STUCK_AT / TIMEOUT.
- Horodatage : ticks ChibiOS au début et fin du test pour estimer durée.
- Motif en cours lors de la première erreur.

### 10.5 Chronologie boot ChibiOS (ordre et priorités)
1. **RESET** : CPU dans état reset, caches inactifs.
2. **Clocks** : activation PLLs, distribution FMC clock à 100 MHz (avant init SDRAM).
3. **MPU** : configuration initiale régions internes (ITCM/DTCM/SRAM D2 non-cacheable audio, périphériques). Priorité très haute.
4. **FMC init** : activation GPIO FMC + FMC clock gating ; priorité haute, juste après MPU.
5. **SDRAM init** : séquence d’états section 10.2 ; priorité haute mais après FMC init ; bloquante jusqu’à SDRAM_READY.
6. **BIST rapide** : immédiatement après SDRAM_READY ; priorité basse (au-dessous UI future et bien en dessous audio) mais exécuté avant démarrage des threads applicatifs ; durée < 1 MiB.
7. **Audio thread start** : priorité absolue temps réel (plus haute des threads applicatifs), démarre après BIST rapide validé ou contourné en mode dégradé.
8. **UI threads start** : priorité inférieure à l’audio, supérieure aux tâches de maintenance ; démarrent après audio pour éviter contention mémoire initiale.

### 10.6 Mode dégradé — Politique système
- Conditions de FAULT : échec séquence FMC (timeout commande, rafraîchissement inactif), BIST rapide FAIL, rafraîchissement hors spécification détecté.
- Conditions de DEGRADED : BIST étendu échoué alors que séquence d’init réussie, ou erreurs sporadiques détectées en maintenance ; rafraîchissement ajusté en dessous spec mais SDRAM partiellement utilisable.
- Informations exposées : état (READY/DEGRADED/FAULT), compteur d’erreurs, première adresse fautive, motif en échec, timestamp ChibiOS ; UI future reçoit ces champs via driver status.
- Interdictions en DEGRADED :
  - Aucune région SDRAM exposée aux modules supérieurs (get_region retourne NULL/base=0, size=0).
  - Pointeurs vers SDRAM renvoyés NULL ; allocations internes forcées vers SRAM interne uniquement.
  - Pas d’écriture/lecture SDRAM par UI/audio ; audio continue exclusivement en SRAM interne D2.
- Autorisations : audio temps réel et séquenceur restent actifs en RAM interne ; tasks maintenance peuvent lire l’état et relancer un BIST mais ne peuvent pas déverrouiller SDRAM sans reboot.

### 10.7 API — Nommage contractuel (sans prototypes)
- Services publics : `sdram_init`, `sdram_status`, `sdram_run_bist`, `sdram_get_region`, `sdram_get_error`.
- États : `SDRAM_NOT_INITIALIZED`, `SDRAM_INITIALIZING`, `SDRAM_READY`, `SDRAM_DEGRADED`, `SDRAM_FAULT`.
- Erreurs : `SDRAM_ERR_NONE`, `SDRAM_ERR_FMC_TIMEOUT`, `SDRAM_ERR_FMC_CMD`, `SDRAM_ERR_REFRESH`, `SDRAM_ERR_BIST_FAIL`, `SDRAM_ERR_PARAM`.
- Régions : `SDRAM_REGION_CACHE`, `SDRAM_REGION_NONCACHE`, `SDRAM_REGION_INVALID` (pour retour NULL).
- BIST résultat : `BIST_PASS`, `BIST_FAIL`, `BIST_ABORT` avec champs du rapport 10.4.3.

### 10.8 Checklist de validation labo (post-bringup)
| Test | Objectif | Durée | Critère d’acceptation |
| --- | --- | --- | --- |
| Test à froid | Vérifier init FMC/SDRAM après >8 h hors tension | 5 min | BIST rapide PASS, aucune erreur refresh. |
| Test à chaud | Valider tenue thermique | 15 min en chauffe (≥50°C boîtier) | BIST rapide PASS, aucune erreur sur échantillonnage linéaire 1 MiB. |
| Refresh longue durée | Vérifier rétention à 100 MHz, compteur COUNT=761 | ≥1 h avec scrubbing 8 MiB toutes les 10 min | 0 erreur, pas de bitflip dans rapports BIST périodiques. |
| EMI (refresh + burst) | Observer stabilité bus avec rafraîchissement actif | 10 min accès burst CPU + refresh auto | Aucun timeout FMC, pas d’erreur BIST ciblé 1 MiB. |
| Charge audio max | Garantir isolation audio (SRAM interne) | 10 min playback 48 kHz/24 bits ping/pong | Latence audio intacte, aucune interaction SDRAM détectée. |
| Scanlines walking-1 | Détecter lignes bloquées ou aliasing adressage | 5 min, walking 1/0 sur 8 MiB | 0 erreur dans rapport BIST, adresse fautive = NULL. |


