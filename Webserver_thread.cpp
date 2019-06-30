/*-----------------------------------------------------------------
Progetto: Webserver con thread
Autore: Simone d'Angelo
Classe: 4AITI IIS Cobianchi
Docente: Vito Fausto Donato
Sistemi operativi supportati: O.S. deritvati da Linux
-----------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>

typedef struct _ThreadParametri {
	int slotID;
}ThreadParametri;

#define MAXCLIENT 5
#define BYTES 1024
#define DIM 99999
pthread_t clientThreadID[MAXCLIENT]; //vettore dove andranno salvati i thread id
int statoThread[MAXCLIENT]; //indica se ci sono trasmissioni avviate (contiene -1 se no, contiene il tid se sì)
ThreadParametri *mioParam;//salva lo slot per ogni thread

char *ROOT;
int listenfd;
int clients[MAXCLIENT];//contiene i descrittori dei socket
void startServer(char *);
void *respond(void *);
int slot=0;//indice identificativo del thread nel vettore dei client


int main(){
    char PORT[6];
    int i;

    mioParam=(ThreadParametri*)malloc(sizeof(ThreadParametri));//alloca in memoria la struttura parametro thread (id dello slot)
    ROOT = getenv("PWD");  //carica nella stringa ROOT il path corrente
    strcpy(PORT,"14006");  //Assegnazione della porta TCP

    //Rimosso il parsing dei parametri su riga di comando

    printf("Server started porta:%s,  root directory:  %s\n",PORT,ROOT);
    for (i=0; i<MAXCLIENT; i++){ statoThread[i]=-1; clients[i]=-1;} // Setting lo stato dei client a -1: "no client connected"
    startServer(PORT);

    while (1)    { // ACCEPT delle connessioni
        printf("Accept...\n");
        clients[slot] = accept(listenfd, (struct sockaddr*)NULL, NULL);//accetta il primo slot libero
        if (clients[slot]<0)
        	perror ("Accept() error...");//errore accept
        else {
            mioParam->slotID=slot;
            statoThread[slot]=pthread_create(&(clientThreadID[slot]), NULL,respond, (void *)(mioParam));

            /*//↑ fa partire la funzione respond in un thread concorrente non bloccante con condivisione di memoria tra padre(main) e figlio (accept),
            con come argomento mioparam e salvando il thread id in clientthreadid[slot]
            //pthread_create deve ritornare 0*/

            if (statoThread[slot]!=0) {
        			perror("Errore nella creazione del thread\n");
        	}
        }
        printf("Rientro dal thread...\n");
        while (clients[slot]!=-1)
        	slot = (slot+1)%MAXCLIENT;//quando il numero di client supera la dimensione massima si riparte dall'inizio
    }
    return 0;
}

void startServer(char *port){
struct sockaddr_in *clientArrivo=(struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));//si assegna al puntatore clientarrivo la cella di memoria ritornata dalla malloc

    clientArrivo->sin_family=AF_INET;
    clientArrivo->sin_addr.s_addr = htonl(INADDR_ANY);
    clientArrivo->sin_port = htons(atoi(port)); //l'ordine dei bit usato in internet e' diverso da quello sul pc per cui si shiftano di 8 bit a sinistra

    listenfd = socket (AF_INET, SOCK_STREAM, 0);//in listenfd viene restituito il puntatore al socket
    if (listenfd==-1){ perror("socket() error"); exit(1);}//errore nella ricezione del socket
    //la bind associa al socket la porta di ascolto ed i dati relativi al client
    if (bind(listenfd, (struct sockaddr*)clientArrivo, sizeof(*clientArrivo)) == -1) {
    	  perror("bind() error");exit(1);//errore nella bind
    }
    if ( listen (listenfd, MAXCLIENT) != 0 ){ // listen sul socket
        perror("listen() error");//errore nella listen
        exit(1);
    }
}

void *respond(void *pn){ //client connection: invia la pagina web ai client
    char mesg[DIM], *reqline[3]/*puntatore a char per il parsing della stringa*/, datifilehtml[BYTES], path[DIM];
    int rcvd, fd, bytes_read,n;
    ThreadParametri* mieiPar;

    mieiPar=(ThreadParametri*)pn;
    n=mieiPar->slotID;
    printf("slot N° %d\n",n);
    memset( (void*)mesg, (int)'\0', DIM ); //setta il messaggio a vuoto
    rcvd=recv(clients[n], mesg, DIM, 0);  //riceve il messaggio mesg di dimensione DIM richiesto dal client che ha come socket clients[n], dove n è lo slotID relativo al thread
    printf("Ricezione dal browser...\n");
    if (rcvd<0)
    	fprintf(stderr,("recv() error\n"));//errore nella recv
    else if (rcvd==0)
        fprintf(stderr,"Disconnessione anomala del client\n");//errore socket chiuso
    else  {  //messaggio ricevuto

    	printf("%s", mesg);



    	reqline[0] = strtok (mesg, " \t\n");  //strtok separa le righe ogni volta che incontra \t o \n
        if ( strncmp(reqline[0], "GET\0", 4)==0 ) { //compara n caratteri di due stringhe; //nessun percorso? specificato
            reqline[1] = strtok (NULL, " \t"); //se c'è NULL al posto del puntatore a char salva la stringa che trova dopo il carattere delimiter, quindi il nome del file inviato dopo la parola riservata GET
            reqline[2] = strtok (NULL, " \t\n");  //salva versione di HTTP
            if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 ){
                write(clients[n], "HTTP/1.0 400 Richiesta errata\n", 25);//versione sbagliata di HTTP
            }
            else {  //parsing
                if ( strncmp(reqline[1], "/\0", 2)==0 )
                    sprintf(reqline[1],"%s","/index.html"); //la stringa salvata è vuota, filename non specificato: index.html aperto di default...

                sprintf(path,"%s%s",ROOT,reqline[1]); //sprintf immagazzina la stringa nel buffer puntato da path; //sprintf(*str, char* formato, char *rimpiazzostr); //immagazzina reqline in path con formato stringa
                printf("file: %s\n", path);
                if ( (fd=open(path, O_RDONLY))!=-1 ){    //FILE trovato (sola lettura)
                    printf("File aperto...\n");
                	send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0); //invia l'ok al sockid del thread, clients[n] è il sockid
                    while ( (bytes_read=read(fd, datifilehtml, BYTES))>0 ){
                        write (clients[n], datifilehtml, bytes_read);//invia il file a blocchi di 1024 byte (BYTES=1024) finchè fd non è finito
                    }
                    close(fd);
                    printf("Fine trasferimento pagina...\n");
                }
                else {
                	write(clients[n], "HTTP/1.0 404 Not Found\n", 23); //file non trovato
                	printf("File non trovato...");
                }
            }            
        }
    } //Closing SOCKET
    shutdown (clients[n], SHUT_RDWR);//Disattiva ulteriori operazioni di read/write sul socket clients[n]
    clients[n]=-1;  //libera lo slot del client con il thread vengono evitati i segnali, essendoci condivisione di memoria
    printf("Chiusura socket...\n");
    
    return NULL;
}
