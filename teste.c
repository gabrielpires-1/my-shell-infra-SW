#include <stdio.h>

int main(int argc, char *argv[]) {
    // Verifica se pelo menos um argumento foi fornecido
    if (argc < 2) {
        printf("Uso: %s <argumento>\n", argv[0]);
        return 1; // Retorna 1 para indicar um erro
    }

    // Imprime os argumentos fornecidos
    printf("Número de argumentos: %d\n", argc - 1);

    printf("Argumentos fornecidos:\n");
    for (int i = 1; i < argc; i++) {
        printf("%d: %s\n", i, argv[i]);
    }

    return 0; // Retorna 0 para indicar sucesso
}
