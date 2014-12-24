
//
//  main.m
//  server
//
//  Created by mac on 13-4-15.
//  Copyright (c) 2013年 mac. All rights reserved.
//

#import <stdio.h>
#include <unistd.h>
#import <stdlib.h>
#import <string.h>
#import <sys/types.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>
int fileCnt=0;
#define PREFIX ""
#define MAXLINE 1024
 FILE *m3u8fp;
struct options_t {
    const char *input_file;
    long segment_duration;
    const char *output_prefix;
    const char *m3u8_file;
    char *tmp_m3u8_file;
    const char *url_prefix;
    long num_segments;
};

int write_index_file(const struct options_t options, const unsigned int first_segment, const unsigned int last_segment, const int end) {
    FILE *index_fp;
    char *write_buf;
    unsigned int i;
    
    index_fp = fopen(options.tmp_m3u8_file, "w");
    if (!index_fp) {
        fprintf(stderr, "Could not open temporary m3u8 index file (%s), no index file will be created\n", options.tmp_m3u8_file);
        return -1;
    }
    
    write_buf = (char*)malloc(sizeof(char) * 1024);
    if (!write_buf) {
        fprintf(stderr, "Could not allocate write buffer for index file, index file will be invalid\n");
        fclose(index_fp);
        return -1;
    }
    
    if (options.num_segments) {
        snprintf(write_buf, 1024, "#EXTM3U\n#EXT-X-TARGETDURATION:%lu\n#EXT-X-MEDIA-SEQUENCE:%u\n", options.segment_duration, first_segment);
    }
    else {
        snprintf(write_buf, 1024, "#EXTM3U\n#EXT-X-TARGETDURATION:%lu\n", options.segment_duration);
    }
    if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
        fprintf(stderr, "Could not write to m3u8 index file, will not continue writing to index file\n");
        free(write_buf);
        fclose(index_fp);
        return -1;
    }
    
    for (i = first_segment; i <= last_segment; i++) {
        snprintf(write_buf, 1024, "#EXTINF:%lu,\n%s%s-%u.ts\n", options.segment_duration, options.url_prefix, options.output_prefix, i);
        if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
            fprintf(stderr, "Could not write to m3u8 index file, will not continue writing to index file\n");
            free(write_buf);
            fclose(index_fp);
            return -1;
        }
    }
    
    if (end) {
        snprintf(write_buf, 1024, "#EXT-X-ENDLIST\n");
        if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
            fprintf(stderr, "Could not write last file and endlist tag to m3u8 index file\n");
            free(write_buf);
            fclose(index_fp);
            return -1;
        }
    }
    
    free(write_buf);
    fclose(index_fp);
    
    return rename(options.tmp_m3u8_file, options.m3u8_file);
}
int CreateServerSocket(short port)
{
    int socket_server=0;
    socket_server=socket(AF_INET,SOCK_STREAM,0);
    //填充IP与端口
    /*
     struct sockaddr_in {
     __uint8_t	sin_len;
     sa_family_t	sin_family;
     in_port_t	sin_port;
     struct	in_addr sin_addr;
     char		sin_zero[8];
     };
     */
    struct sockaddr_in sAddr = {0};
    sAddr.sin_len=sizeof(sAddr);
    sAddr.sin_family=AF_INET;
    sAddr.sin_port= htons(port); //本机字节序转网络字节序
    sAddr.sin_addr.s_addr=INADDR_ANY;
    //绑定  //(struct sockaddr *)&sAddr强转
    if (bind(socket_server, (struct sockaddr *)&sAddr, sizeof(sAddr)) != 0) {
        return 0;
    }
    //监听
    if(listen(socket_server, 100) != 0){
        return 0;
    }
    return socket_server;
}
//接收连接并返回客户端的scoket
int AcceptClientSocket(int ServerSocket)
{
    int ClinetSocket=0;
    struct sockaddr_in cAddr = {0};
    socklen_t len=sizeof(cAddr);
    ClinetSocket = accept(ServerSocket, (struct sockaddr *)&cAddr, &len);
    char * ipaddress=inet_ntoa(cAddr.sin_addr);
    printf("accept--->%s,%d",ipaddress,cAddr.sin_port);
    return ClinetSocket;
}
void CreateM3u8(char* filename){
   
    m3u8fp=fopen(filename,"w");
    fwrite("#EXTM3U\n#EXT-X-TARGETDURATION:15\n#EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-PLAYLIST-TYPE:EVENT\n",sizeof(char),strlen("#EXTM3U\n#EXT-X-TARGETDURATION:15\n#EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-PLAYLIST-TYPE:EVENT\n"),m3u8fp);
    fclose(m3u8fp);
}
void AppendM3u8(char*filename){
    m3u8fp=fopen("test.m3u8","a+");
    char writebuf[1024];
    bzero(writebuf,0);
    
    sprintf(writebuf,"#EXTINF:10\n%s\n",filename);
    fwrite(writebuf,sizeof(char),strlen(writebuf),m3u8fp);
    fclose(m3u8fp);
}
void Message(int socket)
{
    CreateM3u8("test.m3u8");
    char send_Message[1024]="hello\n";
    char buf[1024];
    int file_len;
    int recv_len,write_len;
    FILE * fd;
    char filename[200];
    //send(socket, send_Message, strlen(send_Message)+1, 0);
    bzero(buf,0);
    while (1) {
        recv_len=recv(socket, &file_len, MAXLINE, 0);
        if(file_len==0)break;
        printf("recv:%d\n",recv_len);
        printf("<-----%d------>",file_len);
        sprintf(filename,"%srecord_%d.ts",PREFIX,fileCnt);
        fd=fopen(filename,"w");
        bzero(buf, MAXLINE);
        while ((recv_len = recv(socket, buf, MAXLINE,0))) {
            /* receiver data part commented by guoqingbo*/
            if(recv_len < 0) {
                printf("Recieve Data From Server Failed!\n");
                break;
            }
            write_len = fwrite(buf, sizeof(char), recv_len, fd);
            if (write_len < recv_len) {
                printf("Write file failed\n");
                break;
            }
            bzero(buf,MAXLINE);
            file_len-=recv_len;
            if(file_len==0){
                fclose(fd);
                break;
            }
        }
        printf("\nFinish Recieve\n");
        AppendM3u8(filename);
        fileCnt++;
    }
    fclose(m3u8fp);
    close(socket);
}

int main(int argc, const char * argv[])
{
    short port=9898;
    if (argc>1) {
        port=(short)atoi(argv[1]); //就一个port的参数， ip 地址你自己查一下
    }
    int ServerSocket = CreateServerSocket(port);
    if (ServerSocket == 0) {
        printf("Cerate scoket error\n");
        return 0;
    }
    printf("Cerate scoket ok! listen at port:%d\n",port);
    
    while (true)
    {
        int ClinetScoket=AcceptClientSocket(ServerSocket);
        if (ClinetScoket == 0)
        {
            printf("Client connect error\n");
            return 0;
        }
        printf("Client connect ok.....\n");
        Message(ClinetScoket);
        return 0;
    }
    
    return 0;
}

