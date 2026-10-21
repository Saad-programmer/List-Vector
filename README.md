# Documentation complète : benchmark `std::vector` vs `std::list` (vs `std::deque`)

Auteur du code d'origine : Baptiste Wicht (licence MIT) — fichier `bench.cpp`.
Document de référence pour l'analyse : article « Performance of vector vs list » (mesures sur Intel Core i7 Q820 @ 1,73 GHz, GCC 4.7.2, `-O2 -march=native -std=c++11`).

> **Avertissements de lecture (à lire avant le reste)**
> 1. `bench.cpp` dépend de `bench.hpp` et `policies.hpp`, qui n'ont **pas** été fournis. Le comportement des politiques (`FillBack`, `Find`, `Insert`, etc.) est donc **déduit de leurs noms, de la structure du code et de l'article**. Ces passages sont signalés par « (déduit) ».
> 2. Les chiffres cités viennent de **l'article** (ancienne version du benchmark, matériel de 2010, tailles de collections différentes de celles de `bench.cpp` actuel). Ils servent à illustrer les ordres de grandeur et les tendances, pas à prédire vos propres mesures.
> 3. Certains tableaux de l'article sont manifestement mal étiquetés (voir section 8). Je ne les utilise pas pour tirer de conclusions.

---

## Table des matières

1. Problématique et objectif
2. Théorie : complexité vs réalité matérielle
3. Architecture du projet
4. Le code de `bench.cpp`, pas à pas
5. Les benchmarks, un par un (théorie, pratique, résultats)
6. Tableau comparatif global
7. Guide de décision
8. Limites, erreurs et pièges de la méthodologie
9. Compiler, exécuter, reproduire
10. Conclusion

---

## 1. Problématique et objectif

En C++, `std::vector` et `std::list` sont les deux conteneurs séquentiels les plus connus. La sagesse populaire dit :

- **`list`** pour les insertions/suppressions au milieu (O(1) une fois l'itérateur connu) ;
- **`vector`** pour tout le reste.

Or la complexité asymptotique ignore le **coût réel d'accès à la mémoire**. Le benchmark répond à la question :

> Dans la pratique, sur du matériel moderne, pour quelles charges de travail et quelles tailles d'éléments `list` est-il réellement meilleur que `vector` ?

La version actuelle du code ajoute `std::deque` comme troisième concurrent, et teste des types d'éléments variés (trivials de 8 octets à 4 Ko, types non triviaux avec `std::string`, déplaçables ou non, `noexcept` ou non).

---

## 2. Théorie : complexité vs réalité matérielle

### 2.1 Complexités théoriques

| Opération | `vector` | `list` | `deque` |
|---|---|---|---|
| `push_back` | O(1) amorti | O(1) | O(1) |
| `push_front` | O(n) | O(1) | O(1) |
| Accès par indice | O(1) | O(n) | O(1) |
| Recherche linéaire | O(n) | O(n) | O(n) |
| Insertion/suppression au milieu (itérateur connu) | O(n) | O(1) | O(n) |
| Tri | O(n log n) | O(n log n) | O(n log n) |
| Destruction | O(n) si destructeur non trivial, sinon ~O(1) | O(n) (un `delete` par nœud) | O(n) / par blocs |

Sur le papier, la recherche est équivalente et `list` gagne sur l'insertion. C'est ce que le benchmark remet en cause.

### 2.2 Les trois facteurs qui font dérailler la théorie

**a) Le cache CPU et la localité spatiale.**
Quand le processeur lit une adresse, il charge une **ligne de cache entière** (typiquement 64 octets). Dans un `vector`, les éléments sont contigus : lire l'élément *i* amène gratuitement les éléments suivants dans le cache, et le *prefetcher* matériel anticipe le parcours. Dans une `list`, chaque nœud est alloué séparément : chaque saut de pointeur est potentiellement un **cache miss** (accès à la RAM, ordre de 100 cycles ou plus, contre ~4 pour le cache L1).

**b) Le coût de l'allocation.**
`list` fait **une allocation par élément** (nœud = élément + 2 pointeurs `prev`/`next`, soit 16 octets de surcharge en 64 bits). `vector` alloue rarement (croissance géométrique), donc en amorti beaucoup moins.

**c) La taille et la nature de l'élément.**
Déplacer un élément dans un `vector` (décalage lors d'une insertion, échange lors d'un tri) coûte proportionnellement à sa taille (ou à son coût de déplacement pour un type non trivial). Dans une `list`, l'élément ne bouge jamais : seuls des pointeurs sont modifiés. Plus l'élément est gros ou cher à copier, plus `list` regagne du terrain.

Ces trois facteurs s'opposent : le cache et les allocations favorisent `vector`, le coût de déplacement des gros éléments favorise `list`. Le benchmark mesure le point d'équilibre.

---

## 3. Architecture du projet

```
bench.cpp        -> définit les types testés, les benchmarks, et main()
bench.hpp        -> (non fourni) moteur : bench<>(), bench_types<>(), new_graph<>(), graphs::output()
policies.hpp     -> (non fourni) politiques : Empty, FilledRandom, FillBack, Find, Insert, ...
plf_timsort.h    -> inclus, non utilisé dans le fichier fourni
plf_colony.h     -> inclus, non utilisé dans le fichier fourni
```

Le design repose sur des **politiques (policy-based design)** : un benchmark = un conteneur + une unité de temps + une politique d'initialisation + une politique d'opération.

```cpp
bench<Conteneur, Unité, PolitiqueInit, PolitiqueOpération...>("nom_courbe", tailles);
```

Exemple : `bench<std::vector<T>, microseconds, Empty, ReserveSize, FillBack>("vector_reserve", sizes)` :
- conteneur : `std::vector<T>` ;
- unité : microsecondes ;
- initialisation : `Empty` (conteneur vide au départ) ;
- opérations : `ReserveSize` (appelle `reserve(n)` d'abord) puis `FillBack` (ajoute n éléments avec `push_back`) ;
- `sizes` : liste des valeurs de n testées.

---

## 4. Le code de `bench.cpp`, pas à pas

### 4.1 Inclusions

| Header | Rôle |
|---|---|
| `<vector>`, `<list>`, `<deque>` | Conteneurs comparés |
| `<random>` | Génération/mélange aléatoire (utilisé par les politiques) |
| `<array>` | Remplissage des types de taille paramétrée |
| `<algorithm>` | `std::find`, `std::sort`, `std::remove`, etc. |
| `<thread>`, `<iostream>`, `<cstdint>`, `<typeinfo>`, `<memory>` | Utilitaires du moteur (`<memory>` pour les `unique_ptr` de `SmartFilled`/`SmartDelete`, déduit) |
| `plf_timsort.h`, `plf_colony.h` | Inclus mais **inutilisés** dans ce fichier (reliquat ou extension prévue) |

Remarque : `std::string` est utilisé sans inclure `<string>` explicitement ; cela compile grâce à des inclusions transitives, ce qui n'est pas garanti par le standard.

### 4.2 Les traits `constexpr` (vérification d'invariants)

Ces fonctions servent uniquement aux `static_assert` : elles garantissent à la compilation que chaque type de test possède bien les propriétés annoncées. Un benchmark dont les types ne sont pas ce qu'on croit est un benchmark faux.

| Fonction | Vérifie |
|---|---|
| `is_trivial_of_size<T>(size)` | `T` est trivial **et** `sizeof(T) == size` |
| `is_non_trivial_of_size<T>(size)` | `T` est non trivial, de la bonne taille, copiable et déplaçable (constructeur + affectation) |
| `is_non_trivial_nothrow_movable<T>()` | Non trivial, constructeur et affectation de déplacement `noexcept` |
| `is_non_trivial_non_nothrow_movable<T>()` | Non trivial, déplaçable, mais **pas** `noexcept` |
| `is_non_trivial_non_movable<T>()` | Non trivial, copiable mais non déplaçable (défini, non utilisé ici) |
| `is_small<T>()` | `sizeof(T) <= sizeof(std::size_t)`, c.-à-d. 8 octets ; sert à exclure `vector` des tests trop lents (voir 5.3) |

Pourquoi `noexcept` compte : lors d'une réallocation, `std::vector` utilise `std::move_if_noexcept`. Si le constructeur de déplacement n'est pas `noexcept` **et** qu'un constructeur de copie existe, le `vector` **copie** les éléments (pour conserver la garantie forte d'exception). C'est la raison d'être de la paire `NonTrivialStringMovable` / `NonTrivialStringMovableNoExcept`.

### 4.3 Les types testés

#### `Trivial<N>` : type trivial de taille N octets

```cpp
template<int N>
struct Trivial {
    std::size_t a;                              // clé de comparaison (8 octets)
    std::array<unsigned char, N-sizeof(a)> b;   // remplissage pour atteindre N octets
    bool operator<(const Trivial &other) const { return a < other.a; }
};
```
- `a` sert à la comparaison (`operator<`), `b` n'est que du « poids mort » pour atteindre la taille voulue.
- Spécialisation `Trivial<8>` : sans tableau (un `std::array` de taille 0 serait inutile ici).
- La copie est un simple `memcpy` : le coût est proportionnel à N, sans surcoût de logique.

Instances : `TrivialSmall` (8 o), `TrivialMedium` (32 o), `TrivialLarge` (128 o), `TrivialHuge` (1 Ko), `TrivialMonster` (4 Ko).

#### `NonTrivialStringMovable` : type avec `std::string`

- Contient une `std::string` de plus de 15 caractères, volontairement pour **dépasser le SSO** (Small String Optimization) : copier la chaîne nécessite alors une vraie allocation dynamique, donc c'est coûteux.
- Destructeur déclaré (`= default`) : **cela supprime la génération implicite du constructeur/opérateur de déplacement**. Le « déplacement » retombe donc sur la **copie**. Le commentaire du code dit « easy to move », mais en réalité ce type est **copié** lors des déplacements. C'est pour cela que le `static_assert` `is_non_trivial_non_nothrow_movable` passe : `is_move_constructible` est vrai (la copie accepte une rvalue) mais pas `nothrow`.
- Taille typique avec libstdc++ : 32 (string) + 8 (`a`) = 40 octets.

#### `NonTrivialStringMovableNoExcept` : même idée, déplacement vrai et `noexcept`

- Définit explicitement constructeur de déplacement `noexcept = default` et affectation par déplacement `noexcept` (via `std::swap` des membres).
- Le `vector` peut donc **déplacer** (échange de pointeurs internes de la string) lors des réallocations, au lieu de copier.
- Permet de mesurer l'impact de `noexcept` sur les performances du `vector`.

#### `NonTrivialArray<N>` : non trivial, copie coûteuse par la taille

- `a` (clé) + `std::array<unsigned char, N-8>` privé.
- Non trivial à cause de l'initialiseur de membre `a = 0` et du constructeur par défaut fourni.
- Instancié en `NonTrivialArray<32>` (32 octets) : permet de comparer un type non trivial et un type trivial de **même taille** (`TrivialMedium`).

### 4.4 Les structures de benchmark

Chaque `bench_xxx<T>` est un *template struct* avec une méthode statique `run()` :
1. `new_graph<T>("nom", "unité")` crée un graphique pour ce type et cette opération ;
2. définit la liste `sizes` ;
3. appelle `bench<...>` une fois par conteneur/variante.

### 4.5 Orchestration

```cpp
template<typename ...Types>
void bench_all(){
    bench_types<bench_fill_back, Types...>();   // lance bench_fill_back<T>::run() pour chaque T
    ...
    bench_types<bench_number_crunching, TrivialSmall, TrivialMedium>();  // seulement 2 types : trop lent
}
```

`main()` instancie `bench_all` avec 8 types, puis `graphs::output(graphs::Output::GOOGLE)` génère les graphiques (sortie Google Charts, déduit).

Au total : 9 benchmarks × 8 types + 1 benchmark × 2 types, soit 74 graphiques.

---

## 5. Les benchmarks, un par un

Les chiffres ci-dessous proviennent de l'article (sauf mention). `n` = nombre d'éléments.

### 5.1 `bench_fill_back` : remplissage par `push_back`

**Code** : tailles de 100 000 à 1 000 000. Variantes : `vector`, `list`, `deque`, `vector_reserve` (avec `reserve(n)` préalable), et trois versions avec `std::back_inserter` (`*_inserter`).

**Théorie** : `vector` amorti O(1) mais réalloue et recopie/déplace ~log₂(n) fois. `reserve` supprime ces réallocations. `list` fait n allocations. `deque` alloue par blocs.

**Pratique et résultats (article, élément de 1 Ko, ms)** :

| n | vector_pre | vector | list |
|---|---|---|---|
| 1 000 | 0 | 9 | 1 |
| 10 000 | 12 | 245 | 18 |
| 100 000 | 949 | 2 635 | 1 153 |
| 1 000 000 | 9 138 | 23 654 | 11 270 |

(Ce tableau correspond au graphique 1 024 octets, dont l'axe monte à 25 000 ms.)

**Analyse** :
- `vector` avec `reserve` est le plus rapide : une seule allocation.
- Sans `reserve`, le `vector` est ~2,6× plus lent que `vector_pre` à 1 Ko, car chaque réallocation recopie de gros éléments.
- Pour un élément de 1 Ko, `list` est plus rapide que `vector` **sans** `reserve` (11 270 vs 23 654 ms) : la copie lors de la réallocation coûte plus cher que l'allocation par nœud.
- Note : l'article affirme que `vector_pre` est « un ordre de grandeur plus rapide » que `list` ; ses propres chiffres 1 Ko (9 138 vs 11 270) ne le montrent pas. L'écart est bien plus marqué pour de petits éléments (graphique 8 octets).
- Les types non triviaux illustrent l'effet de `noexcept` : sans lui, les réallocations **copient** les `std::string` (allocations en cascade) ; avec lui, elles les **déplacent**.

### 5.2 `bench_emplace_back` : `emplace_back`

Même scénario que 5.1 avec construction sur place (pas de copie temporaire). Pas de variantes `reserve` ni `inserter`. Mesure le gain de la construction directe, surtout visible sur les types non triviaux.

### 5.3 `bench_fill_front` : insertion en tête

**Code** : tailles de 10 000 à 100 000 (réduites car très lent). `vector` n'est testé que si `is_small<T>()`, c.-à-d. pour les types de 8 octets : avec de gros éléments, l'exécution serait interminable.

**Théorie** : chaque `insert(begin(), x)` décale **tous** les éléments : O(n) par insertion, O(n²) au total pour `vector`. O(1) pour `list` et `deque`.

**Résultats (article, 8 octets, ms)** :

| n | vector | list |
|---|---|---|
| 10 000 | 40 | 0 |
| 50 000 | 949 | 2 |
| 100 000 | 3 937 | 4 |
| 200 000 | 16 003 | 9 |
| 300 000 | 42 393 | 12 |

**Analyse** : croissance quadratique nette pour `vector` (×4 quand n double). À n = 300 000, `list` est ~3 500× plus rapide. C'est le pire cas pour `vector`. `deque`, conçu pour cela, est l'alternative idéale. Aucun intérêt à tester de plus gros éléments : ce serait encore pire.

### 5.4 `bench_emplace_front` : `emplace_front`

Variante par construction sur place de 5.3, avec la même restriction `is_small<T>()` pour `vector`. Conclusions identiques.

### 5.5 `bench_linear_search` : recherche linéaire

**Code** : tailles de 1 000 à 10 000. Initialisation `FilledRandom` (valeurs [0, N] mélangées). Opération `Find` : chaque nombre de [0, N] est recherché avec `std::find` (déduit).

**Théorie** : O(n) pour tous, donc a priori équivalents. Complexité totale : O(n²) pour chercher les n valeurs.

**Résultats (article, 8 octets, µs)** :

| n | vector | list |
|---|---|---|
| 100 | 0 | 11 |
| 1 000 | 0 | 1 545 |
| 5 000 | 0 | 35 886 |
| 10 000 | 0 | 150 865 |
| 20 000 | 0 | 614 496 |

À 1 Ko : `list` atteint 3 356 432 µs à n = 20 000, `vector` reste proche de 0 à l'échelle du graphique.

**Analyse** : le cas emblématique de la **localité de cache**. Le `vector` est parcouru séquentiellement (lignes de cache + prefetching) ; la `list`, dont les nœuds sont éparpillés après le mélange, enchaîne des cache misses. L'écart dépasse plusieurs ordres de grandeur, et la taille de l'élément n'inverse jamais la tendance (elle ralentit même les deux, car moins d'éléments tiennent dans une ligne de cache).

### 5.6 `bench_random_insert` : insertion à position aléatoire

**Code** : tailles de 10 000 à 100 000, unité ms. Conteneur rempli aléatoirement, puis 1 000 insertions à une position aléatoire. **La position est trouvée par recherche linéaire** (O(n) pour les deux), puis suivie de l'insertion.

**Théorie** : `list` gagne sur l'insertion (O(1) vs O(n)), mais la recherche domine et pénalise `list`.

**Résultats (article, ms, n = 1 000 à 10 000)** :

| n | vector 8 o | list 8 o | vector 32 o | list 32 o | vector 1 Ko | list 1 Ko |
|---|---|---|---|---|---|---|
| 1 000 | 9 | 85 | 27 | 120 | 1 821 | 167 |
| 10 000 | 14 | 106 | 47 | 155 | 3 308 | 228 |

**Analyse** :
- **8 octets** : `vector` ~7,6× plus rapide (n = 10 000). Copier de petits éléments contigus coûte moins que parcourir la liste.
- **32 octets** : l'écart se réduit (~3,3×) mais `vector` reste devant.
- **1 Ko** : le rapport s'inverse, `list` ~14,5× plus rapide (n = 10 000). Le décalage de gros éléments devient dominant.
- Le temps de `list` varie peu avec la taille de l'élément : seule la recherche dépend de n, et il y a peu de recherches (1 000).
- Le point de croisement se situe vers 64 octets sur la machine de l'auteur.
- Si l'itérateur était déjà connu (aucune recherche), `list` gagnerait nettement plus tôt : c'est le seul scénario où son O(1) s'exprime pleinement.

### 5.7 `bench_random_remove` : suppression à position aléatoire

**Code** : même schéma que 5.6 avec la politique `Erase`. Variante supplémentaire `vector_rem` avec `RemoveErase` (idiome `erase(remove(...))`, déduit), qui compacte en un seul passage au lieu de décaler à chaque suppression.

**Théorie** : symétrique de l'insertion.

**Résultats** : l'article affirme que le comportement est le même que pour l'insertion (petits éléments : `vector` gagne ; gros éléments : `list` gagne). **Mais les tableaux de cette section de l'article sont erronés** (voir section 8), je ne cite donc pas de chiffres.

### 5.8 `bench_sort` : tri

**Code** : tailles de 100 000 à 1 000 000. `vector` et `deque` : `std::sort` ; `list` : `list::sort()` (déduit, d'après l'article).

**Théorie** : O(n log n) partout. Mais `std::sort` échange des éléments (≥ 3 copies par échange pour un type non déplaçable) alors que `list::sort` ne fait que relier des pointeurs.

**Résultats (article, ms)** :

| n | vector 8 o | list 8 o | vector 1 Ko | list 1 Ko |
|---|---|---|---|---|
| 10 000 | 2 | 26 | 224 | 50 |
| 100 000 | 163 | 684 | 4 289 | 1 083 |
| 1 000 000 | 2 147 | 15 950 | 50 973 | 17 975 |

**Analyse** :
- **8 octets** : `vector` ~7,4× plus rapide à 1 M d'éléments (cache et prédiction de branchement).
- **1 Ko** : `list` ~2,8× plus rapide, car ses temps varient peu avec la taille (aucune copie d'éléments) alors que ceux du `vector` explosent (copies de 1 Ko).
- Le benchmark teste aussi les types non triviaux : un déplacement `noexcept` bon marché rapproche `vector` de ses meilleures performances.

### 5.9 `bench_destruction` : destruction du conteneur

**Code** : initialisation `SmartFilled` et opération `SmartDelete` (déduit : conteneur alloué dynamiquement via pointeur intelligent, puis libéré, le temps mesuré étant celui de la destruction).

**Théorie** : `vector<Trivial>` libère un seul bloc (destruction quasi gratuite, pas de destructeurs à appeler). `list` doit appeler `delete` sur chaque nœud (n libérations). Pour les types non triviaux, les destructeurs de `std::string` ajoutent n libérations.

**Résultats** : non fournis dans l'article (non testé dans sa version). Attendu : `vector` ≪ `deque` < `list` pour les types triviaux.

### 5.10 `bench_number_crunching` : insertion triée (calcul intensif)

**Code** : tailles de 10 000 à 100 000, **uniquement** `TrivialSmall` et `TrivialMedium` (trop lent pour le reste). Politique `RandomSortedInsert` : on insère des valeurs aléatoires en maintenant le conteneur trié ; la position est trouvée par parcours, puis on insère.

**Résultats (article, 8 octets, ms)** :

| n | vector | list |
|---|---|---|
| 10 000 | 45 | 166 |
| 50 000 | 928 | 10 665 |
| 100 000 | 3 753 | 50 766 |
| 200 000 | 15 185 | 231 480 |
| 300 000 | 34 293 | 715 892 |

**Analyse** : `vector` est ~21× plus rapide à n = 300 000 et l'écart se creuse. Parcourir une liste coûte bien plus cher que décaler de petits éléments. Un `std::lower_bound` sur le `vector` (recherche dichotomique, impossible sur `list` en O(log n)) améliorerait encore le résultat.

---

## 6. Tableau comparatif global

| Charge de travail | Petits éléments (≤ 32 o) | Gros éléments (≥ 1 Ko) | Remarque |
|---|---|---|---|
| Remplissage `push_back` | `vector` (avec `reserve` idéalement) | `vector` avec `reserve`, sinon `list` possible | L'allocation domine |
| Insertion en tête | `deque` / `list` | `deque` / `list` | `vector` quadratique |
| Recherche linéaire | **`vector`** (×100 à ×1000+) | **`vector`** | Effet de cache |
| Insertion/suppression aléatoire avec recherche | **`vector`** | **`list`** (~14× à 1 Ko) | Croisement vers 64 o |
| Tri | **`vector`** (~7×) | **`list`** (~3× à 1 Ko) | `list` ne copie pas |
| Destruction | `vector` | `vector` (types triviaux) | `list` : n libérations |
| Insertion triée | **`vector`** (~20×) | non testé | Parcours coûteux en `list` |

---

## 7. Guide de décision

1. **Par défaut, `std::vector`.** Appelez `reserve()` si vous connaissez la taille approximative.
2. **Insertions/suppressions fréquentes en tête** : `std::deque` (ou `list`).
3. **Insertions/suppressions au milieu** : `list` seulement si les deux conditions suivantes sont réunies :
   - les éléments sont gros (de l'ordre de 64 octets ou plus) ou très coûteux à déplacer ;
   - vous détenez **déjà** des itérateurs (pas de recherche linéaire).
   Sinon `vector` (ou, pour un ensemble trié, un conteneur associatif ou un `vector` trié avec `lower_bound`).
4. **Gros éléments à réordonner** (tri, permutations) : envisager `list`, ou plus souvent un `vector<T*>` / `vector` d'indices pour éviter de copier les gros objets.
5. **Types non triviaux** : rendre le constructeur et l'affectation de déplacement `noexcept`, sinon `vector` copie lors des réallocations.
6. **Toujours mesurer** sur votre matériel et vos données : le point de croisement dépend du cache, de l'allocateur et du compilateur.

---

## 8. Limites, erreurs et pièges de la méthodologie

**Dans l'article**
- La section « Random Remove » reprend les tableaux et titres d'autres sections : elle contient un tableau intitulé « Push front (8 bytes) » et un autre intitulé « Sort (8 bytes) », identiques à ceux des sections Push Front et Sort. Les vraies données de suppression sont donc absentes.
- Le texte sur `fill` annonce « un ordre de grandeur » entre `vector_pre` et `list`, ce que le tableau 1 Ko ne confirme pas (9 138 vs 11 270 ms).
- Le tableau de remplissage placé sous les deux graphiques ne correspond qu'au cas 1 024 octets.
- Matériel et compilateur anciens (2010, GCC 4.7.2) : les rapports ont évolué (allocateurs, prefetchers, `-O2` différent).

**Dans `bench.cpp`**
- `plf_timsort.h` et `plf_colony.h` sont inclus mais non utilisés dans ce fichier.
- `<string>` non inclus explicitement.
- Le commentaire de `NonTrivialStringMovable` (« easy to move ») est trompeur : sans déplacement implicite (destructeur déclaré), c'est une copie.
- `is_non_trivial_non_movable` est défini mais jamais utilisé.

**Méthodologiques**
- Les temps dépendent de la fragmentation du tas : une `list` fraîchement remplie séquentiellement a des nœuds proches en mémoire ; **mélangée** (`FilledRandom`), elle perd cette localité, ce qui est le cas défavorable réaliste.
- Aucune statistique (moyenne, écart-type, répétitions) n'apparaît dans le code visible ; vérifier dans `bench.hpp`.
- Un seul compilateur, une seule machine, un seul allocateur.
- Les résultats pour `n` très grand sortent du cache L2/L3 et peuvent changer de régime.

---

## 9. Compiler, exécuter, reproduire

```bash
# Exemple (adapter les chemins ; bench.hpp, policies.hpp, plf_*.h doivent être présents)
g++ -std=c++14 -O2 -march=native -pthread bench.cpp -o bench
./bench > results.html     # la sortie dépend de graphs::output (GOOGLE : page Google Charts)
```

Remarques :
- C++11 minimum ; C++14 recommandé (certains `constexpr` du code utilisent des fonctions de type-traits modernes).
- Prévoir un temps d'exécution **long** (74 graphiques, tailles jusqu'à 10⁶, `Monster` de 4 Ko × 10⁶ = ~4 Go de mémoire pour certains tests : vérifier la RAM disponible ou réduire les tailles).
- Pour des mesures fiables : désactiver le mode économie d'énergie, fixer la fréquence CPU, fermer les autres applications, répéter les exécutions.

---

## 10. Conclusion

Le benchmark démontre que **la complexité algorithmique seule est un mauvais guide** :

- `vector` domine dès que l'accès mémoire séquentiel compte (recherche, parcours, insertion triée, tri de petits éléments) grâce au cache et au prefetching.
- `list` ne devient pertinente que quand les éléments sont **volumineux** et qu'on évite les recherches (insertion/suppression au milieu, tri sans copie).
- `deque` est le bon choix quand on insère aux deux extrémités.
- Les détails d'implémentation des types (`noexcept`, déplacement, taille, SSO) changent sensiblement les résultats.

Règle pratique : **commencer par `vector`, mesurer, ne changer que si les mesures le justifient.**
