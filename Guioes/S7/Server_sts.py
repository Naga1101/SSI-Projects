# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.serialization import load_pem_public_key
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import certificate

conn_cnt = 0
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

class ServerWorker(object):
    """ Classe que implementa a funcionalidade do SERVIDOR. """
    def __init__(self, cnt, addr=None):
        """ Construtor da classe. """
        self.id = cnt
        self.addr = addr
        self.msg_cnt = 0
        self.shared_key = None
        self.aesgcm = None

    def process(self, msg):
        """ Processa uma mensagem (`bytestring`) enviada pelo CLIENTE.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1
        txt = msg.decode()
        print('%d : %r' % (self.id,txt))
        new_msg = txt.upper().encode()
        #
        return new_msg if len(new_msg)>0 else None
    
    async def handshake(self, writer, reader):

            parameters = dh.DHParameterNumbers(p,g).parameters()
            private_key_dh = parameters.generate_private_key()
            public_key_dh = private_key_dh.public_key()

            pem = public_key_dh.public_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PublicFormat.SubjectPublicKeyInfo
            )

            # espera receber a public key do client
            msg = await reader.read(max_msg_size)
            if msg:
                # deserialize key
                client_public_key = load_pem_public_key(msg)
                
                # chave priv no cert
                private_key = certificate.get_private_key_cert("MSG_SERVER.key")

                # assinar as duas chaves
                # pem = chave server | msg = chave cliente
                signature = private_key.sign(
                    # server pub_key + client pub_key
                    pem + msg,
                    padding.PSS(
                        mgf=padding.MGF1(hashes.SHA256()),
                        salt_length=padding.PSS.MAX_LENGTH
                    ),
                    hashes.SHA256()
                )

                # cert pem 
                cert = certificate.cert_get("MSG_SERVER.crt")

                keys_pair = mkpair(pem, signature)
                cert_keys_pair = mkpair(keys_pair, cert)

                writer.write(cert_keys_pair)

                # espera pela sig e certificado do cliente
                msg2 = await reader.read()

                if msg2:
                    client_signature, client_cert = unpair(msg2)
                    client_certificate = certificate.cert_load_serialized(client_cert)

                    if certificate.valida_certificado(client_certificate, 'User 1 (SSI MSG Relay Client 1)') is False:
                        print("Certificate is Invalid")
                    
                    public_key = client_certificate.public_key()
                    public_key.verify(
                        signature,
                        msg + pem,
                        padding.PSS(
                            mgf=padding.MGF1(hashes.SHA256()),
                            salt_length=padding.PSS.MAX_LENGTH
                        ),
                        hashes.SHA256()
                    )

                    cshared_key = private_key_dh.exchange(client_public_key)
                    derived_key = HKDF(
                        algorithm=hashes.SHA256(),
                        length=32,
                        salt=None,
                        info=b"handshake data",
                    ).derive(cshared_key)

                    print(f"Derived key: {derived_key}")

                    self.shared_key = derived_key # assign new key
                    self.aesgcm = AESGCM(self.shared_key) # start AESGCM


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
    print("Started Handshake")
    await srvwrk.handshake(writer, reader)
    print("Ended Handshake")
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