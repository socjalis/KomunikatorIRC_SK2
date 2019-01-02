#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>


#define SERVER_PORT 1234
#define QUEUE_SIZE 5

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
//TODO
    int fd_socket;
    pthread_mutex_t* m;
};

int login(char username[10])
{
    FILE * file;
    file = fopen ("users.txt","r");
    if (file == NULL)
    {
        printf("Blad przy otwieraniu users.txt");
    }
    int found = 0;
    char *word = (char*)malloc(10);
    while(fscanf(file, "%s", word) != EOF)
    {
    	if(strcmp(word, username) == 0)
    	{
    		found = 1;
    	}
    }
    if(found == 1){
        printf("Found\n");
        fclose(file);
        return 1;
    }
    else{
        printf("Not found\n");
        fclose(file);
        file = fopen ("users.txt","a");
        if (file == NULL)
        {
            perror("Blad przy otwieraniu users.txt");
        }
        fprintf(file, "%s", username);
        printf("%s", username);
        fflush(file);
        fflush(stdout);
        
        return 0;
    }
    
}

//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *t_data)
{
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    //dostęp do pól struktury: (*th_data).pole
    char buf[10] = "";

    //pthread_mutex_lock(th_data -> m);
    read(th_data->fd_socket, buf, 10);
    //pthread_mutex_unlock(th_data -> m);


    printf("%s", buf);
    //workaround
    buf[strlen(buf)-1] = 0;
    //login
    int check_login = login(buf);


    free(th_data);

    close(th_data->fd_socket);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor, pthread_mutex_t* m1) {
    //wynik funkcji tworzącej wątek
    int create_result = 0;

    //uchwyt na wątek
    pthread_t thread1;

    //dane, które zostaną przekazane do wątku
    struct thread_data_t *t_data = (struct thread_data_t*) malloc(sizeof(struct thread_data_t));
    t_data->fd_socket = connection_socket_descriptor;
    //zamek
    t_data->m = m1;

    create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
       printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       exit(-1);
    }

    //TODO (przy zadaniu 1) odbieranie -> wyświetlanie albo klawiatura -> wysyłanie
}

int main(int argc, char* argv[])
{
    int server_socket_descriptor;
    int connection_socket_descriptor;
    int bind_result;
    int listen_result;
    char reuse_addr_val = 1;
    struct sockaddr_in server_address;
    pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
    //inicjalizacja gniazda serwera

    memset(&server_address, 0, sizeof(struct sockaddr));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);   
    server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_descriptor < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
        exit(1);
    }
    setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEPORT, (char*)&reuse_addr_val, sizeof(reuse_addr_val)); 
    bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
    if (bind_result < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
        exit(1);
    }   
    listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
    if (listen_result < 0) {
        fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
        exit(1);
    }
    
    while(1)
    {
        connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
        if (connection_socket_descriptor < 0)
        {
            fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
            exit(1);
        }   
        handleConnection(connection_socket_descriptor, &m1);
    }   
    close(server_socket_descriptor);
    return(0);
}
