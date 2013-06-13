#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <queue>
#include <algorithm>
using namespace std;

#include "header.h"

int deskryptorSocketuAkceptoraZdalnego = -1;
int deskryptorSocketuAkceptoraLokalnego = -1;
const char * sciezkaDoSocketuLokalnego = "";
pair<pthread_mutex_t, vector<int> > deskryptorySocketowPerKlient;

void clean()
{
        if(*sciezkaDoSocketuLokalnego)
          unlink(sciezkaDoSocketuLokalnego);
        if(-1 != deskryptorSocketuAkceptoraLokalnego)
          close(deskryptorSocketuAkceptoraLokalnego);
        if(-1 != deskryptorSocketuAkceptoraZdalnego)
          close(deskryptorSocketuAkceptoraZdalnego);
        for(int i=0;i<deskryptorySocketowPerKlient.second.size();++i)
          if(-1 != deskryptorySocketowPerKlient.second[i])
            close(deskryptorySocketowPerKlient.second[i]);
}

pair<pthread_mutex_t, map<int, pthread_mutex_t> > mutexyPerKlient;
pair<pthread_mutex_t, map<string, int> > klienci;
pair<pthread_mutex_t, map<int, bool> > dostepnoscSocketuPerKlient;
pair<pthread_mutex_t, map<int, queue<pair<string, int> > > > kolejkaPolecenINadawcowPerKlient;
pair<pthread_mutex_t, map<int, string> > wynikPoleceniaPerKlient;

void * watekPerKlient(void* _arg){
  
  char buf[MAX_MSG_SIZE+1];
  buf[MAX_MSG_SIZE] = '\0';
  char buforNaWynikiPolecen[MAX_MSG_SIZE+1];
  buforNaWynikiPolecen[MAX_MSG_SIZE] = '\0';
  
  int deskryptorSocketuPerKlient = *(int *)_arg;
  
  pthread_mutex_t * adresMutexuPerKlient;
  
  pthread_mutex_lock(&mutexyPerKlient.first);
  {
    adresMutexuPerKlient = &mutexyPerKlient.second[deskryptorSocketuPerKlient];
  }
  pthread_mutex_unlock(&mutexyPerKlient.first);
  
  if(recv(deskryptorSocketuPerKlient, &buf, MAX_MSG_SIZE, 0) == -1) sysError("recv error");
  
  cout << deskryptorSocketuPerKlient << ": " << buf << endl;
  
  if(' ' != buf[1] || '0' != buf[0]) sysError("Nieprawidlowy format wiadomosci.");
  
  string message;
  
  pthread_mutex_lock(&klienci.first);
  {
    if(klienci.second.end() == klienci.second.find(&buf[2])){
      klienci.second.insert(make_pair(&buf[2], deskryptorSocketuPerKlient));
      
      message = "4 ";
      for(map<string, int>::iterator it = klienci.second.begin();klienci.second.end() != it;++it)
        message += it->first + "\n";
    }
    else{
      message = "E Proba ponownego logowania z tym samym identyfikatorem.";
    }
  }
  pthread_mutex_unlock(&klienci.first);
  
  if((unsigned)message.size()+1 != send(deskryptorSocketuPerKlient, message.c_str(), message.size()+1, 0)) sysError("sendto error");
  
  pthread_mutex_lock(&wynikPoleceniaPerKlient.first); {
    wynikPoleceniaPerKlient.second[deskryptorSocketuPerKlient] = "";
  } pthread_mutex_unlock(&wynikPoleceniaPerKlient.first);
  
  for(;;){
    
    pthread_mutex_lock(&dostepnoscSocketuPerKlient.first); {
      dostepnoscSocketuPerKlient.second[deskryptorSocketuPerKlient] = false;
    } pthread_mutex_unlock(&dostepnoscSocketuPerKlient.first);
    
    pthread_mutex_lock(adresMutexuPerKlient);
    
    bool dostepnoscSocketu;
    
    pthread_mutex_lock(&dostepnoscSocketuPerKlient.first); {
      dostepnoscSocketu = dostepnoscSocketuPerKlient.second[deskryptorSocketuPerKlient];
    } pthread_mutex_unlock(&dostepnoscSocketuPerKlient.first);
    
    if(dostepnoscSocketu){
      
      if(-1 == recv(deskryptorSocketuPerKlient, (void *) &buf, MAX_MSG_SIZE, 0)) sysError("recv error");
      
      cout << deskryptorSocketuPerKlient << ": " << buf << endl;
      
      if(' ' != buf[1]) sysError("Nieprawidlowy format wiadomosci.");
    
      if('1' == buf[0]){ // listowanie klientow
        message = "5 ";
        
        pthread_mutex_lock(&klienci.first); {
          for(map<string, int>::iterator it = klienci.second.begin(); klienci.second.end() != it; ++it)
            message += it->first + "\n";
        } pthread_mutex_unlock(&klienci.first);
        
        
        if((unsigned)message.size()+1 != send(deskryptorSocketuPerKlient, message.c_str(), message.size()+1, 0)) sysError("send error");
      }
      else if('2' == buf[0]){ // polecenie
        NYI
  //     
  //     string message;
  //     bool zepsutePolaczenie = false;
  //     
  //     string dummy = &buf[2];
  //     int indeksPierwszegoBialegoZnaku = dummy.find_first_of(bialeZnaki);
  //     if(string::npos == indeksPierwszegoBialegoZnaku)
  //       message = "Nieprawidlowy format wiadomosci.";
  //     else{
  //       string identyfikatorKlientaKtoryMaWykonacPolecenie = dummy.substr(0, indeksPierwszegoBialegoZnaku);
  //       string polecenie = "8 " + dummy.substr(indeksPierwszegoBialegoZnaku+1);
  //       
  //       if(klienci.end() != klienci.find(identyfikatorKlientaKtoryMaWykonacPolecenie)){
  //         
  //         if(UNIX == klienci.find(identyfikatorKlientaKtoryMaWykonacPolecenie)->second.type){
  //           
  // //             cout << endl << "!!!" << endl << (AF_UNIX == klienci.find(identyfikatorKlientaKtoryMaWykonacPolecenie)->second.val_un.sun_family) << endl << klienci.find(identyfikatorKlientaKtoryMaWykonacPolecenie)->second.val_un.sun_path << endl << "!!!" << endl;
  //           
  //           if((unsigned)polecenie.size()+1 != sendto(deskryptorSocketuAkceptoraLokalnego, polecenie.c_str(), polecenie.size()+1, 0, (const struct sockaddr *) &klienci.find(identyfikatorKlientaKtoryMaWykonacPolecenie)->second.val_un, sizeof(struct sockaddr_un))) sysError("sendto error");
  //         }
  //         else{
  //           if((unsigned)polecenie.size()+1 != sendto(deskryptorSocketuAkceptoraZdalnego, polecenie.c_str(), polecenie.size()+1, 0, (const struct sockaddr *) &klienci.find(identyfikatorKlientaKtoryMaWykonacPolecenie)->second.val_in, sizeof(struct sockaddr_in))) sysError("sendto error");
  //         }
  //         
  //         setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv_piecSekund,sizeof(struct timeval));
  //         
  //         errno = 0;
  //         if(-1 == recv(
  //           (UNIX == klienci.find(identyfikatorKlientaKtoryMaWykonacPolecenie)->second.type) ?
  //           deskryptorSocketuAkceptoraLokalnego :
  //           deskryptorSocketuAkceptoraZdalnego
  //           , (void *) &buforNaWynikiPolecen, MAX_MSG_SIZE, 0)){
  //           if(EINTR == errno || EWOULDBLOCK == errno){
  //             zepsutePolaczenie = true;
  //             klienci.erase(identyfikatorKlientaKtoryMaWykonacPolecenie);
  //           }
  //           else
  //             sysError("recv error");
  //         }
  //         
  //         setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv_zero,sizeof(struct timeval));
  //         
  //         if(!zepsutePolaczenie){
  //           if(' ' != buforNaWynikiPolecen[1] || '7' != buforNaWynikiPolecen[0]) sysError("Bledny format otrzymanej wiadomosci.");
  //           
  //           message = buforNaWynikiPolecen;
  //         }
  //       }
  //       else{
  //         message = "E Ten klient nie jest zalogowany.";
  //       }
  //       
  //     }
  //     
  //     if(!zepsutePolaczenie &&
  //        (unsigned)message.size()+1 != sendto(fd, message.c_str(), message.size()+1, 0, 
  //                                             ((fd == deskryptorSocketuAkceptoraLokalnego) ? (const struct sockaddr *) &adresSocketuLokalnego : (const struct sockaddr *) &adresSocketuZdalnego),
  //                                             addrlen)) sysError("sendto error");
      }
      else if('3' == buf[0]){ // wylogowanie
        
        pthread_mutex_lock(&deskryptorySocketowPerKlient.first); {
          vector<int>::iterator it  = find (deskryptorySocketowPerKlient.second.begin(), deskryptorySocketowPerKlient.second.end(), deskryptorSocketuPerKlient);
          if(deskryptorySocketowPerKlient.second.end() == it) sysError("Niespojnosc danych (brak deskryptora skojarzonego z konczacym sie watkiem)!");
          *it = -1;
        } pthread_mutex_unlock(&deskryptorySocketowPerKlient.first);
        
        pthread_mutex_lock(&klienci.first); {
          map<string, int>::iterator it;
          for(it = klienci.second.begin(); klienci.second.end() != it && it->second != deskryptorSocketuPerKlient; ++it);
          
          if(klienci.second.end() == it) sysError("Niespojnosc danych (brak identyfikatora skojarzonego z konczacym sie watkiem)!");
          else
            klienci.second.erase(it);
        } pthread_mutex_unlock(&klienci.first);
        
        pthread_mutex_lock(&mutexyPerKlient.first); {
          mutexyPerKlient.second.erase(deskryptorSocketuPerKlient);
        } pthread_mutex_unlock(&mutexyPerKlient.first);
        
        pthread_mutex_lock(&dostepnoscSocketuPerKlient.first); {
          dostepnoscSocketuPerKlient.second.erase(deskryptorSocketuPerKlient);
        } pthread_mutex_unlock(&dostepnoscSocketuPerKlient.first);
        
        pthread_mutex_lock(&kolejkaPolecenINadawcowPerKlient.first); {
          kolejkaPolecenINadawcowPerKlient.second.erase(deskryptorSocketuPerKlient);
        } pthread_mutex_unlock(&kolejkaPolecenINadawcowPerKlient.first);
        
        pthread_mutex_lock(&wynikPoleceniaPerKlient.first); {
          wynikPoleceniaPerKlient.second.erase(deskryptorSocketuPerKlient);
        } pthread_mutex_unlock(&wynikPoleceniaPerKlient.first);
        
        cout << deskryptorSocketuPerKlient << ": Wylogowany." << endl;
        
        return 0;
      }
      else sysError("Nieprawidlowy format wiadomosci.");
    }
    
    
    pair<string, int> polecenieINadawca = make_pair("", -1);
    
    pthread_mutex_lock(&kolejkaPolecenINadawcowPerKlient.first);
    {
      if(kolejkaPolecenINadawcowPerKlient.second[deskryptorSocketuPerKlient].size()){
        polecenieINadawca = kolejkaPolecenINadawcowPerKlient.second[deskryptorSocketuPerKlient].front();
        kolejkaPolecenINadawcowPerKlient.second[deskryptorSocketuPerKlient].pop();
      }
    }
    pthread_mutex_unlock(&kolejkaPolecenINadawcowPerKlient.first);
    
    if(-1 != polecenieINadawca.second){
      NYI
    }
  }
  
  
  return 0;
}

int main(int argc, char ** argv){
  
  if(3 != argc)
  sysError("Uzycie: ./serwer numerPortu sciezkaDoSocketuLokalnego\n"
           "gdzie numerPortu to numer portu dla socketu z dziedziny Internet, sciezkaDoSocketuLokalnego to sciezka w systemie plikow dla soketu z dziedziny Unix\n");

  char * endptr;

  int numerPortu = strtol(argv[1], &endptr, 0);
  if(*endptr || numerPortu < 0)
    sysError("Numer portu musi byc nieujemna liczba calkowita.");
  
  sciezkaDoSocketuLokalnego = argv[2];
  if(strlen(sciezkaDoSocketuLokalnego) >= UNIX_PATH_MAX)
    sysError("Sciezka dla socketu z dziedziny UNIX musi miec dlugosc mniejsza niz UNIX_PATH_MAX (" STR(UNIX_PATH_MAX) ").");
  
  /////

  if(atexit(clean)) sysError("atexit error");
  if(SIG_ERR == signal(SIGINT, interrupt)) sysError("signal error");
  
  
  
  deskryptorSocketuAkceptoraLokalnego = socket(AF_UNIX, SOCK_STREAM, 0);
  if(-1 == deskryptorSocketuAkceptoraLokalnego) sysError("socket error");
  
  struct sockaddr_un adresSocketuLokalnego = {0};
//   memset(&adresSocketuLokalnego, 0, sizeof(struct sockaddr_un));
  adresSocketuLokalnego.sun_family = AF_UNIX;
  strcpy(adresSocketuLokalnego.sun_path, sciezkaDoSocketuLokalnego);
  
  if(-1 == bind(deskryptorSocketuAkceptoraLokalnego, (struct sockaddr *) &adresSocketuLokalnego, sizeof(struct sockaddr_un)) ) sysError("bind error");
  
  if(-1 == listen(deskryptorSocketuAkceptoraLokalnego, WIELKOSC_BACKLOGU)) sysError("listen error");
  
  
  
  deskryptorSocketuAkceptoraZdalnego = socket(AF_INET, SOCK_STREAM, 0);
  if(-1 == deskryptorSocketuAkceptoraZdalnego) sysError("socket error");
  
  struct sockaddr_in adresSocketuZdalnego = {0};
//   memset(&adresSocketuZdalnego, 0, sizeof(struct sockaddr_in));
  adresSocketuZdalnego.sin_family = AF_INET;
  adresSocketuZdalnego.sin_port = htons(numerPortu);
//   if(0 == inet_aton("127.0.0.1", &(adresSocketuZdalnego.sin_addr))) sysError("inet_aton error");
  adresSocketuZdalnego.sin_addr.s_addr = INADDR_ANY;

  if(-1 == bind(deskryptorSocketuAkceptoraZdalnego, (struct sockaddr *) &adresSocketuZdalnego, sizeof(struct sockaddr_in))) sysError("bind error");
  
  if(-1 == listen(deskryptorSocketuAkceptoraZdalnego, WIELKOSC_BACKLOGU)) sysError("listen error");
  
  
  
  if(pthread_mutex_init(&deskryptorySocketowPerKlient.first, NULL)) sysError("pthread_mutex_init error");
  if(pthread_mutex_init(&mutexyPerKlient.first, NULL)) sysError("pthread_mutex_init error");
  if(pthread_mutex_init(&klienci.first, NULL)) sysError("pthread_mutex_init error");
  if(pthread_mutex_init(&dostepnoscSocketuPerKlient.first, NULL)) sysError("pthread_mutex_init error");
  if(pthread_mutex_init(&kolejkaPolecenINadawcowPerKlient.first, NULL)) sysError("pthread_mutex_init error");
  if(pthread_mutex_init(&wynikPoleceniaPerKlient.first, NULL)) sysError("pthread_mutex_init error");
  
  
  
  fd_set selectSet;
  queue<int> acceptorActionQueue;
  
  for(;;){
    
    FD_ZERO(&selectSet);
    FD_SET(deskryptorSocketuAkceptoraLokalnego, &selectSet);
    FD_SET(deskryptorSocketuAkceptoraZdalnego, &selectSet);
    
    int nfds_minus1 = max(deskryptorSocketuAkceptoraLokalnego, deskryptorSocketuAkceptoraZdalnego);
    
    pthread_mutex_lock(&deskryptorySocketowPerKlient.first); {
      for(int i=0;i<deskryptorySocketowPerKlient.second.size();++i){
        FD_SET(deskryptorySocketowPerKlient.second[i], &selectSet);
        nfds_minus1 = max(nfds_minus1, deskryptorySocketowPerKlient.second[i]);
      }
    } pthread_mutex_unlock(&deskryptorySocketowPerKlient.first);
    
    if(-1 == select(1+nfds_minus1, &selectSet, NULL, NULL, NULL)) sysError("select error");
    
    if(FD_ISSET(deskryptorSocketuAkceptoraLokalnego, &selectSet)) acceptorActionQueue.push(deskryptorSocketuAkceptoraLokalnego);
    if(FD_ISSET(deskryptorSocketuAkceptoraZdalnego, &selectSet)) acceptorActionQueue.push(deskryptorSocketuAkceptoraZdalnego);
    
    pthread_mutex_lock(&deskryptorySocketowPerKlient.first); {
      for(int i=0;i<deskryptorySocketowPerKlient.second.size();++i)
        if(FD_ISSET(deskryptorySocketowPerKlient.second[i], &selectSet))
          acceptorActionQueue.push(deskryptorySocketowPerKlient.second[i]);
    } pthread_mutex_unlock(&deskryptorySocketowPerKlient.first);
    
    while(acceptorActionQueue.size()){
      
      int fd = acceptorActionQueue.front();
      acceptorActionQueue.pop();
      
      if(fd == deskryptorSocketuAkceptoraLokalnego || fd == deskryptorSocketuAkceptoraZdalnego){
        
        int nowyDeskryptorSocketuKlienta =  accept(fd, NULL, NULL);
        if(-1 == nowyDeskryptorSocketuKlienta) sysError("accept error");
        
        pthread_mutex_lock(&deskryptorySocketowPerKlient.first); {
          deskryptorySocketowPerKlient.second.push_back(nowyDeskryptorSocketuKlienta);
        } pthread_mutex_unlock(&deskryptorySocketowPerKlient.first);
        
        
        pthread_mutex_lock(&mutexyPerKlient.first);
        {
          if(pthread_mutex_init(&mutexyPerKlient.second[nowyDeskryptorSocketuKlienta], NULL)) sysError("pthread_mutex_init error");
          pthread_mutex_lock(&mutexyPerKlient.second[nowyDeskryptorSocketuKlienta]);
        }
        pthread_mutex_unlock(&mutexyPerKlient.first);
        
        
        pthread_mutex_lock(&dostepnoscSocketuPerKlient.first); {
          dostepnoscSocketuPerKlient.second[nowyDeskryptorSocketuKlienta] = false;
        } pthread_mutex_unlock(&dostepnoscSocketuPerKlient.first);
        
        
        cout << "Polaczono z klientem o deskryptorze " << nowyDeskryptorSocketuKlienta << endl;
        
        pthread_t dummy;
        if(pthread_create(&dummy, NULL, watekPerKlient, (void*)&deskryptorySocketowPerKlient.second.back())) sysError("pthread_create error");
        
      }
      else{
        
        pthread_mutex_lock(&dostepnoscSocketuPerKlient.first);
        {
          dostepnoscSocketuPerKlient.second[fd] = true;
        }
        pthread_mutex_unlock(&dostepnoscSocketuPerKlient.first);
        
        pthread_mutex_unlock(&mutexyPerKlient.second[fd]);
        
      }
    }
    
  }
  
  /////
  
  return 0;

}























