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
  char csrc[] = "{\"child\":\"Message Data\"}" ;

  DATAOBJECT *dhp = donew() ;
  DATAOBJECT *dhgp = donew() ;
  DATAOBJECT *dhn = donew() ;

  int pass=1 ;

  // Put child source data into parent dhp

  dosetdata(dhp, do_string, csrc, strlen(csrc), "/parent") ;

  // Extract parent data as json string 

  char *pas = doasjson(dhp, NULL) ;

  if (!pas) {

    printf("1. doasjson failed: %s\n", pas) ;
    pass=0 ;

  }

  else {

    // Put parent string into dhgp

    dosetdata(dhgp, do_string, pas, strlen(pas), "/grandparent") ;

  }

  char *all = doasjson(dhgp, NULL) ;
  printf("Generated JSON: %s\n", all) ;

  if (!all) {

    printf("2. doasjsonfailed: %s\n", dojsonparsestrerror(dhgp)) ;
    pass=0 ;

  } else {

    if (strcmp(all, EXPECTED)!=0) {
      printf("3. output not as expected\n") ;
      pass=0 ;
    }

    if (!dofromjson(dhn, all)) {

      printf("4. dofromjson failed: %s\n", dojsonparsestrerror(dhn)) ;
      pass=0 ;

    }

  }

  dodump(dhgp, "Embedded String Object") ;
  printf("As JSON: %s\n\n", all) ;

  
  if (pass) {
    printf("Test Passed\n") ;
  } else {
    printf("Test FAILED\n") ;
  }
    
  dodelete(dhp) ;
  dodelete(dhgp) ;
  dodelete(dhn) ;

  exit(pass) ;
}
