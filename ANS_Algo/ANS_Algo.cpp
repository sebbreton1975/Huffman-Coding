#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_CODE_LENGTH 256

#if defined(_MSC_VER)
#define strdup _strdup
#endif


// Structure d'un noeud de l'arbre de Huffman
typedef struct Node {
    char c;
    int freq;
    struct Node* left;
    struct Node* right;
} Node;

// Créer un nouveau noeud
Node* createNode(char c, int freq, Node* left, Node* right) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->c = c;
    node->freq = freq;
    node->left = left;
    node->right = right;
    return node;
}

// Vérifie si le noeud est une feuille
int isLeaf(Node* node) {
    return (node->left == NULL) && (node->right == NULL);
}

// Libérer l'arbre de Huffman
void freeTree(Node* root) {
    if (root == NULL)
        return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

// Construction de l'arbre de Huffman à partir de la table de fréquence
Node* buildHuffmanTree(int freq[256]) {
    // Création d'un tableau de noeuds pour chaque caractère présent
    Node* nodes[256];
    int count = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            nodes[count++] = createNode((char)i, freq[i], NULL, NULL);
        }
    }

    if (count == 0)
        return NULL; // Aucun caractère dans le fichier

    // Combiner les deux noeuds de plus petite fréquence jusqu'à avoir un seul noeud racine
    while (count > 1) {
        // Trouver les deux noeuds de plus petite fréquence
        int min1 = 0, min2 = 1;
        if (nodes[min2]->freq < nodes[min1]->freq) {
            int temp = min1;
            min1 = min2;
            min2 = temp;
        }
        for (int i = 2; i < count; i++) {
            if (nodes[i]->freq < nodes[min1]->freq) {
                min2 = min1;
                min1 = i;
            }
            else if (nodes[i]->freq < nodes[min2]->freq) {
                min2 = i;
            }
        }

        // Créer un nouveau noeud interne dont la fréquence est la somme des deux
        Node* left = nodes[min1];
        Node* right = nodes[min2];
        Node* newNode = createNode('$', left->freq + right->freq, left, right);

        // Remplacer le noeud de min1 par le nouveau noeud et supprimer min2 du tableau
        nodes[min1] = newNode;
        nodes[min2] = nodes[count - 1];
        count--;
    }

    return nodes[0];
}

// Générer les codes Huffman en parcourant l'arbre
void generateCodes(Node* root, char** codes, char* code, int depth) {
    if (!root)
        return;

    // Si le noeud est une feuille, on stocke le code dans la table
    if (isLeaf(root)) {
        code[depth] = '\0';
        codes[(unsigned char)root->c] = strdup(code);
        return;
    }

    // Parcours gauche : ajouter '0'
    code[depth] = '0';
    generateCodes(root->left, codes, code, depth + 1);

    // Parcours droite : ajouter '1'
    code[depth] = '1';
    generateCodes(root->right, codes, code, depth + 1);
}

// Fonction pour écrire un bit dans le flux de sortie
void writeBit(FILE* out, unsigned char* buffer, int* bitCount, int bit) {
    if (bit)
        *buffer |= (1 << (7 - *bitCount));
    (*bitCount)++;
    if (*bitCount == 8) {
        fputc(*buffer, out);
        *buffer = 0;
        *bitCount = 0;
    }
}

// Fonction principale de compression d'un fichier par Huffman
void compressFile(const char* inputFileName, const char* outputFileName) {
    FILE* in = fopen(inputFileName, "rb");
    if (!in) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s\n", inputFileName);
        return;
    }

    // Calculer la fréquence de chaque caractère
    int freq[256] = { 0 };
    int ch;
    while ((ch = fgetc(in)) != EOF) {
        freq[ch]++;
    }
    // Retourner au début du fichier
    fseek(in, 0, SEEK_SET);

    // Construire l'arbre de Huffman
    Node* root = buildHuffmanTree(freq);
    if (!root) {
        fclose(in);
        return;
    }

    // Générer la table de codes
    char* codes[256] = { 0 };
    char code[MAX_CODE_LENGTH];
    generateCodes(root, codes, code, 0);

    // Ouvrir le fichier de sortie en mode binaire
    FILE* out = fopen(outputFileName, "wb");
    if (!out) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s pour l'écriture\n", outputFileName);
        fclose(in);
        freeTree(root);
        return;
    }

    // Écrire l'en-tête : la table de fréquence (pour permettre la décompression)
    fwrite(freq, sizeof(int), 256, out);

    // Encoder le fichier : pour chaque caractère, écrire le code binaire correspondant
    unsigned char buffer = 0;
    int bitCount = 0;
    while ((ch = fgetc(in)) != EOF) {
        char* codeStr = codes[(unsigned char)ch];
        for (int i = 0; codeStr[i] != '\0'; i++) {
            if (codeStr[i] == '1')
                writeBit(out, &buffer, &bitCount, 1);
            else
                writeBit(out, &buffer, &bitCount, 0);
        }
    }

    // Si des bits restent dans le tampon, les écrire
    if (bitCount > 0) {
        fputc(buffer, out);
    }

    // Libérer les ressources
    fclose(in);
    fclose(out);
    freeTree(root);
    for (int i = 0; i < 256; i++) {
        if (codes[i])
            free(codes[i]);
    }
}

int main() {
    int i = 1;
    char inputFileName[100];
    char outputFileName[100];

    // Boucle de test sur des fichiers "0.txt", "1.txt", etc.
    while (1) {
        sprintf(inputFileName, "C:/Users/donde/source/test/random/%dbook.txt", i);
        FILE* testFile = fopen(inputFileName, "rb");
        if (!testFile) {
            // Si le fichier n'existe pas, on arrête la boucle
            break;
        }
        fclose(testFile);

        sprintf(outputFileName, "C:/Users/donde/source/test/random/%dbook.huff", i);

        clock_t start = clock();
        compressFile(inputFileName, outputFileName);
        clock_t end = clock();
        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
        printf("Compression du fichier %s en %s terminee en %.3f secondes.\n",
            inputFileName, outputFileName, time_spent);

        if (i++ >= 1)
            break;
    }

    if (i == 0) {
        printf("Aucun fichier a compresser.\n");
    }

    return 0;
}
