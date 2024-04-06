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

error_messages = [b"MSG RELAY SERVICE: unknown message!", b"MSG RELAY SERVICE: uid not found!"]

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
    
    def handle_responde(self, msg, type):
        # print(msg)
        if msg != b"": 
            msg, _ = process_received_message(msg, self.shared_DHKey, self.algorythm_AES, userdata)   
        
        # Check for an error message
        if msg in error_messages:
            print(msg.decode())
            return -1

        if type == "getmsg":

            # unpair the received data
            msg_aesKey_pair, cert = unpair(msg)
            bson_message, aes_key = unpair(msg_aesKey_pair)

            # get the peer public key
            certificate = cert_loadObject(cert)
            peer_public_key = certificate.public_key()

            # decrypt the aes key with the user private key
            AESKey = decrypt_rsa(aes_key, get_private_key(userdata))
            algorithm = algorithms.AES(AESKey)

            # load the bson message 
            message_data = bson.loads(bson_message)

            subject = message_data["subject"]
            body = message_data["body"]

            # unpair sigatures from the content
            signed_subj, subj = unpair(subject)
            signed_body, message = unpair(body)

            valid_subj = verify_signature(subj, signed_subj, peer_public_key)
            valid_body = verify_signature(message, signed_body, peer_public_key)

            if valid_subj is False or valid_body is False:
                print("MSG RELAY SERVICE: verification error!", file=sys.stderr)
            
            else:
                plainSubject = decode_client_message(subj, algorithm)
                plainBody = decode_client_message(message, algorithm)

                print(f"Subject: {plainSubject.decode()}\n")
                print(f"Body: {plainBody.decode()}")


        elif type == "askqueue":
            
            # check for empty inbox
            if msg == b'error':
                print("Inbox is empty")
                return 1

            # unpair the received data
            msg_aesKey_pair, cert = unpair(msg)
            bson_message, aes_key = unpair(msg_aesKey_pair)

            # get the peer public_key
            certificate = cert_loadObject(cert)
            peer_public_key = certificate.public_key()

            # decrypt the aes key with the user private key
            AESKey = decrypt_rsa(aes_key, get_private_key(userdata))
            algorithm = algorithms.AES(AESKey)

            # load the bson message
            message_data = bson.loads(bson_message)

            print("<NUM>:<SENDER>:<TIME>:<SUBJECT>")
            if "messages" in message_data:
                for message in message_data["messages"]:
                    number = message["number"]
                    sender = message["sender"]
                    time = message["time"]
                    subject = message["subject"]

                    signature, message = unpair(subject)

                    valid = verify_signature(message, signature, peer_public_key)

                    if valid is False:
                        print("MSG RELAY SERVICE: verification error!", file=sys.stderr)

                    else:
                        plainText = decode_client_message(message, algorithm)

                        print(f"{number}:{sender}:{time}:{plainText.decode()}\n")
        else:
            #normal message received
            message = msg.decode()
            print("\n" + message)

    def start_sending_process(self, msg):
        return process_send_message(msg, self.shared_DHKey, self.algorythm_AES, userdata)
    
    def start_receiving_process(self, msg):
        return process_received_message(msg, self.shared_DHKey, self.algorythm_AES, userdata)   

    def process(self):
        """ Processa uma mensagem (`bytestring`) enviada pelo SERVIDOR.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1
        status = 0
        msg_type = None
        
        message = ""
        i = 0
        if sys.argv[1] == "-user":
            i = 3
        else:
            i = 1


        # 3 = i
        # 4 = i + 1
        if sys.argv[i] == "help":
            msg_type = "help"
            message = help_command()
            status = 1
        
        elif sys.argv[i] == "askqueue":
            
            message = askqueue_command()
            msg_type = "askqueue"

        elif sys.argv[i] == "send":
            set_target(sys.argv[i+1])
            
            subject = ' '.join(sys.argv[i+2:])

            send_header_handdler(sys.argv[i+1], subject)

            if len(sys.argv) >= i+3:
                print("Enter message body: ")
                message_body = input()

                if len(message_body) > max_msg_size:
                    print(f"Message reached 1000 bytes limit, unable to send, limit exceeded by {message - max_msg_size}")
                else:
                    message = send_add_body(message_body)
                    msg_type = "send"
                    status = 1
        
        elif sys.argv[i] == "getmsg":
            
            msg_number = sys.argv[i+1]
            message = getmsg_command(msg_number)
            msg_type = "getmsg"

        else:
            message = invalid_commad()
            status = 2

        return message, status, msg_type
    
    def handle_target_info(self,cert):

        # validar o certificado recebido
        certificado = cert_loadObject(cert)
        valid = valida_cert(certificado, get_target_name())

        if valid is False:
            print("MSG RELAY SERVICE: verification error!", file=sys.stderr)
            return -1
        
        #obtem a public key do destinatraio da mensagem
        peer_publickey = certificado.public_key()

        # gera chave para aes
        key = os.urandom(32)
        algorithm = algorithms.AES(key)

        # obtem o msg bson com os conteudos cifrados e assinados
        message = get_send_message(algorithm, get_private_key(userdata))
    
        #cifrar a chave com rsa
        aes_key = encrypt_rsa(key, peer_publickey)


        certificate = get_certificado(userdata)
        certificate_bytes = certificate.public_bytes(encoding=serialization.Encoding.PEM)

        # pair assinaturas, aes key
        key_and_bson = mkpair(message, aes_key)
        # acerscentar o certificado 
        final_message = mkpair(key_and_bson, certificate_bytes)

        return final_message
    

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
        if not valid:
            print("MSG RELAY SERVICE: verification error!", file=sys.stderr)

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

    msg, status, msg_type = client.process()

    # status for normal messages
    if status == 0:
        await client.handshake(writer, reader)  

        enc_msg = client.start_sending_process(msg)
        
        writer.write(enc_msg)
        msg = await reader.read(max_msg_size)

        client.handle_responde(msg, msg_type)

        writer.write(b'\n')
        print('\nSocket closeddd!')
        writer.close()

    # send message 
    elif status == 1:
        await client.handshake(writer, reader)  

        #obter o destino da mensagem
        get_target_msg = get_target()

        # obter a mensagem para enviar
        target_msg = client.start_sending_process(get_target_msg)

        #enviar o target e esperar pelo certificado dele
        writer.write(target_msg)
        peer_cert = await reader.read(max_msg_size)

        # verificar se a mensagem devolvida pelo servidor foi de erro
        if peer_cert in error_messages:
            print(peer_cert.decode())
            return -1


        peer_cert_dec, _= client.start_receiving_process(peer_cert)

        # receber a mensagem com os conteudos encoded
        enc_peer_msg = client.handle_target_info(peer_cert_dec)

        peer_msg = client.start_sending_process(enc_peer_msg)

        writer.write(peer_msg)
        msg = await reader.read(max_msg_size)

        client.handle_responde(msg, msg_type)

        writer.write(b'\n')
        print('\nSocket closed!')
        writer.close()

    
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

    if len(sys.argv) < 2:
        print("MSG RELAY SERVICE: verification error!"
                   """
    • send <UID> <SUBJECT> 
    • askqueue 
    • getmsg <NUM>
    • help
    """, file=sys.stderr)
        sys.exit(1)

    if sys.argv[1] == "-user":
        if not os.path.isfile(sys.argv[2]):
            raise FileNotFoundError(f"Userdata {sys.argv[2]} not found")
        else:
            userdata = sys.argv[2]
    else:
        userdata = "userdata.p12"



if __name__ == "__main__":
    try:
        check_user_data()
        run_client()
    #except ValueError as e:
        #print(e)
    except FileNotFoundError as e:
        print(e)
