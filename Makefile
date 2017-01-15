hlssvr: HLSServer/main.c
	$(CC) -o hlssvr HLSServer/main.c

clean:
	rm -f hlssvr
