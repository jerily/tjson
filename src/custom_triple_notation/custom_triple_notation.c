#include <tcl.h>
#include <string.h>
#include "custom_triple_notation.h"

int tjson_CustomToTyped(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr);

static int tjson_CustomConvertTypeValueToTyped(Tcl_Interp *interp, Tcl_Obj *typePtr, Tcl_Obj *valuePtr, Tcl_Obj **resultPtr) {
    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    switch (type[0]) {
        case 's':
            if (type_length == 6 && 0 == strcmp("string", type)) {
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("S", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_DuplicateObj(valuePtr));
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'i':
            if ((type_length == 3 && 0 == strcmp("int", type))
                || (type_length == 5 && 0 == strcmp("int32", type))
                || (type_length == 5 && 0 == strcmp("int64", type))) {
                int int_value;
                if (TCL_OK != Tcl_GetIntFromObj(interp, valuePtr, &int_value)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid int", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("N", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewIntObj(int_value));
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'b':
            if (type_length == 7 && 0 == strcmp("boolean", type)) {
                int flag;
                if (TCL_OK != Tcl_GetBooleanFromObj(interp, valuePtr, &flag)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid boolean", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("BOOL", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewBooleanObj(flag));
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'd':
            if (type_length == 6 && 0 == strcmp("double", type)) {
                double double_value;
                if (TCL_OK != Tcl_GetDoubleFromObj(interp, valuePtr, &double_value)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid double", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("N", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewDoubleObj(double_value));
                *resultPtr = subListPtr;
            } else if (type_length == 8 && 0 == strcmp("document", type)) {

                Tcl_Obj *convertedPtr;
                if (TCL_OK != tjson_CustomToTyped(interp, valuePtr, &convertedPtr)) {
                    return TCL_ERROR;
                }
                *resultPtr = convertedPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'a':
            if (type_length == 5 && 0 == strcmp("array", type)) {
                int arrObjc;
                Tcl_Obj **arrObjv;
                if (TCL_OK != Tcl_ListObjGetElements(interp, valuePtr, &arrObjc, &arrObjv) || (arrObjc % 3 != 0)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid array triple notation spec", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *elemListPtr = Tcl_NewListObj(0, NULL);
                for (int j = 0; j < arrObjc; j+=3) {
                    Tcl_Obj *elemNamePtr = arrObjv[j];
                    Tcl_Obj *elemTypePtr = arrObjv[j+1];
                    Tcl_Obj *elemValuePtr = arrObjv[j+2];

                    int elemName_index;
                    if (TCL_OK != Tcl_GetIntFromObj(interp, elemNamePtr, &elemName_index) || (elemName_index != j/3)) {
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid array index", -1));
                        return TCL_ERROR;
                    }

                    Tcl_Obj *convertedPtr;
                    if (TCL_OK != tjson_CustomConvertTypeValueToTyped(interp, elemTypePtr, elemValuePtr, &convertedPtr)) {
                        return TCL_ERROR;
                    }

                    Tcl_ListObjAppendElement(interp, elemListPtr, convertedPtr);

                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("L", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, elemListPtr);
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        default:
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type in spec", -1));
            return TCL_ERROR;  /* invalid type */
    }

    return TCL_OK;
}

int tjson_CustomToTyped(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {

    // The triple notation is a flat list containing triples of the form
    //
    //    NAME TYPE VALUE
    //
    //where "TYPE" might have the following values:
    //
    //  "array",
    //  "binary",
    //  "boolean",
    //  "int32",
    //  "int64",
    //  "datetime",
    //  "decimal128",
    //  "document",
    //  "double",
    //  "minkey",
    //  "maxkey",
    //  "null",
    //  "oid",
    //  "regex",
    //  "string",
    //  "timestamp",
    //  "unknown"

    int objc;
    Tcl_Obj **objv;
    if (TCL_OK != Tcl_ListObjGetElements(interp, specPtr, &objc, &objv) || (objc % 3 != 0)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid triple notation spec", -1));
        return TCL_ERROR;  /* invalid spec length */
    }

    Tcl_Obj *dictPtr = Tcl_NewDictObj();
    for (int i = 0; i < objc; i+=3) {
        Tcl_Obj *namePtr = objv[i];
        Tcl_Obj *typePtr = objv[i+1];
        Tcl_Obj *valuePtr = objv[i+2];

        Tcl_Obj *convertedPtr;
        if (TCL_OK != tjson_CustomConvertTypeValueToTyped(interp, typePtr, valuePtr, &convertedPtr)) {
            return TCL_ERROR;
        }
        Tcl_DictObjPut(interp, dictPtr, namePtr, convertedPtr);
    }
    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("M", -1));
    Tcl_ListObjAppendElement(interp, listPtr, dictPtr);
    *resultPtr = listPtr;
    return TCL_OK;
}

static int tjson_TypedConvertTypeValueToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // "specPtr" must be a list of two elements, the first of which must be the "type" and the second the "value"
    int length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &length) || length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, must be a list of two elements", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from list", -1));
        return TCL_ERROR;
    }

    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    switch(type[0]) {
        case 'S':
            if (type_length == 1) {
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("string", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_DuplicateObj(valuePtr));
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'N':
            if (type_length == 1) {
                int int_value;
                Tcl_Obj *numTypePtr = NULL;
                Tcl_Obj *numValuePtr = NULL;
                if (TCL_OK != Tcl_GetIntFromObj(interp, valuePtr, &int_value)) {
                    double double_value;
                    if (TCL_OK != Tcl_GetDoubleFromObj(interp, valuePtr, &double_value)) {
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid double", -1));
                        return TCL_ERROR;
                    } else {
                        numTypePtr = Tcl_NewStringObj("double", -1);
                        numValuePtr = Tcl_NewDoubleObj(double_value);
                    }
                } else {
                    numTypePtr = Tcl_NewStringObj("int32", -1);
                    numValuePtr = Tcl_NewIntObj(int_value);
                }
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, numTypePtr);
                Tcl_ListObjAppendElement(interp, listPtr, numValuePtr);
                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'B':
            if (type_length == 4 && 0 == strcmp("BOOL", type)) {
                int flag;
                if (TCL_OK != Tcl_GetBooleanFromObj(interp, valuePtr, &flag)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid boolean", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("boolean", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewBooleanObj(flag));
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'L':
            if (type_length == 1) {
                int arrObjc;
                Tcl_Obj **arrObjv;
                if (TCL_OK != Tcl_ListObjGetElements(interp, valuePtr, &arrObjc, &arrObjv)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid array triple notation spec", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *elemListPtr = Tcl_NewListObj(0, NULL);
                for (int j = 0; j < arrObjc; j++) {
                    Tcl_Obj *elemValuePtr = arrObjv[j];

                    Tcl_Obj *convertedPtr;
                    if (TCL_OK != tjson_TypedConvertTypeValueToCustom(interp, elemValuePtr, &convertedPtr)) {
                        return TCL_ERROR;
                    }

                    Tcl_Obj *convertedTypePtr;
                    Tcl_Obj *convertedValuePtr;
                    if (TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 0, &convertedTypePtr)
                        || TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 1, &convertedValuePtr)) {
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from converted", -1));
                        return TCL_ERROR;
                    }

                    Tcl_ListObjAppendElement(interp, elemListPtr, Tcl_NewIntObj(j));
                    Tcl_ListObjAppendElement(interp, elemListPtr, convertedTypePtr);
                    Tcl_ListObjAppendElement(interp, elemListPtr, convertedValuePtr);
                }
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("array", -1));
                Tcl_ListObjAppendElement(interp, listPtr, elemListPtr);
                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'M':
            if (type_length == 1) {
                // "valuePtr" is a dict
                Tcl_DictSearch search;
                Tcl_Obj *dictKeyPtr;
                Tcl_Obj *dictValuePtr;
                int done;
                if (TCL_OK != Tcl_DictObjFirst(interp, valuePtr, &search, &dictKeyPtr, &dictValuePtr, &done)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, first from dict", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                for (; !done; Tcl_DictObjNext(&search, &dictKeyPtr, &dictValuePtr, &done)) {
                    Tcl_Obj *convertedPtr;
                    if (TCL_OK != tjson_TypedConvertTypeValueToCustom(interp, dictValuePtr, &convertedPtr)) {
                        return TCL_ERROR;
                    }

                    // extract "convertedTypePtr" and "convertedValuePtr" from "convertedPtr"
                    int converted_length;
                    if (TCL_OK != Tcl_ListObjLength(interp, convertedPtr, &converted_length) || converted_length != 2) {
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, converted length != 2", -1));
                        return TCL_ERROR;
                    }

                    Tcl_Obj *convertedTypePtr;
                    Tcl_Obj *convertedValuePtr;
                    if (TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 0, &convertedTypePtr)
                        || TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 1, &convertedValuePtr)) {
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from converted", -1));
                        return TCL_ERROR;
                    }

                    Tcl_ListObjAppendElement(interp, subListPtr, dictKeyPtr);
                    Tcl_ListObjAppendElement(interp, subListPtr, convertedTypePtr);
                    Tcl_ListObjAppendElement(interp, subListPtr, convertedValuePtr);
                }

                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("document", -1));
                Tcl_ListObjAppendElement(interp, listPtr, subListPtr);

                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
    }
    return TCL_OK;
}

int tjson_TypedToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // "specPtr" must be a list with two elements, the first of which must be the type and the second the value
    int length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &length) || length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, must be a list of two elements", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from list", -1));
        return TCL_ERROR;
    }

    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);

    if (type[0] != 'M') {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("no support in triple notation", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);

    // "valuePtr" must be a dict
    Tcl_DictSearch search;
    Tcl_Obj *dictKeyPtr;
    Tcl_Obj *dictValuePtr;
    int done;
    if (TCL_OK != Tcl_DictObjFirst(interp, valuePtr, &search, &dictKeyPtr, &dictValuePtr, &done)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while reading dict", -1));
        return TCL_ERROR;
    }
    for (; !done; Tcl_DictObjNext(&search, &dictKeyPtr, &dictValuePtr, &done)) {
        Tcl_Obj *convertedPtr;
        if (TCL_OK != tjson_TypedConvertTypeValueToCustom(interp, dictValuePtr, &convertedPtr)) {
            return TCL_ERROR;
        }

        // extract "convertedTypePtr" and "convertedValuePtr" from "convertedPtr"
        int converted_length;
        if (TCL_OK != Tcl_ListObjLength(interp, convertedPtr, &converted_length) || converted_length != 2) {
            fprintf(stderr, "converted_length: %d\n", converted_length);
            Tcl_SetObjResult(interp, Tcl_NewStringObj("converted must be a list of two elements", -1));
            return TCL_ERROR;
        }

        Tcl_Obj *convertedTypePtr;
        Tcl_Obj *convertedValuePtr;
        if (TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 0, &convertedTypePtr)
            || TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 1, &convertedValuePtr)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec", -1));
            return TCL_ERROR;
        }

        Tcl_ListObjAppendElement(interp, listPtr, dictKeyPtr);
        Tcl_ListObjAppendElement(interp, listPtr, convertedTypePtr);
        Tcl_ListObjAppendElement(interp, listPtr, convertedValuePtr);
    }

    *resultPtr = listPtr;
    return TCL_OK;

}