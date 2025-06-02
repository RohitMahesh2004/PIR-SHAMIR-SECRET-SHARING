#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define P 97
#define MAX_VECTOR 10
#define PORT 8081

struct SharePacket;

typedef int (*MultiplyFunc)(int, int);
typedef int (*DotProductFunc)(struct SharePacket*);

typedef struct SharePacket {
    int shares[MAX_VECTOR * 2];
    int length;
    MultiplyFunc multiply;
    DotProductFunc compute_dot_product_share;
} SharePacket;

int multiply_impl(int a, int b) {
    return (a * b) % P;
}

int compute_dot_product_impl(SharePacket *packet) {
    int n = packet->length / 2;
    int result = 0;
    for (int i = 0; i < n; i++) {
        int a_i = packet->shares[i];
        int b_i = packet->shares[i + n];
        result = (result + packet->multiply(a_i, b_i)) % P;
    }
    return result;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);
    printf("[SERVER 1] Listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        SharePacket packet;
        recv(client_sock, &packet, sizeof(SharePacket), 0);
        packet.multiply = multiply_impl;
        packet.compute_dot_product_share = compute_dot_product_impl;

        int n = packet.length / 2;
        printf("[SERVER 1] Received shares:\n");
        printf("  Vector a shares: ");
        for (int i = 0; i < n; i++) printf("%d%s", packet.shares[i], i < n - 1 ? ", " : "");
        printf("\n  Vector b shares: ");
        for (int i = 0; i < n; i++) printf("%d%s", packet.shares[i + n], i < n - 1 ? ", " : "");
        printf("\n");

        int dot_product_share = packet.compute_dot_product_share(&packet);
        printf("[SERVER 1] Computed dot product share: %d\n", dot_product_share);
        send(client_sock, &dot_product_share, sizeof(int), 0);
        close(client_sock);
    }

    close(server_sock);
    return 0;
}
