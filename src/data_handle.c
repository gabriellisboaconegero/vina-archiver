#include <stdlib.h>
#include "macros.h"
#include "data_handle.h"

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

    // caso ocorra de localizacao final ser dentro da data window
    if (start_point + window_size > where){
        // ajuste para facilitar a função de ler e escrever
        if (window_size >= MAX_SZ){
            off_to_write += window_size - MAX_SZ;
            off_to_read += window_size - MAX_SZ;
        }

        // atualizar valore para ir lendo do final
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
