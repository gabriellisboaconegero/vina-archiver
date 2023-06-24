#ifndef OPERACOES_H_
#define OPERACOES_H_

// Operacao de remover do archive
// dada uma lista de nomes remove eles do archive
int op_remove(char *archive_name, char **f_names, int names_sz);

// Operacao de extrair do archive
// extrai todos os arquivos se names_sz for 0
// caso conmtrario extrai os nomes passados, criando os diretoris necessarios
int op_extrair(char *archive_name, char **f_names, int names_sz);

// Operacao de inserir no archive
// Insere os arquivos passados, substituindo se ja estiver la dentro
int op_inserir(char *archive_name, char **f_names, int names_sz, int atualizar);

// Operacao de mover membros no archive
// Move o conteudo do membro 1 para depois do conteudo do membro parent
int op_move(char *archive_name, char *name_to_move, char *name_parent);

// Operacao de mover membros no archive
// Lista os arquivos no archive no estilo tar -tvf
int op_list(char *archive_name);
#endif
