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

#define MSG_BUF_SIZE 256

char* parse(char* in_buf, char* out_buf) {
    char leftNumber[MSG_BUF_SIZE];
    char rightNumber[MSG_BUF_SIZE];
    char operator = 0;
    
    memset(&leftNumber[0], 0, MSG_BUF_SIZE);
    memset(&rightNumber[0], 0, MSG_BUF_SIZE);
    
    int operatorPosition = -1;
    for(int i = 0; i < strlen(in_buf); i++) {
        if(in_buf[i]=='+' || in_buf[i]=='-' || in_buf[i]=='*' || in_buf[i]=='/') {
            operatorPosition = i;
            break;
        }
    }
    
    strncpy (&leftNumber[0], in_buf, operatorPosition);
    operator = in_buf[operatorPosition];
    //the -1 is needed so that we dont copy the \n into the rightNumber string
    strncpy (&rightNumber[0], in_buf+operatorPosition+1, strlen(in_buf+operatorPosition+1)-1);
    
    //hex gets auto detected
    int left = strtol(leftNumber, NULL, 0);
    int right = strtol(rightNumber, NULL, 0);
    
    //bin not, so we see if the condition for a binary number is fullfilled
    int length = 0;
    length = strlen(leftNumber);
    if(length >= 2) {
        if(leftNumber[1] != 'x' && leftNumber[length-1] == 'b') {
            leftNumber[length-1] = '\0';
            left = strtol(leftNumber, NULL, 2);
        }
    }
    
    length = strlen(rightNumber);
    if(length >= 2) {
        if(rightNumber[1] != 'x' && rightNumber[length-1] == 'b') {
            rightNumber[length-1] = '\0';
            right = strtol(rightNumber, NULL, 2);
        }
    }
    
    int result = 0;
    switch(operator) {
        case '+': {
            result = left + right; 
            //not the best solutions, but shits boring who cares
            if(result < left + right) {
                sprintf(out_buf, "Zahlen zu groß! Überlauf!");
                return out_buf;
            }
            break;
        }
        case '-': {
            result = left - right; 
            //not the best solutions, but shits boring who cares
            if(result > left - right) {
                sprintf(out_buf, "Zahlen zu groß! Überlauf!");
                return out_buf;
            }
            break;
        }

        case '*': {
            result = left * right; 
            //not the best solutions, but shits boring who cares
            if(result < left * right) {
                sprintf(out_buf, "Zahlen zu groß! Überlauf!");
                return out_buf;
            }
            break;
        }
        case '/': {
            if(right != 0) {
                result = left / right; 
                break;
            }
            else {
                sprintf(out_buf, "Division durch null night möglich!");
                return out_buf;
                break;
            }
        }
    }
    
    int result_buf = result;
    int bin_str_len = 0;
    char bin_str_buf[34];
    
    for(int i = 31; i>=0; i--) {
        if((result_buf & 1) == 1)
            bin_str_len = i;
        bin_str_buf[i] = '0' + (result_buf & 1);
        result_buf = result_buf >> 1;
    }
    bin_str_buf[32] = 'b';
    bin_str_buf[33] = '\0';
    
    sprintf(out_buf, "%d 0x%X %s", result, result, &bin_str_buf[bin_str_len]);
    
    return out_buf;
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
	hints.ai_family = AF_INET6;
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
                char in_buf[MSG_BUF_SIZE];
                char out_buf[MSG_BUF_SIZE];
                memset(&in_buf,0,MSG_BUF_SIZE);
                memset(&out_buf,0,MSG_BUF_SIZE);
                
                if (recv(new_fd, in_buf, MSG_BUF_SIZE, 0) == -1) {
                    perror("recv");
                    exit(1);
                }
                
                parse(in_buf, out_buf);
                if (send(new_fd, out_buf, strlen(out_buf), 0) == -1) {
                    perror("send");
                    exit(1);
                }
            }
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

