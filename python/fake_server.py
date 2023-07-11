import socket
import threading

# 서버 주소와 포트 설정
SERVER_HOST = '127.0.0.1'
SERVER_PORT = 10000

# TCP 소켓 생성
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 서버 주소와 포트에 바인딩
server_socket.bind((SERVER_HOST, SERVER_PORT))

# 클라이언트 연결 대기
server_socket.listen(1)

print('서버가 시작되었습니다. 클라이언트 연결을 기다립니다...')

# 클라이언트 연결 수락
client_socket, client_address = server_socket.accept()
print(f'클라이언트가 연결되었습니다: {client_address[0]}:{client_address[1]}')

def receive_messages():
    # 클라이언트로부터 데이터 수신 및 출력
    while True:
        data = client_socket.recv(1024).decode()
        if not data:
            # 클라이언트 연결 종료
            print('클라이언트와의 연결이 종료되었습니다.')
            break
        print(f'수신한 메시지: {data}')

def send_messages():
    # 서버에서 클라이언트에게 메시지 전송
    while True:
        message = input('클라이언트에게 보낼 메시지를 입력하세요 ("quit"로 종료): ')
        client_socket.send(message.encode())
        if message.lower() == 'quit':
            # 서버 종료
            break

# 스레드를 사용하여 수신 및 전송 기능을 병렬로 처리
receive_thread = threading.Thread(target=receive_messages)
send_thread = threading.Thread(target=send_messages)

receive_thread.start()
send_thread.start()

receive_thread.join()
send_thread.join()

# 소켓 닫기
client_socket.close()
server_socket.close()
