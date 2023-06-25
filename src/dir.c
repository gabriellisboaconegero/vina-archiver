#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>
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
        fread(&mmd->pos, sizeof(mmd->pos), 1, archive);
        fread(&mmd->st_uid, sizeof(mmd->st_uid), 1, archive);
        fread(&mmd->st_mode, sizeof(mmd->st_mode), 1, archive);
        fread(&mmd->st_mtim, sizeof(mmd->st_mtim), 1, archive);
    }

    return 1;
}

int dump_meta(FILE *archive, struct metad_t *md){
    struct memb_md_t *mmd;
    size_t inicio_meta;

    if (archive == NULL || md == NULL)
        return FAIL;
    
    if (md->memb_sz == 0)
        return NO_META;

    // Pega off_set dos metadados para colocar no final 
    inicio_meta = SIZE_OF_MEMBS(md);
    fseek(archive, inicio_meta, SEEK_SET);

    // quantos membros tem
    fwrite(&md->memb_sz, sizeof(md->memb_sz), 1, archive);
    for (size_t i = 0; i < md->memb_sz; i++){
        mmd = md->membs[i];
        // arruma as posicoes antes de imprimir
        mmd->pos = i;
        fwrite(&mmd->size, sizeof(mmd->size), 1, archive);
        // Pega o tamanho do nome
        fwrite(mmd->name, 1, MMD_NAME_SZ(mmd), archive);
        fwrite(&mmd->m_size, sizeof(mmd->m_size), 1, archive);
        fwrite(&mmd->off_set, sizeof(mmd->off_set), 1, archive);
        fwrite(&mmd->pos, sizeof(mmd->pos), 1, archive);
        fwrite(&mmd->st_uid, sizeof(mmd->st_uid), 1, archive);
        fwrite(&mmd->st_mode, sizeof(mmd->st_mode), 1, archive);
        fwrite(&mmd->st_mtim, sizeof(mmd->st_mtim), 1, archive);
    }
    // off set de onde comeca o diretorio
    fwrite(&inicio_meta, sizeof(size_t), 1, archive);
    // truncar caso tenha acontecido alguma remocao ou
    // substituicao
    ftruncate(fileno(archive), ftell(archive));

    return OK;
}

void lista_meta(struct metad_t *md){
    struct memb_md_t *mmd;
    if (md == NULL)
        return;

    for (size_t i = 0; i < md->memb_sz; i++){
        mmd = md->membs[i];
        mmd->pos = i;
        lista_membro(mmd);
    }
}

void print_meta(struct metad_t *md){
    struct memb_md_t *mmd;
    if (md == NULL)
        return;

    printf("nº membros: %ld\n", md->memb_sz);
    printf("nº     offset      size name\n");
    for (size_t i = 0; i < md->memb_sz; i++){
        mmd = md->membs[i];
        mmd->pos = i;
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
    if (md->memb_sz > md->cap){
        free(mmd);
        return (struct memb_md_t *)NULL;
    }
    md->membs[id] = mmd;

    return mmd;
}

struct memb_md_t *destroi_membro(struct memb_md_t *mmd){
    if (mmd == NULL)
        return (struct memb_md_t *)NULL;
    if (mmd->name != NULL)
        free(mmd->name);
    free(mmd);
    return (struct memb_md_t *)NULL;
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

int substitui_membro(struct metad_t *md, size_t index, struct stat st){
    if (md == NULL)
        return 0;

    md->membs[index]->m_size = st.st_size;
    md->membs[index]->st_uid = st.st_uid;
    md->membs[index]->st_mode = st.st_mode;
    md->membs[index]->st_mtim = st.st_mtime;
    for (size_t i = index + 1; i < md->memb_sz; i++)
        md->membs[i]->off_set = md->membs[i - 1]->off_set + 
                                md->membs[i - 1]->m_size;
    return 1;
}

static int swap_mmd(struct metad_t *md, size_t id1, size_t id2){
    struct memb_md_t *temp;
    if (md == NULL)
        return 0;

    temp = md->membs[id1];
    md->membs[id1] = md->membs[id2];
    md->membs[id2] = temp;

    return 1;
}

int move_membro(struct metad_t *md, size_t index, size_t index_parent){
    size_t index_to_update;
    if (md == NULL)
        return 0;

    if (index >= md->memb_sz || index_parent >= md->memb_sz){
        ERRO("Nao foi possivel mover o elemento de index %ld para depois de %ld",
                index, index_parent);
        return 0;
    }

    //   i  p
    // 0123456
    // abcdefg
    //

    // Se o membro for movido para uma posicao menor
    if (index > index_parent){
        for (size_t i = index; i > index_parent + 1; i--)
            swap_mmd(md, i, i - 1);
        index_to_update = index_parent + 1;
    }
    else{
        for (size_t i = index; i < index_parent; i++)
            swap_mmd(md, i, i + 1);

        index_to_update = index;
        if (index == 0){
            md->membs[index]->off_set = 0L;
            index_to_update++;
        }
    }

    // atualiza os off_set
    for (size_t i = index_to_update; i < md->memb_sz; i++)
        md->membs[i]->off_set = md->membs[i - 1]->off_set + 
                                md->membs[i - 1]->m_size;
    
    return 1;
}

int busca_membro(char *name, struct metad_t *md, size_t *id){
    int is_dir;
    size_t name_sz;
    struct memb_md_t *mmd;

    if (name == NULL || md == NULL)
        return 0;

    for (size_t i = 0; i < md->memb_sz; i++){
        mmd = md->membs[i];
        // se for diretorio, tirar o './' para comparar
        is_dir = strchr(mmd->name, '/') != NULL;
        name_sz = strlen(mmd->name) - 2 * is_dir;
        if (strncmp(mmd->name + 2 * is_dir,   // se for dir pula o './'
                    name + (name[0] == '/'),  // Se comeca com '/' pular
                    name_sz) == 0)
        {
            *id = i;
            return 1;
        }
    }

    return 0;
}

static void print_mode(mode_t m){
    // era para ser o de diretorio, mas nao eh inserido diretorio
    // entao apenas mostra - para ficar o tar -tvf
    printf("-");
    printf(m & S_IRUSR ? "r" : "-");
    printf(m & S_IWUSR ? "w" : "-");
    printf(m & S_IXUSR ? "x" : "-");
    printf(m & S_IRGRP ? "r" : "-");
    printf(m & S_IWGRP ? "w" : "-");
    printf(m & S_IXGRP ? "x" : "-");
    printf(m & S_IROTH ? "r" : "-");
    printf(m & S_IWOTH ? "w" : "-");
    printf(m & S_IXOTH ? "x" : "-");
}

void lista_membro(struct memb_md_t *mmd){
    char strtime[MAX_T_SZ] = {0};
    if (mmd == NULL){
        printf("[NULL]\n");
        return;
    }
    // format time como tar -tvf
    strftime(strtime, MAX_T_SZ, "%Y-%m-%d %H:%M", localtime(&mmd->st_mtim));

    print_mode(mmd->st_mode);
    printf(" %-10s %9ld %s (%3ld) %s\n",
            getpwuid(mmd->st_uid)->pw_name,
            mmd->m_size,
            strtime,
            mmd->pos,
            mmd->name);
}

void print_membro(struct memb_md_t *mmd){
    if (mmd == NULL){
        printf("!!!MEMBRO NULO\n");
        return;
    }

    printf("%2ld: %9ld %9ld %s\n",
            mmd->pos,
            mmd->off_set,
            mmd->m_size,
            mmd->name);
}
// MEMBRO
