#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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

Trabalho *BuscarTrabalho(Shell *ProcessFlow, int Id) {
    for (int Indice = 0; Indice < ProcessFlow->QuantidadeTrabalhos; Indice++) {
        if (ProcessFlow->Trabalhos[Indice].Id == Id) {
            return &ProcessFlow->Trabalhos[Indice];
        }
    }
    return NULL;
}

void ColherTrabalhosFinalizados(Shell *ProcessFlow) {
    for (int Indice = 0; Indice < ProcessFlow->QuantidadeTrabalhos; Indice++) {
        Trabalho *TrabalhoAtual = &ProcessFlow->Trabalhos[Indice];
        if (TrabalhoAtual->Estado != TRABALHO_EXECUTANDO) {
            continue;
        }

        int StatusSaida;
        pid_t Resultado = waitpid(TrabalhoAtual->Pid, &StatusSaida, WNOHANG);
        if (Resultado == TrabalhoAtual->Pid) {
            TrabalhoAtual->Estado = TRABALHO_FINALIZADO;
            TrabalhoAtual->CodigoSaida = WIFEXITED(StatusSaida) ? WEXITSTATUS(StatusSaida) : -1;
        }
    }
}

void AguardarTodosTrabalhos(Shell *ProcessFlow) {
    for (int Indice = 0; Indice < ProcessFlow->QuantidadeTrabalhos; Indice++) {
        Trabalho *TrabalhoAtual = &ProcessFlow->Trabalhos[Indice];
        if (TrabalhoAtual->Estado != TRABALHO_EXECUTANDO) {
            continue;
        }

        int StatusSaida;
        waitpid(TrabalhoAtual->Pid, &StatusSaida, 0);
        TrabalhoAtual->Estado = TRABALHO_FINALIZADO;
        TrabalhoAtual->CodigoSaida = WIFEXITED(StatusSaida) ? WEXITSTATUS(StatusSaida) : -1;
    }
}

pid_t IniciarTarefa(Tarefa *TarefaAlvo, int DescritorEntrada, int DescritorSaida) {
    pid_t Pid = fork();

    if (Pid == 0) {
        if (DescritorEntrada != -1) {
            dup2(DescritorEntrada, STDIN_FILENO);
            close(DescritorEntrada);
        } else if (TarefaAlvo->ArquivoEntrada != NULL) {
            int Fd = open(TarefaAlvo->ArquivoEntrada, O_RDONLY);
            if (Fd == -1) {
                fprintf(stderr, "nao foi possivel abrir arquivo de entrada '%s'\n", TarefaAlvo->ArquivoEntrada);
                _exit(1);
            }
            dup2(Fd, STDIN_FILENO);
            close(Fd);
        }

        if (DescritorSaida != -1) {
            dup2(DescritorSaida, STDOUT_FILENO);
            close(DescritorSaida);
        } else if (TarefaAlvo->ArquivoSaida != NULL) {
            int Flags = O_WRONLY | O_CREAT | (TarefaAlvo->ModoAnexar ? O_APPEND : O_TRUNC);
            int Fd = open(TarefaAlvo->ArquivoSaida, Flags, 0644);
            if (Fd == -1) {
                fprintf(stderr, "nao foi possivel abrir arquivo de saida '%s'\n", TarefaAlvo->ArquivoSaida);
                _exit(1);
            }
            dup2(Fd, STDOUT_FILENO);
            close(Fd);
        }

        execvp(TarefaAlvo->Argumentos[0], TarefaAlvo->Argumentos);
        fprintf(stderr, "nao foi possivel executar '%s'\n", TarefaAlvo->Argumentos[0]);
        _exit(127);
    }

    return Pid;
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
        NovaTarefa->Argumentos[NovaTarefa->QuantidadeArgumentos++] = strdup(Argumentos[Indice]);
    }
    NovaTarefa->Argumentos[NovaTarefa->QuantidadeArgumentos] = NULL;

    ProcessFlow->QuantidadeTarefas++;
    printf("tarefa '%s' cadastrada\n", NovaTarefa->Nome);
}

void ComandoInput(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos) {
    if (QuantidadeArgumentos < 3) {
        fprintf(stderr, "uso: input <tarefa> <arquivo>\n");
        return;
    }

    Tarefa *TarefaAlvo = BuscarTarefa(ProcessFlow, Argumentos[1]);
    if (TarefaAlvo == NULL) {
        fprintf(stderr, "tarefa '%s' nao encontrada\n", Argumentos[1]);
        return;
    }

    free(TarefaAlvo->ArquivoEntrada);
    TarefaAlvo->ArquivoEntrada = strdup(Argumentos[2]);
}

void ComandoOutput(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos) {
    if (QuantidadeArgumentos < 3) {
        fprintf(stderr, "uso: output <tarefa> <arquivo>\n");
        return;
    }

    Tarefa *TarefaAlvo = BuscarTarefa(ProcessFlow, Argumentos[1]);
    if (TarefaAlvo == NULL) {
        fprintf(stderr, "tarefa '%s' nao encontrada\n", Argumentos[1]);
        return;
    }

    free(TarefaAlvo->ArquivoSaida);
    TarefaAlvo->ArquivoSaida = strdup(Argumentos[2]);
    TarefaAlvo->ModoAnexar = 0;
}

void ComandoAppend(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos) {
    if (QuantidadeArgumentos < 3) {
        fprintf(stderr, "uso: append <tarefa> <arquivo>\n");
        return;
    }

    Tarefa *TarefaAlvo = BuscarTarefa(ProcessFlow, Argumentos[1]);
    if (TarefaAlvo == NULL) {
        fprintf(stderr, "tarefa '%s' nao encontrada\n", Argumentos[1]);
        return;
    }

    free(TarefaAlvo->ArquivoSaida);
    TarefaAlvo->ArquivoSaida = strdup(Argumentos[2]);
    TarefaAlvo->ModoAnexar = 1;
}

void ComandoWorkdir(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos) {
    (void)ProcessFlow;

    if (QuantidadeArgumentos < 2) {
        fprintf(stderr, "uso: workdir <diretorio>\n");
        return;
    }

    if (chdir(Argumentos[1]) == -1) {
        fprintf(stderr, "diretorio '%s' nao encontrado\n", Argumentos[1]);
    }
}

void ComandoStart(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos) {
    if (QuantidadeArgumentos < 2) {
        fprintf(stderr, "uso: start <tarefa>\n");
        return;
    }

    Tarefa *TarefaAlvo = BuscarTarefa(ProcessFlow, Argumentos[1]);
    if (TarefaAlvo == NULL) {
        fprintf(stderr, "tarefa '%s' nao encontrada\n", Argumentos[1]);
        return;
    }

    if (ProcessFlow->QuantidadeTrabalhos >= MAXIMO_TRABALHOS) {
        fprintf(stderr, "numero maximo de trabalhos em background atingido\n");
        return;
    }

    pid_t Pid = IniciarTarefa(TarefaAlvo, -1, -1);
    if (Pid <= 0) {
        fprintf(stderr, "nao foi possivel iniciar a tarefa '%s'\n", TarefaAlvo->Nome);
        return;
    }

    Trabalho *NovoTrabalho = &ProcessFlow->Trabalhos[ProcessFlow->QuantidadeTrabalhos];
    NovoTrabalho->Id = ProcessFlow->ProximoIdTrabalho++;
    NovoTrabalho->Pid = Pid;
    strncpy(NovoTrabalho->NomeTarefa, TarefaAlvo->Nome, TAMANHO_MAXIMO_NOME - 1);
    NovoTrabalho->NomeTarefa[TAMANHO_MAXIMO_NOME - 1] = '\0';
    NovoTrabalho->Estado = TRABALHO_EXECUTANDO;
    NovoTrabalho->CodigoSaida = 0;
    ProcessFlow->QuantidadeTrabalhos++;

    printf("[%d] %d\n", NovoTrabalho->Id, NovoTrabalho->Pid);
}

void ComandoJobs(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos) {
    (void)Argumentos;
    (void)QuantidadeArgumentos;

    ColherTrabalhosFinalizados(ProcessFlow);

    for (int Indice = 0; Indice < ProcessFlow->QuantidadeTrabalhos; Indice++) {
        Trabalho *TrabalhoAtual = &ProcessFlow->Trabalhos[Indice];
        if (TrabalhoAtual->Estado == TRABALHO_EXECUTANDO) {
            printf("[%d] %d executando %s\n", TrabalhoAtual->Id, TrabalhoAtual->Pid, TrabalhoAtual->NomeTarefa);
        } else {
            printf("[%d] %d finalizado (codigo %d) %s\n", TrabalhoAtual->Id, TrabalhoAtual->Pid, TrabalhoAtual->CodigoSaida, TrabalhoAtual->NomeTarefa);
        }
    }
}

void ComandoWait(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos) {
    if (QuantidadeArgumentos < 2) {
        fprintf(stderr, "uso: wait <jobId>\n");
        return;
    }

    int Id = atoi(Argumentos[1]);
    Trabalho *TrabalhoAlvo = BuscarTrabalho(ProcessFlow, Id);
    if (TrabalhoAlvo == NULL) {
        fprintf(stderr, "job '%s' nao encontrado\n", Argumentos[1]);
        return;
    }

    if (TrabalhoAlvo->Estado == TRABALHO_EXECUTANDO) {
        int StatusSaida;
        waitpid(TrabalhoAlvo->Pid, &StatusSaida, 0);
        TrabalhoAlvo->Estado = TRABALHO_FINALIZADO;
        TrabalhoAlvo->CodigoSaida = WIFEXITED(StatusSaida) ? WEXITSTATUS(StatusSaida) : -1;
    }

    printf("[%d] %d finalizado (codigo %d)\n", TrabalhoAlvo->Id, TrabalhoAlvo->Pid, TrabalhoAlvo->CodigoSaida);
}

void ComandoRun(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos) {
    if (QuantidadeArgumentos < 3) {
        fprintf(stderr, "run sequential|parallel|pipe, tarefa...>\n");
        return;
    }

    char *Modo = Argumentos[1];
    int QuantidadeTarefas = QuantidadeArgumentos - 2;

    if (QuantidadeTarefas > MAXIMO_PIPELINE) {
        fprintf(stderr, "numero maximo de tarefas em um run excedido\n");
        return;
    }

    Tarefa *Tarefas[MAXIMO_PIPELINE];
    for (int Indice = 0; Indice < QuantidadeTarefas; Indice++) {
        Tarefas[Indice] = BuscarTarefa(ProcessFlow, Argumentos[Indice + 2]);
        if (Tarefas[Indice] == NULL) {
            fprintf(stderr, "tarefa '%s' nao encontrada\n", Argumentos[Indice + 2]);
            return;
        }
    }

    if (strcmp(Modo, "sequential") == 0) {
        for (int Indice = 0; Indice < QuantidadeTarefas; Indice++) {
            pid_t Pid = IniciarTarefa(Tarefas[Indice], -1, -1);
            if (Pid > 0) {
                waitpid(Pid, NULL, 0);
            }
        }
    } else if (strcmp(Modo, "parallel") == 0) {
        pid_t Pids[MAXIMO_PIPELINE];
        for (int Indice = 0; Indice < QuantidadeTarefas; Indice++) {
            Pids[Indice] = IniciarTarefa(Tarefas[Indice], -1, -1);
        }
        for (int Indice = 0; Indice < QuantidadeTarefas; Indice++) {
            if (Pids[Indice] > 0) {
                waitpid(Pids[Indice], NULL, 0);
            }
        }
    } else if (strcmp(Modo, "pipe") == 0) {
        int QuantidadePipes = QuantidadeTarefas - 1;
        int Pipes[MAXIMO_PIPELINE - 1][2];

        for (int Indice = 0; Indice < QuantidadePipes; Indice++) {
            if (pipe(Pipes[Indice]) == -1) {
                fprintf(stderr, "nao foi possivel criar pipe\n");
                for (int Anterior = 0; Anterior < Indice; Anterior++) {
                    close(Pipes[Anterior][0]);
                    close(Pipes[Anterior][1]);
                }
                return;
            }
            fcntl(Pipes[Indice][0], F_SETFD, FD_CLOEXEC);
            fcntl(Pipes[Indice][1], F_SETFD, FD_CLOEXEC);
        }

        pid_t Pids[MAXIMO_PIPELINE];
        for (int Indice = 0; Indice < QuantidadeTarefas; Indice++) {
            int DescritorEntrada = (Indice == 0) ? -1 : Pipes[Indice - 1][0];
            int DescritorSaida = (Indice == QuantidadeTarefas - 1) ? -1 : Pipes[Indice][1];
            Pids[Indice] = IniciarTarefa(Tarefas[Indice], DescritorEntrada, DescritorSaida);
        }

        for (int Indice = 0; Indice < QuantidadePipes; Indice++) {
            close(Pipes[Indice][0]);
            close(Pipes[Indice][1]);
        }

        for (int Indice = 0; Indice < QuantidadeTarefas; Indice++) {
            if (Pids[Indice] > 0) {
                waitpid(Pids[Indice], NULL, 0);
            }
        }
    } else {
        fprintf(stderr, "modo desconhecido '%s'\n", Modo);
    }
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
    } else if (strcmp(Tokens[0], "run") == 0) {
        ComandoRun(ProcessFlow, Tokens, QuantidadeTokens);
    } else if (strcmp(Tokens[0], "input") == 0) {
        ComandoInput(ProcessFlow, Tokens, QuantidadeTokens);
    } else if (strcmp(Tokens[0], "output") == 0) {
        ComandoOutput(ProcessFlow, Tokens, QuantidadeTokens);
    } else if (strcmp(Tokens[0], "append") == 0) {
        ComandoAppend(ProcessFlow, Tokens, QuantidadeTokens);
    } else if (strcmp(Tokens[0], "start") == 0) {
        ComandoStart(ProcessFlow, Tokens, QuantidadeTokens);
    } else if (strcmp(Tokens[0], "jobs") == 0) {
        ComandoJobs(ProcessFlow, Tokens, QuantidadeTokens);
    } else if (strcmp(Tokens[0], "wait") == 0) {
        ComandoWait(ProcessFlow, Tokens, QuantidadeTokens);
    } else if (strcmp(Tokens[0], "workdir") == 0) {
        ComandoWorkdir(ProcessFlow, Tokens, QuantidadeTokens);
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
        fflush(stdout);
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

    AguardarTodosTrabalhos(&ProcessFlow);

    return 0;
}
