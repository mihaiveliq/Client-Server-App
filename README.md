# Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

Observatii generale:
    Dinamica programului este urmatoarea:
        Serverul este o platforma de intermediere intre clienti udp care sunt
        provideri de content pentru diferite topicuri la care se aboneaza
        clienti de tip tcp.
        Clientii de tip udp sunt implementati in schelet, iar cei tcp in
        fisierul subscriber.cpp, respectiv serverul in server.cpp.
    La inceput vom deschide doi socketi, unul de tip udp si unul de tip tcp.
    De asemenea vom creea un vector in care vom retine clientii tcp care
    se conecteaza. Un client de tip tcp retine un vector de topicuri la care
    este abonat, un vector de mesaje pe care le primeste cat este ofline si
    niste campuri ajutatoare.
    In cadrul serverului, se pot primi cereri pe socketul tcp pentru abonari
    la topicuri, pe cel udp pentru a primi content pe care il va trimite in
    clientii tcp destinati. Trimiterea se face sub forma unei structuri 
    formata din ip, port, un camp care verifica daca este iesirea si un 
    string parsat in client continand topicul, tipul mesajului si mesajul.
    De asemenea, se pot primi cereri de la tastatura pentru exit si de la
    clientii tcp de subscribe si unsubscribe.
    In fisierul client, la deschiderea socketului se trimite un mesaj cu
    id-ul acestuia si apoi se primesc mesajele corespunzatoare topicurilor 
    la care este abonat sub forma de structura mentionata inainte. De la
    tastatura se primesc cereri de tip exit, subscribe sau unsubscribe,
    tratate corespunzator.

Detalii:
    -Pentru a se trimite campurile structurilor in ordinea corecta au fost
    necesare casturi la (char *) si declarari ale structurilor de tipul
    <<struct __attribute__((packed)) "structura">>
    
