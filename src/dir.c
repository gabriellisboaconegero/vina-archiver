#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "macros.h"
#include "dir.h"

// META
struct metad_t *init_meta(size_t max_cap){
    struct metad_t *md = calloc(1, sizeof(struct metad_t));
    if (md == NULL)
        return (struct metad_t *)NULL;

    md->membs = calloc(max_cap, sizeof(struct memb_md_t *));
    if  (md->membs == NULL){
        free(md);
        return (struct metad_t *)NULL;
    }
    md->memb_sz = 0;
    md->cap = max_cap;

    return md;
}

struct metad_t *destroi_meta(struct metad_t *md){
    if (md == NULL)
        return (struct metad_t *)NULL;
    for (size_t i = 0; i < md->memb_sz; i++)
        destroi_membro(md->membs[i]);

    free(md->membs);
    free(md);
    md = NULL;

    return md;
}

// retorna 0 em caso de haver algum erro
// 1 caso contrario
int get_meta_size(FILE *archive, size_t *meta_sz){
    size_t ini_meta;

    if (archive == NULL || meta_sz == NULL)
        return 0;

    // Pula para pegar o off set que comeca o dir
    if (fseek(archive, -sizeof(size_t), SEEK_END) == -1)
        return 0;

    if (fread(&ini_meta, sizeof(size_t), 1, archive) != 1)
        return 0;

    // Pula para o inicio do dir
    if (fseek(archive, ini_meta, SEEK_SET) == -1)
        return 0;

    // le a quantidade de membros
    if (fread(meta_sz, sizeof(size_t), 1, archive) != 1)
        return 0;

    return 1;
}

// retorna 0 em caso de erro
// 1 caso cotrario
// OBS: responsabilidade de quem chamar a funcao de colocar
// o ponteiro de leitura no inicio do diretorio
int get_meta(FILE *archive, struct metad_t *md, size_t n_membs){
    struct memb_md_t *mmd;
    size_t name_sz;

    if (archive == NULL || md == NULL)
        return 0;

    for (size_t i = 0; i < n_membs; i++){
        mmd = add_membro(md);
        if (mmd == NULL){
            ERRO("%s", MEM_ERR_MSG);
            return 0;
        }
        // referencia para o membro
        fread(&mmd->size, sizeof(mmd->size), 1, archive);
        name_sz = MMD_NAME_SZ(mmd);
        // aloca memoria com '\0' em todo o nome
        // + 1 para ser cstr
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

// OBS: responsabilidade de quem chamar dump_meta
// de arrumar a posicao do ponteiro de escrita
int dump_meta(FILE *archive, struct metad_t *md){
    struct memb_md_t *mmd;
    size_t inicio_meta;

    if (archive == NULL || md == NULL)
        return 0;

    // Pega off_set dos metadados para colocar no final 
    inicio_meta = ftell(archive);

    // quantos membros tem
    fwrite(&md->memb_sz, sizeof(md->memb_sz), 1, archive);
    for (size_t i = 0; i < md->memb_sz; i++){
        mmd = md->membs[i];
        fwrite(&mmd->size, sizeof(mmd->size), 1, archive);
        // Pega o tamanho do nome
        fwrite(mmd->name, 1, MMD_NAME_SZ(mmd), archive);
        fwrite(&mmd->m_size, sizeof(mmd->m_size), 1, archive);
        fwrite(&mmd->off_set, sizeof(mmd->off_set), 1, archive);
    }
    // off set de onde comeca o diretorio
    fwrite(&inicio_meta, sizeof(size_t), 1, archive);
    // truncar caso tenha acontecido alguma remocao ou
    // substituicao
    ftruncate(fileno(archive), ftell(archive));

    return 1;
}

void print_meta(struct metad_t *md){
    struct memb_md_t *mmd;
    if (md == NULL)
        return;

    printf("nº membros: %ld\n", md->memb_sz);
    for (size_t i = 0; i < md->memb_sz; i++){
        mmd = md->membs[i];
        printf("membro %ld\n", i);
        print_membro(mmd);
    }
}
// META

// MEMBRO
struct memb_md_t *add_membro(struct metad_t *md){
    struct memb_md_t *mmd;
    size_t id;
    mmd = calloc(1, sizeof(struct memb_md_t));
    if (mmd == NULL)
        return (struct memb_md_t *)NULL;

    id = md->memb_sz++;
    md->membs[id] = mmd;
    if (md->memb_sz > md->cap){
        free(md->membs[id]);
        md->membs[id] = NULL;
        return (struct memb_md_t *)NULL;
    }

    return mmd;
}

int remove_membro(struct metad_t *md, size_t index){
    struct memb_md_t *mmd;
    if (md == NULL || index >= md->memb_sz)
        return 0;

    mmd = md->membs[index];
    for (size_t i = index + 1; i < md->memb_sz; i++){
        // arruma off set dos proximos
        md->membs[i]->off_set -= mmd->m_size;
        // shifta para index anterior
        md->membs[i - 1] = md->membs[i];
    }
    md->memb_sz--;
    destroi_membro(mmd);

    return 1;
}

struct memb_md_t *destroi_membro(struct memb_md_t *mmd){
    if (mmd == NULL)
        return (struct memb_md_t *)NULL;
    if (mmd->name != NULL)
        free(mmd->name);
    free(mmd);
    return (struct memb_md_t *)NULL;
}

int busca_membro(char *name, struct metad_t *md, size_t *id){
    size_t name_sz;

    if (name == NULL || md == NULL)
        return 0;

    for (size_t i = 0; i < md->memb_sz; i++){
        name_sz = strlen(md->membs[i]->name);
        if (strncmp(md->membs[i]->name, name, name_sz) == 0){
            *id = i;
            return 1;
        }
    }

    return 0;
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
// MEMBRO
