# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
import socket
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives.serialization import load_pem_public_key
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import certificate

conn_port = 8443
max_msg_size = 9999

p = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF
g = 2

def mkpair(x, y):
    """produz uma byte-string contendo o tuplo '(x,y)' ('x' e 'y' são byte-strings)"""
    len_x = len(x)
    len_x_bytes = len_x.to_bytes(2, "little")
    return len_x_bytes + x + y


def unpair(xy):
    """extrai componentes de um par codificado com 'mkpair'"""
    len_x = int.from_bytes(xy[:2], "little")
    x = xy[2 : len_x + 2]
    y = xy[len_x + 2 :]
    return x, y



class Client:
    """ Classe que implementa a funcionalidade de um CLIENTE. """
    def __init__(self, sckt=None):
        """ Construtor da classe. """
        self.sckt = sckt
        self.msg_cnt = 0
        self.shared_key = None
        self.aesgcm = None


    def process(self, msg=b""):
        """ Processa uma mensagem (`bytestring`) enviada pelo SERVIDOR.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt +=1
        #
        # ALTERAR AQUI COMPORTAMENTO DO CLIENTE
        #
        print('Received (%d): %r' % (self.msg_cnt , msg.decode()))
        print('Input message to send (empty to finish)')
        new_msg = input().encode()
        #
        return new_msg if len(new_msg)>0 else None
    
    async def handshake(self, writer, reader):

        parameters = dh.DHParameterNumbers(p,g).parameters()
        private_key_dh = parameters.generate_private_key()

        pem = private_key_dh.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

        writer.write(pem)

        msg = await reader.read(max_msg_size)
        if msg:
            key_pair, cert = unpair(msg)
            server_key_pem, signature = unpair(key_pair)

            server_public_key_dh = load_pem_public_key(server_key_pem)

            private_key = certificate.get_private_key_cert("MSG_CLI1.key")
            server_certificate = certificate.cert_load_serialized(cert)

            if certificate.valida_certificado(server_certificate, 'SSI Message Relay Server') is False:
                print("Certificate is invalid")
            
            server_public_key = server_certificate.public_key()
            #try:
            server_public_key.verify(
                signature,
                server_key_pem + pem,
                padding.PSS(
                    mgf=padding.MGF1(hashes.SHA256()),
                    salt_length=padding.PSS.MAX_LENGTH
                ),
                hashes.SHA256()
                )
            #except:
             #   print("Server key validation failed")

            # cliente envia a signature e cert 
            client_signature = private_key.sign(
                pem + server_key_pem,
                padding.PSS(
                    mgf=padding.MGF1(hashes.SHA256()),
                    salt_length=padding.PSS.MAX_LENGTH
                ),
                hashes.SHA256()
            )

            client_cert = certificate.cert_get("MSG_CLI1.crt")

            data = mkpair(client_signature, client_cert)
            writer.write(data)

            cshared_key = private_key_dh.exchange(server_public_key_dh)

            derived_key = HKDF(
                algorithm=hashes.SHA256(),
                length=32,
                salt=None,
                info=b"handshake data",
            ).derive(cshared_key)
            
            print(f"Derived key: {derived_key}")

            self.KEY = derived_key # assign new key
            self.aesgcm= AESGCM(self.shared_key) # start AESGCM






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

    print("Started Handshake")
    await client.handshake(writer, reader)
    print("Ended Handshake")
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
