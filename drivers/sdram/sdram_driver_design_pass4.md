# SDRAM driver — pass 4 verrouillage (audio-first modèle A)

Ce document fige les règles de concurrence, la machine d'états et les contrats d'exécution associés aux headers `sdram_driver.h`, `sdram_driver_priv.h` et `sdram_layout.h`. Aucun code fonctionnel n'est fourni.

## 1. Concurrence et atomicité

### Verrous et éléments protégés
* `sdram_ctx.state`, `sdram_ctx.last_error` et `sdram_ctx.bist_running` sont protégés par un **mutex ChibiOS (`mutex_t`) dédié au contexte**. Toute lecture/écriture passe par une section critique courte (`chMtxLock`/`chMtxUnlock`).
* `sdram_ctx.last_bist_result` est associé à `bist_running` : il est écrit à la fin d'un BIST sous le même mutex pour garantir la cohérence du triplet {state, last_error, bist_running}.

### Mécanisme choisi
* **Mutex** : garantit exclusion mutuelle entre threads init/audio/maintenance sans bloquer les ISR (API thread-only). Les sémaphores binaires sont réservés aux synchronisations d'événements, pas à la protection du contexte global.
* **Lock système (`chSysLock`)** : non utilisé ici pour éviter de bloquer l'ordonnanceur plus que nécessaire et pour rester compatible avec des sections critiques contenant des appels non-ISR-safe (journalisation BIST, accès MPU abstrait).

### Cas particuliers
* `sdram_get_region()` pendant un BIST :
  * Le mutex est pris pour lire `state` et `bist_running`.
  * Si `bist_running == true`, l'appel **échoue** immédiatement (`false`, base=0, size=0) pour éviter toute lecture concurrente des zones testées.
* `sdram_run_bist()` appelé deux fois :
  * Le mutex empêche les lancements concurrents. Si `bist_running == true`, le deuxième appel retourne `false` sans modifier l'état ni les timestamps.

### Diagramme textuel des accès concurrents
```
Thread init          Thread audio                 Thread maintenance (BIST)
- écrit state        - lit state                  - lit/écrit state
- écrit last_error   - lit last_error             - lit/écrit last_error
- écrit bist_running - interdit (lecture)        - lit/écrit bist_running
- écrit descriptors  - lit descriptors            - lit descriptors
- lit/writempu?      - aucune écriture contexte  - remplit last_bist_result
```
* Toutes les entrées/sorties sur `state/last_error/bist_running/last_bist_result` passent par le mutex du contexte.
* L'audio ne modifie jamais le contexte : il lit seulement `state` et obtient des régions via `sdram_get_region()`.

## 2. Machine d'états runtime

États : `SDRAM_NOT_INITIALIZED` → `SDRAM_INITIALIZING` → (`SDRAM_READY` | `SDRAM_DEGRADED` | `SDRAM_FAULT`).

### Transitions autorisées
* Boot : `NOT_INITIALIZED` → `INITIALIZING` (entrée `sdram_init`).
* Init réussie : `INITIALIZING` → `READY`.
* Init partiellement dégradée (p.ex. BIST rapide fail mais carte partiellement utilisable) : `INITIALIZING` → `DEGRADED`.
* Init fatale (FMC/refresh) : `INITIALIZING` → `FAULT`.
* BIST rapide échoue : `READY` → `DEGRADED` (zones audio conservées mais marquées non fiables).
* BIST complet échoue : `READY` → `FAULT` (contrat de sécurité, audio stoppé) ou `DEGRADED` → `FAULT` selon sévérité configurée ; par défaut passage en `FAULT` car couverture complète.
* BIST complet réussi depuis `DEGRADED` (voir §3) : transition conditionnelle `DEGRADED` → `READY` si autorisée par politique.
* Maintenance corrective : `FAULT` → `DEGRADED` interdit ; seul un **reset logiciel** autorise `FAULT` → `NOT_INITIALIZED` (reboot).
* Reset logiciel : toute state → `NOT_INITIALIZED` (reboot complet, pas de transition à chaud).

### Transitions interdites
* Saut direct `NOT_INITIALIZED` → `READY/DEGRADED/FAULT` sans passer par `INITIALIZING`.
* Retour spontané `READY` → `INITIALIZING` (pas de re-init à chaud).
* `DEGRADED` → `INITIALIZING` (nécessite reset).
* `FAULT` → `READY` ou `FAULT` → `DEGRADED` sans reset.

### Déclencheurs détaillés
* **Échec init (FMC/refresh)** : `INITIALIZING` → `FAULT`, `last_error = SDRAM_ERR_FMC_TIMEOUT/SDRAM_ERR_REFRESH`.
* **Échec BIST rapide** : si lancé en fin d'init, `INITIALIZING` → `DEGRADED`, `last_error = SDRAM_ERR_BIST_FAIL`.
* **Échec BIST complet** : depuis `READY` → `FAULT` (audio stoppé), depuis `DEGRADED` → `FAULT` (confirmation panne). `last_error = SDRAM_ERR_BIST_FAIL`.
* **Reset logiciel** : force réinitialisation globale ; lors du prochain `sdram_init`, l'état repart à `INITIALIZING`.

## 3. Politique READY ↔ DEGRADED ↔ FAULT

* **DEGRADED → READY** : autorisé **uniquement** si un BIST complet (mode `SDRAM_BIST_MODE_FULL`) réussit après isolement audio. Cette remontée valide la mémoire entière et réactive toutes les régions. Sans BIST complet (p.ex. uniquement BIST rapide), le retour en READY est interdit ; un reboot reste possible mais non obligatoire.
* **FAULT → READY** : **interdit** sans reboot. Le passage par `FAULT` signifie que la configuration FMC/MPU ou l'intégrité mémoire est invalide ; seule une réinitialisation logicielle et un nouveau `sdram_init` peuvent rétablir `READY`.

## 4. Gestion de `SDRAM_CACHE_RESIDUAL`

* Si `SDRAM_ENABLE_CACHE_RESIDUAL = 0` (valeur actuelle) :
  * Le descripteur **n'est pas présent** dans `sdram_region_descriptors[]` pour simplifier les parcours.
  * `sdram_get_region(SDRAM_CACHE_RESIDUAL)` retourne `false` avec `base = 0`, `size = 0`, `flags = 0` quelle que soit l'état (`READY/DEGRADED`), et ne modifie pas le contexte.
* Si `SDRAM_ENABLE_CACHE_RESIDUAL = 1` :
  * Le descripteur est présent, marqué `SDRAM_REGION_FLAG_CACHEABLE | SDRAM_REGION_FLAG_CPU_ONLY | SDRAM_REGION_FLAG_OPTIONAL`.
  * En `DEGRADED`, il peut être masqué si la portion testée a échoué ; l'appel renvoie `false` et base/size=0 mais `flags` reste cohérent (OPTIONAL).

## 5. Séquencement BIST ↔ audio

* **Droit de lancer un BIST complet** : uniquement le thread maintenance dédié (priorité basse) après coordination explicite avec l'audio. Les BIST rapides peuvent être lancés par `sdram_init`.
* **Conditions préalables** :
  * Audio stoppé ou buffers détachés de la SDRAM (SAI/DMA pointant vers SRAM D2) avant un BIST complet.
  * Pendant le BIST complet, toute requête `sdram_get_region()` échoue (voir §1) : l'audio ne peut pas réacquérir les buffers.
* **BIST demandé alors que l'audio est actif** :
  * L'appelant reçoit `false` si `bist_running` ou si l'état audio déclaré bloque la séquence. Optionnellement, un hook de coordination peut signaler au moteur audio de se réarmer ; le BIST ne démarre qu'une fois l'audio détaché.
  * Aucun arrêt forcé dans le driver SDRAM : la responsabilité de désactiver le flux audio incombe au caller avant de relancer `sdram_run_bist()`.

## 6. Esquisse de structure `sdram_driver.c` (sans code)

```
sdram_driver.c
  - Section includes
    Rôle : importer `ch.h`, `hal.h` (pour types FMC/MPU), headers SDRAM. Pas de CMSIS direct.
    Contraintes : inclure uniquement ce qui est nécessaire pour rester déterministe et testable.

  - Contexte global
    Rôle : définition de `sdram_driver_ctx_t sdram_ctx`, mutex de contexte, tableaux de descripteurs et constantes internes.
    Dépendances : `ch.h` pour mutex, `sdram_layout.h` pour descriptors.
    Temps réel : accès protégés, sections critiques minimales.

  - Init state machine
    Rôle : implémenter `sdram_init()` avec transitions d'états, paramétrage `last_error`, lancement BIST rapide optionnel.
    Dépendances : helpers FMC/MPU, BIST quick.
    Temps réel : thread-only, séquence bloquante mais unique au boot.

  - FMC init wrapper
    Rôle : encapsuler `sdram_hw_init_sequence()` (LLD/hal abstrait), gérer timeouts/erreurs et mise à jour de l'état.
    Dépendances : HAL SDRAM/FMC via wrappers fournis par ChibiOS.
    Temps réel : exécution monothread, aucune attente active prolongée.

  - MPU config
    Rôle : appeler `sdram_configure_mpu_regions()` pour marquer les zones non-cacheables (audio) et la zone cacheable optionnelle.
    Dépendances : fonctions MPU de ChibiOS, constantes `SDRAM_ENABLE_CACHE_RESIDUAL`.
    Temps réel : configuration unique au boot ; interdit en présence de threads audio actifs.

  - BIST engine
    Rôle : implémenter `sdram_bist_start()` et la boucle de motifs (quick/full) en thread maintenance. Gérer timestamps et `last_bist_result`.
    Dépendances : primitives mémoire (`memset`, accès SDRAM), `chVTGetSystemTimeX` pour timestamps.
    Temps réel : interdit aux ISR, consommation CPU contrôlée, peut céder (`chThdYield`) entre patterns.

  - API publique wrappers
    Rôle : implémenter `sdram_status`, `sdram_get_error`, `sdram_run_bist`, `sdram_get_region` en appliquant les politiques de mutex et de validation d'état.
    Dépendances : mutex de contexte, tables de descripteurs.
    Temps réel : sections critiques courtes, aucune allocation dynamique.
```

