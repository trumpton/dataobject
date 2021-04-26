//
// dataobject_protobuf.c
//
// Protobuf conversion functions
//
//

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "dataobject_private.h"
#include "../dataobject.h"


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Output data as a Protobuf string
// @param[in] dh Data object handle
// @param[out] len Length of Protobuf data produced
// @return Protobuf data string or NULL if error
//


char * doasprotobuf(IDATAOBJECT *dh, int *len)
{
  if (!dh) {
    fprintf(stderr, "doasprotobuf: called with NULL handle\n") ;
    return 0 ;
  }
 
  _do_cleartmp(dh) ;

  IDATAOBJECT *h = dh ;

  while (h) {

    // ignore any labels not in the form fXXXX

    if (h->label && h->label[0]=='f') {

      int fieldnum = atoi( &(h->label[1]) ) ;

      if (h->child) {

        // Recurse to generate child data

        int childdatalen ;
        char *childdata = doasprotobuf(h->child, &childdatalen) ;

        // Child type / header

        int fieldnumlen ;
        char *fielddat = _do_tovarint( (fieldnum<<3)|2, &fieldnumlen) ;
        _do_appendtmp(dh, fielddat, fieldnumlen) ;

        // Child Length

        int childlengthlen ;
        char *childlength = _do_tovarint(childdatalen, &childlengthlen) ;
        _do_appendtmp( dh, childlength, childlengthlen) ;

        // Finally, attach the child data

        _do_appendtmp( dh, childdata, childdatalen) ;

        // And tidy up

        _do_cleartmp(h->child) ;

      } else {

        switch (h->type) {

          case do_64bit:
          case do_32bit:
          case do_enum:
          case do_uint32:
          case do_uint64:
          case do_int32: 
          case do_int64:
          case do_sint32:
          case do_sint64:
          case do_bool: {

            int len ;

            char *field = _do_tovarint( (fieldnum<<3)|0, &len) ;
            _do_appendtmp(dh, field, len) ;

            char *varint = _do_tovarint(h->d1, &len) ;
            _do_appendtmp(dh, varint, len) ;

            break ;

          } 

          case do_sfixed64:
          case do_fixed64:
          case do_double: {

            int len ;

            char *field = _do_tovarint( (fieldnum<<3)|1, &len) ;
            _do_appendtmp(dh, field, len) ;

            char *var64 = _do_tofixed64(h->d1) ;
            _do_appendtmp(dh, var64, 8) ;

            break ;

          }

          case do_fixed32:
          case do_sfixed32:
          case do_float: {

            int len ;

            char *field = _do_tovarint( (fieldnum<<3)|5, &len) ;
            _do_appendtmp(dh, field, len) ;

            char *var32 = _do_tofixed32(h->d1) ;
            _do_appendtmp(dh, var32, 4) ;

            break ;

          }


          case do_string:
          case do_data: {

            int len ;

            char *field = _do_tovarint( (fieldnum<<3)|2, &len) ;
            _do_appendtmp(dh, field, len) ;

            char *length = _do_tovarint(h->d1, &len) ;
            _do_appendtmp( dh, length, len) ;

            _do_appendtmp( dh, h->d2, h->d1 ) ;

            break ;

          }

        }

      }

    }

    // Move to next entry in chain

    h = h->next ;

  }

  // Terminate and return 

  if (len) { (*len) = dh->tmpbuflen ; } 
  return dh->tmpbuf ;

}



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// Build a data object from a protobuf source.  Note that
// encapsulation is not done here as it is not known as to 
// whether a do_data element is a string or encapsulated
// data.  Encapsulation expansion of do_data is performed 
// as required through the use of nodefromprotobuf.
//
// @brief Builds data object from protobuf source
// @param(out) dh Data object handle
// @param(in) protobuf Pointer to protobuf data
// @param(in) buflen Length of Protobuf data
// @return Length of data processed
//

int _do_fromprotobuf(IDATAOBJECT *dh, char *protobuf, int buflen) 
{
  if (!dh || !protobuf) return -1 ;

  if (buflen==0) return 0 ;

  int p=0 ; // Current position in protobuf

  IDATAOBJECT *d = dh ;

  while (p<buflen) {

    if ( p>0 ) {
      d->next = donew() ;
      d = d->next ;
    }

    int l ; // Length of message
    unsigned long int n ;

    // Get type and id

    l = _do_fromvarint(&protobuf[p], &n, buflen-p) ;
    if (l<0) { 
      goto fail ;
    }
    p+=l ;

    // Extract ID and type
    int id = n>>3 ;
    int type = n&7 ;

    char f[16] ;
    sprintf(f, "f%d", id) ;
    d->label = malloc(strlen(f)+1) ;

    if (!d->label) {
      goto fail ;
    }
    strcpy(d->label, f) ;

    switch (type) {

    case 0: // Varint

      l = _do_fromvarint(&protobuf[p], &n, buflen-p) ;
      if (l<0) { 
        goto fail ;
      }

      p+=l ;
      d->type = do_uint64 ;
      d->d1 = n ;
      break ;

    case 1: // Fixed32

      l = _do_fromfixed32(&protobuf[p], &n, buflen-p) ;
      if (l<0) { 
        goto fail ;
      }
      p+=l ;
      d->d1 = n ;
      d->type = do_fixed32 ;
      break ;

    case 2: // Data

      l = _do_fromvarint(&protobuf[p], &n, buflen-p) ;
      if (l<0) { 
        goto fail ;
      }
      p+=l ;
      if (n>buflen-p) {
        printf("n (%ld) > buflen (%d) - p(%d)\n", n, buflen, p) ; 
        goto fail ;
      }
      d->d1 = n ;
      d->type = do_data ;
      d->d2 = malloc(n+1) ;
      if (!d->d2) {
        printf("d->d2 = malloc(%d)\n", (int)n+1) ;
        goto fail ;
      }
      memcpy(d->d2, &protobuf[p], n) ;
      // Null attached, but not included in d->d1
      d->d2[n]='\0' ;
      p+=n ;
      break ;

    case 5: // Fixed64

      l = _do_fromfixed64(&protobuf[p], &n, buflen-p) ;
      if (l<0) { 
        goto fail ;
      }
      p+=l ;
      d->d1 = n ;
      d->type = do_fixed64 ;
      break ;

    default: // Not supported
      d->type = do_unknown ;
      break ;
    }

  }

  return buflen ;

fail:
  doclear(dh) ;
  return -1 ;
}


int dofromprotobuf(IDATAOBJECT *dh, char *protobuf, int buflen) 
{
  doclear(dh) ;
  int r = _do_fromprotobuf(dh, protobuf, buflen) ;
  if (r>=0) {
    return 1 ;
  } else {
    fprintf(stderr, "dofromprotobuf: error decoding\n") ;
    return 0 ;
  }
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Expands a node containing protobuf data
// @param(out) dh Data object handle
// @param(in) path Path to node
// @return True on success
//

int doexpandfromprotobuf(IDATAOBJECT *root, char *path)
{
  IDATAOBJECT *node = dosearchrecord(root, path) ;
  if (!node) return 0 ;
  if (node->child) return 0 ;
  if (node->type!=do_data && node->type!=do_string) return 0 ;
  if (!node->d2) return 0 ;
  node->child = donew() ;
  if (!node->child) return 0 ;
  if (!dofromprotobuf(node->child, node->d2, node->d1)) {
    dodelete(node->child) ;
    node->child=NULL ;
    return 0 ;
  } else {
    free(node->d2) ;
    node->d2=NULL ;
    node->d1=0 ;
    node->type=do_node ;
   return 1 ;
  }
}



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// LOCAL FUNCTIONS
//
//

int _do_fromvarint(char *buf, unsigned long int *n, int buflen)
{
  if (buflen<=0 || !buf || !n) return 0 ;

  int i=0 ;
  (*n)=0 ;
  int shift=0 ;
  int done=0 ;

  do {
    (*n) = (*n) | ((unsigned long int)(buf[i] & 0x7F))<<shift ;
    done = !(buf[i]&0x80) ;
    shift+=7 ;
    i++ ;

  } while (i<buflen && !done) ;

  return i ;
}

int _do_fromfixed32(char *buf, unsigned long int *n, int buflen) 
{
  if (buflen<4) return -1 ;

  int i=0 ;
  (*n)=0 ;

  for (i=0; i<4; i++) {
    (*n) = ((*n)<<8) | buf[i] ;
  }

  return 4 ;
}

int _do_fromfixed64(char *buf, unsigned long int *n, int buflen) 
{
  if (buflen<8) return -1 ;

  int i=0 ;
  (*n)=0 ;

  for (i=0; i<8; i++) {
    (*n) = ((*n)<<8) | buf[i] ;
  }

  return 8 ;
}




///////////////////////////////////////////////////////////
//
// @brief Converts an integer to a varint
// @param(in) n Integer to convert
// @param(in) len Pointer to length
// @return Pointer to varint result of length (len)
//

char * _do_tovarint(unsigned long int n, int *len)
{
  static char varint[8] ;

  if      ( ! (n & 0xFFFFFFFFFFFFFF80 ) ) { (*len)=1 ; }
  else if ( ! (n & 0xFFFFFFFFFFFFC000 ) ) { (*len)=2 ; }
  else if ( ! (n & 0xFFFFFFFFFFE00000 ) ) { (*len)=3 ; }
  else if ( ! (n & 0xFFFFFFFFF0000000 ) ) { (*len)=4 ; }
  else if ( ! (n & 0xFFFFFFF800000000 ) ) { (*len)=5 ; }
  else if ( ! (n & 0xFFFFFC0000000000 ) ) { (*len)=6 ; }
  else if ( ! (n & 0xFFFE000000000000 ) ) { (*len)=7 ; }
  else if ( ! (n & 0xFF00000000000000 ) ) { (*len)=7 ; }
  else { (*len)=8 ; }


  for ( int i=0 ; i < (*len) ; i++ ) { 
    varint[i] = (n&0x7F) | ((i<(*len)-1)?0x80:0x00) ;
    n = (n>>7) ;
  }

  return varint ;

}


///////////////////////////////////////////////////////////
//
// @brief Converts an integer to a fixed 64 string
// @param(in) n Integer to convert
// @param(in) len Pointer to length
// @return Pointer to fixed64 of length 8
//

char * _do_tofixed64(unsigned long int n)
{
  static char fixed64[8] ;

  for ( int i=0 ; i < 8 ; i-- ) { 
    fixed64[8-i] = (n&0xFF) ;
    n = (n>>8) ;
  }

  return fixed64 ;
}


///////////////////////////////////////////////////////////
//
// @brief Converts an integer to a fixed 32 string
// @param(in) n Integer to convert
// @param(in) len Pointer to length
// @return Pointer to fixed32 result of length 4
//

char * _do_tofixed32(unsigned long int n)
{
  static char fixed32[4] ;

  for ( int i=0 ; i < 4 ; i-- ) { 
    fixed32[4-i] = (n&0xFF) ;
    n = (n>>8) ;
  }

  return fixed32 ;
}


