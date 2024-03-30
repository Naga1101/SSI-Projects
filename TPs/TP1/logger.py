import os
from datetime import datetime

session_start = None
session_end = None

def generate_log_file():
    global session_start

    session_start = datetime.now()
    session_date = session_start.strftime("%Y-%m-%d")
    session_file = "logs/sessions-" + session_date + ".log"

    # verificar se existe, senão apaga conteudos
    if not os.path.exists(session_file):
        file = open(session_file, "w")
        file.close()

    return session_file

def log_session_start(session_file):
    with open(session_file, "a+") as f:
        f.write("-----------------BEGIN-SESSION-----------------\n")
        f.write(f"Session start: {datetime.now()}\n\n")
        f.write(f"<DATETIME> -> <SENDER>\n")

def log_session_end(session_file):
    session_end = datetime.now()
    session_duration = session_end - session_start

    with open(session_file, "a+") as f:
        f.write(f"\nSession end: {datetime.now()}\n")
        f.write(f"Session duration: {session_duration}\n")
        f.write("------------------END-SESSION------------------\n\n")

def log_action(session_file, action, sender, target):
    with open(session_file, "a+") as f:
        if target is None:
            f.write(f"{datetime.now()} -> {sender} : {action}\n")
        else:
            f.write(f"{datetime.now()} -> {sender} : {action} -> {target}\n")

def log_invalid_action(session_file, action, sender):
    with open(session_file, "a+") as f:
        f.write(f"{datetime.now()} -> {sender} : Invalid Request -> {action}\n")
    
def log_invalid_command(session_file, sender):
    with open(session_file, "a+") as f:
        f.write(f"{datetime.now()} -> {sender} : Invalid Command\n")
