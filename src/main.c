#include <stdio.h>
#include <unistd.h>
#include "operacoes.h"
#include "macros.h"

int main(int argc, char **argv){
    int option, o_i, o_a, o_m, o_x, o_r, o_c;
    int optindex, members_sz;
    char *target = NULL, *archive_name = NULL, **members_names;
    char last_set = ' ';
    o_i = o_a = o_m = o_x = o_r = o_c = 0;
    
    while ((option = getopt(argc, argv, "iaxrchm:")) != -1){
        switch (option){
            case 'i':
                if (last_set != ' '){
                    ERRO("Opcoes -i e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_i = 1;
                last_set = 'i';
                break;
            case 'a':
                if (last_set != ' '){
                    ERRO("Opcoes -a e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_a = 1;
                last_set = 'a';
                break;
            case 'm':
                target = optarg;
                if (last_set != ' '){
                    ERRO("Opcoes -m e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_m = 1;
                last_set = 'm';
                break;
            case 'x':
                if (last_set != ' '){
                    ERRO("Opcoes -x e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_x = 1;
                last_set = 'x';
                break;
            case 'r':
                if (last_set != ' '){
                    ERRO("Opcoes -r e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_r = 1;
                last_set = 'r';
                break;
            case 'c':
                if (last_set != ' '){
                    ERRO("Opcoes -c e -%c fora passadas juntas", last_set);
                    USAGE(stderr);
                    return 1;
                }
                o_c = 1;
                last_set = 'c';
                break;
            case 'h':
                USAGE(stdout);
                return 0;
            default:
                USAGE(stderr);
                return 1;
        }
    }

    optindex = optind;
    if (argc <= optindex){
        ERRO("%s", "Faltando o parametro <archive>");
        USAGE(stderr);
        return 1;
    }

    archive_name = argv[optindex++];
    members_sz = argc - optindex;
    members_names = &argv[optindex];

    if (o_c)
        return op_list(archive_name);
    if (o_x)
        return op_extrair(archive_name, members_names, members_sz);

    if (members_sz <= 0){
        ERRO("Esta faltando a lista de membros para realizar a operacao -%c", last_set);
        USAGE(stderr);
        return 1;
    }

    if (o_i)
        return op_inserir(archive_name, members_names, members_sz, 0);
    if (o_a){
        return op_inserir(archive_name, members_names, members_sz, 1);
    }
    if (o_m){
        return op_move(archive_name, members_names[0], target);
    }
    if (o_r)
        return op_remove(archive_name, members_names, members_sz);

    USAGE(stdout);
    return 0;
}
