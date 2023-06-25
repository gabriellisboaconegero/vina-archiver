#ifndef DIR_H_
#define DIR_H_
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "macros.h"
#define MAX_T_SZ 128
#define MMD_NAME_SZ(mmd) ((mmd)->size - SIZE_OF_MMD)
#define LAST_MMD(md) ((md)->membs[(md)->memb_sz - 1])
#define SIZE_OF_MEMBS(md) (LAST_MMD(md)->off_set + LAST_MMD(md)->m_size)

struct memb_md_t {
    // size_t dir_id;
    size_t size;            // tamanho do metadado, contado o nome
    char *name;             // Nome do arquivo
    size_t off_set;         // off set em relacao ao inicio do archiver
    size_t m_size;          // tamanho do membro
    size_t pos;             // ordem no diretorio
    uid_t     st_uid;       // User ID
    mode_t    st_mode;      // Permisoes
    time_t st_mtim;         // Data de modificacao
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

// mostra no stdout o conteudo dos metadados
void lista_meta(struct metad_t *md);
// META

// MEMBRO
// insere membro novo no meta dado
struct memb_md_t *add_membro(struct metad_t *md);

// libera memoria
struct memb_md_t *destroi_membro(struct memb_md_t *mmd);

// Remove membro do meta dado
int remove_membro(struct metad_t *md, size_t index);

// substitui membro, atualizando os dados dele
int substitui_membro(struct metad_t *md, size_t index, struct stat st);

// move membro de index para depois de membro de index_parent
int move_membro(struct metad_t *md, size_t index, size_t index_parent);

// Busca o membor pelo nome passado, retornando 1 se achou
// e 0 em caso contrario
// o index e retornado pelo ponteiro passado
int busca_membro(char *name, struct metad_t *md, size_t *id);

// mostra no stout o conteudo do membro
void print_membro(struct memb_md_t *mmd);

// mostra no stout o conteudo do membro
void lista_membro(struct memb_md_t *mmd);
// MEMBRO

#endif
