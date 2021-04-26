//
// dataobject.h
//
// This function manages a hierarchical dataobject.
//
// This can be used for either protobuf or json structures.
// Note that signed integers and floats are stored in protobuf format
//
// https://developers.google.com/protocol-buffers/docs/reference/cpp
//
// EXAMPLES
//
//  DATAOBJECT *dh = donew() ;
//  dosetuint(dh, do_int32, 5, "/records") ;
//  dosetuint(dh, do_int64, 1234, "/integers/sequence") ;
//  dosetuint(dh, do_int64, 4321, "/integers/reverse") ;
//  dosetbool(dh, do_bool, 1, "/boolean/affirmative") ;
//  dosetuint(dh, do_uint32, 1, "/array/*/first") ;
//  dosetsint(dh, do_sint64, -2, "/array/*/second") ;
//  dosetreal(dh, do_double, 0.12, "/real/number") ;
//  dosetdata(dh, do_string, "String", 7, "/data/string") ;
//  printf("%s\n", doasjson(dh)) ;
//  dodelete(dh) ;
//
//  DATAOBJECT *ph = donew() ;
//  dofromprotobuf(ph, (char *)protobuf_data, (int)protobuf_len) ;
//  int version = dogetuint(ph, "/f1") ;
//  char *receiver = dogetdata(ph, do_string, "/f2", NULL) ;
//  char *sender = dogetdata(ph, do_string, "/f3", NULL) ;
//  char *namespace = dogetdata([h, do_string, "/f4", NULL) ;
//  char *data = dogetdata(ph, do_data, "/f5", &len) ;
//  ....
//  dodelete(ph) ;
//
//
// DATA SET METHOD    DATA READ METHOD    COMMENTS
//
// dosetuint, ...     dogetuint, ...      No special requirements
//
// dosetuint, ...     doasjson            No special requirements
//                                        provided type is set for
//                                        each parameter.
//
// dosetuint, ...     doasprotobuf        No special requirements
//                                        provided type is set for
//                                        each parameter.
//
// dofromjson         dogetsint, ...      No special requirements
//                                        Note that data is imported
//                                        as either do_sint64, do_bool,
//                                        do_string, or do_double
// 
// dofromjson         doasjson            No special requirements
//
// dofromjson         doasprotobuf        Requires labels to be in the
//                                        form 'fXXXX' where XXXX is the
//                                        number which will be used for
//                                        the protobuf field_number.
//
// dofromprotobuf     dogetuint, ...      Note that data is imported
//                                        as either do_fixed64 or 
//                                        do_data irrespective of the
//                                        encoding. The type parameter 
//                                        in the doget... function is
//                                        used to decode.
//
// dofromprotobuf     doasprotobuf        No special requirements
//
// dofromprotobuf     doasjson            Note that data is imported
//                                        as either do_fixed64 or 
//                                        do_data.
//                                        As the types are not known
//                                        it is necessary to assign
//                                        the types for each data 
//                                        item by calling dosettype
//                                        before doasjson can be used.
//                                        Finally, as the import cannot
//                                        distinguish between a do_data
//                                        which contains data or an 
//                                        embedded structure, it is
//                                        necessary to call doget* on
//                                        the child data to force the 
//                                        decode to happen before calling
//                                        doasjson if any hierarchy 
//                                        which needs expanding.
//
// dofromini          dogetsint, ...      No special requirements
//                                        Note that data is imported
//                                        as either do_sint64, do_bool,
//                                        do_string, or do_double
// 
// dofromini          doasjson            No special requirements
//

#ifndef _DATAOBJECT_DEFINED
#define _DATAOBJECT_DEFINED

#ifndef DATAOBJECT
typedef struct {
} DATAOBJECT ;
#endif

enum dataobject_type {
  do_int32, do_int64, do_uint32, do_uint64, do_sint32, do_sint64, do_bool, do_enum,
  do_64bit, do_fixed64, do_sfixed64, do_double,
  do_string, do_data,
  do_32bit, do_fixed32, do_sfixed32, do_float,
  do_node, do_unknown
} ;


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//
// GENERAL FUNCTIONS
//

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Creates a data object
// @return pointer to DATAOBJECT, or NULL on error (errno set)
//

DATAOBJECT *donew() ;



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Clears dataobject structure
// @param(in) pointer to DATAOBJECT
// @return True on success
//

int doclear(DATAOBJECT *dh) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Delete and free a dataobject
// @param(in) dh DATAOBJECT handle
// @return True on success
//

int dodelete(DATAOBJECT *dh) ;



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents to be an unsigned int
// @param(in) dh DATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Unsigned int / boolean / enumeration to save
// @param(in) path Path to item
// @return True on success
//

int dosetuint(DATAOBJECT *dh, enum dataobject_type type, unsigned long int data, char *path) ;



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents to be an signed int
// @param(in) dh DATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Signed int to save
// @param(in) path Path to item
// @return True on success
//

int dosetsint(DATAOBJECT *dh, enum dataobject_type type, signed long int data, char *path) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents
// @param(in) dh DATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Pointer to data source
// @param(in) datalen Length of data to store
// @param(in) path Path to item
// @return True on success
//

int dosetdata(DATAOBJECT *dh, enum dataobject_type type, char *data, int datalen, char *path) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents to be a real number
// @param(in) dh DATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data float or double to save
// @param(in) path Path to item
// @return True on success
//

int dosetreal(DATAOBJECT *dh, enum dataobject_type type, double data, char *path) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the type of the requested item (does not affect underlying data)
// @param(in) dh DATAOBJECT handle
// @param(in) type Record type
// @param(in) path Path to item
// @return True on success
//

int dosettype(DATAOBJECT *dh, enum dataobject_type type, char *path) ;



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the dataobject contents to be an unsigned int
// @param(in) dh DATAOBJECT handle
// @param(in) type Record type
// @param(out) n Pointer to location to store results
// @param(in) path Path to item
// @return True on success
//

int dogetuint(DATAOBJECT *dh, enum dataobject_type type, unsigned long int *n, char *path) ;



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the dataobject contents to be an signed int
// @param(in) dh DATAOBJECT handle
// @param(in) type Record type
// @param(out) n Pointer to location to store results
// @param(in) path Path to item
// @return True on success
//

long int dogetsint(DATAOBJECT *dh, enum dataobject_type type,  long int *n, char *path) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the dataobject contents
// @param(in) dh DATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Pointer to data source
// @param(in) datalen Length of data to store
// @param(in) path Path to item
// @return Pointer to data or NULL on error or not found
//

char * dogetdata(DATAOBJECT *dh, enum dataobject_type type, int *datalen, char *path) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Sets the dataobject contents to be a real number
// @param(in) dh DATAOBJECT handle
// @param(in) type Record type: 
// @param(in) data Pointer to double to store result
// @param(in) path Path to item
// @return True on success
//

int dogetreal(DATAOBJECT *dh, enum dataobject_type type, double *data, char *path) ;



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Get the type of the requested item 
// @param(in) dh DATAOBJECT handle
// @param(in) path Path to item
// @return Record type
//

enum dataobject_type dogettype(DATAOBJECT *dh, char *path) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Renames the identified node
// @param(in) dh DATAOBJECT handler
// @param(in) path Path in dh to get
// @param(in) newname Name of new node
// @return true on success

int dorenamenode(DATAOBJECT *dh, char *path, char *newname) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the handle of a node at the given path, creates as reqired
// @param(in) dh DATAOBJECT handler
// @param(in) path Path in dh to get
// @return handle of new node created

DATAOBJECT * dogetnode(DATAOBJECT *dh, char *path) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Get handle of a node at the given path
// @param(in) root DATAOBJECT handle
// @param(in) path Path to item
// @return Pointer to data object or NULL if not found
//

DATAOBJECT * dofindnode(DATAOBJECT *root, char *path) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Gets the handle of a node's child
// @param(in) dh DATAOBJECT handler
// @return handle of new node created

DATAOBJECT * dochild(DATAOBJECT *dh) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Get pointer to record within root structure
// @param(in) root DATAOBJECT handle
// @param(in) path Path to item
// @return Pointer to data object or NULL if not found
//

DATAOBJECT *dosearchrecord(DATAOBJECT *root, char *path) ;



///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Paste a copy of the object into root structure
// @param(in) root DATAOBJECT handle for destination
// @param(in) path Path from root to store pasteptr
// @param(in) pasteptr pointer to DATAOBJECT to paste into root
// @param(in) merge If true, merge data where possible
// @return True on success
//

int dopastecopy(DATAOBJECT *root, char *path, DATAOBJECT *pasteptr, int merge) ;



//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//
// JSON FUNCTIONS
//


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Output data as a JSON string
// @param[in] dh Data object handle
// @param[out] len Length of JSON data produced
// @return JSON data string or NULL if error
//

char * doasjson(DATAOBJECT *dh, int *len) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Import data from JSON and places in dh object
// @param[in] dh Data object handle
// @param[in] json NULL terminated JSON data
// @return True on success, updates dojsonparsestrerror on failure
//

int dofromjson(DATAOBJECT *dh, char *json)  ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Expands a node containing a json string
// @param(out) dh Data object handle
// @param(in) path Path to node
// @return True on success, updates dojsonparsestrerror on failure
//

int doexpandfromjson(DATAOBJECT *root, char *path) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Returns JSON Parse error strin
// @param(out) dh Data object handle
// @return Parse error message 

char * dojsonparsestrerror(DATAOBJECT *dh) ;


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//
// PROTOBUF FUNCTIONS
//


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Output data as a Protobuf string
// @param[in] dh Data object handle
// @param[out] len Length of Protobuf data produced
// @return Protobuf data string or NULL if error
//


char * doasprotobuf(DATAOBJECT *dh, int *len) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Builds data object from protobuf source
// @param[in] dh Data object handle
// @param[out] buflen Length of Protobuf data
// @return true on success
//

int dofromprotobuf(DATAOBJECT *dh, char *protobuf, int buflen) ;


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Expands a node containing protobuf data
// @param(out) dh Data object handle
// @param(in) path Path to node
// @return True on success
//

int doexpandfromprotobuf(DATAOBJECT *root, char *path) ;


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//
// DIAGNOSTIC FUNCTIONS
//


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
// @brief Dumps data structure output to stdout
// @param[in] dh Data object handle
// @return nothing
//

void dodump(DATAOBJECT *dh, char *title) ;

#endif

