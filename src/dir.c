#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "macros.h"
#include "dir.h"

struct meta_t *init_meta(size_t max_cap){
    struct meta_t *md = calloc(1, sizeof(struct meta_t));
    if (md == NULL)
        return (struct meta_t *)NULL;

    md->membs = calloc(max_cap, sizeof(struct memb_md_t *));
    if  (md->membs == NULL){
        free(md);
        return (struct meta_t *)NULL;
    }
    md->memb_sz = 0;

    return md;
}

struct meta_t *destroi_meta(struct metad_t *md){
    struct memb_md_t *mmd, *aux;
    if (md == NULL)
        return (struct meta_t *)NULL;
    for (size_t i = 0; i < md->memb_sz; i++)
        destroi_membro(md->membs[i]);

    free(md->membs);
    free(md);
    md = NULL;

    return md;
}

// TODO: verificar se precisa checar todos o fread para erro
int get_meta(FILE *archive, struct metad_t *md){
    struct memb_md_t *mmd;
    size_t ini_dir, name_sz;

    if (archive == NULL || md == NULL)
        return 0;

    // Pula para pegar o off set que comeca o dir
    if (fseek(archive, -sizeof(size_t), SEEK_END) == -1)
        return 0;

    fread(&ini_dir, sizeof(size_t), 1, archive);
    // Pula para o inicio do dir
    if (fseek(archive, ini_dir, SEEK_SET) == -1)
        return 0;

    // le a quantidade de membros
    fread(&(md->memb_sz), sizeof(size_t), 1, archive);
    for (size_t i = 0; i < md->memb_sz; i++){
        mmd = add_membro(md);
        if (mmd == NULL){
            ERRO("%s", MEM_ERR_MSG);
            return 0;
        }
        // referencia para o membro
        fread(&mmd->size, sizeof(mmd->size), 1, archive);
        name_sz = MMD_NAME_SZ(mmd);
        // aloca memoria com '\0' em todo o nome
        mmd->name = calloc(name_sz + 1, sizeof(char));
        if (mmd->name == NULL){
            ERRO("%s", MEM_ERR_MSG);
            return 0;
        }
        fread(mmd->name, 1, name_sz, archive);
        fread(&mmd->m_size, sizeof(mmd->m_size), 1, archive);
        fread(&mmd->off_set, sizeof(mmd->off_set), 1, archive);
    }

    return 1;
}

// TODO: verificar se os fwrite funconam todos
int dump_meta(FILE *archive, struct metad_t *md){
    struct memb_md_t *mmd;
    size_t inicio_dir;

    if (archive == NULL || md == NULL)
        return 0;

    //TODO: os metadado seram jogados onde foi deixado o ponteiro
    // de escrever do arquive, arrumar ele dentro da funcao ou fora??
    // ou seja, decidir dentro ou fora onde acaba os dados
    inicio_dir = ftell(archive);

    // quantos membros tem
    fwrite(&md->memb_sz, sizeof(md->memb_sz), 1, archive);
    mmd = md->head;
    while (mmd != NULL){
        fwrite(&mmd->size, sizeof(mmd->size), 1, archive);
        // Pega o tamanho do nome
        fwrite(mmd->name, 1, MMD_NAME_SZ(mmd), archive);
        fwrite(&mmd->m_size, sizeof(mmd->m_size), 1, archive);
        fwrite(&mmd->off_set, sizeof(mmd->off_set), 1, archive);
        mmd = mmd->prox;
    }
    // off set de onde comeca o diretorio
    fwrite(&inicio_dir, sizeof(size_t), 1, archive);

    return 1;
}

struct memb_md_t *cria_membro(){
    return (struct memb_md_t *)calloc(1, sizeof(struct memb_md_t));
}

struct memb_md_t *destroi_membro(struct memb_md_t *mmd){
    if (mmd == NULL)
        return (struct memb_md_t *)NULL;
    free(mmd);
    return (struct memb_md_t *)NULL;
}

struct memb_md_t *busca_membro(char *name,
                               struct memb_md_t **mmd_ant,
                               struct metad_t *md)
{
    size_t name_sz;
    struct memb_md_t *mmd;

    if (name == NULL || md == NULL)
        return NULL;

    name_sz = strlen(name);
    *mmd_ant = NULL;
    mmd = md->head;
    while (mmd != NULL && !find){
        if (strncmp(name, mmd->name, name_sz) == 0)
            find = 1;
        *mmd_ant = mmd;
        mmd = mmd->prox;
    }

    return mmd;
}


void print_membro(struct memb_md_t *mmd){
    if (mmd == NULL){
        printf("!!!MEMBRO NULO\n");
        return;
    }
    printf("\tnome: %s\n", mmd->name);
    printf("\tmeta dado size: %ld\n", mmd->size);
    printf("\tmembro size: %ld\n", mmd->m_size);
    printf("\tmembro off set: %ld\n", mmd->off_set);
}

void print_meta(struct metad_t *md){
    struct memb_md_t *mmd;
    int i = 0;
    if (md == NULL)
        return;

    printf("nº membros: %ld\n", md->memb_sz);
    mmd = md->head;
    while (mmd != NULL){
        printf("membro %d\n", i);
        print_membro(mmd);
        mmd = mmd->prox;
        i++;
    }
}
