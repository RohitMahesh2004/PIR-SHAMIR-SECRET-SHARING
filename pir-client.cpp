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

int modinv(int a, int p) {
    int res = 1, power = p - 2;
    while (power) {
        if (power & 1) res = (res * a) % p;
        a = (a * a) % p;
        power >>= 1;
    }
    return res;
}

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

int lagrange_interpolate(int *shares) {
    int x[] = {1, 2, 3};  // x values of shares
    int result = 0;

    for (int i = 0; i < N_SERVERS; i++) {
        int num = 1, den = 1;
        for (int j = 0; j < N_SERVERS; j++) {
            if (i != j) {
                num = (num * (0 - x[j] + P)) % P;
                den = (den * (x[i] - x[j] + P)) % P;
            }
        }
        int li = (num * modinv(den, P)) % P;
        result = (result + shares[i] * li) % P;
    }

    return (result + P) % P;
}

typedef struct {
    int db_share[MAX_DB_SIZE];
    int query_share[MAX_DB_SIZE];
    int length;
} QueryPacket;

int send_query(QueryPacket *packet, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    send(sock, packet, sizeof(*packet), 0);

    int response;
    recv(sock, &response, sizeof(response), 0);
    close(sock);
    return response;
}

int main() {
    srand(time(NULL));

    int DB[MAX_DB_SIZE] = {42, 17, 23, 56, 89};  // Example DB
    int n = 5;
    int index;

    printf("Enter index to retrieve (0 to %d): ", n - 1);
    scanf("%d", &index);

    // Prepare DB shares for each server
    QueryPacket packets[N_SERVERS] = {0};
    for (int i = 0; i < n; i++) {
        int shares[N_SERVERS], coeffs[THRESHOLD + 1];
        generate_shares(DB[i], shares, coeffs);
        for (int j = 0; j < N_SERVERS; j++) {
            packets[j].db_share[i] = shares[j];
            packets[j].length = n;
        }
    }

    // Query vector (1 at index, 0 elsewhere)
    int query[MAX_DB_SIZE] = {0};
    query[index] = 1;

    for (int i = 0; i < n; i++) {
        int shares[N_SERVERS], coeffs[THRESHOLD + 1];
        generate_shares(query[i], shares, coeffs);
        for (int j = 0; j < N_SERVERS; j++) {
            packets[j].query_share[i] = shares[j];
        }
    }

    printf("\nSending queries to servers...\n");
    int responses[N_SERVERS];
    for (int i = 0; i < N_SERVERS; i++) {
        responses[i] = send_query(&packets[i], SERVER_PORTS[i]);
        printf("Server %d returned: %d\n", i + 1, responses[i]);
    }

    int result = lagrange_interpolate(responses);
    printf("\n🎯 Retrieved DB[%d] = %d\n", index, result);
    return 0;
}
