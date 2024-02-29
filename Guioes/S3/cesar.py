import sys

def preproc(str):
    l = []
    for c in str:
        if c.isalpha():
            l.append(c.upper())
    return "".join(l)

def encode(translation, message):
    # A - 65 | Z - 90
    encoded_message = ""
    for letter in message:
        ascii = ord(letter)
        translated = ascii + translation
        if translated > 90:
            to_fix = translated - 90
            encoded_message += chr(65 + to_fix)
        else:
            encoded_message += chr(translated)
    return encoded_message

def decode(translation, message):
    decoded_message = ""

    for letter in message:
        ascii = ord(letter)
        translated = ascii - translation
        if translated < 65:
            to_fix = (90 - (65 - translated))
            decoded_message += chr(to_fix)
        else:
            decoded_message += chr(translated)
    return decoded_message

def main(operation, secret_key, message):
    # A = 65
    translation = ord(secret_key) - 65
    if operation == "enc":
        upd_message = preproc(message)
        enconded = encode(translation, upd_message)
        print(enconded)
    if operation == "dec":
        decoded = decode(translation, message)
        print(decoded)


if __name__ == "__main__":
    main('enc', 'G', "CartagoEstaNoPapo")
    main('dec', 'G', "IGXZGMUKYZGTUVGVU")
    #main(sys.argv[1], sys.argv[2], sys.argv[3])
