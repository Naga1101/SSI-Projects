# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
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

conn_cnt = 0
conn_port = 8443
max_msg_size = 9999
cert_srv = "MSG_SERVER.crt"
key_srv = "MSG_SERVER.key"


class ServerWorker(object):
    """ Classe que implementa a funcionalidade do SERVIDOR. """
    def __init__(self, cnt, addr=None):
        self.id = cnt
        self.addr = addr
        self.msg_cnt = 0
        self.shared_DHKey = b""
        self.aesgcm = None
        self.srv_privRSA_KEY = self.handleKey(key_srv)
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

    
    def process(self, msg):
        """ Processa uma mensagem (`bytestring`) enviada pelo CLIENTE.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        print(msg)
        old_nonce = msg[:12]
        dt = self.aesgcm.decrypt(old_nonce, msg[12:], None)
        self.msg_cnt += 1
        #
        # ALTERAR AQUI COMPORTAMENTO DO SERVIDOR
        #        
        txt = dt
        print('%d : %r' % (self.id,txt))
        new_msg = txt.upper()
        nonce = os.urandom(12)
        ct = self.aesgcm.encrypt(nonce, new_msg, None)
        final_msg = nonce + ct
        
        return final_msg if len(new_msg)>0 else None
    
    
    async def handshake(self, writer, reader):
        
        p = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF
        g = 2

        # gera com os parametros dh da chave publica e privada
        parameters = dh.DHParameterNumbers(p,g).parameters()
        server_private_key = parameters.generate_private_key()

        server_public_key_bytes = server_private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

        print("espera pela chave dh do cliente...")

        # server recebe a chave publica do cliente
        client_public_key_bytes = await reader.read(max_msg_size)
        client_public_key = serialization.load_pem_public_key(client_public_key_bytes)
        
        # para da chave publica do server e da chave publica do cliente
        chaves_pubServ_pubCli = mkpair(server_public_key_bytes, client_public_key_bytes)
        # print(chaves_pubServ_pubCli)
        print("assinar chaves")

        # assinar o par das chaves com a do chave privada rsa server
        sign_Keys = self.srv_privRSA_KEY.sign(
            chaves_pubServ_pubCli,
            padding.PSS(
                mgf=padding.MGF1(hashes.SHA256()),
                salt_length=padding.PSS.MAX_LENGTH
            ),
            hashes.SHA256()
        )
        # print("chaves assinadas", sign_Keys)

        # par das chaves assinadas com a chave publica do server
        pair_pubKeyServ_signKeys = mkpair(server_public_key_bytes, sign_Keys) 
        # print("Par de chaves: ", pair_pubKeyServ_signKeys)

        cert_server = cert_read(cert_srv)
        # print("certificado do server: ", cert_server)

        # certificado do server e as chaves publicas dh do cliente e do server
        reply = mkpair(pair_pubKeyServ_signKeys, cert_server)

        # print(reply)
        
        print("enviar chaves assinadas pelo server")

        writer.write(reply)

        print("esperar pelas chaves assinadas pelo cliente...")

        pair_signKeys_certCli = await reader.read(max_msg_size)

        sign_KeysCli, cert_client_bytes = unpair(pair_signKeys_certCli)

        cert_client = cert_loadObject(cert_client_bytes)
        print("cereal number", cert_client.serial_number)

        chaves_pubCli_pubSrv = mkpair(client_public_key_bytes, server_public_key_bytes)

        print("pares descompactados")
        print("validar certificado do cliente")

        teste = valida_cert(cert_client, "MSG_CLI1")
        # if not teste: print("Certificado não validado")

        print("validar chaves assinadas do server")

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

        print("derivar chave partilhada")

        shared_key = server_private_key.exchange(client_public_key)

        derived_key = HKDF(
            algorithm=hashes.SHA256(),
            length=32,
            salt=None,
            info=b'handshake data',
            ).derive(shared_key)
        
        print(f"Derived key: {derived_key}")

        self.shared_DHKey = derived_key # assign new key
        self.aesgcm= AESGCM(self.shared_DHKey) # start AESGCM


#
#
# Funcionalidade Cliente/Servidor
#
# obs: não deverá ser necessário alterar o que se segue
#


async def handle_echo(reader, writer):
    # print("aqui")
    global conn_cnt
    conn_cnt +=1
    addr = writer.get_extra_info('peername')
    print(conn_cnt, addr)
    srvwrk = ServerWorker(conn_cnt, addr)
    # print("teste")

    #setup shared key
    await srvwrk.handshake(writer, reader)

    data = await reader.read(max_msg_size)
    while True:
        if not data: continue
        if data[:1]==b'\n': break
        data = srvwrk.process(data)
        if not data: break
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