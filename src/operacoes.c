#include <stdio.h>
#include "dir.h"
#include "archive.h"
#include "macros.h"

// retorna 1 em caso de erro
// 0 caso contrario
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

    // trunca para retirar o diretorio

    for (int i = 0; i < names_sz && !f_rm; i++){
#if DEBUG
        printf("Removendo arquivo [%s]\n", f_names[i]);
#endif

        if (!busca_membro(f_names[i], md, &index))
            ERRO("Nao foi possivel remover o arquivo [%s], nao existe em [%s] (Ignorando)",
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

    // se for extrair por nome
    if (names_sz > 0){
        for (int i = 0; i < names_sz; i++){
#if DEBUG
            printf("Extraindo arquivo [%s]\n", f_names[i]);
#endif 
            if (!busca_membro(f_names[i], md, &index))
                ERRO("Nao foi possivel extrair o arquivo [%s], nao existe em [%s] (Ignorando)",
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
                if (!atualiza_file(archive, index, md, f_in_name)){
                    PERRO("Nao foi possivel atualizar o arquivo [%s]", f_in_name);
                    fclose(archive);
                    destroi_meta(md);
                    return 1;
                }
            }else if (!substitui_file(archive, index, md, f_in_name)){
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
    
    if (!busca_membro(name_to_move, md, &index)){
        ERRO("Nao existe membro com nome [%s] para mover", name_to_move);
        destroi_meta(md);
        fclose(archive);
        return 1;
    }

    if (!busca_membro(name_parent, md, &index_parent)){
        ERRO("Nao existe membro com nome [%s]", name_parent);
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
    
    lista_meta(md);
#if DEBUG
    print_meta(md);
#endif
    fclose(archive);
    destroi_meta(md);

    return 0;
}
