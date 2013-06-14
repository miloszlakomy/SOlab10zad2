#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <string>
#include <queue>
using namespace std;

#include "header.h"

const string commandsInfo = "Dostepne polecenia: \n"
                            "1 - odpytanie serwera o aktualna liste zarejestrowanych klientow,\n"
                            "2 identyfikatorKlientaKtoryMaWykonacPolecenie polecenie - przekazanie serwerowi polecenia do wykonania przez wybranego innego klienta (polecenie wykonywane przez popen a rezultat zwracany jest do klienta proszacego o wykonanie polecenia), pytajacy klient wyswietla otrzymane informacje na ekranie,\n"
                            "3 - wyslanie serwerowi informacji o zakonczeniu sesji i zakonczenie programu.";

int deskryptorSocketuKlienta = -1;
string sciezkaDoSocketuPerKlient = "";

void clean()
{
        if(sciezkaDoSocketuPerKlient.size())
          unlink(sciezkaDoSocketuPerKlient.c_str());
        if(-1 != deskryptorSocketuKlienta)
          close(deskryptorSocketuKlienta);
}

int main(int argc, char ** argv){
  
  if(3 != argc)
  sysError("Uzycie:\n"
           " ./klient INET adresIPSerwera:numerPortu\n"
           "lub\n"
           " ./klient UNIX sciezkaDoSocketuLokalnegoSerwera\n"
           "gdzie adresIPSerwera to adres IP dla socketu serwera z dziedziny Internet, numerPortu to numer portu dla socketu serwera z dziedziny Internet, sciezkaDoSocketuLokalnegoSerwera to sciezka w systemie plikow dla soketu serwera z dziedziny Unix.\n");
  
  string typ = argv[1];
  int numerPortu = -1;
  const char * adresIPSerwera = "";
  const char * sciezkaDoSocketuLokalnegoSerwera = "";
  
  if("INET" == typ){
    char * wskaznikNaDwukropek = strchr(argv[2], ':');
    if(!wskaznikNaDwukropek)
      sysError("Nieprawidlowy format adresu.");
    
    *wskaznikNaDwukropek = '\0';
    
    adresIPSerwera = argv[2];
    
    char * endptr;
    
    numerPortu = strtol(wskaznikNaDwukropek+1, &endptr, 0);
    if(*endptr || numerPortu < 0)
      sysError("Numer portu musi byc nieujemna liczba calkowita.");
  }
  else if("UNIX" == typ){
    sciezkaDoSocketuLokalnegoSerwera = argv[2];
    if(strlen(sciezkaDoSocketuLokalnegoSerwera) >= UNIX_PATH_MAX)
      sysError("Sciezka dla socketu z dziedziny UNIX musi miec dlugosc mniejsza niz UNIX_PATH_MAX (" STR(UNIX_PATH_MAX) ").");
  }
  else sysError("Pierwszy argument przyjmuje tylko wartosci: INET, UNIX.");
  
  /////

  if(atexit(clean)) sysError("atexit error");
  if(SIG_ERR == signal(SIGINT, interrupt)) sysError("signal error");
  
  char identyfikatorKlienta[MAX_MSG_SIZE-1];
  identyfikatorKlienta[MAX_MSG_SIZE-2] = '\0';
  if(-1 == gethostname(identyfikatorKlienta, (MAX_MSG_SIZE-3)/2)) sysError("gethostname error");
  if(strlen(identyfikatorKlienta) > (MAX_MSG_SIZE-3)/2)
    identyfikatorKlienta[(MAX_MSG_SIZE-3)/2] = '\0';
  size_t dlugoscNazwyDomeny = strlen(identyfikatorKlienta);
  identyfikatorKlienta[dlugoscNazwyDomeny] = '.';
  if(-1 == getdomainname(identyfikatorKlienta + dlugoscNazwyDomeny + 1, (MAX_MSG_SIZE-3)/2)) sysError("getdomainname error");
  
  identyfikatorKlienta[0] = getchar();
  getchar();
  
//   struct sockaddr * adresSocketuSerwera;
//   socklen_t rozmiarStrukturyAdresu;
  
  struct sockaddr_un adresSocketuLokalnegoSerwera = {0};
  struct sockaddr_in adresSocketuZdalnegoSerwera = {0};
  
  if("UNIX" == typ){
    deskryptorSocketuKlienta = socket(AF_UNIX, SOCK_STREAM, 0);
    if(-1 == deskryptorSocketuKlienta) sysError("socket error");
    
//     struct sockaddr_un me = {0};
//     me.sun_family = AF_UNIX;
//     sciezkaDoSocketuPerKlient = string("/tmp/") + identyfikatorKlienta;
//     strcpy(me.sun_path, sciezkaDoSocketuPerKlient.c_str());
//     if(-1 == bind(deskryptorSocketuKlienta, (struct sockaddr *) &me, sizeof(struct sockaddr_un)) ) sysError("bind error");
    
    
//     memset(&adresSocketuLokalnegoSerwera, 0, sizeof(struct sockaddr_un));
    adresSocketuLokalnegoSerwera.sun_family = AF_UNIX;
    strcpy(adresSocketuLokalnegoSerwera.sun_path, sciezkaDoSocketuLokalnegoSerwera);
    
    if(-1 == connect(deskryptorSocketuKlienta, (struct sockaddr *)&adresSocketuLokalnegoSerwera, sizeof(struct sockaddr_un))) sysError("connect error");
    
//     adresSocketuSerwera = (struct sockaddr *) &adresSocketuLokalnegoSerwera;
//     rozmiarStrukturyAdresu = sizeof(struct sockaddr_un);
  }
  else /*if("INET" == typ)*/{
    deskryptorSocketuKlienta = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == deskryptorSocketuKlienta) sysError("socket error");
    
    
//    memset(&adresSocketuZdalnegoSerwera, 0, sizeof(struct sockaddr_in));
    adresSocketuZdalnegoSerwera.sin_family = AF_INET;
    adresSocketuZdalnegoSerwera.sin_port = htons(numerPortu);
    if(0 == inet_aton(adresIPSerwera, &(adresSocketuZdalnegoSerwera.sin_addr))) sysError("inet_aton error");
    
    if(-1 == connect(deskryptorSocketuKlienta, (struct sockaddr *)&adresSocketuZdalnegoSerwera, sizeof(struct sockaddr_in))) sysError("connect error");
    
//     adresSocketuSerwera = (struct sockaddr *) &adresSocketuZdalnegoSerwera;
//     rozmiarStrukturyAdresu = sizeof(struct sockaddr_in);
  }
  
  string message;
    
  char buf[MAX_MSG_SIZE+1];
  buf[MAX_MSG_SIZE] = '\0';
  
//   socklen_t addrlen;
  
  message = string("0 ") + identyfikatorKlienta;
  
  if((unsigned)message.size()+1 != send(deskryptorSocketuKlienta, message.c_str(), message.size()+1, 0)) sysError("send error");
  
//   addrlen = rozmiarStrukturyAdresu;
  if(-1 == recv(deskryptorSocketuKlienta, (void *) &buf, MAX_MSG_SIZE, 0) ) sysError("recv error");
  
  if(' ' != buf[1]) sysError("Bledny format otrzymanej wiadomosci.");
  if('E' == buf[0]) sysError(&buf[2]);
  if('4' != buf[0]) sysError("Bledny format otrzymanej wiadomosci.");
  
  puts(buf+2);
  
  fd_set selectSet;
  queue<int> actionQueue;
  
  cout << commandsInfo << "\n\n> " << flush;
  
  for(;;){
    
    FD_ZERO(&selectSet);
    FD_SET(deskryptorSocketuKlienta, &selectSet);
    FD_SET(stdinFD, &selectSet);
    
    if(-1 == select(1+(deskryptorSocketuKlienta>stdinFD ? deskryptorSocketuKlienta : stdinFD),
                    &selectSet, NULL, NULL, NULL)) sysError("select error");
    
    if(FD_ISSET(deskryptorSocketuKlienta, &selectSet)) actionQueue.push(deskryptorSocketuKlienta);
    if(FD_ISSET(stdinFD, &selectSet)) actionQueue.push(stdinFD);
    
    while(actionQueue.size()){
      
      int fd = actionQueue.front();
      actionQueue.pop();
      
      if( stdinFD == fd ){
        string input;
        getline(cin, input);
        
        if("1" == input){ // listowanie klientow
          message = "1 ";
          
          if((unsigned)message.size()+1 != send(deskryptorSocketuKlienta, message.c_str(), message.size()+1, 0)) sysError("send error");
          
//           addrlen = rozmiarStrukturyAdresu;
          if(-1 == recv(deskryptorSocketuKlienta, (void *) &buf, MAX_MSG_SIZE, 0) ) sysError("recv error");
          
          if(' ' != buf[1] || '5' != buf[0]) sysError("Bledny format otrzymanej wiadomosci.");
          
          puts(buf+2);
        }
        else if("2 " == input.substr(0,2)){ // polecenie
          message = "2 " + input.substr(2);
          
          if((unsigned)message.size()+1 != send(deskryptorSocketuKlienta, message.c_str(), message.size()+1, 0)) sysError("send error");
          
//           addrlen = rozmiarStrukturyAdresu;
          if(-1 == recv(deskryptorSocketuKlienta, (void *) &buf, MAX_MSG_SIZE, 0) ) sysError("recv error");
          
          if(' ' != buf[1]) sysError("Bledny format otrzymanej wiadomosci.");
          if('E' == buf[0]) sysError(&buf[2]);
          if('7' != buf[0]) sysError("Bledny format otrzymanej wiadomosci.");
          
          puts(buf+2);
        }
        else if("3" == input){ // wylogowanie
          message = string("3 ") + identyfikatorKlienta;
          
          if((unsigned)message.size()+1 != send(deskryptorSocketuKlienta, message.c_str(), message.size()+1, 0)) sysError("send error");
          
          return 0;
        }
        else sysError("Nieprawidlowa opcja.\n" + commandsInfo);
        
        cout << "> " << flush;
      }
      else{
        if(-1 == recv(fd, (void *) &buf, MAX_MSG_SIZE, 0) ) sysError("recv error");
        
        if(' ' != buf[1] || '8' != buf[0]) sysError("Nieprawidlowy format wiadomosci.");
        
        FILE * popenFileHandler = popen(&buf[2], "r");
        if(NULL == popenFileHandler) sysError("popen error");
        
        strcpy(buf, "7 ");
        
        int i=1;
        do{
          ++i;
          buf[i] = getc(popenFileHandler);
        }while(EOF != buf[i] && i < MAX_MSG_SIZE);
        buf[i] = '\0';
        
        if(-1 == pclose(popenFileHandler)) sysError("pclose error");
        
        if((unsigned)i+1 != send(deskryptorSocketuKlienta, buf, i+1, 0)) sysError("send error");
      }
    }
  }
  
  /////
  
  return 0;

}























