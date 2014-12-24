//
//  main.c
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
#define BUFFER_SIZE 1024

int create_server_socket(short port)
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        return -1;
    }
    if (listen(server_socket, 100) != 0) {
        return -1;
    }
    return server_socket;
}

int accept_client_socket(int server_socket)
{
    struct sockaddr_in addr = {0};
    socklen_t len = sizeof(addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&addr, &len);
    char *ip_address = inet_ntoa(addr.sin_addr);
    printf("accept--->%s, %d\n", ip_address, addr.sin_port);
    return client_socket;
}

int create_m3u8(const char* m3u8_filename)
{
    FILE *m3u8_file = fopen(m3u8_filename, "w");
    if (m3u8_file == NULL) {
        return -1;
    }
    fwrite("#EXTM3U\n#EXT-X-TARGETDURATION:15\n#EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-PLAYLIST-TYPE:EVENT\n", sizeof(char), strlen("#EXTM3U\n#EXT-X-TARGETDURATION:15\n#EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-PLAYLIST-TYPE:EVENT\n"), m3u8_file);
    fclose(m3u8_file);
    return 0;
}

int append_m3u8(const char *m3u8_filename, char *ts_filename)
{
    FILE *m3u8_file = fopen(m3u8_filename, "a+");
    if (m3u8_file == NULL) {
        return -1;
    }
    char buf[BUFFER_SIZE];
    bzero(buf, 0);
    sprintf(buf, "#EXTINF:10\n%s\n", ts_filename);
    fwrite(buf, sizeof(char), strlen(buf), m3u8_file);
    fclose(m3u8_file);
    return 0;
}

void communicate(int socket)
{
    const char *m3u8_filename = "test.m3u8";
    int ret = create_m3u8(m3u8_filename);
    if (ret < 0) {
        fprintf(stderr, "Create m3u8 file failed\n");
        return;
    }
    char buf[BUFFER_SIZE];
    long recv_len, write_len;
    FILE *ts_file;
    char ts_filename[2014];
    int file_len = 0, file_count = 0;

    while (1) {
        recv_len = recv(socket, &file_len, BUFFER_SIZE, 0);
        if (recv_len < 0) {
            printf("Receive data from server failed\n");
            break;
        }
        if (file_len == 0) {
            // This file is zero length, so we continue to next
            continue;
        }
        sprintf(ts_filename, "%srecord_%d.ts", PREFIX, file_count);
        ts_file = fopen(ts_filename, "w");
        if (ts_file == NULL) {
            fprintf(stderr, "Open TS file failed\n");
            break;
        }
        bzero(buf, BUFFER_SIZE);
        while ((recv_len = recv(socket, buf, BUFFER_SIZE, 0))) {
            if (recv_len < 0) {
                printf("Receive data from server failed\n");
                break;
            }
            write_len = fwrite(buf, sizeof(char), recv_len, ts_file);
            if (write_len < recv_len) {
                printf("Write to TS file failed\n");
                break;
            }
            bzero(buf, BUFFER_SIZE);
            file_len -= recv_len;
            if (file_len == 0) {
                break;
            }
        }
        fclose(ts_file);
        printf("TS file %s is received\n", ts_filename);
        ret = append_m3u8(m3u8_filename, ts_filename);
        if (ret < 0) {
            fprintf(stderr, "Append m3u8 file failed\n");
            break;
        }

        file_count++;
    }
}

int main(int argc, const char * argv[])
{
    // Use port 9898 if not specified
    short port = 9898;
    if (argc > 1) {
        port = (short)atoi(argv[1]);
    }
    
    int server_socket = create_server_socket(port);
    if (server_socket < 0) {
        printf("Cerate socket failed\n");
        return -1;
    }
    printf("Cerate server socket success. Listen at port:%d\n", port);
    
    int client_socket = accept_client_socket(server_socket);
    if (client_socket < 0) {
        printf("Client connect failed\n");
        return -1;
    }
    printf("Client connect success. Begin communicating.....\n");
    
    communicate(client_socket);
    
    close(client_socket);

    return 0;
}

