from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives.serialization.pkcs12 import serialize_key_and_certificates
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.serialization.pkcs12 import load_key_and_certificates
from datetime import datetime, timedelta
from cryptography.x509 import load_pem_x509_certificate
from cryptography.hazmat.primitives.serialization import NoEncryption


# def get_userdata(p12_fname):
#     with open(p12_fname, "rb") as f:
#         p12 = f.read()
#     password = None 
#     (private_key, user_cert, [ca_cert]) = load_key_and_certificates(p12, password)
#     return private_key, user_cert, ca_cert


def generate_p12(private_key, user_cert, ca_cert, name, p12_fname, passphrase=None):

    if not isinstance(private_key, rsa.RSAPrivateKey):
        raise ValueError("Private key must be an RSA private key.")

    with open(p12_fname, "wb") as f:
        f.write(
            serialize_key_and_certificates(
                name.encode(), 
                private_key, 
                user_cert, 
                [ca_cert], 
                encryption_algorithm=NoEncryption())
                )
        
def gerar_parametros_p12(nome):
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
        backend=default_backend()
    )

    subject = issuer = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, u"PT"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u"Minho"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, u"Braga"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"Universidade do Minho"),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, u"SSI MSG RELAY SERVICE"),
        x509.NameAttribute(NameOID.COMMON_NAME, nome),
        x509.NameAttribute(NameOID.PSEUDONYM, nome),
    ])

    user_cert = x509.CertificateBuilder().subject_name(
        subject
    ).issuer_name(
        issuer
    ).public_key(
        private_key.public_key()
    ).serial_number(
        x509.random_serial_number()
    ).not_valid_before(
        datetime.utcnow()
    ).not_valid_after(
        datetime.utcnow() + timedelta(days=3650)
    ).sign(
        private_key, hashes.SHA256(), default_backend()
    )

    with open("MSG_CA.crt", "rb") as ca_file:
        ca_cert_bytes = ca_file.read()

    ca_cert = load_pem_x509_certificate(ca_cert_bytes, default_backend())

    generate_p12(private_key, user_cert, ca_cert, nome, nome + ".p12")


# gerar_parametros_p12("amanhã")
# priv, u_cert, ca_cert = get_userdata("teste.p12")

# print("Generated p12")
# print("private key: ", priv)
# print("user cert: ", u_cert)
# print("ca cert: ", ca_cert)

# parsed_cert = x509.load_pem_x509_certificate(u_cert.public_bytes(serialization.Encoding.PEM), default_backend())

# print("user Pseudonym: ", u_cert.subject.get_attributes_for_oid(x509.NameOID.PSEUDONYM)[0].value)