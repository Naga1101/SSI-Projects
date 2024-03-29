from cryptography.hazmat.primitives.serialization.pkcs12 import load_key_and_certificates
from datetime import datetime

def get_userdata(p12_fname):
    with open(p12_fname, "rb") as f:
        p12 = f.read()
    password = None
    (private_key, user_cert, [ca_cert]) = load_key_and_certificates(p12, password)
    print("DATA GET DONE")
    return (private_key, user_cert, ca_cert)

def handle_send_command(txt, message_queue, sender):
    print("Send command received")

    tokens = txt.split()

    print(message_queue)
    print(txt)
    i = 2
    uid = tokens[1]
    print(uid, "UID")
    print("Message Queue: ", message_queue)

    if uid not in message_queue:
        return "UID Doesnt exist".encode()

    subject_tokens = []
    while tokens[i] != "|":
        subject_tokens.append(tokens[i])
        i += 1

    # skip ao separador subject | body
    i += 1  

    subject = " ".join(subject_tokens) 
    message = " ".join(tokens[i:])
    timestamp = datetime.now()

    print(f"UID: {uid}")
    print(f"SENDER: {sender}")
    print(f"TIMESTAMP: {timestamp}")
    print(f"SUBJECT: {subject}")
    print(f"MESSAGE: {message}")

    # aqui exception ou algum fix caso o uid nao exista

    # False = Ainda nao foi lida
    message_queue[uid].append((sender, timestamp, subject, message, False))

    print("Current message queue: ", message_queue)

    # Response must be in bytes
    return f"Message queued for UID {uid}".encode()

def handle_askqueue_command(txt, message_queue, sender):
    print("askqueue Command Received")
    # preciso verificar que uid enviou, usar 1 como placeholder
    response = "<NUM>:<SENDER>:<TIME>:<SUBJECT>\n"

    uid_queue = message_queue[sender]
    print(message_queue)
    message_n = 1

    if not uid_queue:
        response = "Your inbox is empty"
    else:
        for message in uid_queue:
            # se nao foi lida
            if message[4] is False:
                response += f"{message_n}:{message[0]}:{message[1]}:{message[2]}\n"
            message_n += 1

    print(response)
    return response.encode()

def handle_getmsg_command(txt, message_queue, sender):
    print("GETMSG Command received")

    tokens = txt.split()
    print(tokens)
    print(txt)
    msg_number = int(tokens[1]) - 1  # fix ao index

    uid_queue = message_queue[sender]

    if msg_number < len(uid_queue):
        message = uid_queue[msg_number]
        print(f"Message: {message}")

        # Marcar como lido
        msg_read = (message[0], message[1], message[2], message[3], True)
        message_queue[sender][msg_number] = msg_read

        print("Message marked as read.")
        response = (f"Subject: {message[2]}\n"
                    f"Message: {message[3]}")
        return response.encode()

    else:
        print(f"Message number {msg_number + 1} doesnt exist")
        response = f"Message number {msg_number + 1} doesnt exist, or its alread marked as read"

    return response.encode()

def handle_user_command(txt, sender):
    args = txt.split()
    
    # caso de omissao
    if len(args) == 1:
        print("NAO TA FEITO")
        fname = sender + ".p12"
        private_key, user_cert, ca_cert = get_userdata(fname)

        response = str(private_key) + "|\n" + str(user_cert) + "|\n" + str(ca_cert)
    
    # isto é so para o caso de vir com FNAME e nao por omissao
    else:
        fname = args[1]
        print(f"getting info from {fname}")
        private_key, user_cert, ca_cert = get_userdata(fname)

        # depois temos de arranjar esta informação direito
        response = str(private_key) + "|\n" + str(user_cert) + "|\n" + str(ca_cert)

    return response.encode()

def handle_help_command(txt):
    help_text = """
    • -user <FNAME>
    • send <UID> <SUBJECT> 
    • askqueue 
    • getmsg <NUM>
    • help
    """
    response = help_text.encode()
    return response