# ProcessFlow

Orquestrador de processos implementado em C para a disciplina de Infraestrutura de Software (Implementação 1). Cadastra tarefas (programas do sistema) e as executa por meio de processos filhos, com suporte a execução sequencial, paralela, pipes, redirecionamento de entrada/saída, diretório de trabalho e execução em background.

## Arquivos

| Arquivo | Descrição |
|---|---|
| `processflow.c` | Implementação completa: parsing, comandos, motor de execução (`fork`/`exec`/`wait`/`dup2`/`pipe`) e o `main`. |
| `processflow.h` | Structs (`Tarefa`, `Trabalho`, `Shell`) e protótipos de todas as funções. |
| `Makefile` | Alvos `all`, `clean` e `test`. |
| `evidencias.log` | Saída real de testes manuais executados sobre o programa compilado. |
| `tests/*.pf` | Arquivos de workflow de exemplo, um por funcionalidade, usados pelo `make test`. |

## Sistema operacional

Desenvolvido e testado em **Linux (Ubuntu, via WSL2)**. O programa usa apenas chamadas POSIX (`fork`, `execvp`, `wait`/`waitpid`, `dup2`, `pipe`, `open`, `chdir`) e compila em qualquer Linux/Unix/macOS com `gcc`.

## Como compilar

```bash
make all
```

Gera o binário `processflow` na raiz do projeto, com `gcc -Wall -Wextra -std=c11`.

Para limpar o binário gerado:

```bash
make clean
```

## Como executar

**Modo interativo** (sem argumentos):

```bash
./processflow
```

Apresenta o prompt `processflow>` para digitar os comandos.

**Modo workflow** (lendo um arquivo `.pf`):

```bash
./processflow caminho/para/arquivo.pf
```

Cada linha do arquivo é impressa antes de ser processada. Ambos os modos terminam com o comando `exit` (ou `CTRL-D`/fim do arquivo).

## Comandos suportados

```
task <nome> <programa> [argumentos...]
run sequential <tarefa1> [tarefa2...]
run parallel <tarefa1> [tarefa2...]
run pipe <tarefa1> [tarefa2...]
input <tarefa> <arquivo>
output <tarefa> <arquivo>
append <tarefa> <arquivo>
workdir <diretorio>
start <tarefa>
jobs
wait <jobId>
exit
```

## Como testar

```bash
make test
```

Roda o `processflow` contra todos os arquivos `.pf` em `tests/`. Também é possível testar manualmente, criando um arquivo `.pf` com os comandos desejados e executando `./processflow arquivo.pf`, ou digitando os comandos direto no modo interativo.

O `evidencias.log` na raiz do projeto contém a saída real de uma bateria de testes cobrindo todos os comandos, casos de erro e casos de borda (linha vazia, espaços múltiplos, código de saída diferente de zero, término de processos paralelos em ordens diferentes, EOF sem `exit`).
