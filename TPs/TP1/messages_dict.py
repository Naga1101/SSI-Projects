import bson

help_text = """
    • send <UID> <SUBJECT> 
    • askqueue 
    • getmsg <NUM>
    • help
    """

send_format = {
    "tipo": 1,
    "uid": "",
    "subject": "",
    "body": ""
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

    bson_send_data = bson.dumps(send_format)
    # print(bson_send_data)
    # print('\n', bson.loads(bson_send_data))  # resultado {'tipo': 1, 'send': 'send', 'uid': '1', 'subject': 'macaco', 'body': 'teste'}
    return bson_send_data

def askqueue_command():
    return bson.dumps(askqueue_format)

def getmsg_command(num):
    getmsg_format["num"] = num

    return bson.dumps(getmsg_format)