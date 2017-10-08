/***********************************************************************
* File Name: .c
*	      
* Author:    Ana Catarina Sa (ist178665) e Miguel Moreira (ist181048)
* Data:      21 de Novembro de 2015   
* 
* NAME       RELIABLE MESSAGE BOARD (Projecto de RCI 2017)
*
* DESCRIPTION Programa resposavel por implementar o servidor de mensagens 
*            do sistema de comunicacao fiavel entre utilizadores.
* 
* ************************************************************************/
#include "msgserv.h"



/***********************************************************************
* main()
*
* Arguments: arc, argv
*
**********************************************************************/
int main(int argc, char *argv[]){

    fd_set rfds;
    time_t sec=0, asec=0;
    MsgServ *msid, *aux;
    char buf[500];
    struct timeval tv;
    int whl=1, fd, join=0, retval=1, maxfd, clock=0;

    /* Verifica correcao dos argumentos */
    checkArguments( argc, argv );
    
    /* Preencher a lista de servidores */
    msid = initListMsgServ( argc, argv );
    tv.tv_sec = msid->r; tv.tv_usec=0;
    
    /* File Descriptor da socket UDP para ligacao ao das identidades */
    fd = socket(AF_INET, SOCK_DGRAM, 0); if(fd==-1) packageExit( msid );

    while( whl == 1 ){
        /* Actualiza o file descriptor set, com os fd's pertinentes ao funcionamento do programa */
        if (join == 1) retval = refreshSelect( msid, &maxfd, &sec, &asec,  &tv , &rfds);
        /* Actualiza o file descriptor set, com os fd's pertinentes a inicializacao do programa */
        else {  FD_ZERO(&rfds); FD_SET(0, &rfds);
                retval = select(1, &rfds, (fd_set*) NULL, (fd_set*) NULL, NULL);    }
        
        /* Significa que o contador expirou, fazer o refresh da ligação */
        /* O servidor liga-se a todos os outros apenas quando é feito o primeiro join. A partir dai todas as novas ligações deveram ser estabelecidas pelos servidores que se ligarem e a lista será actualizada com os servidores que sairam por verificaçao da continuidade da ligacao TCP ao mesmo */
        if (retval == 0){   joinIDServer(msid, fd);
                            asec = 0; sec = 0;
                            tv.tv_sec = msid->r;    };
        
        /* Erro no select */
        if (retval < 0 ){ fprintf(stderr, "Erro no select main.\n"); packageExit( msid ); }
        
        /* Verifica se o stdin (fd 0) tem input do utilizador para atender */
        if( FD_ISSET(0, &rfds) != 0 ){
            fgets( buf, 100, stdin);
            
            if( !strcmp(buf, JOIN) || !strcmp(buf, SHOWS ) || !strcmp(buf, SHOWM ) || !strcmp(buf, EXIT ) ){
                if ( !strcmp( buf, JOIN) && join == 1 ) printf("%s", INFO5);
                /* Lida com o comando de join */
                if ( !strcmp( buf, JOIN) && join == 0 ){
                    /* Permite o registo no servidor de identidades */
                    joinIDServer(msid, fd);
                    
                    /* Aloca e  Preenche a lista de servidores fazendo um pedido GET_SERVERS */
                    join = fillServStr( msid, fd, buf );
   
                    if(join == 1){
                        /* Estabelecer uma ligacao TCP com todos os servidores conhecidos */
                        connectAllServ( msid );
                        
                        /* Inicializa um servidor de TCP disponivel a aceitar pedidos de outros servidores */
                        createTCPServ( msid );
                        
                        /* Inicializa um servidor de UDP disponivel para aceitar pedidos dos terminais */;
                        createUDPServ( msid );
                        
                        /* Alocar e preencher a lista de mensagens fazendo um pedido SGET_MESSAGES aos servidores de mensagens conhecidos */
                        initMsgList( msid, &clock);
                        printf("%s%s%s", DES1, ID1, DES1);
                    }
                }
                /* Rotina para atender a ordem de mostrar os servidores conhecidos ou as mensagens guardadas no terminal */
                if( join == 1) refreshShowCmd( msid, buf );
                else printf("%s", INFO4);
                
                if ( !strcmp( buf, EXIT ) ){    printf("%s%s%s\n", DES1, EXIT1, DES1); whl=0;   }
            }
            else printf("\nIncorrect usage of the user interface. %s", INFO);
            
            if ( strcmp(buf,EXIT)!=0 ) printf("\nInput Command:\n");
        }
     
        if (join == 1){
           
            /* Rotina atende a pedidos dirigidos ao servidor UDP vindos dos Terminais */
            if( FD_ISSET(msid->fdudp, &rfds) != 0 ) refreshUDPServ(msid, &clock);
            
            /* Rotina atende a pedidos dirigidos ao servidor TCP vindos de outros servidores de mensagens */
            if( FD_ISSET(msid->fdtcp, &rfds) != 0 ) refreshTCPServ( msid, fd );
            
            /* Rotina lida com a comunicacao entre servidores de mensagens */
            aux = msid->next;
            while ( aux != NULL ) {
                if( FD_ISSET(aux->fdtcp, &rfds) != 0 ) commsTCP( msid, aux, &clock );
                aux=aux->next;
      	    }
        }
    }

    /* Rotinas executam o fecho de todas as sockets utilizadas */
    close( fd );
    closeFDs( msid );
    /* Rotina dealoca toda a memoria utilizada ao longo do programa */
    freeSTRs( msid );
    
    return(0);

}



/***********************************************************************
* refreshTCPServ()
*
* Arguments: File descriptor do servidor de identidades e estrutura
*           principal do servidor de mensagens
* Returns: void
* Description: Rotina atende a pedidos dirigidos ao servidor TCP vindos
*             de outros servidores de mensagens
*
**********************************************************************/
void refreshTCPServ( MsgServ* msid, int fd ){

    int newfd;
    unsigned int clilen;
    struct sockaddr_in cli_addr;
    
    memset( (void*) &cli_addr, (int) '\0', sizeof(cli_addr));
    clilen = sizeof(cli_addr);

    /* Aceita o pedido de ligacao e chama a rotina responsavel por criar e preencher a estrutura do servidor de mensagens */
    if( (newfd = accept(msid->fdtcp, (struct sockaddr*) &cli_addr, &clilen)) >= 0 ) createMsgServ( msid, fd, newfd);
    else{
        fprintf(stderr, "Erro no estabelecimento de nova sessao TCP com um Servidor de Mensagens.\n");
        packageExit( msid );
    }

    return;
}



/***********************************************************************
* refreshShowCmd()
*
* Arguments: Estrutura principal do servidor de mensagens e buffer com 
*           pedido introduzido no terminal
* Returns: void
* Description: Rotina para atender a ordem de mostrar os servidores 
*             conhecidos ou as mensagens guardadas no terminal
*
**********************************************************************/
void refreshShowCmd( MsgServ* msid, char* buf  ){

    int i;
    MsgServ* aux;
    MsgInfo* saux;
    
    /* Caso seja um pedido para mostrar os servidores conhecidos */
    if ( !strcmp( buf, SHOWS ) ){
        aux = msid;
        i=1;
        printf("%s", INFO2);
        /* Corre a lista de servidores conhecidos, imprimindo-a */
        while( aux != NULL){
            printf("                   SERVER %d:\nNAME: %s IP: %s UPT: %d TPT: %d\n", i, aux->name, inet_ntoa(*    aux->ip), aux->upt, aux->tpt);
            i++; aux = aux->next;
        }
        printf("%s", DES1);
    }
    /* Caso seja um pedido para mostrar as mensagens conhecidas */
    else if(!strcmp( buf, SHOWM ) ){
        printf("%s", INFO3);
    
        /* Caso nao existam mensagens conhecidas */
        if( msid->msgs->number == 0 ) printf("       THERE ARE NO MESSAGES TO BE DISPAYED!\n");
        /* Caso existam mensagens conhecidas */
        else{
            printf("      THERE ARE %d MESSAGES TO BE DISPAYED:\n", msid->msgs->number);
            saux = msid->msgs->beg;
            i=0;
            /* Corre a lista de mensagens conhecidas, imprimindo-a no terminal */
            while( saux != NULL ){
                i++;
                printf("NUMBER: %d MESSAGE: %s\n", i, saux->msg);
                saux = saux->next;
            }
        }
        printf("%s", DES1);
    }
    
    return;
}



/***********************************************************************
* refreshUDPServ()
*
* Arguments: Estrutura principal do servidor de mensagens e ponteiro
*           para o relogio logico do servidor de mensagens
* Returns: void
* Description: Rotina atende a pedidos dirigidos ao servidor UDP vindos
*             dos Terminais
*
**********************************************************************/
void refreshUDPServ( MsgServ* msid, int* clock ) {

	int nread, n;
	char buffer[128], *aux2, *aux3;
	struct sockaddr_in addr;
	unsigned int addrlen;

    memset( (void*) &addr, (int)'\0', sizeof(addr) );
    
    addrlen=sizeof(addr);
    nread = recvfrom(msid->fdudp, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (nread < 0 ) packageExit( msid );
    
    if( nread > 140 ) nread = 140;
    buffer[nread]='\0';
             
    if ( !strncmp( buffer, "PUBLISH", 7) ){

        aux2 = strstr( buffer, " "); aux2 ++;
        aux3 = strchr( aux2, '\0');
        n = aux3 - aux2 ;
        
        /* Rotina permite o broadcast da mensagem recebida da aplicacao terminal por todos os servidores de mensagens conhecidos */
        broadcast( msid, aux2, n , *clock );
        /* Rotina preenche a lista de mensagens do servidor com a que foi recebida */
        fillMsgList( msid, aux2, n , *clock );
        (*clock)++;
    }
                
    else if ( !strncmp( buffer, "GET_MESSAGES", 12) ){
        aux2 = strstr( buffer, " "); aux2 ++;
        n = atoi( aux2 ) ;
        /* Rotina permite a resposta ao pedido com n mensagens que temos em memoria */
        sendNMsgs( msid, &addr, n);
    }
    
	return;
}



/***********************************************************************
* refreshSelect()
*
* Arguments: Estrutura principal do servidor de mensagens, estrutura do
*           timeout do select, valores auxiliares para actualizacao do
*           tempo e o file descriptor set
* Returns: valor de retorno do select
* Description: Actualiza o file descriptor set, com os fd's pertinentes
*             ao funcionamento do programa e actualiza o timeout do 
*             select para o proximo ciclo do mesmo
*
**********************************************************************/
int refreshSelect( MsgServ* msid, int* maxfd, time_t* sec, time_t* asec,  struct timeval* tv ,fd_set* rfds){
 
    MsgServ* aux;
    int retval;

    /* Actualizar os fd's das varias sockets em utilizacao, fazendo set apenas para os servidores em uso e para o stdin*/
    FD_ZERO( rfds);
    FD_SET(0, rfds);                       /* Stdio */
    *maxfd = 0;
    FD_SET(msid->fdudp, rfds);             /* Socket Servidor UDP p/Terminais */
    *maxfd = MAX(*maxfd, msid->fdudp);
    FD_SET(msid->fdtcp, rfds);             /* Socket Servidor TCP p/ Pedidos MsgServers */
    *maxfd = MAX(*maxfd, msid->fdtcp);
            
    aux = msid->next;
    while( aux != NULL ){
        FD_SET(aux->fdtcp, rfds);         /* Socket Cliente TCP p/Comunicacao MsgServers */
        *maxfd = MAX(*maxfd, aux->fdtcp);
        aux = aux->next;
    }
    
    /* Actualiza o timeout do select */
    if ((*tv).tv_sec > 0) (*tv).tv_sec = (*tv).tv_sec - ( (int)*asec - (int)*sec );
    if ((*tv).tv_sec < 0) (*tv).tv_sec=0;
          
    *sec = time(NULL);
    retval = select(*maxfd+1, rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) tv);
    *asec = time(NULL);

	return retval;
}



/***********************************************************************
* packageExit()
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: void
* Description: Funcao permite abortar o programa em caso de erro 
*             fechanco todas as sockets utilizadas e libertando a memoria
*             alocada
*
**********************************************************************/
void packageExit( MsgServ* msid ){
   
    closeFDs( msid );
    freeSTRs( msid );

    exit(1);
}



/***********************************************************************
* closeFDs()
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: void
* Description: Funcao realiza o fecho de todas as sockets usadas
*
**********************************************************************/
void closeFDs( MsgServ *msid ){
    
    MsgServ *aux;
    
    /* Fecho das sockets dos servidores UDP e TCP do servidor de identidades */
    close( msid->fdtcp );
    close( msid->fdudp );
    
    /* Fecho das sockets utilizadas para as comunicacoes TCP entre servidores de mensagens */
    aux = msid;
    while( aux != NULL ){
        /* Permite a correcta terminacao da sessao TCP estabelecida com os servidores de mensagens */
        close( aux->fdtcp );
        aux = aux->next;
    }

    return;
}



/***********************************************************************
* freeSTRs()
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: void
* Description: Funcao realiza a dealocacao de toda a memoria utilizada
*
**********************************************************************/
void freeSTRs( MsgServ *msid ){
    
    /* Dealoca todas as estruturas MsgServ menos a estrutura principal */
    if( msid->next != NULL) recursListFree( msid->next );
    
    if( msid->name!= NULL ) free( msid->name );
    if( msid->ip!= NULL ) free( msid->ip );
    //if( msid->siip != NULL ) free( msid->siip );
    
    /* Dealoca todas as estruturas de mensagens */
    if( msid->msgs != NULL){
        if( msid->msgs->beg != NULL ) recursMessFree( msid->msgs->beg );
        free( msid->msgs );
    }

    if( msid != NULL ) free(msid);
    
    return;
}



/***********************************************************************
 * recursMessFree()
 *
 * Arguments: Estrutura de uma mensagem
 * Returns: void
 * Description: Permite dealocar toda a memoria utilizada para o armaze-
 *             namento de informacao relacionada com as mensagens
 *
 **********************************************************************/
void recursMessFree( MsgInfo* pont ){
    
    if(pont == NULL) return;
    if(pont->next != NULL) recursMessFree(pont->next);
    
    if(pont->msg != NULL) free(pont->msg);
    free(pont);
    
    return;
}



/***********************************************************************
* commsTCP()
*
* Arguments: Estrutura principal do servidor de mensagens, ponteiro para
*           o servidor de mensagens a que a rotina esta a atender e
*           ponteiro para o valor do relogio logico do servidor
* Returns: void
* Description: Rotina lida com a comunicacao entre servidores de mensagens,
*             sendo responsavel por atender a pedidos SGET_MESSAGES e 
*             SMESSAGES. E ainda responsavel pela remocao do servidor
*             em questao caso esse tenha terminado a sessao connosco
*
**********************************************************************/
void commsTCP( MsgServ* msid, MsgServ* ptr, int *clock ){
    
    int nwritten, nread, n, i, tread = 0;
    char *aux, *aux2, buffer[200], buf[25];
    MsgInfo *pnt;
    
    memset( (void*) buffer, (int) '\0', sizeof(buffer) ); aux=buffer;
    
    /* Leitura do pedido enviado pelo servidor de mensagens */
    while(1){
        /* A mensagem e demasiado grande para respeitar o protocolo. Como tal o servidor esta em incumprimento protocolar e e removido para evitar futuros conflitos */
        if( tread > 200){
            printf("A mensagem enviada pelo servidor de mensagens e demasiado longa, nao respeitando as regras protocolares.\n");
            removeMsgServ( msid, ptr );
            return;
        }
        
        nread = read( ptr->fdtcp, aux, 1 );
        /* Terminacao imprevista da ligacao TCP. O servidor em questao e removido da lista de forma a recuperar do erro */
        if( nread < 0 ){
            printf("%s\nThe TCP session was terminated in an unpredicted way with the server %s.\n%s\nInput command:\n", DES1, ptr->name, DES1);
            removeMsgServ( msid, ptr );
            return;
        }
        /* Lida com um pedido de terminacao de sessao enviado */
        else if (nread == 0){
            printf("%s\nServer %s disconnected from your private cluster.\n%s\nInput command:\n", DES1, ptr->name, DES1);
            removeMsgServ( msid, ptr );
            return;
        }
        else{
            tread = tread + nread;
            if ( aux[nread-1] == '\n') break;
            aux += nread;
        }
    }
    
    /* Selecao do tipo de pedido recebido. Caso se trate de um pedido SGET_MESSAGES */
    if ( !strncmp( buffer, SGETM, 14 ) ){   
        aux = (char*) malloc ( sizeof(char) *  (10 + (145 * msid->msgs->number) ) );
        if ( aux == NULL){
            fprintf(stderr, "Erro na alocacao de memoria para o atendimento do pedido vindo do servidor de mensagens.\n");
            packageExit( msid );
        }

        /* Criacao da mensagem de resposta ao pedido SGET_MESSAGES com todas as mensagens conhecidas */
        memset( (void*) aux, (int) '\0', sizeof(aux) );
        strcpy( aux, "SMESSAGES\n" );
        aux2 = strstr( aux, "\n" ); aux2++;
        
        pnt = msid->msgs->beg;
        while( pnt != NULL ){
            sprintf( aux2, "%d;", pnt->clock );
            aux2 = strchr( aux2, '\0' );
            strcpy( aux2, pnt->msg );
            i = strlen(pnt->msg);
            aux2+=i; aux2[0]='\n'; aux2++;
            pnt = pnt->next;
        }
        aux2 = aux; n = strlen(aux2);

        /* Envio da resposta ao pedido */
        while( n>0 ){
            if( (nwritten = write( ptr->fdtcp, aux2, n) ) <= 0 ){
                removeMsgServ( msid, ptr );
                free(aux);
                return;
            }
            n-=nwritten;
            aux2+=nwritten;
        }
        free(aux);
    }
   
    /* Caso se trate de um pedido SMESSAGES. Uma vez que o servidor actualiza toda a sua lista apenas no inicio, caso receba um pedido SMESSAGES significa que se trata do broadcast de uma mensagem publicada para outro servidor de mensagens. Age de acordo. */
    else if( !strncmp( buffer, SMESS, 10 ) ){
        i = tread; n = 10; aux++;
        
        while(1){
            if( tread > ( 200 - i ) ){
                printf("%s\nThe message sent by the message server does not respect the communications protocol.\n%s\nInput command:\n", DES1, DES1 );
                removeMsgServ( msid, ptr );
                return;
            }
            nread = read( ptr->fdtcp, aux, n);
            if( nread < 0 ){
                if ( n == 1 ){
                    printf("%s\nFailed to communicate with the message server.\n%s\nInput command:\n", DES1, DES1);
                    removeMsgServ( msid, ptr );
                    return;
                }
                else n--;
            }
            else if (nread == 0){
                printf("%s\nUnexpected termination of the TCP session.\n%s\nInput command:\n", DES1, DES1);
                removeMsgServ( msid, ptr );
                return;
            }
            else{
                tread = tread + nread;
                if ( aux[ nread-1 ] == '\n') break;
                aux += nread;
            }
        }
        
        aux = strstr( buffer, "\n"); aux++;
        aux2 = strstr( buffer, ";");
        n = strlen( aux ) - strlen( aux2 );
        strncpy( buf, aux, n ); buf[n]='\0'; aux2++;
        aux = strstr( aux2, "\n");
        /* Actualizacao do valor do relogio logico do servidor de mensagens */
        *clock = MAX(  (*clock), atoi(buf) ) + 1;
        /* Rotina responsavel por preencher a lista de mensagens com a que foi recebida */
        fillMsgList( msid, aux2 , strlen( aux2 ) - strlen( aux ) , atoi(buf) );
    }
    else printf( "\nA message that does not meet the communications protocol was received from a message server.\n\nInput Command:\n" );
    
    return;
}
          


/***********************************************************************
* removeMsgServ
*
* Arguments: Estrutura principal do servidor de mensagens e ponteiro
*           semelhante que indica o servidor a remover
* Returns: void
* Description: Rotina permite a correcta remocao do servidor de mensagens
*             considerado, fechando sessao TCP com o mesmo e dealocando
*             toda a memoria que por este era ocupada
*
**********************************************************************/
void removeMsgServ( MsgServ* msid, MsgServ* aux ){
    
    MsgServ *aux2, *aux3;
    
    /* Busca do servidor considerado na lista conhecida */
    aux2 = msid;
    while( strcmp( aux->name, aux2->next->name ) != 0 ){
        aux2=aux2->next;
    }
    aux3 = aux->next;
    
    /* Fecho de sessao TCP */
    close( aux->fdtcp );
    free( aux->name );
    free( aux->ip );
    free( aux );
    /* Garante coerencia da lista resultante */
    aux2->next = aux3;

    return;
}



/***********************************************************************
* sendNMsgs
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: void
* Description: Rotina permite a resposta a um pedido GET_MESSAGES
*             com as ultimas n mensagens que temos em memoria
*
**********************************************************************/
void sendNMsgs( MsgServ* msid, struct sockaddr_in* addr, int n){
    
    int i;
    char *aux, *aux2;
    MsgInfo *pnt;
    unsigned int addrlen;
    
    aux = (char*) malloc ( sizeof(char) *  (9+ (141 * n) ) );
    if ( aux == NULL){
        fprintf(stderr, "O servidor de mensagens nao me respondeu ao pedido de GET_MESSAGES.\n");
        packageExit( msid );
    }
    
    /* Formatacao da mensagem a enviar */
    memset( (void*) aux, (int) '\0', sizeof(aux) );
    strcpy( aux, "MESSAGES\n" );
    aux2 = strstr( aux, "\n" ); aux2++;
    
    /* Caso nao tenhamos tantas mensagens quanto as pedidas */
    if ( n > msid->msgs->number ){
        pnt = msid->msgs->beg;
        
        while( pnt != NULL ){
            strcpy( aux2, pnt->msg );
            i = strlen(pnt->msg); aux2+=i;
            aux2[0]='\n'; aux2++;            
            pnt = pnt->next;
        }
    }
    /* Caso existam mais mensagens em memoria do que as pedidas. A lista (duplamente ligada) e percorrida do fim ate satisfazer o numero de mensagens a enviar. */
    else{
        pnt = msid->msgs->end;
        for( i=0; i < n-1 ; i++) pnt = pnt->prv;
        while( pnt != NULL ){
            strcpy( aux2, pnt->msg );
            i = strlen(pnt->msg); aux2+=i;
            aux2[0]='\n'; aux2++;
            pnt=pnt->next;
        }
    }
    
    addrlen = sizeof(*addr);
    i = sendto(msid->fdudp, aux, strlen(aux) , 0, (struct sockaddr*) addr, addrlen);
    if(i < 0){
        fprintf(stderr, "Erro no envio de mensagem de resposta ao GET_MESSAGES N do servidor de identidades.\n");
        packageExit( msid );
    }
    
    free(aux);
    return;
}



/***********************************************************************
* initMsgList
*
* Arguments: Estrutura principal do servidor de mensagense e ponteiro
*           para o relogio logico do servidor
* Returns:
* Description:
*
**********************************************************************/
void initMsgList( MsgServ* msid, int* clock ){
    
    MsgServ* serv;
    char mrbuffer[70000], buffer[25], *ptr, *aux;
    int nbytes, nleft, nwritten, n, i=0;
    int nread=0, tread=0;
    
    printf("\nRetrieving chat history from the cluster's message servers...\nThis may take several minutes to complete.\n");
    
    serv = msid->next;
    while( serv != NULL ){
        if( serv->fdtcp == 0) serv=serv->next;
        else break;
    }
    
    if( serv == NULL ){
        printf("No messages are available from this conversation, as there are no active message servers.\n");
        fillMsgList( msid, NULL, 0, 0);
        return;
    }
    
    memset( (void*) mrbuffer, (int) '\0', sizeof(mrbuffer) );
    ptr = strcpy(mrbuffer, "SGET_MESSAGES\n"); nbytes = 14;
           
    /* Faz o pedido aos servidores */
    nleft = nbytes;
    while( nleft > 0 ){
        nwritten = write(serv->fdtcp, ptr, nleft);
        if(nwritten <= 0){
            fprintf(stderr, "Erro no envio do pedido aos servidores de mensagens.\n");
            packageExit( msid );
        }
        nleft -= nwritten;
        ptr += nwritten;
    }

    /* Obtencao e tratamento da resposta */
    ptr = mrbuffer;   
    memset( (void*) mrbuffer, (int) '\0', sizeof(mrbuffer) );   
    n = 5120;
    while(1){
        if( tread > 65000 ){
            fprintf(stderr, "A mensagem enviada pelo servidor de mensagens nao respeita as regras protocolares.\n");
            packageExit( msid );
        }
        
        nread = read(serv->fdtcp, ptr, n);
        if( nread < 0 ){
            if ( n == 1 ){
                fprintf(stderr, "Erro de comunicacao com o servidor de mensagens.\n");
                packageExit( msid );
            }
            else if( n <= 10 ) n--;
            else n = n/2;
        }
        else if (nread == 0){
            fprintf(stderr, "Terminacao inesperada da sessao TCP.\n\n");
            packageExit( msid );
        }
        else{
            tread = tread + nread;
            if ( ptr[nread-1] == '\n') break;
            ptr += nread;
        }
    }   
    mrbuffer[ tread ] = '\0';
    
    /* Tratar a resposta ao mesmo tempo que completamos a Lista de Mensagens */
    aux = strstr(mrbuffer, "\n"); aux++;
    
    if ( strstr( aux,"\n" ) == NULL ){
        /* A mensagem com a lista de mensagens vem em branco */
        printf("\nNo messages available in the cluster's server.\n");
        fillMsgList( msid, NULL, 0, 0);
    }
    
    while( aux[0] != '\0' ){
        ptr = strstr(aux, ";");
        nread = strlen(aux) - strlen(ptr);      
        strncpy( buffer, aux, nread ); buffer[nread] = '\0';        
        (*clock) = MAX( (*clock), atoi(buffer) ) + 1;        
        aux = strstr(ptr, "\n"); ptr++;
        nread = strlen(ptr) - strlen(aux);
        /* Chama rotina responsavel por adicionar mensagens individualmente */
        fillMsgList( msid, ptr, nread, atoi(buffer) );
        aux++; i++;
    }   
    printf("Retrieved %d messages from the message server cluster.\n", i);    
    return;
}



/***********************************************************************
* broadcast
*
* Arguments: Estrutura principal do servidor de mensagens, ponteiro para
*           buffer com mensagem a enviar, dimensao da mensagem a enviar e 
*           valor do relogio logico a imprimir na mesma
* Returns: void
* Description: if nread = 0, deixamos apenas o ListaStruct iniciado
*
**********************************************************************/
void broadcast( MsgServ* msid, char* buffer, int nread, int clock){
    
    MsgServ* serv;
    int nwritten, n;
    char aux[200], *aux2;
    
    memset( (void*) aux, (int) '\0', sizeof(aux) );
    strcpy( aux, "SMESSAGES\n" );
    aux2 = strstr( aux, "\n" );
    aux2++;
    
    sprintf( aux2, "%d;", clock );
            
    aux2 = strchr( aux2, '\0' );
    strncpy( aux2, buffer, nread );
    aux2 += nread;
    aux2[0] = '\n';
    
    n = strlen(aux);
    aux2 = aux;
    
    serv=msid->next;
    while ( serv != NULL){
    
        while( n>0 ){
            if( (nwritten = write(serv->fdtcp, aux2, n) ) <= 0 ){
                fprintf(stderr, "Erro na escrita do broadcast para os outros servidores.\n");
                packageExit( msid );
            }
            n -= nwritten;
            aux2+=nwritten;
        }
        n = strlen(aux);
        aux2=aux;
        serv = serv->next;
    }
    
    return;
    
}
    


/***********************************************************************
* fillMsgList
*
* Arguments: Estrutura principal do servidor de mensagens, buffer com o
*           corpo da mensagem a inserir, inteiro com tamanho do corpo da 
*           mensagem e valor do relogio logico a associar
* Returns: void
* Description: Rotina faz todo o tratamento da lista de mensagens, desde
*             a inicializacao em vazio da mesma, a insercao (ordenada por
*             tempo logico correspondente) de uma nova mensagem na lista.
*             Caso nread = 0 a lista deve apenas ser inicializada
*
**********************************************************************/
void fillMsgList( MsgServ* msid, char* buffer, int nread, int clock ){
    
    MsgInfo *amsg=NULL, *smsg;
    
    /* Se nao for para inicializar a lista vazia, estas estruturas devem ser alocadas e preenchidas */
    if( nread != 0 && buffer != NULL ){
        amsg = (MsgInfo*) malloc( sizeof( MsgInfo ) );
        if( amsg == NULL){
            fprintf(stderr, "Erro na alocacao de memoria para o atendimento do pedido vindo do servidor de mensagens.\n");
            packageExit( msid );
        }
            
        amsg->clock = clock;       
        amsg->msg = (char*) malloc( (nread+1) * sizeof(char) );
        if( amsg->msg == NULL ){
            fprintf(stderr, "Erro na alocacao de memoria.\n");
            packageExit( msid );
        }
        
        strncpy( amsg->msg, buffer, nread );
        (amsg->msg)[nread] = '\0';    
        amsg->prv = NULL; amsg->next = NULL;
    }
    
    /* Se a lista estiver vazia, devemos inicializa/la, alocando o bloco principal das mensagens */
    if( msid->msgs == NULL ){
        msid->msgs = (MsgStr*) malloc( sizeof( MsgStr) );
        if( msid->msgs == NULL){
            fprintf(stderr, "Erro na alocacao de memoria.\n");
            packageExit( msid );
        }
        /* Caso a inicializacao seja feita com uma mensagem */
        if( nread == 0 && buffer == NULL ){
            msid->msgs->number = 0;
            msid->msgs->beg = NULL;
            msid->msgs->end = NULL;
            return;
        }
        /* Caso queiramos inicializar a lista vazia (quando nao existem mensagens obtidas dos restantes servidores de mensagens) */
        else{
            msid->msgs->number = 1;
            msid->msgs->beg = amsg;
            msid->msgs->end = amsg;
        }
    }
    else{
        /* Se a estrutura tiver sido inicializada mas estiver vazia, devemos proceder */
        if( msid->msgs->number == 0 ){
            msid->msgs->beg = amsg;
            msid->msgs->end = amsg;
            msid->msgs->number++;
        }     
        else{
            if ( (msid->msgs->number)+1 > msid->m){
                /* Se a minha lista estiver cheia e a mensagem que eu quero adicionar é anterior à mais antiga que tenho guardada, nao o devo fazer */
                if ( msid->msgs->beg->clock >= clock ){
                    free(amsg->msg);
                    free(amsg);
                    return;
                }
                /* No caso de o servidor so permitir uma mensagem */
                if( msid->m == 1){
                    free(msid->msgs->beg->msg);
                    free(msid->msgs->beg);
                    msid->msgs->beg = amsg;
                    msid->msgs->end = amsg;
                }
                else{
                    /* Primeiro apago a primeira mensagem guardada na lista para poder adicionar a pretendida */
                    smsg = msid->msgs->beg->next;
                    free(msid->msgs->beg->msg);
                    free(msid->msgs->beg);
                    smsg->prv = NULL;
                    msid->msgs->beg = smsg;             
                    /* Chama a rotina responsavel pela correcta inserção, respeitando os relocios logicos */
                    orderlyFillList( msid, amsg, clock );
                }
            }           
            else{
                /* Chama a rotina responsavel pela correcta inserção, respeitando os relocios logicos */
                orderlyFillList( msid, amsg, clock );
                (msid->msgs->number) ++;
            }
        }
    }   
    return;
}
  


/***********************************************************************
* createMsgServ
*
* Arguments: Estrutura principal do servidor de mensagens, estrutura da 
*            mensagem a inserir e o valor do relogio logico
* Returns: void
* Description: Rotina responsavel pela correcta inserção, respeitando
*             os relocios logicos
*
**********************************************************************/
void orderlyFillList( MsgServ* msid, MsgInfo* amsg, int clock ){

    MsgInfo *aux, *aux2;
    
    /* Adicionar o novo bloco no inicio */
    if( msid->msgs->beg->clock >= clock ){
        msid->msgs->beg->prv = amsg;
        amsg->next = msid->msgs->beg;
        msid->msgs->beg = amsg;
    }

    /* Adicionar o novo bloco no final */
    else if ( msid->msgs->end->clock < clock){
        msid->msgs->end->next = amsg;
        amsg->prv = msid->msgs->end;
        msid->msgs->end = amsg;
    }

    /* Adicionar o novo bloco a meio */
    else{
        aux2=msid->msgs->beg;
        while( aux2 != NULL ){
            if( aux2->clock >= clock ) break;
            aux2 = aux2->next;
        }
        aux = aux2->prv;
        aux->next = amsg;
        amsg->prv = aux;
        amsg->next = aux2;
        aux2->prv = amsg;
    }

    return;
}



/***********************************************************************
* createMsgServ
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: Ponteiro para estrutura do servidor criado
* Description: Funcao responsavel por criar uma estrutura para um servidor
*            de mensagem que se tente ligar por TCP.
*
**********************************************************************/
MsgServ* createMsgServ( MsgServ* msid, int fd, int newfd  ){
    
    struct timeval timeout;
    fd_set rfds;
    int n, counter, port;
    char buffer[500], ip[20], name[50], upt[20], tpt[20] ;
    char *aux, *aux2;
    unsigned int addrlen, addrlen2;
    struct sockaddr_in addr, addr2;
    
    memset( (void*) &addr, (int)'\0', sizeof(addr) );  
    addr.sin_addr = *msid->siip ;
    addr.sin_port = htons(msid->sipt);
    addr.sin_family = AF_INET;
    
    /* Envio de um pedido ao servidor de identidades para fornecer a lista de todos os servidores registados */
    addrlen = sizeof(addr);
    n=sendto(fd, "GET_SERVERS",11 ,0 ,(struct sockaddr*)&addr, addrlen);
    if(n==-1){
        fprintf(stderr, "Erro no envio de mensagem para obter lista de servidores junto do servidor de identidades.\n");
        packageExit( msid );
    }
    
    FD_ZERO(&rfds); FD_SET(fd, &rfds);
    timeout.tv_sec=3; timeout.tv_usec=0;
    
    counter = select(fd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, &timeout);
    if (counter == 0) printf("\nFailed to obtain response from identity server. If you have manually defined its IP or port, please make sure they are correct.\n");
    if (counter < 0 ){
        fprintf(stderr, "Erro na selecao do file descriptor.\n");
        packageExit( msid );
    }
    
    if(FD_ISSET(fd,&rfds) != 0 && counter != 0){
        n=recvfrom(fd, buffer, 500, 0, (struct sockaddr*)&addr, &addrlen );
        if(n < 0){
            fprintf(stderr, "Erro na rececao da mensagem com lista dos servidores do servidor de identidades.\n");
            packageExit( msid );
        }
        
        /* Obtencao do IP e porta TCP utilizada pelo servidor que se tenta registar junto de nos, estabelecendo uma ligacao TCP */
        addrlen2 = sizeof( addr2 );
        getpeername(newfd, (struct sockaddr *) &addr2, &addrlen2 );
        strcpy( ip, inet_ntoa( addr2.sin_addr ) );	
        port = ntohs( addr2.sin_port );
        
        /* Procura do IP do servidor que se tenta ligar na lista de servidores para obtencao de mais informacoes a cerca do mesmo. Como o porto de ligacao que obtemos nao corresponde ao indicado ao de identidades (que e o porto que atende a pedidos, e nao aquele que esta ligado ao outro servidor) nao podemos conclusivamente escolher por procura. No entanto por consequencia temporal, o servidor que se esta a tentar ligar a nos tem de ser o ultimo que se registou no servidor de identidades, mesmo quando varios servidores partilham o mesmo IP. Como tal e procurada a ultima ocurrencia do IP na lista recebida do servidor de identidades */
        aux2 = strrstr( buffer, ip );
        if (aux2 == NULL){
            fprintf(stderr, "New server with IP Adress %s tried to connect without first registering with the Identity Server.\n", inet_ntoa( addr2.sin_addr ));
            printf("Blocked the attempted connection of an unauthorised server %s to the private cluster.\n\nInput Command:\n", name);
            close(newfd);
            return( NULL );
        }

        aux2 --; aux=aux2;
        while( aux[0] != '\n' ) aux --; aux++; 
        n = aux2 - aux;
        if (n>=50) n = 49;
        strncpy( name, aux, n ); name[ n ] = '\0';
        
        /* Caso o nome obtido nao contenha o codigo dos servidores pertencentes ao Cluster privado, este nao e adicionado sendo a ligacao TCP terminada de imediato */
        if( strstr( name, SEC_CODE ) != NULL && strcmp( msid->name, name) != 0 ){  
            aux = strrstr( buffer, ip );
            aux = strstr( aux, ";" );
            aux++;
            aux2 = strstr( aux, ";");
            n = strlen(aux) - strlen(aux2);
            if (n>=20) n = 19;
            strncpy( upt, aux, n ); upt[n] = '\0';    
            aux2 ++;
            aux= strstr(aux, "\n");
            n = strlen(aux2) - strlen(aux);
            if (n>=20) n = 19;
            strncpy( tpt, aux2, n ); tpt[n] = '\0';
            
            printf("%s\nServer %s connected to your private message cluster.\n%s\nInput Command:\n", DES1, name, DES1);
            /* Rotina responsavel por adicionar o servidor a nossa lista */
            return addListMsgServ( msid, name, ip, upt, tpt, newfd);
    	}
        else{
            printf("Blocked the attempted connection of an unauthorised server %s to the private cluster.\n\nInput Command:\n", name);
            close(newfd);
            return( NULL );
        }
    }
    else{
        fprintf(stderr, "Erro na comunicacao com o servidor UDP para obtencao da identidade do Servidor de Mensagens a ligar.\n");
        packageExit( msid );
        exit(1);
    }
}



/***********************************************************************
* strrstr()
*
* Arguments: String y que representa a agulha e a x que representa o
*           palheiro
* Returns: Ponteiro para a ocurrencia da string y em x ou NULL caso 
*         a string y nao exista na x
* Description: Implementacao da funcao que procura a ultima ocurrencia
*             da string y na string x
*
**********************************************************************/
char* strrstr( char*x, char* y ){
    
	int m = strlen(x);
	int n = strlen(y);
    
	char*X = malloc(m+1);
    if( X == NULL){
        fprintf(stderr, "Erro na alocacao de memoria.\n");
        exit(1);
    }
    
	char*Y = malloc(n+1);
    if( Y == NULL){
        fprintf(stderr, "Erro na alocacao de memoria.\n");
        exit(1);
    }
    
	int i;
	
	for( i = 0; i<m; i++ ) X[m-1-i]=x[i]; X[m] = 0;
	for( i = 0; i<n; i++ ) Y[n-1-i]=y[i]; Y[n] = 0;

	char*Z = strstr(X, Y);
	if(Z){
		int ro = Z-X;
		int lo = ro + n - 1;
		int ol = m - 1 - lo;
		Z = x+ ol;
	} 
	free(X); free(Y);

	return Z;
}



/***********************************************************************
* createTCPServ()
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: void
* Description: Estrutura responsavel por criacao de um servidor TCP.
*             Esta marca a socket como a aceitar conecoes que cheguem
*
**********************************************************************/
void createTCPServ( MsgServ* msid ){

    struct sockaddr_in addr;
    
    if( (msid->fdtcp = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr, "Erro no estabelecimento da socket.\n");
        packageExit( msid );
    }

    memset( (void*) &addr, (int) '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl( INADDR_ANY );
    addr.sin_port = htons( msid->tpt );
    
    if( bind(msid->fdtcp, (struct sockaddr*) &addr, sizeof(addr) ) == -1 ){
        fprintf(stderr, "Erro no bind da socket para o servidor TCP.\n");
        packageExit( msid );
    }

    if( listen(msid->fdtcp, 10) == -1){
        fprintf(stderr, "Erro no estabelecimento do listen.\n");
        packageExit( msid );
    }
    
    return;
                   
}



/***********************************************************************
* connectAllServ()
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns:
* Description: Estabelece uma ligacao TCP como Cliente com todos os servidores 
*            presentes na lista. Caso esta nao seja conseguida, o servidor 
*            e removido, permitindo ao programa recuperar do erro
*
**********************************************************************/
void connectAllServ( MsgServ* msid ){
 
    int n, i=0, e=0;
    MsgServ* serv;
    struct sockaddr_in addr;
    
    printf("\nConnecting to the Message Server Cluster...\n");
    
    serv = msid->next;
    while( serv != NULL ){
        
        serv->fdtcp = socket( AF_INET, SOCK_STREAM, 0); if(serv->fdtcp==-1){
            fprintf(stderr, "Erro no estabelecimento da socket.\n");
            packageExit( msid );
        }
        
        memset( (void*)&addr, (int)'\0', sizeof(addr) );
        addr.sin_family = AF_INET;
        addr.sin_addr = *serv->ip ;
        addr.sin_port = htons( serv->tpt );
        
        n=connect( serv->fdtcp, (struct sockaddr*) &addr, sizeof(addr) );
        if( n < 0 ){
            fprintf(stderr, "Failed to handshake with the %s message server.\n",serv->name);
            removeMsgServ( msid, serv );
            i++;
        }
        else{
            printf("Established connection with %s.\n", serv->name);
            e++;
        }
        serv=serv->next;
    }
    
    if(i==0 && e==0) printf("No servers found to connect to.\n");
    else printf("\nConnected to a network of %d servers.\n", e) ;

    return ;
}



/***********************************************************************
* createUDPServ()
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: void
* Description: Cria um servidor UDP para atender a pedidos vindos do terminal.
*
**********************************************************************/
void createUDPServ( MsgServ* msid ){

    struct sockaddr_in addr;
    
    if( (msid->fdudp = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ){
        fprintf(stderr, "Erro no estabelecimento da socket.\n");
        packageExit( msid );
    }

    memset( (void*) &addr, (int) '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl( INADDR_ANY) ;
    addr.sin_port = htons( msid->upt );
    
    if ( (bind( msid->fdudp, (struct sockaddr*) &addr, sizeof(addr)) ) == -1 ){
        fprintf(stderr, "Erro no bind da socket para o servidor UDP.\n");
        packageExit( msid );
    }

    return;
}

    
    
/***********************************************************************
* fillServStr()
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: 1 caso tenha tido sucesso ou 0 em caso de falha (nao obteve 
*         resposta do servidor de identidades
* Description: Responsavel por preencher a lista de servidores com a
*             informacao retirada do servidor de identidades
*
**********************************************************************/
int fillServStr( MsgServ* msid, int fd, char* buf ){

    int n, counter;
    char* aux, *saux, buffer[100];
    unsigned int addrlen;
    fd_set rfds;
    MsgServ* serv;
    struct timeval timeout;
    struct sockaddr_in addr;
   
    memset( (void*) &addr, (int)'\0', sizeof(addr) );
    addr.sin_addr = *msid->siip ;
    addr.sin_port = htons(msid->sipt);
    addr.sin_family = AF_INET;
  
    n=sendto(fd, "GET_SERVERS",11 ,0 ,(struct sockaddr*)&addr,sizeof(addr));
    if(n==-1){
        fprintf(stderr, "Erro no envio de mensagem para obter lista de servidores junto do servidor de identidades.\n");
        packageExit( msid );
    }

    addrlen = sizeof(addr); 
    FD_ZERO(&rfds); FD_SET(fd, &rfds);
    timeout.tv_sec=3; timeout.tv_usec=0;
    
    counter = select(fd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, &timeout);
    if (counter == 0) printf("\nFailed to obtain response from identity server. If you have manually defined its IP or port, please make sure they are correct.\n");
    if (counter <0 ){
        fprintf(stderr, "Erro no select do file descriptor.\n");
        packageExit( msid );
    }
 
    if(FD_ISSET(fd,&rfds) != 0 && counter != 0){
        n=recvfrom(fd, buf, 500, 0, (struct sockaddr*)&addr, &addrlen);
        if(n < 0){
            fprintf(stderr, "Erro na rececao da mensagem com lista dos servidores do servidor de identidades.\n");
            packageExit( msid );
        }
 
        buf[n]='\0';
        aux = strstr(buf, "\n"); aux++;
        serv=msid;
 
        while(  aux[0] != '\0' ){
            saux = strstr(aux, ";");
            n = strlen(aux) - strlen(saux);
            strncpy( buffer , aux, n ); buffer[ n ] = '\0';

            /* Caso o servidor a adicionar nao tenha o codigo de seguranca, nao sera adicionado ao Cluster privado de servidores */
            if( strstr( buffer, SEC_CODE) != NULL ){
                /* Tratamento da informacao e preenchimento da estrutura */
                if ( strcmp(buffer, msid->name) != 0 ){
                    serv->next = (MsgServ*) malloc( sizeof( MsgServ ) );
                    if( serv->next == NULL){
                        fprintf(stderr, "Erro na alocacao de memoria.\n");
                        packageExit( msid );
                    }

                    serv = serv->next;
                    serv->ip = (IP*) malloc( sizeof( IP ) );     
                    serv->name = (char*) malloc( sizeof(char) * (n+1) );
                    if( serv->name == NULL || serv->ip == NULL ){
                        fprintf(stderr, "Erro na alocacao de memoria.\n");
                        packageExit( msid );
                    }

                    serv->siip = NULL; serv->msgs = NULL; serv->next = NULL;
                    serv->sipt = 0; serv->m = 0; serv->r = 0; serv->fdtcp = 0; serv->fdudp = 0;
                    strncpy( serv->name, aux, n ); serv->name[ n ] = '\0';
                    saux++; aux = strstr(saux, ";");
                    n = strlen(saux) - strlen(aux);
                    strncpy(buffer, saux, n ); buffer[ n ] = '\0';
          
                    if(!inet_aton( buffer,  serv->ip) ){
                        fprintf(stderr, "\nInvalid IP address.\n");
                        packageExit( msid );
                    }

                    aux++; saux = strstr(aux, ";");
                    n = strlen(aux) - strlen(saux);
                    strncpy( buffer, aux, n ); buffer[n] = '\0';
                    serv->upt = atoi( buffer );
                    saux++; aux = strstr(saux, "\n");
                    n = strlen(saux) - strlen(aux);
                    strncpy( buffer, saux, n ); buffer[n]='\0';      
                    serv->tpt = atoi( buffer ); aux++;
                }
                else{ aux = strstr(aux, "\n"); aux++; }
            }
            else{ aux = strstr(aux, "\n"); aux++; }
        }
        return 1;
    }
    return 0;
}



/***********************************************************************
* RecursListFree()
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: void
* Description: Rotina permite a dealocacao da memoria utilizada para as
*            informacoes relativas a lista de servidores
*
**********************************************************************/
void recursListFree( MsgServ* pont ){
    
    if(pont == NULL ) return;
    if(pont->next != NULL) recursListFree(pont->next);
    
    free(pont->ip);
    free(pont->name);
    free(pont);
    
    return;
}



/***********************************************************************
* checkArguments()
*
* Arguments: Estrutura principal do servidor de mensagens
* Returns: void
* Description: Verifica a correcao dos argumentos na chamada da funcao
*
**********************************************************************/
void checkArguments( int argc, char* argv[] ){

    int i;
    
    if(argc < 9 || argc > 17 || argc % 2 ==0 ){
        fprintf(stderr, "\nIncorrect Usage.\nUse as follows: msgserv -n name -j ip -u upt -t tpt [-i siip] [-p sipt] [-m m] [-r r]\n\n");
        exit(1);
    }
    
    /*Verificar correcao dos primeiros quatro parametros, fundamentais */
    if( strcmp(argv[1],"-n")!=0 || strcmp(argv[3],"-j")!=0 || strcmp(argv[5],"-u")!=0 || strcmp(argv[7],"-t")!=0 ){
        fprintf(stderr, "\nPlease specify the required parameters -n, -j, -u and -t.\n");
        fprintf(stderr, "Use as follows: msgserv -n name -j ip -u upt -t tpt [-i siip] [-p sipt] [-m m] [-r r]\n\n");
        exit(1);
    }
    
    /* Verifica que todas as posicoes pares tem uma das letras -n, -j, -u, -t, -i, -p, -m, -r, como tal garantindo que sao preenchidos apenas os parametros estabelecidos */
    if ( argc > 9){
        for( i=argc-1; i>8; i=i-2 ){
            if (  !(!strcmp(argv[i-1],"-i") || !strcmp(argv[i-1],"-p") || !strcmp(argv[i-1],"-m") || !strcmp(argv[i-1],"-r") ) ){
                fprintf(stderr, "\nPlease only use the specified optional parameters -i, -p, -m and -r.\n");
                fprintf(stderr, "Use as follows: msgserv -n name -j ip -u upt -t tpt [-i siip] [-p sipt] [-m m] [-r r]\n\n");
                exit(1);
            }
        }
    }
    
    printf("%s\n  Welcome to the Message Server Control Interface!   \n%s", DES1, DES1);
    
    return;
}



/***********************************************************************
* joinIDServer()
*
* Arguments: Estrutura principal do servidor de mensagens e socket
*          para comunicacao com o servidor de identidades
* Returns: void
* Description: Rotina que permite o registo do servidor de mensagens no
*             servidor de identidades
*
**********************************************************************/
void joinIDServer( MsgServ* msid, int fd){

    int n;
    char aux[200];
    struct sockaddr_in addr;

    memset( (void*) &addr, (int)'\0', sizeof(addr) );
    
    addr.sin_addr = *msid->siip;
    addr.sin_port = htons(msid->sipt);
    addr.sin_family=AF_INET;

    sprintf(aux, "REG %s;%s;%d;%d\n", msid->name, inet_ntoa(*msid->ip), msid->upt, msid->tpt);
    n=sendto(fd, aux, strlen(aux),0 ,(struct sockaddr*)&addr,sizeof(addr));
    if(n==-1){
        fprintf(stderr, "Erro no envio de mensagem para registo junto do servidor de identidades.\n");
        packageExit( msid );
    }
    
    return;
}

      
/***********************************************************************
* initListMsgServ()
*
* Arguments: argc e argv
* Returns: Estrutura principal do servidor de mensagens
* Description: Funcao permite a criacao da estrutura principal do servidor
*            de mensagens, preenchendo/a com informacao obtida da chamada 
*            da funcao msgserv ou predefinida
*
**********************************************************************/
MsgServ* initListMsgServ( int argc, char* argv[] ){

    MsgServ *new;
    struct hostent *h;
    int i;

    new = (MsgServ*) malloc( sizeof( MsgServ ) );
    if( new == NULL ){
        fprintf(stderr, "Erro na alocacao de memoria para uma nova estrutura da Lista de Servidores.\n");
        exit(1);
    }

    new->ip = (IP*) malloc( sizeof( IP ) ); 
    new->name = (char*) malloc( sizeof(char) * ( strlen(argv[2])+1 )  );  
    new->siip = (IP*) malloc( sizeof( IP ) );
    if( new->siip == NULL || new->name == NULL || new->ip == NULL ){
        fprintf(stderr, "Erro na alocacao de memoria.\n");
        exit(1);
    }
    
    new->next = NULL; new->msgs = NULL;
    new->fdudp = 0; new->fdtcp = 0;
	
    strncpy( new->name, argv[2], strlen(argv[2]) );
	(new->name)[ strlen(argv[2]) ] = '\0';

    if( !inet_aton( argv[4], new->ip) ){
        fprintf(stderr, "\nInvalid IP address.\n");
        packageExit( new );
    }

    new->upt = atoi(  argv[6] );
    new->tpt = atoi(  argv[8] );

    i = argc-1;
    if( i>9 ){
        if ( strcmp ( argv[i-1], "-r" ) == 0 ){
            new->r = atoi(  argv[i] );
            i = i -2;
        } else new->r = STD_R;

        if ( strcmp ( argv[i-1], "-m" ) == 0 ){
            new->m = atoi( argv[i] );
            i = i-2;
        } else new->m = STD_M;

        if ( strcmp ( argv[i-1], "-p" ) == 0 ){
            new->sipt = atoi( argv[i] );
            i = i-2;
        } else new->sipt = STD_SIPT;

        if ( strcmp ( argv[i-1], "-i" ) == 0 ){
            if ( !inet_aton(  argv[i], new->siip ) ){
                fprintf(stderr, "\nInvalid IP address.\n");
                packageExit( new );
            }
            i = i-2;
        } else{
            if((h=gethostbyname("tejo.tecnico.ulisboa.pt")) == NULL){
                fprintf(stderr, "Erro na obtencao do endereco IP do servidor de identidades.\n");
                packageExit( new );
            }
            new->siip = (IP*) h->h_addr_list[0];
        }
    } else{
        new->r = STD_R;
        new->m = STD_M;
        new->sipt = STD_SIPT;
        if((h = gethostbyname("tejo.tecnico.ulisboa.pt")) == NULL){
            fprintf(stderr, "Erro na obtencao do endereco IP do servidor de identidades.\n");
            packageExit( new );
        }
        new->siip = (IP*) h->h_addr_list[0];
    }
    
  
    printf( "\nNAME: %s IP: %s UPT: %d TPT: %d \n", new->name, inet_ntoa(*new->ip), new->upt, new->tpt);
    printf( "SIIP: %s SIPT: %d  M: %d  R: %d \n", inet_ntoa(*new->siip), new->sipt, new->m, new->r );
    printf("\n            %s\nInput Command:\n", INFO);
    
    return new;
}


  
/***********************************************************************
* addListMsgServ()
*
* Arguments:
* Returns: Estrutura do servidor adicionada a lista
* Description: Rotina responsavel por adicionar um servidor a lista
*
**********************************************************************/
MsgServ* addListMsgServ( MsgServ* msid, char* name, char* ip, char* upt, char* tpt, int fdtcp){

    MsgServ *new, *aux;

    new = (MsgServ*) malloc( sizeof( MsgServ ) );
    if( new == NULL ){
        fprintf(stderr, "Erro na alocacao de memoria para uma nova estrutura da Lista de Servidores.\n");
        packageExit( msid );
    }

    new->name = (char*) malloc( sizeof(char) * (strlen(name)+1) );
    if( new->name == NULL){
        fprintf(stderr, "Erro na alocacao de memoria.\n");
        packageExit( msid );
    }
    
    strcpy( new->name, name);
    
    new->ip = (IP*) malloc( sizeof( IP ) );
    if( new->ip == NULL){
        fprintf(stderr, "Erro na alocacao de memoria.\n");
        packageExit( msid );
    }
    
    if( !inet_aton( ip, new->ip) ){
        fprintf(stderr, "\nInvalid IP address.\n");
        packageExit( msid );
    }
    
    new->upt = atoi(  upt );
    new->tpt = atoi(  tpt );
    new->siip = NULL;
    
    new->sipt = 0;
    new->m = 0;
    new->r = 0;
    
    new->fdtcp = fdtcp;
    new->fdudp = 0;
    new->msgs = NULL;
    new->next=NULL;
    
    aux = msid;
    while(aux->next != NULL) aux=aux->next;
    aux->next = new;

    return new;
}
