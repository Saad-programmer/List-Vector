📘 Rapport de Laboratoire Technique
<div align="center">
Parcours en largeur (BFS) sur arbre *n*-aire
Implémentation avec std::queue, validation et mesure de performance

💻 Machine de test	HP EliteBook 850 G8 Notebook PC
📅 Date	2025
</div>
📄 Résumé
Ce rapport documente l'implémentation et l'évaluation de trois programmes C++ réalisant un parcours en largeur (Breadth-First Search, BFS) sur un arbre *n*-aire. L'objectif est double :

(i) valider la correction de l'algorithme par une analyse pas-à-pas ;
(ii) quantifier son coût d'exécution sur la machine de test.

Les trois programmes étudiés — queue.cpp, queuePerf.cpp et parcours_largeur.cpp — partagent le même algorithme mais diffèrent par l'instrumentation et la gestion mémoire. Les résultats confirment la conformité de l'ordre de sortie à la sémantique BFS (1 2 3 4 5 6 7 8), et situent le temps d'exécution mesuré à 28 µs, dominé par les entrées/sorties. Une fuite mémoire est identifiée dans parcours_largeur.cpp. Le rapport conclut sur les cas d'usage appropriés de std::queue et sur les recommandations de compilation pour la plateforme cible.

🔑 Mots-clés : BFS · std::queue · arbre *n*-aire · STL C++ · std::chrono · performance · fuite mémoire

🗂️ Table des matières
#	Section	Page
1	Introduction et objectifs	3
2	Cadre théorique	3
3	Méthodologie et protocole expérimental	4
4	Implémentations	5
5	Résultats	6
6	Analyse et discussion	8
7	Recommandations HP EliteBook 850 G8	10
8	Conclusion	10
A	Annexe A — queue.cpp	10
B	Annexe B — queuePerf.cpp	11
C	Annexe C — parcours_largeur.cpp	13
D	Annexe D — Journal terminal	13
1. Introduction et objectifs
1.1 Contexte
Dans le cadre du module ILISI2 — Virtualisation, l'étude comparative des structures de la bibliothèque standard C++ (STL) a été étendue aux algorithmes de parcours d'arbre. Ce rapport se concentre sur le parcours en largeur (BFS) et sur l'usage de l'adaptateur std::queue.

1.2 Objectifs du laboratoire
#	Objectif
O1	Implémenter un BFS correct sur un arbre *n*-aire en C++ moderne (C++17)
O2	Vérifier expérimentalement l'ordre de parcours par une trace manuelle et une exécution réelle
O3	Instrumenter le parcours avec std::chrono pour mesurer le temps d'exécution
O4	Comparer trois variantes d'implémentation et identifier leurs différences (mémoire, sortie, instrumentation)
O5	Discuter des cas d'usage de std::queue et des alternatives
O6	Formuler des recommandations adaptées à la plateforme HP EliteBook 850 G8
1.3 Périmètre
Le rapport couvre uniquement les arbres *n*-aires (chaque nœud possède un nombre arbitraire d'enfants). Il exclut les graphes généraux, les arbres pondérés et les variantes BFS multi-sources.

2. Cadre théorique
2.1 Définition du BFS
Le Breadth-First Search est un algorithme de parcours qui explore un arbre niveau par niveau :

🟡 Niveau 0 → racine
🔵 Niveau 1 → tous ses enfants
🟢 Niveau 2 → tous ses petits-enfants
🟠 Niveau 3 → etc.

Il s'appuie sur une file (structure FIFO) dont les opérations sont résumées ci-dessous.

📋 Tableau 1 — Opérations de l'adaptateur std::queue

Opération	Description	Complexité
push(x)	Ajoute *x* à l'arrière de la file	O(1)
pop()	Retire l'élément en tête (sans le retourner)	O(1)
front()	Accède à l'élément en tête	O(1)
back()	Accède à l'élément en queue	O(1)
empty()	Teste si la file est vide	O(1)
size()	Retourne le nombre d'éléments	O(1)
2.2 Propriétés fondamentales
📋 Tableau 2 — Propriétés algorithmiques du BFS

Propriété	Valeur / Description
📦 Structure de données	File FIFO (std::queue)
⏱️ Complexité temporelle	O(N), N = nombre de nœuds
💾 Complexité spatiale	O(w), *w* = largeur maximale de l'arbre
🔀 Ordre de visite	Par niveau, de gauche à droite
🎯 Garantie	Plus court chemin en nombre d'arêtes (graphe non pondéré)
2.3 Principe algorithmique
text
1. Initialiser une file vide.
2. Enfiler la racine si elle existe.
3. Tant que la file n'est pas vide :
   (a) Défiler le nœud en tête et le traiter (affichage, calcul, etc.).
   (b) Enfiler tous les enfants non nuls de ce nœud.
2.4 Conteneur sous-jacent
💡 Par défaut, std::queue encapsule un std::deque, structure de blocs mémoires offrant des insertions/suppressions efficaces aux deux extrémités. std::list est également autorisé mais moins performant en localité mémoire.

3. Méthodologie et protocole expérimental
3.1 Environnement de test
📋 Tableau 3 — Environnement matériel et logiciel

Élément	Valeur
💻 Machine	HP EliteBook 850 G8 Notebook PC
👤 Utilisateur	alan
🐧 Système	Linux (shell bash)
📁 Répertoire de travail	~/Documents/ILISI2/Virtualisation/...
⚙️ Compilateur	g++, standard C++17
📚 Bibliothèques	STL (queue, vector, chrono)
3.2 Commandes de compilation
bash
g++ -std=c++17 -O2 -Wall -Wextra queue.cpp            -o queue
g++ -std=c++17 -O2 -Wall -Wextra queuePerf.cpp        -o queuePerf
g++ -std=c++17 -O2 -Wall -Wextra parcours_largeur.cpp -o parcours_largeur
Option	Rôle
-std=c++17	Standard requis pour <chrono>
-O2	Optimisation
-Wall -Wextra	Avertissements
3.3 Jeu de test
L'arbre de test comporte 8 nœuds. Il est volontairement hétérogène : nœuds à 0, 1 ou 2 enfants, profondeur 3.

<div align="center">
text
                  ┌───┐
                  │ 1 │
                  └─┬─┘
          ┌─────────┼─────────┐
          ▼         ▼         ▼
        ┌───┐     ┌───┐     ┌───┐
        │ 2 │     │ 3 │     │ 4 │
        └─┬─┘     └───┘     └─┬─┘
        ┌─┴─┐                 │
        ▼   ▼                 ▼
      ┌───┐┌───┐            ┌───┐
      │ 5 ││ 6 │            │ 7 │
      └─┬─┘└───┘            └───┘
        ▼
      ┌───┐
      │ 8 │
      └───┘
Figure 1 — Arbre de test utilisé dans queue.cpp et queuePerf.cpp

</div>
4. Implémentations
4.1 Variante 1 : queue.cpp (référence)
4.1.1 Structure du nœud
cpp
struct Node {
    int data;
    std::vector<Node*> children;
    Node(int value) : data(value) {}
};
Chaque nœud stocke une valeur entière et un vecteur de pointeurs vers ses enfants, autorisant un nombre arbitraire d'enfants.

4.1.2 Fonction de parcours BFS
cpp
void parcoursQueue(Node* root) {
    if (root == nullptr) return;
    std::queue<Node*> container;
    container.push(root);                    // (1) préparer la racine

    std::cout << "std::queue (Largeur/BFS) : ";

    while (!container.empty()) {
        Node* current = container.front();   // (2) lire le prochain nœud
        container.pop();                     // (3) le défiler

        std::cout << current->data << " ";   // (4) TRAITEMENT

        for (Node* child : current->children) {  // (5) enfiler les enfants
            if (child != nullptr) container.push(child);
        }
    }
    std::cout << std::endl;
}
4.1.3 Libération mémoire
cpp
void deleteTree(Node* root) {
    if (root == nullptr) return;
    for (Node* child : root->children) {
        deleteTree(child);
    }
    delete root;
}
✅ La suppression est post-ordre : les enfants sont libérés avant leur parent, évitant toute fuite.

4.2 Variante 2 : queuePerf.cpp (instrumentée)
Identique à la variante 1, mais encadre l'appel à parcoursQueue par une mesure std::chrono :

cpp
auto start = std::chrono::high_resolution_clock::now();
parcoursQueue(root);
auto end = std::chrono::high_resolution_clock::now();

auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                (end - start).count();

std::cout << "Temps d'execution : " << duration << " microsecondes (us)\n";
4.3 Variante 3 : parcours_largeur.cpp (simplifiée)
Version concise utilisant les listes d'initialisation, mais sans libération mémoire :

cpp
Node* root = new Node(1);
Node* n2 = new Node(2);
Node* n3 = new Node(3);
Node* n4 = new Node(4);

root->children = {n2, n3, n4};
n2->children = {new Node(5), new Node(6)};
n4->children = {new Node(7)};

parcoursEnLargeur(root);
5. Résultats
5.1 Validation par trace manuelle
📋 Tableau 4 — Trace d'exécution du BFS sur l'arbre de test

Itération	Nœud traité	File après traitement	Sortie accumulée
0 (init)	—	[1]	—
1	1	[2, 3, 4]	1
2	2	[3, 4, 5, 6]	1 2
3	3	[4, 5, 6]	1 2 3
4	4	[5, 6, 7]	1 2 3 4
5	5	[6, 7, 8]	1 2 3 4 5
6	6	[7, 8]	1 2 3 4 5 6
7	7	[8]	1 2 3 4 5 6 7
8	8	[]	1 2 3 4 5 6 7 8
✅ La sortie est conforme à la sémantique BFS : 1 2 3 4 5 6 7 8.

5.2 Visualisation par niveau
<div align="center">
text
                  🟡 1        ← Niveau 0
               ┌──┼──┐
              🔵2 🔵3 🔵4      ← Niveau 1
             ┌─┴─┐    │
            🟢5 🟢6  🟢7      ← Niveau 2
             │
            🟠8                ← Niveau 3
Figure 2 — Ordre de visite par niveau (🟡 niveau 0, 🔵 niveau 1, 🟢 niveau 2, 🟠 niveau 3)

</div>
5.3 Décompte par niveau
📋 Tableau 5 — Répartition des nœuds par niveau

Niveau	Nombre de nœuds	Ordre d'affichage
0	1	1
1	3	2, 3, 4
2	3	5, 6, 7
3	1	8
Total	8	1 2 3 4 5 6 7 8
5.4 Sorties réelles observées
📋 Tableau 6 — Résultats d'exécution dans le terminal

Programme	Sortie
./queue	std::queue (Largeur/BFS) : 1 2 3 4 5 6 7 8
./parcours_largeur	Parcours en largeur : 1 2 3 4 5 6 7
./queuePerf	1 2 3 4 5 6 7 8 + Temps d'execution : 28 µs
5.5 Comparaison des trois variantes
📋 Tableau 7 — Synthèse comparative des trois implémentations

Critère	queue.cpp	queuePerf.cpp	parcours_largeur.cpp
Nombre de nœuds	8	8	7
Instrumentation chrono	❌ Non	✅ Oui	❌ Non
Libération mémoire	deleteTree	deleteTree	⚠️ absente
Sortie observée	1 2 3 4 5 6 7 8	idem + 28 µs	1 2 3 4 5 6 7
Fuite mémoire	✅ Non	✅ Non	⚠️ Oui
5.6 Mesure de performance
📋 Tableau 8 — Résultat de la mesure chronométrée (queuePerf)

Métrique	Valeur
⏱️ Horloge utilisée	std::chrono::high_resolution_clock
📏 Unité	microsecondes (µs)
Durée mesurée	28 µs
🎯 Périmètre	parcours + affichage
6. Analyse et discussion
6.1 Interprétation des 28 µs
La durée mesurée est dominée par les entrées/sorties (std::cout), et non par l'algorithme lui-même. Le parcours pur de 8 nœuds est de l'ordre de quelques centaines de nanosecondes.

Pour isoler le coût algorithmique, il faudrait :

📦 Stocker les valeurs dans un std::vector<int> pendant le parcours

⏱️ Mesurer la boucle de parcours seule (sans affichage)

🔁 Répéter la mesure (≥ 1000 itérations) et calculer la moyenne

⚡ Compiler avec -O2

6.2 Correction et robustesse
✅ Le BFS est correct sur l'arbre de test (ordre conforme, tableau 4)

✅ Les cas limites sont gérés : racine nulle (retour immédiat) et enfants nuls (filtrage dans la boucle)

⚠️ La variante parcours_largeur.cpp présente une fuite mémoire : tous les Node* alloués par new restent en mémoire. À corriger par un deleteTree analogue à celui de queue.cpp.

6.3 Cas d'usage de std::queue (BFS)
📋 Tableau 9 — Situations où std::queue / BFS est approprié

Situation	Justification
🎯 Plus court chemin (graphe non pondéré)	Le premier chemin trouvé est le plus court en nombre d'arêtes
📊 Parcours par niveau	Affichage hiérarchique, calcul de profondeur
🔍 Recherche d'un nœud proche de la racine	Le BFS atteint rapidement les nœuds peu profonds
🌳 Arbres peu profonds et larges	Coût mémoire acceptable si la largeur reste modérée
👥 Réseaux sociaux / recommandation	Exploration par degrés de séparation
6.4 Cas où il faut éviter std::queue / BFS
📋 Tableau 10 — Situations où le BFS est déconseillé et alternatives

Situation	Alternative et justification
🌲 Arbres très larges	DFS avec pile : O(h) au lieu de O(w)
📏 Très grande profondeur	DFS itératif ou récursif
🔄 Détection de cycle / tri topologique	DFS plus naturel et plus rapide
🔙 Recherche en profondeur (backtracking)	DFS ou récursion
⚖️ Graphes pondérés	Dijkstra (file de priorité) ou A*
🎯 Besoin d'une file prioritaire	std::priority_queue
⏱️ Temps réel strict	Buffer circulaire statique (pas d'allocation par élément)
6.5 Alternatives à std::queue
Alternative	Cas d'usage
std::deque direct	Même comportement, accès aléatoire en plus
std::list	Itérateurs stables
Buffer circulaire maison	Performances temps réel
std::stack	DFS
std::priority_queue	Dijkstra / A*
7. Recommandations pour la plateforme HP EliteBook 850 G8
📋 Tableau 11 — Recommandations spécifiques à la machine de test

Aspect	Recommandation
⚙️ Compilateur	g++ ≥ 9, standard C++17
⚡ Optimisation	-O2 en développement, -O3 en production
🔍 Vérification mémoire	valgrind --leak-check=full ./programme
📊 Benchmarks	Séparer les I/O du parcours, répéter, moyenner
📦 Conteneur enfants	std::vector (cache-friendly, localité mémoire)
🎯 Conteneur file	std::deque par défaut
💡 Sur un processeur Intel Core de 11ᵉ génération (équipement typique du 850 G8), la mesure de 28 µs est cohérente avec le coût des appels à std::cout et la granularité de high_resolution_clock.

8. Conclusion
Le BFS implémenté avec std::queue est un algorithme simple, lisible et efficace (O(N)) pour parcourir un arbre par niveaux. Les trois programmes étudiés illustrent respectivement :

Programme	Apport
✅ queue.cpp	Implémentation de référence, complète et correcte
⏱️ queuePerf.cpp	Instrumentation temporelle, mesure dominée par les I/O
⚠️ parcours_largeur.cpp	Variante concise mais sujette à fuite mémoire
Le BFS est particulièrement adapté aux arbres peu profonds et aux recherches de plus court chemin non pondéré, mais sa consommation mémoire proportionnelle à la largeur de l'arbre le rend inadapté aux structures très larges, où un DFS est préférable.

🎯 Sur la plateforme HP EliteBook 850 G8, la compilation avec g++ -std=c++17 -O2 est recommandée, et toute évaluation de performance doit exclure les entrées/sorties et être répétée.

🔮 Perspectives
📊 Benchmark isolé (parcours sans affichage, 1000 itérations)

⚖️ Comparaison BFS avec std::deque vs std::list vs buffer circulaire

🔄 Extension au BFS sur graphe avec détection de cycles

🔬 Profilage avec perf pour identifier les points chauds

📎 Annexe A — Code source complet queue.cpp
cpp
#include <iostream>
#include <queue>
#include <vector>

struct Node {
    int data;
    std::vector<Node*> children;
    Node(int value) : data(value) {}
};

void parcoursQueue(Node* root) {
    if (root == nullptr) return;

    std::queue<Node*> container;
    container.push(root);

    std::cout << "std::queue (Largeur/BFS) : ";

    while (!container.empty()) {
        Node* current = container.front();
        container.pop();

        std::cout << current->data << " ";

        for (Node* child : current->children) {
            if (child != nullptr) container.push(child);
        }
    }
    std::cout << std::endl;
}

void deleteTree(Node* root) {
    if (root == nullptr) return;
    for (Node* child : root->children) {
        deleteTree(child);
    }
    delete root;
}

int main() {
    Node* root = new Node(1);

    Node* n2 = new Node(2);
    Node* n3 = new Node(3);
    Node* n4 = new Node(4);

    root->children.push_back(n2);
    root->children.push_back(n3);
    root->children.push_back(n4);

    Node* n5 = new Node(5);
    Node* n6 = new Node(6);
    n2->children.push_back(n5);
    n2->children.push_back(n6);

    Node* n7 = new Node(7);
    n4->children.push_back(n7);

    Node* n8 = new Node(8);
    n5->children.push_back(n8);

    parcoursQueue(root);

    deleteTree(root);

    return 0;
}
📎 Annexe B — Code source complet queuePerf.cpp
cpp
#include <iostream>
#include <queue>
#include <vector>
#include <chrono>

struct Node {
    int data;
    std::vector<Node*> children;
    Node(int value) : data(value) {}
};

void parcoursQueue(Node* root) {
    if (root == nullptr) return;

    std::queue<Node*> container;
    container.push(root);

    std::cout << "std::queue (Largeur/BFS) : ";

    while (!container.empty()) {
        Node* current = container.front();
        container.pop();

        std::cout << current->data << " ";

        for (Node* child : current->children) {
            if (child != nullptr) container.push(child);
        }
    }
    std::cout << std::endl;
}

void deleteTree(Node* root) {
    if (root == nullptr) return;
    for (Node* child : root->children) {
        deleteTree(child);
    }
    delete root;
}

int main() {
    Node* root = new Node(1);
    Node* n2 = new Node(2); Node* n3 = new Node(3); Node* n4 = new Node(4);
    root->children.push_back(n2); root->children.push_back(n3);
    root->children.push_back(n4);
    Node* n5 = new Node(5); Node* n6 = new Node(6);
    n2->children.push_back(n5); n2->children.push_back(n6);
    Node* n7 = new Node(7); n4->children.push_back(n7);
    Node* n8 = new Node(8); n5->children.push_back(n8);

    auto start = std::chrono::high_resolution_clock::now();

    parcoursQueue(root);

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                    (end - start).count();

    std::cout << "------------------------------------------\n";
    std::cout << "Temps d'execution : " << duration
              << " microsecondes (us)\n";
    std::cout << "------------------------------------------\n";

    deleteTree(root);
    return 0;
}
📎 Annexe C — Code source complet parcours_largeur.cpp
cpp
#include <iostream>
#include <vector>
#include <queue>

struct Node {
    int data;
    std::vector<Node*> children;
    Node(int val) : data(val) {}
};

void parcoursEnLargeur(Node* root) {
    if (root == nullptr) return;

    std::queue<Node*> file;
    file.push(root);

    std::cout << "Parcours en largeur : ";

    while (!file.empty()) {
        Node* current = file.front();
        file.pop();

        std::cout << current->data << " ";

        for (Node* child : current->children) {
            if (child != nullptr) {
                file.push(child);
            }
        }
    }
    std::cout << std::endl;
}

int main() {
    Node* root = new Node(1);

    Node* n2 = new Node(2);
    Node* n3 = new Node(3);
    Node* n4 = new Node(4);

    root->children = {n2, n3, n4};

    n2->children = {new Node(5), new Node(6)};
    n4->children = {new Node(7)};

    parcoursEnLargeur(root);

    return 0;
}
📎 Annexe D — Journal du terminal
text
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/.../largeur$ ls
parcours_largeur  parcours_largeur.cpp  queue  queue.cpp  queuePerf  queuePerf.cpp

alan@.../largeur$ ./queue
std::queue (Largeur/BFS) : 1 2 3 4 5 6 7 8

alan@.../largeur$ ./parcours_largeur
Parcours en largeur : 1 2 3 4 5 6 7

alan@.../largeur$ ./queuePerf
std::queue (Largeur/BFS) : 1 2 3 4 5 6 7 8
------------------------------------------
Temps d'execution : 28 microsecondes (us)
------------------------------------------
<div align="center">
📄 Rapport de laboratoire — ILISI2 Virtualisation / C++ / STL
💻 HP EliteBook 850 G8 Notebook PC

</div>
