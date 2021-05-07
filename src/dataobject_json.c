//
// dataobject_json.c
//
// JSON Conversion functions
//
//

// TODO json decide turns an integer of 2 into 4 - sets wrong type or gets wrong type
// integers should be stored as signed and retrieved as signed

// TODO json 'null' needs to be stored as do_string with a null pointer
// this needs to be handled in both to and from json
// Also, the protobuf output needs to handle this too (probably a 0 length)
// And also the dodump


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "dataobject_private.h"
#include "../dataobject.h"

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// Internal Functions
//

char * _do_asjson_start(IDATAOBJECT *dh, int *len, int isarray) ;
int _do_fromjson_start(IDATAOBJECT *root, IDATAOBJECT *dh, char *json) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Output data as a JSON string
// @param[in] dh Data object handle
// @param[out] len Pointer to JSON length (NULL if not required)
// @return JSON data string or NULL if error
//

char * doasjson(IDATAOBJECT *dh, int *len)
{
  if (!dh) {
    fprintf(stderr, "doasjson: called with NULL handle\n") ;
    return 0 ;
  }
 
  _do_cleartmp(dh) ;
  _do_appendtmp(dh, "{", 1) ;
  if (dh->label) _do_asjson_start(dh, len, 0) ;
  _do_appendtmp(dh, "}", 1) ;

  return dh->tmpbuf ;
}


///////////////////////////////////////////////////////////
//
// @brief Import data from JSON and places in dh object
// @param[in] dh Data object handle
// @param[in] json NULL terminated JSON data
// @return True on success, updates dojsonparsestrerror on failure
//


int dofromjson(IDATAOBJECT *dh, char *json, ...) 
{
// TODO: VARARGS
  return _do_fromjson_start(dh, dh, json) ;
}



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Expands a node containing a json string
// @param(out) dh Data object handle
// @param(in) path Path to node
// @return True on success
//

int doexpandfromjson(IDATAOBJECT *root, char *path, ...)
{
  IDATAOBJECT *node = dofindnode(root, path) ;
  if (!node) return 0 ;
  if (node->child) return 0 ;
  if (node->type!=do_data && node->type!=do_string) return 0 ;
  if (!node->d2) return 0 ;
  node->child = donew() ;
  if (!node->child) return 0 ;
  if (!_do_fromjson_start(root, node->child, node->d2)) {
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
//
// @brief Returns JSON Parse error string
// @param(out) dh Data object handle
//

char * dojsonparsestrerror(IDATAOBJECT *dh)
{
  if (!dh) {
    return "Bad Pointer" ;
  } else if (!dh->jsonparsestatus) {
    return "OK" ;
  } else {
    return dh->jsonparsestatus ;
  } 
}




///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// Internal _do_asjson function
//

char * _do_asjson_start(IDATAOBJECT *dh, int *len, int isarray)
{

  IDATAOBJECT *h = dh ;

  while (h) {

    // Append label

    if (!(isarray)) {
      _do_appendtmp( dh, "\"", 1 ) ;
      if (h->label) _do_appendtmp( dh, h->label, strlen(h->label) ) ;
      _do_appendtmp( dh, "\":", 2 ) ;
    }

    if (h->type==do_node && !h->isarray) {

      // Recurse / append {child}

      _do_appendtmp( dh, "{", 1 ) ;

      if (h->child) {
        int len ;
        _do_cleartmp(h->child) ;
        char *sub = _do_asjson_start(h->child, &len, 0) ;
        if (len>=0) { 
          _do_appendtmp( dh, sub, len ) ; 
        }
        _do_cleartmp(h->child) ;
      }

      _do_appendtmp( dh, "}", 1 ) ;
      _do_cleartmp(h->child) ;

    } else if (h->type==do_node && h->isarray) {

      // Recurse / append [child]

      _do_appendtmp( dh, "[", 1 ) ;

      if (h->child) {
        int len ;
        _do_cleartmp(h->child) ;
        char *sub = _do_asjson_start(h->child, &len, 1) ;
        if (len>=0) { 
          _do_appendtmp( dh, sub, len ) ; 
        }
        _do_cleartmp(h->child) ;
      }

      _do_appendtmp( dh, "]", 1 ) ;

    } else {

      // Append data

      char num[32] ;

      switch (h->type) {

        case do_64bit:
        case do_32bit:
        case do_enum:
        case do_uint32:
        case do_uint64:
        case do_fixed64:
        case do_fixed32:
        case do_int32: 
        case do_int64:

          sprintf(num, "%lu", h->d1) ;
          _do_appendtmp( dh, num, strlen(num) ) ;
          break ;

        case do_sint32:
        case do_sfixed32:
        case do_sint64:
        case do_sfixed64:

          sprintf(num, "%ld", _do_signeddecode(h->d1)) ;
          _do_appendtmp( dh, num, strlen(num) ) ;
          break ;

        case do_string:
        case do_data:    

          if (!h->d2) {

            _do_appendtmp(dh, "null",4) ;

          } else {

            _do_appendtmp( dh, "\"", 1 ) ;

            for (int i=0; i<h->d1; i++) {

              if (h->d2[i]=='\"') {
                _do_appendtmp( dh, "\\\"", 2) ;

              } else if (h->d2[i]=='\\') {
                _do_appendtmp( dh, "\\\\", 2) ;

              } else if (h->d2[i]=='\n') {
                _do_appendtmp( dh, "\\n", 2) ;

              } else if (h->d2[i]=='\r') {
                _do_appendtmp( dh, "\\r", 2) ;

              } else if (h->d2[i]=='\t') {
                _do_appendtmp( dh, "\\t", 2) ;

              } else if (h->d2[i]=='\'') {
                _do_appendtmp( dh, "\\u0027", 2) ;

              } else if (h->d2[i]<32) {
                char escaped[32] ;
                snprintf(escaped, sizeof(escaped)-1, "\\u%04X", h->d2[i]) ;
                _do_appendtmp( dh, "\\u", 2) ;

              } else {
              _do_appendtmp( dh, &(h->d2[i]), 1) ;
              }
            }
            _do_appendtmp( dh, "\"", 1 ) ;

          }

          break ;

        case do_bool:

          if (h->d1) _do_appendtmp( dh, "true", 4 ) ;
          else _do_appendtmp( dh, "false", 5 ) ;
          break ;

        case do_float:

          sprintf(num, "%f", _do_floatdecode(h->d1)) ;
          _do_appendtmp( dh, num, strlen(num) ) ;
          break ;

        case do_double:

          sprintf(num, "%f", _do_doubledecode(h->d1)) ;
          _do_appendtmp( dh, num, strlen(num) ) ;
          break ;

      }
    }

    // Move to next entry in chain

    h = h->next ;
    if (h) _do_appendtmp( dh, ",", 1 ) ;

  }

  // Return 

  if (len) { (*len) = dh->tmpbuflen ; } 
  return dh->tmpbuf ;

}



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// Internal _do_fromjson_start functions
//

int _do_jsonfieldlen(char *str)
{
  int instring ;
  int escape=0 ;
  int len=0 ;
  
  if (*str=='\"') {
    instring=1 ;
    len++ ;
  } else {
    instring=0 ;
  }

  int done=0 ;

  do {

    if (str[len]=='\0') {

      done=1 ;

    } else if (!escape && instring && str[len]=='\"') {

      // Finish string

      len++ ;
      done=1 ;

    } else if (instring) {

      // Skip through string

      len++ ;

    } else if ( !escape && !instring && 
                ( isalnum(str[len]) || str[len]=='.' || 
                  str[len]=='+' || str[len]=='-' ) ) {

      // Step through text or number

      len++ ;

    } else if (escape) {

      // Skip escaped character

      len++ ;

    } else {

      // Anthing else is the end!

      done=1 ;

    }

    escape = (str[len]=='\\') ;


  } while ( !done ) ;

  return len ;
}


int _do_fromjson(IDATAOBJECT *rootroot, IDATAOBJECT *entryroot, char *json, int *pos, int depth, int isarray) 
{
  enum { PARSEOK, BADCHAR, NOLABEL, ERRMALLOC, 
         ARRAYENDEXPECTED, OBJECTENDEXPECTED, ERRORCHILD} parseerror = PARSEOK ;

  IDATAOBJECT *entry = NULL ;
  int commadetected ;
  int count=0 ;

  // maintain depth counter

  if (depth>10) goto fail ;

  // Loop

  do {

    // skip spaces

    while (isspace(json[*pos])) (*pos)++ ;
    if (json[*pos]=='\0' || json[*pos]=='}' || json[*pos]==']') break ;

    // if first node, use the passed object, otherwise
    // create new node, and set to root or next

    if (entry==NULL) {

      entry = entryroot ;
      _do_clear(entry, 0, 1) ;

    } else {

      IDATAOBJECT *newentry = donew() ;
      if (!newentry) {
        parseerror = ERRMALLOC ;
        goto fail ;
      }
      entry->next = newentry ;
      entry = entry->next ;

    }

    if (isarray) {

      // if isarray create label as an array

      char counter[16] ;
      sprintf(counter, "%d", count++) ;
      entry->label = malloc(strlen(counter)+1) ;
      if (!entry->label) {
        parseerror = ERRMALLOC ;
        goto fail ;
      }
      strcpy(entry->label, counter) ;
      entry->type = do_node ;

    } else if (json[*pos]=='\"') {

      // if !isarray fetch label from json

      int labellen = _do_jsonfieldlen(&(json[*pos])) ;
      entry->label = malloc(labellen-1) ;
      if (!entry->label) {
        parseerror = ERRMALLOC ;
        goto fail ;
      }
      strncpy(entry->label, &(json[(*pos)+1]), labellen-2) ;
      entry->label[labellen-2]='\0' ;
      (*pos)+=labellen ;
      entry->type = do_node ;
      entry->isarray = isarray ;

    } else {

      parseerror = NOLABEL ;
      goto fail ;

    }

    // skip white spaces or colons

    while (isspace(json[*pos]) || json[*pos]==':') (*pos)++ ;


    // Check character

    char ch = json[*pos] ;

    //  if { recurse and store results in child
    //  if [ recurse array and store results in child

    if (ch=='{' || ch=='[') {

      (*pos)++ ;

// TODO: change so that entry->isarray goes, and do_nodearray used

      entry->type = do_node ;
      entry->child = donew() ;
      entry->isarray = (ch=='[') ;

      _do_fromjson(rootroot, entry->child, json, pos, depth+1, entry->isarray) ;

      if (!entry->child->label) {
        // No data was filled in to child
        dodelete(entry->child) ;
        entry->child=NULL ;
      }

      if ((*pos)<0) {
        // Error message should have been set in child
        // e.g. missing ] or } termination
        parseerror = ERRORCHILD ;
        goto fail ;
      }

      (*pos)++ ;

    }


    // else capture record data and update position

    else {

        int datalen = _do_jsonfieldlen(&(json[*pos])) ;

        if (json[*pos]=='\"') {

          // "string" -> store string\0 and length=6

// TODO: parse escaped characters and \u unicode sequences
// Need a de-escape function which has 2 passes (the first just calculates
// the length, and the second actually places the data

          entry->d1 = 0 ;
          entry->d2 = malloc(datalen-1) ; // string\0
          entry->type = do_data ;
          if (!entry->d2) {
            parseerror = ERRMALLOC ;
            goto fail ;
          }
          entry->d1 = (datalen-2) ;  // string without quotes
          if (datalen>2) {
            // Copy data for non-empty strings
            strncpy(entry->d2, &(json[(*pos)+1]), datalen-2) ;
          }
          entry->d2[datalen-2]='\0' ;

        } else if (json[*pos]=='n' || json[*pos]=='N') {

          // null string
          // entry->d1 remains as 0 ;
          // entry->d2 remains NULL ;
          entry->type = do_string ;


        } else if (json[*pos]=='t' || json[*pos]=='T') {

          // boolean
          entry->d1 = 1 ;
          entry->type = do_bool ;

        } else if (json[*pos]=='f' || json[*pos]=='F') {

          // boolean
          entry->d1 = 0 ;
          entry->type = do_bool ;

        } else if (isdigit(json[*pos]) || json[*pos]=='+' || json[*pos]=='-' || json[*pos]=='.') {

          // number

          int isfloat=0 ;

          int l=0 ;
          while (isdigit(json[(*pos)+l]) || json[(*pos)+l]=='+' || json[(*pos)+l]=='-' || json[(*pos)+l]=='.') { 
            isfloat |= json[(*pos)+l]=='.' ;
            l++ ; 
          }

          if (isfloat) {

            // float 

            double d ;
            sscanf(&json[(*pos)], "%lf", &d) ;
            entry->d1 = _do_doubleencode(d) ;
            entry->type = do_double ;

          } else {

            // Signed int

            signed long int j = atoi(&json[*pos]) ;
            entry->d1 = _do_signedencode(j) ;
            entry->type = do_sint64 ;

          }

        } else {

          parseerror=BADCHAR ;
          goto fail ;

        }

        (*pos)+=datalen ;

    }

    // Skip to end of record ( treat ,, ,\n, as , )

    commadetected=0 ;
    while (isspace(json[*pos]) || json[*pos]==',') {
      if (json[*pos]==',') commadetected=1 ;
      (*pos)++ ;
    }

  } while (commadetected) ;

  if (isarray && json[(*pos)]!=']') {
    parseerror=ARRAYENDEXPECTED ;
    goto fail ;
  }

  if (!isarray && json[(*pos)]!='}') {
    parseerror=OBJECTENDEXPECTED ;
    goto fail ;
  }

fail:

  if (parseerror==PARSEOK) {

    // Clear error message

    if (rootroot->jsonparsestatus) free(rootroot->jsonparsestatus) ;
    rootroot->jsonparsestatus=NULL ;
    return 1 ;

  } else if (parseerror==ERRORCHILD) {

    // Leave error message unadulterated

    return 0 ;

  } else {

    // Set parse error message

    char errormessage[256] ;

    if (rootroot->jsonparsestatus) free(rootroot->jsonparsestatus) ;
    rootroot->jsonparsestatus=NULL ;

    snprintf(errormessage, sizeof(errormessage)-11, 
           "%s at character %d, found : ",
           (parseerror==NOLABEL) ? "Missing Label" :
           (parseerror==BADCHAR) ? "Unexpected Character" :
           (parseerror==ARRAYENDEXPECTED) ? "Expected ]" :
           (parseerror==OBJECTENDEXPECTED) ? "Expected }" :
           (parseerror==ERRMALLOC) ? "Out of Memory" : "?",
           (*pos)) ;

    strncat(errormessage, &json[(*pos)], 10);
    rootroot->jsonparsestatus = malloc(strlen(errormessage)+1) ;
    strcat(errormessage, "...") ;
    if (rootroot->jsonparsestatus) {
      strcpy(rootroot->jsonparsestatus, errormessage) ;
    }

    (*pos)=-1 ;
    _do_clear(entryroot, 0, (entryroot!=rootroot)) ;

    return 0 ;

  }

}


int _do_fromjson_start(IDATAOBJECT *root, IDATAOBJECT *dh, char *json) 
{
  if (!dh) return 0 ;

  int len=0 ;

  while (isspace(*json)) json++ ;
  char ch = (*json) ;
  if (ch=='{' || ch=='[') {
    json++ ;
    _do_fromjson(root, dh, json, &len, 0, (ch=='[') ) ;
  }

  return (len>=0) ;

}




// This function is currently unused

// 
// @brief Removes JSON escape sequences
// @param(in) src - Source data string (does not stop at \0)
// @param(in) srclen - Length of Source string
// @param(out) dst - if not NULL, pointer to store location
// @param(in) maxdst - Max length of dst
// @return Number of bytes that are / would be placed in dst
//

// RFC 7159
// char = unescaped /
//        escape (
//            %x22 /          ; "    quotation mark  U+0022
//            %x5C /          ; \    reverse solidus U+005C
//            %x2F /          ; /    solidus         U+002F
//            %x62 /          ; b    backspace       U+0008
//            %x66 /          ; f    form feed       U+000C
//            %x6E /          ; n    line feed       U+000A
//            %x72 /          ; r    carriage return U+000D
//            %x74 /          ; t    tab             U+0009
//            %x75 4HEXDIG )  ; uXXXX                U+XXXX
//

// TODO: This is very crude, and doesn't handle UTF16 character
// sequences.

#define htoi(h) (h>='0' && h<='9')?(h-'0'):(tolower(h)-'a'+10)

int _do_unescape(char *src, int srclen, char *dst, int maxdst)
{
  int i=0, j=0 ;

  while (i<srclen) {

    if (src[i]!='\\' || i==(srclen-1)) {

      if (dst) { dst[j] = src[i] ; }
      i++ ; j++ ;

    } else {

      switch (src[++i]) {

        case '\\':

          if (dst) { dst[j] = src[i] ; }
          i++ ; j++ ;
          break ;

        case '/':

          if (dst) { dst[j] = '/' ; }
          i++ ; j++ ;
          break ;

        case 'f':

          if (dst) { dst[j] = '\f' ; }
          i++ ; j++ ;
          break ;

        case 'n':

          if (dst) { dst[j] = '\n' ; }
          i++ ; j++ ;
          break ;

        case 'r':

          if (dst) { dst[j] = '\r' ; }
          i++ ; j++ ;
          break ;

        case 't':

          if (dst) { dst[j] = '\t' ; }
          i++ ; j++ ;
          break ;

        case 'u':

          if (i<=(srclen-4)) {
            int ch ;
            ch = htoi(src[i]) * 0x1000 +
                 htoi(src[i+1]) * 0x0100 +
                 htoi(src[i+2]) * 0x0010 +
                 htoi(src[i+3]) ;
            if (dst) { dst[j] = ch ; }
            i+=4 ; j++ ;
            break ;

        default:

            if (dst) {
              dst[j]='\\' ;
              dst[j+1]=src[i] ;
            }
            i++ ; j+=2 ;
            break ;
        }
      }      
    }
  }

  return j ;

}


