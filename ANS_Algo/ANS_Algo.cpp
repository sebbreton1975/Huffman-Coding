#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_MSC_VER)
#define strdup _strdup
#endif

#define MAX_CODE_LENGTH 256

// Structure d'un noeud de l'arbre de Huffman
typedef struct Node {
    char c;
    int freq;
    struct Node* left;
    struct Node* right;
} Node;

// Création d'un noeud
Node* createNode(char c, int freq, Node* left, Node* right) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->c = c;
    node->freq = freq;
    node->left = left;
    node->right = right;
    return node;
}

// Vérification si le noeud est une feuille
int isLeaf(Node* node) {
    return (node->left == NULL) && (node->right == NULL);
}

// Libération de l'arbre
void freeTree(Node* root) {
    if (!root)
        return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

// Construction de l'arbre de Huffman à partir de la table de fréquence
Node* buildHuffmanTree(int freq[256]) {
    Node* nodes[256];
    int count = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0)
            nodes[count++] = createNode((char)i, freq[i], NULL, NULL);
    }
    if (count == 0)
        return NULL;
    while (count > 1) {
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
            else if (nodes[i]->freq < nodes[min2]->freq)
                min2 = i;
        }
        Node* left = nodes[min1];
        Node* right = nodes[min2];
        Node* newNode = createNode('$', left->freq + right->freq, left, right);
        nodes[min1] = newNode;
        nodes[min2] = nodes[count - 1];
        count--;
    }
    return nodes[0];
}

// Génération des codes Huffman par parcours de l'arbre
void generateCodes(Node* root, char** codes, char* code, int depth) {
    if (!root)
        return;
    if (isLeaf(root)) {
        code[depth] = '\0';
        codes[(unsigned char)root->c] = strdup(code);
        return;
    }
    code[depth] = '0';
    generateCodes(root->left, codes, code, depth + 1);
    code[depth] = '1';
    generateCodes(root->right, codes, code, depth + 1);
}

// Écriture d'un bit dans le fichier de sortie
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

// Lecture d'un bit depuis le fichier d'entrée
int readBit(FILE* in, unsigned char* buffer, int* bitCount) {
    if (*bitCount == 0) {
        int ch = fgetc(in);
        if (ch == EOF)
            return -1;
        *buffer = (unsigned char)ch;
        *bitCount = 8;
    }
    int bit = (*buffer >> 7) & 1;
    *buffer <<= 1;
    (*bitCount)--;
    return bit;
}

// Compression par Huffman
void compressFileHuffman(const char* inputFileName, const char* outputFileName) {
    FILE* in = fopen(inputFileName, "rb");
    if (!in) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s\n", inputFileName);
        return;
    }
    int freq[256] = { 0 };
    int ch;
    while ((ch = fgetc(in)) != EOF)
        freq[ch]++;
    fseek(in, 0, SEEK_SET);
    Node* root = buildHuffmanTree(freq);
    if (!root) {
        fclose(in);
        return;
    }
    char* codes[256] = { 0 };
    char code[MAX_CODE_LENGTH];
    generateCodes(root, codes, code, 0);

    FILE* out = fopen(outputFileName, "wb");
    if (!out) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s pour écriture\n", outputFileName);
        fclose(in);
        freeTree(root);
        return;
    }
    // Écriture de la table de fréquences en en-tête
    fwrite(freq, sizeof(int), 256, out);

    unsigned char buffer = 0;
    int bitCount = 0;
    while ((ch = fgetc(in)) != EOF) {
        char* codeStr = codes[(unsigned char)ch];
        for (int i = 0; codeStr[i] != '\0'; i++)
            writeBit(out, &buffer, &bitCount, (codeStr[i] == '1'));
    }
    if (bitCount > 0)
        fputc(buffer, out);

    fclose(in);
    fclose(out);
    freeTree(root);
    for (int i = 0; i < 256; i++)
        if (codes[i])
            free(codes[i]);
}

// Décompression par Huffman
void decompressFileHuffman(const char* inputFileName, const char* outputFileName) {
    FILE* in = fopen(inputFileName, "rb");
    if (!in) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s\n", inputFileName);
        return;
    }
    int freq[256] = { 0 };
    if (fread(freq, sizeof(int), 256, in) != 256) {
        fprintf(stderr, "Erreur de lecture de l'en-tête dans %s\n", inputFileName);
        fclose(in);
        return;
    }
    int totalSymbols = 0;
    for (int i = 0; i < 256; i++)
        totalSymbols += freq[i];
    Node* root = buildHuffmanTree(freq);
    if (!root) {
        fclose(in);
        return;
    }

    FILE* out = fopen(outputFileName, "wb");
    if (!out) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s pour écriture\n", outputFileName);
        fclose(in);
        freeTree(root);
        return;
    }

    unsigned char buffer = 0;
    int bitCount = 0;
    int symbolsDecoded = 0;
    while (symbolsDecoded < totalSymbols) {
        Node* current = root;
        while (!isLeaf(current)) {
            int bit = readBit(in, &buffer, &bitCount);
            if (bit == -1) break;
            current = (bit == 0) ? current->left : current->right;
        }
        if (isLeaf(current)) {
            fputc(current->c, out);
            symbolsDecoded++;
        }
    }

    fclose(in);
    fclose(out);
    freeTree(root);
}

// Vérification que deux fichiers sont identiques
int verifyFiles(const char* file1, const char* file2) {
    FILE* f1 = fopen(file1, "rb");
    FILE* f2 = fopen(file2, "rb");
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return 0;
    }
    int result = 1, ch1, ch2;
    do {
        ch1 = fgetc(f1);
        ch2 = fgetc(f2);
        if (ch1 != ch2) { result = 0; break; }
    } while (ch1 != EOF && ch2 != EOF);
    fclose(f1);
    fclose(f2);
    return result;
}

int main() {
    int i = 1;
    char inputFileName[100], compFileName[100], decompFileName[100];

    while (1) {
        sprintf(inputFileName, "C:/Users/donde/source/test/random/%dbook.txt", i);
        FILE* testFile = fopen(inputFileName, "rb");
        if (!testFile)
            break;
        fclose(testFile);

        sprintf(compFileName, "C:/Users/donde/source/test/random/%dbook.huff", i);
        sprintf(decompFileName, "%d_dehuff.txt", i);

        clock_t start = clock();
        compressFileHuffman(inputFileName, compFileName);
        clock_t end = clock();
        printf("Compression Huffman: %s -> %s en %.3f s.\n",
            inputFileName, compFileName, (double)(end - start) / CLOCKS_PER_SEC);

        start = clock();
        decompressFileHuffman(compFileName, decompFileName);
        end = clock();
        printf("Décompression Huffman: %s -> %s en %.3f s.\n",
            compFileName, decompFileName, (double)(end - start) / CLOCKS_PER_SEC);

        if (verifyFiles(inputFileName, decompFileName))
            printf("Vérification : %s et %s sont identiques.\n\n", inputFileName, decompFileName);
        else
            printf("Vérification : %s et %s diffèrent.\n\n", inputFileName, decompFileName);
        if (i++ >= 1)
            break;
    }
    if (i == 0)
        printf("Aucun fichier à compresser/décompresser.\n");
    return 0;
}
