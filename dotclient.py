import socket
import pickle

HOST = '127.0.0.1'
PORT = 65432
P = 97  

def client_send_vector():
    print("Type 'stop' to end the session.")
    while True:
        user_input = input("Enter your client-side vector a (space-separated): ")
        if user_input.lower() == "stop":
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                
                s.recv(4096)
                s.send(pickle.dumps("stop"))
            print("[CLIENT] Session ended.")
            break

        try:
            a_vec = list(map(int, user_input.split()))
        except ValueError:
            print("[ERROR] Invalid input. Please enter space-separated integers.")
            continue

        if any(x >= P for x in a_vec):
            print(f"Warning: Some elements in vector 'a' are ≥ {P}. They will be reduced modulo {P}.")

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                
                msg = s.recv(4096)
                if msg != b"READY":
                    print("[CLIENT ERROR] Unexpected server response.")
                    continue
                
                s.send(pickle.dumps(a_vec))

                data = s.recv(4096)
                try:
                    dot_product = pickle.loads(data)
                    print(f"[CLIENT] Received secure dot product: {dot_product} (mod {P})\n")
                except:
                    print(f"[CLIENT ERROR] Server response: {data.decode()}")
        except ConnectionRefusedError:
            print("[CLIENT ERROR] Could not connect to the server. Is it running?")
            break


if __name__ == "__main__":
    client_send_vector()
