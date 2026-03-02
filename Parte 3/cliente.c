/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Aluno: Nº: 129838      Nome: José Miguel Oliveira Romeiro 
 ** Nome do Módulo: cliente.c
 ** Descrição/Explicação do Módulo:
 **
 ** Este módulo implementa o processo Cliente, que é responsável por fazer os pedidos para estacionar no parque.
 ** Para inicializar o Cliente, este abre a Message Queue (a mesma Message Queue do Servidor), para puder comunicar e enviar os seus pedidos ao servidor,
 ** assumindo assim que já existe, sendo um pouco mais simples na implementação (não é necessário IPC_EXCL nem IPC_CREAT, utiliza-se apenas 0). Arma
 ** todos os sinais a serem tratados pelo cliente (SIGINT e SIGALRM).
 ** De seguida, trata-se de fazer o check in, por isso o Cliente faz um "questionário" ao uitilizador a pedir as informações necessárias para o Servidor 
 ** realizar o seu estacionamento. Quando obtem todos os dados necessário este envia uma mensagem ao Servidor do type = MSGTYPE_LOGIN.
 ** Ativa uma pequena espera (alarme) enquanto agurda a resposta do Servidor.
 ** Se a mensagem recebida for do status = CLIENT_ACCEPTED, o Cliente dá sucesso. Caso seja status = ESTACIONAMENTO_TERMINADO, dá também sucesso, mas 
 ** não é possível estacionar. Desativa o alarme ativado atriormente.
 ** Enquanto o ultizadar não terminar o estacionamento (Ctrl-C), o Cliente fica à espera de receber uma mensagem relativa ao PID cliente. Se a mensagem lida
 ** status = INFO_TARIFA, o Cliente dá sucesso com o texto do campo da mensagem enviada pelo Servidor Dedicado. Se tiver status = ESTACIONAMENTO_TERMINADO
 ** termina o Cliente.
 ** Quando o utilizador quiser terminar o estacionamento, o Cliente envia uma mensagem ao Servidor Dedicado com status = TERMINA_ESTACIONAMENTO, retornando
 ** o funcionamento normal do cliente.
 ** Caso o cliente demorou mais tempo que o esperado a receber mensagem do Servidor este dá um erro de Time Out.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int msgId = -1;                         // Variável que tem o ID da Message Queue
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
int recebeuRespostaServidor = FALSE;    // Variável que determina se o Cliente já recebeu uma resposta do Servidor

/**
 * @brief Processamento do processo Cliente.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
int main () {
    so_debug("<");

    // c1_IniciaCliente:
    c1_1_GetMsgQueue(IPC_KEY, &msgId);
    c1_2_ArmaSinaisCliente();

    // c2_CheckinCliente:
    c2_1_InputEstacionamento(&clientRequest);
    c2_2_EscrevePedido(msgId, clientRequest);

    c3_ProgramaAlarme(MAX_ESPERA);

    // c4_AguardaRespostaServidor:
    c4_1_EsperaRespostaServidor(msgId, &clientRequest);
    c4_2_DesligaAlarme();

    c5_MainCliente(msgId, &clientRequest);

    so_error("Cliente", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief c1_1_GetMsgQueue Ler a descrição da tarefa C1.1 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void c1_1_GetMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

   int msgId = msgget(ipcKey, 0);
   if(msgId == -1) {
    so_error("C1.1", "Não foi possível abrir a messgage queue.");
    exit(1);
   }

   *pmsgId = msgId;

   so_success("C1.1", "Message queue aberta");
    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief c1_2_ArmaSinaisCliente Ler a descrição da tarefa C1.2 no enunciado
 */
void c1_2_ArmaSinaisCliente() {
    so_debug("<");

    if(signal(SIGINT, c6_TrataCtrlC) == SIG_ERR) {
        so_error("C1.2", "Não foi possível o sinal SIGUSR1.");
        exit(1);
    }
    if(signal(SIGALRM, c7_TrataAlarme) == SIG_ERR) {
        so_error("C1.2", "Não foi possível o sinal SIGUSR2.");
        exit(1);
    }

    so_success("C1.2", "Sinais armados.");
    so_debug(">");
}

/**
 * @brief c2_1_InputEstacionamento Ler a descrição da tarefa C2.1 no enunciado
 * @param pclientRequest (O) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_1_InputEstacionamento(MsgContent *pclientRequest) {
    so_debug("<");

    char input[100];
    char *pointer;

    printf ("Introduza a matrícula da viatura: ");
    so_gets(input, sizeof(input));
    pointer = input;
   
    while(*pointer == ' ') {
        pointer++;
    }
    
    if( *pointer == '\0') {
        so_error("C2.1", "A matrícula não pode ser vazia");
        exit(1);
    }

    strcpy(pclientRequest->msgData.est.viatura.matricula, input);

    printf ("Introduza a páis da viatura: ");
    so_gets (input, sizeof(input));
    pointer = input;
    while(*pointer == ' ') {
        pointer++;
    }
    if( *pointer == '\0') {
        so_error("C2.1", "O país não pode ser vazio");
        exit(1);
    }
    
    strcpy(pclientRequest->msgData.est.viatura.pais, input);

    printf ("Introduza a categoria da viatura: ");
    so_gets (input, sizeof(input));
    pointer = input;

    while(*pointer == ' ') {
        pointer++;
    }

    if(*pointer == '\0') {
        so_error("C2.1", "A categoria não pode ser vazia");
        exit(1);
    }

    pclientRequest->msgData.est.viatura.categoria = input[0];

    printf ("Introduza o nome do condutor da viatura: ");
    so_gets (input, sizeof(input));
    pointer = input;

    while(*pointer == ' ') {
        pointer++;
    }

    if( *pointer == '\0') {
        so_error("C2.1", "O nome do condutor não pode ser vazio");
        exit(1);
    }

    strcpy(pclientRequest->msgData.est.viatura.nomeCondutor, input);

    pclientRequest->msgData.est.pidCliente = getpid();
    pclientRequest->msgData.est.pidServidorDedicado = -1;

    so_success("C2.1", "%s %s %c %s %d %d", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief c2_2_EscrevePedido Ler a descrição da tarefa C2.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_2_EscrevePedido(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if(msgsnd(msgId, &clientRequest, sizeof(MsgContent)-sizeof(long), 0) == -1) {
        so_error("C2.2", "Não foi possível enviar a mensagem para o servidor.");
        exit(1);
    }
    
    so_success("C2.2", "Mensagem enviada");
    so_debug(">");
}

/**
 * @brief c3_ProgramaAlarme Ler a descrição da tarefa C3 no enunciado
 * @param segundos (I) número de segundos a programar no alarme
 */
void c3_ProgramaAlarme(int segundos) {
    so_debug("< [@param segundos:%d]", segundos);

    if(signal(SIGALRM, c7_TrataAlarme) == SIG_ERR) {
        so_error("C3", "Não foi possível armar o sinal SIGALRM.");
        exit(1);
    }

    alarm(segundos);

    so_success("C3", "Espera resposta em %d segundos", segundos);
    so_debug(">");
}

/**
 * @brief c4_1_EsperaRespostaServidor Ler a descrição da tarefa C4.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c4_1_EsperaRespostaServidor(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    if(msgrcv(msgId, pclientRequest, sizeof(MsgContent)-sizeof(long), pclientRequest->msgData.est.pidCliente, 0) == -1) {
        so_error("C4.1", "Não é possível estacionar.");
        exit(1);
    }

    if(pclientRequest->msgData.status == ESTACIONAMENTO_TERMINADO) {
        so_success("C4.1", "Não é possível estacionar.");
        exit(1);
    }
    
    so_success("C4.1", "Check-in realizado");
    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief c4_2_DesligaAlarme Ler a descrição da tarefa C4.2 no enunciado
 */
void c4_2_DesligaAlarme() {
    so_debug("<");

    alarm(0);
    so_success("C4.2", "Desliguei alarme");

    so_debug(">");
}

/**
 * @brief c5_MainCliente Ler a descrição da tarefa C5 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c5_MainCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    if(msgrcv(msgId, pclientRequest, sizeof(MsgContent)-sizeof(long), pclientRequest->msgData.est.pidCliente, 0) == -1) {
        so_error("C5", "Não foi possível receber a mensagem do servidor.");
        exit(1);
    }

    if(pclientRequest->msgData.status == INFO_TARIFA) {
        so_success("C5", "%s", pclientRequest->msgData.infoTarifa);
        c5_MainCliente(msgId, pclientRequest);
    }

    else if(pclientRequest->msgData.status == ESTACIONAMENTO_TERMINADO) {
        so_success("C5", "Estacionamento terminado.");
        exit(1);
    }

    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief  c6_TrataCtrlC Ler a descrição da tarefa C6 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c6_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d, msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", sinalRecebido, msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if(msgsnd(msgId, &clientRequest, sizeof(MsgContent)-sizeof(long), 0) == -1) {
        so_error("C6", "Erro ao enviar a mensagem para o servidor.");
        exit(1);
    }

    so_success("C6", "Cliente: Shutdown.");	
    so_debug(">");
}

/**
 * @brief  c7_TrataAlarme Ler a descrição da tarefa C7 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c7_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_error("C7", "Cliente: Timeout");
    exit(0);
    so_debug(">");
}