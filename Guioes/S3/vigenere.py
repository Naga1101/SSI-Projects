import sys

def preproc(str):
    l = []
    for c in str:
        if c.isalpha():
            l.append(c.upper())
    return "".join(l)

def letter_encode(translation, letter):
    ascii = ord(letter)
    translated = ascii + translation

    if translated > 90:
        to_fix = translated - 90
        return chr(64 + to_fix)
    else:
        return chr(translated)

def encode(translation, message):
    encoded_message = ""
    translation_pos = 0
    trsize = len(translation)
    for letter in message:
        if translation_pos >= trsize:
            translation_pos = 0
        encoded_message += letter_encode(translation[translation_pos], letter)
        translation_pos += 1

    return encoded_message

def letter_decode(translation, letter):
    ascii = ord(letter)
    translated = ascii - translation

    if translated < 65:
        to_fix = (90 - (65 - translated))
        return chr(to_fix)
    else:
        return chr(translated)
def decode(translation, message):
    decoded_message = ""
    translation_pos = 0
    trsize = len(translation)
    for letter in message:
        if translation_pos >= trsize:
            translation_pos = 0
        decoded_message += letter_decode(translation[translation_pos], letter)
        translation_pos += 1

    return decoded_message

def main(operation, secret_key, message):
    # A = 65
    translation_array = []
    for letter in secret_key:
        translation_array.append(ord(letter) - 65)
    if operation == "enc":
        upd_message = preproc(message)
        enconded = encode(translation_array, upd_message)
        print(enconded)
    if operation == "dec":
        decoded = decode(translation_array, message)
        print(decoded)


if __name__ == "__main__":
    main('enc', 'BACO', "CifraIndecifravel")
    main('dec', 'BACO', "DIHGBIPRFCKTSAXSM")
    
    #main(sys.argv[1], sys.argv[2], sys.argv[3])
