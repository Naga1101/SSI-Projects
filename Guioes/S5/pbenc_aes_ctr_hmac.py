import os
import sys
from cryptography.hazmat.primitives import hashes, hmac
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes


def encode_message(file):
    modo = ".enc"
    with open(file, 'rb') as f:
        data = f.read()
    f.close()
    # derive
    salt = os.urandom(16)
    nonce = os.urandom(16)
    kdf = PBKDF2HMAC(
        algorithm = hashes.SHA256(),
        length = 64,
        salt = salt,
        iterations = 480000,
    )
    password = input("Insira uma password: ")
    key = kdf.derive(password.encode())
    
    algoritmo = algorithms.AES((key[:32]))
    cipher = Cipher(algoritmo, modes.CTR(nonce))
    encryptor = cipher.encryptor()
    cypherText = encryptor.update(data)

    h = hmac.HMAC((key[32:]), hashes.SHA256())
    h.update(cypherText)
    signature = h.finalize()
    
    with open(file + modo, 'wb') as f:
        data = f.write(salt)
        data = f.write(nonce)
        data = f.write(signature)
        data = f.write(cypherText)
    f.close()

def decode_message(file):
    modo = ".dec"
    with open(file, 'rb') as f:
        salt = f.read(16)
        nonce = f.read(16)
        signature = f.read(32)
        data = f.read()
    f.close()

    kdf = PBKDF2HMAC(
        algorithm = hashes.SHA256(),
        length = 64,
        salt = salt,
        iterations = 480000,
    )
    password = input("Insira uma password: ")
    key = kdf.derive(password.encode())

    algoritmo = algorithms.AES((key[:32]))
    cipher = Cipher(algoritmo, modes.CTR(nonce))
    decryptor = cipher.decryptor()
    textoDecifrado = decryptor.update(data)

    h = hmac.HMAC((key[32:]), hashes.SHA256())
    h.update(data)
    h.verify(signature)

    with open(file + modo, 'wb') as f:
        data = f.write(textoDecifrado)
    f.close()



def handler():
    match sys.argv[1]:
        case "enc":
            encode_message(sys.argv[2])
        case "dec":
            decode_message(sys.argv[2])
        case _:
            print("Inputs inválidos, experimente [setup | enc | dec]")


if __name__ == '__main__':
    handler()