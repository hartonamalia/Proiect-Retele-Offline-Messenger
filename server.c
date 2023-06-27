#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <sys/wait.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#define PORT 2024

sqlite3 *db;
char *err = 0;
char user[256];
int logat = 0;
/*--------------------------------------Baza de date-se deschide------------------------------------------------*/

void setDataBase()
{
    int rc = sqlite3_open("useri.db", &db);
    if (rc != SQLITE_OK)
    {
        printf("Eroare\n");
    }
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS Utilizatori(ID INTEGER PRIMARY KEY, Username TEXT,Password TEXT,Online INT);", 0, 0, &err);
    sqlite3_exec(db, "Update Utilizatori SET Online=0;", 0, 0, &err);
}

void RedeschidereBaza()
{
    int rc = sqlite3_open("useri.db", &db);
    if (rc != SQLITE_OK)
    {
        printf("Eroare\n");
    }
}

/*--------------------------------------Baza de date functii------------------------------------------------*/

void inserareUtilizator(char username[256], char password[256])
{
    char sql[256];
    char *err = 0;
    bzero(sql, 256);
    sprintf(sql, "INSERT INTO Utilizatori(Username,Password) VALUES('%s','%s');", username, password);
    sqlite3_exec(db, sql, 0, 0, &err);
    sqlite3_close_v2(db);
    RedeschidereBaza();
}
void codificareParola(char *password) {
  int i, len = strlen(password);
  for (i = 0; i < len; i++) {
    password[i] = password[i] + 1;  // adauga 1 la fiecare caracter
  }
}
void inregistrare(char username[256], char password[256], char msgrasp[8192])
{
    sqlite3_stmt *stmt;
    char sql[256];
    bzero(sql, 256);
    sprintf(sql, "SELECT Id FROM Utilizatori WHERE Username='%s';", username);
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    int ok = 0;
    sqlite3_step(stmt);
    ok = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (ok == 0)//daca nu exista deja un cont cu acelasi username
    {
        codificareParola(password);
        printf("parola = %s\n",password);
        inserareUtilizator(username, password);
        logat = 1;
        char sql[256];
        bzero(sql, 256);
        sprintf(sql, "UPDATE Utilizatori SET Online=1 WHERE Username='%s';", username);
        sqlite3_exec(db, sql, 0, 0, &err);
        strcpy(user, username);
        sqlite3_close_v2(db);
        RedeschidereBaza();
        sprintf(msgrasp, "[server]Inregistrare cu succes cu numele de utilizator %s", user);
    }
    else
    {
        strcpy(msgrasp, "[server]Utilizator existent!");
    }
}
int verificareStareOnline(char username[256])
{
    sqlite3_stmt *stmt;
    char sql[256];
    strcpy(sql, "SELECT Online FROM Utilizatori WHERE Username='");
    strcat(sql, username);
    strcat(sql, "';");
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    int ok = 0;
    sqlite3_step(stmt);
    ok = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return ok;
}
void decodificareParola(char *password) {
  int i, len = strlen(password);
  for (i = 0; i < len; i++) {
    password[i] = password[i] - 1;  // scade 1 fiecarui caracter
  }
}
void login(char username[256], char password[256], char msgrasp[8192])
{
    sqlite3_stmt *stmt;
    char sql[256];
    sprintf(sql,"SELECT Id FROM Utilizatori WHERE Username='%s';",username);
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    int ok = 0;
    sqlite3_step(stmt);
    ok = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (ok != 0)
    {
        ok = verificareStareOnline(username);
        if (ok == 0)
        {
            strcpy(sql, "SELECT Password FROM Utilizatori WHERE Username='");
            strcat(sql, username);
            strcat(sql, "';");
            sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            sqlite3_step(stmt);
            char *msg = (char *)sqlite3_column_text(stmt, 0);//stocheaza parola din baza de date
            decodificareParola(msg);
            if (strcmp(password, msg) == 0)
            {
                ok = 1;//parola introdusa de la tastatura si cea din baza de date coincid
            }
            else
            {
                ok = 0;//parola introdusa de la tastatura si cea din baza de date nu coincid
            }
            sqlite3_finalize(stmt);
            if (ok == 1)
            {
                strcpy(sql, "UPDATE Utilizatori SET Online=");
                strcat(sql, "1");
                strcat(sql, "  WHERE Username='");
                strcat(sql, username);
                strcat(sql, "';");
                sqlite3_exec(db, sql, 0, 0, &err);
                sqlite3_close_v2(db);
                RedeschidereBaza();
                strcpy(msgrasp, "[server]Autentificare cu succes, username-ul: ");
                strcat(msgrasp, username);
                strcpy(user, username);
                logat = 1;
            }
            else
            {
                strcpy(msgrasp, "[server]Parola incorecta!");
            }
        }
        else
        {
            strcpy(msgrasp, "[server]Un utilizator este deja logat in acest cont!");
        }
    }
    else
    {
        strcpy(msgrasp, "[server]Utilizator inexistent!");
    }
}
/*--------------------------------------Inbox------------------------------------------------*/

void creareInbox(char utilizator[256])
{
    char sql[1024];
    int rc;

    sprintf(sql, "CREATE TABLE IF NOT EXISTS %s (ID INTEGER PRIMARY KEY, Sender TEXT);", utilizator);
    rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Eroare la creare tabel: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_close_v2(db);
    RedeschidereBaza();
}
void inserareInbox(char utilizator[256], char sender[256])
{
    char sql[1024];
    sql[0] = '\0';
    sprintf(sql, "INSERT INTO %s(Sender) SELECT '%s' WHERE NOT EXISTS (SELECT Sender FROM %s WHERE Sender='%s');", utilizator, sender, utilizator, sender);
    int rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Error: %s\n", err);
        sqlite3_free(err);
    }
    sqlite3_close_v2(db);
    RedeschidereBaza();
}

/*--------------------------------------Existenta/Creare conversatie------------------------------------------------*/

int verificareTabel(char first[256], char second[256])
{
    char sql1[1024];
    char sql2[1024];
    sqlite3_stmt *stmt;
    sprintf(sql1, "SELECT ID FROM %s%s;", first, second);
    sprintf(sql2, "SELECT ID FROM %s%s;", second, first);

    sqlite3_prepare_v2(db, sql1, -1, &stmt, 0);
    sqlite3_step(stmt);

    int id;
    id = sqlite3_column_int(stmt, 0);
    if (id != 0)
    {
        sqlite3_finalize(stmt);
        return 1;// exista, cu prima combinatie 
    }
    else
    {
        sqlite3_prepare_v2(db, sql2, -1, &stmt, 0);
        sqlite3_step(stmt);
        id = sqlite3_column_int(stmt, 0);
        if (id != 0)
        {
            sqlite3_finalize(stmt);
            return 2;
        }
        else
        {
            sqlite3_finalize(stmt);
            return 0;//tabelul cu conv  nu exista 
        }
    }
}
void creareConversatie(char sender[256], char receiver[256])
{
    char sql[1024];
    char first[256];
    char second[256];
    bzero(sql, 1024);
    bzero(first, 1024);
    bzero(second, 1024);
    if ((verificareTabel(sender, receiver) == 1) || (verificareTabel(sender, receiver) == 0))
    {
        strcpy(first, sender);
        strcpy(second, receiver);
    }
    else if (verificareTabel(sender, receiver) == 2)
    {
        strcpy(first, receiver);
        strcpy(second, sender);
    }
    sprintf(sql, "CREATE TABLE IF NOT EXISTS %s%s(ID INTEGER PRIMARY KEY, Sender TEXT,Receiver TEXT,Mesaj TEXT,Citit Integer, Timp TEXT);", first, second);
    printf("SQL= %s\n", sql);
    sqlite3_exec(db, sql, 0, 0, &err);
    printf("Eroare  Creare= %s\n", err);
    sqlite3_close_v2(db);
    RedeschidereBaza();
}
/*--------------------------------------Arata utilizatorii din baza de date------------------------------------------------*/

// Functia afiseaza toti utilizatorii din tabela Utilizatori
void showUsers(char msgrasp[8192])
{
    // Pregateste interogarea SELECT
    char sql[256];
    sqlite3_stmt *stmt;
    int id;
    int identificator;
    char *msg;
    // Preia cel mai mare Id din tabela Utilizatori
    int ok = sqlite3_prepare_v2(db, "SELECT ID FROM Utilizatori ORDER BY Id DESC LIMIT 1;", -1, &stmt, 0);
    if (ok != SQLITE_OK)
    {
        printf("Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
        return;
    }

    // Executa interogarea
    ok = sqlite3_step(stmt);
    if (ok == SQLITE_ROW)
    {
        id = sqlite3_column_int(stmt, 0);
    }
    else if (ok != SQLITE_DONE)
    {
        printf("Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
        return;
    }

    // Finalizeaza interogarea
    sqlite3_finalize(stmt);
    {
        // Parcurge fiecare rand din tabela Utilizatori
        for (int i = 1; i <= id; i++)
        {
            // Reseteaza variabila auxiliara
            char aux[256] = "";
            // Construieste interogarea SELECT
            snprintf(sql, sizeof(sql), "SELECT Id,Username FROM Utilizatori WHERE Id=%d", i);
            // Preia valorile coloanelor Id si Username
            ok = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            if (ok != SQLITE_OK)
            {
                printf("Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
                return;
            }
            ok = sqlite3_step(stmt);
            if (ok == SQLITE_ROW)
            {
                identificator = sqlite3_column_int(stmt, 0);
                msg = (char *)sqlite3_column_text(stmt, 1);

                // Formateaza valorile Id si Username in variabila auxiliara
                sprintf(aux, "%d.%s\n", identificator, msg);
            }
            else if (ok != SQLITE_DONE)
            {
                printf("Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
                return;
            }
            sqlite3_finalize(stmt);
            // Concateneaza variabila auxiliara la msgrasp
            strcat(msgrasp, aux);
        }
    }
}
/*--------------------------------------Pentru a vedea inbox-ul------------------------------------------------*/
void seeInbox(char msgrasp[8192])
{
    char sql[1024];
    sqlite3_stmt *stmt;
    int id;
    int identificator;
    char *msg;
    // Preia cel mai mare Id din tabela Index
    sprintf(sql, "SELECT ID FROM %s ORDER BY Id DESC LIMIT 1;", user);
    int ok = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (ok != SQLITE_OK)
    {
        printf("Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
        return;
    }
    ok = sqlite3_step(stmt);
    if (ok == SQLITE_ROW)
    {
        id = sqlite3_column_int(stmt, 0);
    }
    else if (ok != SQLITE_DONE)
    {
        printf("Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_finalize(stmt);
    if (id > 0)
    {
        printf("Id = %d\n", id);
        for (int i = 1; i <= id; i++)
        {
            char aux[256] = "";
            snprintf(sql, sizeof(sql), "SELECT Id,Sender FROM %s WHERE Id=%d", user, i);
            ok = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            if (ok != SQLITE_OK)
            {
                printf("Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
                return;
            }
            ok = sqlite3_step(stmt);
            if (ok == SQLITE_ROW)
            {
                identificator = sqlite3_column_int(stmt, 0);
                msg = (char *)sqlite3_column_text(stmt, 1);
                sprintf(aux, "%d.%s\n", identificator, msg);
            }
            else if (ok != SQLITE_DONE)
            {
                printf("Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
                return;
            }
            sqlite3_finalize(stmt);
            strcat(msgrasp, aux);
        }
    }
    else
    {
        strcpy(msgrasp, "[server]Inbox-ul dumneavoastra este gol, nu aveti niciun mesaj nou!\n");
    }
}
/*--------------------------------------Trimite mesaj------------------------------------------------*/

void sendMessage(char sender[256], char receiver[256], char mesaj[8192], char msgrasp[8192])
{
    char sql[1024];
    char senderSign[256];
    char first[256];
    char second[256];
    char msg[8192];
    bzero(sql, 1024);
    bzero(first, 1024);
    bzero(second, 1024);
    bzero(senderSign, 1024);
    showUsers(msg); // retinem lista cu numele utilizatorilor
    if (strstr(msg, receiver) != 0)//verific ca cel catre care trimite mesajul sa fie in lista de utilizatori
    {
        creareInbox(sender);
        creareInbox(receiver);
        // Creează o conversație între sender și receiver
        creareConversatie(user, receiver);

        if ((verificareTabel(sender, receiver) == 1) || (verificareTabel(sender, receiver) == 0))
        {
            strcpy(first, sender);
            strcpy(second, receiver);
        }
        else if (verificareTabel(sender, receiver) == 2)
        {
            strcpy(first, receiver);
            strcpy(second, sender);
        }

        inserareInbox(receiver, sender);
        sprintf(sql, "INSERT INTO %s%s(Sender, Receiver, Mesaj,Citit,Timp) VALUES ('%s', '%s', '[%s]%s','0',datetime('now','localtime'));", first, second, sender, receiver, sender, mesaj);//inserez mesajul
        sqlite3_exec(db, sql, 0, 0, &err);
        sqlite3_close_v2(db);
        RedeschidereBaza();
        sprintf(msgrasp, "[server]Mesaj trimis cu succes catre %s", receiver);
    }
    else
    {
        strcpy(msgrasp, "[server]Utilizatorul caruia incercati sa-i trimiteti un mesaj nu se afla in baza de date");
    }
}

/*--------------------------------------Arata istoricul conversatiei dintre 2 utilizatori------------------------------------------------*/
void removeFromInbox(char friend[256])
{
    char sql[1024];
    sprintf(sql, "DELETE FROM %s WHERE Sender='%s';", user, friend);
    printf("%s\n", sql);
    sqlite3_exec(db, sql, 0, 0, &err);
    sqlite3_close_v2(db);
    RedeschidereBaza();
}

void seeHistory(char friend[256], char msgrasp[8192])
{
    char sql[256], first[256], second[256], nr[3], *msg;
    sqlite3_stmt *stmt;
    int id = 0;

    bzero(first, 256);
    bzero(second, 256);
    bzero(sql, 256);

    strcpy(sql, "SELECT ID FROM ");

    if (verificareTabel(user, friend) == 1)
    {
        strcpy(first, user);
        strcpy(second, friend);
        id = 1;
    }
    else if (verificareTabel(user, friend) == 2)
    {
        strcpy(first, friend);
        strcpy(second, user);
        id = 1;
    }

    if (id != 0)
    {
        strcat(sql, first);
        strcat(sql, second);
        strcat(sql, " ORDER BY Id DESC LIMIT 1;");
        sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        sqlite3_step(stmt);
        id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        for (int i = 1; i <= id; i++)
        {
            bzero(sql, 256);
            strcpy(sql, "Update ");
            strcat(sql, first);
            strcat(sql, second);
            strcat(sql, " SET Citit = 1 WHERE Id=");
            removeFromInbox(friend);
            bzero(nr, 3);
            sprintf(nr, "%d", i);
            strcat(sql, nr);
            strcat(sql, ";");
            sqlite3_exec(db, sql, 0, 0, &err);
        }
        for (int i = 1; i <= id; i++)
        {
            bzero(sql, 256);
            strcpy(sql, "Select Id, Mesaj, Timp FROM ");
            strcat(sql, first);
            strcat(sql, second);
            strcat(sql, " WHERE Id=");
            bzero(nr, 3);
            sprintf(nr, "%d", i);
            strcat(sql, nr);
            strcat(sql, ";");

            sqlite3_stmt *stmt2;
            int rc = sqlite3_prepare_v2(db, sql, -1, &stmt2, 0);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
                exit(1);
            }

            rc = sqlite3_step(stmt2);
            if (rc == SQLITE_ROW)
            {
                msg = (char *)sqlite3_column_text(stmt2, 0);
                strcat(msgrasp, msg);
                strcat(msgrasp, ".");
                msg = (char *)sqlite3_column_text(stmt2, 1);
                strcat(msgrasp, msg);
                strcat(msgrasp, " ");
                msg = (char *)sqlite3_column_text(stmt2, 2);
                strcat(msgrasp, msg);
                strcat(msgrasp, "\n");
            }
            else if (rc != SQLITE_DONE)
            {
                fprintf(stderr, "Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
                exit(1);
            }

            sqlite3_finalize(stmt2);
        }
    }
    else
    {
        strcpy(msgrasp, "[server]Nu aveti niciun mesaj trimis cu acest utilizator!\n");
    }
}
void seeNewMessages(char friend[256], char msgrasp[8192])
{
    char sql[8192], first[256], second[256], nr[3], *msg;
    sqlite3_stmt *stmt;
    int id = 0;

    bzero(first, 256);
    bzero(second, 256);
    bzero(sql, 8192);

    if (verificareTabel(user, friend) == 1)
    {
        strcpy(first, user);
        strcpy(second, friend);
        id = 1;
    }
    else if (verificareTabel(user, friend) == 2)
    {
        strcpy(first, friend);
        strcpy(second, user);
        id = 1;
    }
    seeInbox(msgrasp);
    if (id != 0)
    {
        if (strstr(msgrasp, friend) != 0)
        {
            bzero(msgrasp, 8192);
            strcpy(sql, "SELECT ID FROM ");
            strcat(sql, first);
            strcat(sql, second);
            strcat(sql, " ORDER BY Id DESC LIMIT 1;");
            sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            sqlite3_step(stmt);
            id = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            for (int i = 1; i <= id; i++)
            {
                bzero(sql, 8192);
                sprintf(sql, "Select Id, Mesaj, Timp FROM %s%s WHERE Citit='0' AND Sender='%s' AND ID = %d;", first, second, friend, i);
                printf("COmanda %s\n", sql);
                sqlite3_stmt *stmt2;
                int rc = sqlite3_prepare_v2(db, sql, -1, &stmt2, 0);
                if (rc != SQLITE_OK)
                {
                    fprintf(stderr, "Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
                    exit(1);
                }

                rc = sqlite3_step(stmt2);
                if (rc == SQLITE_ROW)
                {
                    msg = (char *)sqlite3_column_text(stmt2, 0);
                    strcat(msgrasp, msg);
                    strcat(msgrasp, ".");
                    msg = (char *)sqlite3_column_text(stmt2, 1);
                    strcat(msgrasp, msg);
                    strcat(msgrasp, " ");
                    msg = (char *)sqlite3_column_text(stmt2, 2);
                    strcat(msgrasp, msg);
                    strcat(msgrasp, "\n");
                }
                else if (rc != SQLITE_DONE)
                {
                    fprintf(stderr, "Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
                    exit(1);
                }
                sprintf(sql, "Update %s%s SET Citit = 1 WHERE Sender='%s' AND ID = %d;", first, second, friend, i);
                sqlite3_exec(db, sql, 0, 0, &err);
                sqlite3_finalize(stmt2);
                sqlite3_close_v2(db);
                RedeschidereBaza();
                printf("Mesaj este %s\n", msgrasp);
                removeFromInbox(friend);
            }
        }
        else
        {
            strcpy(msgrasp, "[server]Nu aveti vreun mesaj nou de la acest utilizator!\n");
        }
    }
    else
    {
        strcpy(msgrasp, "[server]Nu aveti niciun mesaj trimis cu acest utilizator!\n");
    }
}

void reply(char identificator[3], char receiver[256], char mesaj[8192], char msgrasp[8192])
{
    char lista_utilizatori[1024]; 
    char sql[1024];
    char first[256];
    char second[256];
    char msg[8192];
    char *ret;
    sqlite3_stmt *stmt;
    bzero(sql, 1024);
    bzero(first, 1024);
    bzero(second, 1024);
    showUsers(lista_utilizatori); // retinem lista cu numele  tuturor utilizatorilor
    if (strstr(lista_utilizatori, receiver) != 0)
    {
        if ((verificareTabel(user, receiver) == 1) || (verificareTabel(user, receiver) == 0))
        {
            strcpy(first, user);
            strcpy(second, receiver);
        }
        else if (verificareTabel(user, receiver) == 2)
        {
            strcpy(first, receiver);
            strcpy(second, user);
        }
        inserareInbox(receiver, user);
        bzero(sql, 8192);
        bzero(msg, 8192);
        int id;
        id = atoi(identificator);
        sprintf(sql, "Select Mesaj FROM %s%s WHERE ID = %d;", first, second, id);//selectez mesajul caruia vrem sa i dam reply
        sqlite3_stmt *stmt2;
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt2, 0);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
            exit(1);
        }

        rc = sqlite3_step(stmt2);
        if (rc == SQLITE_ROW)
        {
            ret = (char *)sqlite3_column_text(stmt2, 0);//mesajul care vrem sa i dam reply
            strcat(msg, "[Reply la] ");
            strcat(msg, ret);
            strcat(msg, ":\n");
            strcat(msg, mesaj);
            strcat(msg, " ");

        }
        else if (rc != SQLITE_DONE)
        {
            fprintf(stderr, "Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
            exit(1);
        }
        sqlite3_finalize(stmt2); 
        sendMessage(user, receiver, msg, msgrasp);
    }
    else
    {
        strcpy(msgrasp, "[server]Utilizatorul caruia incercati sa-i trimiteti un replay nu se afla in baza de date");
    }
}

/*--------------------------------------Main------------------------------------------------*/

int main()
{
    char msgrasp[8192] = " ";
    char msg[8192];
    struct sockaddr_in server;
    struct sockaddr_in from;
    setDataBase();
    int sd; // descriptorul de socket 
        
         /* crearea unui socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }
      /* pregatirea structurilor de date */
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));
      /* umplem structura folosita de server */
  	  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
      /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
       /* utilizam un port utilizator */
    server.sin_port = htons(PORT);

     /*atasam socketul*/
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }
      
           /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen(sd, 5) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }


      /* servim in mod concurent clientii...folosind fork */

    while (1)
    {
        int client;
        int length = sizeof(from);

        printf("Asteptam la portul %d...\n", PORT);
        fflush(stdout);
      /* acceptam un client (stare blocanta pana la realizarea conexiunii) */
        client = accept(sd, (struct sockaddr *)&from, &length);
        if (client < 0)
        {
            perror("Eroare la accept().\n");
            continue;
        }
	 /* s-a realizat conexiunea, se astepta mesajul */

        int pid;
        if ((pid = fork()) == -1)
        {
            close(client);
            continue;
        }
        else if (pid > 0)
        {
            close(client);
            while (waitpid(-1, NULL, WNOHANG))
                ;
            continue;
        }
        else if (pid == 0)
        {
            close(sd);
            while (1) // permite procesarea mai multor comenzi
            {
                bzero(msg, 8192);
                printf("Asteptam mesajul...\n");
                fflush(stdout);

                if (read(client, msg, 8192) <= 0)
                {
                    perror("Eroare la read() de la client.\n");
                    close(client);
                    continue;
                }
                msg[strlen(msg) - 1] = '\0';
                printf("Mesajul a fost receptionat...%s\n", msg);
                bzero(msgrasp, 8192);
                if (strcmp(msg, "exit") == 0) // quit
                {
                    if (logat == 1)
                    {
                        char sql[1024];
                        bzero(sql, 1024);
                        sprintf(sql, "UPDATE Utilizatori SET Online=0 WHERE Username='%s';", user);
                        sqlite3_exec(db, sql, 0, 0, &err);
                        sqlite3_close_v2(db);
                        RedeschidereBaza();
                        logat = 0;
                    }
                    close(client);
                    exit(0);
                }
                else if (strcmp(msg, "help") == 0) // quit
                {
                    strcpy(msgrasp, "[server]Comenzile disponibile daca nu sunteti logat sunt:\n1.inregistrare <nume> <parola>\n2.login <nume> <parola>\n3.help\n4.exit\n Daca sunteti logat puteti apela comenzile:\n1.showUsers\n2.sendMessage <nume_destinatar> <mesaj>\n3.seeHistory <nume_destinatar>\n4.seeInbox\n5.seeNewMessages <nume_destinatar>\n6.reply <id_conversatie> <nume_destinatar> <mesaj>\n7.help\n8.logout\n9.exit\n10.quit\n");
                }
                else if (strstr(msg, "inregistrare ") && logat == 0)
                {
                    char *p;
                    char username[256], password[256], mesaj[4096];
                    username[0] = '\0';
                    password[0] = '\0';
                    p = strtok(msg, " ");
                    p = strtok(NULL, " ");
                    if (p != NULL)
                    {
                        strcpy(username, p);
                        p = strtok(NULL, " ");
                        if (p != NULL)
                        {
                            strcpy(password, p);
                        }
                    }

                    if (username[0] != '\0' && password[0] != '\0')
                    {
                        strcpy(msgrasp, username);
                        strcat(msgrasp, password);
                        inregistrare(username, password, msgrasp);
                    }
                    else
                        strcpy(msgrasp, "[server]Comanda introdusa gresit, formatul corect este inregistrare <username> <password>\n");
                }
                else if (strstr(msg, "inregistrare ") && logat == 1)
                {
                    strcpy(msgrasp, "[server]Sunteti deja autentificat intr-un cont!\n");
                }
                // Dacă mesajul conține cuvântul "login" și utilizatorul nu este deja autentificat
                else if (strstr(msg, "login ") && logat == 0)
                {
                    // Inițializarea variabilelor
                    char *p;
                    char username[256], password[256], mesaj[4096];
                    bzero(username, 256);
                    bzero(password, 256);

                    // Extragerea numelui de utilizator și a parolei din mesaj
                    p = strtok(msg, " ");
                    if (p != NULL)
                    {
                        p = strtok(NULL, " ");
                        if (p != NULL)
                        {
                            strcpy(username, p);
                            p = strtok(NULL, " ");
                            if (p != NULL)
                            {
                                strcpy(password, p);
                            }
                        }
                    }

                    if (username[0] != '\0' && password[0] != '\0')
                    {
                        // Atât numele de utilizator cât și parola sunt prezente, deci încercați să autentificați utilizatorul
                        login(username, password, msgrasp);
                    }
                    else
                    { // Fie numele de utilizator sau parola lipsește, deci afișați un mesaj de eroare
                        strcpy(msgrasp, "[server]Comanda introdusă greșit, formatul corect este login <username> <password>\n");
                    }
                }
                else if (strstr(msg, "login ") && logat == 1)
                {
                    strcpy(msgrasp, "[server]Sunteti deja autentificat intr-un cont!\n");
                }
                else if (strstr(msg, "logout") && logat == 1)
                {
                    // Construirea interogării SQL pentru a seta starea utilizatorului la offline
                    char sql[1024];
                    bzero(sql, 1024);
                    sprintf(sql, "UPDATE Utilizatori SET Online=0 WHERE Username='%s';", user);

                    sqlite3_exec(db, sql, 0, 0, &err);
                    sqlite3_close_v2(db);
                    RedeschidereBaza();

                    // Setarea variabilei logat la 0 și afișarea unui mesaj de succes
                    logat = 0;
                    strcpy(msgrasp, "[server]Delogare cu succes!\n");
                }
                else if (strstr(msg, "logout") && logat == 0)
                {
                    strcpy(msgrasp, "[server]Nu sunteti logat intr-un cont!\n");
                }
                else if (strcmp(msg, "showUsers") == 0 && logat == 1)
                {
                    showUsers(msgrasp);
                }
                else if (strcmp(msg, "showUsers") == 0 && logat == 0)
                {
                    strcpy(msgrasp, "[server]Pentru a putea vedea lista utilizatorilor trebuie sa fiti logat\n");
                }
                else if (strcmp(msg, "seeInbox") == 0 && logat == 1)
                {
                    seeInbox(msgrasp);
                }
                else if (strcmp(msg, "seeInbox") == 0 && logat == 0)
                {
                    strcpy(msgrasp, "[server]Pentru a putea vedea daca ati primit vreun mesaj nou trebuie sa fiti logat\n");
                }
                // Verifică dacă mesajul conține "sendMessage " și dacă utilizatorul este logat
                else if (strstr(msg, "sendMessage ") && logat == 1)
                {
                    sqlite3_close_v2(db);
                    RedeschidereBaza();
                    // Declară variabilele necesare
                    char *p;
                    char sender[256], receiver[256], mesaj[8192] = "";
                    sender[0] = '\0';
                    receiver[0] = '\0';
                    // Folosește strtok pentru a parsa mesajul în cuvinte
                    p = strtok(msg, " ");
                    p = strtok(NULL, " ");
                    if (p != NULL)
                    {
                        // Copiază al doilea cuvant în receiver
                        strcpy(receiver, p);
                        p = strtok(NULL, " ");
                        if (p != NULL)
                            // Adaugă al treilea cuvant la mesaj
                            strcat(mesaj, p);
                    }
                    // Parsează toate cuvintele ramase
                    while (p != NULL)
                    {
                        p = strtok(NULL, " ");
                        if (p != NULL)
                        {
                            strcat(mesaj, " ");
                            strcat(mesaj, p);
                        }
                    }
                    // Verifică dacă s-au parsat corect sender-ul, receiver-ul și mesajul
                    if (strcmp(msg, "sendMessage") == 0 && receiver[0] != 0 && mesaj[0] != '\0')
                    {
                        // Inserează mesajul în baza de date
                        sendMessage(user, receiver, mesaj, msgrasp);
                    }
                    else
                    {
                        // Afișează un mesaj de eroare dacă nu s-au parsat corect sender-ul, receiver-ul sau mesajul
                        strcpy(msgrasp, "[server]Comanda introdusa gresit, formatul corect este sendMessage receiver mesaj\n");
                    }
                }
                else if (strstr(msg, "sendMessage ") && logat == 0)
                {
                    strcpy(msgrasp, "[server]Pentru a putea trimite un mesaj trebuie sa fiti logat\n");
                }
                else if (strstr(msg, "seeHistory ") && logat == 1)
                {
                    char *p;
                    char receiver[256];
                    receiver[0] = '\0';
                    p = strtok(msg, " ");

                    if (p != NULL)
                    {
                        {
                            p = strtok(NULL, " ");
                            if (p != NULL)
                                ;
                            strcpy(receiver, p);
                        }
                        if (receiver != '\0')
                        {
                            seeHistory(receiver, msgrasp);
                        }
                        else
                        {
                            strcpy(msgrasp, "[server]Comanda introdusa gresit, formatul corect este seeHistory nume_utilizator\n");
                        }
                    }
                }
                else if (strstr(msg, "seeHistory ") && logat == 0)
                {
                    strcpy(msgrasp, "[server]Pentru a putea vedea istoricul mesajelor cu un alt utilizator trebuie sa fiti logat\n");
                }
                else if (strstr(msg, "seeNewMessages ") && logat == 1)
                {
                    char *p;
                    char receiver[256];
                    receiver[0] = '\0';
                    p = strtok(msg, " ");

                    if (p != NULL)
                    {
                        {
                            p = strtok(NULL, " ");
                            if (p != NULL)
                                strcpy(receiver, p);
                        }
                        if (strcmp(msg,"seeNewMessages")==0 && receiver != '\0')
                        {
                            seeNewMessages(receiver, msgrasp);
                        }
                        else
                        {
                            strcpy(msgrasp, "[server]Comanda introdusa gresit, formatul corect este seeNewMessages nume_utilizator\n");
                        }
                    }
                }
                else if (strstr(msg, "seeNewMessages ") && logat == 0)
                {
                    strcpy(msgrasp, "[server]Pentru a putea vedea mesajele noi primite de la un alt utilizator trebuie sa fiti logat\n");
                }
                else if (strstr(msg, "reply ") && logat == 1)
                {
                    char *p;
                    char id[3], receiver[256];
                    char mesaj[8192];
                    bzero(mesaj, 8192);
                    id[0] = '\0';
                    p = strtok(msg, " ");
                    if (p != NULL)
                    {
                        p = strtok(NULL, " ");
                        if (p != NULL)
                        {
                            strcpy(id, p);
                            p = strtok(NULL, " ");
                            if (p != NULL)
                            {
                                strcpy(receiver, p);
                            }
                        }
                    }
                    while (p != NULL)
                    {
                        p = strtok(NULL, " ");
                        if (p != NULL)
                        {
                            strcat(mesaj, " ");
                            strcat(mesaj, p);
                        }
                    }
                    // Verifică dacă s-au parsat corect sender-ul, receiver-ul și mesajul
                    if (id[0] != '\0' && receiver[0] != 0 && mesaj[0] != '\0')
                    {
                        // Inserează mesajul în baza de date
                        reply(id, receiver, mesaj, msgrasp);
                    }
                    else
                    { // Fie numele de utilizator sau parola lipsește, deci afișați un mesaj de eroare
                        strcpy(msgrasp, "[server]Comanda introdusă greșit, formatul corect este reply <id> mesaj nume_utilizator\n");
                    }
                }
                else if (strstr(msg, "reply ") && logat == 0)
                {
                    strcpy(msgrasp, "[server]Pentru a putea da reply unui mesaj trebuie sa fiti logat\n");
                }
                else
                {
                    strcpy(msgrasp, "[server]Comanda gresita\n");
                }

                printf("Trimitem mesajul inapoi...%s\n", msgrasp);

                if (write(client, msgrasp, 8192) <= 0)
                {
                    perror("Eroare la write() catre client.\n");
                    continue;
                }
                else
                    printf("Mesajul a fost trasmis cu succes.\n");
            }
            close(client);
            exit(0);
        }

    } // while
}
