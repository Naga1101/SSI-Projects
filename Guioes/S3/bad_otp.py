import sys
import os
import random

def bad_prng(n):
    """ an INSECURE pseudo-random number generator """
    random.seed(random.randbytes(2))
    return random.randbytes(n)

def setup(bytes_to_generate, filename):
    iv = bad_prng(bytes_to_generate)
    with open(filename, "wb") as f:
        f.write(iv)

def encode_message(message, key):
    bytes_arr = bytearray()

    for byte, key in zip(message, key):
        xorByte = byte ^ key
        bytes_arr.append(xorByte)

    return bytes(bytes_arr)

def enc(filename, key_file):
    with open(filename, 'rb') as file:
        message = file.read()
    with open(key_file, 'rb') as file:
        key = file.read()
    
    encode = encode_message(message, key)

    with open(f"{filename}.enc", 'wb') as file:  
        file.write(encode)


def decode_message(message, key):
    bytes_arr = bytearray()

    for byte, key in zip(message, key):
        xorByte = byte ^ key
        bytes_arr.append(xorByte)

    return bytes(bytes_arr)

def dec(filename, key_file):
    with open(filename, 'rb') as file:
        message_encoded = file.read()
    with open(key_file, 'rb') as file:
        key = file.read()
    
    decoded = decode_message(message_encoded, key)

    with open(f"{filename}.dec", 'wb') as file:  
        file.write(decoded)

def main(mode):

    if mode == 'setup':
        # [2] = numero de bytes aleatorios
        # [3] = nome do ficheiro
        setup(int(sys.argv[2]), sys.argv[3])
    if mode == 'enc':
        # [2] = nome do ficheiro a cifrar
        # [3] = ficheiro que contém a chave one-time-pad
        enc(sys.argv[2], sys.argv[3])
    if mode == 'dec':
        # [2] = nome do ficheiro a decifrar
        # [3] = ficheiro que contém a chave one-time-pad
        dec(sys.argv[2], sys.argv[3])


if __name__ == "__main__":
    main(sys.argv[1])


