#ifndef TJSON_CUSTOM_TRIPLE_NOTATION_H
#define TJSON_CUSTOM_TRIPLE_NOTATION_H

int tjson_CustomToTyped(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr);
int tjson_TypedToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr);

#endif //TJSON_CUSTOM_TRIPLE_NOTATION_H
