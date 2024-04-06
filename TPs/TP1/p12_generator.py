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
from cryptography.x509.oid import ExtendedKeyUsageOID

def get_userdata(p12_fname):
    with open(p12_fname, "rb") as f:
        p12 = f.read()
    password = None 
    (private_key, user_cert, [ca_cert]) = load_key_and_certificates(p12, password)
    return private_key, user_cert, ca_cert

def generate_p12(private_key, user_cert, ca_cert, name, p12_fname, passphrase=None):

    if not isinstance(private_key, rsa.RSAPrivateKey):
        raise ValueError("Private key must be an RSA private key.")

    with open("projCA/" + p12_fname, "wb") as f:
        f.write(
            serialize_key_and_certificates(
                name.encode(), 
                private_key, 
                user_cert, 
                [ca_cert], 
                encryption_algorithm=NoEncryption())
                )

def gerar_ca_cert():
    ca_priv_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048
    )

    ca_subject = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, "PT"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Minho"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, "Braga"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Universidade do Minho"),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, "SSI MSG RELAY SERVICE"),
        x509.NameAttribute(NameOID.COMMON_NAME, "MSG RELAY SERVICE CA"),
        x509.NameAttribute(NameOID.PSEUDONYM, "MSG_CA")
    ])

    ca_cert = x509.CertificateBuilder(
        ).subject_name(
            ca_subject
        ).issuer_name(
            ca_subject
        ).public_key(
            ca_priv_key.public_key()
        ).serial_number(
            x509.random_serial_number()
        ).not_valid_before(
            datetime.utcnow()
        ).not_valid_after(
            datetime.utcnow() + timedelta(days=3650)
        ).add_extension( 
            x509.BasicConstraints(ca=False, path_length=None), critical=True,
        ).add_extension(
            x509.KeyUsage(digital_signature=True, content_commitment=True, key_encipherment=False, data_encipherment=False, key_agreement=False, key_cert_sign=False, crl_sign=False, encipher_only=False, decipher_only=False), critical=True,
        ).sign(
            private_key=ca_priv_key,
            algorithm=hashes.SHA256(),
        )
    
    generate_p12(ca_priv_key, ca_cert, ca_cert, "MSG_CA", "MSG_CA.p12")

def gerar_parametros_server_p12():
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
        backend=default_backend()
    )

    ca_key, ca_cert, _ = get_userdata("projCA/MSG_CA.p12")
    
    subject = issuer = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, u"PT"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u"Minho"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, u"Braga"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"Universidade do Minho"),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, u"SSI MSG RELAY SERVICE"),
        x509.NameAttribute(NameOID.COMMON_NAME, u"MSG_SERVER"),
        x509.NameAttribute(NameOID.PSEUDONYM, u"MSG_SERVER"),
    ])

    user_cert = x509.CertificateBuilder().subject_name(
        subject
    ).issuer_name(
        ca_cert.subject
    ).public_key(
        private_key.public_key()
    ).serial_number(
        x509.random_serial_number()
    ).not_valid_before(
        datetime.utcnow()
    ).not_valid_after(
        datetime.utcnow() + timedelta(days=3650)
    ).add_extension( 
        x509.BasicConstraints(ca=False, path_length=None), critical=True,
    ).add_extension(
        x509.KeyUsage(digital_signature=True, content_commitment=True, key_encipherment=False, data_encipherment=False, key_agreement=False, key_cert_sign=False, crl_sign=False, encipher_only=False, decipher_only=False), critical=True,
    ).add_extension(
        x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]),critical=False
    ).sign(
        ca_key, hashes.SHA256(), default_backend()
    )

    generate_p12(private_key, user_cert, ca_cert, "MSG_SERVER", "MSG_SERVER.p12")
        
def gerar_parametros_p12(nome):
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
        backend=default_backend()
    )

    ca_key, ca_cert, _ = get_userdata("projCA/MSG_CA.p12")
    
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
        ca_cert.subject
    ).public_key(
        private_key.public_key()
    ).serial_number(
        x509.random_serial_number()
    ).not_valid_before(
        datetime.utcnow()
    ).not_valid_after(
        datetime.utcnow() + timedelta(days=3650)
    ).add_extension( 
        x509.BasicConstraints(ca=False, path_length=None), critical=True,
    ).add_extension(
        x509.KeyUsage(digital_signature=True, content_commitment=True, key_encipherment=False, data_encipherment=False, key_agreement=False, key_cert_sign=False, crl_sign=False, encipher_only=False, decipher_only=False), critical=True,
    ).add_extension(
        x509.ExtendedKeyUsage([ExtendedKeyUsageOID.CLIENT_AUTH]),critical=False
    ).sign(
        ca_key, hashes.SHA256(), default_backend()
    )

    generate_p12(private_key, user_cert, ca_cert, nome, nome + ".p12")

# Gerar certificados do server e do ca 

# gerar_ca_cert()

# gerar_parametros_server_p12()

# _, ca_cert, _ = get_userdata("projCA/MSG_CA.p12")
# _, server_cert, ca_cert2 = get_userdata("projCA/MSG_SERVER.p12")

# print("Certificates generated!")
# print("CA certificate: ", ca_cert)
# print("Server certificate: ", server_cert)
# print("CA of server certificate: ", ca_cert2)