#ifndef DIR_H_
#define DIR_H_
#include <stdio.h>
#include "macros.h"
#define MMD_NAME_SZ(mmd) ((mmd)->size - SIZE_OF_MMD)

//TODO: ver alocacao ou nao do nome
struct memb_md_t {
    size_t size;            // tamanho do metadado, contado o nome
    char *name;             // Nome do arquivo
    size_t off_set;         // off set em relacao ao inicio do archiver
    size_t m_size;          // tamanho do membro
    struct memb_md_t *prox; // ptr para o proximo
};

struct metad_t {
    struct memb_md_t *head;
    struct memb_md_t *tail;
    size_t memb_sz;
};

// inicializa a estrutura de metadados
int init_meta(struct metad_t *md, size_t memb_sz);

// Libera memoria alocada para os metadados
int destroi_meta(struct metad_t *md);

// aloca memoria para membro no diretorio
struct memb_md_t *cria_membro();

// libera memoria
struct memb_md_t *destroi_membro(struct memb_md_t *mmd);

// busca membro pelo nome dele, retorna-o
// o mmd_ant eh atualizado para o membro anterior ao
// retornado. Se o mmd achado for a head, mmd_ant vai ser NULL
struct memb_md_t *busca_membro(char *name, struct memb_md_t **mmd_ant, struct metad_t *md);

// retorna 0 em caso de erro
// 1 caso contrario
int get_meta(FILE *archive, struct metad_t *md);

// Joga os netadados para um arquivo
int dump_meta(FILE *archive, struct metad_t *md);

// mostra no stdout o conteudo dos metadados
void print_meta(struct metad_t *md);

// mostra no stout o conteudo do membro
void print_membro(struct memb_md_t *mmd);

#endif
