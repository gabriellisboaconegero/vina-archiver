#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "macros.h"
#include "archive.h"
#include "data_handle.h"

// retorna FAIL em casso de erro
// FILE_MISSING se nao conseguir pegar o estat do name passado
// OK caso contrario
int insere_file(FILE *archive, char *name, struct metad_t *md){
    int is_dir;
    struct stat st;
    size_t f_sz, name_sz;
    struct memb_md_t *mmd, *mmd_anterior;

    if (archive == NULL || name == NULL || md == NULL)
        return FAIL;

    if (stat(name, &st) == -1)
        return FILE_MISSING;

    f_sz = st.st_size;

    mmd = add_membro(md);
    if (mmd == NULL){
        ERRO("%s", MEM_ERR_MSG);
        ERRO("%s", "Nao foi possivel inserir os arquivos no archive");
        return FAIL;
    }

    // se for diretorio coloca "./"
    is_dir = strchr(name, '/') != NULL;
    name_sz = strlen(name);
    if (is_dir)
        name_sz += 2;

    mmd->name = calloc(name_sz + 1, sizeof(char));
    if (mmd->name == NULL){
        ERRO("%s", MEM_ERR_MSG);
        return FAIL;
    }

    // adiciona o './'se for dir
    if (is_dir){
        mmd->name[0] = '.';
        mmd->name[1] = '/';
    }
    // caso o nome ja tenha '/'
    if (name[0] == '/')
        strncpy(&(mmd->name[1]), name, strlen(name));
    else
        strncpy(&(mmd->name[2 * is_dir]), name, strlen(name));

    mmd->size = name_sz + SIZE_OF_MMD;
    mmd->m_size = f_sz;
    mmd->pos = md->memb_sz;
    mmd->st_uid = st.st_uid;
    mmd->st_mode = st.st_mode;
    mmd->st_mtim = st.st_mtim.tv_sec;
    // Se no tiver nenhum outro membro
    if (md->memb_sz == 1)
        mmd->off_set = 0L;
    else{
        mmd_anterior = md->membs[md->memb_sz - 2];
        mmd->off_set = mmd_anterior->off_set + mmd_anterior->m_size;
    }

    if (fdumpf(archive, name, mmd) != mmd->m_size){
        PERRO("Nao foi possivel escrever os dados do arquivo [%s]", name);
        return FAIL;
    }

    return OK;
}

// retorna FAIL se acorreu algum ero
// OK caso contrario
int substitui_file(FILE *archive, size_t index, struct metad_t *md, char *file_sub){
    size_t f_sz;
    struct stat st;
    struct memb_md_t *mmd;

    if (archive == NULL || md == NULL)
        return FAIL;
    
    mmd = md->membs[index];
    if (stat(file_sub, &st) == -1){
        PERRO("[%s]: ", mmd->name);
        return FAIL;
    }

    f_sz = st.st_size;

    // se o tamnho for maior eh preciso aumentar o espaco do archive
    if (f_sz > mmd->m_size)
        ftruncate(fileno(archive), SIZE_OF_MEMBS(md) + f_sz);

    // se for o ultimo membro nao precisa mover nada
    if (index < md->memb_sz - 1)
        // move tudo depois do membro para a nova posicao
        if (!copy_data_window(archive,
                    mmd->off_set + mmd->m_size,
                    SIZE_OF_MEMBS(md) - (mmd->off_set + mmd->m_size),
                    mmd->off_set + f_sz))
        return FAIL;

    if (!substitui_membro(md, index, st)){
        ERRO("%s", "Inalcancavel");
        return FAIL;
    }

    if (fdumpf(archive, file_sub, mmd) != mmd->m_size){
        PERRO("Nao foi possivel escrever os dados do arquivo [%s]", mmd->name);
        return FAIL;
    }

    return OK;
}

// retorna FAIL se acorreu algum ero
// OK caso atualizacao foi feita sem erro
// NONE caso nao tenha sido feita a substituicao
int atualiza_file(FILE *archive, size_t index, struct metad_t *md, char *file_sub){
    struct stat st;
    struct memb_md_t *mmd;

    if (archive == NULL || md == NULL)
        return FAIL;
    
    mmd = md->membs[index];
    if (stat(mmd->name, &st) == -1){
        PERRO("[%s]: ", mmd->name);
        return FAIL;
    }

    // se o tempo for maior quer dizer que eh mais recente
    return st.st_mtim.tv_sec > mmd->st_mtim ?
        substitui_file(archive, index, md, file_sub) :
        NONE;
}

// cria od diretorios necessarios para criar o arquivo
static int extract_dir(char *path_name){
    char *slash, *path_cp = path_name;
    
    while ((slash = strchr(path_cp, '/')) != NULL){
        // coloca o \0 para que o mkdir nao tente criar tudo de uma vez
        *slash = '\0';
        if (mkdir(path_name, S_IRWXU) && errno != EEXIST){
            *slash = '/';
            PERRO("Nao foi possivel criar o diretorio [%s]", path_name);
            return FAIL;
        }
        // volta para o /, pois esta alterando o nome e tem voltar
        *slash = '/';
        // atualiza o path_cp, que eh de onde o strchr comeca a procurar pela /
        path_cp = slash + 1;
    }

    return OK;
}

// retorna FAIL em caso de erro
// OK caso contrario
int extract_file(FILE *archive, struct memb_md_t *mmd){
    char *buffer[MAX_SZ];
    FILE *f_out;
    size_t buf_times;

    if (archive == NULL || mmd == NULL)
        return FAIL;
    
    // cria os diretorios necessarios
    if (!extract_dir(mmd->name))
        return FAIL;

    f_out = fopen(mmd->name, "w");
    if (f_out == NULL)
        return FAIL;

    if(chmod(mmd->name, mmd->st_mode)){
        PERRO("Nao foi possivel alterar as permissoes do arquivo [%s]", mmd->name);
        return FAIL;
    }
    if (fseek(archive, mmd->off_set, SEEK_SET) == -1)
        return FAIL;

    buf_times = mmd->m_size / MAX_SZ;
    while(buf_times--){
        fread(buffer, BYTE_SZ, MAX_SZ, archive);
        fwrite(buffer, BYTE_SZ, MAX_SZ, f_out);
    }

    fread(buffer, BYTE_SZ, mmd->m_size % MAX_SZ, archive);
    fwrite(buffer, BYTE_SZ, mmd->m_size % MAX_SZ, f_out);

    fclose(f_out);
    return OK;
}

// retorna FAIL caso aconteca algum erro
// REMOVE_FILE caso tenha removido o ultimo membro
// OK casso contrario
int remove_file(FILE *archive, size_t index, struct metad_t *md){
    struct memb_md_t *mmd, *prox_mmd;
    
    if (archive == NULL || md == NULL)
        return FAIL;

    if (md->memb_sz == 1)
        return REMOVE_FILE;

    if (index >= md->memb_sz){
        ERRO("Nao existe membro com index %ld", index);
        return FAIL;
    }
    
    mmd = md->membs[index];
    // apenas mover se tiver conteudo para mover
    if (index < md->memb_sz - 1){
        prox_mmd = md->membs[index + 1];
        // pega tudo depois do membro para remover e coloca na sua posicao
        if (!copy_data_window(archive,
                         prox_mmd->off_set,
                         SIZE_OF_MEMBS(md) - prox_mmd->off_set,
                         mmd->off_set))
                return FAIL;
    }
    
    // remove o membro dos meta dados
    if (!remove_membro(md, index)){
        ERRO("%s", "Inalcancavel");
        return FAIL;
    }

    return OK;
}

// retorna FAIL se acorreu algum ero
// OK caso contrario
int move_file(FILE *archive, size_t index, size_t index_parent, struct metad_t *md){
    struct memb_md_t *mmd, *mmd_after;

    if (archive == NULL || md == NULL)
        return FAIL;

    if (md->memb_sz == 1)
        return FAIL;

    if (index >= md->memb_sz){
        ERRO("Nao existe membro com index %ld", index);
        return FAIL;
    }

    mmd = md->membs[index];
    // mmd_parent = md->membs[index_parent];
    // Aloca espaco no final para mover
    ftruncate(fileno(archive), SIZE_OF_MEMBS(md) + mmd->m_size);

    // copia para final do arquivo
    if (!copy_data_window(archive,
                     mmd->off_set,
                     mmd->m_size,
                     SIZE_OF_MEMBS(md)))
        return FAIL;

    // se for por depois do ultimo, apenas volta todo o espaco deixado
    if (index_parent == md->memb_sz - 1){
        if (!copy_data_window(archive,
                    mmd->off_set + mmd->m_size,
                    SIZE_OF_MEMBS(md) - mmd->off_set,
                    mmd->off_set))
            return FAIL;
    }else{
        mmd_after = md->membs[index_parent + 1];
        // se o membro vem depois do parent
        if (index > index_parent){
            if (!copy_data_window(archive,
                        mmd_after->off_set,
                        mmd->off_set - mmd_after->off_set,
                        mmd_after->off_set + mmd->m_size))
                return FAIL;
            // copia de volta o dado para a posicao
            if (!copy_data_window(archive,
                        SIZE_OF_MEMBS(md),
                        mmd->m_size,
                        mmd_after->off_set))
                return FAIL;
        }else{
            if (!copy_data_window(archive,
                        mmd->off_set + mmd->m_size,
                        mmd_after->off_set - (mmd->off_set + mmd->m_size),
                        mmd->off_set))
                return FAIL;
            // copia de volta o dado para a posicao
            if (!copy_data_window(archive,
                        SIZE_OF_MEMBS(md),
                        mmd->m_size,
                        mmd_after->off_set - mmd->m_size))
                return FAIL;
        }

    }

    // move o membro nos meta dados
    move_membro(md, index, index_parent);
    return OK;
}
