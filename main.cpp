#include "locker.h"
#include "http_conn.h"
#include "threadpool.h"
#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <error.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>

#define MAX_FD 65535
#define MAX_EVENTS_NUM 10000
extern void addfd(int epollfd, int fd, bool oneshot);
extern void removefd(int epollfd, int fd);
extern void modfd(int epollfd, int fd, int ev);

void addsig(int sig, void(handler)(int)){

    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}


int main(int argc, char* argv[]){


    if(argc <= 1){
        
        printf("按照以下方式运行 %s port_num\n", basename(argv[0]));
        return 1;
    }

    int port = atoi(argv[1]);
    addsig(SIGPIPE, SIG_IGN);

    threadpool<http_conn>* pool = NULL;

    try{

        pool = new threadpool<http_conn>;
    }catch(...){

        return -1;
    }

    http_conn* users = new http_conn[MAX_FD];

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){

        perror("socket");
        exit(-1);
    }
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);

    int ret = bind(listenfd, (struct sockaddr*)& saddr, sizeof(saddr));
    if(ret == -1){

        perror("bind");
        exit(-1);
    }
    ret = listen(listenfd, 5);
    if(ret == -1){

        perror("listen");
        exit(-1);
    }

    struct epoll_event events[MAX_EVENTS_NUM];
    int epollfd = epoll_create(5);
    addfd(epollfd, listenfd, false);

    http_conn::m_epollfd = epollfd;


    while(true){

        int num = epoll_wait(epollfd, events, MAX_EVENTS_NUM, -1);
        if(num < 0 && errno != EINTR){

            printf("epoll failure\n");
            break;
        }
        for(int i = 0; i < num; ++i){

            int sockfd = events[i].data.fd;
            if(sockfd == listenfd){

                struct sockaddr_in caddr;
                socklen_t len = sizeof(caddr);
                int connfd = accept(sockfd, (struct sockaddr*) &caddr, &len);
                if(http_conn::m_user_count >= MAX_FD){

                    close(connfd);
                    continue;
                }

                users[connfd].init(connfd, caddr);


            }else if(events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)){

                users[sockfd].close_conn();
            }else if(events[i].events & EPOLLIN){

                if(users[sockfd].read()){

                    pool->append(users + sockfd);
                }else{
                    users[sockfd].close_conn();
                }

            }else if(events[i].events & EPOLLOUT){

                if(!users[sockfd].write()){

                    users[sockfd].close_conn();
                }
            }
        }
    }
    close(epollfd);
    close(listenfd);
    delete []users;
    delete pool;
    return 0;
}