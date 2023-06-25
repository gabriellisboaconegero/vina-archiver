Gabriel Lisboa Conegero
GRR: 20221255

                                     VINA++

# Arquivos fonte
    Os arquivos fonte estão no diretório src. São eles:archive.c, archive.h,
dir.c, dir.h, operacoes.c, operacoes.h, macros.h, main.c.  Outros arquivos
    enviados foram: Makefile, LEIAME.txt.

## Organização do diretório
    Referenciado também como vetor de membros ou metadados, ele foi organizado
colocando-o no final do archive, assim tendo mais liberdade para sobrescrever
os dados lá uma vez que já foi todo carregado em memória no início das
operações. Os últimos quatro bytes do archive indicam onde o diretor começa, e
os primeiros quatro bytes do diretório informam quantos membros ele tem. O
offset do diretório foi colocado no final pois fica mais fácil de manipular e
visualizar o conteúdo dos membros se eles começam no zero do archive.

    Os únicos momentos em que a função ftruncate é utilizada é para abrir espaço
para a movimentação ou substituição de membros. Após a operação ser feita, não
se trunca o archive para o tamanho correto ainda, pois como no final das
operações o diretor será escrito imediatamente após o último membro, então a
truncagem é feita após a inserção do diretório.

## O macros.h
    Nessa biblioteca estão os macros principais utilizados em todo o projeto,
como macros de erro, tamanho máximo de buffer e mensagem de uso.
	
## Os módulos
    O projeto foi organizado em volta de alguns módulos principais. Divididos em
manipulação do archive, manipulação e gestão dos metadados e pré processamento
dos dados passados.

### Módulo data_handle
	Esse módulo é o que manipula os dados dentro do archive.

* copy_data_window:
    Essa função é a responsável por fazer toda a movimentação de dados
internos do archive necessária pelas operações de mover, remover,
substituir, etc. Utiliza apenas um buffer de MAX_SZ (definido em macros.h).
Ela recebe três parâmetros: o local de início dos dados, o tamanho dos
dados e o local final dos dados. A função não é responsável por alocar
espaço para mover os dados caso necessário, ela assume que existe.

* fdumpf
    Essa função é a responsável por pegar o conteúdo de um arquivo e escrever no
archive, usando buffers de no máximo MAX_SZ (defino em macros.h). O local em
que vai ser escrito esse conteúdo depende do parâmetro mmd. pode sobrescrever
dado no archive.

### Módulo archive
    Esse módulo possui as funções que fazem as operações em apenas um membro
dentro do archive. Ajustam os parâmetros adequados para a manipulação de
dados, alocando espaço no arquivo se preciso e definindo da onde ler,
extrair, escrever, mover, etc. Não atualizam o vetor de membros, apenas
dizem como manipular os dados e então usa as funções  do módulo dir.

### Módulo dir
    Esse módulo é o responsável por manipular, criar e excluir os membros e o
vetor de membros, atualizando-os de acordo com as operações feitas. Nele
está declarada a estrutura de dados utilizada. Ele também lida com a
manipulação do espaço de metadados no archive.  Um membro é uma estrutura
com os campos especificados no enunciado do trabalho e com uma propriedade
a mais, o size (tamanho do metadado de membro).  Já o vetor de membros é
    uma array de ponteiros para membro. Foi escolhido o
vetor pela praticidade de acesso a um membro e a membros perto dele, e também
pela facilidade de criar o vetor uma vez que a quantidade máxima de membros não
muda durante a execução do programa.  As funções principais são as de pegar os
metadados do archive e carregar no vetor de membros, escrever os novos
metadados no archive, listar os membros e atualizar o vetor de acordo com a
operação. Além de gerenciar a alocação de memória dos metadados.
    O módulo também tem alguns macros úteis como SIZE_OF_MEMBS que retorna qual
o tamanho utilizados apenas pelo conteúdo dos membros dentro do archive.

### Módulo operações
    Esse módulo contém as funções que são chamadas na utilização de cada tipo de
operação pelo programa. As funções dele são responsáveis por abrir o archive,
inicializar o vetor de membros usando o módulo dir. Verificar a existências dos
arquivos utilizados e aplicar a operação para todos os arquivos passados como
parâmetro se necessário.  Todas as funções abrem o archive e inicializam o
vetor com os membros guardados. E no final liberam sua memória, também fechando
o archive.
    Todas, com exceção da op_list, chama dump_meta para escrever no final do
archive os metadados atualizados.

## Dificuldades do desenvolvimento
    A parte mais complexa do trabalho foi a manipulação de ponteiros e criar uma
função generalizada de movimentação de dados (copy_data_window). Após a criação
dela, a dificuldade foi manipular ponteiros para as operações de mover, remover
e substituir. As ferramentas de debug utilizadas foram gdb e xxd, sem elas o
processo seria muito mais difícil visto que elas permitem a visualização de
cada etapa.

## Bugs e comportamentos
    BUG: Caso o archive passado não esteja no formato correto, podem haver erros
imprevistos.
    COMPORTAMENTO: O programa em vez de parar quando encontrar algum arquivo que
não pode ser removido, substituído, extraído ou inserido, continua executando e
tentando operar nos próximos arquivos.

