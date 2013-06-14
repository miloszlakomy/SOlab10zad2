#ifndef HEADER_H
#define HEADER_H

void sysError(string str){
  perror(str.c_str());
  exit(1);
}

#define _STR(x) #x
#define STR(x) _STR(x)
#define sysError(msg) sysError(string( STR(__LINE__) ": " ) + msg)

#define NYI sysError("Not yet implemented.");

const size_t MAX_MSG_SIZE = 1024;

void interrupt(int signo)
{
        exit(0);
}

const int stdinFD = 0;

const string bialeZnaki = " \t\n\v\f\r";

const struct timeval tv_zero = {
  .tv_sec = 0,
  .tv_usec = 0
};

const struct timeval tv_piecSekund = {
  .tv_sec = 5,
  .tv_usec = 0
};

const int WIELKOSC_BACKLOGU = 100;

#endif