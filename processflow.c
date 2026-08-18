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
