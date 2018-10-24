/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define STD_PORT "5000"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

char* parse(char* input) {
    char* leftNumber = malloc(100);
    char* rightNumber = malloc(100);
    char* operator = malloc(1);
    
    int operatorPosition = -1;
    for(int i = 0; i < strlen(input); i++) {
        if(input[i]=='+' || input[i]=='-' || input[i]=='*' || input[i]=='/') {
            operatorPosition = i;
            break;
        }
    }
    
    strncpy (leftNumber, input, operatorPosition);
    strncpy (operator, input+operatorPosition, 1);
    strncpy (rightNumber, input+operatorPosition+1, strlen(input+operatorPosition+1));
    
    //printf("leftNumber %s \n", leftNumber);
    //printf("operator %s \n", operator);
    //printf("rightNumber %s \n", rightNumber);
    
    int left = strtol(leftNumber, NULL, 0);
    int right = strtol(rightNumber, NULL, 0);
    
    if(strlen(leftNumber) >= 2) {
        if(leftNumber[1] != 'x' && leftNumber[strlen(leftNumber)-1] == 'b') {
            left = strtol(leftNumber, NULL, 2);
        }
    }
    if(strlen(rightNumber) >= 2) {
        if(rightNumber[1] != 'x' && rightNumber[strlen(rightNumber)-1] == 'b') {
            right = strtol(rightNumber, NULL, 2);
        }
    }
    
    int result = 0;
    switch(operator[0]) {
        case '+': result = left + right; break;
        case '-': result = left - right; break;
        case '*': result = left * right; break;
        case '/': result = left / right; break;
    }
    
    int result_buf = result;
    int bin_str_len = 0;
    char bin_str_buf[34];
    
    for(int i = 31; i>=0; i--) {
        if(result_buf&1 == 1)
            bin_str_len = i;
        bin_str_buf[i] = '0' + (result_buf & 1);
        result_buf = result_buf >> 1;
    }
    bin_str_buf[32] = 'b';
    bin_str_buf[33] = '\0';
    
    char* output = malloc(100);
    sprintf(output, "%d 0x%X %s", result, result, &bin_str_buf[bin_str_len]);
    
    free(leftNumber);
    free(rightNumber);
    free(operator);
    
    //printf("%s", output);
    
    return output;
}

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


char* parseInput(char* input)
{
    
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

    char* port = STD_PORT;
	if (argc == 2) {
        port = argv[1];
    }
        
	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			
            while(1) {
                int numbytes;
                char buf[100];
                if ((numbytes = recv(new_fd, buf, 100-1, 0)) == -1) {
                    perror("recv");
                    exit(1);
                }

                buf[numbytes] = '\0';
                char* output = parse(buf);
                //printf("received '%s'\n",parse(buf));
                if (send(new_fd, output, strlen(output), 0) == -1)
                    perror("send");
            }
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

