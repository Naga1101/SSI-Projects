import bson
from encrypt_decypt_handler import *

help_text = """
    • send <UID> <SUBJECT> : Send a message to another person, pseudonym needed
    • askqueue : Get a list of queued mesages from the user
    • getmsg <NUM> : Get the contents of a specific message from the user
    • -gen <fname> : Generate a new .p12 file with the requested name 
    • help : Shows all the command possible
    """

send_format = {
    "tipo": 1,
    "uid": "",
    "subject": "",
    "body": ""
}

get_target_info = {
    "tipo": 0,
    "target": ""
}

askqueue_format = {
    "tipo": 2,
}

getmsg_format = {
    "tipo": 3,
    "num": ""
}

def help_command():
    return help_text

def invalid_commad():
    response = """\nMSG RELAY SERVICE: command error!\n\n"""
    
    return response + help_text

def send_header_handdler(uid, subject):
    send_format["uid"] = uid
    send_format["subject"] = subject  
    # print(send_format)

def send_add_body(body):
    send_format["body"] = body


def get_send_message(aes_key, private_key):
    # primeiro da encode a cada elemento
    for key in ["uid", "subject", "body"]:
            send_format[key] = encode_client_message(send_format[key].encode(), aes_key)
            #print("send_format[key]", send_format[key])

    # assina cada elemento da mensagem
    for key in ["uid", "subject", "body"]:
        send_format[key] = sign_message(send_format[key], private_key)
        #print("send_format[key]", send_format[key])

    bson_send_data = bson.dumps(send_format)
    return bson_send_data

def get_send_message2():
    bson_send_data = bson.dumps(send_format)
    return bson_send_data

def set_target(target):
    get_target_info["target"] = target

def get_target_name():
    return get_target_info["target"]

def get_target():
    return bson.dumps(get_target_info)


def askqueue_command():
    return bson.dumps(askqueue_format)

def getmsg_command(num):
    getmsg_format["num"] = num

    return bson.dumps(getmsg_format)