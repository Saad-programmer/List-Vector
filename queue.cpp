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
    container.push(root);                          // (1) on prépare la racine

    std::cout << "std::queue (Largeur/BFS) : ";

    while (!container.empty()) {
        Node* current = container.front();         // (2) on LIT le prochain nœud
        container.pop();                           // (3) on le DÉFILE (retire de la file)

        std::cout << current->data << " ";         // (4)  ICI : LE NŒUD EST PARCOURU 

        for (Node* child : current->children) {    // (5) on enfile ses enfants
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
    // Construction d'un arbre exemple :
    //              1
    //           /  |  \
    //          2   3   4
    //         / \      |
    //        5   6     7
    //       /
    //      8

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

    // Parcours en largeur (BFS)
    parcoursQueue(root);

    // Liberation de la mémoire
    deleteTree(root);

    return 0;
}