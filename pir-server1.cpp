#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 9101  // Change to 9102 and 9103 for other servers
#define P 97
#define MAX_DB_SIZE 10

struct QueryPacket {
    int db_share[MAX_DB_SIZE];
    int query_share[MAX_DB_SIZE];
    int length;
};

int modmul(int a, int b) {
    return (a * b) % P;
}

int compute_dot_product(QueryPacket *packet) {
    int sum = 0;
    for (int i = 0; i < packet->length; i++) {
        sum = (sum + modmul(packet->db_share[i], packet->query_share[i])) % P;
    }
    return sum;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("[SERVER @ %d] Waiting for PIR queries...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);

        QueryPacket packet;
        recv(client_fd, &packet, sizeof(packet), 0);

        int result = compute_dot_product(&packet);
        send(client_fd, &result, sizeof(result), 0);
        printf("[SERVER @ %d] Sent dot product share: %d\n", PORT, result);
        close(client_fd);
    }

    return 0;
}
