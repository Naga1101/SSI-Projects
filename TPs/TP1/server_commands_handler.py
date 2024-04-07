from cryptography.hazmat.primitives.serialization.pkcs12 import load_key_and_certificates
from datetime import datetime
from logger import *
from encrypt_decypt_handler import *
import asyncio
import bson

p12_file = "projCA/MSG_SERVER.p12"

send_target_format = {
    "publickey" : "",
    "certificate" : ""
}

def set_target(publickey, certificate):
    send_target_format["publickey"] = publickey
    send_target_format["certificate"] = certificate

    return bson.dumps(send_target_format)

def get_userdata(p12_fname, session_file):
    with open(p12_fname, "rb") as f:
        p12 = f.read()
    password = None
    (private_key, user_cert, [ca_cert]) = load_key_and_certificates(p12, password)
    return (private_key, user_cert, ca_cert)

def handle_askqueue_command(message_queue, sender, session_file):
    print("ASKQUEUE Command Received from", sender)

    # obtem a queue do target
    uid_queue = message_queue.get(sender, [])

    messages_list = [] # lista das mensagens a enviar

    if not uid_queue :
        response = "Empty"
        return response.encode()
    
    elif all(message[3] is True for message in uid_queue):
        response = "Empty"
        return response.encode()
    else:
        for message_n, message in enumerate(uid_queue, start=1):
            if not message[3]:

                msg_aesKey_pair, certificate = unpair(message[2])
                bson_message, aes_key = unpair(msg_aesKey_pair)

                message_dict = bson.loads(bson_message)
                subject = message_dict['subject']
                print("dict: ", message_dict)
                print("subject: ", subject)
                messages_list.append({
                    "number": message_n,
                    "sender": message[0],
                    "time": message[1],
                    "subject": subject
                })
        
        response = {"messages": messages_list}

    response_bson = bson.dumps(response)

    first = mkpair(response_bson, aes_key)
    second = mkpair(first, certificate)

    log_action(session_file, "ASKQUEUE", sender, None)

    return second

def handle_getmsg_command_bson(message_queue, sender, session_file, num):
    print("GETMSG Command received from", sender)

    msg_number = int(num) - 1  # fix ao index

    # obtem o queue do cliente
    uid_queue = message_queue.get(sender, [])

    if msg_number < len(uid_queue):
        
        # obtem a mensagem que foi requested
        message = uid_queue[msg_number]

        # Marcar como lido
        msg_read = (message[0], message[1], message[2], True)
        message_queue[sender][msg_number] = msg_read

        log_action(session_file, "GETMSG", sender, msg_number + 1)

        return message[2]

    else:
        print(f"Message number {msg_number + 1} does not exist.")
        log_invalid_action(session_file, "GETMSG", sender)
        response = "MSG RELAY SERVICE: unknown message!"

        # Encode the error message as BSON (You can also choose to handle errors differently)
        #error_message_dict = {
        #    "Error": response
        #}
        #response_bson = bson.dumps(error_message_dict)
        return response.encode()

async def handle_get_target_data_command(uids, message_queue, sender, session_file, target, reader, writer, shared_DHKey, algorythm_AES):
    print("SEND command receivied from", sender)

    # uid nao existente
    if target not in uids:
        writer.write("MSG RELAY SERVICE: uid not found!".encode())
        return -1
    else:
        certificate = uids[target]
        # print(uids)
    
    # enc the message and send it
    rp = process_send_message(certificate, shared_DHKey, algorythm_AES, p12_file)
    writer.write(rp)

    #wait for the message
    message = await reader.read(99999)

    peer_message, _ = process_received_message(message, shared_DHKey, algorythm_AES, p12_file)

    timestamp = datetime.datetime.now().timestamp()

    #register the message
    message_queue[target].append((sender, timestamp, peer_message, False))

    log_action(session_file, "SENDMSG", sender, target)

    # print(message_queue)