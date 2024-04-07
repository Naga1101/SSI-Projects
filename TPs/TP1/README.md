# Relatório TP1

Neste relatório abordamos a nossa implementação de um sistema de mensagens cliente-servidor, com enfâse na segurança das suas comunicações.

## Segurança / Decisões tomadas

Para a comunicação Cliente - Servidor ser protegida a ataques "Man-in-the-middle", utilizamos o algoritmo Diffie-Hellman para troca de chaves para utilizar chaves simétrica para criptografar a sua comunicação. 

Para garantir também a integridade das mensagens formam usados certificados e assinaturas para validar a identifidade e fonte das mensangens.

Para a encriptação das mensagens trocadas entre o Cliente - Servidor foi utilizado o algoritmo AES-GCM para garantir a confidenciabilidade das mensagens em conjunto com o HMAC para verificar a integridade e autenticidade da mensagem.

Para prevenir um servidor curioso, o conteúdo das mensagens é encriptado usando AES_GCM e assinado pelo cliente, no qual a chave usada é encriptada usando RSA com a chave pública do destinatário, esta chave encriptada irá acompanhar a mensagem para que apenas o destinatário possa visualizar o conteudo da mensagem.

## Handshake inicial

Este handshake inicial é utilizado para a troca de chaves entre cliente-servidor para garantir a segurança da comunicação entre os mesmos, neste handshake o cliente começa por realizar a geração dos parâmetros para o diffie-hellman e envia os mesmos para o servidor, depois ambos trocam as chaves públicas, certificados e assinaturas, pro fim fazem a verificação dos mesmos, se verificação for feita com sucesso, ambos calculam a sua chave partilhada.

Durante este processo o servidor também realiza um registo do cliente, usando o seu UID para criar uma entrada na queue de mensagens, e na lista de UIDs onde irá ser ocompanhado pelo seu certificado.

## Comandos

### Comando - -user <fname>

Este comando é reconhecido pelo cliente como uma flag, no caso de se chamar a função com a flag -user <fname> o ficheiro .p12 utilizado pelo cliente será o selecionado pelo mesmo, tendo o seu caminho sido fornecido no <fname>.


### Comando - -gen <fname>

Este comando é utilizado por um cliente, e gera lhe um ficheiro .p12 com uma chave privada, um certificado e com o issuer MSG_CA. Após a criação deste ficheiro o cliente poderá utilizá-lo em junção com o comando -user para executar os comandos que iremos explicar a seguir. Qualquer ficheiro criado será armazenado na pasta projCA.

### Comando - SEND

Este comando vem em conjunto com o uid do destinatário e o subject, e será depois pedido para inserir o conteudo da mensagem que não deve exceder os 1000 bytes, depois do utilizador fornecer toda a informação, este vai enviar uma mensagem "SEND `<DESTINATION>`" ao servidor, à qual o servidor irá responder com o certificado do destinatário da mensagem, ao receber o certificado, é verificado se o certificado é válido, depois uma chave AES é gerada para criptografar o conteudo da mensagem, após o conteudo ser criptografado com a chave AES, a chave é então criptografada com a chave publica do destinatário obtida no certifico recebido usando o RSA. Por fim mensagem criptografada, a chave AES criptografada, assinatura digital e o certificado do `<SENDER>` são enviadas ao servidor.

Ao receber a mensagem o servidor irá guardar os detalhes na queue do cliente a quem se destina a mensagem.

### Comando - ASKQUEUE

Com este comando o cliente pede ao servidor todas as suas mensagens não lidas enviando uma mensagem "ASKQUEUE", ao receber a mensagem e após validar os dados, o servidor procura na queue do cliente que enviou o request, todas as mensagens nao lidas, e envia para cada mensagem o seu número, sender, timestamp, e subject.

Quando o cliente recebe esta mensagem além de validar os dados do servidor, valida também o subject enviado, verificando a assinatura do SENDER através do certificado que vinha a companhar a mensagem, e de seguida desencriptando a chave que também acompanhava a mensagem com a sua chave privada, finalmente obtem o conteudo original da mensagem, e imprime todos os detalhes no stdout.

No caso de não ter mensagens por ler, é apresentada a mensagem "Your inbox is empty".

### Comando - GETMSG `<NUM>`

Este comando pede ao servidor uma mensagem, identificada por número, enviando GETMSG `<X>`, ao receber esta mensagem o servidor vai a queue buscar a mensagem requisitada, e envia a mesma ao cliente, ao receber esta mensagem e verificar que foi enviada pelo servidor, obtem o certificado do SENDER para verificar a assinatura da mensagem, e desencriptar a chave AES que acompanhava a mensagem utilizando a sua chave privada, usa essa mesma chave para obter a mensagem original e faz envia para o stdout.

### Comando - HELP

Este comando é responsavel por informar o utilizador dos diferentes comandos disponveis para o cliente, é também apresentado no caso do utilizador inserir um comando inválido.

## Funcionalidades adicionais

### Criação de certificados

De forma ao servidor trabalhar com os certificados criados por nós foi necessário fazer umas ligeiras alterações ao código do mesmo. Desta forma começamos por criar as funções necessárias para criar os ficheiros .p12, visto que para estes certificados serem válidos teríamos de ter a chave privada do issuer MSG_CA e apenas nos foi fornecido o seu certificado pelos docentes. Foi necessário criarmos também um ficheiro .p12 para o MSG_CA, para tal analisámos o conteúdo do certificado fornecido e criámos um idêntico que foi então inserido no ficheiro MSG_CA.p12. 
Devido a esta alteração foi necessário também criarmos um ficheiro .p12 para o servidor tendo o processo anterior para o ficheiro CA sido replicado para criar então o ficheiro MSG_SERVER.p12. Estes dois ficheiros serão então utilizados quer nas autenticações de qualquer certificado que venhamos a utilizar com um cliente(MSG_CA.p12) quer no funcionamento do servidor visto que este envés de utilizar o MSG_SERVER.p12 fornecido pelos docentes passa a utilizar o que nós geramos.
Desta forma os certificados que nos foram fornecidos pelos docentes deixam de poder ser utilizados visto que estes já não podem ser validados pelo novo MSG_CA. Nós tinhamos como plano implementar uma opção para escolher o tipo de certificados que iríamos utilizar ao correr o server mas não conseguimos implementar esta funcionalidade a tempo da entrega sendo que assim apenas os certificados presentes no ficheiro projCA podem ser utilizados.
Por fim ao gerar um ficheiro .p12 para um cliente este é gerado de forma semelhante ao do server mudando apenas uma das extensões na criação do mesmo. Estes ficheiros são criados no file p12_generator.py e as funções de criação do .p12 do CA e do .p12 do server apenas foram utilizadas na primeira criação destes ficheiros.

### Logs

Foram implementadas logs no servidor, as logs são armazenadas na pasta /logs e guardadas em ficheiros .log, é gerado um ficheiro para cada dia, e dentro do ficheiro são apresentadas as sessões realizadas no respetivo dia, o inicio das sessões sao indicadas por "BEGIN-SESSION" e pela data de inicialização, o fim das sessões é indicado por "END-SESSION", pela data de finalização e pela duração da sessão, durante a sessão é realizado um log por cada comando realizado pelo cliente, todos os logs são acompanhados pela data de realização e pelo nome do cliente que fez o request ao servidor, aparece também o comando utilizado, no caso de ser um comando SEND, aparecente também o destinatário do mesmo, e no caso de um comando GETMSG, a mensagem que o cliente requisitou.

# Trabalho Futuro

Para trabalho futuro gostariamos de melhorar a formatação das mensagens enviadas de modo a ser mais consistente e finalizar a implementação do modo que define o tipo de certificados que irão ser utilizados, podendo estes ser os certificados fornecidos pelos docentes ou os certificados que nós geramos.
