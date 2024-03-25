# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
from commands_handler import *
from queue import Queue

conn_cnt = 0
conn_port = 8443
max_msg_size = 1000


# Variaveis

uids = {0: "MSG_SERVER"}
message_queue = {}

class ServerWorker(object):
    """ Classe que implementa a funcionalidade do SERVIDOR. """

    def __init__(self, cnt, addr=None):
        """ Construtor da classe. """
        self.id = cnt
        self.addr = addr
        self.msg_cnt = 0

    def valid_message(self, msg):
        key = msg.decode().split(" ")
        print(key, "valid message key")
        if key[0] in ["askqueue", "help"] and len(key) == 1:
            return 1
        elif key[0] == "-user" and len(key) == 2:
            return 1
        elif key[0] == "getmsg" and len(key) == 2:
            return 1
        elif key[0] == "send" and len(key) > 3:
            return 1
        return -1

    def process(self, msg):
        """ Processa uma mensagem (`bytestring`) enviada pelo CLIENTE.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1
        #
        # ALTERAR AQUI COMPORTAMENTO DO SERVIDOR
        #
        txt = msg.decode()
        print('%d : %r' % (self.id, txt))
        #new_msg = txt.upper().encode()

        #return new_msg if len(new_msg)>0 else None
        response = "Invalid command or missing arguments! Try help".encode()

        if self.valid_message(msg) == 1:

            # diferentes tipos de request do client

            if txt.startswith("-user"):
                response = handle_user_command(txt, message_queue)

            elif txt.startswith("help"):
                response = handle_help_command(txt, message_queue)

            elif txt.startswith("send"):
                print(txt)
                response = handle_send_command(txt, message_queue)

            elif txt.startswith("askqueue"):
                response = handle_askqueue_command(txt, message_queue)

            elif txt.startswith("getmsg"):
                response = handle_getmsg_command(txt, message_queue)

        return response


def uid_gen(client_address):
    last_uid = list(uids)[-1]
    next_uid = int(last_uid) + 1
    uids[next_uid] = client_address
    message_queue[next_uid] = []
    return next_uid


#
#
# Funcionalidade Cliente/Servidor
#
# obs: não deverá ser necessário alterar o que se segue
#


async def handle_echo(reader, writer):
    global conn_cnt
    conn_cnt += 1
    addr = writer.get_extra_info('peername')
    srvwrk = ServerWorker(conn_cnt, addr)

    # gerar uid do utilizador
    uid = uid_gen(addr)
    print(f"Unique UID given to {addr} : {uid}")

    data = await reader.read(max_msg_size)
    while True:
        if not data: continue
        if data[:1] == b'\n': break
        data = srvwrk.process(data)
        if not data: break
        # encriptar a data
        writer.write(data)
        await writer.drain()
        data = await reader.read(max_msg_size)
    print("[%d]" % srvwrk.id)
    writer.close()


def run_server():
    loop = asyncio.new_event_loop()
    coro = asyncio.start_server(handle_echo, '127.0.0.1', conn_port)
    server = loop.run_until_complete(coro)

    # Serve requests until Ctrl+C is pressed
    print('Serving on {}'.format(server.sockets[0].getsockname()))
    print('  (type ^C to finish)\n')
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass
    # Close the server
    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()
    print('\nFINISHED!')


run_server()