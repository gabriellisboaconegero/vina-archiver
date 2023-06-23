#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "macros.h"
#include "archive.h"

static void val_plus(size_t *a, size_t b){
    *a += b;
}

static void val_minus(size_t *a, size_t b){
    *a -= b;
}

// retorna 0 em caso de não haver espaço para mover o conteudo ou algum erro
// 1 caso contrario
int copy_data_window(FILE *archive, size_t start_point, size_t window_size, size_t where){
    size_t archive_sz, off_to_read, off_to_write, resto_sz;
    size_t *final_read, *final_write;
    char buffer[MAX_SZ] = {0};
    void (*update_func)(size_t *, size_t);

    if (archive == NULL)
        return 0;

    fseek(archive, 0L, SEEK_END);
    archive_sz = ftell(archive);

    // copiar para o lado esquerdo
    // verifica se é valido os parametros passados
    if (start_point + window_size > archive_sz){
        ERRO("%s", "Start point invalido para copiar");
        return 0;
    }

    off_to_read = start_point;
    off_to_write = where;
    resto_sz = window_size;
    update_func = val_plus;

    // salvar a posição final de escrita e leitura, dado
    // que elas são diferente quandos tem que mover para a direita
    // ou esquerda
    final_read = &off_to_read;
    final_write = &off_to_write;

    // copiar o conteudo para uma area que tem o proprio conteudo
    // overlap
    if (start_point + window_size > where){
        // ajuste para facilitar a função de ler e escrever
        if (window_size >= MAX_SZ){
            off_to_write += window_size - MAX_SZ;
            off_to_read += window_size - MAX_SZ;
        }

        update_func = val_minus;
        final_read = &start_point;
        final_write = &where;
    }

    // Copiar os dados, usando a update_func para decidir como
    while (resto_sz >= MAX_SZ){
        fseek(archive, off_to_read, SEEK_SET);
        fread(buffer, BYTE_SZ, MAX_SZ, archive);

        fseek(archive, off_to_write, SEEK_SET);
        fwrite(buffer, BYTE_SZ, MAX_SZ, archive);

        update_func(&off_to_write, MAX_SZ);
        update_func(&off_to_read, MAX_SZ);
        resto_sz -= MAX_SZ;
    }
    fseek(archive, *final_read, SEEK_SET);
    fread(buffer, BYTE_SZ, resto_sz, archive);

    fseek(archive, *final_write, SEEK_SET);
    fwrite(buffer, BYTE_SZ, resto_sz, archive);

    return 1;
}

// retorna 0 em caso de erro
// 1 caso contrario
int extract_file(FILE *archive, struct memb_md_t *mmd){
    char *buffer[MAX_SZ];
    FILE *f_out;
    size_t buf_times;

    if (archive == NULL || mmd == NULL)
        return 0;

    f_out = fopen(mmd->name, "w");
    if (f_out == NULL){
        PERRO("[%s]", mmd->name);
        return 0;
    }
    if (fseek(archive, mmd->off_set, SEEK_SET) == -1)
        return 0;

    buf_times = mmd->m_size / MAX_SZ;
    while(buf_times--){
        fread(buffer, BYTE_SZ, MAX_SZ, archive);
        fwrite(buffer, BYTE_SZ, MAX_SZ, f_out);
    }

    fread(buffer, BYTE_SZ, mmd->m_size % MAX_SZ, archive);
    fwrite(buffer, BYTE_SZ, mmd->m_size % MAX_SZ, f_out);

    fclose(f_out);
    return 1;
}

// retorna quantos bytes foram escritos
// retorna 0 se algo deu errado
size_t fdumpf(FILE *archive, char *f_in_name, struct memb_md_t *mmd){
    FILE *f_in;
    void *buffer[MAX_SZ];
    size_t buf_times, bytes_write = 0;

    if (mmd == NULL)
        return 0;

    f_in = fopen(f_in_name, "r");
    if (f_in == NULL){
        PERRO("[%s]: ", f_in_name);
        return 0;
    }

    fseek(archive, mmd->off_set, SEEK_SET);
    buf_times = mmd->m_size / MAX_SZ;
    while(buf_times--){
        fread(buffer, BYTE_SZ, MAX_SZ, f_in);
        bytes_write += fwrite(buffer, BYTE_SZ, MAX_SZ, archive);
    }

    fread(buffer, BYTE_SZ, mmd->m_size % MAX_SZ, f_in);
    bytes_write += fwrite(buffer, BYTE_SZ, mmd->m_size % MAX_SZ, archive);
    fclose(f_in);

    return bytes_write;
}

// retorna os tipos de erros NUL_PARAM, NO_MEMBRO caso queira usar
// caso de tudo certo retorna REMOVE_OK
// TODO: pegar e shifitar apenas os dados, nao o diretorio tambem
// TODO: alguma forma de remover mais eficiente. Agora ta removendo file por file
// e quando remove ja shifta tudo
int remove_file(FILE *archive, size_t index, struct metad_t *md){
    struct memb_md_t *mmd, *prox_mmd;
    
    if (archive == NULL || md == NULL)
        return REMOVE_FAIL;

    if (md->memb_sz == 1)
        return REMOVE_FILE;

    if (index >= md->memb_sz){
        ERRO("Nao existe membro com index %ld", index);
        return REMOVE_FAIL;
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
                return REMOVE_FAIL;
    }
    
    // remove o membro dos meta dados
    if (!remove_membro(md, index)){
        ERRO("%s", "Inalcancavel");
        return REMOVE_FAIL;
    }

    return REMOVE_OK;
}

// retorna 0 em casso de erro
// 1 caso contrario
int insere_file(FILE *archive, char *name, struct metad_t *md){
    struct stat st;
    size_t f_sz, name_sz;
    struct memb_md_t *mmd, *mmd_anterior;

    if (archive == NULL || name == NULL || md == NULL)
        return 0;

    if (stat(name, &st) == -1){
        PERRO("[%s]: ", name);
        return 0;
    }

    f_sz = st.st_size;

    mmd = add_membro(md);
    if (mmd == NULL){
        ERRO("%s", MEM_ERR_MSG);
        ERRO("%s", "Nao foi possivel inserir os arquivos no archive");
        return 0;
    }

    name_sz = strlen(name);
    mmd->name = calloc(name_sz + 1, sizeof(char));
    if (mmd->name == NULL){
        ERRO("%s", MEM_ERR_MSG);
        return 0;
    }
    strncpy(mmd->name, name, name_sz);
    mmd->size = name_sz + SIZE_OF_MMD;
    mmd->m_size = f_sz;
    mmd->pos = md->memb_sz;
    mmd->st_uid = st.st_uid;
    mmd->st_mode = st.st_mode;
    mmd->st_mtim = st.st_mtime;
    // Se no tiver nenhum outro membro
    if (md->memb_sz == 1)
        mmd->off_set = 0L;
    else{
        mmd_anterior = md->membs[md->memb_sz - 2];
        mmd->off_set = mmd_anterior->off_set + mmd_anterior->m_size;
    }

    // TODO: inserir todos os novos arquivos no diretorio primeiro
    // para dps inserir o conteudo????
    if (fdumpf(archive, name, mmd) != mmd->m_size){
        PERRO("Nao foi possivel escrever os dados do arquivo [%s]", name);
        return 0;
    }

    return 1;
}

// retorna 0 se acorreu algum ero
// 1 caso contrario
int substitui_file(FILE *archive, size_t index, struct metad_t *md){
    size_t f_sz;
    struct stat st;
    struct memb_md_t *mmd;

    if (archive == NULL || md == NULL)
        return 0;
    
    mmd = md->membs[index];
    if (stat(mmd->name, &st) == -1){
        PERRO("[%s]: ", mmd->name);
        return 0;
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
        return 0;

    if (!substitui_membro(md, index, f_sz)){
        ERRO("%s", "Inalcancavel");
        return 0;
    }

    if (fdumpf(archive, mmd->name, mmd) != mmd->m_size){
        PERRO("Nao foi possivel escrever os dados do arquivo [%s]", mmd->name);
        return 0;
    }

    return 1;
}

// retorna 0 se acorreu algum ero
// 1 caso atualizacao foi feita sem erro
// 2 caso nao tenha sido feita
int atualiza_file(FILE *archive, size_t index, struct metad_t *md){
    struct stat st;
    struct memb_md_t *mmd;

    if (archive == NULL || md == NULL)
        return 0;
    
    mmd = md->membs[index];
    if (stat(mmd->name, &st) == -1){
        PERRO("[%s]: ", mmd->name);
        return 0;
    }

    // se o tempo for maior quer dizer que eh mais recente
    return st.st_mtime > mmd->st_mtim ? substitui_file(archive, index, md) : 2;
}

// retorna 0 se acorreu algum ero
// 1 caso contrario
int move_file(FILE *archive, size_t index, size_t index_parent, struct metad_t *md){
    size_t archive_sz, index_to_retrive;
    struct memb_md_t *mmd, *mmd_after, *mmd_parent, *mmd_to_retrive;

    if (archive == NULL || md == NULL)
        return 0;

    if (md->memb_sz == 1)
        return 0;

    if (index >= md->memb_sz){
        ERRO("Nao existe membro com index %ld", index);
        return 0;
    }

    mmd = md->membs[index];
    // mmd_parent = md->membs[index_parent];
    // Aloca espaco no final para mover
    ftruncate(fileno(archive), SIZE_OF_MEMBS(md) + mmd->m_size);
    // salva tamanho inicial para volar ao tamnho correto depois
    archive_sz = SIZE_OF_MEMBS(md) + mmd->m_size;

    // copia para final do arquivo
    if (!copy_data_window(archive,
                     mmd->off_set,
                     mmd->m_size,
                     SIZE_OF_MEMBS(md)))
        return 0;

    // se for por depois do ultimo, apenas volta todo o espaco deixado
    if (index_parent == md->memb_sz - 1){
        if (!copy_data_window(archive,
                    mmd->off_set + mmd->m_size,
                    SIZE_OF_MEMBS(md) - mmd->off_set,
                    mmd->off_set))
            return 0;
    }else{
        mmd_after = md->membs[index_parent + 1];
        // se o membro vem depois do parent
        if (index > index_parent){
            if (!copy_data_window(archive,
                        mmd_after->off_set,
                        mmd->off_set - mmd_after->off_set,
                        mmd_after->off_set + mmd->m_size))
                return 0;
        }else{
            if (!copy_data_window(archive,
                        mmd->off_set + mmd->m_size,
                        mmd_after->off_set - (mmd->off_set + mmd->m_size),
                        mmd->off_set));
            return 0;
        }

        // copia de volta o dado para a posicao
        if (!copy_data_window(archive,
                    SIZE_OF_MEMBS(md),
                    mmd->m_size,
                    mmd_after->off_set))
            return 0;
    }

    /*
    // se for mover para depois do ultimo elemento
    // nao precisa mover o conteudo para colocar
    if (index_parent < md->memb_sz - 1){
        mmd_after = md->membs[index_parent + 1];
        // abre espaco antes do mmd_after
        copy_data_window(archive,
                mmd_after->off_set,
                SIZE_OF_MEMBS(md) - mmd_after->off_set,
                mmd_after->off_set + mmd->m_size);
        // atualiza tabela de off_set, comeca pelo proximo depois de mmd_parent
        for (size_t i = index_parent + 1; i < md->memb_sz; i++)
            md->membs[i]->off_set += mmd->m_size;
    }

    // copia mmd para o espaco
    copy_data_window(archive,
            mmd->off_set,
            mmd->m_size,
            mmd_parent->off_set + mmd_parent->m_size);

    index_to_retrive = index + 1;
    // nao precisa voltar o conteudo se o bloco movido foi o
    // ultimo, pois eh so truncar para o tamanho original
    if (index_to_retrive < md->memb_sz){
        // volta o conteudo dos blks_depois 
        mmd_to_retrive = md->membs[index_to_retrive];
        copy_data_window(archive,
                mmd_to_retrive->off_set,
                archive_sz - mmd_to_retrive->off_set,
                mmd->off_set);
    }*/

    // move o membro nos meta dados
    move_membro(md, index, index_parent);
    return 1;
}
