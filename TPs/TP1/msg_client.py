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
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography import x509
from certificados import *

conn_port = 8443
max_msg_size = 9999
max_send_msg_size = 1000

cert_cli = "MSG_CLI1.crt"
key_cli = "MSG_CLI1.key"

class Client:
    """ Classe que implementa a funcionalidade de um CLIENTE. """

    def __init__(self, sckt=None):
        """ Construtor da classe. """
        self.sckt = sckt
        self.msg_cnt = 0
        self.msg_cnt = 0
        self.shared_key = None
        self.cli_privRSA_KEY = self.handleKey(key_cli)  # chace dos p12
        self.cert = None
        self.algorythm_AES = None

    def handleKey(self, key_path):
        fileKey = open(key_path, "rb")
        data = fileKey.read()
        fileKey.close()

        password = "1234"
        password_bytes = password.encode('utf-8')
        
        key = serialization.load_pem_private_key(
                data,
                password=password_bytes 
            )

        return key


    def process(self, msg):
        """ Processa uma mensagem (`bytestring`) enviada pelo SERVIDOR.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1
        #
        # ALTERAR AQUI COMPORTAMENTO DO CLIENTE
        #

        if msg != b"": msg = decode_message(msg, self.shared_DHKey, self.algorythm_AES)   

        #print('Received (%d): %r' % (self.msg_cnt , msg.decode()))
        print("\n" + msg.decode())
        print('Input message to send (empty to finish)')

        command = input().strip()
        if command.startswith('-user'):
            args = command.split()

            if len(args) > 1:
                fname = args[1]
                message = f'-user {fname}'
            else:
                fname = ""
                message = "-user"    

            return encode_message(message.encode(), self.shared_DHKey, self.algorythm_AES)

        if command.startswith('send'):
            print("Enter message body: ")
            message_body = input()

            message_size = len(message_body) + len(command)

            if message_size > max_msg_size:
                print("Message reached 1000 bytes limit, unable to send")
            else:
                # falta caso subject ter mais de 1 palavra
                # uid vai com o certificado?
                message = f"{command} | {message_body}"

                return encode_message(message.encode(), self.shared_DHKey, self.algorythm_AES)

        elif command.startswith('askqueue'):
            message = 'askqueue'
            return encode_message(message.encode(), self.shared_DHKey, self.algorythm_AES)

        elif command.startswith('help'):
            message = 'help'
            return encode_message(message.encode(), self.shared_DHKey, self.algorythm_AES)

        elif command.startswith('getmsg'):
            msg_number = command.split()[1]
            message = f"getmsg {msg_number}"

            return encode_message(message.encode(), self.shared_DHKey, self.algorythm_AES)

        return encode_message(command.encode(), self.shared_DHKey, self.algorythm_AES)

    async def handshake(self, writer, reader):

        print("cliente a gerar os parametros")

        parameters = dh.generate_parameters(generator=2, key_size=2048)

        # serializar os parametros e enviar ao server
        param_bytes = parameters.parameter_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.ParameterFormat.PKCS3
        )

        print("cliente envia os parametros")

        # enviar ao servidor
        writer.write(param_bytes)

        client_private_key = parameters.generate_private_key()

        client_public_key_bytes = client_private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )


        # cliente envia a sua public key gerada pelos parametros dh
        writer.write(client_public_key_bytes)

        print("esperar pelas chaves assinadas pelo server...")

        # espera pela resposta
        reply = await reader.read(max_msg_size)

        # print(reply)
        
        pair_pubKeyServ_signKeys, cert_server_bytes = unpair(reply)

        # print(cert_server_bytes)

        cert_server = cert_loadObject(cert_server_bytes)
        
        server_public_key_bytes, sign_Keys = unpair(pair_pubKeyServ_signKeys)
        
        # criar um par de chaves para validar as sign Keys
        pair_pubKeyServ_pubKeyCli = mkpair(server_public_key_bytes, client_public_key_bytes)
        
        print("pares descompactados")
        print("validar certificado do server")

        teste = valida_cert(cert_server, 'MSG_SERVER')
        # if not teste: print("Certificado não validado")

        print("validar chaves assinadas do server")

        # validar se as chaves recebidas estão corretas
        public_RSA_key_server = cert_server.public_key()
        public_RSA_key_server.verify(
            sign_Keys,
            pair_pubKeyServ_pubKeyCli,
            padding.PSS(
                mgf=padding.MGF1(hashes.SHA256()),
                salt_length=padding.PSS.MAX_LENGTH
            ),
            hashes.SHA256()
        )

        print("derivar chave partilhada")
        
        # derivar a chave
        server_public_key = serialization.load_pem_public_key(server_public_key_bytes)
        shared_key = client_private_key.exchange(server_public_key)

        derived_key = HKDF(
            algorithm=hashes.SHA256(),
            length=32,
            salt=None,
            info=b'handshake data',
        ).derive(shared_key)

        print(f"Derived key: {derived_key}")
        self.shared_DHKey = derived_key # assign new key

        # fazer par de chave publica dh do cliente com chave publica dh do server
        chaves_pubCli_pubSrv = mkpair(client_public_key_bytes, server_public_key_bytes)

        # assinar o par das chaves com a do chave privada rsa server
        sign_Keys = self.cli_privRSA_KEY.sign(
            chaves_pubCli_pubSrv,
            padding.PSS(
                mgf=padding.MGF1(hashes.SHA256()),
                salt_length=padding.PSS.MAX_LENGTH
            ),
            hashes.SHA256()
        )

        cert_client = cert_read(cert_cli)

        pair_signKeys_certCli = mkpair(sign_Keys, cert_client)

        print("enviar chaves assinadas pelo cliente")

        writer.write(pair_signKeys_certCli)
        
        self.algorythm_AES = algorithms.AES(self.shared_DHKey)

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
