//
// dataobject test
//
// parsejson.c
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../dataobject.h"

int check(char *section, DATAOBJECT *dh, char *var, enum dataobject_type type, long int ival, double fval) 
{

  if (ival!=0) {
    long int i=0 ;
    if (!dogetsint(dh, do_sint64, &i, var) || ival!=i) { 
      printf("%s %s value incorrect (%ld != %ld)\n", section, var, i, ival) ;
      return 0 ;
    } else {
      printf("%s %s value OK (value = %ld)\n", section, var, i) ;
      return 1 ;
    }
  }

  if (fval!=0) {
    double f=0 ;
    if (!dogetreal(dh, do_double, &f, var) || fval!=f) { 
      printf("%s %s value incorrect (%f != %f)\n", section, var, f, fval) ;
      return 0 ;
    } else {
      printf("%s %s value OK (value = %f)\n", section, var, f) ;
      return 1 ;
    }
  }

  if (type != dogettype(dh, var)) {
    printf("%s %s type incorrect\n", section, var) ;
    return 0 ;
  } else {
    printf("%s %s type OK\n", section, var) ;
    return 1 ;
  }

}




int parsejson()
{
  DATAOBJECT *dh = donew() ;
  int ttest=0 ;
  int tpass=0 ;
  int thispass=0 ;
  long int i ;
  double f ;


/************************************************************************/

  printf("1. TESTING PARSING FROM JSON\n") ;
  thispass=0 ;

  char j1[] = "{\n"
              "  \"string\": \"abcdefg\",\n"
              "  \"int1\": 1,\n"
              "  \"int2\": -2,\n"
              "  \"real1\": 3.1,\n"
              "  \"real2\": -4.2,\n"
              "  \"bool\": true,\n"
              "  \"array\": [\n"
              "    \"a1\", \"a2\", \"a3\"\n"
              "  ],\n"
              "  \"emptyarray\": [],\n"
              "  \"arrayofobj\" [\n"
              "     { \"title\": \"o1\" }, { \"title\": \"o2\" }\n"
              "  ]\n"
              "}\n" ;

  printf("j1 source = %s\n", j1) ;

  if (dofromjsonu(dh, j1)) {
    printf("\n1.1 Parse of j1 (unexpanded) passed OK\n") ;
    thispass++ ;
  } else {
    printf("\n1.1 Parse of j1 TEST FAILED: %s\n", dojsonparsestrerror(dh)) ;
  }

  thispass+=check("1.1.1", dh, "/int1", do_unquoted, 0, 0.0) ;
  thispass+=check("1.1.1", dh, "/int1", do_unquoted, 1, 0.0) ;
  thispass+=check("1.1.1", dh, "/int1", do_sint64, 0, 0.0) ;

  thispass+=check("1.1.2", dh, "/int2", do_unquoted, 0, 0.0) ;
  thispass+=check("1.1.2", dh, "/int2", do_unquoted, -2, 0.0) ;
  thispass+=check("1.1.2", dh, "/int2", do_sint64, 0, 0.0) ;

  thispass+=check("1.1.3", dh, "/real1", do_unquoted, 0, 0.0) ;
  thispass+=check("1.1.3", dh, "/real1", do_unquoted, 0, 3.1) ;
  thispass+=check("1.1.3", dh, "/real1", do_double, 0, 0.0) ;

  thispass+=check("1.1.4", dh, "/real2", do_unquoted, 0, 0.0) ;
  thispass+=check("1.1.4", dh, "/real2", do_unquoted, 0, -4.2) ;
  thispass+=check("1.1.4", dh, "/real2", do_double, 0, 0.0) ;

  thispass+=check("1.1.5", dh, "/bool", do_unquoted, 0, 0.0) ;
  thispass+=check("1.1.5", dh, "/bool", do_unquoted, 1, 0) ;
  thispass+=check("1.1.5", dh, "/bool", do_bool, 0, 0.0) ;

  if (doisvalidjson(dh)) {
    printf("1.1.6 json appears valid - OK\n") ;
    thispass++ ;
  } else {
    printf("1.1.6 json is not valid - FAIL - %s\n", dojsonparsestrerror(dh)) ;
  }


  tpass += thispass ;
  ttest += 17 ;

  doclear(dh) ;


  thispass = 0 ;

  if (dofromjson(dh, j1)) {
    printf("\n1.2 Parse of j1 (expanding all) passed OK\n") ;
    thispass++ ;
  } else {
    printf("\n1.2 Parse of j1 TEST FAILED: %s\n", dojsonparsestrerror(dh)) ;
  }

  thispass+=check("1.2.1", dh, "/int1", do_sint64, 0, 0.0) ;
  thispass+=check("1.2.1", dh, "/int1", do_sint64, 1, 0.0) ;

  thispass+=check("1.2.2", dh, "/int2", do_sint64, 0, 0.0) ;
  thispass+=check("1.2.2", dh, "/int2", do_sint64, -2, 0.0) ;

  thispass+=check("1.2.3", dh, "/real1", do_double, 0, 0.0) ;
  thispass+=check("1.2.3", dh, "/real1", do_double, 0, 3.1) ;

  thispass+=check("1.2.4", dh, "/real2", do_double, 0, 0.0) ;
  thispass+=check("1.2.4", dh, "/real2", do_double, 0, -4.2) ;

  thispass+=check("1.1.5", dh, "/bool", do_bool, 0, 0.0) ;
  thispass+=check("1.2.5", dh, "/bool", do_bool, 1, 0) ;

  tpass += thispass ;
  ttest += 11 ;

  dodump(dh, "Expanded") ;


/************************************************************************/

  doclear(dh) ;

  printf("2. TESTING ERROR CASES\n") ;
  thispass = 0 ;

  char j2[] = "{badlabel:1}" ;

  if (dofromjson(dh, j2)) {
    dodump(dh,"\n2.1 Parse of j2 TEST FAILED: error not trapped\n") ;
  } else {
    printf("\n2.1 Parse of j2 rejected OK: %s\n", dojsonparsestrerror(dh)) ;
    thispass++ ;
  }

  doclear(dh) ;

  char j3[] = "{\"extraopen\":{{}" ;

  if (dofromjson(dh, j3)) {
    dodump(dh, "\n2.2 Parse of j3 TEST FAILED: error not trapped\n") ;
  } else {
    printf("\n2.2 Parse of j3 rejected OK: %s\n", dojsonparsestrerror(dh)) ;
    thispass++ ;
  }

  tpass += thispass ;
  ttest += 2 ;

/************************************************************************/

  printf("\n3. TESTING PARSING TO JSON\n\n") ;
  thispass = 0 ;

  doclear(dh) ;
  dosetuint(dh, do_uint32, 1, "/array/+/uint") ;
  dosetsint(dh, do_sint32, -1, "/array/*/sint") ;
  dosetreal(dh, do_double, 2.1, "/array/+/double") ;
  dosetdata(dh, do_string, "abc", 3, "/child/string") ;
  dosetdata(dh, do_unquoted, "$(unq)", 6, "/unquoted") ;
  dosetuint(dh, do_bool, 1, "/boolean") ;

  int len=0 ;
  char *aj1 = doasjson(dh, &len) ;
  char *aj1p = "{\"array\":[{\"uint\":1,\"sint\":-1},{\"double\":2.100000}],\"child\":{\"string\":\"abc\"},\"unquoted\":$(unq),\"boolean\":true}";


  if (strcmp(aj1, aj1p)==0) {
    printf("3.1 To Json test passed\n") ;
    thispass++ ;
  } else {
    printf("3.1 To Json test FAILED: \n%s\n%s\n", aj1,aj1p) ;
  }

  if (!doisvalidjson(dh)) {
    printf("3.2 json is not valid - OK - %s\n", dojsonparsestrerror(dh)) ;
    thispass++ ;
  } else {
    printf("3.2 json appears valid - FAIL - %s\n", dojsonparsestrerror(dh)) ;
  }


  tpass += thispass ;
  ttest += 2 ;


/************************************************************************/


  printf("\n\nRESULTS\n\n") ;

  if (tpass==ttest) {
    printf("%d/%d Test Passed\n", tpass, ttest) ;
  } else {
    printf("%d/%d Test FAILED\n", ttest-tpass, ttest) ;
  }
    
  dodelete(dh) ;

  exit(ttest-tpass) ;
}
