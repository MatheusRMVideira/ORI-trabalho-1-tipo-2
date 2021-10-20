//Trabalho Pratico 1- Organizacao e Recuperacao de Informacoes - UFScar
//Professor: Ricardo Cerri
//Integrantes do Grupo:
//Matheus Rezende Milani Videira
//Matheus da Silva Vieira
// 20/10/2021

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO
    Remocao logica
*/

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
int open_files(struct files *arquivos, FILE* out){
    long int firstEmptyReg = -1; //Os primeiros 4 bytes do arquivo principal sao utilizados para rastrear espacos disponiveis
    char fileEnd = '@'; //Alternativo ao EOF, que apresentou inconsistencias no Windows
    //Abre os arquivos
    arquivos->dataBase = fopen("./dataBase.bin", "rb+");
    arquivos->primaryIndex = fopen("./primaryIndex.bin", "rb+");
    arquivos->secundaryIndex = fopen("./secundaryIndex.bin", "rb+");
    //Caso um dos arquivos nao esteja aberto, cria ele e abre
    if(arquivos->dataBase == NULL){ 
        arquivos->dataBase = fopen("./dataBase.bin", "wb+");
        fwrite(&firstEmptyReg, sizeof(firstEmptyReg), 1, arquivos->dataBase);
        fwrite(&fileEnd, sizeof(fileEnd), 1, arquivos->dataBase);
    }
    if(arquivos->primaryIndex == NULL){
        arquivos->primaryIndex = fopen("./primaryIndex.bin", "wb+");
        fwrite(&fileEnd, sizeof(fileEnd), 1, arquivos->primaryIndex);
    }
    if(arquivos->secundaryIndex == NULL){
        arquivos->secundaryIndex = fopen("./secundaryIndex.bin", "wb+");
        fwrite(&fileEnd, sizeof(fileEnd), 1, arquivos->secundaryIndex);
    }
    //Caso nao consiga abrir ou criar um dos arquivos, finaliza o programa
    if(arquivos->dataBase == NULL || arquivos->primaryIndex == NULL || arquivos->secundaryIndex == NULL){
        fprintf(out, "Erro ao abrir os arquivos, finalizando o programa\n\n");
        fclose(arquivos->dataBase);
        fclose(arquivos->primaryIndex);
        fclose(arquivos->secundaryIndex);
        exit(0);
    }
    return 1;
}

//Realiza a insercao logica de um registro no arquivo, atualizando os indices primario e secundario
//Retorna 1;
int save_to_file(struct files *arquivos, struct registro reg){
    char validReg =  'V'; //O registro a ser inserido eh valido

    //Bloco de variaveis para armazenar o tamanho de cada campo do registro
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


    //Posiciona o arquivo principal e indice primario no inicio do arquivo
    fseek(arquivos->dataBase, 0, SEEK_SET);
    fseek(arquivos->primaryIndex, 0, SEEK_SET);

    //Algoritmo para insercao logica no arquivo principal e reutilizacao de espaco no indice primario
    //Utiliza a Worst-fit para determinar o melhor espaco para a insercao do registro
    unsigned int maxRegSize, tempRegSize; //Variaveis para armazenar o tamanho dos registros para comparacao
    char tempValidReg; //Armazena se o registro lido eh valido
    //Variaveis para armazenar a posicao dos registros da lista ligada
    long int maxRegPos, nextRegPos, tempRegPos, tempNextRegPos, tempPrevRegPos, prevRegPos;
    int endOfprimaryReg = 0; //Variavel que indica se o ponteiro esta no final ou nao do indice primario
    maxRegSize = 0;
    nextRegPos = 0;
    tempPrevRegPos = 0;

    //Le o inicio do arquivo principal para verificar se existe algum espaco livre
    fread(&tempNextRegPos, sizeof(tempNextRegPos), 1, arquivos->dataBase); 
    if(tempNextRegPos != -1){ //Caso possua, iniciar procura no arquivo principal e no indice primario

        while(tempNextRegPos != -1){ //Enquanto nao chegar ao final da lista ligada de espacos livres
            //Avanca para a posicao e armazena onde esta
            fseek(arquivos->dataBase, tempNextRegPos, SEEK_SET);
            tempRegPos = ftell(arquivos->dataBase);
            //Verifica se o registro realmente eh invalido(livre), ou se houve um erro e estamos na posicao incorreta
            fread(&tempValidReg, sizeof(tempValidReg), 1, arquivos->dataBase);
            if(tempValidReg != 'I')
                break; //Se o registro nao for invalido, sai do laco
            //Le o tamanho e a proxima posicao do registro
            fread(&tempRegSize, sizeof(tempRegSize), 1, arquivos->dataBase);
            fread(&tempNextRegPos, sizeof(tempRegPos), 1, arquivos->dataBase);
            //Se a posicao em que estamos, for a maior, armazena ela
            if(tempRegSize > maxRegSize && tempRegSize > regSize){
                maxRegSize = tempRegSize;
                maxRegPos = tempRegPos;
                nextRegPos = tempNextRegPos;
                prevRegPos = tempPrevRegPos;
            }
            //Avanca a variavel que marca a ultima posicao em que estavamos
            tempPrevRegPos = tempRegPos;
        }

        //Procura pela entrada invalida no indice primario para reutilizar o seu espaco
        while(1){

            if(fgetc(arquivos->primaryIndex) == '@'){ //Se chegar ao fim do arquivo
                fseek(arquivos->primaryIndex, -1, SEEK_CUR);
                endOfprimaryReg = 1;
                break; //Sai do laco
            }
            fseek(arquivos->primaryIndex, -1, SEEK_CUR);
            //Le se a entrada eh valida, se for invalida, sai do laco para escrever nela
            fread(&tempValidReg, sizeof(tempValidReg), 1, arquivos->primaryIndex);
            if(tempValidReg == 'I'){
                fseek(arquivos->primaryIndex, -1*(long int)sizeof(tempValidReg), SEEK_CUR);
                break;
            }
            //Caso seja valida, avanca a posicao no indice primario para a proxima entrada
            fseek(arquivos->primaryIndex, sizeof(reg.key) + 2*sizeof(long int), SEEK_CUR);
        }
    } else {
        //Caso nao exista espaco vazio, avanca para o final do arquivo de indice primario e marca que esta no final
        fseek(arquivos->primaryIndex, -1, SEEK_END);
        endOfprimaryReg = 1;
    }
    
    if(maxRegSize > 0){ //Se o algoritmo de Worst-Fit encontrou uma posicao
        fseek(arquivos->dataBase, maxRegPos, SEEK_SET); //Avanca para ela
    } else {
        fseek(arquivos->dataBase, -1, SEEK_END); //Senao avanca para o final do arquivo principal
    }

    
    //Cria a entrada no indice primario
    //Formato: [valido][chave][posicao][offset para proxima entrada]
    //Eh escrito em uma posicao vazia, ou no fim do arquivo de indice primario
    long int nextIndexEntry = -1; //Proxima entrada no indice primario (lista ligada)
    long int dataBasePos = ftell(arquivos->dataBase); //Posicao do registro no arquivo principal
    long int primaryIndexPos = ftell(arquivos->primaryIndex); //Posicao da entrada no indice primario
    fwrite(&validReg, sizeof(validReg), 1, arquivos->primaryIndex); //Escreve que a entrada no indice primario eh valida
    fwrite(&reg.key, sizeof(reg.key), 1, arquivos->primaryIndex); //Escreve a chave no indice primario
    fwrite(&dataBasePos, sizeof(dataBasePos), 1, arquivos->primaryIndex); //Escreve o offset correspondente ao registro no indice primario

    //Escreve no indice secundario
    //Formato:[tamanho do nome][nome][offset para indice primario]
    //O offset forma uma lista ligada com todas as entradas correspondentes a esse nome no indice primario
    unsigned int tempNameSize; //Temporario tamanho do nome
    char tempBuffer[256]; //Temporario armazena o nome
    int foundName = 0; //Se achou o nome no indice secundario ou nao

    //Volta ao inicio do arquivo de indice secundario
    fseek(arquivos->secundaryIndex, 0, SEEK_SET); 
    //Enquanto nao acabar o arquivo ou achar o nome
    while(1){
        //Detecta se chegou ao final do arquivo de indice secundario, se sim, sai do loop
        if(fgetc(arquivos->secundaryIndex) == '@'){
            fseek(arquivos->secundaryIndex, -1, SEEK_CUR);
            break;
        } 
        fseek(arquivos->secundaryIndex, -1, SEEK_CUR);
        //Le o tamanho do nome e o nome
        fread(&tempNameSize, sizeof(tempNameSize), 1, arquivos->secundaryIndex); 
        fread(&tempBuffer, 1, tempNameSize, arquivos->secundaryIndex);
        //Se o nome for igual ao que estamos inserindo
        if(strcmp(tempBuffer, reg.firstName) == 0){ 
            //Atualiza o offset a ser escrito no indice primario para apontar para entrada ja presente no indice, criando a lista ligada
            fread(&nextIndexEntry, sizeof(nextIndexEntry), 1, arquivos->secundaryIndex);
            fseek(arquivos->secundaryIndex, -1*(long int)sizeof(nextIndexEntry), SEEK_CUR); //Volta a posicao para escrever novo offset
            //Escreve novo offset correspondente a nova entrada no registro, que aponta para a entrada antiga
            fwrite(&primaryIndexPos, sizeof(primaryIndexPos), 1, arquivos->secundaryIndex);
            //Vai para o final do arquivo
            fseek(arquivos->secundaryIndex, 0, SEEK_END);
            foundName = 1; //Marca que encontrou o nome
            break;
        }
        //Caso nao seja o nome, avanca para proxima entrada
        fseek(arquivos->secundaryIndex, sizeof(nextIndexEntry), SEEK_CUR);
    }
    //Se chegou ao fim do indice e nao achou o nome, cria nova entrada no indice
    if(foundName == 0){
        char endOfFile = '@';
        fwrite(&firstNameSize, sizeof(firstNameSize), 1, arquivos->secundaryIndex);
        fwrite(&reg.firstName, 1, firstNameSize, arquivos->secundaryIndex);
        fwrite(&primaryIndexPos, sizeof(primaryIndexPos), 1, arquivos->secundaryIndex);
        fwrite(&endOfFile, sizeof(endOfFile), 1, arquivos->secundaryIndex); //Atualiza a posicao do substituto do EOF ('@')
    }

    //Escreve o offset para a proxima entrada no indice primario no indice primario
    fwrite(&nextIndexEntry, sizeof(nextIndexEntry), 1, arquivos->primaryIndex);

    //Se a entrada no indice primario foi escrita no final do arquivo, atualiza a posicao do '@'
    if(endOfprimaryReg){
        char endOfFile = '@';
        fwrite(&endOfFile, sizeof(endOfFile), 1, arquivos->primaryIndex);
    }

    //Escreve o registro no arquivo principal
    //Formato: [valido][tam. reg.][tam. chave][chave][tam. sobrenome][sobrenome][tam. nome][nome][tam. cidade][cidade][tam. endr.][endr.][tam. estado][estado][tam. CEP][CEP][tam. fone][fone]
    
    fwrite(&validReg, sizeof(validReg), 1, arquivos->dataBase); //Escreve que eh valido

    //Caso for inserido em um espaco livre dentro do arquivo, determina se o tamanho restante entre ele e o prox. registro
    //eh suficiente para inserir outro registro, caso nao seja, ocupa esse espaco e preenche com '#' para indicar fragmentacao

    long int regSizeDiff = maxRegSize - regSize; //Diferenca entre o tamanho do espaco disponivel e o espaco a ser preenchido
    long int minRegSize = sizeof(validReg) + 2*sizeof(regSizeDiff); //Tamanho minimo de um registro
    if(regSizeDiff < minRegSize && maxRegSize > 0){ //Caso o espaco restante seja menor
        fwrite(&maxRegSize, sizeof(unsigned int), 1, arquivos->dataBase); //Ocupa todo o espaco disponivel
    } else {
        fwrite(&regSize, sizeof(unsigned int), 1, arquivos->dataBase); //Senao, escreve o tamanho que registro realmente ocupa
    }
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

    if(maxRegSize < 1){ //Caso o registro foi no final do arquivo principal
        //Atualiza a posicao do '@'
        char endOfFile = '@';
        fwrite(&endOfFile, sizeof(endOfFile), 1, arquivos->dataBase);
        //Caso o registro foi inserido em espaco livre no meio do arquivo, e o espaco restante eh o suficiente para criar novo registro
    } else if(regSizeDiff >= minRegSize && maxRegSize > 0){
        //Cria novo registro invalido, para sinalizar espaco livre 
        validReg = 'I';
        long int resizedRegPos = ftell(arquivos->dataBase);
        fwrite(&validReg, sizeof(validReg), 1, arquivos->dataBase); //Escreve que eh invalido

        tempRegSize = regSizeDiff - sizeof(validReg) - sizeof(tempRegSize);
        fwrite(&tempRegSize, sizeof(tempRegSize), 1, arquivos->dataBase); //Escreve o espaco disponivel
        fwrite(&nextRegPos, sizeof(nextRegPos), 1, arquivos->dataBase); //Escreve a proxima posicao livre disponivel

        if(prevRegPos != 0){ //Caso a posicao anterior nao seja o inicio do arquivo
            //Avanca para a o campo de proxima regiao livre no registro da lista ligada anterior ao inserido 
            fseek(arquivos->dataBase, prevRegPos + sizeof(validReg) + sizeof(tempRegSize), SEEK_SET);
        } else {
            //Retorna ao inicio do arquivo
            fseek(arquivos->dataBase, 0, SEEK_SET);
        }
        //Escreve a posicao do novo registro invalido criado como espaco livre
        fwrite(&resizedRegPos, sizeof(resizedRegPos), 1, arquivos->dataBase);

    } else { //Caso o registro foi inserido em espaco livre no meio do arquivo, e o espaco restante nao eh o suficiente para criar novo registro
        char tempChar = '#';
        int i;
        for(i = 0; i < regSizeDiff; i++) //Preenche o espaco restante com '#'
            fwrite(&tempChar, sizeof(char), 1, arquivos->dataBase);

        if(prevRegPos != 0){ //Caso a posicao anterior nao seja o inicio do arquivo
            //Avanca para a o campo de proxima regiao livre no registro da lista ligada anterior ao inserido
            fseek(arquivos->dataBase, prevRegPos + sizeof(validReg) + sizeof(tempRegSize), SEEK_SET);
        } else {
            //Retorna ao inicio do arquivo
            fseek(arquivos->dataBase, 0, SEEK_SET);
        }
        //Escreve a proxima posicao livre no arquivo, removendo esse registro da lista de espacos livres
        fwrite(&nextRegPos, sizeof(nextRegPos), 1, arquivos->dataBase);
    }

    return 1;
}

//Captura a entrada de dados do usuario e salva em um registro
//Retorna: 1 - caso sucesso e 0 - caso falha
int in_to_reg(struct registro* reg, FILE* in, FILE* out){
    char strInput[256];
    char temp;
    fscanf(in, "%c",&temp); //Limpa possiveis '\n' presentes na stdin
    fprintf(out, "Inserindo um registro\n");
    //Captura, sanitiza e salva a chave
    fprintf(out, "Chave: ");
    if(fscanf(in, "%[^\n]", strInput) < 1)
        return 0;
    fscanf(in, "%c",&temp);
    reg->key= sanitize_input_integer(strInput);

    //Captura e salva o sobrenome
    fprintf(out, "Sobrenome: ");
    if(fscanf(in, "%[^\n]", strInput) < 1)
        return 0;
    fscanf(in, "%c",&temp);
    strcpy(reg->lastName, strInput);

    //Captura e salva o nome
    fprintf(out, "Nome: ");
    if(fscanf(in, "%[^\n]", strInput) < 1)
        return 0;
    fscanf(in, "%c",&temp);
    strcpy(reg->firstName, strInput);

    //Captura e salva o endereco
    fprintf(out, "Endereco: ");
    if(fscanf(in, "%[^\n]", strInput) < 1)
        return 0;
    fscanf(in, "%c",&temp);
    strcpy(reg->address, strInput);

    //Captura e salva a cidade
    fprintf(out, "Cidade: ");
    if(fscanf(in, "%[^\n]", strInput) < 1)
        return 0;
    fscanf(in, "%c",&temp);
    strcpy(reg->city, strInput);

    //Captura e salva o estado
    fprintf(out, "Estado (sigla): ");
    if(fscanf(in, "%[^\n]", strInput) < 1)
        return 0;
    fscanf(in, "%c",&temp);
    strcpy(reg->state, strInput);

    //Captura, sanitiza e salva o CEP
    fprintf(out, "Cep: ");
    if(fscanf(in, "%[^\n]", strInput) < 1)
        return 0;
    fscanf(in, "%c",&temp);
    reg->zip= sanitize_input_integer(strInput);

    //Captura, sanitiza e salva o telefone
    fprintf(out, "Telefone: ");
    if(fscanf(in, "%[^\n]", strInput) < 1)
        return 0;
    reg->phone = sanitize_input_integer(strInput);

    fprintf(out, "\n");
    return 1;
}

//Funcao para unir in_to_reg e save_to_file
//Torna possivel capturar e gravar multiplos registros sequencialmente
void save_multiple_reg(struct files *arquivos, struct registro *reg, FILE* in, FILE* out){
    while(1){
        fprintf(out, "Aperte [ENTER] sem digitar um dos campos para sair\n");
        if(in_to_reg(reg, in, out) == 0)
            return;
        save_to_file(arquivos, *reg);
    }
}

//Le um registro do arquivo principal e salva em reg
//Retorna: 1 - caso sucesso, 0 - caso falha, -1 - caso tenha chego ao final do arquivo
int read_register(struct files *arquivos, struct registro *reg){
    char validReg;
    unsigned int regSize, keySize, lastNameSize, firstNameSize;
    unsigned int addressSize, citySize, stateSize, zipSize, phoneSize;
    regSize = 0;

    //Verifica se nao eh o final do arquivo
    if(fgetc(arquivos->dataBase) == '@')
        return -1; //Caso seja retorna erro
    fseek(arquivos->dataBase, -1, SEEK_CUR);


    //Le se o registro eh valido e o tamanho dele
    fread(&validReg, sizeof(validReg), 1, arquivos->dataBase);
    fread(&regSize, sizeof(regSize), 1, arquivos->dataBase);

    //Caso nao seja valido, verifica se eh invalido ou lixo (final do arquivo)
    if(validReg != 'V'){
        if(validReg == 'I'){ //Caso seja invalido
            fseek(arquivos->dataBase, regSize, SEEK_CUR);//Ignora o registro e move para o proximo
            return 0; //Moveu para o proximo
        } else {
            return -1; //Caso seja lixo, retorna erro
        }
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

    
    if(regSize != 0) //Caso ainda possua algo no registro, avanca para o final do registro
        fseek(arquivos->dataBase, regSize, SEEK_CUR);

    return 1;

}

//Imprime o reg na stdout
//Retorna: 1 - caso sucesso, 0 - caso falha
int print_register(struct registro *reg, FILE* out){
    if(reg == NULL){
        return 0;
    }
    fprintf(out, "Pessoa %u:\n", reg->key);
    fprintf(out, "Sobrenome: %s\n", reg->lastName);
    fprintf(out, "Nome: %s \n", reg->firstName);
    fprintf(out, "Endereco: %s \n", reg->address);
    fprintf(out, "Cidade: %s\n", reg->city);
    fprintf(out, "Estado: %c%c\n", reg->state[0], reg->state[1]);
    fprintf(out, "CEP: %llu\n", reg->zip);
    fprintf(out, "Telefone: %llu\n\n", reg->phone);
    return 1;
}

//Funcao para unir read_register e print_all_register
//Torna possivel imprimir todos os registros validos presentes no arquivo principal
void print_all_registers(struct files *arquivos, struct registro *reg, FILE* out){
    int ret = 0;
    fseek(arquivos->dataBase, sizeof(long int), SEEK_SET); //Volta a posicao no arquivo principal para o inicio
    while(ret != -1){ //Enquanto nao chegar ao final do arquivo
        ret = read_register(arquivos, reg); //Le o registro e salva em reg
        if(ret == 1){
            print_register(reg, out); //Imprime reg
        }
    }
}

//Realiza a busca sequencial no arquivo principal
//Salva o registro correspondente a chave searchKey em reg
//Retorna: 1 - caso sucesso, 0 - caso falha
int search_by_key(struct files *arquivos, struct registro *reg, unsigned int searchKey){
    char validReg;
    unsigned int regSize, regKey, keySize, lastNameSize, firstNameSize;
    unsigned int addressSize, citySize, stateSize, zipSize, phoneSize;
    regSize = 0;

    fseek(arquivos->dataBase, sizeof(long int), SEEK_SET); //Volta a posicao no arquivo principal para o inicio
    
    while(1){ //Busca pelo arquivo todo

        //Verifica se chegou ao final do arquivo
        if(fgetc(arquivos->dataBase) == '@')
            return 0;
        fseek(arquivos->dataBase, -1, SEEK_CUR);

        fread(&validReg, sizeof(validReg), 1, arquivos->dataBase); //Le se o registro eh valido
        fread(&regSize, sizeof(regSize), 1, arquivos->dataBase); //Le o tamanho do registro

        if(validReg != 'V'){ //Caso nao seja valido
            if(validReg == 'I'){ //Caso seja invalido
                if(fseek(arquivos->dataBase, regSize, SEEK_CUR) != 0){ //Avanca para o proximo registro
                    return 0; //Caso falha no fseek, retorna falha
                }
                continue; //Pula para a proxima iteracao do loop
            }
            return 0; //Caso seja != 1 e != 0, retorna falha (lixo)
        }

        fread(&keySize, sizeof(unsigned int), 1, arquivos->dataBase); //Le o tamanho da chav
        fread(&regKey, keySize, 1, arquivos->dataBase); //Le a chave
        regSize -= sizeof(keySize) + keySize; //Reduz o tamanho da chave do tamanho total do arquivo

        if(regKey != searchKey){ //Caso a chave nao seja a que procura
            if(fseek(arquivos->dataBase, regSize, SEEK_CUR) != 0){ //Avanca para o proximo registro
                return 0; //Caso falha no fseek, retorna falha
            }
            continue; //Pula para a proxima iteracao do loop
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

        
        if(regSize != 0) //Caso ainda possua algo no registro, avanca para o final do registro
            fseek(arquivos->dataBase, regSize, SEEK_CUR);

        return 1;
    }    
}

//Funcao para buscar um nome no indice secundario
//Caso o nome seja encontrado, imprime todos os registros correspondentes
//Retorna: 1 - caso sucesso, 0 - caso falha
int search_by_name(struct files *arquivos, struct registro *reg, char *searchName, FILE* out){
    char tempStr[256];
    unsigned int tempUInt;
    long int dataBasePos, primaryIndexPos;
    char validReg;
    //Retorna para o inicio do indice secundario
    fseek(arquivos->secundaryIndex, 0, SEEK_SET);
    while(1){
        //Verifica se chegou ao final do arquivo
        if(fgetc(arquivos->secundaryIndex) == '@')
            return 0;
        fseek(arquivos->secundaryIndex, -1, SEEK_CUR);


        fread(&tempUInt, sizeof(tempUInt), 1, arquivos->secundaryIndex); //Le o tamanho do nome
        //Caso o tamanho seja igual ao procurado
        if(tempUInt == (strlen(searchName) + 1)){ 
            fread(&tempStr, 1, tempUInt, arquivos->secundaryIndex); //Le o nome
            //Caso o nome seja igual ao procurado
            if(strcmp(tempStr, searchName) == 0){ 
                fread(&primaryIndexPos, sizeof(primaryIndexPos), 1, arquivos->secundaryIndex); //Le o offset
                while(primaryIndexPos != -1){ //Laco para iterar sobre todas as entradas correspondentes no indice primario
                    //Avanca a posicao para a entrada no indice primario
                    fseek(arquivos->primaryIndex, primaryIndexPos, SEEK_SET); 
                    fread(&validReg, sizeof(validReg), 1, arquivos->primaryIndex); //Le se a entrada eh valida
                    fread(&tempUInt, sizeof(tempUInt), 1, arquivos->primaryIndex); //Le a chave
                    fread(&dataBasePos, sizeof(dataBasePos), 1, arquivos->primaryIndex); //Le a posicao no arquivo principal
                    if(validReg == 'V'){ //Caso seja valida
                        fseek(arquivos->dataBase, dataBasePos, SEEK_SET); //Avanca para a posicao no arquivo principal
                        if(read_register(arquivos, reg) == 1){ //Caso leia com sucesso
                            print_register(reg, out); //Imprime o registro
                        }
                    }
                    fread(&primaryIndexPos, sizeof(primaryIndexPos), 1, arquivos->primaryIndex); //Le o offset da proxima entrada correspondente
                }
                //Avanca todos os arquivos para o final, por seguranca
                fseek(arquivos->dataBase, 0, SEEK_END);
                fseek(arquivos->primaryIndex, 0, SEEK_END);
                fseek(arquivos->secundaryIndex, 0 , SEEK_END);
                return 1;
            } else { //Caso nao seja o nome procurado, avanca para a proxima entrada no indice secundario
                fseek(arquivos->secundaryIndex, sizeof(primaryIndexPos), SEEK_CUR);
            }
        } else { //Caso o tamanho nao seja igual, avanca para a proxima entrada no indice secundario
            fseek(arquivos->secundaryIndex, tempUInt + sizeof(primaryIndexPos), SEEK_CUR);
        }
    }
}

//Funcao para a remocao logica de registros
//Marca o registro como invalido 'I' no arquivo principal e no indice primario
//Tambem remove todas as referencias ao registro do indice primario e secundario
//Formato do espaco livre: [valido][tam. reg.][offset lista]
//Retorna: 1 - caso sucesso, 0 - caso falha
int logical_removal(struct files *arquivos, unsigned int removeKey){
    unsigned int entryKey, regSize;
    char validReg;
    long int nextIndexPos, thisIndexPos, tempIndexPos, tempIndexPos2;
    long int dataBasePos;
    //Retorna o indice primario ao inicio do arquivo
    fseek(arquivos->primaryIndex, 0, SEEK_SET);

    //Procura pela entrada correspondente ao registro no indice primario
    while(1){
        //Verifica se chegou ao final do arquivo
        if(fgetc(arquivos->primaryIndex) == '@')
            return 0; //Chegou ao final do indice primario sem encontrar a chave
        fseek(arquivos->primaryIndex, -1, SEEK_CUR);

        //Armazena a posicao da entrada atual e le se a entrada eh valida
        thisIndexPos = ftell(arquivos->primaryIndex); 
        fread(&validReg, sizeof(validReg), 1, arquivos->primaryIndex);
        if(validReg == 'V'){ //Caso seja valida
            //Le a chave e compara com a que procuramos
            fread(&entryKey, sizeof(entryKey), 1, arquivos->primaryIndex);
            if(entryKey == removeKey){ //Caso seja a chave
                //Le a posicao do registro no arquivo principal
                fread(&dataBasePos, sizeof(dataBasePos), 1, arquivos->primaryIndex); 
                validReg = 'I';
                //Retorna para validReg no indice primario
                fseek(arquivos->primaryIndex, -1*(long int)(sizeof(validReg)+sizeof(entryKey)+sizeof(dataBasePos)), SEEK_CUR);

                //Escreve que a entrada nao eh valida no indice primario e le o offset para a proxima entrada da lista ligada
                fwrite(&validReg, sizeof(validReg), 1, arquivos->primaryIndex);
                fseek(arquivos->primaryIndex, sizeof(entryKey) + sizeof(dataBasePos), SEEK_CUR);
                fread(&nextIndexPos, sizeof(nextIndexPos), 1, arquivos->primaryIndex);

                //Volta para o campo do offset e armazena a posicao
                fseek(arquivos->primaryIndex, -1*(long int)sizeof(tempIndexPos), SEEK_CUR);
                tempIndexPos2 = ftell(arquivos->primaryIndex);

                //Procura a entrada anterior a ele na lista ligada e se remove da lista
                //Volta ao inicio do arquivo
                fseek(arquivos->primaryIndex, 0, SEEK_SET);
                while(1){
                    //Verifica se chegou ao final do arquivo
                    if(fgetc(arquivos->primaryIndex) == '@')
                        break;
                    fseek(arquivos->primaryIndex, -1, SEEK_CUR);

                    fread(&validReg, sizeof(validReg), 1, arquivos->primaryIndex); //Le se a entrada eh valida
                    if(validReg == 'V'){ //Caso seja valida
                        //Le se o offset da entrada eh a posicao da entrada que estamos removendo
                        fseek(arquivos->primaryIndex, (sizeof(entryKey) + sizeof(dataBasePos)), SEEK_CUR);
                        fread(&tempIndexPos, sizeof(tempIndexPos), 1, arquivos->primaryIndex);
                        if(tempIndexPos == thisIndexPos){ //Caso o offset seja a posicao
                            //Escreve que o offset eh o proximo da entrada que estamos removendo
                            //A entrada que estamos removendo nao esta mais presente na lista ligada
                            fseek(arquivos->primaryIndex, -1*(long int)sizeof(tempIndexPos), SEEK_CUR);
                            fwrite(&nextIndexPos, sizeof(nextIndexPos), 1, arquivos->primaryIndex);
                            break;
                        }
                    } else {
                        //Caso a entrada nao seja valida, avancar para a proxima
                        fseek(arquivos->primaryIndex, (sizeof(entryKey) + sizeof(dataBasePos) + sizeof(tempIndexPos)), SEEK_CUR);
                    }
                }

                //Volta para a posicao que estavamos antes de procurar pela entrada anterior na lista
                fseek(arquivos->primaryIndex, tempIndexPos2, SEEK_SET);
                tempIndexPos = -1;

                //Procura no indice secundario para remover referencia a entrada, caso exista
                //Volta o indice secundario para o inicio do arquivo
                fseek(arquivos->secundaryIndex, 0, SEEK_SET);
                unsigned int firstNameSize;
                while(1){
                    //Verifica se chegou ao final do arquivo
                    if(fgetc(arquivos->secundaryIndex) == '@')
                        break;
                    fseek(arquivos->secundaryIndex, -1, SEEK_CUR);
                    //Le o offset do indice primario salvo na entrada do indice secundario
                    fread(&firstNameSize, sizeof(firstNameSize), 1, arquivos->secundaryIndex);
                    fseek(arquivos->secundaryIndex, firstNameSize, SEEK_CUR);
                    fread(&tempIndexPos, sizeof(tempIndexPos), 1, arquivos->secundaryIndex);

                    if(tempIndexPos == thisIndexPos){ //Caso o offset seja o da entrada que estamos removendo
                        //Escreve que o novo offset eh o da proxima entrada da lista
                        fseek(arquivos->secundaryIndex, -1*(long int)sizeof(tempIndexPos), SEEK_CUR);
                        fwrite(&nextIndexPos, sizeof(nextIndexPos), 1, arquivos->secundaryIndex);
                        break;
                    }
                }
                //Sai do laco principal
                break;

            } else {
                //Caso nao seja a chave, avanca para a proxima entrada
                fseek(arquivos->primaryIndex, (sizeof(dataBasePos) + sizeof(nextIndexPos)), SEEK_CUR);
            }
        } else {
            //Caso a entrada nao seja valida, avanca para a proxima entrada
            fseek(arquivos->primaryIndex, (sizeof(entryKey) + sizeof(dataBasePos) + sizeof(nextIndexPos)), SEEK_CUR);
        }
    }

    //Insere o registro que removemos no final da lista ligada de espacos livres
    long int tempPos;
    //Retorna ao inicio do arquivo principal
    fseek(arquivos->dataBase, 0, SEEK_SET);
    //Le o inicio da lista
    fread(&tempPos, sizeof(tempPos), 1, arquivos->dataBase);
    //Enquanto nao chegar ao fim da lista, avanca sobre ela
    while(tempPos != -1){ 
        fseek(arquivos->dataBase, tempPos + sizeof(validReg) + sizeof(regSize), SEEK_SET);
        fread(&tempPos, sizeof(tempPos), 1, arquivos->dataBase);
    }
    //Escreve o offset para o espaco que removemos no ultimo registro da lista
    fseek(arquivos->dataBase, -1*(long int)sizeof(tempPos), SEEK_CUR);
    fwrite(&dataBasePos, sizeof(dataBasePos), 1, arquivos->dataBase);

    //Avanca para o registro que removemos no arquivo principal
    fseek(arquivos->dataBase, dataBasePos, SEEK_SET); 
    validReg = 'I';
    //Escreve que o arquivo eh invalido e que o proximo espaco livre possui offset -1 (nao existe)
    fwrite(&validReg, sizeof(validReg), 1, arquivos->dataBase);
    fseek(arquivos->dataBase, sizeof(regSize), SEEK_CUR);
    tempPos = -1;
    fwrite(&tempPos, sizeof(tempPos), 1, arquivos->dataBase);

    return 1;
}


int main(){
    FILE* out, *in;
    int escolha;
    printf("Deseja utilizar o Console (1) ou uma Bateria de testes (2)?\n");
    printf("Por favor digite o numero da sua escolha\n");
    scanf("%d", &escolha);
    getchar();
    if(escolha == 2){
        out = fopen("./output.txt", "w");
        in = fopen("./input.txt", "r");
        if(in == NULL){
            printf("Erro ao ler arquivo input.txt, finalizando o programa\n");
            fclose(in);
            fclose(out);
            return 0;
        }
    } else {
        out = stdout;
        in = stdin;
    } 
    struct registro reg;
    struct files arquivos;
    unsigned int temp;
    open_files(&arquivos, out);
    while(1){
        //Imprime menu principal
        fprintf(out, "Digite o numero para escolher uma das opcoes abaixo:\n");
        fprintf(out, "1 - Insercao de varios registros\n");
        fprintf(out, "2 - Inserir um registro\n");
        fprintf(out, "3 - Imprimir todos os registros\n");
        fprintf(out, "4 - Recuperar um registro pela key\n");
        fprintf(out, "5 - Recuperar um registro pelo nome\n");
        fprintf(out, "6 - Remover um registro\n");
        fprintf(out, "7 - Sair\n");
        fscanf(in,"%d", &escolha);
        //Selecao das funcoes
        switch(escolha){
            case 1:
                save_multiple_reg(&arquivos, &reg, in, out);
                break;
            case 2:
                if(in_to_reg(&reg, in, out) != 0)
                    save_to_file(&arquivos, reg);
                break;
            case 3:
                print_all_registers(&arquivos, &reg, out);
                break;
            case 4:
                fprintf(out, "Digite a chave para procura-la: ");
                fscanf(in, "%u", &temp);
                if(search_by_key(&arquivos, &reg, temp) == 0){
                    fprintf(out, "Chave nao encontrada!\n\n");
                } else {
                    fprintf(out, "Chave encontrada!\n");
                    print_register(&reg, out);
                }
                break;
            case 5:
                char tempStr[256];
                fprintf(out, "Digite um nome para procura-lo: ");
                fscanf(in, "%s", tempStr);
                if(search_by_name(&arquivos, &reg, tempStr, out) == 1){
                    fprintf(out, "Essas foram as entradas encontradas\n\n");
                } else {
                    fprintf(out, "Nao foi encontrada nenhuma entrada\n\n");
                }
                break;
            case 6:
                fprintf(out, "Digite a chave a ser removida: ");
                fscanf(in, "%u", &temp);
                if(logical_removal(&arquivos, temp) == 1){
                    fprintf(out, "Registro removido com sucesso!\n\n");
                } else {
                    fprintf(out, "Registro nao encontrado\n\n");
                }
                break;
            case 7:
                fclose(arquivos.dataBase);
                fclose(arquivos.primaryIndex);
                fclose(arquivos.secundaryIndex);
                fclose(in);
                fclose(out);
                return 0;
                break;
            default:
                fprintf(out, "Escolha invalida\n\n");
        }
        if(in == stdin){
            fprintf(out, "PRESSIONE [ENTER] para continuar\n");
            getchar();
            while( getchar() != '\n' );
        }
    }
}