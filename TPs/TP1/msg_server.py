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
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography import x509
from certificados import *
from logger import *


conn_cnt = 0
conn_port = 8443
max_msg_size = 9999
max_send_msg_size = 1000

# Variaveis

session_file = generate_log_file()
p12_file = "MSG_SERVER.p12"
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
        self.srv_privRSA_KEY = self.handleKey()
        self.cert = None
        self.algorythm_AES = None

    def handleKey(self):

        private_key = get_private_key(p12_file)

        return private_key
    

    def valid_message(self, msg):
        key = msg.decode().split(" ")
        if key[0] in ["askqueue", "help"] and len(key) == 1:
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

        msg, sender = process_received_message(msg, self.shared_DHKey, self.algorythm_AES, "MSG_SERVER.p12")

        txt = msg.decode()
        print('%d : %r' % (self.id, txt))

        # caso nao entre em nenhuma condição
        response = """MSG RELAY SERVICE: command error!\n\n
            • send <UID> <SUBJECT> 
            • askqueue 
            • getmsg <NUM>
            • help
            """.encode()

        if self.valid_message(msg) == 1:

            # diferentes tipos de request do client

            """if txt.startswith("-user"):

                print("user info request received")
                response = handle_user_command(txt, sender, session_file)"""

            if txt.startswith("help"):
                response = handle_help_command(txt, sender, session_file)

            elif txt.startswith("send"):
                print(txt)
                response = handle_send_command(txt, message_queue, sender, session_file)

            elif txt.startswith("askqueue"):
                response = handle_askqueue_command(txt, message_queue, sender, session_file)

            elif txt.startswith("getmsg"):
                response = handle_getmsg_command(txt, message_queue, sender, session_file)
            
            return process_send_message(response, self.shared_DHKey, self.algorythm_AES, "MSG_SERVER.p12")

        else:
            log_invalid_command(session_file, sender)
            return process_send_message(response, self.shared_DHKey, self.algorythm_AES, "MSG_SERVER.p12")

    async def handshake(self, writer, reader):

        print("-----------------HANDSHAKE------------------------\n")
        print("A aguardar a geração dos parâmetros e envio do cliente")
        param_bytes = await reader.read(max_msg_size)

        print("Parâmetros recebidos")

        # desserializar os parametros
        parameters = serialization.load_pem_parameters(param_bytes)

        server_private_key = parameters.generate_private_key()
        server_public_key_bytes = server_private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

        print("Server - à espera pela chave dh do cliente...")

        # server recebe a chave publica do cliente
        client_public_key_bytes = await reader.read(max_msg_size)
        client_public_key = serialization.load_pem_public_key(client_public_key_bytes)
        
        # para da chave publica do server e da chave publica do cliente
        chaves_pubServ_pubCli = mkpair(server_public_key_bytes, client_public_key_bytes)
        # print(chaves_pubServ_pubCli)
        print("Server - assinar as chaves")

        # assinar o par das chaves com a do chave privada rsa server
        sign_Keys = self.srv_privRSA_KEY.sign(
            chaves_pubServ_pubCli,
            padding.PSS(
                mgf=padding.MGF1(hashes.SHA256()),
                salt_length=padding.PSS.MAX_LENGTH
            ),
            hashes.SHA256()
        )
        print("Server - chaves assinadas")

        # par das chaves assinadas com a chave publica do server
        pair_pubKeyServ_signKeys = mkpair(server_public_key_bytes, sign_Keys) 
        # print("Par de chaves: ", pair_pubKeyServ_signKeys)

        #cert_server = cert_read(cert_srv)
        certificate_server = get_certificado(p12_file)
        cert_server = certificate_server.public_bytes(encoding=serialization.Encoding.PEM)

        # print("certificado do server: ", cert_server)

        # certificado do server e as chaves publicas dh do cliente e do server
        reply = mkpair(pair_pubKeyServ_signKeys, cert_server)

        # print(reply)
        
        print("A enviar chaves assinadas pelo server")

        writer.write(reply)

        print("À espera pelas chaves assinadas pelo cliente...")

        pair_signKeys_certCli = await reader.read(max_msg_size)

        sign_KeysCli, cert_client_bytes = unpair(pair_signKeys_certCli)

        cert_client = cert_loadObject(cert_client_bytes)
        print("Serial number: ", cert_client.serial_number)

        chaves_pubCli_pubSrv = mkpair(client_public_key_bytes, server_public_key_bytes)

        print("Pares de chaves descompactadors")

        print("Iniciar Validação do certificado do cliente")

        valid = valida_cert(cert_client, "MSG_CLI1")
        if valid: 
            print("Certificado validado")
            # var sender para log
            sender = uid_gen(cert_client.subject.get_attributes_for_oid(x509.NameOID.PSEUDONYM)[0].value)
        else:
            print("Validação do certificado falhada")
            return -1

        # validar se as chaves recebidas estão corretas
        public_RSA_key_client = cert_client.public_key()
        public_RSA_key_client.verify(
            sign_KeysCli,
            chaves_pubCli_pubSrv,
            padding.PSS(
                mgf=padding.MGF1(hashes.SHA256()),
                salt_length=padding.PSS.MAX_LENGTH
            ),
            hashes.SHA256()
        )

        print("Derivar shared key")

        shared_key = server_private_key.exchange(client_public_key)


        derived_key = HKDF(
            algorithm=hashes.SHA256(),
            length=32,
            salt=None,
            info=b'handshake data',
        ).derive(shared_key)

        print(f"Shared Key: {derived_key}")

        self.shared_DHKey = derived_key # assign new key
        self.algorythm_AES = algorithms.AES(self.shared_DHKey)
        log_action(session_file, "Initial Handshake", sender, None)

        print("---------------------------------------------------")


def uid_gen(PSEUDONYM):
    last_uid = list(uids)[-1]
    next_uid = int(last_uid) + 1
    uids[next_uid] = PSEUDONYM
    if PSEUDONYM not in message_queue:
        message_queue[PSEUDONYM] = []
    return PSEUDONYM

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

    print("Message Queue ", message_queue)

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
    global session_file
    log_session_start(session_file)
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
    log_session_end(session_file)
    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()
    print('\nFINISHED!')


run_server()