//
// dataobject_private.h
//
//
//

#ifndef _DATAOBJECT_PRIVATE_DEFINED
#define _DATAOBJECT_PRIVATE_DEFINED


#define DATAOBJECT IDATAOBJECT

typedef struct IDATAOBJECT {

  // Linked list of objects at this level
  struct IDATAOBJECT *next ;

  // Hierarchical child object
  struct IDATAOBJECT *child ;

  // Data Label and type
  // Arrays have ascii labels "0", "1" ...
  char *label ;
  int type ;
  int isarray ;  // Label is part of an array

  // Data storage
  unsigned long int d1 ;
  char *d2 ;

  // Temporary store / output buffer
  char *tmpbuf ;
  int tmpbufsize ;
  int tmpbuflen ;

  // JSON Parse error message
  char *jsonparsestatus ;

} IDATAOBJECT ;

// dataobject.c functions

IDATAOBJECT *_do_search(IDATAOBJECT *root, int forcecreate, char *path) ;
int _do_set(IDATAOBJECT *dh, int type, unsigned long int ldata, char *data, int datalen, char *path) ;
int _do_appendtmp(IDATAOBJECT *dh, char *src, int srclen) ;
int _do_cleartmp(IDATAOBJECT *dh) ;
int _do_clear(IDATAOBJECT *dh, int cleartop, int cleartopjsonerror) ;
int _do_strtcmp(char *haystack, char *needle, char term) ;
unsigned long int _do_signedencode(signed long int n) ;
signed long int _do_signeddecode(unsigned long int n) ;
unsigned long int _do_floatencode(float f) ;
float _do_floatdecode(unsigned long int n) ;
unsigned long int _do_doubleencode(double f) ;
double _do_doubledecode(unsigned long int n) ;

// dataobject_protobuf.c functions

char * _do_tovarint(unsigned long int n, int *len) ;
char * _do_tofixed32(unsigned long int n) ;
char * _do_tofixed64(unsigned long int n) ;

int _do_fromvarint(char *buf, unsigned long int *n, int buflen) ;
int _do_fromfixed32(char *buf, unsigned long int *n, int buflen) ;
int _do_fromfixed64(char *buf, unsigned long int *n, int buflen) ;

// Expand the do_data into the protobuf object

int _do_fromprotobuf(IDATAOBJECT *dh, char *protobuf, int buflen) ;


#endif


