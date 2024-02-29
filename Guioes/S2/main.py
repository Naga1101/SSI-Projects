import sys


def word_counter(line):
    return len(line.split(' '))


def char_counter(line):
    x = line.count(' ')
    return len(line) - x


def main(inp):
    file = open(inp, "r")
    lines = 0
    words = 0
    chars = 0
    for line in file:
        lines += 1
        words += word_counter(line)
        chars += char_counter(line)

    print("Argumentos da linha de comando: ", inp)
    print(f"File has {lines} Lines, {words} Words and {chars} Characters")


if __name__ == "__main__":
    #main(sys.argv[1])
    main('teste.txt')
