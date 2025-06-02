#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9103  
#define P 97
#define MAX_DB_SIZE 10

struct DBUploadPacket {
    int db_share[MAX_DB_SIZE];
    int length;
};

struct QueryPacket {
    int db_share[MAX_DB_SIZE];
    int query_share[MAX_DB_SIZE];
    int length;
};

int modmul(int a, int b) {
    return (a * b) % P;
}

int compute_dot_product(const QueryPacket *packet) {
    int sum = 0;
    for (int i = 0; i < packet->length; i++) {
        sum = (sum + modmul(packet->db_share[i], packet->query_share[i])) % P;
    }
    return sum;
}

int main() {
    int db_share[MAX_DB_SIZE] = {0};
    int db_loaded = 0;

    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);
    printf("[SERVER @ %d] Ready for connections...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);

        if (!db_loaded) {
            DBUploadPacket upload;
            recv(client_fd, &upload, sizeof(upload), 0);
            memcpy(db_share, upload.db_share, sizeof(int) * upload.length);
            db_loaded = 1;
            printf("[SERVER @ %d] DB share received.\n", PORT);
            close(client_fd);
            continue;
        }

        QueryPacket query;
        recv(client_fd, &query, sizeof(query), 0);
        memcpy(query.db_share, db_share, sizeof(int) * query.length);
        int result = compute_dot_product(&query);
        send(client_fd, &result, sizeof(result), 0);
        printf("[SERVER @ %d] Dot product share sent: %d\n", PORT, result);
        close(client_fd);
    }
    return 0;
}
