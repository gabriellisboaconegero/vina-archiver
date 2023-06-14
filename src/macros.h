#ifndef MACROS_H_
#define MACROS_H_
#define MAX_SZ 1024
#include <errno.h>
#include <stdarg.h>

#define BYTE_SZ 1
#define perro(msg, ...) do{             \
    fprintf(stderr, msg, __VA_ARGS__);  \
    fflush(stdout);                     \
    perror(NULL);} while(0)

#ifdef DEBUG
#define ERRO(fmt, ...) fprintf(stderr, "[ERRO][%s, %d]: "fmt"\n", __FILE__, __LINE__, __VA_ARGS__)
#define PERRO(msg, ...) perro("[ERRO][%s, %d]: "msg, __FILE__, __LINE__, __VA_ARGS__)
#else
#define ERRO(fmt, ...) fprintf(stderr, "[ERRO]: "fmt"\n", __VA_ARGS__)
#define PERRO(msg, ...) perro("[ERRO]: "msg, __VA_ARGS__)
#endif

#define MEM_ERR_MSG "Nao foi possivel alocar memoria"
#define SIZE_OF_MMD (sizeof(struct memb_md_t) - sizeof(char *) - sizeof(struct memb_md_t *))
#define USAGE(fd)                                                                                                                   \
    do{                                                                                                                             \
        fprintf(fd, "USAGE: vina++ <opcao> <archive> [membro1 membro2 ...]\n\n");                                                   \
        fprintf(fd, "OPCOES:\n");                                                   \
        fprintf(fd, "\t-i: Inserir um ou mais membros no archive\n");                                                               \
        fprintf(fd, "\t    Caso membro ja estaja no archive ele sera substituido\n\n");                                             \
        fprintf(fd, "\t-a: Funciona igual a opcao -i, porem a substituicao so ocorre se o membro passado for mais recente \n\n");   \
        fprintf(fd, "\t-m <target>: Move o membro indicado na linha de comando, para depois do membro target\n\n");                 \
        fprintf(fd, "\t-x: Extrai os membros indicados do arquive\n");                                                              \
        fprintf(fd, "\t    Caso nenhum membro seja passado todos sao extraidos\n\n");                                               \
        fprintf(fd, "\t-r: Remove os membros indicados do arquive\n\n");                                                            \
        fprintf(fd, "\t-c: Lista o conteudo do archive\n");                                                                         \
        fprintf(fd, "\t    <name> <UID> <permissions> <size> <date>\n\n");                                                          \
        fprintf(fd, "\t-h: Imprime essas menssagens\n\n");                                                                          \
    }while(0)                                                                                                                       \

#endif
