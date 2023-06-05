#ifndef ARCHIVE_H_
#define ARCHIVE_H_
#include <stdio.h>
#include "dir.h"

int extract_file(FILE *archive, struct memb_md_t *mmd);

size_t fdumpf(FILE *archive, char *f_in_name, struct memb_md_t *mmd);

int remove_file(FILE *archive, struct memb_md_t *mmd);

int insere_file(FILE *archive, char *name, struct metad_t *md);

#endif
