/*-----------------------------------------------------------------
Progetto: Webserver con oggetti (libreria di funzionamento)
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

#define MAXCLIENT 1000
#define BYTES 1024
#define DIM 99999
#define SIGUSR1 10

typedef sockaddr_in Sockaddr_in;

using namespace std;

class socdes			//Creo la classe del descrittore del socket
{
private:

	pid_t pid;			//Dichiarazione degli attibuti della classe del socket
	int socket, id;
	socdes *succ, *prec;

public:

	socdes()			//Metodo "costruttore" di default dell'oggetto
	{
		succ=NULL;
		prec=NULL;
		socket=0;
	}

	socdes(pid_t val, int sock)			//Metodo "costruttore" dell'oggetto con passaggio di parametri
	{
		socket=sock;
		pid=val;
		succ=NULL;
		prec=NULL;
	}

	~socdes()
	{

	}

	void setsock(int val)			//Metodo per il settaggio del valore del socket
	{
		socket=val;
	}

	void setpid(pid_t val)			//Metodo per il settaggio del valore del pid			
	{
		pid=val;
	}

	int getsock()					//Metodo per la restituzione del socket
	{
		return socket;
	}
		
	pid_t getpid()					//Metodo per la restituzione del pid
	{
		return pid;
	}

	void setpnt(int sent, socdes *pnt)		//Metodi per i settaggi del puntatore (varia "sent" in base al fatto che si analizzi il precedente o il successivo)
	{
		if(sent==1)
		{
			prec=pnt;
		}
		else
		{
			succ=pnt;
		}
	}

	socdes * getpnt(int sent)		//Metodi per la restituzione del puntatore (varia "sent" in base al fatto che si analizzi il precedente o il successivo)
	{
		if(sent==1)
		{
			return prec;
		}
		else
		{
			return succ;
		}
	}

	void setid(int val)			//Metodo per il settaggio dell'id (per id si intende lo storico della connessione)
	{
		id=val;
	}

	int getid()					//Metodo per la restituzione dell'id (per id si intende lo storico della connessione)
	{
		return id;
	}
};
