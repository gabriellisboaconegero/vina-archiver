#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "dir.h"
#include "macros.h"
#include "archive.h"

#define F_PATH_IN "./teste/teste.in"
#define F_ARCHIVE "./teste/archive.vpp"

//TODO: fazer de uma forma melhor o remove. ta bem ruim
//TODO: dando erro quando remove e nenhum nome eh removido
int op_remove(char *archive_name, char **f_names, int names_sz){
    FILE *archive;
    struct metad_t md;
    struct memb_md_t *mmd, *mmd_ant, *aux;

    if (!init_meta(&md, 0)){
        PERRO("%s", "Inalcancavel");
        exit(1);
    }

    archive = fopen(archive_name, "r+");
    if (archive == NULL){
        PERRO("Nao foi possivel abrir o arquivo [%s]", archive_name);
        exit(1);
    }

    if (!get_meta(archive, &md)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        exit(1);
    }

    // trunca para retirar o diretorio
    // melhorando a logica do remove isso mudo tambem
    ftruncate(fileno(archive), md.tail->off_set + md.tail->m_size);
    fseek(archive, 0, SEEK_END);

    for (int i = 0; i < names_sz; i++){
        printf("Removendo arquivo [%s]\n", f_names[i]);
        mmd = busca_membro(f_names[i], &mmd_ant, &md);
        if (mmd == NULL)
            ERRO("Nao foi possivel remover o arquivo [%s], nao existe em [%s]",
                    f_names[i], archive_name);
        else{
            if (!remove_file(archive, mmd)){
                PERRO("Nao foi possivel remover o arquivo [%s]", mmd->name);
                exit(1);
            }

            // atualizar o off_set dos membros depois do membro a ser removido
            aux = mmd->prox;
            while (aux != NULL){
                aux->off_set -= mmd->m_size;
                aux = aux->prox;
            }

            //caso mmd seja tail
            if (mmd == md.tail)
                md.tail = mmd_ant;

            // Caso mmd nao seja md.head
            if (mmd_ant != NULL)
                mmd_ant->prox = mmd->prox;
            else
                md.head = mmd->prox;
            mmd = destroi_membro(mmd);
            md.memb_sz--;
        }
    }
    dump_meta(archive, &md);

    print_meta(&md);
    fclose(archive);

    return 1;
}

// ta extraindo tudo por agora
// TODO: Criar os diretorios necessarios
int op_extrair(char *archive_name, char **f_names, int names_sz, int extract_all){
    // extrair, usar o mode "r"
    FILE *archive;
    struct memb_md_t *mmd, *mmd_ant;
    struct metad_t md;
    if (!init_meta(&md, 0)){
        ERRO("%s", "Inalcancavel");
        exit(1);
    }

    if (archive_name == NULL)
        return 0;

    archive = fopen(archive_name, "r");
    if (archive == NULL){
        PERRO("[%s]", archive_name);
        exit(1);
    }

    if (!get_meta(archive, &md)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        exit(1);
    }

    // se for extrair por nome
    if (!extract_all){
        for (int i = 0; i < names_sz; i++){
            printf("Extraindo arquivo [%s]\n", f_names[i]);
            mmd = busca_membro(f_names[i], &mmd_ant, &md);
            if (mmd == NULL)
                ERRO("Nao foi possivel extrair o arquivo [%s], nao existe em [%s]",
                        f_names[i], archive_name);
            else{
                if (!extract_file(archive, mmd)){
                    PERRO("Nao foi possivel extrair o aquivo [%s]", mmd->name);
                    exit(1);
                }
            }
        }
    }else{
        mmd = md.head;
        while (mmd != NULL){
            printf("Extraindo arquivo [%s]\n", mmd->name);
            if (!extract_file(archive, mmd)){
                PERRO("Nao foi possivel extrair o aquivo [%s]", mmd->name);
                exit(1);
            }
            mmd = mmd->prox;
        }
    }
    fclose(archive);

    return 1;
}

// retorna 0 se algo der errado
// 1 caso contrario
// TODO: apagar arquivos criados caso de algum erro
// TODO: substituir o arquivo passado se ele ja tiver la
int op_inserir(char *archive_name, char **f_names, int names_sz){
    // abrir com r+ deixa escrever e ler sem apagar nada
    FILE *archive;
    char *f_in_name;
    struct metad_t md;
    if (!init_meta(&md, 0)){
        ERRO("%s", "Inalcancavel");
        exit(1);
    }

    if (archive_name == NULL || f_names == NULL)
        return 0;

    if (names_sz == 0)
        return 1;

    archive = fopen(archive_name, "r+");
    // nao existia o arquivo .vpp
    if (archive == NULL){
        // criando caso nao exista
        archive = fopen(F_ARCHIVE, "w");
        if (archive == NULL){
            PERRO("nao foi possivel abrir o arquivo [%s]", F_ARCHIVE);
            exit(1);
        }
    }else if (!get_meta(archive, &md)){
        PERRO("nao foi possivel pegar os metadados do arquivo [%s]", F_ARCHIVE);
        exit(1);
    }

    for (int i = 0; i < names_sz; i++){
        f_in_name = f_names[i];
        printf("Inserindo arquivo [%s]\n", f_in_name);
        if (!insere_file(archive, f_in_name, &md))
            PERRO("Nao foi possivle inserir arquivo [%s]", f_in_name);
    }
    if (!dump_meta(archive, &md)){
        ERRO("%s", "Inalcancavel");
        exit(1);
    }
    fclose(archive);

    return 1;
}

void op_list(char *archive_name){
    // extrair, usar o mode "r"
    FILE *archive;
    struct metad_t md;
    if (!init_meta(&md, 0)){
        ERRO("%s", "Inalcancavel");
        exit(1);
    }

    if (archive_name == NULL)
        return;

    archive = fopen(archive_name, "r");
    if (archive == NULL){
        PERRO("[%s]", archive_name);
        exit(1);
    }

    if (!get_meta(archive, &md)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        exit(1);
    }
    
    print_meta(&md);
    if (!destroi_meta(&md)){
        ERRO("%s", "Inalcancavel");
        exit(1);
    }
}

int main(int argc, char **argv){
    //op_inserir(F_ARCHIVE, &argv[1], argc - 1);
    //op_extrair(F_ARCHIVE, &argv[1], argc - 1, 0);
    op_remove(F_ARCHIVE, &argv[1], argc - 1);
    //op_list(F_ARCHIVE);
    return 0;
}
