#ifndef DATA_HANDLE_H_
#define DATA_HANDLE_H_
#include <stdio.h>
#include "dir.h"

// Move uma parte dos dados do archive comecando em start_point, movendo 
// window_size dados para o lado esquerdo para a posicao where
int copy_data_window(FILE *archive, size_t start_point, size_t window_size, size_t where);

// insere o conteudo de f_in_name na localizao dada por mmmd no archive
size_t fdumpf(FILE *archive, char *f_in_name, struct memb_md_t *mmd);
#endif
