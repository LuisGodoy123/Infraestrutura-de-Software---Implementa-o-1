#ifndef PROCESSFLOW_H
#define PROCESSFLOW_H

#include <sys/types.h>

#define TAMANHO_MAXIMO_LINHA 1024
#define MAXIMO_TOKENS 128
#define MAXIMO_TAREFAS 128
#define MAXIMO_TRABALHOS 128
#define TAMANHO_MAXIMO_NOME 64

typedef struct {
    char Nome[TAMANHO_MAXIMO_NOME];
    char *Argumentos[MAXIMO_TOKENS];
    int QuantidadeArgumentos;
    char *ArquivoEntrada;
    char *ArquivoSaida;
    int ModoAnexar;
} Tarefa;

typedef enum { TRABALHO_EXECUTANDO, TRABALHO_FINALIZADO } EstadoTrabalho;

typedef struct {
    int Id;
    pid_t Pid;
    char NomeTarefa[TAMANHO_MAXIMO_NOME];
    EstadoTrabalho Estado;
    int CodigoSaida;
} Trabalho;

typedef struct {
    Tarefa Tarefas[MAXIMO_TAREFAS];
    int QuantidadeTarefas;
    Trabalho Trabalhos[MAXIMO_TRABALHOS];
    int QuantidadeTrabalhos;
    int ProximoIdTrabalho;
} Shell;

void InicializarShell(Shell *ProcessFlow);
Tarefa *BuscarTarefa(Shell *ProcessFlow, const char *Nome);
Trabalho *BuscarTrabalho(Shell *ProcessFlow, int Id);
void ColherTrabalhosFinalizados(Shell *ProcessFlow);
void AguardarTodosTrabalhos(Shell *ProcessFlow);

void ComandoTask(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos);
void ComandoRun(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos);
void ComandoInput(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos);
void ComandoOutput(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos);
void ComandoAppend(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos);
void ComandoWorkdir(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos);
void ComandoStart(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos);
void ComandoJobs(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos);
void ComandoWait(Shell *ProcessFlow, char **Argumentos, int QuantidadeArgumentos);

pid_t IniciarTarefa(Tarefa *TarefaAlvo, int DescritorEntrada, int DescritorSaida);

int Tokenizar(char *Linha, char **Tokens, int MaximoTokens);
int ProcessarLinha(Shell *ProcessFlow, char *Linha);

void ExecutarModoInterativo(Shell *ProcessFlow);
void ExecutarModoWorkflow(Shell *ProcessFlow, const char *NomeArquivo);

#endif
