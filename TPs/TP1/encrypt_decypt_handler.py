import asyncio
import os
from encrypt_decypt_handler import *
from cryptography.hazmat.primitives import hashes, hmac
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.asymmetric import padding
from certificados import *

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

def process_send_message(txt, key, algorithm, source):
    encoded_message = encode_message(txt, key, algorithm)

    private_key, cert, _ = get_userdata(source)

    certificate = cert.public_bytes(encoding=serialization.Encoding.PEM)

    signature = private_key.sign(
        encoded_message,
        padding.PSS(
            mgf=padding.MGF1(hashes.SHA256()),
            salt_length=padding.PSS.MAX_LENGTH
        ),
        hashes.SHA256()
    )

    sign_message = mkpair(signature, encoded_message)
    final_message = mkpair (sign_message, certificate)

    return final_message

def process_received_message(txt, key, algorithm, source):

    sign_message, cert = unpair(txt)
    signature, encoded_message = unpair(sign_message)

    certificate = cert_loadObject(cert)

    # verificar a assinatura

    certificate.public_key().verify(
        signature,
        encoded_message,
        padding.PSS(
            mgf=padding.MGF1(hashes.SHA256()),
            salt_length=padding.PSS.MAX_LENGTH
        ),
        hashes.SHA256()
    )
    
    plainText = decode_message(encoded_message, key, algorithm)

    sender = certificate.subject.get_attributes_for_oid(x509.NameOID.PSEUDONYM)[0].value

    return plainText, sender


