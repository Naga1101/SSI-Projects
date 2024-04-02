import bson
from datetime import datetime

# Example message arguments
sender = "Alice"
recipient = "Bob"
content = "Hello, Bob!"
timestamp = datetime.now()

# Define the message dictionary
message = {
    "sender": sender,
    "recipient": recipient,
    "content": content,
    "timestamp": timestamp
}

# Convert the dictionary to BSON
bson_data = bson.dumps(message)

print(bson_data + b"\nteste")

# Decode the BSON data
decoded_message = bson.loads(bson_data)

print("2",bson.loads(bson_data + b"teste"))

# Print the decoded message
print("1", decoded_message)


# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
import socket
import os
import sys
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
from messages_dict import *
from certificados import *

conn_port = 8443
max_msg_size = 9999
max_send_msg_size = 1000

p12_file = "MSG_CLI1.p12"
userdata = None

class Client:
    """ Classe que implementa a funcionalidade de um CLIENTE. """

    def __init__(self, sckt=None):
        """ Construtor da classe. """
        self.sckt = sckt
        self.msg_cnt = 0
        self.msg_cnt = 0
        self.shared_key = None
        self.cli_privRSA_KEY = self.handleKey()  # chave dos p12
        self.cert = None
        self.algorythm_AES = None

    def handleKey(self):

        private_key = get_private_key(userdata)

        return private_key
    
    def handle_responde(self, msg):
        if msg != b"": 
            print("---------------------------------------------------")
            msg, _ = process_received_message(msg, self.shared_DHKey, self.algorythm_AES, userdata)   

        # print error cases to sderr
        message = msg.decode()
        if message.startswith("MSG RELAY SERVICE: "):
            sys.stderr.write("\n" + message + "\n")
        else:
            print("\n" + message)

    def start_sending_process(self, msg):
        return process_send_message(msg.encode(), self.shared_DHKey, self.algorythm_AES, userdata)

    def process(self):
        """ Processa uma mensagem (`bytestring`) enviada pelo SERVIDOR.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1
        status = 0

        if sys.argv[3] == "help":

            message = help_command()
            status = 1
        
        elif sys.argv[3] == "askqueue":
            
            message = askqueue_command()
            message = 'askqueue'
    
        elif sys.argv[3] == "send":
            send_header_handdler(sys.argv[3], sys.argv[4], sys.argv[5])
            
            print("Enter message body: ")
            message_body = input()

            message_size = len(message_body) + len(sys.argv[4:])

            if message_size > max_msg_size:
                print(f"Message reached 1000 bytes limit, unable to send, limit exceeded by {message - max_msg_size}")
            else:
                message = send_add_body(message_body)
                print(message)
                message = f"{' '.join(sys.argv[3:])} | {message_body}"
        
        elif sys.argv[3] == "getmsg":
            
            msg_number = sys.argv[4]
            message = getmsg_command(msg_number)
            message = f"getmsg {msg_number}"

        return message, status

    async def handshake(self, writer, reader):

        print("-----------------HANDSHAKE------------------------\n")

        print("Cliente a gerar os parametros")

        parameters = dh.generate_parameters(generator=2, key_size=2048)

        # serializar os parametros e enviar ao server
        param_bytes = parameters.parameter_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.ParameterFormat.PKCS3
        )

        print("Cliente envia os parametros")

        # enviar ao servidor
        writer.write(param_bytes)

        client_private_key = parameters.generate_private_key()

        client_public_key_bytes = client_private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )


        # cliente envia a sua public key gerada pelos parametros dh
        writer.write(client_public_key_bytes)

        print("Esperar pelas chaves assinadas pelo server...")

        # espera pela resposta
        reply = await reader.read(max_msg_size)

        # print(reply)
        
        pair_pubKeyServ_signKeys, cert_server_bytes = unpair(reply)

        # print(cert_server_bytes)

        cert_server = cert_loadObject(cert_server_bytes)
        
        server_public_key_bytes, sign_Keys = unpair(pair_pubKeyServ_signKeys)
        
        # criar um par de chaves para validar as sign Keys
        pair_pubKeyServ_pubKeyCli = mkpair(server_public_key_bytes, client_public_key_bytes)
        
        print("Pares descompactados")
        print("Validar certificado do server")

        valid = valida_cert(cert_server, 'MSG_SERVER')
        #if not valid:
        #    sys.stderr("MSG RELAY SERVICE: verification error!")

        print("Validar chaves assinadas do server")

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

        print("Derivar chave partilhada")
        
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

        certificate_client = get_certificado(userdata)
        cert_client = certificate_client.public_bytes(encoding=serialization.Encoding.PEM)
        #cert_client = cert_read(cert_cli)

        pair_signKeys_certCli = mkpair(sign_Keys, cert_client)

        print("Enviar chaves assinadas pelo cliente")

        writer.write(pair_signKeys_certCli)
        
        self.algorythm_AES = algorithms.AES(self.shared_DHKey)
        
        print("--------------------------------------------------\n")

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

    msg, status = client.process()

    if status == 0:
        await client.handshake(writer, reader)  

        enc_msg = client.start_sending_process(msg)
        
        writer.write(enc_msg)
        msg = await reader.read(max_msg_size)

        client.handle_responde(msg)
    else:
        print(msg)

    writer.write(b'\n')
    print('\nSocket closed!')
    writer.close()


def run_client():
    loop = asyncio.get_event_loop()
    loop.run_until_complete(tcp_echo_client())

def check_user_data():
    global userdata

    if len(sys.argv) < 3 or sys.argv[1] != "-user":
        raise ValueError ("Usage: msg_client.py -user <FNAME> args")
    if not os.path.isfile(sys.argv[2]):
        raise FileNotFoundError(f"Userdata {sys.argv[2]} not found")
    else:
        userdata = sys.argv[2]


if __name__ == "__main__":
    try:
        check_user_data()
        run_client()
    except ValueError as e:
        print(e)
    except FileNotFoundError as e:
        print(e)
