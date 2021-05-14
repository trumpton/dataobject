//
// dataobject test
//
// setget.c
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../dataobject.h"

int main()
{
  char EXPECTED[] = "{\"grandparent\":\"{\\\"parent\\\":\\\"{\\\\\\\"child\\\\\\\":\\\\\\\"Message Data\\\\\\\"}\\\"}\"}" ;

  char src[] = "{\"child\":\"Message Data\"}" ;

  DATAOBJECT *dh = donew() ;
  DATAOBJECT *dhp = donew() ;
  int pass=1 ;

  dosetdata(dh, do_string, src, strlen(src), "/parent") ;
  char *pas = doasjson(dh, NULL) ;

  if (!pas) {
    printf("doasjson failed: %s\n", dojsonparsestrerror(dh)) ;
    pass=0 ;
  }

  else {
    dhp = donew() ;
    dosetdata(dhp, do_string, pas, strlen(pas), "/grandparent") ;
  }

  char *all = doasjson(dhp, NULL) ;

  if (!all) {
    printf("doasjsonfailed: %s\n", dojsonparsestrerror(dhp)) ;
    pass=0 ;
  } else {

    if (strcmp(all, EXPECTED)!=0) {
      printf("output not as expected: %s\n", all) ;
      pass=0 ;
    }
  }

  dodump(dhp, "Embedded String Object") ;
  printf("As JSON: %s\n\n", all) ;

  if (pass) {
    printf("Test Passed\n") ;
  } else {
    printf("Test FAILED\n") ;
  }
    
  dodelete(dh) ;
  dodelete(dhp) ;

  exit(pass) ;
}
