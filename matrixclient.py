import socket
import pickle

HOST = '127.0.0.1'
PORT = 65432
P = 97  

def get_matrix_input():
    rows = int(input("Enter number of rows for client matrix A: "))
    cols = int(input("Enter number of columns for client matrix A: "))
    print(f"Enter matrix A ({rows}x{cols}):")
    matrix = []
    for i in range(rows):
        while True:
            try:
                row = list(map(int, input(f"Row {i+1}: ").split()))
                if len(row) != cols:
                    print("Incorrect number of elements. Try again.")
                    continue
                matrix.append(row)
                break
            except ValueError:
                print("Invalid input. Enter integers only.")
    return matrix

def client_send_matrix():
    print("Type 'stop' to end the session.")
    while True:
        user_command = input("\nProceed with matrix input? (yes/stop): ").strip().lower()
        if user_command == "stop":
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                s.send(pickle.dumps("stop"))
            print("[CLIENT] Session terminated.")
            break

        matrix_a = get_matrix_input()
        if any(any(x >= P for x in row) for row in matrix_a):
            print(f"Warning: Elements ≥ {P} will be computed modulo {P}.")

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                ready = s.recv(4096)  
                if ready != b"READY":
                    print("[CLIENT] Unexpected server response.")
                    continue

                s.send(pickle.dumps(matrix_a))
                data = s.recv(4096)
                result = pickle.loads(data)

                print("\n[CLIENT] Received secure matrix product (mod P):")
                for row in result:
                    print("  ", row)
        except ConnectionRefusedError:
            print("[CLIENT ERROR] Cannot connect to server.")
            break

if __name__ == "__main__":
    client_send_matrix()
