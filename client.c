#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

extern int errno;

int port;

int main (int argc, char *argv[])
{
    int sd,ok=1;
    struct sockaddr_in server;// structura folosita pt conectare
    char msg[8192];// mesajul trimis

    /* exista toate argumentele in linia de comanda? */

    if (argc != 3)
    {
        printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }
    port = atoi (argv[2]);//stabilim portul
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)//creez socket ul
    {
        perror ("Eroare la socket().\n");
        return errno;
    }

      /* umplem structura folosita pentru realizarea conexiunii cu serverul */
       /* familia socket-ului */
    server.sin_family = AF_INET;
        /* adresa IP a serverului */
    server.sin_addr.s_addr = inet_addr(argv[1]);
       /* portul de conectare */
    server.sin_port = htons (port);

            /* ne conectam la server */
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
        perror ("Eroare la connect().\n");
        return errno;
    }

    while(ok)
    { 
        bzero (msg, 8192);
        printf ("[server]Introduceti o comanda(exit, help): ");
        fflush (stdout);
        read (0, msg, 8192);
        //printf("[%s]\n",msg);
        if(!strcmp(msg,"exit\n") || !strcmp(msg,"Exit\n") || !strcmp(msg,"exit") || !strcmp(msg,"Exit")) ok=0;
        msg[strlen(msg)]='\0';
        if (write (sd, msg, 8192) <= 0)
        {
            perror ("Eroare la write() spre server.\n");
            return errno;
        }

        if (read (sd, msg, 8192) < 0)
        {
            perror ("Eroare la read() de la server.\n");
            return errno;
        }
        printf ("Mesajul primit este:\n%s\n", msg);
        if(ok==0) close(sd);
    }
    /* inchidem conexiunea, am terminat */
    close (sd);
}

