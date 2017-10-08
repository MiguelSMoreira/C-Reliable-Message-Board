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

/* Inclusões */
#ifndef rmb_h
#define rmb_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>


/* Definicoes*/
#define STD_SIPT 59000
#define SEC_CODE "CLST"
#define PBLSH "publish"
#define SHOWS "show_servers\n"
#define SHOWM "show_latest_messages"
#define EXIT "exit\n"
#define DES1 "\n-----------------------------------------------------\n"
#define INFO "Use the following commands:\n'show_servers' to list all server identities,\n'publish [message]' to publish a [message] in the message servers,\n'show_latest_messages [n]' to show the latest [n] messages stored,\n'exit' to close the application.\n"
#define INFO2 "\n----------------- SERVER LIST -----------------------\n\n"
#define INFO3 "\n----------------- MESSAGE LIST ----------------------\n\n"

/* Estruturas */
typedef struct in_addr IP;

/* Estrutura base do terminal */
typedef struct TrmStruct TrmStr;
struct TrmStruct{
    IP* siip;
    int sipt;
};

/* Estrutura base de cada servidor de mensagens */
typedef struct MsgServerID MsgServ;
struct MsgServerID{
    char* name;
    IP* ip;
    int upt;
    int fd;
    MsgServ* next;
};

/* Prototipos funcoes */
void checkArguments( int, char** );
TrmStr* initTerminal( int, char** );
int fillServStr( MsgServ**, TrmStr*, int );
void RecursListFree( MsgServ* );
void printServers( MsgServ*, int);
void closeAll( MsgServ*, TrmStr*, int );
void packageExit( MsgServ*, TrmStr* );
char* getNMsg( MsgServ*, TrmStr *, int );
void printfInfo( MsgServ*, TrmStr*, int);
void showMsgRoutine( MsgServ* , TrmStr* , char* );
void pblshRoutine( MsgServ*, TrmStr*, char* );


#endif /* rmb_h */
