/*-----------------------------------------------------------------
Progetto: Webserver con oggetti 
Autore: Andrea Buzzi
Classe: 4AITI IIS Cobianchi
Docente: Vito Fausto Donato
Sistemi operativi supportati: O.S. deritvati da Linux
Avvertenze: Funziona solo con il file "Intestazione_oggetti.h"
-----------------------------------------------------------------*/

#include "Intestazione_oggetti.h"

socdes * punta;
socdes * tmp;

char *ROOT;
int listenfd, exitstatusfiglio, nclient;

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
	int PORT,slot,storico;

	punta = new socdes;					//Creo un oggetto della classe relativa al socket_descriptor
	punta->setpnt(2,NULL);				//Lo setto vuoto

	slot = 0;
	storico=0;

	ROOT=(char *)malloc (50*sizeof(char));			//Estraggo l'indirizzo necessario alla trasmissione della pagina web
	ROOT = getenv("PWD");
	PORT=13001;

	cout<<"Server inizializzato alla porta: "<<PORT<<" e punta alla cartella: "<<ROOT<<endl<<endl;

    startServer(PORT);								//Inizializzo il webserver alla porta selezionata
    signal(SIGUSR1, GestoreSegnale);				//Sensibilizzo il server ad un segnale SIGUSR1

    cout<<"In attesa di connessione...\n";

    while(1)			//Ciclo in cui ricevo le connessioni
    {
    	if( assegna() != -1)		//Verifico se sia possibile accettare ancora delle connessioni
    	{
			tmp->setsock(accept(listenfd,(struct sockaddr *)NULL,NULL));		//Mi creo un oggetto temporaneo

			if(tmp->getsock()>0)		//Connessione e accettazione effettuata correttamente
			{
				cout<<"Connessione effettuata numero: "<<storico+1<<endl;

				storico++;			//Incremento lo storico

				tmp->setid(storico);		//Setto nell'oggetto creato in precedenza l'id
				tmp->setpid(fork());		//Apro un nuovo processo in concorrenza e metto il pid nell'oggetto

				if((tmp->getpid())==0)		//Parte valida solo per il processo figlio
				{
					 //signal(SIGUSR1,DisattivaSegnale);
					 respond(slot);					//Trasferimento della pagina sul socket
					 kill(getpid(),SIGUSR1);		//Chiudo il processo figlio
					 exit(tmp->getid());			//Returno i
				}
			}
			else			//Errore nell'accettazione della connessioni
			{
				perror("\nAccept() error... ");
			}
	   }
    }

    return 0;
}

void startServer(int port)		//Funzioni di libreria che permettono l'inizializzazione del webserver
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

void GestoreSegnale(int nsegnale)		//Funzione di gestione del segnale 
{
	signal(SIGUSR1, DisattivaSegnale);			//Disattivo la sensibiltà al segnale

	unsigned short idn=0;
	socdes * ptr, * ptr2, * ptr3;

	waitpid(-1,&exitstatusfiglio,0);		//Identifico il processo figlio che ha terminato

	idn=exitstatusfiglio >> 8;				//Eseguo uno shift di 8 bit per recuperare l'id originale 
	ptr=punta;								//Sposto il puntatore

	while(idn!=ptr->getid())				//Cerco il figlio tra i vari oggetti
	{
		ptr=ptr->getpnt(2);
		ptr2=ptr;
	}

	ptr=ptr->getpnt(1);						//Queste operazioni mi permettono di andare a stabilire il nuovo ordine degli oggetti andando ad escludere il figlio che è stato cancelllato
	ptr3=ptr;
	ptr=ptr->getpnt(1);
	ptr->setpnt(1,ptr2);
	ptr2->setpnt(2,ptr);
	delete ptr3;
	nclient--;

	cout<<"Slot liberato!"<<endl;

	signal(SIGUSR1, GestoreSegnale);		//Rendo sensibile il programma al segnale
}

int assegna( )					//Funzione che verifica se è possibile aprire un nuovo socket e se è possibile lo assegna
{
	socdes * pntapp;

	if(nclient<=MAXCLIENT)				//Verifico il numero di client connessi
	{
		pntapp=punta;

		while((pntapp->getpnt(2))!=NULL)			//Scorro la lista di oggetti
		{
			pntapp=pntapp->getpnt(2);
		}

		tmp= new socdes;		//Creo il nuovo puntatore e lo metto nella lista
		pntapp->setpnt(2,tmp);
		tmp->setpnt(1,pntapp);
		nclient++;				//Incremento il numero di client connessi

		return 7;
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
	bytericevuti = recv(tmp->getsock(), messaggio, DIM, 0);
	estrazione(messaggio, sito);		//Estraggo dalla stringa ricevuta dal client l'indirizzo della pagina web

	if(strcmp(sito,"/")==0)			//Se non è presente alcun indirizzo di default punto al sito index.html
	{
		strcat(PATH,"/index.html");
	}
	else
	{
		strcat(PATH,sito);
	}

	if((fd=open(PATH,O_RDONLY))!=-1)			//Trasferimento della pagina
	{
		cout<<"\nFile aperto: inizio trasferimento pagina!\n";

		send(tmp->getsock(), "HTTP/1.0 200 OK\n\n",17,0);

		while((byteletti=read(fd,datifilehtml,BYTES))>0)
		{
			write(tmp->getsock(),datifilehtml,byteletti);
		}

		close(fd);

		cout<<"\nFile chiuso: fine trasferimento pagina!\n";
	}
	else										//Nessun file da spedire! ERRORE 404
	{
		write(tmp->getsock(),"HTTP/1.0 404 Not found\n\n",23);
		cout<<"\nFile non trovato!\n";
	}

	shutdown(tmp->getsock(),SHUT_RDWR);			//Chiusura del socket
}

void estrazione(char msg[],char sito[])				//Funzione che estrae l'URL della pagina web
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
