// TCP socket SOCK_STREAM

#include "bor-util.h"

int boucleWhile = 1;

void handle(int sig) {
    printf("Signal recu : %d\n", sig);
    switch (sig) {
        case SIGINT: boucleWhile = 0; break;
        case SIGPIPE: boucleWhile = 0; break;
    }
}

void cleanEnd(int soc, const char* path){
    close(soc);
    unlink(path);
    printf("Client down\n");
}

int contact_serv(int soc) {
    int result;
    char buffer[1024];
    printf("Entrez une suite de digits :\n");

    result = bor_read_str(0, buffer, sizeof (buffer) - 1);
    if (result <= 0){
        perror("Erreur ou ctrl+D a l entre clavier\n");
        return result;
    }

    result = bor_write_str(soc, buffer);
    if (result <= 0){
        perror("Erreur a l ecriture\n");
         return result;
    }

    result = bor_read_str(soc, buffer, sizeof (buffer) - 1);
    if (result <= 0){
        perror("Erreur a la reception du message provenant du serveur\n");
        return result;
    }

    printf("Reponse du serveur : %s\n", buffer);

    return 1;
}

int main(int argc, char* argv[]){

    bor_signal(SIGINT, handle, 0);
    bor_signal(SIGPIPE, handle, 0);

    // Test nous permettant de vérifier le nombre d'arguments donnés en paramètres.
    if( argc != 3 ){
        perror("Nombre d'arguments invalide\n");
        return -1;
    }

    printf("Client up\n");

    const char* cliPath = argv[1];    //Adresse donnée du client
    const char* serPath = argv[2];    //Adresse donnée du serveur

    struct sockaddr_un cliSock;
    struct sockaddr_un serSock;

    int soc = bor_create_socket_un(SOCK_STREAM, cliPath, &cliSock);
    if(soc < 0){
        perror("Erreur a la creation du socket Client\n");
        return -1;
    }

    // Création adresse socket pour le serveur
    bor_set_sockaddr_un(serPath, &serSock);

    // Connexion au serveur
    printf("Tentative de connexion TCP au socket %s\n", serPath);
    if( bor_connect_un(soc, &serSock) < 0 ){
        perror("Erreur a la connexion entre le client et le serveur\n");
        cleanEnd(soc, cliPath);
        return -1;
    }
    printf("Connexion etablie\n");

    while( boucleWhile ){
        if( contact_serv(soc) <= 0 ) break;
    }

    cleanEnd(soc, cliPath);
    return 0;
}