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
    print(f"[CLIENT CONNECTED] {addr}")
    while True:
        conn.send(b"READY")
        try:
            data = conn.recv(4096)
            if not data:
                break
            client_vector = pickle.loads(data)
            if client_vector == "stop":
                print("[SERVER] Client requested termination.")
                break

            server_vector = list(map(int, input("Enter server-side vector b (space-separated): ").split()))
            if len(client_vector) != len(server_vector):
                conn.send(pickle.dumps("ERROR: Vectors must be the same length."))
                continue

            print(f"\nSecure Dot Product via Shamir Secret Sharing")
            print(f"Field: F_{P}, Threshold: t={THRESHOLD}, Number of parties: n={N_SERVERS}")
            print(f"Vectors:\n  a = {client_vector}\n  b = {server_vector}\n")
            print("Sharing Phase:")

            a_shares, b_shares = [], []
            for i in range(len(client_vector)):
                a_i_shares, a_poly = generate_shares(client_vector[i], N_SERVERS, THRESHOLD, P)
                b_i_shares, b_poly = generate_shares(server_vector[i], N_SERVERS, THRESHOLD, P)
                a_shares.append(a_i_shares)
                b_shares.append(b_i_shares)
                print(f"  a[{i}] = {client_vector[i]} → poly = {a_poly}, shares = {a_i_shares}")
                print(f"  b[{i}] = {server_vector[i]} → poly = {b_poly}, shares = {b_i_shares}")

            print("\nComputation Phase:")
            dot_shares = []
            for j in range(N_SERVERS):
                print(f"  Server {j+1}:")
                total = 0
                for i in range(len(client_vector)):
                    prod = a_shares[i][j] * b_shares[i][j] % P
                    print(f"    a[{i}]_share = {a_shares[i][j]}, b[{i}]_share = {b_shares[i][j]}, product = {prod}")
                    total = (total + prod) % P
                dot_shares.append(total)
                print(f"    Dot product share: {total}")

            x_vals = list(range(1, N_SERVERS + 1))
            dot_product = lagrange_interpolate_zero(x_vals[:2 * THRESHOLD + 1], dot_shares[:2 * THRESHOLD + 1], P)
            print(f"\nReconstructed dot product = {dot_product} (mod {P})")

            print("\nResharing Final Result:")
            reshared, coeffs = generate_shares(dot_product, N_SERVERS, THRESHOLD, P)
            for i in range(N_SERVERS):
                print(f"  Server {i+1} receives reshared dot product: {reshared[i]}")
            print(f"\nPolynomial used for resharing: f(x) = {format_polynomial(coeffs, P)}")

            conn.send(pickle.dumps(dot_product))
        except Exception as e:
            print("[SERVER ERROR]", e)
            break
    conn.close()

def start_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(('127.0.0.1', 65432))
        s.listen()
        print(f"[SERVER] Listening on 127.0.0.1:65432")
        while True:
            conn, addr = s.accept()
            threading.Thread(target=handle_client, args=(conn, addr)).start()

if __name__ == "__main__":
    start_server()
