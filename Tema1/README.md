# Dataplane Router
***

<br/>

## *Scopul lucrarii*
---

> Scopul acestei teme este de a simula modul in care un Router primeste, actualizeaza si redirectioneaza pachete de tip IP, ARP sau ICMP

<br/>

## *Modalitate de implementare*
---
> Pentru simplificarea implementarii sunt definite doua structuri de date auxiliare (in plus fata de scheletul temei):
 1. struct **ip_pack** - care contine:
    * **void \*buf** - pachetul IP pus in coada de asteptare
    * **uint32_t next_ip** - adresa ip pentru next hop
    * **int len** - lungimea pachetului
    * **int interface** - interfata pe care urmeaza sa fie trimis

<br/>

### **Trie**
---
> Trie este un arbore binar ce are rolul de a optimiza cautarea in tabela de rutare, reducand complexitatea de la O(n) la O(1).

Arborele a fost definit folosind urmatoarea structura:

- struct **node** care contine:
    * **struct node \*p** - parintele nodului
    * **struct node \*l** - copilul stang al nodului
    * **struct node \*r** - copilul drept al nodului
    * **struct route_table_entry \*entry** - intrarea in tabela de rutare caracteristic nodului

In fisierul trie.c se gaseste structura de mai sus impreuna cu urmatoarele functii:

1. *void* **add_entry_in_tree**(*struct route_table_entry* \*e, *struct node* \*t) - aceasta primeste ca parametrii intrarea e si radacina arborelui t. Functia verifica daca daca masca este mai lunga decat prefixul si in caz afirmativ, parcurge arborele folosindu-se de bitii prefixului pentru a alege directia. In cazul in care un nod exista, se realizeaza doar deplasarea in arbore. In caz contrar, un nou nod este creeat. Dupa un numar de pasi egal cu dimensiunea mastii, in nodul curent este retiunta intrarea din tabela de rutare

2. *struct node \** **create_tree**(*struct route_table_entry*  \*rtable, *int*  rtable_len) - aceasta functie parcurge intreaga tabel de rutare si apeleaza functia *add_entry_in_tree*, creeand astfel arborele

3. *struct route_table_entry \** **get_best_route**(*uint32_t* ip_dest, *struct node* \*t) - parcurge arborele t in functie de bitii adresei pi a destinatiei. In momentul cand urmatorul nod indicat este null, este returnat entry ul nodului curent

<br/>

### **IP & ICMP**
---

>In momentul in care a fost primit un pachet IP sau ICMP, este verificat si actualizat checksum-ul. In cazul in care acesta da eroare, routerul va da drop la pachet.

>In cazul in care ttl a ajuns la 0 sau nu exista niciun drum care sa duca pachetul spre destinatie, se va da drop la pachetul primit si se va creea un pachet icmp ce va fi trimis inapoi la sursa pentru a semnala eroarea. In plus, daca a fost primit un pachet de tipul ICMP request si routerul constata ca este pentru el, acesta va raspunde trimitand inapoi un pachet de tipul ICMP response.

Creearea pachetelor ICMP este facuta cu ajutorul functiei:
* *void* **create_icmp**(*void* \*buf, *size_t* \*len, *int* type, *int* code) - aceasta are rolul de a completa campurile necesare in alcatuirea unui pachet ICMP (inversand sursa cu destinatia, adaugand tipul si codul plus alte campuri, actualizand checksum ul si copiind headerul IP al mesajului primit impreuna cu 64 de bytes de informatie la final)

>In cazul in care pachetul IP trebuie doar trimis mai departe, se verifica daca este cunoscuta adresa mac a urmatorului hop. In cazul in care aceasta apare in tabela arp, adresele mac sursa si destinatie sunt actualizate iar pachetul este trimis mai departe. In cazul in care adresa mac a destinatiei nu este cunoscuta, pachetul este introdul in coada de asteptare (impreuna cu ip-ul urmatorului hop, interfata de plecare si lungimea sa), iar routerul va trimite un arp request.

<br/>

### **ARP**
---