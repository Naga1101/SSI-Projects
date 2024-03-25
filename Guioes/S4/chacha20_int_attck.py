import os
import sys

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

def main():
    fctxt = sys.argv[1] # nome ficheiro
    pos = sys.argv[2] # pos onde ptxtAtPos foi cifrado
    ptxtAtPos = sys.argv[3]
    newPtxtAtPost =  sys.argv[4] # o que se obtem quando se decifra o file

if __name__ == "__main__":
    main()