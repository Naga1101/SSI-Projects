# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
import socket
import os
from encrypt_decypt_handler import *
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

conn_port = 8443
max_msg_size = 999


class Client:
    """ Classe que implementa a funcionalidade de um CLIENTE. """

    def __init__(self, sckt=None):
        """ Construtor da classe. """
        self.sckt = sckt
        self.msg_cnt = 0
        self.msg_cnt = 0
        self.shared_key = None
        self.algorythm_AES = None


    def process(self, msg):
        """ Processa uma mensagem (`bytestring`) enviada pelo SERVIDOR.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1
        #
        # ALTERAR AQUI COMPORTAMENTO DO CLIENTE
        #

        if msg != b"": msg = decode_message(msg, self.shared_key, self.algorythm_AES)   

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

                return encode_message(message.encode(), self.shared_key, self.algorythm_AES)

        if command.startswith('askqueue'):
            message = 'askqueue'
            return encode_message(message.encode(), self.shared_key, self.algorythm_AES)

        if command.startswith('help'):
            message = 'help'
            return encode_message(message.encode(), self.shared_key, self.algorythm_AES)

        if command.startswith('getmsg'):
            msg_number = command.split()[1]
            message = f"getmsg {msg_number}"

            return encode_message(message.encode(), self.shared_key, self.algorythm_AES)

        return encode_message(command.encode(), self.shared_key, self.algorythm_AES)

    async def handshake(self, writer, reader):

        # p = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF
        # g = 2
        # parameters = dh.DHParameterNumbers(p,g).parameters()

        # gerar os DH Paramenters e enviar ao server
        parameters = dh.generate_parameters(generator=2, key_size=2048)

        # serializar os parametros e enviar ao server
        param_bytes = parameters.parameter_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.ParameterFormat.PKCS3
        )
        # enviar ao servidor
        writer.write(param_bytes)

        server_private_key = parameters.generate_private_key()

        server_public_key_bytes = server_private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

        writer.write(server_public_key_bytes)

        client_public_key_bytes = await reader.read(max_msg_size)
        client_public_key = serialization.load_pem_public_key(client_public_key_bytes)

        shared_key = server_private_key.exchange(client_public_key)

        derived_key = HKDF(
            algorithm=hashes.SHA256(),
            length=32,
            salt=None,
            info=b'handshake data',
        ).derive(shared_key)

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

    await client.handshake(writer, reader)

    msg = client.process(b"")

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
