@@ -0,0 +1,355 @@
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXPENDING 2    /* Maximum outstanding connection requests */
#define RCVBUFSIZE 40    /* Size of receive buffer */
#define N 40            /* Size of players names, enough i think */
#define DIM 10

 // global variables
 int r;		// r : the random number wich decides who plays first
 int conn;    // connected threads
 char triliza[DIM][DIM]; // the triliza gound buffer
 pthread_mutex_t lock;  // mutex for threads
pthread_barrier_t bar;
 char buff[RCVBUFSIZE], buff_r[RCVBUFSIZE], buff_win[RCVBUFSIZE];       /* Buffers for send, receive and winner string */
 char sc_buff[N];				// buffer for writing in score file string

 void init_triliza(){
	int i,j;
	for(i=0; i<DIM; i++){
		for(j=0; j<DIM; j++){
			triliza[i][j] = '-' ;
		}
	}
 }

 void bind_addr(int fd, unsigned short ServPort, char *IP){   // bind and listen
    struct sockaddr_in ServAddr;     /* Local address */
    int err;

    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));       /* Zero out structure */
    ServAddr.sin_family = PF_INET;                /* Internet address family */
    ServAddr.sin_addr.s_addr = inet_addr(IP);     /* IP */
    ServAddr.sin_port = htons(ServPort);          /* Local port */

    /* Bind to the local address */
    err = bind(fd, (struct sockaddr *) &ServAddr, sizeof(ServAddr));
    if (err < 0) {
        printf("bind() failed");
        exit(-1);
    }

    /* Mark the socket so it will listen for incoming connections */
    err = listen(fd, MAXPENDING);
    if (err < 0) {
        printf("listen() failed");
        exit(-1);
    }
 }

 int connect_client(int fd, struct sockaddr_in ClntAddr){
       unsigned int clntLen;            /* Length of client address data structure */
       int clnt_fd;

       clntLen = sizeof(ClntAddr);      /* Set the size of the in-out parameter */

       /* Wait for a client to connect */
       clnt_fd = accept(fd, (struct sockaddr *) &ClntAddr, &clntLen);
       if (clnt_fd < 0) {
           printf("accept() failed");
           exit(-1);
       }

      return clnt_fd;
 }

 void read_client(int clnt_fd, char *buff_r){     /* Receive message from client */
    int err;
    do{
      err = read(clnt_fd, buff_r, RCVBUFSIZE);
      if ( err < 0 ){
        printf("Error in read()\n Goodbye!!\n");
        exit(-1);
      }
    } while( err <= 0);
 }

 void write_client(int clnt_fd, char *buff2){        /* Send message to client */
    int err;
    err = write(clnt_fd, buff2, RCVBUFSIZE);
    if ( err < 0 ){
        printf("Error in write()\n Goodbye!!\n");
        exit(-1);
    }
 }

 struct thread_data{
   int thread_id;
   int sd;
 };

 void *clients( void *data ){
	int fd;				        	 /* Socket descriptor for server */
	int tid;					 // Thread id
	int clnt_fd;	   			         /* Socket descriptor for client */
        struct sockaddr_in ClntAddr;   			 /* Client address */
        int err, i, j;		    			 // error checking and i,j for loop
        char c[1]; 					 // To put integers into strings
        char name[N];					 /* Names of the players */
	struct thread_data *my_data;			 /* To transfer the data from the arg */
        char cords[3];   			         // cordinates to play
	int line, col;					 // line and col of triliza
	char w[1];					 // who won
	int s=0;					 // s for score

	// Save the data from the argument
	my_data = (struct thread_data *) data;
	tid = my_data->thread_id;
	fd = my_data->sd;

	clnt_fd = connect_client(fd, ClntAddr);

	read_client(clnt_fd, buff_r); // Read client's name
	strcpy(name, buff_r+6);

	strcpy(buff, "WELCOME ");     // Send client's number
	sprintf(c, "%d", tid);        // put the int into a string
	strcat(buff, c);

	write_client(clnt_fd, buff);
	printf("Connecting with client %d, name: %s\n", tid, name);
	fflush(NULL);

	/* Use mutex to prevent fault */
	pthread_mutex_lock(&lock);
	conn++; // When a new player is connected
	pthread_mutex_unlock(&lock);

	/* connected to client! */

	while(conn < 2) // Wait for 2 players to be connected

	/* When both are connected send the message */
	sleep(2);
	strcpy(buff, "STARTING");
	write_client(clnt_fd, buff);

	sleep(2);
	strcpy(buff, "BATTLE ");
	sprintf(c, "%d", r);        // put the int into a string
	strcat(buff, c);

	write_client(clnt_fd, buff); // Send the message "BATTLE 1" or "BATTLE 2"

     for(;;){
	sleep(1);
        err = 1; // to check

	if ( tid == r ){
           pthread_mutex_lock(&lock);
	   s++;
 	  do{
           do{
	     read_client(clnt_fd, buff_r); // Read client's play
           } while (strncmp(buff_r, "TARGET ",7));

	   // Check if client's cords are correct
	   strcpy(cords, buff_r + 7);
           line = cords[0] - '0';
           col = cords[2] - '0';
//	   printf("line = %d\ncol = %d\n", line,col);
	   if ((line < 0) || (line > DIM -1) || (col < 0) || (col > DIM-1)){
		err = 0;
		strcpy(buff, "TARGET_ERROR");
	   } else if ( triliza[line][col] != '-' ){
                err = 0;
                strcpy(buff, "TARGET_ERROR");
	   } else {
		strcpy(buff, "TARGET_OK");
		err = 1;
	   }

           write_client(clnt_fd, buff); // send response for client's play
	  } while(err == 0);

           if (r == 0) triliza[line][col] = 'X';
	   else  triliza[line][col] = 'O';

	   // Winner check
	   for(i=0; i<DIM-2; i++){
                for(j=0; j<DIM; j++){
                      if ( triliza[i][j] == 'X' && triliza[i+1][j] == 'X' && triliza[i+2][j] == 'X' ) strcpy(buff_win, "WINNER 0");
                      if ( triliza[i][j] == 'O' && triliza[i+1][j] == 'O' && triliza[i+2][j] == 'O' ) strcpy(buff_win, "WINNER 1");
		}
	   }
	   for(i=0; i<DIM; i++){
                for(j=0; j<DIM-2; j++){
                      if ( triliza[i][j] == 'X' && triliza[i][j+1] == 'X' && triliza[i][j+2] == 'X' ) strcpy(buff_win, "WINNER 0");
                      if ( triliza[i][j] == 'O' && triliza[i][j+1] == 'O' && triliza[i][j+2] == 'O' ) strcpy(buff_win, "WINNER 1");
		}
	   }
	   for(i=0; i<DIM-2; i++){
                for(j=0; j<DIM-2; j++){
                      if ( triliza[i][j] == 'X' && triliza[i+1][j+1] == 'X' && triliza[i+2][j+2] == 'X' ) strcpy(buff_win, "WINNER 0");
                      if ( triliza[i][j] == 'O' && triliza[i+1][j+1] == 'O' && triliza[i+2][j+2] == 'O' ) strcpy(buff_win, "WINNER 1");
		}
	   }
	   for(i=2; i<DIM; i++){
                for(j=0; j<DIM-2; j++){
                      if ( triliza[i][j] == 'X' && triliza[i-1][j+1] == 'X' && triliza[i-2][j+2] == 'X' ) strcpy(buff_win, "WINNER 0");
                      if ( triliza[i][j] == 'O' && triliza[i-1][j+1] == 'O' && triliza[i-2][j+2] == 'O' ) strcpy(buff_win, "WINNER 1");
		}
	   }
           pthread_mutex_unlock(&lock);
        }
        fflush(NULL);

	if ( tid != r ){
           while (strcmp(buff, "TARGET_OK")) sleep(1);

           pthread_mutex_lock(&lock);
	   strcpy(buff, buff_r);
	   write_client(clnt_fd, buff); // send other client's play
	   strcpy(buff, "  ");
	   strcpy(buff_r, "  ");
	   r++;
	   r = r % 2;
           pthread_mutex_unlock(&lock);
	}

	pthread_barrier_wait(&bar);

           pthread_mutex_lock(&lock);
        write_client(clnt_fd, buff_win); // send buffer for winner check
           pthread_mutex_unlock(&lock);

	pthread_barrier_wait(&bar);

	if ( strncmp(buff_win, "WINNER ", 7) == 0 ) break;
     }

        strcpy(w, buff_win+7); // who won
	sprintf(c, "%d", tid); // put tid into a string
	if (strncmp(w, c, 1) == 0){
	  strcpy(sc_buff, name);
   	  sprintf(c, "%d", s); // put s (score) into a string
	  strcat(sc_buff, ": ");
	  strcat(sc_buff, c);
	  strcat(sc_buff, "\n");
	}

	sleep(5);
	close(clnt_fd);    /* Close clients socket */
	pthread_exit(NULL);

 }

 int main(int argc, char **argv){
    int fd;                     		         /* Socket descriptor for server */
    char *IP;                     		         /* IP address (dotted quad) */
    unsigned short ServPort;        			 /* Server port */
    int err, i;			    			 // error checking and i for loop
    pthread_t thread[2];				 // Threads for clients
    struct thread_data thd[2];				 // Thread data for argument
    int* status;					 // for pthread_join
    char fname[]="score.txt";				 // file name for score saving file
    int sc;						 // file descriptor for score saving file

    if (argc != 3)     /* Test for correct number of arguments */
    {
        printf("Usage:  %s <Server Port> <IP>\n", argv[0]);
        exit(1);
    }

    ServPort = atoi(argv[1]);  /* First arg:  local port */
    IP = argv[2];              /* Second arg: server IP address (dotted quad) */

    /* Create socket for incoming connections */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if ( fd < 0) {
        printf("socket() failed");
        exit(-1);
    }

    bind_addr(fd, ServPort, IP);

    conn = 0; // 0 connected players yet

    /* Generate a random number to decide who will play first  */
    srand(time(NULL));
    r = rand() % 2;

    // initialize triliza ground
    init_triliza();

    // initialize barrier for threads
    pthread_barrier_init (&bar, NULL, 2);

    // connect with clients
    for(i=0; i<2; i++){
	/* Passind the arguments */
      thd[i].thread_id = i;
      thd[i].sd = fd;

	/* Creating the theads for the clients */
      err = pthread_create(&thread[i], NULL, *clients, (void *) &thd[i]);
      if (err < 0) {
	perror("Problem creating threads");
	fflush(NULL);
	exit(-1);
     }
    }
    /* Receive message from client
    read_client(clnt_fd, buff_r);

    strcpy(buff,buff_r);

    Send received string and receive again until end of transmission
    write_client(clnt_fd, buff);
    */

    strcpy(buff_win, "WINNER_NO"); // initialize winner buffer

	/* Waiting the threads to finish */
    for(i=0; i<2; i++){
	err = pthread_join(thread[i], (void *) &status);
	if (err < 0){
		perror("Error in pthread_join()");
		fflush(NULL);
		exit(-1);
	}
    }


    // score in .txt file
    sc = open(fname, O_RDWR | O_CREAT, (mode_t) 0600);
       if ( fd < 0 ) {
               perror("Problem opening file");
               exit(-1);
       }


    lseek(sc, 0, SEEK_END);
    err = write(sc,sc_buff,strlen(sc_buff)*sizeof(char));
    if ( err < 0 ){
       perror("I am sorry");
       exit(-1);
    }

    close(sc);
    close(fd);
    return 0;
 }
