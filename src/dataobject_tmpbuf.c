//
// dataobject_tmpbuf.c
//
// This function manages the tmpbuf
//
//


#include <malloc.h>
#include <string.h>
#include <assert.h>


#include "dataobject_private.h"
#include "../dataobject.h"


///////////////////////////////////////////////////////////
//
// @brief Append data to tmp data buffer allocating as required
// @param(in) dh Handle of data object
// @param(in) src Data to append
// @param(in) srclen Length of data to append
// @return true on success
//

int _do_appendtmp(IDATAOBJECT *dh, char *src, int srclen)
{
  static int alloclen = 128;

  if (!dh) {
    assert(dh) ;
    return 0 ;
  }

  // Resize buffer if necessary

  if ( (dh->tmpbuflen + srclen) > dh->tmpbufsize ) {

    // Calculate newsize (srclen+1) to allow for null termination
    int newsize = dh->tmpbufsize + ( 1 + srclen/alloclen ) * alloclen  ;

    char *np = realloc( dh->tmpbuf, newsize + 1 ) ;
    if (!np) { return 0 ; }
    
    dh->tmpbuf = np ; 
    dh->tmpbufsize = newsize ;

  }

  // Transfer data and null terminate

  memcpy( &(dh->tmpbuf[dh->tmpbuflen]), src, srclen) ;
  dh->tmpbuflen += srclen ;
  dh->tmpbuf[dh->tmpbuflen] = '\0' ;

  return 1 ;
}


///////////////////////////////////////////////////////////
//
// @brief Cleares / releases tmp data
// @param(in) dh Handle of data object
// @return true on success
//

int _do_cleartmp(IDATAOBJECT *dh)
{
  if (!dh) return 0 ;
  IDATAOBJECT *h = dh ;
  do {
    if (h->tmpbuf) free(h->tmpbuf) ;
    h->tmpbuf=NULL ;
    h->tmpbufsize=0 ;
    h->tmpbuflen=0 ;
    h=h->next ;
  } while (h) ;
  return 1 ;
}


