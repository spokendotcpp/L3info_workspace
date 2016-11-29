#include "bor-util.h"

#define SLOTS_NB 32
#define REQSIZE 2048
#define RESPSIZE 2048

int shutDown = 1;
char buffer[2048];

void handleSignal(int sig){
    printf("\nSignal recu : %d\n", sig);
    switch (sig) {
        case SIGINT: shutDown = 0;
        case SIGPIPE: shutDown = 0;
    }
}

typedef enum { S_FREE, S_READ_REQUEST, S_WRITE_RESPONSE } State;

typedef struct {
    State state;
    int soc;                    // Socket de service, défault -1
    int req_pos;                // Position d'insertion pour la prochaine lecture
    int end_header;             // Position de la fin d'entête
    char req[REQSIZE];          // Stockage de la requête

    int resp_pos;                // Position de l'insertion pour la réponses
    char resp[RESPSIZE];         // Stockage de la réponses
    struct sockaddr_in adr;     // Adresse du client
    int file_fd;                // file descriptor

} Slot;

typedef struct {
    Slot slots[SLOTS_NB];
    int listen_soc;             // Socket d'écoute
    struct sockaddr_in adr;     // Adresse du serveur
} Server;

typedef enum { M_NONE, M_GET, M_TRACE, } Id_method;

typedef enum {
    C_OK = 200,
    C_BAD_REQUEST = 400,
    C_NOT_FOUND = 404,
    C_METHOD_UNKNOWN = 501,
} Code_response;

typedef struct {
    char methode[REQSIZE];
    char url[REQSIZE];
    char version[REQSIZE];
    char path[REQSIZE];
    Id_method id_meth;
    Code_response code_resp;
} Info_header;

int is_free_slot(Slot* slot){ return slot->state == S_FREE; }

void init_slot(Slot* slot){
    slot->state = S_FREE;
    slot->soc = -1;
    slot->req_pos = 0;
    slot->end_header = 0;
    slot->resp_pos = 0;
    slot->req[0] = '\0';
    slot->resp[0] = '\0';
    slot->file_fd = 0;

    memset(&slot->adr, 0, sizeof( slot->adr ));
}

void free_slot(Slot* slot){
    if( !is_free_slot(slot) ){

        if( slot->file_fd != 1 )
            close(slot->file_fd);

        close(slot->soc);
        init_slot(slot);
    }
}

void init_server(Server* ser){
    for(unsigned int i=0; i < SLOTS_NB; ++i)
        init_slot( &ser->slots[i] );

    ser->listen_soc = -1;
    memset(&ser->adr, 0, sizeof( ser->adr ));
}

int find_free_slot(Server* ser){
    for(unsigned int i=0; i < SLOTS_NB; ++i)
        if(is_free_slot(&ser->slots[i]))
            return i;

    return -1;
}

int read_remaining_request(Slot* slot){

    int result = bor_read_str(slot->soc, slot->req + slot->req_pos, REQSIZE - slot->req_pos);
    if (result > 0) slot->req_pos += result;
    printf("Reçu : \"%s\"\n", slot->req);

    return result;
}

int write_remaining_response(Slot* slot){
    int result = -1;

    if(RESPSIZE - slot->resp_pos > 0){
        result = bor_write_str(slot->soc, slot->resp + slot->resp_pos);
        if(result <= 0){
            perror("Erreur : bor_write_str fail\n");
            return -1;
        }
        slot->resp_pos += result;
    }
    else{
        slot->resp_pos = 0;
    }

    return result;
}


int search_end_header(Slot* slot, int beginning){
    for(int i=beginning; i < REQSIZE - beginning; ++i){
        if( slot->req[i] == '\n' && i+1 < REQSIZE - beginning ){
            if( slot->req[i+1] == '\n')
                return i;
        }

        if( slot->req[i] == '\r' && i+3 < REQSIZE - beginning ){
            if( slot->req[i+1] == '\n' && slot->req[i+2] == '\r' && slot->req[i+3] == '\n' )
                return i;
        }
    }

    return -1;
}

char* get_http_error_message(Code_response code){
    switch(code){
        case C_OK : return "OK";
        case C_BAD_REQUEST : return "BAD REQUEST";
        case C_NOT_FOUND : return "NOT FOUND";
        case C_METHOD_UNKNOWN : return "METHOD UNKNOWN";
        default : return "ERROR";
    }
}

Id_method get_id_method(char* method){
    if ( strcasecmp(method, "GET") == 0 ) return M_GET;
    if ( strcasecmp(method, "TRACE") == 0 ) return M_TRACE;
    return M_NONE;
}

void close_server(Server* ser){
    close(ser->listen_soc);

    for(unsigned int i=0; i < SLOTS_NB; ++i)
        free_slot(&ser->slots[i]);
}

int start_server(Server* ser, int port){
    init_server(ser);

    int soc = bor_create_socket_in(SOCK_STREAM, port, &ser->adr);  //Création de la socket serveur
    if( soc < 0 ){
        perror("Erreur : creation de la socket serveur fail\n");
        return -1;
    }

    ser->listen_soc = soc;
    if( bor_listen(ser->listen_soc, SLOTS_NB) < 0 ){
        close_server(ser);
        perror("Erreur : socket d ecoute fail\n");
        return -1;
    }

    return 0;
}

int accept_connection(Server* ser){
    int index = find_free_slot(ser);

    if(index < 0) {
        printf("Trop de clients connectes\n");
        return -1;
    }

    Slot* slot = &ser->slots[ index ];
    slot->soc = bor_accept_in(ser->listen_soc, &slot->adr);

    if(slot->soc < 0){
        perror("Erreur au niveau du bor_accept_in");
        return -1;
    }

    slot->state = S_READ_REQUEST;

    return 0;
}

int prepare_file(Slot* slot, Info_header* info){
    if( sscanf(info->url, "%[^?]", info->path) != 1 ) return -1;

    slot->file_fd = open(info->path, O_RDONLY);
    if( slot->file_fd < 0 ) return -1;

    return 0;
}

void analyse_request(Slot* slot, Info_header* info){
    int result = sscanf(slot->req, "%s %s %s\n", info->methode, info->url, info->version);
    if(result != 3){
        info->code_resp = C_BAD_REQUEST;
        return;
    }

    if( strcasecmp(info->version, "HTTP/1.0") != 0 && strcasecmp( info->version, "HTTP/1.1") != 0 ){
        info->code_resp = C_BAD_REQUEST;
        return;
    }

    info->id_meth = get_id_method(info->methode);

    switch(info->id_meth){
        case M_TRACE : info->code_resp = C_OK; break;
        case M_GET :
            printf("%d\n",info->code_resp);
            result = prepare_file(slot, info);
            if(result < 0) info->code_resp = C_NOT_FOUND;
            else info->code_resp = C_OK;
        break;
        default : info->code_resp = C_METHOD_UNKNOWN;
    }
}

void prepare_response(Slot* slot, Info_header* info){
    time_t t;   // Initialisation du "temps"
    time(&t);

    char* errorMessage = get_http_error_message(info->code_resp);
    int position = sprintf(slot->resp, "HTTP/1.1 %d %s\n", info->code_resp, errorMessage);

    position += sprintf(slot->resp + position, "Date: %s", ctime(&t));

    position += sprintf(slot->resp + position, "Server: spokendotcpp\nConnection: close\n");

    if( info->code_resp != C_OK ){
        position += sprintf(slot->resp + position,
                "Content-Type: text/html\n\n"
                "<html><head>\n"
                "<meta charset=\"utf-8\">"
                "<title>ERROR %d</title>\n"
                "</head><body>\n"
                "<h1>%s</h1>\n"
                "</body></html>\n",
                info->code_resp, errorMessage);
        return;
    }


    switch( info->id_meth ){
        case M_TRACE :
            position += snprintf(slot->resp + position, REQSIZE - position, "Content-Type: message/http\n\n%s", slot->resp);
        return;

        case M_GET :
            position += sprintf(slot->resp + position,
                "Content-Type: text/html\n\n"
                "<html><head>\n"
                "<meta charset=\"utf-8\">"
                "<title>Success</title>\n"
                "</head><body>\n"
                "<h1>%s found</h1>\n"
                "</body></html>\n", info->path);
        return;

        default: return;
    }
}

int read_request(Slot* slot){
    int result = read_remaining_request(slot);
    if( result <= 0 ){
        perror("fail reading remaining request\n");
        return -1;
    }

    int header = search_end_header(slot, 0);
    if( header < 0 ){
        perror("no end request found\n");
        return -1;
    }

    Info_header info;
    analyse_request(slot, &info);
    prepare_response(slot, &info);

    slot->state = S_WRITE_RESPONSE;

    return result;
}

int write_response(Slot* slot){

    int result = write_remaining_response(slot);
    if( result <= 0 ){
        perror("writing remaining response fail\n");
        return -1;
    }

    if( slot->resp_pos < (int) strlen(slot->resp) ) return 1; // Si l'entête n'a pas été complètement écrite

    return 0;
}


void handle_ready_slot(Slot* slot, fd_set* set_read, fd_set* set_write){
    int request = 1;

    if( is_free_slot(slot) ) return;

    if( slot->state == S_READ_REQUEST && FD_ISSET(slot->soc, set_read) )
        request = read_request(slot);

    else if( slot->state == S_WRITE_RESPONSE && FD_ISSET(slot->soc, set_write) )
        request = write_response(slot);

    if(request <= 0) free_slot(slot);
}

void prepare_select(Server* ser, int* maxfd, fd_set* set_read, fd_set* set_write){
    Slot* slot;

    *maxfd = ser->listen_soc;

    FD_ZERO(set_read);
    FD_ZERO(set_write);
    FD_SET(ser->listen_soc, set_read);

    for(unsigned int i = 0; i < SLOTS_NB; ++i){
        slot = &ser->slots[i];

        if( is_free_slot(slot) ) continue;

        if( slot->state == S_READ_REQUEST  ){
            FD_SET(slot->soc, set_read);
            if( slot->soc > *maxfd ) *maxfd = slot->soc;
        }

        if( slot->state == S_WRITE_RESPONSE ){
            FD_SET(slot->soc, set_write);
            if( slot->soc > *maxfd ) *maxfd = slot->soc;
        }
    }
}

int do_select(Server* ser){
    int maxfd;

    fd_set set_read;
    fd_set set_write;

    printf("init prepare_select ... \n");
    prepare_select(ser, &maxfd, &set_read, &set_write);

    int result = select(maxfd + 1, &set_read, &set_write, NULL, NULL);
    if(result <= 0) {
        perror("Erreur : do select fail\n");
        return -1;
    }

    if( FD_ISSET(ser->listen_soc, &set_read) ) accept_connection(ser);

    for(int i = 0; i < SLOTS_NB; i++)
        handle_ready_slot(&ser->slots[i], &set_read, &set_write);

    return 0;
}


int main(int argc, char* argv[]) {
    bor_signal(SIGINT, handleSignal, 0);
    bor_signal(SIGPIPE, handleSignal, 0);

    if( argc != 2 ){
        perror("Erreur : nombre d arguments incorrects\n");
        return -1;
    }

    Server ser;
    int serPort = atoi(argv[1]);

    printf("init server ... ");
    if( start_server(&ser, serPort) < 0 ) return -1;
    printf("done !\n");

    for(;;){
        printf("init select ... \n");
        if( do_select(&ser) < 0 ){
            perror("fail\n");
            break;
        }

        if( shutDown == 0 ) break;
    }

    close_server(&ser);
    printf("server down\n");

    return 0;
}
