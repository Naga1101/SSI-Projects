import sys

def main():
    if len(sys.argv) != 2:
        print("Uso: python3 print.py <nome_do_arquivo>")
        return 1
    try:
        with open(sys.argv[1], 'r') as file:
            print(file.read(), end="")
    except Exception as e:
        print(f"Erro ao abrir ou ler: {e}", file=sys.stderr)
        return 2

if __name__ == "__main__":
    main()