import socket

# Configurações do cliente
HOST = '192.168.100.10'  # IP do servidor
PORT = 65432            # A mesma porta usada pelo servidor

# Criando um socket TCP/IP
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
    client_socket.connect((HOST, PORT))
    print("Conectado ao servidor.")

    # Escolha a mensagem a ser enviada ('1' ou '0')
    mensagem = '0'  # Você pode alterar para '0' conforme necessário

    # Enviando a mensagem ao servidor
    client_socket.sendall(mensagem.encode())
    print(f"Mensagem '{mensagem}' enviada com sucesso.")

    # Recebendo uma resposta do servidor
    data = client_socket.recv(1024)
    print('Resposta do servidor:', data.decode())
