//
//  to_csv.c
//  rp_client_tools
//
//  Created by Brandon Kinman on 11/15/14.
//  Copyright (c) 2014 Kinmantech. All rights reserved.
//

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "to_csv.h"

static int sock_fd = -1;
static int client_sock_fd = -1;

static int connection_init(int is_tcp, int is_client, const char *ip_addr, int ip_port);
static void connection_cleanup(void);

static int connection_init(int is_tcp, int is_client, const char *ip_addr, int ip_port)
{
    struct sockaddr_in server, cli_addr;
    int rc;
    
    printf("Creating %s %s\n", is_tcp ? "TCP" : "UDP", is_client?"CLIENT":"SERVER");
    
    sock_fd = socket(PF_INET, is_tcp ? SOCK_STREAM : SOCK_DGRAM, 0);
    
    if (sock_fd < 0)
    {
        fprintf(stderr, "create socket failed, %d\n", errno);
        return -1;
    }
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip_addr);
    server.sin_port = htons(ip_port);
    
    if(is_client)
    {
        printf("connecting to %s:%i\n", ip_addr, ip_port);
        
        rc = connect(sock_fd, (struct sockaddr *)&server, sizeof(server));
        if (rc < 0)
        {
            fprintf(stderr, "connect failed, %d\n", errno);
            connection_cleanup();
        }
    }
    else
    {
        socklen_t clilen;
        server.sin_addr.s_addr = INADDR_ANY;
        if (bind(sock_fd, (struct sockaddr *) &server, sizeof(server)) < 0)
        {
            fprintf(stderr, "bind failed, %d\n",errno);
            connection_cleanup();
            return -1;
        }
        if(listen(sock_fd,5) < 0)
        {
            fprintf(stderr, "listen failed, %d\n",errno);
            connection_cleanup();
            return -1;
        }
        
        clilen = sizeof(cli_addr);
        client_sock_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, &clilen);
        if(client_sock_fd < 0)
        {
            fprintf(stderr, "accept failed, %d\n",errno);
            connection_cleanup();
            return -1;
        }
    }
    
    if(is_client)
        return sock_fd;
    else
        return client_sock_fd;
}

static void connection_cleanup(void)
{
    close(sock_fd);
    close(client_sock_fd);
}

int main(int argc, char* argv[])
{
    int sock_fd;
    FILE* output_fp;
    int16_t data_point;
    
    printf("Connecting as a TCP client to 192.168.3.2:14000\n");
    sock_fd = connection_init(1,1,"192.168.3.2",14000);
    if(sock_fd < 0)
    {
        fprintf(stderr,"Connection initialization failed\n");
        return 1;
    }
    
    printf("Connected.\n");
    
    output_fp = fopen("output.csv", "wb");
    if(NULL == output_fp)
    {
        fprintf(stderr,"Problem opening output file (%s).\n",strerror(errno));
        return 1;
    }
    
    while(1)
    {
        int times = 0;
        
        read(sock_fd, &data_point, 2*sizeof(uint8_t));
        
        fprintf(output_fp, "%d,\n",data_point);
        
        times++;
        
        if(times == 10000)
        {
            times = 0;
            fflush(output_fp);
        }
    }
    
    connection_cleanup();
    
    return 0;
}