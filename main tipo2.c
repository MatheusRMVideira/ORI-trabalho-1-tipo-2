#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct files {
    FILE* dataBase; //Arquivo principal onde os registros sao armazenados
    FILE* primaryIndex; //Indice primario, contem se o registro eh valido, chave, offset do registro e RRN para proxima entrada no indice
    FILE* secundaryIndex; //Indice secundario, contem nome, RRN para indice primario
};

struct registro {
    unsigned int key; //Chave do registro
    char lastName[256]; //Sobrenome
    char firstName[256]; //Nome
    char city[256]; //Cidade
    char address[256]; //Endereco
    char state[2]; //Estado (sigla)
    unsigned long long int zip; //CEP
    unsigned long long int phone; //Telefone
};

//Sanitiza a string passada como argumento
//Remove caracteres comuns de serem enviados em conjunto com o numero de telefone e CEP
//Retorna o unsigned long long int contido na string input
unsigned long long int sanitize_input_integer(char* input){
    int i,j;
    unsigned long long int returnInt;
    const char strSan[] = "|#()- "; //Caracteres a ser removido
    i = 0;
    while(i<strlen(input)){ //itera pela string
        if (strchr(strSan, input[i]) != NULL){ //Se o caractere i da string for invalido
            for (j=i; j<strlen(input); j++) //Itera pela string, movendo todos os caracteres apos i, uma posicao para a frente 
                input[j]=input[j+1];
        }
        else i++;
    }
    returnInt = strtoull(input, NULL, 10); //Converte string para unsigned long long int
    return returnInt;
}

//Funcao basica para abrir os arquivos
//Retorna: 1 - sucesso e 0 - erro
int open_files(struct files *arquivos){ 
    arquivos->dataBase = fopen("dataBase.bin", "wb+");
    arquivos->primaryIndex = fopen("primaryIndex.bin", "wb+");
    arquivos->secundaryIndex = fopen("secundaryIndex.bin", "wb+");
    return 1;
}

//Salva um registro nos arquivos
//Atualiza o indice primario e secundario
int save_to_file(struct files *arquivos, struct registro reg){
    short int validReg = 1; //O registro a ser inserido eh valido
    unsigned int regSize, keySize, lastNameSize, firstNameSize, addressSize, citySize, stateSize, zipSize, phoneSize;
    keySize = sizeof(unsigned int); //Tamanho da key em bytes
    lastNameSize = strlen(reg.lastName) + 1; //Tamanho do sobrenome em bytes
    firstNameSize = strlen(reg.firstName) + 1; //Tamanho do nome em bytes
    addressSize = strlen(reg.address) + 1; //Tamanho do endereco em bytes
    citySize = strlen(reg.city) + 1; //Tamanho da cidade em bytes
    zipSize = sizeof(unsigned long long int); //Tamanho do CEP em bytes
    phoneSize = sizeof(unsigned long long int); //Tamanho do telefone em bytes
    stateSize = 2; //Tamanho do Estado em bytes

    //Calcula o tamanho total do registro
    regSize = 9*sizeof(unsigned int) + lastNameSize + firstNameSize + 
              addressSize + citySize + 2*sizeof(char)+ 2*sizeof(unsigned long long int);

    //Escreve no indice primário
    long int dataBasePos = ftell(arquivos->dataBase); //Posicao do registro no arquivo principal
    long int primaryIndexPos = ftell(arquivos->primaryIndex); //Posicao da entrada no indice primario
    int nextRRN = -1; //Proxima entrada no indice primario (lista ligada)
    fwrite(&validReg, sizeof(validReg), 1, arquivos->primaryIndex); //Escreve que a entrada no indice primario eh valida
    fwrite(&reg.key, sizeof(reg.key), 1, arquivos->primaryIndex); //Escreve a chave no indice primario
    fwrite(&dataBasePos, sizeof(long int), 1, arquivos->primaryIndex); //Escreve o offset correspondente ao registro no indice primario

    //Escreve no indice secundario
    unsigned int tempSI; //Temporario tamanho do nome
    char tempBuffer[256]; //Temporario armazena o nome
    int foundName = 0; //Se achou o nome no indice secundario ou nao
    rewind(arquivos->secundaryIndex); //Volta ao inicio do arquivo de indice secundario
    //Enquanto nao acabar o arquivo ou achar o nome
    while(1){
        //Detecta se chegou ao final do arquivo de indice secundario, se sim, sai do loop
        if(fgetc(arquivos->secundaryIndex) == EOF)
            break;
        fseek(arquivos->secundaryIndex, -1, SEEK_CUR);
        fread(&tempSI, sizeof(tempSI), 1, arquivos->secundaryIndex); //Le o tamanho do nome
        if(tempSI == firstNameSize){ //Se o tamanho do nome no indice for o mesmo que o do que esta a ser escrito
            fread(&tempBuffer, 1, tempSI, arquivos->secundaryIndex); //Le o nome
            if(strcmp(tempBuffer, reg.firstName) == 0){ //Se o nome for igual
                //Atualiza o RRN a ser escrito no indice primario para corresponder com a outra entrada no indice, criando a lista ligada
                fread(&nextRRN, sizeof(nextRRN), 1, arquivos->secundaryIndex);
                fseek(arquivos->secundaryIndex, -1*(long int)sizeof(nextRRN), SEEK_CUR); //Volta a posicao para escrever novo RRN
                //Escreve novo RRN correspondente a nova entrada no registro, que aponta para a entrada antiga
                fwrite(&primaryIndexPos, sizeof(primaryIndexPos), 1, arquivos->secundaryIndex);
                //Vai para o final do arquivo
                fseek(arquivos->secundaryIndex, 0, SEEK_END);
                foundName = 1;
                break;
            }
        }
            //Caso nao achou o nome, avanca para proxima entrada
            fseek(arquivos->secundaryIndex, tempSI + sizeof(nextRRN), SEEK_CUR);
    }
    //Se chegou ao fim do indice e nao achou o nome, cria nova entrada no indice
    if(foundName == 0){
        fwrite(&firstNameSize, sizeof(firstNameSize), 1, arquivos->secundaryIndex);
        fwrite(&reg.firstName, 1, firstNameSize, arquivos->secundaryIndex);
        fwrite(&primaryIndexPos, sizeof(primaryIndexPos), 1, arquivos->secundaryIndex);
    }

    //Escreve o RRN para a proxima entrada no indice primario no indice primario
    fwrite(&nextRRN, sizeof(nextRRN), 1, arquivos->primaryIndex);

    //Escreve o registro no arquivo principal
    fwrite(&validReg, sizeof(validReg), 1, arquivos->dataBase); //Escreve que eh valido
    fwrite(&regSize, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve o tamanho do registro
    fwrite(&keySize, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve o tamanho da chave
    fwrite(&reg.key, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve a chave

    fwrite(&lastNameSize, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve o tamanho do sobrenome
    fwrite(&reg.lastName, 1, lastNameSize, arquivos->dataBase); //Escreve o sobrenome

    fwrite(&firstNameSize, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve o tamanho do nome
    fwrite(&reg.firstName, 1, firstNameSize, arquivos->dataBase); //Escreve o nome

    fwrite(&citySize, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve o tamanho da cidade
    fwrite(&reg.city, 1, citySize , arquivos->dataBase); //Escreve a cidade

    fwrite(&addressSize, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve o tamanho do endereco
    fwrite(&reg.address, 1, addressSize, arquivos->dataBase); //Escreve o endereco

    fwrite(&stateSize, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve o tamanho do estado
    fwrite(&reg.state, 1, stateSize, arquivos->dataBase); //Escreve o estado

    fwrite(&zipSize, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve o tamanho do CEP
    fwrite(&reg.zip, sizeof(reg.zip), 1, arquivos->dataBase); //Escreve o CEP

    fwrite(&phoneSize, sizeof(unsigned int), 1, arquivos->dataBase); //Escreve o tamanho do telefone
    fwrite(&reg.phone, sizeof(reg.phone), 1, arquivos->dataBase); //Escreve o telefone

    return 1;
}

//Captura a entrada de dados do usuario e salva em um registro
//Retorna: 1 - caso sucesso e 0 - caso falha
int stdin_to_reg(struct registro* reg){
    char strInput[256];
    char temp;
    scanf("%c",&temp); //Limpa possiveis '\n' presentes na stdin
    printf("Inserindo um registro\n");
    //Captura, sanitiza e salva a chave
    printf("Chave: ");
    if(scanf("%[^\n]", strInput) < 1)
        return 0;
    scanf("%c",&temp);
    reg->key= sanitize_input_integer(strInput);

    //Captura e salva o sobrenome
    printf("Sobrenome: ");
    if(scanf("%[^\n]", strInput) < 1)
        return 0;
    scanf("%c",&temp);
    strcpy(reg->lastName, strInput);

    //Captura e salva o nome
    printf("Nome: ");
    if(scanf("%[^\n]", strInput) < 1)
        return 0;
    scanf("%c",&temp);
    strcpy(reg->firstName, strInput);

    //Captura e salva o endereco
    printf("Endereco: ");
    if(scanf("%[^\n]", strInput) < 1)
        return 0;
    scanf("%c",&temp);
    strcpy(reg->address, strInput);

    //Captura e salva a cidade
    printf("Cidade: ");
    if(scanf("%[^\n]", strInput) < 1)
        return 0;
    scanf("%c",&temp);
    strcpy(reg->city, strInput);

    //Captura e salva o estado
    printf("Estado (sigla): ");
    if(scanf("%[^\n]", strInput) < 1)
        return 0;
    scanf("%c",&temp);
    strcpy(reg->state, strInput);

    //Captura, sanitiza e salva o CEP
    printf("Cep: ");
    if(scanf("%[^\n]", strInput) < 1)
        return 0;
    scanf("%c",&temp);
    reg->zip= sanitize_input_integer(strInput);

    //Captura, sanitiza e salva o telefone
    printf("Telefone: ");
    if(scanf("%[^\n]", strInput) < 1)
        return 0;
    reg->phone = sanitize_input_integer(strInput);

    return 1;
}

//Funcao para unir stdin_to_reg e save_to_file
//Torna possivel capturar e gravar multiplos registros sequencialmente
void save_multiple_reg(struct files *arquivos, struct registro *reg){
    while(1){
        printf("Aperte [ENTER] sem digitar um dos campos para sair\n");
        if(stdin_to_reg(reg) == 0)
            return;
        save_to_file(arquivos, *reg);
    }
}

//Le um registro do arquivo principal e salva em reg
//Retorna: 1 - caso sucesso, 0 - caso falha, -1 - caso tenha chego ao final do arquivo
int read_register(struct files *arquivos, struct registro *reg){
    short int validReg;
    unsigned int regSize, keySize, lastNameSize, firstNameSize;
    unsigned int addressSize, citySize, stateSize, zipSize, phoneSize;
    regSize = 0;

    //Le se o registro eh valido e o tamanho dele
    fread(&validReg, sizeof(validReg), 1, arquivos->dataBase);
    fread(&regSize, sizeof(unsigned int), 1, arquivos->dataBase);

    //Caso nao seja valido, verifica se eh invalido ou lixo (final do arquivo)
    if(validReg != 1){
        if(validReg == 0){ //Caso seja invalido
            if(fseek(arquivos->dataBase, regSize, SEEK_CUR) != 0){ //Ignora o registro e move para o proximo
                return -1; //Caso falha no fseek
            } else {
                return 0; //Moveu para o proximo
            }
        }
        return -1; //Caso seja lixo, retorna que chegou ao final do arquivo
    }

    fread(&keySize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho da chave
    fread(&reg->key, keySize, 1, arquivos->dataBase); //Le a chave
    regSize -= sizeof(keySize) + keySize; //Reduz o tamanho da chave do tamanho total do registro

    fread(&lastNameSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do sobrenome
    fread(&reg->lastName, 1, lastNameSize, arquivos->dataBase); //Le o sobrenome
    regSize -= sizeof(lastNameSize) + lastNameSize; //Reduz o tamanho do sobrenome do tamanho total do registro

    fread(&firstNameSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do nome
    fread(&reg->firstName, 1, firstNameSize, arquivos->dataBase); //Le o nome
    regSize -= sizeof(firstNameSize) + firstNameSize; //Reduz o tamanho do nome do tamanho total do registro

    fread(&citySize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho da cidade
    fread(&reg->city, 1, citySize, arquivos->dataBase); //Le a cidade
    regSize -= sizeof(citySize) + citySize; //Reduz o tamanho da cidade do tamanho total do registro

    fread(&addressSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do endereco
    fread(&reg->address, 1, addressSize, arquivos->dataBase); //Le o endereco
    regSize -= sizeof(addressSize) + addressSize; //Reduz o tamanho do endereco do tamanho total do registro

    fread(&stateSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do estado
    fread(&reg->state, 1, stateSize, arquivos->dataBase); //Le o estado
    regSize -= sizeof(stateSize) + stateSize; //Reduz o tamanho do estado do tamanho total do registro

    fread(&zipSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do CEP
    fread(&reg->zip, zipSize, 1, arquivos->dataBase); //Le o CEP
    regSize -= sizeof(zipSize) + zipSize; //Reduz o tamanho do CEP do tamanho total do registro

    fread(&phoneSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do telefone
    fread(&reg->phone, 1, phoneSize, arquivos->dataBase); //Le o telefone
    regSize -= sizeof(phoneSize) + phoneSize; //Reduz o tamanho do telefone do tamanho total do registro

    //Caso regSize != 0, houve um erro na leitura ou registro esta corrompido
    if(regSize != 0){
        printf("ERRO!");
        return 0;
    }

    return 1;

}

//Imprime na stdout o reg
//Retorna: 1 - caso sucesso, 0 - caso falha
int print_register(struct registro *reg){
    if(reg == NULL){
        return 0;
    }
    printf("Pessoa %u:\n", reg->key);
    printf("Sobrenome: %s\n", reg->lastName);
    printf("Nome: %s \n", reg->firstName);
    printf("Endereco: %s \n", reg->address);
    printf("Cidade: %s\n", reg->city);
    printf("Estado: %c%c\n", reg->state[0], reg->state[1]);
    printf("CEP: %llu\n", reg->zip);
    printf("Telefone: %llu\n\n", reg->phone);
    return 1;
}

//Funcao para unir read_register e print_all_register
//Torna possivel imprimir todos os registros validos presentes no arquivo principal
void print_all_registers(struct files *arquivos, struct registro *reg){
    int ret = 0;
    rewind(arquivos->dataBase); //Volta a posicao no arquivo principal para o inicio
    while(ret != -1){ //Enquanto nao chegar ao final do arquivo
        ret = read_register(arquivos, reg); //Le o registro e salva em reg
        if(ret == 1){
            print_register(reg); //Imprime reg
        }
    }
}

//Realiza a busca sequencial no arquivo principal
//Salva o registro correspondente a chave searchKey em reg
//Retorna: 1 - caso sucesso, 0 - caso falha
int search_by_key(struct files *arquivos, struct registro *reg, unsigned int searchKey){
    short int validReg;
    unsigned int regSize, regKey, keySize, lastNameSize, firstNameSize;
    unsigned int addressSize, citySize, stateSize, zipSize, phoneSize;
    regSize = 0;

    rewind(arquivos->dataBase); //Volta a posicao no arquivo principal para o inicio
    
    while(1){ //Busca pelo arquivo todo

        fread(&validReg, sizeof(validReg), 1, arquivos->dataBase); //Le se o registro eh valido
        fread(&regSize, sizeof(regSize), 1, arquivos->dataBase); //Le o tamanho do registro

        if(validReg != 1){ //Caso nao seja valido
            if(validReg == 0){ //Caso seja invalido
                if(fseek(arquivos->dataBase, regSize, SEEK_CUR) != 0){ //Avanca para o proximo registro
                    return 0; //Caso falha no fseek, retorna falha
                } else {
                    continue; //Pula para a proxima iteracao do loop
                }
            }
            return 0; //Caso seja != 1 e != 0, retorna falha (lixo)
        }

        fread(&keySize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho da chav
        fread(&regKey, keySize, 1, arquivos->dataBase); //Le a chave
        regSize -= sizeof(keySize) + keySize; //Reduz o tamanho da chave do tamanho total do arquivo

        if(regKey != searchKey){ //Caso a chave nao seja a que procura
            if(fseek(arquivos->dataBase, regSize, SEEK_CUR) != 0){ //Avanca para o proximo registro
                return 0; //Caso falha no fseek, retorna falha
            } else {
                continue; //Pula para a proxima iteracao do loop
            }
        }

        //Encontramos o registro desejado

        reg->key = regKey; //Salva a chave do registro

        fread(&lastNameSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do sobrenome
        fread(&reg->lastName, 1, lastNameSize, arquivos->dataBase); //Le o sobrenome
        regSize -= sizeof(lastNameSize) + lastNameSize; //Reduz o tamanho do sobrenome do tamanho total do registro

        fread(&firstNameSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do nome
        fread(&reg->firstName, 1, firstNameSize, arquivos->dataBase); //Le o nome
        regSize -= sizeof(firstNameSize) + firstNameSize; //Reduz o tamanho do nome do tamanho total do registro

        fread(&citySize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho da cidade
        fread(&reg->city, 1, citySize, arquivos->dataBase); //Le a cidade
        regSize -= sizeof(citySize) + citySize; //Reduz o tamanho da cidade do tamanho total do registro

        fread(&addressSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do endereco
        fread(&reg->address, 1, addressSize, arquivos->dataBase); //Le o endereco
        regSize -= sizeof(addressSize) + addressSize; //Reduz o tamanho do endereco do tamanho total do registro

        fread(&stateSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do estado
        fread(&reg->state, 1, stateSize, arquivos->dataBase); //Le o estado
        regSize -= sizeof(stateSize) + stateSize; //Reduz o tamanho do estado do tamanho total do registro

        fread(&zipSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do CEP
        fread(&reg->zip, zipSize, 1, arquivos->dataBase); //Le o CEP
        regSize -= sizeof(zipSize) + zipSize; //Reduz o tamanho do CEP do tamanho total do registro

        fread(&phoneSize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho do telefone
        fread(&reg->phone, 1, phoneSize, arquivos->dataBase); //Le o telefone
        regSize -= sizeof(phoneSize) + phoneSize; //Reduz o tamanho do telefone do tamanho total do registro

        //Caso regSize != 0, houve um erro na leitura ou registro esta corrompido
        if(regSize != 0){
            printf("ERRO!");
            return 0;
        }
        return 1;
    }    
}

int main(){
    struct registro reg;
    struct files arquivos;
    int escolha;
    unsigned int temp;
    open_files(&arquivos);
    while(1){
        //Imprime menu principal
        printf("Digite o numero para escolher uma das opcoes abaixo:\n");
        printf("1 - Insercao de varios registros\n");
        printf("2 - Inserir um registro\n");
        printf("3 - Imprimir todos os registros\n");
        printf("4 - Recuperar um registro pela key\n");
        printf("5 - Recuperar um registro pelo nome\n");
        printf("6 - Remover um registro\n");
        printf("7 - Sair\n");
        scanf("%d", &escolha);
        //Selecao das funcoes
        switch(escolha){
            case 1:
                save_multiple_reg(&arquivos, &reg);
                break;
            case 2:
                if(stdin_to_reg(&reg) != 0)
                    save_to_file(&arquivos, reg);
                break;
            case 3:
                print_all_registers(&arquivos, &reg);
                break;
            case 4:
                printf("Digite a chave para procura-la: ");
                scanf("%u", &temp);
                if(search_by_key(&arquivos, &reg, temp) == 0){
                    printf("Chave nao encontrada!\n\n");
                } else {
                    printf("Chave encontrada!\n");
                    print_register(&reg);
                }
                break;
            case 5:
                break;
            case 6:
                break;
            case 7:
                fclose(arquivos.dataBase);
                fclose(arquivos.primaryIndex);
                fclose(arquivos.secundaryIndex);
                return 0;
                break;
            default:
                printf("Escolha invalida\n\n");
        }
        printf("PRESSIONE [ENTER] para continuar\n");
        getchar();
        while( getchar() != '\n' );
    }
}