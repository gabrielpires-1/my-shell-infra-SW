#include <stdio.h>
#include <stdlib.h>

int main() {
    char nome[100];
    int idade;

    // Solicita ao usuário para fornecer seu nome e idade
    printf("Digite seu nome: ");
    scanf("%s", nome);
    printf("Digite sua idade: ");
    scanf("%d", &idade);

    // Abre um arquivo chamado "dados.txt" para escrever
    FILE *file = fopen("dados.txt", "w");

    if (file == NULL) {
        perror("Erro ao criar o arquivo");
        return 1;
    }

    // Escreve as informações no arquivo
    fprintf(file, "Nome: %s\n", nome);
    fprintf(file, "Idade: %d\n", idade);

    // Fecha o arquivo
    fclose(file);

    printf("As informações foram gravadas no arquivo 'dados.txt'.\n");

    return 0;
}
