#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "macros.h"
#include "archive.h"

// TODO: mesma historia, verificar se precisa checar fread e fwrite
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
        fread(buffer, 1, MAX_SZ, archive);
        fwrite(buffer, 1, MAX_SZ, f_out);
    }

    fread(buffer, 1, mmd->m_size % MAX_SZ, archive);
    fwrite(buffer, 1, mmd->m_size % MAX_SZ, f_out);

    fclose(f_out);
    return 1;
}

// retorna quantos bytes foram escritos
// retorna 0 se alog deu errado
size_t fdumpf(FILE *archive, char *f_in_name, struct memb_md_t *mmd){
    FILE *f_in = fopen(f_in_name, "r");
    void *buffer[MAX_SZ];
    size_t buf_times, bytes_write = 0;
    if (f_in == NULL){
        PERRO("[%s]: ", f_in_name);
        exit(1);
    }
    if (mmd == NULL)
        return 0;

    fseek(archive, mmd->off_set, SEEK_SET);
    buf_times = mmd->m_size / MAX_SZ + 1;
    while(--buf_times){
        fread(buffer, 1, MAX_SZ, f_in);
        bytes_write += fwrite(buffer, 1, MAX_SZ, archive);
    }

    fread(buffer, 1, mmd->m_size % MAX_SZ, f_in);
    bytes_write += fwrite(buffer, 1, mmd->m_size % MAX_SZ, archive);
    fclose(f_in);

    return bytes_write;
}

// retorna 0 se nao for possivel retirar o arquivo e nao tiver erro critico
// 1 caso contrario
// TODO: pegar e shifitar apenas os dados, nao o diretorio tambem
// TODO: alguma forma de remover mais eficiente. Agora ta removendo file por file
// e quando remove ja shifta tudo
int remove_file(FILE *archive, struct memb_md_t *mmd){
    size_t archive_sz, off_to_write, off_to_read, buf_times, resto_sz;
    char *buffer[MAX_SZ];
    
    if (archive == NULL || mmd == NULL)
        return 0;

    fseek(archive, 0L, SEEK_END);
    archive_sz = ftell(archive);

    off_to_read = mmd->off_set + mmd->m_size;
    off_to_write = mmd->off_set;

    resto_sz = archive_sz - off_to_read;
    buf_times = resto_sz / MAX_SZ + 1;
    while(--buf_times){
        fseek(archive, off_to_read, SEEK_SET);
        fread(buffer, sizeof(char), MAX_SZ, archive);

        fseek(archive, off_to_write, SEEK_SET);
        fwrite(buffer, sizeof(char), MAX_SZ, archive);
        off_to_write += MAX_SZ;
        off_to_read += MAX_SZ;
    }
    fseek(archive, off_to_read, SEEK_SET);
    fread(buffer, sizeof(char), resto_sz % MAX_SZ, archive);

    fseek(archive, off_to_write, SEEK_SET);
    fwrite(buffer, sizeof(char), resto_sz % MAX_SZ, archive);

    ftruncate(fileno(archive), ftell(archive));
    return 1;
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

    // Debug
    mmd = cria_membro();
    if (mmd == NULL){
        ERRO("%s", MEM_ERR_MSG);
        ERRO("%s", "Nao foi possivel inserir os arquivos no archive");
        exit(1);
    }

    // Pega o anterior caso precise do off_set dele
    mmd_anterior = md->tail;
    // Insere novo membro no final
    if (md->head == NULL){
        md->head = mmd;
        md->tail = mmd;
    }else{
        md->tail->prox = mmd;
        md->tail = mmd;
    }
    md->memb_sz++;

    name_sz = strlen(name);
    // TODO: nome alocado no argv[0] ou em outro lugar
    // verificar se pode fazer assim
    mmd->name = name;
    mmd->size = name_sz + SIZE_OF_MMD;
    mmd->m_size = f_sz;
    if (mmd_anterior == NULL)
        mmd->off_set = 0L;
    else
        mmd->off_set = mmd_anterior->off_set + mmd_anterior->m_size;

    if (fdumpf(archive, name, mmd) != mmd->m_size){
        PERRO("Nao foi possivel escrever os dados do arquivo [%s]", name);
        exit(1);
    }

    return 1;
}

