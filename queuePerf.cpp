#include <iostream>
#include <queue>
#include <vector>
#include <chrono> // Requis pour la mesure du temps

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
    // Construction de l'arbre
    Node* root = new Node(1);
    Node* n2 = new Node(2); Node* n3 = new Node(3); Node* n4 = new Node(4);
    root->children.push_back(n2); root->children.push_back(n3); root->children.push_back(n4);
    Node* n5 = new Node(5); Node* n6 = new Node(6);
    n2->children.push_back(n5); n2->children.push_back(n6);
    Node* n7 = new Node(7); n4->children.push_back(n7);
    Node* n8 = new Node(8); n5->children.push_back(n8);

    // --- Début de la mesure de performance ---
    auto start = std::chrono::high_resolution_clock::now();
    
    parcoursQueue(root);
    
    auto end = std::chrono::high_resolution_clock::now();
    // --- Fin de la mesure de performance ---

    // Calcul de la durée en microsecondes
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "Temps d'execution : " << duration << " microsecondes (us)" << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    deleteTree(root);
    return 0;
}
