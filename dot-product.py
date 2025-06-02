import random

def modinv(a, p):
    return pow(a, p - 2, p)


def is_prime(p):
    if p < 2: return False
    for i in range(2, int(p**0.5) + 1):
        if p % i == 0:
            return False
    return True


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


def reshare_secret_verbose(secret, n, t, P):
    shares, coeffs = generate_shares(secret, n, t, P)
    poly_equation = format_polynomial(coeffs, P)
    return shares, coeffs, poly_equation


def secure_dot_product_verbose(a, b, n, t, P):
    if len(a) != len(b):
        raise ValueError("Vectors a and b must be the same length.")
    if n < 2 * t + 1:
        raise ValueError(f"Number of servers n={n} must be at least 2t + 1 = {2 * t + 1} to reconstruct degree-2t polynomial.")
    if any(x >= P for x in a + b):
        print("Warning: One or more elements in vectors 'a' or 'b' are ≥ P. They will be reduced modulo P automatically.")

    k = len(a)

    print(f"\nSecure Dot Product via Shamir Secret Sharing")
    print(f"Field: F_{P}, Threshold: t={t}, Number of parties: n={n}")
    print(f"Vectors:\n  a = {a}\n  b = {b}\n")

    
    print("Sharing Phase:")
    a_shares = []
    b_shares = []

    for i in range(k):
        a_i_shares, a_poly = generate_shares(a[i], n, t, P)
        b_i_shares, b_poly = generate_shares(b[i], n, t, P)
        a_shares.append(a_i_shares)
        b_shares.append(b_i_shares)
        print(f"  a[{i}] = {a[i]} → poly = {a_poly}, shares = {a_i_shares}")
        print(f"  b[{i}] = {b[i]} → poly = {b_poly}, shares = {b_i_shares}")

    print("\nComputation Phase:")
    dot_shares = []
    for j in range(n):
        print(f"  Server {j+1}:")
        dot_j = 0
        for i in range(k):
            prod = a_shares[i][j] * b_shares[i][j] % P
            print(f"    a[{i}]_share = {a_shares[i][j]}, b[{i}]_share = {b_shares[i][j]}, product = {prod}")
            dot_j = (dot_j + prod) % P
        dot_shares.append(dot_j)
        print(f"    Dot product share: {dot_j}")

    x_vals = list(range(1, n + 1))
    result = lagrange_interpolate_zero(x_vals[:2*t + 1], dot_shares[:2*t + 1], P)
    print(f"\nReconstructed dot product = {result} (mod {P})")

    print("\nResharing Final Result:")
    reshared, coeffs, poly_str = reshare_secret_verbose(result, n, t, P)
    for i in range(n):
        print(f"  Server {i+1} receives reshared dot product: {reshared[i]}")
    print(f"\nPolynomial used for resharing: f(x) = {poly_str}")
    return result


if __name__ == "__main__":
    P = int(input("Enter a prime number for finite field F_P (e.g., 101): "))
    
    if not is_prime(P):
        raise ValueError("Error: P must be a prime number.")

    a_vec = list(map(int, input("Enter vector a (space-separated): ").split()))
    b_vec = list(map(int, input("Enter vector b (space-separated): ").split()))
    n_servers = int(input("Enter number of servers (n): "))
    threshold = int(input("Enter threshold (t): "))

    secure_dot_product_verbose(a_vec, b_vec, n_servers, threshold, P)
