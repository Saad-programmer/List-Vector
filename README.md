📊 Benchmark std::vector vs std::list (vs std::deque)
<div align="center">
Documentation technique complète

Auteur du code d'origine : Baptiste Wicht (licence MIT) — fichier bench.cpp

Référence : article « Performance of vector vs list »

Matériel de référence : Intel Core i7 Q820 @ 1,73 GHz · GCC 4.7.2 · -O2 -march=native -std=c++11

</div>
⚠️ Avertissements de lecture
1. bench.cpp dépend de bench.hpp et policies.hpp, non fournis. Le comportement des politiques (FillBack, Find, Insert, etc.) est déduit de leurs noms, de la structure du code et de l'article. Ces passages sont signalés par « (déduit) ».

2. Les chiffres cités viennent de l'article (ancienne version du benchmark, matériel de 2010, tailles différentes de bench.cpp actuel). Ils illustrent des ordres de grandeur et tendances, pas vos propres mesures.

3. Certains tableaux de l'article sont mal étiquetés (voir §8). Ils ne sont pas utilisés pour conclure.

🗂️ Sommaire
#	Section	#	Section
1	Problématique et objectif	6	Tableau comparatif global
2	Théorie : complexité vs réalité	7	Guide de décision
3	Architecture du projet	8	Limites et pièges
4	Le code pas à pas	9	Compiler & reproduire
5	Les benchmarks un par un	10	Conclusion
🎯 1. Problématique et objectif
En C++, std::vector et std::list sont les deux conteneurs séquentiels les plus connus. La sagesse populaire dit :

Conteneur	Usage recommandé
🟦 list	Insertions/suppressions au milieu — O(1) une fois l'itérateur connu
🟩 vector	Tout le reste
❓ Question centrale

Dans la pratique, sur du matériel moderne, pour quelles charges de travail et quelles tailles d'éléments list est-il réellement meilleur que vector ?

La version actuelle du code ajoute std::deque comme troisième concurrent et teste des types d'éléments variés :

triviaux de 8 octets à 4 Ko ;

types non triviaux avec std::string ;

déplaçables ou non ;

noexcept ou non.

🧠 2. Théorie : complexité vs réalité matérielle
2.1 Complexités théoriques
Opération	vector	list	deque
push_back	O(1) amorti	O(1)	O(1)
push_front	O(n)	O(1)	O(1)
Accès par indice	O(1)	O(n)	O(1)
Recherche linéaire	O(n)	O(n)	O(n)
Insertion/suppression milieu (itérateur connu)	O(n)	O(1)	O(n)
Tri	O(n log n)	O(n log n)	O(n log n)
Destruction	O(n) si non trivial, sinon ~O(1)	O(n) (un delete/nœud)	O(n) par blocs
Sur le papier, la recherche est équivalente et list gagne sur l'insertion. C'est ce que le benchmark remet en cause.

2.2 Les trois facteurs qui font dérailler la théorie
<table> <tr> <td width="33%" valign="top">
🚀 a) Cache CPU & localité
Le processeur charge une ligne entière (64 octets) à chaque accès.

vector : contigu → prefetching, cache hits gratuits.

list : nœuds éparpillés → cache miss à chaque saut (~100 cycles vs ~4 en L1).

</td> <td width="33%" valign="top">
💰 b) Coût d'allocation
list : 1 allocation par élément + 16 octets de surcharge (prev/next en 64 bits).

vector : croissance géométrique → allocations rares et amorties.

</td> <td width="33%" valign="top">
📦 c) Taille de l'élément
vector : déplacer un élément coûte proportionnel à sa taille.

list : l'élément ne bouge jamais, seuls les pointeurs changent.

Plus l'élément est gros → plus list regagne du terrain.

</td> </tr> </table>
⚖️ Équilibre
Ces trois facteurs s'opposent : cache et allocations favorisent vector ; coût de déplacement des gros objets favorise list. Le benchmark mesure le point d'équilibre.

🏗️ 3. Architecture du projet
Fichier	Rôle
📄 bench.cpp	Définit les types testés, les benchmarks, et main()
⚙️ bench.hpp	(non fourni) Moteur : bench<>(), bench_types<>(), new_graph<>(), graphs::output()
🎛️ policies.hpp	(non fourni) Politiques : Empty, FilledRandom, FillBack, Find, Insert, …
📦 plf_timsort.h	Inclus, non utilisé
📦 plf_colony.h	Inclus, non utilisé
Design par politiques (policy-based design) : un benchmark = conteneur + unité de temps + politique d'initialisation + politique d'opération.

cpp
bench<Conteneur, Unite, PolitiqueInit, PolitiqueOperation...>("nom_courbe", taille)
Exemple concret :

cpp
bench<std::vector<T>, microseconds, Empty, ReserveSize, FillBack>("vector_reserve", sizes)
Paramètre	Valeur
Conteneur	std::vector<T>
Unité	microseconds
Init	Empty (conteneur vide)
Opérations	ReserveSize (appelle reserve(n)) → FillBack (push_back × n)
sizes	Liste des *n* testés
🔬 4. Le code de bench.cpp, pas à pas
4.1 Inclusions
Header	Rôle
<vector> <list> <deque>	Conteneurs comparés
<random>	Génération / mélange aléatoire
<array>	Remplissage des types à taille paramétrée
<algorithm>	std::find, std::sort, std::remove, …
<thread> <iostream> <cstdint> <typeinfo> <memory>	Utilitaires moteur (déduit)
plf_timsort.h plf_colony.h	Inclus, inutilisés
💡 Remarque — std::string est utilisé sans #include <string>. Cela compile par inclusions transitives, non garanti par le standard.

4.2 Les traits constexpr (vérification d'invariants)
Ces fonctions alimentent des static_assert : elles garantissent à la compilation que chaque type de test possède les propriétés annoncées. Un benchmark dont les types ne sont pas ce qu'on croit est un benchmark faux.

Trait	Vérifie que…
is_trivial_of_size<T>(size)	T est trivial et sizeof(T) == size
is_non_trivial_of_size<T>(size)	T non trivial, bonne taille, copiable + déplaçable
is_non_trivial_nothrow_movable<T>()	Non trivial, constructeur/affectation de déplacement noexcept
is_non_trivial_non_nothrow_movable<T>()	Non trivial, déplaçable, pas noexcept
is_non_trivial_non_movable<T>()	Non trivial, copiable, non déplaçable (défini, non utilisé)
is_small<T>()	sizeof(T) <= sizeof(std::size_t) (8 o) — exclut vector des tests trop lents (§5.3)
💡 Pourquoi noexcept compte
Lors d'une réallocation, std::vector utilise std::move_if_noexcept. Si le constructeur de déplacement n'est pas noexcept et qu'un constructeur de copie existe, le vector copie les éléments (garantie forte d'exception). C'est la raison d'être de la paire NonTrivialStringMovable / NonTrivialStringMovableNoExcept.

4.3 Les types testés
4.3.1 Trivial<N> — type trivial de N octets
cpp
template<int N>
struct Trivial {
    std::size_t a;                              // clé de comparaison (8 octets)
    std::array<unsigned char, N - sizeof(a)> b; // remplissage pour atteindre N octets

    bool operator<(const Trivial& other) const {
        return a < other.a;
    }
};
Point	Détail
🔑 a	Sert à la comparaison (operator<)
🪨 b	« Poids mort » pour atteindre la taille N
🔧 Trivial<8>	Spécialisation sans tableau
📋 Copie	Simple memcpy → coût ∝ N, sans logique
Instances : TrivialSmall (8 o) · TrivialMedium (32 o) · TrivialLarge (128 o) · TrivialHuge (1 Ko) · TrivialMonster (4 Ko)

4.3.2 NonTrivialStringMovable — type avec std::string
Aspect	Détail
📏 std::string	> 15 caractères → dépasse le SSO → vraie allocation dynamique à la copie
⚠️ Destructeur	= default → supprime la génération implicite du déplacement
🔄 « Déplacement »	Retombe sur la copie (commentaire trompeur « easy to move »)
✅ static_assert	is_non_trivial_non_nothrow_movable passe (move_constructible via copy, pas nothrow)
📐 Taille	32 (string) + 8 (a) = 40 octets
4.3.3 NonTrivialStringMovableNoExcept — déplacement vrai et noexcept
Constructeur de déplacement noexcept = default

Affectation par déplacement noexcept via std::swap

→ le vector déplace (échange de pointeurs internes) lors des réallocations

But : mesurer l'impact de noexcept sur les performances du vector

4.3.4 NonTrivialArray<N> — copie coûteuse par la taille
a (clé) + std::array<unsigned char, N-8> privé

Non trivial (initialiseur a = 0 + constructeur par défaut)

Instancié en NonTrivialArray<32> → comparable à TrivialMedium (même taille)

4.4 Les structures de benchmark
Chaque bench_xxx<T> est un template struct avec une méthode statique run() :

new_graph<T>("nom", "unité") — crée un graphique

Définit la liste sizes

Appelle bench<...> — une fois par conteneur/variante

4.5 Orchestration
cpp
template<typename ...Types>
void bench_all() {
    bench_types<bench_fill_back, Types...>();     // lance bench_fill_back<T>::run()
    // ...
    bench_types<bench_number_crunching, TrivialSmall, TrivialMedium>();
    // seule cette ligne pour number_crunching
}
Bilan : main() instancie bench_all avec 8 types, puis graphs::output(Output::GOOGLE) génère les graphiques.

📊 Total : 9 benchmarks × 8 types + 1 benchmark × 2 types = 74 graphiques.

📈 5. Les benchmarks, un par un
Les chiffres proviennent de l'article (sauf mention). n = nombre d'éléments.

5.1 bench_fill_back — remplissage par push_back
Code : tailles 100 000 → 1 000 000. Variantes : vector, list, deque, vector_reserve, et trois versions *_inserter.

Théorie : vector amorti O(1) mais réalloue ~log₂(n) fois → reserve supprime ces réallocations. list fait *n* allocations. deque alloue par blocs.

Résultats (élément 1 Ko, ms) :

*n*	vector_pre	vector	list
1 000	9	11	10
10 000	122	451	8
100 000	949	2 635	115
1 000 000	9 138	23 654	1 270
Analyse :

✅ vector avec reserve → le plus rapide (une seule allocation)

⚠️ Sans reserve, vector est ~2,6× plus lent que vector_pre à 1 Ko

💡 À 1 Ko, list bat vector sans reserve (11 270 vs 23 654 ms)

📝 L'article prétend « un ordre de grandeur » entre vector_pre et list → non confirmé par ses propres chiffres (9 138 vs 11 270)

🎯 noexcept : sans lui, les réallocations copient les std::string ; avec lui, elles les déplacent

5.2 bench_emplace_back — emplace_back
Même scénario que §5.1 avec construction sur place (pas de copie temporaire). Pas de variantes reserve ni inserter. Mesure le gain de la construction directe — surtout visible sur les types non triviaux.

5.3 bench_fill_front — insertion en tête
Code : tailles 10 000 → 100 000 (réduites car très lent). vector testé uniquement si is_small<T>() (types de 8 octets).

Théorie : chaque insert(begin(), x) décale tous les éléments → O(n)/insertion → O(n²) total pour vector. O(1) pour list et deque.

Résultats (8 octets, ms) :

*n*	vector	list
10 000	4	0
50 000	94	9
100 000	393	74
200 000	1 600	393
300 000	4 239	312
Analyse : croissance quadratique pour vector (×4 quand *n* double). À 300 000, list est ~3 500× plus rapide. 💥 Pire cas absolu pour vector. → deque = alternative idéale.

5.4 bench_emplace_front — emplace_front
Variante par construction sur place de §5.3, même restriction is_small<T>(). Conclusions identiques.

5.5 bench_linear_search — recherche linéaire
Code : tailles 1 000 → 10 000. Init FilledRandom (valeurs [0,N] mélangées). Opération Find : std::find pour chaque nombre (déduit).

Théorie : O(n) partout → a priori équivalents. Complexité totale : O(n²).

Résultats (8 octets, µs) :

*n*	vector	list
1 000	1	11
1 000	1 545	—
5 000	35 886	—
10 000	150 865	—
20 000	614 496	—
À 1 Ko : list atteint 3 356 432 µs à *n* = 20 000 ; vector reste proche de 0 à l'échelle du graphique.

Analyse : 🏆 Cas emblématique de la localité de cache. vector parcouru séquentiellement (lignes de cache + prefetching) ; list éparpillée → enchaîne les cache misses. Écart de plusieurs ordres de grandeur, jamais inversé par la taille.

5.6 bench_random_insert — insertion à position aléatoire
Code : tailles 10 000 → 100 000 (ms). Conteneur rempli aléatoirement, puis 1 000 insertions à position aléatoire. Position trouvée par recherche linéaire (O(n) dans les deux cas).

Théorie : list gagne sur l'insertion (O(1) vs O(n)) mais la recherche domine → pénalise list.

Résultats (article, ms, n = 1 000 à 10 000) :

<table> <tr><td width="50%" valign="top">
🟩 Petits éléments

8 o : vector ~7,6× plus rapide (n=10 000)

32 o : écart réduit (~3,3×), vector toujours devant

</td><td width="50%" valign="top">
🟦 Gros éléments

1 Ko : rapport inversé, list ~14,5× plus rapide

Point de croisement : ~64 octets

</td></tr> </table>
⏱️ Le temps de list varie peu avec la taille de l'élément (seule la recherche dépend de *n*, et il y a peu de recherches)

🎯 Si l'itérateur est déjà connu (aucune recherche), list gagne nettement plus tôt — seul scénario où son O(1) s'exprime pleinement

5.7 bench_random_remove — suppression à position aléatoire
Code : même schéma que §5.6 avec politique Erase. Variante vector_rem avec RemoveErase (idiome erase(remove(...)), déduit) — compacte en un seul passage.

Théorie : symétrique de l'insertion.

Résultats : comportement identique à l'insertion (petits → vector, gros → list). ⚠️ Tableaux erronés dans l'article (§8) → aucun chiffre cité.

5.8 bench_sort — tri
Code : tailles 100 000 → 1 000 000. vector & deque : std::sort ; list : list::sort() (déduit).

Théorie : O(n log n) partout. Mais std::sort échange des éléments (≥ 3 copies/échange pour non déplaçable) vs list::sort qui ne relie que des pointeurs.

Résultats (ms) :

*n*	vector (8o)	list (8o)	vector (1Ko)	list (1Ko)
10 000	2	26	224	50
100 000	163	684	4 289	1 083
1 000 000	2 147	15 950	50 973	17 975
Analyse :

🟩 8 o : vector ~7,4× plus rapide à 1M (cache + prédiction de branchement)

🟦 1 Ko : list ~2,8× plus rapide (aucune copie d'éléments)

🎯 Types non triviaux : déplacement noexcept bon marché → rapproche vector de ses meilleures perfs

5.9 bench_destruction — destruction du conteneur
Code : SmartFilled + SmartDelete (déduit : conteneur via pointeur intelligent, temps de destruction mesuré).

Théorie :

vector<Trivial> → un seul bloc libéré (quasi gratuit)

list → delete par nœud (*n* libérations)

Types non triviaux → *n* destructeurs std::string en plus

Résultats : ⚠️ non fournis dans l'article. Attendu : vector ≪ deque < list pour types triviaux.

5.10 bench_number_crunching — insertion triée (calcul intensif)
Code : tailles 10 000 → 100 000, uniquement TrivialSmall & TrivialMedium. Politique RandomSortedInsert : insertion en maintenant trié, position trouvée par parcours.

Résultats (8 octets, ms) :

*n*	vector	list
10 000	45	166
50 000	928	10 665
100 000	3 753	50 766
200 000	15 185	231 480
300 000	34 293	715 892
Analyse : vector ~21× plus rapide à 300 000, et l'écart se creuse. Parcourir une liste ≫ décaler de petits éléments. 💡 std::lower_bound sur vector (dichotomie, impossible sur list en O(log n)) améliorerait encore.

🧮 6. Tableau comparatif global
Charge de travail	Petits (≤ 32 o)	Gros (≥ 1 Ko)	Remarque
Remplissage push_back	🟩 vector (avec reserve)	🟩 vector + reserve, sinon 🟦 list	L'allocation domine
Insertion en tête	🟨 deque / 🟦 list	🟨 deque / 🟦 list	vector quadratique
Recherche linéaire	🟩 vector (×100 → ×1000+)	🟩 vector	Effet de cache
Insert./suppr. aléatoire + recherche	🟩 vector	🟦 list (~14× à 1 Ko)	Croisement ~64 o
Tri	🟩 vector (~7×)	🟦 list (~3× à 1 Ko)	list ne copie pas
Destruction	🟩 vector	🟩 vector (triviaux)	list : *n* libérations
Insertion triée	🟩 vector (~20×)	—	Parcours coûteux en list
🧭 7. Guide de décision
<table> <tr><td width="50%" valign="top">
✅ Choisir std::vector par défaut
Toujours commencer par vector.

Appeler reserve() si taille connue.

Types non triviaux → rendre le déplacement noexcept.

Gros objets à réordonner → préférer vector<T*> ou vector d'indices.

</td><td width="50%" valign="top">
🔀 Changer de conteneur si…
Insertion/suppression en tête → std::deque (ou list)

Insertion/suppression au milieu → list seulement si :

éléments gros (≥ 64 o) ou très coûteux à déplacer ;

itérateurs déjà détenus (pas de recherche linéaire).

Ensemble trié → conteneur associatif ou vector trié + lower_bound.

</td></tr> </table>
🎯 Règle d'or
Toujours mesurer sur votre matériel et vos données. Le point de croisement dépend du cache, de l'allocateur et du compilateur.

⚠️ 8. Limites, erreurs et pièges de la méthodologie
8.1 Dans l'article
Problème	Détail
🔁 Section « Random Remove »	Reprend les tableaux « Push front » et « Sort » → vraies données absentes
📉 Texte sur fill	Annonce « un ordre de grandeur » non confirmé (9 138 vs 11 270)
🏷️ Tableau de remplissage	Ne correspond qu'au cas 1 024 octets
⏳ Matériel/compilateur	2010, GCC 4.7.2 → résultats obsolètes
8.2 Dans bench.cpp
📦 plf_timsort.h et plf_colony.h inclus mais non utilisés

📝 <string> non inclus explicitement

🎭 Commentaire « easy to move » de NonTrivialStringMovable trompeur (c'est une copie)

🔇 is_non_trivial_non_movable défini mais jamais utilisé

8.3 Méthodologiques
🧩 Fragmentation du tas : une list séquentielle garde la localité ; FilledRandom la détruit (cas défavorable réaliste)

📊 Pas de statistiques (moyenne, écart-type, répétitions) dans le code visible

🖥️ Un seul compilateur, une machine, un allocateur

📉 Grands *n* → sortent du cache L2/L3 → changement de régime possible

🛠️ 9. Compiler, exécuter, reproduire
bash
# Adapter les chemins ; bench.hpp, policies.hpp, plf_*.h doivent être présents
g++ -std=c++14 -O2 -march=native -pthread bench.cpp -o bench
./bench > results.html   # sortie selon graphs::output (GOOGLE : page Google Charts)
<table> <tr><td width="33%" valign="top">
🔧 Prérequis
C++11 minimum

C++14 recommandé (constexpr modernes)

bench.hpp, policies.hpp, plf_*.h présents

</td><td width="33%" valign="top">
💾 Ressources
74 graphiques

Tailles jusqu'à 10⁶

Monster 4 Ko × 10⁶ → ~4 Go RAM (à vérifier ou réduire)

⏱️ Temps d'exécution long

</td><td width="33%" valign="top">
🎯 Mesures fiables
Désactiver l'économie d'énergie

Fixer la fréquence CPU

Fermer les autres applications

Répéter les exécutions

</td></tr> </table>
🏁 10. Conclusion
La complexité algorithmique seule est un mauvais guide.

Conteneur	Quand il domine	Pourquoi
🟩 vector	Accès séquentiel : recherche, parcours, insertion triée, tri de petits éléments	Cache + prefetching
🟦 list	Éléments volumineux + pas de recherche : insertion/suppression milieu, tri sans copie	Aucune copie d'éléments
🟨 deque	Insertions aux deux extrémités	Blocs + O(1) aux bornes
🎭 Les détails d'implémentation des types (noexcept, déplacement, taille, SSO) changent sensiblement les résultats.

💎 Règle pratique finale
Commencer par vector, mesurer, ne changer que si les mesures le justifient.

<div align="center">
📄 Documentation générée à partir de bench.cpp — Baptiste Wicht (MIT)
📚 Référence : article « Performance of vector vs list »

</div>
