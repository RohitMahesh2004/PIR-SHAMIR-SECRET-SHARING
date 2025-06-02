#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define P 97
#define MAX_DB_SIZE 10
#define THRESHOLD 1
#define N_SERVERS 3
int SERVER_PORTS[N_SERVERS] = {9101, 9102, 9103};

struct DBUploadPacket {
    int db_share[MAX_DB_SIZE];
    int length;
};

void generate_shares(int secret, int *shares, int *coeffs) {
    coeffs[0] = secret;
    for (int i = 1; i <= THRESHOLD; i++) coeffs[i] = rand() % P;

    for (int i = 0; i < N_SERVERS; i++) {
        int x = i + 1, val = 0;
        for (int j = 0; j <= THRESHOLD; j++) {
            int term = coeffs[j];
            for (int k = 0; k < j; k++) term = (term * x) % P;
            val = (val + term) % P;
        }
        shares[i] = val;
    }
}

void send_db_to_server(DBUploadPacket *packet, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    send(sock, packet, sizeof(*packet), 0);
    close(sock);
}

int main() {
    srand(time(NULL));
    int DB[MAX_DB_SIZE] = {42, 17, 23, 56, 89};
    int n = 5;

    DBUploadPacket packets[N_SERVERS] = {0};
    for (int i = 0; i < n; i++) {
        int shares[N_SERVERS], coeffs[THRESHOLD + 1];
        generate_shares(DB[i], shares, coeffs);
        for (int j = 0; j < N_SERVERS; j++) {
            packets[j].db_share[i] = shares[j];
            packets[j].length = n;
        }
    }

    for (int i = 0; i < N_SERVERS; i++) {
        send_db_to_server(&packets[i], SERVER_PORTS[i]);
        printf("Sent DB share to server %d\n", i + 1);
    }

    return 0;
}