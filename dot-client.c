#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define P 97
#define THRESHOLD 1
#define N_SERVERS 3
#define MAX_VECTOR 10
#define PORT_BASE 8080
#define LOCALHOST "127.0.0.1"

typedef struct SharePacket {
    int shares[MAX_VECTOR * 2];
    int length;

    
    void (*generate_vector_shares)(int *vec, int length, struct SharePacket *packets, int is_b);
    int  (*reconstruct_dot_product)(int *shares, int threshold, int prime);
} SharePacket;

int modinv(int a, int p) {
    int res = 1;
    int power = p - 2;
    while (power) {
        if (power & 1) res = (res * a) % p;
        a = (a * a) % p;
        power >>= 1;
    }
    return res;
}

void generate_shares(int secret, int *shares, int t, int n, int p, int *coeffs) {
    coeffs[0] = secret;
    for (int i = 1; i <= t; i++) {
        coeffs[i] = rand() % p;
    }
    for (int i = 0; i < n; i++) {
        int x = i + 1;
        int val = 0;
        for (int j = 0; j <= t; j++) {
            int term = coeffs[j];
            for (int k = 0; k < j; k++) term = (term * x) % p;
            val = (val + term) % p;
        }
        shares[i] = val;
    }
}

void generate_vector_shares_impl(int *vec, int length, SharePacket *packets, int is_b) {
    for (int i = 0; i < length; i++) {
        int shares[N_SERVERS];
        int coeffs[THRESHOLD + 1];
        generate_shares(vec[i], shares, THRESHOLD, N_SERVERS, P, coeffs);
        for (int j = 0; j < N_SERVERS; j++) {
            int index = is_b ? i + length : i;
            packets[j].shares[index] = shares[j];
        }
    }
}

int reconstruct_dot_product_impl(int *shares, int threshold, int prime) {
    int result = 0;
    int x[] = {1, 2, 3};

    for (int i = 0; i <= threshold + 1; i++) {
        int num = 1, den = 1;
        for (int j = 0; j <= threshold + 1; j++) {
            if (i != j) {
                int neg_xj = (-x[j]) % prime;
                if (neg_xj < 0) neg_xj += prime;
                num = (num * neg_xj) % prime;

                int diff = (x[i] - x[j]) % prime;
                if (diff < 0) diff += prime;
                den = (den * diff) % prime;
            }
        }
        int li = (num * modinv(den, prime)) % prime;
        result = (result + ((shares[i] * li) % prime)) % prime;
    }

    if (result < 0) result += prime;
    return result;
}

int connect_and_send(int port, SharePacket *packet) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, LOCALHOST, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        exit(1);
    }

    send(sock, packet, sizeof(SharePacket), 0);
    int response;
    recv(sock, &response, sizeof(int), 0);
    close(sock);
    return response;
}

int main() {
    srand(time(NULL));
    int a[MAX_VECTOR], b[MAX_VECTOR];
    int n;

    printf("Enter vector length (<= %d): ", MAX_VECTOR);
    scanf("%d", &n);

    printf("Enter vector a: ");
    for (int i = 0; i < n; i++) scanf("%d", &a[i]);

    printf("Enter vector b: ");
    for (int i = 0; i < n; i++) scanf("%d", &b[i]);

    SharePacket packets[N_SERVERS];

    for (int i = 0; i < N_SERVERS; i++) {
        packets[i].length = n * 2;
        packets[i].generate_vector_shares = generate_vector_shares_impl;
        packets[i].reconstruct_dot_product = reconstruct_dot_product_impl;
    }

    packets[0].generate_vector_shares(a, n, packets, 0);  
    packets[0].generate_vector_shares(b, n, packets, 1);  

    printf("\n--- Shares Sent to Each Server ---\n");
    for (int i = 0; i < N_SERVERS; i++) {
        printf("Server %d: ", i + 1);
        for (int j = 0; j < packets[i].length; j++) {
            printf("%d%s", packets[i].shares[j], j < packets[i].length - 1 ? ", " : "");
        }
        printf("\n");
    }
    
    int dot_shares[N_SERVERS];
    printf("\n--- Shares returned by Each Server ---\n");
    for (int i = 0; i < N_SERVERS; i++) {
        dot_shares[i] = connect_and_send(PORT_BASE + i + 1, &packets[i]);
        printf("Server %d returned share: %d\n", i + 1, dot_shares[i]);
    }
    int result = packets[0].reconstruct_dot_product(dot_shares, THRESHOLD, P);
    printf("\nReconstructed Dot Product = %d (mod %d)\n", result, P);
    return 0;
}
