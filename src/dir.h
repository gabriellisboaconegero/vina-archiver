#ifndef DIR_H_
#define DIR_H_
#include <stdio.h>
#include "macros.h"
#define MMD_NAME_SZ(mmd) ((mmd)->size - SIZE_OF_MMD)

// TODO: colocar uma propriedade de ordem inserida
struct memb_md_t {
    // size_t dir_id;
    size_t size;            // tamanho do metadado, contado o nome
    char *name;             // Nome do arquivo
    size_t off_set;         // off set em relacao ao inicio do archiver
    size_t m_size;          // tamanho do membro
};

struct metad_t{
    struct memb_md_t **membs;
    size_t cap;
    size_t memb_sz;
};

// META
// inicializa a estrutura de metadados
struct metad_t *init_meta(size_t memb_sz);

// Libera memoria alocada para os metadados
struct metad_t *destroi_meta(struct metad_t *md);

// pega a quantiddade de membros que tem no arquivo
int get_meta_size(FILE *archive, size_t *meta_sz);

// retorna 0 em caso de erro
// 1 caso contrario
int get_meta(FILE *archive, struct metad_t *md, size_t n_membs);

// Joga os netadados para um arquivo
int dump_meta(FILE *archive, struct metad_t *md);

// mostra no stdout o conteudo dos metadados
void print_meta(struct metad_t *md);
// META

// MEMBRO
// insere membro novo no meta dado
struct memb_md_t *add_membro(struct metad_t *md);

// Remove membro do meta dado
int remove_membro(struct metad_t *md, size_t index);

// libera memoria
struct memb_md_t *destroi_membro(struct memb_md_t *mmd);

// Busca o membor pelo nome passado, retornando se achou
// retorna -1 se nao achar
// o index e retornado pelo ponteiro passado
int busca_membro(char *name, struct metad_t *md, size_t *id);

// mostra no stout o conteudo do membro
void print_membro(struct memb_md_t *mmd);
// MEMBRO

#endif
