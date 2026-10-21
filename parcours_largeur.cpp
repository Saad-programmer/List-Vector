#include <iostream>
#include <vector>
#include <queue>

// Définition d'un nœud de l'arbre
struct Node {
    int data;
    std::vector<Node*> children; // Liste des enfants 

    Node(int val) : data(val) {}
};

void parcoursEnLargeur(Node* root) {
    if (root == nullptr) return;

    // Creation 
    std::queue<Node*> file;
    
    // enfiler la racine
    file.push(root);

    std::cout << "Parcours en largeur : ";

    while (!file.empty()) {
        //On recupere le nœud au debut 
        Node* current = file.front();
        file.pop();

        // On traite (affiche) la valeur du nœud actuel
        std::cout << current->data << " ";

        // ajoute les enfants du nœud actuel dans la file
        for (Node* child : current->children) {
            if (child != nullptr) {
                file.push(child);
            }
        }
    }
    std::cout << std::endl;
}

int main() {
    //        1
    //      / | \
    //     2  3  4
    //    / \    |
    //   5   6   7

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