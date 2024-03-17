import os
import sys
from cryptography.hazmat.primitives import hashes, hmac
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305


def encode_message(file):
    modo = ".enc"
    with open(file, 'rb') as f:
        data = f.read()
    f.close()

    aad = b"Placeholder"
    nonce = os.urandom(12)
    key = ChaCha20Poly1305.generate_key()
    with open("chacha.key", "wb") as f:
        f.write(key)
    chacha = ChaCha20Poly1305(key)
    ct = chacha.encrypt(nonce, data, aad)
    with open(file + modo, 'wb') as f:
        f.write(nonce)
        f.write(ct)

def decode_message(file):
    modo = ".dec"
    aad = b"Placeholder"

    with open(file, 'rb') as f, open("chacha.key") as f2:
        data = f.read()
        nonce = data[:12]
        enc = data[:12]
        chcha = ChaCha20Poly1305(f2.read())
        dec_data = chcha.decrypt(nonce, enc, aad)

    with open(file + modo, 'wb') as f:
        data = f.write(dec_data)

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