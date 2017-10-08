/***********************************************************************
 * File Name: .h
 *
 * Author:    Ana Catarina Sa (ist178665) e Miguel Moreira (ist181048)
 * Data:      21 de Novembro de 2015
 *
 * NAME       RELIABLE MESSAGE BOARD (Projecto de RCI 2017)
 *
 * DESCRIPTION
 *
 * ************************************************************************/

/* Inclusoes */
#ifndef msgservheader
#define msgservheader

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

/* Definicoes */
#define STD_R 10
#define STD_M 200
#define STD_SIPT 59000
#define JOIN "join\n"
#define SHOWS "show_servers\n"
#define SHOWM "show_messages\n"
#define SMESS "SMESSAGES\n"
#define SGETM "SGET_MESSAGES\n"
#define DES1 "\n-----------------------------------------------------\n"
#define EXIT1 "\n          Thank you for using MsgServer!\n"
#define EXIT "exit\n"
#define SEC_CODE "CLST"
#define MAX( a, b ) ( ( a > b) ? a : b )
#define ID1 "\n    You are now connected to the Identity Server!\n               Enjoy our service.\n"
#define INFO "Use the following commands:\n'join' to register the Message Server with the ID Server,\n'show_servers' to list all server identities,\n'show_messages' to list all saved messages,\n'exit' to close the application.\n"
#define INFO2 "\n----------------- SERVER LIST -----------------------\n\n"
#define INFO3 "\n---------------- MESSAGE LIST ----------------------\n\n"
#define INFO4 "\nYou are required to register your server before accessing stored messages or view active servers.\nUse 'join' command to register.\n"
#define INFO5 "\nYou cannot connect to the Network multiple times.\nIf you wish to disconnect, please use command exit.\n"

/* Estruturas */
typedef struct in_addr IP;

/* Estrutura corresponde a cada mensagem guardada */
typedef struct Messages MsgInfo;
struct Messages{
    int clock;
    char *msg;
    MsgInfo *next;
    MsgInfo *prv;
};

/* Estrutura base das mensagens */
typedef struct MsgStruct MsgStr;
struct MsgStruct{
    int number;        /* Numero de mensagens guardadas */
    MsgInfo* beg;
    MsgInfo* end;
};

/* Estrutura base do servidor de mensagens */
/* O fd_tcp do primeiro elemento corresponde ao porto do servidor que esta a atender pedidos de sessao dos outros servidores de mensagens e o fd_udp do primeiro elemento corresponde ao porto que atende a pedidos dos terminais*/
/* Os fd_tcp dos restantes elementos correspondem aos descritores das ligacoes estabelecidads com os outros servidores de mensagens. Os restantes fd_udp nao se encontram utilizados */
typedef struct MsgServerID MsgServ;
struct MsgServerID{
    char* name;
    IP* ip;
    int upt;
    int tpt;
    IP* siip;
    int sipt;
    int m;
    int r;
    int fdtcp;
    int fdudp;
    MsgStr* msgs;
    MsgServ* next;
};

/* Prototipos funcoes */
MsgServ* addListMsgServ( MsgServ*, char*, char*, char*, char*, int);
MsgServ* initListMsgServ( int, char** );
MsgServ* createMsgServ( MsgServ*, int, int );
void checkArguments( int , char** );
void joinIDServer( MsgServ*, int );
int fillServStr( MsgServ*, int, char*);
void recursListFree( MsgServ* );
void createUDPServ( MsgServ* );
void connectAllServ( MsgServ* );
void createTCPServ( MsgServ* );
void commsTCP( MsgServ* , MsgServ* , int*);
void initMsgList( MsgServ* , int*);
void fillMsgList( MsgServ* , char* , int, int);
char* strrstr( char*, char* );
void sendNMsgs( MsgServ*, struct sockaddr_in*, int );
void broadcast( MsgServ*, char* , int , int );
void recursMessFree( MsgInfo* );
void freeSTRs( MsgServ* );
void closeFDs( MsgServ* );
void removeMsgServ( MsgServ*, MsgServ* );
void orderlyFillList( MsgServ* , MsgInfo*, int );
void packageExit( MsgServ* );
int refreshSelect( MsgServ*, int*, time_t*, time_t*, struct timeval*, fd_set* );
void refreshUDPServ( MsgServ*, int* );
void refreshShowCmd( MsgServ*, char* );
void refreshTCPServ( MsgServ*, int );


#endif
