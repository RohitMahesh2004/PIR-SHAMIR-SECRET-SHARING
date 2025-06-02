import socket
import threading
import random
import pickle

P = 97
THRESHOLD = 2
N_SERVERS = 5

def modinv(a, p):
    return pow(a, p - 2, p)

def lagrange_interpolate_zero(xs, ys, P):
    total = 0
    for i in range(len(xs)):
        xi, yi = xs[i], ys[i]
        li = 1
        for j in range(len(xs)):
            if i != j:
                xj = xs[j]
                li *= (-xj * modinv(xi - xj, P)) % P
                li %= P
        total += yi * li
        total %= P
    return total

def generate_shares(secret, n, t, P):
    coeffs = [secret] + [random.randint(1, P - 1) for _ in range(t)]
    shares = []
    for i in range(1, n + 1):
        val = sum(coeffs[j] * pow(i, j, P) for j in range(t + 1)) % P
        shares.append(val)
    return shares, coeffs

def format_polynomial(coeffs, P):
    terms = []
    for i, coef in enumerate(coeffs):
        if coef == 0:
            continue
        if i == 0:
            terms.append(f"{coef}")
        elif i == 1:
            terms.append(f"{coef}x")
        else:
            terms.append(f"{coef}x^{i}")
    return " + ".join(terms) + f" (mod {P})"

def handle_client(conn, addr):
    while True:
        conn.send(b"READY")
        try:
            data = conn.recv(4096)
            if not data:
                break
            client_matrix = pickle.loads(data)
            if client_matrix == "stop":
                break

            server_matrix = []
            rows_b = len(client_matrix[0])
            cols_b = int(input("Enter number of columns for server matrix B: "))
            print(f"Enter matrix B ({rows_b}x{cols_b}):")
            for _ in range(rows_b):
                row = list(map(int, input().split()))
                while len(row) != cols_b:
                    print("Incorrect number of elements. Try again.")
                    row = list(map(int, input().split()))
                server_matrix.append(row)

            result_matrix = []
            for row_idx, row_a in enumerate(client_matrix):
                result_row = []
                for j in range(cols_b):
                    print(f"\nComputing dot product of A row {row_idx + 1} and B column {j + 1}:")
                    b_col = [server_matrix[k][j] for k in range(rows_b)]
                    a_shares, b_shares = [], []
                    for i in range(len(row_a)):
                        a_i_shares, a_poly = generate_shares(row_a[i], N_SERVERS, THRESHOLD, P)
                        b_i_shares, b_poly = generate_shares(b_col[i], N_SERVERS, THRESHOLD, P)
                        a_shares.append(a_i_shares)
                        b_shares.append(b_i_shares)
                        print(f"  a[{i}] = {row_a[i]} → poly = {a_poly}, shares = {a_i_shares}")
                        print(f"  b[{i}] = {b_col[i]} → poly = {b_poly}, shares = {b_i_shares}")

                    dot_shares = []
                    for s in range(N_SERVERS):
                        total = sum((a_shares[i][s] * b_shares[i][s]) % P for i in range(len(row_a))) % P
                        dot_shares.append(total)
                        print(f"  Server {s+1} share: {total}")

                    x_vals = list(range(1, N_SERVERS + 1))
                    dot_product = lagrange_interpolate_zero(x_vals[:2 * THRESHOLD + 1], dot_shares[:2 * THRESHOLD + 1], P)
                    print(f"  Reconstructed dot product = {dot_product} (mod {P})")

                    reshared, coeffs = generate_shares(dot_product, N_SERVERS, THRESHOLD, P)
                    print("  Reshared dot product shares:")
                    for i in range(N_SERVERS):
                        print(f"    Server {i+1} receives: {reshared[i]}")
                    print(f"  Polynomial used: f(x) = {format_polynomial(coeffs, P)}")

                    result_row.append(dot_product)
                result_matrix.append(result_row)

            conn.send(pickle.dumps(result_matrix))
        except Exception:
            break
    conn.close()

def start_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(('127.0.0.1', 65432))
        s.listen()
        while True:
            conn, addr = s.accept()
            threading.Thread(target=handle_client, args=(conn, addr)).start()

if __name__ == "__main__":
    start_server()
