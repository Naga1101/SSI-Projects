import os
import sys
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms

def encode_message():
    salt = os.urandom(16)
    fileName = sys.argv[2]
    with open(fileName, 'rb') as f:
        data = f.read()
    f.close()

    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=480000,
    )

    password = input("Insira uma password: ")
    key = kdf.derive(password.encode())
    #e

def decode_message():
    fileEncName = sys.argv[2]
    fnameKey = sys.argv[3]
    keyFile = open(fnameKey, "rb")
    key = keyFile.read()
    keyFile.close()

    fEncoded = open(fileEncName, "rb")
    nonce = fEncoded.read(16)
    encText = fEncoded.read()
    fEncoded.close()

    algoritmo = algorithms.ChaCha20(key, nonce)
    cipher = Cipher(algoritmo, mode=None)
    decifrar = cipher.decryptor()
    textoDecifrado = decifrar.update(encText)

    fnameResult = f"{fileEncName}.dec"
    fileResult = open(fnameResult, "wb")
    fileResult.write(textoDecifrado)
    fileResult.close()

def handler():
    match sys.argv[1]:
        case "enc":
            encode_message()
        case "dec":
            decode_message()
        case _:
            print("Inputs inválidos, experimente [setup | enc | dec]")

if __name__ == "__main__":
    handler()