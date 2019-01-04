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
#include <dirent.h> 

#define SERVER_PORT 1234
#define QUEUE_SIZE 5

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
    int fd_socket;
	int id;
	int* cons;
};
void parse_msg(char* msg, char* type, char* topic, char* post, char* buf){
	int size = strlen(msg);
	int i = 0;
	int delimiters = 0;
	int currentchar = 0;
	msg[1]='\0';
	strcpy(type, &msg[0]);

	type[1] = '\0';

	for(i = 2; i < size; i++){
		if(msg[i] != '#'){
			if(delimiters == 0) topic[currentchar] = msg[i];
			else if(delimiters == 1) post[currentchar] = msg[i];
			currentchar++;
		}
		else{
			if (delimiters == 1) {
				post[currentchar] = '#';
			}
			delimiters++;
			currentchar = 0;
			if (delimiters > 2) return;
		}
	}
}
void sendpost(char* topic, char* post, int* cons, char* username){
	DIR *d;
	struct dirent *dir;
	d = opendir("./sessions");
	if(d){
		//przegladam sesje uzytkownikow
    	while ((dir = readdir(d)) != NULL) {
			if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
			{
				char pathname[40] = "./sessions/";
				strcat(pathname, dir->d_name);
				FILE* file;
				file = fopen(pathname, "r");
				//wyszukuje w sesji uzytkownika tematu
				char word[20] = "";
    			while(fscanf(file, "%s", word) != EOF)
    			{
    				if(strcmp(word, topic) == 0)
    				{
						int id;
						sscanf(dir->d_name, "%d", &id);
						printf("id: %d\n", id);
						fflush(stdout);
						char fmsg[1100] = "";
						strcat(fmsg, username);
						strcat(fmsg, "(");
						strcat(fmsg, topic);
						strcat(fmsg, "): ");
						strcat(fmsg, post);

						write(cons[id], fmsg, strlen(fmsg));
    				}
    			}
				fclose(file);
			}
    	}
  	}
	closedir(d);
}
int restore_session(char* username, int id){
	DIR *d;
	struct dirent *dir;
	d = opendir("./topics");
	//jeśli otworzylismy folder...
	if(d){
		char word[100] = "";
		FILE* file;
		char pathname[40] = "./sessions/";
		char str[12];
		sprintf(str, "%d", id);
		strcat(pathname, str);
		open (pathname, O_WRONLY | O_CREAT, 0777);
		file = fopen(pathname, "w");

		
		//sprawdzamy wszystkie tematy, w poszukiwaniu wskazanego uzytkownika
    	while ((dir = readdir(d)) != NULL) {
			if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
			{
				char pathname2[40] = "./topics/";
				//printf("%s\n", dir->d_name);
				fflush(file);
				FILE* file2;
				strcat(pathname2, dir->d_name);
				file2 = fopen(pathname2, "r");
				if (file2 == NULL){
					perror("Błąd przy otwieraniu pliku w funkcji restore_session");
				}
    			while(fscanf(file2, "%s", word) != EOF)
    			{
					//jesli znalezlismy uzytkownika zapisanego do danego tematu, dodajemy do jego sesji ten
    				if(strcmp(word, username) == 0)
    				{
						
						fprintf(file, "%s\n", dir->d_name);
						fflush(file);
						break;
    				}
    			}
				fclose(file2);
			}
    	}
		fclose(file);
    	closedir(d);
  	}
	return 0;
}

int subscribe(char* username, char* topic, int id){
	char pathname[40] = "./topics/";
	strcat(pathname, topic);

	FILE * file;
    file = fopen(pathname ,"r");
	if (file == NULL)
    {
        puts("Nowa kategoria, dodaje plik z subskrybentami");
		open (pathname, O_WRONLY | O_CREAT, 0777);

    }
    else{
		puts("Istniejąca kategoria, dodaję subskrybcję");
		char word[20] = "";
		while(fscanf(file, "%s", word) != EOF)
		{	
			if(strcmp(word, username) == 0)
			{
				//już było
				puts("Subskrypcja była już dodana!");
				fclose(file);
				return 0;
			}
		}
	}
	
	file = fopen(pathname, "a");
    if (file == NULL)
	{
		perror("Blad przy otwieraniu ");
	}
	fprintf(file, "%s\n", username);
	fflush(file);
	fclose(file);
	restore_session(username, id);
	return 1;
}

int login(char username[10]){  
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
        fclose(file);
        return 1;
    }
    else{
        fclose(file);
        file = fopen ("users.txt","a");
        if (file == NULL)
        {
            perror("Blad przy otwieraniu users.txt");
        }
        fprintf(file, "%s\n", username);
        fflush(file);
        
        return 0;
    }
    
}

int readn(int socket, int size, char* buf){
	int i = 0;
	char c;
	while(read(socket, &c, 1) > 0){
		if(i == size) break;
		buf[i] = c;
		i++;
		if(c == '\n') break;
	}
	return i;
}

int read_to_delimiter(int socket, int size, char* buf){
	int i = 0;
	char c;
	while(read(socket, &c, 1) > 0){
		if(c == '\n' || i == size) break;
		buf[i] = c;
		i++;
	}
	return i;
}
//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *t_data){
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    char buf[20] = "";
	
	char *msg;
	
	msg = "Połączono, podaj login\n";
	write(th_data->fd_socket, msg, strlen(msg));

	//wczytanie loginu
    read_to_delimiter(th_data->fd_socket, 20, buf);
    
	//login
    int check_login = login(buf);
	if(check_login == 0){
		msg = "Nowy użytkownik, założono konto\n";
		write(th_data->fd_socket, msg, strlen(msg));
	}
	else{
		msg = "Zalogowano\n";
		write(th_data->fd_socket, msg, strlen(msg));
		//wnowienie sesji
		puts("Wznowienie sesji\n");
		restore_session(buf, th_data->id);
	}

	//glówna pętla wątku
	while(1==1){
		char client_msg[1000];
		strcpy(client_msg, "");
		read_to_delimiter(th_data->fd_socket, 1000, client_msg);
		char type [2] = "";
		char topic[100] = "";
		char post [898] = "";
		parse_msg(client_msg, type, topic, post, buf);
		char c = type[0];
		if(c == '1'){
			puts("Dodanie subskrypcji");
			puts("Temat:");
			puts(topic);
			subscribe(buf, topic, th_data->id);
		}
		else if(c == '3'){
			puts("Wysyłanie wiadomości");
			puts("Temat:");
			puts(topic);
			
			puts("Post:");
			puts(post);
			sendpost(topic, post, th_data->cons, buf);
		}
		else if(c == '9'){
			puts("Zakończenie wątku");
			char path[100] = "exec rm -r -f ./sessions/";
			char sid[5] = "";
			sprintf(sid, "%d", th_data->id);
			strcat(path, sid);
			system(path);
			break;
		}
		strcpy(client_msg, "");
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
	t_data->cons = cons;
    
    create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
       printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       exit(-1);
    }
}

int main(int argc, char* argv[])
{
    int server_socket_descriptor;
    int connection_socket_descriptor;
    int bind_result;
    int listen_result;
    char reuse_addr_val = 1;
    struct sockaddr_in server_address;
	int connections [100];
	int id = 0;

	//czyszczenie sesji
	system("exec rm -r -f ./sessions/*");
    
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
	puts("KONIEC DZIAŁANIA PROGRAMU");
    return(0);
}
