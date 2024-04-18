# QUESTÃO: Q1

### Criar ficheiros, exercitar permissões e controlo de acesso

Criação do ficheiro:
- touch ssi_sem8

Permissões de dono/grupo de leitura escrita e execução, others leitura e execução:
- chmod u=rwx,g=rwx,o=rx ssi_sem8.txt

### Criar diretorias (contendo ficheiros), exercitar permissões e controlo de acesso

- mkdir sem8
- touch sem8/aula.txt
- chmod u=rwx,g=rx,o=rx sem8
- chmod u=rw,g=r,o=r sem8/aula.txt

# QUESTÃO Q2

### Criar utilizador

Adicionamos os 3 elementos do groupo
- sudo adduser rui2
- sudo adduser nuno
- sudo adduser gui
 
### Criar grupos contendo dois elementos da equipa e um contendo todos os elementos da equipa

Criamos um grupo com rui2 e nuno e outro com os 3 elementos da equipa

 - sudo groupadd grupoRN
 - sudo groupadd grupoALL

Adicionamos os membros aos respetivos grupos:
 - sudo usermod -a -G grupoAll rui2
 - sudo usermod -a -G grupoAll nuno
 - sudo usermod -a -G grupoAll gui

 - sudo usermod -a -G grupoRN rui2
 - sudo usermod -a -G grupoRN nuno

Finalmente iniciamos sessão com os utilizadores e verificamos o acesso aos ficheiros:
 - su - rui2

# QUESTÃO: Q3

- sudo chmod u+s print_file

De seguida utilizamos os ficheiros gerados previamente com os vários utilizadores e testamos o programa criado

# QUESTÃO: Q4

começamos por verificar a ACL do ficheiro criado previamente

 - getfacl ssi_sem8.txt

de seguida modificamos a ACL para rui2 poder ler e escrever, e do grupo "grupoAll", poder ler

setfacl -m u:rui2:rw ssi_sem8.txt
setfacl -m g:grupoAll:r sem8.txt