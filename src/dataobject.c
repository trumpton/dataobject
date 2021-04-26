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


#include "dataobject_private.h"
#include "../dataobject.h"

// Local Functions


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
// @brief Get pointer to sub-object within root structure
// @param(in) root IDATAOBJECT handle
// @param(in) path Path to item
// @param(in) expandprotobuf If true, do_protobuf elements are expanded
// @param(in) forcecreate If true and node does not exist, it is automatically created
// @return Pointer to data object or NULL if not found
//

// Return a node or record - do not auto create

IDATAOBJECT *dogetchild(IDATAOBJECT *root, char *path)
{
  IDATAOBJECT *result = _do_search(root, path, 0) ;
  if (result) return result->child ;
  else return NULL ;
}

// Return a record

IDATAOBJECT *dosearchrecord(IDATAOBJECT *root, char *path)
{
  IDATAOBJECT *result = _do_search(root, path, 0) ;
  if (!result || result->type==do_node) return NULL ;
  else return result ;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the handle of a node at the given path, and create if not exiting
// @param(in) dh IDATAOBJECT handler
// @param(in) path Path in dh to get
// @return handle of the node

IDATAOBJECT * dogetnode(IDATAOBJECT *dh, char *path)
{
  return _do_search(dh, path, 1) ;
}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the handle of a node at the given path
// @param(in) dh IDATAOBJECT handler
// @param(in) path Path in dh to get
// @return handle of the node

IDATAOBJECT * dofindnode(IDATAOBJECT *dh, char *path)
{
  return _do_search(dh, path, 0) ;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the handle of a node's child
// @param(in) dh DATAOBJECT handler
// @return handle of new node created

IDATAOBJECT * dochild(DATAOBJECT *dh) 
{
  if (!dh) return NULL ;
  else return dh->child ;
}



IDATAOBJECT *_do_search(IDATAOBJECT *root, char *path, int forcecreate)
{

  if (!root || !path) return NULL ;

  IDATAOBJECT *nh = root ;

  while (*path=='/') path++ ;

  do {

    if ( ( *path!='\0' &&  nh->label &&  _do_strtcmp(path, nh->label, '/') ) ||
         ( *path!='\0' && !nh->label ) ||
         ( *path=='+' && isdigit(nh->label[0]) && nh->next==NULL ) ) {

      // Use first entry

      if (!nh->label) {
        int p ;
        for (p=0; path[p]!='\0' && path[p]!='/'; p++) ;
        nh->label=malloc(p+1) ;
        strncpy(nh->label, path, p) ; nh->label[p]='\0' ;
      }
      
      // Match found

      path += strlen(nh->label) ;
      while (*path=='/') path++ ;

      if (*path=='\0') {

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

    } else if (forcecreate && *path!='\0') {

      // Match not found at end of chain, so attach
      // hierarchy to the end of the chain

      nh->next = donew() ;
      if (!nh->next) goto fail ;
      nh = nh->next ;

      while (*path!='\0') {

        int l ;
        for (l=0; path[l]!='\0' && path[l]!='/'; l++) ;

        nh->label = malloc(l+1) ;
        if (!nh->label) goto fail ;
        strncpy(nh->label, path, l) ; nh->label[l]='\0' ; 

        nh->type = do_node ;
        path += l ;
        while (*path=='/') path++ ;

        if (*path!='\0') {
          nh->child = donew() ;
          if (!nh->child) goto fail ;
          nh = nh->child ;
        }

      }

    } else {

      // At end of chain and match not found

      return NULL ;
    }


  } while (*path!='\0') ;

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
  IDATAOBJECT *node = dofindnode(dh, path) ;
  
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
// @brief Paste a copy of the object into root structure
// @param(in) root IDATAOBJECT handle for destination
// @param(in) path Path from root to store pasteptr
// @param(in) pasteptr pointer to IDATAOBJECT to paste into root
// @param(in) merge If true, merge data where possible
// @return True on success
//
int _do_pastecopy(IDATAOBJECT *dest, IDATAOBJECT *rootdest, IDATAOBJECT *src, int level) ;

int dopastecopy(IDATAOBJECT *root, char *path, IDATAOBJECT *pasteptr, int merge)
{
  if (!root || !path || !pasteptr) return 0 ;

  // Search path

  IDATAOBJECT *s1 = dogetnode(root, path) ;

  if (!merge && s1->child) { 

    // Found and merge not requested, purge tree
    doclear(s1->child) ;

  } 

  // Paste tree
  return _do_pastecopy(s1, s1, pasteptr, 0) ;  

}


int _do_pastecopy(IDATAOBJECT *dest, IDATAOBJECT *rootdest, IDATAOBJECT *src, int level)
{

  if (!dest || !src) return 0 ;

  IDATAOBJECT *d=dest, *s=src ;

  while (s && s->label) {

    IDATAOBJECT *d = dest ;

    // Search for matching entry

    int found=0 ;
    do {
      if (d->label && strcmp(d->label, s->label)==0) {
        found=1 ;
      } else if (d->next) {
        d = d->next ;
      }
    } while ( d->next && !found  ) ;

    // Trap attempts to copy self

    if (s == rootdest && level != 0 ) { return 1 ; }

    // Create blank entry if none found

    if (!found && d->label) {
      d->next = donew() ;
      if (!d->next) goto fail ;
      d=d->next ;
    }

    // Create and copy label if not found

    if (!d->label) {
      d->label = malloc(strlen(s->label)+1) ;
      if (!d->label) goto fail ;
      strcpy(d->label, s->label) ;
    }

    // Copy d1


    d->d1 = s->d1 ;

    // Copy data / d2

    if (s->d2) {
      d->d2 = malloc(d->d1) ;
      if (!d->d2) goto fail ;
      memcpy(d->d2, s->d2, d->d1) ;
    }

    // Copy type

    d->type = s->type ;

    // Recurse to child

    if (s->child) {
      if (d->child) {
      } else {
        d->child = donew() ;
        if (!d->child) goto fail ;
      }
      _do_pastecopy(d->child, rootdest, s->child, level+1) ;
    }

    // Skip to next one

    s = s->next ;
  }

  return 1 ;

fail:

  doclear(dest) ;
  return 0 ;
  
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

int dosetuint(IDATAOBJECT *dh, enum dataobject_type type, unsigned long int data, char *path)
{
  if (!dh) {
    fprintf(stderr, "dosetuint: called with NULL handle\n") ;
    return 0 ;
  }
  assert(path) ;
  assert (type==do_uint32 || type==do_uint64 || type==do_bool || type==do_enum ||
      type==do_64bit || type==do_fixed64 || type==do_32bit || type==do_fixed32) ;

  return _do_set(dh, type, data, NULL, 0, path) ;
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

int dosetsint(IDATAOBJECT *dh, enum dataobject_type type, signed long int data, char *path)
{
  if (!dh) {
    fprintf(stderr, "dosetsint: called with NULL handle\n") ;
    return 0 ;
  }
  assert(path) ;
  assert(type==do_sint32 || type==do_sfixed32 || type==do_sint64 || type==do_sfixed64) ;

  unsigned long int udata ;
  int r ;

  switch(type) {

  case do_sint32:
  case do_sfixed32:

    r = _do_set(dh, type, _do_signedencode(data), NULL, 0, path) ;
    break ;

  case do_sint64:
  case do_sfixed64:

    r = _do_set(dh, type, _do_signedencode(data), NULL, 0, path) ;
    break ;

  default:

    assert(!type) ;
    break ;

  }

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

int dosetdata(IDATAOBJECT *dh, enum dataobject_type type, char *data, int datalen, char *path)
{
  if (!dh) {
    fprintf(stderr, "dosetdata: called with NULL handle\n") ;
    return 0 ;
  }
  assert(path) ;
  assert(type==do_string || type==do_data) ;
  return _do_set(dh, type, 0, data, datalen, path) ;
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

int dosettreal(IDATAOBJECT *dh, enum dataobject_type type, double data, char *path) 
{
  if (!dh) {
    fprintf(stderr, "dosetdouble: called with NULL handle\n") ;
    return 0 ;
  }
  assert(path) ;
  switch (type) {
  case do_float:
    return _do_set(dh, type, _do_floatencode(data), NULL, 1, path) ;
    break ;
  case do_double:
    return _do_set(dh, type, _do_doubleencode(data), NULL, 1, path) ;
    break ;
  default:
    assert(!type) ;
    break ;
  }

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

int dosettype(IDATAOBJECT *dh, enum dataobject_type type, char *path) 
{

  IDATAOBJECT *node = dofindnode(dh, path) ;

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

int dogetuint(IDATAOBJECT *dh, enum dataobject_type type, unsigned long int *n, char *path)
{
  IDATAOBJECT *node = dofindnode(dh, path) ;
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

long int dogetsint(IDATAOBJECT *dh, enum dataobject_type type,  long int *n, char *path)
{
  IDATAOBJECT *node = dofindnode(dh, path) ;
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

char * dogetdata(IDATAOBJECT *dh, enum dataobject_type type, int *datalen, char *path) 
{
  IDATAOBJECT *h = _do_search(dh, path, 0) ;
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

int dogetreal(IDATAOBJECT *dh, enum dataobject_type type, double *data, char *path)
{
  IDATAOBJECT *node = dofindnode(dh, path) ;
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

enum dataobject_type dogettype(IDATAOBJECT *dh, char *path)
{
  IDATAOBJECT *node = dofindnode(dh, path) ;
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



