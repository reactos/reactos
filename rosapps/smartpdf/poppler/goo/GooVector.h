#ifndef _VECTOR_H
#define _VECTOR_H
#include "goo/gtypes.h"


template<class T>
class GooVector{
private:
   
   int _size;
   T*  last;
   T*  storage; 
 
   void resize(){
     if (_size==0) _size=2;else _size=2*_size;
      T *tmp=new T[_size];
     if (storage){
       last=copy(storage,last,tmp);
       delete [] storage;
      }
     else last=tmp; 
     storage=tmp;
    }

   T* copy(T* src1,T* src2,T* dest){
     T* tmp=src1;
     T* d=dest;
      while(tmp!=src2){
        *d=*tmp;
         d++;tmp++;
       }
      return d;
   }

public:
 typedef T* iterator;

 GooVector(){
  _size=0;
  last=0;
  storage=0;
}



virtual ~GooVector(){
  delete[] storage ;
}  

void reset(){
  last=storage;
}

int size(){
  return (last-storage);
}   
void push_back(const T& elem){
  if (!storage||(size() >=_size)) resize();
        *last=elem;
         last++;
  
     
} 


T pop_back() {
    if (last!=storage) last--;

    return *last;
} 


T operator[](unsigned int i){
 return *(storage+i);
}
  

GBool isEmpty() const{
 return !_size || (last==storage) ;
}



iterator begin() const{
 return storage;
}

iterator end() const {
  return last;
}
};
#endif



   
  
  



