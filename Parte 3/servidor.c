/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Aluno: Nº: 129838      Nome: José Miguel Oliveira Romeiro
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo:
 **
 ** Este módulo implementa o Servidor(pai) e os Servidores Dedicados(filhos).
 ** Para tal, começa se com a inicialização do Servidor, que cria uma Message Queue, para puder comunicar com os Clientes.
 ** Cria também um Grupo de semáforos, que é usado para controlar o acesso à BD, para controlar o acesso ao logFile, 
 ** para controlar a espera dos Servidores Dedicados que querem terminar, para controlar o número de lugares livres no parque
 **  e por fim para controlar o acesso à Shared Memory com a variável tarifa atual. Para terminar a inicialização,
 ** o Servidor acede/cria à Shared Memory reservada para o utilizador (no meu caso para o endereço 0xa129838) e inicia o parque de estacionamento.
 ** Depois de já termos tudo inicializado, o servidor fica à  espera de recber pedidos (pela Message Queue) dos Clientes e caso receba, cria
 ** um Servidor Dedicado, que toma conta do pedido do cliente, enquanto o servidor fica à espera de mais pedidos.
 ** O servidor trata também de, no caso de o dono do parque querer terminar o Servidor, terminar o servidor após receber o Sinal vindo do Ctrl-C (SIGINT).
 ** Caso receba esse sinal, inicia-se o encerramento do Servidor. Como é normal, quando o servidor pai morre, os filhos ficam orfãos, sendo adotados pelo INIT.
 ** Por isso, eles continuam a correr até terminarem o seu trabalho. Enquanto não terminam, o Servidor percorre todos os lugares do parque à procura de 
 ** Servidores Dedicados e encia-lhes o sinal SIGUSR2, para que eles terminem o seu trabalho. Para garantir que não há nenhuma alteração na base de dados,
 ** ou seja, que haja algum servidor dedicado que ainda fique por lá ou se tire demasiados, utiliza-se um semáforo SRV_DEDICADOS, que é decrementado para 0,
 ** garantindo que todos os Servidores Dedicados terminam antes de o Servidor terminar.
 ** Por fim, o Servidor apaga todos os elementos de IPC que criou / utlizou, como a Shared Memory, a Message Queue e o Grupo de Semáforos.
 ** Cada vez que um Servidor Dedicado termina, o Servidor recebe o sinal SIGCHLD, que é tratado por uma função que decrementa o número de Servidores Dedicados 
 ** que ainda estão a correr e envia uma mensagem de sucesso para o log, indicando que o Servidor Dedicado terminou.
 **
 ** Agora entra a parte dos Servidores Dedicados, que são os filhos do Servidor.
 ** Para inicializar o Servidor Dedicado, ele arma todos os Sinais necessários para o seu funcionamento como o SIGUSR2, que é usado para terminar o Servidor Dedicado,
 ** o SIGCHLD, que é usado para indicar ao Servidor que um Servidor Dedicado terminou e o SIGINT, que é ignorado, para não interferir com o Servidor. Verifica se o
 ** pedido recebido pelo Cliente é válido, liga-se à Shared Memory já existente da Face e ao grupo de semáforos da Face, que apenas tem um semáforo (MUTEX_FACE).
 ** Para fechar a inicialização, o Servidor Dedicado fica à espera de um lugar livre no parque (utilizando os semáforos LUGARES_PARQUE e MUTEX_BD), que é o lugar
 ** onde o Cliente vai estacionar. Esta implementação é muito melhor e mais fácil de implementar do que a do parte-2, porque com os semáforos, basta apenas tentar
 ** decrementar o semáforo LUGARES_PARQUE, que caso seja possível sem dar bloqueio (decrementação faz com que o semáforo tenha valor negativo), apenas reserva-se
 ** o lugar. caso dê bloqueio, o Servidor Dedicado espera que haja uma incrementação no semáforo LUGARES_PARQUE, terminando assim o bloqueio, que indica que há um lugar livre.
 ** De seguida, trata de verificar a validade do pedido do Cliente, verificando a matrícula, o país, a categoria e o nome do condutor.
 ** Depois de tudo validado, o Servidor Dedicado adormece um tempo para simular que o Cliente está realmente a estancionar o carro, enviando uma mensagem de sucesso 
 ** ao Cliente, indicando que o pedido foi aceite. Por fim, escreve no logFile a entrada do Cliente, indicando que o Cliente estacionou o carro e aguarda pelo Checkout do Cliente.
 ** O Servidor Dedicado dica à espera de receber uma mensagem do Cliente, mas a única que lhe imoporta é a de CheckOut (status = TERMINA_CLIENTE). Enquanto não recebe esta mensagem,
 ** o Servidor Dedicado envia de minuto em minuto uma mensagem ao Cliente com informações à cerca da tarifa atual, através da MSG. Para terminar, escreve o log da saída da viatura.
 ** Para terminar o Servidor Dedicado, este vai  incrementar o semáforo LUGARES_PARQUE, indicando que surgiu uma vaga no paruqe, caso esteja cheio, desbloqueando, possivelmente, 
 ** o semáforo LUGARES_PARQUE. Liberta também o lugares ocupado na base de dados. Por fim, envia uma maensagem ao Cliente (status = ESTACIONAMENTO_TERMINADO) a indicar que o 
 ** Servidor Dedicado terminou e termina o processo.
 ** Quero realçar a importancia do MutEx, que evita a entrada de vários processos na zona crítica.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int nrServidoresDedicados = 0;          // Número de servidores dedicados (só faz sentido no processo Servidor)
int shmId = -1;                         // Variável que tem o ID da Shared Memory
int msgId = -1;                         // Variável que tem o ID da Message Queue
int semId = -1;                         // Variável que tem o ID do Grupo de Semáforos
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento = NULL;   // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD = -1;                // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile = -1;               // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile
int shmIdFACE = -1;                     // Variável que tem o ID da Shared Memory da entidade externa FACE
int semIdFACE = -1;                     // Variável que tem o ID do Grupo de Semáforos da entidade externa FACE
int *tarifaAtual = NULL;                // Inteiro definido pela entidade externa FACE com a tarifa atual do parque

/**
 * @brief  Processamento do processo Servidor e dos processos Servidor Dedicado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 * @return Success (0) or not (<> 0)
 */

void sem_wait(int semId, int SEM_NAME, int FLAG) {
    struct sembuf op = {
        .sem_num = SEM_NAME,
        .sem_op = -1,
        .sem_flg = FLAG
    };
    if(semop(semId, &op, 1) == -1){
        so_error("sem_wait", "Erro a decrementar o semáforo.");
        exit(1);
    }
}

void sem_signal(int semId, int SEM_NAME, int FLAG) {
    struct sembuf op = {
        .sem_num = SEM_NAME,
        .sem_op = 1,
        .sem_flg = FLAG
    };
    if(semop(semId, &op, 1) == -1){
        so_error("sem_signal", "Erro a incrementar o semáforo.");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    so_debug("<");

    s1_IniciaServidor(argc, argv);
    s2_MainServidor();

    so_error("Servidor", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief s1_iniciaServidor Ler a descrição da tarefa S1 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 */
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");

    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);
    s1_2_ArmaSinaisServidor();
    s1_3_CriaMsgQueue(IPC_KEY, &msgId);
    s1_4_CriaGrupoSemaforos(IPC_KEY, &semId);
    s1_5_CriaBD(IPC_KEY, &shmId, dimensaoMaximaParque, &lugaresEstacionamento);

    so_debug(">");
}

/**
 * @brief s1_1_ObtemDimensaoParque Ler a descrição da tarefa S1.1 no enunciado
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 * @param pdimensaoMaximaParque (O) número máximo de lugares do parque, especificado pelo utilizador
 */
void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque) {
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);

    if(argc != 2) {
        so_error("S1.1", "Número de argumentos inválido");
        exit(1);
    }

    char *arg = argv[1];
    int i = 0;
    *pdimensaoMaximaParque = 0;
    
    while(arg[i] != '\0') {
        if(arg[i] < '0' || arg[i] > '9') {
            so_error("S1.1", "Argumento inválido");
            exit(1);
        }
        *pdimensaoMaximaParque = (*pdimensaoMaximaParque * 10) + (arg[i] - '0');
        i++;
    }

    if( *pdimensaoMaximaParque <= 0 ){
        so_error("S1.1", "O ponteiro para a dimensão máxima do parque é nulo");
        exit(1);
    }

    so_success("S1.1", "O argumento recebido foi aceite.");
    so_debug("> [@return *pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
}

/**
 * @brief s1_2_ArmaSinaisServidor Ler a descrição da tarefa S1.2 no enunciado
 */
void s1_2_ArmaSinaisServidor() {
    so_debug("<");

    if(signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar o sinal SIGINT");
        exit(1);
    }
    if(signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar o sinal SIGUSR2");
        exit(1);
    }

    so_success("S1.2", "Os sinais foram armados com sucesso.");
    so_debug(">");
}

/**
 * @brief s1_3_CriaMsgQueue Ler a descrição da tarefa s1.3 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void s1_3_CriaMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    int msgExistente = msgget( ipcKey, 0666 );
    if( msgExistente != -1 ) {
        if(msgctl( msgExistente, IPC_RMID, 0 ) == -1 ){
            so_error("S1.3", "Erro ao remover a Message Queue existente.");
            exit(1);
        }
    }
    msgExistente = msgget( ipcKey, IPC_CREAT | IPC_EXCL | 0666 );
    if( msgExistente == -1 ) {
        so_error("S1.3", "Erro ao criar a Message Queue.");
        exit(1);
    }
    
    so_success("S1.3", "A Message Queue foi criada com sucesso.");
    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief s1_4_CriaGrupoSemaforos Ler a descrição da tarefa s1.4 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param psemId (O) identificador aberto de IPC
 */
void s1_4_CriaGrupoSemaforos(key_t ipcKey, int *psemId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    int semExistente = semget(ipcKey, 4, 0666);
    if(semExistente != -1) {
        if(semctl(semExistente, 0, IPC_RMID) == -1 ){
            so_error("S1.4", "Erro ao remover o semáforo existente.");
            exit(1);
        }
        so_success("S1.4", "O semáforo foi removido com sucesso.");
    }   

    semExistente = semget(ipcKey, 4, IPC_CREAT | IPC_EXCL | 0666);
    if(semExistente == -1 ) {
        so_error("S1.4", "Erro ao criar o grupo de semáforos.");
        exit(1);
    }

    int erro = 1;
    if(semctl(semExistente, SEM_MUTEX_BD, SETVAL, 1) == -1){
        erro = 0;
    }
    if(semctl(semExistente, SEM_MUTEX_LOGFILE, SETVAL, 1) == -1){
        erro = 0;
    }
    if(semctl(semExistente, SEM_SRV_DEDICADOS, SETVAL, 0) == -1){
        erro = 0;
    }
    if(semctl(semExistente, SEM_LUGARES_PARQUE, SETVAL, dimensaoMaximaParque) == -1) {
        erro = 0;
    }

    if(!erro) {
        so_error("S1.4", "Erro ao inicializar o grupo de semáforos.");
        exit(1);
    }

    *psemId = semExistente;

    so_success("S1.4", "O grupo de semáforos foi criado.");
    so_debug("> [@return *psemId:%d]", *psemId);
}

/**
 * @brief s1_5_CriaBD Ler a descrição da tarefa S1.5 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pshmId (O) identificador aberto de IPC
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
 */
void s1_5_CriaBD(key_t ipcKey, int *pshmId, int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    so_debug("< [@param ipcKey:0x0%x, dimensaoMaximaParque:%d]", ipcKey, dimensaoMaximaParque);

        int shmSize = sizeof(Estacionamento) * dimensaoMaximaParque;
        *pshmId = shmget(ipcKey, shmSize, 0666);
        if( *pshmId != -1){
            *plugaresEstacionamento = (Estacionamento *) shmat(*pshmId, NULL, 0);
            if(*plugaresEstacionamento == (void*) -1){
                so_error("S1.5", "Erro ao dar attach ao shared memory.");
                exit(1);
            }
            so_success("S1.5", "Shared memory acedido com sucesso.");
        } else {
            *pshmId = shmget(ipcKey, shmSize, IPC_CREAT | 0666);
            if(*pshmId == -1){
                so_error("S1.5", "Erro ao criar o shared memory.");
                exit(1);
            }
            *plugaresEstacionamento = (Estacionamento *) shmat(*pshmId, NULL, 0);
            if(*plugaresEstacionamento == (void*) -1){
                so_error("S1.5", "Erro ao dar attach ao shared memory.");
                exit(1);
            }
            for(int i = 0; i < dimensaoMaximaParque; i++){
                (*plugaresEstacionamento)[i].pidCliente = DISPONIVEL;
            }
            so_success("S1.5", "Shared memory criada com sucesso.");
        }
    
    so_debug("> [@return *pshmId:%d, *plugaresEstacionamento:%p]", *pshmId, *plugaresEstacionamento);
}

/**
 * @brief s2_MainServidor Ler a descrição da tarefa S2 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s2_MainServidor() {
    so_debug("<");

    while (TRUE) {
        s2_1_LePedidoCliente(msgId, &clientRequest);
        s2_2_CriaServidorDedicado(&nrServidoresDedicados);
    }

    so_debug(">");
}

/**
 * @brief s2_1_LePedidoCliente Ler a descrição da tarefa S2.1 no enunciado.
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) pedido recebido, enviado por um Cliente
 */
void s2_1_LePedidoCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    if(msgrcv(msgId, pclientRequest, sizeof(MsgContent)-sizeof(long), MSGTYPE_LOGIN, 0) == -1){
        so_error("S2.1", "Erro ao receber a mensagem.");
        s4_EncerraServidor();
    }

    //sleep(1);  // TEMPORÁRIO, os alunos deverão comentar este statement apenas
                // depois de terem a certeza que não terão uma espera ativa
    so_success ("S2.1", "%s %d" , pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.pidCliente);
    so_debug("> [@return *pclientRequest:[%s:%s:%c:%s:%d.%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief s2_2_CriaServidorDedicado Ler a descrição da tarefa S2.2 no enunciado
 * @param pnrServidoresDedicados (O) número de Servidores Dedicados que foram criados até então
 */
void s2_2_CriaServidorDedicado(int *pnrServidoresDedicados) {
    so_debug("<");

    pid_t pidServidorDedicado = fork();
    if(pidServidorDedicado == -1){
        so_error("S2.2", "Não foi possível criar o Servidor Dedicado");
        s4_EncerraServidor();
    }

    if(pidServidorDedicado == 0){
        so_success("S2.2", "SD: Nasci com PID %d", getpid());
        sd7_MainServidorDedicado();
    }

    if(pidServidorDedicado > 0){
        so_success("S2.2", "Servidor: Iniciei SD %d", pidServidorDedicado);
    }


    so_debug("> [@return *pnrServidoresDedicados:%d", *pnrServidoresDedicados);
}

/**
 * @brief s3_TrataCtrlC Ler a descrição da tarefa S3 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s3_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

   if(sinalRecebido == SIGINT) {
    so_success("S3", "Servidor: Start Shutdown");
    s4_EncerraServidor();
   }

    so_debug(">");
}

/**
 * @brief s4_EncerraServidor Ler a descrição da tarefa S4 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s4_EncerraServidor() {
    so_debug("<");

    s4_1_TerminaServidoresDedicados(lugaresEstacionamento, dimensaoMaximaParque);
    s4_2_AguardaFimServidoresDedicados(nrServidoresDedicados);
    s4_3_ApagaElementosIPCeTermina(shmId, semId, msgId);

    so_debug(">");
}

/**
 * @brief s4_1_TerminaServidoresDedicados Ler a descrição da tarefa S4.1 no enunciado
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 */
void s4_1_TerminaServidoresDedicados(Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque) {
    so_debug("< [@param lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", lugaresEstacionamento, dimensaoMaximaParque);
    
    sem_wait(semId, SEM_MUTEX_BD, 0);

    for(int i = 0; i < dimensaoMaximaParque; i++){
        if( lugaresEstacionamento[i].pidServidorDedicado != DISPONIVEL ){
            if(kill(lugaresEstacionamento[i].pidServidorDedicado, SIGUSR2) == -1){
                so_error("S4.1", "Não foi possível enviar SIGUSR2.");
            }
        }
    }

    sem_signal(semId, SEM_MUTEX_BD, 0);

    so_success("S4.1", "Sinais enviados com sucesso.");
    so_debug(">");
}

/**
 * @brief s4_2_AguardaFimServidoresDedicados Ler a descrição da tarefa S4.2 no enunciado
 * @param nrServidoresDedicados (I) número de Servidores Dedicados que foram criados até então
 */
void s4_2_AguardaFimServidoresDedicados(int nrServidoresDedicados) {
    so_debug("< [@param nrServidoresDedicados:%d]", nrServidoresDedicados);

    if(nrServidoresDedicados <= 0) {
        so_success("S4.2", "Não existem Servidores Dedicados a quererem terminar.");
        return;
    }

    struct sembuf op_barreira = {
        .sem_num = SEM_SRV_DEDICADOS,
        .sem_op = -nrServidoresDedicados,
        .sem_flg = 0
    };
    if(semop(semId, &op_barreira, 1) == -1){
        so_error("S4.2", "Não foi possível decrementar no semáforo SRV_DEDICADOS");
    }

    so_success("S4.2", "Aguardei o fim de %d Servidores Dedicados.", nrServidoresDedicados);
    so_debug(">");
}

/**
 * @brief s4_3_ApagaElementosIPCeTermina Ler a descrição da tarefa S4.2 no enunciado
 * @param shmId (I) identificador aberto de IPC
 * @param semId (I) identificador aberto de IPC
 * @param msgId (I) identificador aberto de IPC
 */
void s4_3_ApagaElementosIPCeTermina(int shmId, int semId, int msgId) {
    so_debug("< [@param shmId:%d, semId:%d, msgId:%d]", shmId, semId, msgId);

    if(shmctl(shmId, IPC_RMID, NULL) == -1){
        so_error("S4.3", "Não foi possível remover shm");
        exit(1);
    }
    if(semctl(semId, 0, IPC_RMID) == -1){
        so_error("S4.3", "Não foi possível remover sem");
        exit(1);
    }
    if(msgctl(msgId, IPC_RMID, NULL) == -1){
        so_error("S4.3", "Não foi possível remover msg");
        exit(1);
    }

    so_success("S4.3", "Servidor: End Shutdown");
    exit(0);
    so_debug(">");
}

/**
 * @brief s5_TrataTerminouServidorDedicado Ler a descrição da tarefa S5 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    int servidorDedicado;
    pid_t pidTerminou = wait(&servidorDedicado);

    if(sinalRecebido = SIGCHLD) {
        so_success("S5", "Servidor: Confirmo que terminou o SD %d", pidTerminou);
    } else {
        so_error("S5", "Servidor: Sinal recebido não é SIGCHLD, mas sim %d", sinalRecebido);
        exit(1);
    }

    nrServidoresDedicados--;

    so_debug("> [@return nrServidoresDedicados:%d", nrServidoresDedicados);
}

/**
 * @brief sd7_ServidorDedicado Ler a descrição da tarefa SD7 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd7_MainServidorDedicado() {
    so_debug("<");

    // sd7_IniciaServidorDedicado:
    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_GetShmFACE(KEY_FACE, &shmIdFACE);
    sd7_4_GetSemFACE(KEY_FACE, &semIdFACE);
    sd7_5_ProcuraLugarDisponivelBD(semId, clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);

    // sd8_ValidaPedidoCliente:
    sd8_1_ValidaMatricula(clientRequest);
    sd8_2_ValidaPais(clientRequest);
    sd8_3_ValidaCategoria(clientRequest);
    sd8_4_ValidaNomeCondutor(clientRequest);

    // sd9_EntradaCliente:
    sd9_1_AdormeceTempoRandom();
    sd9_2_EnviaSucessoAoCliente(msgId, clientRequest);
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    // sd10_AcompanhaCliente:
    sd10_1_AguardaCheckout(msgId);
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);

    sd11_EncerraServidorDedicado();

    so_error("Servidor Dedicado", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
}

/**
 * @brief sd7_1_ArmaSinaisServidorDedicado Ler a descrição da tarefa SD7.1 no enunciado
 */
    void sd7_1_ArmaSinaisServidorDedicado() {
        so_debug("<");

        if(signal(SIGUSR2, sd12_TrataSigusr2) == SIG_ERR){
            so_error("SD7.1", "Não foi possível armar SIGUSR2!");
            exit(1);
        }

        if(signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR){
            so_error("SD7.1", "Não foi possível armar SIGCHLD!");
            exit(1);
        }

        if(signal(SIGINT, SIG_IGN) == SIG_ERR){
            so_error("SD7.1", "Não foi possível armar SIGINT!");
            exit(1);
        }
        
        so_success("SD7.1", "Sinais armados com sucesso.");
        so_debug(">");
    }

/**
 * @brief sd7_2_ValidaPidCliente Ler a descrição da tarefa SD7.2 no enunciado
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd7_2_ValidaPidCliente(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if(clientRequest.msgData.est.pidCliente <= 0) {
        so_error("SD7.2", "PID do cliente inválido.");
        exit(1);
    }

    so_success("SD7.2", "PID do cliente válido.");
    so_debug(">");
}

/**
 * @brief sd7_3_GetShmFACE Ler a descrição da tarefa SD7.3 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param pshmIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_3_GetShmFACE(key_t ipcKeyFace, int *pshmIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);

    int shmExistente = shmget(ipcKeyFace, sizeof(int), 0666);
    if(shmExistente == -1) {
        so_error("SD7.3", "Não foi possível aceder à shared memory da FACE");
        exit(1);
    }

    void *shmAddr = shmat(shmExistente, NULL,0);
    if(shmAddr == (void*) -1){
        so_error("SD7.3", "Não foi possível realizar o attach da shared memory da FACE");
        exit(1);
    }

    *pshmIdFACE = shmExistente;

    so_success("SD7.3", "Shared memory da FACE acedido");
    so_debug("> [@return *pshmIdFACE:%d]", *pshmIdFACE);
}

/**
 * @brief sd7_4_GetSemFACE Ler a descrição da tarefa SD7.4 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param psemIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_4_GetSemFACE(key_t ipcKeyFace, int *psemIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);

    int semExistente = semget(KEY_FACE, 4, 0666);
    if(semExistente == -1){
        so_error("SD7.4", "Não foi possível aceder ao semáforo da FACE");
        exit(1);
    }

    *psemIdFACE = semExistente;
    
    so_success("SD7.4", "Semáforo da FACE acedido");
    so_debug("> [@return *psemIdFACE:%d]", *psemIdFACE);
}

/**
 * @brief sd7_5_ProcuraLugarDisponivelBD Ler a descrição da tarefa SD7.5 no enunciado
 * @param semId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd7_5_ProcuraLugarDisponivelBD(int semId, MsgContent clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    so_debug("< [@param semId:%d, clientRequest:[%s:%s:%c:%s:%d:%d], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", semId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, lugaresEstacionamento, dimensaoMaximaParque);

    struct sembuf semOp;
    semOp.sem_num = SEM_LUGARES_PARQUE;
    semOp.sem_op = -1;
    semOp.sem_flg = 0;
    
    if(semop(semId, &semOp, 1) == -1){
        so_error("SD7.5", "Não foi possível decrementar o semáforo LUGARES_PARQUE");
        exit(1);
    }

    semOp.sem_num = SEM_MUTEX_BD;
    semOp.sem_op = -1;

    if(semop(semId, &semOp, 1) == -1){
        so_error("SD7.5", "Não foi possível decrementar o semáforo MUTEX_BD");
        exit(1);
    }

    for(int i = 0; i < dimensaoMaximaParque; i++) {
        if(lugaresEstacionamento[i].pidCliente == DISPONIVEL) {
            lugaresEstacionamento[i].pidCliente = 0;
            lugaresEstacionamento[i].pidCliente = clientRequest.msgData.est.pidCliente;
            lugaresEstacionamento[i].viatura = clientRequest.msgData.est.viatura;
            lugaresEstacionamento[i].pidServidorDedicado = clientRequest.msgData.est.pidServidorDedicado;
            *pindexClienteBD = i;
            so_success("SD7.5", "Reservei Lugar: %d.", *pindexClienteBD);
            break;
        }
    }

    semOp.sem_op = 1;
    if(semop(semId, &semOp, 1) == -1){
        so_error("SD7.5", "Erro a incrementar o semáforo MUTEX_BD");
        exit(1);
    }
    
    so_debug("> [*pindexClienteBD:%d]", *pindexClienteBD);
}

/**
 * @brief  sd8_1_ValidaMatricula Ler a descrição da tarefa SD8.1 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_1_ValidaMatricula(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    for(int i = 0; clientRequest.msgData.est.viatura.matricula[i] != '\0'; i++){
        if(!isupper(clientRequest.msgData.est.viatura.matricula[i]) && !isdigit(clientRequest.msgData.est.viatura.matricula[i])){
            so_error("SD8.1", "Matricula não cumpre os requisitos.");
            sd11_EncerraServidorDedicado();
            exit(1);
        }
    }

    so_success("SD8.1", "Matrícula válida.");
    so_debug(">");
}

/**
 * @brief  sd8_2_ValidaPais Ler a descrição da tarefa SD8.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_2_ValidaPais(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if(strlen(clientRequest.msgData.est.viatura.pais) != 2){
        so_error("SD8.2", "Código de país não cumpre os requisitos.");
        sd11_EncerraServidorDedicado();
        exit(1);
    }
    if(!isupper(clientRequest.msgData.est.viatura.pais[0]) || !isupper(clientRequest.msgData.est.viatura.pais[1])){
        so_error("SD8.2", "Código de país não cumpre os requisitos.");
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    so_success("SD8.2", "Código de país válido.");
    so_debug(">");
}

/**
 * @brief  sd8_3_ValidaCategoria Ler a descrição da tarefa SD8.3 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_3_ValidaCategoria(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if(clientRequest.msgData.est.viatura.categoria != 'P' && clientRequest.msgData.est.viatura.categoria != 'L' && clientRequest.msgData.est.viatura.categoria != 'M') {
        so_error("SD8.3", "Categoria inválida.");
        sd11_EncerraServidorDedicado();
        exit(1);
    }
    
    so_success("SD8.3", "Categoria válida.");
    so_debug(">");
}

/**
 * @brief  sd8_4_ValidaNomeCondutor Ler a descrição da tarefa SD8.4 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_4_ValidaNomeCondutor(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    FILE *users = fopen(FILE_USERS, "r");
    int encontrou = 0;

    if(users == NULL) {
        so_error("SD8.4", "Não foi possível abrir o ficheiro de utilizadores.");
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    char line[100];
    while(so_fgets(line, sizeof(line), users) != NULL){
        char lineCopy[100];
        strcpy(lineCopy, line);

        char* frags[7] = {NULL};
        int counter = 0;
        char* part = strtok(lineCopy, ":");

        while( part != NULL && counter < 7){
            frags[counter ++] = part;
            part= strtok(NULL, ":");
        }
        if(counter >= 5 && frags [4][0] != '\0'){
            char driverName[100];
            strcpy(driverName, frags[4]);

            char* comma = strstr(driverName, ",");
            if(comma != NULL){
                *comma = '\0';
            }
            if (strcmp(driverName, clientRequest.msgData.est.viatura.nomeCondutor) == 0) {
                encontrou = 1;
                so_success("SD8.4", "Nome do condutor corresponde a um nome da base de dados.");
            }
        }
    }

    fclose(users);

    if(!encontrou){
        so_error("SD8.4", "Nome do condutor não corresponde a um nome da base de dados.");
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    so_debug(">");
}

/**
 * @brief sd9_1_AdormeceTempoRandom Ler a descrição da tarefa SD9.1 no enunciado
 */
void sd9_1_AdormeceTempoRandom() {
    so_debug("<");

    int tempoRandom = rand() % MAX_ESPERA + 1;
    sleep(tempoRandom);

    so_success("SD9.1", "%d", tempoRandom);
    so_debug(">");
}

/**
 * @brief sd9_2_EnviaSucessoAoCliente Ler a descrição da tarefa SD9.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd9_2_EnviaSucessoAoCliente(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if(msgsnd(msgId, &clientRequest, sizeof(MsgContent)- sizeof(long), 0) == -1){
        so_error("SD9.2", "Não foi possível enviar mensagem ao cliente.");
        sd11_EncerraServidorDedicado();
        exit(1);
    }
    
    so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", indexClienteBD);
    so_debug(">");
}

/**
 * @brief sd9_3_EscreveLogEntradaViatura Ler a descrição da tarefa SD9.3 no enunciado
 * @param logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 * @param pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param plogItem (O) registo de Log para esta viatura
 */
void sd9_3_EscreveLogEntradaViatura(char *logFilename, MsgContent clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [@param logFilename:%s, clientRequest:[%s:%s:%c:%s:%d:%d]]", logFilename, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    FILE *pointer = fopen(logFilename, "a+b");
    if(pointer == NULL) {
        so_error("SD9.3", "Não foi possível abrir o ficheiro logFilename.");
        sd11_EncerraServidorDedicado();
    }

    if(fseek(pointer, 0, SEEK_END) != 0){
        so_error("SD9.3", "Não foi possível endereçar o log.");
        fclose(pointer);
        sd11_EncerraServidorDedicado();
    }

    long size = ftell(pointer);
    if(size == -1){
        so_error("SD9.3", "Não foi possível obter o tamanho do ficheiro.");
        fclose(pointer);
        sd11_EncerraServidorDedicado();
    }

    if(pposicaoLogfile){
        *pposicaoLogfile = size;
    }

    time_t timer = time(NULL);
    struct tm *currentTime = localtime(&timer);
    if(currentTime == NULL){
        so_error("SD9.3", "Não foi possível obter o tempo atual.");
        fclose(pointer);
        sd11_EncerraServidorDedicado();
    }

    LogItem newLog;
    newLog.viatura = clientRequest.msgData.est.viatura;
    strftime(newLog.dataEntrada, sizeof(newLog.dataEntrada), "%Y-%m-%dT%Hh%M", currentTime);
    strcpy(newLog.dataSaida, "");

    size_t written = fwrite(&newLog, sizeof(LogItem), 1, pointer);
    if(written != 1){
        so_error("SD9.3", "Não foi possível escrever no ficheiro log.");
        fclose(pointer);
        sd11_EncerraServidorDedicado();
    }

    fclose(pointer);

    if(plogItem){
        *plogItem = newLog;
    }

    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s", *pposicaoLogfile, plogItem->viatura.matricula, plogItem->dataEntrada);
    so_debug("> [*pposicaoLogfile:%ld, *plogItem:[%s:%s:%c:%s:%s:%s]]", *pposicaoLogfile, plogItem->viatura.matricula, plogItem->viatura.pais, plogItem->viatura.categoria, plogItem->viatura.nomeCondutor, plogItem->dataEntrada, plogItem->dataSaida);
}

/**
 * @brief  sd10_1_AguardaCheckout Ler a descrição da tarefa SD10.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 */
void sd10_1_AguardaCheckout(int msgId) {
    so_debug("< [@param msgId:%d]", msgId);

    MsgContent msg;

    alarm(60);
        
    while(1){
        if(msgrcv(msgId, &msg, sizeof(MsgContent)-sizeof(long), getpid(), 0) == -1) {
            so_error("SD10.1", "Não foi possível receber a mensegagem do Cliente.");
            if(errno != EINTR){
                sd11_EncerraServidorDedicado();
                break;
            }
            continue;
        }

        if(msg.msgData.status == TERMINA_ESTACIONAMENTO){
            clientRequest = msg;
            so_success("SD10.1", "SD: A viatura %s deseja sair do parque", clientRequest.msgData.est.viatura.matricula);
            break;
        }
    }

    so_debug(">");
}


/**
 * @brief  sd10_1_1_TrataAlarme Ler a descrição da tarefa SD10.1.1 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd10_1_1_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    sem_wait(semIdFACE, SEM_MUTEX_BD, IPC_NOWAIT);
    int tarifa = *tarifaAtual;
    sem_signal(semIdFACE, SEM_MUTEX_BD, IPC_NOWAIT);

    time_t atual = time(NULL);
    struct tm *t = localtime(&atual);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%Hh%M", t);

    MsgContent msg;
    msg.msgType = clientRequest.msgData.est.pidCliente;
    msg.msgData.status = INFO_TARIFA;
    msg.msgData.est = clientRequest.msgData.est;

    snprintf(msg.msgData.infoTarifa, sizeof(msg.msgData.infoTarifa), "%s Tarifa atual:%d", timestamp, tarifa );
    

    if(msgsnd(msgId, &msg, sizeof(MsgContent)-sizeof(long), 0) == -1) {
        so_error("SD10.1.1", "Não foi possível enviar mensagem INFO_TARIFA.");
    }
    
    so_success("SD10.1.1", "Info Tarifa");

    alarm(60);
    so_debug(">");
}

/**
 * @brief  sd10_2_EscreveLogSaidaViatura Ler a descrição da tarefa SD10.2 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  posicaoLogfile (I) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  logItem (I) registo de Log para esta viatura
 */
void sd10_2_EscreveLogSaidaViatura(char *logFilename, long posicaoLogfile, LogItem logItem) {
    so_debug("< [@param logFilename:%s, posicaoLogfile:%ld, logItem:[%s:%s:%c:%s:%s:%s]]", logFilename, posicaoLogfile, logItem.viatura.matricula, logItem.viatura.pais, logItem.viatura.categoria, logItem.viatura.nomeCondutor, logItem.dataEntrada, logItem.dataSaida);

    FILE *fLogFile = fopen(logFilename, "r+b");
    if(fLogFile == NULL){
        so_error("SD10.2", "Não foi possível abrir logFile.");
        sd11_EncerraServidorDedicado();
    }

    if(fseek(fLogFile, posicaoLogfile, SEEK_SET) != 0){
        so_error("SD10.2", "Não foi possível endereçar logFile.");
        fclose(fLogFile);
        sd11_EncerraServidorDedicado();
    }

    if(fwrite(&logItem, sizeof(LogItem), 1, fLogFile) != 1){
        so_error("SD10.2", "Não foi possível escrever no logFile");
        fclose(fLogFile);
        sd11_EncerraServidorDedicado();	
    }

    fclose(fLogFile);

    so_success("SD10.2", "SD: Atualizei log na posição %ld: Saída Cliente %s em %s", posicaoLogfile, logItem.viatura.matricula, logItem.dataSaida);
    sd11_EncerraServidorDedicado();
    so_debug(">");
}

/**
 * @brief  sd11_EncerraServidorDedicado Ler a descrição da tarefa SD11 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd11_EncerraServidorDedicado() {
    so_debug("<");

    sd11_1_LibertaLugarViatura(semId, lugaresEstacionamento, indexClienteBD);
    sd11_2_EnviaTerminarAoClienteETermina(msgId, clientRequest);

    so_debug(">");
}

/**
 * @brief sd11_1_LibertaLugarViatura Ler a descrição da tarefa SD11.1 no enunciado
 * @param semId (I) identificador aberto de IPC
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd11_1_LibertaLugarViatura(int semId, Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param semId:%d, lugaresEstacionamento:%p, indexClienteBD:%d]", semId, lugaresEstacionamento, indexClienteBD);

    if(indexClienteBD < 0){
        so_error("SD11.1", "Índice do cliente inválido.");
    }

    struct sembuf semOp = {
        .sem_num = SEM_LUGARES_PARQUE,
        .sem_op = 1,
        .sem_flg = 0,
    };

    if(semop(semId, &semOp, 1) == -1){
        so_error("SD11.1", "Não foi possível aumentar o semáforo LUGARES_PARQUE.");
        exit(1);
    }

    lugaresEstacionamento[indexClienteBD].pidCliente = DISPONIVEL;
    lugaresEstacionamento[indexClienteBD].pidServidorDedicado = DISPONIVEL;

    semOp.sem_num = SEM_MUTEX_BD;
    semOp.sem_op = -1;
    if(semop(semId, &semOp, 1) == -1){
        so_error("SD11.1", "Não foi possível decrementar o semáforo MUTEX_BD.");
        exit(1);
    }

    semOp.sem_num = NO_FLAGS;
    semOp.sem_op = 1;
    if(semop(semId, &semOp, 1) == -1){
        so_error("SD11.1", "Não foi possível aumentar o semáforo NO_FLAGS.");
        exit(1);
    }

    so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);
    so_debug(">");
}

/**
 * @brief sd11_2_EnviaTerminarAoClienteETermina Ler a descrição da tarefa SD11.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd11_2_EnviaTerminarAoClienteETermina(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if(msgsnd(msgId, &clientRequest, sizeof(MsgContent)-sizeof(long), 0) == -1){
        so_error("SD11.2", "Não foi possível enviar mensagem ao cliente.");
        exit(1);
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

    if(sinalRecebido == SIGUSR2) {
        so_success("SD12", "SD: Recebi pedido do Servidor para terminar");
        sem_signal(semId, SEM_SRV_DEDICADOS, 0);
        sd11_EncerraServidorDedicado();
    }


    so_debug(">");
}