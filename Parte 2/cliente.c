/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2024/2025, Enunciado Versão 2+
 **
 ** Aluno: Nº: 129838      Nome: José Miguel Oliveira Romeiro
 ** Nome do Módulo: cliente.c
 ** Descrição/Explicação do Módulo:
 ** Neste módulo é implementado o processo cliente do sistema de estacionamento.
 ** Para iniciar o processo cliente, valida-se a existência do ficheiro server.fifo e arma-se todos os sinais necessários.
 ** De seguida, trata-se do check-in do cliente, isto é, pede-se ao cliente para introduzir os dados e verifica-se se estão corretos. Escreve em binário.
 ** Define-se um tempo de espera para o cliente e aguarda-se a resposta acerca do sucesso ou não do check-in. Se for sucesso, desarma-se o alarme e pede ao utilizador para ecrever "sair", caso deseja sair.
 ** Se o cliente escrever, começa-se a tratar do término do cliente, enviando um sinal ao servidor dedicado, aguardando a resposta.
 ** Por fim termna-se o cliente.
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"

/*** Variáveis Globais ***/
Estacionamento clientRequest;           // Pedido enviado do Cliente para o Servidor
int recebeuRespostaServidor = FALSE;
FILE *fFifoServidor;
char temp[100];
char input[100];   

/**
 * @brief Processamento do processo Cliente
 */


 void c1_IniciaCliente();
 void c1_1_ValidaFifoServidor(char *filename);
 void c1_2_ArmaSinaisCliente();
 void c2_CheckinCliente();
 void c2_1_InputEstacionamento(Estacionamento *clientRequest);
 void c2_2_AbreFifoServidor(char *fifoName, FILE **fFifoServidor);
 void c2_3_EscrevePedido(FILE *fFifoServidor, Estacionamento clientRequest);
 void c3_ProgramaAlarme(int seconds);
 void c4_EsperaRespostaServidor();
 void c4_1_EsperaRespostaServidor();
 void c4_2_DesligaAlarme();
 void c4_3_InputEsperaCheckout();
 void c5_EncerraCliente();
 void c5_1_EnviaSigusr1AoServidor(Estacionamento clientRequest);
 void c5_2_EsperaRespostaServidorETermina();

int main () {
    c1_IniciaCliente();
    c2_CheckinCliente();
    c2_1_InputEstacionamento(&clientRequest);
    FILE *fFifoServidor;
    c2_2_AbreFifoServidor(FILE_REQUESTS, &fFifoServidor);
    c2_3_EscrevePedido(fFifoServidor, clientRequest);
    c3_ProgramaAlarme(MAX_ESPERA);
    c4_EsperaRespostaServidor();
    c4_1_EsperaRespostaServidor();
    c4_2_DesligaAlarme();
    c4_3_InputEsperaCheckout();
    c5_EncerraCliente();
    c5_1_EnviaSigusr1AoServidor(clientRequest);
    c5_2_EsperaRespostaServidorETermina();
}

/**
 * @brief  c1_IniciaCliente    Ler a descrição da tarefa C1 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void c1_IniciaCliente() {

    so_debug("<");

    c1_1_ValidaFifoServidor(FILE_REQUESTS);
    c1_2_ArmaSinaisCliente();


    so_debug(">");

}

/**
 * @brief  c1_1_ValidaFifoServidor      Ler a descrição da tarefa C1.1 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @return Success  (TRUE or FALSE)
 */
void c1_1_ValidaFifoServidor(char *filenameFifoServidor) {

    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    struct stat fifo;

    if(stat(filenameFifoServidor, &fifo) == 0 && S_ISFIFO(fifo.st_mode)) {
        so_success("C1.1", "FIFO %s existe", filenameFifoServidor);
    }
    else {
        so_error("C1.1", "FIFO %s não existe", filenameFifoServidor);
        exit(1);
    }

    so_debug(">");
}

/**
 * @brief  c1_3_ArmaSinaisCliente      Ler a descrição da tarefa C1.3 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void c1_2_ArmaSinaisCliente() {
    so_debug("<");
    
    struct sigaction action;

    action.sa_sigaction = c6_TrataSigusr1;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGUSR1, &action, NULL) == -1) {
        so_error("C1.2", "Não foi possível o sinal SIGUSR1");
        exit(1);
    }
    if(signal(SIGHUP, c7_TrataSighup) == SIG_ERR) {
        so_error("C1.2", "Check in sem sucesso ou estacionamento acabou");
        exit(1);
    }
    if(signal(SIGINT, c8_TrataCtrlC) == SIG_ERR) {
        so_error("C1.2", "Não foi possível armar o CTRL-C");
        exit(1);
    }
    if(signal(SIGALRM, c9_TrataAlarme) == SIG_ERR) {
        so_error("C1.2", "Não foi possível armar o alarme");
        exit(1);
    }

    so_success("C1.2", "Check-in confimado!");
    so_debug(">");
}

/**
 * @brief  c2_CheckinCliente      Ler a descrição da tarefa C2 no enunciado
 * @return Success  (TRUE or FALSE)
 */

void c2_CheckinCliente() {
    so_debug("<");

    c2_1_InputEstacionamento(&clientRequest);
    FILE *fFifoServidor;
    c2_2_AbreFifoServidor(FILE_REQUESTS, &fFifoServidor);
    c2_3_EscrevePedido(fFifoServidor, clientRequest);

    so_debug(">");
}

/**
 * @brief  c2_1_InputEstacionamento      Ler a descrição da tarefa C2.1 no enunciado
 * @param  pclientRequest (O) pedido a ser enviado por este Cliente ao Servidor
 * @return Success  (TRUE or FALSE)
 */
void c2_1_InputEstacionamento(Estacionamento *pclientRequest) {
    so_debug("<");

    char buffer[100];
    char *pointer;

    printf ("Introduza a matrícula da viatura: ");
    so_gets(buffer, sizeof(buffer));
    pointer = buffer;
   
    while(*pointer == ' ') {
        pointer++;
    }
    
    if( *pointer == '\0') {
        so_error("C2.1", "A matrícula não pode ser vazia");
        exit(1);
    }

    strcpy((*pclientRequest).viatura.matricula, buffer);

    printf ("Introduza a páis da viatura: ");
    so_gets (buffer, sizeof(buffer));
    pointer = buffer;
    while(*pointer == ' ') {
        pointer++;
    }
    if( *pointer == '\0') {
        so_error("C2.1", "O país não pode ser vazio");
        exit(1);
    }
    
    strcpy((*pclientRequest).viatura.pais, buffer);

    printf ("Introduza a categoria da viatura: ");
    so_gets (buffer, sizeof(buffer));
    pointer = buffer;

    while(*pointer == ' ') {
        pointer++;
    }

    if(*pointer == '\0') {
        so_error("C2.1", "A categoria não pode ser vazia");
        exit(1);
    }

    (*pclientRequest).viatura.categoria = buffer[0];

    printf ("Introduza o nome do condutor da viatura: ");
    so_gets (buffer, sizeof(buffer));
    pointer = buffer;

    while(*pointer == ' ') {
        pointer++;
    }

    if( *pointer == '\0') {
        so_error("C2.1", "O nome do condutor não pode ser vazio");
        exit(1);
    }

    strcpy((*pclientRequest).viatura.nomeCondutor, buffer);

    (*pclientRequest).pidCliente = getpid();
    (*pclientRequest).pidServidorDedicado = -1;

    so_success("C2.1", "%s %s %c %s %d %d", (*pclientRequest).viatura.matricula, (*pclientRequest).viatura.pais, (*pclientRequest).viatura.categoria, (*pclientRequest).viatura.nomeCondutor, (*pclientRequest).pidCliente, (*pclientRequest).pidServidorDedicado);
    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->viatura.matricula, pclientRequest->viatura.pais, pclientRequest->viatura.categoria, pclientRequest->viatura.nomeCondutor, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
}

/**
 * @brief  c2_2_AbreFifoServidor      Ler a descrição da tarefa C2.2 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @param  pfFifoServidor (O) descritor aberto do ficheiro do FIFO do servidor
 * @return Success  (TRUE or FALSE)
 */

void c2_2_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    *pfFifoServidor = fopen(filenameFifoServidor, "wb");
    if(*pfFifoServidor == NULL) {
        so_error("C2.2", "Não foi possível abrir FIFO %s", filenameFifoServidor);
        exit(1);
    }
    
    so_success("C2.2", "FIFO %s aberto", filenameFifoServidor);
    so_debug("> [*pfFifoServidor:%p]", *pfFifoServidor);
}

/**
 * @brief  c2_3_EscrevePedido      Ler a descrição da tarefa C2.3 no enunciado
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 * @param  clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 * @return Success  (TRUE or FALSE)
 */
void c2_3_EscrevePedido(FILE *fFifoServidor, Estacionamento clientRequest) {
    so_debug("< [@param fFifoServidor:%p, clientRequest:[%s:%s:%c:%s:%d:%d]]", fFifoServidor, clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if(fFifoServidor == NULL) {
        so_error("C2.3", "FIFO %s não está aberto", FILE_REQUESTS);
        exit(1);
    }

    size_t written = fwrite(&clientRequest, sizeof(Estacionamento), 1, fFifoServidor);
    if(written != 1) {
        so_error("C2.3", "Não foi possível escrever no FIFO");
        exit(1);
    }

    fflush(fFifoServidor);

    so_success("C2.3", "Pedido escrito no FIFO");
    so_debug(">");
}

/**
 * @brief  c3_ProgramaAlarme      Ler a descrição da tarefa C3 no enunciado
 * @param  segundos (I) número de segundos a programar no alarme
 * @return Success  (TRUE or FALSE)
 */
void c3_ProgramaAlarme(int segundos) {
    so_debug("< [@param segundos:%d]", segundos);

    if(signal(SIGALRM, c9_TrataAlarme) == SIG_ERR) {
        so_error("C3", "Não foi possível armar o alarme");
        exit(1);
    }

    alarm(segundos);

    so_success("C3", "Espera resposta em %d segundos", segundos);
    so_debug(">");
}

/**
 * @brief  c4_EsperaRespostaServidor      Ler a descrição da tarefa C4 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void c4_EsperaRespostaServidor() {
    so_debug("<");

    c4_1_EsperaRespostaServidor();
    c4_2_DesligaAlarme();
    c4_3_InputEsperaCheckout();

    so_success("c4", "Resposta do servidor recebido");
    so_debug(">");
}

/**
 * @brief  c4_1_DesligaAlarme      Ler a descrição da tarefa C4.1 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void c4_1_EsperaRespostaServidor(){
    so_debug("<");

    struct sigaction action;
    action.sa_sigaction = c6_TrataSigusr1;
    action.sa_flags = SA_SIGINFO;
    
   if(signal(SIGHUP, c7_TrataSighup) == SIG_ERR) {
        so_error("C4.1", "Não foi possível armar o sinal SIGHUP");
        exit(1);
    }
    else if(sigaction(SIGUSR1, &action, NULL) == -1) {
        so_error("C4.1", "Não foi possível armar o sinal SIGUSR1");
        exit(1);
    }

    pause();

    so_success("C4.1", "Check-in realizado com sucesso");
    so_debug(">");
}

/**
 * @brief  c4_2_InputEsperaCheckout      Ler a descrição da tarefa C4.2 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void c4_2_DesligaAlarme() {
    so_debug("<");

    alarm(0);

    so_success("C4.2", "Desliguei alarme");
    so_debug(">");
}

void c4_3_InputEsperaCheckout(){
    so_debug("<");

    char leave[100];
    while(1){
        printf("Escreva 'sair' para sair do estacionamento: ");
        so_gets(leave, sizeof(leave));

        if(strcmp(leave, "sair") == 0) {
            so_success("C4.3", "Utilizador pretende terminar estacionamento");
            c5_EncerraCliente();
            break;
        }
        else {
            so_error("C4.3", "Entrada inválida");
        }
    }

    so_debug(">");
}

/**
 * @brief  c5_EncerraCliente      Ler a descrição da tarefa C5 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void c5_EncerraCliente() {
    so_debug("<");

    
    so_error("c5", "O programa nunca deveria ter chegado a este ponto");
    so_debug(">");
}

/**
 * @brief  c5_1_EnviaSigusr1AoServidor      Ler a descrição da tarefa C5.1 no enunciado
 * @param  clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 * @return Success  (TRUE or FALSE)
 */
void c5_1_EnviaSigusr1AoServidor(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    int servidorDedicado = clientRequest.pidServidorDedicado;
    if(servidorDedicado <=0) {
        so_error("C5.1", "PID do servidor dedicado inválido");
        exit(1);
    }

    if(kill(servidorDedicado, SIGUSR1) == -1) {
        so_error("C5.1", "Não foi possível enviar SIGUSR1 para o servidor  %d", servidorDedicado);
        exit(1);
    }

    so_success("C5.1", "SIGUSR1 enviado ao servidor %d", servidorDedicado);
    so_debug(">");
}

/**
 * @brief  c5_2_EsperaRespostaServidorETermina      Ler a descrição da tarefa C5.2 no enunciado
 * @return Success  (TRUE or FALSE)
 */
void c5_2_EsperaRespostaServidorETermina() {
    so_debug("<");

    if(signal(SIGHUP, c7_TrataSighup) == SIG_ERR) {
        so_error("C5.2", "Não foi possível armar o sinal SIGHUP");
        exit(1);
    }

    pause();

    so_success("C5.2", "Cliente terminado");
    exit(0);
    so_debug(">");
}

/**
 * @brief  c6_TrataSigusr1      Ler a descrição da tarefa C6 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c6_TrataSigusr1(int sinalRecebido, siginfo_t *siginfo, void *context) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    pid_t servidorDedicado = (*siginfo).si_pid;

    clientRequest.pidServidorDedicado = servidorDedicado;
    
    so_success("C6", "Check-in concluído com sucesso pelo Servidor Dedicado %d", servidorDedicado);
    so_debug(">");
}

/**
 * @brief  c7_TrataSighup      Ler a descrição da tarefa C7 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c7_TrataSighup(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("C7", "Estacionamento terminado");
    exit(0);
    so_debug(">");
}

/**
 * @brief  c8_TrataCtrlC      Ler a descrição da tarefa c8 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c8_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("C8", "Cliente: Shutdown");
    c5_EncerraCliente();
    so_debug(">");
}

/**
 * @brief  c9_TrataAlarme      Ler a descrição da tarefa c9 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 * @return Success  (TRUE or FALSE)
 */
void c9_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_error("C9", "Cliente: Timeout");
    exit(0);
    so_debug(">");
}