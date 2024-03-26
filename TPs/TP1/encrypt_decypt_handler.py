import asyncio
import os
from commands_handler import *
from encrypt_decypt_handler import *
from cryptography.hazmat.primitives import hashes, hmac
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes


def decode_message(txt, key, algorythm):
    nonce = txt[:16]
    signature = txt[16:48]
    data = txt[48:]

    cipher = Cipher(algorythm, modes.GCM(nonce))
    decryptor = cipher.decryptor()
    textoDecifrado = decryptor.update(data)

    h = hmac.HMAC((key), hashes.SHA256())
    h.update(data)
    h.verify(signature)

    # print(textoDecifrado)

    return textoDecifrado

def encode_message(txt, key, algorythm):
    # print(txt, key, algorythm)

    nonce = os.urandom(16)
    
    cipher = Cipher(algorythm, modes.GCM(nonce))
    encryptor = cipher.encryptor()
    cypherText = encryptor.update(txt)

    h = hmac.HMAC((key), hashes.SHA256())
    h.update(cypherText)
    signature = h.finalize()

    final_message = nonce + signature + cypherText

    # print(final_message)

    return final_message