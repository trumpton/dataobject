//
// dataobject_dump.c
//
// Dumps dataobject structure to stdout
//
//

#include <stdio.h>

#include "dataobject_private.h"
#include "../dataobject.h"

void _do_dump(IDATAOBJECT *dh, int depth) ;

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Dumps data structure output to stdout
// @param[in] dh Data object handle
// @return nothing
//

void dodump(IDATAOBJECT *dh, char *title)
{
  if (!dh) {
    fprintf(stderr, "dodump: called with NULL handle\n") ;
    return ;
  }

  if (title) printf("\nDATAOBJECT DUMP: %s\n", title) ;
  _do_dump(dh, 0) ;
  if (title) printf("\n") ;
}

void _do_dump(IDATAOBJECT *dh, int depth)
{

  if (depth>100) {
    fprintf(stderr, "dodump: recursion depth too great\n") ;
    return ;
  }

  while (dh) {
    for (int i=0; i<depth; i++) printf("  ") ;
    printf("/%s%s%s (%s):", 
      dh->isarray?"[":"",
      dh->label?dh->label:"<empty label>",
      dh->isarray?"]":"",
      (dh->type) == do_int32 ? "int32" :
      (dh->type) == do_int64 ? "int64" :
      (dh->type) == do_uint32 ? "uint32" :
      (dh->type) == do_uint64 ? "uint64" :
      (dh->type) == do_sint32 ? "sint32" :
      (dh->type) == do_sint64 ? "sint64" :
      (dh->type) == do_bool ? "bool" :
      (dh->type) == do_enum ? "enum" :
      (dh->type) == do_64bit ? "64bit" :
      (dh->type) == do_fixed64 ? "fixed64" :
      (dh->type) == do_sfixed64 ? "sfixed64" :
      (dh->type) == do_double ? "double" :
      (dh->type) == do_node ? "node" :
      (dh->type) == do_string ? "string" :
      (dh->type) == do_data ? "data" :
      (dh->type) == do_32bit ? "32bit" :
      (dh->type) == do_fixed32 ? "fixed32" :
      (dh->type) == do_sfixed32 ? "sfixed32" :
      (dh->type) == do_float ? "float" : "????") ;

    if (!dh->child) {

      printf(" %ld", dh->d1) ;

      if (dh->d2) {
        printf(" - ") ;
        for (int i=0; i<dh->d1 && i<32; i++) {
          if (dh->d2[i]>=' ' && dh->d2[i]<=127) printf("%c", dh->d2[i]) ;
          else printf(".") ;
        }
      }

    }

    printf("\n") ;

    if (dh->child) {
      _do_dump(dh->child, depth+1) ;
    }

    dh=dh->next ;
  }
}


