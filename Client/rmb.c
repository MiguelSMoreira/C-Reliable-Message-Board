/***********************************************************************
* File Name: .c
*
* Author:    Ana Catarina Sa (ist178665) e Miguel Moreira (ist181048)
* Data:      21 de Novembro de 2015
*
* NAME       RELIABLE MESSAGE BOARD (Projecto de RCI 2017)
*
* DESCRIPTION Programa resposavel por implementar o terminal de mensagens
*            do sistema de comunicacao fiavel entre utilizadores.
*
* ************************************************************************/
#include "rmb.h"


/***********************************************************************
 * main()
 *
 * Arguments: argc e argv
 *
 **********************************************************************/
int main(int argc, char *argv[]){
    
    MsgServ *msid=NULL;
    TrmStr *trmid=NULL;
    int fd, wh=1, ret;
    char buf[500];
    
    /* Verificacao da correcao dos argumentos de chamada da funcao */
    checkArguments( argc, argv );
    
    /* Inicializacao e preenchimento da estrutura base do terminal */
    trmid = initTerminal( argc, argv );
    
    /* File Descriptor da socket UDP para ligacao ao das identidades */
    fd = socket( AF_INET, SOCK_DGRAM, 0 ); if(fd==-1) exit(1);

    /* Preenchimento da lista de servidores conhecidos ao terminal */
    ret = fillServStr( &msid, trmid, fd );

    /* Impressao de informacao adicional */
    printfInfo( msid, trmid, ret);
    
    /* Inicio */
    printf("\nInput Command:\n");
    while( wh == 1 ){
        
        fgets( buf, 500, stdin);
        
        if( !strcmp( buf, SHOWS) ||  !strncmp( buf, PBLSH, 7) || !strncmp( buf, SHOWM, 20) || !strcmp( buf, EXIT ) ){
            
            /* Rotina responsavel por lidar com instrucoes para exibir a lista dos servidores de mensagens conhecidos pelo terminal */
            if ( !strcmp( buf, SHOWS) ){
                /* Inicializa ou actualiza a lista de servidores conhecidos, pedindo a informacao ao servidor de identidades */
                ret = fillServStr( &msid, trmid, fd );
                /* Imprime a informacao dos servidores no terminal */
                printServers( msid, ret);
            }
            /* Rotina responsavel por lidar com instrucoes para a publicacao de mensagens pelo terminal */
            else if ( !strncmp( buf, PBLSH, 7) ) pblshRoutine( msid, trmid, buf );

            /* Rotina responsavel por lidar com instrucoes para a exibicao das mensagens conhecidas pelo grupo de servidores de mensagens no terminal */
            else if ( !strncmp( buf, SHOWM, 20) ) showMsgRoutine( msid, trmid, buf);
            
            else if ( !strcmp( buf, EXIT ) ){
                printf("%s\n        Thank you for using the RMB Chat!\n%s\n", DES1, DES1);
                wh = 0;
            }
            
        }
        else printf("\nIncorrect usage of the user interface. %s", INFO);
        
        if ( strcmp(buf,EXIT) != 0 ) printf("\nInput Command:\n");
    }
    
    closeAll( msid, trmid, fd);
    return 0;
}



/***********************************************************************
 * pblshRoutine()
 *
 * Arguments:
 * Returns:
 * Description:
 *
 **********************************************************************/
void pblshRoutine( MsgServ* msid, TrmStr* trmid, char* buf ) {

    int m, n, i;
    struct sockaddr_in addr;
    char buffer[149], *aux, *aux2, *aux3;
    
    if(msid==NULL) printf("\nNo available message servers to attend to your request.\nTo register use command 'show_servers' in the Terminal.\n");
    else{
        aux = strstr( buf, " ");
        aux2 = strstr( buf, "\n");
        aux2[0] = '\0';
        
        if(aux != NULL){
            aux++;
            n = aux2 - aux;
        
            if(n>140) printf("\nThe service only supports messages up to 140 characters in length.\n");
            else if(n>0){
                strncpy(buffer, "PUBLISH ", 8);
                buffer[8] = '\0';
                strcat( buffer, aux );
            
                memset( (void*) &addr, (int)'\0', sizeof(addr) );
                addr.sin_addr = *msid->ip ;
                addr.sin_port = htons(msid->upt);
                addr.sin_family = AF_INET;
            
                m=sendto(msid->fd, buffer, n+8 ,0 ,(struct sockaddr*)&addr, sizeof(addr));
                if(m==-1){
                    fprintf(stderr, "Erro no envio de mensagem para obter lista de servidores junto do servidor de identidades.\n");
                    packageExit( msid, trmid);
                }
            
                aux2 = NULL;
                aux3 = getNMsg( msid, trmid, 3);
                if (aux3 != NULL) {
                    aux2 = strstr( buffer , aux );
                    if ( aux2 != NULL) printf("\nYour message was correctly uploaded to the message server.\n");
                    else{
                        for( i=0 ; i<4; i++){
                            m=sendto(msid->fd, buffer, n+8 ,0 ,(struct sockaddr*)&addr, sizeof(addr));
                            if(m==-1){
                                fprintf(stderr, "Erro no envio de mensagem para obter lista de servidores junto do servidor de identidades.\n");
                                packageExit( msid, trmid);
                            }
                        
                            aux3 = getNMsg( msid, trmid, 3);
                            if( aux3 != NULL ){
                                aux2 = strstr( buffer , aux );
                                if( aux2 != NULL ){
                                    printf("\nYour message was correctly uploaded to the message server.\n"); break;
                                }
                            }
                            else printf("We were unable to connect to the message server. We recommend you to update the message server to which you are connected. You can do so using the 'show_servers' command.\n");
                        }
                    }
                    free(aux3);
                }
                else printf("We were unable to connect to the message server. We recommend you to update the message server to which you are connected. You can do so using the 'show_servers' command.\n");
                if( aux2 == NULL) printf("We were unable to determine wether the message was correctly uploadded to the message server.\n");
            }
            else printf("Incorrect usage of the interface.\nThe user must specify a [message] with size diferent of zero.\n");
        }
        else printf("Incorrect usage of the interface.\nThe user must specify a [message] to publish.\n");
    }
    return;
}



/***********************************************************************
 * showMsgRoutine()
 *
 * Arguments:
 * Returns:
 * Description: 
 *
 **********************************************************************/
void showMsgRoutine( MsgServ* msid, TrmStr* trmid, char* buf ){
    
    int n, i, m;
    char *aux, *aux2, *aux3, buffer[500] ;
    
    if( msid == NULL ) printf("\nNo available message servers to attend to your request.\nTo register use command 'show_servers' in the Terminal.\n");
    else{
    
        aux = strstr( buf, " ");
        if( aux != NULL ) aux ++;
        aux2 = strstr( buf, "\n");
        aux2[0] = '\0';
        n = atoi(aux);
    
        if( aux != NULL && (n = atoi(aux)) != 0 ){
            aux3 = getNMsg( msid, trmid, n);
            if( aux3 != NULL){
                /* Impressao no terminal dos resultados obtidos */
                printf("%s", INFO3);
                aux = strstr( aux3, "\n"); aux++;
                aux2 = strstr( aux, "\n");
                i=1;
            
                while( aux2 != NULL ){
                    m = strlen(aux) - strlen(aux2);
                    strncpy(buffer, aux, m);
                    buffer[m] = '\0';
                    printf("MESSAGE %d: %s\n", i, buffer);
                
                    aux = aux2; aux++;
                    aux2 = strstr( aux, "\n");
                    i++;
                }
            
                printf("%s", DES1);
                i --;
                if( i != n) printf("\nOnly %d of the %d requested messages were printed, as there are no more available\nin the message servers\n", i, n);
            
                free(aux3);
            }
            else printf("\nError in retrieving messages from the message server.\n");
        }
        else printf("\nIncorrect usage of the user interface.\nYou must specify a number [n] of messages to show.\n");
    }
    
    return;
}



/***********************************************************************
 * printfInfo()
 *
 * Arguments:
 * Returns:
 * Description:
 *
 **********************************************************************/
void printfInfo( MsgServ* msid, TrmStr* trmid, int ret){

    printf("%s\n    Welcome to the Message Terminal Interface!   \n%s\n         SIIP: %s SIPT: %d\n\n            %s", DES1, DES1, inet_ntoa(*trmid->siip), trmid->sipt, INFO);
    
    
    if( ret==0) printf("\nFailed in the communication with the message server.\n");
    if (ret==1 && msid==NULL) printf("\nNo available message servers in the message server to attend to your request. Please try again later.\nTo register use command 'show_servers' in the Terminal.\n");
    if(ret==1 && msid!=NULL) printf("\nYou have been connected to the message server %s with IP %s and Port %d.\n", msid->name, inet_ntoa( *msid->ip), msid->upt );

    return;
}



/***********************************************************************
 * getNMsg()
 *
 * Arguments:
 * Returns:
 * Description:
 *
 **********************************************************************/
char* getNMsg( MsgServ* msid, TrmStr *trmid, int n ){

    int m, counter;
    char* aux3;
    fd_set rfds;
    char buf[20];
    struct sockaddr_in addr;
    unsigned int addrlen;
    struct timeval timeout;
    
    sprintf(buf, "GET_MESSAGES %d", n);
        
    memset( (void*) &addr, (int)'\0', sizeof(addr) );
    addr.sin_addr = *msid->ip ;
    addr.sin_port = htons(msid->upt);
    addr.sin_family = AF_INET;
        
    m=sendto(msid->fd, buf, strlen(buf), 0,(struct sockaddr*)&addr, sizeof(addr));
    if(m==-1){
        fprintf(stderr, "Erro no envio de mensagem para obter lista de mensagens junto do servidor de identidades.\n");
        packageExit( msid, trmid);
    }
        
    addrlen = sizeof(addr);
    
    FD_ZERO(&rfds);
    FD_SET(msid->fd, &rfds);
    
    timeout.tv_sec=3;
    timeout.tv_usec=0;
    
    counter = select(msid->fd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, &timeout);
    if (counter == 0){
        printf("\nFailed to obtain response from message server.\n");
        return NULL;
    }
    if (counter < 0 ) packageExit( msid, trmid);
    
    if(FD_ISSET(msid->fd,&rfds) && counter != 0){
            
        aux3 = (char*) malloc( sizeof(char) *  (9+ (141 * n) ));
        if (aux3==NULL){
            fprintf(stderr, "Falha no alocamento de memoria para o buffer de rececao das mensagens.\n");
            packageExit( msid, trmid);
        }
            
        memset( (void*) aux3, (int)'\0', sizeof(aux3) );
            
        m=recvfrom(msid->fd, aux3, 9+(141*n), 0, (struct sockaddr*)&addr, &addrlen);
        if(m==-1){
            fprintf(stderr, "Erro na rececao da mensagem com lista dos servidores do servidor de identidades.\n");
            packageExit( msid, trmid);
        }
        
        return aux3;
    }
    
    return NULL;
}



/***********************************************************************
 * packageExit()
 *
 * Arguments:
 * Returns:
 * Description: 
 *
 **********************************************************************/
void packageExit( MsgServ* msid, TrmStr* trmid){
    
    MsgServ* aux;
    
    if( msid == NULL ){
        aux = msid;
        while( aux != NULL ){
            close( aux->fd );
            aux = aux->next;
        }
    }
    
    if ( msid != NULL ) RecursListFree( msid );
    if (trmid->siip != NULL ) free(trmid->siip);
    if (trmid != NULL ) free(trmid);
    
    return;
}



/***********************************************************************
 * closeAll()
 *
 * Arguments:
 * Returns:
 * Description:
 *
 **********************************************************************/
void closeAll( MsgServ* msid, TrmStr* trmid, int fd){

    MsgServ* aux;
    
    close( fd );

    aux = msid;
    while( aux != NULL ){
        close( aux->fd );
        aux = aux->next;
    }

    RecursListFree( msid );
    //free(trmid->siip);
    free(trmid);

    return;
}



/***********************************************************************
 * printServers()
 *
 * Arguments:
 * Returns:
 * Description:
 *
 **********************************************************************/
void printServers( MsgServ* msid, int ret){

    MsgServ* aux;
    int i;
    
    if (ret != 0){
        aux = msid;
        i=1;
        printf("%s", INFO2);
        while( aux != NULL){
            printf("                   SERVER %d:\nNAME: %s IP: %s UPT: %d\n", i, aux->name, inet_ntoa(*aux->ip), aux->upt);
            i++;
            aux = aux->next;
        }
        if( i == 1 ) printf("       NO AVAILABLE SERVERS IN THE CLUSTER.\n");
            printf("%s", DES1);
    }

    return;
}



/***********************************************************************
 * fillServStr()
 *
 * Arguments:
 * Returns:
 * Description: return 0, nao recebeu resposta, return 1, recebeu
 *
 **********************************************************************/
int fillServStr( MsgServ** msid, TrmStr* trmid, int fd ){
    
    int n, i, counter;
    char* aux, *saux, buffer[100], buf[500];
    unsigned int addrlen;
    fd_set rfds;
    MsgServ *serv, *aserv;
    struct timeval timeout;
    struct sockaddr_in addr;
    
    memset( (void*) &addr, (int)'\0', sizeof(addr) );
    addr.sin_addr = *trmid->siip ;
    addr.sin_port = htons(trmid->sipt);
    addr.sin_family = AF_INET;
    
    n=sendto(fd, "GET_SERVERS",11 ,0 ,(struct sockaddr*)&addr,sizeof(addr));
    if(n==-1){
        fprintf(stderr, "\nErro no envio de mensagem para obter lista de servidores junto do servidor de identidades.\n");
        packageExit( *msid, trmid);
    }
    
    addrlen = sizeof(addr);
    FD_ZERO(&rfds); FD_SET(fd, &rfds);
    timeout.tv_sec=3; timeout.tv_usec=0;
    
    counter = select(fd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, &timeout);
    if (counter == 0){
        printf("\nFailed to obtain response from identity server. If you have manually defined its IP or port, please make sure they are correct.\n");
        return 0;
    }
    if (counter < 0 ) packageExit( *msid, trmid);
    
    if(FD_ISSET(fd,&rfds) && counter != 0){
        n=recvfrom(fd, buf, 499, 0, (struct sockaddr*)&addr, &addrlen);
        if(n<0){
            fprintf(stderr, "Erro na rececao da mensagem com lista dos servidores do servidor de identidades.\n");
            packageExit( *msid, trmid);
        }
        
        buf[n]='\0';
        if (n==499){
            fprintf(stderr, "Buffer overflow!\nThe identity server returned too many message servers to proccess.\n");
            packageExit( *msid, trmid);
        }
        
        aux = strstr(buf, "\n");
        aux++;
        if(*msid != NULL){
            RecursListFree( *msid );
            *msid = NULL;
        }
        
        i = 0;
        while( strcmp( aux, "\0" ) != 0 ){
            saux = strstr(aux, ";");
            n = strlen(aux) - strlen(saux);
            strncpy( buffer , aux, n ); buffer[ n ] = '\0';
            
            if( strstr( buffer, SEC_CODE) != NULL ){
                serv = (MsgServ*) malloc( sizeof( MsgServ ) );
                if( serv == NULL){
                    fprintf(stderr, "Erro na alocacao de memoria.\n");
                    packageExit( *msid, trmid);
                }
                
                if( i == 0 ) *msid = serv;
                serv->ip = (IP*) malloc( sizeof( IP ) );
                serv->name = (char*) malloc( sizeof(char) * (n+1) );
                if( serv->ip == NULL || serv->name == NULL){
                    fprintf(stderr, "Erro na alocacao de memoria para uma nova Estrutura de Terminal\n");
                    packageExit( *msid, trmid);
                }
                
                serv->next = NULL;
                serv->fd = socket( AF_INET, SOCK_DGRAM, 0 ); if(serv->fd==-1) packageExit( *msid, trmid);
                strncpy( serv->name, aux, n ); serv->name[ n ] = '\0';
                saux++;
                aux = strstr(saux, ";");
                n = strlen(saux) - strlen(aux);
                strncpy(buffer, saux, n ); buffer[ n ] = '\0';
                
                if(!inet_aton( buffer,  serv->ip) ){
                    fprintf(stderr, "Invalid IP address.\n");
                    packageExit( *msid, trmid);
                }
                
                aux++;
                saux = strstr(aux, ";");
                n = strlen(aux) - strlen(saux);
                strncpy( buffer, aux, n ); buffer[n] = '\0';
                serv->upt = atoi( buffer );
                saux++;
                aux = strstr(saux, "\n");
                aux++;
                
                if( i != 0 ) aserv->next=serv;
                aserv = serv;
                i++;
            }
            else{ aux = strstr(aux, "\n"); aux++;  }
        }
        return 1;
    }
    return 0;
}




/***********************************************************************
 * RecursListFree()
 *
 * Arguments:
 * Returns:
 * Description:
 *
 **********************************************************************/
void RecursListFree( MsgServ* pont ){
    
    if( pont == NULL ) return;
    if( pont->next != NULL ) RecursListFree(pont->next);
    free(pont->ip);
    free(pont->name);
    free(pont);
    
    return;
}



/***********************************************************************
 * initTerminal()
 *
 * Arguments:
 * Returns:
 * Description:
 *
 **********************************************************************/
TrmStr* initTerminal( int argc, char* argv[] ){
    
    TrmStr *new;
    struct hostent *h;
  
    new = (TrmStr*) malloc( sizeof( TrmStr ) );
    new->siip = (IP*) malloc( sizeof( IP ) );
    if( new == NULL || new->siip == NULL){
        fprintf(stderr, "Erro na alocacao de memoria para uma nova Estrutura de Terminal\n");
        packageExit( NULL, new);
    }

    if ( argc == 1){
        if((h=gethostbyname("tejo.tecnico.ulisboa.pt")) == NULL) packageExit( NULL, new);
        new->siip = (IP*) h->h_addr_list[0];
        new->sipt = STD_SIPT;
    }
    
    else if ( argc == 3){
        if ( !strcmp ( argv[ argc - 2], "-i" ) ){
            if ( !inet_aton(  argv[ argc - 1 ], new->siip ) ){
                fprintf(stderr, "Invalid IP address.\n");
                packageExit( NULL, new);
            }
        }
        else{
            if((h=gethostbyname("tejo.tecnico.ulisboa.pt")) == NULL) packageExit( NULL, new);
            new->siip = (IP*) h->h_addr_list[0];
        }
        
        if ( !strcmp ( argv[ argc - 2 ], "-p" ) ) new->sipt = atoi( argv[ argc - 1] );
        else new->sipt = STD_SIPT;
    }
    else if ( argc == 5){
        if ( !strcmp ( argv[ argc - 2], "-p" ) ){
            new->sipt = atoi( argv[ argc - 1 ] );
        }
        else{
            fprintf(stderr, "\nPlease only the specified optional parameters in the correct order.\nUse as follows: rmb [-i siip] [-p sipt]\n\n");
            packageExit( NULL, new);
        }
        
        if ( !strcmp ( argv[ argc - 4], "-i" ) ){
            if ( !inet_aton(  argv[ argc - 3 ], new->siip ) ){
                fprintf(stderr, "Invalid IP address.\n");
                packageExit( NULL, new);
            }
        }
        else{
            fprintf(stderr, "\nPlease only the specified optional parameters in the correct order.\nUse as follows: rmb [-i siip] [-p sipt]\n\n");
            packageExit( NULL, new);
        }
    }
    
    return new;
}

    
/***********************************************************************
 * checkArguments()
 *
 * Arguments:
 * Returns:
 * Description:
 *
 **********************************************************************/
void checkArguments( int argc, char* argv[] ){
    
    int i;
    
    if(argc < 1 || argc > 5 || argc % 2 == 0 ){
        fprintf(stderr, "\nIncorrect Usage.\nUse as follows: rmb [-i siip] [-p sipt]\n\n");
        exit(1);
    }

    if ( argc > 1){
        for( i=argc-1; i>1; i=i-2 ){
            if (  !(!strcmp(argv[i-1],"-i") || !strcmp(argv[i-1],"-p") ) ){
                fprintf(stderr, "\nPlease only use the specified optional parameters -i, -p.\n");
                fprintf(stderr, "Use as follows: rmb [-i siip] [-p sipt]\n\n");
                exit(1);
            }
        }
    }
    
    return;
}
