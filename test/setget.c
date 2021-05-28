//
// dataobject test
//
// setget.c
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../dataobject.h"

int setget()
{
  DATAOBJECT *dh = donew() ;
  dodump(dh, "Empty Structure") ;
  int pass=1 ;
  int thispass=0 ;

  dosetdata(dh, do_string, "/S/t/r/i/n/g/1", 14, "/%s/string1","node1") ;
  dosetdata(dh, do_string, "String2", 7, "/node1/string2") ;
  dosetdata(dh, do_string, "String3", 7, "/node2/string3") ;
  dosetdata(dh, do_string, "String4", 7, "/node3/string4") ;

  char *s23 = dogetdata(dh, do_string, NULL, "/node2/string3") ;
  char *s34 = dogetdata(dh, do_string, NULL, "/node3/string4") ;

  DATAOBJECT *d1h = donoden(dh,0) ;

  char *ss11 = dogetdata(donoden(dh,0), do_string, NULL, "/string1") ;
  char *ss12 = dogetdata(donoden(dh,0), do_string, NULL, "/string2") ;

  dodump(dh, "Data in Structure") ;
  dodump(d1h, "Node[0] contents") ;

  thispass = s23 && strcmp(s23, "String3")==0 ;
  printf("Testing %s==%s - %s\n", s23?s23:"?", "String3", thispass?"pass":"FAIL") ;
  pass &=thispass ;

  thispass = ss11 && strcmp(ss11, "/S/t/r/i/n/g/1")==0 ;
  printf("Testing %s==%s - %s\n", ss11?ss11:"?", "/S/t/r/i/n/g/1", thispass?"pass":"FAIL") ;
  pass &=thispass ;

  thispass = s34 && strcmp(s34, "String4")==0 ;
  printf("Testing %s==%s - %s\n", s34?s34:"?", "String4", thispass?"pass":"FAIL") ;
  pass &=thispass ;

  thispass = ss12 && strcmp(ss12, "String2")==0 ;
  printf("Testing %s==%s - %s\n", ss12?ss12:"?", "String2", thispass?"pass":"FAIL") ;
  pass &=thispass ;

  if (pass) {
    printf("Test Passed\n") ;
  } else {
    printf("Test FAILED\n") ;
  }
    
  dodelete(dh) ;

  exit(pass) ;
}
