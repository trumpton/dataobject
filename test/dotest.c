
#include <stdio.h>
#include <stdlib.h>


int parsejson() ;
int embeddedstring() ;
int setget() ;


int main()
{
  int testok=1 ;

  testok &= parsejson() ;
  testok &= embeddedstring() ;
  testok &= setget() ;

  if (testok) {

    printf("All tests passed\n") ;
    exit(0) ;

  } else {

    printf("Test failures encountered\n") ;
    exit(1) ;

  }

}


