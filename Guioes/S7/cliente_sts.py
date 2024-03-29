# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
import socket
import sys
import os
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography import x509
from certificados import *

conn_port = 8443
max_msg_size = 9999
cert_cli = "MSG_CLI1.crt"
key_cli = "MSG_CLI1.key"

class Client:
    """ Classe que implementa a funcionalidade de um CLIENTE. """
    def __init__(self, sckt=None):
        """ Construtor da classe. """
        self.sckt = sckt
        self.msg_cnt = 0
        self.shared_DHKey = b''
        self.aesgcm = None
        self.cli_privRSA_KEY = self.handleKey(key_cli)
        self.cert = None

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

    def process(self, msg=b""):
        """ Processa uma mensagem (`bytestring`) enviada pelo SERVIDOR.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        if(msg):            
            old_nonce = msg[:12]
            dt = self.aesgcm.decrypt(old_nonce, msg[12:], None)
            print('Received (%d): %r' % (self.msg_cnt , dt))
            print('Input message to send (empty to finish)')
            self.msg_cnt +=1
        #
        # ALTERAR AQUI COMPORTAMENTO DO CLIENTE
        #
        new_msg = input().encode()
        nonce = os.urandom(12)
        ct = self.aesgcm.encrypt(nonce, new_msg, None)
        final_msg = nonce + ct
        
        return final_msg if len(new_msg)>0 else None
    
    async def handshake(self, writer, reader):

        p = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF
        g = 2

        # gera com os parametros dh da chave publica e privada
        parameters = dh.DHParameterNumbers(p,g).parameters()
        client_private_key = parameters.generate_private_key()

        client_public_key_bytes = client_private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

        print("enviar chave dh do cliente")


        # cliente envia a sua public key gerada pelos parametros dh
        writer.write(client_public_key_bytes)

        print("esperar pelas chaves assinadas pelo server...")

        # espera pela resposta
        reply = await reader.read(max_msg_size)
        
        pair_pubKeyServ_signKeys, cert_server_bytes = unpair(reply)

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

        self.aesgcm= AESGCM(self.shared_DHKey) # start AESGCM



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

    msg = client.process()
    while msg:
        writer.write(msg)
        msg = await reader.read(max_msg_size)
        if msg :
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