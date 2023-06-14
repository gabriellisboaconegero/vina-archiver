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
    int f_rm = 0;
    size_t meta_sz;
    struct metad_t *md;
    struct memb_md_t *mmd;

    archive = fopen(archive_name, "r+");
    if (archive == NULL){
        PERRO("Nao foi possivel abrir o arquivo [%s]", archive_name);
        exit(1);
    }

    if (!get_meta_size(archive, &meta_sz)){
        PERRO("Nao foi possivel pegar quantos membros tem no arquivo [%s]", archive_name);
        exit(1);
    }

    md = init_meta(meta_sz);
    if (md == NULL){
        ERRO("%s", MEM_ERR_MSG);
        exit(1);
    }

    if (!get_meta(archive, md, meta_sz)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        exit(1);
    }
    
    // trunca para retirar o diretorio
    // TODO: melhorando a logica do remove isso mudo tambem
    mmd = md->membs[md->memb_sz - 1];
    ftruncate(fileno(archive), mmd->off_set + mmd->m_size);
    fseek(archive, 0, SEEK_END);

    for (int i = 0; i < names_sz && !f_rm; i++){
        printf("Removendo arquivo [%s]\n", f_names[i]);
        switch (remove_file(archive, f_names[i], md)){
            case NULL_PARAM:{
                PERRO("Nao foi possivel remover o arquivo [%s]", f_names[i]);
                exit(1);
            }; break;

            case NO_MEMBRO:
                ERRO("Nao foi possivel retirar o arquivo [%s]", f_names[i]); break;

            case REMOVE_FILE:{
                printf("Arquivo [%s] vazio, excluindo ele\n", archive_name);
                if (remove(archive_name) == -1){
                    PERRO("[%s]", archive_name);
                    exit(1);
                }
                f_rm = 1;
            }; break;
        }
    }

    if (!f_rm)
        dump_meta(archive, md);

    fclose(archive);
    print_meta(md);
    destroi_meta(md);

    return 1;
}

// TODO: Criar os diretorios necessarios
int op_extrair(char *archive_name, char **f_names, int names_sz, int extract_all){
    FILE *archive;
    size_t meta_sz, index;
    struct memb_md_t *mmd;
    struct metad_t *md;
    if (archive_name == NULL)
        return 0;

    archive = fopen(archive_name, "r");
    if (archive == NULL){
        PERRO("[%s]", archive_name);
        exit(1);
    }

    if (!get_meta_size(archive, &meta_sz)){
        PERRO("Nao foi possivel pegar quantos membros tem no arquivo [%s]", archive_name);
        exit(1);
    }

    md = init_meta(meta_sz);
    if (md == NULL){
        ERRO("%s", MEM_ERR_MSG);
        exit(1);
    }

    if (!get_meta(archive, md, meta_sz)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        exit(1);
    }

    // se for extrair por nome
    if (!extract_all){
        for (int i = 0; i < names_sz; i++){
            printf("Extraindo arquivo [%s]\n", f_names[i]);
            if (!busca_membro(f_names[i], md, &index))
                ERRO("Nao foi possivel extrair o arquivo [%s], nao existe em [%s]",
                        f_names[i], archive_name);
            else{
                mmd = md->membs[index];
                if (!extract_file(archive, mmd)){
                    PERRO("Nao foi possivel extrair o aquivo [%s]", mmd->name);
                    exit(1);
                }
            }
        }
    }else{
        for (size_t i = 0; i < md->memb_sz; i++){
            mmd = md->membs[i];
            printf("Extraindo arquivo [%s]\n", mmd->name);
            if (!extract_file(archive, mmd)){
                PERRO("Nao foi possivel extrair o aquivo [%s]", mmd->name);
                exit(1);
            }
        }
    }
    fclose(archive);
    print_meta(md);
    destroi_meta(md);

    return 1;
}

// retorna 0 se algo der errado
// 1 caso contrario
// TODO: apagar arquivos criados caso de algum erro
// TODO: substituir o arquivo passado se ele ja tiver la
int op_inserir(char *archive_name, char **f_names, int names_sz){
    // abrir com r+ deixa escrever e ler sem apagar nada
    FILE *archive;
    size_t meta_sz, index;
    char *f_in_name;
    struct metad_t *md;

    if (archive_name == NULL || f_names == NULL)
        return 0;

    if (names_sz == 0)
        return 1;

    archive = fopen(archive_name, "r+");
    // nao existia o arquivo .vpp
    if (archive == NULL){
        // criando caso nao exista
        archive = fopen(archive_name, "w");
        if (archive == NULL){
            PERRO("nao foi possivel abrir o arquivo [%s]", archive_name);
            exit(1);
        }
        md = init_meta(names_sz);
        if (md == NULL){
            ERRO("%s", MEM_ERR_MSG);
            exit(1);
        }
    }else{
        if (!get_meta_size(archive, &meta_sz)){
            PERRO("Nao foi possivel pegar quantos membros tem no arquivo [%s]", archive_name);
            exit(1);
        }

        meta_sz += names_sz;
        md = init_meta(meta_sz);
        if (md == NULL){
            ERRO("%s", MEM_ERR_MSG);
            exit(1);
        }

        if (!get_meta(archive, md, meta_sz - names_sz)){
            PERRO("nao foi possivel pegar os metadados do arquivo [%s]", archive_name);
            exit(1);
        }
    }

    for (int i = 0; i < names_sz; i++){
        f_in_name = f_names[i];
        if (!busca_membro(f_in_name, md, &index)){
            printf("Inserindo arquivo [%s]\n", f_in_name);
            if (!insere_file(archive, f_in_name, md))
                PERRO("Nao foi possivel inserir arquivo [%s]", f_in_name);
        }else{
            printf("Substituindo arquivo [%s]\n", f_in_name);
            if (!substitui_file(archive, index, md)){
                PERRO("Nao foi possivel substituir o arquivo [%s]", f_in_name);
                exit(1);
            }
        }
    }

    dump_meta(archive, md);
    fclose(archive);
    print_meta(md);
    destroi_meta(md);

    return 1;
}

void op_list(char *archive_name){
    // extrair, usar o mode "r"
    FILE *archive;
    size_t meta_sz;
    struct metad_t *md;
    if (archive_name == NULL)
        return;

    archive = fopen(archive_name, "r");
    if (archive == NULL){
        PERRO("[%s]", archive_name);
        exit(1);
    }

    if (!get_meta_size(archive, &meta_sz)){
        PERRO("Nao foi possivel pegar quantos membros tem no arquivo [%s]", archive_name);
        exit(1);
    }

    md = init_meta(meta_sz);
    if (md == NULL){
        ERRO("%s", MEM_ERR_MSG);
        exit(1);
    }

    if (!get_meta(archive, md, meta_sz)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        exit(1);
    }
    
    fclose(archive);
    print_meta(md);
    destroi_meta(md);
}

int main(int argc, char **argv){
    int option, o_i, o_a, o_m, o_x, o_r, o_c;
    char *target = NULL, *archive_name = NULL;
    char last_set = ' ';
    o_i = o_a = o_m = o_x = o_r = o_c = 0;
    
    while ((option = getopt(argc, argv, "iaxrchm:")) != -1){
        switch (option){
            case 'i':
                if (last_set != ' '){
                    ERRO("Opcoes -i e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_i = 1;
                last_set = 'i';
                break;
            case 'a':
                if (last_set != ' '){
                    ERRO("Opcoes -a e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_a = 1;
                last_set = 'a';
                break;
            case 'm':
                target = optarg;
                if (last_set != ' '){
                    ERRO("Opcoes -m e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_m = 1;
                last_set = 'm';
                break;
            case 'x':
                if (last_set != ' '){
                    ERRO("Opcoes -x e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_x = 1;
                last_set = 'x';
                break;
            case 'r':
                if (last_set != ' '){
                    ERRO("Opcoes -r e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_r = 1;
                last_set = 'r';
                break;
            case 'c':
                if (last_set != ' '){
                    ERRO("Opcoes -c e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_c = 1;
                last_set = 'c';
                break;
            case 'h':
                USAGE(stdout);
                return 0;
            default:
                USAGE(stderr);
                return 1;
        }
    }

    //op_inserir(F_ARCHIVE, &argv[1], argc - 1);
    //op_extrair(F_ARCHIVE, &argv[1], argc - 1, 1);
    //op_remove(F_ARCHIVE, &argv[1], argc - 1);
    //op_list(F_ARCHIVE);
    return 0;
}
