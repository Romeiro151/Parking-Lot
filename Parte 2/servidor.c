/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2024/2025, Enunciado Versão 2+
 **
 ** Aluno: Nº: 129838      Nome: José Miguel Oliveira Romeiro
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo:
 **  Neste módulo procura-se implementar um servidor que trata do check-in de viaturas.
 **  Começo por iniciar o servidor, que recebe um único argumento, a dimensão do parque e cria, com bases no argumento, o parque de estacionamento.
 **  De seguida, arma os sinais SIGINT, para tratar do CTRL-C, que termina o servidor assim que é pressionado, e SIGCHLD para terminar os processos filhos. Para terminar cria o ficheiro server.fifo.
 **  Começo por fazer um ciclo que garante o funcionamento do servidor, onde leio o pedido do Cliente, verifico se existe algum lugar disponível e, caso exista, crio um servidor dedicado(filho).
 **  Para terminar o servidor, começo por eliminar o server.fifo. Caso este não seja terminado e um dos seus filhos for, envia-se um sinal para o servidor a alertar de tal e termina o servidor dedicado corresponde ao sinal enviado.
 **  No que toca ao servidor dedicado, começa-se por armar os sinais a serem tratados por este e certifica-se, caso não exista lugar disponível no estacionamento, que envia um sinal ao Cliente a avisá-lo do mesmo.
 **  Valida-se o pedido do Cliente, verificando os argumentos recebidos. Caso esteja tudo bem, envia-se um sinal ao cliente e regista-se a entrada da viatura no parque de estacionamento.
 **  Caso o cliente saia do parque, atualiza-se o registo com a data de saída do parque de estacionamento.
 **  Para encerrar um servidor dedicado, apaga-se o registo do cliente, atualizando o array de estacionamento e o logfile. Envia também um sinal ao cliente a avisar  que o check-in terminou.
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"

/*** Variáveis Globais ***/
Estacionamento clientRequest;           // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento;  // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD;                     // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile;                    // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile
char *getDataHoraAtual();             // String com a data e hora atual
/**
 * @brief Processamento do processo Servidor e dos processos Servidor Dedicado
 */

int main(int argc, char *argv[]) {
    so_debug("<");

    s1_IniciaServidor(argc, argv);
    s2_MainServidor();

    so_error("Servidor", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief  s1_IniciaServidor    Ler a descrição da tarefa S1 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");

    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);

    s1_2_CriaBD(dimensaoMaximaParque, &lugaresEstacionamento);

    s1_3_ArmaSinaisServidor();

    s1_4_CriaFifoServidor(FILE_REQUESTS);   

    so_debug(">");
}

void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pDimensaoMaximaParque) {
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);

    if(argc != 2){
        so_error("S1.1", "Número de argumentos inválido");
        exit(1);   
    }

    char *arg = argv[1];
    int i =0;
    *pDimensaoMaximaParque = 0;

    while(arg[i] != '\0') {
        if( arg[i] < '0' || arg[i] > '9') {
            so_error("S1.1", "Argumento não é um número");
            exit(1);
        }
        *pDimensaoMaximaParque = *pDimensaoMaximaParque * 10 + (arg[i] - '0');
        i++;
    }

    if (*pDimensaoMaximaParque < 1) {
        so_error("S1.1", "Dimensão do parque inválida");
        exit(1);
    }

    so_success("S1.1", "Servidor iniciado!" );
    so_debug("> [*pDimensaoMaximaParque:%d]", *pDimensaoMaximaParque);
}
/**
 * @brief  s1_2_CriaBD      Ler a descrição da tarefa S1.2 no enunciado
 * @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param  plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
 * @return Success  (TRUE or FALSE)
 */
void s1_2_CriaBD(int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    so_debug("< [@param dimensaoMaximaParque:%d]", dimensaoMaximaParque);

    *plugaresEstacionamento = (Estacionamento *)malloc(dimensaoMaximaParque * sizeof(Estacionamento));

    if (*plugaresEstacionamento == NULL) {
        so_error("S1.2", "Não foi possíevel alocar a memória para o array de lugares de estacionamento");
        exit(1);
    }

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        (*plugaresEstacionamento)[i].pidServidorDedicado = DISPONIVEL;
        (*plugaresEstacionamento)[i].pidCliente = DISPONIVEL;
    }

    so_success("S1.2", "Estacionamento criado!");

    so_debug("> [*plugaresEstacionamento:%p]", *plugaresEstacionamento);

}

/**
 * @brief  s1_3_ArmaSinaisServidor  Ler a descrição da tarefa S1.3 no enunciado
 * @return Success  (TRUE or FALSE)
 */


void s1_3_ArmaSinaisServidor() {
    so_debug("<");

    if(signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {
        so_error("S1.3", "Não foi possível armar o sinal SIGINT");
        exit(1);
    }
    if(signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) {
        so_error("S1.3", "Não foi possível sinal SIGCHLD");
        exit(1);
    }

    so_success("S1.3", "Sinais armados");
    so_debug(">");
}

/**
 * @brief  s1_4_CriaFifoServidor    Ler a descrição da tarefa S1.4 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @return Success  (TRUE or FALSE)
 */
void s1_4_CriaFifoServidor(char *filenameFifoServidor) {
    so_debug("<");

    unlink(filenameFifoServidor);

    if(mkfifo(filenameFifoServidor, 0666) == -1) {
        so_error("S1.4", "Não foi possível criar o FIFO do servidor");
        exit(1);
    }

    so_success("S1.4", "FIFO do servidor criado");
    so_debug(">");
}

/**
 * @brief  s2_MainServidor      Ler a descrição da taref S2 no enunciado
 * @return Success  (TRUE or FALSE)a
 */
void s2_MainServidor() {
    so_debug("<");

    FILE *fFifoServidor;

    while( TRUE ) {
        s2_1_AbreFifoServidor(FILE_REQUESTS, &fFifoServidor);
        s2_2_LePedidosFifoServidor(fFifoServidor);
        sleep(10);
    }

    so_debug(">");
}

/**
 * @brief  s2_1_AbreFifoServidor    Ler a descrição da tarefa S2.1 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @param  pfFifoServidor (O) descritor aberto do ficheiro do FIFO do servidor
 * @return Success  (TRUE or FALSE)
 */
void s2_1_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    FILE *file = NULL;
    int try = 5;

    while (try > 0) {
        file = fopen(filenameFifoServidor, "rb");
        if (file != NULL){
            break;
        }
        try--;
    }

    *pfFifoServidor = file;
    if (*pfFifoServidor == NULL) {
        so_error("S2.1", "Não foi possível abrir o FIFO");
        s4_EncerraServidor(filenameFifoServidor);
        return;
    }

    so_success("S2.1", "FIFO do servidor aberto");
    so_debug("> [*pfFifoServidor:%p]", *pfFifoServidor);
}

/**
 * @brief  s2_2_LePedidosFifoServidor    Ler a descrição da tarefa S2.2 no enunciado
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 * @return Success  (TRUE or FALSE)
 */
void s2_2_LePedidosFifoServidor(FILE *fFifoServidor) {
    so_debug("<");

    int terminaCiclo2 = FALSE;
    while (TRUE) {
        terminaCiclo2 = s2_2_1_LePedido(fFifoServidor, &clientRequest);
        if (terminaCiclo2)
            break;
        s2_2_2_ProcuraLugarDisponivelBD(clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);
        s2_2_3_CriaServidorDedicado(lugaresEstacionamento, indexClienteBD);
    }

    so_debug(">");
}

/**
 * @brief  s2_2_1_LePedido    Ler a descrição da tarefa S2.2.1 no enunciado
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 * @param  pclientRequest (O) pedido recebido, enviado por um Cliente
 * @return Success  (TRUE or FALSE)
 */
int s2_2_1_LePedido(FILE *fFifoServidor, Estacionamento *pclientRequest) {
    int naoHaMaisPedidos = TRUE;
    so_debug("< [@param fFifoServidor:%p]", fFifoServidor);
    
    errno = 0;
    size_t read = fread(pclientRequest, sizeof(Estacionamento), 1, fFifoServidor);

    if (read == 1){
        naoHaMaisPedidos = FALSE;
        so_success("S2.2.1", "Li Pedido do FIFO");
    }else if (read == 0 && errno == 0) {
        fFifoServidor = NULL;
        so_success("S2.2.1", "Não há mais registos no FIFO");

    }
    else {
        so_error("S2.2.1", "Não foi possível ler o FIFO do servidor");
        s4_EncerraServidor(FILE_REQUESTS);
    }

    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d.%d]]", pclientRequest->viatura.matricula, pclientRequest->viatura.pais, pclientRequest->viatura.categoria, pclientRequest->viatura.nomeCondutor, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
    return naoHaMaisPedidos;
}

/**
 * @brief  s2_2_2_ProcuraLugarDisponivelBD  Ler a descrição da tarefa S2.2.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param  pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 * @return Success  (TRUE or FALSE)
 */
void s2_2_2_ProcuraLugarDisponivelBD(Estacionamento clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado, lugaresEstacionamento, dimensaoMaximaParque);

    int position = -1;
    for(int index = 0; index < dimensaoMaximaParque; index++){
        if(lugaresEstacionamento[index].pidCliente == -1){
            lugaresEstacionamento[index] = clientRequest;
            position = index;
            break;
        }
    }

    if(position != -1){
        *pindexClienteBD = position;
        so_success("S2.2.2", "Reservei Lugar: %d", position);
    }
    else{
        *pindexClienteBD = -1;
        so_error("S2.2.2", "Não há lugar disponível");
    }

    so_debug("> [*pindexClienteBD:%d]", *pindexClienteBD);
}

/**
 * @brief  s2_2_3_CriaServidorDedicado    Ler a descrição da tarefa S2.2.3 no enunciado
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 * @return Success  (TRUE or FALSE)
 */

void s2_2_3_CriaServidorDedicado(Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);

    pid_t pidServidorDedicado = fork();
    
    if(pidServidorDedicado == 0){
        so_success("S2.2.3", "SD: Nasci com PID %d", getpid());
        sd7_MainServidorDedicado();
        exit(0);
    }

    else if( pidServidorDedicado > 0){
        lugaresEstacionamento[indexClienteBD].pidServidorDedicado = pidServidorDedicado;
        so_success("S2.2.3" , "Servidor: Iniciei SD %d", pidServidorDedicado);
    }

    else{
        so_error("S2.2.3", "Não foi possível criar o servidor dedicado");
        s4_EncerraServidor(FILE_REQUESTS);
    }

    so_debug(">");
}

/**
 * @brief  s3_TrataCtrlC    Ler a descrição da tarefa S3 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s3_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("S3", "Servidor: Start Shutdown");

    int index = 0;
    while(index < dimensaoMaximaParque){
        pid_t pidServidor = lugaresEstacionamento[index].pidServidorDedicado;
        pid_t pidCliente = lugaresEstacionamento[index].pidCliente;

        if(pidCliente > 0 && pidServidor != DISPONIVEL){
            int result = kill(pidServidor, SIGUSR2);
            if(result == -1){
                so_error("S3", "Não foi possível enviar o SIGUSR2 para o servidor dedicado %d", pidServidor);
                s4_EncerraServidor(FILE_REQUESTS);
                exit(1);
            }
            else{
                so_success("S3", "Enviei sinal SIGUSR2 para o servidor dedicado %d", pidServidor);
                lugaresEstacionamento[index].pidServidorDedicado = DISPONIVEL;
                lugaresEstacionamento[index].pidCliente = DISPONIVEL;
            }
        }

        index++;
    }

    s4_EncerraServidor(FILE_REQUESTS);
    so_debug(">");
}

/**
 * @brief  s4_EncerraServidor    Ler a descrição da tarefa S4 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @return Success  (TRUE or FALSE)
 */
void s4_EncerraServidor(char *filenameFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    if(unlink(filenameFifoServidor) == -1) {
        so_error("S4", "Não foi possível remover o server.fifo da diretoria local");
        exit(0);
    }

    so_success("S4", "Confirmo que terminou o SD %d", getpid());
    exit(0);

    so_debug(">");
}

/**
 * @brief  s5_TrataTerminouServidorDedicado    Ler a descrição da tarefa S5 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    int process;
    pid_t finishedPID = wait(&process);

    if(finishedPID > 0){
        so_success("S5", "Servidor: Confirmo que terminou o SD %d", finishedPID);
    }

    so_debug(">");
}

/**
 * @brief  sd7_IniciaServidorDedicado    Ler a descrição da tarefa SD7 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void sd7_MainServidorDedicado() {
    so_debug("<");

    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_ValidaLugarDisponivelBD(indexClienteBD);

    so_debug(">");
}

/**
 * @brief  sd7_1_ArmaSinaisServidorDedicado    Ler a descrição da tarefa SD7.1 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void sd7_1_ArmaSinaisServidorDedicado() {
    so_debug("<");

    if (signal(SIGUSR2, sd11_EncerraServidorDedicado) == SIG_ERR || signal(SIGUSR1, sd11_EncerraServidorDedicado) == SIG_ERR || signal(SIGINT, sd11_EncerraServidorDedicado) == SIG_ERR) {
        so_error("SD7.1", "Não foi possível armar o SIGUSR2 no servidor dedicado");
        exit(1);
    }
    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, sd13_TrataSigusr1);
    signal(SIGUSR2, sd12_TrataSigusr2);
    so_success("SD7.1", "Sinais armados com sucesso");

    so_debug(">");
}

/**
 * @brief  sd7_2_ValidaPidCliente    Ler a descrição da tarefa SD7.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @return Success  (TRUE or FALSE)
 */
void sd7_2_ValidaPidCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    int pidCliente = clientRequest.pidCliente;
    if(pidCliente > 0){
        so_success("SD7.2", "PID do cliente válido");
    } 
    else{
        so_error("SD7.2", "PID do cliente inválido");
        exit(1);
    }

    so_debug(">");
}

/**
 * @brief  sd7_3_ValidaLugarDisponivelBD    Ler a descrição da tarefa SD7.3 no enunciado
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 * @return Success  (TRUE or FALSE)
 */
void sd7_3_ValidaLugarDisponivelBD(int indexClienteBD) {
    so_debug("< [@param indexClienteBD:%d]", indexClienteBD);

    if(indexClienteBD < 0){
        so_error("SD7.3", "Não há lugar disponível");
        exit(1);

        if(lugaresEstacionamento[indexClienteBD].pidCliente > 0){
            if(kill(lugaresEstacionamento[indexClienteBD].pidCliente, SIGUSR2) == -1){
                so_error("SD7.3", "Erro ao enviar sinal SIGUSR2 para o servidor dedicado %d", lugaresEstacionamento[indexClienteBD].pidCliente);
            }else{
                so_success("SD7.3", "Enviei sinal SIGUSR2 para o servidor dedicado %d", lugaresEstacionamento[indexClienteBD].pidCliente);
            }
        }
        sd11_EncerraServidorDedicado();
    }
    
    else{
        so_success("SD7.3", "Há lugar disponível");
    }
    so_debug(">");
}

/**
 * @brief  sd8_ValidaPedidoCliente    Ler a descrição da tarefa SD8 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void sd8_ValidaPedidoCliente() { 
    so_debug("<");

    sd8_1_ValidaMatricula(clientRequest);

    sd8_2_ValidaPais(clientRequest);

    sd8_3_ValidaCategoria(clientRequest);

    sd8_4_ValidaNomeCondutor(clientRequest);

    so_success("sd8", "Pedido do cliente válido");
    so_debug(">");
}

/**
 * @brief  sd8_1_ValidaMatricula    Ler a descrição da tarefa SD8.1 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @return Success  (TRUE or FALSE)
 */
void sd8_1_ValidaMatricula(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    const char *licensePlate = clientRequest.viatura.matricula;

    for(int index = 0; licensePlate[index] != '\0'; index++){
        char currentChar = licensePlate[index];
        if(!isupper(currentChar) && !isdigit(currentChar)){
            so_error("SD8.1", "Matricula inválida");
            sd11_EncerraServidorDedicado();
            exit(1);
        }
    }

    so_success("SD8.1", "Matricula válida");
    so_debug(">");
}

/**
 * @brief  sd8_2_ValidaPais    Ler a descrição da tarefa SD8.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @return Success  (TRUE or FALSE)
 */
void sd8_2_ValidaPais(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    const char *country = clientRequest.viatura.pais;
    if(strlen(country) != 2 || !isupper(country[0]) || !isupper(country[1])){
        so_error("SD8.2", "Código de país inválido");
        sd11_EncerraServidorDedicado();
    }

    so_success("SD8.2", "Código de país válido");
    so_debug(">");
}

/**
 * @brief  sd8_3_ValidaCategoria    Ler a descrição da tarefa SD8.3 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @return Success  (TRUE or FALSE)
 */
void sd8_3_ValidaCategoria(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    char category = clientRequest.viatura.categoria;
    if(category != 'P' && category != 'L' && category != 'M'){
        so_error("SD8.3", "Categoria inválida");
        sd11_EncerraServidorDedicado();
    }

    so_success("SD8.3", "Categoria válida");
    so_debug(">");
}

/**
 * @brief  sd8_4_ValidaNomeCondutor    Ler a descrição da tarefa SD8.4 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @return Success  (TRUE or FALSE)
 */
void sd8_4_ValidaNomeCondutor(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    char *name = clientRequest.viatura.nomeCondutor;
    FILE *usersFile = fopen(FILE_USERS, "r");
    if(usersFile == NULL){
        so_error("SD8.4", "Não foi possível abrir o ficheiro _etc_passwd");
        sd11_EncerraServidorDedicado();
    }

    char line[512];
    while(so_fgets(line, sizeof(line), usersFile)){
        char lineCopy[512];
        strcpy(lineCopy, line);

        char *frags[7] = {NULL};
        int counter = 0;
        char *part = strtok(lineCopy, ":");

        while(part != NULL && counter < 7){
            frags[counter ++] = part;
            part = strtok(NULL, ":");
        }

        if( counter >= 5 && frags[4][0] != '\0'){
            char driverName[80];
            strcpy(driverName,frags[4]);

            char *comma = strstr(driverName, ",");
            if(comma != NULL){
                *comma = '\0';
            }

            if(strcmp(driverName, name) == 0){
                so_success("SD8.4", "Nome do condutor corresponde a um utilizador do sistema");
                return;
            }
        }

    }

    fclose(usersFile);
    so_error("SD8.4", "Nome do condutor não existe no tigre");
    sd11_EncerraServidorDedicado();
    so_debug(">");
}

/**
 * @brief  sd9_EntradaCliente    Ler a descrição da tarefa SD9 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void sd9_EntradaCliente() {
    so_debug("<");

    sd9_1_AdormeceTempoRandom();

    sd9_2_EnviaSigusr1AoCliente(clientRequest);
    
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    so_debug(">");
}

/**
 * @brief  sd9_1_AdormeceTempoRandom    Ler a descrição da tarefa SD9.1 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void sd9_1_AdormeceTempoRandom() {
    so_debug("<");

    int time = rand() % MAX_ESPERA + 2;
    sleep(time);

    so_success("SD9.1", "%d", time);    
    so_debug(">");
}

/**
 * @brief  sd9_2_EnviaSigusr1AoCliente    Ler a descrição da tarefa SD9.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @return Success  (TRUE or FALSE)
 */
void sd9_2_EnviaSigusr1AoCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    int place = -1;
    int pidCliente = clientRequest.pidCliente;
    for(int index=0; index < dimensaoMaximaParque; index++){
        if(lugaresEstacionamento[index].pidCliente == pidCliente){
            place = index;
            break;
        }
    }

    if(kill(pidCliente, SIGUSR1) == -1){
        so_error("SD9.2", "Erro ao enviar sinal SIGUSR1 ao cliente");
        sd11_EncerraServidorDedicado();
    }
    
    so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", place);
    so_debug(">");
}

/**
 * @brief  s1_IniciaServidor    Ler a descrição da tarefa S1 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @param  pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  plogItem (O) registo de Log para esta viatura
 * @return Success  (TRUE or FALSE)
 */
void sd9_3_EscreveLogEntradaViatura(char *logFilename, Estacionamento clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [@param logFilename:%s, clientRequest:[%s:%s:%c:%s:%d:%d]]",
             logFilename,
             clientRequest.viatura.matricula,
             clientRequest.viatura.pais,
             clientRequest.viatura.categoria,
             clientRequest.viatura.nomeCondutor,
             clientRequest.pidCliente,
             clientRequest.pidServidorDedicado);

    FILE *pointer = fopen(logFilename, "a+b");
    if (!pointer){
        so_error("SD9.3", "Erro ao abrir logFile");
        sd11_EncerraServidorDedicado();
    }

    fseek(pointer, 0, SEEK_END);
    long size = ftell(pointer);
    if (pposicaoLogfile) {
        *pposicaoLogfile = size;
    }

    time_t timer = time(NULL);
    struct tm *currentTime = localtime(&timer);
    if (!currentTime) {
        so_error("SD9.3", "Não foi possível obter o tempo atual");
        fclose(pointer);
        sd11_EncerraServidorDedicado();
    }

    LogItem newLogItem;
    newLogItem.viatura = clientRequest.viatura;
    strftime(newLogItem.dataEntrada, sizeof(newLogItem.dataEntrada), "%Y-%m-%dT%Hh%M", currentTime);
    strcpy(newLogItem.dataSaida, "");

    size_t written = fwrite(&newLogItem, sizeof(LogItem), 1, pointer);
    if (written != 1){
        so_error("SD9.3", "Não foi possível escrever no logfile");
        fclose(pointer);
        sd11_EncerraServidorDedicado();
    }

    fclose(pointer);

    if (plogItem) {
        *plogItem = newLogItem;
    }

    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s",
               *pposicaoLogfile,
               (*plogItem).viatura.matricula,
               (*plogItem).dataEntrada);

    so_debug("> [*pposicaoLogfile:%ld, *plogItem:[%s:%s:%c:%s:%s:%s]]",
             *pposicaoLogfile,
             plogItem->viatura.matricula,
             plogItem->viatura.pais,
             plogItem->viatura.categoria,
             plogItem->viatura.nomeCondutor,
             plogItem->dataEntrada,
             plogItem->dataSaida);
}



/**
 * @brief  sd10_AguardaCheckout    Ler a descrição da tarefa SD10 no enunciado
 * @return Success  (TRUE or FALSE)
 */

void sd10_AcompanhaCliente(){
    so_debug("<");

    sd10_1_AguardaCheckout();    
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);
  
    so_debug(">");
}

void sd10_1_AguardaCheckout() {
    so_debug("<");

    pause();

    so_success("SD10.1", "SD: A viatura %s deseja sair do parque", logItem.viatura.matricula);
}
    


/**
 * @brief  sd10_2_EscreveLogSaidaViatura    Ler a descrição da tarefa SD10.2 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  posicaoLogfile (I) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  logItem (I) registo de Log para esta viatura
 * @return Success  (TRUE or FALSE)
 */
void sd10_2_EscreveLogSaidaViatura(char *logFilename, long posicaoLogfile, LogItem logItem) {
    so_debug("< [@param logFilename:%s, posicaoLogfile:%ld, logItem:[%s:%s:%c:%s:%s:%s]]",
                logFilename,
                posicaoLogfile,
                logItem.viatura.matricula,
                logItem.viatura.pais,
                logItem.viatura.categoria,
                logItem.viatura.nomeCondutor,
                logItem.dataEntrada,
                logItem.dataSaida);

    FILE *fLogfile = fopen(logFilename, "r+b");
    if (fLogfile == NULL){
        so_error("SD10.2", "Erro ao abrir logFile");
        sd11_EncerraServidorDedicado();
    }

    if(fseek(fLogfile, posicaoLogfile, SEEK_SET) != 0){
        so_error("SD10.2", "Erro ao endereçar logfile");
        fclose(fLogfile);
        sd11_EncerraServidorDedicado();
    }

    if(fwrite(&logItem, sizeof(LogItem), 1, fLogfile) != 1){
        so_error("SD10.2", "Não foi possível escrever no logfile");
        fclose(fLogfile);
        sd11_EncerraServidorDedicado();
    }

    fclose(fLogfile);

    so_success("SD10.2", "SD: Atualizei log na posição %ld: Saída Cliente %s em %s",
               posicaoLogfile,
               logItem.viatura.matricula,
               logItem.dataSaida);
    sd11_EncerraServidorDedicado();
    so_debug(">");

}

/**
 * @brief  sd11_EncerraServidorDedicado    Ler a descrição da tarefa SD11 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void sd11_EncerraServidorDedicado() {
    so_debug("<");

    so_error("Servidor", "O servidor nunca deveria ter chegado a este ponto!");
    so_debug(">");
}

/**
 * @brief  sd11_1_LibertaLugarViatura    Ler a descrição da tarefa SD11.1 no enunciado
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 * @return Success  (TRUE or FALSE)
 */
void sd11_1_LibertaLugarViatura(Estacionamento *lugaresEstacionamento, int indexClienteBD) {

    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);

    if(indexClienteBD < 0){
        so_error("SD11.1", "Índice do cliente inválido");
    }
        
    lugaresEstacionamento[indexClienteBD].pidServidorDedicado = DISPONIVEL;
    lugaresEstacionamento[indexClienteBD].pidCliente = DISPONIVEL;

    so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);
    so_debug(">");
}

/**
 * @brief  sd11_2_EnviaSighupAoClienteETerminaSD    Ler a descrição da tarefa SD11.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @return Success  (TRUE or FALSE)
 */
void sd11_2_EnviaSighupAoClienteETermina(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    int pidCliente = clientRequest.pidCliente;
    if(pidCliente <= 0){
        so_error("SD11.2", "PID do cliente inválido");
        return;
    }

    if(kill(pidCliente, SIGHUP) == -1){
        so_error("SD11.2", "Erro ao enviar sinal SIGHUP ao cliente");
    }
    
    so_success("SD11.2", "SD: Shutdown");
    exit(0);
    so_debug(">");
}

/**
 * @brief  sd12_TrataSigusr2    Ler a descrição da tarefa SD12 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd12_TrataSigusr2(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    if(sinalRecebido == SIGUSR2){
        so_success("SD12", "SD: Recebi pedido do Servidor para terminar");
        sd11_EncerraServidorDedicado();
    }    
    
    so_debug(">");
}

void sd13_TrataSigusr1(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    if(sinalRecebido == SIGUSR1){
        so_success("SD13", "SD: Recebi pedido do Cliente para terminar o estacionamento");
    }
    
    so_debug(">");
}