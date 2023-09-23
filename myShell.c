#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>

// Nó da fila
typedef struct Node
{
    char *name; // "ls -l" -> <- "ps"
    struct Node *next;
    struct Node *prev;
} Node;

// Definição da estrutura da fila
typedef struct Queue
{
    Node *head;
    Node *tail;
    char *last_command;
    char *style;

    /*
    mode 0 - interative
    mode 1 - batch
    */
    int mode;
} Queue;

int pidBack;

void *command_thread(void *arg);
Queue *createQueue(int argc);
void enqueue(Queue *queue, char *name);
void printQueue(Queue *queue);
int execute_seq(Queue *queue);
void execute_par(Queue *queue);
int sizeList(struct Node *head);
void clearQueue(Queue *queue);
int isEmpty(struct Queue *queue);
void dequeue(Queue *queue);
int containsSymbol(char *command, char *symbol);
void divideCommandBetweenSymbol(char *command, char **partOne, char **partTwo, char *symbol);
int executeLine(char *line, Queue *queue);
void removeWhiteSpacesBeforeAfter(char *string);
int isBackground(char *string);
int executePipe(char *command);
int executeRedirecting(char *totalCommand);

int main(int argc, char *argv[])
{
    Queue *queue = createQueue(argc);
    int should_run = 1; /* flag to determine when to exit program */
    size_t n = 0;
    char *line = NULL;
    int firstCommand = 1;

    if (argc > 2)
    {
        printf("Invalid number of arguments.\n");
        return 1;
    }

    if (argc > 1)
    {
        printf("Archive: %s\n", argv[1]);
        // Abra o arquivo para leitura
        FILE *file = fopen(argv[1], "r");
        if (file == NULL)
        {
            perror("Erro ao abrir o arquivo");
            return 1;
        }

        // Ler uma linha do arquivo
        while (should_run && getline(&line, &n, file) != -1)
        {
            removeWhiteSpacesBeforeAfter(line);

            if (!strcmp(line, "style sequential"))
            {
                strcpy(queue->style, "seq>");
            }
            else if (!strcmp(line, "style parallel"))
            {
                strcpy(queue->style, "par>");
            }
            else
            {
                executeLine(line, queue);

                if (!strcmp(queue->style, "seq>"))
                {
                    execute_seq(queue);
                }
                else
                {
                    execute_par(queue);
                }
            }

            clearQueue(queue);
        }
        fclose(file);
    }
    else
    {
        while (should_run)
        {
            printf("gpac %s ", queue->style);
            fflush(stdout); // fflush exibe imediatamente o que existe no buffer (osh>)

            getline(&line, &n, stdin);

            removeWhiteSpacesBeforeAfter(line);

            if (containsSymbol(line, "!!") && !firstCommand)
            {
                char *posicao = strstr(line, "!!");
                char *resultLine = (char *)malloc(strlen(line) + strlen(queue->last_command) - 1);

                // Calcule o tamanho da substring antes de "!!"
                int tamanhoAntes = posicao - line;

                // Copie a parte da primeira string antes de "!!" para o resultado
                strncpy(resultLine, line, tamanhoAntes);
                resultLine[tamanhoAntes] = '\0'; // Adicione o caractere nulo para indicar o final da string

                // Concatene a segunda string ao resultado
                strcat(resultLine, queue->last_command);

                // Concatene a parte da primeira string após "!!" ao resultado
                strcat(resultLine, posicao + 2); // +2 para pular os caracteres "!!"
                line = (char *)malloc(strlen(resultLine) + 1);
                strcpy(line, resultLine);
                free(resultLine);
            }

            strcpy(queue->last_command, line);

            if (feof(stdin))
            {
                printf("\nCTRL-D pressed\nExiting shell...\n");
                return 0;
            }
            else if (!strcmp(line, "style sequential"))
            {
                firstCommand = 0;
                strcpy(queue->style, "seq>");
            }
            else if (!strcmp(line, "style parallel"))
            {
                firstCommand = 0;
                strcpy(queue->style, "par>");
            }
            else if (containsSymbol(line, "!!") && firstCommand)
            {
                printf("No commands\n");
            } 
            // bug do espaço aquii
            else
            {
                firstCommand = 0;
                executeLine(line, queue);

                if (!strcmp(queue->style, "seq>"))
                {
                    execute_seq(queue);
                }
                else
                {
                    execute_par(queue);
                }
            }
            clearQueue(queue);
        }
    }
    free(line);
    return 0;
}

void removeWhiteSpacesBeforeAfter(char *str)
{
    int length = strlen(str);
    int i = 0, j = length - 1;

    // Encontrar o primeiro caractere não espaço em branco do início da string
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n')
    {
        i++;
    }

    // Encontrar o primeiro caractere não espaço em branco do final da string
    while (str[j] == ' ' || str[j] == '\t' || str[j] == '\n')
    {
        j--;
    }

    // Copiar os caracteres não espaço em branco de volta para a string original
    int k = 0;
    for (; i <= j; i++, k++)
    {
        str[k] = str[i];
    }

    // Adicionar o caractere nulo terminador
    str[k] = '\0';
}

int executeLine(char *line, Queue *queue)
{
    // Antes de adicionar à fila, remova qualquer quebra de linha
    size_t length = strlen(line);
    if (length > 0 && line[length - 1] == '\n')
    {
        line[length - 1] = '\0';
    }

    // Usando strtok para dividir a string em palavras pelo espaço
    char *token = strtok(line, ";");
    while (token != NULL)
    {
        removeWhiteSpacesBeforeAfter(token);
        enqueue(queue, token);
        token = strtok(NULL, ";");
    }
    return 0;
}

void *command_thread(void *arg)
{
    char *command = (char *)malloc(strlen((char *)arg) + 1);
    strcpy(command, (char *)arg);
    int returnCode;

    if (!strcmp(command, "exit"))
    {
        exit(0);
    }
    else if (!strcmp(command, "style sequential"))
    {
        printf("You cannot change the shell style while executing other commands.\n"
               "Please type just \'style sequential\' or \'style parallel\' to change the shell style.\n");
    }
    else if (!strcmp(command, "style parallel"))
    {
        printf("You cannot change the shell style while executing other commands.\n"
               "Please type just \'style sequential\' or \'style parallel\' to change the shell style.\n");
    }
    if (containsSymbol(command, ">"))
    {
        char *commandToSystem = NULL;
        char *file = NULL;
        FILE *archive;

        if (containsSymbol(command, ">>"))
        {
            divideCommandBetweenSymbol(command, &commandToSystem, &file, ">>");
            removeWhiteSpacesBeforeAfter(file);
            archive = fopen(file, "a");
        }
        else
        {
            divideCommandBetweenSymbol(command, &commandToSystem, &file, ">");
            removeWhiteSpacesBeforeAfter(file);
            archive = fopen(file, "w");
        }

        if (archive == NULL)
        {
            perror("Error while opening the archive");
            return 1;
        }

        int original_stdout = dup(STDOUT_FILENO);
        int fd = fileno(archive);

        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            fflush(stdout);
            perror("Error while redirecting stdout");
            return 3;
        }

        system(commandToSystem);

        if (dup2(original_stdout, STDOUT_FILENO) == -1)
        {
            perror("Error while restoring stdout");
            return 4;
        };
    }
    else
    {
        returnCode = system(command);
        if (returnCode != 0)
        {
            printf("Command %s execution failed or returned non-zero: %d\n", command, returnCode);
        }
    }

    free(command);
    pthread_exit(NULL); // Encerra a thread
}

// Função para criar uma fila vazia
Queue *createQueue(int argc)
{
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (queue == NULL)
    {
        fprintf(stderr, "Memory allocation error for the queue\n");
        exit(1);
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->last_command = (char *)malloc(sizeof(strlen("No commands") + 1));
    if (queue->last_command == NULL)
    {
        fprintf(stderr, "Memory allocation error for the queue last command\n");
        exit(1);
    }
    strcpy(queue->last_command, "No commands");
    queue->style = (char *)malloc(sizeof(strlen("seq>") + 1));
    if (queue->last_command == NULL)
    {
        fprintf(stderr, "Memory allocation error for the queue style\n");
        exit(1);
    }
    strcpy(queue->style, "seq>\0");

    // interative
    if (argc > 1)
        queue->mode = 1;
    // batch
    else
        queue->mode = 0;

    return queue;
}

// Função para adicionar um elemento à fila (lista duplamente encadeada)
void enqueue(Queue *queue, char *name)
{
    // Crie um novo nó com os dados
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        fprintf(stderr, "Erro ao alocar memória para o nó\n");
        exit(1);
    }
    newNode->name = (char *)malloc(strlen(name) + 1);
    strcpy(newNode->name, name);

    // Inicialize os ponteiros prev e next para NULL
    newNode->prev = NULL;
    newNode->next = NULL;

    // Se a fila estiver vazia, o novo nó é tanto a head quanto a tail
    if (queue->tail == NULL)
    {
        queue->head = queue->tail = newNode;
        return;
    }

    // Caso contrário, adicione o novo nó à retaguarda e atualize a retaguarda
    newNode->prev = queue->tail; // O nó anterior ao novo nó é a retaguarda atual
    queue->tail->next = newNode; // O próximo nó após a retaguarda atual é o novo nó
    queue->tail = newNode;       // Atualize a retaguarda para o novo nó
}

// Função para imprimir os elementos da fila
void printQueue(Queue *queue)
{
    Node *current = queue->head;
    while (current != NULL)
    {
        printf("%s -> ", current->name);
        current = current->next;
    }
    printf("\n");
}

// às vezes há problema de buffer nesta função.
// antes das threads terminarem é exibido o gpac
void execute_par(Queue *queue)
{
    Node *current = queue->head;

    // child
    // child cria n threads, representando n threads
    // revisar isso

    int num_threads = sizeList(queue->head);

    pthread_t threads[num_threads];

    int i = 0;
    while (current != NULL)
    {
        if (!strcmp(current->name, "exit"))
        {
            exit(0);
        }
        else if (!strcmp(current->name, "style sequential"))
        {
            printf("You cannot change the shell style while executing other commands.\n"
                   "Please type just \'style sequential\' or \'style parallel\' to change the shell style.\n");
        }
        else if (!strcmp(current->name, "style parallel"))
        {
            printf("You cannot change the shell style while executing other commands.\n"
                   "Please type just \'style sequential\' or \'style parallel\' to change the shell style.\n");
        }
        else if (pthread_create(&threads[i], NULL, (void *)command_thread, current->name) != 0)
        {
            fprintf(stderr, "Erro ao criar a thread\n");
            exit(1);
        }
        i++;
        current = current->next;
    }

    // Aguarde as threads terminarem
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

int execute_seq(Queue *queue)
{
    pid_t parent_pid = getppid();

    Node *current = queue->head;

    if (isBackground(current->name))
    {
        pidBack = fork();

        if (pidBack == -1)
        {
            perror("fork");
            return 1;
        }

        if (pidBack == 0)
        {
            // Este é o processo filho
            execlp("/bin/sh", "sh", "-c", current->name, NULL);
            perror("execlp"); // Em caso de erro na execução do comando
            return 1;
        }
        else
        {

            kill(pidBack, SIGSTOP);
            printf("Processo filho suspenso (PID: %d)\n", pidBack);
            fflush(stdout);
        }
        current = current->next;
    }

    if (current == NULL)
        return 0;

    if (!strcmp(current->name, "fg"))
    {
        // Se arg for "fg", retoma o processo filho
        kill(pidBack, SIGCONT);
        wait(NULL);
        current = current->next;
    }

    if (current == NULL)
        return 0;

    int pid = fork();

    if (pid < 0)
    {
        perror("Fork Failed");
        return 1;
    }
    else if (pid == 0)
    {
        // child
        // child cria n forks, representando n processos
        // revisar isso

        while (current != NULL)
        {
            if (!strcmp(current->name, "exit"))
            {
                clearQueue(queue);
                // revisar isso
                kill(parent_pid, SIGTERM);
                exit(0);
            }
            else if (!strcmp(current->name, "style sequential"))
            {
                printf("You cannot change the shell style while executing other commands.\n"
                       "Please type just \'style sequential\' or \'style parallel\' to change the shell style.\n");
            }
            else if (!strcmp(current->name, "style parallel"))
            {
                printf("You cannot change the shell style while executing other commands.\n"
                       "Please type just \'style sequential\' or \'style parallel\' to change the shell style.\n");
            }
            else if (containsSymbol(current->name, ">"))
            {
                executeRedirecting(current->name);
            }
            else if (containsSymbol(current->name, "|"))
            {
                executePipe(current->name);
            }
            else
            {
                int pid1 = fork();

                if (pid1 < 0)
                {
                    fprintf(stderr, "Fork Failed");
                    return 4;
                }
                else if (pid1 == 0)
                {
                    if (queue->mode == 1)
                    {
                        printf("%s\n", current->name);
                        fflush(stdout);
                    }

                    execlp("/bin/sh", "sh", "-c", current->name, NULL);
                    perror("execlp");
                    exit(0);
                }
                wait(NULL);
                fflush(stdout);
            }

            current = current->next;
        }
        exit(0);
    }

    waitpid(pid, NULL, 0);

    return 0;
}

// Função para calcular o tamanho da lista encadeada
int sizeList(struct Node *head)
{
    int tamanho = 0; // Inicializa o tamanho como zero

    // Percorre a lista enquanto não atingir o final
    while (head != NULL)
    {
        tamanho++;         // Incrementa o tamanho
        head = head->next; // Move para o próximo nó
    }

    return tamanho;
}

void clearQueue(Queue *queue)
{
    Node *current = queue->head;
    while (current != NULL)
    {
        Node *temp = current;
        current = current->next;
        free(temp->name); // Libera a memória alocada para o nome do nó
        free(temp);       // Libera a memória alocada para o nó
    }
    queue->head = NULL;
    queue->tail = NULL;
}

// Função para verificar se a fila está vazia
int isEmpty(struct Queue *queue)
{
    return queue->head == NULL;
}

// Returns 1 if it haves the symbol
int containsSymbol(char *command, char *symbol)
{
    if (strstr(command, symbol) != NULL)
    {
        return 1;
    }
    return 0;
}

// It divides the command in 2 with 'symbol': partOne (left of 'symbol') and partTwo (right of 'symbol')
void divideCommandBetweenSymbol(char *command, char **partOne, char **partTwo, char *symbol)
{
    char *symbolPos;

    if (strlen(symbol) == 1)
    {
        // Se symbol é um caractere, use strchr para encontrá-lo
        symbolPos = strchr(command, symbol[0]);
    }
    else
    {
        // Se symbol é uma string, use strstr para encontrá-lo
        symbolPos = strstr(command, symbol);
    }

    // Se o symbolPos for encontrado
    if (symbolPos != NULL)
    {
        size_t length = symbolPos - command; // Comprimento da primeira parte

        // Alocar memória para a primeira parte e copiá-la
        *partOne = (char *)malloc(length + 1);
        strncpy(*partOne, command, length);
        (*partOne)[length] = '\0';

        // Alocar memória para a segunda parte e copiá-la
        *partTwo = strdup(symbolPos + strlen(symbol));
    }
    else
    { // Se o symbolPos não for encontrado
        *partOne = strdup(command);
        *partTwo = NULL;
    }
}

int isBackground(char *command)
{
    int lenght = strlen(command);

    char *string = (char *)malloc(lenght + 1);

    strcpy(string, command);

    if (string == NULL || string[0] == '\0')
    {
        // Trate casos especiais, como string vazia ou nula
        return 0;
    }

    char *lastString = NULL;
    // Usando strtok para dividir a string em palavras pelo espaço
    char *token = strtok(string, " ");
    while (token != NULL)
    {
        lastString = token;
        token = strtok(NULL, " ");
    }

    // Verificar se a última substring é "&" (não verificar caracteres de nova linha)
    if (lastString != NULL && strcmp(lastString, "&") == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int executePipe(char *command)
{
    char *partOne = NULL;
    char *partTwo = NULL;
    divideCommandBetweenSymbol(command, &partOne, &partTwo, "|");

    fflush(stdout);
    int fd[2];
    if (pipe(fd) == -1)
    {
        printf("Error with the pipe\n");
        return 1;
    }

    int pid1 = fork();
    if (pid1 < 0)
    {
        fprintf(stderr, "Fork Failed");
        return 2;
    }

    if (pid1 == 0)
    {
        // Child Process 1
        fflush(stdout);
        if (dup2(fd[1], STDOUT_FILENO) == -1)
        {
            perror("Error while redirecting stdout");
            return 3;
        };
        close(fd[0]);
        close(fd[1]);
        execlp("/bin/sh", "sh", "-c", partOne, NULL);
        perror("execlp");
        exit(0);
    }

    int pid2 = fork();
    if (pid2 < 0)
    {
        fprintf(stderr, "Fork Failed");
        return 3;
    }

    if (pid2 == 0)
    {
        // Child Process 2
        if (dup2(fd[0], STDIN_FILENO) == -1)
        {
            perror("Error while redirecting stdout");
            return 3;
        };
        close(fd[1]);
        close(fd[0]);
        execlp("/bin/sh", "sh", "-c", partTwo, NULL);
        perror("execlp");
        exit(0);
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

int executeRedirecting(char *totalCommand)
{
    char *command = NULL;
    char *file = NULL;
    FILE *archive;

    int pid1 = fork();

    if (pid1 < 0)
    {
        fprintf(stderr, "Fork Failed");
        return 2;
    }
    else if (pid1 == 0)
    {

        if (containsSymbol(totalCommand, ">>"))
        {
            divideCommandBetweenSymbol(totalCommand, &command, &file, ">>");
            removeWhiteSpacesBeforeAfter(file);
            archive = fopen(file, "a");
        }
        else
        {
            divideCommandBetweenSymbol(totalCommand, &command, &file, ">");
            removeWhiteSpacesBeforeAfter(file);
            archive = fopen(file, "w");
        }

        if (archive == NULL)
        {
            perror("Error while opening the archive");
            return 1;
        }

        int fd = fileno(archive);

        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            fflush(stdout);
            perror("Error while redirecting stdout");
            return 3;
        }

        if(containsSymbol(command, "|")){
            executePipe(command);
            exit(0);
        }

        execlp("/bin/sh", "sh", "-c", command, NULL);
        perror("execlp");
        exit(0);
    }

    waitpid(pid1, NULL, 0);
    fflush(stdout);
}