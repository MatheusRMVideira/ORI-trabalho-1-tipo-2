/*
Autores:
Matheus da Silva Vieira - 791085 BCC
Matheus Rezende Milani Videira - 790809 BCC
*/


/* O tamanho de cada campo sera representado por uma constante. */
# define TAM_LAST 40    //tamanho do campo last name
# define TAM_FIRST 25   //tamanho do campo first name
# define TAM_ADDRESS 40 //tamanho do campo address
# define TAM_CITY 16    //tamanho do campo city
# define TAM_STATE 32    //tamanho do campo state
# define TAM_ZIP 10     //tamanho do campo zip
# define TAM_PHONE 15 //tamanho do campo phone
# define TAM_KEY 6 //tamanho do campo chave
# define TAM_REG 1 + TAM_LAST+TAM_FIRST+TAM_ADDRESS+TAM_CITY+TAM_STATE+TAM_ZIP+TAM_PHONE+TAM_KEY //tamanho de um registro

// Bibliotecas
# include <stdio.h>  
# include <string.h> 
# include <stdlib.h> 

//FILE* database;

/*DEFINICAO DE UM REGISTRO DE TAMANHO FIXO COM CAMPOS DE TAMANHO FIXO */
         /* Registro que define os dados de uma pessoa */
typedef struct{
    int deleted; //mostra se um registro foi deletado ou não
    char key[TAM_KEY]; //Chave do registro
    char lastName[TAM_LAST]; //Sobrenome
    char firstName[TAM_FIRST]; //Nome
    char city[TAM_CITY]; //Cidade
    char address[TAM_ADDRESS]; //Endereco
    char state[TAM_STATE]; //Estado (sigla)
    char zip[TAM_ZIP]; //CEP
    char phone[TAM_PHONE]; //Telefone
}registro;

// Le valor do campo e salva no buffer
void obterCampo(char *buffer, int tamanho){
    char bufferAux[TAM_REG];
    char *espacoAdic;
    int i;
    
    // Le o valor do campo
    fflush(stdin);
    scanf("%[^\n]s", bufferAux);
    strcat(buffer, bufferAux);
    
    espacoAdic = (char *) malloc (tamanho - strlen(bufferAux) + 1);

    // Enche com * os espaços em branco que sobram
    for(i = 0; i < tamanho - strlen(bufferAux); i++){

        espacoAdic[i] = '*'; 
    }
    espacoAdic[i] = '\0';
    // Concatena o valor do campo com os asteriscos
    strcat(buffer, espacoAdic);
}


// Abre o arquivo dataBase2.bin com o modo de abertura especificado pelo usuário
FILE *getFile (char *modoAbertura){
    char *pathname;
    FILE * arquivo;
    //printf("Digite o nome do arquivo: ");
    //scanf("%s", pathname);
    pathname = "dataBase2.bin";

    arquivo = fopen(pathname, modoAbertura);

    // Se o arquivo nao existir solicita para que passe o caminho de algum arquivo que exista para abrir
    while(NULL == arquivo){
        printf("Impossivel abrir o arquivo especificado. Digite novamente.\n");
        printf("Digite o caminho do arquivo: ");
        scanf("%s", pathname);
        arquivo = fopen(pathname, modoAbertura);
    }

    return arquivo;
}

//Guarda valores no buffer
void getReg(char *buffer){
    strcat(buffer, "0");
    //obterCampo(buffer, sizeof(unsigned int));
    printf("Digite a chave: ");
    obterCampo(buffer, TAM_KEY);

    printf("Digite o ultimo nome: ");
    obterCampo(buffer, TAM_LAST);

    printf("Digite o primeiro nome: ");
    obterCampo(buffer, TAM_FIRST);

    printf("Digite o telefone: ");
    obterCampo(buffer, TAM_PHONE);

    printf("Digite o endereco: ");
    obterCampo(buffer, TAM_ADDRESS);

    printf("Digite a cidade: ");
    obterCampo(buffer, TAM_CITY);

    printf("Digite o estado: ");
    obterCampo(buffer, TAM_STATE);

    printf("Digite o CEP: ");
    obterCampo(buffer, TAM_ZIP);

    putchar('\n');

}
// Salva os valores do buffer no registrador
void writeReg(char *buffer, FILE *arquivo){
    char deleted;
    fseek(arquivo, 0, SEEK_SET);
    while(fread(&deleted, 1, 1, arquivo) == 1){
        fseek(arquivo, -1, SEEK_CUR);
        if(deleted == 1){
            //Se for um espaco livre, sai do laco para escrever nele
            break;
        }
        fseek(arquivo, TAM_REG, SEEK_CUR);
    //fwrite(delimitador, TAM_REG, 1, arquivo);
    }
    fwrite(buffer, TAM_REG, 1, arquivo);
    buffer[0] = '\0';
    //buffer[TAM_REG] = '#';
}

// Escreve os registros
void write(){
    FILE * arquivo; //arquivo onde serao escritos os registros
    int continua;
    char buffer[TAM_REG] = "\0";
    arquivo = getFile("ab");

    if(NULL == arquivo){
        return;
    }
    printf("1 - Escrever Registro \t 0 - Sair \n");
    scanf("%d", &continua);
    putchar('\n');
    while(continua == 1){
        // Salva valor do registrador no buffer
        getReg(buffer);
        // Escreve no registrador
        writeReg(buffer, arquivo);
        //fwrite(delimitador, sizeof(char), 1, arquivo);
        printf("1 - Escrever Registro\t 0 - Sair\n");
        scanf("%d", &continua);
        putchar('\n');
    }

    fclose(arquivo);
}

// Copia para string campo o campo do buffer
void readAuxReg(char *campo, char *buffer, int *inicioCampo, int tamanho){ 
    strncpy(campo, &buffer[*inicioCampo], tamanho);
    campo[tamanho - 1] = '\0';
    *inicioCampo += tamanho;
}

// Le Registrador
void readReg(registro *umRegistro, char *buffer){
    int inicioCampo = 1;

    // Copia para o umRegistro->key o campo correspondente do buffer
    umRegistro->deleted = buffer[0] - 48;
    readAuxReg(umRegistro->key, buffer, &inicioCampo, TAM_KEY);
    readAuxReg(umRegistro->lastName, buffer, &inicioCampo, TAM_LAST);
    readAuxReg(umRegistro->firstName, buffer, &inicioCampo, TAM_FIRST);
    readAuxReg(umRegistro->phone, buffer, &inicioCampo, TAM_PHONE);
    readAuxReg(umRegistro->address, buffer, &inicioCampo, TAM_ADDRESS);
    readAuxReg(umRegistro->city, buffer, &inicioCampo, TAM_CITY);
    readAuxReg(umRegistro->state, buffer, &inicioCampo, TAM_STATE);
    readAuxReg(umRegistro->zip, buffer, &inicioCampo, TAM_ZIP);

}

// Pega registros do arquivo
int recuperarReg (char *buffer, FILE *arquivo){
    return fread(buffer, TAM_REG, 1, arquivo);
}

// Imprime o Registro
void printReg(registro umRegistro){
    printf("Registro:\n");
    printf("\t%s %s\n", "Chave:", umRegistro.key);
    printf("\t%s %s\n", "Ultimo Nome:", umRegistro.lastName);
    printf("\t%s %s\n", "Primeiro Nome:", umRegistro.firstName);
    printf("\t%s %s\n", "Telefone:", umRegistro.phone);
    printf("\t%s %s\n", "Endereco:", umRegistro.address);
    printf("\t%s %s\n", "Cidade:", umRegistro.city);
    printf("\t%s %s\n", "Estado:", umRegistro.state);
    printf("\t%s %s\n", "CEP:", umRegistro.zip);
}


// Ler todos os registros 
void readAllReg(){
    registro umRegistro;

    FILE *arquivo;
    char buffer[TAM_REG];

    // Abre o arquivo
    arquivo = getFile("rb");

    // Pega um registro por vez
    while(recuperarReg(buffer, arquivo)){
        // Le e imprime um registro por vez
        fflush(stdin);
        readReg(&umRegistro, buffer);
        if(umRegistro.deleted == 0){
            printReg(umRegistro);
            getchar();
        }
    }

    putchar('\n');

    // Fecha o arquivo
    fclose(arquivo);
}

// Procura registro pelo numero
void searchReg(){
    char buffer[TAM_REG];
    int res;
    int i = 0;

    registro umRegistro;
    FILE *arquivo;
    
    // Abre arquivo
    arquivo = getFile("rb");

    // Pega num do registro desejado
    printf("\nDigite o numero do registro que deseja ver: ");
    scanf("%d",&res);

    // Passa por todos os registros até chegar no registro desejado
    while(recuperarReg(buffer, arquivo)){
        if(i == res){
            // Se for o registro desejado le e imprime o registro
            fflush(stdin);
            readReg(&umRegistro, buffer);
            if(umRegistro.deleted == 0){
                printReg(umRegistro);
            } else {
                printf("Registro no estado de apagado\n");
            }
            getchar();
        }
        i+= 1;
    }

    putchar('\n');
    // Fecha o arquivo
    fclose(arquivo);
    
}

// Procura registro pela chave
void searchKey(){ 
    char buffer[TAM_REG];
    int searchKey;
    registro umRegistro;
    FILE *arquivo;
    int key;
    
    // Abre o arquivo
    arquivo = getFile("rb");

    // Pega a chave desejada
    printf("\nDigite o numero da chave que deseja ver: ");
    scanf("%d", &searchKey);
    
    // Pega registros do arquivo
    while(recuperarReg(buffer, arquivo)){  
        fflush(stdin);
        // Le registro
        readReg(&umRegistro, buffer);
        // Transforma registro em inteiro
        key = atoi(umRegistro.key);
        // Se o registro for igual a chave desejada imprime o registro
        if(key == searchKey && umRegistro.deleted == 0){
            printReg(umRegistro);
            getchar();
        }
    }

    putchar('\n');
    // Fecha o arquivo
    fclose(arquivo);
    
}


// Procura registro pelo primeiro nome
void searchName(){ 
    char buffer[TAM_REG];
    char searchName[TAM_FIRST];
    registro umRegistro;
    FILE *arquivo;
    char regFirst[TAM_FIRST];

    // Abre o arquivo
    arquivo = getFile("rb");
    char *token;

    // Guarda o nome que deseja procurar nos registros
    printf("\nDigite o primeiro nome do registro que deseja ver: ");
    scanf("%s", &searchName);
    getchar();

    while(recuperarReg(buffer, arquivo)){
        int ret;
        char *cmp; 
        fflush(stdin);
        // Le registro
        readReg(&umRegistro, buffer);
        // Pega apenas a parte com dado do registro e ignora a parte vazia representada com *
        token = strtok(umRegistro.firstName, "*");
        
        // Se o nome desejado for igual ao nome do registro imprime o registro
        if(strcmp(searchName, token) == 0 && umRegistro.deleted == 0){
            printReg(umRegistro);
            getchar();
        }
        
    }
    
    putchar('\n');
    // Fecha o arquivo
    fclose(arquivo);
    
}

/*IMPLEMENTAÇÃO ERRADA DO DELETE */
void deleteRegister(){
    char buffer[TAM_REG];
    int deletedReg;
    int i = 0;

    registro umRegistro;
    FILE *arquivo;
    
    arquivo = getFile("rb");

    printf("\nDigite o numero do registro que deseja remover: ");
    scanf("%d",&deletedReg);

    while(recuperarReg(buffer, arquivo)){
        if(i == deletedReg){
            fflush(stdin);
            readReg(&umRegistro, buffer);
            //getchar();
            umRegistro.deleted = 1;

            fseek(arquivo, -TAM_REG, SEEK_CUR);
            
            printf("Registro removido com sucesso\n");
        }
        i+= 1;
    }


    putchar('\n');
    fclose(arquivo);
}

int main(){
    int opcao; //readAllReg, write ou pesquisar

    // Arquivo 
    fopen("dataBase2.bin", "wb+");

    // Menu
    printf("IMPLEMENTACAO USANDO REGISTROS DE TAMANHO FIXO COM CAMPOS DE TAMANHO FIXO \n\n");
    printf("1-Ler todos os registros\n2-Escrever registros\n3-Pesquisar registro pelo numero\n4-Pesquisar registro pela chave\n5-Pesquisar registro pelo nome\n6-Deletar registro\n");
    // Input
    printf("Digite a Opcao (7 para terminar): ");
    scanf("%d", &opcao);
    putchar('\n');

    // Opções
    while(opcao != 7){
        switch(opcao){
            case 1: readAllReg();
                break;
            case 2: write();
                break;
            case 3: searchReg();
                break;
            case 4: searchKey();
                break;
            case 5: searchName();
                break;
            case 6: deleteRegister();
                break;
            default: printf("Opcao invalida! Digite novamente.\n");
                break;
        }
        // Menu
        printf("1-Ler todos os registros\n2-Escrever registros\n3-Pesquisar registro pelo numero\n4-Pesquisar registro pela chave\n5-Pesquisar registro pelo nome\n6-Deletar registro\n");
        printf("Digite a Opcao (7 para terminar): ");
        // Input
        fflush(stdin);
        scanf("%d", &opcao);
        putchar('\n');
    }
    return 0;
}