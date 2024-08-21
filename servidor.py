import socket

HOST = '0.0.0.0' # Endereço IP do servidor
PORT = 65432            # Porta que o servidor vai escutar

def handle_client(client_socket):
    while True:
        data = client_socket.recv(1024).decode('utf-8')
        
        if not data:
            break
        
        print(f"Recebido: {data}")
        
        if data == '1':
            response = "Pega"
        elif data == '0':
            response = "Credito"
        else:
            response = "Comando desconhecido"

        client_socket.send(response.encode('utf-8'))
        print(f"Enviado: {response}")

    client_socket.close()

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen(5)
    print(f"Servidor escutando em {HOST}:{PORT}")

    while True:
        client_socket, addr = server.accept()
        print(f"Conexão aceita de {addr}")
        handle_client(client_socket)

if __name__ == "__main__":
    main()
