@@ -0,0 +1,253 @@
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define RCVBUFSIZE 32   /* Size of receive buffer */
#define N    40         /* Max name's lenght */
#define DIM 10

// global variables

 char tril[DIM][DIM];	// Triliza Ground
 int line, col;		// line, col for playing

// functions

 void connect_server(int fd, unsigned short ServPort, char *servIP){
    /* server address structure */
    struct sockaddr_in ServAddr;     /* server address */
    int err;

    memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
    ServAddr.sin_family      = AF_INET;             /* Internet address family */
    ServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    ServAddr.sin_port        = htons(ServPort); /* Server port */

    /* Connect to server */
    err = connect(fd, (struct sockaddr *) &ServAddr, sizeof(ServAddr));
    if ( err < 0) {
        printf("connect() failed");
        exit(-1);
    }

 }

 void send_server(int fd, char *buff){	    			/* Send to the server */
    int err;
    err = write(fd, buff, RCVBUFSIZE);
    if ( err < 0 ){
     printf("Error in write()\n Goodbye!!\n");
     exit(-1);
    }
 }

 void read_server(int fd, char *buff2){       		/* Read fromt the server */
    int err;
    do{
      err = read(fd, buff2, RCVBUFSIZE);
      if ( err < 0 ){
       printf("Error in read()\n Goodbye!!\n");
       exit(-1);
      }
    } while( err <= 0);
 }

 void init_tril(){			// initialize triliza ground
    int i,j;
    for(i=0; i<DIM; i++){
	for(j=0; j<DIM; j++){
		tril[i][j] = '-';
	}
    }
 }

 void print_tril(){			// print triliza ground
        int i, j, flag;

        printf("\n\n  ");			// space in the beginning

        for(i=0; i<DIM; i++){		// Numbers for the cols
		printf("  %d ", i);
        }
	printf("\n  ");

        for(i=0; i<DIM; i++){		// first line of the triliza ground
                printf(" ___");
        }
        printf("\n  ");			// end of first line

        for(i=0; i<DIM*3; i++){			// the lines
                flag = (i+1) % 3;		// to separate the boxes
                for(j=0; j<DIM; j++){		// the cols
                        if (flag < 1){
                                printf("|___");
                        } else if (flag == 2){
                                printf("| %c ", tril[i/3][j]);	// print the symbol in the middle of the box
                        } else  printf("|   ");
                }
		if (flag == 1){
			printf("|\n%d ", i/3);
		} else  printf("|\n  ");			// end of line
        }

        printf("\n\n");		// space in the end
 }

 void play(int fd, char *buff, char *buff_r){
	char cords[3];		// cordinates to play

    do{

        printf("Give the cordinates you want to play [ usage:  line,column] ( 0 - 9) \n");  //play
        scanf("%s", cords);

        line = cords[0] - '0';
        col = cords[2] - '0';
        tril[line][col] = 'X';       // update triliza ground
        print_tril();                   // print triliza

        // send to server
        strcpy(buff, "TARGET ");
        strcat(buff, cords);
        send_server(fd, buff);

        do{
          read_server(fd, buff_r);
        } while (strncmp(buff_r, "TARGET",6));

    } while (strcmp(buff_r, "TARGET_ERROR") == 0);


 }

 void opp_play(int fd, char *buff_r){
	char cords[3];		// cordinates to play

        printf("Waiting opponent...\n");
        sleep(2);

        do{
          read_server(fd, buff_r);
        } while (strncmp(buff_r, "TARGET ",7));

        strcpy(cords, buff_r + 7);

        line = cords[0] - '0';
        col = cords[2] - '0';
        tril[line][col] = 'O';       // update triliza ground
        print_tril();                   // print triliza

 }

int check_winner(int fd,char *buff_win){
	int win = 0;

   	do{
          read_server(fd, buff_win);  // read buffer for winner check
	} while (strncmp(buff_win, "WINNER", 6));
	if ( strncmp(buff_win, "WINNER ", 7) == 0 ) win = 1;
	return win;
}

int main(int argc, char **argv)
{
    int fd;                     		        /* Socket descriptor */
    unsigned short ServPort;         			/* server port */
    char *servIP;                    			/* Server IP address (dotted quad) */
    int err; 			 		        // error checking
    char buff[RCVBUFSIZE], buff_r[RCVBUFSIZE];	        /* Buffer for echo string */
    char buff_win[RCVBUFSIZE];
    char num[1], name[N];				// Client's number and name
    char pl[1];						// who is playing first
    int i;						// for loops

    if (argc != 3)    /* Test for correct number of arguments */
    {
       printf("Usage: %s  <Echo Port> <Server IP>\n", argv[0]);
       exit(1);
    }

    ServPort = atoi(argv[1]); /* given port in arg 1 */
    servIP = argv[2];         /* Second arg: server IP address (dotted quad) */

    /* Create a reliable, stream socket using TCP */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if ( fd < 0) {
        printf("socket() failed\n");
        exit(-1);
    }

    connect_server(fd, ServPort, servIP);   // connect to server

    printf("Please give your name: \n");
    scanf("%s", name);
    strcpy(buff, "HELLO ");
    strcat(buff, name);

    send_server(fd, buff);  // send the name to server

    read_server(fd, buff_r); // read from server the number that client has
    strcpy(num, buff_r+8);
    printf("Welcome %s, you are player %s \n", name, num);

    strcpy(buff_win, "WINNER_NO"); // initialize winner buffer

    do{
      read_server(fd, buff_r);
    } while (strcmp(buff_r, "STARTING"));

    printf("\n%s\n", buff_r);

    do{
      read_server(fd, buff_r);
    } while (strncmp(buff_r, "BATTLE", 6));

    init_tril();			// initialize the triliza table

    strcpy(pl, buff_r+7); // who plays first
    if (strncmp(pl, num, 1) == 0){
	printf("You are playing first\n");
	for(i=0;;i++){
		play(fd, buff, buff_r); // playing
		err = check_winner(fd, buff_win);
		if ( err == 1 ) break;
		strcpy(buff, "  ");
		strcpy(buff_r, "  ");
		opp_play(fd, buff_r);   //opponent is playing
		err = check_winner(fd, buff_win);
		if ( err == 1 ) break;
	}
    } else {
	printf("Opponent is playing first\n");
	for(i=0;;i++){
		opp_play(fd, buff_r); //opponent is playing
		err = check_winner(fd, buff_win);
		if ( err == 1 ) break;
		strcpy(buff, "  ");
		strcpy(buff_r, "  ");
		play(fd, buff, buff_r); // playing
		err = check_winner(fd, buff_win);
		if ( err == 1 ) break;
	}
    }
    printf("\n");    /* Print a final linefeed */
    printf("\n");    /* Print a final linefeed */

    strcpy(pl, buff_win+7); // who WON
    if (strncmp(pl, num, 1) == 0){
	printf("YOU WON !!!!!!!\n\nGOODBYE\n");
    } else printf("YOU LOST !!\n\nGOODBYE\n");

    printf("\n");    /* Print a final linefeed */

    close(fd);
    return 0;
}


