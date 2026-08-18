#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "processflow.h"

void InicializarShell(Shell *ProcessFlow) {
    ProcessFlow->QuantidadeTarefas = 0;
    ProcessFlow->QuantidadeTrabalhos = 0;
    ProcessFlow->ProximoIdTrabalho = 1;
}

Tarefa *BuscarTarefa(Shell *ProcessFlow, const char *Nome) {
    for (int Indice = 0; Indice < ProcessFlow->QuantidadeTarefas; Indice++) {
        if (strcmp(ProcessFlow->Tarefas[Indice].Nome, Nome) == 0) {
            return &ProcessFlow->Tarefas[Indice];
        }
    }
    return NULL;
}

int Tokenizar(char *Linha, char **Tokens, int MaximoTokens) {
    int Quantidade = 0;
    char *Token = strtok(Linha, " \t\r\n");
    while (Token != NULL && Quantidade < MaximoTokens - 1) {
        Tokens[Quantidade++] = Token;
        Token = strtok(NULL, " \t\r\n");
    }
    Tokens[Quantidade] = NULL;
    return Quantidade;
}

void ComandoTask(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos) {
    if (QuantidadeArgumentos < 3) {
        fprintf(stderr, "task, nome, programa,argumentos...\n");
        return;
    }
    if (ProcessFlow->QuantidadeTarefas >= MAXIMO_TAREFAS) {
        fprintf(stderr, "numero maximo de tarefas atingido\n");
        return;
    }

    Tarefa *NovaTarefa = &ProcessFlow->Tarefas[ProcessFlow->QuantidadeTarefas];
    strncpy(NovaTarefa->Nome, Argumentos[1], TAMANHO_MAXIMO_NOME - 1);
    NovaTarefa->Nome[TAMANHO_MAXIMO_NOME - 1] = '\0';
    NovaTarefa->ArquivoEntrada = NULL;
    NovaTarefa->ArquivoSaida = NULL;
    NovaTarefa->ModoAnexar = 0;

    NovaTarefa->QuantidadeArgumentos = 0;
    for (int Indice = 2; Indice < QuantidadeArgumentos && NovaTarefa->QuantidadeArgumentos < MAXIMO_TOKENS - 1; Indice++) {
        NovaTarefa->Argumentos[NovaTarefa->QuantidadeArgumentos++] = Argumentos[Indice];
    }
    NovaTarefa->Argumentos[NovaTarefa->QuantidadeArgumentos] = NULL;

    ProcessFlow->QuantidadeTarefas++;
    printf("tarefa '%s' cadastrada\n", NovaTarefa->Nome);
}

int ProcessarLinha(Shell *ProcessFlow, char *Linha) {
    char *Tokens[MAXIMO_TOKENS];
    int QuantidadeTokens = Tokenizar(Linha, Tokens, MAXIMO_TOKENS);

    if (QuantidadeTokens == 0) {
        return 1;
    }

    if (strcmp(Tokens[0], "exit") == 0) {
        return 0;
    } else if (strcmp(Tokens[0], "task") == 0) {
        ComandoTask(ProcessFlow, Tokens, QuantidadeTokens);
    } else {
        fprintf(stderr, "comando desconhecido '%s'\n", Tokens[0]);
    }

    return 1;
}

void ExecutarModoInterativo(Shell *ProcessFlow) {
    char Linha[TAMANHO_MAXIMO_LINHA];

    while (1) {
        printf("processflow> ");
        fflush(stdout);

        if (fgets(Linha, sizeof(Linha), stdin) == NULL) {
            printf("\n");
            break;
        }

        if (!ProcessarLinha(ProcessFlow, Linha)) {
            break;
        }
    }
}

void ExecutarModoWorkflow(Shell *ProcessFlow, const char *NomeArquivo) {
    FILE *Arquivo = fopen(NomeArquivo, "r");
    if (Arquivo == NULL) {
        fprintf(stderr, "nao foi possivel abrir o arquivo workflow '%s'\n", NomeArquivo);
        exit(1);
    }

    char Linha[TAMANHO_MAXIMO_LINHA];
    while (fgets(Linha, sizeof(Linha), Arquivo) != NULL) {
        printf("%s", Linha);
        if (Linha[strlen(Linha) - 1] != '\n') {
            printf("\n");
        }
        if (!ProcessarLinha(ProcessFlow, Linha)) {
            break;
        }
    }

    fclose(Arquivo);
}

int main(int QuantidadeArgumentos, char *Argumentos[]) {
    if (QuantidadeArgumentos > 2) {
        fprintf(stderr, "uso: %s [workflowFile]\n", Argumentos[0]);
        return 1;
    }

    Shell ProcessFlow;
    InicializarShell(&ProcessFlow);

    if (QuantidadeArgumentos == 2) {
        ExecutarModoWorkflow(&ProcessFlow, Argumentos[1]);
    } else {
        ExecutarModoInterativo(&ProcessFlow);
    }

    return 0;
}
