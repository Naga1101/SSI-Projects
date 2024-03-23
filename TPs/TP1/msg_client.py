# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
import socket

conn_port = 8443
max_msg_size = 999


class Client:
    """ Classe que implementa a funcionalidade de um CLIENTE. """

    def __init__(self, sckt=None):
        """ Construtor da classe. """
        self.sckt = sckt
        self.msg_cnt = 0

    def process(self, msg=b""):
        """ Processa uma mensagem (`bytestring`) enviada pelo SERVIDOR.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1
        #
        # ALTERAR AQUI COMPORTAMENTO DO CLIENTE
        #
        #print('Received (%d): %r' % (self.msg_cnt , msg.decode()))
        print("\n" + msg.decode())
        print('Input message to send (empty to finish)')

        command = input().strip()

        if command.startswith('send'):
            print("Enter message body: ")
            message_body = input()

            message_size = len(message_body) + len(command)

            if message_size > max_msg_size:
                print("Message reached 1000 bytes limit, unable to send")
            else:
                # falta caso subject ter mais de 1 palavra
                # uid vai com o certificado?
                message = f"{command} {message_body}"

                return message.encode()

        if command.startswith('askqueue'):
            message = 'askqueue'
            return message.encode()

        if command.startswith('help'):
            message = 'help'
            return message.encode()

        if command.startswith('getmsg'):
            msg_number = command.split()[1]
            message = f"getmsg {msg_number}"

            return message.encode()



#
#
# Funcionalidade Cliente/Servidor
#
# obs: não deverá ser necessário alterar o que se segue
#


async def tcp_echo_client():
    reader, writer = await asyncio.open_connection('127.0.0.1', conn_port)
    addr = writer.get_extra_info('peername')
    client = Client(addr)
    msg = client.process()

    while msg:
        writer.write(msg)
        msg = await reader.read(max_msg_size)
        if msg:
            msg = client.process(msg)
        else:
            break
    writer.write(b'\n')
    print('Socket closed!')
    writer.close()


def run_client():
    loop = asyncio.get_event_loop()
    loop.run_until_complete(tcp_echo_client())


run_client()
