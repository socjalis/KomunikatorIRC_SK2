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
#include <fcntl.h>
#include <sys/stat.h>

#define SERVER_PORT 1234
#define QUEUE_SIZE 5

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
//TODO
    int fd_socket;
	int id;
	int cons[100];
};
//TODO
int unsubscribe(char* username, char* topic){
	FILE * file;
    file = fopen(topic ,"r");
	if (file == NULL)
    {
        puts("Nowa kategoria, dodaje plik z subskrybentami");
		open (topic, O_WRONLY | O_CREAT, 0777);
		file = fopen(topic, "w");
	    if (file == NULL)
		{
			perror("Blad przy otwieraniu users.txt");
		}
    }
    else{
		int found = 0;
		char word[20] = "";
		while(fscanf(file, "%s", word) != EOF)
		{	
			if(strcmp(word, username) == 0)
			{
				//już było
				return 0;
			}
		}
	}

	fprintf(file, "%s\n", username);
	fflush(file);
	return 1;
}

int subscribe(char* username, char* topic){
	FILE * file;
    file = fopen(topic ,"r");
	if (file == NULL)
    {
        puts("Nowa kategoria, dodaje plik z subskrybentami");
		open (topic, O_WRONLY | O_CREAT, 0777);
		file = fopen(topic, "w");
	    if (file == NULL)
		{
			perror("Blad przy otwieraniu users.txt");
		}
    }
    else{
		int found = 0;
		char word[20] = "";
		while(fscanf(file, "%s", word) != EOF)
		{	
			if(strcmp(word, username) == 0)
			{
				//już było
				return 0;
			}
		}
	}

	fprintf(file, "%s\n", username);
	fflush(file);
	return 1;
}

int login(char username[10])
{    
    FILE * file;
    file = fopen ("users.txt","r");
    if (file == NULL)
    {
        perror("Blad przy otwieraniu users.txt");
    }
    int found = 0;
    char word[20] = "";
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
        fprintf(file, "%s\n", username);
        printf("%s", username);
        fflush(file);
        fflush(stdout);
        
        return 0;
    }
    
}

int readn(int socket, int size, char* buf){
	int i = 0;
	char c;
	while(read(socket, &c, 1) > 0){
		if(c == '\n' || i == 10) break;
		buf[i] = c;
		i++;
	}
	return i;
}
//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *t_data)
{
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    //dostęp do pól struktury: (*th_data).pole
    char buf[10] = "";
	
	char *msg;
	char client_msg[1000];
	
	msg = "Połączono, podaj login\n";
	write(th_data->fd_socket, msg, strlen(msg));

	//wczytanie loginu
    readn(th_data->fd_socket, 10, buf);
    

	//login
    int check_login = login(buf);
	if(check_login == 0){
		msg = "Nowy użytkownik, założono konto\n";
		write(th_data->fd_socket, msg, strlen(msg));
	}
	else{
		msg = "Zalogowano\n";
		write(th_data->fd_socket, msg, strlen(msg));
	}
	
	while(1==1){
		readn(th_data->fd_socket, 1000, client_msg);
		if(client_msg[0] == '1'){
			puts("Dodanie subskrypcji\n");
			char topic[1000]="";
			strcpy(topic, &client_msg[1]);
			puts(topic);
			subscribe(buf, topic);
		}
		else if(client_msg[0] == '2'){
			puts("Usunięcie subskrypcji\n");
			char topic[49]="";
			memcpy(topic, &client_msg[1], 49);
			puts(topic);
			unsubscribe(buf, topic);
		}
		else if(client_msg[0] == '3'){
			puts("Dodanie wiadomości\n");
			char topic[49]="";
			memcpy(topic, &client_msg[1], 49);
			puts(topic);
			char post[950]="";
			strcpy(post, &client_msg[50]);
			//send(topic, post);
		}
	}
    free(th_data);

    close(th_data->fd_socket);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor, int id, int* cons) {
    //wynik funkcji tworzącej wątek
    int create_result = 0;

    //uchwyt na wątek
    pthread_t thread1;

    //dane, które zostaną przekazane do wątku
    struct thread_data_t *t_data = (struct thread_data_t*) malloc(sizeof(struct thread_data_t));
    t_data->fd_socket = connection_socket_descriptor;
	t_data->id = id;
	memcpy(t_data->cons, cons, sizeof t_data->cons);
    
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
	int connections [100];
	int id = 0;
	
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
    setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val)); 
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
        handleConnection(connection_socket_descriptor, id, connections);
		connections[id] = connection_socket_descriptor;
		id++;
    }   
    close(server_socket_descriptor);
    return(0);
}
