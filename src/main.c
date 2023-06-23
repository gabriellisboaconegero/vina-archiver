#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "dir.h"
#include "macros.h"
#include "archive.h"

// retorna 1 em caso de erro
// 0 caso contrario
//TODO: fazer de uma forma melhor o remove. ta bem ruim
// Talvez usar um vetor de removidos, marcar elese e depois só mover os
// que sobraram
//TODO: dando erro quando remove e nenhum nome eh removido
int op_remove(char *archive_name, char **f_names, int names_sz){
    FILE *archive;
    int f_rm = 0;
    size_t meta_sz, index;
    struct metad_t *md;

    archive = fopen(archive_name, "r+");
    if (archive == NULL){
        PERRO("[%s]", archive_name);
        return 1;
    }

    if (!get_meta_size(archive, &meta_sz)){
        PERRO("Nao foi possivel pegar quantos membros tem no arquivo [%s]",
                archive_name);
        fclose(archive);
        return 1;
    }

    md = init_meta(meta_sz);
    if (md == NULL){
        ERRO("%s", MEM_ERR_MSG);
        fclose(archive);
        return 1;
    }

    if (!get_meta(archive, md, meta_sz)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        fclose(archive);
        destroi_meta(md);
        return 1;
    }
    
    // TODO: melhorando a logica do remove isso mudo tambem
    // TODO: ver se pode continuar removendo mesmo que nao de para remover
    // um dos arquivos

    // trunca para retirar o diretorio
    //mmd = md->membs[md->memb_sz - 1];
    //ftruncate(fileno(archive), mmd->off_set + mmd->m_size);

    for (int i = 0; i < names_sz && !f_rm; i++){
#if DEBUG
        printf("Removendo arquivo [%s]\n", f_names[i]);
#endif

        if (!busca_membro(f_names[i], md, &index))
            ERRO("Nao foi possivel remover o arquivo [%s], nao existe em [%s]",
                    f_names[i], archive_name);
        else{
            switch (remove_file(archive, index, md)){
                case REMOVE_FAIL:
                    PERRO("Nao foi possivel remover o arquivo [%s]", f_names[i]);
                    fclose(archive);
                    destroi_meta(md);
                    return 1;
                    break;
                case REMOVE_FILE:
#if DEBUG
                    printf("Arquivo [%s] vazio, excluindo ele\n", archive_name);
#endif
                    if (remove(archive_name) == -1){
                        PERRO("[%s]", archive_name);
                        fclose(archive);
                        destroi_meta(md);
                        return 1;
                    }
                    f_rm = 1;
                    break;
                case REMOVE_OK:
                    break;
            }
        }
    }

    if (!f_rm)
        dump_meta(archive, md);
#if DEBUG
    print_meta(md);
#endif
    fclose(archive);
    destroi_meta(md);

    return 0;
}

// retorna 1 em caso de erro
// 0 caso contrario
// TODO: Criar os diretorios necessarios
int op_extrair(char *archive_name, char **f_names, int names_sz){
    FILE *archive;
    size_t meta_sz, index;
    struct memb_md_t *mmd;
    struct metad_t *md;
    if (archive_name == NULL)
        return 1;

    archive = fopen(archive_name, "r");
    if (archive == NULL){
        PERRO("[%s]", archive_name);
        return 1;
    }

    if (!get_meta_size(archive, &meta_sz)){
        PERRO("Nao foi possivel pegar quantos membros tem no arquivo [%s]",
                archive_name);
        fclose(archive);
        return 1;
    }

    md = init_meta(meta_sz);
    if (md == NULL){
        ERRO("%s", MEM_ERR_MSG);
        fclose(archive);
        return 1;
    }

    if (!get_meta(archive, md, meta_sz)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        fclose(archive);
        destroi_meta(md);
        return 1;
    }

    // TODO: ver se pode continuar extraindo mesmo se nao der para extrair um
    // dos membros
    // se for extrair por nome
    if (names_sz > 0){
        for (int i = 0; i < names_sz; i++){
#if DEBUG
            printf("Extraindo arquivo [%s]\n", f_names[i]);
#endif 
            if (!busca_membro(f_names[i], md, &index))
                ERRO("Nao foi possivel extrair o arquivo [%s], nao existe em [%s]",
                        f_names[i], archive_name);
            else{
                mmd = md->membs[index];
                if (!extract_file(archive, mmd)){
                    PERRO("Nao foi possivel extrair o aquivo [%s]", mmd->name);
                    fclose(archive);
                    destroi_meta(md);
                    return 1;
                }
            }
        }
    }else{
        for (size_t i = 0; i < md->memb_sz; i++){
            mmd = md->membs[i];
#if DEBUG
            printf("Extraindo arquivo [%s]\n", mmd->name);
#endif 
            if (!extract_file(archive, mmd)){
                PERRO("Nao foi possivel extrair o aquivo [%s]", mmd->name);
                fclose(archive);
                destroi_meta(md);
                return 1;
            }
        }
    }

#if DEBUG
    print_meta(md);
#endif
    fclose(archive);
    destroi_meta(md);

    return 0;
}

// retorna 1 se algo der errado
// 0 caso contrario
// TODO: apagar arquivos criados caso de algum erro
int op_inserir(char *archive_name, char **f_names, int names_sz, int atualizar){
    // abrir com r+ deixa escrever e ler sem apagar nada
    FILE *archive;
    size_t meta_sz, index;
    char *f_in_name;
    struct metad_t *md;

    if (archive_name == NULL || f_names == NULL)
        return 1;

    if (names_sz == 0)
        return 1;

    archive = fopen(archive_name, "r+");
    // nao existia o arquivo .vpp
    if (archive == NULL){
        // criando caso nao exista
        archive = fopen(archive_name, "w");
        if (archive == NULL){
            PERRO("[%s]", archive_name);
            return 1;
        }
        md = init_meta(names_sz);
        if (md == NULL){
            ERRO("%s", MEM_ERR_MSG);
            fclose(archive);
            return 1;
        }
    }
    // mexistia o arquivo .vpp
    else{
        if (!get_meta_size(archive, &meta_sz)){
            PERRO("Nao foi possivel pegar quantos membros tem no arquivo [%s]",
                    archive_name);
            fclose(archive);
            return 1;
        }

        // passar o meta_sz + names_sz pois assim nao precisa realocar memoria
        meta_sz += names_sz;
        md = init_meta(meta_sz);
        if (md == NULL){
            ERRO("%s", MEM_ERR_MSG);
            fclose(archive);
            return 1;
        }

        // porem ao pegar os memebros que ja estao no arquivo, pega apenas os
        // que ja estavam la dentro. Ou seja, o novo meta_sz - names_sz. que eh
        // quantos membros tem no arvhive
        if (!get_meta(archive, md, meta_sz - names_sz)){
            PERRO("nao foi possivel pegar os metadados do arquivo [%s]", archive_name);
            fclose(archive);
            destroi_meta(md);
            return 1;
        }
    }

    for (int i = 0; i < names_sz; i++){
        f_in_name = f_names[i];
        // busca o membro, se nao achar insere
        // caso contrario vai substituir
        if (!busca_membro(f_in_name, md, &index)){
#if DEBUG
            printf("Inserindo arquivo [%s]\n", f_in_name);
#endif
            if (!insere_file(archive, f_in_name, md)){
                PERRO("Nao foi possivel inserir arquivo [%s]", f_in_name);
                destroi_meta(md);
                fclose(archive);
                return 1;
            }
        }
        // susbtituindo arquivo
        else{
#if DEBUG
            printf("Substituindo arquivo [%s]\n", f_in_name);
#endif
            if (atualizar){
                if (!atualiza_file(archive, index, md)){
                    PERRO("Nao foi possivel atyalizar o arquivo [%s]", f_in_name);
                    fclose(archive);
                    destroi_meta(md);
                    return 1;
                }
            }else if (!substitui_file(archive, index, md)){
                PERRO("Nao foi possivel substituir o arquivo [%s]", f_in_name);
                fclose(archive);
                destroi_meta(md);
                return 1;
            }
        }
    }

#if DEBUG
    print_meta(md);
#endif
    dump_meta(archive, md);
    fclose(archive);
    destroi_meta(md);

    return 0;
}

// imagino que -a significa atualizar
int op_atualiza(char *archive_name, char **f_names, int names_sz){

}

// retorna 1 se algo der errado
// 0 em caso contrario
int op_move(char *archive_name, char *name_to_move, char *name_parent){
    FILE *archive;
    size_t meta_sz, index, index_parent;
    struct metad_t *md;

    archive = fopen(archive_name, "r+");
    if (archive == NULL){
        PERRO("[%s]", archive_name);
        return 1;
    }

    if (!get_meta_size(archive, &meta_sz)){
        PERRO("Nao foi possivel pegar quantos membros tem no arquivo [%s]",
                archive_name);
        fclose(archive);
        return 1;
    }
    if (meta_sz == 1){
        ERRO("%s", "Archive tem apenas um membro");
        fclose(archive);
        return 1;
    }

    md = init_meta(meta_sz);
    if (md == NULL){
        ERRO("%s", MEM_ERR_MSG);
        fclose(archive);
        return 1;
    }

    if (!get_meta(archive, md, meta_sz)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        fclose(archive);
        destroi_meta(md);
        return 1;
    }

    // trunca para retirar o diretorio
    //mmd = md->membs[md->memb_sz - 1];
    //ftruncate(fileno(archive), mmd->off_set + mmd->m_size);
    
    if (!busca_membro(name_to_move, md, &index)){
        ERRO("Nao existe membro com nome [%s] para mover", name_to_move);
        destroi_meta(md);
        fclose(archive);
        return 1;
    }

    if (!busca_membro(name_parent, md, &index_parent)){
        ERRO("Nao existe membro com nome [%s] para mover [%s] para depois dele",
                name_to_move, name_parent);
        destroi_meta(md);
        fclose(archive);
        return 1;
    }

    // caso ja nao esteja na posicao correta ou seja o mesmo elemento
    if (index_parent != index && index != index_parent + 1){
        if (!move_file(archive, index, index_parent, md)){
            PERRO("Nao foi possivel mover [%s] para depois de [%s]",
                    name_to_move, name_parent);
            destroi_meta(md);
            fclose(archive);
            return 1;
        }

        dump_meta(archive, md);
    }

#if DEBUG
    print_meta(md);
#endif
    fclose(archive);
    destroi_meta(md);

    return 0;

}

// retorna 1 em caso de erro
// 0 caso contrario
int op_list(char *archive_name){
    FILE *archive;
    size_t meta_sz;
    struct metad_t *md;
    if (archive_name == NULL)
        return 1;

    archive = fopen(archive_name, "r");
    if (archive == NULL){
        PERRO("[%s]", archive_name);
        return 1;
    }

    if (!get_meta_size(archive, &meta_sz)){
        PERRO("Nao foi possivel pegar quantos membros tem no arquivo [%s]",
                archive_name);
        return 1;
    }

    md = init_meta(meta_sz);
    if (md == NULL){
        ERRO("%s", MEM_ERR_MSG);
        return 1;
    }

    if (!get_meta(archive, md, meta_sz)){
        PERRO("Nao foi possivel pegar os meta dados do arquivo [%s]", archive_name);
        return 1;
    }
    
    // TODO: implementar para listar metadados
    lista_meta(md);
#if DEBUG
    print_meta(md);
#endif
    fclose(archive);
    destroi_meta(md);

    return 0;
}

int main(int argc, char **argv){
    int option, o_i, o_a, o_m, o_x, o_r, o_c;
    int optindex, members_sz;
    char *target = NULL, *archive_name = NULL, **members_names;
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

    optindex = optind;
    if (argc <= optindex){
        ERRO("%s", "Faltando o parametro <archive>");
        USAGE(stderr);
        return 1;
    }

    archive_name = argv[optindex++];
    members_sz = argc - optindex;
    members_names = &argv[optindex];
    //TODO: verificar se todos os arquivos existem

    if (o_c)
        return op_list(archive_name);
    if (o_x)
        return op_extrair(archive_name, members_names, members_sz);

    if (members_sz <= 0){
        ERRO("Esta faltando a lista de membros para realizar a operacao -%c", last_set);
        USAGE(stderr);
        return 1;
    }

    if (o_i)
        return op_inserir(archive_name, members_names, members_sz);
    if (o_a){
        printf("Nao implementado opcao -a\n");
        return 0;
    }
    if (o_m){
        return op_move(archive_name, members_names[0], target);
    }
    if (o_r)
        return op_remove(archive_name, members_names, members_sz);

    printf("Inalcancavel\n");
    return 0;
}
