import os
import sys
import struct
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

def setup():
    filenameKey = sys.argv[2]
    key = os.urandom(32)

    fileKey = open(filenameKey, "wb")
    fileKey.write(key)
    fileKey.close()

    print(f"Chave {filenameKey} criada com 32 bytes!")

def encode_message():
    nonce = os.urandom(16)
    fileName = sys.argv[2]
    fnameKey = sys.argv[3]
    keyFile = open(fnameKey, "rb")
    key = keyFile.read()
    keyFile.close()

    fTextoNormal = open(fileName, "rb")
    textoNormal = fTextoNormal.read()
    fTextoNormal.close()

    algoritmo = algorithms.ChaCha20(key, nonce)
    cipher = Cipher(algoritmo, mode=None)
    encriptar = cipher.encryptor()
    textoEncryptado = encriptar.update(textoNormal)

    fnameResult = f"{fileName}.enc"
    fileResult = open(fnameResult, "wb")
    fileResult.write(nonce)
    fileResult.write(textoEncryptado)
    fileResult.close()

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
        case "setup":
            setup()
        case "enc":
            encode_message()
        case "dec":
            decode_message()
        case _:
            print("Inputs inválidos, experimente [setup | enc | dec]")

if __name__ == "__main__":
    handler()