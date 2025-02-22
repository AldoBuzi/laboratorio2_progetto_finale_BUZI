
#define _GNU_SOURCE

#include <sys/socket.h>
#include <netinet/in.h>
#include "xerrori.h"
#include <unistd.h>
#include <arpa/inet.h>
#define HOST "127.0.0.1"
#define PORT 55116
#define Max_sequence_length 2048
#define here __LINE__, __FILE__
ssize_t writen(int fd, void *ptr, size_t n) {
    size_t   nleft;
    ssize_t  nwritten;
    nleft = n;
    while (nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) < 0) {
            if (nleft == n) return -1; /* error, return -1 */
            else break; /* error, return amount written so far */
        } else if (nwritten == 0) break;
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n - nleft); /* return >= 0 */
}

void *thread_body(void *args){
    char *fileName = (char *) args;
    FILE *f = fopen(fileName, "r");
    if(f == NULL) termina("Errore apertura file");
    struct sockaddr_in serv_addr;
    int fd_skt = socket(AF_INET, SOCK_STREAM, 0);
    if(fd_skt < 0) termina("Errore socket");
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);

    if (connect(fd_skt, &serv_addr, sizeof(serv_addr)) < 0)
        termina("Errore apertura connessione");
    char connection_type = 'b';
    writen(fd_skt,&connection_type,sizeof(char)  );
    char *line_buf = NULL;
    size_t line_buf_size = 0;
    while(true){
        ssize_t  s = getline(&line_buf, &line_buf_size, f);
        if(s==EOF) break;
        if(s <0) termina("Errore lettura");
        assert(strlen(line_buf) <2048);
        line_buf[strlen(line_buf)+1] = '\0';
        short temp = (short) strlen(line_buf)+1;
        short length = (short) htons(temp);
        ssize_t e = writen(fd_skt,&length,sizeof(short) );
        if(e!=sizeof(short)) termina("Errore write2");
        e = writen(fd_skt,line_buf,sizeof(char) * (strlen(line_buf)+1));
        if(e!=sizeof(char) * strlen(line_buf)+1) termina("Errore write");
    }
    free(line_buf);
    short n = 0;
    ssize_t e = writen(fd_skt,&n,sizeof(short)  );
    if(e!=sizeof(short)) termina("Errore write valore di uscita");
    if(close(fd_skt)<0)
        perror("Errore chiusura socket");
    fclose(f);
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    if(argc < 2) termina("Devi passare il nome di almeno un file da aprire");
    pthread_t thread_array[argc-1];
    for(int key = 0; key < argc-1; key++){
        xpthread_create(&thread_array[key],NULL,&thread_body, (void *) argv[key+1],here);
    }
    for(int key = 0; key < argc-1; key++){
        xpthread_join(thread_array[key],NULL,here);
    }
    return 0;
}
