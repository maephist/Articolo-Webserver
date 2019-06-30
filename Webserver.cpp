/*-----------------------------------------------------------------
Progetto: Webserver
Autore: Andrea Buzzi
Classe: 4AITI IIS Cobianchi
Docente: Vito Fausto Donato
Sistemi operativi supportati: O.S. deritvati da Linux
-----------------------------------------------------------------*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXCLIENT 5
#define BYTES 1024
#define DIM 99999
#define SIGUSR1 10

using namespace std;

typedef sockaddr_in Sockaddr_in;

struct cldescr		//Struttura del socket descriptor
{
	pid_t pid;
	int socket;
	bool libero;
};

char *ROOT;
int listenfd, pid, exitstatusfiglio,nclient;
cldescr vet[MAXCLIENT];				//Vettori con i socket

void error(char *);
int assegna();
void startServer(int);
void respond(int);
void GestoreSegnale(int);
void DisattivaSegnale(int);
void estrazione(char *, char *);
int estraiposizione(char *, char *);

int main()
{
	int PORT,slot;

	slot = 0;

	ROOT=(char *)malloc (50*sizeof(char));			//Estraggo l'indirizzo necessario alla trasmissione della pagina web
	ROOT = getenv("PWD");
	PORT=13002;

	printf("Server inizializzato alla porta: %d directory: %s\n",PORT,ROOT);

    for(int i=0;i<MAXCLIENT;i++)		//Libero il vettore
    {
		vet->socket = 0;
		vet->libero = true;
    }

    startServer(PORT);								//Inizializzo il segnale alla porta selezionata
    signal(SIGUSR1, GestoreSegnale);				//Rendo il programma sensibile al segnale SIGUSR1

    printf("In attesa di connessione...\n");

    while(1)					//Ciclo in cui accetto connessioni
    {
    	if((slot = assegna()) != 1)			//Cerco l'indice del vettore se possibile per accettare il socket descriptor
    	{
			vet[slot].socket=accept(listenfd,(struct sockaddr *)NULL,NULL);			//Accettazione della connessione

			if(vet[slot].socket>0)			//ACcettazione andata a buon fine
			{
				printf("Connessione effettuata numero: %d\n",slot+1);
				nclient++;

				if((vet[slot].pid=fork())==0)			//Creazione del processo figlio
				{
					 signal(SIGUSR1,DisattivaSegnale);			//Disattivazione della sensibiltà del segnale nel figlio
					 respond(slot);								//Trasmissione dellla pagina
					 kill(getpid(),SIGUSR1);
					 exit(slot);
				}
			}
			else 				//Errore nell'accettazione della connessione 
			{
				perror("\nAccept() error... ");
			}
			printf("Rientro dopo la fork...\n");

        }
    }

    return 0;
}

void startServer(int port)			//Funzioni di libreria che permettono l'inizializzazione del webserver
{
    Sockaddr_in *clientArrivo=(Sockaddr_in*) malloc(sizeof(Sockaddr_in));

    clientArrivo->sin_family=AF_INET;
    clientArrivo->sin_addr.s_addr=htonl(INADDR_ANY);
    clientArrivo->sin_port=htons(port);

    listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd==-1)
    {
        perror("socket() error...");
        exit(1);
    }

    if(bind(listenfd,(struct sockaddr*)clientArrivo,sizeof(*clientArrivo)))
    {
        perror("bind() error...");
        exit(2);
    }

    if(listen(listenfd,MAXCLIENT)!=0)
    {
        perror("listen() error...");
        exit(3);
    }
}

void DisattivaSegnale(int nsegnale)
{

}

void GestoreSegnale(int nsegnale)   //Funzione di gestione del segnale
{
	signal(SIGUSR1, DisattivaSegnale);			//Disattivo la sensibiltà al segnale

	unsigned short idn=0;

	waitpid(-1,&exitstatusfiglio,0);			//Identifico il processo figlio che ha terminato
	idn=exitstatusfiglio >> 8;					//Eseguo uno shift di 8 bit per recuperare l'indice del figlio nel vettore

	printf("\nLiberato slot %d \n", idn);

	vet[idn].libero=true;			//Azzero la sentinella
	nclient--; 

	signal(SIGUSR1, GestoreSegnale);
}

int assegna()
{
	int i=0;

	while(i<MAXCLIENT)			//Scorro il vettore alla ricera cel primo poso liber
	{
		if(vet[i].libero==true)		//Posto libero
		{
			vet[i].libero=false;		//Occupo il posto
			return i;					//Restituisco l'indice
		}

		i++;
	}

	return -1;
}

void respond(int slot)
{
	char messaggio[DIM], sito[DIM], datifilehtml[DIM];
	int bytericevuti = 0, byteletti=0, fd;
	char *PATH;

	PATH=getenv("PWD");
	memset((void *)messaggio, (int)'\0', DIM);
	memset((void *)sito, (int)'\0', DIM);
	bytericevuti = recv(vet[slot].socket, messaggio, DIM, 0);
	estrazione(messaggio, sito);			//Estraggo dalla stringa ricevuta dal client l'indirizzo della pagina web

	if(strcmp(sito,"/")==0)					//Se non è presente alcun indirizzo di default punto al sito index.html
	{
		strcat(PATH,"/index.html");
	}
	else
	{
		strcat(PATH,sito);
	}

	printf("\n%s\n",PATH);

	if((fd=open(PATH,O_RDONLY))!=-1)			//Trasferimento della pagina
	{
		printf("\nFile aperto: inizio trasferimento pagina!\n");

		send(vet[slot].socket, "HTTP/1.0 200 OK\n\n",17,0);

		while((byteletti=read(fd,datifilehtml,BYTES))>0)
		{
			write(vet[slot].socket,datifilehtml,byteletti);
		}

		close(fd);

		printf("\nFile chiuso: fine trasferimento pagina!\n");
	}
	else			// Nessun file da spedire!ERRORE 404
	{
		write(vet[slot].socket,"HTTP/1.0 404 Not found\n\n",23);
		printf("\nFile non trovato!\n");
	}

	shutdown(vet[slot].socket,SHUT_RDWR);			////Chiusura del socket
}

void estrazione(char msg[],char sito[])			//Chiusura del socket
{
	char stringa1[]={"GET "},stringa2[]={" HTTP"};
	int indice,conta=0;

	for(indice=estraiposizione(msg,stringa1);indice<(estraiposizione(msg,stringa2)-strlen(stringa2));indice++)
    {
        sito[conta]=msg[indice];
		conta++;
    }
}

int estraiposizione(char str1[], char str2[])
{
    int i=0,j=0;
	bool start=false;

	while(i<strlen(str1))
	{
	    j=0;
	    if(str1[i]==str2[j])
        {
            start=true;
			i++;
			j++;
        }
        if(start)
        {
            while(j<strlen(str2))
            {
                if(str2[j]!=str1[i])
                {
                    start=false;
					i-=j;
                    break;
                }
                else
                {
                    i++;
                    j++;
                }
            }

            if(start)
            {
                return i;
            }
        }
		i++;
	}
	return i;
}
