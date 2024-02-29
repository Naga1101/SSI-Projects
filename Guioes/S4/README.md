**Questão 2**

Q: Qual o impacto de se considerar um NONCE fixo (e.g. tudo 0)? Que implicações terá essa prática na segurança da cifra?

R: A utilização de um NONCE fixo levaria a sérias falhas de segurança, pois tornaria o texto cifrado previsível. Por exemplo, se dois textos diferentes utilizassem um NONCE fixo, estes ficariam idênticos, o que tornaria o texto bastante vulnerável a ataques.

**Questão 3**

Q: Qual o impacto de utilizar o programa chacha20_int_attck.py nos criptogramas produzidos pelos programas cfich_aes_cbc.py e cfich_aes_ctr.py? Comente/justifique a resposta.

R: 