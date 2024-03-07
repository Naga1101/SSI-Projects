import os
import sys
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.primitives.ciphers.modes import CBC

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

    padder = padding.PKCS7(128).padder()
    padded_plaintext = padder.update(textoNormal)
    padded_plaintext += padder.finalize()

    iv = os.urandom(1)
    mode = CBC(iv)

    algoritmo = algorithms.AE(key)
    cipher = Cipher(algoritmo, mode=mode)
    encriptar = cipher.encryptor()
    textoEncryptado = encryptor.update(padded_plaintext) + encryptor.finalize()

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
    iv = fEncoded.read(16)
    encText = fEncoded.read()
    fEncoded.close()

    cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
    decryptor = cipher.decryptor()
    padded_plaintext = decryptor.update(encText) + decryptor.finalize()
    
    # Remover padding
    unpadder = padding.PKCS7(128).unpadder()
    textoDecifrado = unpadder.update(padded_plaintext) + unpadder.finalize()

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