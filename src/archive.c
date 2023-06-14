#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "macros.h"
#include "archive.h"

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
        exit(1);
    }
    if (fseek(archive, mmd->off_set, SEEK_SET) == -1)
        return 0;

    buf_times = mmd->m_size / MAX_SZ + 1;
    while(--buf_times){
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
        exit(1);
    }

    fseek(archive, mmd->off_set, SEEK_SET);
    buf_times = mmd->m_size / MAX_SZ + 1;
    while(--buf_times){
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
int remove_file(FILE *archive,
                char *name,
                struct metad_t *md)
{
    size_t archive_sz, off_to_write, off_to_read, buf_times, resto_sz, index;
    struct memb_md_t *mmd = NULL;
    char *buffer[MAX_SZ];
    
    if (archive == NULL || md == NULL || name == NULL)
        return NULL_PARAM;
    
    if (!busca_membro(name, md, &index))
        return NO_MEMBRO;

    if (md->memb_sz == 1){
        remove_membro(md, index);
        return REMOVE_FILE;
    }
    
    mmd = md->membs[index];
    fseek(archive, 0L, SEEK_END);
    archive_sz = ftell(archive);

    off_to_read = mmd->off_set + mmd->m_size;
    off_to_write = mmd->off_set;

    resto_sz = archive_sz - off_to_read;
    buf_times = resto_sz / MAX_SZ + 1;
    while(--buf_times){
        fseek(archive, off_to_read, SEEK_SET);
        fread(buffer, BYTE_SZ, MAX_SZ, archive);

        fseek(archive, off_to_write, SEEK_SET);
        fwrite(buffer, BYTE_SZ, MAX_SZ, archive);
        off_to_write += MAX_SZ;
        off_to_read += MAX_SZ;
    }
    fseek(archive, off_to_read, SEEK_SET);
    fread(buffer, BYTE_SZ, resto_sz % MAX_SZ, archive);

    fseek(archive, off_to_write, SEEK_SET);
    fwrite(buffer, BYTE_SZ, resto_sz % MAX_SZ, archive);

    ftruncate(fileno(archive), ftell(archive));

    if (!remove_membro(md, index)){
        ERRO("%s", "Inalcancavel");
        exit(1);
    }

    return REMOVE_OK;
}

// retorna 0 se nao for possivel inserir arquivo e nao teve erro fatal
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
        exit(1);
    }

    name_sz = strlen(name);
    mmd->name = calloc(name_sz + 1, sizeof(char));
    if (mmd->name == NULL){
        ERRO("%s", MEM_ERR_MSG);
        exit(1);
    }
    strncpy(mmd->name, name, name_sz);
    mmd->size = name_sz + SIZE_OF_MMD;
    mmd->m_size = f_sz;
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
        exit(1);
    }

    return 1;
}

int substitui_file(FILE *archive,
                   size_t index,
                   struct metad_t *md)
{
    struct memb_md_t *mmd;
    struct stat st;
    size_t f_sz;

    if (archive == NULL || md == NULL)
        return 0;
    
    mmd = md->membs[index];
    if (stat(mmd->name, &st) == -1){
        PERRO("[%s]: ", mmd->name);
        return 0;
    }

    f_sz = st.st_size;
    // se for o ultimo apenas deixa o off_set e escreve por cima dos meta dados
    if (index == md->memb_sz - 1){
        mmd->m_size = f_sz;
        if (fdumpf(archive, mmd->name, mmd) != mmd->m_size){
            PERRO("Nao foi possivel escrever os dados do arquivo [%s]", mmd->name);
            exit(1);
        }
    }else{
        printf("Nao implementado \n");
        //  remove depois insere no final
    }

    return 1;
}
