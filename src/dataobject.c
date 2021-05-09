//
// dataobject.c
//
// This function manages a hierarchical dataobject.
//
// https://developers.google.com/protocol-buffers/docs/reference/cpp
//

//
// Dataobjects are stored in a hierarchy
//
//  OBJECT - NEXT - NEXT - NEXT
//            |
//           CHILD - NEXT - NEXT
//
// Objects can be nodes (which can contain arrays)
// They can also contain integer, float or text data
//
// Example JSON
//
// {
//    "count": 5, 
//    "root": {
//       "type": "array",
//       "data": [ "a", "b", "c" ]
//    }
// }
//
// Creates:
//
//  count:5 -- root:{}
//               |
//           type:"array" -- data:[]
//                             |                   
//                            0:"a"  -- 1:"b"  -- 2:"c"
//
//

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>


#include "dataobject_private.h"
#include "../dataobject.h"

// Local Functions

#define _do_genvpf(format, buf) \
  { \
    va_list args ; \
    va_start(args,format) ; \
    size_t len=vsnprintf(buf, 0, format, args) ; \
    buf=malloc(len+1) ; \
    if (buf) { \
      va_start(args,format) ; \
      vsnprintf(buf, len+1, format, args) ; \
    } \
  }

#define _do_freevpf(buf) \
  if (buf) { free(buf) ; buf=NULL ; }



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Creates a data object
// @return pointer to IDATAOBJECT, or NULL on error (errno set)
//

IDATAOBJECT *donew()
{
  IDATAOBJECT *dh ;
  dh=malloc(sizeof(IDATAOBJECT)) ;
  if (!dh) return NULL ;

  memset(dh, '\0', sizeof(IDATAOBJECT)) ;
  dh->type = -1 ;

  return dh ;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Creates a data object
// @param(in) root Object to clone or NULL
// @return pointer to DATAOBJECT, or NULL on error (errno set)
//

int _do_copynode(IDATAOBJECT *src, IDATAOBJECT *dst)
{
  if (!src || !dst) return 0 ;
  IDATAOBJECT *s=src, *d=dst ;
  
  do {

    // Recurse

    if (s->child) {
      d->child = donew() ;
      _do_copynode(s->child, d->child) ;
    }

    // Copy label

    if (s->label) {
      d->label = malloc(strlen(s->label)+1) ;
      if (!d->label) goto fail ;
      strcpy(d->label, s->label) ;
    }

    // Copy ints

    d->type = s->type ;
    d->isarray = s->isarray ;
    d->d1 = s->d1 ;

    // Copy string buffer data

    if (s->d2 && s->d1 > 0) {
      d->d2 = malloc(s->d1) ;
      if (!d->d2) goto fail ;
      memcpy(d->d2, s->d2, s->d1) ;
    }

    // Skip to next entry

    if (s->next) {
      d->next = donew() ;
      d = d->next ;
      if (!d) goto fail ;
    }

    s = s->next ;

  } while (s) ;

  return 1 ;

fail:
  return 0 ;
}

DATAOBJECT *donewfrom(IDATAOBJECT *root) 
{
  IDATAOBJECT *dh = donew() ;
  if (!root || !dh) return dh ;

  if (_do_copynode(root, dh)) {
    return dh ;
  } else {
    dodelete(dh) ;
    return NULL ;
  }
  
}




///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Clears dataobject structure
// @param(in) pointer to IDATAOBJECT
// @param(in) cleartop If true, completely frees the top
// @return True on success
//

int doclear(IDATAOBJECT *dh)
{
  // Clear everything but don't free the top handle
  _do_clear(dh, 0, 1) ;
}

int _do_clear(IDATAOBJECT *dh, int cleartop, int cleartopjsonerror)
{
  if (!dh) {
    fprintf(stderr, "doclear: called with NULL handle\n") ;
    return 0 ;
  }

  // Delete chain

  IDATAOBJECT *dn = dh ;

  while (dn) {

    IDATAOBJECT *next = dn->next ;

    // Recurse into child

    if (dn->child) { _do_clear(dn->child, 1, 1) ; }

    // Reset associated data

    dn->child = NULL ;
    dn->next = NULL ;

    dn->d1=0 ;
    dn->type=-1 ;
    dn->isarray=0 ;

    if (dn->tmpbuf) free(dn->tmpbuf) ; dn->tmpbuf=NULL ;
    if (dn->label) free(dn->label) ; dn->label=NULL ;
    if (dn->d2) free(dn->d2) ; dn->d2=NULL ;

    if ((cleartop || dn!=dh || cleartopjsonerror) && dn->jsonparsestatus) {
      free(dn->jsonparsestatus) ;
      dn->jsonparsestatus=NULL ;
    }
    
    if (dn!=dh || cleartop) {

      // Remove the structure itself

      free(dn) ;

    }

    dn=next ;
  }

  return 1 ;
}



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//

int dodelete(IDATAOBJECT *dh)
{
  if (!dh) {
    fprintf(stderr, "dodelete: called with NULL handle\n") ;
    return 0 ;
  }

  // Clear structure entirely
  _do_clear(dh, 1, 1) ;

  return 1 ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the handle of a node at the given path, and create if not exiting
// @param(in) dh IDATAOBJECT handler
// @param(in) path Path in dh to get
// @return handle of the node

IDATAOBJECT * dogetnode(IDATAOBJECT *dh, char *path, ...)
{
  char *vpath ;
  _do_genvpf(path, vpath) ;
  IDATAOBJECT *r=_do_search(dh, 1, vpath) ;
  _do_freevpf(vpath) ;
  return r ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the handle of a node at the given path
// @param(in) dh IDATAOBJECT handler
// @param(in) path Path in dh to get
// @return handle of the node

IDATAOBJECT * dofindnode(IDATAOBJECT *dh, char *path, ...)
{
  char *vpath ;
  _do_genvpf(path, vpath) ;
  IDATAOBJECT *r=_do_search(dh, 0, vpath) ;
  _do_freevpf(vpath) ;
  return r ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Get handle of the nth node
// @param(in) root DATAOBJECT handle
// @param(in) n Count of node
// @return Pointer to node itself or NULL if not found
//

DATAOBJECT * donoden(DATAOBJECT *root, int n) 
{
  DATAOBJECT *p = root ;

  while (p && n>0) {
    p=p->next ;
    n-- ;
  }

  return p ;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the handle of a node's child
// @param(in) dh DATAOBJECT handler
// @return handle of new node's child

IDATAOBJECT * dochild(DATAOBJECT *dh) 
{
  if (!dh) return NULL ;
  else return dh->child ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the dataobject label
// @param(in) dh IDATAOBJECT handle
// @return Pointer to node label
//

char * donodelabel(IDATAOBJECT *dh)
{
  if (!dh) return NULL ;
  return dh->label ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the dataobject contents
// @param(in) dh IDATAOBJECT handle
// @param(out) len Pointer to location to store length or NULL
// @return Pointer to node contents
//

char * donodedata(IDATAOBJECT *dh, int *len)
{
  if (!dh) return NULL ;
  if (len) (*len) = dh->d1 ;
  return dh->d2 ;
}



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// Internal search function
//

IDATAOBJECT *_do_search(IDATAOBJECT *root, int forcecreate, char *path)
{

  if (!root || !path) return NULL ;

  IDATAOBJECT *nh = root ;

  char *p = path ;

  // Skip leading slashes

  while (*p=='/') p++ ;

  do {

    // If the current label in the path matches this entry
    // Or search is for + and the entry is the last numerical (append array)

    if ( ( *p!='\0' &&  nh->label && _do_strtcmp(p, nh->label, '/') ) ||
         ( *p=='+' && isdigit(nh->label[0]) && nh->next==NULL ) ) {

      // Found a match
      // Skip along path to next entry

      p += strlen(nh->label) ;
      while (*p=='/') p++ ;

      if (*p=='\0') {

        // At the end of the path
        return nh ;

      }


      if (nh->child) { 

        // Descend

        nh = nh->child ;

      }


    } else if (nh->next) {

      // Match not found, progress along chain

      nh=nh->next ;
      //entrynum++ ;

    } else if (forcecreate && *p!='\0') {

      // Match not found at end of chain, so attach
      // hierarchy to the end of the chain

      if (nh->label) {

        // Create new entry (unless the do structure
        // is empty, and in that case, just use the 
        // first one

        nh->next = donew() ;
        if (!nh->next) goto fail ;
        nh = nh->next ;

      }

      while (*p!='\0') {

        int l ;
        for (l=0; p[l]!='\0' && p[l]!='/'; l++) ;

        nh->label = malloc(l+1) ;
        if (!nh->label) goto fail ;
        strncpy(nh->label, p, l) ; nh->label[l]='\0' ; 

        nh->type = do_node ;
        p += l ;
        while (*p=='/') p++ ;

        if (*p!='\0') {
          nh->child = donew() ;
          if (!nh->child) goto fail ;
          nh = nh->child ;
        }

      }

    } else {

      // At end of chain and match not found

      return NULL ;
    }


  } while (*p!='\0') ;

  return nh ;

fail:

  return NULL ;

}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Renames the identified node
// @param(in) dh IDATAOBJECT handler
// @param(in) path Path in dh to get
// @param(in) newname Name of new node
// @return true on success

int dorenamenode(IDATAOBJECT *dh, char *path, char *newname) 
{
  IDATAOBJECT *node = _do_search(dh, 0, path) ;
  
  if (!node) return 0 ;
  if (strstr(newname, "/")!=NULL) {
    fprintf(stderr, "dorenamenode error - received newname containing '/'\n") ;
    return 0 ;
  }

  if (node->label) free(node->label) ;
  node->label = malloc(strlen(newname)+1) ;
  if (!node->label) return 0 ;
  strcpy(node->label, newname) ;
  return 1 ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents to be an unsigned int
// @param(in) dh IDATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Unsigned int / boolean / enumeration to save
// @param(in) path Path to item
// @return True on success
//

int dosetuint(IDATAOBJECT *dh, enum dataobject_type type, unsigned long int data, char *path, ...)
{
  if (!dh) {
    fprintf(stderr, "dosetuint: called with NULL handle\n") ;
    return 0 ;
  }
  assert(path) ;
  assert (type==do_uint32 || type==do_uint64 || type==do_bool || type==do_enum ||
      type==do_64bit || type==do_fixed64 || type==do_32bit || type==do_fixed32) ;

  char *vpath ;
  _do_genvpf(path, vpath) ;
  int r=_do_set(dh, type, data, NULL, 0, vpath) ;
  _do_freevpf(vpath) ;
  return r ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents to be an signed int
// @param(in) dh IDATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Signed int to save
// @param(in) path Path to item
// @return True on success
//

int dosetsint(IDATAOBJECT *dh, enum dataobject_type type, signed long int data, char *path, ...)
{
  if (!dh) {
    fprintf(stderr, "dosetsint: called with NULL handle\n") ;
    return 0 ;
  }
  assert(path) ;
  assert(type==do_sint32 || type==do_sfixed32 || type==do_sint64 || type==do_sfixed64) ;

  unsigned long int udata ;
  int r ;
  char *vpath ;
  _do_genvpf(path, vpath) ;

  switch(type) {

  case do_sint32:
  case do_sfixed32:

    r = _do_set(dh, type, _do_signedencode(data), NULL, 0, vpath) ;
    break ;

  case do_sint64:
  case do_sfixed64:

    r = _do_set(dh, type, _do_signedencode(data), NULL, 0, vpath) ;
    break ;

  default:

    assert(!type) ;
    break ;

  }

  _do_freevpf(vpath) ;
  return r ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents
// @param(in) dh IDATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Pointer to data source
// @param(in) datalen Length of data to store
// @param(in) path Path to item
// @return True on success
//

int dosetdata(IDATAOBJECT *dh, enum dataobject_type type, char *data, int datalen, char *path, ...)
{
  if (!dh) {
    fprintf(stderr, "dosetdata: called with NULL handle\n") ;
    return 0 ;
  }
  assert(path) ;
  assert(type==do_string || type==do_data) ;

  char *vpath ;
  _do_genvpf(path, vpath) ;

  int r=_do_set(dh, type, 0, data, datalen, vpath) ;

  _do_freevpf(vpath) ;
  return r ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents to be a real number
// @param(in) dh IDATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data float or double to save
// @param(in) path Path to item
// @return True on success
//

int dosetreal(IDATAOBJECT *dh, enum dataobject_type type, double data, char *path, ...) 
{
  if (!dh) {
    fprintf(stderr, "dosetdouble: called with NULL handle\n") ;
    return 0 ;
  }
  assert(path) ;

  int r=0 ;
  char *vpath ;
  _do_genvpf(path, vpath) ;

  switch (type) {
  case do_float:
    r=_do_set(dh, type, _do_floatencode(data), NULL, 1, vpath) ;
    break ;
  case do_double:
    r=_do_set(dh, type, _do_doubleencode(data), NULL, 1, vpath) ;
    break ;
  default:
    assert(!type) ;
    break ;
  }

  _do_freevpf(vpath) ;
  return r ;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the type of the requested item (does not affect underlying data)
// @param(in) dh IDATAOBJECT handle
// @param(in) type Record type
// @param(in) path Path to item
// @return True on success
//

int dosettype(IDATAOBJECT *dh, enum dataobject_type type, char *path, ...) 
{

  char *vpath ;
  _do_genvpf(path, vpath) ;
  IDATAOBJECT *node = _do_search(dh, 0, vpath) ;
  _do_freevpf(vpath) ;

  if (!node) return 0 ;
  if (node->child) return 0 ;

  // If changing type to data, but no data present, fail

  if ((type==do_data || type==do_string) && !node->d2) return 0 ;

  // Set type

  node->type = type ;

  // If no longer a string, free data

  if (type!=do_data && type!=do_string && node->d2) {
    free(node->d2) ;
    node->d2 = 0 ;
  }

  return 0 ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the dataobject contents to be an unsigned int.  If string, returns length.
// @param(in) dh IDATAOBJECT handle
// @param(in) type Record type
// @param(out) n Pointer to location to store results
// @param(in) path Path to item
// @return True on success
//

int dogetuint(IDATAOBJECT *dh, enum dataobject_type type, unsigned long int *n, char *path, ...)
{

  char *vpath ;
  _do_genvpf(path, vpath) ;

  IDATAOBJECT *node = _do_search(dh, 0, vpath) ;
  _do_freevpf(vpath) ;

  if (!node) return 0 ;


  switch (node->type) {
  case do_64bit:
  case do_32bit:
  case do_enum:
  case do_uint32:
  case do_uint64:
  case do_int32: 
  case do_int64:
  case do_fixed32:
  case do_fixed64:
  case do_string:
  case do_data:

    // Return number

    (*n) = node->d1 ;
    return 1 ;
    break ;

  case do_sint32:
  case do_sint64:
  case do_sfixed32:
  case do_sfixed64:

    // discard sign and return

    (*n) = (node->d1 >> 1) ;
    return 1 ;
    break ;

  case do_float: {

    // fetch, round, discard sign and return float

    float f = _do_floatdecode(node->d1) ;
    if (f<0) f = -f ;
    (*n) = (unsigned long int)f ;
    return 1 ;
    break ;
  }

  case do_double: {

    // fetch, round, discard sign and return double
    float f = _do_doubledecode(node->d1) ;
    if (f<0) f = -f ;
    (*n) = (unsigned long int)f ;
    return 1 ;
    break ;
  }

  default:

    // Type not supported
    return 0 ;

  }

  // Catch all
  return 0 ;
}




///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the dataobject contents to be an signed int, if string returns length.
// @param(in) dh IDATAOBJECT handle
// @param(in) type Record type
// @param(out) n Pointer to location to store results
// @param(in) path Path to item
// @return True on success
//

long int dogetsint(IDATAOBJECT *dh, enum dataobject_type type,  long int *n, char *path, ...)
{
  char *vpath ;
  _do_genvpf(path, vpath) ;

  IDATAOBJECT *node = _do_search(dh, 0, vpath) ;

  _do_freevpf(vpath) ;

  if (!node) return 0 ;

  switch (node->type) {

  case do_64bit:
  case do_32bit:
  case do_enum:
  case do_uint32:
  case do_uint64:
  case do_int32: 
  case do_int64:
  case do_fixed32:
  case do_fixed64:

    // Return number
    (*n) = node->d1 ;
    return 1 ;
    break ;

  case do_bool:
    if (node->d1) return 1 ;
    else return 0 ;
    break ;

  case do_sint32:
  case do_sint64:
  case do_sfixed32:
  case do_sfixed64:

    // discard sign and return
    (*n) = _do_signeddecode(node->d1) ;
    return 1 ;
    break ;

  case do_float: {

    // fetch, round and return float

    float f = _do_floatdecode(node->d1) ;
    (*n) = (unsigned long int)f ;
    return 1 ;

  }

  case do_double: {

    // fetch, round and return double
    float f = _do_doubledecode(node->d1) ;
    (*n) = (unsigned long int)f ;

    return 1 ;

  }

  default:

    // Error, unknown type
    return 0 ;
  }

  // Catch all
  return 0 ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the dataobject contents
// @param(in) dh IDATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Pointer to data source
// @param(in) datalen Length of data to store
// @param(in) path Path to item
// @return Pointer to data or NULL on error or not found
//

char * dogetdata(IDATAOBJECT *dh, enum dataobject_type type, int *datalen, char *path, ...) 
{
  char *vpath ;
  _do_genvpf(path, vpath) ;

  IDATAOBJECT *h = _do_search(dh, 0, vpath) ;

  _do_freevpf(vpath) ;

  if (!h) return NULL ;
  if (datalen) (*datalen) = h->d1 ;
  return h->d2 ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents to be a real number
// @param(in) dh IDATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Pointer to double to store result
// @param(in) path Path to item
// @return True on success
//

int dogetreal(IDATAOBJECT *dh, enum dataobject_type type, double *data, char *path, ...)
{
  char *vpath ;
  _do_genvpf(path, vpath) ;

  IDATAOBJECT *node = _do_search(dh, 0, vpath) ;

  _do_freevpf(vpath) ;

  if (!node) return 0 ;

  switch (node->type) {

  case do_64bit:
  case do_32bit:
  case do_enum:
  case do_uint32:
  case do_uint64:
  case do_int32: 
  case do_int64:
  case do_fixed32:
  case do_fixed64:

    // Return number

    (*data) = (double)node->d1 ;
    return 1 ;
    break ;

  case do_sint32:
  case do_sint64:
  case do_sfixed32:
  case do_sfixed64:

    // Return signed number

    (*data) = (double)_do_signeddecode(node->d1) ;
    return 1 ;
    break ;

  case do_float:

    // fetch, and return float

    (*data) = (double)_do_floatdecode(node->d1) ;
    return 1 ;

  case do_double:

    // fetch, and return double

    (*data) = (double)_do_doubledecode(node->d1) ;
    return 1 ;

  default:

    // Error, unknown type
    return 0 ;
  }

  // Catch all
  return 0 ;
}



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Get the type of the requested item 
// @param(in) dh IDATAOBJECT handle
// @param(in) path Path to item
// @return Record type
//

enum dataobject_type dogettype(IDATAOBJECT *dh, char *path, ...)
{
  char *vpath ;
  _do_genvpf(path, vpath) ;

  IDATAOBJECT *node = _do_search(dh, 0, path) ;

  _do_freevpf(vpath) ;

  if (!node) return 0 ;

  return node->type ;
}



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// LOCAL FUNCTIONS
//
//


///////////////////////////////////////////////////////////
//
// @brief Set data type within structure
// @param(in) dh Handle of data object
// @param(in) type Type of data to set
// @param(in) ldata Unsigned long data
// @param(in) data String data (or NULL)
// @param(in) datalen Length of String data
// @param(in) fdata Float data
// @param(in) path Path at which data is to be stored
// @return true on success
//

int _do_set(IDATAOBJECT *dh, int type, unsigned long int ldata, char *data, int datalen, char *path)
{
  // Check handles

  if (!dh || !path) {
    assert(dh) ;
    assert(path) ;
    return 0 ;
  }

  IDATAOBJECT *h = dogetnode(dh, path) ;
  if (!h) goto fail ;

  // Store the data

  h->type=type ;

  switch(type) {

  case do_string:
  case do_data:

    if (h->d2) {
      free(h->d2) ;
      h->d2 = NULL ;
      h->d1 = 0 ;
    }

    if (data && datalen>=0) {
      h->d2 = malloc(datalen + 1) ;
      if (!h->d2) goto fail ;
      memcpy(h->d2, data, datalen) ;
      h->d2[datalen]='\0' ;
      h->d1 = datalen ;
    }
    break ;

  default:

    if (h->d2) {
      free(h->d2) ;
      h->d2=NULL ;
    }

    h->d1 = ldata ;
    break ;

  }

  return 1 ;

fail:
  return 0 ;

}

int _do_strtcmp(char *haystack, char *needle, char term)
{
  while (*haystack == *needle && *needle!='\0' && *haystack!='\0') {
    haystack++ ;
    needle++ ;
  }
  return (*needle=='\0' && (*haystack=='\0' || *haystack==term)) ;
}

unsigned long int _do_signedencode(signed long int n)
{
  if (n<0) {
    return ( (-n) << 1) + 1 ;
  } else {
    return (n << 1) ;
  }
}

signed long int _do_signeddecode(unsigned long int n)
{
  if (n&1) {
    return -(signed int)( n>>1 ) ;
  } else {
    return (signed int)(n>>1) ;
  }
}

unsigned long int _do_floatencode(float f) 
{
  unsigned int i ;
  memcpy((unsigned char *)&i, (unsigned char *)&f, sizeof(unsigned int)) ;
  return (unsigned long int)i ;
}

float _do_floatdecode(unsigned long int n) 
{
  float f ;
  unsigned int i = n ;
  memcpy((unsigned char *)&f, (unsigned char *)&i, sizeof(float)) ;
  return f ;
}

unsigned long int _do_doubleencode(double f) 
{
  unsigned long int i ;
  memcpy((unsigned char *)&i, (unsigned char *)&f, sizeof(unsigned long int)) ;
  return i ;
}

double _do_doubledecode(unsigned long int n) 
{
  double d ;
  memcpy((unsigned char *)&d, (unsigned char *)&n, sizeof(double)) ;
  return d ;
}



