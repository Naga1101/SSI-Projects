import sys

def decode(translation, message):
    decoded_message = ""

    for letter in message:
        ascii = ord(letter)
        translated = ascii - translation
        if translated < 65:
            to_fix = translated - 65
            decoded_message += chr(65 + to_fix)
        else:
            decoded_message += chr(translated)
    return decoded_message

def check_encode(encoded_message, word):
    word_pos = 0
    for translation in range (0, 26):
        attempt = decode(translation, encoded_message)
        if word in attempt:
            return attempt, (translation+65)


def main(argumentos):
    encoded_message = argumentos[0]

    for attempt in argumentos[1:]:
        result = check_encode(encoded_message, attempt)

        if result is not None:
            print(chr(result[1]))
            print(result[0])
            break

if __name__ == "__main__":
    teste = ["IGXZGMUKYZGTUVGVU", 'BACO', 'TAPO']
    teste2 = ["IGXZGMUKYZGTUVGVU", 'BACO', 'PAPO']
    main(teste2)
    #main(sys.argv)
