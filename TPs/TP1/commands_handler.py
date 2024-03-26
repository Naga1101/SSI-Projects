def handle_send_command(txt, message_queue):
    print("Send command received")

    tokens = txt.split()

    uid = tokens[1]
    subject = tokens[2]
    message = " ".join(tokens[3:])

    print(f"UID: {uid}")
    print(f"SUBJECT: {subject}")
    print(f"MESSAGE: {message}")

    # aqui exception ou algum fix caso o uid nao exista

    # False = Ainda nao foi lida
    message_queue[int(uid)].append((subject, message, False))

    print("Current message queue: ", message_queue)

    # Response must be in bytes
    return f"Message queued for UID {uid}".encode()

def handle_askqueue_command(txt, message_queue):
    print("askqueue Command Received")
    # preciso verificar que uid enviou, usar 1 como placeholder
    response = "<NUM>:<SENDER>:<TIME>:<SUBJECT>\n"

    uid_queue = message_queue[1]
    message_n = 1

    if not uid_queue:
        response = "Your inbox is empty"
    else:
        for message in uid_queue:
            # se nao foi lida
            if message[2] is False:
                response += f"{message_n}:<SENDER>:<TIME>:{message[0]}\n"
                message_n += 1

    print(response)
    return response.encode()

def handle_getmsg_command(txt, message_queue):
    print("GETMSG Command received")

    uid_placeholder = 1

    tokens = txt.split()
    print(tokens)
    print(txt)
    msg_number = int(tokens[1]) - 1  # fix ao index

    uid_queue = message_queue[uid_placeholder]

    if msg_number < len(uid_queue):
        message = uid_queue[msg_number]
        print(f"Message: {message}")

        # Marcar como lido
        msg_read = (message[0], message[1], True)
        message_queue[uid_placeholder][msg_number] = msg_read

        print("Message marked as read.")
        response = (f"Subject: {message[0]}\n"
                    f"Message: {message[1]}")
        return response.encode()

    else:
        print(f"Message number {msg_number + 1} doesnt exist, or its alread marked as read")
        response = f"Message number {msg_number + 1} doesnt exist, or its alread marked as read"

    return response.encode()

def handle_user_command(txt, message_queue):

    response = "User command received".encode()
    return response

def handle_help_command(txt, message_queue):
    help_text = """
    • -user <FNAME>
    • send <UID> <SUBJECT> 
    • askqueue 
    • getmsg <NUM>
    • help
    """
    response = help_text.encode()
    return response