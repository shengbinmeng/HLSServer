
//
//  main.m
//  HLSServer
//


#import <stdio.h>
#include <unistd.h>
#import <stdlib.h>
#import <string.h>
#import <sys/types.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>

#define PREFIX ""
#define MAXLINE 1024

int create_server_socket(short port)
{
    int server_socket = 0;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        return 0;
    }
    if (listen(server_socket, 100) != 0){
        return 0;
    }
    return server_socket;
}

int accept_client_socket(int server_socket)
{
    int client_socket = 0;
    struct sockaddr_in addr = {0};
    socklen_t len = sizeof(addr);
    client_socket = accept(server_socket, (struct sockaddr *)&addr, &len);
    char *ip_address = inet_ntoa(addr.sin_addr);
    printf("accept--->%s, %d", ip_address, addr.sin_port);
    return client_socket;
}

void create_m3u8(const char* m3u8_filename) {
    FILE *m3u8_file = fopen(m3u8_filename, "w");
    fwrite("#EXTM3U\n#EXT-X-TARGETDURATION:15\n#EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-PLAYLIST-TYPE:EVENT\n", sizeof(char), strlen("#EXTM3U\n#EXT-X-TARGETDURATION:15\n#EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-PLAYLIST-TYPE:EVENT\n"), m3u8_file);
    fclose(m3u8_file);
}

void append_m3u8(const char *m3u8_filename, char *ts_filename) {
    FILE *m3u8_file = fopen(m3u8_filename, "a+");
    char writebuf[1024];
    bzero(writebuf, 0);
    sprintf(writebuf, "#EXTINF:10\n%s\n", ts_filename);
    fwrite(writebuf,sizeof(char), strlen(writebuf),m3u8_file);
    fclose(m3u8_file);
}

void receive_data(int socket)
{
    const char *m3u8_filename = "test.m3u8";
    create_m3u8(m3u8_filename);
    char buf[1024];
    int file_len;
    long recv_len, write_len;
    FILE *ts_file;
    char ts_filename[200];
    int file_count = 0;
    bzero(buf, 0);

    while (1) {
        recv_len = recv(socket, &file_len, MAXLINE, 0);
        if (file_len == 0) {
            break;
        }
        printf("recv:%ld\n", recv_len);
        printf("<-----%d------>",file_len);
        
        sprintf(ts_filename, "%srecord_%d.ts", PREFIX, file_count);
        ts_file = fopen(ts_filename, "w");
        bzero(buf, MAXLINE);
        while ((recv_len = recv(socket, buf, MAXLINE, 0))) {
            if(recv_len < 0) {
                printf("Recieve Data From Server Failed!\n");
                break;
            }
            write_len = fwrite(buf, sizeof(char), recv_len, ts_file);
            if (write_len < recv_len) {
                printf("Write file failed\n");
                break;
            }
            bzero(buf, MAXLINE);
            file_len -= recv_len;
            if (file_len == 0) {
                fclose(ts_file);
                break;
            }
        }
        printf("\nFinish Recieve\n");
        append_m3u8(m3u8_filename, ts_filename);
        file_count++;
    }
    close(socket);
}

int main(int argc, const char * argv[])
{
    short port = 9898;
    if (argc > 1) {
        port = (short)atoi(argv[1]); //就一个port的参数， ip 地址你自己查一下
    }
    
    int server_socket = create_server_socket(port);
    if (server_socket == 0) {
        printf("Cerate scoket error\n");
        return 0;
    }
    printf("Cerate scoket ok! listen at port:%d\n", port);
    
    int client_scoket = accept_client_socket(server_socket);
    if (client_scoket == 0) {
        printf("Client connect error\n");
        return 0;
    }
    printf("Client connect ok.....\n");
    
    receive_data(client_scoket);
    
    return 0;
}

