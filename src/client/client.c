/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <regex.h>

#include <arpa/inet.h>

#define STD_PORT "5000" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
    
}

char* checkStr(char* input){
    
    char* output = malloc(100);
    char* leftNumber = malloc(100);
    char* rightNumber = malloc(100);
    int operatorPosition = -1;
    for(int i = 0; i < strlen(input); i++) {
        if(input[i]=='+' || input[i]=='-' || input[i]=='*' || input[i]=='/') {
            operatorPosition = i;
            break;
        }
    }
    if(operatorPosition ==-1){
        input[strlen(input)-1] = '\0';
        sprintf(output, "%s%s\n", input, "+0");
        sprintf(leftNumber,"%s",input);
        sprintf(rightNumber,"%d",0);
    }else{
        sprintf(output,"%s",input);
        strncpy (leftNumber, input, operatorPosition);
        strncpy (rightNumber, input+operatorPosition+1, strlen(input+operatorPosition+1));
    }
    
    //"^[0-9]+$"
    //0[xX][0-9a-fA-F]+
    //(^[0-9]*$|0[xX][0-9a-fA-F]+)
    //(^[0-9]*$|0[xX][0-9a-fA-F]+|[01]+b)
    
    regex_t re;
    if (regcomp(&re, "([0-9]+|0[xX][0-9a-fA-F]+|[01]+b)", REG_EXTENDED|REG_NOSUB) != 0) return "-1";
    int status = regexec(&re, leftNumber, 0, NULL, 0);
    regfree(&re);
    if (status != 0) return "-1";
    if (regcomp(&re, "([0-9]+|0[xX][0-9a-fA-F]+|[01]+b)", REG_EXTENDED|REG_NOSUB) != 0) return 0;
    status = regexec(&re, rightNumber, 0, NULL, 0);
    regfree(&re);
    if (status != 0) return "-1";
    return output;
    
}


int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (!(argc == 2 || argc == 3)) {
	    fprintf(stderr,"usage: hostname [port]\n");
	    exit(1);
	}

    char* port = STD_PORT;
	if (argc == 3) {
        port = argv[2];
    }
        
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
    
    while(1) {
        char str[100];
        memset(&str,0,100);
        fgets(str,100,stdin);
        
        char* checkedStr= checkStr(str);
        if(checkedStr=="-1"){
            printf("Wrong Input\n");
            continue;
        }
        //getc(stdin);
        printf("sending: %s",checkedStr);
        send(sockfd, checkedStr, strlen(checkedStr), 0);
        //send(sockfd, str, strlen(str), 0);
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
        }
        buf[numbytes] = '\0';

        printf("client: received '%s'\n",buf);
    }

	

	close(sockfd);

	return 0;
}

