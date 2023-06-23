#ifndef ARCHIVE_H_
#define ARCHIVE_H_
#include <stdio.h>
#include "dir.h"
#define REMOVE_OK 1
#define REMOVE_FILE 2
#define REMOVE_FAIL 3

// Move uma parte dos dados do archive comecando em start_point, movendo 
// window_size dados para o lado esquerdo para a posicao where
int copy_data_window(FILE *archive, size_t start_point, size_t window_size, size_t where);

// extrai o membro passado para o arquivo de mesmo nome
int extract_file(FILE *archive, struct memb_md_t *mmd);

// Escreve o conteudodo membro no archive
size_t fdumpf(FILE *archive, char *f_in_name, struct memb_md_t *mmd);

// remove um arquivo do archive, atualiza os membros
int remove_file(FILE *archive, size_t index, struct metad_t *md);

// insere um novo membro, atualizando os metadados
int insere_file(FILE *archive, char *name, struct metad_t *md);

// substitui um arquivo, coloca ele no final do archive
int substitui_file(FILE *archive, size_t index, struct metad_t *md);

// atualiza um arquivo, subtituindo ele se o parament
int atualiza_file(FILE *archive, size_t index, struct metad_t *md);

// move conteudo de membro de com index para depois
// do conteudo do membro de index_parent
int move_file(FILE *archive, size_t index, size_t index_parent, struct metad_t *md);

#endif
