# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
import os
from commands_handler import *
from encrypt_decypt_handler import *
from queue import Queue
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

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
        self.shared_key = None
        self.algorythm_AES = None

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

        msg = decode_message(msg, self.shared_key, self.algorythm_AES)

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

        # print(response)
        return encode_message(response, self.shared_key, self.algorythm_AES)

    async def handshake(self, writer, reader):

        # p = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF
        # g = 2
        #parameters = dh.DHParameterNumbers(p,g).parameters()

        # espera receber os parametros do cliente
        param_bytes = await reader.read(max_msg_size)

        # desserializar os parametros
        parameters = serialization.load_pem_parameters(param_bytes)

        client_private_key = parameters.generate_private_key()
        client_public_key_bytes = client_private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

        # print(client_public_key_bytes)
        writer.write(client_public_key_bytes)


        server_public_key_bytes = await reader.read(max_msg_size)
        server_public_key = serialization.load_pem_public_key(server_public_key_bytes)

        shared_key = client_private_key.exchange(server_public_key)

        derived_key = HKDF(
            algorithm=hashes.SHA256(),
            length=32,
            salt=None,
            info=b'handshake data',
        ).derive(shared_key)

        print(f"Derived key: {derived_key}")

        # salt = os.urandom(16)

        # kdf = PBKDF2HMAC(
        #    algorithm=hashes.SHA256(),
        #    length=64,
        #    salt=salt,
        #    iterations=480000,
        # )

        self.shared_key = derived_key
        print(self.shared_key, ": SHARED KEY")
        # key = kdf.derive(self.shared_key)
        self.algorythm_AES = algorithms.AES(self.shared_key)


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

    #setup shared key
    await srvwrk.handshake(writer, reader)   # Ver ordem com o ui gen

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